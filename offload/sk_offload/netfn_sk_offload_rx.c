/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload_rx.c
 *      Network Sock Offload Receive Processing.
 */
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <net/protocol.h>

#include "netfn_sk_offload_priv.h"

/*
 * netfn_sk_offload_rx()
 *      Netfn Sock Offload Rx processing
 */
bool netfn_sk_offload_rx(struct ppe_vp_cb_info *info, void *cb_data)
{
	int ret = 0;

	/*
	 * TODO:
	 * Invoke the appropriate callback API
	 * as set during Rx Init.
	 */

	return ret;
}

/*
 * netfn_sk_offload_rx_init()
 *      Netfn Sock Offload Rx Initialization
 */
void netfn_sk_offload_rx_init(struct netfn_sk_offload_ctx *ctx)
{
	int (*proto_handler)(struct sk_buff *skb);
	struct sock *sk = ctx->sk;
	u16 proto = sk->sk_protocol;
	unsigned short family = sk->sk_family;

	rcu_read_lock();
	switch (proto) {
	case IPPROTO_TCP:
	case IPPROTO_UDP:
		if (family == AF_INET) {
			struct net_protocol *proto4 = rcu_dereference(inet_protos[proto]);
			proto_handler = proto4->handler;
		} else {
			struct inet6_protocol * proto6 = rcu_dereference(inet6_protos[proto]);
			proto_handler = proto6->handler;
		}

		ctx->gro_rcv = NULL;
		ctx->kernel_proto_handler = proto_handler;
		break;
	default:
		pr_err("Unhandled protocol (%d) for Socket offload\n", proto);
		break;
	}
	rcu_read_unlock();
}
