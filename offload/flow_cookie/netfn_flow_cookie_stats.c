/*
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

#include <linux/debugfs.h>
#include <linux/netdevice.h>
#include <netfn_flow_cookie.h>
#include "netfn_flow_cookie_priv.h"

static const char *netfn_flow_cookie_stats_base_str[] = {
	"Flow DB init ops successful", /* Flow DB init operations successful */
	"Flow DB init ops failed",       /* Flow DB init operations failed */
	"Flow DB init ops failed due to unavailable memory", /* Flow DB init operations failed due to memory issues */
	"Flow DB init ops failed due to other reasons",  /* Flow DB init operations failed due to other reasons */
	"Flow DBs active",		/* Active no.of Flow DBs */
	"Flow DB Free ops performed successfully",	/* Flow DB Free operations successful*/
	"Flow DB Free ops failed",      /* Flow DB Free operations failed*/
	"Flows active for the DBs",	/* Active no.of Flows added to DBs */
	"Flow rule add ops successful", /* Flow rule add operations successful */
	"Flow rule add ops failed", /* Flow rule add operations failed */
	"Flow rule add ops failed due to unavailable memory", /* Flow rule add operations failed due to memory issues*/
	"Flow rule add ops failed due to other reasons", /* Flow rule add operations failed due to other reasons */
	"Flow rule delete ops successful", /* Flow rule delete operations successful */
	"Flow rule delete ops failed", /* Flow rule delete operations failed */
	"Flow ID fetch calls successful", /* Flow ID fetch calls successful */
	"Flow ID fetch calls failed", /* Flow ID fetch calls failed */
	"Sawf handle fetch calls successful", /* Sawf handle fetch calls successful */
	"Sawf handle fetch calls failed", /* Sawf handle fetch calls failed */
	"Flow Mark fetch calls successful", /* Flow Mark fetch calls successful */
	"Flow Mark fetch calls failed", /* Flow Mark fetch calls failed */
	"SCS ID fetch calls successful", /* SCS ID fetch calls successful */
	"SCS ID fetch calls failed", /* SCS ID fetch calls failed */
	"Flow Cookie fetch calls successful", /* Flow Cookie fetch calls successful */
	"Flow Cookie fetch calls failed", /* Flow Cookie fetch calls failed */
	"Flow Hit counts", /* Hit count for various fetch calls for the entire DB */
};

static const char *netfn_flow_cookie_stats_base_db_str[] = {
	"Flow DB Free ops failed",      /* Flow DB Free operations failed*/
	"Flows active for the DB",     /* Active no.of Flows added to this DB */
	"Flow rule add ops successful", /* Flow rule add operations successful */
	"Flow rule add ops failed", /* Flow rule add operations failed */
	"Flow rule add ops failed due to unavailable memory", /* Flow rule add operations failed due to memory issues*/
	"Flow rule add ops failed due to other reasons", /* Flow rule add operations failed due to other reasons */
	"Flow rule delete ops successful", /* Flow rule delete operations successful */
	"Flow rule delete ops failed", /* Flow rule delete operations failed */
	"Flow ID fetch calls successful", /* Flow ID fetch calls successful */
	"Flow ID fetch calls failed", /* Flow ID fetch calls failed */
	"Sawf handle fetch calls successful", /* Sawf handle fetch calls successful */
	"Sawf handle fetch calls failed", /* Sawf handle fetch calls failed */
	"Flow Mark fetch calls successful", /* Flow Mark fetch calls successful */
	"Flow Mark fetch calls failed", /* Flow Mark fetch calls failed */
	"SCS ID fetch calls successful", /* SCS ID fetch calls successful */
	"SCS ID fetch calls failed", /* SCS ID fetch calls failed */
	"Flow Cookie fetch calls successful", /* Flow Cookie fetch calls successful */
	"Flow Cookie fetch calls failed", /* Flow Cookie fetch calls failed */
	"Flow Hit counts", /* Hit count for various fetch calls for this DB */
};

/*
 * netfn_flow_cookie_stats_reset_global
 * 	API to reset the flow cookie stats.
 *
 * @param[in]
 * void
 *
 * @return
 * void
 *
 */
