/*
 *******************************************************************************
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *******************************************************************************
 */

/*
 * netfn_flow_cookie_db.c
 *     Flow Cookie Functionality File.
 *     Invokes suitable DB calls into the
 *     DB infra and perform the necessary DB operations.
 */

#include <linux/types.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <net/udp.h>
#include <netfn_types.h>
#include <netfn_flow_cookie.h>
#include "netfn_flow_cookie_hash.h"
#include "netfn_flow_cookie_priv.h"

/*
 * netfn_flow_cookie_db_hash_table_alloc()
 *	Allocate the hash table for the database entries.
 */
bool netfn_flow_cookie_db_hash_table_alloc(struct netfn_flow_cookie_db *d, uint32_t num_entries)
{
	struct hlist_head *dhtptr;
	uint32_t i;

	dhtptr = (struct hlist_head *)kzalloc((num_entries * sizeof(struct hlist_head)), GFP_ATOMIC);

	if (!dhtptr) {
		netfn_flow_cookie_warn("%p: Failed to allocate database hash table", d);
		return false;
	}

	/*
	 * We assign the RCU based DB Hash table as created
	 * to the DB handle here.
	 */
	d->db_table = dhtptr;

	/*
	 * We initialize the DB Hash table here.
	 */
	for (i = 0; i < num_entries; i++) {
		INIT_HLIST_HEAD(&d->db_table[i]);
	}

	d->hash_size = num_entries;

	/*
	 * We label the DB instance
	 * with the following Hash flag.
	 */
	d->flags |= NETFN_FLOW_COOKIE_DB_FLAG;

	return true;
}

/*
 * netfn_flow_cookie_db_handle_store
 *	We use the following API to store
 *	the corresponding DB handle into the DB array.
 *
 * @param[in]
 * netfn_flow_cookie_db handle
 *
 * @return
 * Success or Failure
 *
 */
static int netfn_flow_cookie_db_handle_store(struct netfn_flow_cookie_db *d)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint8_t id;

	spin_lock_bh(&nfc_ctx->lock);
	for (id = 0; id < NETFN_FLOW_COOKIE_DB_ARRAY_SIZE; id++) {
		if (rcu_dereference_protected(nfc_ctx->db_array[id], lockdep_is_held(&nfc_ctx->lock)) == NULL) {
			netfn_flow_cookie_info("Found the place to store the DB handle "
					       "at id:%d", id);
			break;
		}
	}

	if (id == NETFN_FLOW_COOKIE_DB_ARRAY_SIZE) {
		netfn_flow_cookie_warn("No more space left to store "
				       "the DB handle for this DB instance");
		spin_unlock_bh(&nfc_ctx->lock);
		return -1;
	}

	/*
	 * Store the DB handle into the netfn_flow_cookie_db array
	 * at the appropriate id available.
	 */
	rcu_assign_pointer(nfc_ctx->db_array[id], d);
	d->id = id;
	netfn_flow_cookie_info("Successfully stored the DB handle in DB array, "
			       "netfn_flow_cookie_db ref:%p, id:%d", d, id);

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DBS_ACTIVE]);
	spin_unlock_bh(&nfc_ctx->lock);

	return 0;
}

/*
 * netfn_flow_cookie_db_handle_remove
 * 	We use the following API to remove
 *      the corresponding DB handle from the DB array.
 *
 * @param[in]
 * netfn_flow_cookie_db handle
 *
 * @return
 * Success or Failure
 *
 */
static int netfn_flow_cookie_db_handle_remove(struct netfn_flow_cookie_db *d)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint8_t id;

	spin_lock_bh(&nfc_ctx->lock);
	for (id = 0; id < NETFN_FLOW_COOKIE_DB_ARRAY_SIZE; id++) {
		if (rcu_dereference_protected(nfc_ctx->db_array[id], lockdep_is_held(&nfc_ctx->lock)) == d) {
			netfn_flow_cookie_info("Found the index where the current DB handle "
				"is stored, id :%d", id);
			break;
		}
	}

	if (id == NETFN_FLOW_COOKIE_DB_ARRAY_SIZE) {
		netfn_flow_cookie_warn("DB handle for this DB instance is not found in the DB array");
		spin_unlock_bh(&nfc_ctx->lock);
		return -1;
	}

	/*
	 * Remove the DB handle from the netfn_flow_cookie_db array
	 * by marking the value to be NULL
	 * at the appropriate id found.
	 */
	rcu_assign_pointer(nfc_ctx->db_array[id], NULL);
	netfn_flow_cookie_info("Successfully removed the DB handle from the DB array, netfn_flow_cookie_db ref:%p", d);

	atomic64_dec(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DBS_ACTIVE]);
	spin_unlock_bh(&nfc_ctx->lock);

	return 0;
}

/*
 * netfn_flow_cookie_db_instance_free()
 * 	Destroy the netfn_flow_cookie_db instance.
 * 	User should ensure that all the DB entries
 * 	are deleted and the references as taken over
 * 	various DB operations are all cleared for this
 * 	operation to go through.
 *
 */
