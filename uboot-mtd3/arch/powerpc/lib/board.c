/*
 * (C) Copyright 2000-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
#include <watchdog.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#ifdef CONFIG_8xx
#include <mpc8xx.h>
#endif
#ifdef CONFIG_5xx
#include <mpc5xx.h>
#endif
#ifdef CONFIG_MPC5xxx
#include <mpc5xxx.h>
#endif
#if defined(CONFIG_CMD_IDE)
#include <ide.h>
#endif
#if defined(CONFIG_CMD_SCSI)
#include <scsi.h>
#endif
#if defined(CONFIG_CMD_KGDB)
#include <kgdb.h>
#endif
#ifdef CONFIG_STATUS_LED
#include <status_led.h>
#endif
#include <net.h>
#ifdef CONFIG_GENERIC_MMC
#include <mmc.h>
#endif
#include <serial.h>
#ifdef CONFIG_SYS_ALLOC_DPRAM
#if !defined(CONFIG_CPM2)
#include <commproc.h>
#endif
#endif
#include <version.h>
#if defined(CONFIG_BAB7xx)
#include <w83c553f.h>
#endif
#include <dtt.h>
#if defined(CONFIG_POST)
#include <post.h>
#endif
#if defined(CONFIG_LOGBUFFER)
#include <logbuff.h>
#endif
#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
#include <asm/cache.h>
#endif
#ifdef CONFIG_PS2KBD
#include <keyboard.h>
#endif

#ifdef CONFIG_ADDR_MAP
#include <asm/mmu.h>
#endif

#ifdef CONFIG_MP
#include <asm/mp.h>
#endif

#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif

#ifdef CONFIG_SYS_UPDATE_FLASH_SIZE
extern int update_flash_size (int flash_size);
#endif

#if defined(CONFIG_SC3)
extern void sc3_read_eeprom(void);
#endif

#if defined(CONFIG_CMD_DOC)
void doc_init (void);
#endif
#if defined(CONFIG_HARD_I2C) || \
    defined(CONFIG_SOFT_I2C)
#include <i2c.h>
#endif
#include <spi.h>
#include <nand.h>

static char *failed = "*** failed ***\n";

#if defined(CONFIG_OXC) || defined(CONFIG_PCU_E) || defined(CONFIG_RMU)
extern flash_info_t flash_info[];
#endif

#if defined(CONFIG_START_IDE)
extern int board_start_ide(void);
#endif
#include <environment.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_ENV_IS_EMBEDDED)
#define TOTAL_MALLOC_LEN	CONFIG_SYS_MALLOC_LEN
#elif ( ((CONFIG_ENV_ADDR+CONFIG_ENV_SIZE) < CONFIG_SYS_MONITOR_BASE) || \
	(CONFIG_ENV_ADDR >= (CONFIG_SYS_MONITOR_BASE + CONFIG_SYS_MONITOR_LEN)) ) || \
      defined(CONFIG_ENV_IS_IN_NVRAM)
#define	TOTAL_MALLOC_LEN	(CONFIG_SYS_MALLOC_LEN + CONFIG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CONFIG_SYS_MALLOC_LEN
#endif

#if !defined(CONFIG_SYS_MEM_TOP_HIDE)
#define CONFIG_SYS_MEM_TOP_HIDE	0
#endif

extern ulong __init_end;
extern ulong _end;
ulong monitor_flash_len;

#if defined(CONFIG_CMD_BEDBUG)
#include <bedbug/type.h>
#endif

/************************************************************************
 * Utilities								*
 ************************************************************************
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

/************************************************************************
 * Init Utilities							*
 ************************************************************************
 * Some of this code should be moved into the core functions,
 * but let's get it working (again) first...
 */

static int init_baudrate (void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;
	return (0);
}

/***********************************************************************/

void __board_add_ram_info(int use_default)
{
	/* please define platform specific board_add_ram_info() */
}
void board_add_ram_info(int) __attribute__((weak, alias("__board_add_ram_info")));


