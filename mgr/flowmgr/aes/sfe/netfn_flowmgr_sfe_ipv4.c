/*
 **************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_flowmgr_sfe_ipv4.c
 *	Netfn flow manager sfe ipv4 file
 */

#include <linux/types.h>
#include <linux/etherdevice.h>
#include <sfe_api.h>
#include <netfn_flowmgr.h>
#include <flowmgr/netfn_flowmgr_priv.h>
#include "netfn_flowmgr_sfe.h"
#include "netfn_flowmgr_sfe_ipv4.h"

/*
 * netfn_flowmgr_sfe_v4_get_stats()
 *	Get SFE IPv4 stats for a single connection
 */
netfn_flowmgr_ret_t netfn_flowmgr_sfe_v4_get_stats(struct netfn_flowmgr_flow_conn_stats *stats)
{
	struct sfe_ipv4_single_conn_stats sfe_stats;
	netfn_tuple_type_t tuple_type;
	bool sfe_status;

	tuple_type = stats->tuple.tuple_type;

	/*
	 * Check tuple type
	 */
	if (!(tuple_type == NETFN_TUPLE_3TUPLE) && !(tuple_type == NETFN_TUPLE_4TUPLE) && !(tuple_type == NETFN_TUPLE_5TUPLE)) {
		netfn_flowmgr_warn("Unsupported tupple type in SFE acceleration mode, tuple_type = %d\n", tuple_type);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_UNSUPPORTED_TUPLE_TYPE, 0);
	}

	sfe_stats.tuple.flow_ident = 0;
	sfe_stats.tuple.return_ident = 0;
	if (tuple_type == NETFN_TUPLE_3TUPLE) {
		sfe_stats.tuple.protocol = stats->tuple.tuples.tuple_3.protocol;
		sfe_stats.tuple.flow_ip = stats->tuple.tuples.tuple_3.src_ip[0];
		sfe_stats.tuple.return_ip = stats->tuple.tuples.tuple_3.dest_ip[0];
	} else if (tuple_type == NETFN_TUPLE_4TUPLE) {
		sfe_stats.tuple.protocol = stats->tuple.tuples.tuple_4.protocol;
		sfe_stats.tuple.flow_ip = stats->tuple.tuples.tuple_4.src_ip[0];
		sfe_stats.tuple.return_ip = stats->tuple.tuples.tuple_4.dest_ip[0];
		if (stats->tuple.tuples.tuple_4.ident_type == NETFN_4TUPLE_VALID_SRC_PORT) {
			sfe_stats.tuple.flow_ident = stats->tuple.tuples.tuple_4.l4_ident;
		} else {
			sfe_stats.tuple.return_ident = stats->tuple.tuples.tuple_4.l4_ident;
		}
	} else if (tuple_type == NETFN_TUPLE_5TUPLE) {
                sfe_stats.tuple.protocol = stats->tuple.tuples.tuple_5.protocol;
                sfe_stats.tuple.flow_ip = stats->tuple.tuples.tuple_5.src_ip[0];
                sfe_stats.tuple.return_ip = stats->tuple.tuples.tuple_5.dest_ip[0];
                sfe_stats.tuple.flow_ident = stats->tuple.tuples.tuple_5.l4_src_ident;
                sfe_stats.tuple.return_ident = stats->tuple.tuples.tuple_5.l4_dest_ident;
        }
	sfe_stats.org_dev = stats->org_netdev;
	sfe_stats.reply_dev = stats->reply_netdev;

	sfe_status = sfe_ipv4_get_single_conn_stats(&sfe_stats);
	if (!sfe_status) {
		netfn_flowmgr_warn("SFE v4 get sfe_stats failed for below tuples:\n"
					"protocol = %u\n"
					"flow_ip = %pI4\n"
					"return_ip = %pI4\n"
					"flow_ident = %u\n"
					"return_ident = %u\n",
					sfe_stats.tuple.protocol,
					&sfe_stats.tuple.flow_ip,
					&sfe_stats.tuple.return_ip,
					sfe_stats.tuple.flow_ident,
					sfe_stats.tuple.return_ident);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_GET_SFE_STATS_FAILED, sfe_status);
	}

	/*
	 * Copy sfe stats to the conn stats structure to update the user stats.
	 */
	stats->conn_stats.ip_version = ETH_P_IP;
	stats->conn_stats.protocol = sfe_stats.tuple.protocol;

	/*
	 * original direction stats
	 */
	stats->conn_stats.org_src_ip[0] = sfe_stats.tuple.flow_ip;
	stats->conn_stats.org_dest_ip[0] = sfe_stats.tuple.return_ip;
	stats->conn_stats.org_src_ident = sfe_stats.tuple.flow_ident;
	stats->conn_stats.org_dest_ident = sfe_stats.tuple.return_ident;
	stats->conn_stats.org_tx_pkt_count = sfe_stats.tx_packet_count;
	stats->conn_stats.org_rx_pkt_count = sfe_stats.rx_packet_count;
	stats->conn_stats.org_tx_byte_count = sfe_stats.tx_byte_count;
	stats->conn_stats.org_rx_byte_count = sfe_stats.rx_byte_count;

	/*
	 * reply direction stats
	 */
	stats->conn_stats.reply_src_ip[0] = sfe_stats.tuple.return_ip;
	stats->conn_stats.reply_dest_ip[0] = sfe_stats.tuple.flow_ip;
	stats->conn_stats.reply_src_ident = sfe_stats.tuple.return_ident;
	stats->conn_stats.reply_dest_ident = sfe_stats.tuple.flow_ident;
	stats->conn_stats.reply_tx_pkt_count = sfe_stats.rx_packet_count;
	stats->conn_stats.reply_rx_pkt_count = sfe_stats.tx_packet_count;
	stats->conn_stats.reply_tx_byte_count = sfe_stats.rx_byte_count;
	stats->conn_stats.reply_rx_byte_count = sfe_stats.tx_byte_count;

	return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_SUCCESS, sfe_status);
}

