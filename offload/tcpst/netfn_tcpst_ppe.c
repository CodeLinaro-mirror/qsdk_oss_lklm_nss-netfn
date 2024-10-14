/*
 * netfn_tcpst_ppe.c
 *	Netfn TCP Speedtest PPE acceleration helper
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/in6.h>
#include <linux/debugfs.h>
#include <net/inet_sock.h>
#include <netfn_flowmgr.h>
#include <ppe_drv.h>

/*
 * netfn_tcpst_ppe_get_macaddr()
 *	Fetch mac address using dst neigbour.
 */
static bool netfn_tcpst_ppe_get_macaddr(struct dst_entry *dst, void *daddr, uint8_t *mac_addr)
{
	struct neighbour *neigh;
	uint8_t nud_state;

	/*
	 * retrieve the neighbour
	 */
	neigh = dst_neigh_lookup(dst, daddr);
	if (!neigh) {
		pr_err("%px: neighbour lookup failed\n",  dst);
		return false;
	}

	read_lock_bh(&neigh->lock);
	nud_state = neigh->nud_state;
	ether_addr_copy(mac_addr, neigh->ha);
	read_unlock_bh(&neigh->lock);
	neigh_release(neigh);

	if ((neigh->nud_state & NUD_VALID) == 0) {
		pr_err("%px: neighbour state is invalid\n", dst);
		return false;
	}

	return true;
}

/*
 * netfn_tcpst_ppe_accel()
 */
bool netfn_tcpst_ppe_accel(struct sock *sk, struct net_device *vp_dev)
{
	struct netfn_flowmgr_flow_info *orig_info, *reply_info;
	struct netfn_tuple_5tuple *orig_tuple, *reply_tuple;
	struct netfn_flowmgr_create_rule reply = {0};
	struct netfn_flowmgr_create_rule orig = {0};
	netfn_flowmgr_ret_t status;
	struct inet_sock *inet;
	struct dst_entry *dst;

	orig_tuple = &orig.tuple.tuples.tuple_5;
	reply_tuple = &reply.tuple.tuples.tuple_5;
	orig_info = &orig.flow_info;
	reply_info = &reply.flow_info;

	/*
	 * dst should be already filled during connect()
	 */
	dst = sk_dst_get(sk);
	if (!dst) {
		pr_err("%px: Failed to get dst from socket\n", sk);
		return false;
	}

	/*
	 * Fill Orginal tuple.
	 */
	inet = inet_sk(sk);
	if (sk->sk_family == AF_INET) {
		orig_tuple->src_ip.ip4.s_addr = inet->inet_rcv_saddr;
		orig_tuple->dest_ip.ip4.s_addr = inet->inet_daddr;
		orig.tuple.ip_version = IPVERSION;
	} else {
		orig_tuple->src_ip.ip6 = inet->sk.sk_v6_rcv_saddr;
		orig_tuple->dest_ip.ip6 = inet->sk.sk_v6_daddr;
		orig.tuple.ip_version = 6;
	}

	orig.tuple.tuple_type = NETFN_TUPLE_5TUPLE;
	orig_tuple->l4_src_ident = inet->inet_sport;;
	orig_tuple->l4_dest_ident = inet->inet_dport;
	orig_tuple->protocol = IPPROTO_TCP;

	/*
	 * Fill original device info
	 */
	orig_info->out_dev = orig_info->top_outdev = dst->dev;
	orig_info->in_dev = orig_info->top_indev = vp_dev;
	orig_info->flow_mtu = READ_ONCE(dst->dev->mtu);

	memcpy(orig_info->flow_src_mac, dst->dev->dev_addr, ETH_ALEN);
	status = netfn_tcpst_ppe_get_macaddr(dst, &orig_tuple->dest_ip, orig_info->flow_dest_mac);
	dst_release(dst);

	if (!status) {
		pr_err("%px: Failed to get neigbour mac address\n", dst);
		return false;
	}

	/*
	 * Reply is exact reverse of original.
	 */
	memcpy(&reply_tuple->dest_ip, &orig_tuple->src_ip, sizeof(reply_tuple->dest_ip));
	memcpy(&reply_tuple->src_ip, &orig_tuple->dest_ip, sizeof(reply_tuple->src_ip));
	memcpy(reply_info->flow_dest_mac, orig_info->flow_src_mac, ETH_ALEN);
	memcpy(reply_info->flow_src_mac, orig_info->flow_dest_mac, ETH_ALEN);

	reply_tuple->l4_dest_ident = orig_tuple->l4_src_ident;
	reply_tuple->l4_src_ident = orig_tuple->l4_dest_ident;
	reply_tuple->protocol = orig_tuple->protocol;
	reply.tuple.ip_version = orig.tuple.ip_version;
	reply.tuple.tuple_type = orig.tuple.tuple_type;

	/*
	 * Fill device.
	 */
	reply_info->out_dev = reply_info->top_outdev = orig_info->in_dev;
	reply_info->in_dev = reply_info->top_indev  = orig_info->out_dev;
	reply_info->flow_mtu = orig_info->flow_mtu;

	/*
	 * TODO: Parse VLAN, PPPOE based on dst->dev
	 */
	orig.flow_flags |= NETFN_FLOWMGR_FLOW_FLAG_VP_FLOW;
	reply.flow_flags |= NETFN_FLOWMGR_FLOW_FLAG_VP_FLOW;

	status = netfn_flowmgr_rule_accel(&orig, &reply, NETFN_FLOWMGR_ACCEL_MODE_PPE);
	if (NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(status) != NETFN_FLOWMGR_RET_SUCCESS) {
		pr_err("%px: Failed to create flow rule status(%x)\n", sk, status);
		return false;
	}

	return true;
}

