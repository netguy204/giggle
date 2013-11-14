CPP_SRC=webmain.cpp audio_sdl.cpp
BIN=webmain.bin

CC=emcc
CXX=em++

EMFLAGS?=-s FORCE_ALIGNED_MEMORY=1 -s DOUBLE_MODE=0 -s OUTLINING_LIMIT=100000
CFLAGS?=-O2

CFLAGS+=#--js-library browser.js

LDFLAGS+= -lGL -lm -lutil -ldl
CFLAGS+=-DBUILD_SDL -DEMSCRIPTEN

PLATFORM=linux

EMFLAGS+=-s TOTAL_MEMORY=67108864 # -s LEGACY_GL_EMULATION=1

main.js: webmain.bc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(EMFLAGS)

main.html: webmain.bc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS) $(EMFLAGS)

webmain.bc: webmain.bin
	cp $< $@

include Common.mk
