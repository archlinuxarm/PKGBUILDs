TEXT_BASE = 0x00e00000

# include NPE ethernet driver
BOARDLIBS = arch/arm/cpu/ixp/npe/libnpe.a

LDSCRIPT := $(SRCTREE)/board/$(BOARDDIR)/u-boot.lds
