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
#include <linux/rculist.h>
#include <net/addrconf.h>
#include <crypto/aes.h>

#include "netfn_dtls_priv.h"

/*
 * netfn_dtls_session_add()
 *	Create a dtls sesison under netdevice.
 */
struct netfn_dtls_session *netfn_dtls_session_alloc(struct netfn_dtls_cfg *cfg, netfn_tuple_t *t, struct netfn_dtls_tun *tun)
{
	/*
	 * TODO: Allocate new session memory and Allocate transformation context for EIP driver.
	 */
	BUG_ON(1);
	return NULL;
}

/*
 * netfn_dtls_session_del()
 *	Delete dtls Session associated with the netdevice.
 */
void netfn_dtls_session_free(struct netfn_dtls_session *ses)
{
	/*
	 * TODO: Lookup session using tuple and Free.
	 */
	BUG_ON(1);
}
