/*
 **************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 **************************************************************************
 */

/*
 * netfn_flowmgr_ppe.c
 *	Netfn flow manager ppe file
 */
#include <linux/types.h>
#include <netfn_flowmgr.h>
#include <flowmgr/netfn_flowmgr_priv.h>
#include "netfn_flowmgr_ppe.h"
#include "netfn_flowmgr_ppe_ipv4.h"
#include "netfn_flowmgr_ppe_ipv6.h"

/*
 * netfn_flowmgr_ppe_create_rule()
 *	Create ppe flow rule
 */
netfn_flowmgr_ret_t netfn_flowmgr_ppe_create_rule(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply)
{
	enum netfn_flowmgr_tuple_ip_version org_ipver;
	enum netfn_flowmgr_tuple_ip_version reply_ipver;

	org_ipver = original->tuple.ip_version;
	reply_ipver = reply->tuple.ip_version;

	/*
	 * IP version check
	 * Both original and reply direction should have matching ip version.
	 */
	if (org_ipver != reply_ipver) {
		netfn_flowmgr_warn("IP version is not matching for original and reply direction\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_IP_VERSION_MISMATCH, 0);
	}

	if(org_ipver == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {
		/*
		 * If IP version is v4, call v4 create rule API
		 */
		return netfn_flowmgr_ppe_create_v4_rule(original, reply);
	}
	else if (org_ipver == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {
		/*
		 * If IP version is v6, call v6 create rule API
		 */
		return netfn_flowmgr_ppe_create_v6_rule(original, reply);
	} else {
		netfn_flowmgr_warn("Incorrect IP version\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_INCORRECT_IP_VERSION, 0);
	}
}

/*
 * netfn_flowmgr_ppe_destroy_rule()
 *	Destroy ppe flow rule
 */
netfn_flowmgr_ret_t netfn_flowmgr_ppe_destroy_rule(struct netfn_flowmgr_destroy_rule *original, struct netfn_flowmgr_destroy_rule *reply)
{
	enum netfn_flowmgr_tuple_ip_version org_ipver;
	enum netfn_flowmgr_tuple_ip_version reply_ipver;

	org_ipver = original->tuple.ip_version;
	reply_ipver = reply->tuple.ip_version;

	/*
	 * IP version check
	 * Both original and reply direction should have matching ip version.
	 */
	if (org_ipver != reply_ipver) {
		netfn_flowmgr_warn("IP version is not matching for original and reply direction\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_IP_VERSION_MISMATCH, 0);
	}

	if(org_ipver == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {
		/*
		 * If IP version is v4, call v4 destroy rule API
		 */
		return netfn_flowmgr_ppe_destroy_v4_rule(original, reply);
	} else if (org_ipver == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {
		/*
		 * If IP version is v6, call v6 destroy rule API
		 */
		return netfn_flowmgr_ppe_destroy_v6_rule(original, reply);
	} else {
		netfn_flowmgr_warn("Incorrect IP version\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_INCORRECT_IP_VERSION, 0);
	}
}

/*
 * netfn_flowmgr_ppe_dscp_pri_add()
 *	Add dscp based prioritization
 */
netfn_flowmgr_ret_t netfn_flowmgr_ppe_dscp_pri_add(struct netfn_flowmgr_dscp_priority *dscpinfo)
{
	struct ppe_acl_rule acl_rule = {0};
	struct net_device *netdev = NULL;
	ppe_acl_ret_t ppe_acl_status;
	netfn_flowmgr_ret_t status = 0;

	/*
	 * Create acl rule
	 */
	acl_rule.valid_flags |= PPE_ACL_RULE_MATCH_TYPE_DSCP_TC_VALID;

	/*
	 * Bind the ACL rule to the WAN port
	 */
	acl_rule.stype = PPE_ACL_RULE_SRC_TYPE_DEV;
        netdev = dscpinfo->src_dev;
	if (!netdev) {
		netfn_flowmgr_warn("No WAN net device\n");
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_INVALID_DSCP_RULE, 0);
	}

	strlcpy(acl_rule.src.dev_name, netdev->name, IFNAMSIZ);
	acl_rule.rules[PPE_ACL_RULE_MATCH_TYPE_DSCP_TC].rule.dscp_tc.l3_dscp_tc = dscpinfo->dscp_val;

	/*
	 * Action
	 */
	acl_rule.cmn.cmn_flags |=  PPE_ACL_RULE_CMN_FLAG_NO_RULEID;
	acl_rule.action.flags = PPE_ACL_RULE_ACTION_FLAG_ENQUEUE_PRI_CHANGE_EN;
	acl_rule.action.flags |= PPE_ACL_RULE_ACTION_FLAG_REDIR_TO_CORE_EN;
	acl_rule.action.enqueue_pri = dscpinfo->priority;
	acl_rule.action.redir_core = dscpinfo->core_id;

	/*
	 * Call ppe acl rule create API
	 */
	ppe_acl_status = ppe_acl_rule_create(&acl_rule);
	if (ppe_acl_status != PPE_ACL_RET_SUCCESS) {
                netfn_flowmgr_warn("Failed to create ACL rule for dscp prioritization, error:%d", ppe_acl_status);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_DSCP_RULE_ADD_FAILED, ppe_acl_status);
        }

	dscpinfo->rule_id = acl_rule.rule_id;
	netfn_flowmgr_trace("DSCP priority added, rule_id = %u", dscpinfo->rule_id);

	return status;
}

/*
 * netfn_flowmgr_ppe_dscp_pri_del()
 *	Delete dscp based prioritization
 */
netfn_flowmgr_ret_t netfn_flowmgr_ppe_dscp_pri_del(int16_t rule_id)
{
	netfn_flowmgr_ret_t status = 0;
	ppe_acl_ret_t ppe_acl_status;

	/*
	 * Call ppe acl rule destroy API with rule id obtained during create
	 */
	ppe_acl_status = ppe_acl_rule_destroy(rule_id);
	if (ppe_acl_status != PPE_ACL_RET_SUCCESS) {
                netfn_flowmgr_warn("Failed to destroy ACL rule for ruleid:%d, error:%d", rule_id, ppe_acl_status);
		return NETFN_FLOWMGR_SET_NETFN_STATUS_CODE(NETFN_FLOWMGR_RET_DSCP_RULE_DEL_FAILED, ppe_acl_status);
        }

	netfn_flowmgr_trace("ACL rule for rule_id:%d destroyed successfully\n", rule_id);
	return status;
}

/*
 * netfn_flowmgr_ppe_validate_vlan_info()
 *	Validate vlan info in original and reply direction
 */
netfn_flowmgr_ret_t netfn_flowmgr_ppe_validate_vlan_info(struct netfn_flowmgr_create_rule *original, struct netfn_flowmgr_create_rule *reply)
{
	uint32_t org_inner_ingress_vlan_tag;
	uint32_t org_inner_egress_vlan_tag;
	uint32_t org_outer_ingress_vlan_tag;
	uint32_t org_outer_egress_vlan_tag;
	uint32_t reply_inner_ingress_vlan_tag;
	uint32_t reply_inner_egress_vlan_tag;
	uint32_t reply_outer_ingress_vlan_tag;
	uint32_t reply_outer_egress_vlan_tag;
	netfn_flowmgr_ret_t status = NETFN_FLOWMGR_RET_SUCCESS;

	/*
	 * Original direction inner and outer vlan rules
	 */
	org_inner_ingress_vlan_tag = original->rule_info.vlan_rule.inner.ingress_vlan_tag;
	org_inner_egress_vlan_tag = original->rule_info.vlan_rule.inner.egress_vlan_tag;
	org_outer_ingress_vlan_tag = original->rule_info.vlan_rule.outer.ingress_vlan_tag;
	org_outer_egress_vlan_tag = original->rule_info.vlan_rule.outer.egress_vlan_tag;

	/*
	 * Reply direction inner and outer vlan rules
	 */
	reply_inner_ingress_vlan_tag = reply->rule_info.vlan_rule.inner.ingress_vlan_tag;
	reply_inner_egress_vlan_tag = reply->rule_info.vlan_rule.inner.egress_vlan_tag;
	reply_outer_ingress_vlan_tag = reply->rule_info.vlan_rule.outer.ingress_vlan_tag;
	reply_outer_egress_vlan_tag = reply->rule_info.vlan_rule.outer.egress_vlan_tag;

	/*
	 * Inner VLAN(primary) rule match
	 */
	if ((org_inner_ingress_vlan_tag != reply_inner_egress_vlan_tag) || (org_inner_egress_vlan_tag != reply_inner_ingress_vlan_tag)) {
		netfn_flowmgr_warn("Inner VLAN config mismatch in org and reply direction\n");
		netfn_flowmgr_warn("org_inner_ingress_vlan_tag = 0x%x, org_inner_egress_vlan_tag = 0x%x\n",
						org_inner_ingress_vlan_tag, org_inner_egress_vlan_tag);
		netfn_flowmgr_warn("reply_inner_ingress_vlan_tag = 0x%x, reply_inner_egress_vlan_tag = 0x%x\n",
						reply_inner_ingress_vlan_tag, reply_inner_egress_vlan_tag);
		status = NETFN_FLOWMGR_RET_PPE_INNER_VLAN_MISMATCH;
	}

	/*
	 * Outer VLAN(secondary) rule match
	 */
	if ((org_outer_ingress_vlan_tag != reply_outer_egress_vlan_tag) || (org_outer_egress_vlan_tag != reply_outer_ingress_vlan_tag)) {
		netfn_flowmgr_warn("Outer VLAN config mismatch in org and reply direction\n");
		status = NETFN_FLOWMGR_RET_PPE_OUTER_VLAN_MISMATCH;
	}
	return status;
}

/*
 * netfn_flowmgr_ppe_change_ip_endianess()
 *	Change the endianess from network to host order.
 */
void netfn_flowmgr_ppe_change_ip_endianess(uint32_t ip_addr[], int ip_version)
{
	/*
	 * Change the endianess of IPv4 addr from network to host
	 */
	if (ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {
		ip_addr[0] = ntohl(ip_addr[0]);
	}

	/*
	 * Change the endianess of IPv6 addr from network to host
	 */
	if (ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {
		ip_addr[0] = ntohl(ip_addr[0]);
		ip_addr[1] = ntohl(ip_addr[1]);
		ip_addr[2] = ntohl(ip_addr[2]);
		ip_addr[3] = ntohl(ip_addr[3]);
	}
}

/*
 * netfn_flowmgr_ppe_deinit()
 *	De-initialization of PPE callbacks
 */
void netfn_flowmgr_ppe_deinit(void)
{
	/*
	 * Unregister the callbacks with netfn core.
	 */
	netfn_flowmgr_ae_ops_unregister(NETFN_FLOWMGR_AE_TYPE_PPE);
}

/*
 * netfn_flowmgr_ppe_init()
 *	PPE initialization.
 */
bool netfn_flowmgr_ppe_init(void)
{
	struct netfn_flowmgr_ae_ops ppe_ops = {0};
	netfn_flowmgr_ae_type_t ae_type = NETFN_FLOWMGR_AE_TYPE_PPE;

	/*
	 * Assign PPE rule function pointers.
	 */
	ppe_ops.create_rule = netfn_flowmgr_ppe_create_rule;
	ppe_ops.destroy_rule = netfn_flowmgr_ppe_destroy_rule;

	/*
	 * Register the callbacks with netfn core.
	 */
	netfn_flowmgr_ae_ops_register(ae_type, &ppe_ops);

	return true;
}