static int init_func_ram (void)
{
#ifdef	CONFIG_BOARD_TYPES
	int board_type = gd->board_type;
#else
	int board_type = 0;	/* use dummy arg */
#endif
	puts ("DRAM:  ");

	if ((gd->ram_size = initdram (board_type)) > 0) {
		print_size (gd->ram_size, "");
		board_add_ram_info(0);
		putc('\n');
		return (0);
	}
	puts (failed);
	return (1);
}

/***********************************************************************/

#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
static int init_func_i2c (void)
{
	puts ("I2C:   ");
	i2c_init (CONFIG_SYS_I2C_SPEED, CONFIG_SYS_I2C_SLAVE);
	puts ("ready\n");
	return (0);
}
#endif

#if defined(CONFIG_HARD_SPI)
static int init_func_spi (void)
{
	puts ("SPI:   ");
	spi_init ();
	puts ("ready\n");
	return (0);
}
#endif

/***********************************************************************/

#if defined(CONFIG_WATCHDOG)
static int init_func_watchdog_init (void)
{
	puts ("       Watchdog enabled\n");
	WATCHDOG_RESET ();
	return (0);
}
# define INIT_FUNC_WATCHDOG_INIT	init_func_watchdog_init,

static int init_func_watchdog_reset (void)
{
	WATCHDOG_RESET ();
	return (0);
}
# define INIT_FUNC_WATCHDOG_RESET	init_func_watchdog_reset,
#else
# define INIT_FUNC_WATCHDOG_INIT	/* undef */
# define INIT_FUNC_WATCHDOG_RESET	/* undef */
#endif /* CONFIG_WATCHDOG */

/************************************************************************
 * Initialization sequence						*
 ************************************************************************
 */

init_fnc_t *init_sequence[] = {
#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	probecpu,
#endif
#if defined(CONFIG_BOARD_EARLY_INIT_F)
	board_early_init_f,
#endif
#if !defined(CONFIG_8xx_CPUCLK_DEFAULT)
	get_clocks,		/* get CPU and bus clocks (etc.) */
#if defined(CONFIG_TQM8xxL) && !defined(CONFIG_TQM866M) \
    && !defined(CONFIG_TQM885D)
	adjust_sdram_tbs_8xx,
#endif
	init_timebase,
#endif
#ifdef CONFIG_SYS_ALLOC_DPRAM
#if !defined(CONFIG_CPM2)
	dpram_init,
#endif
#endif
#if defined(CONFIG_BOARD_POSTCLK_INIT)
	board_postclk_init,
#endif
	env_init,
#if defined(CONFIG_8xx_CPUCLK_DEFAULT)
	get_clocks_866,		/* get CPU and bus clocks according to the environment variable */
	sdram_adjust_866,	/* adjust sdram refresh rate according to the new clock */
	init_timebase,
#endif
	init_baudrate,
	serial_init,
	console_init_f,
	display_options,
#if defined(CONFIG_8260)
	prt_8260_rsr,
	prt_8260_clks,
#endif /* CONFIG_8260 */
#if defined(CONFIG_MPC83xx)
	prt_83xx_rsr,
#endif
	checkcpu,
#if defined(CONFIG_MPC5xxx)
	prt_mpc5xxx_clks,
#endif /* CONFIG_MPC5xxx */
#if defined(CONFIG_MPC8220)
	prt_mpc8220_clks,
#endif
	checkboard,
	INIT_FUNC_WATCHDOG_INIT
#if defined(CONFIG_MISC_INIT_F)
	misc_init_f,
#endif
	INIT_FUNC_WATCHDOG_RESET
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	init_func_i2c,
#endif
#if defined(CONFIG_HARD_SPI)
	init_func_spi,
#endif
#ifdef CONFIG_POST
	post_init_f,
#endif
	INIT_FUNC_WATCHDOG_RESET
	init_func_ram,
#if defined(CONFIG_SYS_DRAM_TEST)
	testdram,
#endif /* CONFIG_SYS_DRAM_TEST */
	INIT_FUNC_WATCHDOG_RESET

	NULL,			/* Terminate this list */
};

