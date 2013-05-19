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
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "vector.h"
#include "rect.h"
#include "spriteatlas.h"
#include "ooc.h"
#include "soundmgr.h"
#include "spriter.h"
#include "pipeline.h"
#include "color.h"
#include "utils.h"
#include "input.h"

#include <lua.hpp>
#include <Box2D/Box2D.h>

#include <map>

#define MAX_INFLIGHT_INPUTS 512

#define BSCALE 64.0f

#define LUT_WORLD "World"
#define LUT_GO "Go"
#define LUT_COMPONENT "Component"
#define LUT_UNIVERSE "Universe"
#define LUT_PSDEFINITION "PSDefinition"
#define LUT_PSCOMPONENT "PSComponent"
#define LUT_AUDIOHANDLE "AudioHandle"
#define LUT_JOINT "Joint"

typedef enum {
  MASK_NON_COLLIDER = 0,
  MASK_PLATFORM = 1,
  MASK_PLATFORMER = 2,
  MASK_ENEMY_PLATFORM = 4
} CollisionMask;

class GO;

enum MessageKind {
  MESSAGE_COLLIDING,      // agent is colliding with other (cother, cself)
  MESSAGE_TIMER_EXPIRED,  // args (payload)
  MESSAGE_EXPLOSION_NEARBY,
  MESSAGE_PARENT_CHANGE,
};

struct LString {
  size_t length;
  char str[0];
};

LString* malloc_lstring(const char* data, size_t length);
void free_lstring(LString* str);

LString* frame_alloc_lstring(const char* data, size_t length);

class Message {
 public:
  void operator delete(void* obj);

  struct DLLNode_ node;
  GO* source;
  int kind;
  size_t nbytes;
  char content[0];
};

enum RenderLayers {
  LAYER_BACKERDROP,
  LAYER_BACKDROP,
  LAYER_BACKERGROUND,
  LAYER_BACKGROUND,
  LAYER_PLAYER,
  LAYER_FOREGROUND,
  LAYER_MENU,
  LAYER_MAX,
};

class World;

class Scene {
 public:
  RenderableCommand* renderables[LAYER_MAX];

  // base layers are interpreted as if they contain only basesprite
  // representations (no rotation, color, or shifted origin)
  BaseSprite baseLayers[LAYER_MAX];

  // layers must include rotation
  BaseSprite layers[LAYER_MAX];

  // particles must also include color
  BaseSprite particles[LAYER_MAX];
  ColoredRect testRects[LAYER_MAX];

  int dx, dy;
  Rect_ camera_rect;
  World* world;

  Scene(World* world);

  void addRenderable(int layer, Renderable* renderable, void* args);
  void addRelative(BaseSprite* list, BaseSprite sprite);
  void addAbsolute(BaseSprite* list, BaseSprite sprite);
  void addRelative(ColoredRect* list, ColoredRect rect);

  void start(); // after the camera is ready
  void enqueue();
};

enum ComponentPriority {
  PRIORITY_THINK,
  PRIORITY_ACT,
  PRIORITY_SHOW,
  PRIORITY_LEAST
};

class Component : public Object {
 public:
  OBJECT_PROTO(Component);

  // invalid constructor, throws
  Component(void* invalid);
  Component(GO* go, ComponentPriority priority);
  virtual ~Component();

  virtual void init();
  virtual void update(float dt);
  virtual void messages_received();

  inline Scene* scene();
  inline GO* camera();


  struct DLLNode_ node;
  struct DLLNode_ world_node;

  GO* go;
  ComponentPriority priority;
  int delete_me;
};

void LCpush_lut(lua_State *L, const char* metatable, void* ut);
void LCpush_entry(lua_State* L, SpriteAtlasEntry entry);
SpriteAtlasEntry LCcheck_entry(lua_State* L, int pos);
void LCpush_component(lua_State *L, Component *comp);

