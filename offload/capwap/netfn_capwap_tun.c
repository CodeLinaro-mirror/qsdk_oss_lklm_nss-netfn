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
#include <linux/if_arp.h>

#include <ppe_vp_public.h>

#include "netfn_capwap.h"
#include "netfn_capwap_priv.h"
#include "netfn_capwap_hdr.h"
#include "netfn_capwap_tun.h"
#include "netfn_capwap_enc.h"
#include "netfn_capwap_dec.h"

/*
 * netfn_capwap_dec_rx_skb()
 *	Receive an SKB queueu for processing.
 */
static rx_handler_result_t netfn_capwap_tun_rx(struct sk_buff **pskb)
{
	BUG_ON(1);
	return RX_HANDLER_CONSUMED;
}

/*
 * netfn_capwap_tun_tx()
 *	Device transmit function
 */
static netdev_tx_t netfn_capwap_tun_tx(struct sk_buff *skb, struct net_device *dev)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);
	unsigned int needed_hroom, needed_troom;
	struct netfn_capwap_pkt_stats *stats;
	struct netfn_capwap_prehdr *phdr;
	bool expand_skb;

	stats = &nct->stats;
	needed_hroom = dev->needed_headroom;
	needed_troom = dev->needed_tailroom;

	/*
	 * Check if we have sufficient headroom and tailroom.
	 */
	expand_skb = (skb_cloned(skb) || (skb_headroom(skb) < needed_hroom) || (skb_tailroom(skb) < needed_troom));
	if (expand_skb && pskb_expand_head(skb, needed_hroom, needed_troom, GFP_NOWAIT | __GFP_NOWARN)) {
		pr_warn_ratelimited("%px: Failed to expand SKB with headroom(%u) tailroom(%u)\n",
				dev, skb_headroom(skb), skb_tailroom(skb));
		goto fail;
	}

	phdr = (struct netfn_capwap_prehdr *)skb->data;

	/*
	 * Check for capwap version.
	 */
	if (unlikely(phdr->vers != NETFN_CAPWAP_VERSION)) {
		pr_warn_ratelimited("%px: Version mismatch actual:%d, expected: %d.\n", skb,
				phdr->vers, NETFN_CAPWAP_VERSION);
		goto fail;
	}

	/*
	 * Check for dtls packets.
	 */
	if (unlikely(phdr->type & NETFN_CAPWAP_PKT_TYPE_DTLS)) {
		pr_warn_ratelimited("%px: We don't support DTLS packets encapsulation.\n", skb);
		goto fail;
	}

	/*
	 * Number of wireless sections shouldn't exceed max count.
	 */
	if (unlikely(phdr->nwireless > NETFN_CAPWAP_MAX_NWIRELESS)) {
		pr_warn_ratelimited("%px: Number of wireless sections exceeds max count(%d).\n",
				phdr, NETFN_CAPWAP_MAX_NWIRELESS);
		goto fail;
	}

	/*
	 * IPI packet to desired core.
	 */
	if (netfn_pkt_steer_send(nct->tx_steer, skb, capwap_core)) {
		return NETDEV_TX_OK;
	}
fail:
	/*
	 * dev_kfree_skb has a NULL check inside.
	 */
	NETFN_CAPWAP_TUN_STATS_INC(&stats->tx_failures);
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

/*
 * netfn_capwap_tun_open()
 *	Open the device for usage
 */
static int netfn_capwap_tun_open(struct net_device *dev)
{
	netif_start_queue(dev);
	return 0;
}

/*
 * netfn_capwap_tun_stop()
 *	Stop the device from usage
 */
static int netfn_capwap_tun_stop(struct net_device *dev)
{
	netif_stop_queue(dev);
	return 0;
}

/*
 * netfn_capwap_tun_dev_stats64()
 *	Get tunnel statistics
 */
static void netfn_capwap_tun_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *dev_stats)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);
	struct netfn_capwap_pkt_stats *pkt_stats = &nct->stats;

	dev_stats->rx_packets = READ_ONCE(pkt_stats->rx_pkts);
	dev_stats->rx_bytes = READ_ONCE(pkt_stats->rx_bytes);
	dev_stats->tx_packets = READ_ONCE(pkt_stats->tx_pkts);
	dev_stats->tx_bytes = READ_ONCE(pkt_stats->tx_bytes);
	dev_stats->tx_dropped = READ_ONCE(pkt_stats->tx_failures);
	dev_stats->rx_dropped = READ_ONCE(pkt_stats->rx_failures);
}

