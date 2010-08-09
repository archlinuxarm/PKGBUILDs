/*
 * (C) Copyright 2002, 2003
 * Daniel Engstr�m, Omicron Ceti AB, daniel@omicron.se
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <pci.h>
#include <asm/ic/sc520.h>

#define PROBE_BUFFER_SIZE 1024
static unsigned char buffer[PROBE_BUFFER_SIZE];

#define SC520_MAX_FLASH_BANKS  3
#define SC520_FLASH_BANK0_BASE 0x38000000  /* BOOTCS */
#define SC520_FLASH_BANK1_BASE 0x30000000  /* ROMCS0 */
#define SC520_FLASH_BANK2_BASE 0x28000000  /* ROMCS1 */
#define SC520_FLASH_BANKSIZE   0x8000000

#define AMD29LV016B_SIZE        0x200000
#define AMD29LV016B_SECTORS     32

flash_info_t    flash_info[SC520_MAX_FLASH_BANKS];

#define READY 1
#define ERR   2
#define TMO   4

/*-----------------------------------------------------------------------
 */


static u32 _probe_flash(u32 addr, u32 bw, int il)
{
	u32 result=0;

	/* First do an unlock cycle for the benefit of
	 * devices that need it */

	switch (bw) {

	case 1:
		*(volatile u8*)(addr+0x5555) = 0xaa;
		*(volatile u8*)(addr+0x2aaa) = 0x55;
		*(volatile u8*)(addr+0x5555) = 0x90;

		/* Read vendor */
		result = *(volatile u8*)addr;
		result <<= 16;

		/* Read device */
		result |= *(volatile u8*)(addr+2);

		/* Return device to data mode */
		*(volatile u8*)addr = 0xff;
		*(volatile u8*)(addr+0x5555), 0xf0;
		break;

	case 2:
		*(volatile u16*)(addr+0xaaaa) = 0xaaaa;
		*(volatile u16*)(addr+0x5554) = 0x5555;

		/* Issue identification command */
		if (il == 2) {
			*(volatile u16*)(addr+0xaaaa) = 0x9090;

			/* Read vendor */
			result = *(volatile u8*)addr;
			result <<= 16;

			/* Read device */
			result |= *(volatile u8*)(addr+2);

			/* Return device to data mode */
			*(volatile u16*)addr =  0xffff;
			*(volatile u16*)(addr+0xaaaa), 0xf0f0;

		} else {
			*(volatile u8*)(addr+0xaaaa) = 0x90;
			/* Read vendor */
			result = *(volatile u16*)addr;
			result <<= 16;

			/* Read device */
			result |= *(volatile u16*)(addr+2);

			/* Return device to data mode */
			*(volatile u8*)addr = 0xff;
			*(volatile u8*)(addr+0xaaaa), 0xf0;
		}

		break;

	 case 4:
		*(volatile u32*)(addr+0x5554) = 0xaaaaaaaa;
		*(volatile u32*)(addr+0xaaa8) = 0x55555555;

		switch (il) {
		case 1:
			/* Issue identification command */
			*(volatile u8*)(addr+0x5554) = 0x90;

			/* Read vendor */
			result = *(volatile u16*)addr;
			result <<= 16;

			/* Read device */
			result |= *(volatile u16*)(addr+4);

			/* Return device to data mode */
			*(volatile u8*)addr =  0xff;
			*(volatile u8*)(addr+0x5554), 0xf0;
			break;

		case 2:
			/* Issue identification command */
			*(volatile u32*)(addr + 0x5554) = 0x00900090;

			/* Read vendor */
			result = *(volatile u16*)addr;
			result <<= 16;

			/* Read device */
			result |= *(volatile u16*)(addr+4);

			/* Return device to data mode */
			*(volatile u32*)addr =  0x00ff00ff;
			*(volatile u32*)(addr+0x5554), 0x00f000f0;
			break;

		case 4:
			/* Issue identification command */
			*(volatile u32*)(addr+0x5554) = 0x90909090;

			/* Read vendor */
			result = *(volatile u8*)addr;
			result <<= 16;

			/* Read device */
			result |= *(volatile u8*)(addr+4);

			/* Return device to data mode */
			*(volatile u32*)addr =  0xffffffff;
			*(volatile u32*)(addr+0x5554), 0xf0f0f0f0;
			break;
		}
		break;
	}


	return result;
}

