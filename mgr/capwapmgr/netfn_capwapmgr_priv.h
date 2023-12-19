/*
 * netfn_capwapmgr_priv.h
 *	Network function's CAPWAP manager private header file.
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __NETFN_CAPWAPMGR_PRIV_H
#define __NETFN_CAPWAPMGR_PRIV_H

#include <linux/spinlock.h>
#include <linux/if_ether.h>
#include <linux/kref.h>

#include <netfn_capwap.h>
#include <netfn_flow.h>
#include <netfn_dtls.h>

/*
 * CAPWAP manager debug macros
 */
#if (NETFN_CAPWAPMGR_DEBUG_LEVEL == 3)
#define netfn_capwapmgr_assert(c, s, ...)
#else
#define netfn_capwapmgr_assert(c, s, ...) if (!(c)) { printk(KERN_CRIT "%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__); BUG_ON(!(c)); }
#endif

/*
 * If dynamic debug is enabled, use pr_debug.
 */
#define netfn_capwapmgr_warn(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define netfn_capwapmgr_info(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define netfn_capwapmgr_trace(s, ...) pr_debug("%s[%d]:" s, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define NETFN_CAPWAPMGR_STATS_SYNC_TIMER_CORE 3	/* Default value for stats sync timer core */

/*
 * netfn_capwapmgr_stats
 *	Capwap manager stats.
 */
struct netfn_capwapmgr_stats {
	atomic_t tun_dev_alloc;		/* Number of tunnels allocated */
	atomic_t tun_dev_free;		/* Number of tunnels freed */
	atomic_t tunid_dev_alloc;	/* Number of tunid dev allocated */
	atomic_t tunid_dev_free;	/* Number of tunid dev freed */
	atomic64_t error_stats[NETFN_CAPWAPMGR_ERROR_MAX];
					/* Error stats */
};

/*
 * netfn_capwapmgr
 *	Global instance.
 */
struct netfn_capwapmgr {
	struct kref ref;			/* Driver reference */
	struct completion completion;		/* Completion to wait for freeing the driver */
	spinlock_t lock;			/* Global lock */
	struct dentry *dentry;			/* Debugfs for driver stats */
	struct netfn_capwapmgr_stats stats;	/* Stats */
};

extern struct netfn_capwapmgr g_mgr;	/* Global manager object */
extern void netfn_capwapmgr_final(struct kref *kref);

/*
 * netfn_capwapmgr_drv_ref()
 * 	Increment driver object reference
 */
static inline struct netfn_capwapmgr *netfn_capwapmgr_drv_ref(struct netfn_capwapmgr *mgr)
{
	kref_get(&mgr->ref);
	return mgr;
}

/*
 * netfn_capwapmgr_drv_deref()
 * 	Decrement driver object reference
 */
static inline void netfn_capwapmgr_drv_deref(struct netfn_capwapmgr *mgr)
{
	kref_put(&mgr->ref, netfn_capwapmgr_final);
}
#endif /* __NETFN_CAPWAPMGR_PRIV_H */
