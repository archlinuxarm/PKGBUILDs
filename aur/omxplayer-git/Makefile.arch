include Makefile.include

CFLAGS+=-std=c++0x -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -DTARGET_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_CMAKE_CONFIG -D__VIDEOCORE4__ -U_FORTIFY_SOURCE -Wall -DHAVE_OMXLIB -DUSE_EXTERNAL_FFMPEG  -DHAVE_LIBAVCODEC_AVCODEC_H -DHAVE_LIBAVUTIL_OPT_H -DHAVE_LIBAVUTIL_MEM_H -DHAVE_LIBAVUTIL_AVUTIL_H -DHAVE_LIBAVFORMAT_AVFORMAT_H -DHAVE_LIBAVFILTER_AVFILTER_H -DHAVE_LIBSWRESAMPLE_SWRESAMPLE_H -DOMX -DOMX_SKIP64BIT -ftree-vectorize -DUSE_EXTERNAL_OMX -DTARGET_RASPBERRY_PI -DUSE_EXTERNAL_LIBBCM_HOST

LDFLAGS+=-L./ -lc -lWFC -lGLESv2 -lEGL -lbcm_host -lopenmaxil -lfreetype -lz -ldbus-1

INCLUDES+=-I./ -Ilinux

DIST ?= omxplayer-dist

SRC=linux/XMemUtils.cpp \
		utils/log.cpp \
		DynamicDll.cpp \
		utils/PCMRemap.cpp \
		utils/RegExp.cpp \
		OMXSubtitleTagSami.cpp \
		OMXOverlayCodecText.cpp \
		BitstreamConverter.cpp \
		linux/RBP.cpp \
		OMXThread.cpp \
		OMXReader.cpp \
		OMXStreamInfo.cpp \
		OMXAudioCodecOMX.cpp \
		OMXCore.cpp \
		OMXControl.cpp \
		Keyboard.cpp \
		OMXVideo.cpp \
		OMXAudio.cpp \
		OMXClock.cpp \
		File.cpp \
		OMXPlayerVideo.cpp \
		OMXPlayerAudio.cpp \
		OMXPlayerSubtitles.cpp \
		SubtitleRenderer.cpp \
		Unicode.cpp \
		Srt.cpp \
		KeyConfig.cpp \
		omxplayer.cpp

OBJS+=$(filter %.o,$(SRC:.cpp=.o))

all: omxplayer.bin

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@ -Wno-deprecated-declarations

version:
	sh gen_version.sh > version.h 

omxplayer.bin: version $(OBJS)
	$(CXX) $(LDFLAGS) -o omxplayer.bin $(OBJS) -lvchiq_arm -lvcos -lrt -lpthread -lavutil -lavcodec -lavformat -lswscale -lswresample -lpcre
	#arm-unknown-linux-gnueabi-strip omxplayer.bin

clean:
	for i in $(OBJS); do (if test -e "$$i"; then ( rm $$i ); fi ); done
	@rm -f omxplayer.old.log omxplayer.log
	@rm -f omxplayer.bin
	@rm -rf $(DIST)
	@rm -f omxplayer-dist.tar.gz

install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/doc/omxplayer
	install omxplayer $(DESTDIR)/usr/bin/
	install omxplayer.bin $(DESTDIR)/usr/bin/
	install README.md COPYING $(DESTDIR)/usr/share/doc/omxplayer/
