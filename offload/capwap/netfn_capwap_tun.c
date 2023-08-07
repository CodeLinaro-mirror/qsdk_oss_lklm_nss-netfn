/*
 * netfn_capwap_tun.c
 *	Network function's CAPWAP offload tunnel configuration.
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
#include <linux/bitmap.h>

#include "netfn_capwap.h"
#include "netfn_capwap_priv.h"
#include "netfn_capwap_hdr.h"
#include "netfn_capwap_tun.h"

/*
 * netfn_capwap_tun_xmit()
 *	Capwap tunnel xmit
 */
netdev_tx_t netfn_capwap_tun_xmit(struct sk_buff *skb, struct net_device *dev)
{
	return dev->netdev_ops->ndo_start_xmit(skb, dev);
}

/*
 * netfn_capwap_tun_dev_is_dtls()
 *	Returns true if the device is dtls.
 */
static inline bool netfn_capwap_tun_dev_is_dtls(const struct net_device *dev)
{
	BUG_ON(1);

	return true;
}

/*
 * netfn_capwap_tun_notify()
 *	Called to notify if the skb can be queued or not.
 */
bool netfn_capwap_tun_notify(struct netfn_capwap_tun *nct, cpumask_t *mask, struct sk_buff *skb, struct netfn_capwap_multicore_queue *mc_q)
{
	BUG_ON(1);

	return true;
}

/*
 * netfn_capwap_tun_open()
 *	open the device for usage
 */
static int netfn_capwap_tun_open(struct net_device *dev)
{
	struct netfn_capwap_tun *nct __attribute__((unused)) = netdev_priv(dev);

	netif_start_queue(dev);
	return 0;
}

/*
 * netfn_capwap_tun_stop()
 *	stop the device from usage
 */
static int netfn_capwap_tun_stop(struct net_device *dev)
{
	struct netfn_capwap_tun *nct __attribute__((unused)) = netdev_priv(dev);

	netif_stop_queue(dev);

	/*
	 * TODO: Drain all packets from netdev queue.
	 */
	return 0;
}

/*
 * netfn_capwap_tun_tx()
 *	Tunnel transmit function
 */
static netdev_tx_t netfn_capwap_tun_tx(struct sk_buff *skb, struct net_device *dev)
{
	BUG_ON(1);

	/*
	 * TODO: Add tun_tx support.
	 */
	return NETDEV_TX_OK;
}

/*
 * netfn_capwap_tun_dev_stats64()
 *	Get tunnel statistics
 */
static void netfn_capwap_tun_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *dev_stats)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);

	dev_stats->rx_packets = READ_ONCE(nct->stats.rx_pkts);
	dev_stats->rx_bytes = READ_ONCE(nct->stats.rx_bytes);
	dev_stats->tx_packets = READ_ONCE(nct->stats.tx_pkts);
	dev_stats->tx_bytes = READ_ONCE(nct->stats.tx_bytes);
	dev_stats->tx_dropped = READ_ONCE(nct->stats.tx_failures);
	dev_stats->rx_dropped = READ_ONCE(nct->stats.rx_failures);
}

/*
 * netfn_capwap_tun_mtu()
 *	change tunnel MTU
 */
static int netfn_capwap_tun_mtu(struct net_device *dev, int mtu)
{
	BUG_ON(1);
	dev->mtu = mtu;

	return 0;
}

/*
 * netfn_capwap_tun_destructor()
 *	Tunnel dev destructor called for final free
 */
static void netfn_capwap_tun_destructor(struct net_device *dev)
{
	BUG_ON(1);

	return;
}

/*
 * capwap_rtnl_link_ops
 *	rtnl link ops.
 */
struct rtnl_link_ops capwap_rtnl_link_ops = {
	.kind = "capwap",
};

/*
 * capwap_dev_ops
 *	capwap device operations.
 */
static const struct net_device_ops capwap_dev_ops = {
	.ndo_open = netfn_capwap_tun_open,
	.ndo_stop = netfn_capwap_tun_stop,
	.ndo_start_xmit = netfn_capwap_tun_tx,
	.ndo_get_stats64 = netfn_capwap_tun_get_stats64,
	.ndo_change_mtu = netfn_capwap_tun_mtu,
};

/*
 * netfn_capwap_tun_setup()
 *	Device setup function
 */
void netfn_capwap_tun_setup(struct net_device *dev)
{
	BUG_ON(1);
	dev->priv_destructor = netfn_capwap_tun_destructor;

	return;
}

/*
 * netfn_capwap_tun_alloc()
 *	Allocates a new tunnel.
 */
struct net_device *netfn_capwap_tun_alloc(struct netfn_capwap_tun_cfg *cfg)
{
	struct net_device *dev = NULL;

	BUG_ON(1);
	return dev;
}
EXPORT_SYMBOL(netfn_capwap_tun_alloc);

/*
 * netfn_capwap_tun_free()
 *	Frees tunnel corresponding to dev passed.
 */
bool netfn_capwap_tun_free(struct net_device *dev)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tun_free);

/*
 * netfn_capwap_tun_bind()
 *	Binds tunnel netdev to next netdev.
 */
bool netfn_capwap_tun_bind(struct net_device *dev, struct net_device *next)
{
	BUG_ON(1);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tun_bind);

/*
 * netfn_capwap_tun_stats_get()
 *	Gets tunnel stats corresponding to dev.
 */
int netfn_capwap_tun_stats_get(struct net_device *dev, struct netfn_capwap_tun_stats *stats)
{
	BUG_ON(1);

	return 0;
}
EXPORT_SYMBOL(netfn_capwap_tun_stats_get);
