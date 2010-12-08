###############################################################################
#
# General Definitions
#
###############################################################################

TARGET_DIR = /usr/local/sbin
SCRIPT_DIR = /etc

LIB_DIRS   = 

INC_DIRS   = 

CC_DEBUG   = -g
CFLAGS     = $(CC_DEBUG) $(INC_DIRS) -Wall
CC         = gcc
LD         = $(CC)

###############################################################################
#
# Main Dependencies
#
###############################################################################

TARGET  = hd-idle

LIBS    = 

SRCS    = hd-idle.c

OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -g root -o root $(TARGET) $(TARGET_DIR)

install-debian:
	install -g root -o root $(TARGET) /usr/sbin
	install -g root -o root scripts/debian/init.d/hd-idle /etc/init.d
	install -g root -o root scripts/debian/default/hd-idle /etc/default
	@echo
	@echo Please run \"update-rc.d hd-idle defaults\" to start hd-idle automatically
	@echo and check /etc/default/hd-idle for configuration information

hd-idle.o:     hd-idle.c

$(TARGET): $(OBJS)
	$(LD) $(CC_DEBUG) -o $(TARGET) $(OBJS) $(LIB_DIRS) $(LIBS)


