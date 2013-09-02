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
#include "steering.h"
#include "config.h"

#include <math.h>

OBJECT_IMPL(Steering, Object);
OBJECT_PROPERTY(Steering, params);
OBJECT_PROPERTY(Steering, force);
OBJECT_PROPERTY(Steering, computed);

Steering::Steering(void* empty) {}

void Steering::begin(SteeringParams p) {
  params = p;
  vector_zero(&force);
  computed = 0;
}
OBJECT_METHOD(Steering, begin, void, (SteeringParams));

void Steering::complete() {
  // clamp the force
  vector_clamp(&force, &force, params.force_max);
}
OBJECT_METHOD(Steering, complete, void, ());

void Steering::apply_desired_velocity(Vector_ desired_vel, Vector_ src_vel) {
  vector_sub(&force, &desired_vel, &src_vel);
  computed = 1;
}
OBJECT_METHOD(Steering, apply_desired_velocity, void, (Vector_, Vector_));

void Steering::seek(Vector_ tgt, Vector_ src, Vector_ src_vel) {
  struct Vector_ desired_vel;
  vector_direction_scaled(&desired_vel, &tgt, &src, params.speed_max);
  apply_desired_velocity(desired_vel, src_vel);
}
OBJECT_METHOD(Steering, seek, void, (Vector_, Vector_, Vector_));

void Steering::arrival(Vector_ tgt, Vector_ src, Vector_ src_vel, float slowing_dist) {
  struct Vector_ to_target;
  vector_sub(&to_target, &tgt, &src);

  float mag = vector_mag(&to_target);
  if(fabsf(mag) < 0.0001) return;

  float speed = MIN(params.speed_max, (mag / slowing_dist) * params.speed_max);

  struct Vector_ desired_vel;
  vector_scale(&desired_vel, &to_target, speed / mag);
  apply_desired_velocity(desired_vel, src_vel);
}
OBJECT_METHOD(Steering, arrival, void, (Vector_, Vector_, Vector_, float));

void Steering::flee(Vector_ tgt, Vector_ src, Vector_ src_vel) {
  struct Vector_ desired_vel;
  vector_direction_scaled(&desired_vel, &src, &tgt, params.speed_max);
  apply_desired_velocity(desired_vel, src_vel);
}
OBJECT_METHOD(Steering, flee, void, (Vector_, Vector_, Vector_));

void Steering::predict(Vector prediction, Vector_ tgt, Vector_ tgt_vel, Vector_ src) {
  // assume the intercept time is the time it would take us to get to
  // where the target is now
  struct Vector_ to_target;
  vector_sub(&to_target, &tgt, &src);

  float dt = vector_mag(&to_target) / params.speed_max;

  // project the target to that position
  struct Vector_ dx;
  vector_scale(&dx, &tgt_vel, dt);
  vector_add(prediction, &tgt, &dx);
}

void Steering::pursuit(Vector_ tgt, Vector_ tgt_vel, Vector_ src, Vector_ src_vel) {
  struct Vector_ prediction;
  predict(&prediction, tgt, tgt_vel, src);
  seek(prediction, src, src_vel);
}
OBJECT_METHOD(Steering, pursuit, void, (Vector_, Vector_, Vector_, Vector_));

void Steering::evasion(Vector_ tgt, Vector_ tgt_vel, Vector_ src, Vector_ src_vel) {
  struct Vector_ prediction;
  predict(&prediction, tgt, tgt_vel, src);
  flee(prediction, src, src_vel);
}
OBJECT_METHOD(Steering, evasion, void, (Vector_, Vector_, Vector_, Vector_));

