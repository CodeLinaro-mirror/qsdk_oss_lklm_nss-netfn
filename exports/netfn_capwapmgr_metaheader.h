/*
 **************************************************************************
 * Copyright (c) 2015-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023,2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

/*
 * TODO: Update the file name since its used by both capwapmgr and offload.
 */

#ifndef __NETFN_CAPWAPMGR_METAHEADER_H
#define __NETFN_CAPWAPMGR_METAHEADER_H

#include <linux/types.h>

/*
 * CAPWAP version
 */
#define NSS_CAPWAP_VERSION_V1		0x1
				/**< RFC CAPWAP version. */
#define NSS_CAPWAP_VERSION_V2		0x2
				/**< Initial CAPWAP version for a customer. */

/*
 * Type of packet. These are mutually exclusive fields.
 */
#define NSS_CAPWAP_PKT_TYPE_UNKNOWN	0x0000
				/**< Don't know the type of CAPWAP packet. */
#define NSS_CAPWAP_PKT_TYPE_CONTROL	0x0001
				/** It's a control CAPWAP packet src_port=5247. */
#define NSS_CAPWAP_PKT_TYPE_DATA	0x0002
				/**< It's a data CAPWAP packet src_port=5246. */

/*
 * Addtional fields for identifying what's there in the packet.
 */
#define NSS_CAPWAP_PKT_TYPE_WIRELESS_INFO	0x0008
				/**< W=1, wireless info present. */
#define NSS_CAPWAP_PKT_TYPE_802_11		0x0010
				/**< T=1, then set wbid=1. */
#define NSS_CAPWAP_PKT_TYPE_802_3		0x0020
				/**< Data is in 802.3 format. */


#define NSS_CAPWAP_WINFO_SZ 8
				/**< Size of Wireless Info. */

/**
 * nss_capwap_metaheader
 *	CAPWAP metaheader per-packet for both encap (TX) and decap (RX).
 */
struct nss_capwap_metaheader {
	uint8_t version;	/**< CAPWAP version. */
	uint8_t rid;		/**< Radio ID. */
	uint16_t tunnel_id;	/**< Tunnel-ID. */
	uint8_t dscp;		/**< DSCP value. */
	uint8_t vlan_pcp;	/**< VLAN priority .P marking. */
	uint16_t type;		/**< Type of CAPWAP packet & What was there in CAPWAP header. */
	uint16_t nwireless;	/**< Number of wireless info sections in CAPWAP header. */
	uint16_t wireless_qos;	/**< 802.11e qos info. */
	uint16_t outer_sgt;	/**< Security Group Tag value in the TrustSec header. */
	uint16_t inner_sgt;	/**< Security Group Tag value in the TrustSec header. */
	uint32_t flow_id;	/**< Flow identification. */
	uint16_t vapid;		/**< VAP ID info. */

	uint16_t reserved;	/**< Reserved for backwards comparibility. */

	/*
	 * Put the wl_info at last so we don't have to do copy if 802.11 to 802.3 conversion did not happen.
	 */
	uint8_t wl_info[NSS_CAPWAP_WINFO_SZ];	/**< Wireless info preserved from the original packet. */
} __packed __aligned(4);

#endif /* __NSS_CAPWAPMGR_METAHEADER_H */
