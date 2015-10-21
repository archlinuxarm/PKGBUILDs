all: simg2img make_ext4fs

install:
	install -m755 -d $(DESTDIR)/usr/bin
	install -m755 -t $(DESTDIR)/usr/bin simg2img make_ext4fs

clean:
	rm -f **/*.o

.PHONY: clean

SIMG2IMG_SRCS += core/libsparse/simg2img.c
SIMG2IMG_SRCS += core/libsparse/backed_block.c
SIMG2IMG_SRCS += core/libsparse/output_file.c
SIMG2IMG_SRCS += core/libsparse/sparse.c
SIMG2IMG_SRCS += core/libsparse/sparse_crc32.c
SIMG2IMG_SRCS += core/libsparse/sparse_err.c
SIMG2IMG_SRCS += core/libsparse/sparse_read.c

SIMG2IMG_CFLAGS  += -DHAVE_OFF64_T=1 -std=gnu99 -I core/libsparse/include
SIMG2IMG_LIBS += -lz

simg2img: $(SIMG2IMG_SRCS)
	$(CC) -o $@ $(CFLAGS) $(SIMG2IMG_CFLAGS) $(LDFLAGS) $(SIMG2IMG_SRCS) $(SIMG2IMG_LIBS)

MAKE_EXT4FS_SRCS += extras/ext4_utils/make_ext4fs_main.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/make_ext4fs.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/ext4fixup.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/ext4_utils.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/allocate.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/contents.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/extent.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/indirect.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/uuid.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/sha1.c
MAKE_EXT4FS_SRCS += extras/ext4_utils/wipe.c
MAKE_EXT4FS_SRCS += core/libsparse/backed_block.c
MAKE_EXT4FS_SRCS += core/libsparse/output_file.c
MAKE_EXT4FS_SRCS += core/libsparse/sparse.c
MAKE_EXT4FS_SRCS += core/libsparse/sparse_crc32.c
MAKE_EXT4FS_SRCS += core/libsparse/sparse_err.c
MAKE_EXT4FS_SRCS += core/libsparse/sparse_read.c
MAKE_EXT4FS_SRCS += libselinux/src/callbacks.c
MAKE_EXT4FS_SRCS += libselinux/src/check_context.c
MAKE_EXT4FS_SRCS += libselinux/src/freecon.c
MAKE_EXT4FS_SRCS += libselinux/src/init.c
MAKE_EXT4FS_SRCS += libselinux/src/label.c
MAKE_EXT4FS_SRCS += libselinux/src/label_android_property.c
MAKE_EXT4FS_SRCS += libselinux/src/label_file.c

MAKE_EXT4FS_CFLAGS  += -DHAVE_OFF64_T=1 -std=gnu99 -I core/libsparse/include -I libselinux/include -I core/include -I extras/ext4_utils
MAKE_EXT4FS_LIBS += -lz

make_ext4fs: $(MAKE_EXT4FS_SRCS)
	$(CC) -o $@ $(CFLAGS) $(MAKE_EXT4FS_CFLAGS) $(LDFLAGS) $(MAKE_EXT4FS_SRCS) $(MAKE_EXT4FS_LIBS)