void Steering::offsetpursuit(Vector_ tgt, Vector_ tgt_vel,
                             Vector_ src, Vector_ src_vel, float offset) {

  struct Vector_ prediction;
  predict(&prediction, tgt, tgt_vel, src);

  struct Vector_ p2s;
  vector_sub(&p2s, &src, &prediction);
  float mag = vector_mag(&p2s);

  vector_scale(&p2s, &p2s, offset / mag);

  struct Vector_ offset_tgt;
  vector_add(&offset_tgt, &prediction, &p2s);

  seek(offset_tgt, src, src_vel);
}
OBJECT_METHOD(Steering, offsetpursuit, void, (Vector_, Vector_, Vector_, Vector_, float));

void Steering::offsetarrival(Vector_ tgt, Vector_ src, Vector_ src_vel,
                             float offset, float slowing_dist) {
  struct Vector_ to_src;

  vector_sub(&to_src, &src, &tgt);
  float mag = vector_mag(&to_src);
  if(fabsf(mag) > 0.0001f) {
    vector_scale(&to_src, &to_src, offset / mag);
  } else {
    // pick an arbitrary departure direction
    to_src.x = 0.0f;
    to_src.y = 1.0f;
  }

  struct Vector_ offset_tgt;
  vector_add(&offset_tgt, &tgt, &to_src);

  arrival(offset_tgt, src, src_vel, slowing_dist);
}
OBJECT_METHOD(Steering, offsetarrival, void, (Vector_, Vector_, Vector_, float, float))

int Steering::followpath(TileMap* map, PathInstance pi, Vector_ src, Vector_ src_vel,
                         float max_offset) {
  struct Vector_ projobj;
  struct Vector_ src_vel_offset;
  vector_scale(&src_vel_offset, &src_vel, params.application_time);
  vector_add(&projobj, &src, &src_vel_offset);

  // find the closest point on the path
  struct Vector_ tgt;
  float dist;
  int step = path_next_closest_point(&tgt, map, pi, &projobj, &dist);

  // close enough?
  if(dist <= max_offset) {
    return step;
  }

  // steer towards the point
  seek(tgt, src, src_vel);
  return step;
}

void Steering::avoidance(SteeringObstacle* objs, int nobjs, Vector_ src, Vector_ src_vel,
                         float src_radius, float src_range) {

  float speed = vector_mag(&src_vel);
  if(speed < 0.01) {
    computed = 0;
    return; // don't do anything
  }

  struct Vector_ src_vel_norm;
  vector_scale(&src_vel_norm, &src_vel, 1.0f / speed);

  int ii;
  SteeringObstacle* closest = NULL;
  for(ii = 0; ii < nobjs; ++ii) {
    // put the object in our frame
    SteeringObstacle* obj = &objs[ii];

    struct Vector_ objpos;
    vector_sub(&objpos, &obj->center, &src);

    // project along our direction of travel. bail if behind us
    struct Vector_ objproj;
    float projdist = vector_project2(&objproj, &objpos, &src_vel_norm);
    if(projdist < 0) continue;

    // is that in range?
    if(projdist > src_range) continue;

    // does the cylinder intersect it?
    struct Vector_ perp_offset;
    vector_sub(&perp_offset, &objproj, &objpos);
    if(vector_mag(&perp_offset) > (src_radius + obj->radius)) continue;

    // this is an intersection. store the projection distance
    obj->cylinder_dist = projdist;
    obj->perp_offset = perp_offset;

    if(closest == NULL || obj->cylinder_dist < closest->cylinder_dist) {
      closest = obj;
    }
  }

  if(!closest) {
    // we're done
    computed = 0;
    return;
  }

  // steer away from closest collision
  struct Vector_ goal_vel;
  float pmag = vector_mag(&closest->perp_offset);
  if(pmag < 0.01) {
    // obstacle is dead-ahead, just turn
    closest->perp_offset.x = src_vel.y;
    closest->perp_offset.y = -src_vel.x;
    pmag = speed;
  }
  vector_scale(&goal_vel, &closest->perp_offset, params.speed_max / pmag);
  apply_desired_velocity(goal_vel, src_vel);
}
