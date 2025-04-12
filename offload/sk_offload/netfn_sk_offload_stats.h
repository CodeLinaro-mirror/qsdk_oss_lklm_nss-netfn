/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload_stats.h
 *      Network Sock Offload Flow Statistics.
 */

#define NETFN_SOCK_OFFLOAD_MAX_STR_LEN 64       /* Maximum print lenght */

struct netfn_sk_offload_ctx;

/*
 * netfn_sk_offload_stats
 *	Debug statistics for all the connections.
 */
struct netfn_sk_offload_stats {
	uint64_t total_conns;			/* Total connections */
	uint64_t ctx_alloc;			/* Context alloc success */
	uint64_t ctx_alloc_fail_nomem;		/* Context alloc fail due to no memory */
	uint64_t ctx_alloc_fail_max_conn; 	/* Context alloc fail due to Max Conns */
	uint64_t ctx_alloc_fail_stats;		/* Context alloc fail due to stats failure */
	uint64_t ctx_alloc_fail_rule;		/* Context alloc fail due to Rule failure */
	uint64_t ctx_alloc_fail_sanity;		/* Context alloc fail due to Sanity failure */
	uint64_t ctx_dealloc;			/* Context dealloc success */
	uint64_t rx_pkts;			/* Rx pkts received */
	uint64_t rx_bytes;			/* Rx bytes received */
	uint64_t tx_pkts;			/* Tx pkts sent */
	uint64_t tx_bytes;			/* Tx bytes sent */
};

/*
 * netfn_sk_offload_stats_ctx
 *	Statistics per connection.
 */
struct netfn_sk_offload_stats_ctx {
	uint64_t rx_pkts;			/* Total pkts rcvd */
	uint64_t rx_bytes;			/* Total bytes rcvd */
	uint64_t dst_miss_pkts;			/* Total times dst wasn't found */
	uint64_t tx_pkts;			/* Tx pkts sent */
	uint64_t tx_bytes;			/* Tx bytes sent */
};

/*
 * Netfn Sock Offload Global stats APIs.
 */
int netfn_sk_offload_stats_init(void);
void netfn_sk_offload_stats_deinit(void);
