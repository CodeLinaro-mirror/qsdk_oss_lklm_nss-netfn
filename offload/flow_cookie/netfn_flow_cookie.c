/*
 **************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * netfn_flow_cookie.c
 *      Network Flow Cookie Functionality Manager
 */
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <netfn_flow_cookie.h>
#include "netfn_flow_cookie_priv.h"
#include "netfn_flow_cookie_dump.h"

#define NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_STR_LEN 8

/*
 * We use the following entities to store the
 * state as configured from sysctl command
 * for the statically controlled logging purposes.
 */
uint32_t static_dbg_level;

static char static_dbg_level_str[NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_STR_LEN];

/*
 * We use the following global context
 * to store and use all the relevant
 * entities as parsed across various
 * files of the Flow Cookie Module.
 *
 */
struct netfn_flow_cookie_gbl netfn_flow_cookie_gbl_ctx = {0};

/*
 * netfn_flow_cookie_static_dbg_level_handler()
 *	Set static debug level for netfn flow cookie.
 */
static int netfn_flow_cookie_static_dbg_level_handler(struct ctl_table *table,
		int write, void __user *buffer,
		size_t *lenp, loff_t *ppos)
{
	int ret;
	char *level_str;
	enum netfn_flow_cookie_static_dbg_level dbg_level;

	/*
	 * Find the string, return an error if not found
	 */
	ret = proc_dostring(table, write, buffer, lenp, ppos);
	if (ret || !write) {
		return ret;
	}

	level_str = static_dbg_level_str;
	pr_err("dbg_level: %s", level_str);

	if (!strcmp(level_str, "warn")) {
		dbg_level = NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_WARN;
	} else if (!strcmp(level_str, "info")) {
		dbg_level = NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_INFO;
	} else if (!strcmp(level_str, "trace")) {
		dbg_level = NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_TRACE;
	} else if (!strcmp(level_str, "none")) {
		dbg_level = NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_NONE;
	} else {
		pr_err("Usage: echo '[warn|info|trace|none]' > /proc/sys/netfn-flow-cookie/static_dbg_level\n");
		return -EINVAL;
	}

	if (dbg_level >= NETFN_FLOW_COOKIE_DEBUG_LEVEL) {
		pr_err("debug level: %d not compiled in: %d\n", dbg_level, NETFN_FLOW_COOKIE_DEBUG_LEVEL);
		return -EINVAL;
	}

	static_dbg_level = dbg_level;
	return ret;
}

/*
 * netfn_flow_cookie_sub
 *	Netfn Flow Cookie sub directory
 */
static struct ctl_table netfn_flow_cookie_sub[] = {
	{
		.procname       =       "static_dbg_level",
		.data           =       &static_dbg_level_str,
		.maxlen         =       sizeof(char) * NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_STR_LEN,
		.mode           =       0644,
		.proc_handler   =       netfn_flow_cookie_static_dbg_level_handler
	},
	{}
};

/*
 * netfn_flow_cookie_exit_module()
 *      Netfn Flow Cookie Functionality Manager exit function
 */
void __exit netfn_flow_cookie_exit_module(void)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;

	/*
	 * De-Initializie the Debugfs Stats Infra.
	 */
	netfn_flow_cookie_stats_exit();

	/*
	 * De-Initialize the Flow dump implementation.
	 */
	netfn_flow_cookie_dump_exit();

	/*
	 * Unregister sysctl framework
	 */
	unregister_sysctl_table(nfc_ctx->netfn_flow_cookie_sysctl_header);
	nfc_ctx->netfn_flow_cookie_sysctl_header = NULL;

	netfn_flow_cookie_info("Unloading the Netfn Flow Cookie Module\n");
}

/*
 * netfn_flow_cookie_init_module()
 *      Netfn Flow Cookie Functionality Manager init function
 */
int __init netfn_flow_cookie_init_module(void)
{
	int ret = 0;
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;

	/*
	 * Flow Cookie Module Debugfs Stats Initialization.
	 */
	netfn_flow_cookie_stats_init();

	/*
	 * Flow dump implementation.
	 */
	netfn_flow_cookie_dump_init();
	netfn_flow_cookie_info("Done with netfn flow dump initialization");

	/*
	 * Register sysctl framework
	 */
	nfc_ctx->netfn_flow_cookie_sysctl_header = register_sysctl("netfn-flow-cookie", netfn_flow_cookie_sub);

	if (!nfc_ctx->netfn_flow_cookie_sysctl_header) {
		netfn_flow_cookie_warn("Sysctl table configuration failed for Netfn Flow Cookie Module");
		return -EINVAL;
	}

	netfn_flow_cookie_info("NETFN FLOW COOKIE MODULE loaded: (%s)\n", NSS_NETFN_BUILD_ID);

	return ret;
}

module_init(netfn_flow_cookie_init_module);
module_exit(netfn_flow_cookie_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Network Flow Cookie Module Functionality manager");

