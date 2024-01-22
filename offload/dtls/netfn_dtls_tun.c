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
#include <linux/if_arp.h>
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>

#include <ppe_vp_public.h>
#include "netfn_dtls_priv.h"

static int netfn_dtls_rcv_map = 0x8;
module_param(netfn_dtls_rcv_map, int, 0644);
MODULE_PARM_DESC(netfn_dtls_rcv_map, "DTLS receive core Map");

/*
 * netfn_dtls_tun_get_stats()
 *	Update the summary stats.
 */
static void netfn_dtls_tun_get_stats(struct netfn_dtls_tun *tun, struct netfn_dtls_tun_stats *stats)
{
	int words;
	int cpu;
	int i;

	words = (sizeof(*stats) / sizeof(uint64_t));
	memset(stats, 0, sizeof(*stats));

	/*
	 * All statistics are 64bit. So we can just iterate by words.
	 */
	for_each_possible_cpu(cpu) {
		const struct netfn_dtls_tun_stats *sp = per_cpu_ptr(tun->stats_pcpu, cpu);
		uint64_t *stats_ptr = (uint64_t *)stats;
		uint64_t *sp_ptr = (uint64_t *)sp;

		for (i = 0; i < words; i++, stats_ptr++, sp_ptr++)
			*stats_ptr += *sp_ptr;
	}
}

/*
 * netfn_dtls_tun_print_stats()
 *	Read device statistics.
 */
static ssize_t netfn_dtls_tun_print_stats(struct file *fp, char __user *ubuf, size_t sz, loff_t *ppos)
{
	struct netfn_dtls_tun *tun = fp->private_data;
	struct netfn_dtls_tun_stats stats;
	ssize_t len = 0;
	ssize_t max_len;
	char *buf;

	netfn_dtls_tun_get_stats(tun, &stats);

	/*
	 * We need to calculate required string buffer for stats, else full stats may not be captured.
	 */
	max_len = (sizeof(stats) / sizeof(uint64_t)) * NETFN_DTLS_MAX_STR_LEN; /* Members */
	max_len += NETFN_DTLS_MAX_STR_LEN; /* Encap heading */
	max_len += NETFN_DTLS_MAX_STR_LEN; /* Decap heading */

	buf = vzalloc(max_len);
	if (!buf) {
		pr_warn("%px: failed to allocate print buffer (%zu)", tun, max_len);
		return 0;
	}

	/*
	 * Create Stats strings.
	 * TODO: convert this to snprintf engine
	 */
	len = snprintf(buf, max_len, "Session Added: %llu\n", stats.session_alloc);
	len += snprintf(buf + len, max_len - len, "session Removed: %llu\n", stats.session_free);
	len += snprintf(buf + len, max_len - len, "encap switch: %llu\n", stats.encap_switch);
	len += snprintf(buf + len, max_len - len, "Device Encapsulation Statistics:\n");
	len += snprintf(buf + len, max_len - len, "\tTx packets: %llu\n", stats.tx_pkts);
	len += snprintf(buf + len, max_len - len, "\tTx bytes: %llu\n", stats.tx_bytes);
	len += snprintf(buf + len, max_len - len, "\tTx VP Exception: %llu\n", stats.tx_vp_exp);
	len += snprintf(buf + len, max_len - len, "\tTx host: %llu\n", stats.tx_host);
	len += snprintf(buf + len, max_len - len, "\tTx Error: %llu\n", stats.tx_fail);
	len += snprintf(buf + len, max_len - len, "\tTx Fail session: %llu\n", stats.tx_fail_session);

	len += snprintf(buf + len, max_len - len, "Device Decapsulation Statistics:\n");
	len += snprintf(buf + len, max_len - len, "\tRx packets: %llu\n", stats.rx_pkts);
	len += snprintf(buf + len, max_len - len, "\tRx bytes: %llu\n", stats.rx_bytes);
	len += snprintf(buf + len, max_len - len, "\tRx Error: %llu\n", stats.rx_fail);
	len += snprintf(buf + len, max_len - len, "\tRx Fail linearization: %llu\n", stats.rx_fail_linearize);
	len += snprintf(buf + len, max_len - len, "\tRx Fail session: %llu\n", stats.rx_fail_session);

	len = simple_read_from_buffer(ubuf, sz, ppos, buf, len);
	vfree(buf);

	return len;
}