void LCconfigure_object(lua_State *L, Object* obj, int pos);
TypeInfo* LCcheck_type(lua_State *L, int pos);

class Fixture {
 public:
  Fixture();
  Fixture(Component* comp);

  Component* comp;
  b2Fixture* fixture;
};

class CCollidable : public Component {
 public:
  OBJECT_PROTO(CCollidable);

  CCollidable(void* go);
  virtual ~CCollidable();

  Fixture fixture;
};

class CSensor : public Component {
 public:
  OBJECT_PROTO(CSensor);

  CSensor(void* go);
  virtual ~CSensor();
  virtual void update(float dt);

  Fixture fixture;
  int kind;
};

struct LuaThread {
  LuaThread();

  lua_State* state;
  int refid;
  int is_initialized;
  int is_valid;
};

class CScripted : public Component {
 public:
  OBJECT_PROTO(CScripted);

  CScripted(void* go);
  virtual ~CScripted();

  virtual void init();
  virtual void update(float dt);
  virtual void messages_received();

  void set_thread(LuaThread* target, lua_State* thread);
  void set_update_thread(lua_State* thread);
  void set_message_thread(lua_State* thread);
  lua_State* get_update_thread();
  lua_State* get_message_thread();

  void free_thread(LuaThread* thread);
  void step_thread(LuaThread* thread);
  void resume(LuaThread* thread, int args);

  LuaThread update_thread;
  LuaThread message_thread;
};

/* GO: GameObject
 *
 * This is an agent that makes all of its messgaes available to its
 * components. If a component ever cares about messages it should
 * search the list of messages in its parent object for whatever its
 * interested in. The parent will clear its message list and handle
 * terminations at the end of the component update cycle.
 */

class GO : public Object {
 public:
  OBJECT_PROTO(GO);

  GO(void*);
  virtual ~GO();

  Component* add_component(TypeInfo* type);

  template<typename T>
  T* add_c() {
    return (T*)add_component(&T::Type);
  }

  virtual void update(float dt);
  void messages_received();

  Message* create_message(int kind, const char* content, size_t nbytes);
  void send_message(Message* message);

  float get_gravity_scale();
  void set_gravity_scale(float scale);

  int fixed_rotation();
  void set_fixed_rotation(int flag);

  // fast way (no allocation)
  void pos(Vector p);
  void vel(Vector v);
  void set_pos(Vector p);
  void set_vel(Vector v);

  float angle();
  void set_angle(float angle);

  float angle_rate();
  void set_angle_rate(float dadt);

  // script way
  Vector_ slow_get_pos();
  Vector_ slow_get_vel();
  void slow_set_pos(Vector_ p);
  void slow_set_vel(Vector_ p);

  // type is the b2BodyType enum
  void set_body_type(int type);
  int get_body_type();

  void set_active(int val);
  int get_active();

  void apply_impulse(Vector imp);
  void apply_angular_impulse(float angimp);
  void apply_torque(float torque);

  Component* find_component(const TypeInfo* info, Component* last);
  void print_description();

  struct DLLNode_ messages_waiting_node;
  struct DLLNode_ world_node;

  DLL_DECLARE(Component, node) components;
  DLL_DECLARE(Component, node) uninitialized_components;
  DLL_DECLARE(Message, node) inbox;
  DLL_DECLARE(Message, node) inbox_pending;

  // the world we're a part of
  World* world;
  LString* stash;
  b2Body* body;

  int delete_me;
};

void LCpush_go(lua_State *L, GO* go);
GO* LCcheck_go(lua_State *L, int pos);
void LCpush_vector(lua_State *L, Vector vector);

typedef std::map<const char*, SpriteAtlas, cmp_str> NameToAtlas;
typedef std::map<const char*, Entity*, cmp_str> NameToEntity;
typedef std::map<long, AudioHandle*> LongToHandle;

struct Cone {
  Vector_ point;
  Vector_ direction;
  float angle;
};