/*
 * netfn_capwap_tun_mtu()
 *	Change tunnel MTU
 */
static int netfn_capwap_tun_mtu(struct net_device *dev, int mtu)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);
	struct net_device *next_dev = nct->next_dev;

	__dev_set_mtu(next_dev, mtu);
	dev->mtu = next_dev->mtu - nct->enc.outer_hdr_len;
	return 0;
}

/*
 * capwap_tun_ops
 *	Capwap device operations.
 */
static const struct net_device_ops capwap_tun_ops = {
	.ndo_open = netfn_capwap_tun_open,
	.ndo_stop = netfn_capwap_tun_stop,
	.ndo_start_xmit = netfn_capwap_tun_tx,
	.ndo_get_stats64 = netfn_capwap_tun_get_stats64,
	.ndo_change_mtu = netfn_capwap_tun_mtu,
};

/*
 * netfn_capwap_tun_destructor()
 *	Tunnel dev destructor called for final free
 */
static void netfn_capwap_tun_destructor(struct net_device *dev)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);

	pr_info("%px: Freeing the tunnel(%s)\n", &global_nc, dev->name);

	/*
	 * Free pkt steer object.
	 */
	if (nct->tx_steer) {
		netfn_pkt_steer_free(nct->tx_steer);
	}

	if (nct->rx_steer) {
		netfn_pkt_steer_free(nct->rx_steer);
	}

	netfn_capwap_dec_deinit(&nct->dec);
	debugfs_remove_recursive(nct->dentry);
	free_netdev(dev);
	return;
}

/*
 * netfn_capwap_tun_setup()
 *	Device setup function
 */
static void netfn_capwap_tun_setup(struct net_device *dev)
{
	dev->addr_len = ETH_ALEN;
	dev->mtu = ETH_DATA_LEN;
	dev->hard_header_len = NETFN_CAPWAP_MAX_HDR_SZ;
	dev->needed_headroom = NETFN_CAPWAP_MAX_HDR_SZ;
	dev->needed_tailroom = NETFN_CAPWAP_MAX_HDR_SZ;

	dev->type = ARPHRD_VOID;
	dev->ethtool_ops = NULL;
	dev->header_ops = NULL;
	dev->netdev_ops = &capwap_tun_ops;
	dev->priv_destructor = netfn_capwap_tun_destructor;

	/*
	 * Get the MAC address from the ethernet device
	 */
	random_ether_addr(dev->dev_addr);
	memset(dev->broadcast, 0xff, dev->addr_len);
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);

	return;
}

/*
 * netfn_capwap_tun_stats_show()
 *      Show netfn capwap tun statistics.
 */
static ssize_t netfn_capwap_tun_stats_show(struct file *fp, char __user *ubuf, size_t sz, loff_t *ppos)
{
	struct netfn_capwap_tun *nct = fp->private_data;
	struct netfn_capwap_pkt_stats *stats = &nct->stats;
	size_t pkt_stats_sz, stats_sz;
	ssize_t bytes_read = 0;
	size_t size_wr = 0;
	char *buf;

	pkt_stats_sz = sizeof(struct netfn_capwap_pkt_stats) / sizeof(uint64_t) * NETFN_CAPWAP_TUN_STATS_STRLEN;
	stats_sz = (NETFN_CAPWAP_TUN_STATS_WIDTH * NETFN_CAPWAP_TUN_STATS_STRLEN) + pkt_stats_sz;

	buf = vzalloc(stats_sz);
	if (!buf) {
		pr_warn("%px: Unable to allocate memory for stats.\n", fp);
		return -ENOMEM;
	}

	/*
	 * Print stats.
	 */
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "\n------------[pkt stats start]----------\n");
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "rx_pkts", READ_ONCE(stats->rx_pkts));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "rx_bytes", READ_ONCE(stats->rx_bytes));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "rx_failures", READ_ONCE(stats->rx_failures));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "tx_pkts", READ_ONCE(stats->tx_pkts));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "tx_bytes", READ_ONCE(stats->tx_bytes));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "tx_failures", READ_ONCE(stats->tx_failures));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "------------[pkt stats end]-----------\n");

	bytes_read = simple_read_from_buffer(ubuf, sz, ppos, buf, size_wr);
	vfree(buf);
	return bytes_read;
}

