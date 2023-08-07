/*
 * netfn_capwap_enc.h
 *	Network function's CAPWAP offload encapsulation definitions.
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

#ifndef __NETFN_CAPWAP_ENC_H
#define __NETFN_CAPWAP_ENC_H

/*
 * netfn_capwap_enc_init()
 *	Encapsulation initialization.
 */
void netfn_capwap_enc_init(struct netfn_capwap_tun *nct, struct netfn_capwap_tun_cfg *cfg);

/*
 * netfn_capwap_enc_frag()
 *	Fragment the packet.
 */
int netfn_capwap_enc_frag(struct netfn_capwap_tun *nct, struct sk_buff *skb, struct sk_buff_head *q_frag);

/*
 * netfn_capwap_enc()
 *	Encapsulates the packet.
 */
int netfn_capwap_enc(struct netfn_capwap_enc *enc, struct sk_buff_head *q_frag, struct netfn_capwap_enc_mdata *mdata)

#endif /* __NETFN_CAPWAP_ENC_H */