static void netfn_flow_cookie_db_instance_free(struct kref *kref)
{
	struct netfn_flow_cookie_db *d = container_of(kref, struct netfn_flow_cookie_db, ref);
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;

	if (d == NULL) {
		netfn_flow_cookie_warn("Data Base handle already pointing to NULL");
		return;
	}

	if (!(d->flags & NETFN_FLOW_COOKIE_DB_FLAG)) {
		netfn_flow_cookie_warn("%p: Attempt to free a non-local hash table", d);
		return;
	}

	if (!d->db_table) {

		/*
		 * We Free the relevant Memory as taken for the
		 * DB allocation and De-Initialize it's pointer to NULL here
		 * for this uninitialized DB table case.
		 */
		kfree(d);
		d = NULL;
		return;
	}

	/*
	 * Use the following API to reset the DB stats specifically.
	 */
	netfn_flow_cookie_stats_reset_db(d);

	if (netfn_flow_cookie_db_handle_remove(d) == -1) {
		netfn_flow_cookie_warn("Error in removing the DB entry from the DB array, netfn_flow_cookie_db ref:%p", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_FREE_OPS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FREE_OPS_FAILED]);
		return;
	}

	d->flags &= ~NETFN_FLOW_COOKIE_DB_FLAG;

	/*
	 * We Free the relevant Memory as taken for the
	 * DB table allocation and De-Initialize it's pointer to NULL here.
	 */
	kfree(d->db_table);
	d->db_table = NULL;

	/*
	 * We Free the relevant Memory as taken for the
	 * DB allocation and De-Initialize it's pointer to NULL here.
	 */
	kfree(d);
	d = NULL;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_FREE_OPS_SUCCESSFUL]);

	netfn_flow_cookie_info("Done with all the hash table free operations");
	return;
}

/*
 * netfn_flow_cookie_db_instance_ref()
 * 	Increases the references of the netfn_flow_cookie_db instance provided.
 */
void netfn_flow_cookie_db_instance_ref(struct netfn_flow_cookie_db *d)
{
	kref_get(&d->ref);
	return;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_instance_ref);

/*
 * netfn_flow_cookie_db_instance_deref()
 * 	Decreases the references of the netfn_flow_cookie_db instance provided.
 */
bool netfn_flow_cookie_db_instance_deref(struct netfn_flow_cookie_db *d)
{
	if (kref_put(&d->ref, netfn_flow_cookie_db_instance_free)) {
		return true;
	}

	return false;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_instance_deref);

/*
 * netfn_flow_cookie_db_calculate_hash_idx
 *      We use the following API to calculate
 *	the Hash index from the given
 *	5 tuple information.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info
 *
 * @return
 * 32 bit hash index
 *
 */
inline uint32_t netfn_flow_cookie_db_calculate_hash_idx(struct netfn_flow_cookie_db *d, struct netfn_tuple *t)
{
	uint16_t hash_mask = 0;

	hash_mask = d->hash_size - 1;

	if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {
		return netfn_flow_cookie_hash_compute_v4(t->tuples.tuple_5.protocol, t->tuples.tuple_5.src_ip.ip4.s_addr,
				t->tuples.tuple_5.l4_src_ident, t->tuples.tuple_5.dest_ip.ip4.s_addr, t->tuples.tuple_5.l4_dest_ident) & hash_mask;
	} else if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {
		return netfn_flow_cookie_hash_compute_v6(t->tuples.tuple_5.protocol, t->tuples.tuple_5.src_ip.ip6.s6_addr32,
				t->tuples.tuple_5.l4_src_ident, t->tuples.tuple_5.dest_ip.ip6.s6_addr32, t->tuples.tuple_5.l4_dest_ident) & hash_mask;
	}
	return 0;
}

/*
 * netfn_flow_cookie_db_lookup_flow
 * 	We use the following API to perform the lookup
 * 	into the Hash table entries to find the
 * 	appropriate DB entry corresponding to the
 * 	5 tuple information provided and fethes the
 * 	netfn flow structure corresponding to the same.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info, hash index, netfn_flow_cookie_db_entry corresponding to the DB entry.
 *
 * @return
 * void.
 *
 */
