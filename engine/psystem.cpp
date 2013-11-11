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
#include "psystem.h"
#include "giggle_gl.h"
#include "color.h"

static long system_offset(SystemDefinition* def, ParticleFeature feature) {
  if((def->feature_flags & feature) == 0) {
    fail_exit("feature %d is not in this system", feature);
  }

  long offset = 0;

#define FEATURE_OFFSET_GEN(name, type, idx)             \
  if(name == feature) {                                 \
    return offset;                                      \
  }                                                     \
  if(def->feature_flags & name) {                       \
    offset += NEXT_ALIGNED_SIZE(sizeof(type) * def->n); \
  }

  FEATURE_TABLE(FEATURE_OFFSET_GEN);

  fail_exit("feature %d is not declared in the feature table", feature);
  return 0;
}

OBJECT_IMPL(SystemDefinition, Object);

SystemDefinition::SystemDefinition(void* empty) {
  fail_exit("default runtime constructor not valid");
}

SystemDefinition::SystemDefinition(unsigned int n)
  : renderer(NULL), activator(new PSAlwaysActivator(this)), n(n), feature_flags(0),
    layer(LAYER_PLAYER) {
  random_init(&rgen, 1234);
}

ParticleSystemComponent* SystemDefinition::add_component(TypeInfo* type) {
  ParticleSystemComponent* comp = (ParticleSystemComponent*)type->makeInstance(this);

  if(type->isInstanceOf(&ParticleSystemInitializer::Type)) {
    initializers.add_tail((ParticleSystemInitializer*)comp);
  } else if(type->isInstanceOf(&ParticleSystemUpdater::Type)) {
    updaters.add_tail((ParticleSystemUpdater*)comp);
  } else if(type->isInstanceOf(&ParticleSystemTerminator::Type)) {
    terminators.add_tail((ParticleSystemTerminator*)comp);
  } else {
    fail_exit("unrecognized particle system component type");
  }

  feature_flags |= comp->required_features();
  return comp;
}

ParticleSystemComponent* SystemDefinition::find_component(TypeInfo* info) {
  if(activator->typeinfo()->isInstanceOf(info)) {
    return activator;
  }

  DLLNode node = updaters.head;
  while(node) {
    ParticleSystemUpdater* p = updaters.to_element(node);
    if(p->typeinfo()->isInstanceOf(info)) {
      return p;
    }
    node = node->next;
  }

  node = initializers.head;
  while(node) {
    ParticleSystemInitializer* p = initializers.to_element(node);
    if(p->typeinfo()->isInstanceOf(info)) {
      return p;
    }
    node = node->next;
  }

  node = terminators.head;
  while(node) {
    ParticleSystemTerminator* p = terminators.to_element(node);
    if(p->typeinfo()->isInstanceOf(info)) {
      return p;
    }
    node = node->next;
  }

  return NULL;
}
OBJECT_METHOD(SystemDefinition, find_component, ParticleSystemComponent*, (TypeInfo*));

SystemRenderer* SystemDefinition::set_renderer(TypeInfo* type) {
  if(renderer) {
    renderer->destroy();
  }
  renderer = (SystemRenderer*)type->makeInstance(this);
  return renderer;
}

ParticleActivator* SystemDefinition::set_activator(TypeInfo* type) {
  if(activator) {
    activator->release();
  }
  activator = (ParticleActivator*)type->makeInstance(this);
  return activator;
}

void SystemDefinition::update(float dt) {
  DLLNode node = initializers.head;
  while(node) {
    ParticleSystemInitializer* p = initializers.to_element(node);
    p->initialize();
    node = node->next;
  }

  if(activator->enabled) {
    activator->activate(dt);
  }

  node = updaters.head;
  while(node) {
    ParticleSystemUpdater* p = updaters.to_element(node);
    p->update(dt);
    node = node->next;
  }

  node = terminators.head;
  while(node) {
    ParticleSystemTerminator* p = terminators.to_element(node);
    p->terminateExpired();
    node = node->next;
  }
}

