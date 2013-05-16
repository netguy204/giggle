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
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <sys/time.h>

typedef struct Timer_ {
  struct timeval val;
  FILE* pf;
} *Timer;

void timer_start(Timer timer);
long timer_elapsed_msecs(Timer timer);
long timer_elapsed_usecs(Timer timer);
long abs_utime();

#define DISABLE_PROFILING

#ifndef DISABLE_PROFILING
#define PROFILE_START(timer, message) do {                              \
    if(!(timer)->pf) {                                                  \
      (timer)->pf = fopen(message, "w");                                \
    }                                                                   \
    timer_start(timer);                                                 \
  } while(0)

#define PROFILE_END(timer) do {                                  \
    long usecs = timer_elapsed_usecs(timer);                     \
    fprintf((timer)->pf, "%ld %ld\n", abs_utime(), usecs);       \
  } while(0)
#else
#define PROFILE_START(timer, message)
#define PROFILE_END(timer)
#endif

long filename_size(const char* filename);
char* filename_slurp(const char* filename);

// provided by system specific lib
FILE* nativeOpen(const char* fname);
void* fail_exit(const char * message, ...);


#ifdef __ANDROID__
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#else

#define LOGI(...) fprintf(stderr, "INFO: " __VA_ARGS__); fprintf(stderr, "\n")
#define LOGW(...) fprintf(stderr, "WARNING: " __VA_ARGS__); fprintf(stderr, "\n")

#endif

#endif
