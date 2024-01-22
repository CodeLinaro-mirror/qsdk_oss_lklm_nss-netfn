/*
 * netfn_capwapmgr_tunid.c
 *	Network function's CAPWAP manager's tunnel-ID configuration.
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

#include <netfn_capwapmgr.h>
#include "netfn_capwapmgr_priv.h"

/*
 * netfn_capwapmgr_tunid_dev_alloc()
 *	Allocate a CAPWAP tunid dev.
 */
struct net_device *netfn_capwapmgr_tunid_dev_alloc(void)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_dev_alloc);

/*
 * netfn_capwapmgr_tunid_dev_free()
 *	Free CAPWAP tunid dev.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_dev_free(struct net_device *dev)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_dev_free);

/*
 * netfn_capwapmgr_tunid_add()
 *	Adds a new tunnel with specific ID under the tunid dev.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_add(struct net_device *dev, uint8_t tun_id, netfn_capwapmgr_tun_cfg *cfg)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_add);

/*
 * netfn_capwapmgr_tunid_del()
 *	Deletes the tunnel with a specific ID under the tunid dev.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_del(struct net_device *dev, uint8_t tun_id)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_del);

/*
 * netfn_capwapmgr_tunid_get_dtls_dev()
 *	Get DTLS dev associated with the tunnel
 */
struct net_device *netfn_capwapmgr_tunid_get_dtls_dev(struct net_device *dev, uint8_t tun_id)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_get_dtls_dev);

/*
 * netfn_capwapmgr_tunid_update()
 *	Update tunnel configuration
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_update(struct net_device *dev, uint8_t tun_id, union netfn_capwapmgr_tun_cfg_update *cfg)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_cfg_update);

/*
 * netfn_capwapmgr_tunid_get_stats()
 *	Get stats associated with the tunnel ID
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_get_stats(struct net_device *dev, uint8_t tun_id, struct netfn_capwapmgr_stats *stats)
{
	BUG_ON(1);
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_get_stats);
