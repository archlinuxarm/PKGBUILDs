# Maintainer: Oleg Rakhmanov < orakhmanov [at] gmail [dot] com >

pkgname=dkms-8188eu
pkgver=v4.1.4_6773
pkgrel=3
pkgdesc="Driver for Realtek RTL8188EU chipset wireless cards"
arch=('i686' 'x86_64' 'arm' 'armv6h' 'armv7h')
url="http://www.realtek.com.tw/"
license=('GPL')
depends=('dkms' 'linux-headers')
conflicts=()
install=${pkgname}.install
options=(!strip)
_pkgname="8188eu"
source=("rtl8188eu.tar.gz::https://github.com/lwfinger/rtl8188eu/tarball/master"
		"dkms.conf")
		
md5sums=(	'SKIP'
		 '7fc48c25a0eeee98bfc6b5ffcec25926')



package() {
	
	#Change github commit id in folder name to master
	mv ${srcdir}/lwfinger-rtl8188eu-* ${srcdir}/rtl8188eu-master	

	installDir="$pkgdir/usr/src/${_pkgname}-${pkgver}"
	
	install -dm755 "$installDir"

 	cd "${srcdir}/rtl8188eu-master/"

        msg2 "Disabling Power Savings Mode"
	sed -i 's/^CONFIG_POWER_SAVING\ =\ y/CONFIG_POWER_SAVING\ =\ n/' Makefile
     	
 	for d in `find . -type d`
 	do
		install -dm755  "$installDir/$d"
	done
	
	for f in `find . -type f`
	do
		install -m644 "${srcdir}/rtl8188eu-master/$f" "$installDir/$f"
	done

        # Remove existing dkms.conf that comes from git
        rm $installDir/dkms.conf
	# Install our dkms.conf
        install -m644 "$srcdir/dkms.conf" "$installDir"

        #If building for arm, change architecture in dkms.conf
        if [ "$CARCH" == "arm" -o "$CARCH" == "armv6h" -o "$CARCH" == "armv7h" ]; then
           sed -i 's/ARCH\=i386/ARCH\=arm/g' ${installDir}/dkms.conf
        fi

        # Change firmware directory from rtlwifi to 8188eu so it doesn't
        # clash with in-tree rtlwifi driver (if available)
        sed -i 's/rtlwifi\/rtl8188eufw.bin/8188eu\/rtl8188eufw.bin/g' ${installDir}/hal/rtl8188e_hal_init.c

	mkdir -p ${pkgdir}/usr/lib/firmware/8188eu
	cp -n rtl8188eufw.bin ${pkgdir}/usr/lib/firmware/8188eu/	
}
