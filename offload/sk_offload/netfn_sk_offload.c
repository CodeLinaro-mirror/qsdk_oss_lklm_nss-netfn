/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload.c
 *      Network Sock Offload Functionality Manager
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/debugfs.h>
#include <linux/if_arp.h>
#include <linux/if_pppox.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/inet_sock.h>
#include <net/protocol.h>

#include <netfn_flowmgr.h>
#include <ppe_drv.h>

#include "netfn_sk_offload_priv.h"

/*
 * We use the following global context
 * to store and use all the relevant
 * entities as parsed across various
 * files of the Sock Offload Module.
 *
 */
struct netfn_sk_offload g_sk_offload = {0};

int netfn_sk_offload_core_mask = NETFN_SOCK_OFFLOAD_RFS_CORES;
module_param(netfn_sk_offload_core_mask, int, S_IRUSR);
MODULE_PARM_DESC(netfn_sk_offload_core_mask, "Core mask to be used for RFS");

/*
 * netfn_sk_offload_ppe_get_macaddr()
 *	Fetch mac address using dst neigbour.
 */
static bool netfn_sk_offload_ppe_get_macaddr(struct dst_entry *dst, void *daddr, uint8_t *mac_addr)
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
 * netfn_sk_offload_ppe_get_vlan()
 *	Parse and fill VLAN information in create rule.
 */
static struct net_device *netfn_sk_offload_ppe_get_vlan(struct net_device *dev, struct netfn_flowmgr_vlan_rule *orig, struct netfn_flowmgr_vlan_rule *reply)
{
	struct net_device *next_dev;

	/*
	 * Check if the next isn't null
	 */
	next_dev = vlan_dev_next_dev(dev);
	if (!next_dev) {
		pr_warn("Cannot find the physical net_device\n");
		return NULL;
	}

	/*
	 * We don't want next dev to be VLAN
	 * Next dev should be ethernet device
	 */
	if (is_vlan_dev(next_dev) || (next_dev->type != ARPHRD_ETHER)) {
		pr_warn("%px: QinQ or non-ethernet VLAN master (%s) is not supported\n", dev,
				next_dev->name);
		return NULL;
	}

	orig->inner.egress_vlan_tag = reply->inner.ingress_vlan_tag = vlan_dev_vlan_id(dev);
	orig->inner.ingress_vlan_tag = reply->inner.egress_vlan_tag = 0xfff;
	orig->outer.egress_vlan_tag = reply->outer.ingress_vlan_tag = 0xfff;
	orig->outer.ingress_vlan_tag = reply->outer.egress_vlan_tag = 0xfff;

	pr_debug("%px: VLAN rule configured with dev (%s) vlan_id(%u)\n", dev, dev->name, vlan_dev_vlan_id(dev));
	return next_dev;
}

/*
 * netfn_sk_offload_ppe_get_pppoe()
 *	Parse and fill PPPoE information in create rule
 */
static struct net_device *netfn_sk_offload_ppe_get_pppoe(struct net_device *dev, struct netfn_flowmgr_pppoe_rule *orig, struct netfn_flowmgr_pppoe_rule *reply)
{
	struct ppp_channel *channel[1] = {NULL};
	struct net_device *next_dev = NULL;
	int px_proto, err = 0;
	struct pppoe_opt opt;
	int ppp_ch_count;

	if (ppp_is_multilink(dev)) {
		pr_warn("%px: channel is multilink PPP\n", dev);
		return false;
	}

	ppp_ch_count = ppp_hold_channels(dev, channel, 1);
	pr_debug("%px: PPP hold channel ret %d\n", dev, ppp_ch_count);
	if (ppp_ch_count != 1) {
		pr_warn("%px: hold channel for netdevice failed\n", dev);
		return false;
	}

	px_proto = ppp_channel_get_protocol(channel[0]);
	if (px_proto != PX_PROTO_OE) {
		pr_warn("%px: session socket is not of type PX_PROTO_OE\n", dev);
		goto release;
	}

	if (pppoe_channel_addressing_get(channel[0], &opt)) {
		pr_warn("%px: failed to get pppoe options information\n", dev);
		goto release;
	}

