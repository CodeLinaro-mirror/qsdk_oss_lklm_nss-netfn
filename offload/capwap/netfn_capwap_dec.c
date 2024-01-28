/*
 * netfn_capwap_dec.c
 *	Network function's CAPWAP offload decapsulation.
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/sysfs.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/ieee80211.h>
#include <linux/if_ether.h>
#include <linux/if_trustsec.h>
#include <linux/ip.h>

#include <net/protocol.h>
#include <net/dsfield.h>

#include "netfn_capwap.h"
#include "netfn_capwap_hdr.h"
#include "netfn_capwap_priv.h"
#include "netfn_capwap_tun.h"
#include "netfn_capwap_dec.h"

/*
 * netfn_capwap_dec_fast_reasm()
 *	Verify if given 2 skbs can be reassembled.
 */
static struct sk_buff *netfn_capwap_dec_fast_reasm(struct netfn_capwap_dec *dec, struct netfn_capwap_dec_ctx *ctx,
		struct sk_buff *skb)
{
	struct netfn_capwap_hdr_mdata *info1, *info2;
	struct sk_buff *skb1, *skb2;
	int err = 0;

	skb1 = ctx->reasm_cache;
	skb2 = skb;
	info1 = NETFN_CAPWAP_CB(skb1);
	info2 = NETFN_CAPWAP_CB(skb2);

	/*
	 * If these are jumbled frags we swap them.
	 */
	if (unlikely(info1->frag_offset > info2->frag_offset)) {
		skb1 = xchg(&skb2, skb1);
		info1 = xchg(&info2, info1);
	}

	/*
	 * Check if we have enough tailroom. Else fallback to slow reasm
	 * which allocates a new skb.
	 */
	if (unlikely(skb_tailroom(skb1) < skb2->len)) {
		return NULL;
	}

	/*
	 * Unhandled cases;
	 * 1.Mismatched fragments
	 * 2.More than 2 fragments in the chain
	 */

	err = (info1->frag_id != info2->frag_id);
	err += (skb1->len != info2->frag_offset);
	err += !!info1->frag_offset;
	err += !info2->frag_end;
	if (err) {
		return NULL;
	}

	/*
	 * TODO: Setup cache line(s) based on src_len upto a max of 4
	 */
	prefetchw(skb_tail_pointer(skb1));

	/*
	 * Coalesce the 2 skbs.
	 */
	__skb_put_data(skb1, skb2->data, skb2->len);
	dev_kfree_skb(skb2);

	ctx->reasm_cache = NULL;
	NETFN_CAPWAP_TUN_STATS_INC(&dec->stats.fast_reasm);
	return skb1;
}

/*
 * netfn_capwap_dec_linearize()
 *	Linearizes the chain of skbs
 */
static struct sk_buff *netfn_capwap_dec_linearize(struct netfn_capwap_frags *frags)
{
	struct sk_buff *iter, *skb;
	uint16_t skb_size;

	skb_size = frags->tot_sz + NETFN_CAPWAP_HEADROOM_RESERVE + NETFN_CAPWAP_TAILROOM_RESERVE;
	skb = dev_alloc_skb(skb_size);
	if (!skb) {
		pr_debug("%px: Unable to allocate skb of size(%d)\n", &frags->list, skb_size);
		goto done;
	}

	skb_reserve(skb, NETFN_CAPWAP_HEADROOM_RESERVE);

	/*
	 * TODO: Handle offset corruption case.
	 */

	skb_queue_walk(&frags->list, iter) {
		uint16_t offset = NETFN_CAPWAP_CB(iter)->frag_offset;
		memcpy(skb->data + offset, iter->data, iter->len);
	}

	__skb_put(skb, frags->tot_sz);
done:
	dev_kfree_skb_list_fast(&frags->list);
	return skb;
}

/*
 * netfn_capwap_dec_slow_reasm()
 *	Reassemble the packets.
 */
