TARGET = JapReader
OBJS = text.o disp.o dict.o main.o

INCDIR =
CFLAGS = -G0 -Wall -O0 -std=c99 $(shell $(PSPDEV)/psp/bin/freetype-config --cflags)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS= -lpspgu -lpsppower $(shell $(PSPDEV)/psp/bin/freetype-config --libs) -lsqlite3

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = JapReader
PSP_FW_VERSION = 500
PSP_LARGE_MEMORY = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