/*
 * netfn_dtls_tun_open()
 *	Netdevice open handler.
 */
static int netfn_dtls_tun_open(struct net_device *dev)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	int err;

	/*
	 * Set VP netdevice to UP state.
	 */
	err = dev_open(tun->vp_dev, NULL);
	if (err < 0) {
		pr_err("%s: Failed to Open netdevce, error(%d)\n", dev->name, err);
		return err;
	}

	netif_start_queue(dev);
	return 0;
}

/*
 * netfn_dtls_tun_stop()
 *	Netdevice stop handler.
 */
static int netfn_dtls_tun_stop(struct net_device *dev)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);

	dev_close(tun->vp_dev);
	netif_stop_queue(dev);
	return 0;
}

/*
 * netfn_dtls_tun_stats64()
 *	Handler to fetch netdevice rtnl statistics.
 */
static void netfn_dtls_tun_stats64(struct net_device *dev, struct rtnl_link_stats64 *rtnl_stats)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_tun_stats stats = {0};

	memset(rtnl_stats, 0, sizeof(*rtnl_stats));
	netfn_dtls_tun_get_stats(tun, &stats);

	rtnl_stats->tx_packets = stats.tx_pkts;
	rtnl_stats->tx_bytes = stats.tx_bytes;
	rtnl_stats->tx_dropped = stats.tx_fail;
	rtnl_stats->rx_packets = stats.rx_pkts;
	rtnl_stats->rx_bytes = stats.rx_bytes;
	rtnl_stats->rx_dropped = stats.rx_fail;
}

/*
 * netfn_dtls_tun_mtu()
 *	Update device MTU.
 */
static int netfn_dtls_tun_mtu(struct net_device *dev, int mtu)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_session *ses;
	uint32_t overhead;

	/*
	 * Get Session overhead.
	 */
	rcu_read_lock_bh();

	ses = rcu_dereference_bh(tun->enc.active);
	if (!ses) {
		pr_err("%s: Encapsulation session must be added first\n", dev->name);

		rcu_read_unlock_bh();
		return -EINVAL;
	}

	overhead = netfn_dtls_session_get_overhead(ses);
	rcu_read_unlock_bh();

	dev->mtu = mtu - overhead;
	return 0;
}

/*
 * netfn_dtls_tun_vp_err()
 *	VP callback for outer exception.
 */
bool netfn_dtls_tun_vp_err(struct net_device *dev, struct sk_buff *skb, void *cb_data)
{
	struct netfn_dtls_tun_stats *tun_stats;
	struct netfn_dtls_tun *tun = cb_data;

	tun_stats = this_cpu_ptr(tun->stats_pcpu);
	tun_stats->tx_fail++;
	consume_skb(skb);
	return true;
}

/*
 * netfn_dtls_tun_vp_alloc()
 *	Allocate VP for DTLS netdevice.
 */
static struct net_device *netfn_dtls_tun_alloc_vp(struct net_device *dev)
{
	struct ppe_vp_ai vpai = {0};
	struct net_device *vp_dev;

	/*
	 * Allocate new VP
	 */
	vpai.type = PPE_VP_TYPE_SW_L3;
	vpai.usr_type = PPE_VP_USER_TYPE_ACTIVE;
	vpai.core_mask = netfn_dtls_rcv_map;
	vpai.dst_cb = netfn_dtls_tun_rcv;
	vpai.dst_cb_data = netdev_priv(dev);
	vpai.src_cb = netfn_dtls_tun_vp_err;
	vpai.src_cb_data = netdev_priv(dev);
	vpai.queue_num = 0;

	vp_dev = ppe_vp_alloc_dev(dev, &vpai);
	if (!vp_dev) {
		pr_debug("%px: Failed to allocate VP, status(%d).\n", dev, vpai.status);
		return NULL;
	}

