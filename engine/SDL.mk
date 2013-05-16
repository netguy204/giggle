CPP_SRC=testlib_sdl.cpp audio_sdl.cpp
BIN=sdlmain

CFLAGS+=-Ivender

EXE_OBJS+=glew.o

PLATFORM:=$(shell uname)
ifeq ($(PLATFORM), Darwin)
	LDFLAGS+= -Fvender/ -framework OpenGL -framework SDL -framework Cocoa -ldl
	CFLAGS+= -Ivender/SDL.framework/Headers -DBUILD_SDL -mmacosx-version-min=10.5
	SDL_INJECT=-Ivender/SDL.framework/Headers -include "SDL.h"
	EXE_OBJS+=SDLMain.o
	PLATFORM=macosx
else
ifeq ($(PLATFORM), MINGW32_NT-5.1)
	SDL_LIBS=`vender/SDL-1.2.15.msys/bin/sdl-config --libs`
	LDFLAGS+= -Lvender/SDL-1.2.15.msys/lib -Lvender/pthreads.msys/lib -lglu32 -lopengl32 $(SDL_LIBS)
	CFLAGS+= -Ivender/SDL-1.2.15.msys/include -Ivender/pthreads.msys/include -DBUILD_SDL -DWINDOWS -DGLEW_STATIC
	SDL_INJECT=-include "SDL/SDL.h" -Ivender/SDL-1.2.15.msys/include
	PLATFORM=windows
else
	LDFLAGS+= -lGL -lm -lutil `sdl-config --libs` -ldl
	CFLAGS+=`sdl-config --cflags` -DBUILD_SDL
	SDL_INJECT=-include "SDL/SDL.h"
	PLATFORM=linux
endif
endif

include Common.mk

# force include of SDL header so that it can do it's main redirection
# magic
gambitmain.o: gambitmain.cpp
	$(CXX) -c $< $(SDL_INJECT)

SDLMain.o: SDLMain.m
	$(CXX) -c $< -Ivender/SDL.framework/Headers

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