static struct sk_buff *netfn_capwap_dec_slow_reasm(struct netfn_capwap_dec *dec, struct netfn_capwap_dec_ctx *ctx,
		struct sk_buff *skb)
{
	struct netfn_capwap_dec_stats *stats = &dec->stats;
	struct netfn_capwap_hdr_mdata *info;
	struct sk_buff *reasm_skb = NULL;
	struct netfn_capwap_frags *frags;
	struct sk_buff_head *frag_list;
	uint16_t idx;

	info = NETFN_CAPWAP_CB(skb);
	idx = NETFN_CAPWAP_FRAG_IDX(info->frag_id, NETFN_CAPWAP_TUN_MAX_REASM_WIN);

	frags = &ctx->reasm_table[idx];
	frag_list = &frags->list;

	/*
	 * If slow reassembly queue is empty we park skb in queue and exit.
	 */
	if (skb_queue_empty(frag_list)) {
		netfn_capwap_frags_add(frags, skb);
		return NULL;
	}

	/*
	 * If this is an overlap then we free the previous list.
	 */
	if (unlikely(info->frag_id != frags->frag_id)) {
		NETFN_CAPWAP_TUN_STATS_ADD(&stats->drop_missing_frags, skb_queue_len(frag_list));
		dev_kfree_skb_list_fast(frag_list);
		netfn_capwap_frags_init(frags);
	}

	/*
	 * Add to reassembly queue
	 */
	netfn_capwap_frags_add(frags, skb);

	/*
	 * Check if total payload size exceeds max payload size
	 */
	if (unlikely(frags->tot_sz > dec->max_payload_sz)) {
		NETFN_CAPWAP_TUN_STATS_INC(&stats->drop_large_frags);
		dev_kfree_skb_list_fast(frag_list);
		return NULL;
	}

	/*
	 * Check if the number of frags exceeds max limit.
	 */
	if (unlikely(skb_queue_len(frag_list) > dec->max_frags)) {
		NETFN_CAPWAP_TUN_STATS_ADD(&stats->drop_max_frags, skb_queue_len(frag_list));
		dev_kfree_skb_list_fast(frag_list);
		goto done;
	}

	/*
	 * Check if we have received all frags.
	 */
	if (frags->frag_sz != frags->tot_sz) {
		return NULL;
	}

	reasm_skb = netfn_capwap_dec_linearize(frags);
	if (!reasm_skb) {
		NETFN_CAPWAP_TUN_STATS_INC(&stats->drop_linearize_fail);
	}

	NETFN_CAPWAP_TUN_STATS_INC(&stats->slow_reasm);
done:
	netfn_capwap_frags_init(frags);
	return reasm_skb;
}

/*
 * netfn_capwap_dec_80211_to_8023()
 *	Converts 802.11 packets to 802.3 packets.
 *	-----------------------------------------
 *	|CAPWAP HDR | WIRELESS INFO | 802.11 PKT|
 *	-----------------------------------------
 */