static void netfn_flow_cookie_db_lookup_flow(struct netfn_flow_cookie_db *d, struct netfn_tuple *t, uint32_t hash_idx, struct netfn_flow_cookie_db_entry **flow_fetched)
{
	struct netfn_flow_cookie_db_entry *flow;

	if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {

		hlist_for_each_entry_rcu(flow, &(d->db_table[hash_idx]), node) {
			if (likely(flow->tuple.tuples.tuple_5.l4_src_ident == t->tuples.tuple_5.l4_src_ident)
					&& likely(flow->tuple.tuples.tuple_5.l4_dest_ident == t->tuples.tuple_5.l4_dest_ident)
					&& likely(flow->tuple.tuples.tuple_5.src_ip.ip4.s_addr == t->tuples.tuple_5.src_ip.ip4.s_addr)
					&& likely(flow->tuple.tuples.tuple_5.dest_ip.ip4.s_addr == t->tuples.tuple_5.dest_ip.ip4.s_addr)
					&& likely(flow->tuple.tuples.tuple_5.protocol == t->tuples.tuple_5.protocol)
					&& likely(flow->tuple.ip_version == t->ip_version)) {
				*flow_fetched = flow;
				break;
			}
		}

	} else if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {

		hlist_for_each_entry_rcu(flow, &(d->db_table[hash_idx]), node) {
			if (likely(flow->tuple.tuples.tuple_5.l4_src_ident == t->tuples.tuple_5.l4_src_ident)
					&& likely(flow->tuple.tuples.tuple_5.l4_dest_ident == t->tuples.tuple_5.l4_dest_ident)
					&& likely(flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[3] == t->tuples.tuple_5.src_ip.ip6.s6_addr32[3])
					&& likely(flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[2] == t->tuples.tuple_5.src_ip.ip6.s6_addr32[2])
					&& likely(flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[1] == t->tuples.tuple_5.src_ip.ip6.s6_addr32[1])
					&& likely(flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[0] == t->tuples.tuple_5.src_ip.ip6.s6_addr32[0])
					&& likely(flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[3] == t->tuples.tuple_5.dest_ip.ip6.s6_addr32[3])
					&& likely(flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[2] == t->tuples.tuple_5.dest_ip.ip6.s6_addr32[2])
					&& likely(flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[1] == t->tuples.tuple_5.dest_ip.ip6.s6_addr32[1])
					&& likely(flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[0] == t->tuples.tuple_5.dest_ip.ip6.s6_addr32[0])
					&& likely(flow->tuple.tuples.tuple_5.protocol == t->tuples.tuple_5.protocol)
					&& likely(flow->tuple.ip_version == t->ip_version)) {
				*flow_fetched = flow;
				break;
			}
		}
	}

	return;
}

/*
 * _netfn_flow_cookie_db_get_cookie
 *
 *	 We use the following API to fetch the
 *	 appropriate netfn flow cookie for the
 *	 corresponding flow entry if it exists
 *	 in the Hash table DB basing on the
 *	 5 tuple info provided.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info, cookie to be filled.
 *
 * @return
 * Success/Failure.
 *
 */
static netfn_flow_cookie_status_t _netfn_flow_cookie_db_get_cookie(struct netfn_flow_cookie_db *d, struct netfn_tuple *t, struct netfn_flow_cookie *cookie)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint32_t hash_idx;
	struct netfn_flow_cookie_db_entry *flow = NULL;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	hash_idx = netfn_flow_cookie_db_calculate_hash_idx(d, t);

	rcu_read_lock_bh();
	netfn_flow_cookie_db_lookup_flow(d, t, hash_idx, &flow);

	if (!flow) {

		rcu_read_unlock_bh();

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	*cookie = flow->flow_cookie;

	/*
	 * TODO: Plan to optimize these.
	 */
	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_HIT_COUNT_FOR_VARIOUS_FETCH_CALLS]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_HIT_COUNT_FOR_VARIOUS_FETCH_CALLS]);

	atomic64_inc(&flow->Hit_count);
	rcu_read_unlock_bh();

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}

/*
 * netfn_flow_cookie_db_entry_extract_tuple_ipv4
 *	We use the following API to extract the
 *	5 tuple info of the flow from the SKB.
 *
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 * @param[in] t		Pointer to the tuple being filled.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
static netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_extract_tuple_ipv4(struct sk_buff *skb, uint32_t offset, struct netfn_tuple *t)
{
	struct iphdr *iph;
	struct tcphdr *tcph;
	struct udphdr *udph;
	uint32_t l4_hdr_offset;

	/* Extracting the IP Header */
	iph = (struct iphdr *)(skb->data + offset);
	l4_hdr_offset = sizeof(struct iphdr) + offset;

	switch (iph->protocol) {
	case IPPROTO_TCP:

		/* Extracting the TCP Header */
		tcph = (struct tcphdr *)(skb->data + l4_hdr_offset);

		t->ip_version = NETFN_FLOWMGR_TUPLE_IP_VERSION_V4;
		t->tuple_type = NETFN_TUPLE_5TUPLE;
		t->tuples.tuple_5.src_ip.ip4.s_addr = ntohl(iph->saddr);
		t->tuples.tuple_5.dest_ip.ip4.s_addr = ntohl(iph->daddr);
		t->tuples.tuple_5.l4_src_ident = ntohs(tcph->source);
		t->tuples.tuple_5.l4_dest_ident = ntohs(tcph->dest);
		t->tuples.tuple_5.protocol = iph->protocol;

		return NETFN_FLOW_COOKIE_STATUS_SUCCESS;

	case IPPROTO_UDP:

		/* Extracting the UDP header */
		udph = (struct udphdr *)(skb->data + l4_hdr_offset);

		t->ip_version = NETFN_FLOWMGR_TUPLE_IP_VERSION_V4;
		t->tuple_type = NETFN_TUPLE_5TUPLE;
		t->tuples.tuple_5.src_ip.ip4.s_addr = ntohl(iph->saddr);
		t->tuples.tuple_5.dest_ip.ip4.s_addr = ntohl(iph->daddr);
		t->tuples.tuple_5.l4_src_ident = ntohs(udph->source);
		t->tuples.tuple_5.l4_dest_ident = ntohs(udph->dest);
		t->tuples.tuple_5.protocol = iph->protocol;

		return NETFN_FLOW_COOKIE_STATUS_SUCCESS;

	default:
		pr_warn_ratelimited(KERN_INFO "Unknown protocol: %d\n", iph->protocol);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}
}

