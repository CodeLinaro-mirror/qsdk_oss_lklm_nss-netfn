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

/*
 * netfn_auto_flowcookie.c
 *	Netfn auto flowcookie
 */
#include <netfn_flow_cookie.h>
#include "netfn_auto_flowcookie.h"

/*
 * genl_family for flowcookie
 */
static struct genl_family netfn_auto_flowcookie_genl_family;

/*
 * prototypes
 */
static inline int netfn_auto_flowcookie_genl_cmd(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_rule_add(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_rule_delete(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_rule_init(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_flow_id_get(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_flow_mark_get(struct sk_buff *skb, struct genl_info *info);
static inline int netfn_auto_flowcookie_sawf_handle_get(struct sk_buff *skb, struct genl_info *info);

/*
 * nla_policy for flowcookie
 */
static struct nla_policy netfn_auto_flowcookie_genl_policy[NETFN_AUTO_FLOWCOOKIE_GNL_MAX + 1] = {
    [FLOWCOOKIE_INFO]			= { .type = NLA_NESTED, },
	[FLOWCOOKIE_TUPLE_INFO]		= { .type = NLA_NESTED, },
	[FLOWCOOKIE_HASH_TABLE]	= { .type = NLA_U32, },
	[FLOWCOOKIE_RULE_ADD_STATUS]	= { .type = NLA_U32, },
	[FLOWCOOKIE_RULE_DELETE_STATUS]	= { .type = NLA_U32, },
};

/*
 * operation table called by the generic netlink layer based on the command
 */
static const struct genl_ops netfn_auto_flowcookie_genl_ops[] = {
	{
		.cmd = NETFN_AUTO_FLOWCOOKIE_GENL_CMD,
		.doit = netfn_auto_flowcookie_genl_cmd,
		.validate = GENL_DONT_VALIDATE_STRICT | GENL_DONT_VALIDATE_DUMP,
		.flags = GENL_ADMIN_PERM,
	},
};

/*
 * flowcookie family definition
 */
static struct genl_family netfn_auto_flowcookie_genl_family = {
	.name           = "flowcookie",
	.version        = 1,
	.hdrsize        = 0,
	.maxattr        = NETFN_AUTO_FLOWCOOKIE_GNL_MAX,
	.policy     	= netfn_auto_flowcookie_genl_policy,
	.netnsok        = true,
	.module         = THIS_MODULE,
	.ops            = netfn_auto_flowcookie_genl_ops,
	.n_ops          = ARRAY_SIZE(netfn_auto_flowcookie_genl_ops),
};

/*
 * TODO: Support for multiple db
 * flowcookie db handle
 */
static struct netfn_flow_cookie_db *d;

/*
 * netfn_auto_flowcookie_genl_cmd()
 * 	flowcookie general cmd
 */
static inline int netfn_auto_flowcookie_genl_cmd(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg = NULL;
	void *hdr = NULL;
	char *cmd;
	int cmd_index;

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg) {
		netfn_auto_info("Failed to allocate netlink message to accomodate rule\n");
		return -ENOMEM;
	}

	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq,
			&netfn_auto_flowcookie_genl_family, 0, NETFN_AUTO_FLOWCOOKIE_GENL_CMD);

	if (!hdr) {
		netfn_auto_info("Failed to put hdr in netlink buffer\n");
		nlmsg_free(msg);
		return -ENOMEM;
	}

	if(info->attrs[CMD]) {
		cmd = nla_data(info->attrs[CMD]);
		netfn_auto_info("\nFLOWCOOKIE cmd %s\n", cmd);
		cmd_index = netfn_auto_flowcookie_get_cmd_index(cmd);

		switch(cmd_index) {
		case 0:
			netfn_auto_flowcookie_rule_add(skb, info);
			break;
		case 1:
			netfn_auto_flowcookie_rule_delete(skb, info);
			break;
		case 2:
			netfn_auto_flowcookie_rule_init(skb, info);
			break;
		case 3:
			netfn_auto_flowcookie_flow_id_get(skb, info);
			break;
		case 4:
			netfn_auto_flowcookie_flow_mark_get(skb, info);
			break;
		case 5:
			netfn_auto_flowcookie_sawf_handle_get(skb, info);
			break;
		default:
			netfn_auto_info("\nWrong cmd for flowcookie\n");
			goto error;
		}
	} else {
		netfn_auto_info("\nCommand not found for flowcookie\n");
		goto error;
	}

	genlmsg_end(msg, hdr);
	return genlmsg_unicast(genl_info_net(info), msg, info->snd_portid);
error:
	return -EINVAL;
}

/*
 * netfn_auto_flowcookie_rule_add()
 * 	parse info for rule add
 */
static inline int netfn_auto_flowcookie_rule_add(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr *attr = NULL, *flow_cookie_info = NULL;
	netfn_tuple_t tuple;
	struct netfn_flow_cookie flow_cookie;

	netfn_auto_info("\nThis is for flowcookie add\n");

	if(info->attrs[FLOWCOOKIE_INFO]) {
		int rem, flow_id, flow_mark;

		netfn_auto_info("\nFLOW_COOKIE_INFO not empty\n");

		flow_cookie_info = info->attrs[FLOWCOOKIE_INFO];
		if(!flow_cookie_info) {
			netfn_auto_info("\nWrong val for FLOW_COOKIE_INFO\n");
			return -EINVAL;
		}

		nla_for_each_nested(attr, flow_cookie_info, rem) {
			netfn_auto_info("\nattr->nla_type: %d\n", attr->nla_type);
			if(attr->nla_type == FLOWCOOKIE_FLOW_ID) {
				flow_id = nla_get_u32(attr);
				flow_cookie.flow_id = flow_id;
				netfn_auto_info("\nFrom netlink, flow_cookie_flow_id :%d\n", flow_id);
			} else if(attr->nla_type == FLOWCOOKIE_FLOW_MARK) {
				flow_mark = nla_get_u32(attr);
				flow_cookie.flow_mark = flow_mark;
				netfn_auto_info("\nFrom netlink, flow_cookie_flow_mark :%d\n", flow_mark);
			} else if(attr->nla_type == FLOWCOOKIE_SAWF_HANDLE) {
				int rem, val;
				struct nlattr *sawf_handle = NULL;
				netfn_auto_info("\nFLOWCOOKIE_SAWF_HANDLE not empty\n");
				nla_for_each_nested(sawf_handle, attr, rem) {
					netfn_auto_info("\nsawf_handle->nla_type: %d\n", sawf_handle->nla_type);

					if(sawf_handle->nla_type == FLOWCOOKIE_SAWF_ID) {
						val = nla_get_u32(sawf_handle);
						flow_cookie.sawf_handle.sawf_id = val;
						netfn_auto_info("\nFrom netlink, flow_cookie_sawf_id :%d\n", val);
					}
				}
			}
		}
	} else {
		netfn_auto_info("\nNetlink FLOW_COOKIE_INFO failed\n");
	}

	if(info->attrs[FLOWCOOKIE_TUPLE_INFO]) {
		struct nlattr *tuple_info = NULL;
		tuple_info = info->attrs[FLOWCOOKIE_TUPLE_INFO];
		netfn_auto_flowcookie_parse_tuple(&tuple, tuple_info);
	} else {
		netfn_auto_info("\nNetlink Tuple info failed\n");
	}

	netfn_flow_cookie_db_entry_add(d, &tuple, &flow_cookie);
	return 0;
}

/*
 * netfn_auto_flowcookie_rule_delete()
 * 	parse info for rule delete
 */
static inline int netfn_auto_flowcookie_rule_delete(struct sk_buff *skb, struct genl_info *info)
{
	netfn_tuple_t tuple;

	netfn_auto_info("\nThis is for flowcookie delete\n");

	if(info->attrs[FLOWCOOKIE_TUPLE_INFO]) {
		struct nlattr *tuple_info = NULL;
		tuple_info = info->attrs[FLOWCOOKIE_TUPLE_INFO];
		netfn_auto_flowcookie_parse_tuple(&tuple, tuple_info);
	} else {
		netfn_auto_info("\nNetlink Tuple info failed\n");
	}

	netfn_flow_cookie_db_entry_del(d, &tuple);
	return 0;
}

/*
 * netfn_auto_flowcookie_rule_init()
 * 	parse info for rule init
 */
static inline int netfn_auto_flowcookie_rule_init(struct sk_buff *skb, struct genl_info *info)
{
	int hash_table_size;

	netfn_auto_info("\nThis is for flowcookie init\n");

	if(info->attrs[FLOWCOOKIE_HASH_TABLE_SIZE]) {
		hash_table_size = nla_get_u32(info->attrs[FLOWCOOKIE_HASH_TABLE_SIZE]);
		netfn_auto_info("\nFrom Netlink, flow cookie hash table size: %d\n", hash_table_size);
	} else {
		netfn_auto_info("\nNetlink hash table size failed this is an updated one\n");
	}

	d = netfn_flow_cookie_db_alloc(hash_table_size);

	return 0;
}

/*
 * netfn_auto_flowcookie_flow_id_get()
 * 	parse info for flow id
 */
static inline int netfn_auto_flowcookie_flow_id_get(struct sk_buff *skb, struct genl_info *info)
{
	netfn_tuple_t tuple;
	uint32_t flow_id;

	netfn_auto_info("\nThis is for flowcookie get_flow_id\n");

	if(info->attrs[FLOWCOOKIE_TUPLE_INFO]) {
		struct nlattr *tuple_info = NULL;
		tuple_info = info->attrs[FLOWCOOKIE_TUPLE_INFO];
		netfn_auto_flowcookie_parse_tuple(&tuple, tuple_info);
	} else {
		netfn_auto_info("\nNetlink Tuple info failed\n");
	}

	flow_id = netfn_flow_cookie_db_entry_get_flow_id(d, &tuple);
	netfn_auto_info("\nFLOW_ID: %d\n", flow_id);

	return 0;
}

/*
 * netfn_auto_flowcookie_flow_mark_get()
 * 	parse info for flow mark
 */
static inline int netfn_auto_flowcookie_flow_mark_get(struct sk_buff *skb, struct genl_info *info)
{
	netfn_tuple_t tuple;
	uint32_t flow_mark;
	netfn_auto_info("\nThis is for flowcookie get_flow_mark\n");

	if(info->attrs[FLOWCOOKIE_TUPLE_INFO]) {
		struct nlattr *tuple_info = NULL;
		tuple_info = info->attrs[FLOWCOOKIE_TUPLE_INFO];
		netfn_auto_flowcookie_parse_tuple(&tuple, tuple_info);
	} else {
		netfn_auto_info("\nNetlink Tuple info failed\n");
	}

	flow_mark = netfn_flow_cookie_db_entry_get_flow_mark(d, &tuple);
	netfn_auto_info("\nFlow_mark: %d\n", flow_mark);
	return 0;
}

/*
 * netfn_auto_flowcookie_sawf_handle_get()
 * 	parse info for sawf handle
 */
static inline int netfn_auto_flowcookie_sawf_handle_get(struct sk_buff *skb, struct genl_info *info)
{
	netfn_tuple_t tuple;
	struct netfn_flow_cookie_sawf_handle netfn_auto_sawf_handle;

	netfn_auto_info("\nThis is for flowcookie get_sawf_handle\n");

	if(info->attrs[FLOWCOOKIE_TUPLE_INFO]) {
		struct nlattr *tuple_info = NULL;
		tuple_info = info->attrs[FLOWCOOKIE_TUPLE_INFO];
		netfn_auto_flowcookie_parse_tuple(&tuple, tuple_info);
	} else if(info->attrs[FLOWCOOKIE_SAWF_HANDLE]) {
		int rem, val;
		struct nlattr *sawf_handle = NULL, *attr = info->attrs[FLOWCOOKIE_SAWF_HANDLE];

		netfn_auto_info("\nFLOWCOOKIE_SAWF_HANDLE not empty\n");
		nla_for_each_nested(sawf_handle, attr, rem) {
			netfn_auto_info("\nsawf_handle->nla_type: %d\n", sawf_handle->nla_type);
			if(sawf_handle->nla_type == FLOWCOOKIE_SAWF_ID) {
				val = nla_get_u32(sawf_handle);
				netfn_auto_sawf_handle.sawf_id = val;
				netfn_auto_info("\nFrom netlink, flow_cookie_sawf_id :%d\n", val);
			}
		}

	} else {
		netfn_auto_info("\nWrong attribute in netfn_auto_flowcookie_get_sawf_handle\n");
	}

	netfn_flow_cookie_db_entry_get_sawf_handle(d, &tuple, &netfn_auto_sawf_handle);

	return 0;
}

/*
 * netfn_auto_flowcookie_init()
 *	init module
 */
static int __init netfn_auto_flowcookie_init(void)
{
	int err;
	err = genl_register_family(&netfn_auto_flowcookie_genl_family);
	if(err) {
		netfn_auto_info_always("Failed to register flowcookie generic netlink family with error: %d\n", err);
		return -EINVAL;
	} else {
		netfn_auto_info_always("Flowcookie family registration successful\n");
	}

	return 0;
}

/*
 * netfn_auto_flowcookie_exit()
 *	deinit module
 */
static void __exit netfn_auto_flowcookie_exit(void)
{

    int err;
	err = genl_unregister_family(&netfn_auto_flowcookie_genl_family);
	if(err) {
		netfn_auto_info_always("Failed to unregister flowcookie generic netlink family with error: %d\n", err);
		return;
	}

    netfn_auto_info("\nExit complete for netfn_auto flowcookie_exit\n");
	return;

}

module_init(netfn_auto_flowcookie_init);
module_exit(netfn_auto_flowcookie_exit);


MODULE_DESCRIPTION("NETFN AUTO FLOWCOOKIE");
MODULE_LICENSE("Dual BSD/GPL");
