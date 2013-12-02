#include "mesh.h"

OBJECT_IMPL(Mesh, Object);
OBJECT_PROPERTY(Mesh, type);
OBJECT_PROPERTY(Mesh, entry);

Mesh::Mesh(void* _game)
  : type(MESH_TRIS), entry(NULL) {
}

void Mesh::add_point(const Vector_& point, const Color& color) {
  points.push_back(point);
  colors.push_back(color);
}
OBJECT_METHOD(Mesh, add_point, void, (Vector_, Color));

void Mesh::add_point_and_tcoord(const Vector_& point, const Color& color, const Vector_& tcoord) {
  add_point(point, color);
  tcoords.push_back(tcoord);
}
OBJECT_METHOD(Mesh, add_point_and_tcoord, void, (Vector_, Color, Vector_));

long Mesh::nverts() const {
  return points.size();
}
OBJECT_METHOD(Mesh, nverts, long, ());

void Mesh::clear() {
  points.clear();
  colors.clear();
}
OBJECT_METHOD(Mesh, clear, void, ());

const Vector_& Mesh::get_point(long idx) const {
  return points[idx];
}
OBJECT_METHOD(Mesh, get_point, Vector_, (long));

const Color& Mesh::get_color(long idx) const {
  return colors[idx];
}
OBJECT_METHOD(Mesh, get_color, Color, (long));

const Vector_& Mesh::get_tcoord(long idx) const {
  return tcoords[idx];
}
OBJECT_METHOD(Mesh, get_tcoord, Vector_, (long));

Program* vertcolor_program_loader() {
  Program *program = renderer_link_shader("vertcolor.vert",
                                          "vertcolor.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_COLOR0, "color",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_DONE);
  return program;
}

Program* vertcolortex_program_loader() {
  Program *program = renderer_link_shader("vertcolortex.vert",
                                          "vertcolortex.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_COLOR0, "color",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_DONE);
  return program;
}

struct MeshRendererArgs {
  Vector_* verts;
  Color* colors;
  Vector_* tcoords;
  Texture* texture;
  long nverts;
  int type;
};

OBJECT_IMPL(MeshRenderer, Renderable);
MeshRenderer::MeshRenderer(void* empty)
  : Renderable(empty) {
}

void MeshRenderer::render(void* args) {
  MeshRendererArgs* mesh = (MeshRendererArgs*)args;
  Program* program;
  bool tex = false;
  if(mesh->tcoords) {
    program = get_program(vertcolortex_program_loader);
    tex = true;
  } else {
    program = get_program(vertcolor_program_loader);
  }

  gl_check(program->use());

  GLMemory verts;
  GLMemory colors;
  GLMemory texs;

  GIGGLE->renderer->gl_bufinit(&verts);
  GIGGLE->renderer->gl_bufinit(&colors);
  if(tex) GIGGLE->renderer->gl_bufinit(&texs);

  size_t vsize = sizeof(Vector_) * mesh->nverts;
  GIGGLE->renderer->gl_claim(&verts, vsize);
  memcpy(verts.data, mesh->verts, vsize);

  size_t csize = sizeof(Color) * mesh->nverts;
  GIGGLE->renderer->gl_claim(&colors, csize);
  memcpy(colors.data, mesh->colors, csize);

  if(tex) {
    size_t tsize = sizeof(Vector_) * mesh->nverts;
    GIGGLE->renderer->gl_claim(&texs, tsize);
    memcpy(texs.data, mesh->tcoords, tsize);
  }

  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, verts.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(Vector_), 0));
  GIGGLE->renderer->gl_unclaim(&verts);

  gl_check(glEnableVertexAttribArray(GLPARAM_COLOR0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, colors.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_COLOR0, 4, GL_FLOAT, GL_FALSE, sizeof(Color), 0));
  GIGGLE->renderer->gl_unclaim(&colors);

  if(tex) {
    gl_check(glEnableVertexAttribArray(GLPARAM_TEXCOORD0));
    gl_check(glBindBuffer(GL_ARRAY_BUFFER, texs.buffer));
    gl_check(glVertexAttribPointer(GLPARAM_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(Vector_), 0));
    GIGGLE->renderer->gl_unclaim(&texs);
    gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
    mesh->texture->bind();
  }

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE,
                              GIGGLE->renderer->orthographic_projection.data));

  if(mesh->type == MESH_TRIS) {
    gl_check(glDrawArrays(GL_TRIANGLES, 0, mesh->nverts));
  } else if(mesh->type == MESH_LINES) {
    gl_check(glDrawArrays(GL_LINES, 0, mesh->nverts));
  } else if(mesh->type == MESH_LINE_STRIP) {
    gl_check(glDrawArrays(GL_LINE_STRIP, 0, mesh->nverts));
  } else {
    gl_check(glDrawArrays(GL_LINE_LOOP, 0, mesh->nverts));
  }
}