extern int _probe_flash_end;
asm ("_probe_flash_end:\n"
     ".long 0\n");

static int identify_flash(unsigned address, int width)
{
	int is;
	int device;
	int vendor;
	int size;
	unsigned res;

	u32 (*_probe_flash_ptr)(u32 a, u32 bw, int il);

	size = (unsigned)&_probe_flash_end - (unsigned)_probe_flash;

	if (size > PROBE_BUFFER_SIZE) {
		printf("_probe_flash() routine too large (%d) %p - %p\n",
		       size, &_probe_flash_end, _probe_flash);
		return 0;
	}

	memcpy(buffer, _probe_flash, size);
	_probe_flash_ptr = (void*)buffer;

	is = disable_interrupts();
	res = _probe_flash_ptr(address, width, 1);
	if (is) {
		enable_interrupts();
	}


	vendor = res >> 16;
	device = res & 0xffff;


	return res;
}

ulong flash_init(void)
{
	int i, j;
	ulong size = 0;

	for (i = 0; i < SC520_MAX_FLASH_BANKS; i++) {
		unsigned id;
		ulong flashbase = 0;
		int sectsize = 0;

		memset(flash_info[i].protect, 0, CONFIG_SYS_MAX_FLASH_SECT);
		switch (i) {
		case 0:
			flashbase = SC520_FLASH_BANK0_BASE;
			break;
		case 1:
			flashbase = SC520_FLASH_BANK1_BASE;
			break;
		case 2:
			flashbase = SC520_FLASH_BANK2_BASE;
			break;
		default:
			panic("configured too many flash banks!\n");
		}

		id = identify_flash(flashbase, 4);
		switch (id & 0x00ff00ff) {
		case 0x000100c8:
			/* 29LV016B/29LV017B */
			flash_info[i].flash_id =
				(AMD_MANUFACT & FLASH_VENDMASK) |
				(AMD_ID_LV016B & FLASH_TYPEMASK);

			flash_info[i].size = AMD29LV016B_SIZE*4;
			flash_info[i].sector_count = AMD29LV016B_SECTORS;
			sectsize = (AMD29LV016B_SIZE*4)/AMD29LV016B_SECTORS;
			printf("Bank %d: 4 x AMD 29LV017B\n", i);
			break;


		default:
			printf("Bank %d have unknown flash %08x\n", i, id);
			flash_info[i].flash_id = FLASH_UNKNOWN;
			continue;
		}

		for (j = 0; j < flash_info[i].sector_count; j++) {
			flash_info[i].start[j] = flashbase + j * sectsize;
		}
		size += flash_info[i].size;

		flash_protect(FLAG_PROTECT_CLEAR,
			      flash_info[i].start[0],
			       flash_info[i].start[0] + flash_info[i].size - 1,
			      &flash_info[i]);
	}

	/*
	 * Protect monitor and environment sectors
	 */
	flash_protect(FLAG_PROTECT_SET,
		      i386boot_start,
		      i386boot_end,
		      &flash_info[0]);
#ifdef CONFIG_ENV_ADDR
	flash_protect(FLAG_PROTECT_SET,
		      CONFIG_ENV_ADDR,
		      CONFIG_ENV_ADDR + CONFIG_ENV_SIZE - 1,
		      &flash_info[0]);
#endif
	return size;
}

/*-----------------------------------------------------------------------
 */
void flash_print_info(flash_info_t *info)
{
	int i;

	switch (info->flash_id & FLASH_VENDMASK) {

	case (AMD_MANUFACT & FLASH_VENDMASK):
		printf("AMD:   ");
		switch (info->flash_id & FLASH_TYPEMASK) {
		case (AMD_ID_LV016B & FLASH_TYPEMASK):
			printf("4x AMD29LV017B (4x16Mbit)\n");
			break;
		default:
			printf("Unknown Chip Type\n");
			goto done;
			break;
		}

		break;
	default:
		printf("Unknown Vendor ");
		break;
	}


	printf("  Size: %ld MB in %d Sectors\n",
	       info->size >> 20, info->sector_count);

	printf("  Sector Start Addresses:");
	for (i = 0; i < info->sector_count; i++) {
		if ((i % 5) == 0) {
			printf ("\n   ");
		}
		printf (" %08lX%s", info->start[i],
			info->protect[i] ? " (RO)" : "     ");
	}
	printf ("\n");

done:	;
}

