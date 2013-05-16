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
#ifndef GAMEEVENTS_H
#define GAMEEVENTS_H

#include "vector.h"

/* 1d inputs. Keypresses and analog triggers */
#define KEY_TABLE(m)                            \
  m(K_TAB, 9)                                   \
  m(K_ENTER, 13)                                \
  m(K_ESCAPE, 27)                               \
  m(K_SPACE, 32)                                \
  m(K_BACKSPACE, 127)                           \
  m(K_LEFTRIGHT, 133)                           \
  m(K_UPDOWN, 134)                              \
  m(K_LEFTRIGHT_SECONDARY, 135)                 \
  m(K_UPDOWN_SECONDARY, 136)                    \
  m(K_ALT, 140)                                 \
  m(K_CTRL, 141)                                \
  m(K_SHIFT, 142)                               \
  m(K_INS, 143)                                 \
  m(K_DEL, 144)                                 \
  m(K_PGDN, 145)                                \
  m(K_PGUP, 146)                                \
  m(K_HOME, 147)                                \
  m(K_END, 148)                                 \
  m(K_MOUSE1, 187)                              \
  m(K_MOUSE2, 188)                              \
  m(K_MOUSE3, 189)                              \
  m(K_MOUSE4, 190)                              \
  m(K_MOUSE5, 191)                              \
  m(K_MOUSE6, 192)                              \
  m(K_MOUSE7, 193)                              \
  m(K_MOUSE8, 194)                              \
  m(K_MWHEELDOWN, 195)                          \
  m(K_MWHEELUP, 196)                            \
  m(K_JOY1, 197)                                \
  m(K_JOY2, 198)                                \
  m(K_JOY3, 199)                                \
  m(K_JOY4, 200)                                \
  m(K_JOY5, 201)                                \
  m(K_JOY6, 202)                                \
  m(K_JOY7, 203)                                \
  m(K_JOY8, 204)                                \
  m(K_JOY9, 205)                                \
  m(K_JOY10, 206)                               \
  m(K_JOY11, 207)                               \
  m(K_JOY12, 208)                               \
  m(K_JOY13, 209)                               \
  m(K_JOY14, 210)                               \
  m(K_JOY15, 211)                               \
  m(K_JOY16, 212)                               \
  m(K_JOY17, 213)                               \
  m(K_JOY18, 214)                               \
  m(K_JOY19, 215)                               \
  m(K_JOY20, 216)                               \
  m(K_JOY21, 217)                               \
  m(K_JOY22, 218)                               \
  m(K_JOY23, 219)                               \
  m(K_JOY24, 220)                               \
  m(K_JOY25, 221)                               \
  m(K_JOY26, 222)                               \
  m(K_JOY27, 223)                               \
  m(K_PRINT_SCR, 252)                           \
  m(K_RIGHT_ALT, 253)                           \
  m(K_LAST_KEY, 254)


enum KeyNumber {
#define DEFINE_KEY_ENUM(name, value) name = value,
  KEY_TABLE(DEFINE_KEY_ENUM)
  K_MAX
};

class Binding {
 public:
  virtual ~Binding();
  virtual void activate(float value) = 0;
};

/* 0 VALUE denotes released, non-zero denotes pressed. For truly
   analog controls, this value will range from [-1, 1] and will have
   system specific dead-zone already applied. handle_key be activated
   from an arbitrary thread context. However, set_key_binding is not
   synchronized, so use caution. */
void input_init();

void handle_key(KeyNumber key, float value);
void set_key_binding(KeyNumber key, Binding* binding);
int find_keynumber(const char* name);

/* Spatial Input Table */
#define SI_TABLE(m)                             \
  m(SI_MOUSE, 1)                                \
  m(SI_TOUCH1, 2)                               \
  m(SI_TOUCH2, 3)                               \
  m(SI_TOUCH3, 4)                               \
  m(SI_TOUCH4, 5)                               \
  m(SI_TOUCH5, 6)

enum SpatialInputNumber {
#define DEFINE_SPATIAL_ENUM(name, value) name = value,
  SI_TABLE(DEFINE_SPATIAL_ENUM)
  SI_MAX
};

struct SpatialInput {
  Vector_ absolute;
  Vector_ relative;
};

class SpatialInputBinding {
 public:
  virtual ~SpatialInputBinding();
  // input pointer is valid only during the call to activate.
  virtual void activate(SpatialInput* input) = 0;
};

void handle_spatialinput(SpatialInputNumber key, SpatialInput* input);
void set_si_binding(SpatialInputNumber key, SpatialInputBinding* binding);
int find_sinumber(const char* name);

#endif
