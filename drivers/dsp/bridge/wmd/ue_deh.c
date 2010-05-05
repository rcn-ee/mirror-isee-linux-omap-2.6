/*
 * ue_deh.c
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Implements upper edge DSP exception handling (DEH) functions.
 *
 * Copyright (C) 2005-2006 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*  ----------------------------------- Host OS */
#include <dspbridge/host_os.h>
#include <plat/dmtimer.h>

/*  ----------------------------------- DSP/BIOS Bridge */
#include <dspbridge/std.h>
#include <dspbridge/dbdefs.h>
#include <dspbridge/errbase.h>

/*  ----------------------------------- Trace & Debug */
#include <dspbridge/dbc.h>

/*  ----------------------------------- OS Adaptation Layer */
#include <dspbridge/cfg.h>
#include <dspbridge/ntfy.h>
#include <dspbridge/drv.h>

/*  ----------------------------------- Link Driver */
#include <dspbridge/wmddeh.h>

/*  ----------------------------------- Platform Manager */
#include <dspbridge/dev.h>
#include <dspbridge/wcd.h>
#include <dspbridge/wdt.h>

/* ------------------------------------ Hardware Abstraction Layer */
#include <hw_defs.h>
#include <hw_mmu.h>

/*  ----------------------------------- This */
#include "mmu_fault.h"
#include "_tiomap.h"
#include "_deh.h"
#include "_tiomap_mmu.h"
#include "_tiomap_pwr.h"
#include <dspbridge/io_sm.h>

#define ALIGN_DOWN(x, a)  ((x)&(~((a)-1)))

/* GP Timer number to trigger interrupt for MMU-fault ISR on DSP */
#define GPTIMER_FOR_DSP_MMU_FAULT      8
/* Bit mask to enable overflow interrupt */
#define GPTIMER_IRQ_OVERFLOW           2
/* Max time to check for GP Timer IRQ */
#define GPTIMER_IRQ_WAIT_MAX_CNT       1000

static struct hw_mmu_map_attrs_t map_attrs = { HW_LITTLE_ENDIAN,
	HW_ELEM_SIZE16BIT,
	HW_MMU_CPUES
};

static void *dummy_va_addr;

static struct omap_dm_timer *timer;

dsp_status bridge_deh_create(struct deh_mgr **ret_deh_mgr,
		struct dev_object *hdev_obj)
{
	dsp_status status = DSP_SOK;
	struct deh_mgr *deh_mgr;
	struct wmd_dev_context *hwmd_context = NULL;

	/*  Message manager will be created when a file is loaded, since
	 *  size of message buffer in shared memory is configurable in
	 *  the base image. */
	/* Get WMD context info. */
	dev_get_wmd_context(hdev_obj, &hwmd_context);
	DBC_ASSERT(hwmd_context);
	dummy_va_addr = NULL;
	/* Allocate IO manager object: */
	deh_mgr = kzalloc(sizeof(struct deh_mgr), GFP_KERNEL);
	if (!deh_mgr) {
		status = -ENOMEM;
		goto leave;
	}

	/* Create an NTFY object to manage notifications */
	deh_mgr->ntfy_obj = kmalloc(sizeof(struct ntfy_object), GFP_KERNEL);
	if (deh_mgr->ntfy_obj) {
		ntfy_init(deh_mgr->ntfy_obj);
	} else {
		status = -ENOMEM;
		goto err;
	}

	/* Create a MMUfault DPC */
	tasklet_init(&deh_mgr->dpc_tasklet, mmu_fault_dpc, (u32) deh_mgr);

	/* Fill in context structure */
	deh_mgr->hwmd_context = hwmd_context;
	deh_mgr->err_info.dw_err_mask = 0L;
	deh_mgr->err_info.dw_val1 = 0L;
	deh_mgr->err_info.dw_val2 = 0L;
	deh_mgr->err_info.dw_val3 = 0L;

	/* Install ISR function for DSP MMU fault */
	if ((request_irq(INT_DSP_MMU_IRQ, mmu_fault_isr, 0,
					"DspBridge\tiommu fault",
					(void *)deh_mgr)) == 0)
		status = DSP_SOK;
	else
		status = -EPERM;

err:
	if (DSP_FAILED(status)) {
		/* If create failed, cleanup */
		bridge_deh_destroy(deh_mgr);
		deh_mgr = NULL;
	} else {
		timer = omap_dm_timer_request_specific(
					GPTIMER_FOR_DSP_MMU_FAULT);
		if (timer) {
			omap_dm_timer_disable(timer);
		} else {
			pr_err("%s: GPTimer not available\n", __func__);
			return -ENODEV;
		}
	}

leave:
	*ret_deh_mgr = deh_mgr;

