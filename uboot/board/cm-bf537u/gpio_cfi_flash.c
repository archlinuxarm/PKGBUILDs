/*
 * gpio_cfi_flash.c - GPIO-assisted Flash Chip Support
 *
 * Copyright (c) 2009 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <asm/blackfin.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include "gpio_cfi_flash.h"

#define GPIO_PIN_1  GPIO_PH0
#define GPIO_MASK_1 (1 << 21)
#define GPIO_MASK   (GPIO_MASK_1)

void *gpio_cfi_flash_swizzle(void *vaddr)
{
	unsigned long addr = (unsigned long)vaddr;

	gpio_set_value(GPIO_PIN_1, addr & GPIO_MASK_1);

#ifdef GPIO_MASK_2
	gpio_set_value(GPIO_PIN_2, addr & GPIO_MASK_2);
#endif

	SSYNC();

	return (void *)(addr & ~GPIO_MASK);
}

#define __raw_writeq(value, addr) *(volatile u64 *)addr = value
#define __raw_readq(addr) *(volatile u64 *)addr

#define MAKE_FLASH(size, sfx) \
void flash_write##size(u##size value, void *addr) \
{ \
	__raw_write##sfx(value, gpio_cfi_flash_swizzle(addr)); \
} \
u##size flash_read##size(void *addr) \
{ \
	return __raw_read##sfx(gpio_cfi_flash_swizzle(addr)); \
}
MAKE_FLASH(8, b)  /* flash_write8()  flash_read8() */
MAKE_FLASH(16, w) /* flash_write16() flash_read16() */
MAKE_FLASH(32, l) /* flash_write32() flash_read32() */
MAKE_FLASH(64, q) /* flash_write64() flash_read64() */

void gpio_cfi_flash_init(void)
{
	gpio_request(GPIO_PIN_1, "gpio_cfi_flash");
#ifdef GPIO_MASK_2
	gpio_request(GPIO_PIN_2, "gpio_cfi_flash");
#endif
	gpio_cfi_flash_swizzle((void *)CONFIG_SYS_FLASH_BASE);
}
