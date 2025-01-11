TARGET=AsmSample

AS=vasmm68k_mot
CC=m68k-amigaos-gcc

SOURCES=AsmSample.s
#		debug.c \
		

ASFLAGS=-m68080 -quiet -Fhunk -I ../../include -I /opt/amiga/m68k-amigaos/ndk-include

#CFLAGS=-std=c99 -Ofast -fomit-frame-pointer -m68080 -mhard-float -Wall -Wno-unused-function -DAMIGA=1 -I ../../include

LFLAGS=-L. -noixemul -nostartfiles -nostdlib -e StartCode
LDLIBS=-lamiga -lm

#OBJSC=$(SOURCES:.c=.o)
#OBJS=$(OBJSC:.s=.o)
OBJS=$(SOURCES:.s=.o)

all: $(SOURCES) $(TARGET) Makefile

$(TARGET): $(OBJS) Makefile
	$(CC) $(LFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

purge: clean
	rm -f $(TARGET)

strip:
	m68k-amigaos-strip $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