void SystemDefinition::render(Camera* camera) {
  renderer->submit(camera, layer, this);
}

size_t SystemDefinition::features_size() {
  size_t allocation = 0;

#define FEATURE_SIZE_GEN(name, type, idx)                       \
  if(feature_flags & name) {                                    \
    allocation += NEXT_ALIGNED_SIZE(sizeof(type) * n);          \
  }

  FEATURE_TABLE(FEATURE_SIZE_GEN);

  return allocation;
}

void SystemDefinition::init() {
  long allocation = features_size();

  features = (char*)malloc(allocation);

  DLLNode node = initializers.head;
  while(node) {
    ParticleSystemInitializer* p = initializers.to_element(node);
    p->firstInitialize();
    node = node->next;
  }

  activator->activate(0);
}

struct ReleaseComponent {
  bool operator()(ParticleSystemComponent* p) {
    p->release();
    return 0;
  }
};

SystemDefinition::~SystemDefinition() {
  if(activator) activator->release();

  initializers.foreach(ReleaseComponent());

  updaters.foreach(ReleaseComponent());

  terminators.foreach(ReleaseComponent());

  free(features);
  if(renderer) {
    renderer->destroy();
  }
}

static void* system_feature(SystemDefinition* def, ParticleFeature feature) {
  long offset = system_offset(def, feature);
  return &def->features[offset];
}

float* SystemDefinition::system_featuref(ParticleFeature feature) {
  return (float*)system_feature(this, feature);
}

Vector SystemDefinition::system_featurev(ParticleFeature feature) {
  return (Vector)system_feature(this, feature);
}

Color* SystemDefinition::system_featurec(ParticleFeature feature) {
  return (Color*)system_feature(this, feature);
}

char* SystemDefinition::system_featurech(ParticleFeature feature) {
  return (char*)system_feature(this, feature);
}

int SystemDefinition::supports_feature(ParticleFeature feature) {
  return feature_flags & feature;
}

Program* p_es_program_loader() {
  Program *program = renderer_link_shader("engine_resources/p_es.vert",
                                          "engine_resources/p_es.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_SCALE, "scale",
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_TEX_BL, "tex_bl",
                        UNIFORM_TEX_TR, "tex_tr",
                        UNIFORM_DONE);
  return program;
}

OBJECT_IMPL(SystemRenderer, Renderable);

SystemRenderer::SystemRenderer(void *empty)
  : Renderable(empty) {
}

void SystemRenderer::render(void *args) {
}

void SystemRenderer::submit(Camera* camera, int layer, SystemDefinition* def) {
}

OBJECT_IMPL(ParticleActivator, ParticleSystemComponent);
OBJECT_PROPERTY(ParticleActivator, enabled);

ParticleActivator::ParticleActivator(void* _def)
  : ParticleSystemComponent(_def), enabled(1) {
}

void ParticleActivator::activate(float dt) {
}

OBJECT_IMPL(P_ESSystemRenderer, SystemRenderer);
OBJECT_PROPERTY(P_ESSystemRenderer, entry);
OBJECT_PROPERTY(P_ESSystemRenderer, scale);

P_ESSystemRenderer::P_ESSystemRenderer(void* empty)
  : SystemRenderer(empty) {
}

struct P_ESParams {
  float scale;
  SpriteAtlasEntry entry;
  unsigned int n;
  Vector_ positions[0];
};

void P_ESSystemRenderer::submit(Camera* camera, int layer, SystemDefinition* def) {
  P_ESParams* params = (P_ESParams*)GIGGLE->renderer->alloc(sizeof(P_ESParams) +
                                                            sizeof(Vector_) * def->n);

  params->scale = scale;
  params->entry = entry;

  // copy only valid positions
  Vector positions = def->system_featurev(PF_POSITION);
  char* valids = def->system_featurech(PF_VALID);

  params->n = 0;
  for(int ii = 0; ii < def->n; ++ii) {
    if(valids[ii]) {
      params->positions[params->n++] = positions[ii];
    }
  }

  camera->addRenderable(layer, this, params);
}

