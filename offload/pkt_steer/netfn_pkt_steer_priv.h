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

#ifndef __NETFN_PKT_STEER_PRIV_H
#define __NETFN_PKT_STEER_PRIV_H

#include <linux/debugfs.h>
#include <linux/netdevice.h>
#include <netfn_pkt_steer.h>

#define NETFN_PKT_STEER_FIFO_MAX 512
#define NETFN_PKT_STEER_ROUND_UP(x) (1U << fls(x))	/* Round up to nearest sqaure */
#define NETFN_PKT_STEER_FIFO_SZ(budget) NETFN_PKT_STEER_ROUND_UP((budget) * 2)	/* handling more than 2x burst is overkill */
#define NETFN_PKT_STEER_FIFO_INC(i, sz) (((i) + 1) & ((sz) - 1))

#define NETFN_PKT_STEER_IDX_SUB(i, val, max) (((i) - (val) + (max)) & ((max) - 1))
#define NETFN_PKT_STEER_AVAIL_COUNT(head, tail, max) \
	NETFN_PKT_STEER_IDX_SUB(head, tail, max)

/*
 * netfn_pkt_steer_fifo
 * 	skb queue for napi operations
 */
struct netfn_pkt_steer_fifo {
	struct sk_buff **skbs;			/* SKB FIFO */
	size_t size;				/* FIFO size in square of 2 */
	atomic64_t prod;			/* Produce index */
	atomic64_t cons;			/* Consumer index */

	struct u64_stats_sync syncp;		/* sync for stats */
	struct netfn_pkt_steer_stats stats;	/* statistics */
};

/*
 * netfn_pkt_steer_pcpu
 * 	Per Core NAPI structure.
 */
struct netfn_pkt_steer_pcpu {
	struct napi_struct napi;		/* NAPI object */
	netfn_pkt_steer_recv_t cb;		/* Receive calback */
	void *app_data;				/* Callback appdata */
	int core_id;				/* Core ID associated with NAPI */
	call_single_data_t csd;			/* IPI descriptor */

	struct netfn_pkt_steer_fifo fifo[NR_CPUS];	/* Cross CPU SKB Queue */
};

/*
 * netfn_pkt_steer
 * 	Per Core NAPI structure.
 */
struct netfn_pkt_steer {
	struct net_device ndev;			/* Netdevice for NAPI */
	struct netfn_pkt_steer_pcpu __percpu *pcpu;	/* Per CPU NAPI entry */
};

#endif /* __NETFN_PKT_STEER_PRIV_H */
