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

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/if.h>
#include <linux/list.h>
#include <linux/completion.h>

#include <ppe_drv.h>
#include <ppe_drv_eip.h>

#include "netfn_dtls_priv.h"

/*
 * global dtls instance.
 */
struct netfn_dtls_drv g_dtls_drv;


/*
 * netfn_dtls_drv_final()
 *	Mark final deref completion.
 */
void netfn_dtls_drv_final(struct kref *kref)
{
	struct netfn_dtls_drv *drv = container_of(kref, struct netfn_dtls_drv, ref);

	complete(&drv->completion);
}

/*
 * netfn_dtls_init_module()
 *	Module initialization
 */
int __init netfn_dtls_init_module(void)
{
	struct netfn_dtls_drv *drv = &g_dtls_drv;
	enum eip_svc svc = EIP_SVC_DTLS;
	int err = 0;

	/*
	 * Check if DMA is loaded but not enabled.
	 */
	if (!eip_is_enabled()) {
		pr_info("EIP Module can't be loaded as EIP is unavailable\n");
		return -1;
	}

	drv->is_inline = eip_is_inline_supported();

	/*
	 * Assign the EIP service code
	 */
	if (drv->is_inline) {
		svc = EIP_SVC_HYBRID_DTLS;

		/*
		 * Map DTLS service codes (PPE2EIP and EIP2PPE) to the EIP197
		 * unicast queue. This directs PPE to steer DTLS flows to EIP197
		 * for inline crypto offload.
		 */
		ppe_drv_eip_sc_queue_config_set_dtls();
	}

	/*
	 * Initialize the global object.
	 * Dereference: netfn_dtls_exit_module()
	 */
	init_completion(&drv->completion);

	drv->session_cache = kmem_cache_create("netfn_dtls_session", sizeof(struct netfn_dtls_session), 0, 0, NULL);
	if (!drv->session_cache) {
		pr_err("%px: Failed to allocate kmem cache for Session\n", drv);
		return -ENOMEM;
	}

	/*
	 * Create DMA context.
	 */
	drv->ctx = eip_ctx_alloc(svc, &drv->dentry);
	if (!drv->ctx) {
		pr_err("%px: Failed to create DMA context\n", drv);
		err = -ENOSYS;
		goto ctx_fail;
	}

	/*
	 * TODO: PPE specific initialization?.
	 */

	kref_init(&drv->ref);
	pr_info("NETFN dtls module loaded %s\n", NSS_NETFN_BUILD_ID);

	return 0;

ctx_fail:
	kmem_cache_destroy(drv->session_cache);
	return err;
}

/*
 * netfn_dtls_exit_module()
 *	Module exit cleanup
 */
void __exit netfn_dtls_exit_module(void)
{
	struct netfn_dtls_drv *drv = &g_dtls_drv;

	/*
	 * Dereference: netfn_dtls_init_module()
	 */
	netfn_dtls_drv_deref(drv);

	/*
	 * Wait for all deref (netfn_dtls_drv_final).
	 * Drv reference is usally taken by device allocation.
	 */
	wait_for_completion(&drv->completion);

	eip_ctx_free(drv->ctx);
	drv->ctx = NULL;
	kmem_cache_destroy(drv->session_cache);
	pr_info("NETFN dtls module unloaded %s\n", NSS_NETFN_BUILD_ID);
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NSS NETFN DTLS client");

module_init(netfn_dtls_init_module);
module_exit(netfn_dtls_exit_module);
