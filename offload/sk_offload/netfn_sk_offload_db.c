/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload_db.c
 *      Network Sock Offload Flow DB Management.
 */
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include "netfn_sk_offload_priv.h"

/*
 * netfn_sk_offload_db_add
 * 	Store the connection info against the corresponding flow index
 */
bool netfn_sk_offload_db_add(struct netfn_sk_offload_ctx *ctx, uint16_t flow_idx)
{

	/*
	 * TODO:
	 * Perform below operations under mutex lock
	 * 1. Check if entry is already stored in DB array.
	 * 2. Find the free index in the DB array.
	 * 3. Initialize the DB entry with ctx and flow index.
	 * 4. Store the entry in the DB array.
	 */

	return true;
}

/*
 * netfn_sk_offload_db_del
 *      Delete the connection info for the corresponding flow index
 */
void netfn_sk_offload_db_del(struct netfn_sk_offload_ctx *ctx)
{

	/*
	 * TODO:
	 * Perform below operations under mutex lock
	 * 1. Find the entry corresponding to the flow_idx.
	 * 2. Mark the entry to Null in the DB Array.
	 * 3. De-Initialize the DB entry.
	 */

	return;
}


/*
 * netfn_sk_offload_db_get_ctx
 * 	Retrieve the ctx associated with the flow index.
 */
struct netfn_sk_offload_ctx *netfn_sk_offload_db_get_ctx(uint16_t flow_idx)
{
	struct netfn_sk_offload_ctx *ctx = NULL;

	/*
	 * TODO:
	 * Perform below operations under RCU lock.
	 * 1. Find the DB entry for the flow index by performing the lookup.
	 * 2. Extract the ctx info from the entry.
	 */

	return ctx;
}