static uint8_t netfn_capwap_dec_80211_to_8023(struct netfn_capwap_dec *dec, struct netfn_capwap_prehdr *preh, uint8_t *start)
{
	struct ieee80211_hdr_3addr *wlan_hdr;
	uint8_t smac[ETH_ALEN];
	uint8_t dmac[ETH_ALEN];
	uint8_t next_hdr = 0;
	struct ethhdr *eth;
	__le16 frame_ctrl;
	uint16_t protocol;

	/*
	 * We only convert 802.11 DATA frame.
	 */
	wlan_hdr = (struct ieee80211_hdr_3addr *)start;
	frame_ctrl = ntohs(wlan_hdr->frame_control);

	/*
	 * Save mac addresses for constructing 802.3 frame.
	 */
	ether_addr_copy((u8 *)smac, (u8 *)wlan_hdr->addr1);
	ether_addr_copy((u8 *)dmac, (u8 *)wlan_hdr->addr3);

	next_hdr += sizeof(*wlan_hdr);

	/*
	 * Strip the Qos header (802.11e)
	 */
	if (likely(ieee80211_is_data_qos(frame_ctrl))) {
		preh->wlan_qos = *(uint16_t *)(start + next_hdr);
		next_hdr += sizeof(preh->wlan_qos);
	}

	next_hdr += NETFN_CAPWAP_SNAP_HDR_LEN;

	/*
	 * TODO: Handle trustsec check.
	 */
	if (dec->features & NETFN_CAPWAP_FEATURES_INNER_SGT) {
		BUG_ON(1);
	}

	protocol = *(uint16_t *)(start + next_hdr);
	next_hdr += sizeof(uint16_t);


	/*
	 * Start writing the new ethernet header
	 */
	next_hdr -= sizeof(*eth);

	eth = (struct ethhdr *)(start + next_hdr);
	ether_addr_copy((u8 *)eth->h_dest, (u8 *)dmac);
	ether_addr_copy((u8 *)eth->h_source, (u8 *)smac);
	eth->h_proto = protocol;

	return next_hdr;
}

/*
 * netfn_capwap_dec_capwap()
 *      Decapsulates capwap
 */
static uint8_t *netfn_capwap_dec_capwap(struct netfn_capwap_dec *dec, struct sk_buff *skb, uint8_t *start, struct netfn_capwap_prehdr *preh)
{
	struct netfn_capwap_hdr *nch = (struct netfn_capwap_hdr *)start;
	uint8_t cw_hlen, next_hdr = 0, preamble;
	struct netfn_capwap_hdr_mdata *mdata;
	union netfn_capwap_hdr_info info;
	union netfn_capwap_hdr_frag frag;

	mdata = NETFN_CAPWAP_CB(skb);
	info.word = ntohl(nch->info);
	frag.word = ntohl(nch->frag);

	/*
	 * For capwap pkts preamble should be zero.
	 */
	preamble = netfn_capwap_hdr_get_preamble(info);
	if (unlikely(preamble)) {
		preh->type |= NETFN_CAPWAP_PKT_TYPE_INVALID;
		return start;
	}

	mdata->frag = netfn_capwap_hdr_has_fbit(info);
	mdata->frag_end = netfn_capwap_hdr_has_lbit(info);
	mdata->keep_alive = netfn_capwap_hdr_has_kbit(info);

	mdata->frag_id = netfn_capwap_hdr_get_frag_id(frag);
	mdata->frag_offset = netfn_capwap_hdr_get_frag_offset(frag);

	/*
	 * Pull capwap header
	 */
	next_hdr += sizeof(*nch);

	preh->vers = NETFN_CAPWAP_VERSION;
	preh->rid = netfn_capwap_hdr_get_rid(info);
	cw_hlen = netfn_capwap_hdr_get_hlen(info);

	/*
	 * We skip the processing if:
	 * 1. Its keepalive packet or
	 * 2. Its not first fragment.
	 */
	if (mdata->keep_alive || (mdata->frag && mdata->frag_offset)) {
		NETFN_CAPWAP_TUN_STATS_ADD(&dec->stats.keepalive_pkts, mdata->keep_alive);
		return start + next_hdr;
	}

	/*
	 * TODO: Make this as a new function for optimization to avoid hitting
	 * above if condition twice.
	 */
	/*
	 * If wireless info is present it needs to be sent as part of preheader.
	 */
	if (netfn_capwap_hdr_has_wbit(info)) {
		struct netfn_capwap_winfo *winfo = (struct netfn_capwap_winfo *)(start + next_hdr);
		uint8_t nwireless;

		preh->type |= NETFN_CAPWAP_PKT_TYPE_WINFO;

		/*
		 * If wireless info exceeds the max sections then we punt inner packet to host.
		 */
		nwireless = cw_hlen - sizeof(*nch);
		if (unlikely(nwireless > NETFN_CAPWAP_MAX_NWIRELESS)) {
			return start + next_hdr;
		}

		BUILD_BUG_ON(NETFN_CAPWAP_MAX_NWIRELESS > sizeof(*winfo));
		preh->nwireless = nwireless;
		*((struct netfn_capwap_winfo *)preh->winfo) = *winfo;

		/*
		 * Pull the wirelesss information header
		 */
		next_hdr += sizeof(*winfo);
	}

	/*
	 * If inner is 802.11 packet than we convert it to 802.3.
	 */
	if (netfn_capwap_hdr_has_tbit(info)) {
		struct ieee80211_hdr_3addr *wlan_hdr = (struct ieee80211_hdr_3addr *)(start + next_hdr);

		if (!unlikely(ieee80211_is_data(ntohs(wlan_hdr->frame_control)))) {
			return (start + next_hdr);
		}

		/*
		 * Convert inner 802.11 packet to 802.3 packet.
		 */
		next_hdr += netfn_capwap_dec_80211_to_8023(dec, preh, (start + next_hdr));
		preh->type |= NETFN_CAPWAP_PKT_TYPE_8023;
	}

	return (start + next_hdr);
}

