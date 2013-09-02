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
#include "pathfinder.h"
#include "config.h"

#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

PathElement::PathElement(PathElement* parent, int heuristic, int id)
  : parent(parent), heuristic(heuristic), id(id) {
  if(parent) {
    distance = parent->distance + 1;
  } else {
    distance = 0;
  }
}

void vector_path_direction(Vector dir, TileMap map, Path path, int test0, int pathdir) {
  int test1 = path_next_idx(path, test0, pathdir);
  if(test1 == -1) {
    int temp = test0;
    test0 = path_next_idx(path, test0, -pathdir);
    test1 = temp;
  }

  assert(test1 >= 0 && test1 < path->nsteps);

  struct Vector_ p0;
  struct Vector_ p1;
  vector_tilecenter(&p0, map, path->steps[test0]);
  vector_tilecenter(&p1, map, path->steps[test1]);
  vector_sub(dir, &p1, &p0);
  vector_norm(dir, dir);
}

int path_end_idx(Path path, int pathdir) {
  if(pathdir > 0) {
    return path->nsteps - 1;
  } else {
    return 0;
  }
}

int path_begin_idx(Path path, int pathdir) {
  if(pathdir > 0) {
    return 0;
  } else {
    return path->nsteps - 1;
  }
}

void vector_path_end(Vector end, Path path, TileMap map, int pathdir) {
  int end_idx = path_end_idx(path, pathdir);
  vector_tilecenter(end, map, path->steps[end_idx]);
}

void vector_path_begin(Vector begin, Path path, TileMap map, int pathdir) {
  int begin_idx = path_begin_idx(path, pathdir);
  vector_tilecenter(begin, map, path->steps[begin_idx]);
}

int path_next_idx(Path path, int current_idx, int pathdir) {
  assert(current_idx != -1);

  int next = current_idx + pathdir;
  if(next >= 0 && next < path->nsteps) return next;
  return -1;
}

int pathinstance_next_idx(PathInstance pi) {
  return path_next_idx(pi->path, pi->pathpos, pi->pathdir);
}

void vector_pathinstance_direction(Vector dir, TileMap map, PathInstance pi) {
  vector_path_direction(dir, map, pi->path, pi->pathpos, pi->pathdir);
}

int pathinstance_end_idx(PathInstance pi) {
  return path_end_idx(pi->path, pi->pathdir);
}

int pathinstance_begin_idx(PathInstance pi) {
  return path_begin_idx(pi->path, pi->pathdir);
}

void vector_pathinstance_end(Vector end, PathInstance pi, TileMap map) {
  vector_path_end(end, pi->path, map, pi->pathdir);
}

void vector_pathinstance_begin(Vector begin, PathInstance pi, TileMap map) {
  vector_path_begin(begin, pi->path, map, pi->pathdir);
}

int path_next_closest_point(Vector point, TileMap map, PathInstance pi, Vector pos, float* dist) {
  float closest_dist2 = INFINITY;
  int origin_idx = -1;

  int ii;
  int range;
  for(ii = pi->pathpos; ii != -1; ii = path_next_idx(pi->path, ii, pi->pathdir)) {
    int idx = ii;

    range = abs(idx - pi->pathpos);
    if(range > pi->max_skip_range) break;

    // if we run out of path then back off one. this ensures that
    // we'll get at least one hit if we're queried at the end of the
    // path
    int next_idx = path_next_idx(pi->path, idx, pi->pathdir);
    if(next_idx == -1) {
      next_idx = idx;
      idx = path_next_idx(pi->path, next_idx, -pi->pathdir);
    }

    int test0 = pi->path->steps[idx];
    int test1 = pi->path->steps[next_idx];
    struct Vector_ path_center0;
    struct Vector_ path_center1;
    struct Vector_ tangent;
    struct Vector_ projection;
    struct Vector_ projection_pt;
    struct Vector_ p0_to_pos;
    Vector refpoint;

    float pt0pt1_dist;
    float pathdot;

    vector_tilecenter(&path_center0, map, test0);
    vector_tilecenter(&path_center1, map, test1);
    vector_sub(&tangent, &path_center1, &path_center0);
    vector_sub(&p0_to_pos, pos, &path_center0);
    vector_norm(&tangent, &tangent);
    pt0pt1_dist = vector_dist(&path_center0, &path_center1);

    // project the query point onto the path
    pathdot = vector_project2(&projection, &p0_to_pos, &tangent);

    if(pathdot < 0) {
      // if the point is behind the starting point then compute distance
      // from the starting point
      refpoint = &path_center0;
    } else if(pathdot > pt0pt1_dist) {
      // if the point is beyond the ending point then compute distance
      // from the ending point
      refpoint = &path_center1;
    } else {
      // if the point is between the start and the end then compute
      // the projection point and find the distance from that
      vector_add(&projection_pt, &path_center0, &projection);
      refpoint = &projection_pt;
    }

    // compute and test the distance
    float dist2 = vector_dist2(pos, refpoint);
    if(dist2 < closest_dist2) {
      closest_dist2 = dist2;
      *point = *refpoint;
      origin_idx = idx;
    }
  }

  assert(dist);
  assert(origin_idx != -1);

  *dist = sqrtf(closest_dist2);

  return origin_idx;
}