void P_ESSystemRenderer::render(void *args) {
  P_ESParams* params = (P_ESParams*)args;

  Vector positions = params->positions;
  const int nverts = params->n;
  const int nfloats = nverts * 2;

  Program *program = get_program(p_es_program_loader);
  program->use();
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE,
                              GIGGLE->renderer->orthographic_projection.data));

  params->entry->atlas->image->texture->bind();
  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniform2f(program->requireUniform(UNIFORM_TEX_TR), entry->u0, entry->v0));
  gl_check(glUniform2f(program->requireUniform(UNIFORM_TEX_BL), entry->u1, entry->v1));

  // do this both ways. GLES targets assume GL_PROGRAM_POINT_SIZE but
  // macs before Lion don't support it. No harm in double dipping.
  gl_check(glUniform1f(program->requireUniform(UNIFORM_SCALE), params->scale * entry->w));
#ifndef ANDROID
  glPointSize(params->scale * entry->w);
#endif

  GLuint vertex_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));

  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nfloats, positions,
                        GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_check(glDrawArrays(GL_POINTS, 0, nverts));
  glDisableVertexAttribArray(GLPARAM_VERTEX);
}

Program* pc_es2_program_loader() {
  Program *program = renderer_link_shader("engine_resources/pc_es2.vert",
                                          "engine_resources/pc_es2.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_COLOR0, "color",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_DONE);
  return program;
}


/* (P)osition and (S)cale are defined per particle. (E)ntity is per
   system. */
struct PSC_EParams {
  SpriteAtlasEntry entry;
  unsigned int n;
  Vector positions;
  float* scales;
  Color* colors;
  char* valids;
};

OBJECT_IMPL(PSC_E2SystemRenderer, SystemRenderer);
OBJECT_PROPERTY(PSC_E2SystemRenderer, entry);

PSC_E2SystemRenderer::PSC_E2SystemRenderer(void *empty)
  : SystemRenderer(empty), entry(NULL) {
}

void PSC_E2SystemRenderer::submit(Camera* camera, int layer, SystemDefinition* def) {
  PSC_EParams* params = (PSC_EParams*)GIGGLE->renderer->alloc(sizeof(PSC_EParams));

  params->entry = entry;
  params->n = def->n;
  params->positions = (Vector)GIGGLE->renderer->alloc(sizeof(Vector_) * def->n);
  params->scales = (float*)GIGGLE->renderer->alloc(sizeof(float) * def->n);
  params->colors = (Color*)GIGGLE->renderer->alloc(sizeof(Color) * def->n);
  params->valids = (char*)GIGGLE->renderer->alloc(sizeof(char) * def->n);

  memcpy(params->positions, def->system_featurev(PF_POSITION), def->n * sizeof(Vector_));
  memcpy(params->scales, def->system_featuref(PF_SCALE), def->n * sizeof(float));
  memcpy(params->colors, def->system_featurec(PF_COLOR), def->n * sizeof(Color));
  memcpy(params->valids, def->system_featurech(PF_VALID), def->n * sizeof(char));

  camera->addRenderable(layer, this, params);
}

void PSC_E2SystemRenderer::render(void *args) {
  PSC_EParams* params = (PSC_EParams*)args;

  Vector positions = params->positions;
  float* scales = params->scales;
  Color* colors = params->colors;
  char* valids = params->valids;

  const int nquads = params->n;
  const int ntris = nquads * 2;
  const int nverts = ntris * 3;
  const int nfloats = nverts * 2;

  Program *program = get_program(pc_es2_program_loader);
  program->use();

  SpriteAtlasEntry entry = params->entry;
  entry->atlas->image->texture->bind();
  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));

  int vert_idx = 0;
  int tex_idx = 0;
  int col_idx = 0;

  GLMemory vertmem;
  GLMemory texmem;
  GLMemory colmem;

  GLfloat *verts = (GLfloat*)gl_claim(gl_bufinit(&vertmem), sizeof(GLfloat) * nfloats)->data;
  GLfloat *texs = (GLfloat*)gl_claim(gl_bufinit(&texmem), sizeof(GLfloat) * nfloats)->data;
  GLfloat *cols = (GLfloat*)gl_claim(gl_bufinit(&colmem), sizeof(GLfloat) * nverts * 4)->data;

  for(int ii = 0; ii < params->n; ++ii) {
    if(!valids[ii]) continue;

    float hs = (scales[ii] / 2.0f) * entry->w;

    float minx = positions[ii].x - hs;
    float maxx = positions[ii].x + hs;
    float miny = positions[ii].y - hs;
    float maxy = positions[ii].y + hs;

    // bottom-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v0;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;

    // bottom-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v0;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;

    // top-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v1;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;

    // top-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v1;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;

    // top-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v1;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;

    // bottom-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v0;
    cols[col_idx++] = colors[ii].r;
    cols[col_idx++] = colors[ii].g;
    cols[col_idx++] = colors[ii].b;
    cols[col_idx++] = colors[ii].a;
  }

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE,
                              GIGGLE->renderer->orthographic_projection.data));

  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vertmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&vertmem);

  gl_check(glEnableVertexAttribArray(GLPARAM_COLOR0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, colmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_COLOR0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&colmem);

  gl_check(glEnableVertexAttribArray(GLPARAM_TEXCOORD0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, texmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&texmem);

  if(vert_idx == 0) return;
  gl_check(glDrawArrays(GL_TRIANGLES, 0, vert_idx / 2));

  glDisableVertexAttribArray(GLPARAM_TEXCOORD0);
  glDisableVertexAttribArray(GLPARAM_COLOR0);
  glDisableVertexAttribArray(GLPARAM_VERTEX);
}

/* p_es2 renders a quad instead of a point sprite. This lets us scale
   beyond the point size limit of opengl (usually 64, apparently) */
Program* p_es2_program_loader() {
  Program *program = renderer_link_shader("engine_resources/p_es2.vert",
                                          "engine_resources/p_es2.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_DONE);
  return program;
}

OBJECT_IMPL(P_ES2SystemRenderer, P_ESSystemRenderer);

P_ES2SystemRenderer::P_ES2SystemRenderer(void* empty)
  : P_ESSystemRenderer(empty) {
}

void P_ES2SystemRenderer::render(void *args) {
  P_ESParams* params = (P_ESParams*)args;

  Vector positions = params->positions;
  const int nquads = params->n;
  const int ntris = nquads * 2;
  const int nverts = ntris * 3;
  const int nfloats = nverts * 2;

  Program *program = get_program(p_es2_program_loader);
  program->use();

  SpriteAtlasEntry entry = params->entry;
  entry->atlas->image->texture->bind();
  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));

  float hs = (entry->w * params->scale) / 2.0f;
  int vert_idx = 0;
  int tex_idx = 0;

  GLMemory vertmem;
  GLMemory texmem;

  GLfloat *verts = (GLfloat*)gl_claim(gl_bufinit(&vertmem), sizeof(GLfloat) * nfloats)->data;
  GLfloat *texs = (GLfloat*)gl_claim(gl_bufinit(&texmem), sizeof(GLfloat) * nfloats)->data;

  for(int ii = 0; ii < params->n; ++ii) {
    float minx = positions[ii].x - hs;
    float maxx = positions[ii].x + hs;
    float miny = positions[ii].y - hs;
    float maxy = positions[ii].y + hs;

    // bottom-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v0;

    // bottom-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v0;

    // top-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v1;

    // top-right
    verts[vert_idx++] = maxx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u1;
    texs[tex_idx++] = entry->v1;

    // top-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = maxy;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v1;

    // bottom-left
    verts[vert_idx++] = minx;
    verts[vert_idx++] = miny;
    texs[tex_idx++] = entry->u0;
    texs[tex_idx++] = entry->v0;
  }

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE,
                              GIGGLE->renderer->orthographic_projection.data));

  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vertmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&vertmem);

  gl_check(glEnableVertexAttribArray(GLPARAM_TEXCOORD0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, texmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&texmem);

  gl_check(glDrawArrays(GL_TRIANGLES, 0, vert_idx / 2));
  glDisableVertexAttribArray(GLPARAM_TEXCOORD0);
  glDisableVertexAttribArray(GLPARAM_VERTEX);
}

