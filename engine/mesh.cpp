#include "mesh.h"

OBJECT_IMPL(Mesh, Object);

Mesh::Mesh(void* _world) {
}

void Mesh::add_point(const Vector_& point, const Color& color) {
  points.push_back(point);
  colors.push_back(color);
}
OBJECT_METHOD(Mesh, add_point, void, (Vector_, Color));

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

Program* vertcolor_program_loader() {
  Program *program = renderer_link_shader("engine_resources/vertcolor.vert",
                                          "engine_resources/vertcolor.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_COLOR0, "color",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_DONE);
  return program;
}

struct MeshRendererArgs {
  Vector_* verts;
  Color* colors;
  long nverts;
};

OBJECT_IMPL(MeshRenderer, Renderable);
MeshRenderer::MeshRenderer(void* empty)
  : Renderable(empty), program(NULL) {
}

void MeshRenderer::render(void* args) {
  MeshRendererArgs* mesh = (MeshRendererArgs*)args;
  if(!program) program = get_program(vertcolor_program_loader);

  gl_check(program->use());

  GLMemory verts;
  GLMemory colors;
  gl_bufinit(&verts);
  gl_bufinit(&colors);

  size_t vsize = sizeof(Vector_) * mesh->nverts;
  gl_claim(&verts, vsize);
  memcpy(verts.data, mesh->verts, vsize);

  size_t csize = sizeof(Color) * mesh->nverts;
  gl_claim(&colors, csize);
  memcpy(colors.data, mesh->colors, csize);

  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, verts.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(Vector_), 0));
  gl_unclaim(&verts);

  gl_check(glEnableVertexAttribArray(GLPARAM_COLOR0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, colors.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_COLOR0, 4, GL_FLOAT, GL_FALSE, sizeof(Color), 0));
  gl_unclaim(&colors);

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, mesh->nverts));
}



OBJECT_IMPL(CMesh, Component);
OBJECT_ACCESSOR(CMesh, mesh, get_mesh, set_mesh);
OBJECT_PROPERTY(CMesh, layer);

CMesh::CMesh(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), mesh(NULL), layer(LAYER_FOREGROUND) {
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

  // convert the mesh into arguments
  long nverts = mesh->nverts();
  size_t vsize = sizeof(Vector_) * nverts;
  size_t csize = sizeof(Color) * nverts;

  // allocate enough space in one go
  char* mem = (char*)frame_alloc(sizeof(MeshRendererArgs) + vsize + csize);
  MeshRendererArgs* marg = (MeshRendererArgs*)mem;
  marg->verts = (Vector_*)&mem[sizeof(MeshRendererArgs)];
  marg->colors = (Color*)&mem[sizeof(MeshRendererArgs) + vsize];
  marg->nverts = nverts;

  memcpy(marg->verts, &mesh->points[0], vsize);
  memcpy(marg->colors, &mesh->colors[0], csize);
  camera->addRenderable(layer, renderer, marg);
}
