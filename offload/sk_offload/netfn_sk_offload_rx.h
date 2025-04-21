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
 * netfn_sk_offload_rx_action
 *	GRO coalesce check actions
 */
enum netfn_sk_offload_rx_action {
	NETFN_SK_OFFLOAD_RX_GRO_NONE,		/* No action */
	NETFN_SK_OFFLOAD_RX_GRO_SKIP,           /* Skip GRO */
	NETFN_SK_OFFLOAD_RX_GRO_START,          /* Start new GRO chain */
	NETFN_SK_OFFLOAD_RX_GRO_QUEUE,          /* Coalesce packets */
	NETFN_SK_OFFLOAD_RX_GRO_FLUSH,          /* Coalesce packet and Flush chain */
	NETFN_SK_OFFLAOD_RX_GRO_RESTART,        /* Flush the existing GRO chain and start new chain */
};

/*
 * netfn_sk_offload_rx_state
 *  GRO state per connection
 */
struct netfn_sk_offload_rx_state {
	struct {
                struct sk_buff *skb;           /* First packet in the GRO list*/
                uint32_t len;                   /* Total packet length */
                uint32_t truesize;              /* Total truesize of all SKBs */
        } head_cache;

	struct sk_buff_head frag_q;		/* Fragment queue for rest of the chain */

	union {
		struct {
			uint16_t hlen;		/* TCP header length */
			uint32_t flag_word;	/* Flag word of head TCP header */
			uint32_t ack_seq;	/* Ack sequence in head TCP header */
			uint32_t next_seq;	/* Expected next data sequence */
		} tcp;

		struct {
			uint16_t data_len;	/* UDP Payload length */
			uint16_t next_seq;	/* Payload sequence number if available */
		} udp;
	};
};
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
void __rx_gro_purge(struct netfn_sk_offload_ctx *ctx);