ulong get_effective_memsize(void)
{
#ifndef	CONFIG_VERY_BIG_RAM
	return gd->ram_size;
#else
	/* limit stack to what we can reasonable map */
	return ((gd->ram_size > CONFIG_MAX_MEM_MAPPED) ?
		 CONFIG_MAX_MEM_MAPPED : gd->ram_size);
#endif
}

/************************************************************************
 *
 * This is the first part of the initialization sequence that is
 * implemented in C, but still running from ROM.
 *
 * The main purpose is to provide a (serial) console interface as
 * soon as possible (so we can see any error messages), and to
 * initialize the RAM so that we can relocate the monitor code to
 * RAM.
 *
 * Be aware of the restrictions: global data is read-only, BSS is not
 * initialized, and stack space is limited to a few kB.
 *
 ************************************************************************
 */

#ifdef CONFIG_LOGBUFFER
unsigned long logbuffer_base(void)
{
	return CONFIG_SYS_SDRAM_BASE + get_effective_memsize() - LOGBUFF_LEN;
}
#endif

void board_init_f (ulong bootflag)
{
	bd_t *bd;
	ulong len, addr, addr_sp;
	ulong *s;
	gd_t *id;
	init_fnc_t **init_fnc_ptr;
#ifdef CONFIG_PRAM
	int i;
	ulong reg;
	uchar tmp[64];		/* long enough for environment variables */
#endif

	/* Pointer is writable since we allocated a register for it */
	gd = (gd_t *) (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_GBL_DATA_OFFSET);
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

#if !defined(CONFIG_CPM2) && !defined(CONFIG_MPC512X) && \
    !defined(CONFIG_MPC83xx) && !defined(CONFIG_MPC85xx) && \
    !defined(CONFIG_MPC86xx)
	/* Clear initial global data */
	memset ((void *) gd, 0, sizeof (gd_t));
#endif

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr) () != 0) {
			hang ();
		}
	}

	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 *
	 * Reserve memory at end of RAM for (top down in that order):
	 *  - area that won't get touched by U-Boot and Linux (optional)
	 *  - kernel log buffer
	 *  - protected RAM
	 *  - LCD framebuffer
	 *  - monitor code
	 *  - board info struct
	 */
	len = (ulong)&_end - CONFIG_SYS_MONITOR_BASE;

	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This should work
	 * for arch/ppc and arch/powerpc. Only Linux board ports in
	 * arch/powerpc with bootwrapper support, that recalculate the
	 * memory size from the SDRAM controller setup will have to
	 * get fixed.
	 */
	gd->ram_size -= CONFIG_SYS_MEM_TOP_HIDE;

	addr = CONFIG_SYS_SDRAM_BASE + get_effective_memsize();

#if defined(CONFIG_MP) && (defined(CONFIG_MPC86xx) || defined(CONFIG_E500))
	/*
	 * We need to make sure the location we intend to put secondary core
	 * boot code is reserved and not used by any part of u-boot
	 */
	if (addr > determine_mp_bootpg()) {
		addr = determine_mp_bootpg();
		debug ("Reserving MP boot page to %08lx\n", addr);
	}
#endif

#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
	/* reserve kernel log buffer */
	addr -= (LOGBUFF_RESERVE);
	debug ("Reserving %dk for kernel logbuffer at %08lx\n", LOGBUFF_LEN, addr);
#endif
#endif

#ifdef CONFIG_PRAM
	/*
	 * reserve protected RAM
	 */
	i = getenv_r ("pram", (char *)tmp, sizeof (tmp));
	reg = (i > 0) ? simple_strtoul ((const char *)tmp, NULL, 10) : CONFIG_PRAM;
	addr -= (reg << 10);		/* size is in kB */
	debug ("Reserving %ldk for protected RAM at %08lx\n", reg, addr);