	return vp_dev;
}

/*
 * netfn_dtls_tun_destructor()
 *	Free netdevice memory.
 */
static void netfn_dtls_tun_destructor(struct net_device *dev)
{
	struct netfn_dtls_drv *drv = &g_dtls_drv;
	struct netfn_dtls_tun *tun = netdev_priv(dev);

	/*
	 * Conditionally free the resources as this may be called
	 * during partial cleanup on dev setup failure.
	 */
	if (tun->vp_dev) {
		ppe_vp_free_dev(tun->vp_dev);
		tun->vp_dev = NULL;
	}

	if (tun->stats_pcpu) {
		free_percpu(tun->stats_pcpu);
		tun->stats_pcpu = NULL;
	}

	debugfs_remove_recursive(tun->dentry);
	tun->dentry = NULL;

	pr_info("%px: DTLS device freed\n", dev);
	free_netdev(dev);
	netfn_dtls_drv_deref(drv);
}

/*
 * DTLS device callbacks.
 */
static const struct net_device_ops dtls_tun_ops = {
	.ndo_open = netfn_dtls_tun_open,
	.ndo_stop = netfn_dtls_tun_stop,
	.ndo_start_xmit = netfn_dtls_tun_xmit,
	.ndo_get_stats64 = netfn_dtls_tun_stats64,
	.ndo_change_mtu = netfn_dtls_tun_mtu,
};

/*
 * DTLS stats callback.
 */
const struct file_operations netfn_dtls_tun_file_ops = {
	.open = simple_open,
	.llseek = default_llseek,
	.read = netfn_dtls_tun_print_stats,
};

/*
 * netfn_dtls_tun_setup()
 *	Setup dtls connection device.
 */
static void netfn_dtls_tun_setup(struct net_device *dev)
{
	dev->addr_len = ETH_ALEN;
	dev->mtu = ETH_DATA_LEN;

	/*
	 * Set based on maximum possible headers.
	 */
	dev->needed_headroom = NETFN_DTLS_TUN_MAX_HEADROOM;
	dev->needed_tailroom = NETFN_DTLS_TUN_MAX_TAILROOM;
	dev->type = ARPHRD_TUNNEL;
	dev->ethtool_ops = NULL; /* TODO:  Can we use ethtool interface for device statistics */
	dev->header_ops = NULL;
	dev->netdev_ops = &dtls_tun_ops;
	dev->priv_destructor = netfn_dtls_tun_destructor;

	/*
	 * Assign random ethernet address.
	 */
	random_ether_addr(dev->dev_addr);
	memset(dev->broadcast, 0xff, dev->addr_len);
	memcpy(dev->perm_addr, dev->dev_addr, dev->addr_len);
}

/*
 * netfn_dtls_tun_session_del()
 *	Delete session under tunnel.
 */
void netfn_dtls_tun_session_del(struct net_device *dev, bool encap, __be16 epoch)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_session *ses_to_free = NULL;
	struct netfn_dtls_tun_stats *tun_stats;
	struct netfn_dtls_tun_state *state;
	struct netfn_dtls_session *ses;

	tun_stats = this_cpu_ptr(tun->stats_pcpu);
	state = encap ? &tun->enc : &tun->dec;

	mutex_lock(&tun->lock);

	/*
	 * active State
	 */
	ses = rcu_dereference_protected(state->active, lockdep_is_held(&tun->lock));
	if (ses && ses->epoch == epoch) {
		RCU_INIT_POINTER(state->active, NULL);
		ses_to_free = ses;
		goto done;
	}

	pr_debug("%px: No Active session present for epoch(%u)\n", dev, ntohs(epoch));

	/*
	 * Pending State
	 */
	ses = rcu_dereference_protected(state->pending, lockdep_is_held(&tun->lock));
	if (ses && ses->epoch == epoch) {
		RCU_INIT_POINTER(state->pending, NULL);
		ses_to_free = ses;
	}

