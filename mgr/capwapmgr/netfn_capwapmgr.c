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

#include <netfn_capwapmgr.h>
#include "netfn_capwapmgr_priv.h"

/*
 * Global capwapmanager object.
 */
struct netfn_capwapmgr g_mgr;

/*
 * netfn_capwapmgr_stats_str
 *	Stats strings.
 */
static const char *netfn_capwapmgr_stats_str[] = {
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
	"CAPWAP offlaod tunid dtls alloc failed",
					/**< Failed to free capwap tunid dev in offload engine */
	"CAPWAP offlaod tunid dev bind failed",
					/**< Failed to free capwap tunid dev in offload engine */
	"CAPWAP offload netfn flow cookie DB alloc failed",
					/**< Failed to instantiate the flow cookie DB in offload engine */
	"CAPWAP offload netfn flow cookie add failed",
					/**< Failed to add flow cookie entry as reported in offload engine */
	"CAPWAP offload netfn flow cookie delete failed",
					/**< Failed to delete flow cookie entry as reported in offload engine */
};

/*
 * netfn_capwapmgr_final
 *	Mark final deref completion.
 */
void netfn_capwapmgr_final(struct kref *kref)
{
        struct netfn_capwapmgr *mgr = container_of(kref, struct netfn_capwapmgr, ref);

        complete(&mgr->completion);
}

/*
 * netfn_capwapmgr_stats_print
 *	API to print error statistics.
 */
static int netfn_capwapmgr_stats_print(struct seq_file *sf, void *ptr)
{

          struct netfn_capwapmgr *mgr = &g_mgr;
          uint64_t error_count;
          int i = 0;

          seq_puts(sf, "\n################ CAPWAPMGR statistics Start ################\n");
          seq_printf(sf, "\tTunnels allocated: %u\n", atomic_read(&mgr->stats.tun_dev_alloc));
          seq_printf(sf, "\tTunnels freed: %u\n", atomic_read(&mgr->stats.tun_dev_free));
          seq_printf(sf, "\tTunid dev allocated : %u\n", atomic_read(&mgr->stats.tunid_dev_alloc));
          seq_printf(sf, "\tTunid dev freed: %u\n", atomic_read(&mgr->stats.tunid_dev_free));
          seq_puts(sf, "\n");

          seq_puts(sf, "Error Statistics\n");

          for (i = 1; i < NETFN_CAPWAPMGR_ERROR_MAX; i++) {
                  error_count = atomic64_read(&mgr->stats.error_stats[i]);
                  seq_printf(sf, "\t[%s]: %llu\n", netfn_capwapmgr_stats_str[i-1],
                                                                  error_count);
          }

          seq_puts(sf, "\n################ CAPWAPMGR Statistics End  ################\n");

          return 0;
}

/*
 * netfn_capwapmgr_stats_open
 *	Open file handler for common stats debugfs.
 */
static int netfn_capwapmgr_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, netfn_capwapmgr_stats_print, inode->i_private);
}

/*
 * netfn_capwapmgr_stats_ops
 *	File operations for capwapmanager common stats
 */
const struct file_operations netfn_capwapmgr_stats_ops = {
	.open = netfn_capwapmgr_stats_open,
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
	struct netfn_capwapmgr *mgr = &g_mgr;
	struct netfn_flow_cookie_db *d;

	/*
	 * Deref: netfn_capwapmgr_exit_module().
	 */
	init_completion(&mgr->completion);
	kref_init(&mgr->ref);
	spin_lock_init(&mgr->lock);

	/*
	 * Instantiate the Flow Cookie Data Base here.
	 */
	d = netfn_flow_cookie_db_alloc(NETFN_CAPWAPMGR_FLOW_COOKIE_HASHTABLE_SIZE);
	if (!d) {
		atomic64_inc(&mgr->stats.error_stats[NETFN_CAPWAPMGR_ERROR_FLOW_COOKIE_DB_ALLOC]);
		netfn_capwapmgr_warn("Failed Instantiating a FLOW DB instance "
			"from CAPWAP Manager Module");
		return -1;
	}

	/*
	 * Assign the DB handle to
	 * the CAPWAP Manager module here.
	 */
	spin_lock_bh(&mgr->lock);
	mgr->d = d;
	spin_unlock_bh(&mgr->lock);

	/*
	 * Pass the DB handle to
	 * the Capwap offload module here.
	 */
	netfn_capwap_set_db_handle(mgr->d);

#if defined(NETFN_CAPWAPMGR_ONE_NETDEV)
	netfn_capwapmgr_dev = netfn_capwapmgr_tunid_dev_alloc();
	if (!netfn_capwapmgr_dev) {
		netfn_capwapmgr_warn("Failed to Capwap Net Device\n");
		return -1;
	}
#endif

        /*
         * Create a debugfs entry for capwap netfn engine.
         */
        mgr->dentry = debugfs_create_dir("qca-nss-netfn-capwapmgr", NULL);
        if (!mgr->dentry) {
                netfn_capwapmgr_warn("%p, Unable to create debugsfs entry for NETFN capwapmgr\n", mgr);
        }

	if (!debugfs_create_file("stats", S_IRUGO, mgr->dentry,
                NULL, &netfn_capwapmgr_stats_ops)) {
		netfn_capwapmgr_warn("%p: Unable to create error statistics file entry in debugfs\n", mgr);
	}

	netfn_capwapmgr_info("NETFN CAPWAPMGR loaded: (%s)\n", NETFN_CAPWAPMGR_BUILD_ID);
        return 0;
}

/*
 * netfn_capwapmgr_exit_module()
 *	netfn capwapmgr exit module function
 */
void __exit netfn_capwapmgr_exit_module(void)
{
	struct netfn_capwapmgr *mgr = &g_mgr;

#if defined(NETFN_CAPWAPMGR_ONE_NETDEV)
	netfn_capwapmgr_tunid_dev_free(netfn_capwapmgr_dev);
#endif

	/*
	 * Latch onto CAPWAP Offload to ensure that
	 * the ref is cleared on DB handle.
	 * netfn_flow_cookie_db_instance_deref() as implemented
	 * in the DB free API internally will take care this
	 * by safely performing the DB free operation by tracking
	 * the ref count properly.
	 *
	 * Flow Cookie Data Base Deallocation is being done here.
	 *
	 */
	if (netfn_flow_cookie_db_instance_deref(mgr->d)) {

		/*
		 * Clear the DB handle as passed to
		 * the Capwap offload module here.
		 */
		netfn_capwap_clear_db_handle();
	}

	/*
	 * Ref: netfn_capwapmgr_init_module().
	 */
	netfn_capwapmgr_deref(mgr);

	/*
	 * Wait for all the derefs.
	 * Reference is taken on driver during tunnel allocations.
	 */
	wait_for_completion(&mgr->completion);

	/*
	 * Remove debugfs entries
	 */
	debugfs_remove_recursive(mgr->dentry);
}

module_init(netfn_capwapmgr_init_module);
module_exit(netfn_capwapmgr_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NETFN CAPWAP manager");
