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

#include "netfn_pkt_steer_priv.h"

/*
 * netfn_pkt_steer_alloc()
 *	Allocate Packet steering object.
 */
netfn_pkt_steer_t netfn_pkt_steer_alloc(struct netfn_pkt_steer_info *info)
{
}

/*
 * netfn_pkt_steer_free()
 *	Free packet steering object
 */
void netfn_pkt_steer_free(netfn_pkt_steer_t ps)
{
}

/*
 * netfn_pkt_steer_send()
 *	Send packet to remote CPU.
 */
int netfn_pkt_steer_send(netfn_pkt_steer_t ps, struct sk_buff_head *head, int cpu, uint64_t *tx_bytes)
{
}

/*
 * netfn_pkt_steer_init_module()
 *	Module initialization
 */
int __init netfn_pkt_steer_init_module(void)
{
	pr_info("NETFN packet steer module loaded %s\n", NSS_NETFN_BUILD_ID);
}

/*
 * netfn_pkt_steer_exit_module()
 *	Module exit cleanup
 */
void __exit netfn_pkt_steer_exit_module(void)
{
	pr_info("NETFN packet steer module unloaded %s\n", NSS_NETFN_BUILD_ID);
}
