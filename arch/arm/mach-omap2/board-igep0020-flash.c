/*
 * linux/arch/arm/mach-omap2/board-igep0020-flash.c
 *
 * Copyright (C) 2009 Integration Software and Electronics Engineering
 *
 * Modified from linux/arch/arm/mach-omap2/board-omap3evm-flash.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/onenand_regs.h>
#include <linux/types.h>
#include <linux/io.h>

#include <asm/mach/flash.h>

#include <mach/board-igep0020.h>
#include <mach/board.h>
#include <mach/gpmc.h>
#include <mach/nand.h>
#include <mach/onenand.h>

static int igep2_onenand_setup(void __iomem *, int freq);

static struct mtd_partition igep2_onenand_partitions[] = {
	{
		.name           = "X-Loader",
		.offset         = 0,
		.size           = 4*(64*2048),
		.mask_flags     = MTD_WRITEABLE
	},
	{
		.name           = "U-Boot",
		.offset         = MTDPART_OFS_APPEND,
		.size           =  15*(64*2048),
		.mask_flags     = MTD_WRITEABLE
	},
	{
		.name           = "U-Boot Env",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 1*(64*2048),
	},
	{
		.name           = "Kernel",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 40*(64*2048),
	},
	{
		.name           = "File System",
		.offset         = MTDPART_OFS_APPEND,
		.size           = MTDPART_SIZ_FULL,
	},
};

static struct omap_onenand_platform_data igep2_onenand_data = {
	.parts = igep2_onenand_partitions,
	.nr_parts = ARRAY_SIZE(igep2_onenand_partitions),
	.onenand_setup = igep2_onenand_setup,
	.dma_channel	= -1,	/* disable DMA in OMAP OneNAND driver */
};

static struct platform_device igep2_onenand_device = {
	.name		= "omap2-onenand",
	.id		= -1,
	.dev = {
		.platform_data = &igep2_onenand_data,
	},
};

static struct mtd_partition igep2_nand_partitions[] = {
	/* All the partition sizes are listed in terms of NAND block size */
	{
		.name		= "xloader-nand",
		.offset		= 0,
		.size		= 4*(128 * 1024),
		.mask_flags	= MTD_WRITEABLE
	},
	{
		.name		= "uboot-nand",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 14*(128 * 1024),
		.mask_flags	= MTD_WRITEABLE
	},
	{
		.name		= "params-nand",

		.offset		= MTDPART_OFS_APPEND,
		.size		= 2*(128 * 1024)
	},
	{
		.name		= "linux-nand",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 40*(128 * 1024)
	},
	{
		.name		= "jffs2-nand",
		.size		= MTDPART_SIZ_FULL,
		.offset		= MTDPART_OFS_APPEND,
	},
};

static struct omap_nand_platform_data igep2_nand_data = {
	.parts		= igep2_nand_partitions,
	.nr_parts	= ARRAY_SIZE(igep2_nand_partitions),
	.nand_setup	= NULL,
	.dma_channel	= -1,		/* disable DMA in OMAP NAND driver */
	.dev_ready	= NULL,
};

static struct resource igep2_nand_resource = {
	.flags		= IORESOURCE_MEM,
};

static struct platform_device igep2_nand_device = {
	.name		= "omap2-nand",
	.id		= 0,
	.dev		= {
		.platform_data	= &igep2_nand_data,
	},
	.num_resources	= 1,
	.resource	= &igep2_nand_resource,
};

/*
 *      igep2_onenand_setup - Set the onenand sync mode
 *      @onenand_base:  The onenand base address in GPMC memory map
 *
 */

static int igep2_onenand_setup(void __iomem *onenand_base, int freq)
{
	/* nothing is required to be setup for onenand as of now */
	return 0;
}

void __init igep2_flash_init(void)
{
	u8		cs = 0;
	u8		onenandcs = GPMC_CS_NUM + 1, nandcs = GPMC_CS_NUM + 1;
	u32		gpmc_base_add = OMAP34XX_GPMC_VIRT;

	while (cs < GPMC_CS_NUM) {
		u32 ret = 0;
		ret = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG1);

		/*
		* xloader/Uboot would have programmed the NAND/oneNAND
		* base address for us This is a ugly hack. The proper
		* way of doing this is to pass the setup of u-boot up
		* to kernel using kernel params - something on the
		* lines of machineID. Check if NAND/oneNAND is configured
		*/
		if ((ret & 0xC00) == 0x800) {
			/* Found it!! */
			if (nandcs > GPMC_CS_NUM)
				nandcs = cs;
		} else {
			ret = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
			if ((ret & 0x3F) == (ONENAND_MAP >> 24))
				onenandcs = cs;
		}
		cs++;
	}
	if ((nandcs > GPMC_CS_NUM) && (onenandcs > GPMC_CS_NUM)) {
		printk(KERN_INFO "NAND/OneNAND: Unable to find configuration "
				" in GPMC\n ");
		return;
	}

	if (nandcs < GPMC_CS_NUM) {
		igep2_nand_data.cs	= nandcs;
		igep2_nand_data.gpmc_cs_baseaddr = (void *)(gpmc_base_add +
					GPMC_CS0_BASE + nandcs*GPMC_CS_SIZE);
		igep2_nand_data.gpmc_baseaddr   = (void *) (gpmc_base_add);

		if (platform_device_register(&igep2_nand_device) < 0) {
			printk(KERN_ERR "Unable to register NAND device\n");
		}
	}

	if (onenandcs < GPMC_CS_NUM) {
		igep2_onenand_data.cs = onenandcs;
		if (platform_device_register(&igep2_onenand_device) < 0)
			printk(KERN_ERR "Unable to register OneNAND device\n");
	}
}

