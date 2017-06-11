setenv kernel_loadaddr "0x11000000"
setenv initrd_loadaddr "0x13000000"
setenv condev "console=ttyS0,115200n8 console=tty0 no_console_suspend consoleblank=0"
setenv bootargs "root=LABEL=ROOTFS rootflags=data=writeback rw ${condev} fsck.repair=yes net.ifnames=0 mac=${mac}"
setenv boot_start booti ${kernel_loadaddr} ${initrd_loadaddr} ${dtb_mem_addr}
if fatload usb 0 ${initrd_loadaddr} uInitrd; then if fatload usb 0 ${kernel_loadaddr} zImage; then if fatload usb 0 ${dtb_mem_addr} dtb.img; then run boot_start; else store dtb read ${dtb_mem_addr}; run boot_start;fi;fi;fi;
if fatload usb 1 ${initrd_loadaddr} uInitrd; then if fatload usb 1 ${kernel_loadaddr} zImage; then if fatload usb 1 ${dtb_mem_addr} dtb.img; then run boot_start; else store dtb read ${dtb_mem_addr}; run boot_start;fi;fi;fi;
if fatload usb 2 ${initrd_loadaddr} uInitrd; then if fatload usb 2 ${kernel_loadaddr} zImage; then if fatload usb 2 ${dtb_mem_addr} dtb.img; then run boot_start; else store dtb read ${dtb_mem_addr}; run boot_start;fi;fi;fi;
if fatload usb 3 ${initrd_loadaddr} uInitrd; then if fatload usb 3 ${kernel_loadaddr} zImage; then if fatload usb 3 ${dtb_mem_addr} dtb.img; then run boot_start; else store dtb read ${dtb_mem_addr}; run boot_start;fi;fi;fi;
if fatload mmc 0 ${initrd_loadaddr} uInitrd; then if fatload mmc 0 ${kernel_loadaddr} zImage; then if fatload mmc 0 ${dtb_mem_addr} dtb.img; then run boot_start; else store dtb read ${dtb_mem_addr}; run boot_start;fi;fi;fi;


