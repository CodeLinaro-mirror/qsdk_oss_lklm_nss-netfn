/*
 **************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/**
 * @file netfn_flowmgr.h
 *	Netfn flow manager definitions.
 */

#include <linux/etherdevice.h>
#include <netfn_types.h>

#ifndef __NETFN_FLOWMGR_H
#define __NETFN_FLOWMGR_H

/* External typedef for return type */
typedef uint16_t netfn_flowmgr_ret_t;

/* Define a macro to get  netfn flowmgr specific error */
#define NETFN_FLOWMGR_GET_NETFN_ERROR_CODE(error_code) \
		(((error_code) & 0xFF))

/* Define a macro to get  netfn flowmgr specific error */
#define NETFN_FLOWMGR_GET_AE_ERROR_CODE(error_code) \
		(((error_code) >> 8) & 0xFF)

/* Flow flags */
#define NETFN_FLOWMGR_FLOW_FLAG_BRIDGE_FLOW				0x0001  /**< Bridge flow */
#define NETFN_FLOWMGR_FLOW_FLAG_DS_FLOW					0x0002  /**< Flow type flag for DS flow */
#define NETFN_FLOWMGR_FLOW_FLAG_VP_FLOW					0x0004  /**< Flow type flag for VP flow */
#define NETFN_FLOWMGR_FLOW_FLAG_SRC_MAC					0x0008  /**< Source MAC is valid for MAC rule */

/*
 * Rule valid flags
 */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_VLAN				0x0001  /**< VLAN is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_PPPOE				0x0002  /**< PPPoE is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_DSCP_MARKING			0x0004  /**< DSCP is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_PRIORITY				0x0008  /**< Priority is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_TRUSTSEC				0x0010  /**< Trustsec is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_QOS				0x0020  /**< QoS is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_SAWF				0x0040  /**< SAWF is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_QDISC				0x0080  /**< Qdisc is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_VLAN_FILTER			0x0100  /**< Vlan filter is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_UDP_LITE				0x0400  /**< UDP lite is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT				0x0800  /**< Src NAT rule is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT				0x1000  /**< Dst NAT rule is valid */
#define NETFN_FLOWMGR_VALID_RULE_FLAG_MAC				0x2000  /**< MAC rule is valid */

/**
 * enum netfn_flowmgr_ret_status
 *	netfn flow manager return status
 */
