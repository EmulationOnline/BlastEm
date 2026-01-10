.PHONY: all repl clean

ifeq ($(CC),cc)
CC = cc
endif

# Blastem defines COREOBJS and LIBOBJS which demo the key files needed, as well
# as the conditionals needed to avoid unneeded features.
BLAST_FLAGS=-Wreturn-type -Werror=return-type -Werror=implicit-function-declaration -Wno-unused-value  -Wpointer-arith -Werror=pointer-arith 
BLASTOPTS=-fPIC -flto -std=gnu99 -DHAS_PROC -DHAVE_UNISTD_H -DX86_64 -DDISABLE_ZLIB $(BLAST_FLAGS)
BLASTOPTS_ISLIB=$(BLASTOPTS) -DIS_LIB
# EMBEDFLAGS=--std=c2x -shared -fPIC -Wfatal-errors -fvisibility=hidden -static-libgcc -O3
EMBEDFLAGS=--std=c2x -shared -fPIC -Wfatal-errors -static-libgcc -O3
B=blastem/
BUNDLED_LIBZ:=adler32.zlib.o compress.zlib.o crc32.zlib.o deflate.zlib.o gzclose.zlib.o gzlib.zlib.o gzread.zlib.o\
	gzwrite.zlib.o infback.zlib.o inffast.zlib.o inflate.zlib.o inftrees.zlib.o trees.zlib.o uncompr.zlib.o zutil.zlib.o
NET=
TERMINAL=terminal.o
M68KOBJS=m68k.o
AUDIOOBJS=ym2612.o ymf262.o ym_common.o psg.o wave.o flac.o vgm.o event_log.o render_audio.o rf5c164.o
BLASTOBJ=system.o genesis.o vdp.o io.o romdb.o hash.o xband.o realtec.o i2c.o nor.o $(M68KOBJS) \
	sega_mapper.o multi_game.o megawifi.o $(NET) serialize.o $(TERMINAL) $(CONFIGOBJS) gst.o \
	$(TRANSOBJS) $(AUDIOOBJS) saves.o jcart.o gen_player.o coleco.o pico_pcm.o ymz263b.o \
	segacd.o lc8951.o cdimage.o cdd_mcu.o cd_graphics.o cdd_fader.o sft_mapper.o mediaplayer.o \
	laseractive.o upd78k2_dis.o upd78k2.o osd_font.o pd0178.o $(BUNDLED_LIBZ) z80.o

libmd.so: libmd.c corelib.h $(BLASTOBJ)
	$(CC) $(EMBEDFLAGS) libmd.c $(BLASTOBJ) -o libmd.so

# Some don't build with IS_LIB, build without that flag.
vdp.o: blastem/vdp.c
	$(CC) $(BLASTOPTS) -c $< -o $@

# Try to build most object files with -DIS_LIB
%.o: blastem/%.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@

%.zlib.o: blastem/zlib/%.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@


main: main.c libmd.so
	$(CC) main.c -lSDL2 -L. -l:libmd.so -o main 2>err.txt; wc -l err.txt

all: libmd.so main

.PHONY: repl
repl:
	ls *.c corelib.h | entr -c make
.PHONY: mrepl
mrepl: 
	ls *.c corelib.h | entr -c make all

clean:
	rm -f libmd.so main *.o
gdb:
	LD_LIBRARY_PATH=$(shell pwd) gdb --args ./main "$(ROM)"
run:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)"