OBJECT_IMPL(ParticleSystemComponent, Object);

ParticleSystemComponent::ParticleSystemComponent(void* _def)
  : def((SystemDefinition*)_def) {
}

ParticleSystemComponent::~ParticleSystemComponent() {
  // we can't use virtual typeinfo() to determine the type here
  // because in the destructor all the derived type information has
  // been lost. We'll have to figure out what list we're in the slow
  // way.
  if(def->initializers.contains((ParticleSystemInitializer*)this)) {
    def->initializers.remove((ParticleSystemInitializer*)this);
  } else if(def->updaters.contains((ParticleSystemUpdater*)this)) {
    def->updaters.remove((ParticleSystemUpdater*)this);
  } else if(def->terminators.contains((ParticleSystemTerminator*)this)) {
    def->terminators.remove((ParticleSystemTerminator*)this);
  } else {
    // we're not in a list. that's fine (ParticleActivators)
  }
}

unsigned int ParticleSystemComponent::required_features() {
  return 0;
}

OBJECT_IMPL(ParticleSystemInitializer, ParticleSystemComponent);

ParticleSystemInitializer::ParticleSystemInitializer(void* _def)
  : ParticleSystemComponent(_def) {
}

void ParticleSystemInitializer::firstInitialize() {
}

void ParticleSystemInitializer::initialize() {
}

