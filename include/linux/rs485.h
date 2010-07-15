/*
 * include/linux/rs485.h
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


#ifndef LINUX_SERIAL_RS485_H
#define LINUX_SERIAL_RS485_H

/* RS-485 support */
struct serial_rs485_settings {
	unsigned short flags;			/* RS485 feature flags */
#define SER_RS485_MODE			(3 << 0) /* Mask for mode. */
#define SER_RS485_MODE_DISABLED		(0 << 0)
#define SER_RS485_MODE_RTS		(1 << 0)
#define SER_RS485_MODE_DTR		(2 << 0)
#define SER_RS485_RTS_TX_LOW		(1 << 2) /* Inverted RTS */
#define SER_RS485_DTR_TX_LOW		(1 << 3) /* Inverted DTR */
	unsigned long delay_before_send;	/* Microseconds */
	unsigned long delay_after_send;		/* Microseconds */
	unsigned short padding[5];		/* Memory is cheap, new structs
					are a royal PITA .. */
};

#endif
