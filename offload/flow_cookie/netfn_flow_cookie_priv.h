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

/*
 * @file netfn_flow_cookie_priv.h
 * Netfn Flow Cookie Functionality Manager header file.
 */

#include "netfn_flow_cookie_db.h"

#ifndef __NETFN_FLOW_COOKIE_H
#define __NETFN_FLOW_COOKIE_H

#define NETFN_FLOW_COOKIE_DEBUG_LEVEL 3

extern uint32_t static_dbg_level;

/*
 * netfn_flow_cookie_static_dbg_level
 *	Netfn flow cookie static debug level
 */
enum netfn_flow_cookie_static_dbg_level {
	NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_NONE,
	NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_WARN,
	NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_INFO,
	NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_TRACE,
};

/*
 * netfn_flow_cookie_gbl
 *	 Netfn flow cookie global context.
 */
struct netfn_flow_cookie_gbl {
	struct dentry *dentry;								/* Debugfs entry */
	struct ctl_table_header *netfn_flow_cookie_sysctl_header;			/* Netfn Flow Cookie Sysctl */
	struct netfn_flow_cookie_db __rcu *db_array[NETFN_FLOW_COOKIE_DB_ARRAY_SIZE];	/* DB Array */
	atomic64_t gbl_base_stats[NETFN_FLOW_COOKIE_GBL_STATS_MAX];			/* Global Stats */
	spinlock_t lock;								/* Common Spin Lock */
};

extern struct netfn_flow_cookie_gbl netfn_flow_cookie_gbl_ctx;

#if (NETFN_FLOW_COOKIE_DEBUG_LEVEL < 1)
#define netfn_flow_cookie_assert(c, s, ...)
#else
#define netfn_flow_cookie_assert(c, s, ...) {	\
	if (!(c)) {	\
		printk(KERN_CRIT "%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
		BUG_ON(!(c)); \
	} \
}
#endif /* NETFN_FLOW_COOKIE_DEBUG_LEVEL */

/*
 * Compile messages for dynamic enable/disable
 */
#if defined(CONFIG_DYNAMIC_DEBUG)
#define netfn_flow_cookie_warn(s, ...)      pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define netfn_flow_cookie_info(s, ...)      pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define netfn_flow_cookie_trace(s, ...)     pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else /* CONFIG_DYNAMIC_DEBUG */
/*
 * Statically compile messages at different levels
 */
#if (NETFN_FLOW_COOKIE_DEBUG_LEVEL < 2)
#define netfn_flow_cookie_warn(s, ...)
#else
#define netfn_flow_cookie_warn(s, ...) {	\
	if (static_dbg_level >= NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_WARN) \
		pr_warn("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__) \
}
#endif

#if (NETFN_FLOW_COOKIE_DEBUG_LEVEL < 3)
#define netfn_flow_cookie_info(s, ...)
#else
#define netfn_flow_cookie_info(s, ...) {	\
	if (static_dbg_level >= NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_INFO) \
		pr_notice("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__) \
}
#endif

#if (NETFN_FLOW_COOKIE_DEBUG_LEVEL < 4)
#define netfn_flow_cookie_trace(s, ...)
#else
#define netfn_flow_cookie_trace(s, ...) {	\
	if (static_dbg_level >= NETFN_FLOW_COOKIE_STATIC_DBG_LEVEL_TRACE) \
		pr_info("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__) \
}
#endif
#endif /* CONFIG_DYNAMIC_DEBUG */

#endif /*__NETFN_FLOW_COOKIE_H */
