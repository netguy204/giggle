/*
 *  This file is part of GambitGameLib.
 *
 *  GambitGameLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GambitGameLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GambitGameLib.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "utils.h"
#include "giggle.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef BUILD_SDL
#ifndef EMSCRIPTEN
#include <SDL2/SDL.h>
#endif
#endif

void* fail_exit(const char * message, ...) {
  va_list args;
  va_start(args, message);
#ifdef __ANDROID__
  __android_log_vprint(ANDROID_LOG_ERROR, "native-activity", message, args);
#else
#ifdef BUILD_SDL
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), message, args);
#ifdef EMSCRIPTEN
  fprintf(stderr, buffer);
#else
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "fail-exit",
                           buffer, NULL);
#endif
#else
  fprintf(stderr, "FAIL_EXIT: ");
  vfprintf(stderr, message, args);
#endif
#endif
  va_end(args);

  fprintf(stderr, "\n");
  fflush(stderr);
  exit(1);
  return NULL;
}

void timer_start(Timer timer) {
  gettimeofday((struct timeval*)timer, NULL);
}

long timer_elapsed_msecs(Timer timer) {
  struct timeval now;
  gettimeofday(&now, NULL);

  long last = timer->val.tv_usec / 1000 + timer->val.tv_sec * 1000;
  long next = now.tv_usec / 1000 + now.tv_sec * 1000;

  return next - last;
}

long timer_elapsed_usecs(Timer timer) {
  struct timeval now;
  gettimeofday(&now, NULL);

  long last = timer->val.tv_usec + timer->val.tv_sec * 1000000;
  long next = now.tv_usec + now.tv_sec * 1000000;

  return next - last;
}

// absolute time. zero on first call
Timer_ abs_timer;
long abs_utime() {
  if(!abs_timer.pf) {
    timer_start(&abs_timer);
    abs_timer.pf = (FILE*)&abs_timer;
  }
  return timer_elapsed_usecs(&abs_timer);
}

// resolve a filename on our path
char resolve_buffer[1024];

const char* filename_resolve(const char* filename) {
  FILE* f = fopen(filename, "r");
  if(f) {
    fclose(f);
    return filename;
  }

  snprintf(resolve_buffer, sizeof(resolve_buffer), "giggle/engine_resources/%s", filename);
  return resolve_buffer;
}

long filename_size(const char* filename) {
  filename = filename_resolve(filename);

  FILE* f = fopen(filename, "r");
  if(!f) fail_exit("file %s does not exist", filename);

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fclose(f);
  return size;
}

char* filename_slurp(const char* filename) {
  filename = filename_resolve(filename);

  long size = filename_size(filename);
  char* data = (char*)malloc(size + 1);

  FILE* f = fopen(filename, "rb");
  fread(data, 1, size, f);
  fclose(f);
  data[size] = '\0';

  return data;
}
