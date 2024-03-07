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

#include <linux/debugfs.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

#include <netfn_capwapmgr.h>
#include "netfn_capwapmgr_priv.h"
#include "netfn_capwapmgr_tun.h"
#include "netfn_capwapmgr_tunid.h"

/*
 * netfn_capwapmgr_tunid_dev_alloc()
 *	Allocate a CAPWAP tunid dev.
 */
struct net_device *netfn_capwapmgr_tunid_dev_alloc(void)
{
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct netfn_capwapmgr *mgr = &g_mgr;
	struct net_device *dev;

	dev = netfn_capwap_tunid_alloc(sizeof(struct netfn_capwapmgr_tunid_ctx));
	if (!dev) {
		netfn_capwapmgr_warn("Failed to allocate tunid dev\n");
		return NULL;
	}

	/*
	 * dev_put: netfn_capwapmgr_tunid_free.
	 */
	dev_hold(dev);
	ctx = netfn_capwap_tunid_pvt_get(dev);

	spin_lock_init(&ctx->lock);
	bitmap_zero(ctx->active_tunnels, NETFN_CAPWAPMGR_TUNNELS_MAX);
	memset(ctx->tunnels, 0, sizeof(ctx->tunnels));

	/*
	 * deref: netfn_capwapmgr_tunid_free.
	 */
	ctx->mgr = netfn_capwapmgr_ref(mgr);
	return dev;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_dev_alloc);

/*
 * netfn_capwapmgr_tunid_dev_free()
 *	Free CAPWAP tunid dev.
 *
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_dev_free(struct net_device *dev)
{
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct netfn_capwapmgr *mgr = &g_mgr;

	ctx = netfn_capwap_tunid_pvt_get(dev);
	if (!bitmap_empty(ctx->active_tunnels, NETFN_CAPWAPMGR_TUNNELS_MAX)) {
		netfn_capwapmgr_warn("%p: Tun-ID still contains active tunnels\n", dev);
   		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	/*
	 * hold:netfn_capwapmgr_tunid_alloc()
	 */
	dev_put(dev);
	if (!netfn_capwap_tunid_free(dev)) {
		netfn_capwapmgr_warn("%px: Failed to free tunid dev\n", dev);
		dev_hold(dev);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	/*
	 * ref:netfn_capwapmgr_tunid_alloc().
	 */
	netfn_capwapmgr_deref(mgr);
	return NETFN_CAPWAPMGR_SUCCESS;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_dev_free);

/*
 * netfn_capwapmgr_tunid_toggle_state
 *	Enable/disable a tunid tunnel.
 *
 * NOTE:This API should be called with rtnl lock held.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_toggle_state(struct net_device *dev, uint8_t tun_id, bool enable)
{
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct net_device *tunid_dev = NULL;
	long bit_pos;

	BUG_ON(!rtnl_is_locked());

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	/*
	 * Get tunid dev context from offload engine.
	 */
	ctx = netfn_capwap_tunid_pvt_get(dev);
	if (!test_bit(tun_id, ctx->active_tunnels)) {
		netfn_capwapmgr_warn("%px: Tunnel Id %d Does not exist\n", dev, tun_id);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	tunid_ctx = ctx->tunnels[tun_id];
	tunid_dev = tunid_ctx->capwap_dev;

	bit_pos = __builtin_ffs(NETFN_CAPWAPMGR_TUN_STATE_ENABLED) - 1;
	if (enable) {
		dev_open(tunid_dev, NULL);
		__set_bit(bit_pos, tunid_ctx->state);
	} else {
		dev_close(tunid_dev);
		__clear_bit(bit_pos, tunid_ctx->state);
	}

	return NETFN_CAPWAPMGR_SUCCESS;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_toggle_state);

/*
 * netfn_capwapmgr_tunid_del()
 *	Deletes the tunnel with a specific ID under the tunid dev.
 *
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_del(struct net_device *dev, uint8_t tun_id)
{
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct net_device *tunid_dev = NULL;

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	/*
	 * Get tunid dev context from offload engine.
	 */
	ctx = netfn_capwap_tunid_pvt_get(dev);
	spin_lock(&ctx->lock);
	if (!__test_and_clear_bit(tun_id, ctx->active_tunnels)) {
		netfn_capwapmgr_warn("%px: Tunnel Id %d Does not exist\n", dev, tun_id);
		spin_unlock(&ctx->lock);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	tunid_ctx = ctx->tunnels[tun_id];
	tunid_dev = tunid_ctx->capwap_dev;
	spin_unlock(&ctx->lock);

	/*
	 * De-initialize the tunnel.
	 */
	if (!netfn_capwapmgr_tun_deinit(tunid_ctx)) {
		netfn_capwapmgr_warn("%px: Tunnel Id %d deinit failed\n", dev, tun_id);
		goto fail;
	}

	/*
	 * dev_hold:netfn_capwapmgr_tunid_add()
	 */
	dev_put(tunid_dev);

	/*
	 * Delete capwap tunnel.
	 */
	if (!netfn_capwap_tunid_del(dev, tun_id)) {
		netfn_capwapmgr_warn("%px: Failed to free capwap tun \n", dev);
		dev_hold(tunid_dev);
		goto fail;
	}

	/*
	 * Clear the tunnel context
	 */
	ctx->tunnels[tun_id] = NULL;
	return NETFN_CAPWAPMGR_SUCCESS;
fail:
	__set_bit(tun_id, ctx->active_tunnels);
	return NETFN_CAPWAPMGR_ERROR_MAX;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_del);