/*
 * netfn_tcpst_ppe_decel()
 */
void netfn_tcpst_ppe_decel(struct sock *sk)
{
	struct netfn_tuple_5tuple *reply_tuple, *orig_tuple;
	struct netfn_flowmgr_destroy_rule reply = {0};
	struct netfn_flowmgr_destroy_rule orig = {0};
	netfn_flowmgr_ret_t status;
	struct inet_sock *inet;

	reply_tuple = &reply.tuple.tuples.tuple_5;
	orig_tuple = &orig.tuple.tuples.tuple_5;

	/*
	 * Fill Orginal tuple.
	 */
	inet = inet_sk(sk);
	if (sk->sk_family == AF_INET) {
		orig_tuple->src_ip.ip4.s_addr = inet->inet_rcv_saddr;
		orig_tuple->dest_ip.ip4.s_addr = inet->inet_daddr;
		orig.tuple.ip_version = IPVERSION;
	} else {
		orig_tuple->src_ip.ip6 = inet->sk.sk_v6_rcv_saddr;
		orig_tuple->dest_ip.ip6 = inet->sk.sk_v6_daddr;
		orig.tuple.ip_version = 6;
	}

	orig.tuple.tuple_type = NETFN_TUPLE_5TUPLE;
	orig_tuple->l4_src_ident = inet->inet_sport;
	orig_tuple->l4_dest_ident = inet->inet_dport;
	orig_tuple->protocol = IPPROTO_TCP;

	/*
	 * Reply is exact reverse of original.
	 */
	memcpy(&reply_tuple->dest_ip, &orig_tuple->src_ip, sizeof(reply_tuple->dest_ip));
	memcpy(&reply_tuple->src_ip, &orig_tuple->dest_ip, sizeof(reply_tuple->src_ip));

	reply_tuple->l4_dest_ident = orig_tuple->l4_src_ident;
	reply_tuple->l4_src_ident = orig_tuple->l4_dest_ident;
	reply_tuple->protocol = orig_tuple->protocol;
	reply.tuple.ip_version = orig.tuple.ip_version;
	reply.tuple.tuple_type = NETFN_TUPLE_5TUPLE;

	status = netfn_flowmgr_rule_decel(&orig, &reply, NETFN_FLOWMGR_ACCEL_MODE_PPE);
	if (NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(status) != NETFN_FLOWMGR_RET_SUCCESS) {
		pr_err("%px: Failed to destroy flow rule status(%x)\n", sk, status);
	}
}