typedef enum netfn_flowmgr_ret_status {
	NETFN_FLOWMGR_RET_SUCCESS = 0,				/**< Success */
	NETFN_FLOWMGR_RET_CREATE_RULE_FAILED,			/**< Create rule failure */
	NETFN_FLOWMGR_RET_DESTROY_RULE_FAILED,			/**< Destroy rule failure */
	NETFN_FLOWMGR_RET_NO_MEM,				/**< Failure due to AE flow tables full */
	NETFN_FLOWMGR_RET_FEATURE_NOT_SUPPORTED,		/**< Failure due to unsupported feature */
	NETFN_FLOWMGR_RET_FAIL_INVALID_PARAM,			/**< Failure due to invalid parameters */
	NETFN_FLOWMGR_RET_INVALID_RULE,				/**< Failure due to invalid rule */
	NETFN_FLOWMGR_RET_INVALID_ACCEL_MODE,			/**< Failure due to invalid accel mode */
	NETFN_FLOWMGR_RET_TUPLE_MISMATCH,			/**< Failure due to tuple mismatch between org and reply */
	NETFN_FLOWMGR_RET_UNSUPPORTED_TUPLE_TYPE,		/**< Failure due to unsupported tuple type */
	NETFN_FLOWMGR_RET_PPE_UNSUPPORTED_TUPLE_TYPE,		/**< Failure due to unsupported tuple type */
	NETFN_FLOWMGR_RET_IP_ADDR_MISMATCH,			/**< Failure due to IP address mismatch between org and reply */
	NETFN_FLOWMGR_RET_IP_VERSION_MISMATCH,			/**< Failure due to IP version mismatch between org and reply */
	NETFN_FLOWMGR_RET_INCORRECT_IP_VERSION,			/**< Failure due to incorrect IP version */
	NETFN_FLOWMGR_RET_PROTOCOL_MISMATCH,			/**< Failure due to protocol mismatch between org and reply */
	NETFN_FLOWMGR_RET_FLOW_FLAGS_MISMATCH,			/**< Failure due to flow flags mismatch between org and reply */
	NETFN_FLOWMGR_RET_PPE_PROTOCOL_MISMATCH,		/**< Failure due to protocol mismatch between org and reply */
	NETFN_FLOWMGR_RET_PORT_MISMATCH,			/**< Failure due to port mismatch between org and reply */
	NETFN_FLOWMGR_RET_PPE_PORT_MISMATCH,			/**< Failure due to port mismatch between org and reply */
	NETFN_FLOWMGR_RET_PPE_IP_ADDR_MISMATCH,			/**< Failure due to IP address mismatch between org and reply */
	NETFN_FLOWMGR_RET_PPE_TUPLE_MISMATCH,			/**< Failure due to tuple mismatch between org and reply */
	NETFN_FLOWMGR_RET_PPE_INVALID_RULE,			/**< Failure due to invalid rule */
	NETFN_FLOWMGR_RET_PPE_INVALID_INTERFACE_INDEX,		/**< Failure due to invalid interface index */
	NETFN_FLOWMGR_RET_PPE_SIMUL_SNAT_DNAT,			/**< Failure due to simultaneous SNAT and DNAT */
	NETFN_FLOWMGR_RET_PPE_INVALID_SNAT_IP_CONFIGURATION,	/**< Failure due to incorrect SNAT information */
	NETFN_FLOWMGR_RET_PPE_INVALID_SNAT_PORT_CONFIGURATION,	/**< Failure due to incorrect SNAT information */
	NETFN_FLOWMGR_RET_PPE_INVALID_DNAT_IP_CONFIGURATION,	/**< Failure due to incorrect SNAT information */
	NETFN_FLOWMGR_RET_PPE_INVALID_DNAT_PORT_CONFIGURATION,	/**< Failure due to incorrect SNAT information */
	NETFN_FLOWMGR_RET_PPE_INNER_VLAN_MISMATCH,		/**< Failure due to PPE inner VLAN mismatch in org and reply */
	NETFN_FLOWMGR_RET_PPE_OUTER_VLAN_MISMATCH,		/**< Failure due to PPE outer VLAN mismatch in org and reply */
	NETFN_FLOWMGR_RET_PPE_NAT_WITH_BRIDGE_UNSUPPORTED,	/**< Failure due to NAT with bridge */
	NETFN_FLOWMGR_RET_PPE_INVALID_DEV_IN_FLOW_RULE,		/**< Failure due to invalid netdevs */
	NETFN_FLOWMGR_RET_PPE_INVALID_TOP_DEV_IN_FLOW_RULE,	/**< Failure due to invalid top netdevs */
	NETFN_FLOWMGR_RET_INVALID_DSCP_RULE,			/**< Failure due to invalid dscp rule */
	NETFN_FLOWMGR_RET_DSCP_RULE_ADD_FAILED,			/**< Failure due to dscp rule add failed */
	NETFN_FLOWMGR_RET_DSCP_RULE_DEL_FAILED,			/**< Failure due to dscp rule del failed */
	NETFN_FLOWMGR_RET_OPS_NOT_REGISTERED,			/**< Failure due to ops not registered */
} netfn_flowmgr_ret_status_t;

/**
 * netfn_flowmgr_stats_sync_reason
 *      Stats sync reasons.
 */
