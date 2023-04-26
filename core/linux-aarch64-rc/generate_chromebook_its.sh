#!/bin/bash

image=$1
arch=$2
compression=$3
read -a dtb_list

cat <<-ITS_HEADER_END
/dts-v1/;

/ {
    description = "Chrome OS kernel image with one or more FDT blobs";
    images {
        kernel {
            description = "kernel";
            data = /incbin/("${image}");
            type = "kernel_noload";
            arch = "${arch}";
            os = "linux";
            compression = "${compression}";
            load = <0>;
            entry = <0>;
        };
ITS_HEADER_END

for i in ${!dtb_list[@]}; do
	dtb=${dtb_list[${i}]}
	cat <<-FDT_END
	        fdt-$(expr ${i} + 1) {
	            description = "$(basename ${dtb})";
	            data = /incbin/("${dtb}");
	            type = "flat_dt";
	            arch = "${arch}";
	            compression = "none";
	            hash {
	                algo = "sha1";
	            };
	        };
	FDT_END
done

cat <<-ITS_MIDDLE_END
    };
    configurations {
        default = "conf-1";
ITS_MIDDLE_END

for i in "${!dtb_list[@]}"; do
	compat_line=""
	dtb_uncompressed=$(echo ${dtb_list[${i}]} | sed "s/\(\.dtb\).*/\1/g")
	for compat in $(fdtget "${dtb_uncompressed}" / compatible); do
		compat_line+="\"${compat}\","
	done
	cat <<-ITS_CONF_END
	        conf-$(expr ${i} + 1) {
	            kernel = "kernel";
	            fdt = "fdt-$(expr ${i} + 1)";
	            compatible = ${compat_line%,};
	        };
	ITS_CONF_END
done

cat <<-ITS_FOOTER_END
    };
};
ITS_FOOTER_END
