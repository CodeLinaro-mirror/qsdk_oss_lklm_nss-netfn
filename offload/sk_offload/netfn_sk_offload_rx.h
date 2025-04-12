/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * @file netfn_sk_offload_rx.h
 * Netfn Sock Offload Rx processing.
 */

/*
 * netfn_sk_offload_rx_init()
 *      Netfn Sock Offload Rx Initialization
 */
void netfn_sk_offload_rx_init(struct netfn_sk_offload_ctx *ctx);

/*
 * netfn_sk_offload_rx()
 *      Netfn Sock Offload Rx Processing.
 */
bool netfn_sk_offload_rx(struct ppe_vp_cb_info *info, void *cb_data);