void netfn_flow_cookie_stats_reset_global(void)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint32_t i;

	for (i = 0; i < NETFN_FLOW_COOKIE_GBL_STATS_MAX; i++) {
		atomic64_set(&nfc_ctx->gbl_base_stats[i], 0);
	}

	return;
}

/*
 * netfn_flow_cookie_stats_show_global
 *	API to dump the flow cookie stats in the appropriate file.
 *
 * @param[in]
 * File attributes
 *
 * @return
 * int returning Success/Failure
 *
 */
static int netfn_flow_cookie_stats_show_global(struct seq_file *m, void __attribute__((unused))*p)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint32_t i;

	/*
	 * Start displaying the Netfn Flow Cookie Base stats.
	 */
	seq_printf(m, "\n################ Netfn Flow Cookie Generic Statistics Start ################\n");
	seq_printf(m, "\nBase Netfn Flow Cookie Statistics:\n");

	for (i = 0; i < NETFN_FLOW_COOKIE_GBL_STATS_MAX; i++) {
		seq_printf(m, "\t[%s]:  %llu\n", netfn_flow_cookie_stats_base_str[i], atomic64_read(&nfc_ctx->gbl_base_stats[i]));
	}

	seq_printf(m, "\n################ Netfn Flow Cookie Generic Statistics End ################\n\n");

	return 0;
}

/*
 * netfn_flow_cookie_stats_open_global
 * 	API to perform the flow cookie stats file open operation.
 *
 * @param[in]
 * File attributes
 *
 * @return
 * int returning Success/Failure
 *
 */
static int netfn_flow_cookie_stats_open_global(struct inode *inode, struct file *file)
{
	return single_open(file, netfn_flow_cookie_stats_show_global, inode->i_private);
}

/*
 * netfn_flow_cookie_stats_ops_global
 *	File operations for Netfn Flow Cookie Base stats Global.
 */
static const struct file_operations netfn_flow_cookie_stats_ops_global = {
		.open = netfn_flow_cookie_stats_open_global,
		.read = seq_read,
		.llseek = seq_lseek,
		.release = seq_release,
};

/*
 * netfn_flow_cookie_stats_reset_db
 * 	API to reset the flow cookie db stats
 * 	for the specific DB whose DB handle is provided.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 *
 * @return
 * void
 *
 */
void netfn_flow_cookie_stats_reset_db(struct netfn_flow_cookie_db *d)
{
	uint32_t i;

	for (i = 0; i < NETFN_FLOW_COOKIE_DB_STATS_MAX; i++) {
		atomic64_set(&d->db_base_stats[i], 0);
	}

	return;
}

/*
 * netfn_flow_cookie_stats_reset_all_dbs
 *	API to reset the flow cookie db stats for all the DBs.
 *
 * @param[in]
 * void
 *
 * @return
 * void
 *
 */
void netfn_flow_cookie_stats_reset_all_dbs(void)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint8_t id;
	uint32_t i;

	for (id = 0; id < NETFN_FLOW_COOKIE_DB_ARRAY_SIZE; id = id + 1) {

		if (nfc_ctx->db_array[id] == NULL) {
			continue;
		}

		for (i = 0; i < NETFN_FLOW_COOKIE_DB_STATS_MAX; i++) {
			atomic64_set(&nfc_ctx->db_array[id]->db_base_stats[i], 0);
		}
	}

	return;
}

/*
 * netfn_flow_cookie_stats_get_db
 *	API to retrieve the flow cookie db stats for the specific DB.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_stats_get_db(struct netfn_flow_cookie_db *d, atomic64_t db_stats[NETFN_FLOW_COOKIE_DB_STATS_MAX])
{
	uint32_t i;

	if (!d) {
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	/*
	 * Start copying the Netfn Flow Cookie DB Stats.
	 */
	for (i = 0; i < NETFN_FLOW_COOKIE_DB_STATS_MAX; i++) {
		atomic64_set(&db_stats[i], atomic64_read(&d->db_base_stats[i]));
	}

	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;
}

/*
 * netfn_flow_cookie_stats_show_db
 *	API to dump the flow cookie stats in the appropriate file.
 *
 * @param[in]
 * File attributes
 *
 * @return
 * int returning Success/Failure
 *
 */
