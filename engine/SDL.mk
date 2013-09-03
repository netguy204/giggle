CPP_SRC=testlib_sdl.cpp audio_sdl.cpp
BIN=sdlmain

SDL_BASE=$(PWD)/vender/SDL-1.2.15
SDL_ROOT=$(PWD)/vender/SDL-root

PLATFORM:=$(shell uname)
CFLAGS+=-Ivender -I$(SDL_ROOT)/include

EXE_OBJS+=glew.o

SDL_LIBS=$(SDL_ROOT)/bin/sdl-config
#SDL_STATIC_LIBS_FLAGS=-Wl,-Bstatic `$(SDL_LIBS) --static-libs` -Wl,-Bdynamic
SDL_LIBS_FLAGS=`$(SDL_LIBS) --static-libs`
SDL_LIBS_CFLAGS=`$(SDL_LIBS) --cflags`

SDL_DISABLES= --disable-video-fbcon --disable-video-directfb --disable-video-svga --disable-video-wscons --disable-video-vgl --disable-video-ps3 --disable-video-ps2gs --disable-video-x11-xme

ifeq ($(PLATFORM), Darwin)
	SDL_DISABLES+= --disable-video-x11
endif

$(SDL_LIBS):
	cd $(SDL_BASE) && ./configure --prefix=$(SDL_ROOT) $(SDL_DISABLES) && make install

SDL_INJECT=-include "SDL/SDL.h"

ifeq ($(PLATFORM), Darwin)
	LDFLAGS+= -Fvender/ $(SDL_LIBS_FLAGS)
	CFLAGS+= -DBUILD_SDL -mmacosx-version-min=10.5
	EXE_OBJS+=
	PLATFORM=macosx
else
ifeq ($(PLATFORM), MINGW32_NT-5.1)
	LDFLAGS+=-lglu32 -lopengl32 -L$(SDL_ROOT)/lib -Wl,-Bstatic -lmingw32 -lSDLmain -lSDL -mwindows -luser32 -lgdi32 -lwinmm -Wl,-Bdynamic
	CFLAGS+=-DBUILD_SDL -DWINDOWS -DGLEW_STATIC
	PLATFORM=windows
else
	LDFLAGS+= -lGL -lm -lutil $(SDL_LIBS_FLAGS)
	CFLAGS+=$(SDL_LIBS_CFLAGS) -DBUILD_SDL
	PLATFORM=linux
endif
endif

include Common.mk

# force include of SDL header so that it can do it's main redirection
# magic
gambitmain.o: gambitmain.cpp $(SDL_LIBS)
	$(CXX) -c $< $(CFLAGS) $(SDL_INJECT)

testlib_sdl.cpp: $(SDL_LIBS)

audio_test: audio_test.cpp sampler.cpp
	gcc -g -o audio_test sampler.cpp audio_test.cpp `sdl-config --libs` `sdl-config --cflags`

APP=Mosaic.app/Contents

ifeq ($(PLATFORM), macosx)
package: sdlmain resources
	cp sdlmain $(APP)/MacOS
	rm -rf $(APP)/Frameworks/SDL.framework
	cp -r vender/SDL.framework $(APP)/Frameworks/
	install_name_tool -change @rpath/SDL.framework/Versions/A/SDL "@executable_path/../Frameworks/SDL.framework/Versions/A/SDL" $(APP)/MacOS/sdlmain
	rm -rf $(APP)/Resources/*
	cp -r resources $(APP)/Resources/
	cp -r sounds $(APP)/Resources/
	cp mosaic.icns $(APP)/Resources/
	rm -rf Mosaic.app/.DS_Store
	rm -rf $(APP)/_CodeSignature
	codesign -s "Developer ID Application: Brian Taylor" Mosaic.app/
else
ifeq ($(PLATFORM), windows)
package: sdlmain resources
	cp sdlmain windist/
	rm -rf windist/resources windist/sounds
	cp -r resources windist/
	cp -r sounds windist/
endif
endif