/*
 * netfn_flowmgr_sfe_validate_v4_destroy_tuple_info()
 *	Validate tuple info in original and reply direction
 */
static netfn_flowmgr_ret_t netfn_flowmgr_sfe_validate_v4_destroy_tuple_info(struct netfn_flowmgr_destroy_rule *original, struct netfn_flowmgr_destroy_rule *reply)
{
	uint32_t org_src_ip;
	uint32_t org_dst_ip;
	uint32_t reply_src_ip;
	uint32_t reply_dst_ip;
	uint16_t org_ident;
	uint16_t reply_ident;
	uint16_t org_src_ident;
	uint16_t org_dst_ident;
	uint16_t reply_src_ident;
	uint16_t reply_dst_ident;
	netfn_tuple_type_t tuple_type;
	netfn_flowmgr_ret_t status = NETFN_FLOWMGR_RET_SUCCESS;

	netfn_flowmgr_trace("org tuple type = %d\n", original->tuple.tuple_type);
	netfn_flowmgr_trace("reply tuple type = %d\n", reply->tuple.tuple_type);

	/*
	 * Check tuple type of original and reply direction
	 */
	if (original->tuple.tuple_type != reply->tuple.tuple_type){
		netfn_flowmgr_warn("Tuple type is not matching for original and reply direction\n");
		return NETFN_FLOWMGR_RET_SFE_TUPLE_MISMATCH;
	}

	tuple_type = original->tuple.tuple_type;

	/*
	 * Based on tuple type validate the tuple info
	 */
	if (tuple_type == NETFN_TUPLE_3TUPLE) {
		/*
		 * IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_3.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_3.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_3.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_3.dest_ip[0];

		if (org_src_ip != reply_dst_ip || org_dst_ip != reply_src_ip) {
			netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
			netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
			netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
			return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
		}

		/*
		 * Original flow rule protocol should match with reply direction
		 */
		if (original->tuple.tuples.tuple_3.protocol != reply->tuple.tuples.tuple_3.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_3.protocol, reply->tuple.tuples.tuple_3.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}
	} else if (tuple_type == NETFN_TUPLE_4TUPLE) {
		/*
		 * IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_4.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_4.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_4.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_4.dest_ip[0];

		/*
		 * Ports
		 */
		org_ident = original->tuple.tuples.tuple_4.l4_ident;
		reply_ident = reply->tuple.tuples.tuple_4.l4_ident;

		/*
		 * Original flow IPs should match with reply direction
		 */
		if (org_src_ip != reply_dst_ip || org_dst_ip != reply_src_ip) {
			netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
			netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
			netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
			return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
		}

		/*
		 * Original flow rule ports should match with reply direction
		 */
		if (org_ident != reply_ident) {
			netfn_flowmgr_warn("Non-matching port numbers in flow and return direction\n");
			netfn_flowmgr_warn("org_ident = %u\n", org_ident);
			netfn_flowmgr_warn("reply_ident = %u\n", reply_ident);
			return NETFN_FLOWMGR_RET_SFE_PORT_MISMATCH;
		}

		/*
		 * Original flow rule protocol should reversly match with reply direction
		 */
		if (original->tuple.tuples.tuple_4.protocol != reply->tuple.tuples.tuple_4.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_4.protocol, reply->tuple.tuples.tuple_4.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}
        } else if (tuple_type == NETFN_TUPLE_5TUPLE) {
		/*
		 * IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_5.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_5.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_5.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_5.dest_ip[0];

		/*
		 * Ports
		 */
		org_src_ident = original->tuple.tuples.tuple_5.l4_src_ident;
		org_dst_ident = original->tuple.tuples.tuple_5.l4_dest_ident;
		reply_src_ident = reply->tuple.tuples.tuple_5.l4_src_ident;
		reply_dst_ident = reply->tuple.tuples.tuple_5.l4_dest_ident;

		/*
		 * Original flow IPs should match with reply direction
		 */
		if (org_src_ip != reply_dst_ip || org_dst_ip != reply_src_ip) {
			netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
			netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
			netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
			return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
		}

		/*
		 * Original flow rule ports should match with reply direction
		 */
		if ((org_src_ident != reply_dst_ident) || (org_dst_ident != reply_src_ident)) {
			netfn_flowmgr_warn("Non-matching port numbers in flow and return direction\n");
			netfn_flowmgr_warn("org_src_ident = %u, org_dst_ident = %u\n", org_src_ident, org_dst_ident);
			netfn_flowmgr_warn("reply_src_ident = %u, reply_dst_ident = %u\n", reply_src_ident, reply_dst_ident);
			return NETFN_FLOWMGR_RET_SFE_PORT_MISMATCH;
		}

		/*
		 * Original flow rule protocol should reversly match with reply direction
		 */
		if (original->tuple.tuples.tuple_5.protocol != reply->tuple.tuples.tuple_5.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_5.protocol, reply->tuple.tuples.tuple_5.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}

	} else {
		netfn_flowmgr_warn("Unsupported tuple type\n");
		return NETFN_FLOWMGR_RET_SFE_UNSUPPORTED_TUPLE_TYPE;
	}
	return status;
}

/*
 * netfn_flowmgr_sfe_destroy_v4_rule()
 *	destroys flow in sfe
 */