OBJECT_IMPL(ParticleSystemUpdater, ParticleSystemComponent);

ParticleSystemUpdater::ParticleSystemUpdater(void* _def)
  : ParticleSystemComponent(_def) {
}

void ParticleSystemUpdater::update(float dt) {
}

OBJECT_IMPL(ParticleSystemTerminator, ParticleSystemComponent);

ParticleSystemTerminator::ParticleSystemTerminator(void* _def)
  : ParticleSystemComponent(_def) {
}

void ParticleSystemTerminator::terminateExpired() {
}

OBJECT_IMPL(CParticleSystem, Component);
OBJECT_PROPERTY(CParticleSystem, def);

CParticleSystem::CParticleSystem(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), def(NULL) {
}

CParticleSystem::~CParticleSystem() {
  delete def;
}

void CParticleSystem::init() {
  def->component = this;
  def->init();
}

void CParticleSystem::update(float dt) {
  def->update(dt);
}

void CParticleSystem::render(Camera* camera) {
  def->render(camera);
}

OBJECT_IMPL(PSConstantAccelerationUpdater, ParticleSystemUpdater);
OBJECT_PROPERTY(PSConstantAccelerationUpdater, acc);

PSConstantAccelerationUpdater::PSConstantAccelerationUpdater(void* _def)
  : ParticleSystemUpdater(_def) {
  vector_zero(&acc);
}

void PSConstantAccelerationUpdater::update(float dt) {
  Vector positions = def->system_featurev(PF_POSITION);
  Vector velocities = def->system_featurev(PF_VELOCITY);

  Vector_ dv;
  vector_scale(&dv, &acc, dt);

  Vector_ dx;
  for(int ii = 0; ii < def->n; ++ii) {
    vector_scale(&dx, &velocities[ii], dt);
    vector_add(&positions[ii], &positions[ii], &dx);
    vector_add(&velocities[ii], &velocities[ii], &dv);
  }
}

unsigned int PSConstantAccelerationUpdater::required_features() {
  return PF_POSITION | PF_VELOCITY;
}