/*
 * Capwap file operations.
 */
static const struct file_operations file_ops = {
	.open = simple_open,
	.llseek = default_llseek,
	.read = netfn_capwap_tun_stats_show
};

/*
 * netfn_capap_tun_vp_err()
 *	VP callback for outer exception.
 */
bool netfn_capwap_tun_vp_err(struct net_device *dev, struct sk_buff *skb, void *cb_data)
{
	struct netfn_capwap_tun *nct = cb_data;

	NETFN_CAPWAP_TUN_STATS_INC(&nct->stats.tx_failures);
	dev_kfree_skb(skb);
	return true;
}

/*
 * netfn_capap_tun_vp_rx()
 *	VP callback for receiving packets.
 */
bool netfn_capwap_tun_vp_rx(struct net_device *dev, struct sk_buff *skb, void *cb_data)
{
	BUG_ON(1);

	return true;
}

/*
 * netfn_capwap_tun_vp_attach()
 *	Allocate VP corresponding to netdev
 */
static struct net_device *netfn_capwap_tun_vp_attach(struct net_device *dev)
{
	struct ppe_vp_ai vpai = {0};
	struct net_device *vp_dev;

	/*
	 * Allocate VP.
	 */
	vpai.type = PPE_VP_TYPE_SW_L3;
	vpai.usr_type = PPE_VP_USER_TYPE_ACTIVE;
	vpai.core_mask = capwap_core;
	vpai.dst_cb = netfn_capwap_tun_vp_rx;
	vpai.dst_cb_data = netdev_priv(dev);
	vpai.src_cb = netfn_capwap_tun_vp_err;
	vpai.src_cb_data = netdev_priv(dev);
	vpai.queue_num = 0;

	vp_dev = ppe_vp_alloc_dev(dev, &vpai);
	if (!vp_dev) {
		pr_warn("%p, Unable to create VP\n", dev);
		return NULL;
	}

	pr_info("%px: Tun(%s), vp(%s) alloc and bind suceess\n", dev, dev->name, vp_dev->name);
	return vp_dev;
}

/*
 * netfn_capwap_tun_vp_detach()
 *	Unbinds and frees the ppe-vp associated with dev.
 */
static void netfn_capwap_tun_vp_detach(struct net_device *dev)
{
	struct netfn_capwap_tun *nct = netdev_priv(dev);
	struct net_device *vp_dev;

	BUG_ON(nct->vp_dev != nct->next_dev);

	vp_dev = xchg(&nct->vp_dev, NULL);

	/*
	 * Free the vp dev
	 */
	ppe_vp_free_dev(vp_dev);
	pr_info("%px: Tun(%s), unbinded vp dev and freed\n", dev, dev->name);
	return;
}

/*
 * netfn_capwap_tun_tx_cb()
 *	Tx packet steering callback
 */
static void netfn_capwap_tun_tx_cb(struct sk_buff_head *q_head, void *app_data)
{
	struct netfn_capwap_tun *nct = app_data;
	struct netfn_capwap_pkt_stats *stats;
	struct sk_buff *skb, *nskb;

	stats = &nct->stats;

	/*
	 * Encapsulates and transmits out.
	 */
	skb_queue_walk_safe(q_head, skb, nskb) {
		struct sk_buff_head q_frags;

		__skb_queue_head_init(&q_frags);
		__skb_unlink(skb, q_head);

		/*
		 * Encapsulate the packet.
		 */
		if (!netfn_capwap_enc(&nct->enc, skb, &q_frags)) {
			NETFN_CAPWAP_TUN_STATS_INC(&stats->tx_failures);
			dev_kfree_skb_list_fast(&q_frags);
			continue;
		}

		/*
		 * TODO: optimize this path by merging the fragmentation and send loop into one.
		 */
		/*
		 * Trasmit the packet out.
		 */
		skb_queue_walk_safe(&q_frags, skb, nskb) {
			int len = skb->len;

			__skb_unlink(skb, &q_frags);
			skb->dev = nct->next_dev;

			if (dev_queue_xmit(skb) != NETDEV_TX_OK) {
				NETFN_CAPWAP_TUN_STATS_INC(&stats->tx_failures);
				continue;
			}

			NETFN_CAPWAP_TUN_STATS_INC(&stats->tx_pkts);
			NETFN_CAPWAP_TUN_STATS_ADD(&stats->tx_bytes, len);
		}

	}
}