#endif /* CONFIG_PRAM */

	/* round down to next 4 kB limit */
	addr &= ~(4096 - 1);
	debug ("Top of RAM usable for U-Boot at: %08lx\n", addr);

#ifdef CONFIG_LCD
	/* reserve memory for LCD display (always full pages) */
	addr = lcd_setmem (addr);
	gd->fb_base = addr;
#endif /* CONFIG_LCD */

#if defined(CONFIG_VIDEO) && defined(CONFIG_8xx)
	/* reserve memory for video display (always full pages) */
	addr = video_setmem (addr);
	gd->fb_base = addr;
#endif /* CONFIG_VIDEO  */

	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	addr -= len;
	addr &= ~(4096 - 1);
#ifdef CONFIG_E500
	/* round down to next 64 kB limit so that IVPR stays aligned */
	addr &= ~(65536 - 1);
#endif

	debug ("Reserving %ldk for U-Boot at: %08lx\n", len >> 10, addr);

	/*
	 * reserve memory for malloc() arena
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug ("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof (bd_t);
	bd = (bd_t *) addr_sp;
	gd->bd = bd;
	debug ("Reserving %zu Bytes for Board Info at: %08lx\n",
			sizeof (bd_t), addr_sp);
	addr_sp -= sizeof (gd_t);
	id = (gd_t *) addr_sp;
	debug ("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;
	s = (ulong *)addr_sp;
	*s-- = 0;
	*s-- = 0;
	addr_sp = (ulong)s;
	debug ("Stack Pointer at: %08lx\n", addr_sp);

	/*
	 * Save local variables to board info struct
	 */

	bd->bi_memstart  = CONFIG_SYS_SDRAM_BASE;	/* start of  DRAM memory	*/
	bd->bi_memsize   = gd->ram_size;	/* size  of  DRAM memory in bytes */

#ifdef CONFIG_IP860
	bd->bi_sramstart = SRAM_BASE;	/* start of  SRAM memory	*/
	bd->bi_sramsize  = SRAM_SIZE;	/* size  of  SRAM memory	*/
#elif defined CONFIG_MPC8220
	bd->bi_sramstart = CONFIG_SYS_SRAM_BASE;	/* start of  SRAM memory	*/
	bd->bi_sramsize  = CONFIG_SYS_SRAM_SIZE;	/* size  of  SRAM memory	*/
#else
	bd->bi_sramstart = 0;		/* FIXME */ /* start of  SRAM memory	*/
	bd->bi_sramsize  = 0;		/* FIXME */ /* size  of  SRAM memory	*/
#endif

#if defined(CONFIG_8xx) || defined(CONFIG_8260) || defined(CONFIG_5xx) || \
    defined(CONFIG_E500) || defined(CONFIG_MPC86xx)
	bd->bi_immr_base = CONFIG_SYS_IMMR;	/* base  of IMMR register     */
#endif
#if defined(CONFIG_MPC5xxx)
	bd->bi_mbar_base = CONFIG_SYS_MBAR;	/* base of internal registers */
#endif
#if defined(CONFIG_MPC83xx)
	bd->bi_immrbar = CONFIG_SYS_IMMR;
#endif
#if defined(CONFIG_MPC8220)
	bd->bi_mbar_base = CONFIG_SYS_MBAR;	/* base of internal registers */
	bd->bi_inpfreq   = gd->inp_clk;
	bd->bi_pcifreq   = gd->pci_clk;
	bd->bi_vcofreq   = gd->vco_clk;
	bd->bi_pevfreq   = gd->pev_clk;
	bd->bi_flbfreq   = gd->flb_clk;

	/* store bootparam to sram (backward compatible), here? */
	{
		u32 *sram = (u32 *)CONFIG_SYS_SRAM_BASE;
		*sram++ = gd->ram_size;
		*sram++ = gd->bus_clk;
		*sram++ = gd->inp_clk;
		*sram++ = gd->cpu_clk;
		*sram++ = gd->vco_clk;
		*sram++ = gd->flb_clk;
		*sram++ = 0xb8c3ba11;  /* boot signature */
	}