	/*
	 * pppoe_channel_addressing_get returns held device.
	 * And we are already holding parent device reference so release it.
	 */
	next_dev = opt.dev;
	dev_put(next_dev);

	/*
	 * Update the PPPoE rule information and set PPPoE valid flag.
	 */
	orig->session_id = reply->session_id = ntohs((uint16_t)opt.pa.sid);
	memcpy(orig->server_mac, opt.pa.remote, ETH_ALEN);
	memcpy(reply->server_mac, opt.pa.remote, ETH_ALEN);

	/*
	 * PPPoE interface can be created on linux bridge, OVS bridge and LAG devices.
	 * TCPST doesn't support these hierarchies.
	 */
	err += (next_dev->priv_flags & (IFF_EBRIDGE | IFF_OPENVSWITCH));
	err += ((next_dev->flags & IFF_MASTER) && (next_dev->priv_flags & IFF_BONDING));
	if (err) {
		pr_warn("%px: PPPoE over Bridge/LAG unsupported, dev: %s next_dev: %s\n",
				dev, dev->name, next_dev->name);
		next_dev = NULL;
		goto release;
	}

release:
	ppp_release_channels(channel, 1);
	pr_debug("%px: Update PPE PPPoE flow rule with session_id = %u, remote_mac = %pM\n",
			dev, orig->session_id, orig->server_mac);

	return next_dev;
}

/*
 * netfn_sk_offload_rule_create()
 *      Netfn Sock Offload Rule Creation
 */
