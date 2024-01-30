/*
 * netfn_capwap_enc.h
 *	Network function's CAPWAP offload encapsulation definitions.
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

#ifndef __NETFN_CAPWAP_ENC_H
#define __NETFN_CAPWAP_ENC_H

#define NETFN_CAPWAP_ENC_STATS_STRLEN 20
#define NETFN_CAPWAP_ENC_STATS_WIDTH 82

/*
 * netfn_capwap_enc_stats_read()
 *	Reads encap stats.
 */
void netfn_capwap_enc_stats_read(struct netfn_capwap_tun_enc *enc, struct netfn_capwap_tun_stats *stats);

/*
 * netfn_capwap_enc_init()
 *	Encapsulation initialization.
 */
bool netfn_capwap_enc_init(struct netfn_capwap_tun_enc *enc, struct netfn_capwap_enc_cfg *cfg, struct dentry *dentry);

/*
 * netfn_capwap_enc()
 *	Encapsulates the packets/s.
 */
bool netfn_capwap_enc(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb, struct sk_buff_head *q_frag);

/*
 * netfn_capwap_enc_mtu_update()
 *	Update encap mtu
 */
void netfn_capwap_enc_mtu_update(struct netfn_capwap_tun_enc *enc, unsigned int mtu);

#endif /* __NETFN_CAPWAP_ENC_H */
