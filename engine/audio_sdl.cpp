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
#include <SDL2/SDL_audio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "audio.h"
#include "memlib.h"
#include "threadlib.h"

#define NUM_SAMPLES 4096

char * audio_pre_buffer;
CircularBuffer audio_buffer;
Thread audio_thread;
Mutex audio_mutex;

int buffer_samples_can_write() {
  int result;
  mutex_lock(audio_mutex);
  result = circularbuffer_bytes_writable(audio_buffer);
  mutex_unlock(audio_mutex);
  return result / 2;
}

void* audio_exec(void* udata) {
  float buffer_time_us = (float)(1e6 * NUM_SAMPLES) / SAMPLE_FREQ;
  int sample_thresh = NUM_SAMPLES / 2;
  while(1) {
    // wait till we can write a good chunk
    int nsamples;
    while((nsamples = buffer_samples_can_write()) < sample_thresh) {
      usleep(buffer_time_us / 4);
    }

    // compute the audio that we know we need
    audio_fill_buffer((int16_t*)audio_pre_buffer, nsamples);

    // now lock the circular buffer and copy
    mutex_lock(audio_mutex);

    int s1, s2;
    char *b1, *b2;
    int nbytes = nsamples * 2;
    circularbuffer_write_buffers(audio_buffer, &b1, &s1, &b2, &s2,
                                 nbytes);

    int to_write = MIN(nbytes, s1);
    memcpy(b1, audio_pre_buffer, to_write);
    nbytes -= to_write;

    if(nbytes > 0) {
      to_write = MIN(nbytes, s2);
      memcpy(b2, &audio_pre_buffer[s1], to_write);
    }

    mutex_unlock(audio_mutex);

    // don't immediately bang on the lock
    usleep(buffer_time_us / 4);
  }
}

/*
void fill_audio(void *udata, Uint8 *stream, int len) {
  int s1, s2;
  char *b1, *b2;

  mutex_lock(audio_mutex);
  circularbuffer_read_buffers(audio_buffer, &b1, &s1, &b2, &s2,
                              len);

  int tocopy = MIN(len, s1);
  memcpy(stream, b1, tocopy);
  len -= tocopy;

  if(len > 0) {
    tocopy = MIN(len, s2);
    memcpy(stream, b2, tocopy);
  }

  mutex_unlock(audio_mutex);
}
*/

void fill_audio(void *udata, Uint8 *stream, int len) {
  audio_fill_buffer((int16_t*)stream, len / 2);
}

void native_audio_init() {
  // twice the number of samples in the sdl buffer (2 bytes per
  // channel per sample)
  int buffer_size = NUM_SAMPLES * 2 * 2 * 2;
  audio_buffer = circularbuffer_make(buffer_size);
  audio_pre_buffer = (char*)malloc(buffer_size);

  audio_mutex = mutex_create();
  //audio_thread = thread_create(audio_exec, NULL);

  SDL_AudioSpec wanted;

  // Set the audio format
  wanted.freq = SAMPLE_FREQ;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 2;    // 1 = mono, 2 = stereo
  wanted.samples = NUM_SAMPLES;  // Good low-latency value for callback
  wanted.padding = 0;
  wanted.callback = fill_audio;
  wanted.userdata = NULL;

  // Open the audio device, forcing the desired format
  if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
    fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
    exit(-1);
  }

  SDL_PauseAudio(0);
}