#endif

	bd->bi_bootflags = bootflag;	/* boot / reboot flag (for LynxOS)    */

	WATCHDOG_RESET ();
	bd->bi_intfreq = gd->cpu_clk;	/* Internal Freq, in Hz */
	bd->bi_busfreq = gd->bus_clk;	/* Bus Freq,      in Hz */
#if defined(CONFIG_CPM2)
	bd->bi_cpmfreq = gd->cpm_clk;
	bd->bi_brgfreq = gd->brg_clk;
	bd->bi_sccfreq = gd->scc_clk;
	bd->bi_vco     = gd->vco_out;
#endif /* CONFIG_CPM2 */
#if defined(CONFIG_MPC512X)
	bd->bi_ipsfreq = gd->ips_clk;
#endif /* CONFIG_MPC512X */
#if defined(CONFIG_MPC5xxx)
	bd->bi_ipbfreq = gd->ipb_clk;
	bd->bi_pcifreq = gd->pci_clk;
#endif /* CONFIG_MPC5xxx */
	bd->bi_baudrate = gd->baudrate;	/* Console Baudrate     */

#ifdef CONFIG_SYS_EXTBDINFO
	strncpy ((char *)bd->bi_s_version, "1.2", sizeof (bd->bi_s_version));
	strncpy ((char *)bd->bi_r_version, U_BOOT_VERSION, sizeof (bd->bi_r_version));

	bd->bi_procfreq = gd->cpu_clk;	/* Processor Speed, In Hz */
	bd->bi_plb_busfreq = gd->bus_clk;
#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || \
    defined(CONFIG_440EP) || defined(CONFIG_440GR) || \
    defined(CONFIG_440EPX) || defined(CONFIG_440GRX)
	bd->bi_pci_busfreq = get_PCI_freq ();
	bd->bi_opbfreq = get_OPB_freq ();
#elif defined(CONFIG_XILINX_405)
	bd->bi_pci_busfreq = get_PCI_freq ();
#endif
#endif

	debug ("New Stack Pointer is: %08lx\n", addr_sp);

	WATCHDOG_RESET ();

#ifdef CONFIG_POST
	post_bootmode_init();
	post_run (NULL, POST_ROM | post_bootmode_get(0));
#endif

	WATCHDOG_RESET();

	gd->relocaddr = addr; /* Record relocation address, useful for debug */

	memcpy (id, (void *)gd, sizeof (gd_t));

	relocate_code (addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
}

