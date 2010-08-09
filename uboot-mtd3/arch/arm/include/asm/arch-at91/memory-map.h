/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian.pop@leadtechdesign.com>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __ASM_ARM_ARCH_MEMORYMAP_H__
#define __ASM_ARM_ARCH_MEMORYMAP_H__

#include <asm/arch/hardware.h>

#define USART0_BASE AT91_USART0
#define USART1_BASE AT91_USART1
#define USART2_BASE AT91_USART2
#define USART3_BASE (AT91_BASE_SYS + AT91_DBGU)
#define SPI0_BASE	AT91_BASE_SPI

#endif /* __ASM_ARM_ARCH_MEMORYMAP_H__ */
