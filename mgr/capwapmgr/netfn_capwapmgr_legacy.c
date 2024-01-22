/*
 **************************************************************************
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023,2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * nss_capwapmgr.c
 *	Legacy CAPWAP manager
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>

#include <netfn_capwapmgr.h>
#include "netfn_capwapmgr_priv.h"

/*
 * Netfn dtls flags are left shifted by 1.
 */
#define NSS_CAPWAPMGR_DTLS_FLAG_SHIFT_TO_NETFN 1

/*
 * NETFN dtls offload only supports 32bit nounce value
 */
#define NSS_CAPWAPMGR_DTLS_NONCE_LEN_MAX 4

/*
 * netfn_capwapmgr_legacy_rule2tun_cfg
 *	Wrapper API to get tunnel config from legacy tunnel create rules.
 */
static void netfn_capwapmgr_legacy_rule2tun_cfg(struct nss_ipv4_create *v4, struct nss_ipv6_create *v6, struct nss_capwap_rule_msg *capwap_rule, struct nss_dtlsmgr_config *dtls_data, struct netfn_capwapmgr_tun_cfg *cfg)
{
	BUG_ON(1);
}

/*
 * netfn_capwapmgr_netfn_stats2legacy_stats.
 *	Wrapper API to get legacy stats from netfn stats.
 */
static void netfn_capwapmgr_netfn_stats2legacy_stats(struct netfn_capwap_tun_stats *netfn_stats, struct nss_capwap_tunnel_stats *legacy_stats)
{
	BUG_ON(1);
}

/*
 * netfn_capwapmgr_netfn_status2legacy_status.
 *	Wrapper API to get legacy status from netfn status.
 */
static nss_capwapmgr_status_t netfn_capwapmgr_netfn_status2legacy_status(netfn_capwapmgr_status_t netfn_status)
{
	BUG_ON(1);
	return 0;

}
