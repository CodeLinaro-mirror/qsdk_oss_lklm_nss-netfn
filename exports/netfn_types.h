/*
 **************************************************************************
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 **************************************************************************
 */

/**
 * @file netfn_types.h
 *	Netfn definitions.
 */

#include <linux/types.h>

#ifndef __NETFN_TYPES_H
#define __NETFN_TYPES_H

/**
 * netfn_tuple_type
 *      Tuple type.
 */
typedef enum netfn_tuple_type {
	NETFN_TUPLE_3TUPLE,		/**< 3-tuple is valid */
	NETFN_TUPLE_4TUPLE,		/**< 4-tuple is valid */
	NETFN_TUPLE_5TUPLE,		/**< 5-tuple is valid */
} netfn_tuple_type_t;

/**
 * netfn_l4_ident_type
 *      four tuple valid field
 */
typedef enum netfn_l4_ident_type {
	NETFN_4TUPLE_VALID_SRC_PORT = 1,	/**< Src Port is valid in 4tuple */
	NETFN_4TUPLE_VALID_DST_PORT,		/**< Dest Port is valid in 4tuple */
} netfn_l4_ident_type_t;

typedef struct netfn_tuple_3tuple {
	uint32_t src_ip[4];		/**< Source IP address */
	uint32_t dest_ip[4];		/**< Destination IP address */
	uint8_t protocol;		/**< L4 protocol, e.g., TCP,UDP or UDP Lite */
} netfn_tuple_type_3tuple_t;

typedef struct netfn_tuple_4tuple {
	netfn_l4_ident_type_t ident_type;	/**< valid ident in 4 tuple */
	uint32_t src_ip[4];			/**< Source IP address */
	uint32_t dest_ip[4];			/**< Destination IP address */
	uint16_t l4_ident;			/**< Valid port - src port or dest port */
	uint8_t protocol;			/**< L4 protocol, e.g., TCP,UDP or UDP Lite */
} netfn_tuple_type_4tuple_t;

typedef struct netfn_tuple_5tuple {
	uint32_t src_ip[4];		/**< Source IP address */
	uint32_t dest_ip[4];		/**< Destination IP address */
	uint16_t l4_src_ident;		/**< Source L4 port, e.g., TCP or UDP port */
	uint16_t l4_dest_ident;		/**< Destination L4 port, e.g., TCP or UDP port */
	uint8_t protocol;		/**< L4 protocol, e.g., TCP,UDP or UDP Lite */
} netfn_tuple_type_5tuple_t;

/**
 * netfn_tuple
 *      Flow tuple info for IPv4 and IPv6.
 */
typedef struct netfn_tuple {
	uint8_t ip_version;			/**< IPv4 or IPv6 */
	netfn_tuple_type_t tuple_type;		/**< Valid tuple type */
	union {
		struct netfn_tuple_3tuple tuple_3;
		struct netfn_tuple_4tuple tuple_4;
		struct netfn_tuple_5tuple tuple_5;
	} tuples;
} netfn_tuple_t;

#endif /* __NETFN_TYPES_H */