/*-----------------------------------------------------------------------
 */

/* this needs to be inlined, the SWTMRMMILLI register is reset by each read */
#define __udelay(delay) \
{	\
	unsigned micro; \
	unsigned milli=0; \
	\
	micro = sc520_mmcr->swtmrmilli; \
	 \
	for (;;) { \
		\
		milli += sc520_mmcr->swtmrmilli; \
		micro = sc520_mmcr->swtmrmicro; \
		\
		if ((delay) <= (micro + (milli * 1000))) { \
			break; \
		} \
	} \
} while (0)

static u32 _amd_erase_flash(u32 addr, u32 sector)
{
	unsigned elapsed;

	/* Issue erase */
	*(volatile u32*)(addr + 0x5554) = 0xAAAAAAAA;
	*(volatile u32*)(addr + 0xaaa8) = 0x55555555;
	*(volatile u32*)(addr + 0x5554) = 0x80808080;
	/* And one unlock */
	*(volatile u32*)(addr + 0x5554) = 0xAAAAAAAA;
	*(volatile u32*)(addr + 0xaaa8) = 0x55555555;
	/* Sector erase command comes last */
	*(volatile u32*)(addr + sector) = 0x30303030;

	elapsed = sc520_mmcr->swtmrmilli; /* dummy read */
	elapsed = 0;
	__udelay(50);
	while (((*(volatile u32*)(addr + sector)) & 0x80808080) != 0x80808080) {

		elapsed += sc520_mmcr->swtmrmilli;
		if (elapsed > ((CONFIG_SYS_FLASH_ERASE_TOUT/CONFIG_SYS_HZ) * 1000)) {
			*(volatile u32*)(addr) = 0xf0f0f0f0;
			return 1;
		}
	}

	*(volatile u32*)(addr) = 0xf0f0f0f0;

	return 0;
}

extern int _amd_erase_flash_end;
asm ("_amd_erase_flash_end:\n"
     ".long 0\n");

int flash_erase(flash_info_t *info, int s_first, int s_last)
{
	u32 (*_erase_flash_ptr)(u32 a, u32 so);
	int prot;
	int sect;
	unsigned size;

	if ((s_first < 0) || (s_first > s_last)) {
		if (info->flash_id == FLASH_UNKNOWN) {
			printf("- missing\n");
		} else {
			printf("- no sectors to erase\n");
		}
		return 1;
	}

	if ((info->flash_id & FLASH_VENDMASK) == (AMD_MANUFACT & FLASH_VENDMASK)) {
		size = (unsigned)&_amd_erase_flash_end - (unsigned)_amd_erase_flash;

		if (size > PROBE_BUFFER_SIZE) {
			printf("_amd_erase_flash() routine too large (%d) %p - %p\n",
			       size, &_amd_erase_flash_end, _amd_erase_flash);
			return 0;
		}

		memcpy(buffer, _amd_erase_flash, size);
		_erase_flash_ptr = (void*)buffer;

	}  else {
		printf ("Can't erase unknown flash type - aborted\n");
		return 1;
	}

	prot = 0;
	for (sect=s_first; sect<=s_last; ++sect) {
		if (info->protect[sect]) {
			prot++;
		}
	}

	if (prot) {
		printf ("- Warning: %d protected sectors will not be erased!\n", prot);
	} else {
		printf ("\n");
	}


	/* Start erase on unprotected sectors */
	for (sect = s_first; sect<=s_last; sect++) {

		if (info->protect[sect] == 0) { /* not protected */
			int res;
			int flag;

			/* Disable interrupts which might cause a timeout here */
			flag = disable_interrupts();

			res = _erase_flash_ptr(info->start[0], info->start[sect]-info->start[0]);

			/* re-enable interrupts if necessary */
			if (flag) {
				enable_interrupts();
			}


			if (res) {
				printf("Erase timed out, sector %d\n", sect);
				return res;
			}

			putc('.');
		}
	}


	return 0;
}

