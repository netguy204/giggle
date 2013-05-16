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
#include "gameobject.h"
#include "memlib.h"
#include "config.h"
#include "testlib.h"
#include "utils.h"
#include "psystem.h"

#include <math.h>
#include <Box2D/Dynamics/Joints/b2Joint.h>

void LCpush_entry(lua_State* L, SpriteAtlasEntry entry) {
  lua_newtable(L);
  lua_pushlightuserdata(L, entry);
  lua_setfield(L, -2, "entry");
  lua_pushinteger(L, entry->w);
  lua_setfield(L, -2, "w");
  lua_pushinteger(L, entry->h);
  lua_setfield(L, -2, "h");
}

SpriteAtlasEntry LCcheck_entry(lua_State* L, int pos) {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "position %d does not contain a table", pos);
  }

  lua_getfield(L, pos, "entry");
  SpriteAtlasEntry entry = (SpriteAtlasEntry)lua_touserdata(L, -1);
  if(!entry) {
    luaL_error(L, "table at position %d does not have lightuserdata at `entry'", pos);
  }

  lua_pop(L, 1);
  return entry;
}

void init_lstring(LString* str, const char* data, size_t length) {
  str->length = length;
  if(data) {
    memcpy(str->str, data, length);
  }
  str->str[str->length] = '\0';
}

LString* malloc_lstring(const char* data, size_t length) {
  LString* result = (LString*)malloc(sizeof(LString) + length + 1);
  init_lstring(result, data, length);
  return result;
}

void free_lstring(LString* str) {
  free(str);
}

LString* frame_alloc_lstring(const char* data, size_t length) {
  LString *result = (LString*)frame_alloc(sizeof(LString) + length + 1);
  init_lstring(result, data, length);
  return result;
}

void LCpush_lstring(lua_State *L, LString* str) {
  if(str) {
    lua_pushlstring(L, str->str, str->length);
  } else {
    lua_pushnil(L);
  }
}

LString* LCcheck_lstring(lua_State *L, int pos) {
  size_t length;
  const char* value = lua_tolstring(L, pos, &length);
  return malloc_lstring(value, length);
}

void LCpush_color(lua_State *L, Color *c) {
  lua_createtable(L, 4, 0);

  lua_pushnumber(L, c->r);
  lua_rawseti(L, -2, 1);

  lua_pushnumber(L, c->g);
  lua_rawseti(L, -2, 2);

  lua_pushnumber(L, c->b);
  lua_rawseti(L, -2, 3);

  lua_pushnumber(L, c->a);
  lua_rawseti(L, -2, 4);
}

void LCcheck_color(lua_State *L, int pos, Color *c) {
  if(!lua_istable(L, pos)) {
    luaL_argerror(L, pos, "`color table' expected");
  }

  lua_rawgeti(L, pos, 1);
  c->r = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 2);
  c->g = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 3);
  c->b = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 4);
  c->a = luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

void LCpush_message(lua_State *L, Message* msg) {
  lua_newtable(L);
  LCpush_go(L, msg->source);
  lua_setfield(L, -2, "source");
  lua_pushlstring(L, msg->content, msg->nbytes);
  lua_setfield(L, -2, "content");
  lua_pushlightuserdata(L, msg);
  lua_setfield(L, -2, "message");
}

Message* LCcheck_message(lua_State *L, int pos) {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "expected message, got non-message-table");
  }
  lua_getfield(L, pos, "message");
  Message* result = (Message*)lua_touserdata(L, -1);
  lua_pop(L, 1);
  if(!result) {
    luaL_error(L, "`message' field of message-table was not set");
  }
  return result;
}

void LCpush_animation(lua_State *L, Animation* anim) {
  lua_newtable(L);
  lua_pushnumber(L, anim->length_ms / 1000.0);
  lua_setfield(L, -2, "length");
  lua_pushboolean(L, anim->looping);
  lua_setfield(L, -2, "looping");
  lua_pushlightuserdata(L, anim);
  lua_setfield(L, -2, "animation");
}

Animation* LCcheck_animation(lua_State *L, int pos) {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "no animation-table at %d", pos);
  }

  lua_getfield(L, pos, "animation");
  Animation* anim = (Animation*)lua_touserdata(L, -1);
  lua_pop(L, 1);

  if(!anim) {
    luaL_error(L, "animation-table did not contain `animation' field at %d", pos);
  }
  return anim;
}

void LCpush_vector(lua_State* L, Vector v) {
  lua_createtable(L, 2, 0);
  lua_pushnumber(L, v->x);
  lua_rawseti(L, -2, 1);
  lua_pushnumber(L, v->y);
  lua_rawseti(L, -2, 2);
}