OBJECT_IMPL(CMesh, Component);
OBJECT_ACCESSOR(CMesh, mesh, get_mesh, set_mesh);
OBJECT_PROPERTY(CMesh, layer);
OBJECT_PROPERTY(CMesh, tform);

CMesh::CMesh(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), mesh(NULL),
    tform(NULL), layer(LAYER_FOREGROUND) {
  renderer = new MeshRenderer(NULL);
}

void CMesh::set_mesh(Mesh* newmesh) {
  if(mesh) mesh->release();
  newmesh->retain();
  mesh = newmesh;
}

Mesh* CMesh::get_mesh() {
  return mesh;
}

void CMesh::render(Camera* camera) {
  if(!mesh) return;

  Vector_ pos;
  go->pos(&pos);

  // convert the mesh into arguments
  bool tex = !mesh->tcoords.empty();
  long nverts = mesh->nverts();
  size_t vsize = sizeof(Vector_) * nverts;
  size_t csize = sizeof(Color) * nverts;
  size_t tsize = sizeof(Vector_) * (tex ? nverts : 0);
  // allocate enough space in one go
  char* mem = (char*)GIGGLE->renderer->alloc(sizeof(MeshRendererArgs) + vsize + csize + tsize);
  MeshRendererArgs* marg = (MeshRendererArgs*)mem;
  marg->verts = (Vector_*)&mem[sizeof(MeshRendererArgs)];
  marg->colors = (Color*)&mem[sizeof(MeshRendererArgs) + vsize];
  if(tex) {
    marg->tcoords = (Vector_*)&mem[sizeof(MeshRendererArgs) + vsize + csize];
    marg->texture = mesh->entry->atlas->image->texture;
  } else {
    marg->tcoords = NULL;
  }

  if(tform) {
    // apply the transform to the un-modified point
    Vector_ r;
    for(long ii = 0; ii < nverts; ++ii) {
      tform->vmult(&r, &mesh->points[ii]);
      vector_add(&marg->verts[ii], &pos, &r);
    }
  } else {
    // just add our offset
    for(long ii = 0; ii < nverts; ++ii) {
      vector_add(&marg->verts[ii], &pos, &mesh->points[ii]);
    }
  }

  memcpy(marg->colors, &mesh->colors[0], csize);
  if(tex) {
    // adjust the tcoords given the atlas entry
    float u0 = mesh->entry->u0;
    float u1 = mesh->entry->u1;
    float v0 = mesh->entry->v0;
    float v1 = mesh->entry->v1;

    float du = u1 - u0;
    float dv = v1 - v0;
    for(long ii = 0; ii < nverts; ++ii) {
      marg->tcoords[ii].x = u0 + mesh->tcoords[ii].x * du;
      marg->tcoords[ii].y = v0 + mesh->tcoords[ii].y * dv;
    }
  }

  marg->nverts = nverts;
  marg->type = mesh->type;

  camera->addRenderable(layer, renderer, marg);
}



OBJECT_IMPL(Walls, Object);

Walls::Walls(void* _game) {
}

void Walls::add_wall(const Vector_& start, const Vector_& end) {
  Wall wall;
  wall.start = start;
  wall.end = end;
  walls.push_back(wall);
}
OBJECT_METHOD(Walls, add_wall, void, (Vector_, Vector_));

void Walls::clear() {
  walls.clear();
}
OBJECT_METHOD(Walls, clear, void, ());

long Walls::nwalls() const {
  return walls.size();
}
OBJECT_METHOD(Walls, nwalls, long, ());

void Walls::get_wall(Vector_& start, Vector_& end, long idx) const {
  start = walls[idx].start;
  end = walls[idx].end;
}

int Walls::get_wall(lua_State* L, int pos) {
  int idx = luaL_checknumber(L, pos + 1);
  Vector_ start, end;
  get_wall(start, end, idx);

  lua_createtable(L, 2, 0);

  LCpush(L, start);
  lua_rawseti(L, -2, 1);

  LCpush(L, end);
  lua_rawseti(L, -2, 2);

  return 1;
}
OBJECT_LUA_METHOD(Walls, get_wall);