done:
	mutex_unlock(&tun->lock);

	if (!ses_to_free)
		return;

	/*
	 * Wait for all reader to complete and Free.
	 */
	synchronize_rcu();

	netfn_dtls_session_free(ses_to_free);
	tun_stats->session_free++;
	pr_debug("%px: Session deleted for epoch(%u)\n", dev, ntohs(epoch));
}
EXPORT_SYMBOL(netfn_dtls_tun_session_del);

/*
 * netfn_dtls_tun_session_encap_switch()
 *	Switch pending state into active state. Free active state if any.
 */
void netfn_dtls_tun_session_encap_switch(struct net_device *dev)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_tun_stats *tun_stats;
	struct netfn_dtls_tun_state *state;
	struct netfn_dtls_session *pending;
	struct netfn_dtls_session *active;

	tun_stats = this_cpu_ptr(tun->stats_pcpu);
	state = &tun->enc;

	/*
	 * Switch Pending into Active
	 */
	mutex_lock(&tun->lock);

	active = rcu_dereference_protected(state->active, lockdep_is_held(&tun->lock));
	pending = rcu_dereference_protected(state->pending, lockdep_is_held(&tun->lock));
	RCU_INIT_POINTER(state->pending, NULL);
	rcu_assign_pointer(state->active, pending);

	mutex_unlock(&tun->lock);

	/*
	 * If there was any active state, Free it.
	 */
	tun_stats->encap_switch++;
	if (!active) {
		return;
	}

	/*
	 * Wait for all reader to complete and Free.
	 */
	synchronize_rcu();

	pr_debug("%px: Session freed for epoch(%u)\n", dev, ntohs(active->epoch));
	netfn_dtls_session_free(active);
	tun_stats->session_free++;
}
EXPORT_SYMBOL(netfn_dtls_tun_session_encap_switch);

/*
 * netfn_dtls_tun_session_add()
 *	Add session under tunnel.
 */
int netfn_dtls_tun_session_add(struct net_device *dev, struct netfn_dtls_cfg *cfg, netfn_tuple_t *t)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_session *ses_to_free = NULL;
	struct netfn_dtls_tun_stats *tun_stats;
	struct netfn_dtls_session *pending;
	struct netfn_dtls_session *active;
	struct netfn_dtls_tun_state *state;
	__be16 epoch = htons(cfg->epoch);
	struct netfn_dtls_session *ses;
	bool encap;

	tun_stats = this_cpu_ptr(tun->stats_pcpu);
	encap = (cfg->flags & NETFN_DTLS_FLAG_ENC);
	state = encap ? &tun->enc : &tun->dec;

	ses = netfn_dtls_session_alloc(cfg, t, tun);
	if (!ses) {
		pr_err("%px: Unable to allocate session for epoch(%u)\n", dev, cfg->epoch);
		return -ENOMEM;
	}

	mutex_lock(&tun->lock);

	/*
	 * Try adding to Active first, if session is already present add it to
	 * pending. Caller will have to call switch()
	 */
	active = rcu_dereference_protected(state->active, lockdep_is_held(&tun->lock));
	if (!active) {
		rcu_assign_pointer(state->active, ses);
		goto done;
	}

	if (active->epoch == epoch) {
		goto duplicate;
	}

	pending = rcu_dereference_protected(state->pending, lockdep_is_held(&tun->lock));
	if (!pending) {
		rcu_assign_pointer(state->pending, ses);
		goto done;
	}

	if (pending->epoch == epoch) {
		goto duplicate;
	}

	/*
	 * Free pending and assign new.
	 */
	ses_to_free = pending;
	rcu_assign_pointer(state->pending, ses);

done:
	tun_stats->session_alloc++;
	mutex_unlock(&tun->lock);

	/*
	 * Free if any pending was present.
	 * This can happen when there is no change cipher on pending state.
	 */
	if (!ses_to_free)
		return 0;

	pr_debug("%px: Free session with epoch(%u)\n", dev, ses_to_free->epoch);
	synchronize_rcu();
	netfn_dtls_session_free(ses_to_free);
	tun_stats->session_free++;
	return 0;

