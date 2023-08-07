/*
 * netfn_capwap_dec.h
 *	Network function's CAPWAP offload decapsulation defintions.
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

#ifndef __NETFN_CAPWAP_DEC_H
#define __NETFN_CAPWAP_DEC_H

/*
 * netfn_capwap_dec_rx_skbs()
 *	Receive an SKB queueu for processing.
 */
void netfn_capwap_dec_rx_skbs(void *app_data, struct sk_buff_head *q_head);

/*
 * netfn_capwap_dec_init()
 *	Initializes the decap object.
 */
void netfn_capwap_dec_init(struct netfn_capwap_tun *nct, struct net_device *dev);

#endif /* __NETFN_CAPWAP_DEC_H */
