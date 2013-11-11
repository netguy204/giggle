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
#ifndef PSYSTEM_H
#define PSYSTEM_H

#include "giggle.h"
#include "spriteatlas.h"
#include "color.h"
#include "utils.h"
#include "gameobject.h"

#define FEATURE_TABLE(m)                                        \
  m(PF_VALID, char, 0)                                          \
  m(PF_POSITION, Vector_, 1)                                    \
  m(PF_VELOCITY, Vector_, 2)                                    \
  m(PF_ACCELERATION, Vector_, 3)                                \
  m(PF_COLOR, Color, 4)                                         \
  m(PF_SCALE, float, 5)                                         \
  m(PF_SCALE_RATE, float, 6)                                    \
  m(PF_ANGLE, float, 7)                                         \
  m(PF_ANGLE_RATE, float, 8)                                    \
  m(PF_TIME, float, 9)                                          \
  m(PF_ENTRY, SpriteAtlasEntry, 10)

#define FEATURE_ENUM_GEN(name, type, idx)                               \
  name = 1<<(idx+1),

// position is an assumed feature of every particle. Need to keep
// track of alignment. Each property should be 8 byte aligned.
enum ParticleFeature {
  FEATURE_TABLE(FEATURE_ENUM_GEN)
};

class SystemDefinition;

class ParticleSystemComponent : public Object {
 public:
  OBJECT_PROTO(ParticleSystemComponent);

  ParticleSystemComponent(void* def);
  virtual ~ParticleSystemComponent();

  virtual unsigned int required_features();

  SystemDefinition* def;
  DLLNode_ node;
};

class ParticleSystemInitializer : public ParticleSystemComponent {
 public:
  OBJECT_PROTO(ParticleSystemInitializer);

  ParticleSystemInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
};

class ParticleSystemUpdater : public ParticleSystemComponent {
 public:
  OBJECT_PROTO(ParticleSystemUpdater);

  ParticleSystemUpdater(void* def);

  virtual void update(float dt);
};

class ParticleSystemTerminator : public ParticleSystemComponent {
 public:
  OBJECT_PROTO(ParticleSystemTerminator);

  ParticleSystemTerminator(void* def);

  virtual void terminateExpired();
};

class SystemRenderer : public Renderable {
 public:
  OBJECT_PROTO(SystemRenderer);
  SystemRenderer(void* empty);

  virtual void render(void *args);
  virtual void submit(Camera* camera, int layer, SystemDefinition* def);
};

class ParticleActivator : public ParticleSystemComponent {
 public:
  OBJECT_PROTO(ParticleActivator);
  ParticleActivator(void* def);

  virtual void activate(float dt);

  int enabled;
};

class SystemDefinition : public Object {
 public:
  OBJECT_PROTO(SystemDefinition);
  SystemDefinition(void* empty);

  SystemDefinition(unsigned int n);
  ~SystemDefinition();

  ParticleSystemComponent* add_component(TypeInfo* type);
  ParticleSystemComponent* find_component(TypeInfo* type);

  SystemRenderer* set_renderer(TypeInfo* type);
  ParticleActivator* set_activator(TypeInfo* type);

  void update(float dt);
  void render(Camera* camera);

  void init();

  float* system_featuref(ParticleFeature feature);
  Vector system_featurev(ParticleFeature feature);
  Color* system_featurec(ParticleFeature feature);
  char* system_featurech(ParticleFeature feature);
  int supports_feature(ParticleFeature feature);
  size_t features_size();

  // initializers (positions, velocities, etc. need one per feature)
  // updaters, N, hopefully each writing each property no more than once
  // terminator. 1, maybe. time or position based
  DLL_DECLARE(ParticleSystemInitializer, node) initializers;
  DLL_DECLARE(ParticleSystemUpdater, node) updaters;
  DLL_DECLARE(ParticleSystemTerminator, node) terminators;

  Random_ rgen;
  Component* component;
  SystemRenderer* renderer;
  ParticleActivator* activator;

  unsigned int n;
  unsigned int feature_flags;
  int layer;

  char* features;
};

class P_ESSystemRenderer : public SystemRenderer {
 public:
  OBJECT_PROTO(P_ESSystemRenderer);
  P_ESSystemRenderer(void* empty);

  virtual void render(void *args);
  virtual void submit(Camera* camera, int layer, SystemDefinition* def);

  SpriteAtlasEntry entry;
  float scale;
};

class PSC_E2SystemRenderer : public SystemRenderer {
 public:
  OBJECT_PROTO(PSC_E2SystemRenderer);
  PSC_E2SystemRenderer(void* empty);

  virtual void render(void *args);
  virtual void submit(Camera* camera, int layer, SystemDefinition* def);

  SpriteAtlasEntry entry;
};

class P_ES2SystemRenderer : public P_ESSystemRenderer {
 public:
  OBJECT_PROTO(P_ES2SystemRenderer);
  P_ES2SystemRenderer(void* empty);

  virtual void render(void *args);
};

class CParticleSystem : public Component {
 public:
  OBJECT_PROTO(CParticleSystem);

  CParticleSystem(void* go);
  virtual ~CParticleSystem();

  virtual void init();
  virtual void update(float dt);
  virtual void render(Camera* camera);

  SystemDefinition* def;
};

class PSConstantAccelerationUpdater : public ParticleSystemUpdater {
 public:
  OBJECT_PROTO(PSConstantAccelerationUpdater);

  PSConstantAccelerationUpdater(void* def);

  virtual void update(float dt);
  virtual unsigned int required_features();

  Vector_ acc;
};

class PSTimeAlphaUpdater : public ParticleSystemUpdater {
 public:
  OBJECT_PROTO(PSTimeAlphaUpdater);

