/*
 * netfn_capwapmgr_tun.c
 *	Network function's CAPWAP manager's tunnel configuration.
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

#include <linux/debugfs.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/netdevice.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/timer.h>

#include <netfn_capwapmgr.h>
#include <netfn_capwapmgr_metaheader.h>
#include "netfn_capwapmgr_priv.h"

/*
 * netfn_capwapmgr_tun_stats_print()
 *	API to print the CAPWAP offload stats associated with the tunnel.
 */
static void netfn_capwapmgr_tun_stats_print(struct seq_file *sf, struct net_device *dev)
{
	BUG_ON(1);
}

/*
 * netfn_capwapmgr_tun_get_reply_tuple()
 *	Get reply direction tuple from tunnel cfg.
 */
static inline void netfn_capwapmgr_tun_get_reply_tuple(struct netfn_capwapmgr_tun_cfg *cfg, netfn_tuple_t *tuple)
{
	BUG_ON(1);
}

/*
 * netfn_capwapmgr_tun_cfg2destroy_rule()
 *	API to get original and reply flow destroy rule from tun cfg
 */
static inline void netfn_capwapmgr_tun_cfg2destroy_rule(struct netfn_capwapmgr_tun_cfg *cfg, struct netfn_flowmgr_destroy_rule *original, struct netfn_flowmgr_destroy_rule *reply)
{
	BUG_ON(1);
}

/*
 * netfn_capwapmgr_tun_flow_stats_sync()
 *	Timer function to handle stats sync from flow manager.
 */
static void netfn_capwapmgr_tun_flow_stats_sync(netfn_tuple_t *tuple, struct netfn_capwapmgr_flow_stats *stats)
{
	BUG_ON(1);
}

/*
 * netfn_capwamgr_tun_flow_rule_create()
 *	Create flow rule.
 *
 * This API creates flow rule based on the tunnel configuration.
 */
static netfn_capwapmgr_ret_t netfn_capwapmgr_tun_flow_rule_create(struct netfn_capwapmgr_tun_cfg *cfg)
{
	BUG_ON(1);
	return 1;
}

/*
 * netfn_capwapmgr_tun_flow_rule_destroy().
 *	Destroy flow rule assocaited with the tunnel.
 */
static netfn_capwapmgr_ret_t netfn_capwapmgr_tun_flow_rule_destroy(struct netfn_capwapmgr_tun_cfg *cfg)
{
	BUG_ON(1);
	return 1;
}

/*
 * netfn_capwapmgr_tun_dtls_tunnel_alloc()
 *	Allocate a DTLS netdevice.
 *
 * This API Allocates the DTLS netdevice in the offload engine.
 * Also creates Encap and Decap sessions under the netdevice.
 */
static struct net_device *netfn_capwapmgr_tun_dtls_tunnel_alloc(struct netfn_dtls_cfg *encap, struct netfn_dtls_cfg *decap, netfn_tuple_t *tuple)
{
	BUG_ON(1);

	return NULL;
}

/*
 * netfn_capwapmgr_tun_dtls_tunnel_free()
 *	Free DTLS netdevice.
 *
 * This API deletes encap and decap sessions.
 * It also frees the DTLS netdevice.
 */
static netfn_capwapmgr_ret_t netfn_capwapmgr_tun_dtls_tunnel_free(struct net_device *dev)
{
	BUG_ON(1);

	return 1;
}

/*
 * netfn_capwapmgr_tun_alloc()
 *	Allocate a CAPWAP tunnel.
 */
struct net_device *netfn_capwapmgr_tun_alloc(struct netfn_capwapmgr_tun_cfg *cfg)
{
	BUG_ON(1);
	return NULL;
}
EXPORT_SYMBOL(netfn_capwapmgr_tun_alloc);

/*
 * netfn_capwapmgr_tun_free()
 *	Free the CAPWAP tunnel.
 */
bool netfn_capwapmgr_tun_free(struct net_device *dev)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tun_free);

/*
 * netfn_capwapmgr_tun_update()
 *	Update tunnel configuration.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tun_update(struct net_device *dev, union netfn_capwapmgr_tun_update *cfg)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tun_cfg_update);

/*
 * netfn_capwapmgr_tun_get_dtls_dev()
 *	Get the DTLS dev associated with the tunnel.
 */
struct net_device *netfn_capwapmgr_tun_get_dtls_dev(struct net_device *dev)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tun_get_dtls_dev);

/*
 * netfn_capwapmgr_tun_get_stats()
 *	Get stats associated with capwap tunnel
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tun_get_stats(struct net_device *dev, struct netfn_capwapmgr_stats *stats)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tun_get_stats);