netfn_flowmgr_ret_t netfn_flowmgr_sfe_destroy_v4_rule(struct netfn_flowmgr_destroy_rule *original, struct netfn_flowmgr_destroy_rule *reply)
{
	netfn_tuple_type_t tuple_type;
	struct sfe_ipv4_msg *nim;
	struct sfe_ipv4_rule_destroy_msg *nirdm;
	netfn_flowmgr_ret_t netfn_status = NETFN_FLOWMGR_RET_SUCCESS;
	sfe_tx_status_t sfe_status = SFE_TX_SUCCESS;

	/*
	 * Validate SFE tuple information
	 */
	netfn_status = netfn_flowmgr_sfe_validate_v4_destroy_tuple_info(original, reply);
	if (netfn_status != NETFN_FLOWMGR_RET_SUCCESS) {
		netfn_flowmgr_warn("Invalid tuple info, SFE rule destroy failed\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(netfn_status, sfe_status);
	}

	nim = (struct sfe_ipv4_msg *)kzalloc(sizeof(struct sfe_ipv4_msg), GFP_ATOMIC);
	if (!nim) {
		netfn_flowmgr_warn("no memory for sfe ipv4 message structure instance: %px, %px\n", original, reply);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_NO_MEM, sfe_status);
	}

	nim->cm.type = SFE_TX_DESTROY_RULE_MSG;

	tuple_type = original->tuple.tuple_type;

	/*
	 * Prepare deceleration message
	 */
	sfe_ipv4_msg_init(nim, SFE_SPECIAL_INTERFACE_IPV4, SFE_TX_DESTROY_RULE_MSG,
			sizeof(struct sfe_ipv4_rule_destroy_msg), NULL, NULL);
	nirdm = &nim->msg.rule_destroy;
	nirdm->tuple.protocol = tuple_type;

	if (tuple_type == NETFN_TUPLE_3TUPLE) {
		nirdm->tuple.protocol = original->tuple.tuples.tuple_3.protocol;
		nirdm->tuple.flow_ip = original->tuple.tuples.tuple_3.src_ip[0];
		nirdm->tuple.return_ip = original->tuple.tuples.tuple_3.dest_ip[0];
	} else if (tuple_type == NETFN_TUPLE_4TUPLE) {
		nirdm->tuple.protocol = original->tuple.tuples.tuple_4.protocol;
		nirdm->tuple.flow_ip = original->tuple.tuples.tuple_4.src_ip[0];
		nirdm->tuple.return_ip = original->tuple.tuples.tuple_4.dest_ip[0];
		if (original->tuple.tuples.tuple_4.ident_type == NETFN_4TUPLE_VALID_SRC_PORT) {
			nirdm->tuple.flow_ident = original->tuple.tuples.tuple_4.l4_ident;
		} else {
			nirdm->tuple.return_ident = original->tuple.tuples.tuple_4.l4_ident;
		}
	} else {
		nirdm->tuple.protocol = original->tuple.tuples.tuple_5.protocol;
		nirdm->tuple.flow_ip = original->tuple.tuples.tuple_5.src_ip[0];
		nirdm->tuple.return_ip = original->tuple.tuples.tuple_5.dest_ip[0];
		nirdm->tuple.flow_ident = original->tuple.tuples.tuple_5.l4_src_ident;
		nirdm->tuple.return_ident = original->tuple.tuples.tuple_5.l4_dest_ident;
	}

	/*
	 * Dump the information passed to SFE acceleration engine
	 */
	netfn_flowmgr_trace("Destroy v4 flow from SFE:\n"
				"flow_ip: %pI4\n"
				"return_ip: %pI4\n"
				"flow_ident: %u\n"
				"return_ident: %u\n"
				"protocol: %u\n",
				&nirdm->tuple.flow_ip,
				&nirdm->tuple.return_ip,
				nirdm->tuple.flow_ident,
				nirdm->tuple.return_ident,
				nirdm->tuple.protocol);

	/*
	 * Destroy the SFE connection cache entry.
	 */
	sfe_status = sfe_ipv4_tx_with_resp(NULL, nim);
	if (sfe_status != SFE_TX_SUCCESS) {
		netfn_flowmgr_warn("SFE rule destroy failed with error: %u\n", sfe_status);
		netfn_status = NETFN_FLOWMGR_RET_DESTROY_RULE_FAILED;
		goto done;
	}

	netfn_flowmgr_warn("SFE v4 rule destroy success\n");
done:
	kfree(nim);
	return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(netfn_status, sfe_status);
}

/*
 * netfn_flowmgr_sfe_fill_v4_default_create_msg_values
 *	Fills default values necessary to accel SFE flow,
 *	These values can be changed later dependent on flow info
 */