int pathfinder_heuristic(TileMap map, int p1, int p2) {
  struct TilePosition_ tp1, tp2;
  tileposition_tilemap(&tp1, map, p1);
  tileposition_tilemap(&tp2, map, p2);

  int dx = tp1.x - tp2.x;
  int dy = tp1.y - tp2.y;

  // manhatten distance
  return abs(dx) + abs(dy); //sqrt(dx*dx + dy*dy);
}

void TileMapPathfinder::candidatesFrom(Candidates& candidates, PathElement* element) {
#define IS_CANDIDATE(idx)                               \
    (idx < sz && idx >= 0                               \
     && (map->tile_specs[map->tiles[idx]].bitmask & TILESPEC_PASSABLE))

  int start = element->id;
  int row = map->width_IT;
  int sz = tilemap_size(map);

  int above = start + row;
  if(IS_CANDIDATE(above)) {
    PathElement pelem(element, pathfinder_heuristic(map, start, above), above);
    candidates.add(pelem);
  }

  int below = start - row;
  if(IS_CANDIDATE(below)) {
    PathElement pelem(element, pathfinder_heuristic(map, start, below), below);
    candidates.add(pelem);
  }

  int left = start - 1;
  if(IS_CANDIDATE(left)) {
    PathElement pelem(element, pathfinder_heuristic(map, start, left), left);
    candidates.add(pelem);
  }

  int right = start + 1;
  if(IS_CANDIDATE(right)) {
    PathElement pelem(element, pathfinder_heuristic(map, start, right), right);
    candidates.add(pelem);
  }
}

int pathfinder_visibility_callback(TileMap map, TilePosition pos, void* udata) {
  int index = tilemap_index(map, pos);
  int spec = map->tiles[index];
  if(map->tile_specs[spec].bitmask & TILESPEC_COLLIDABLE) return 1;
  return 0;
}

void pathfinder_simplifypath(TileMap map, int* steps, int *nsteps) {
  int read_idx;
  int write_idx = 0;

  struct TilePosition_ last_tp;

  for(read_idx = 0; read_idx < *nsteps; ++read_idx) {
    int step = steps[read_idx];

    // always include the first
    if(read_idx == 0) {
      tileposition_tilemap(&last_tp, map, step);
      ++write_idx;
      continue;
    }

    struct TilePosition_ current_tp;
    tileposition_tilemap(&current_tp, map, step);

    int result = tilemap_trace_line(map, &last_tp, &current_tp,
                                    pathfinder_visibility_callback, NULL);

    // didn't encounter obstruction
    if(!result) {
      continue;
    }

    // we hit our first obstruction, back off one and search from
    // there
    steps[write_idx++] = steps[read_idx - 1];
    last_tp = current_tp;
  }

  // always include the endpoint
  if(steps[write_idx - 1] != steps[*nsteps - 1]) {
    steps[write_idx++] = steps[*nsteps - 1];
  }

  *nsteps = write_idx;
}

int* pathfinder_findpath(TileMap map, int p1, int p2, int* count) {
  int sz = tilemap_size(map);
  StackAllocator allocator = stack_allocator_make(sizeof(struct PathElement) * sz * 2,
                                                  "pathfinder_temp_allocator");
  Candidates candidates(allocator);

  TileMapPathfinder mapifc(map);

  PathElement* result = pathfinder_findpath2(mapifc, p1, p2, candidates);

  int* path = NULL;
  if(result) {
    path = (int*)malloc(sizeof(int) * (result->distance + 1));
    int ii = 0;
    while(result) {
      path[ii++] = result->id;
      result = result->parent;
    }
    *count = ii;
  } else {
    *count = 0;
  }

  stack_allocator_release(allocator);

  pathfinder_simplifypath(map, path, count);

  return path;
}

PathElement* pathfinder_findpath2(WorldPathfinderIfc& map, int p1, int p2, Candidates& candidates) {
  PathElement start_elem(NULL, 0, p1);
  candidates.add(start_elem);

  int count = 0;
  while(!candidates.empty()) {
    PathElement* elem = candidates.top();
    candidates.pop();

    if(candidates.isMarked(elem)) {
      // we already found a better way to this point
      continue;
    }

    if(elem->id == p2) {
      // we reached the goal!
      return elem;
    }

    // get candidates reachable from this point
    map.candidatesFrom(candidates, elem);
  }

  // no possible path
  return NULL;
}