	return status;
}

dsp_status bridge_deh_destroy(struct deh_mgr *deh_mgr)
{
	if (!deh_mgr)
		return -EFAULT;

	/* Release dummy VA buffer */
	bridge_deh_release_dummy_mem();
	/* If notification object exists, delete it */
	if (deh_mgr->ntfy_obj) {
		ntfy_delete(deh_mgr->ntfy_obj);
		kfree(deh_mgr->ntfy_obj);
	}
	/* Disable DSP MMU fault */
	free_irq(INT_DSP_MMU_IRQ, deh_mgr);

	/* Free DPC object */
	tasklet_kill(&deh_mgr->dpc_tasklet);

	/* Deallocate the DEH manager object */
	kfree(deh_mgr);

	/* The GPTimer is no longer needed */
	omap_dm_timer_free(timer);
	timer = NULL;

	return DSP_SOK;
}

dsp_status bridge_deh_register_notify(struct deh_mgr *deh_mgr, u32 event_mask,
		u32 notify_type,
		struct dsp_notification *hnotification)
{
	dsp_status status = DSP_SOK;

	if (!deh_mgr)
		return -EFAULT;

	if (event_mask)
		status = ntfy_register(deh_mgr->ntfy_obj, hnotification,
					event_mask, notify_type);
	else
		status = ntfy_unregister(deh_mgr->ntfy_obj, hnotification);

	return status;
}