netfn_flowmgr_ret_t netfn_sk_offload_rule_create(struct netfn_sk_offload_ctx *ctx)
{
	netfn_flowmgr_ret_t status = NETFN_FLOWMGR_RET_CREATE_RULE_FAILED;
	struct netfn_flowmgr_flow_info *orig_info, *reply_info;
	struct netfn_tuple_5tuple *orig_tuple, *reply_tuple;
	struct netfn_sk_offload *so = &g_sk_offload;
	struct netfn_flowmgr_create_rule reply = {0};
	struct netfn_flowmgr_create_rule orig = {0};
	struct net_device *out_dev;
	struct inet_sock *inet;
	struct dst_entry *dst;

	struct net_device *vp_dev = so->vp_dev;
	struct sock *sk = ctx->sk;

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
		return status;
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

	out_dev = dst->dev;
	orig.tuple.tuple_type = NETFN_TUPLE_5TUPLE;
	orig_tuple->l4_src_ident = inet->inet_sport;;
	orig_tuple->l4_dest_ident = inet->inet_dport;
	orig_tuple->protocol = sk->sk_protocol;

	ctx->tuple = orig.tuple;

	/*
	 * If PPPoE is present, fill PPPoE specific information in create rule
	 */
	if (out_dev->type == ARPHRD_PPP) {
		reply.rule_info.rule_valid_flags |= NETFN_FLOWMGR_VALID_RULE_FLAG_PPPOE;

		out_dev = netfn_sk_offload_ppe_get_pppoe(out_dev,  &orig.rule_info.pppoe_rule, &reply.rule_info.pppoe_rule);
		if (!out_dev) {
			pr_warn("%px: Failed to configure pppoe, dst_dev:%px\n", sk, dst->dev);
			goto release;
		}
	}

	/*
	 * If VLAN is present, fill VLAN specific information in create rule
	 */
	if (is_vlan_dev(out_dev)) {
		orig.rule_info.rule_valid_flags |= NETFN_FLOWMGR_VALID_RULE_FLAG_VLAN;
		reply.rule_info.rule_valid_flags |= NETFN_FLOWMGR_VALID_RULE_FLAG_VLAN;

		out_dev = netfn_sk_offload_ppe_get_vlan(out_dev, &orig.rule_info.vlan_rule, &reply.rule_info.vlan_rule);
		if (!out_dev) {
			pr_warn("%px: Failed to configure VLAN, dst_dev:%px\n", sk, dst->dev);
			goto release;
		}
	}

	/*
	 * Fill original device info
	 */
	orig_info->out_dev = out_dev;
	orig_info->top_outdev = dst->dev;
	orig_info->in_dev = orig_info->top_indev = vp_dev;
	orig_info->flow_mtu = READ_ONCE(out_dev->mtu);

	memcpy(orig_info->flow_src_mac, out_dev->dev_addr, ETH_ALEN);

	if (!netfn_sk_offload_ppe_get_macaddr(dst, &orig_tuple->dest_ip, orig_info->flow_dest_mac)) {
		pr_err("%px: Failed to get neigbour mac address, dst:%px\n", sk, dst);
		goto release;
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
	reply_info->in_dev = out_dev;
	reply_info->top_indev  = dst->dev;
	reply_info->out_dev = reply_info->top_outdev = vp_dev;
	reply_info->flow_mtu = orig_info->flow_mtu;

	/*
	 * VP Flags
	 */
	orig.flow_flags |= NETFN_FLOWMGR_FLOW_FLAG_VP_FLOW;
	reply.flow_flags |= NETFN_FLOWMGR_FLOW_FLAG_VP_FLOW;

	/*
	 * Flow_idx Flags
	 */
	orig.flow_flags |= NETFN_FLOWMGR_FLOW_SW_MDATA_VALID;
	reply.flow_flags |= NETFN_FLOWMGR_FLOW_SW_MDATA_VALID;

	status = netfn_flowmgr_rule_accel(&orig, &reply, NETFN_FLOWMGR_ACCEL_MODE_PPE);
	if (NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(status) != NETFN_FLOWMGR_RET_SUCCESS) {
		pr_err("%px: Failed to create flow rule status(%x)\n", sk, status);
		goto release;
	}

release:
	dst_release(dst);
	return status;
}

/*
 * netfn_sk_offload_rule_destroy()
 *      Netfn Sock Offload Rule Destroy
 */
static void netfn_sk_offload_rule_destroy(struct netfn_sk_offload_ctx *ctx)
{
	struct netfn_tuple_5tuple *reply_tuple, *orig_tuple;
	struct netfn_flowmgr_destroy_rule reply = {0};
	struct netfn_flowmgr_destroy_rule orig = {0};
	netfn_flowmgr_ret_t status;

	struct sock *sk = ctx->sk;

	reply_tuple = &reply.tuple.tuples.tuple_5;
	orig_tuple = &orig.tuple.tuples.tuple_5;

	orig.tuple = ctx->tuple;

	/*
	 * Reply is exact reverse of original.
	 */
	memcpy(&reply_tuple->dest_ip, &orig_tuple->src_ip, sizeof(reply_tuple->dest_ip));
	memcpy(&reply_tuple->src_ip, &orig_tuple->dest_ip, sizeof(reply_tuple->src_ip));

	reply_tuple->l4_dest_ident = orig_tuple->l4_src_ident;
	reply_tuple->l4_src_ident = orig_tuple->l4_dest_ident;
	reply_tuple->protocol = orig_tuple->protocol;
	reply.tuple.ip_version = orig.tuple.ip_version;
	reply.tuple.tuple_type = orig.tuple.tuple_type;

	status = netfn_flowmgr_rule_decel(&orig, &reply, NETFN_FLOWMGR_ACCEL_MODE_PPE);
	if (NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(status) != NETFN_FLOWMGR_RET_SUCCESS) {
		pr_warn("%px: Failed to destroy flow rule status(%x)\n", sk, status);
	}
}

/*
 * netfn_sk_offload_disable()
 *      Netfn Sock Offload Disable
 */
static void netfn_sk_offload_disable(struct sock *sk)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	struct netfn_sk_offload_stats *stats;
	struct netfn_sk_offload_ctx *ctx = NULL;

	stats = this_cpu_ptr(so->stats);

	ctx = xchg(&sk->sk_user_data, NULL);

	netfn_sk_offload_rule_destroy(ctx);
	netfn_sk_offload_db_del(ctx);
	free_percpu(ctx->stats);

	kfree(ctx);

	stats->ctx_dealloc++;
	atomic_dec(&so->num_conns);
}

/*
 * netfn_sk_offload_enable()
 *      Netfn Sock Offload Enable
 */
