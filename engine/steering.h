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

  void begin(SteeringParams params, float application_time);
  void complete();

  void apply_desired_velocity(Vector_ desired_vel, Vector_ src_vel);
  void seek(Vector_ tgt, Vector_ src, Vector_ src_vel);
  void arrival(Vector_ tgt, Vector_ src, Vector_ src_vel, float slowing_dist);
  void flee(Vector_ tgt, Vector_ src, Vector_ src_vel);
  void predict(Vector prediction, Vector_ tgt, Vector_ tgt_vel, Vector_ src);
  void pursuit(Vector_ tgt, Vector_ tgt_vel, Vector_ src, Vector_ src_vel);
  void evasion(Vector_ tgt, Vector_ tgt_vel, Vector_ src, Vector_ src_vel);
  void offsetpursuit(Vector_ tgt, Vector_ tgt_vel, Vector_ src, Vector_ src_vel, float offset);
  void offsetarrival(Vector_ tgt, Vector_ src, Vector_ src_vel,
                     float offset, float slowing_dist);
  void avoidance(SteeringObstacle* objs, int nobjs, Vector_ src, Vector_ src_vel,
                 float src_radius, float src_range);
  void followpath(PathInstance* pi, Vector_ src, Vector_ src_vel, float max_offset);

  SteeringParams params;
  float application_time;
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
}

#endif