OBJECT_IMPL(PSTimeAlphaUpdater, ParticleSystemUpdater);
OBJECT_PROPERTY(PSTimeAlphaUpdater, time_constant);
OBJECT_PROPERTY(PSTimeAlphaUpdater, max_scale);

PSTimeAlphaUpdater::PSTimeAlphaUpdater(void *_def)
  : ParticleSystemUpdater(_def), max_scale(1.0f) {
}

void PSTimeAlphaUpdater::update(float dt) {
  float* times = def->system_featuref(PF_TIME);
  float* scales = def->system_featuref(PF_SCALE);
  Color* colors = def->system_featurec(PF_COLOR);

  for(int ii = 0; ii < def->n; ++ii) {
    times[ii] -= dt;
    colors[ii].a = 1.0f - expf(-times[ii] / time_constant);
    scales[ii] = expf(-times[ii] / time_constant) * max_scale;
  }
}

unsigned int PSTimeAlphaUpdater::required_features() {
  return PF_TIME | PF_COLOR | PF_SCALE;
}

OBJECT_IMPL(PSFireColorUpdater, ParticleSystemUpdater);
OBJECT_PROPERTY(PSFireColorUpdater, max_life);
OBJECT_PROPERTY(PSFireColorUpdater, start_temperature);
OBJECT_PROPERTY(PSFireColorUpdater, end_temperature);

PSFireColorUpdater::PSFireColorUpdater(void* _def)
  : ParticleSystemUpdater(_def), max_life(1),
    start_temperature(6000), end_temperature(4000) {
}

void PSFireColorUpdater::update(float dt) {
  float color[3];
  float* times = def->system_featuref(PF_TIME);
  Color* colors = def->system_featurec(PF_COLOR);

  float s = (start_temperature - end_temperature) / max_life;

  for(int ii = 0; ii < def->n; ++ii) {
    float temp = end_temperature + times[ii] * s;
    color_for_temp(temp, color);
    colors[ii].r = color[0];
    colors[ii].g = color[1];
    colors[ii].b = color[2];
  }
}

unsigned int PSFireColorUpdater::required_features() {
  return PF_TIME | PF_COLOR;
}

OBJECT_IMPL(PSBoxInitializer, ParticleSystemInitializer);
OBJECT_PROPERTY(PSBoxInitializer, initial);
OBJECT_PROPERTY(PSBoxInitializer, refresh);
OBJECT_PROPERTY(PSBoxInitializer, minv);
OBJECT_PROPERTY(PSBoxInitializer, maxv);

PSBoxInitializer::PSBoxInitializer(void* _def)
  : ParticleSystemInitializer(_def) {
  vector_zero(&minv);
  vector_zero(&maxv);
}

void PSBoxInitializer::firstInitialize() {
  Vector positions = def->system_featurev(PF_POSITION);
  Vector velocities = def->system_featurev(PF_VELOCITY);
  char* valids = def->system_featurech(PF_VALID);

  Vector_ center;
  def->component->go->pos(&center);

  float minx = center.x + initial.minx;
  float maxx = center.x + initial.maxx;
  float miny = center.y + initial.miny;
  float maxy = center.y + initial.maxy;

  for(int ii = 0; ii < def->n; ++ii) {
    positions[ii].x = rand_in_range(&def->rgen, minx, maxx);
    positions[ii].y = rand_in_range(&def->rgen, miny, maxy);
    velocities[ii].x = rand_in_range(&def->rgen, minv.x, maxv.x);
    velocities[ii].y = rand_in_range(&def->rgen, minv.y, maxv.y);
    valids[ii] = 0;
  }
}

void PSBoxInitializer::initialize() {
  Vector positions = def->system_featurev(PF_POSITION);
  Vector velocities = def->system_featurev(PF_VELOCITY);
  char* valids = def->system_featurech(PF_VALID);

  Vector_ center;
  def->component->go->pos(&center);

  float minx = center.x + refresh.minx;
  float maxx = center.x + refresh.maxx;
  float miny = center.y + refresh.miny;
  float maxy = center.y + refresh.maxy;

  for(int ii = 0; ii < def->n; ++ii) {
    if(!valids[ii]) {
      positions[ii].x = rand_in_range(&def->rgen, minx, maxx);
      positions[ii].y = rand_in_range(&def->rgen, miny, maxy);
      velocities[ii].x = rand_in_range(&def->rgen, minv.x, maxv.x);
      velocities[ii].y = rand_in_range(&def->rgen, minv.y, maxv.y);
    }
  }
}

