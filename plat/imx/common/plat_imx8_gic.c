/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bl_common.h>
#include <gicv3.h>
#include <interrupt_props.h>
#include <plat_imx8.h>
#include <platform.h>
#include <platform_def.h>
#include <utils.h>
#include <debug.h>
#include <mmio.h>
#include <gic_common.h>

/* the GICv3 driver only needs to be initialized in EL3 */
uintptr_t rdistif_base_addrs[PLATFORM_GIC_CORE_COUNT];

static const interrupt_prop_t g01s_interrupt_props[] = {
	INTR_PROP_DESC(8, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP0, GIC_INTR_CFG_LEVEL),
};

static unsigned int plat_imx_mpidr_to_core_pos(unsigned long mpidr)
{
#if (defined ECOCKPIT_A53) || (defined ECOCKPIT_A72)
	return (unsigned int)plat_gic_core_pos_by_mpidr(mpidr);
#else
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
#endif
}

const gicv3_driver_data_t arm_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.interrupt_props = g01s_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(g01s_interrupt_props),
	.rdistif_num = PLATFORM_GIC_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.mpidr_to_core_pos = plat_imx_mpidr_to_core_pos,
};

void plat_gic_driver_init(void)
{
	/*
	 * the GICv3 driver is initialized in EL3 and does not need
	 * to be initialized again in S-EL1. This is because the S-EL1
	 * can use GIC system registers to manage interrupts and does
	 * not need GIC interface base addresses to be configured.
	 */
#if IMAGE_BL31
	gicv3_driver_init(&arm_gic_data);
#endif
}

void plat_gic_init(void)
{
#if (defined ECOCKPIT_A53) || (defined ECOCKPIT_A72)
	/* check if GICD already configured, in case of 2nd partition (re)boot */
	uint32_t gicd_ctlr = mmio_read_32(PLAT_GICD_BASE + GICD_CTLR);

	if (gicd_ctlr & (CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1S_BIT | CTLR_ENABLE_G1NS_BIT)) {
		NOTICE("GIC Distributor already configured: skip gicv3_distif_init \n");
	} else {
		gicv3_distif_init();
		NOTICE("plat_gic_init: gicv3_distif_init done\n");
	}
#else
	gicv3_distif_init();
#endif
	gicv3_rdistif_init(plat_get_core_pos());
	gicv3_cpuif_enable(plat_get_core_pos());
}

void plat_gic_cpuif_enable(void)
{
	gicv3_cpuif_enable(plat_get_core_pos());
}

void plat_gic_cpuif_disable(void)
{
	gicv3_cpuif_disable(plat_get_core_pos());
}

void plat_gic_pcpu_init(void)
{
	gicv3_rdistif_init(plat_get_core_pos());
}

void plat_gic_save(unsigned int proc_num, struct plat_gic_ctx *ctx)
{
	/* save the gic rdist/dist context */
	for (int i = 0; i < PLATFORM_GIC_CORE_COUNT; i++)
		gicv3_rdistif_save(i, &ctx->rdist_ctx[i]);
	gicv3_distif_save(&ctx->dist_ctx);
}

void plat_gic_restore(unsigned int proc_num, struct plat_gic_ctx *ctx)
{
	/* restore the gic rdist/dist context */
	gicv3_distif_init_restore(&ctx->dist_ctx);
	for (int i = 0; i < PLATFORM_GIC_CORE_COUNT; i++)
		gicv3_rdistif_init_restore(i, &ctx->rdist_ctx[i]);
}