/*
 * netfn_capwap_tun_rx_cb()
 *	Rx packet steering callback
 */
static void netfn_capwap_tun_rx_cb(struct sk_buff_head *q_head, void *app_data)
{
	BUG_ON(1);
}

/*
 * netfn_capwap_tun_alloc()
 *	Allocates a new tunnel.
 */
struct net_device *netfn_capwap_tun_alloc(struct netfn_capwap_tun_cfg *cfg)
{
	struct netfn_pkt_steer_info info = {0};
	struct netfn_capwap *nc = &global_nc;
	struct netfn_capwap_tun *nct;
	struct net_device *dev;
	int err;

	/*
	 * Allocate a new device for tunnel.
	 */
	dev = alloc_netdev(sizeof(*nct), "capwap%d", NET_NAME_ENUM, netfn_capwap_tun_setup);
	if (!dev) {
		pr_warn("%p: failed to allocate capwap tunnel device\n", nc);
		return NULL;
	}

	nct = netdev_priv(dev);

	/*
	 * Tunnel init
	 */
	nct->nc = nc;
	nct->dev = dev;
	memset(&nct->stats, 0, sizeof(nct->stats));

	/*
	 * Create debug entry for tunnel.
	 */
	nct->dentry = debugfs_create_dir(dev->name, nc->dentry);
	if (!nct->dentry) {
		pr_warn("%p: failed to create debugfs entry for capwap(%s)\n", nct, dev->name);
		dev->priv_destructor(dev);
		return NULL;
	}

	if (!debugfs_create_file("stats", S_IRUGO, nct->dentry, nct, &file_ops)) {
		pr_warn("%p: Unable to create file for capwap(%s) stats.\n", nct, dev->name);
		dev->priv_destructor(dev);
		return NULL;
	}

	/*
	 * Register netdevice
	 * On failure destructor is invoked by linux.
	 */
	err = register_netdev(dev);
	if (err) {
		pr_warn("%p: failed to register capwap(%s) netdev with err(%d)\n", dev, dev->name, err);
		return NULL;
	}

	/*
	 * Add a VP for netdev.
	 */
	nct->vp_dev = netfn_capwap_tun_vp_attach(dev);
	if (!nct->vp_dev) {
		pr_warn("%px: Failed to allocate VP.\n", dev);
		unregister_netdev(dev);
		return NULL;
	}

	nct->next_dev = nct->vp_dev;

	/*
	 * Encap init.
	 */
	if (!netfn_capwap_enc_init(&nct->enc, &cfg->enc, nct->dentry)) {
		pr_warn("%px: Encapsulation init failed.\n", dev);
		goto fail;
	}

	/*
	 * Decap init.
	 */
	if (!netfn_capwap_dec_init(&nct->dec, &cfg->dec, nct->dentry)) {
		pr_warn("%px: Decapsulation init failed.\n", dev);
		goto fail;
	}

	/*
	 * Allocate tx pkt steering object.
	 */
	info.cb = netfn_capwap_tun_tx_cb;
	info.app_data = nct;
	info.budget = NETFN_CAPWAP_TUN_TX_BUDGET;
	info.weight = 1;
	nct->tx_steer = netfn_pkt_steer_alloc(&info);
	if (!nct->tx_steer) {
		pr_warn("%px: Unable to allocate tx packet steering object.\n", nct);
		goto fail;
	}