/*
 * netfn_capwap_dec_l4()
 *	Common sanity function for udp and udplite.
 */
static uint8_t *netfn_capwap_dec_l4(struct netfn_capwap_dec *dec, struct sk_buff *skb, uint8_t *start, struct netfn_capwap_prehdr *preh)
{
	struct udphdr *udph = (struct udphdr *)start;

	/*
	 * Check if its a control packet.
	 */
	if (unlikely(ntohs(udph->source) == NETFN_CAPWAP_CTRL_PORT)) {
		NETFN_CAPWAP_TUN_STATS_INC(&dec->stats.control_pkts);
		preh->type |= NETFN_CAPWAP_PKT_TYPE_CTRL;
		return start + sizeof(*udph);
	}

	preh->type |= NETFN_CAPWAP_PKT_TYPE_DATA;
	return start + sizeof(*udph);
}

/*
 * netfn_capwap_dec_l3()
 *	Decapsulate L3 header
 */
static uint8_t *netfn_capwap_dec_l3(struct netfn_capwap_dec *dec, struct sk_buff *skb, uint8_t *start, struct netfn_capwap_prehdr *preh)
{
	struct ipv6hdr *ip6h;
	struct iphdr *iph;

	if (ntohs(skb->protocol) == ETH_P_IP) {
		iph = (struct iphdr *)start;

		preh->dscp = ipv4_get_dsfield(iph);
		skb->protocol = iph->protocol;
		return start + sizeof(*iph);
	}

	ip6h = (struct ipv6hdr *)start;

	preh->dscp = ipv6_get_dsfield(ip6h);
	skb->protocol = ip6h->nexthdr;
	return start + sizeof(*ip6h);
}

/*
 * netfn_capwap_dec()
 *	Decapsulates headers. Converts inner packet from 802.11 to 802.3
 *	and adds preheader if needed.
 */