/*
 * netfn_capwapmgr_tunid_add()
 *	Adds a new tunnel with specific ID under the tunid dev.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_add(struct net_device *dev, uint8_t tun_id, struct netfn_capwapmgr_tun_cfg *cfg)
{
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct net_device *tunid_dev = NULL;
	bool tun_init_ret = false;

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	/*
	 * Validate and initialize the configuration.
	 */
	if (netfn_capwapmgr_tun_init_cfg(cfg) != NETFN_CAPWAPMGR_SUCCESS) {
		netfn_capwapmgr_warn("%px: Tunnel Id %d Failed to validate tunnel config\n", dev, tun_id);
		goto fail;
	}

	tunid_dev = netfn_capwap_tunid_add(dev, tun_id, &cfg->capwap, &cfg->tuple, sizeof(struct netfn_capwapmgr_tun_ctx));
	if (!tunid_dev) {
		netfn_capwapmgr_warn("%px: Tunnel Id %d add failed\n", dev, tun_id);
		goto fail;
	}

	tunid_ctx = netfn_capwap_tun_pvt_get(tunid_dev);
	tunid_ctx->capwap_dev = tunid_dev;

	/*
	 * Initialize the tunnel.
	 */
	tun_init_ret = netfn_capwapmgr_tun_init(tunid_ctx, cfg);
	if (!tun_init_ret) {
		netfn_capwapmgr_warn("%px:Failed to initialize the capwap tunnel\n", cfg);
		goto fail;
	}

	/*
	 * Get tunid dev context from offload engine.
	 */
	ctx = netfn_capwap_tunid_pvt_get(dev);
	spin_lock(&ctx->lock);
	if (__test_and_set_bit(tun_id, ctx->active_tunnels)) {
		netfn_capwapmgr_warn("%px: Tunnel ID %d unavailable\n", dev, tun_id);
		spin_unlock(&ctx->lock);
		goto fail;
	}

	/*
	 * dev_put:netfn_capwapmgr_tunid_del()
	 */
	dev_hold(tunid_dev);

	ctx->tunnels[tun_id] = tunid_ctx;
	spin_unlock(&ctx->lock);
	return NETFN_CAPWAPMGR_SUCCESS;

fail:
	if (tun_init_ret) {
		netfn_capwapmgr_tun_deinit(tunid_ctx);
	}

	if (tunid_dev) {
		netfn_capwap_tunid_del(dev, tun_id);
	}

	return NETFN_CAPWAPMGR_ERROR_MAX;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_add);

/*
 * netfn_capwapmgr_tunid_get_dtls_dev()
 *	Get DTLS dev associated with the tunnel
 *
 * A reference is held on the returned DTLS dev.
 */
struct net_device *netfn_capwapmgr_tunid_get_dtls_dev(struct net_device *dev, uint8_t tun_id)
{
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;
	struct net_device *dtls_dev = NULL;

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return dtls_dev;
	}

	ctx = netfn_capwap_tunid_pvt_get(dev);

	spin_lock(&ctx->lock);
	tunid_ctx = ctx->tunnels[tun_id];
	if (test_bit(tun_id, ctx->active_tunnels)) {
		dtls_dev = netfn_capwapmgr_tun_get_dtls_dev(tunid_ctx->capwap_dev);
		dev_hold(dtls_dev);
	}

	spin_unlock(&ctx->lock);
	return dtls_dev;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_get_dtls_dev);

/*
 * netfn_capwapmgr_tunid_update()
 *	Update tunnel configuration
 *
 * NOTE:This API should be called with rtnl lock held.
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_update(struct net_device *dev, uint8_t tun_id, struct netfn_capwapmgr_tun_update *cfg)
{
	netfn_capwapmgr_ret_t ret = NETFN_CAPWAPMGR_ERROR_MAX;
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;

	BUG_ON(!rtnl_is_locked());

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return NETFN_CAPWAPMGR_ERROR_MAX;
	}

	ctx = netfn_capwap_tunid_pvt_get(dev);

	tunid_ctx = ctx->tunnels[tun_id];
	if (test_bit(tun_id, ctx->active_tunnels)) {
		ret = netfn_capwapmgr_tun_update(tunid_ctx->capwap_dev, cfg);
	}

	return ret;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_update);

/*
 * netfn_capwapmgr_tunid_get_stats()
 *	Get stats associated with the tunnel ID
 */
netfn_capwapmgr_ret_t netfn_capwapmgr_tunid_get_stats(struct net_device *dev, uint8_t tun_id, struct netfn_capwapmgr_tun_stats *stats)
{
	netfn_capwapmgr_ret_t ret = NETFN_CAPWAPMGR_ERROR_MAX;
	struct netfn_capwapmgr_tun_ctx *tunid_ctx = NULL;
	struct netfn_capwapmgr_tunid_ctx *ctx = NULL;

	/*
	 * Check for out of bound tunnel id.
	 */
	if(tun_id >= NETFN_CAPWAPMGR_TUNNELS_MAX) {
		netfn_capwapmgr_warn("%p: Tunnel ID %d out of bound\n", dev, tun_id);
		return ret;
	}

	ctx = netfn_capwap_tunid_pvt_get(dev);

	spin_lock(&ctx->lock);
	tunid_ctx = ctx->tunnels[tun_id];
	if (test_bit(tun_id, ctx->active_tunnels)) {
		ret = netfn_capwapmgr_tun_get_stats(tunid_ctx->capwap_dev, stats);
	}

	spin_unlock(&ctx->lock);
	return ret;
}
EXPORT_SYMBOL(netfn_capwapmgr_tunid_get_stats);