typedef enum netfn_flowmgr_stats_sync_reason {
	NETFN_FLOWMGR_STATS_SYNC_REASON_STATS,		/* Sync to synchronize stats */
	NETFN_FLOWMGR_STATS_SYNC_REASON_FLUSH,		/* Sync to flush a connection entry */
	NETFN_FLOWMGR_STATS_SYNC_REASON_EVICT,		/* Sync to evict a connection entry */
	NETFN_FLOWMGR_STATS_SYNC_REASON_DESTROY,	/* Sync to destroy a connection entry */
} netfn_flowmgr_stats_sync_reason_t;

/**
 * netfn_flowmgr_accel_mode
 *	Netfn flow manager accel modes.
 */
typedef enum netfn_flowmgr_accel_mode {
	NETFN_FLOWMGR_ACCEL_MODE_SFE = 1,		/**< SFE Mode */
	NETFN_FLOWMGR_ACCEL_MODE_PPE,			/**< PPE Mode */
} netfn_flowmgr_accel_mode_t;

/**
 * netfn_flowmgr_ae_type
 *	Netfn flow manager ae types.
 */
typedef enum netfn_flowmgr_ae_type {
	NETFN_FLOWMGR_AE_TYPE_SFE = 1,			/**< SFE AE type */
	NETFN_FLOWMGR_AE_TYPE_PPE,			/**< PPE AE type */
	NETFN_FLOWMGR_AE_TYPE_MAX,			/**< MAX */
} netfn_flowmgr_ae_type_t;

/**
 * netfn_flowmgr_stats
 *	Statistics.
 */
typedef struct netfn_flowmgr_stats {
	struct netfn_tuple tuple;			/**< Holds values of the tuple */
	uint32_t rx_pkt_count;				/**< RX packet count */
	uint32_t rx_byte_count;				/**< RX byte count */
	uint32_t tx_pkt_count;				/**< TX packet count */
	uint32_t tx_byte_count;				/**< TX byte count */
	netfn_flowmgr_accel_mode_t mode;		/**< AE */
	netfn_flowmgr_stats_sync_reason_t reason;	/**< Reason for the sync */
} netfn_flowmgr_stats_t;

/**
 * netfn_flowmgr_mac_rule
 *	Information for MAC rule.
 */
struct netfn_flowmgr_mac_xlate_rule {
	uint8_t src_mac[ETH_ALEN];			/**< Source MAC address after forwarding; optional */
	uint8_t dest_mac[ETH_ALEN];			/**< Destination MAC address after forwarding */
};

/**
 * netfn_flowmgr_nat_rule
 *	Information for NAT rule.
 */
struct netfn_flowmgr_ip_xlate_rule {
	uint32_t ip_version;				/**< IPv4 or IPv6 */
	uint32_t src_ip_xlate[4];			/**< Source translated IP address */
	uint32_t dest_ip_xlate[4];			/**< Destination translated IP address */
	uint16_t src_port_xlate;			/**< Source translated port number */
	uint16_t dest_port_xlate;			/**< Destination translated port number */
};

/*
 * netfn_flowmgr_sawf_rule
 *	Information for SAWF rule.
 */
struct netfn_flowmgr_sawf_rule {
	uint32_t mark;					/**< SAWF metadata information */
	uint8_t svc_id;					/**< Service class id */
};

/**
 * netfn_flowmgr_udp_lite_rule
 *	UDP lite parameters.
 */
struct netfn_flowmgr_udp_lite_rule {
	uint16_t csum_cov;		/**< Checksum coverage */
};

/**
 * netfn_flowmgr_qos_rule
 *	Qos rule information.
 */
struct netfn_flowmgr_qos_rule {
	uint32_t qos_tag;			/**< QoS tag associated with this rule, for PPE only 8-bits are valid */
	uint32_t priority;			/**< SKB priority(32-bit) or PPE priority(4-bit) */
        struct net_device *dev;			/**< Netdevice on which qos rule has to be applied */
};

/**
 * netfn_flowmgr_mark_rule
 *	Mark rule information.
 */
struct netfn_flowmgr_mark_rule {
	uint32_t mark;				/**< Service class information */
};

/**
 * netfn_flowmgr_vlan_rule
 *      Information for ingress and egress VLANs.
 */
