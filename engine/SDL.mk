CPP_SRC=giggle_sdl.cpp audio_sdl.cpp sampler.cpp audio.cpp
BINS=sdlmain.bin sdlmain_nt.bin

SDL_BASE=$(PWD)/vender/SDL2-2.0.1
SDL_ROOT=$(PWD)/vender/SDL-root

PLATFORM:=$(shell uname)
CFLAGS+=-Ivender -I$(SDL_ROOT)/include -fdata-sections -ffunction-sections -DBUILD_SDL

EXE_OBJS+=glew.o

SDL_LIBS=$(SDL_ROOT)/bin/sdl2-config
#SDL_STATIC_LIBS_FLAGS=-Wl,-Bstatic `$(SDL_LIBS) --static-libs` -Wl,-Bdynamic
SDL_LIBS_FLAGS=`$(SDL_LIBS) --static-libs`
SDL_LIBS_CFLAGS=`$(SDL_LIBS) --cflags`

SDL_DISABLES= --disable-video-fbcon --disable-video-directfb --disable-video-svga --disable-video-wscons --disable-video-vgl --disable-video-ps3 --disable-video-ps2gs --disable-video-x11-xme

ifeq ($(PLATFORM), Darwin)
SDL_DISABLES+= --disable-video-x11
endif

$(SDL_LIBS):
	cd $(SDL_BASE) && ./configure --prefix=$(SDL_ROOT) $(SDL_DISABLES) && make install
	rm -f $(SDL_ROOT)/lib/*.dll.a

ifeq ($(PLATFORM), Darwin)
# Mac build flags
LDFLAGS+=-Fvender/ $(SDL_LIBS_FLAGS) -dead_strip
CFLAGS+=-mmacosx-version-min=10.5
EXE_OBJS+=
PLATFORM=macosx
else
# now we're building with gcc
LDFLAGS+=-Wl,--gc-sections


ifeq ($(PLATFORM), MINGW32_NT-5.1)
# Windows
LDFLAGS+=-lglu32 -lopengl32 $(SDL_LIBS_FLAGS)
CFLAGS+=-DWINDOWS -DGLEW_STATIC
PLATFORM=windows

else

# Linux
LDFLAGS+= -lGL -lm -lutil $(SDL_LIBS_FLAGS)
CFLAGS+=$(SDL_LIBS_CFLAGS)
PLATFORM=linux
endif
endif

echoecho:
	echo $(PLATFORM)



include Common.mk

$(BINS): $(SDL_LIBS)

audio_test: audio_test.cpp sampler.cpp
	gcc -g -o audio_test sampler.cpp audio_test.cpp `sdl2-config --libs` `sdl2-config --cflags`

APP=Mosaic.app/Contents

ifeq ($(PLATFORM), macosx)
package: sdlmain.bin resources
	cp sdlmain.bin $(APP)/MacOS
	rm -rf $(APP)/Frameworks/SDL.framework
	cp -r vender/SDL.framework $(APP)/Frameworks/
	install_name_tool -change @rpath/SDL.framework/Versions/A/SDL "@executable_path/../Frameworks/SDL.framework/Versions/A/SDL" $(APP)/MacOS/sdlmain.bin
	rm -rf $(APP)/Resources/*
	cp -r resources $(APP)/Resources/
	cp -r sounds $(APP)/Resources/
	cp mosaic.icns $(APP)/Resources/
	rm -rf Mosaic.app/.DS_Store
	rm -rf $(APP)/_CodeSignature
	codesign -s "Developer ID Application: Brian Taylor" Mosaic.app/
else
ifeq ($(PLATFORM), windows)
package: sdlmain.bin resources
	cp sdlmain.bin windist/
	rm -rf windist/resources windist/sounds
	cp -r resources windist/
	cp -r sounds windist/
endif
endif