unsigned int PSBoxInitializer::required_features() {
  return PF_POSITION | PF_VELOCITY | PF_VALID;
}

OBJECT_IMPL(PSTimeInitializer, ParticleSystemInitializer);
OBJECT_PROPERTY(PSTimeInitializer, min_life);
OBJECT_PROPERTY(PSTimeInitializer, max_life);

PSTimeInitializer::PSTimeInitializer(void* _def)
  : ParticleSystemInitializer(_def), min_life(0.2), max_life(1.5) {
}

void PSTimeInitializer::firstInitialize() {
  float* times = def->system_featuref(PF_TIME);

  for(int ii = 0; ii < def->n; ++ii) {
    times[ii] = rand_in_rangef(&def->rgen, 0.0, max_life);
  }
}

void PSTimeInitializer::initialize() {
  float* times = def->system_featuref(PF_TIME);
  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    if(!valids[ii]) {
      times[ii] = rand_in_rangef(&def->rgen, min_life, max_life);
    }
  }
}

unsigned int PSTimeInitializer::required_features() {
  return PF_TIME | PF_VALID;
}

OBJECT_IMPL(PSRandColorInitializer, ParticleSystemInitializer);
OBJECT_PROPERTY(PSRandColorInitializer, min_color);
OBJECT_PROPERTY(PSRandColorInitializer, max_color);

PSRandColorInitializer::PSRandColorInitializer(void* _def)
  : ParticleSystemInitializer(_def) {
  min_color.r = 1.0f;
  min_color.g = 1.0f;
  min_color.b = 1.0f;
  min_color.a = 0.0f;
  max_color.r = 1.0f;
  max_color.g = 1.0f;
  max_color.b = 1.0f;
  max_color.a = 1.0f;
}

void PSRandColorInitializer::firstInitialize() {
  Color* colors = def->system_featurec(PF_COLOR);

  for(int ii = 0; ii < def->n; ++ii) {
    colors[ii].r = rand_in_rangef(&def->rgen, min_color.r, max_color.r);
    colors[ii].g = rand_in_rangef(&def->rgen, min_color.g, max_color.g);
    colors[ii].b = rand_in_rangef(&def->rgen, min_color.b, max_color.b);
    colors[ii].a = rand_in_rangef(&def->rgen, min_color.a, max_color.a);
  }
}

void PSRandColorInitializer::initialize() {
  Color* colors = def->system_featurec(PF_COLOR);
  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    if(!valids[ii]) {
      colors[ii].r = rand_in_rangef(&def->rgen, min_color.r, max_color.r);
      colors[ii].g = rand_in_rangef(&def->rgen, min_color.g, max_color.g);
      colors[ii].b = rand_in_rangef(&def->rgen, min_color.b, max_color.b);
      colors[ii].a = rand_in_rangef(&def->rgen, min_color.a, max_color.a);
    }
  }
}

unsigned int PSRandColorInitializer::required_features() {
  return PF_COLOR | PF_VALID;
}

OBJECT_IMPL(PSRandScaleInitializer, ParticleSystemInitializer);
OBJECT_PROPERTY(PSRandScaleInitializer, min_scale);
OBJECT_PROPERTY(PSRandScaleInitializer, max_scale);

PSRandScaleInitializer::PSRandScaleInitializer(void* _def)
  : ParticleSystemInitializer(_def), min_scale(0), max_scale(1) {
}

void PSRandScaleInitializer::firstInitialize() {
  float* scales = def->system_featuref(PF_SCALE);

  for(int ii = 0; ii < def->n; ++ii) {
    scales[ii] = rand_in_rangef(&def->rgen, min_scale, max_scale);
  }
}