static bool netfn_capwap_dec(struct netfn_capwap_dec *dec, struct sk_buff *skb, bool *is_frag)
{
	struct netfn_capwap_prehdr preh = {0};
	struct netfn_capwap_prehdr *pkt_preh;
	struct netfn_capwap_hdr_mdata *mdata;
	uint8_t *start, *next_hdr;
	uint8_t exception = 0;

	start = next_hdr = skb->data;

	/*
	 * We don't need to reset skb cb as we are overriding
	 * the values read for further use.
	 */
	mdata = NETFN_CAPWAP_CB(skb);

	/************************
	 * Decapsulation starts.*
	 ************************/
	/*
	 * If dtls is enabled we have skip IP and UDP/UDPLite decapsulation.
	 * Its stripped by DTLS.
	 */
	if (dec->features & NETFN_CAPWAP_FEATURES_DTLS) {
		preh.dscp = skb->priority;
		next_hdr = netfn_capwap_dec_capwap(dec, skb, next_hdr, &preh);
		goto skip_headers;
	}

	next_hdr = netfn_capwap_dec_l3(dec, skb, next_hdr, &preh);
	next_hdr = netfn_capwap_dec_l4(dec, skb, next_hdr, &preh);
	next_hdr = netfn_capwap_dec_capwap(dec, skb, next_hdr, &preh);

skip_headers:

	/*
	 * Sanity check to see if we didn't overflow.
	 */
	if (unlikely(next_hdr > skb_tail_pointer(skb))) {
		NETFN_CAPWAP_TUN_STATS_INC(&dec->stats.drop_malformed);
		return false;
	}

	/*
	 * We exception packets if:
	 * 1. If its keepalive.
	 * 2. If its a invalid packet.
	 * 3. If its control packet and not fragmented.
	 */
	exception += mdata->keep_alive;
	exception += (preh.type & NETFN_CAPWAP_PKT_TYPE_INVALID);
	exception += ((preh.type & NETFN_CAPWAP_PKT_TYPE_CTRL) && (!mdata->frag));
	if (exception) {
		mdata->exception = 1;
		goto done;
	}

	/*
	 * We don't add preheader if its not first frag.
	 */
	if (mdata->frag && mdata->frag_offset) {
		goto skip_phdr_add;
	}

	/*
	 * Add preheader.
	 */
	next_hdr -= sizeof(preh);
	pkt_preh = (struct netfn_capwap_prehdr *)next_hdr;
	*pkt_preh = preh;

skip_phdr_add:
	__skb_pull(skb, next_hdr - start);
	*is_frag = !!mdata->frag;

done:
	return true;
}

/*
 * netfn_capwap_dec_stats_show()
 *	Prints the decap stats.
 */
static ssize_t netfn_capwap_dec_stats_show(struct file *fp, char __user *ubuf, size_t sz, loff_t *ppos)
{
	struct netfn_capwap_dec *dec = fp->private_data;
	struct netfn_capwap_dec_stats *stats = &dec->stats;
	size_t dec_stats_sz, stats_sz;
	ssize_t bytes_read = 0;
	size_t size_wr = 0;
	char *buf;

	dec_stats_sz = sizeof(struct netfn_capwap_dec_stats) / sizeof(uint64_t) * NETFN_CAPWAP_DEC_STATS_STRLEN;
	stats_sz = (NETFN_CAPWAP_DEC_STATS_WIDTH * NETFN_CAPWAP_DEC_STATS_STRLEN) + dec_stats_sz;

	buf = vzalloc(stats_sz);
	if (!buf) {
		pr_warn("%px: Unable to allocate memory for stats.\n", fp);
		return -ENOMEM;
	}

	/*
	 * TODO: Convert to scnpritf engine.
	 * Print decap stats.
	 */
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "\n------------[decap stats start]----------\n");
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "num_frags", READ_ONCE(stats->num_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "control_pkts", READ_ONCE(stats->control_pkts));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "keepalive_pkts", READ_ONCE(stats->keepalive_pkts));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "fast_reasm", READ_ONCE(stats->fast_reasm));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "slow_reasm", READ_ONCE(stats->slow_reasm));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "drop_dup_frags", READ_ONCE(stats->drop_dup_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_dec_failure", READ_ONCE(stats->drop_dec_failure));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "drop_max_frags", READ_ONCE(stats->drop_max_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_missing_frags", READ_ONCE(stats->drop_missing_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_large_frags", READ_ONCE(stats->drop_large_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "drop_csum_fail", READ_ONCE(stats->drop_csum_fail));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "drop_malformed", READ_ONCE(stats->drop_malformed));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_linearize_fail", READ_ONCE(stats->drop_linearize_fail));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_reasm_window", READ_ONCE(stats->drop_reasm_window));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "------------[decap stats end]-----------\n");

	bytes_read = simple_read_from_buffer(ubuf, sz, ppos, buf, size_wr);
	vfree(buf);
	return bytes_read;
}

