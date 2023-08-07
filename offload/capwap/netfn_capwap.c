/*
 * netfn_capwap.c
 *	Network function's CAPWAP offload initialization.
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/module.h>
#include <linux/version.h>
#include <linux/sysfs.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/bitmap.h>
#include <linux/sysctl.h>

#include <net/protocol.h>

#include "netfn_capwap.h"
#include "netfn_capwap_priv.h"

/*
 * Core configuration.
 */
int fwd_cores_mask = NETFN_CAPWAP_FWD_CORES_MASK;	/* 0100 represents core 2 */
int capwap_core = NETFN_CAPWAP_CORE;			/* 3 represents core 3 */

/*
 * Global capwap object.
 */
static struct netfn_capwap global_nc;

/*
 * netfn_capwap_root_dir
 *	Root directory for netfn capwap engine.
 */
static struct ctl_table netfn_capwap_root_dir[] = {
	{
		.procname = "capwap",
		.mode = 0555,
	},
};

/*
 * netfn_capwap_root
 *	Root of netfn capwap engine sysctl configuration.
 */
static struct ctl_table netfn_capwap_root[] = {
	{
		.procname = "dev",
		.mode = 0555,
		.child = netfn_capwap_root_dir
	},
};

/*
 * netfn_capwap_init()
 *	module init
 */
int __init netfn_capwap_init(void)
{
	struct netfn_capwap *nc = &global_nc;

	/*
	 * Check for overlapping core configuration.
	 */
	if (fwd_cores_mask & BIT_MASK(capwap_core)) {
		pr_warn("Overlapping cores found.\n");
		return -EINVAL;
	}

	/*
	 * Create sysctl for capwap configuration.
	 */
	nc->sysctl = register_sysctl_table(netfn_capwap_root);
	if (!nc->sysctl) {
		pr_warn("%px: Unable to register sysctl for capwap engine.\n", nc);
		vfree(nc);
		return -ENOMEM;
	}

	/*
	 * Create a debugfs entry for capwap netfn engine.
	 */
	nc->dentry = debugfs_create_dir("qca-nss-netfn-capwap", NULL);
	if (!nc->dentry) {
		pr_warn("%p, Unable to create debugsfs entry for NETFN Capwap engine\n", nc);
	}

	spin_lock_init(&nc->lock);

	pr_info("NETFN CAPWAP engine loaded: (%s)\n", NETFN_BUILD_ID);
	return 0;
}

/*
 * netfn_capwap_exit()
 *	module exit
 */
void __exit netfn_capwap_exit(void)
{
	struct netfn_capwap *nc = &global_nc;

	/*
	 * Remove the debug entry.
	 */
	debugfs_remove_recursive(nc->dentry);

	/*
	 * Unregister the sysctl table.
	 */
	unregister_sysctl_table(nc->sysctl);
}

module_param(fwd_cores_mask, int, S_IRUSR);
module_param(capwap_core, int, S_IRUSR);

module_init(netfn_capwap_init);
module_exit(netfn_capwap_exit);
MODULE_PARM_DESC(fwd_cores_mask, "Core mask to be used for pkt forwarding.");
MODULE_PARM_DESC(capwap_core, "Core marked for processing CAPWAP pkts.");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("CAPWAP offload engine");
