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
#ifndef SOUNDMGR_H
#define SOUNDMGR_H

#include "ooc.h"
#include "threadlib.h"
#include "audio.h"

#include <stdint.h>
#include <map>

enum SoundState {
  SOUND_LOADING,
  SOUND_LOADED,
  SOUND_PLAYING,
};

enum SoundKind {
  SOUND_PRELOAD,
  SOUND_STREAM,
};

enum SoundChannels {
  CHANNEL_EVENT,
  CHANNEL_FOLEY,
  CHANNEL_AMBIANCE,
  CHANNEL_STREAM,
  CHANNEL_MAX,
};

class MSound : public Sound {
public:
  virtual ~MSound();

  long nsamples;
  int16_t* buffer;
};

typedef std::map<const char*, Sound*> NameToSound;

class SoundMgr {
public:
  SoundMgr();
  ~SoundMgr();

  Sound* get_sync(const char* sound, float scale);
  AudioHandle* play(Sound* sound, int channel);
  AudioHandle* stream(const char* sound, long start_sample);

  int channel_next_free[CHANNEL_MAX];
  NameToSound name_to_sound;
};

#endif