void PSRandScaleInitializer::initialize() {
  float* scales = def->system_featuref(PF_SCALE);
  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    if(!valids[ii]) {
      scales[ii] = rand_in_rangef(&def->rgen, min_scale, max_scale);
    }
  }
}

unsigned int PSRandScaleInitializer::required_features() {
  return PF_SCALE | PF_VALID;
}

OBJECT_IMPL(PSConstColorInitializer, ParticleSystemInitializer);
OBJECT_PROPERTY(PSConstColorInitializer, color);

PSConstColorInitializer::PSConstColorInitializer(void* _def)
  : ParticleSystemInitializer(_def) {
}

void PSConstColorInitializer::firstInitialize() {
  Color* colors = def->system_featurec(PF_COLOR);
  for(int ii = 0; ii < def->n; ++ii) {
    colors[ii] = color;
  }
}

void PSConstColorInitializer::initialize() {
  Color* colors = def->system_featurec(PF_COLOR);
  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    if(!valids[ii]) {
      colors[ii] = color;
    }
  }
}

unsigned int PSConstColorInitializer::required_features() {
  return PF_COLOR | PF_VALID;
}

OBJECT_IMPL(PSBoxTerminator, ParticleSystemTerminator);
OBJECT_PROPERTY(PSBoxTerminator, rect);

PSBoxTerminator::PSBoxTerminator(void* _def)
  : ParticleSystemTerminator(_def) {
}

void PSBoxTerminator::terminateExpired() {
  Vector positions = def->system_featurev(PF_POSITION);
  char* valids = def->system_featurech(PF_VALID);

  Vector_ center;
  def->component->go->pos(&center);

  float minx = center.x + rect.minx;
  float maxx = center.x + rect.maxx;
  float miny = center.y + rect.miny;
  float maxy = center.y + rect.maxy;

  for(int ii = 0; ii < def->n; ++ii) {
    float x = positions[ii].x;
    float y = positions[ii].y;
    if(x < minx || x > maxx || y < miny || y > maxy) {
      valids[ii] = 0;
    }
  }
}

unsigned int PSBoxTerminator::required_features() {
  return PF_POSITION | PF_VALID;
}

OBJECT_IMPL(PSTimeTerminator, ParticleSystemTerminator);

PSTimeTerminator::PSTimeTerminator(void* _def)
  : ParticleSystemTerminator(_def) {
}

void PSTimeTerminator::terminateExpired() {
  float* times = def->system_featuref(PF_TIME);
  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    if(times[ii] <= 0) {
      valids[ii] = 0;
    }
  }
}

unsigned int PSTimeTerminator::required_features() {
  return PF_TIME | PF_VALID;
}

OBJECT_IMPL(PSAlwaysActivator, ParticleActivator);

PSAlwaysActivator::PSAlwaysActivator(void* _def)
  : ParticleActivator(_def) {
}

void PSAlwaysActivator::activate(float dt) {
  if(!enabled) return;

  char* valids = def->system_featurech(PF_VALID);

  for(int ii = 0; ii < def->n; ++ii) {
    valids[ii] = 1;
  }
}

OBJECT_IMPL(PSConstantRateActivator, ParticleActivator);
OBJECT_PROPERTY(PSConstantRateActivator, rate);

PSConstantRateActivator::PSConstantRateActivator(void* _def)
  : ParticleActivator(_def), rate(0), bucket(0) {
}

void PSConstantRateActivator::activate(float dt) {
  if(rate == 0) return;

  bucket += dt;

  char* valids = def->system_featurech(PF_VALID);
  int available = bucket * rate;
  if(available == 0) return;

  int actual = 0;
  for(int ii = 0; ii < def->n && available > 0; ++ii) {
    if(!valids[ii]) {
      valids[ii] = 1;
      ++actual;
      --available;
    }
  }

  bucket -= (actual / rate);
}