/*
 * netfn_flow_cookie_db_entry_extract_tuple_ipv6
 *	We use the following API to extract the
 *	5 tuple info of the flow from the SKB.
 *
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 * @param[in] t         Pointer to the tuple being filled.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
static netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_extract_tuple_ipv6(struct sk_buff *skb, uint32_t offset, struct netfn_tuple *t)
{
	struct ipv6hdr *ip6h;
	struct tcphdr *tcph;
	struct udphdr *udph;
	uint32_t l4_hdr_offset;

	/* Extracting the IPV6 Header */
	ip6h = (struct ipv6hdr *)(skb->data + offset);
	l4_hdr_offset = sizeof(struct ipv6hdr) + offset;

	switch (ip6h->nexthdr) {
	case IPPROTO_TCP:

		/* Extracting the TCP Header */
		tcph = (struct tcphdr *)(skb->data + l4_hdr_offset);

		t->ip_version = NETFN_FLOWMGR_TUPLE_IP_VERSION_V6;
		t->tuple_type = NETFN_TUPLE_5TUPLE;
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[0] = ntohl(ip6h->saddr.s6_addr32[0]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[1] = ntohl(ip6h->saddr.s6_addr32[1]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[2] = ntohl(ip6h->saddr.s6_addr32[2]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[3] = ntohl(ip6h->saddr.s6_addr32[3]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[0] = ntohl(ip6h->daddr.s6_addr32[0]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[1] = ntohl(ip6h->daddr.s6_addr32[1]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[2] = ntohl(ip6h->daddr.s6_addr32[2]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[3] = ntohl(ip6h->daddr.s6_addr32[3]);
		t->tuples.tuple_5.l4_src_ident = ntohs(tcph->source);
		t->tuples.tuple_5.l4_dest_ident = ntohs(tcph->dest);
		t->tuples.tuple_5.protocol = ip6h->nexthdr;

		return NETFN_FLOW_COOKIE_STATUS_SUCCESS;

	case IPPROTO_UDP:

		/* Extracting the UDP header */
		udph = (struct udphdr *)(skb->data + l4_hdr_offset);

		t->ip_version = NETFN_FLOWMGR_TUPLE_IP_VERSION_V6;
		t->tuple_type = NETFN_TUPLE_5TUPLE;
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[0] = ntohl(ip6h->saddr.s6_addr32[0]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[1] = ntohl(ip6h->saddr.s6_addr32[1]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[2] = ntohl(ip6h->saddr.s6_addr32[2]);
		t->tuples.tuple_5.src_ip.ip6.s6_addr32[3] = ntohl(ip6h->saddr.s6_addr32[3]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[0] = ntohl(ip6h->daddr.s6_addr32[0]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[1] = ntohl(ip6h->daddr.s6_addr32[1]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[2] = ntohl(ip6h->daddr.s6_addr32[2]);
		t->tuples.tuple_5.dest_ip.ip6.s6_addr32[3] = ntohl(ip6h->daddr.s6_addr32[3]);
		t->tuples.tuple_5.l4_src_ident = ntohs(udph->source);
		t->tuples.tuple_5.l4_dest_ident = ntohs(udph->dest);
		t->tuples.tuple_5.protocol = ip6h->nexthdr;

		return NETFN_FLOW_COOKIE_STATUS_SUCCESS;

	default:
		pr_warn_ratelimited(KERN_INFO "Unknown protocol: %d\n", ip6h->nexthdr);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}
}

/*
 * netfn_flow_cookie_db_entry_add
 *	We use the following API to add the
 *	appropriate DB entry corresponding to the
 *	5 tuple info of the flow into the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info, flow cookie info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_add(struct netfn_flow_cookie_db *d, struct netfn_tuple *t, struct netfn_flow_cookie *cookie)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint32_t hash_idx;
	struct netfn_flow_cookie_db_entry *flow_found = NULL;
	struct netfn_flow_cookie_db_entry *flow = NULL;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	spin_lock_bh(&nfc_ctx->lock);

	hash_idx = netfn_flow_cookie_db_calculate_hash_idx(d, t);

	rcu_read_lock_bh();
	netfn_flow_cookie_db_lookup_flow(d, t, hash_idx, &flow_found);
	rcu_read_unlock_bh();

	/*
	 * Check if the Flow already exists in the DB.
	 * If yes, simply return to avoid further operations.
	 */
	if (flow_found) {
		netfn_flow_cookie_info("%p: Flow rule already exists in the DB", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_ADD_OPS_FAILED_DUE_TO_OTHER_REASONS]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_ADD_OPS_FAILED_DUE_TO_OTHER_REASONS]);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_ADD_OPS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_ADD_OPS_FAILED]);

		spin_unlock_bh(&nfc_ctx->lock);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	flow = (struct netfn_flow_cookie_db_entry *)kzalloc(sizeof(struct netfn_flow_cookie_db_entry), GFP_ATOMIC);
	if (!flow) {
		netfn_flow_cookie_warn("%p: Failed to allocate a flow entry", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_ADD_OPS_FAILED_DUE_TO_UNAVAILABLE_MEMORY]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_ADD_OPS_FAILED_DUE_TO_UNAVAILABLE_MEMORY]);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_ADD_OPS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_ADD_OPS_FAILED]);

		spin_unlock_bh(&nfc_ctx->lock);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	flow->tuple.tuples.tuple_5.l4_src_ident = t->tuples.tuple_5.l4_src_ident;
	flow->tuple.tuples.tuple_5.l4_dest_ident = t->tuples.tuple_5.l4_dest_ident;
	flow->tuple.tuples.tuple_5.protocol = t->tuples.tuple_5.protocol;
	flow->tuple.ip_version = t->ip_version;
	flow->flow_cookie.valid_flag = cookie->valid_flag;
	flow->flow_cookie.flow_id = cookie->flow_id;
	flow->flow_cookie.sawf_handle = cookie->sawf_handle;
	flow->flow_cookie.flow_mark = cookie->flow_mark;
	flow->flow_cookie.scs_id = cookie->scs_id;
	atomic64_set(&flow->Hit_count, 0);

	if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {

		flow->tuple.tuples.tuple_5.src_ip.ip4.s_addr = t->tuples.tuple_5.src_ip.ip4.s_addr;
		flow->tuple.tuples.tuple_5.dest_ip.ip4.s_addr = t->tuples.tuple_5.dest_ip.ip4.s_addr;
	} else if (t->ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {

		flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[0] = t->tuples.tuple_5.src_ip.ip6.s6_addr32[0];
		flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[1] = t->tuples.tuple_5.src_ip.ip6.s6_addr32[1];
		flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[2] = t->tuples.tuple_5.src_ip.ip6.s6_addr32[2];
		flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[3] = t->tuples.tuple_5.src_ip.ip6.s6_addr32[3];
		flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[0] = t->tuples.tuple_5.dest_ip.ip6.s6_addr32[0];
		flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[1] = t->tuples.tuple_5.dest_ip.ip6.s6_addr32[1];
		flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[2] = t->tuples.tuple_5.dest_ip.ip6.s6_addr32[2];
		flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[3] = t->tuples.tuple_5.dest_ip.ip6.s6_addr32[3];
	}

	/*
	 * We insert the DB entry into the
	 * RCU based Hash List here using the
	 * following API.
	 */
	hlist_add_head_rcu(&(flow->node), &(d->db_table[hash_idx]));

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_ADD_OPS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_ADD_OPS_SUCCESSFUL]);
	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOWS_ACTIVE]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOWS_ACTIVE]);

	netfn_flow_cookie_info("Successfully added node:%p", flow);

	spin_unlock_bh(&nfc_ctx->lock);

	synchronize_rcu();

	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_add);

