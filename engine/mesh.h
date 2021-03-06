#ifndef MESH_H
#define MESH_H

#include "ooc.h"
#include "vector.h"
#include "color.h"
#include "giggle.h"
#include "giggle_gl.h"
#include "gameobject.h"

#include <vector>

typedef std::vector<Vector_> Points;
typedef std::vector<Color> Colors;

enum MeshType {
  MESH_LINES,
  MESH_TRIS,
  MESH_LINE_STRIP,
  MESH_LINE_LOOP
};

class Mesh : public Object {
public:
  OBJECT_PROTO(Mesh);

  Mesh(void* _game);

  void add_point(const Vector_& point, const Color& color);
  void add_point_and_tcoord(const Vector_& point, const Color& color, const Vector_& tcoord);

  long nverts() const;
  void clear();

  const Vector_& get_point(long idx) const;
  const Color& get_color(long idx) const;
  const Vector_& get_tcoord(long idx) const;

  Points points;
  Colors colors;
  Points tcoords;
  SpriteAtlasEntry entry;
  int type;
};

class MeshRenderer : public Renderable {
public:
  OBJECT_PROTO(MeshRenderer);
  MeshRenderer(void* empty);
  virtual void render(void* args);
};

class CMesh : public Component {
public:
  OBJECT_PROTO(CMesh);
  CMesh(void* _go);

  void set_mesh(Mesh* mesh);
  Mesh* get_mesh();
  virtual void render(Camera* camera);

  MeshRenderer* renderer;
  Mesh* mesh;
  Matrix44* tform;
  int layer;
};


struct Wall {
  Vector_ start;
  Vector_ end;
};
typedef std::vector<Wall> WallList;

class Walls : public Object {
public:
  OBJECT_PROTO(Walls);
  Walls(void* _game);

  void add_wall(const Vector_& start, const Vector_& end);
  void clear();
  long nwalls() const;

  void get_wall(Vector_& start, Vector_& end, long idx) const;
  int get_wall(lua_State* L, int pos);

  WallList walls;
};


#endif