/*
 * Capwap file operations.
 */
static const struct file_operations file_ops = {
	.open = simple_open,
	.llseek = default_llseek,
	.read = netfn_capwap_dec_stats_show
};

/*
 * netfn_capwap_dec_stats_read()
 *	Reads decap stats.
 */
void netfn_capwap_dec_stats_read(struct netfn_capwap_dec *dec, struct netfn_capwap_tun_stats *stats)
{
	uint64_t *dest = (uint64_t *)&stats->dec;
	uint64_t *src = (uint64_t *)&dec->stats;
	int i;

	for (i = 0; i < (sizeof(dec->stats) / sizeof(uint64_t)); i++) {
		*(uint64_t *)dest = *(uint64_t *)src;
	}
}

/*
 * netfn_capwap_dec_rx_skbs()
 *	Decapsulate and reassemble the skbs.
 */
bool netfn_capwap_dec_rx_skbs(struct netfn_capwap_dec *dec, struct sk_buff_head *q_head)
{
	struct netfn_capwap_dec_ctx *ctx;
	struct sk_buff *skb, *reasm_skb;
	struct sk_buff_head q_frags;
	bool is_frag = false;

	rcu_read_lock_bh();

	ctx = rcu_dereference(dec->ctx);
	if (!ctx) {
		rcu_read_unlock_bh();
		return false;
	}

	__skb_queue_head_init(&q_frags);
	while (likely(skb_queue_len(q_head))) {
		skb = __skb_dequeue(q_head);

		NETFN_CAPWAP_TUN_STATS_INC(&dec->stats.num_frags);

		/*
		 * Decapsulate the skb.
		 */
		if (!netfn_capwap_dec(dec, skb, &is_frag)) {
			NETFN_CAPWAP_TUN_STATS_INC(&dec->stats.drop_dec_failure);
			dev_kfree_skb(skb);
			continue;
		}

		/*
		 * Non-fragmented skb.
		 */
		if (!is_frag) {
			__skb_queue_tail(&q_frags, skb);
			continue;
		}

		/*
		 * Fast reassembly queue is empty
		 */
		if (!ctx->reasm_cache) {
			ctx->reasm_cache = skb;
			continue;
		}

		/*
		 * Handle reassembly for 2 consecutive fragments.
		 */
		reasm_skb = netfn_capwap_dec_fast_reasm(dec, ctx, skb);
		if (likely(reasm_skb)) {
			__skb_queue_tail(&q_frags, reasm_skb);
			continue;
		}

		/*
		 * Pull skb out of fast reasm queue and send it for slow reassembly.
		 */
		skb = xchg(&ctx->reasm_cache, skb);

		/*
		 * These are either non-consecutive fragments or regular packets,
		 * In case of fragments search the decap fragment list for reassembly.
		 * However, if these are new fragments then automatically park them in
		 * the fragment list for future reassembly.
		 */
		reasm_skb = netfn_capwap_dec_slow_reasm(dec, ctx, skb);
		if (reasm_skb) {
			__skb_queue_tail(&q_frags, reasm_skb);
		}
	}

	/*
	 * Splice to original queue.
	 */
	skb_queue_splice_tail(&q_frags, q_head);
	rcu_read_unlock_bh();
	return true;
}

/*
 * netfn_capwap_dec_init()
 *	Initializes the decap object.
 */
bool netfn_capwap_dec_init(struct netfn_capwap_dec *dec, struct netfn_capwap_dec_cfg *cfg, struct dentry *parent)
{
	dec->max_frags = cfg->max_frags;
	dec->max_payload_sz = cfg->max_payload_sz;
	dec->features = cfg->features;

	/*
	 * Allocate debugfs entry.
	 */
	if (!debugfs_create_file("decap", S_IRUGO, parent, dec, &file_ops)) {
		pr_warn("%p: Failed to create encap debug entry.\n", dec);
		return false;
	}

	return true;
}