/*
 * netfn_flow_cookie_db_entry_del
 * 	We use the following API to delete the
 *	appropriate DB entry corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_del(struct netfn_flow_cookie_db *d, struct netfn_tuple *t)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint32_t hash_idx;
	struct netfn_flow_cookie_db_entry *flow = NULL;

	spin_lock_bh(&nfc_ctx->lock);

	hash_idx = netfn_flow_cookie_db_calculate_hash_idx(d, t);

	rcu_read_lock_bh();
	netfn_flow_cookie_db_lookup_flow(d, t, hash_idx, &flow);

	if (!flow) {
		netfn_flow_cookie_warn("%p: Corresponding DB entry does not exist", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_DELETE_OPS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_DELETE_OPS_FAILED]);
		rcu_read_unlock_bh();
		spin_unlock_bh(&nfc_ctx->lock);

		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	/*
	 * We remove the DB entry from the appropriate
	 * RCU based Hash List here using the
	 * following API.
	 */
	hlist_del_rcu(&(flow->node));
	rcu_read_unlock_bh();

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_RULE_DELETE_OPS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_RULE_DELETE_OPS_SUCCESSFUL]);
	atomic64_dec(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOWS_ACTIVE]);
	atomic64_dec(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOWS_ACTIVE]);

	spin_unlock_bh(&nfc_ctx->lock);

	synchronize_rcu();

	/*
	 * We release the memory as allocated
	 * during the DB entry creation here.
	 */
	kfree(flow);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_del);

/*
 * netfn_flow_cookie_db_entry_get_flow_id
 * 	We use the following API to fetch the
 *	flow ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info
 *
 * @return
 * 32 bit Flow ID.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_flow_id(struct netfn_flow_cookie_db *d, struct netfn_tuple *t)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie cookie;
	uint32_t flow_id;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	/*
	 * We Retrieve the flow cookie here.
	 */
	if (_netfn_flow_cookie_db_get_cookie(d, t, &cookie) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_ID_FETCH_CALLS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_ID_FETCH_CALLS_FAILED]);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
	}

	/*
	 * We retrieve the Flow ID here.
	 */
	flow_id = cookie.flow_id;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_ID_FETCH_CALLS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_ID_FETCH_CALLS_SUCCESSFUL]);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return flow_id;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_id);

