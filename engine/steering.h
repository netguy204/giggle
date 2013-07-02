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
#ifndef STEERING_H
#define STEERING_H

#include "vector.h"
#include "pathfinder.h"
#include "ooc.h"
#include "gameobject.h"

struct SteeringParams {
  float force_max;
  float speed_max;
  float old_angle;
  float application_time;
};

struct SteeringObstacle {
  struct Vector_ center;
  struct Vector_ perp_offset; // internal
  float radius;
  float cylinder_dist; // used internally
};

class Steering : public Object {
 public:
  OBJECT_PROTO(Steering);

  Steering(void* empty);

  void begin(SteeringParams params);
  void complete();

  void apply_desired_velocity(Vector desired_vel, Vector src_vel);
  void seek(Vector tgt, Vector src, Vector src_vel);
  void arrival(Vector tgt, Vector src, Vector src_vel, float slowing_dist);
  void flee(Vector tgt, Vector src, Vector src_vel);
  void predict(Vector prediction, Vector tgt, Vector tgt_vel, Vector src);
  void pursuit(Vector tgt, Vector tgt_vel, Vector src, Vector src_vel);
  void evasion(Vector tgt, Vector tgt_vel, Vector src, Vector src_vel);
  void offsetpursuit(Vector tgt, Vector tgt_vel, Vector src, Vector src_vel, float offset);
  void offsetarrival(Vector tgt, Vector src, Vector src_vel,
                     float offset, float slowing_dist);
  void avoidance(SteeringObstacle* objs, int nobjs, Vector src, Vector src_vel,
                 float src_radius, float src_range);
  int followpath(TileMap map, PathInstance path, Vector src,
                 Vector src_vel, float max_offset);

  SteeringParams params;
  Vector_ force;
  int computed;
};

template<>
inline void LCpush<SteeringParams>(lua_State* L, SteeringParams params) {
  lua_newtable(L);

  lua_pushnumber(L, params.force_max);
  lua_setfield(L, -2, "force_max");

  lua_pushnumber(L, params.speed_max);
  lua_setfield(L, -2, "speed_max");

  lua_pushnumber(L, params.old_angle);
  lua_setfield(L, -2, "old_angle");

  lua_pushnumber(L, params.application_time);
  lua_setfield(L, -2, "application_time");
}

template<>
inline void LCcheck<SteeringParams>(lua_State* L, SteeringParams* params, int pos) {

  lua_getfield(L, pos, "force_max");
  params->force_max = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, pos, "speed_max");
  params->speed_max = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, pos, "old_angle");
  params->old_angle = lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_getfield(L, pos, "application_time");
  params->application_time = lua_tonumber(L, -1);
  lua_pop(L, 1);
}

#endif
