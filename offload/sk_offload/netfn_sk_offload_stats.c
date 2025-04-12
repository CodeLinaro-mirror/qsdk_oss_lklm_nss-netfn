/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload_stats.c
 *      Network Sock Offload Flow Statistics.
 */

#include <linux/debugfs.h>
#include <linux/netdevice.h>
#include "netfn_sk_offload_priv.h"

/*
 * netfn_sk_offload_stats_str
 *      Debug statistics global.
 */
static const char *netfn_sk_offload_stats_str[] __attribute__((unused)) = {
	"Total connections",		/* Total connections */
	"Ctx Alloc Success",		/* Context alloc success */
	"Ctx Alloc fail No Memory",	/* Context alloc fail due to no memory */
	"Ctx Alloc fail Max Conns",	/* Context alloc fail due to Max Conns */
	"Ctx Alloc fail Stats",		/* Context alloc fail due to stats failure */
	"Ctx Alloc fail Rule",		/* Context alloc fail due to Rule failure */
	"Ctx Alloc fail Sanity",	/* Context alloc fail due to Sanity failure */
	"Ctx Dealloc Success",		/* Context dealloc success */
	"Rx Packets",			/* Rx pkts received */
	"Rx Bytes",			/* Rx bytes received */
	"Tx Packets",			/* Tx pkts sent */
	"Tx Bytes",			/* Tx bytes sent */
};

/*
 * netfn_sk_offload_stats_ctx_str
 *      Debug statistics per connection.
 */
static const char *netfn_sk_offload_stats_ctx_str[] __attribute__((unused)) = {
	"Rx packets",				/* Rx pkts received */
	"Rx Bytes",				/* Rx Bytes received */
	"Dst miss packets",			/* Destination failure packets */
	"Tx packets",				/* Tx pkts sent */
	"Tx Bytes",				/* Tx Bytes sent */
};

/*
 * netfn_sk_offload_stats_init()
 *	Initialize the Netfn Sock Offload Global Stats.
 */
int netfn_sk_offload_stats_init(void)
{
	/*
	 * Implement the stats init functionality.
	 */
	return 0;
}

/*
 * netfn_sk_offload_stats_deinit()
 *	De-Initialize the Netfn Sock Offload Global Stats.
 */
void netfn_sk_offload_stats_deinit(void)
{
	/*
	 * Implement the stats de-init functionality.
	 */
	return;
}