class Universe;

class LuaKeyBinding : public Binding {
 public:
  LuaKeyBinding(KeyNumber keyn, World* world);
  virtual ~LuaKeyBinding();

  virtual void activate(float value);

  World* world;
  DLLNode_ node;

  // stash so we can unregister at destruct
  KeyNumber keyn;
};

class LuaSIBinding : public SpatialInputBinding {
 public:
  LuaSIBinding(SpatialInputNumber keyn, World* world);
  virtual ~LuaSIBinding();

  virtual void activate(SpatialInput* input);

  World* world;
  DLLNode_ node;

  // stash so we can unregister at destruct
  SpatialInputNumber keyn;
};

class World : public Object {
 public:
  OBJECT_PROTO(World);

  World(void*);
  virtual ~World();

  virtual void update(long delta);
  void update_camera(float dt);

  void load_level(const char* level);

  SpriteAtlas atlas(const char* atlas);
  SpriteAtlasEntry atlas_entry(const char* atlas, const char* entry);
  Animation* animation(const char* scml, SpriteAtlas atlas, const char* anim);

  void broadcast_message(GO* go, float radius, int kind, const char* content, size_t nbytes);
  GO* next_in_cone(GO* last, Rect bounds, Cone* cone);
  void set_time_scale(float scale);
  float get_time_scale();
  void set_gravity(Vector_ vector);
  Vector_ get_gravity();
  GO* create_go();

  GO* camera;
  GO* stage;
  GO* focus;

  Universe* universe;

  Clock clock;
  Clock camera_clock;

  Scene scene;

  lua_State* L;
  b2World bWorld;

  void enqueue_command(CommandFunction fn, void* data);
  void evaluate_commands();

  FixedAllocator lk_alloc;
  FixedAllocator cmd_alloc;
  CommandQueue command_queue;

  float dt;

  DLL_DECLARE(GO, world_node) game_objects;
  DLL_DECLARE(GO, messages_waiting_node) have_waiting_messages;
  DLL_DECLARE(LuaKeyBinding, node) keybindings;
  DLL_DECLARE(LuaSIBinding, node) sibindings;

  DLL_DECLARE(Component, world_node) components;
};

int LCpush_world(lua_State *L, World* world);

class WorldPipelineDelegate : public PipelineElement {
 public:
  WorldPipelineDelegate(World* world);

  virtual void update(long delta);

  World* world;
};

class Universe : public Object {
 public:
  OBJECT_PROTO(Universe);

  Universe(void* path);
  ~Universe();

  SpriteAtlas atlas(const char* atlas);
  SpriteAtlasEntry atlas_entry(const char* atlas, const char* entry);

  Entity* scml_entity(const char* filename, SpriteAtlas atlas);
  Animation* animation(const char* scml, SpriteAtlas atlas, const char* anim);

  virtual void update(long delta);

  AudioHandle* play_sound(Sound* sound, int channel);
  AudioHandle* stream_sound(const char* fname, long start);
  AudioHandle* sound_handle(long handle);

  SoundMgr sound;

  LString* stash;
  LString* lua_path;

  NameToAtlas name_to_atlas;
  NameToEntity name_to_entity;
  LongToHandle long_to_handle;
};

Universe* LCcheck_universe(lua_State *L, int pos);
void LCpush_universe(lua_State *L, Universe* universe);

template <typename Func>
class WorldCallback : public b2QueryCallback {
 public:
 WorldCallback(Func& func) : func(func) {}

  bool ReportFixture(b2Fixture* fixture) {
    return !func((GO*)fixture->GetBody()->GetUserData(), fixture);
  }

  Func& func;
};

template<typename Func>
void world_foreach(World* world, Rect rect, Func func) {
  b2AABB aabb;
  aabb.lowerBound.Set(rect->minx/BSCALE, rect->miny/BSCALE);
  aabb.upperBound.Set(rect->maxx/BSCALE, rect->maxy/BSCALE);
  WorldCallback<Func> callback(func);
  world->bWorld.QueryAABB(&callback, aabb);
}