  PSTimeAlphaUpdater(void* def);

  virtual void update(float dt);
  virtual unsigned int required_features();

  float time_constant;
  float max_scale;
};

class PSFireColorUpdater : public ParticleSystemUpdater {
 public:
  OBJECT_PROTO(PSFireColorUpdater);

  PSFireColorUpdater(void* def);

  virtual void update(float dt);
  virtual unsigned int required_features();

  float max_life;
  float start_temperature;
  float end_temperature;
};

class PSBoxInitializer : public ParticleSystemInitializer {
 public:
  OBJECT_PROTO(PSBoxInitializer);

  PSBoxInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
  virtual unsigned int required_features();

  Rect_ initial;
  Rect_ refresh;
  Vector_ minv;
  Vector_ maxv;
};

class PSTimeInitializer : public ParticleSystemInitializer {
 public:
  OBJECT_PROTO(PSTimeInitializer);

  PSTimeInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
  virtual unsigned int required_features();

  float min_life;
  float max_life;
};

class PSRandColorInitializer : public ParticleSystemInitializer {
 public:
  OBJECT_PROTO(PSRandColorInitializer);

  PSRandColorInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
  virtual unsigned int required_features();

  Color min_color;
  Color max_color;
};

class PSRandScaleInitializer : public ParticleSystemInitializer {
 public:
  OBJECT_PROTO(PSRandScaleInitializer);

  PSRandScaleInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
  virtual unsigned int required_features();

  float min_scale;
  float max_scale;
};

class PSConstColorInitializer : public ParticleSystemInitializer {
 public:
  OBJECT_PROTO(PSConstColorInitializer);
  PSConstColorInitializer(void* def);

  virtual void firstInitialize();
  virtual void initialize();
  virtual unsigned int required_features();

  Color color;
};

class PSBoxTerminator : public ParticleSystemTerminator {
 public:
  OBJECT_PROTO(PSBoxTerminator);

  PSBoxTerminator(void* def);

  virtual void terminateExpired();
  virtual unsigned int required_features();

  Rect_ rect;
};

class PSTimeTerminator : public ParticleSystemTerminator {
 public:
  OBJECT_PROTO(PSTimeTerminator);

  PSTimeTerminator(void* def);

  virtual void terminateExpired();
  virtual unsigned int required_features();
};

class PSAlwaysActivator : public ParticleActivator {
 public:
  OBJECT_PROTO(PSAlwaysActivator);

  PSAlwaysActivator(void* def);

  virtual void activate(float dt);
};

class PSConstantRateActivator : public ParticleActivator {
 public:
  OBJECT_PROTO(PSConstantRateActivator);

  PSConstantRateActivator(void* def);

  virtual void activate(float dt);

  float rate;
  float bucket;
};

// template magic to make SystemDefinition settable from lua
template<>
inline void LCcheck<SystemDefinition*>(lua_State* L, SystemDefinition** def, int pos) {
  if(lua_isuserdata(L, pos)) {
    *def = (SystemDefinition*)LCcheck_object(L, pos);
    return;
  }

  if(!lua_istable(L, pos)) {
    luaL_error(L, "position %d does not contain a sysdef table", pos);
  }

  // number of particles
  lua_getfield(L, pos, "n");
  int n = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  // delete the old system if there is one
  if(*def) {
    delete *def;
  }

  *def = new SystemDefinition(n);

  // renderer
  lua_getfield(L, pos, "renderer");
  if(!lua_istable(L, -1)) {
    luaL_error(L, "sysdef table must include a renderer table");
  }
  lua_getfield(L, -1, "name");
  TypeInfo* type;
  LCcheck(L, &type, -1);
  Renderable *renderer = (*def)->set_renderer(type);
  lua_pop(L, 1);

  lua_getfield(L, -1, "params");
  if(!lua_isnil(L, -1)) {
    LCconfigure_object(L, renderer, -1);
  }
  lua_pop(L, 1); // params
  lua_pop(L, 1); // renderer
  // end renderer

  // activator
  lua_getfield(L, pos, "activator");
  if(lua_istable(L, -1)) {
    lua_getfield(L, -1, "name");
    LCcheck(L, &type, -1);
    ParticleActivator *activator = (*def)->set_activator(type);
    lua_pop(L, 1);

    lua_getfield(L, -1, "params");
    if(!lua_isnil(L, -1)) {
      LCconfigure_object(L, activator, -1);
    }
    lua_pop(L, 1); // params
  }
  lua_pop(L, 1); // activator
  // end activator

  lua_getfield(L, pos, "layer");
  if(!lua_isnil(L, -1)) {
    (*def)->layer = luaL_checknumber(L, -1);
  }
  lua_pop(L, 1);

  // components
  lua_getfield(L, pos, "components");
  if(!lua_istable(L, -1)) {
    luaL_error(L, "sysdef table must contain a components array");
  }

  // iterate over the components
  int idx = 1;
  lua_rawgeti(L, -1, idx++);
  while(!lua_isnil(L, -1)) {
    if(!lua_istable(L, -1)) {
      luaL_error(L, "sysdef tables must contain only psc tables");
    }

    // create the component
    lua_getfield(L, -1, "name");
    TypeInfo* type;
    LCcheck(L, &type, -1);
    ParticleSystemComponent* comp = (*def)->add_component(type);
    lua_pop(L, 1);

    // configure the component
    lua_getfield(L, -1, "params");
    if(!lua_isnil(L, -1)) {
      LCconfigure_object(L, comp, -1);
    }
    lua_pop(L, 1);

    // pop the current component entry
    lua_pop(L, 1);
    lua_rawgeti(L, -1, idx++);
  }
  // pop the last attempted value (a nil)
  // pop the components table
  lua_pop(L, 2);
}

#endif
