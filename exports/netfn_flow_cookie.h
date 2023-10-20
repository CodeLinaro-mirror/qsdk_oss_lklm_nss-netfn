/*
 *******************************************************************************
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
 *******************************************************************************
 */

#include <linux/types.h>
#include <linux/module.h>
#include <netfn_types.h>

#ifndef __NETFN_FLOW_COOKIE_EXPORT_H
#define __NETFN_FLOW_COOKIE_EXPORT_H

#define NETFN_FLOW_COOKIE_SAWF_VALID 0x0000001
#define NETFN_FLOW_COOKIE_SCS_VALID 0x00000002
#define NETFN_FLOW_COOKIE_FLOW_MARK_VALID 0x00000004
#define NETFN_FLOW_COOKIE_FLOW_ID_VALID 0x00000008

struct netfn_flow_cookie_db;			/**< DataBase Structure */

/**
 * enum netfn_flow_cookie_status
 *	netfn flow cookie return status
 */
typedef enum netfn_flow_cookie_status {
	NETFN_FLOW_COOKIE_STATUS_SUCCESS = 0,      /**< Success */
	NETFN_FLOW_COOKIE_STATUS_INVALID_TYPE,     /**< Invalid */
	NETFN_FLOW_COOKIE_STATUS_NETFN_ALLOC_FAIL, /**< Netfn ALLOC FAIL */
	NETFN_FLOW_COOKIE_STATUS_FAILURE,          /**< Flow Cookie Failure */
	NETFN_FLOW_COOKIE_STATUS_MAX,              /**< Status of Max value */
} netfn_flow_cookie_status_t;

/**
 * enum netfn_flow_cookie_invalid_values
 *	Pass the values to be 0s
 *	for the failed flow cookie lookup scenarios.
 */
typedef enum netfn_flow_cookie_invalid_values {
	NETFN_FLOW_COOKIE_INVALID_FLOW_ID = 0,		/**< Invalid Flow ID */
	NETFN_FLOW_COOKIE_INVALID_SAWF_HANDLE = 0,	/**< Invalid Sawf Handle */
	NETFN_FLOW_COOKIE_INVALID_FLOW_MARK = 0,	/**< Invalid Flow Mark */
	NETFN_FLOW_COOKIE_INVALID_SCS_ID = 0,		/**< Invalid SCS ID */
} netfn_flow_cookie_invalid_values_t;

/**
 * netfn_flow_cookie_sawf_handle
 *	Sawf handle structure.
 *	Can modify as per the requirement.
 */
struct netfn_flow_cookie_sawf_handle {
	uint32_t sawf_id;		/**< Sawf Handle ID */
};

/**
 * netfn_flow_cookie structure.
 *	Consists of Flow ID, Sawf Handle and Flow Mark.
 */
struct netfn_flow_cookie {
	uint32_t valid_flag;					/**< Flag tracking cookie params */
	uint32_t flow_id;					/**< Flow ID */
	struct netfn_flow_cookie_sawf_handle sawf_handle;	/**< Sawf Handle */
	uint32_t flow_mark;					/**< Flow Mark */
	uint32_t scs_id;					/**< SCS ID */
};

/**
 * netfn_flow_cookie_db_alloc
 *	Following API is used to initialize the
 *	Flow DB basing on the no.of entries as
 *	passed from the DB init caller.
 *
 * @param[in] num_entries	32 bit Hash Table size
 *
 * @return
 * netfn_flow_cookie_db handle instantiated.
 *
 */
extern struct netfn_flow_cookie_db *netfn_flow_cookie_db_alloc(uint32_t num_entries);

/**
 * netfn_flow_cookie_db_instance_ref
 *	Following API is used to increase the
 *	references of the netfn_flow_cookie_db instance provided
 *	to ensure that the DB is protected from parallel accessing.
 *
 * @param[in] d         netfn_flow_cookie_db handle to be referenced
 *
 * @return
 * void
 *
 */
void netfn_flow_cookie_db_instance_ref(struct netfn_flow_cookie_db *d);

/**
 * netfn_flow_cookie_db_instance_deref
 *	Following API is used to free the
 *	Flow DB instance which has been instantiated post it's usage.
 *
 * @param[in] d         netfn_flow_cookie_db handle to be released
 *
 * @return
 * bool
 *
 */
extern bool netfn_flow_cookie_db_instance_deref(struct netfn_flow_cookie_db *d);

