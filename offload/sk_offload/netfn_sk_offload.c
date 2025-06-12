/*
 **************************************************************************
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_sk_offload.c
 *      Network Sock Offload Functionality Manager
 */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/debugfs.h>
#include <linux/if_arp.h>
#include <linux/if_pppox.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <net/inet_sock.h>
#include <net/protocol.h>

#include <netfn_flowmgr.h>
#include <ppe_drv.h>

#include "netfn_sk_offload_priv.h"

/*
 * We use the following global context
 * to store and use all the relevant
 * entities as parsed across various
 * files of the Sock Offload Module.
 *
 */
struct netfn_sk_offload g_sk_offload = {0};

int netfn_sk_offload_core_mask = NETFN_SOCK_OFFLOAD_RFS_CORES;
module_param(netfn_sk_offload_core_mask, int, S_IRUSR);
MODULE_PARM_DESC(netfn_sk_offload_core_mask, "Core mask to be used for RFS");

/*
 * netfn_sk_offload_notify()
 *      Netfn Sock Offload Notify function.
 */
static int netfn_sk_offload_notify(struct notifier_block *nb, unsigned long action, void *data)
{
	struct sock *sk = (struct sock *)data;
	int ret = NOTIFY_OK;

	pr_info("Received notification from AF_INET module, sk:%p, action:%lu", sk, action);

	/*
	 * TODO:
	 * SK_OFFLOAD_ENABLE_NOTIFY:
	 * 1. Allocation of context.
	 * 2. Perform the Rx Initialization for the context.
	 * 3. Rule creation for socket offloading.
	 *
	 * SK_OFFLOAD_DISABLE_NOTIFY:
	 * 1. Perform the Rx Deinitialization for the context.
	 * 2. Rule Deletion to disable socket offloading.
	 * 3. Free the relevant resources and context.
	 */

	return ret;
}

/*
 * netfn_sk_offload_notifier
 *      Notifier Block for Socket Offload Event.
 */
static struct notifier_block netfn_sk_offload_notifier __attribute__((unused)) = {
	.notifier_call = netfn_sk_offload_notify,
};

/*
 * netfn_sk_offload_exit_module()
 *      Netfn Sock Offload Module exit function
 */
void __exit netfn_sk_offload_exit_module(void)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	int ret = 0;

	/*
	 * TODO:
	 * We ensure sufficient cleanup is done
	 * for all the active contexts before
	 * module goes away.
	 */

	ret = sock_notify_unregister(&netfn_sk_offload_notifier);
	if (ret) {
		pr_err("Failed to un-register from AF_INET Socket Offload Notifier, ret:%d\n", ret);
		return;
	}

	/*
	 * TODO:
	 * 2. De-allocate the VP.
	 */

	if(so->dentry)
		debugfs_remove_recursive(so->dentry);
	pr_info("Unloading the Netfn Sock Offload Module\n");
}

/*
 * netfn_sk_offload_init_module()
 *      Netfn Sock Offload Module init function
 */
int __init netfn_sk_offload_init_module(void)
{
	struct netfn_sk_offload *so = &g_sk_offload;
	int ret = 0;

	pr_info("Netfn Sock Offload Module loaded: (%s)\n", NSS_NETFN_BUILD_ID);

	ret = sock_notify_register(&netfn_sk_offload_notifier);
	if (ret) {
		pr_err("Failed to register to AF_INET Socket Offload Notifier, ret:%d\n", ret);
		return ret;
	}

	/*
	 * TODO:
	 * 2. Allocate the VP.
	 */

	so->dentry = debugfs_create_dir("qca-nss-netfn-sk-offload", NULL);
	if (!so->dentry) {
		pr_warn("%p:Failed to create debugfs_entry for sock offload module\n", so);
	}

	return ret;
}

module_init(netfn_sk_offload_init_module);
module_exit(netfn_sk_offload_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Network Sock Offload Module");