/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */
void board_init_r (gd_t *id, ulong dest_addr)
{
	char *s;
	bd_t *bd;
	ulong malloc_start;

#ifndef CONFIG_SYS_NO_FLASH
	ulong flash_size;
#endif

	gd = id;		/* initialize RAM version of global data */
	bd = gd->bd;

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	/* The Malloc area is immediately below the monitor copy in DRAM */
	malloc_start = dest_addr - TOTAL_MALLOC_LEN;

#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	/*
	 * The gd->cpu pointer is set to an address in flash before relocation.
	 * We need to update it to point to the same CPU entry in RAM.
	 */
	gd->cpu += dest_addr - CONFIG_SYS_MONITOR_BASE;
#endif

#ifdef CONFIG_SERIAL_MULTI
	serial_initialize();
#endif

	debug ("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

	WATCHDOG_RESET ();

	/*
	 * Setup trap handlers
	 */
	trap_init (dest_addr);

#ifdef CONFIG_ADDR_MAP
	init_addr_map();
#endif

#if defined(CONFIG_BOARD_EARLY_INIT_R)
	board_early_init_r ();
#endif

	monitor_flash_len = (ulong)&__init_end - dest_addr;

	WATCHDOG_RESET ();

#ifdef CONFIG_LOGBUFFER
	logbuff_init_ptrs ();
#endif
#ifdef CONFIG_POST
	post_output_backlog ();
#endif

	WATCHDOG_RESET();

#if defined(CONFIG_SYS_DELAYED_ICACHE)
	icache_enable ();	/* it's time to enable the instruction cache */
#endif

#if defined(CONFIG_SYS_INIT_RAM_LOCK) && defined(CONFIG_E500)
	unlock_ram_in_cache();	/* it's time to unlock D-cache in e500 */
#endif

#if defined(CONFIG_BAB7xx) || defined(CONFIG_CPC45)
	/*
	 * Do PCI configuration on BAB7xx and CPC45 _before_ the flash
	 * gets initialised, because we need the ISA resp. PCI_to_LOCAL bus
	 * bridge there.
	 */
	pci_init ();
#endif
#if defined(CONFIG_BAB7xx)
	/*
	 * Initialise the ISA bridge
	 */
	initialise_w83c553f ();
#endif

	asm ("sync ; isync");

	mem_malloc_init (malloc_start, TOTAL_MALLOC_LEN);

#if !defined(CONFIG_SYS_NO_FLASH)
	puts ("FLASH: ");

	if ((flash_size = flash_init ()) > 0) {
# ifdef CONFIG_SYS_FLASH_CHECKSUM
		print_size (flash_size, "");
		/*
		 * Compute and print flash CRC if flashchecksum is set to 'y'
		 *
		 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
		 */
		s = getenv ("flashchecksum");
		if (s && (*s == 'y')) {
			printf ("  CRC: %08X",
				crc32 (0, (const unsigned char *) CONFIG_SYS_FLASH_BASE, flash_size)
			);
		}
		putc ('\n');
# else	/* !CONFIG_SYS_FLASH_CHECKSUM */
		print_size (flash_size, "\n");
# endif /* CONFIG_SYS_FLASH_CHECKSUM */
	} else {
		puts (failed);
		hang ();
	}

	bd->bi_flashstart = CONFIG_SYS_FLASH_BASE;	/* update start of FLASH memory    */
	bd->bi_flashsize = flash_size;	/* size of FLASH memory (final value) */

#if defined(CONFIG_SYS_UPDATE_FLASH_SIZE)
	/* Make a update of the Memctrl. */
	update_flash_size (flash_size);
#endif


# if defined(CONFIG_PCU_E) || defined(CONFIG_OXC) || defined(CONFIG_RMU)
	/* flash mapped at end of memory map */
	bd->bi_flashoffset = TEXT_BASE + flash_size;
# elif CONFIG_SYS_MONITOR_BASE == CONFIG_SYS_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for startup monitor  */
# else
	bd->bi_flashoffset = 0;
# endif
#else	/* CONFIG_SYS_NO_FLASH */

	bd->bi_flashsize = 0;
	bd->bi_flashstart = 0;
	bd->bi_flashoffset = 0;
#endif /* !CONFIG_SYS_NO_FLASH */

	WATCHDOG_RESET ();

	/* initialize higher level parts of CPU like time base and timers */
	cpu_init_r ();

	WATCHDOG_RESET ();

#ifdef CONFIG_SPI
# if !defined(CONFIG_ENV_IS_IN_EEPROM)
	spi_init_f ();
# endif
	spi_init_r ();
#endif

#if defined(CONFIG_CMD_NAND)
	WATCHDOG_RESET ();
	puts ("NAND:  ");
	nand_init();		/* go init the NAND */
#endif

	/* relocate environment function pointers etc. */
	env_relocate ();

	/*
	 * Fill in missing fields of bd_info.
	 * We do this here, where we have "normal" access to the
	 * environment; we used to do this still running from ROM,
	 * where had to use getenv_r(), which can be pretty slow when
	 * the environment is in EEPROM.
	 */

#if defined(CONFIG_SYS_EXTBDINFO)
#if defined(CONFIG_405GP) || defined(CONFIG_405EP)
#if defined(CONFIG_I2CFAST)
	/*
	 * set bi_iic_fast for linux taking environment variable
	 * "i2cfast" into account
	 */
	{
		char *s = getenv ("i2cfast");
		if (s && ((*s == 'y') || (*s == 'Y'))) {
			bd->bi_iic_fast[0] = 1;
			bd->bi_iic_fast[1] = 1;
		} else {
			bd->bi_iic_fast[0] = 0;
			bd->bi_iic_fast[1] = 0;
		}
	}
#else
	bd->bi_iic_fast[0] = 0;
	bd->bi_iic_fast[1] = 0;
#endif	/* CONFIG_I2CFAST */
#endif	/* CONFIG_405GP, CONFIG_405EP */
#endif	/* CONFIG_SYS_EXTBDINFO */

#if defined(CONFIG_SC3)
	sc3_read_eeprom();
#endif

#if defined (CONFIG_ID_EEPROM) || defined (CONFIG_SYS_I2C_MAC_OFFSET)
	mac_read_from_eeprom();
#endif

#ifdef	CONFIG_HERMES
	if ((gd->board_type >> 16) == 2)
		bd->bi_ethspeed = gd->board_type & 0xFFFF;
	else
		bd->bi_ethspeed = 0xFFFF;
#endif

#ifdef CONFIG_CMD_NET
	/* kept around for legacy kernels only ... ignore the next section */
	eth_getenv_enetaddr("ethaddr", bd->bi_enetaddr);
#ifdef CONFIG_HAS_ETH1
	eth_getenv_enetaddr("eth1addr", bd->bi_enet1addr);
#endif
#ifdef CONFIG_HAS_ETH2
	eth_getenv_enetaddr("eth2addr", bd->bi_enet2addr);
#endif
#ifdef CONFIG_HAS_ETH3
	eth_getenv_enetaddr("eth3addr", bd->bi_enet3addr);
#endif
#ifdef CONFIG_HAS_ETH4
	eth_getenv_enetaddr("eth4addr", bd->bi_enet4addr);
#endif
#ifdef CONFIG_HAS_ETH5
	eth_getenv_enetaddr("eth5addr", bd->bi_enet5addr);
#endif
#endif /* CONFIG_CMD_NET */

	/* IP Address */
	bd->bi_ip_addr = getenv_IPaddr ("ipaddr");

	WATCHDOG_RESET ();

#if defined(CONFIG_PCI) && !defined(CONFIG_BAB7xx) && !defined(CONFIG_CPC45)
	/*
	 * Do pci configuration
	 */
	pci_init ();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
	/* Initialize stdio devices */
	stdio_init ();

	/* Initialize the jump table for applications */
	jumptable_init ();

#if defined(CONFIG_API)
	/* Initialize API */
	api_init ();
#endif

	/* Initialize the console (after the relocation and devices init) */
	console_init_r ();

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r ();
#endif

#ifdef	CONFIG_HERMES
	if (bd->bi_ethspeed != 0xFFFF)
		hermes_start_lxt980 ((int) bd->bi_ethspeed);
#endif

#if defined(CONFIG_CMD_KGDB)
	WATCHDOG_RESET ();
	puts ("KGDB:  ");
	kgdb_init ();
#endif

	debug ("U-Boot relocated to %08lx\n", dest_addr);

	/*
	 * Enable Interrupts
	 */
	interrupt_init ();

	/* Must happen after interrupts are initialized since
	 * an irq handler gets installed
	 */
#ifdef CONFIG_SERIAL_SOFTWARE_FIFO
	serial_buffered_init();
#endif

#if defined(CONFIG_STATUS_LED) && defined(STATUS_LED_BOOT)
	status_led_set (STATUS_LED_BOOT, STATUS_LED_BLINKING);
#endif

	udelay (20);

	set_timer (0);

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}
#if defined(CONFIG_CMD_NET)
	if ((s = getenv ("bootfile")) != NULL) {
		copy_filename (BootFile, s, sizeof (BootFile));
	}
#endif

	WATCHDOG_RESET ();

#if defined(CONFIG_DTT)		/* Digital Thermometers and Thermostats */
	dtt_init ();
#endif
#if defined(CONFIG_CMD_SCSI)
	WATCHDOG_RESET ();
	puts ("SCSI:  ");
	scsi_init ();
#endif

#ifdef CONFIG_GENERIC_MMC
	WATCHDOG_RESET ();
	puts ("MMC:  ");
	mmc_initialize (bd);
#endif

#if defined(CONFIG_CMD_DOC)
	WATCHDOG_RESET ();
	puts ("DOC:   ");
	doc_init ();
#endif

#ifdef CONFIG_BITBANGMII
	bb_miiphy_init();
#endif
#if defined(CONFIG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	WATCHDOG_RESET ();
	puts ("Net:   ");
#endif
	eth_initialize (bd);
#endif

#if defined(CONFIG_CMD_NET) && defined(CONFIG_RESET_PHY_R)
	WATCHDOG_RESET ();
	debug ("Reset Ethernet PHY\n");
	reset_phy ();
#endif

#ifdef CONFIG_POST
	post_run (NULL, POST_RAM | post_bootmode_get(0));
#endif

#if defined(CONFIG_CMD_PCMCIA) \
    && !defined(CONFIG_CMD_IDE)
	WATCHDOG_RESET ();
	puts ("PCMCIA:");
	pcmcia_init ();
#endif

#if defined(CONFIG_CMD_IDE)
	WATCHDOG_RESET ();
# ifdef	CONFIG_IDE_8xx_PCCARD
	puts ("PCMCIA:");
# else
	puts ("IDE:   ");
#endif
#if defined(CONFIG_START_IDE)
	if (board_start_ide())
		ide_init ();
#else
	ide_init ();
#endif
#endif

#ifdef CONFIG_LAST_STAGE_INIT
	WATCHDOG_RESET ();
	/*
	 * Some parts can be only initialized if all others (like
	 * Interrupts) are up and running (i.e. the PC-style ISA
	 * keyboard).
	 */
	last_stage_init ();
#endif

#if defined(CONFIG_CMD_BEDBUG)
	WATCHDOG_RESET ();
	bedbug_init ();
#endif

#if defined(CONFIG_PRAM) || defined(CONFIG_LOGBUFFER)
	/*
	 * Export available size of memory for Linux,
	 * taking into account the protected RAM at top of memory
	 */
	{
		ulong pram;
		uchar memsz[32];
#ifdef CONFIG_PRAM
		char *s;

		if ((s = getenv ("pram")) != NULL) {
			pram = simple_strtoul (s, NULL, 10);
		} else {
			pram = CONFIG_PRAM;
		}
#else
		pram=0;
#endif
#ifdef CONFIG_LOGBUFFER
#ifndef CONFIG_ALT_LB_ADDR
		/* Also take the logbuffer into account (pram is in kB) */
		pram += (LOGBUFF_LEN+LOGBUFF_OVERHEAD)/1024;
#endif
#endif
		sprintf ((char *)memsz, "%ldk", (bd->bi_memsize / 1024) - pram);
		setenv ("mem", (char *)memsz);
	}
#endif

#ifdef CONFIG_PS2KBD
	puts ("PS/2:  ");
	kbd_init();
#endif

#ifdef CONFIG_MODEM_SUPPORT
 {
	 extern int do_mdm_init;
	 do_mdm_init = gd->do_mdm_init;
 }
#endif

	/* Initialization complete - start the monitor */

	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		WATCHDOG_RESET ();
		main_loop ();
	}

	/* NOTREACHED - no way out of command loop except booting */
}

void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	show_boot_progress(-30);
	for (;;);
}


#if 0 /* We could use plain global data, but the resulting code is bigger */
/*
 * Pointer to initial global data area
 *
 * Here we initialize it.
 */
#undef	XTRN_DECLARE_GLOBAL_DATA_PTR
#define XTRN_DECLARE_GLOBAL_DATA_PTR	/* empty = allocate here */
DECLARE_GLOBAL_DATA_PTR = (gd_t *) (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_GBL_DATA_OFFSET);
#endif  /* 0 */

/************************************************************************/