/*
 * netfn_flow_cookie_db_entry_add
 *	Following API is used to add the
 *	appropriate DB entry corresponding to the
 *	5 tuple info of the flow into the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t		Pointer to 5 tuple info
 * @param[in] cookie	Pointer to flow cookie info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_add(struct netfn_flow_cookie_db *d, struct netfn_tuple *t, struct netfn_flow_cookie *cookie);

/**
 * netfn_flow_cookie_db_entry_del
 *	Following API is used to delete the
 *	appropriate DB entry corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t		Pointer to 5 tuple info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_del(struct netfn_flow_cookie_db *d, struct netfn_tuple *t);

/**
 * netfn_flow_cookie_db_entry_get_flow_id
 *	Following API is used to fetch the
 *	Flow ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t		Pointer to 5 tuple info
 *
 * @return
 * 32 bit Flow ID.
 * Flow ID 0 refers to flow db entry not found.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_flow_id(struct netfn_flow_cookie_db *d,  struct netfn_tuple *t);

/**
 * netfn_flow_cookie_db_entry_get_flow_id_using_skb
 *	Following API is used to fetch the
 *	Flow ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		 netfn_flow_cookie_db handle
 * @param[in] skb	 Pointer to skb
 * @param[in] offset	 Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit Flow ID.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_flow_id_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset);

/**
 * netfn_flow_cookie_db_entry_get_sawf_handle
 *	Following API is used to fetch the
 *	SAWF handle corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d			 netfn_flow_cookie_db handle
 * @param[in] t		 	 Pointer to 5 tuple info
 * @param[in] sawf_handle	 Pointer to Sawf Handle to be filled.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_sawf_handle(struct netfn_flow_cookie_db *d, struct netfn_tuple *t, struct netfn_flow_cookie_sawf_handle *sawf_handle);

/**
 * netfn_flow_cookie_db_entry_get_sawf_handle_using_skb
 *	Following API is used to fetch the
 *	SAWF handle corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d			netfn_flow_cookie_db handle
 * @param[in] skb		Pointer to skb
 * @param[in] offset		Offset to extract the 5 tuple info.
 * @param[in] sawf_handle	Pointer to Sawf Handle to be filled.
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_sawf_handle_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset,
										       struct netfn_flow_cookie_sawf_handle *sawf_handle);

/**
 * netfn_flow_cookie_db_entry_get_flow_mark
 *	Following API is used to fetch the
 *	Flow Mark corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t		Pointer to 5 tuple info
 *
 * @return
 * 32 bit Flow Mark.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_flow_mark(struct netfn_flow_cookie_db *d, struct netfn_tuple *t);

/**
 * netfn_flow_cookie_db_entry_get_flow_mark_using_skb
 *	Following API is used to fetch the
 *	Flow Mark corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit Flow Mark.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_flow_mark_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset);

/**
 * netfn_flow_cookie_db_entry_get_scs_id
 *	Following API is used to fetch the
 *	SCS ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t		Pointer to 5 tuple info
 *
 * @return
 * 32 bit SCS ID.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_scs_id(struct netfn_flow_cookie_db *d, struct netfn_tuple *t);

/**
 * netfn_flow_cookie_db_entry_get_scs_id_using_skb
 *	Following API is used to fetch the
 *	SCS ID corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 *
 * @return
 * 32 bit SCS ID.
 *
 */
extern uint32_t netfn_flow_cookie_db_entry_get_scs_id_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb, uint32_t offset);

/**
 * netfn_flow_cookie_db_entry_get_flow_cookie
 *	Following API is used to fetch the entire
 *	Flow Cookie corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d		netfn_flow_cookie_db handle
 * @param[in] t         Pointer to 5 tuple info
 * @param[in] cookie    Pointer to flow cookie info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_flow_cookie(struct netfn_flow_cookie_db *d, struct netfn_tuple *t,
									     struct netfn_flow_cookie *flow_cookie);

/**
 * netfn_flow_cookie_db_entry_get_flow_cookie_using_skb
 *	Following API is used to fetch the entire
 *	Flow Cookie corresponding to the
 *	5 tuple info of the flow from the Hash Table.
 *
 * @param[in] d         netfn_flow_cookie_db handle
 * @param[in] skb       Pointer to skb
 * @param[in] offset    Offset to extract the 5 tuple info.
 * @param[in] cookie    Pointer to flow cookie info
 *
 * @return
 * netfn_flow_cookie_status_t
 *
 */
extern netfn_flow_cookie_status_t netfn_flow_cookie_db_entry_get_flow_cookie_using_skb(struct netfn_flow_cookie_db *d, struct sk_buff *skb,
										       uint32_t offset, struct netfn_flow_cookie *flow_cookie);

#endif		/* __NETFN_FLOW_COOKIE_EXPORT_H */