duplicate:
	pr_err("%px: Duplicate session add for epoch(%u)\n", dev, cfg->epoch);
	mutex_unlock(&tun->lock);
	netfn_dtls_session_free(ses);
	return -EEXIST;
}
EXPORT_SYMBOL(netfn_dtls_tun_session_add);

#define NETFN_RCU_GET_INIT(rcu, ptr) {\
		ptr = rcu_dereference_protected(rcu, lockdep_is_held(&tun->lock)); \
		RCU_INIT_POINTER(rcu, NULL); \
		}

/*
 * netfn_dtls_tun_free()
 *	Delete DTLS device associated with the netdevice.
 */
void netfn_dtls_tun_free(struct net_device *dev)
{
	struct netfn_dtls_tun *tun = netdev_priv(dev);
	struct netfn_dtls_session *enc_pending;
	struct netfn_dtls_session *enc_active;
	struct netfn_dtls_session *dec_active;
	struct netfn_dtls_session *dec_pending;

	/*
	 * Free all associated sessions.
	 */
	mutex_lock(&tun->lock);
	NETFN_RCU_GET_INIT(tun->enc.pending, enc_pending);
	NETFN_RCU_GET_INIT(tun->enc.active, enc_active);
	NETFN_RCU_GET_INIT(tun->dec.pending, dec_pending);
	NETFN_RCU_GET_INIT(tun->dec.active, dec_active);
	mutex_unlock(&tun->lock);

	/*
	 * Wait for all reader to complete and Free.
	 */
	synchronize_rcu();
	if (enc_pending)
		netfn_dtls_session_free(enc_pending);
	if (enc_active)
		netfn_dtls_session_free(enc_active);
	if (dec_active)
		netfn_dtls_session_free(dec_active);
	if (dec_pending)
		netfn_dtls_session_free(dec_pending);

	/*
	 * Bring down the device and unregister from linux.
	 */
	unregister_netdev(dev);
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
	int status;

	/*
	 * Netdevice allocation.
	 */
	dev = alloc_netdev(sizeof(*tun) + pvt_sz, "dtlstun%d", NET_NAME_ENUM, netfn_dtls_tun_setup);
	if (!dev) {
		pr_err("%px: Failed to allocate DTLS device\n", drv);
		return NULL;
	}

	/*
	 * Initialize device private structure.
	 */
	tun = netdev_priv(dev);
	memset(tun, 0, sizeof(*tun));
	tun->dev = dev;

	mutex_init(&tun->lock);

	/*
	 * dereference: dev->priv_destructor
	 */
	netfn_dtls_drv_ref(drv);

	tun->stats_pcpu = alloc_percpu_gfp(struct netfn_dtls_tun_stats, GFP_KERNEL | __GFP_ZERO);
	if (!tun->stats_pcpu) {
		pr_err("%px: Failed to allocate stats memory for encap\n", dev);
		dev->priv_destructor(dev);
		return NULL;
	}

	/*
	 * Allocate VP.
	 */
	tun->vp_dev = netfn_dtls_tun_alloc_vp(dev);
	if (!tun->vp_dev) {
		pr_err("%px: Failed to allocate VP\n", dev);
		dev->priv_destructor(dev);
		return NULL;
	}

	/*
	 * Register netdevice with kernel.
	 * kernels invoke the destructor upon failure
	 */
	status = register_netdev(dev);
	if (status < 0) {
		pr_err("%px: Failed to register netdevce, error(%d)\n", dev, status);
		rtnl_unlock();

		return NULL;
	}

	/*
	 * Set netdevice to UP state.
	 */
	rtnl_lock();
	status = dev_open(dev, NULL);
	rtnl_unlock();
	if (status < 0) {
		pr_err("%px: Failed to Open netdevce, error(%d)\n", dev, status);
		unregister_netdev(dev);
		return NULL;
	}

	tun->dentry = debugfs_create_file(dev->name, S_IRUGO, drv->dentry, tun, &netfn_dtls_tun_file_ops);
	if (!tun->dentry) {
		pr_err("%px: Failed to create debugfs\n", dev);
		unregister_netdev(dev);
		return NULL;
	}

	return dev;
}
EXPORT_SYMBOL(netfn_dtls_tun_alloc);
