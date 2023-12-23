/*
 * netfn_capwap_dec.c
 *	Network function's CAPWAP offload decapsulation.
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "netfn_capwap_dec.h"

/*
 * netfn_capwap_dec_stats_show()
 *	Prints the decap stats.
 */
ssize_t netfn_capwap_dec_stats_show(struct netfn_capwap_tun_dec *dec, char *buf, size_t size_wr, ssize_t *stats_sz)
{
	BUG_ON(1);
	return 0;
}

/*
 * netfn_capwap_dec_stats_read()
 *	Reads decap stats.
 */
void netfn_capwap_dec_stats_read(struct netfn_capwap_tun_dec *dec, struct netfn_capwap_tun_stats *dest)
{
	BUG_ON(1);
}

/*
 * netfn_capwap_dec_init()
 *	Initializes the decap object.
 */
bool netfn_capwap_dec_init(struct netfn_capwap_tun_dec *dec, struct netfn_capwap_dec_cfg *cfg, struct dentry *dentry)
{
	BUG_ON(1);
}

/*
 * netfn_capwap_dec_deinit()
 *	Deinitializes the decap object.
 */
void netfn_capwap_dec_deinit(struct netfn_capwap_tun_dec *dec)
{
	BUG_ON(1);
}
