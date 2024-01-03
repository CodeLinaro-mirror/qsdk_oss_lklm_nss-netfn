/*
 * netfn_capwap_tun.h
 *	Network function's CAPWAP offload tunnel defintions.
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
#ifndef __NETFN_CAPWAP_TUN_H
#define __NETFN_CAPWAP_TUN_H

#include "netfn_pkt_steer.h"

#define NETFN_CAPWAP_TUN_RX_BUDGET 64
#define NETFN_CAPWAP_TUN_TX_BUDGET 64
#define NETFN_CAPWAP_TUN_STATS_INC(sptr) ((*(uint64_t *) sptr)++)
#define NETFN_CAPWAP_TUN_STATS_ADD(sptr, value) ({ \
		*((uint64_t *)sptr) += value; \
		})

/*
 * netfn_capwap_tun_enc_hdr
 *	Encapsulation header
 */
struct netfn_capwap_tun_enc_hdr {
	__be32 src_ip[4];		/* Source IP for tunnel. */
	__be32 dest_ip[4];		/* Destination IP for tunnel. */
	__be16 src_port;		/* Source port */
	__be16 dest_port;		/* Destination port */
	__u8 proto_next_hdr;		/* L4 proto. */
	__u8 ip_version;		/* IP version. */
	__u8 ttl;			/* Time to live. */
	__u8 tos;			/* Type of service. */
	__u8 csum_cov;			/* Checksum coverage. */
};

/*
 * netfn_capwap_tun_enc
 *	Encapsulation state
 */
struct netfn_capwap_tun_enc {
	struct net_device *dev;		/* Device pointer for tunnel. */
	struct netfn_capwap_enc_stats stats;	/* Encap tunnel stats. */

	struct netfn_capwap_tun_enc_hdr hdr;	/* Encapsulation header per tunnel. */
	uint32_t features;			/* Features enabled for the tunnel. */

	int mtu;			/* MTU considered for fragmentation. */
	uint16_t ipv4_id;		/* IPv4 ID for the tunnel. */
	uint16_t frag_id;		/* Fragment ID per ipv4_id. */

	uint16_t outer_hdr_len;		/* Outer header length for the tunnel. */
	uint8_t snap_hdr[NETFN_CAPWAP_SNAP_HDR_LEN];
					/* Custom snap header. */
	uint8_t bssid;			/* Basic service set ID. */
};

/*
 * netfn_capwap_tun_dec_frags
 *	Decapsulation frags array.
 */
struct netfn_capwap_tun_dec_frags {
	struct sk_buff_head frags;	/* Reassembly fragment queue. */
	int frag_id;			/* Frag-ID, associated with reasm queue; invalid value is -1 */
};

/*
 * netfn_capwap_tun_dec
 *	Decapsulation state
 */
struct netfn_capwap_tun_dec {
	struct sk_buff *fast_reasm;		/* Fast reassembly buffer. */
	struct netfn_capwap_tun_dec_frags *slow_reasm;
						/* Array of pending fragments. */
	struct net_device *dev;			/* Back pointer to device. */
	struct netfn_capwap_dec_stats stats;	/* Decap tunnel stats. */


	uint32_t max_frags;			/* Maximum number of frags expected. */
	uint32_t max_payload_sz;		/* Maximum size of payload buffer. */
	uint32_t reasm_window_sz;		/* Maximum fragments array size. */
	uint32_t features;			/* Features enabled for the tunnel. */
};

/*
 * netfn_capwap_tun
 * 	CAPWAP tunnel structure
 */
struct netfn_capwap_tun {
	struct netfn_capwap *nc;		/* Pointer to driver object. */
	struct net_device *dev;			/* Pointer to tunnel dev. */
	struct net_device *next_dev;		/* Pointer to nexthop dev. */
	struct net_device *vp_dev;		/* Pointer to VP dev. */

	struct netfn_capwap_pkt_stats stats;	/* Tunnel stats. */
	struct netfn_capwap_tun_enc enc;	/* Tunnel encap object. */
	struct netfn_capwap_tun_dec dec;	/* Tunnel decap object. */
	struct dentry *dentry;			/* Debug entry for the tunnel. */

	netfn_pkt_steer_t tx_steer;		/* Tx packet steering handle. */
	netfn_pkt_steer_t rx_steer;		/* Rx Packet steering handle. */
	uint32_t flags;				/* Flags for this tunnel. */
	uint8_t pvt[] __attribute__((aligned (sizeof(uint32_t))));
						/* User private area pointer. */
};

#endif /* __NETFN_CAPWAP_TUN_H */