template<typename Func>
void world_foreach(World* world, Vector pos, float rad, Func func) {
  Rect_ rect = {pos->x - rad, pos->y - rad, pos->x + rad, pos->y + rad};
  world_foreach(world, &rect, func);
}

int point_in_cone(Cone* cone, Vector point);


inline Scene* Component::scene() {
  return &go->world->scene;
}

inline GO* Component::camera() {
  return go->world->camera;
}

// lua property accessors
void LCpush_animation(lua_State *L, Animation* anim);
Animation* LCcheck_animation(lua_State *L, int pos);
void LCpush_vector(lua_State* L, Vector v);
void LCcheck_vector(lua_State* L, int pos, Vector v);
void LCpush_lstring(lua_State *L, LString* str);
LString* LCcheck_lstring(lua_State *L, int pos);
void LCpush_color(lua_State *L, Color *c);
void LCcheck_color(lua_State *L, int pos, Color *c);

template<>
inline void LCpush<Animation*>(lua_State* L, Animation* anim) {
  LCpush_animation(L, anim);
}

template<>
inline void LCcheck<Animation*>(lua_State* L, Animation** anim, int pos) {
  *anim = LCcheck_animation(L, pos);
}

template<>
inline void LCpush<Rect_>(lua_State* L, Rect_ rect) {
  lua_createtable(L, 4, 0);
  lua_rawseti(L, 1, rect.minx);
  lua_rawseti(L, 2, rect.miny);
  lua_rawseti(L, 3, rect.maxx);
  lua_rawseti(L, 4, rect.maxy);
}

template<>
inline void LCcheck<Rect_>(lua_State* L, Rect_* rect, int pos) {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "position %d does not contain a rect table", pos);
  }

  lua_rawgeti(L, pos, 1);
  rect->minx = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 2);
  rect->miny = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 3);
  rect->maxx = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 4);
  rect->maxy = luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

template<>
inline void LCpush<SpriteAtlasEntry>(lua_State* L, SpriteAtlasEntry entry) {
  LCpush_entry(L, entry);
}

template<>
inline void LCcheck<SpriteAtlasEntry>(lua_State* L, SpriteAtlasEntry* entry, int pos) {
  *entry = LCcheck_entry(L, pos);
}

template<>
inline void LCcheck<LuaThread>(lua_State* L, LuaThread* thread, int pos) {
  if(thread->state) {
    luaL_unref(L, LUA_REGISTRYINDEX, thread->refid);
    thread->state = NULL;
  }

  thread->state = lua_tothread(L, pos);
  if(!thread->state) {
    luaL_error(L, "position %d does not contain a thread", pos);
  }

  lua_pushvalue(L, pos);
  thread->refid = luaL_ref(L, LUA_REGISTRYINDEX);
  thread->is_initialized = 0;
}

template<>
inline void LCpush<Vector_>(lua_State* L, Vector_ v) {
  LCpush_vector(L, &v);
}

template<>
inline void LCcheck<Vector_>(lua_State* L, Vector_* v, int pos) {
  LCcheck_vector(L, pos, v);
}

template<>
inline void LCpush<GO*>(lua_State* L, GO* go) {
  LCpush_go(L, go);
}

template<>
inline void LCcheck<GO*>(lua_State* L, GO** go, int pos) {
  *go = LCcheck_go(L, pos);
}

template<>
inline void LCpush<LString*>(lua_State* L, LString* str) {
  LCpush_lstring(L, str);
}


template<>
inline void LCcheck<LString*>(lua_State* L, LString** str, int pos) {
  if(*str) {
    free_lstring(*str);
  }

  *str = LCcheck_lstring(L, pos);
}

template<>
inline void LCpush<Color>(lua_State* L, Color c) {
  LCpush_color(L, &c);
}


