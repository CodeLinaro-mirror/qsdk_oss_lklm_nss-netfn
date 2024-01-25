/*
 * netfn_capwap.h
 *	Network function's CAPWAP offload public APIs.
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
#ifndef __NETFN_CAPWAP_H
#define __NETFN_CAPWAP_H

#include <netfn_types.h>

#define NETFN_CAPWAP_SNAP_HDR_LEN 6

/*
 * netfn_capwap_enc_cfg
 *	Encapsulation configuration
 */
struct netfn_capwap_enc_cfg {
	netfn_tuple_t tuple;		/**< Tuples associated with tunnel. */
	uint32_t features;		/**< Features enabled for tunnel. */
	uint32_t flags;			/**< Vlan/pppoe/udplite_checksum configured. */
	uint16_t csum_cov;		/**< UDP Checksum coverage. */
	uint16_t mtu;			/**< MTU to be configured for fragmentation. */
	uint8_t ttl;			/**< TTL configured for tunnel. */
	uint8_t tos;			/**< ToS for encapsulation. */
	uint8_t bssid[ETH_ALEN];	/**< Basic service set ID. */
	uint8_t snap_hdr[NETFN_CAPWAP_SNAP_HDR_LEN];
					/**< Custom snap header. */
};

/*
 * netfn_capwap_dec_cfg
 *	Decapsulation configuration
 */
struct netfn_capwap_dec_cfg {
	uint32_t features;		/**< Features enabled for tunnel. */
	uint32_t max_frags;		/**< Maximum number of fragments expected. */
	uint16_t max_payload_sz;	/**< Maximum size of the payload buffer. */
};

/*
 * netfn_capwap_tun_cfg
 *	Tunnel configuration
 */
struct netfn_capwap_tun_cfg {
	struct netfn_capwap_enc_cfg enc;	/**< Encap configuration of tunnel. */
	struct netfn_capwap_dec_cfg dec;	/**< Decap configuration of tunnel. */
	uint16_t id;				/**< Tunnel identifier. */
	uint8_t capwap_ver;			/**< Capwap version. */
	uint8_t features;			/**< Features enabled for tunnel. */
};

/*
 * netfn_capwap_dec_stats
 *	Capwap decap stats.
 */
struct netfn_capwap_dec_stats {
	uint64_t num_frags;			/**< Number of fragments received. */
	uint64_t control_pkts;			/**< Control packets. */
	uint64_t keepalive_pkts;		/**< Keep alive packets recieved. */
	uint64_t fast_reasm;			/**< Fast reassembly hits. */
	uint64_t slow_reasm;			/**< Slow reassembly hits. */
	/*
	 * Drop stats.
	 */
	uint64_t drop_dup_frags;		/**< Dropped duplicate fragments. */
	uint64_t drop_dec_failure;		/**< Failed during decapsulation. */
	uint64_t drop_max_frags;		/**< Exceeds max fragments allowed(10). */
	uint64_t drop_missing_frags;		/**< Drop in reassembly window because of missing fragments. */
	uint64_t drop_large_frags;		/**< Drop because of oversize. */
	uint64_t drop_csum_fail;		/**< Drop because of wrong checksum. */
	uint64_t drop_malformed;		/**< Drop because of malformed packet. */
	uint64_t drop_linearize_fail;		/**< Drop because of linearize failure. */
	uint64_t drop_reasm_window;		/**< Drop because of a non-sequential fragment offset. */
};

/*
 * netfn_capwap_enc_stats
 *	Capwap encap stats.
 */
struct netfn_capwap_enc_stats {
	uint64_t num_pkts;			/**< Number of packets received. */
	uint64_t num_frags;			/**< Number of segments or fragments generated. */
	uint64_t keepalive_pkts;		/**< Keep alive packets recieved. */
	/*
	 * Drop stats.
	 */
	uint64_t drop_queue_full;		/**< Packets dropped because the queue is full. */
	uint64_t drop_mem_alloc;		/**< Packets dropped because of a memory failure. */
	uint64_t drop_ver_mis;			/**< Packets dropped because of a version mismatch. */
	uint64_t drop_insufficient_hroom;	/**< Packets dropped because of insufficent headroom. */
	uint64_t drop_direct_dtls;		/**< Packets dropped because of a DTLS packet. */
	uint64_t drop_nwireless_len;		/**< Packets dropped because invalid nwireless len. */
};

/*
 * netfn_capwap_pkt_stats
 *	Tunnel packet stats.
 */
struct netfn_capwap_pkt_stats {
	uint64_t tx_pkts;		/**< Total tx packets. */
	uint64_t tx_bytes;		/**< Total tx bytes. */
	uint64_t tx_failures;		/**< Total tx failures. */
	uint64_t rx_pkts;		/**< Total rx packets. */
	uint64_t rx_bytes;		/**< Total rx bytes. */
	uint64_t rx_failures;		/**< Total rx failures. */
};

