/*
 *****************************************************************************
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 ****************************************************************************
 */

/*
 * netfn_flow_cookie_hash.c
 *     Hash Functionality File for Flow Cookie DB Module.
 */

#include <linux/in.h>
#include <linux/types.h>
#include "netfn_flow_cookie_hash.h"

uint32_t netfn_flow_cookie_hash_compute_v4(uint8_t protocol, uint32_t src_ip, uint16_t src_ident,
			uint32_t dest_ip, uint16_t dest_ident)
{
	uint32_t hash = 0;

	/*
	 * Following Hash Function is a generic one
	 * and used at various other modules.
	 * Moreover, detailed experimentation has also been performed
	 * and it has been certified that the performace results
	 * are well in acceptable range.
	 */

	hash ^= (ntohl(src_ip ^ dest_ip));

	hash ^= (protocol ^ (ntohs((src_ident) ^ (dest_ident))));

	/*
	 * We perform AND with the NETFN_FLOW_COOKIE_HASH_MASK for the obtained hash_idx
	 * to ensure that we never go out of the boundary chosen.
	 */
	hash = ((hash >> NETFN_FLOW_COOKIE_HASH_SHIFT) ^ hash) & NETFN_FLOW_COOKIE_HASH_MASK;

	return hash;
}

uint32_t netfn_flow_cookie_hash_compute_v6(uint8_t protocol, uint32_t src_ip[4], uint16_t src_ident,
			uint32_t dest_ip[4], uint16_t dest_ident)
{
	uint32_t hash = 0;
	uint8_t idx;

	/*
	 * Following Hash Function is a generic one
	 * and used at various other modules.
	 * Moreover, detailed experimentation has also been performed
	 * and it has been certified that the performace results
	 * are well in acceptable range.
	 */

	for (idx = 0; idx < 4; idx++) {
		hash ^= (ntohl(src_ip[idx] ^ dest_ip[idx]));
	}

	hash ^= (protocol ^ (ntohs((src_ident) ^ (dest_ident))));

	/*
	 * We perform AND with the NETFN_FLOW_COOKIE_HASH_MASK for the obtained hash_idx
	 * to ensure that we never go out of the boundary chosen.
	 */
	hash = ((hash >> NETFN_FLOW_COOKIE_HASH_SHIFT) ^ hash) & NETFN_FLOW_COOKIE_HASH_MASK;

	return hash;
}