struct netfn_flowmgr_vlan_info {
	uint32_t ingress_vlan_tag;			/**< Ingress VLAN tag */
	uint32_t egress_vlan_tag;			/**< Egress VLAN tag */
};

/**
 * netfn_flowmgr_vlan_rule
 *      Information for VLAN connection rules.
 */
struct netfn_flowmgr_vlan_rule {
        struct netfn_flowmgr_vlan_info inner;		/* Inner VLAN info */
        struct netfn_flowmgr_vlan_info outer;		/* Outer VLAN info */
};

/**
 * netfn_flowmgr_br_vlan_filter_rule
 *	Information related to VLAN filtering rules.
 */
struct netfn_flowmgr_br_vlan_filter_rule {
	struct netfn_flowmgr_vlan_info vlan_info;	/**< VLAN tag */
	uint8_t flags;					/**< VLAN flags */
	uint16_t vlan_tpid;				/**< VLAN tag protocol id */
};

/**
 * netfn_flowmgr_dscp_rule
 *      Information for DSCP connection rules.
 */
struct netfn_flowmgr_dscp_rule {
	uint8_t dscp_val;		/**< DSCP value */
};

/**
 * netfn_flowmgr_pppoe_rule
 *      Information for PPPoE connection rules.
 */
struct netfn_flowmgr_pppoe_rule {
	uint16_t session_id;			/**< Session id */
	uint8_t server_mac[ETH_ALEN];		/**< Server MAC address */
};

/**
 * netfn_flowmgr_qdisc_rule
 *      Information for QDISC connection rules.
 */
struct netfn_flowmgr_qdisc_rule {
	uint32_t valid_flags;			/**< Qdisc interface validity flags */
	struct net_device *qdisc_dev;		/**< Netdevice on which qdisc is applied */
};

/**
 * netfn_flowmgr_dscp_priority
 *	Netfn flowmgr dscp priority info
 */
struct netfn_flowmgr_dscp_priority {
	uint8_t dscp_val;			/**< DSCP value */
	uint8_t priority;			/**< Priority */
        struct net_device *src_dev;		/**< Source dev */
	uint8_t core_id;			/**< Core to which the packet has to be redirected */
	/*
	 * Response
	 */
	int16_t rule_id;			/**< Rule ID */
};

/**
 * netfn_flowmgr_rule_info
 *	Information for rules valid in a connection.
 */
struct netfn_flowmgr_rule_info {
	uint32_t rule_valid_flags;					/**< Bit flags associated with the validity of rule parameters */
	struct netfn_flowmgr_ip_xlate_rule ip_xlate_rule;		/**< Holds information about the NAT rule */
	struct netfn_flowmgr_mac_xlate_rule mac_xlate_rule;		/**< Holds information about the MAC rule */
	struct netfn_flowmgr_vlan_rule vlan_rule;			/**< VLAN-related acceleration parameters */
	struct netfn_flowmgr_dscp_rule dscp_rule;			/**< DSCP-related acceleration parameters */
	struct netfn_flowmgr_qos_rule qos_rule;				/**< QoS-related acceleration parameters */
	struct netfn_flowmgr_pppoe_rule pppoe_rule;			/**< PPPoE-related acceleration parameters */
	struct netfn_flowmgr_br_vlan_filter_rule vlan_filter_rule;	/**< VLAN filter related acceleration parameters */
	struct netfn_flowmgr_udp_lite_rule udp_lite_rule;		/**< UDP lite rule parameters */
	struct netfn_flowmgr_sawf_rule sawf_rule;			/**< SAWF rule related parameters */
	struct netfn_flowmgr_qdisc_rule qdisc_rule;			/**< QDISCrelated rule parameters */
};

/**
 * netfn_flowmgr_flow_info
 *	Contains info about flow's connection specific data.
 */
