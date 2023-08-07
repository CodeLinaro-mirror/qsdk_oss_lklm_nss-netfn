/*
 * netfn_capwap_tunid.c
 *	Network function's CAPWAP offload tunnel ID configuration.
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

#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/lockdep.h>
#include <linux/rtnetlink.h>
#include <linux/debugfs.h>
#include <linux/ipv6.h>
#include <linux/if_arp.h>
#include <linux/bitmap.h>

#include "netfn_capwap.h"
#include "netfn_capwap_priv.h"
#include "netfn_capwap_hdr.h"
#include "netfn_capwap_tun.h"
#include "netfn_capwap_tunid.h"

/*
 * netfn_capwap_tunid_xmit()
 *	Capwap tunnel xmit
 */
netdev_tx_t netfn_capwap_tunid_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	return dev->netdev_ops->ndo_start_xmit(skb, dev);
}

/*
 * netfn_capwap_tunid_open()
 *	Start netdev queue.
 */
static int netfn_capwap_tunid_open(struct net_device *dev)
{
	netif_start_queue(dev);
	return 0;
}

/*
 * netfn_capwap_tunid_close()
 *	Stop netdev queue.
 */
static int netfn_capwap_tunid_close(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

/*
 * netfn_capwap_tunid_stats
 *	Get pdev stats.
 */
static void netfn_capwap_tunid_stats(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	return;
}

/*
 * netfn_capwap_tunid_start_xmit()
 *	Start transmiting using pdev.
 */
static netdev_tx_t netfn_capwap_tunid_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	return NETDEV_TX_OK;
}

/*
 * netfn_capwap_tunid_ops
 *	Tunnel id netdev operations.
 */
static const struct net_device_ops netfn_capwap_tunid_ops = {
	.ndo_open               = netfn_capwap_tunid_open,
	.ndo_stop               = netfn_capwap_tunid_close,
	.ndo_start_xmit         = netfn_capwap_tunid_start_xmit,
	.ndo_set_mac_address    = eth_mac_addr,
	.ndo_change_mtu         = eth_change_mtu,
	.ndo_get_stats64        = netfn_capwap_tunid_stats,
};

/*
 * netfn_capwap_tunid_setup()
 *	Tunid setup function.
 */
static void netfn_capwap_tunid_setup(struct net_device *dev)
{
	dev->addr_len = ETH_ALEN;
	dev->mtu = ETH_DATA_LEN;
	dev->needed_headroom = 0;
	dev->needed_tailroom = 0;
	dev->type = ARPHRD_VOID;
	dev->ethtool_ops = NULL;
	dev->header_ops = NULL;
	dev->netdev_ops = &netfn_capwap_tunid_ops;
	dev->priv_destructor = NULL;
	memcpy(dev->dev_addr, "\x00\x00\x00\x00\x00\x00", dev->addr_len);
	memset(dev->broadcast, 0xff, dev->addr_len);
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);
}

/*
 * netfn_capwap_tunid_alloc()
 *	Allocates a new tunid.
 */
struct net_device *netfn_capwap_tunid_alloc(void)
{
	struct net_device *dev = NULL;

	BUG_ON(1);
	return dev;
}
EXPORT_SYMBOL(netfn_capwap_tunid_alloc);

/*
 * netfn_capwap_tunid_add()
 *	Adds tunnel with specific tun id.
 */
bool netfn_capwap_tunid_add(struct net_device *dev, uint8_t id, struct netfn_capwap_tun_cfg *cfg)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tunid_add);

/*
 * netfn_capwap_tunid_free()
 *	Frees tunnel corresponding to dev passed.
 */
bool netfn_capwap_tunid_free(struct net_device *dev)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tunid_free);

/*
 * netfn_capwap_tunid_del()
 *	Delets tun_id specific tunnel.
 */
bool netfn_capwap_tunid_del(struct net_device *dev, uint16_t id)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tunid_del);

/*
 * netfn_capwap_tunid_bind()
 *	Binds tun_id netdev to next netdev
 */
bool netfn_capwap_tunid_bind(struct net_device *dev, uint8_t id, struct net_device *next)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tunid_bind);

/*
 * netfn_capwap_tunid_stats_get()
 *	Gets the tunnel ID stats.
 */
int netfn_capwap_tunid_stats_get(struct net_device *pdev, uint8_t tun_id, struct netfn_capwap_tun_stats *stats)
{
	BUG_ON(1);

	return 0;
}
EXPORT_SYMBOL(netfn_capwap_tunid_stats_get);