void netfn_flowmgr_sfe_fill_v4_default_create_msg_values(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply, struct sfe_ipv4_rule_create_msg *nircm)
{
	struct net_device *in_dev;
	struct net_device *out_dev;
	struct net_device *top_indev;
	struct net_device *top_outdev;

	in_dev = original->flow_info.in_dev;
	out_dev = original->flow_info.out_dev;

	top_indev = original->flow_info.top_indev;
	top_outdev = original->flow_info.top_outdev;

	dev_hold(in_dev);
	dev_hold(out_dev);
	dev_hold(top_indev);
	dev_hold(top_outdev);

	/*
	 * Fill MAC Addresses
	 */
	memcpy(nircm->conn_rule.flow_mac, original->flow_info.flow_src_mac, ETH_ALEN);
	memcpy(nircm->conn_rule.return_mac,  original->flow_info.flow_dest_mac, ETH_ALEN);
	nircm->valid_flags |= SFE_RULE_CREATE_SRC_MAC_VALID;

	nircm->conn_rule.flow_interface_num = in_dev->ifindex;
	nircm->conn_rule.return_interface_num = out_dev->ifindex;
	nircm->conn_rule.flow_top_interface_num = top_indev->ifindex;
	nircm->conn_rule.return_top_interface_num = top_outdev->ifindex;

	dev_put(top_indev);
	dev_put(top_outdev);
	dev_put(in_dev);
	dev_put(out_dev);

	/*
	 * Fill default NAT IP/Port which are same as non NAT IP/Port
	 * In a NAT valid case these values will be changed in a later code block
	 */
	nircm->conn_rule.flow_ip_xlate = nircm->tuple.flow_ip;
	nircm->conn_rule.return_ip_xlate = nircm->tuple.return_ip;
	nircm->conn_rule.flow_ident_xlate = nircm->tuple.flow_ident;
	nircm->conn_rule.return_ident_xlate = nircm->tuple.return_ident;

	/*
	 * MTU info
	 */
	nircm->conn_rule.flow_mtu = original->flow_info.flow_mtu;
	nircm->conn_rule.return_mtu = reply->flow_info.flow_mtu;

	/*
	 * Set Default VLAN Values
	 */
	nircm->vlan_primary_rule.ingress_vlan_tag = SFE_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_primary_rule.egress_vlan_tag = SFE_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_secondary_rule.ingress_vlan_tag = SFE_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_secondary_rule.egress_vlan_tag = SFE_VLAN_ID_NOT_CONFIGURED;

	/*
	 * Fast XMIT Flags
	 *      TODO: Add more detailed flag checks
	 */
	nircm->rule_flags |= SFE_RULE_CREATE_FLAG_FLOW_TRANSMIT_FAST;
	nircm->rule_flags |= SFE_RULE_CREATE_FLAG_RETURN_TRANSMIT_FAST;

	/*
	 * IFACE Flags
	 */
	nircm->rule_flags |= SFE_RULE_CREATE_FLAG_USE_FLOW_BOTTOM_INTERFACE;
	nircm->rule_flags |= SFE_RULE_CREATE_FLAG_USE_RETURN_BOTTOM_INTERFACE;
}

/*
 * netfn_flowmgr_sfe_validate_v4_create_tuple_info()
 *	Validate tuple info in original and reply direction
 */