struct netfn_flowmgr_flow_info {
	struct net_device *in_dev;			/**< Ingress netdev */
	struct net_device *out_dev;			/**< Egress netdev */
	uint8_t flow_src_mac[ETH_ALEN];			/**< Actual Source MAC address */
	uint8_t flow_dest_mac[ETH_ALEN];		/**< Actual Dest MAC address */
	struct net_device *top_indev;			/**< Top ingress netdev */
	struct net_device *top_outdev;			/**< Top egress netdev */
	uint32_t flow_mtu;				/**< Flow MTU */
};

/**
 * netfn_flowmgr_destroy_rule
 *	Netfn flow rule destroy structure for both IPv4 and IPv6.
 */
struct netfn_flowmgr_destroy_rule {
	struct netfn_tuple tuple;			/**< Holds values of the tuple inside a flow rule */
	uint32_t reserved[4];				/**< Reserved */
};

/**
 * netfn_flowmgr_create_rule
 *	Netfn flow rule create structure for both IPv4 and IPv6.
 */
struct netfn_flowmgr_create_rule {
	uint32_t flow_flags;					/**< Bit flags associated with the flow */
	struct netfn_tuple tuple;				/**< Holds values of the tuple inside a flow rule */
	struct netfn_flowmgr_flow_info flow_info;		/**< Flow connection specific information */
	struct netfn_flowmgr_rule_info rule_info;		/**< Flow rule specific information */
	uint32_t reserved[4];					/**< Reserved */
};

/**
 * netfn_flowmgr_dscp_pri_add
 *	Netfn flowmgr add dscp priority.
 *
 * @param[in] dscpinfo dscp related info.
 * @param[in] mode accel mode.
 *
 * @return
 * netfn_flowmgr_ret_t.
 */
netfn_flowmgr_ret_t netfn_flowmgr_dscp_pri_add(struct netfn_flowmgr_dscp_priority *dscpinfo, netfn_flowmgr_accel_mode_t mode);

/**
 * netfn_flowmgr_dscp_pri_del
 *	Netfn flowmgr del dscp priority.
 *
 * @param[in] rule_id dscp rule id.
 * @param[in] mode accel mode.
 *
 * @return
 * netfn_flowmgr_ret_t.
 */
netfn_flowmgr_ret_t netfn_flowmgr_dscp_pri_del(int16_t rule_id, netfn_flowmgr_accel_mode_t mode);

/**
 * netfn_flowmgr_stats_callback_t
 *	Stats registration callback.
 *
 * @param[in] app_data.
 * @param[in] stats.
 *
 * @return
 *
 */
typedef void (*netfn_flowmgr_stats_callback_t)(void *app_data, netfn_flowmgr_stats_t *stats);

/**
 * netfn_flowmgr_stats_callback_register
 *	Register the callbacks with flowmgr for stats.
 *
 * @param[in] app_data.
 * @param[in] stats_cb user callback.
 *
 * @return
 * netfn_flowmgr_ret_t
 */
extern netfn_flowmgr_ret_t netfn_flowmgr_stats_callback_register(void *app_data, netfn_flowmgr_stats_callback_t stats_cb);

/**
 * netfn_flowmgr_rule_decel
 *	API for flow rule deacceleration.
 *
 * @param[in] original flow direction rule.
 * @param[in] reply return direction rule.
 * @param[in] mode accel_mode.
 *
 * @return
 * netfn_flowmgr_ret_t
 */
extern netfn_flowmgr_ret_t netfn_flowmgr_rule_decel(struct netfn_flowmgr_destroy_rule *original, struct netfn_flowmgr_destroy_rule *reply, netfn_flowmgr_accel_mode_t mode);

/**
 * netfn_flowmgr_rule_accel
 *	API for flow rule acceleration.
 *
 * @param[in] original flow direction rule.
 * @param[in] reply return direction rule.
 * @param[in] mode accel_mode.
 *
 * @return
 * netfn_flowmgr_ret_t
 */
extern netfn_flowmgr_ret_t netfn_flowmgr_rule_accel(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply, netfn_flowmgr_accel_mode_t mode);

#endif /* __NETFN_FLOWMGR_H */
