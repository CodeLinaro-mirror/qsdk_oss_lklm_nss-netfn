/*
 * netfn_capwap_priv.h
 *	Network function's CAPWAP offload private definitions.
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#ifndef __NETFN_CAPWAP_PRIV_H
#define __NETFN_CAPWAP_PRIV_H

#define NETFN_CAPWAP_FWD_CORES_MASK 4	/* Default values for core masks */
#define NETFN_CAPWAP_CORE 8		/* Default values for core */

/*
 * netfn_capwap
 *	Driver level instance
 */
struct netfn_capwap {
	struct ctl_table_header *sysctl;/* Sysctl for configuration. */
	struct dentry *dentry;		/* Debug entry for driver. */
	spinlock_t lock;		/* Global lock. */
};
#endif /* __NETFN_CAPWAP_PRIV_H */