	/*
	 * Allocate rx pkt steering object.
	 */
	info.cb = netfn_capwap_tun_rx_cb;
	info.app_data = nct;
	info.budget = NETFN_CAPWAP_TUN_RX_BUDGET;
	info.weight = 1;
	nct->rx_steer = netfn_pkt_steer_alloc(&info);
	if (!nct->rx_steer) {
		pr_warn("%px: Unable to allocate rx packet steering object.\n", nct);
		goto fail;
	}

	pr_info("%px: Successfully allocated tunnel(%s)\n", nct, dev->name);
	return dev;
fail:
	netfn_capwap_tun_vp_detach(dev);
	unregister_netdev(dev);
	return NULL;
}
EXPORT_SYMBOL(netfn_capwap_tun_alloc);

/*
 * netfn_capwap_tun_free()
 *	Frees the tunnel associated with netdev.
 */
bool netfn_capwap_tun_free(struct net_device *dev)
{
	/*
	 * Check if dev is valid.
	 */
	BUG_ON(!dev);

	/*
	 * Free associated VP.
	 */
	netfn_capwap_tun_vp_detach(dev);

	pr_info("%px: Unregistering tunnel(%s).\n", &global_nc, dev->name);
	unregister_netdev(dev);

	return true;
}
EXPORT_SYMBOL(netfn_capwap_tun_free);

/*
 * netfn_capwap_tun_bind()
 *	Binds tunnel netdev to next dev.
 */
bool netfn_capwap_tun_bind(struct net_device *dev, struct net_device *next)
{
	struct netfn_capwap_tun_enc *enc;
	struct net_device *old_next_dev;
	struct netfn_capwap_tun *nct;

	BUG_ON(!dev);
	BUG_ON(!next);

	nct = netdev_priv(dev);
	enc = &nct->enc;

	dev_hold(next);
	old_next_dev = xchg(&nct->next_dev, next);
	BUG_ON(old_next_dev != nct->vp_dev);
	netdev_rx_handler_register(next, &netfn_capwap_tun_rx, dev);

	pr_info("%px: Successfully binded dev(%s)-->next(%s)\n", nct, dev->name, next->name);
	return true;
}
EXPORT_SYMBOL(netfn_capwap_tun_bind);

/*
 * netfn_capwap_tun_unbind()
 *	Unbinds the tunnel from next hop.
 */
bool netfn_capwap_tun_unbind(struct net_device *dev)
{
	struct net_device *old_next_dev;
	struct netfn_capwap_tun *nct;

	BUG_ON(!dev);

	nct = netdev_priv(dev);

	old_next_dev = xchg(&nct->next_dev, nct->vp_dev);
	BUG_ON(old_next_dev == nct->vp_dev);

	netdev_rx_handler_unregister(old_next_dev);
	dev_put(old_next_dev);

	pr_info("%px: Successfully unbinded(%s)\n", nct, dev->name);
	return true;
}

/*
 * netfn_capwap_tun_stats_get()
 *	Gets tunnel stats corresponding to dev.
 */
int netfn_capwap_tun_stats_get(struct net_device *dev, struct netfn_capwap_tun_stats *stats)
{
	struct netfn_capwap_pkt_stats *pkt_stats = &stats->pkts;
	struct netfn_capwap_tun *nct;

	BUG_ON(!dev);
	BUG_ON(!stats);

	nct = netdev_priv(dev);

	/*
	 * Read packet stats atomically.
	 */
	pkt_stats->rx_pkts = READ_ONCE(nct->stats.rx_pkts);
	pkt_stats->rx_bytes = READ_ONCE(nct->stats.rx_bytes);
	pkt_stats->tx_pkts = READ_ONCE(nct->stats.tx_pkts);
	pkt_stats->tx_bytes = READ_ONCE(nct->stats.tx_bytes);
	pkt_stats->rx_failures = READ_ONCE(nct->stats.rx_failures);
	pkt_stats->tx_failures = READ_ONCE(nct->stats.tx_failures);

	/*
	 * Read encap, decap stats.
	 */
	netfn_capwap_enc_stats_read(&nct->enc, stats);
	netfn_capwap_dec_stats_read(&nct->dec, stats);

	pr_info("%px: Stats get successful.\n", nct);
	return 0;
}
EXPORT_SYMBOL(netfn_capwap_tun_stats_get);