static netfn_flowmgr_ret_t netfn_flowmgr_sfe_validate_v4_create_tuple_info(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply)
{
	netfn_tuple_type_t tuple_type;
	uint32_t org_rules;
	uint32_t reply_rules;
	uint32_t org_src_ip;
	uint32_t org_dst_ip;
	uint32_t reply_src_ip;
	uint32_t reply_dst_ip;
	uint32_t org_src_ip_xlate;
	uint32_t org_dst_ip_xlate;
	uint16_t org_ident;
	uint16_t reply_ident;
	uint16_t org_src_ident;
	uint16_t org_dst_ident;
	uint32_t reply_src_ip_xlate;
	uint32_t reply_dst_ip_xlate;
	uint16_t reply_src_ident;
	uint16_t reply_dst_ident;
	uint16_t org_ident_xlate;
	uint16_t reply_ident_xlate;
	uint16_t org_src_ident_xlate;
	uint16_t org_dst_ident_xlate;
	uint16_t reply_src_ident_xlate;
	uint16_t reply_dst_ident_xlate;
	netfn_flowmgr_ret_t status = NETFN_FLOWMGR_RET_SUCCESS;

	netfn_flowmgr_trace("org tuple type = %d\n", original->tuple.tuple_type);
	netfn_flowmgr_trace("reply tuple type = %d\n", reply->tuple.tuple_type);

	/*
	 * Check tuple type of original and reply direction
	 */
	if (original->tuple.tuple_type != reply->tuple.tuple_type){
		netfn_flowmgr_warn("Tuple type is not matching for original and reply direction\n");
		return NETFN_FLOWMGR_RET_SFE_TUPLE_MISMATCH;
	}

	tuple_type = original->tuple.tuple_type;
	org_rules = original->rule_info.rule_valid_flags;
	reply_rules = original->rule_info.rule_valid_flags;

	netfn_flowmgr_trace("org_rules = 0x%x\n", org_rules);
	netfn_flowmgr_trace("reply_rules = 0x%x\n", reply_rules);

	/*
	 * IP translation rule
	 */
	if (((org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) && (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT)) ||
		((reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) && (reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT))) {
		netfn_flowmgr_warn("Failure due to both SNAT and DNAT present simultaneously in rules\n");
		return NETFN_FLOWMGR_RET_SFE_SIMUL_SNAT_DNAT;
	}

	if ((org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) && (reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT)) {
		if(original->flow_flags & NETFN_FLOWMGR_FLOW_FLAG_BRIDGE_FLOW) {
			netfn_flowmgr_warn("NAT with bridging is not supported\n");
			return NETFN_FLOWMGR_RET_SFE_NAT_WITH_BRIDGE_UNSUPPORTED;
		}
	}

	if (!(original->flow_info.in_dev && original->flow_info.out_dev && original->flow_info.top_indev && original->flow_info.top_outdev)) {
		netfn_flowmgr_warn("Invalid Net Device\n");
		return NETFN_FLOWMGR_RET_INVALID_RULE;
	}

	/*
         * Xlate IPs
         */
	org_src_ip_xlate = original->rule_info.ip_xlate_rule.src_ip_xlate[0];
	org_dst_ip_xlate = original->rule_info.ip_xlate_rule.dest_ip_xlate[0];
	reply_src_ip_xlate = reply->rule_info.ip_xlate_rule.src_ip_xlate[0];
	reply_dst_ip_xlate = reply->rule_info.ip_xlate_rule.dest_ip_xlate[0];

	/*
	 * Based on tuple type validate the tuple info
	 */
	if (tuple_type == NETFN_TUPLE_3TUPLE) {
		/*
		 * Non-xlate IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_3.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_3.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_3.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_3.dest_ip[0];

		/*
		 * There are two cases
		 *
		 * Case1: NAT
		 */
		if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip) && (org_src_ip != reply_dst_ip_xlate)) &&
				(org_dst_ip != reply_src_ip) && (org_dst_ip_xlate != reply_src_ip_xlate)){
				netfn_flowmgr_warn("Invalid SNAT configuration\n");
				return NETFN_FLOWMGR_RET_SFE_INVALID_SNAT_IP_CONFIGURATION;
			}
		} else if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip_xlate) && (org_src_ip != reply_dst_ip)) &&
				(org_dst_ip != reply_src_ip_xlate) && (org_dst_ip_xlate != reply_src_ip)){
				netfn_flowmgr_warn("Invalid DNAT configuration\n");
				return NETFN_FLOWMGR_RET_SFE_INVALID_DNAT_IP_CONFIGURATION;
			}
		} else {
			/*
			 * Case 2: Non-NAT
			 */
			if ((org_src_ip != reply_dst_ip) || (org_dst_ip != reply_src_ip)) {
				netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
				netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
				netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
				return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
			}
		}

		/*
		 * Original flow rule protocol should match with reply direction
		 */
		if (original->tuple.tuples.tuple_3.protocol != reply->tuple.tuples.tuple_3.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_3.protocol, reply->tuple.tuples.tuple_3.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}
	} else if (tuple_type == NETFN_TUPLE_4TUPLE) {
		/*
		 * Non-xlate IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_4.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_4.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_4.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_4.dest_ip[0];

		/*
		 * Ports
		 */
		org_ident = original->tuple.tuples.tuple_4.l4_ident;
		reply_ident = reply->tuple.tuples.tuple_4.l4_ident;

		/*
		 * xlate ports
		 */
		org_ident_xlate = original->rule_info.ip_xlate_rule.dest_port_xlate;
		reply_ident_xlate = original->rule_info.ip_xlate_rule.src_port_xlate;

		/*
		 * There are two cases
		 *
		 * Case1: NAT
		 */
		if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip) && (org_src_ip != reply_dst_ip_xlate)) &&
				(org_dst_ip != reply_src_ip) && (org_dst_ip_xlate != reply_src_ip_xlate)) {
				return NETFN_FLOWMGR_RET_SFE_INVALID_SNAT_IP_CONFIGURATION;
			}

			/*
			 * Port xlate check
			 */
			if((org_ident != reply_ident) && (org_ident_xlate != reply_ident_xlate)) {
				return NETFN_FLOWMGR_RET_SFE_INVALID_SNAT_PORT_CONFIGURATION;
			}
		} else if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip_xlate) && (org_src_ip != reply_dst_ip)) &&
				(org_dst_ip != reply_src_ip_xlate) && (org_dst_ip_xlate != reply_src_ip)){
				return NETFN_FLOWMGR_RET_SFE_INVALID_DNAT_IP_CONFIGURATION;
			}

			/*
			 * Port xlate check
			 */
			if((org_ident != reply_ident_xlate) && (org_ident_xlate != reply_ident)){
				return NETFN_FLOWMGR_RET_SFE_INVALID_DNAT_PORT_CONFIGURATION;
			}
		} else {
			/*
			 * Case 2: Non-NAT
			 */
			if (org_src_ip != reply_dst_ip || org_dst_ip != reply_src_ip) {
				netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
				netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
				netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
				return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
			}

			/*
			 * Original flow rule ports should match with reply direction
			 */
			if (org_ident != reply_ident) {
				netfn_flowmgr_warn("Non-matching port numbers in flow and return direction\n");
				netfn_flowmgr_warn("org_ident = %u\n", org_ident);
				netfn_flowmgr_warn("reply_ident = %u\n", reply_ident);
				return NETFN_FLOWMGR_RET_SFE_PORT_MISMATCH;
			}
		}

		/*
		 * Original flow rule protocol should reversly match with reply direction
		 */
		if (original->tuple.tuples.tuple_4.protocol != reply->tuple.tuples.tuple_4.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_4.protocol, reply->tuple.tuples.tuple_4.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}
	} else if (tuple_type == NETFN_TUPLE_5TUPLE) {
		/*
		 * Non-xlate IPs
		 */
		org_src_ip = original->tuple.tuples.tuple_5.src_ip[0];
		org_dst_ip = original->tuple.tuples.tuple_5.dest_ip[0];
		reply_src_ip = reply->tuple.tuples.tuple_5.src_ip[0];
		reply_dst_ip = reply->tuple.tuples.tuple_5.dest_ip[0];

		/*
		 * Ports
		 */
		org_src_ident = original->tuple.tuples.tuple_5.l4_src_ident;
		org_dst_ident = original->tuple.tuples.tuple_5.l4_dest_ident;
		reply_src_ident = reply->tuple.tuples.tuple_5.l4_src_ident;
		reply_dst_ident = reply->tuple.tuples.tuple_5.l4_dest_ident;

		/*
		 * xlate ports
		 */
		org_src_ident_xlate = original->rule_info.ip_xlate_rule.src_port_xlate;
		org_dst_ident_xlate = original->rule_info.ip_xlate_rule.dest_port_xlate;
		reply_src_ident_xlate = reply->rule_info.ip_xlate_rule.src_port_xlate;
		reply_dst_ident_xlate = reply->rule_info.ip_xlate_rule.dest_port_xlate;

		/*
		 * There are two cases
		 *
		 * Case1: NAT
		 */
		if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip) && (org_src_ip != reply_dst_ip_xlate)) &&
				(org_dst_ip != reply_src_ip) && (org_dst_ip_xlate != reply_src_ip_xlate)) {
				return NETFN_FLOWMGR_RET_SFE_INVALID_SNAT_IP_CONFIGURATION;
			}

			/*
			 * Port xlate check
			 */
			if(((org_src_ident_xlate != reply_dst_ident) && (org_src_ident != reply_dst_ident_xlate)) &&
				(org_dst_ident != reply_src_ident) && (org_dst_ident_xlate != reply_src_ident_xlate)) {
				return NETFN_FLOWMGR_RET_SFE_INVALID_SNAT_PORT_CONFIGURATION;
			}
		} else if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT) {
			/*
			 * IP xlate check
			 */
			if(((org_src_ip_xlate != reply_dst_ip_xlate) && (org_src_ip != reply_dst_ip)) &&
				(org_dst_ip != reply_src_ip_xlate) && (org_dst_ip_xlate != reply_src_ip)){
				return NETFN_FLOWMGR_RET_SFE_INVALID_DNAT_IP_CONFIGURATION;
			}

			/*
			 * Port xlate check
			 */
			if(((org_src_ident_xlate != reply_dst_ident_xlate) && (org_src_ident != reply_dst_ident)) &&
				(org_dst_ident != reply_src_ident_xlate) && (org_dst_ident_xlate != reply_src_ident)){
				return NETFN_FLOWMGR_RET_SFE_INVALID_DNAT_PORT_CONFIGURATION;
			}
		} else {
			/*
			 * Case 2: Non-NAT
			 */
			if (org_src_ip != reply_dst_ip || org_dst_ip != reply_src_ip) {
				netfn_flowmgr_warn("Non-matching IP addresses in flow and return direction\n");
				netfn_flowmgr_warn("org_src_ip = %pI4, org_dst_ip = %pI4\n", &org_src_ip, &org_dst_ip);
				netfn_flowmgr_warn("reply_src_ip = %pI4, reply_dst_ip = %pI4\n", &reply_src_ip, &reply_dst_ip);
				return NETFN_FLOWMGR_RET_SFE_IP_ADDR_MISMATCH;
			}

			/*
			 * Original flow rule ports should match with reply direction
			 */
			if ((org_src_ident != reply_dst_ident) || (org_dst_ident != reply_src_ident)) {
				netfn_flowmgr_warn("Non-matching port numbers in flow and return direction\n");
				netfn_flowmgr_warn("org_src_ident = %u, org_dst_ident = %u\n", org_src_ident, org_dst_ident);
				netfn_flowmgr_warn("reply_src_ident = %u, reply_dst_ident = %u\n", reply_src_ident, reply_dst_ident);
				return NETFN_FLOWMGR_RET_SFE_PORT_MISMATCH;
			}
		}

		/*
		 * Original flow rule protocol should reversly match with reply direction
		 */
		if (original->tuple.tuples.tuple_5.protocol != reply->tuple.tuples.tuple_5.protocol) {
			netfn_flowmgr_warn("Non-matching protocol in flow and return direction, org_proto = %u, reply_proto = %u\n",
						original->tuple.tuples.tuple_5.protocol, reply->tuple.tuples.tuple_5.protocol);
			return NETFN_FLOWMGR_RET_SFE_PROTOCOL_MISMATCH;
		}

	} else {
		netfn_flowmgr_warn("Invalid Tuple Type for SFE, tuple_type = %d\n", tuple_type);
		return NETFN_FLOWMGR_RET_SFE_UNSUPPORTED_TUPLE_TYPE;
	}
	return status;
}

