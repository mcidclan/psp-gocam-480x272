TARGET = target
OBJS = main.o

INCDIR = ./include
CFLAGS = -G0 -Os -Wall -Wextra -Werror 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspusbcam -lpspjpeg -lpspusb -lpsppower

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = gocam 512x272

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
