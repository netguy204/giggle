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
#include "input.h"
#include "config.h"

#include <math.h>
#include <string.h>

Binding::~Binding() {}

class Key {
public:
  Key()
    : down(0), value(0.0), binding(NULL) {
  }

  int down;
  float value;
  Binding* binding;
};

class SIKey {
public:
  SIKey()
    : binding(NULL) {
  }

  SpatialInput value;
  SpatialInputBinding* binding;
};

static Key* keys = NULL;
static SIKey* sikeys = NULL;

void input_init() {
  keys = new Key[K_MAX];
  sikeys = new SIKey[SI_MAX];
}

void handle_key(KeyNumber keyn, float value) {
  Key* key = &keys[keyn];
  if(fabs(value) > 0) {
    key->down = 1;
    key->value = value;
  } else {
    key->down = 0;
    key->value = 0.0f;
  }

  if(key->binding) {
    key->binding->activate(value);
  }
}

void set_key_binding(KeyNumber keyn, Binding* binding) {
  Key* key = &keys[keyn];
  if(key->binding) {
    delete key->binding;
  }
  key->down = 0;
  key->value = 0.0f;
  key->binding = binding;
}

int find_keynumber(const char* name) {
  if(!name || !name[0]) return -1;

  if(!name[1]) {
    // assume ascii
    return name[0];
  }

#define GEN_NAME_COMPARE(n, v)                  \
  if(strcmp(#n, name) == 0) {                   \
    return v;                                   \
  }

  KEY_TABLE(GEN_NAME_COMPARE);
  return -1;
}

SpatialInputBinding::~SpatialInputBinding() { }

void handle_spatialinput(SpatialInputNumber keyn, SpatialInput* input) {
  SIKey* key = &sikeys[keyn];
  key->value = *input;

  if(key->binding) {
    key->binding->activate(&key->value);
  }
}

void set_si_binding(SpatialInputNumber keyn, SpatialInputBinding* binding) {
  SIKey* key = &sikeys[keyn];
  if(key->binding) {
    delete key->binding;
  }
  key->binding = binding;
}

int find_sinumber(const char* name) {
  if(!name) return -1;
#define GEN_SNAME_COMPARE(n, v)                 \
  if(strcmp(#n, name) == 0) {                   \
    return v;                                   \
  }

  SI_TABLE(GEN_SNAME_COMPARE);
  return -1;
}
