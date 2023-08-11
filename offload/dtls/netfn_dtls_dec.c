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

#include "netfn_dtls_priv.h"

/*
 * netfn_dtls_dec_err()
 *	Decapsulation completion callback.
 */
void netfn_dtls_dec_err(void *app_data, eip_req_t req, int err)
{
	BUG_ON(1);
}

/*
 * netfn_dtls_dec_done()
 *	Decapsulation completion callback.
 */
void netfn_dtls_dec_done(void *app_data, eip_req_t req)
{
	/*
	 * TODO: Send packet to NETFN CAPWAP.
	 */

	BUG_ON(1);
}

/*
 * netfn_dtls_dec()
 *	Handle UDP/UDPlite encapsulated DTLS+CAPWAP packets.
 */
void netfn_dtls_dec(struct netfn_dtls_session *ses, struct sk_buff *skb)
{

	/*
	 * TODO: Lookup decapsulation session and send packet to EIP driver for decapsulation.
	 */
	BUG_ON(1);
}