void bridge_deh_notify(struct deh_mgr *deh_mgr, u32 ulEventMask, u32 dwErrInfo)
{
	struct wmd_dev_context *dev_context;
	dsp_status status = DSP_SOK;
	u32 mem_physical = 0;
	u32 hw_mmu_max_tlb_count = 31;
	extern u32 fault_addr;
	struct cfg_hostres *resources;
	hw_status hw_status_obj;
	u32 cnt = 0;

	if (!deh_mgr)
		return;

	dev_info(bridge, "%s: device exception\n", __func__);
	dev_context = (struct wmd_dev_context *)deh_mgr->hwmd_context;
	resources = dev_context->resources;

	switch (ulEventMask) {
	case DSP_SYSERROR:
		/* reset err_info structure before use */
		deh_mgr->err_info.dw_err_mask = DSP_SYSERROR;
		deh_mgr->err_info.dw_val1 = 0L;
		deh_mgr->err_info.dw_val2 = 0L;
		deh_mgr->err_info.dw_val3 = 0L;
		deh_mgr->err_info.dw_val1 = dwErrInfo;
		dev_err(bridge, "%s: %s, err_info = 0x%x\n",
				__func__, "DSP_SYSERROR", dwErrInfo);
		dump_dl_modules(dev_context);
		dump_dsp_stack(dev_context);
		break;
	case DSP_MMUFAULT:
		/* MMU fault routine should have set err info structure. */
		deh_mgr->err_info.dw_err_mask = DSP_MMUFAULT;
		dev_err(bridge, "%s: %s, err_info = 0x%x\n",
				__func__, "DSP_MMUFAULT", dwErrInfo);
		dev_info(bridge, "%s: %s, high=0x%x, low=0x%x, "
			"fault=0x%x\n", __func__, "DSP_MMUFAULT",
			(unsigned int) deh_mgr->err_info.dw_val1,
			(unsigned int) deh_mgr->err_info.dw_val2,
			(unsigned int) fault_addr);
		dummy_va_addr = kzalloc(sizeof(char) * 0x1000, GFP_ATOMIC);
		mem_physical =
			ALIGN_DOWN(virt_to_phys(dummy_va_addr), PAGE_SIZE);
		dev_context = (struct wmd_dev_context *)
			deh_mgr->hwmd_context;

		print_dsp_trace_buffer(dev_context);
		dump_dl_modules(dev_context);

		/*
		 * Reset the dynamic mmu index to fixed count if it exceeds
		 * 31. So that the dynmmuindex is always between the range of
		 * standard/fixed entries and 31.
		 */
		if (dev_context->num_tlb_entries >
				hw_mmu_max_tlb_count) {
			dev_context->num_tlb_entries =
				dev_context->fixed_tlb_entries;
		}
		if (DSP_SUCCEEDED(status)) {
			hw_status_obj =
				hw_mmu_tlb_add(resources->dw_dmmu_base,
						mem_physical, fault_addr,
						HW_PAGE_SIZE4KB, 1,
						&map_attrs, HW_SET, HW_SET);
		}

		/*
		 * Send a GP Timer interrupt to DSP.
		 * The DSP expects a GP timer interrupt after an
		 * MMU-Fault Request GPTimer.
		 */
		if (timer) {
			omap_dm_timer_enable(timer);
			/* Enable overflow interrupt */
			omap_dm_timer_set_int_enable(timer,
						     GPTIMER_IRQ_OVERFLOW);
			/*
			 * Set counter value to overflow counter after
			 * one tick and start timer.
			 */
			omap_dm_timer_set_load_start(timer, 0, 0xfffffffe);

			/* Wait 80us for timer to overflow */
			udelay(80);

			/* Check interrupt status and wait for interrupt */
			cnt = 0;
			while (!(omap_dm_timer_read_status(timer) &
				GPTIMER_IRQ_OVERFLOW)) {
				if (cnt++ >= GPTIMER_IRQ_WAIT_MAX_CNT) {
					pr_err("%s: GPTimer interrupt failed\n",
								__func__);
					break;
				}
			}
		}

		/* Clear MMU interrupt */
		hw_mmu_event_ack(resources->dw_dmmu_base,
				HW_MMU_TRANSLATION_FAULT);
		dump_dsp_stack(deh_mgr->hwmd_context);
		omap_dm_timer_disable(timer);
		break;
#ifdef CONFIG_BRIDGE_NTFY_PWRERR
	case DSP_PWRERROR:
		/* reset err_info structure before use */
		deh_mgr->err_info.dw_err_mask = DSP_PWRERROR;
		deh_mgr->err_info.dw_val1 = 0L;
		deh_mgr->err_info.dw_val2 = 0L;
		deh_mgr->err_info.dw_val3 = 0L;
		deh_mgr->err_info.dw_val1 = dwErrInfo;
		dev_err(bridge, "%s: %s, err_info = 0x%x\n",
				__func__, "DSP_PWRERROR", dwErrInfo);
		break;
#endif /* CONFIG_BRIDGE_NTFY_PWRERR */
	case DSP_WDTOVERFLOW:
		deh_mgr->err_info.dw_err_mask = DSP_WDTOVERFLOW;
		deh_mgr->err_info.dw_val1 = 0L;
		deh_mgr->err_info.dw_val2 = 0L;
		deh_mgr->err_info.dw_val3 = 0L;
		dev_err(bridge, "%s: DSP_WDTOVERFLOW\n", __func__);
		break;
	default:
		dev_dbg(bridge, "%s: Unknown Error, err_info = 0x%x\n",
				__func__, dwErrInfo);
		break;
	}

	/* Filter subsequent notifications when an error occurs */
	if (dev_context->dw_brd_state != BRD_ERROR) {
		ntfy_notify(deh_mgr->ntfy_obj, ulEventMask);
#ifdef CONFIG_BRIDGE_RECOVERY
		bridge_recover_schedule();
#endif
	}

	/* Set the Board state as ERROR */
	dev_context->dw_brd_state = BRD_ERROR;
	/* Disable all the clocks that were enabled by DSP */
	dsp_peripheral_clocks_disable(dev_context, NULL);
	/*
	 * Avoid the subsequent WDT if it happens once,
	 * also if fatal error occurs.
	 */
	dsp_wdt_enable(false);
}

dsp_status bridge_deh_get_info(struct deh_mgr *deh_mgr,
		struct dsp_errorinfo *pErrInfo)
{
	DBC_REQUIRE(deh_mgr);
	DBC_REQUIRE(pErrInfo);

	if (!deh_mgr)
		return -EFAULT;

	/* Copy DEH error info structure to PROC error info structure. */
	pErrInfo->dw_err_mask = deh_mgr->err_info.dw_err_mask;
	pErrInfo->dw_val1 = deh_mgr->err_info.dw_val1;
	pErrInfo->dw_val2 = deh_mgr->err_info.dw_val2;
	pErrInfo->dw_val3 = deh_mgr->err_info.dw_val3;

	return DSP_SOK;
}

void bridge_deh_release_dummy_mem(void)
{
	kfree(dummy_va_addr);
	dummy_va_addr = NULL;
}
