#!/bin/bash

srcdir=$1
read -r -d '' dt_list

cat <<-ITS_HEADER_END
/dts-v1/;

/ {
    description = "Chrome OS kernel image with one or more FDT blobs";
    images {
        kernel@1{
            description = "kernel";
            data = /incbin/("arch/arm64/boot/Image");
            type = "kernel_noload";
            arch = "arm64";
            os = "linux";
            compression = "none";
            load = <0>;
            entry = <0>;
        };
ITS_HEADER_END

its_entry_count=1
for dt in ${dt_list}; do
	dts_path=${srcdir}/*/arch/arm64/boot/dts/*/${dt}.dts
	! ls $dts_path >/dev/null && { echo "Error: dt not found $dt" >&2; exit 1; }
	dt_dir=$(basename $(dirname ${dts_path}))
	cat <<-FDT_END
	        fdt@${its_entry_count}{
	            description = "${dt}.dtb";
	            data = /incbin/("arch/arm64/boot/dts/${dt_dir}/${dt}.dtb");
	            type = "flat_dt";
	            arch = "arm64";
	            compression = "none";
	            hash@1{
	                algo = "sha1";
	            };
	        };
	FDT_END
	let its_entry_count=${its_entry_count}+1
done

cat <<-ITS_MIDDLE_END
    };
    configurations {
        default = "conf@1";
ITS_MIDDLE_END

for((i=1;i<${its_entry_count};i++)); do
	cat <<-ITS_CONF_END
	        conf@${i}{
	            kernel = "kernel@1";
	            fdt = "fdt@${i}";
	        };
	ITS_CONF_END
done

cat <<-ITS_FOOTER_END
    };
};
ITS_FOOTER_END
