#ifndef GIGGLE_SDL
#define GIGGLE_SDL

#include "giggle.h"

Renderer* sdl_renderer(Giggle* giggle);
Renderer* sdl_nonthreaded_renderer(Giggle* giggle);

AudioSystem* sdl_streaming_audio(Giggle* giggle);

#endif