/*
 * netfn_flow_cookie_db_entry_get_flow_id_using_skb
 *	 We use the following API to fetch the
 *       Flow ID corresponding to the
 *       5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit Flow ID.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_flow_id_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_tuple tuple;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv4(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_ID_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_ID_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
		}
		return netfn_flow_cookie_db_entry_get_flow_id(d, &tuple);

	} else if (ntohs(skb->protocol) == ETH_P_IPV6) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv6(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_ID_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_ID_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
		}
		return netfn_flow_cookie_db_entry_get_flow_id(d, &tuple);
	}

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_ID_FETCH_CALLS_FAILED]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_ID_FETCH_CALLS_FAILED]);

	return NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_id_using_skb);

/*
 * netfn_flow_cookie_db_entry_get_sawf_handle
 * 	We use the following API to fetch the
 * 	SAWF handle corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info, Sawf Handle Pointer to fill the data.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_sawf_handle(struct netfn_flow_cookie_db *d, struct netfn_tuple *t,
								      struct netfn_flow_cookie_sawf_handle *sawf_handle)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie cookie;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	/*
	 * We Retrieve the flow cookie here.
	 */
	if (_netfn_flow_cookie_db_get_cookie(d, t, &cookie) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

		/*
		 * We fill the Sawf handle ID to invalid value
		 * to indicate that an invalid Sawf Handle
		 * is returned.
		 */
		sawf_handle->sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;

		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SAWF_HANDLE_FETCH_CALLS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SAWF_HANDLE_FETCH_CALLS_FAILED]);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	/*
	 * We retrieve the SAWF handle here.
	 */
	*sawf_handle = cookie.sawf_handle;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SAWF_HANDLE_FETCH_CALLS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SAWF_HANDLE_FETCH_CALLS_SUCCESSFUL]);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_sawf_handle);

/*
 * netfn_flow_cookie_db_entry_get_sawf_handle_using_skb
 *	We use the following API to fetch the
 *      SAWF handle corresponding to the
 *      5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 * @param[in] sawf_handle	Sawf Handle Pointer to fill the data
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_sawf_handle_using_skb(struct netfn_flow_cookie_db *d,
					struct sk_buff *skb, uint32_t offset, struct netfn_flow_cookie_sawf_handle *sawf_handle)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_tuple tuple;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv4(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			/*
			 * We fill the Sawf handle ID to invalid value
			 * to indicate that an invalid Sawf Handle
			 * is returned.
			 */
			sawf_handle->sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SAWF_HANDLE_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SAWF_HANDLE_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_STATUS_FAILURE;
		}

		return netfn_flow_cookie_db_entry_get_sawf_handle(d, &tuple, sawf_handle);

	} else if (ntohs(skb->protocol) == ETH_P_IPV6) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv6(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			/*
			 * We fill the Sawf handle ID to invalid value
			 * to indicate that an invalid Sawf Handle
			 * is returned.
			 */
			sawf_handle->sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SAWF_HANDLE_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SAWF_HANDLE_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_STATUS_FAILURE;
		}

		return netfn_flow_cookie_db_entry_get_sawf_handle(d, &tuple, sawf_handle);
	}

	/*
	 * We fill the Sawf handle ID to invalid value
	 * to indicate that an invalid Sawf Handle
	 * is returned.
	 */
	sawf_handle->sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SAWF_HANDLE_FETCH_CALLS_FAILED]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SAWF_HANDLE_FETCH_CALLS_FAILED]);

	return NETFN_FLOW_COOKIE_STATUS_FAILURE;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_sawf_handle_using_skb);

/*
 * netfn_flow_cookie_db_entry_get_flow_mark
 * 	We use the following API to fetch the
 *	Flow Mark corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info
 *
 * @return
 * 32 bit Flow Mark.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_flow_mark(struct netfn_flow_cookie_db *d, struct netfn_tuple *t)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie cookie;
	uint32_t flow_mark;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	/*
	 * We Retrieve the flow cookie here.
	 */
	if (_netfn_flow_cookie_db_get_cookie(d, t, &cookie) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_MARK_FETCH_CALLS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_MARK_FETCH_CALLS_FAILED]);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
	}

	/*
	 * We retrieve the Flow Mark here.
	 */
	flow_mark = cookie.flow_mark;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_MARK_FETCH_CALLS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_MARK_FETCH_CALLS_SUCCESSFUL]);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return flow_mark;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_mark);