template<>
inline void LCcheck<Color>(lua_State* L, Color* c, int pos) {
  LCcheck_color(L, pos, c);
}

#define OPT_PARM(pos, name, fetch)              \
  do {                                          \
    lua_getfield(L, pos, name);                 \
    if(!lua_isnil(L, -1)) {                     \
      fetch;                                    \
    }                                           \
    lua_pop(L, 1);                              \
  } while(0)

template<>
inline void LCcheck<Fixture>(lua_State* L, Fixture* fixture, int pos) {
  if(fixture->fixture) {
    fixture->fixture->GetBody()->DestroyFixture(fixture->fixture);
  }

  if(!lua_istable(L, pos)) {
    luaL_error(L, "fixture table required");
  }

  b2FixtureDef fixtureDef;

  // only one will be used but we need to scope here to prevent
  // destruct
  b2PolygonShape shape;
  b2PolygonShape poly;

  OPT_PARM(pos, "density", fixtureDef.density = luaL_checknumber(L, -1));
  OPT_PARM(pos, "category", fixtureDef.filter.categoryBits = luaL_checkinteger(L, -1));
  OPT_PARM(pos, "mask", fixtureDef.filter.maskBits = luaL_checkinteger(L, -1));
  OPT_PARM(pos, "group", fixtureDef.filter.groupIndex = luaL_checkinteger(L, -1));
  OPT_PARM(pos, "sensor", fixtureDef.isSensor = lua_toboolean(L, -1));

  lua_getfield(L, pos, "type");
  const char* type = luaL_checkstring(L, -1);
  lua_pop(L, 1);

  if(strcmp(type, "rect") == 0) {
    lua_getfield(L, pos, "w");
    if(!lua_isnumber(L, -1)) {
      luaL_error(L, "rect fixture type requires a width (w)");
    }
    float w = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, pos, "h");
    if(!lua_isnumber(L, -1)) {
      luaL_error(L, "rect fixture type requires a height (h)");
    }
    float h = lua_tonumber(L, -1);
    lua_pop(L, 1);

    Vector_ offset;
    vector_zero(&offset);
    OPT_PARM(pos, "center", LCcheck_vector(L, -1, &offset));

    // build the fixture
    b2Vec2 center;
    center.x = offset.x / BSCALE;
    center.y = offset.y / BSCALE;

    shape.SetAsBox((w/2)/BSCALE, (h/2)/BSCALE, center, 0);

    fixtureDef.shape = &shape;
  } else if(strcmp(type, "poly") == 0) {
    lua_getfield(L, pos, "points");
    if(!lua_istable(L, -1)) {
      luaL_error(L, "poly fixture requires point table");
    }

    b2Vec2 corners[b2_maxPolygonVertices];

    int idx = 1;
    lua_rawgeti(L, -1, idx++);
    while(lua_istable(L, -1)) {
      Vector_ point;
      LCcheck_vector(L, -1, &point);

      if(idx - 2 < b2_maxPolygonVertices) {
        corners[idx - 2] = b2Vec2(point.x / BSCALE, point.y / BSCALE);
      } else {
        luaL_error(L, "poly fixture can not have more than %d points", b2_maxPolygonVertices);
      }

      lua_pop(L, 1);
      lua_rawgeti(L, -1, idx++);
    }
    lua_pop(L, 1); // last non-vector
    lua_pop(L, 1); // points table

    poly.Set(corners, idx - 2);
    fixtureDef.shape = &poly;
  } else {
    luaL_error(L, "fixture type `%s' is not recognized", type);
  }

  // build the fixture
  fixture->fixture = fixture->comp->go->body->CreateFixture(&fixtureDef);
  fixture->fixture->SetUserData(fixture->comp);
}

template<>
inline void LCpush<b2Fixture*>(lua_State* L, b2Fixture* fixture) {
  lua_pushnil(L);
}

#endif
