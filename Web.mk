CPP_SRC=testlib_sdl.cpp audio_sdl.cpp
BIN=sdlmain

CFLAGS+=-Ivender #--js-library browser.js

LDFLAGS+= -lGL -lm -lutil -ldl
CFLAGS+=-DBUILD_SDL
SDL_INJECT=-include "SDL/SDL.h"
PLATFORM=linux

main.html: sdlmain.bc
	$(CC) $< -o $@

main.js: sdlmain.bc
	$(CC) $< -o $@

sdlmain.bc: sdlmain
	cp $< $@

include Common.mk

# force include of SDL header so that it can do it's main redirection
# magic
gambitmain.o: gambitmain.cpp
	$(CXX) -c $< $(SDL_INJECT)