static bool netfn_sk_offload_enable(struct sock *sk)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	struct netfn_sk_offload_stats *stats;
        struct netfn_sk_offload_ctx *ctx = NULL;
        netfn_flowmgr_ret_t status;
        uint16_t flow_idx;

	stats = this_cpu_ptr(so->stats);

	/*
	 * Allocation for Context.
	 */
	ctx = kzalloc(sizeof(*ctx), GFP_ATOMIC);
	if (!ctx) {
		pr_err("%px: Failed to Allocate Context for the socket", sk);
		stats->ctx_alloc_fail_nomem++;
		return false;
	}

	ctx->sk = sk;

	/*
	 * Rx Initialization for Context.
	 */
	netfn_sk_offload_rx_init(ctx);

	/*
	 * Stats Initialization for Context.
	 */
	ctx->stats = alloc_percpu_gfp(struct netfn_sk_offload_stats_ctx, GFP_KERNEL | __GFP_ZERO);
	if (!ctx->stats) {
		pr_warn("%px: Failed to allocate stats memory for connection specific stats\n", ctx);
		stats->ctx_alloc_fail_stats++;
		goto fail1;
	}

	/*
	 * Rule creation for Context.
	 */
	status = netfn_sk_offload_rule_create(ctx);
	if (NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(status) != NETFN_FLOWMGR_RET_SUCCESS) {
		pr_err("%px: Failed to create flow rule, status(%x), ctx:%px\n", sk, status, ctx);
		stats->ctx_alloc_fail_rule++;
		goto fail2;
	}

	/*
	 * Fetch flow_idx for Context.
	 */
	flow_idx = NETFN_FLOWMGR_GET_FLOW_IDX(status);

	/*
	 * DB Addition for Context.
	 */
	if(!netfn_sk_offload_db_add(ctx, flow_idx)) {
		pr_err("%px: Failed to add ctx entry to DB table, ctx:%px\n", sk, ctx);
		stats->ctx_alloc_fail_max_conn++;
		goto fail3;
	}

	sk->sk_user_data =  ctx;
	stats->ctx_alloc++;
	atomic_inc(&so->num_conns);

	return true;

fail3:
	netfn_sk_offload_rule_destroy(ctx);
fail2:
	free_percpu(ctx->stats);
fail1:
        kfree(ctx);
	return false;
}

/*
 * netfn_sk_offload_valid()
 *      Netfn Sock Offload Offload Validity function.
 */
static bool netfn_sk_offload_valid(struct sock *sk)
{
	/*
	 * We do not support offloading when
	 * Iperf is started in 64bit mode.
	 */
	if (sk->offload_appid == NSS_SK_OFFLOAD_IPERF_64) {
		pr_err("%px: Failed to offload, unsupported App version:%d\n", sk, sk->offload_appid);
		return false;
	}

	/*
	 * We support offloading only for IPV4 and IPV6 versions.
	 */
        if (sk->sk_family != AF_INET && sk->sk_family != AF_INET6) {
		pr_err("%px: Failed to offload, unsupported ip_version:%d\n", sk, sk->sk_family);
		return false;
	}

	/*
	 * We support offloading only for TCP and UDP protocols.
	 */
	if (sk->sk_protocol != IPPROTO_TCP && sk->sk_protocol != IPPROTO_UDP) {
		pr_err("%px: Failed to offload, unsupported protocol:%d\n", sk, sk->sk_protocol);
		return false;
	}

	/*
	 * We support offloading only when GRO is enabled for UDP flows.
	 */
	if (sk->sk_protocol == IPPROTO_UDP && !udp_test_bit(GRO_ENABLED, sk)) {
		pr_err("%px: Failed to offload for UDP flows, GRO not enabled\n", sk);
		return false;
	}

	/*
	 * User might wrongly call setsock opt twice. If sk_user_data is set,
	 * we might have already offloaded the socket.
	 */
	if (sk->offload && sk->sk_user_data) {
		pr_err("%px: Failed to offload, socket is already offloaded\n", sk);
		return false;
	}

	/*
	 * sk->sk_user_data is not set meaning we have not offloaded this socket.
	 */
	if (!sk->offload && !sk->sk_user_data) {
		pr_err("%px: Failed to disable offload, socket was not been offloaded\n", sk);
		return false;
	}

	return true;
}