/*-----------------------------------------------------------------------
 * Write a word to Flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
static int _amd_write_word(unsigned start, unsigned dest, unsigned data)
{
	volatile u32 *addr2 = (u32*)start;
	volatile u32 *dest2 = (u32*)dest;
	volatile u32 *data2 = (u32*)&data;
	unsigned elapsed;

	/* Check if Flash is (sufficiently) erased */
	if ((*((volatile u32*)dest) & (u32)data) != (u32)data) {
		return 2;
	}

	addr2[0x5554] = 0xAAAAAAAA;
	addr2[0xaaa8] = 0x55555555;
	addr2[0x5554] = 0xA0A0A0A0;

	dest2[0] = data;

	elapsed = sc520_mmcr->swtmrmilli; /* dummy read */
	elapsed = 0;

	/* data polling for D7 */
	while ((dest2[0] & 0x80808080) != (data2[0] & 0x80808080)) {
		elapsed += sc520_mmcr->swtmrmilli;
		if (elapsed > ((CONFIG_SYS_FLASH_WRITE_TOUT/CONFIG_SYS_HZ) * 1000)) {
			addr2[0] = 0xf0f0f0f0;
			return 1;
		}
	}


	addr2[0] = 0xf0f0f0f0;

	return 0;
}

extern int _amd_write_word_end;
asm ("_amd_write_word_end:\n"
     ".long 0\n");


/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 * 3 - Unsupported flash type
 */

int write_buff(flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
	ulong cp, wp, data;
	int i, l, rc;
	int flag;
	u32 (*_write_word_ptr)(unsigned start, unsigned dest, unsigned data);
	unsigned size;

	if ((info->flash_id & FLASH_VENDMASK) == (AMD_MANUFACT & FLASH_VENDMASK)) {
		size = (unsigned)&_amd_write_word_end - (unsigned)_amd_write_word;

		if (size > PROBE_BUFFER_SIZE) {
			printf("_amd_write_word() routine too large (%d) %p - %p\n",
			       size, &_amd_write_word_end, _amd_write_word);
			return 0;
		}

		memcpy(buffer, _amd_write_word, size);
		_write_word_ptr = (void*)buffer;

	} else {
		printf ("Can't program unknown flash type - aborted\n");
		return 3;
	}


	wp = (addr & ~3);	/* get lower word aligned address */


	/*
	 * handle unaligned start bytes
	 */
	if ((l = addr - wp) != 0) {
		data = 0;
		for (i=0, cp=wp; i<l; ++i, ++cp) {
			data |= (*(uchar *)cp) << (8*i);
		}
		for (; i<4 && cnt>0; ++i) {
			data |= *src++ << (8*i);
			--cnt;
			++cp;
		}
		for (; cnt==0 && i<4; ++i, ++cp) {
			data |= (*(uchar *)cp)  << (8*i);
		}

		/* Disable interrupts which might cause a timeout here */
		flag = disable_interrupts();

		rc = _write_word_ptr(info->start[0], wp, data);

		/* re-enable interrupts if necessary */
		if (flag) {
			enable_interrupts();
		}
		if (rc != 0) {
			return rc;
		}
		wp += 4;
	}

	/*
	 * handle word aligned part
	 */
	while (cnt >= 4) {
		data = 0;

		for (i=0; i<4; ++i) {
			data |= *src++ << (8*i);
		}

		/* Disable interrupts which might cause a timeout here */
		flag = disable_interrupts();

		rc = _write_word_ptr(info->start[0], wp, data);

		/* re-enable interrupts if necessary */
		if (flag) {
			enable_interrupts();
		}
		if (rc != 0) {
			return rc;
		}
		wp  += 4;
		cnt -= 4;
	}

	if (cnt == 0) {
		return 0;
	}

	/*
	 * handle unaligned tail bytes
	 */
	data = 0;
	for (i=0, cp=wp; i<4 && cnt>0; ++i, ++cp) {
		data |= *src++ << (8*i);
		--cnt;
	}

	for (; i<4; ++i, ++cp) {
		data |= (*(uchar *)cp) << (8*i);
	}

	/* Disable interrupts which might cause a timeout here */
	flag = disable_interrupts();

	rc = _write_word_ptr(info->start[0], wp, data);

	/* re-enable interrupts if necessary */
	if (flag) {
		enable_interrupts();
	}

	return rc;

}
