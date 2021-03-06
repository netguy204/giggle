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
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "tiles.h"
#include "heapvector.h"
#include "memlib.h"
#include "listlib.h"

class PathElement {
public:
  PathElement* parent;

  int distance;
  int heuristic;
  int id;

  PathElement(PathElement* parent, int heuristic, int id);

  inline int score() const {
    return distance + heuristic;
  }
};

inline int pathelement2_compare(PathElement* const& a, PathElement* const& b) {
  if(a->score() < b->score()) return -1;
  if(a->score() > b->score()) return 1;

  if(a->heuristic < b->heuristic) return -1;
  if(a->heuristic > b->heuristic) return 1;

  return 0;
}

class Candidates {
public:
  BinaryHeap<PathElement*, StackAllocatorAdapter<PathElement*> > candidates;
  std::vector<char, StackAllocatorAdapter<char> > visited_mask;
  std::vector<PathElement, StackAllocatorAdapter<PathElement> > store;

  inline Candidates(StackAllocator alloc)
    : candidates(pathelement2_compare, StackAllocatorAdapter<PathElement*>(alloc)),
      visited_mask(StackAllocatorAdapter<char>(alloc)),
      store(StackAllocatorAdapter<PathElement>(alloc)) {
  }

  inline void add(const PathElement& el) {
    if(visited_mask.size() <= el.id) {
      visited_mask.resize(el.id + 1, false);
    }
    if(!visited_mask[el.id]) {
      store.push_back(el);
      candidates.insert(&store.back());
    }
  }

  inline PathElement* top() {
    return candidates.top();
  }

  inline void pop() {
    candidates.pop();
  }

  inline void mark(PathElement* el) {
    visited_mask[el->id] = true;
  }

  inline bool isMarked(PathElement* el) {
    return visited_mask[el->id];
  }

  inline bool empty() const {
    return candidates.empty();
  }
};

class WorldPathfinderIfc {
public:
  virtual void candidatesFrom(Candidates& candidates, PathElement* element) = 0;
};

class TileMapPathfinderIfc : public WorldPathfinderIfc {
public:
  TileMap* map;

  inline TileMapPathfinderIfc(TileMap* map)
    : map(map) {
  }

  virtual void candidatesFrom(Candidates& candidates, PathElement* element);
};


PathElement* pathfinder_findpath2(WorldPathfinderIfc& map, int p1, int p2, Candidates& candidates);

class Path : public Object {
public:
  OBJECT_PROTO(Path);

  Path(void* _map);
  ~Path();

  TileMap* map;
  TilePosition start;
  TilePosition end;
  int* steps;
  int nsteps;

  Vector_ step(int idx);
};

// functions that are specifically dealing with pathfinding over a TileMap
class TileMapPathfinder : public Object {
public:
  OBJECT_PROTO(TileMapPathfinder);

  TileMapPathfinder(void* empty);

  Path* findpath(TileMap* map, Vector_ v1, Vector_ v2);
};

void vector_path_direction(Vector dir, Path* path, int test0, int pathdir);
int path_end_idx(Path* path, int pathdir);
int path_begin_idx(Path* path, int pathdir);
void vector_path_end(Vector end, Path* path, int pathdir);
void vector_path_begin(Vector begin, Path* path, int pathdir);

class PathInstance : public Object {
public:
  OBJECT_PROTO(PathInstance);

  PathInstance(void* _path);
  ~PathInstance();

  Path* path;
  int pathdir;
  int pathpos;
  int max_skip_range;
};

// -1 on fall off the path
int path_next_idx(Path* path, int current_idx, int pathdir);
int pathinstance_next_idx(PathInstance* pi);
void vector_pathinstance_direction(Vector dir, PathInstance* pi);
int pathinstance_end_idx(PathInstance* pi);
int pathinstance_begin_idx(PathInstance* pi);
void vector_pathinstance_end(Vector end, PathInstance* pi);
void vector_pathinstance_begin(Vector begin, PathInstance* pi);

// find POINT on PATH closest to POS and return the DIST to it.
int path_next_closest_point(Vector point, PathInstance* pi, Vector pos, float* dist);

#endif
