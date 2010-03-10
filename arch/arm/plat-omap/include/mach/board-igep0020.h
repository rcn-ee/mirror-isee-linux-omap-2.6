/*
 * arch/arm/plat-omap/include/mach/board-igep0020.h
 *
 * Copyright (C) 2009 Integration Software and Electronics Engineering
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

#ifndef __ASM_ARCH_IGEP0020_H
#define __ASM_ARCH_IGEP0020_H

extern void igep2_flash_init(void);

#define ONENAND_MAP		0x20000000

#define IGEP2_SMSC911X_CS	5
#define IGEP2_SMSC911X_GPIO	176

/*  HIGH : It is a rev. B or C (B-compatible) board
    LOW  : It is a rev. C (B-NON compatible) board */
#define IGEP2_GPIO_HW_REV	28

/* Platform: IGEP v2 Hw Rev. B / C (B-compatible) */
#define IGEP2_RB_GPIO_WIFI_NPD 		94
#define IGEP2_RB_GPIO_WIFI_NRESET 	95
#define IGEP2_RB_GPIO_BT_NRESET 	137
/* Platform: IGEP v2 Hw Rev. C (B-non compatible) */
#define IGEP2_RC_GPIO_WIFI_NPD 		138
#define IGEP2_RC_GPIO_WIFI_NRESET 	139
#define IGEP2_RC_GPIO_BT_NRESET 	137

#define IGEP2_GPIO_LED_GREEN 	26
#define IGEP2_GPIO_LED_RED 	27

#define IGEP2_GPIO_EXT_PHY_USB	24

#define IGEP2_THS14F0X_CS	6
#define IGEP2_THS14F0X_GPIO	100

#endif /* __ASM_ARCH_IGEP0020_H */

