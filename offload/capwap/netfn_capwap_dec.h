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
 * netfn_capwap_dec_stats_read()
 *	Reads decap stats.
 */
void netfn_capwap_dec_stats_read(struct netfn_capwap_tun_dec *dec, struct netfn_capwap_tun_stats *stats);

/*
 * netfn_capwap_dec_init()
 *	Initializes the decap object.
 */
bool netfn_capwap_dec_init(struct netfn_capwap_tun_dec *dec, struct netfn_capwap_dec_cfg *cfg);

/*
 * netfn_capwap_dec_deinit()
 *	Deinitializes the decap object.
 */
void netfn_capwap_dec_deinit(struct netfn_capwap_tun_dec *dec);

#endif /* __NETFN_CAPWAP_DEC_H */
