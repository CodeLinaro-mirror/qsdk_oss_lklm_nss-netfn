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
	 * Invoke the appropriate netfn_sk_rcv callback API
	 * as set during Rx Init (ctx->sk_rcv).
	 */

	return ret;
}

/*
 * netfn_sk_offload_rx_init()
 *      Netfn Sock Offload Rx Initialization
 */
void netfn_sk_offload_rx_init(struct netfn_sk_offload_ctx *ctx)
{

	/*
	 * TODO:
	 * Initialize Rx related information in context
	 * basing on Protocol and IP_VER.
	 */
}