/*
 * netfn_flow_cookie_db_entry_get_flow_mark_using_skb
 *	We use the following API to fetch the
 *      Flow Mark corresponding to the
 *      5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit Flow Mark.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_flow_mark_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_tuple tuple;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv4(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_MARK_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_MARK_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
		}

		return netfn_flow_cookie_db_entry_get_flow_mark(d, &tuple);

	} else if (ntohs(skb->protocol) == ETH_P_IPV6) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv6(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_MARK_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_MARK_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
		}

		return netfn_flow_cookie_db_entry_get_flow_mark(d, &tuple);
	}

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_MARK_FETCH_CALLS_FAILED]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_MARK_FETCH_CALLS_FAILED]);

	return NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_mark_using_skb);

/*
 * netfn_flow_cookie_db_entry_get_scs_id
 *	We use the following API to fetch the
 *	SCS ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info
 *
 * @return
 * 32 bit SCS ID.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_scs_id(struct netfn_flow_cookie_db *d, struct netfn_tuple *t)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie cookie;
	uint32_t scs_id;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	/*
	 * We Retrieve the flow cookie here.
	 */
	if (_netfn_flow_cookie_db_get_cookie(d, t, &cookie) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SCS_ID_FETCH_CALLS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SCS_ID_FETCH_CALLS_FAILED]);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_INVALID_SCS_ID;
	}

	/*
	 * We retrieve the SCS ID here.
	 */
	scs_id = cookie.scs_id;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SCS_ID_FETCH_CALLS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SCS_ID_FETCH_CALLS_SUCCESSFUL]);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return scs_id;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_scs_id);

/*
 * netfn_flow_cookie_db_entry_get_scs_id_using_skb
 *	We use the following API to fetch the
 *	SCS ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit SCS ID.
 *
 */
uint32_t netfn_flow_cookie_db_entry_get_scs_id_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_tuple tuple;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv4(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SCS_ID_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SCS_ID_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_SCS_ID;
		}

		return netfn_flow_cookie_db_entry_get_scs_id(d, &tuple);

	} else if (ntohs(skb->protocol) == ETH_P_IPV6) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv6(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SCS_ID_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SCS_ID_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_INVALID_SCS_ID;
		}

		return netfn_flow_cookie_db_entry_get_scs_id(d, &tuple);
	}

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_SCS_ID_FETCH_CALLS_FAILED]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_SCS_ID_FETCH_CALLS_FAILED]);

	return NETFN_FLOW_COOKIE_INVALID_SCS_ID;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_scs_id_using_skb);

/*
 * netfn_flow_cookie_db_entry_get_flow_cookie
 * 	We use the following API to fetch the entire
 *	flow Cookie corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in]
 * netfn_flow_cookie_db handle, 5 tuple info, Flow Cookie Pointer to fill the data.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_flow_cookie(struct netfn_flow_cookie_db *d, struct netfn_tuple *t,
									     struct netfn_flow_cookie *flow_cookie)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie cookie;

	/*
	 * Invoking a ref on the DB handle here
	 * to protect the memory unit in
	 * parallel execution scenarios.
	 */
	netfn_flow_cookie_db_instance_ref(d);

	/*
	 * We Retrieve the flow cookie here.
	 */
	if (_netfn_flow_cookie_db_get_cookie(d, t, &cookie) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

		/*
		 * We fill the Flow Cookie fields to "-1"
		 * to indicate that an invalid Flow Cookie
		 * is returned.
		 */
		flow_cookie->flow_id = NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
		flow_cookie->sawf_handle.sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;
		flow_cookie->flow_mark = NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
		flow_cookie->scs_id = NETFN_FLOW_COOKIE_INVALID_SCS_ID;

		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_COOKIE_FETCH_CALLS_FAILED]);
		atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_COOKIE_FETCH_CALLS_FAILED]);

		/*
		 * Releasing the ref on the DB handle here
		 * when encountered error.
		 */
		netfn_flow_cookie_db_instance_deref(d);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	/*
	 * We retrieve the Flow Cookie here.
	 */
	flow_cookie->flow_id = cookie.flow_id;
	flow_cookie->sawf_handle = cookie.sawf_handle;
	flow_cookie->flow_mark = cookie.flow_mark;
	flow_cookie->scs_id = cookie.scs_id;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_COOKIE_FETCH_CALLS_SUCCESSFUL]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_COOKIE_FETCH_CALLS_SUCCESSFUL]);

	/*
	 * Releasing the ref on the DB handle here.
	 */
	netfn_flow_cookie_db_instance_deref(d);
	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_cookie);

