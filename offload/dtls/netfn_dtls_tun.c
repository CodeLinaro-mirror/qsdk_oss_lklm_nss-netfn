/*
 * Copyright (c) 2024, Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/if.h>

#include "netfn_dtls_priv.h"


/*
 * DTLS device callbacks.
 */
static const struct net_device_ops dtls_tun_ops = {
	.ndo_open = NULL,
	.ndo_stop = NULL,
	.ndo_start_xmit = NULL,
	.ndo_get_stats64 = NULL,
	.ndo_change_mtu = NULL,
};

/*
 * netfn_dtls_tun_free()
 *	Delete DTLS device associated with the netdevice.
 */
void netfn_dtls_tun_free(struct net_device *dev)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);


	/*
	 * TODO: Wait for dependent resource to free.
	 */

	/*
	 * Bring down the device and unregister from linux.
	 */
	rtnl_lock();
	dev_close(dev);
	unregister_netdevice(dev);
	rtnl_unlock();
}
EXPORT_SYMBOL(netfn_dtls_tun_free);

/*
 * netfn_dtls_tun_alloc()
 *	Create a DTLS device for a new connection.
 */
struct net_device *netfn_dtls_tun_alloc(ssize_t pvt_sz)
{
	struct netfn_dtls_drv *drv = &g_dtls_drv;
	struct netfn_dtls_tun *tun;
	struct net_device *dev;

	/*
	 * Netdevice allocation.
	 */
	dev = alloc_netdev(sizeof(*tun), "dtlstun%d", NET_NAME_ENUM, NULL);
	if (!dev) {
		pr_err("%px: Failed to allocate DTLS device\n", drv);
		return NULL;
	}

	/*
	 * TODO: Initialize device private structure & debugfs.
	 */

	return dev;
}
EXPORT_SYMBOL(netfn_dtls_tun_alloc);
