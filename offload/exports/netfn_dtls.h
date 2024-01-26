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

#ifndef __NETFN_DTLS_H
#define __NETFN_DTLS_H

#include <netfn_types.h>
#include <linux/netdevice.h>

/*
 * netfn_dtls_crypto
 *	DTLS Base meta information for session allocation.
 */
struct netfn_dtls_crypto {
	struct {
		uint8_t *key_data;			/* Pointer to the Key array */
		uint16_t key_len;			/* Length of the key */
	} cipher, auth;

	uint32_t nonce;				/* nonce value for certain cipher algorithm */
	const int8_t *algo_name;		/* AEAD algorithm name for cipher operation */
};

/*
 * netfn_dtls_cfg
 *	DTLS meta information for session allocation.
 */
struct netfn_dtls_cfg {
	struct netfn_dtls_crypto base;		/* Base meta information */
	uint32_t replay_win;			/* replay window */
	uint32_t flags;				/* context flags */
#define NETFN_DTLS_FLAG_ENC      BIT(0)	/* set = Encapsulation, clear = Decapsulation */
#define NETFN_DTLS_FLAG_IPV6     BIT(1)	/* set = IPv6 tuple, clear = IPv4 tuple */
#define NETFN_DTLS_FLAG_UDPLITE  BIT(2)	/* set = UDPlite, clear = UDP encapsulation */
#define NETFN_DTLS_FLAG_CAPWAP   BIT(3)	/* set = Capwap hdr enable, clear = capwap hdr disable */
#define NETFN_DTLS_FLAG_CP_TOS  BIT(4)	/* set = copy IP TOS value from SKB, clear = use default TOS */
#define NETFN_DTLS_FLAG_CP_DF   BIT(5)	/* set = copy IP DF value from SKB, clear = use default DF */

	uint16_t version;			/* DTLS version (0xFEFF/v1.0 or 0xFEFD/v1.2) */
	uint16_t epoch;				/* Unique epoch value */
	uint8_t df;				/* Default DF value for outer IP */
	uint8_t dscp;				/* Default dscp value for outer IP */
	uint8_t hop_limit;			/* Default TTL limit for outer IP */
};

/*
 * A tunnel compose of two types of Session(s); one for encapsulation & one for decapsulation.
 * Each tunnel is represented using dev under which Session has to be inserted.
 * DEV --> ENC_SESSION(active, passive) & DEC_SESSION(active, active)
 */

/*
 * netfn_dtls_tun_alloc
 *	Create new DTLS device to handle specific sessions.
 *
 * @param[in] pvt_sz	Private memory size for caller use.
 *
 * @return
 * net_device
 */
struct net_device *netfn_dtls_tun_alloc(ssize_t pvt_sz);

/*
 * netfn_dtls_tun_free
 *	Destroy the DTLS device.
 *
 * @param[in] net_device
 */
void netfn_dtls_tun_free(struct net_device *dev);

/*
 * netfn_dtls_session_add
 *	Insert new session under the DTLS device.
 *
 * @datatypes
 * netfn_dtls_cfg
 * netfn_tuple_t
 *
 * @param[in] dev	DTLS device under which session will be added.
 * @param[in] data	Session parameters.
 * @param[in] t		session outer tuple (identifier).
 *
 * @return
 * Zero when successful, otherwise error code
 */
int netfn_dtls_tun_session_add(struct net_device *dev, struct netfn_dtls_cfg *cfg, netfn_tuple_t *t);

/*
 * netfn_dtls_tun_session_encap_switch
 *	Switch encapsulation session to Recent.
 *
 * @param[in] dev	DTLS device under which encapsulation session will be switched
 */
void netfn_dtls_tun_session_encap_switch(struct net_device *dev);

/*
 * netfn_dtls_session_del
 *	Delete the session from device.
 *
 * @datatypes
 * netfn_dtls_tuple
 *
 * @param[in] dev	DTLS device under which session to be deleted.
 * @param[in] encap	True for Encapsulation session otherwise False.
 * @param[in] epoch	Epoch value used during session addition.
 */
void netfn_dtls_tun_session_del(struct net_device *dev, bool encap, __be16 epoch);

#endif /* __NETFN_DTLS_H */