void LCcheck_vector(lua_State* L, int pos, Vector v) {
  if(!lua_istable(L, pos)) {
    luaL_argerror(L, pos, "`vector table' expected");
  }

  lua_rawgeti(L, pos, 1);
  v->x = luaL_checknumber(L, -1);
  lua_pop(L, 1);
  lua_rawgeti(L, pos, 2);
  v->y = luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

void Message::operator delete(void* obj) {
  fail_exit("no need to delete messages");
  // this is because messages must be fully stack allocated
}

Scene::Scene(World* world)
  : world(world) {
  memset(renderables, 0, sizeof(renderables));
  memset(baseLayers, 0, sizeof(baseLayers));
  memset(layers, 0, sizeof(layers));
  memset(particles, 0, sizeof(particles));
  memset(testRects, 0, sizeof(testRects));
}

void Scene::addRenderable(int layer, Renderable* renderable, void* args) {
  RenderableCommand* command = (RenderableCommand*)frame_alloc(sizeof(RenderableCommand));
  command->renderable = renderable;
  command->args = args;
  command->next = renderables[layer];
  renderables[layer] = command;
}

void Scene::addRelative(BaseSprite* list, BaseSprite sprite) {
  sprite->displayX -= dx;
  sprite->displayY -= dy;
  addAbsolute(list, sprite);
}

void Scene::addAbsolute(BaseSprite* list, BaseSprite sprite) {
  sprite_append(*list, sprite);
}

void Scene::addRelative(ColoredRect* list, ColoredRect rect) {
  rect->minx -= dx;
  rect->maxx -= dx;
  rect->miny -= dy;
  rect->maxy -= dy;

  rect->next = *list;
  *list = rect;
}

void Scene::start() {
  Vector_ cpos;
  world->camera->pos(&cpos);
  dx = floorf(cpos.x);
  dy = floorf(cpos.y);
  camera_rect.minx = dx;
  camera_rect.miny = dy;
  camera_rect.maxx = dx + screen_width;
  camera_rect.maxy = dy + screen_height;
}

void Scene::enqueue() {
  for(int ii = 0; ii < LAYER_MAX; ++ii) {
    if(renderables[ii]) {
      renderables_enqueue_for_screen(renderables[ii]);
      renderables[ii] = NULL;
    }
    if(baseLayers[ii]) {
      renderable_enqueue_for_screen(basespritelist_renderer, baseLayers[ii]);
    }
    if(layers[ii]) {
      renderable_enqueue_for_screen(spritelist_renderer, layers[ii]);
    }
    if(particles[ii]) {
      renderable_enqueue_for_screen(coloredspritelist_renderer, particles[ii]);
    }
    while(testRects[ii]) {
      renderable_enqueue_for_screen(filledrect_renderer, testRects[ii]);
      testRects[ii] = testRects[ii]->next;
    }
    layers[ii] = NULL;
    particles[ii] = NULL;
    baseLayers[ii] = NULL;
  }
}

OBJECT_IMPL(GO, Object);
OBJECT_ACCESSOR(GO, gravity_scale, get_gravity_scale, set_gravity_scale);
OBJECT_ACCESSOR(GO, pos, slow_get_pos, slow_set_pos);
OBJECT_ACCESSOR(GO, vel, slow_get_vel, slow_set_vel);
OBJECT_ACCESSOR(GO, body_type, get_body_type, set_body_type);
OBJECT_ACCESSOR(GO, active, get_active, set_active);
OBJECT_ACCESSOR(GO, angle, angle, set_angle);
OBJECT_ACCESSOR(GO, fixed_rotation, fixed_rotation, set_fixed_rotation);
OBJECT_ACCESSOR(GO, angle_rate, angle_rate, set_angle_rate);
OBJECT_PROPERTY(GO, delete_me);

GO::GO(void* _world)
  : world((World*)_world), delete_me(0), stash(NULL) {
  world->game_objects.add_head(this);

  // the manual says to avoid setting major body properties (like
  // position or dynamic/static) after creation... i'll need to keep
  // tabs on whether this becomes a real issue.
  b2BodyDef bodyDef;
  bodyDef.fixedRotation = true;
  body = world->bWorld.CreateBody(&bodyDef);
  body->SetUserData(this);
}

GO::~GO() {
  this->components.foreach([](Component* comp) -> int {
      delete comp;
      return 0;
    });

  world->game_objects.remove(this);

  // if we had pending messages we need to leave the world pending
  // list
  if(inbox_pending.head) {
    world->have_waiting_messages.remove(this);
  }

  if(stash) free_lstring(stash);

  world->bWorld.DestroyBody(body);
}

Component* GO::add_component(TypeInfo* type) {
  return (Component*)type->makeInstance(this);
}

void GO::update(float dt) {
  // initialize new components
  uninitialized_components.foreach([this](Component* comp) -> int {
      comp->init();
      uninitialized_components.remove(comp);
      components.insert_before_when(comp, [&comp, this](Component* other) {
          return comp->priority < other->priority;
        });
      world->components.insert_before_when(comp, [&](Component* other) {
          return comp->priority < other->priority;
        });
      return 0;
    });
}

void GO::messages_received() {
  // notify components
  components.foreach([](Component* comp) -> int {
      comp->messages_received();
      return 0;
    });

  // clear the inbox. remember, messages are stack allocated so we
  // don't need to free them explicitly
  inbox.zero();
}

float GO::get_gravity_scale() {
  return body->GetGravityScale();
}

void GO::set_gravity_scale(float scale) {
  body->SetGravityScale(scale);
}

void GO::pos(Vector pos) {
  const b2Vec2& bPos = body->GetPosition();
  pos->x = bPos.x * BSCALE;
  pos->y = bPos.y * BSCALE;
}

void GO::vel(Vector vel) {
  const b2Vec2& bVel = body->GetLinearVelocity();
  vel->x = bVel.x * BSCALE;
  vel->y = bVel.y * BSCALE;
}

void GO::set_pos(Vector pos) {
  b2Vec2 bPos;
  bPos.x = pos->x / BSCALE;
  bPos.y = pos->y / BSCALE;
  body->SetTransform(bPos, body->GetAngle());
}

void GO::set_vel(Vector vel) {
  b2Vec2 bVel;
  bVel.x = vel->x / BSCALE;
  bVel.y = vel->y / BSCALE;
  body->SetLinearVelocity(bVel);
}

float GO::angle() {
  return body->GetAngle();
}

void GO::set_angle(float angle) {
  body->SetTransform(body->GetPosition(), angle);
}

float GO::angle_rate() {
  return body->GetAngularVelocity();
}

void GO::set_angle_rate(float dadt) {
  body->SetAngularVelocity(dadt);
}

int GO::fixed_rotation() {
  return body->IsFixedRotation();
}

void GO::set_fixed_rotation(int flag) {
  body->SetFixedRotation(flag);
}

Vector_ GO::slow_get_pos() {
  Vector_ result;
  pos(&result);
  return result;
}

Vector_ GO::slow_get_vel() {
  Vector_ result;
  vel(&result);
  return result;
}

void GO::slow_set_pos(Vector_ pos) {
  set_pos(&pos);
}

void GO::slow_set_vel(Vector_ vel) {
  set_vel(&vel);
}

void GO::set_body_type(int type) {
  body->SetType((b2BodyType)type);
}

int GO::get_body_type() {
  return body->GetType();
}

void GO::set_active(int val) {
  body->SetActive(val);
}

int GO::get_active() {
  return body->IsActive();
}

void GO::apply_impulse(Vector imp) {
  b2Vec2 bImp;
  bImp.x = imp->x / BSCALE;
  bImp.y = imp->y / BSCALE;
  body->ApplyLinearImpulse(bImp, body->GetWorldCenter());
}

void GO::apply_angular_impulse(float angimp) {
  body->ApplyAngularImpulse(angimp);
}

void GO::apply_torque(float torque) {
  body->ApplyTorque(torque);
}

Component* GO::find_component(const TypeInfo* info, Component* last) {
  DLLNode node = components.head;
  if(last) {
    node = last->node.next;
  }

  while(node) {
    Component* comp = components.to_element(node);
    if(comp->typeinfo()->isInstanceOf(info)) {
      return comp;
    }
    node = node->next;
  }

  if(last && uninitialized_components.contains(last)) return NULL;

  node = uninitialized_components.head;
  while(node) {
    Component* comp = uninitialized_components.to_element(node);
    if(comp->typeinfo()->isInstanceOf(info)) {
      return comp;
    }
    node = node->next;
  }
  return NULL;
}

void GO::print_description() {
  fprintf(stderr, "UNINITIALIZED COMPONENTS\n");
  uninitialized_components.foreach([](Component* c) -> int {
      fprintf(stderr, "%s\n", c->typeinfo()->name());
      return 0;
    });
  fprintf(stderr, "ACTIVE COMPONENTS\n");
  components.foreach([](Component* c) -> int {
      fprintf(stderr, "%s\n", c->typeinfo()->name());
      return 0;
    });
}

Message* GO::create_message(int kind, const char* content, size_t nbytes) {
  Message* msg = (Message*)frame_alloc(sizeof(Message) + nbytes);
  msg->source = this;
  msg->kind = kind;
  msg->nbytes = nbytes;
  memcpy(msg->content, content, nbytes);
  return msg;
}

void GO::send_message(Message* message) {
  if(inbox_pending.is_empty()) {
    world->have_waiting_messages.add_head(this);
  }

  inbox_pending.add_tail(message);
}

OBJECT_IMPL(Component, Object);
OBJECT_PROPERTY(Component, delete_me);

Component::Component(void* p)
  : go(NULL), delete_me(0) {
}

Component::Component(GO* go, ComponentPriority priority)
  : go(NULL), priority(priority), delete_me(0) {

  this->go = go;
  go->uninitialized_components.add_head(this);
}

Component::~Component() {
  if(this->go) {
    this->go->components.remove(this);
    this->go->world->components.remove(this);
  }
}

void Component::init() {
}

void Component::update(float dt) {
}

void Component::messages_received() {
}

OBJECT_IMPL(CCollidable, Component);
OBJECT_PROPERTY(CCollidable, fixture);

CCollidable::CCollidable(void* go)
  : Component((GO*)go, PRIORITY_LEAST), fixture(NULL) {
}

CCollidable::~CCollidable() {
  if(fixture) {
    go->body->DestroyFixture(fixture);
  }
}

OBJECT_IMPL(CSensor, Component);
OBJECT_PROPERTY(CSensor, fixture);
OBJECT_PROPERTY(CSensor, kind);

CSensor::CSensor(void* _go)
  : Component((GO*)_go, PRIORITY_THINK), kind(MESSAGE_COLLIDING),
    fixture(NULL) {
}

CSensor::~CSensor() {
  go->body->DestroyFixture(fixture);
}

void CSensor::update(float dt) {
  b2ContactEdge* node = go->body->GetContactList();

  while(node) {
    if(node->contact->IsTouching()) {
      GO* other = NULL;
      if(node->contact->GetFixtureA() == fixture) {
        other = (GO*)node->contact->GetFixtureB()->GetBody()->GetUserData();
      } else if(node->contact->GetFixtureB() == fixture) {
        other = (GO*)node->contact->GetFixtureA()->GetBody()->GetUserData();
      }

      if(other) {
        go->send_message(other->create_message(kind, NULL, 0));
      }
    }
    node = node->next;
  }
}

LuaThread::LuaThread()
  : state(NULL), refid(-1) {
}

OBJECT_IMPL(CScripted, Component);
OBJECT_ACCESSOR(CScripted, update_thread, get_update_thread, set_update_thread);
OBJECT_ACCESSOR(CScripted, message_thread, get_message_thread, set_message_thread);

CScripted::CScripted(void* go)
  : Component((GO*)go, PRIORITY_ACT) {
}

CScripted::~CScripted() {
  // can't do this in the LuaThread destructor because we need to make
  // temporaries
  free_thread(&update_thread);
  free_thread(&message_thread);
}

void CScripted::init() {
  if(!update_thread.state && !message_thread.state) {
    fprintf(stderr, "CScripted initialized with no attached script\n");
    delete_me = 1;
    return;
  }

  step_thread(&update_thread);
  step_thread(&message_thread);
}

void CScripted::free_thread(LuaThread* thread) {
  if(thread->state) {
    luaL_unref(thread->state, LUA_REGISTRYINDEX, thread->refid);
    thread->state = NULL;
  }
}

void CScripted::set_thread(LuaThread* target, lua_State* thread) {
  // release old reference
  if(target->state) {
    luaL_unref(target->state, LUA_REGISTRYINDEX, target->refid);
  }

  // our special arrangement with LCset_value
  // ensures that the top of the lua thread stack contains the thread's
  // refid
  target->state = thread;
  target->refid = lua_tointeger(thread, -1);
  target->is_initialized = 0;
}

void CScripted::set_update_thread(lua_State* thread) {
  set_thread(&update_thread, thread);
}

void CScripted::set_message_thread(lua_State* thread) {
  set_thread(&message_thread, thread);
}

lua_State* CScripted::get_update_thread() {
  return NULL;
}

lua_State* CScripted::get_message_thread() {
  return NULL;
}

void CScripted::step_thread(LuaThread* thread) {
  if(thread->state) {
    if(!thread->is_initialized) {
      thread->is_initialized = 1;
      LCpush_go(thread->state, go);
      LCpush_component(thread->state, this);
      resume(thread, 2);
    }

    resume(thread, 0);
  }
}

void CScripted::update(float dt) {
  // init guaranteed that we always have a script
  step_thread(&update_thread);
}

void CScripted::messages_received() {
  step_thread(&message_thread);
}

void CScripted::resume(LuaThread* thread, int args) {
  int status = lua_resume(thread->state, NULL, args);
  if(status != LUA_YIELD) {
    if(status != LUA_OK) {
      const char* error = lua_tostring(thread->state, -1);
      fail_exit("lua thread failed: %s", error);
    }

    free_thread(thread);

    // when both threads exit, remove ourselves
    if(!message_thread.state && !update_thread.state) {
      delete_me = 1;
    }
  }
}

void LCpush_lut(lua_State *L, const char* metatable, void* ut) {
  if(!ut) {
    lua_pushnil(L);
  } else {
    void** p = (void**)lua_newuserdata(L, sizeof(void*));
    luaL_setmetatable(L, metatable);
    *p = ut;
  }
}

static void* LCcheck_lut(lua_State *L, const char* metatable, int pos) {
  static char error_msg[256];

  if(lua_isnil(L, pos)) {
    return NULL;
  }

  void **ud;
  if(metatable) {
    ud = (void**)luaL_checkudata(L, pos, metatable);
  } else {
    ud = (void**)lua_touserdata(L, pos);
  }

  if(ud == NULL) {
    snprintf(error_msg, sizeof(error_msg), "`%s' expected", metatable);
    luaL_argcheck(L, ud != NULL, pos, error_msg);
  }
  return *ud;
}

static World* LCcheck_world(lua_State *L, int pos) {
  return (World*)LCcheck_lut(L, LUT_WORLD, pos);
}

int LCpush_world(lua_State *L, World* world) {
  LCpush_lut(L, LUT_WORLD, world);
  return 1;
}

GO* LCcheck_go(lua_State *L, int pos) {
  return (GO*)LCcheck_lut(L, LUT_GO, pos);
}

void LCpush_go(lua_State *L, GO* go) {
  LCpush_lut(L, LUT_GO, go);
}

Universe* LCcheck_universe(lua_State *L, int pos) {
  return (Universe*)LCcheck_lut(L, LUT_UNIVERSE, pos);
}

void LCpush_universe(lua_State *L, Universe* universe) {
  LCpush_lut(L, LUT_UNIVERSE, universe);
}

struct LuaKeyData {
  void* fnkey;
  World* world;
  float value;
};

struct LuaSIData {
  void* fnkey;
  World* world;
  SpatialInput value;
};

static void handle_lua_key(void* _kd) {
  LuaKeyData* kd = (LuaKeyData*)_kd;
  lua_pushlightuserdata(kd->world->L, kd->fnkey);
  lua_gettable(kd->world->L, LUA_REGISTRYINDEX);
  lua_pushnumber(kd->world->L, kd->value);
  lua_call(kd->world->L, 1, 0);
  kd->world->lk_alloc.free(kd);
}

struct LuaUnregisterData {
  /* sizeof(this) <= sizeof(LuaKeyData) */
  void* fnkey;
  World *world;
};

static void unregister_lua_fn(void* _ud) {
  LuaUnregisterData* ud = (LuaUnregisterData*)_ud;
  lua_pushlightuserdata(ud->world->L, ud->fnkey);
  lua_pushnil(ud->world->L);
  lua_settable(ud->world->L, LUA_REGISTRYINDEX);
}

LuaKeyBinding::LuaKeyBinding(KeyNumber keyn, World* world)
  : keyn(keyn), world(world) {
  world->keybindings.add_head(this);
}

LuaKeyBinding::~LuaKeyBinding() {
  LuaUnregisterData* ud = (LuaUnregisterData*)world->lk_alloc.alloc();
  ud->fnkey = this;
  ud->world = world;
  world->enqueue_command(unregister_lua_fn, ud);
  world->keybindings.remove(this);
}

void LuaKeyBinding::activate(float value) {
  LuaKeyData* kd = (LuaKeyData*)world->lk_alloc.alloc();
  kd->fnkey = this; // we know this by prior arrangement
  kd->value = value;
  kd->world = world;
  world->enqueue_command(handle_lua_key, kd);
}

static int Lworld_set_keybinding(lua_State *L) {
  World* world = LCcheck_world(L, 1);
  const char* keyname = luaL_checkstring(L, 2);
  int keyn = find_keynumber(keyname);
  if(keyn < 0) {
    lua_pushnil(L);
    return 1;
  }

  LuaKeyBinding* kb = new LuaKeyBinding((KeyNumber)keyn, world);

  // store the provided closure, using the kb as the registry key
  lua_pushlightuserdata(L, kb);
  lua_pushvalue(L, 3); // the closure
  lua_settable(L, LUA_REGISTRYINDEX);
  set_key_binding((KeyNumber)keyn, kb);

  lua_pushboolean(L, true);
  return 1;
}

LuaSIBinding::LuaSIBinding(SpatialInputNumber keyn, World* world)
  : keyn(keyn), world(world) {
  world->sibindings.add_head(this);
}

LuaSIBinding::~LuaSIBinding() {
  LuaUnregisterData* ud = (LuaUnregisterData*)world->lk_alloc.alloc();
  ud->fnkey = this;
  ud->world = world;
  world->enqueue_command(unregister_lua_fn, ud);
  world->sibindings.remove(this);
}

static void handle_si_key(void* _kd) {
  LuaSIData* kd = (LuaSIData*)_kd;
  lua_pushlightuserdata(kd->world->L, kd->fnkey);
  lua_gettable(kd->world->L, LUA_REGISTRYINDEX);
  lua_pushnumber(kd->world->L, kd->value.absolute.x);
  lua_pushnumber(kd->world->L, kd->value.absolute.y);
  lua_pushnumber(kd->world->L, kd->value.relative.x);
  lua_pushnumber(kd->world->L, kd->value.relative.y);
  lua_call(kd->world->L, 4, 0);
  kd->world->lk_alloc.free(kd);
}

void LuaSIBinding::activate(SpatialInput* value) {
  LuaSIData* kd = (LuaSIData*)world->lk_alloc.alloc();
  kd->fnkey = this; // we know this by prior arrangement
  kd->value = *value;
  kd->world = world;
  world->enqueue_command(handle_si_key, kd);
}

static int Lworld_set_sibinding(lua_State *L) {
  World* world = LCcheck_world(L, 1);
  const char* keyname = luaL_checkstring(L, 2);
  int keyn = find_sinumber(keyname);
  if(keyn < 0) {
    lua_pushnil(L);
    return 1;
  }

  LuaSIBinding* kb = new LuaSIBinding((SpatialInputNumber)keyn, world);

  // store the provided closure, using the kb as the registry key
  lua_pushlightuserdata(L, kb);
  lua_pushvalue(L, 3); // the closure
  lua_settable(L, LUA_REGISTRYINDEX);
  set_si_binding((SpatialInputNumber)keyn, kb);

  lua_pushboolean(L, true);
  return 1;
}

static int Lworld_create_go(lua_State *L) {
  World* world = LCcheck_world(L, 1);
  LCpush_go(L, world->create_go());
  return 1;
}

class RevJoint : public Object {
public:
  OBJECT_PROTO(RevJoint);
  RevJoint(void* _joint) {
    joint = (b2RevoluteJoint*)_joint;
  }

  float lower_limit() {
    return joint->GetLowerLimit();
  }

  void set_lower_limit(float val) {
    joint->SetLimits(val, joint->GetUpperLimit());
  }

  float upper_limit() {
    return joint->GetUpperLimit();
  }

  void set_upper_limit(float val) {
    joint->SetLimits(joint->GetLowerLimit(), val);
  }

  b2RevoluteJoint* joint;
  World* world;
};

OBJECT_IMPL(RevJoint, Object);
OBJECT_ACCESSOR(RevJoint, lower_limit, lower_limit, set_lower_limit);
OBJECT_ACCESSOR(RevJoint, upper_limit, upper_limit, set_upper_limit);

int Ljoint_limit(lua_State* L) {
  RevJoint* joint = (RevJoint*)LCcheck_lut(L, LUT_JOINT, 1);
  float limit1 = luaL_checknumber(L, 2);
  float limit2 = luaL_checknumber(L, 3);
  joint->joint->SetLimits(limit1, limit2);
  return 0;
}

int Ljoint_gc(lua_State* L) {
  RevJoint* joint = (RevJoint*)LCcheck_lut(L, LUT_JOINT, 1);
  delete joint;
  return 0;
}

int Ljoint_destroy(lua_State* L) {
  RevJoint* joint = (RevJoint*)LCcheck_lut(L, LUT_JOINT, 1);
  joint->world->bWorld.DestroyJoint(joint->joint);
  return 0;
}

static int Lworld_create_joint(lua_State *L) {
  Vector_ la, lb;

  World* world = LCcheck_world(L, 1);
  GO* ga = LCcheck_go(L, 2);
  LCcheck_vector(L, 3, &la);
  GO* gb = LCcheck_go(L, 4);
  LCcheck_vector(L, 5, &lb);

  b2RevoluteJoint *joint;

  //car class constructor
  b2RevoluteJointDef jointDef;
  jointDef.bodyA = ga->body;
  jointDef.enableLimit = true;
  jointDef.lowerAngle = 0;//with both these at zero...
  jointDef.upperAngle = 0;//...the joint will not move
  jointDef.localAnchorA.Set(la.x / BSCALE, la.y / BSCALE);

  jointDef.bodyB = gb->body;
  jointDef.localAnchorB.Set(lb.x / BSCALE, lb.y / BSCALE);

  joint = (b2RevoluteJoint*)world->bWorld.CreateJoint( &jointDef );
  RevJoint* rj = new RevJoint(joint);
  rj->world = world;
  LCpush_lut(L, LUT_JOINT, rj);
  return 1;
}

static int Lworld_atlas_entry(lua_State *L) {
  World* world = LCcheck_world(L, 1);
  const char* atlas = luaL_checkstring(L, 2);
  const char* entry = luaL_checkstring(L, 3);
  SpriteAtlasEntry result = world->atlas_entry(atlas, entry);
  if(!result) {
    luaL_error(L, "`%s' is not a valid entry in `%s'", entry, atlas);
  }

  LCpush_entry(L, result);
  return 1;
}

static int Lworld_animation(lua_State *L) {
  World* world = LCcheck_world(L, 1);
  const char* scml_file = luaL_checkstring(L, 2);
  const char* atlas_name = luaL_checkstring(L, 3);
  const char* anim_name = luaL_checkstring(L, 4);
  SpriteAtlas atlas = world->atlas(atlas_name);
  Animation* anim = world->animation(scml_file, atlas, anim_name);
  LCpush_animation(L, anim);
  return 1;
}

// world, center, last_go, look angle, cone angle
static int Lworld_next_in_cone(lua_State *L) {
  Cone cone;

  World* world = LCcheck_world(L, 1);
  LCcheck_vector(L, 2, &cone.point);
  GO* last_go = LCcheck_go(L, 3);
  float angle = luaL_checknumber(L, 4);
  cone.angle = luaL_checknumber(L, 5);

  vector_for_angle(&cone.direction, angle);

  GO* next_go = world->next_in_cone(last_go, &world->scene.camera_rect, &cone);
  LCpush_go(L, next_go);
  return 1;
}

static void LCpush_handle(lua_State* L, AudioHandle* h) {
  LCpush_lut(L, LUT_AUDIOHANDLE, h);
}

static AudioHandle* LCcheck_handle(lua_State* L, int pos) {
  return (AudioHandle*)LCcheck_lut(L, LUT_AUDIOHANDLE, pos);
}

static int Laudiohandle_gc(lua_State* L) {
  AudioHandle* h = LCcheck_handle(L, 1);
  h->destroy();
  return 0;
}

static int Laudiohandle_terminate(lua_State* L) {
  AudioHandle* h = LCcheck_handle(L, 1);
  h->terminate();
  return 0;
}

static int Lworld_get_sound(lua_State* L) {
  World* world = LCcheck_world(L, 1);
  const char* sound = luaL_checkstring(L, 2);
  float scale = luaL_checknumber(L, 3);

  lua_pushlightuserdata(L, world->universe->sound.get_sync(sound, scale));
  return 1;
}

static int Lworld_play_sound(lua_State* L) {
  World* world = LCcheck_world(L, 1);
  Sound* sound = (Sound*)lua_touserdata(L, 2);
  luaL_argcheck(L, sound != NULL, 2, "`Sound' expected");
  int channel = luaL_checkinteger(L, 3);
  // the returned handle isn't guarteed to live beyond this
  // call. Clone it and have lua gc it when it's done.
  AudioHandle* handle = world->universe->play_sound(sound, channel);
  LCpush_handle(L, new AudioHandle(*handle));
  return 1;
}

static int Lworld_stream_sound(lua_State* L) {
  World* world = LCcheck_world(L, 1);
  const char* sound = luaL_checkstring(L, 2);
  long start = luaL_checkinteger(L, 3);
  AudioHandle* handle = world->universe->stream_sound(sound, start);
  LCpush_handle(L, new AudioHandle(*handle));
  return 1;
}

static int Lworld_sound_handle(lua_State* L) {
  World* world = LCcheck_world(L, 1);
  long handle_name = luaL_checkinteger(L, 2);
  AudioHandle* handle = world->universe->sound_handle(handle_name);
  if(handle) {
    LCpush_handle(L, new AudioHandle(*handle));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int Lworld_current_sound_sample(lua_State* L) {
  lua_pushinteger(L, audio_current_sample());
  return 1;
}

static Component* LCcheck_component(lua_State *L, int pos) {
  return (Component*)LCcheck_lut(L, LUT_COMPONENT, pos);
}

TypeInfo* LCcheck_type(lua_State *L, int pos) {
  const char* name = luaL_checkstring(L, pos);
  TypeInfo* type = TypeRegistry::instance().find_type(name);
  if(type == NULL) {
    luaL_error(L, "`%s' does not name a registered type", name);
  }
  return type;
}

static Object* LCcheck_object(lua_State *L, int pos) {
  return (Object*)LCcheck_lut(L, NULL, pos);
}

void LCpush_component(lua_State *L, Component *comp) {
  LCpush_lut(L, LUT_COMPONENT, comp);
}

static int Lgo_find_component(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  TypeInfo* type = LCcheck_type(L, 2);
  Component* last = LCcheck_component(L, 3);
  Component* comp = go->find_component(type, last);
  LCpush_component(L, comp);
  return 1;
}

static int Lps_find_component(lua_State *L) {
  SystemDefinition* def = (SystemDefinition*)LCcheck_lut(L, LUT_PSDEFINITION, 1);
  TypeInfo* type = LCcheck_type(L, 2);
  ParticleSystemComponent* comp = def->find_component(type);
  LCpush_lut(L, LUT_PSCOMPONENT, comp);
  return 1;
}

void LCconfigure_object(lua_State *L, Object* obj, int pos) {
  // iterate the table using the keys as property names
  const TypeInfo* type = obj->typeinfo();
  pos = lua_absindex(L, pos);

  lua_pushnil(L);
  while(lua_next(L, pos)) {
    const char* pname = luaL_checkstring(L, -2);
    const PropertyInfo* prop = type->property(pname);
    if(!prop) {
      luaL_error(L, "`%s' does not name a property of `%s'",
                 pname, type->name());
    }
    prop->LCset_value(obj, L, -1);
    lua_pop(L, 1); // pop the value, leave the key
  }
}

static int Lgo_add_component(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  TypeInfo* type = LCcheck_type(L, 2);
  Component* comp = go->add_component(type);

  // argument 3 should be an optional table
  if(lua_gettop(L) != 3) {
    LCpush_component(L, comp);
    return 1;
  }

  if(!lua_istable(L, 3)) {
    luaL_error(L, "argument 3 should be a table");
    return 0;
  }

  LCconfigure_object(L, comp, 3);

  LCpush_component(L, comp);
  return 1;
}

static int Lgo_create_message(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  int kind = luaL_checkinteger(L, 2);
  const char* content = NULL;
  size_t nbytes = 0;
  if(lua_isstring(L, 3)) {
    content = lua_tolstring(L, 3, &nbytes);
  }

  Message* message = go->create_message(kind, content, nbytes);
  LCpush_message(L, message);
  return 1;
}

static int Lgo_send_message(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  Message* message = LCcheck_message(L, 2);
  go->send_message(message);
  return 0;
}

static int Lgo_broadcast_message(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  float range = luaL_checknumber(L, 2);
  int kind = luaL_checkinteger(L, 3);
  const char* content = NULL;
  size_t nbytes = 0;
  if(lua_isstring(L, 4)) {
    content = lua_tolstring(L, 4, &nbytes);
  }

  go->world->broadcast_message(go, range, kind, content, nbytes);
  return 0;
}

static int Lgo_has_message(lua_State *L) {
  GO* go = LCcheck_go(L, 1);
  int type = luaL_checkinteger(L, 2);
  int rvalues = 0;

  go->inbox.foreach([&] (Message* msg) -> int {
      if(msg->kind == type) {
        LCpush_message(L, msg);
        ++rvalues;
      }
      return 0;
    });

  return rvalues;
}

static int Lgo_apply_force(lua_State* L) {
  Vector_ force;

  GO* go = LCcheck_go(L, 1);
  LCcheck_vector(L, 2, &force);

  b2Vec2 bForce;
  bForce.x = force.x / BSCALE;
  bForce.y = force.y / BSCALE;

  go->body->ApplyForceToCenter(bForce);
  return 0;
}

static int Lgo_apply_impulse(lua_State* L) {
  Vector_ imp;

  GO* go = LCcheck_go(L, 1);
  LCcheck_vector(L, 2, &imp);

  go->apply_impulse(&imp);
  return 0;
}

static int Lgo_apply_angular_impulse(lua_State* L) {
  GO* go = LCcheck_go(L, 1);
  float angimp = luaL_checknumber(L, 2);
  go->apply_angular_impulse(angimp);
  return 0;
}

static int Lgo_apply_torque(lua_State* L) {
  GO* go = LCcheck_go(L, 1);
  float torque = luaL_checknumber(L, 2);
  go->apply_torque(torque);
  return 0;
}

static int Lgo_mass(lua_State* L) {
  GO* go = LCcheck_go(L, 1);
  lua_pushnumber(L, go->body->GetMass());
  return 1;
}

static int Lgo_inertia(lua_State* L) {
  GO* go = LCcheck_go(L, 1);
  lua_pushnumber(L, go->body->GetInertia());
  return 1;
}

static int Lgo_contacts(lua_State* L) {
  GO* go = LCcheck_go(L, 1);

  // contact list
  lua_newtable(L);
  int ii = 1;

  b2ContactEdge* node = go->body->GetContactList();
  while(node) {
    b2Fixture* fa = node->contact->GetFixtureA();
    b2Fixture* fb = node->contact->GetFixtureB();
    if(node->contact->IsTouching() &&
       (!fa->IsSensor() && !fb->IsSensor())) {

      b2WorldManifold manifold;
      node->contact->GetWorldManifold(&manifold);

      b2Vec2 norm = manifold.normal;
      GO* other;

      // normal goes from A to B so we need to be careful
      if(node->contact->GetFixtureA()->GetBody() != go->body) {
        other = (GO*)node->contact->GetFixtureA()->GetBody()->GetUserData();
        norm = -norm;
      } else {
        other = (GO*)node->contact->GetFixtureB()->GetBody()->GetUserData();
      }

      Vector_ normv;
      normv.x = norm.x;
      normv.y = norm.y;

      Vector_ pos;
      pos.x = manifold.points[0].x * BSCALE;
      pos.y = manifold.points[0].y * BSCALE;

      // contact
      lua_newtable(L);
      LCpush_go(L, other);
      lua_setfield(L, -2, "other");

      LCpush_vector(L, &normv);
      lua_setfield(L, -2, "normal");

      LCpush_vector(L, &pos);
      lua_setfield(L, -2, "position");

      lua_rawseti(L, -2, ii++);
    }
    node = node->next;
  }

  return 1;
}

static int Lobject_mutate(lua_State* L) {
  const char* name = luaL_checkstring(L, lua_upvalueindex(1));
  Object* obj = LCcheck_object(L, 1);

  const PropertyInfo* prop = obj->typeinfo()->property(name);
  if(prop == NULL) {
    luaL_error(L, "`%s' does not have property `%s'",
               obj->typeinfo()->name(), name);
  }

  if(lua_gettop(L) == 1) {
    prop->LCpush_value(obj, L);
    return 1;
  } else {
    prop->LCset_value(obj, L, 2);
    return 0;
  }
}

static int Lobject_index(lua_State* L) {
  const char* name = luaL_checkstring(L, 2);
  lua_pushcclosure(L, Lobject_mutate, 1);
  return 1;
}

static int Lobject_tostring(lua_State *L) {
  Object* obj = LCcheck_object(L, 1);
  lua_pushstring(L, obj->typeinfo()->name());
  return 1;
}

static int Lobject_eq(lua_State* L) {
  void** objA = (void**)lua_touserdata(L, 1);
  void** objB = (void**)lua_touserdata(L, 2);
  if(objA != objB && (objA == NULL || objB == NULL)) {
    lua_pushboolean(L, 0);
  } else {
    lua_pushboolean(L, *objA == *objB);
  }
  return 1;
}

static int Lobject_key(lua_State* L) {
  void** obj = (void**)lua_touserdata(L, 1);
  if(obj == NULL) {
    lua_pushnil(L);
  } else {
    lua_pushlightuserdata(L, *obj);
  }
  return 1;
}

void LClink_metatable(lua_State *L, const char* name, const luaL_Reg* table) {
  static const luaL_Reg object_m[] = {
    {"__index", Lobject_index},
    {NULL, NULL}};

  luaL_newmetatable(L, name);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);
  luaL_setfuncs(L, table, 0);

  lua_newtable(L);
  luaL_setfuncs(L, object_m, 0);
  lua_setmetatable(L, -2);
}

OBJECT_IMPL(World, Object);
OBJECT_PROPERTY(World, focus);
OBJECT_PROPERTY(World, dt);
OBJECT_ACCESSOR(World, time_scale, get_time_scale, set_time_scale);
OBJECT_ACCESSOR(World, gravity, get_gravity, set_gravity);

void init_lua(World* world) {
  world->clock = clock_make();
  world->camera_clock = clock_make();
  world->camera = world->create_go();
  world->stage = world->create_go();

  lua_State* L = luaL_newstate();
  world->L = L;

  luaL_openlibs(L);

  static const luaL_Reg world_m[] = {
    {"create_go", Lworld_create_go},
    {"atlas_entry", Lworld_atlas_entry},
    {"animation", Lworld_animation},
    {"next_in_cone", Lworld_next_in_cone},
    {"get_sound", Lworld_get_sound},
    {"play_sound", Lworld_play_sound},
    {"stream_sound", Lworld_stream_sound},
    {"sound_handle", Lworld_sound_handle},
    {"current_sound_sample", Lworld_current_sound_sample},
    {"set_keybinding", Lworld_set_keybinding},
    {"set_sibinding", Lworld_set_sibinding},
    {"create_joint", Lworld_create_joint},
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_WORLD, world_m);

  static const luaL_Reg go_m[] = {
    {"add_component", Lgo_add_component},
    {"find_component", Lgo_find_component},
    {"has_message", Lgo_has_message},
    {"create_message", Lgo_create_message},
    {"send_message", Lgo_send_message},
    {"broadcast_message", Lgo_broadcast_message},
    {"apply_force", Lgo_apply_force},
    {"apply_impulse", Lgo_apply_impulse},
    {"apply_angular_impulse", Lgo_apply_angular_impulse},
    {"apply_torque", Lgo_apply_torque},
    {"inertia", Lgo_inertia},
    {"mass", Lgo_mass},
    {"contacts", Lgo_contacts},
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_GO, go_m);

  static const luaL_Reg universe_m[] = {
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_UNIVERSE, universe_m);

  static const luaL_Reg component_m[] = {
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_COMPONENT, component_m);

  static const luaL_Reg psdef_m[] = {
    {"find_component", Lps_find_component},
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_PSDEFINITION, psdef_m);

  static const luaL_Reg pscomp_m[] = {
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_PSCOMPONENT, psdef_m);

  static const luaL_Reg audiohandle_m[] = {
    {"terminate", Laudiohandle_terminate},
    {"__gc", Laudiohandle_gc},
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_AUDIOHANDLE, audiohandle_m);

  static const luaL_Reg joint_m[] = {
    {"__gc", Ljoint_gc},
    {"limit", Ljoint_limit},
    {"destroy", Ljoint_destroy},
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"key", Lobject_key},
    {NULL, NULL}};

  LClink_metatable(L, LUT_JOINT, joint_m);

  LCpush_world(L, world);
  lua_setglobal(L, "world");

  LCpush_go(L, world->camera);
  lua_setglobal(L, "camera");

  LCpush_go(L, world->stage);
  lua_setglobal(L, "stage");

  lua_pushnumber(L, screen_width);
  lua_setglobal(L, "screen_width");

  lua_pushnumber(L, screen_height);
  lua_setglobal(L, "screen_height");

  lua_pop(L, lua_gettop(L));
}

World::World(void* _universe)
  : L(NULL), scene(this), bWorld(b2Vec2(0, -50)), focus(NULL),
    universe((Universe*)_universe),
    lk_alloc(MAX(sizeof(LuaKeyData), sizeof(LuaSIData)),
             MAX_INFLIGHT_INPUTS, "lk_alloc"),
    cmd_alloc(sizeof(Command), MAX_INFLIGHT_INPUTS*2, "cmd_alloc") {
  init_lua(this);
}

World::~World() {
  game_objects.foreach([](GO* go) -> int {
      delete go;
      return 0;
    });

  keybindings.foreach([](LuaKeyBinding* kb) -> int {
      // the keybinding logic deletes the encumbant
      set_key_binding(kb->keyn, NULL);
      return 0;
    });

  sibindings.foreach([](LuaSIBinding* kb) -> int {
      // the keybinding logic deletes the encumbant
      set_si_binding(kb->keyn, NULL);
      return 0;
    });
  lua_close(L);
  clock_free(clock);
  clock_free(camera_clock);
}

void World::update(long delta) {
  // process waiting commands
  evaluate_commands();

  this->dt = clock_update(clock, delta / 1000.0);;

  // do an integration step
  bWorld.Step(dt, 6, 2);
  update_camera(clock_update(camera_clock, delta / 1000.0));

  scene.start();

  // let the game objects initialize any new components
  game_objects.foreach([=](GO* go) -> int {
      if(go->delete_me) {
        delete go;
      } else {
        go->update(dt);
      }
      return 0;
    });

  // update the comonents
  this->components.foreach([=](Component* comp) -> int {
      if(comp->delete_me) {
        delete(comp);
      } else {
        comp->update(dt);
      }
      return 0;
    });

  // run go messages until all messages have been handled
  while(have_waiting_messages.head) {
    have_waiting_messages.foreach([this](GO* go) -> int {
        // move the pending messages into the inbox and remove from
        // waiting list
        go->inbox = go->inbox_pending;
        go->inbox_pending.zero();
        have_waiting_messages.remove(go);

        // tell GO to consume messages
        go->messages_received();
        return 0;
      });
  }
}

void World::update_camera(float dt) {
  if(!focus) return;
  Vector_ offset = {screen_width / 2.0f, screen_height / 2.0f};
  Vector_ desired;
  focus->pos(&desired);
  vector_sub(&desired, &desired, &offset);

  float max_v = 1600;
  const float max_dx = max_v * dt;

  Vector_ cpos;
  camera->pos(&cpos);

  Vector_ delta;
  vector_sub(&delta, &desired, &cpos);
  float mag = vector_mag(&delta);
  if(mag < max_dx) {
    // snap
    camera->set_pos(&desired);
    return;
  }

  vector_scale(&delta, &delta, max_dx / mag);
  vector_add(&cpos, &cpos, &delta);
  camera->set_pos(&cpos);
}

void World::load_level(const char* level) {
  if(!luaL_dofile(L, level)) {
    lua_getglobal(L, "level_init");
    if(!lua_isnil(L, -1)) {
      lua_call(L, 0, 0);
    } else {
      lua_pop(L, 1);
      fail_exit("`level_init' was not defined after loading %s", level);
    }
  } else {
    const char* error = lua_tostring(L, -1);
    fail_exit("level loading failed: %s", error);
  }
}

GO* World::create_go() {
  GO* go = new GO(this);
  return go;
}

SpriteAtlas World::atlas(const char* atlas) {
  return universe->atlas(atlas);
}

SpriteAtlasEntry World::atlas_entry(const char* atlas_name, const char* entry) {
  return universe->atlas_entry(atlas_name, entry);
}

Animation* World::animation(const char* filename, SpriteAtlas atlas, const char* anim) {
  return universe->animation(filename, atlas, anim);
}

void World::set_time_scale(float scale) {
  clock->time_scale = scale;
}

float World::get_time_scale() {
  return clock->time_scale;
}

void World::set_gravity(Vector_ vector) {
  b2Vec2 bVector;
  bVector.x = vector.x / BSCALE;
  bVector.y = vector.y / BSCALE;

  bWorld.SetGravity(bVector);
}

Vector_ World::get_gravity() {
  b2Vec2 bVector = bWorld.GetGravity();
  Vector_ vector;
  vector.x = bVector.x * BSCALE;
  vector.y = bVector.y * BSCALE;
  return vector;
}

void World::broadcast_message(GO* sender, float radius, int kind, const char* content, size_t nbytes) {
  if(radius <= 0) {
    // assume global broadcast
    game_objects.foreach([=](GO* go) -> int {
        go->send_message(sender->create_message(kind, content, nbytes));
        return 0;
      });
    return;
  }

  Vector_ pos;
  sender->pos(&pos);

  world_foreach(this, &pos, radius, [=](GO* go, b2Fixture*) -> int {
      go->send_message(sender->create_message(kind, content, nbytes));
      return 0;
    });
}

int point_in_cone(Cone* cone, Vector point) {
  Vector_ to_point;
  vector_sub(&to_point, point, &cone->point);
  vector_norm(&to_point, &to_point);
  float dot = vector_dot(&cone->direction, &to_point);
  return acosf(dot) <= cone->angle;
}

GO* World::next_in_cone(GO* last, Rect bounds, Cone* cone) {
  float last_dist = 0;
  if(last) {
    Vector_ last_pos;
    last->pos(&last_pos);
    last_dist = vector_dist(&last_pos, &cone->point);
  }

  GO* next_exceeding = NULL;
  float exceeded_by = INFINITY;

  world_foreach(this, bounds, [&](GO* go, b2Fixture* fixture) -> int {
      if(go == last) return 0;

      // test the center and the 4 corners of its AABB
      Vector_ test_points[5];
      go->pos(&test_points[0]);

      const b2AABB& aabb = fixture->GetAABB(0);
      Vector_ lower = {aabb.lowerBound.x * BSCALE, aabb.lowerBound.y * BSCALE};
      Vector_ upper = {aabb.upperBound.x * BSCALE, aabb.upperBound.y * BSCALE};

      test_points[1] = lower; // BL
      test_points[2].x = upper.x; // BR
      test_points[2].y = lower.y;
      test_points[3] = upper; // UR
      test_points[4].x = lower.x; // UL
      test_points[4].y = upper.y;

      for(int ii = 0; ii < 5; ++ii) {
        // if we intersect any of the test points then see if that's
        // the object with the next closest center
        if(point_in_cone(cone, &test_points[ii])) {
          float dist = vector_dist(&test_points[0], &cone->point);
          if(dist > last_dist) {
            float my_exceeded_by = dist - last_dist;
            if(my_exceeded_by < exceeded_by) {
              next_exceeding = go;
              exceeded_by = my_exceeded_by;
            }
          }
          // once we've found an intersecting point there's no need to
          // keep looking. move on to the next object
          break;
        }
      }
      return 0;
    });
  return next_exceeding;
}

void World::enqueue_command(CommandFunction fn, void* data) {
  Command* command = (Command*)cmd_alloc.alloc();
  command->function = fn;
  command->data = data;
  command_queue.enqueue(command);
}

void World::evaluate_commands() {
  Command* cmd;
  while((cmd = command_queue.dequeue_noblock()) != NULL) {
    cmd->function(cmd->data);
    cmd_alloc.free(cmd);
  }
}

WorldPipelineDelegate::WorldPipelineDelegate(World* world)
  : world(world) {
}

void WorldPipelineDelegate::update(long delta) {
  world->update(delta);
  world->scene.enqueue();
  callNext(delta);
}

OBJECT_IMPL(Universe, Object);
OBJECT_PROPERTY(Universe, stash);

Universe::Universe(void* empty)
  : stash(NULL) {
}

Universe::~Universe() {
  if(stash) free_lstring(stash);

  for(NameToAtlas::iterator iter = name_to_atlas.begin();
      iter != name_to_atlas.end(); ++iter) {
    free((char*)iter->first);
    spriteatlas_free(iter->second);
  }

  for(NameToEntity::iterator iter = name_to_entity.begin();
      iter != name_to_entity.end(); ++iter) {
    free((char*)iter->first);
    spriter_free(iter->second);
  }
}

SpriteAtlas Universe::atlas(const char* atlas_name) {
  NameToAtlas::iterator iter = name_to_atlas.find(atlas_name);
  SpriteAtlas atlas;

  if(iter == name_to_atlas.end()) {
    atlas = spriteatlas_load(atlas_name, "png");
    name_to_atlas.insert(std::make_pair(strdup(atlas_name), atlas));
  } else {
    atlas = iter->second;
  }

  return atlas;
}

SpriteAtlasEntry Universe::atlas_entry(const char* atlas_name, const char* entry) {
  return spriteatlas_find(atlas(atlas_name), entry);
}

Entity* Universe::scml_entity(const char* filename, SpriteAtlas atlas) {
  NameToEntity::iterator iter = name_to_entity.find(filename);
  Entity* entity;

  if(iter == name_to_entity.end()) {
    entity = spriter_load(filename, atlas);
    name_to_entity.insert(std::make_pair(strdup(filename), entity));
  } else {
    entity = iter->second;
  }

  return entity;
}

void Universe::update(long delta) {
  LongToHandle::iterator iter = long_to_handle.begin();
  while(iter != long_to_handle.end()) {
    long handle_name = iter->first;
    AudioHandle* handle = iter->second;
    ++iter;

    if(!handle->isCurrent()) {
      handle->destroy();
      long_to_handle.erase(handle_name);
    }
  }
}

Animation* Universe::animation(const char* filename, SpriteAtlas atlas, const char* anim) {
  return spriter_find(scml_entity(filename, atlas), anim);
}

AudioHandle* Universe::play_sound(Sound* s, int channel)  {
  AudioHandle* handle = sound.play(s, channel);
  long_to_handle.insert(std::make_pair(handle->handle, handle));
  return handle;
}

AudioHandle* Universe::stream_sound(const char* fname, long start_sample) {
  AudioHandle* handle = sound.stream(fname, start_sample);
  long_to_handle.insert(std::make_pair(handle->handle, handle));
  return handle;
}

AudioHandle* Universe::sound_handle(long handle) {
  LongToHandle::const_iterator iter = long_to_handle.find(handle);
  if(iter == long_to_handle.end()) return NULL;
  return iter->second;
}

/*
void Universe::update(long delta) {
}

void Universe::init_world() {
}
*/