/*
 * netfn_capwap_tun_stats
 *	Tunnel statistics
 */
struct netfn_capwap_tun_stats {
	struct netfn_capwap_pkt_stats pkts;	/**< Tunnel packet stats. */
	struct netfn_capwap_dec_stats dec;	/**< Tunnel decap stats. */
	struct netfn_capwap_enc_stats enc;	/**< Tunnel encap stats. */
};

/*
 * netfn_capwap_tun_alloc()
 *	Adds a new tunnel.
 * @cfg: Tunnel configuration.
 * @return: Net device associated with tunnel.
 */
struct net_device *netfn_capwap_tun_alloc(struct netfn_capwap_tun_cfg *cfg, ssize_t pvt_sz);

/*
 * netfn_capwap_tun_free()
 *	Deletes the tunnel.
 * @dev: Tunnel netdevice.
 * @return: bool
 */
bool netfn_capwap_tun_free(struct net_device *dev);

/*
 * netfn_capwap_tun_bind()
 *	Binds the 2 netdevices.
 * @dev: dev to binded.
 * @next: dev to bind it to.
 * @return: bool
 */
bool netfn_capwap_tun_bind(struct net_device *dev, struct net_device *next);

/*
 * netfn_capwap_tun_unbind()
 *	Unbinds the tunnel.
 * @dev: dev to unbinded.
 * @return: bool
 */
bool netfn_capwap_tun_unbind(struct net_device *dev);

/*
 * netfn_capwap_tun_stats_get()
 *	Gets the tunnel stats.
 * @dev: Tunnel netdevice.
 * @stats: Tunnel stats object.
 * @return: int
 */
int netfn_capwap_tun_stats_get(struct net_device *dev, struct netfn_capwap_tun_stats *stats);

/*
 * netfn_capwap_tun_pvt_get()
 *	Returns pointer to pvt area
 * @dev: Netdevice.
 * @return: Pointer to private area.
 */
void *netfn_capwap_tun_pvt_get(struct net_device *dev);

/*
 * netfn_capwap_tunid_alloc()
 *	Allocates a new dummy dev.
 * @void:
 * @return: Net device.
 */
struct net_device *netfn_capwap_tunid_alloc(ssize_t pvt_sz);

/*
 * netfn_capwap_tunid_free()
 *	Frees the dummy dev.
 * @dev: Dev to be freed.
 * @return: bool.
 */
bool netfn_capwap_tunid_free(struct net_device *dev);

/*
 * netfn_capwap_tunid_add()
 *	Adds a new tunnel with a specific id.
 * @dev: Dummy netdev incorporating multiple IDs.
 * @id: ID for tunnel configuration.
 * @cfg: Tunnel configuration.
 * @return: bool.
 */
struct net_device *netfn_capwap_tunid_add(struct net_device *dev, uint8_t id, struct netfn_capwap_tun_cfg *cfg, ssize_t pvt_sz);

/*
 * netfn_capwap_tunid_del()
 *	Deletes the tunnel.
 * @dev: Dummy netdev incorporating multiple tun IDs.
 * @id: ID for tunnel configuration.
 * @return: bool
 */
bool netfn_capwap_tunid_del(struct net_device *dev, uint8_t id);

/*
 * netfn_capwap_tunid_bind()
 *	Binds the 2 netdevices.
 * @dev: Dummy netdev hosting multiple netdevs.
 * @id: ID for tunnel configuration.
 * @next: Next dev to be set for tunnel dev.
 * @return: bool
 */
bool netfn_capwap_tunid_bind(struct net_device *dev, uint8_t id, struct net_device *next);

/*
 * netfn_capwap_tunid_unbind()
 *	Unbinds the tunnel id dev.
 * @dev: Dummy netdev hosting multiple netdevs.
 * @id: ID for tunnel configuration.
 * @return: bool
 */
bool netfn_capwap_tunid_unbind(struct net_device *dev, uint8_t id);

/*
 * netfn_capwap_tunid_stats()
 *	Gets the tunnel stats.
 * @dev: Parent netdevice.
 * @id: ID for tunnel configuration.
 * @stats: Tunnel stats object.
 */
int netfn_capwap_tunid_stats_get(struct net_device *dev, uint8_t id, struct netfn_capwap_tun_stats *stats);

/*
 * netfn_capwap_tunid_pvt_get()
 *	Returns pointer to pvt area
 * @dev: Netdevice.
 * @return: Pointer to private area.
 */
void *netfn_capwap_tunid_pvt_get(struct net_device *dev);
#endif /* __NETFN_CAPWAP_H */
