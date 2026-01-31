.PHONY: all repl clean

default: libmd.so

ifeq ($(CC),cc)
CC = cc
endif

CFLAGS := $(filter-out -DANDROID,$(CFLAGS))

# Detect target architecture from CC for cross-compilation
# Use interpreter for ARM, JIT for x86
# Set USE_INTERP=1 to force interpreter on any platform
ifeq ($(USE_INTERP),1)
  TARGET_ARCH := interp
else ifneq (,$(findstring aarch64,$(CC)))
  TARGET_ARCH := aarch64
else ifneq (,$(findstring arm,$(CC)))
  TARGET_ARCH := arm
else
  # Default to x86 for native/unknown
  TARGET_ARCH := x86_64
endif

# Blastem defines COREOBJS and LIBOBJS which demo the key files needed, as well
# as the conditionals needed to avoid unneeded features.
BLAST_FLAGS=-Wreturn-type -Werror=return-type -Werror=implicit-function-declaration -Wno-unused-value  -Wpointer-arith -Werror=pointer-arith
BLASTOPTS_BASE=-fPIC -flto -std=gnu99 -U__ANDROID__ -DHAS_PROC -DHAVE_UNISTD_H -DDISABLE_ZLIB $(BLAST_FLAGS)
ifneq ($(filter x86_64 i686 i386,$(TARGET_ARCH)),)
  BLASTOPTS=$(BLASTOPTS_BASE) -DX86_64
else
  # Interpreter build needs NEW_CORE flag
  BLASTOPTS=$(BLASTOPTS_BASE) -DNEW_CORE
endif
BLASTOPTS_ISLIB=$(BLASTOPTS) -DIS_LIB
# EMBEDFLAGS=--std=c2x -shared -fPIC -Wfatal-errors -fvisibility=hidden -static-libgcc -O3
EMBEDFLAGS=-shared -fPIC -flto -O3 -lm
B=blastem/
BUNDLED_LIBZ:=adler32.zlib.o compress.zlib.o crc32.zlib.o deflate.zlib.o gzclose.zlib.o gzlib.zlib.o gzread.zlib.o\
	gzwrite.zlib.o infback.zlib.o inffast.zlib.o inflate.zlib.o inftrees.zlib.o trees.zlib.o uncompr.zlib.o zutil.zlib.o
NET=net.o
TERMINAL=terminal.o

ifneq ($(filter x86_64 i686 i386,$(TARGET_ARCH)),)
  # x86: use JIT
  M68KOBJS=m68k_core.o m68k_core_x86.o 68kinst.o
  Z80OBJS=z80inst.o z80_to_x86.o
  TRANSOBJS=gen.o backend.o mem.o arena.o gen_x86.o backend_x86.o
else
  # ARM/other: use C interpreter
  M68KOBJS=m68k.o 68kinst.o
  Z80OBJS=z80.o z80inst.o
  TRANSOBJS=gen.o backend.o mem.o arena.o
  INTERP_STUBS=new_core.o
endif

CONFIGOBJS=config.o tern.o util.o paths.o
AUDIOOBJS=ym2612.o ymf262.o ym_common.o psg.o wave.o flac.o vgm.o event_log.o render_audio.o rf5c164.o
COREOBJS_EXTRA=sms.o i8255.o $(Z80OBJS) 

BLASTOBJ=system.o genesis.o vdp.o io.o romdb.o hash.o xband.o realtec.o i2c.o nor.o $(M68KOBJS) \
	sega_mapper.o multi_game.o megawifi.o $(NET) serialize.o $(TERMINAL) $(CONFIGOBJS) gst.o \
	$(TRANSOBJS) $(AUDIOOBJS) saves.o jcart.o gen_player.o coleco.o pico_pcm.o ymz263b.o \
	segacd.o lc8951.o cdimage.o cdd_mcu.o cd_graphics.o cdd_fader.o sft_mapper.o mediaplayer.o \
	laseractive.o upd78k2_dis.o upd78k2.o osd_font.o pd0178.o $(BUNDLED_LIBZ) $(COREOBJS_EXTRA) stubs.o rom.db.o $(INTERP_STUBS)

libmd.so: libmd.o corelib.h $(BLASTOBJ)
	$(CC) $(EMBEDFLAGS) libmd.o $(BLASTOBJ) -o libmd.so
	cp libmd.so libapu.so

# vdp.o now builds with IS_LIB after threading guards were added
vdp.o: blastem/vdp.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@

# Try to build most object files with -DIS_LIB
%.o: blastem/%.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@

%.zlib.o: blastem/zlib/%.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@

libmd.o: libmd.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@
stubs.o: stubs.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@
new_core.o: new_core.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@

# Embed rom.db as a C string constant
rom.db.c: $(B)rom.db
	sed $(B)rom.db -e 's/"/\\"/g' -e 's/^\(.*\)$$/"\1\\n"/' -e '1s/^\(.*\)$$/const char rom_db_data[] = \1/' -e '$$s/^\(.*\)$$/\1;/' > rom.db.c

rom.db.o: rom.db.c
	$(CC) $(BLASTOPTS_ISLIB) -c $< -o $@


main: main.c libmd.so
	$(CC) main.c -lSDL2 -L. -l:libmd.so -o main 2>err.txt; wc -l err.txt

test_interp: test_interp.c libmd.so
	$(CC) test_interp.c -L. -l:libmd.so -lm -Wl,-rpath,'$$ORIGIN' -o test_interp

test_vdp: test_vdp.c libmd.so
	$(CC) test_vdp.c -L. -l:libmd.so -lm -Wl,-rpath,'$$ORIGIN' -o test_vdp

all: libmd.so main

.PHONY: libmd.js
libmd.js:
	make clean
	CC=emcc make libmd.so
	cp libmd.so libmd.js
	

.PHONY: repl
repl:
	ls *.c corelib.h | entr -c make
.PHONY: mrepl
mrepl: 
	ls *.c corelib.h | entr -c make all

clean:
	rm -f libmd.so main *.o *.zlib.o
	find . -name "*.o" -exec rm {} \;
gdb:
	LD_LIBRARY_PATH=$(shell pwd) gdb --args ./main "$(ROM)"
run:
	LD_LIBRARY_PATH=$(shell pwd) ./main "$(ROM)"