static int netfn_flow_cookie_stats_show_db(struct seq_file *m, void __attribute__((unused))*p)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	uint8_t id;
	uint32_t i;

	for (id = 0; id < NETFN_FLOW_COOKIE_DB_ARRAY_SIZE; id = id + 1) {

		if (nfc_ctx->db_array[id] == NULL) {
			continue;
		}

		/*
		 * Start displaying the Netfn Flow Cookie Base stats.
		 */
		seq_printf(m, "\n################ Netfn Flow Cookie DB %d Statistics Start ################\n", id);
		seq_printf(m, "\nBase Netfn Flow Cookie Statistics:\n");
		for (i = 0; i < NETFN_FLOW_COOKIE_DB_STATS_MAX; i++) {
			seq_printf(m, "\t[%s]:  %llu\n", netfn_flow_cookie_stats_base_db_str[i], atomic64_read(&nfc_ctx->db_array[id]->db_base_stats[i]));
		}

		seq_printf(m, "\n################ Netfn Flow Cookie DB %d Statistics End ################\n\n", id);
	}

	return 0;
}

/*
 * netfn_flow_cookie_stats_open_db
 *	API to perform the flow cookie stats file open operation.
 *
 * @param[in]
 * File attributes
 *
 * @return
 * int returning Success/Failure
 *
 */
static int netfn_flow_cookie_stats_open_db(struct inode *inode, struct file *file)
{
	return single_open(file, netfn_flow_cookie_stats_show_db, inode->i_private);
}

/*
 * netfn_flow_cookie_stats_ops_db
 *	File operations for Netfn Flow Cookie DB Base stats
 */
static const struct file_operations netfn_flow_cookie_stats_ops_db = {
	.open = netfn_flow_cookie_stats_open_db,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*
 * netfn_flow_cookie_stats_init
 *	Initialize the infra for the Netfn Flow Cookie Global and DB Specific Stats.
 *
 * @param[in] void
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
netfn_flow_cookie_status_t netfn_flow_cookie_stats_init(void)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;

	nfc_ctx->dentry = debugfs_create_dir("qca-nss-netfn-flow-cookie", NULL);
	if (!nfc_ctx->dentry) {
		netfn_flow_cookie_warn("%p: Unable to create qca-nss-netfn-flow-cookie debugfs stats directory in flow cookie\n", nfc_ctx);
		return NETFN_FLOW_COOKIE_STATUS_FAILURE;
	}

	if (!debugfs_create_file("global_stats", S_IRUGO, nfc_ctx->dentry,  NULL, &netfn_flow_cookie_stats_ops_global)) {
		netfn_flow_cookie_warn("%px: Failed to create debugfs entry for Netfn Flow Cookie Global stats\n", nfc_ctx);
		goto debugfs_stats_init_failed;
	}

	netfn_flow_cookie_info("%px: Done with initializing Netfn Flow Cookie Global Stats\n", nfc_ctx);

	if (!debugfs_create_file("db_stats", S_IRUGO, nfc_ctx->dentry, NULL, &netfn_flow_cookie_stats_ops_db)) {
		netfn_flow_cookie_warn("%px: Failed to create debugfs entry for Netfn Flow Cookie DB stats\n", nfc_ctx);
		goto debugfs_stats_init_failed;
	}

	netfn_flow_cookie_info("%px: Done with initializing Netfn Flow Cookie DB Stats\n", nfc_ctx);

	return NETFN_FLOW_COOKIE_STATUS_SUCCESS;

debugfs_stats_init_failed:
	debugfs_remove_recursive(nfc_ctx->dentry);
	nfc_ctx->dentry = NULL;
	return NETFN_FLOW_COOKIE_STATUS_FAILURE;
}

/*
 * netfn_flow_cookie_stats_exit
 *	De-Initialize the infra for the Netfn Flow Cookie Global and DB Specific Stats.
 *
 * @param[in] void
 *
 * @return
 * void
 *
 */
void netfn_flow_cookie_stats_exit(void)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;

	if (nfc_ctx->dentry) {
		debugfs_remove_recursive(nfc_ctx->dentry);
		nfc_ctx->dentry = NULL;
	}

	return;
}