/*
 * netfn_sk_offload_notify()
 *      Netfn Sock Offload Notify function.
 */
static int netfn_sk_offload_notify(struct notifier_block *nb, unsigned long event, void *data)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	struct netfn_sk_offload_stats *stats;
	struct sock *sk = (struct sock *)data;
	int val = sk->offload;
	int appid = sk->offload_appid;

	stats = this_cpu_ptr(so->stats);

	if (val)
		stats->total_conns++;

	if(!netfn_sk_offload_valid(sk)) {
		stats->ctx_alloc_fail_sanity++;
		return NOTIFY_BAD;
	}

	pr_info("%px: Received notification with event(%lu), offload(%d), app_id(%d)\n",
			sk, event, val, appid);

	switch (event) {
	case SOCK_OFFLOAD_NOTIFY:
		if (val && (sk->sk_state == TCP_ESTABLISHED)) /* enable */
			return netfn_sk_offload_enable(sk) ? NOTIFY_OK : NOTIFY_BAD;

		if (!val) {	/* disable */
			netfn_sk_offload_disable(sk);
			return NOTIFY_OK;
		}

		pr_warn("%px: Unhandled event(%lu), offload:%d\n", sk, event, val);
		break;

	default:
		pr_warn("%px: Unhandled event(%lu)\n", sk, event);
		break;
	}

	return NOTIFY_OK;
}

/*
 * netfn_sk_offload_notifier
 *      Notifier Block for Socket Offload Event.
 */
static struct notifier_block netfn_sk_offload_notifier __attribute__((unused)) = {
	.notifier_call = netfn_sk_offload_notify,
};

/*
 * netfn_sk_offload_exit_module()
 *      Netfn Sock Offload Module exit function
 */
void __exit netfn_sk_offload_exit_module(void)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	int ret = 0;

	/*
	 * TODO:
	 * We ensure sufficient cleanup is done
	 * for all the active contexts before
	 * module goes away.
	 */

	ret = sock_notify_unregister(&netfn_sk_offload_notifier);
	if (ret) {
		pr_err("Failed to un-register from AF_INET Socket Offload Notifier, ret:%d\n", ret);
		return;
	}

	/*
	 * TODO:
	 * 2. De-allocate the VP.
	 */

	free_percpu(so->stats);
	netfn_sk_offload_stats_deinit();

	pr_info("Unloading the Netfn Sock Offload Module\n");
}

/*
 * netfn_sk_offload_init_module()
 *      Netfn Sock Offload Module init function
 */
int __init netfn_sk_offload_init_module(void)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	int ret = 0;

	pr_info("Netfn Sock Offload Module loaded: (%s)\n", NSS_NETFN_BUILD_ID);

	ret = sock_notify_register(&netfn_sk_offload_notifier);
	if (ret) {
		pr_err("Failed to register to AF_INET Socket Offload Notifier, ret:%d\n", ret);
		return ret;
	}

	/*
	 * TODO:
	 * 2. Allocate the VP.
	 */

	so->dentry = debugfs_create_dir("qca-nss-netfn-sk-offload", NULL);
	if (!so->dentry) {
		pr_warn("%p:Failed to create debugfs_entry for sock offload module\n", so);
	}

	so->stats = alloc_percpu_gfp(struct netfn_sk_offload_stats, GFP_KERNEL | __GFP_ZERO);
	if (!so->stats) {
		pr_warn("%px: Failed to allocate stats memory\n", so);
		netfn_sk_offload_stats_deinit();
	}

	if (netfn_sk_offload_stats_init() < 0) {
		free_percpu(so->stats);
		netfn_sk_offload_stats_deinit();
	}

	bitmap_zero(so->sk_bitmap, NETFN_SK_OFFLOAD_MAX_CONNS);
	atomic_set(&so->num_conns, 0);

	return ret;
}

module_init(netfn_sk_offload_init_module);
module_exit(netfn_sk_offload_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Network Sock Offload Module");