/*
 * netfn_flowmgr_sfe_create_v4_rule()
 *	accels flow in sfe
 */
netfn_flowmgr_ret_t netfn_flowmgr_sfe_create_v4_rule(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply)
{
	struct sfe_ipv4_msg *nim;
	struct sfe_ipv4_rule_create_msg *nircm;
	netfn_tuple_type_t tuple_type;
	uint32_t org_rules;
	uint32_t reply_rules;
	sfe_tx_status_t sfe_status = SFE_TX_SUCCESS;
	netfn_flowmgr_ret_t netfn_status = NETFN_FLOWMGR_RET_SUCCESS;

	/*
	 * Validate SFE tuple information
	 */
	netfn_status = netfn_flowmgr_sfe_validate_v4_create_tuple_info(original, reply);
	if (netfn_status != NETFN_FLOWMGR_RET_SUCCESS) {
		netfn_flowmgr_warn("Invalid v4 tuple info, SFE rule creation failed\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(netfn_status, sfe_status);
	}

	nim = (struct sfe_ipv4_msg *)kzalloc(sizeof(struct sfe_ipv4_msg), GFP_ATOMIC);
	if (!nim) {
		netfn_flowmgr_warn("no memory for sfe ipv4 message structure instance: %px, %px\n", original, reply);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_NO_MEM, sfe_status);
	}

	nim->cm.type = SFE_TX_CREATE_RULE_MSG;

	/*
	 * Initialize creation structure.
	 */
	sfe_ipv4_msg_init(nim, SFE_SPECIAL_INTERFACE_IPV4, SFE_TX_CREATE_RULE_MSG,
			sizeof(struct sfe_ipv4_rule_create_msg),
			NULL, NULL);

	nircm = &nim->msg.rule_create;

	/*
	 * Fill local variables
	 */
	org_rules = original->rule_info.rule_valid_flags;
	reply_rules = original->rule_info.rule_valid_flags;
	tuple_type = original->tuple.tuple_type;

	if (tuple_type == NETFN_TUPLE_3TUPLE) {
		nircm->tuple.protocol = original->tuple.tuples.tuple_3.protocol;
		nircm->tuple.flow_ip = original->tuple.tuples.tuple_3.src_ip[0];
		nircm->tuple.return_ip = original->tuple.tuples.tuple_3.dest_ip[0];
	} else if (tuple_type == NETFN_TUPLE_4TUPLE) {
		nircm->tuple.protocol = original->tuple.tuples.tuple_4.protocol;
		nircm->tuple.flow_ip = original->tuple.tuples.tuple_4.src_ip[0];
		nircm->tuple.return_ip = original->tuple.tuples.tuple_4.dest_ip[0];
		nircm->tuple.return_ident = original->tuple.tuples.tuple_4.l4_ident;
		if (original->tuple.tuples.tuple_4.ident_type == NETFN_4TUPLE_VALID_SRC_PORT) {
			nircm->tuple.flow_ident = original->tuple.tuples.tuple_4.l4_ident;
		} else {
			nircm->tuple.return_ident = original->tuple.tuples.tuple_4.l4_ident;
			nircm->rule_flags |= SFE_RULE_CREATE_FLAG_NO_SRC_IDENT;
		}
	} else {
		nircm->tuple.protocol = original->tuple.tuples.tuple_5.protocol;
		nircm->tuple.flow_ip = original->tuple.tuples.tuple_5.src_ip[0];
		nircm->tuple.return_ip = original->tuple.tuples.tuple_5.dest_ip[0];
		nircm->tuple.flow_ident = original->tuple.tuples.tuple_5.l4_src_ident;
		nircm->tuple.return_ident = original->tuple.tuples.tuple_5.l4_dest_ident;
	}

	netfn_flowmgr_sfe_fill_v4_default_create_msg_values(original, reply, nircm);

	/*
	 * TCP Valid
	 */
	if (nircm->tuple.protocol == IPPROTO_TCP) {
		nircm->valid_flags |= SFE_RULE_CREATE_TCP_VALID;
		if (!(org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_TCP)) {
			nircm->rule_flags |= SFE_RULE_CREATE_FLAG_NO_SEQ_CHECK;
		} else {
			nircm->tcp_rule.flow_window_scale = original->rule_info.tcp_rule.window_scale;
			nircm->tcp_rule.flow_max_window = original->rule_info.tcp_rule.window_max;
			nircm->tcp_rule.flow_end = original->rule_info.tcp_rule.end;
			nircm->tcp_rule.flow_max_end = original->rule_info.tcp_rule.max_end;
			nircm->tcp_rule.return_window_scale = reply->rule_info.tcp_rule.window_scale;
			nircm->tcp_rule.return_max_window = reply->rule_info.tcp_rule.window_max;
			nircm->tcp_rule.return_end = reply->rule_info.tcp_rule.end;
			nircm->tcp_rule.return_max_end = reply->rule_info.tcp_rule.max_end;
		}
	}

	/*
	 * Bridged Flow
	 */
	if (original->flow_flags & SFE_RULE_CREATE_FLAG_BRIDGE_FLOW) {
		nircm->rule_flags |= SFE_RULE_CREATE_FLAG_BRIDGE_FLOW;
	}

	/*
	 * SRC MAC rule
	 */
	if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_MAC) {
		memcpy(nircm->src_mac_rule.flow_src_mac, original->rule_info.mac_xlate_rule.src_mac, ETH_ALEN);
		memcpy(nircm->src_mac_rule.return_src_mac, original->rule_info.mac_xlate_rule.dest_mac, ETH_ALEN);
		nircm->src_mac_rule.mac_valid_flags |= SFE_SRC_MAC_FLOW_VALID;
		nircm->src_mac_rule.mac_valid_flags |= SFE_SRC_MAC_RETURN_VALID;
	}

	/*
	 * NAT
	 */
	if (((org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT) && (reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT)) ||
		((org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DST_NAT) && (reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_SRC_NAT))) {
		nircm->conn_rule.flow_ip_xlate = original->rule_info.ip_xlate_rule.src_ip_xlate[0];
		nircm->conn_rule.flow_ident_xlate = original->rule_info.ip_xlate_rule.src_port_xlate;
		nircm->conn_rule.return_ip_xlate = original->rule_info.ip_xlate_rule.dest_ip_xlate[0];
		nircm->conn_rule.return_ident_xlate = original->rule_info.ip_xlate_rule.dest_port_xlate;
	}

	/*
	 * VLAN rule
	 */
	if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_VLAN) {
		/*
		 * Validate VLAN info
		 */
		netfn_status = netfn_flowmgr_sfe_validate_vlan_info(original, reply);
		if (netfn_status != NETFN_FLOWMGR_RET_SUCCESS) {
			netfn_flowmgr_warn("Invalid vlan info, SFE rule creation failed\n");
			goto done;
		}

		if (original->rule_info.vlan_rule.inner.ingress_vlan_tag) {
			nircm->vlan_primary_rule.ingress_vlan_tag = ((original->rule_info.vlan_rule.inner_vlan_tpid << 16) | original->rule_info.vlan_rule.inner.ingress_vlan_tag);
		}

		if (original->rule_info.vlan_rule.inner.egress_vlan_tag) {
			nircm->vlan_primary_rule.egress_vlan_tag = ((original->rule_info.vlan_rule.inner_vlan_tpid << 16) | original->rule_info.vlan_rule.inner.egress_vlan_tag);
		}

		if (original->rule_info.vlan_rule.outer.ingress_vlan_tag) {
			nircm->vlan_secondary_rule.ingress_vlan_tag = ((original->rule_info.vlan_rule.outer_vlan_tpid << 16) | original->rule_info.vlan_rule.outer.ingress_vlan_tag);
		}

		if (original->rule_info.vlan_rule.outer.egress_vlan_tag) {
			nircm->vlan_secondary_rule.egress_vlan_tag = ((original->rule_info.vlan_rule.outer_vlan_tpid << 16) | original->rule_info.vlan_rule.outer.egress_vlan_tag);
		}
		nircm->valid_flags |= SFE_RULE_CREATE_VLAN_VALID;
	}

	/*
	 * DSCP rule
	 */
	if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DSCP_MARKING || reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_DSCP_MARKING ) {
		nircm->dscp_rule.flow_dscp = original->rule_info.dscp_rule.dscp_val;
		nircm->dscp_rule.return_dscp = reply->rule_info.dscp_rule.dscp_val;
		nircm->rule_flags |= SFE_RULE_CREATE_FLAG_DSCP_MARKING;
		nircm->valid_flags |= SFE_RULE_CREATE_DSCP_MARKING_VALID;
	}

	/*
	 * QoS rule
	 */
	if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_QOS || reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_QOS) {
		nircm->qos_rule.flow_qos_tag = original->rule_info.qos_rule.qos_tag;
		nircm->qos_rule.return_qos_tag = reply->rule_info.qos_rule.qos_tag;
		if (original->rule_info.qos_rule.priority) {
			nircm->qos_rule.flow_int_pri = original->rule_info.qos_rule.priority;
		}
		if (reply->rule_info.qos_rule.priority) {
			nircm->qos_rule.return_int_pri = reply->rule_info.qos_rule.priority;
		}
		nircm->valid_flags |= SFE_RULE_CREATE_QOS_VALID;
	}

	/*
	 * PPPoE rule
	 */
	if (org_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_PPPOE) {
		nircm->pppoe_rule.flow_pppoe_session_id = original->rule_info.pppoe_rule.session_id;
		memcpy(nircm->pppoe_rule.flow_pppoe_remote_mac, original->rule_info.pppoe_rule.server_mac, ETH_ALEN);
		nircm->valid_flags |= SFE_RULE_CREATE_PPPOE_DECAP_VALID;
	}

	if (reply_rules & NETFN_FLOWMGR_VALID_RULE_FLAG_PPPOE) {
		nircm->pppoe_rule.return_pppoe_session_id = reply->rule_info.pppoe_rule.session_id;
		memcpy(nircm->pppoe_rule.return_pppoe_remote_mac, reply->rule_info.pppoe_rule.server_mac, ETH_ALEN);
		nircm->valid_flags |= SFE_RULE_CREATE_PPPOE_ENCAP_VALID;
	}

	nircm->valid_flags |= SFE_RULE_CREATE_CONN_VALID;

	/*
	 * Dump the information passed to SFE acceleration engine
	 */
	netfn_flowmgr_trace("Accelerate v4 flow to SFE:\n"
				"flow_ip: %pI4\n"
				"return_ip: %pI4\n"
				"flow_ident: %u\n"
				"return_ident: %u\n"
				"protocol: %u\n"
				"valid_flags: 0x%x\n"
				"rule_flags: 0x%x\n"
				"flow_interface_num: %u\n"
				"return_interface_num: %u\n"
				"flow_top_interface_num: %u\n"
				"return_top_interface_num: %u\n"
				"flow_mac: %pM\n"
				"return_mac: %pM\n"
				"flow_mtu: %u\n"
				"return_mtu: %u\n"
				"flow_ip_xlate: %pI4\n"
				"return_ip_xlate: %pI4\n"
				"flow_ident_xlate: %u\n"
				"return_ident_xlate: %u\n"
				"ingress_inner_vlan_tag: 0x%x\n"
				"egress_inner_vlan_tag: 0x%x\n"
				"ingress_outer_vlan_tag: 0x%x\n"
				"egress_outer_vlan_tag: 0x%x\n"
				"flow_pppoe_session_id: %u\n"
				"flow_pppoe_remote_mac: %pM\n"
				"return_pppoe_session_id: %u\n"
				"return_pppoe_remote_mac: %pM\n"
				"tcp flow_max_window: %u\n"
				"tcp return_max_window: %u\n"
				"tcp flow_end: %u\n"
				"tcp return_end: %u\n"
				"tcp flow_max_end: %u\n"
				"tcp return_max_end: %u\n"
				"tcp flow_window_scale: %u\n"
				"tcp return_window_scale: %u\n"
				"flow_dscp: %u\n"
				"return_dscp: %u\n",
				&nircm->tuple.flow_ip,
				&nircm->tuple.return_ip,
				nircm->tuple.flow_ident,
				nircm->tuple.return_ident,
				nircm->tuple.protocol,
				nircm->valid_flags,
				nircm->rule_flags,
				nircm->conn_rule.flow_interface_num,
				nircm->conn_rule.return_interface_num,
				nircm->conn_rule.flow_top_interface_num,
				nircm->conn_rule.return_top_interface_num,
				nircm->conn_rule.flow_mac,
				nircm->conn_rule.return_mac,
				nircm->conn_rule.flow_mtu,
				nircm->conn_rule.return_mtu,
				&nircm->conn_rule.flow_ip_xlate,
				&nircm->conn_rule.return_ip_xlate,
				nircm->conn_rule.flow_ident_xlate,
				nircm->conn_rule.return_ident_xlate,
				nircm->vlan_primary_rule.ingress_vlan_tag,
				nircm->vlan_primary_rule.egress_vlan_tag,
				nircm->vlan_secondary_rule.ingress_vlan_tag,
				nircm->vlan_secondary_rule.egress_vlan_tag,
				nircm->pppoe_rule.flow_pppoe_session_id,
				nircm->pppoe_rule.flow_pppoe_remote_mac,
				nircm->pppoe_rule.return_pppoe_session_id,
				nircm->pppoe_rule.return_pppoe_remote_mac,
				nircm->tcp_rule.flow_max_window,
				nircm->tcp_rule.return_max_window,
				nircm->tcp_rule.flow_end,
				nircm->tcp_rule.return_end,
				nircm->tcp_rule.flow_max_end,
				nircm->tcp_rule.return_max_end,
				nircm->tcp_rule.flow_window_scale,
				nircm->tcp_rule.return_window_scale,
				nircm->dscp_rule.flow_dscp,
				nircm->dscp_rule.return_dscp);

	sfe_status = sfe_ipv4_tx_with_resp(NULL, nim);
	if (sfe_status != SFE_TX_SUCCESS) {
		netfn_flowmgr_warn("SFE rule create failed with error: %u\n", sfe_status);
		netfn_status = NETFN_FLOWMGR_RET_CREATE_RULE_FAILED;
		goto done;
	}
	netfn_flowmgr_info("SFE rule create success\n");
done:
	kfree(nim);
	return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(netfn_status, sfe_status);
}