/*
 * netfn_flow_cookie_db_entry_get_flow_cookie_using_skb
 *	 We use the following API to fetch the
 *       Flow ID corresponding to the
 *       5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 * @param[in] flow_cookie	Flow Cookie Pointer to fill the data.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_flow_cookie_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb,
										uint32_t offset, struct netfn_flow_cookie *flow_cookie)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_tuple tuple;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv4(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			/*
			 * We fill the Flow Cookie fields to "-1"
			 * to indicate that an invalid Flow Cookie
			 * is returned.
			 */
			flow_cookie->flow_id = NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
			flow_cookie->sawf_handle.sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;
			flow_cookie->flow_mark = NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
			flow_cookie->scs_id = NETFN_FLOW_COOKIE_INVALID_SCS_ID;

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_COOKIE_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_COOKIE_FETCH_CALLS_FAILED]);

			return NETFN_FLOW_COOKIE_STATUS_FAILURE;
		}

		return netfn_flow_cookie_db_entry_get_flow_cookie(d, &tuple, flow_cookie);

	} else if (ntohs(skb->protocol) == ETH_P_IPV6) {
		if (netfn_flow_cookie_db_entry_extract_tuple_ipv6(skb, offset, &tuple) != NETFN_FLOW_COOKIE_STATUS_SUCCESS) {

			/*
			 * We fill the Flow Cookie fields to "-1"
			 * to indicate that an invalid Flow Cookie
			 * is returned.
			 */
			flow_cookie->flow_id = NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
			flow_cookie->sawf_handle.sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;
			flow_cookie->flow_mark = NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
			flow_cookie->scs_id = NETFN_FLOW_COOKIE_INVALID_SCS_ID;

			atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_COOKIE_FETCH_CALLS_FAILED]);
			atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_COOKIE_FETCH_CALLS_FAILED]);


			return NETFN_FLOW_COOKIE_STATUS_FAILURE;
		}

		return netfn_flow_cookie_db_entry_get_flow_cookie(d, &tuple, flow_cookie);
	}

	/*
	 * We fill the Flow Cookie fields to "-1"
	 * to indicate that an invalid Flow Cookie
	 * is returned.
	 */
	flow_cookie->flow_id = NETFN_FLOW_COOKIE_INVALID_FLOW_ID;
	flow_cookie->sawf_handle.sawf_id = NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE;
	flow_cookie->flow_mark = NETFN_FLOW_COOKIE_INVALID_FLOW_MARK;
	flow_cookie->scs_id = NETFN_FLOW_COOKIE_INVALID_SCS_ID;

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_COOKIE_FETCH_CALLS_FAILED]);
	atomic64_inc(&d->db_base_stats[NETFN_FLOW_COOKIE_DB_FLOW_COOKIE_FETCH_CALLS_FAILED]);

	return NETFN_FLOW_COOKIE_STATUS_FAILURE;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_entry_get_flow_cookie_using_skb);

/*
 * netfn_flow_cookie_db_alloc
 * 	We use the following API to initialize the
 * 	Flow DB basing on the no.of entries as
 * 	passed from the DB init caller.
 *
 * @param[in]
 * 32 bit Hash Table size
 *
 * @return
 * netfn_flow_cookie_db handle instantiated.
 *
 */
struct netfn_flow_cookie_db *netfn_flow_cookie_db_alloc(uint32_t num_entries)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie_db *d = NULL;

	d = (struct netfn_flow_cookie_db *)kzalloc(sizeof(struct netfn_flow_cookie_db), GFP_ATOMIC);
	if (!d) {
		netfn_flow_cookie_warn("Failed to allocate memory for the DB instance");
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED_DUE_TO_UNAVAILABLE_MEMORY]);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED]);
		return NULL;
	}

	/*
	 * Storing the relevant fields
	 */
	d->entry_size = sizeof(struct netfn_flow_cookie_db_entry);

	netfn_flow_cookie_info("%p: Done with DB allocation with the entry size: %lu", d, d->entry_size);

	if (num_entries == 0) {
		num_entries = NETFN_FLOW_COOKIE_DB_HASH_SZ;
	}

	if ((num_entries < 0) || (num_entries > NETFN_FLOW_COOKIE_DB_HASH_SZ)) {
		netfn_flow_cookie_warn("Invalid Hash table size passed from user, "
			"using the default num_entries:%d", num_entries);
		num_entries = NETFN_FLOW_COOKIE_DB_HASH_SZ;
	}

	/*
	 * Allocate the Hash Table for netfn_flow_cookie_db here.
	 */
	if (!netfn_flow_cookie_db_hash_table_alloc(d, num_entries)) {
		netfn_flow_cookie_warn("%p: failed to allocate netfn_flow_cookie_db hash table", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED_DUE_TO_UNAVAILABLE_MEMORY]);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED]);

		/*
		 * We Free the relevant Memory as taken for the
		 * DB allocation and De-Initialize it's pointer to NULL here
		 * for this uninitialized DB table case.
		 */
		kfree(d);
		d = NULL;

		return NULL;
	}

	if (netfn_flow_cookie_db_handle_store(d) == -1) {
		netfn_flow_cookie_warn("Error in storing the DB entry in DB array, netfn_flow_cookie_db ref:%p", d);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED_DUE_TO_OTHER_REASONS]);
		atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_FAILED]);

		d->flags &= ~NETFN_FLOW_COOKIE_DB_FLAG;

		/*
		 * We Free the relevant Memory as taken for the
		 * DB table allocation and De-Initialize it's pointer to NULL here.
		 */
		kfree(d->db_table);
		d->db_table = NULL;

		/*
		 * We Free the relevant Memory as taken for the
		 * DB allocation and De-Initialize it's pointer to NULL here.
		 */
		kfree(d);
		d = NULL;

		return NULL;
	}

	atomic64_inc(&nfc_ctx->gbl_base_stats[NETFN_FLOW_COOKIE_GBL_FLOW_DB_INIT_OPS_SUCCESSFUL]);
	netfn_flow_cookie_trace("%p: Done with Hash table database allocation", d);

	/*
	 * We initialize the reference structure here
	 * for the DB instance.
	 */
	kref_init(&d->ref);
	netfn_flow_cookie_trace("%p: Done with initializing ref, Reference count is %d "
			"for the latest DB instance", d, kref_read(&d->ref));
	return d;
}
EXPORT_SYMBOL(netfn_flow_cookie_db_alloc);
