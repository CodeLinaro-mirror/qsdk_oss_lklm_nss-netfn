/*
 * netfn_capwapmgr.c
 *	Network function's CAPWAP manager Initialization.
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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/bitmap.h>

#include "netfn_capwapmgr.h"
#include "netfn_capwapmgr_priv.h"

/*
 * Global capwapmanager object.
 */
struct netfn_capwapmgr g_mgr;

/*
 * netfn_capwapmgr_error_stats_str
 *	Stats strings.
 */
static const char *netfn_capwapmgr_error_stats_str[] = {
	"WAN Net device NULL",
					/**< NULL WAN net device */
	"Top Net Device NULL",
					/**< NULL top net device */
	"Unsupported tuple type",
					/**< Unsupported tuple type */
	"Unsupported L4 protocol",
					/**< Unsupported l4 protocol */
	"CAPWAP offload tunnel alloc failed",
					/**< Failed to allocate capwap tunnel in offload engine */
	"CAPWAP offload tunnel free failed",
					/**< Failed to free capwap tunnel in offload engine */
	"Failed to create flow rule",
					/**< Failed to create flow rule */
	"Failed to destroy flow rule",
					/**< Failed to destroy flow rule */
	"CAPWAP offload tunnel id alloc failed",
					/**< Failed to allocate tunid version of capwap tunnel in offload engine */
	"CAPWAP offload tunnel id free failed",
					/**< Failed to free tunid version of capwap tunnel in offload engine */
	"CAPWAP offload tunnel context get failed",
					/**< Failed to get tunnel context from offload engine */
	"CAPWAP offlaod tunid dev free failed",
					/**< Failed to free capwap tunid dev in offload engine */
};

/*
 * netfn_capwapmgr_final
 *	Mark final deref completion.
 */
void netfn_capwapmgr_final(struct kref *kref)
{
	BUG_ON(1);
	return;
}

/*
 * netfn_capwapmgr_error_stats_print
 *	API to print error statistics.
 */
static int netfn_capwapmgr_error_stats_print(struct seq_file *sf, void *ptr)
{
	BUG_ON(1);
	return 0;

}

/*
 * netfn_capwapmgr_error_stats_open
 *	Open file handler for common stats debugfs.
 */
static int netfn_capwapmgr_error_stats_open(struct inode *inode, struct file *file)
{
	BUG_ON(1);
	return 0;
}

/*
 * netfn_capwapmgr_error_stats_file_ops
 *	File operations for capwapmanager common stats
 */
const struct file_operations netfn_capwapmgr_error_stats_file_ops = {
	.open = netfn_capwapmgr_error_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*
 * netfn_capwapmgr_init_module()
 *	netfn capwapmgr module init function
 */
int __init netfn_capwapmgr_init_module(void)
{
	BUG_ON(1);
	return 0;
}

/*
 * netfn_capwapmgr_exit_module()
 *	netfn capwapmgr exit module function
 */
void __exit netfn_capwapmgr_exit_module(void)
{
	BUG_ON(1);
}

module_init(netfn_capwapmgr_init_module);
module_exit(netfn_capwapmgr_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NETFN CAPWAP manager");