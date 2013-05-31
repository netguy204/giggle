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
#include "compositor.h"

#include <math.h>
#include <Box2D/Dynamics/Joints/b2Joint.h>

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

void free_thread(LuaThread* thread) {
  if(thread->state) {
    luaL_unref(thread->state, LUA_REGISTRYINDEX, thread->refid);
    thread->state = NULL;
  }
}

void resume(LuaThread* thread, int args) {
  int status = lua_resume(thread->state, NULL, args);
  if(status != LUA_YIELD) {
    if(status != LUA_OK) {
      const char* error = lua_tostring(thread->state, -1);
      fail_exit("lua thread failed: %s", error);
    }

    free_thread(thread);
  }
}

void step_thread(LuaThread* thread, GO* go, Component* comp) {
  if(thread->state) {
    if(!thread->is_initialized) {
      thread->is_initialized = 1;
      LCpush(thread->state, go);
      LCpush(thread->state, comp);
      resume(thread, 2);
    }

    resume(thread, 0);
  }
}

OBJECT_IMPL(Camera, Component);
OBJECT_PROPERTY(Camera, pre_render);
OBJECT_PROPERTY(Camera, post_render);
OBJECT_ACCESSOR(Camera, world2camera, get_world2camera, set_world2camera);

Camera::Camera(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), world2camera(NULL) {
  memset(renderables, 0, sizeof(renderables));
  memset(baseLayers, 0, sizeof(baseLayers));
  memset(layers, 0, sizeof(layers));
  memset(particles, 0, sizeof(particles));
  memset(testRects, 0, sizeof(testRects));

  // reasonable default
  world2camera.orthographic_proj(0.0f, screen_width, 0.0f, screen_height,
                                 -1.0f, 1.0f);

  go->world->cameras.add_head(this);
}

Camera::~Camera() {
  go->world->cameras.remove(this);
  free_thread(&pre_render);
  free_thread(&post_render);
}

void Camera::addRenderable(int layer, Renderable* renderable, void* args) {
  RenderableCommand* command = (RenderableCommand*)frame_alloc(sizeof(RenderableCommand));
  command->renderable = renderable;
  command->args = args;
  command->next = renderables[layer];
  renderables[layer] = command;
}

void Camera::addSprite(BaseSprite* list, BaseSprite sprite) {
  sprite_append(*list, sprite);
}

void Camera::addRect(ColoredRect* list, ColoredRect rect) {
  rect->next = *list;
  *list = rect;
}

extern Matrix44 orthographic_projection;

class SetCameraTransform : public Renderable {
public:
  SetCameraTransform(Matrix44& m)
    : Renderable(NULL), matrix(&m) {
  }

  virtual void render(void* args) {
    orthographic_projection = matrix;
  }

  Matrix44 matrix;
};

void Camera::enqueue() {
  step_thread(&pre_render, go, this);

  // set transform
  void* stfbuff = frame_alloc(sizeof(SetCameraTransform));
  SetCameraTransform *stf = new(stfbuff) SetCameraTransform(world2camera);
  renderable_enqueue_for_screen(stf, NULL);

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
  step_thread(&post_render, go, this);
}

Matrix44* Camera::get_world2camera() {
  return &world2camera;
}

void Camera::set_world2camera(Matrix44* m) {
  world2camera = *m;
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
OBJECT_METHOD(GO, apply_impulse, void, (Vector));

void GO::apply_angular_impulse(float angimp) {
  body->ApplyAngularImpulse(angimp);
}
OBJECT_METHOD(GO, apply_angular_impulse, void, (float));

void GO::apply_torque(float torque) {
  body->ApplyTorque(torque);
}
OBJECT_METHOD(GO, apply_torque, void, (float));

void GO::apply_force(Vector force) {
  b2Vec2 bForce;
  bForce.x = force->x / BSCALE;
  bForce.y = force->y / BSCALE;

  body->ApplyForceToCenter(bForce);
}
OBJECT_METHOD(GO, apply_force, void, (Vector));

float GO::mass() {
  return body->GetMass();
}
OBJECT_METHOD(GO, mass, float, ());

float GO::inertia() {
  return body->GetInertia();
}
OBJECT_METHOD(GO, inertia, float, ());

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
OBJECT_METHOD(GO, find_component, Component*, (TypeInfo*, Component*));

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
OBJECT_METHOD(GO, send_message, void, (Message*));

OBJECT_IMPL(Component, Object);
OBJECT_PROPERTY(Component, delete_me);

Component::Component(void* p)
  : go(NULL), delete_me(0) {
  fail_exit("default component constructor not valid");
}

Component::Component(GO* go, ComponentPriority priority)
  : go(NULL), priority(priority), delete_me(0) {

  this->go = go;
  go->uninitialized_components.add_head(this);
}

Component::~Component() {
  if(go) {
    go->components.remove(this);
    go->world->components.remove(this);
  }
}

void Component::init() {
}

void Component::update(float dt) {
}

void Component::messages_received() {
}

void Component::render(Camera* camera) {
}

Fixture::Fixture()
  : comp(NULL), fixture(NULL) {
}

Fixture::Fixture(Component* comp)
  : comp(comp), fixture(NULL) {
}

OBJECT_IMPL(CCollidable, Component);
OBJECT_PROPERTY(CCollidable, fixture);

CCollidable::CCollidable(void* go)
  : Component((GO*)go, PRIORITY_LEAST), fixture(this) {
}

CCollidable::~CCollidable() {
  if(fixture.fixture) {
    go->body->DestroyFixture(fixture.fixture);
  }
}

OBJECT_IMPL(CSensor, Component);
OBJECT_PROPERTY(CSensor, fixture);
OBJECT_PROPERTY(CSensor, kind);

CSensor::CSensor(void* _go)
  : Component((GO*)_go, PRIORITY_THINK), kind(MESSAGE_COLLIDING),
    fixture(this) {
}

CSensor::~CSensor() {
  if(fixture.fixture) {
    go->body->DestroyFixture(fixture.fixture);
  }
}

void CSensor::update(float dt) {
  b2ContactEdge* node = go->body->GetContactList();

  while(node) {
    if(node->contact->IsTouching()) {
      GO* other = NULL;
      if(node->contact->GetFixtureA() == fixture.fixture) {
        other = (GO*)node->contact->GetFixtureB()->GetBody()->GetUserData();
      } else if(node->contact->GetFixtureB() == fixture.fixture) {
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
OBJECT_PROPERTY(CScripted, update_thread);
OBJECT_PROPERTY(CScripted, message_thread);

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

  step_thread(&update_thread, go, this);
  step_thread(&message_thread, go, this);
}

void CScripted::update(float dt) {
  // init guaranteed that we always have at least one script
  step_thread(&update_thread, go, this);

  // when both threads exit, remove ourselves
  if(!message_thread.state && !update_thread.state) {
    delete_me = 1;
  }
}

void CScripted::messages_received() {
  step_thread(&message_thread, go, this);
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
  World* world;
  LCcheck(L, &world, 1);
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
  World* world;
  LCcheck(L, &world, 1);
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

OBJECT_IMPL(RevJoint, Object);
OBJECT_ACCESSOR(RevJoint, lower_limit, lower_limit, set_lower_limit);
OBJECT_ACCESSOR(RevJoint, upper_limit, upper_limit, set_upper_limit);

RevJoint::RevJoint(void* _joint) {
  joint = (b2RevoluteJoint*)_joint;
}

float RevJoint::lower_limit() {
  return joint->GetLowerLimit();
}

void RevJoint::set_lower_limit(float val) {
  joint->SetLimits(val, joint->GetUpperLimit());
}

float RevJoint::upper_limit() {
  return joint->GetUpperLimit();
}

void RevJoint::set_upper_limit(float val) {
  joint->SetLimits(joint->GetLowerLimit(), val);
}

void RevJoint::limit(float lower, float upper) {
  joint->SetLimits(lower, upper);
}
OBJECT_METHOD(RevJoint, limit, void, (float, float));

void RevJoint::destroy() {
  world->bWorld.DestroyJoint(joint);
}
OBJECT_METHOD(RevJoint, destroy, void, ());

static int Laudiohandle_terminate(lua_State* L) {
  AudioHandle* h;
  LCcheck(L, &h, 1);
  h->terminate();
  return 0;
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
  GO* go;
  LCcheck(L, &go, 1);
  TypeInfo* type;
  LCcheck(L, &type, 2);
  Component* comp = go->add_component(type);

  // argument 3 should be an optional table
  if(lua_gettop(L) != 3) {
    LCpush(L, (Component*)comp);
    return 1;
  }

  if(!lua_istable(L, 3)) {
    luaL_error(L, "argument 3 should be a table");
    return 0;
  }

  LCconfigure_object(L, comp, 3);

  LCpush(L, (Component*)comp);
  return 1;
}

static int Lgo_create_message(lua_State *L) {
  GO* go;
  LCcheck(L, &go, 1);
  int kind = luaL_checkinteger(L, 2);
  const char* content = NULL;
  size_t nbytes = 0;
  if(lua_isstring(L, 3)) {
    content = lua_tolstring(L, 3, &nbytes);
  }

  Message* message = go->create_message(kind, content, nbytes);
  LCpush(L, message);
  return 1;
}

static int Lgo_broadcast_message(lua_State *L) {
  GO* go;
  LCcheck(L, &go, 1);
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
  GO* go;
  LCcheck(L, &go, 1);
  int type = luaL_checkinteger(L, 2);
  int rvalues = 0;

  go->inbox.foreach([&] (Message* msg) -> int {
      if(msg->kind == type) {
        LCpush(L, msg);
        ++rvalues;
      }
      return 0;
    });

  return rvalues;
}

static int Lgo_contacts(lua_State* L) {
  GO* go;
  LCcheck(L, &go, 1);

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
      LCpush(L, other);
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

OBJECT_IMPL(World, Object);
OBJECT_PROPERTY(World, dt);
OBJECT_ACCESSOR(World, time_scale, get_time_scale, set_time_scale);
OBJECT_ACCESSOR(World, gravity, get_gravity, set_gravity);
OBJECT_PROPERTY(World, pre_render);
OBJECT_PROPERTY(World, post_render);

void init_lua(World* world) {
  world->clock = clock_make();
  world->camera_clock = clock_make();
  world->stage = world->create_go();
  world->stage->add_component(&Camera::Type);

  lua_State* L = luaL_newstate();
  world->L = L;

  luaL_openlibs(L);

  if(world->universe->lua_path) {
    lua_getglobal(L, "package");
    LCpush(L, world->universe->lua_path);
    lua_setfield(L, -2, "path");
  }
  LCinit_object_metatable(L);

  static const luaL_Reg world_m[] = {
    {"set_keybinding", Lworld_set_keybinding},
    {"set_sibinding", Lworld_set_sibinding},
    {"__gc", Lobject_special_gc},
    {NULL, NULL}};

  LClink_metatable(L, LUT_WORLD, world_m, World::Type);

  static const luaL_Reg go_m[] = {
    {"add_component", Lgo_add_component},
    {"has_message", Lgo_has_message},
    {"create_message", Lgo_create_message},
    {"broadcast_message", Lgo_broadcast_message},
    {"contacts", Lgo_contacts},
    {"__gc", Lobject_special_gc},
    {NULL, NULL}};

  LClink_metatable(L, LUT_GO, go_m, GO::Type);

  static const luaL_Reg comp_m[] = {
    {"__gc", Lobject_special_gc},
    {NULL, NULL}};
  LClink_metatable(L, LUT_COMPONENT, comp_m, Component::Type);

  LCpush(L, world);
  lua_setglobal(L, "world");

  LCpush(L, world->stage);
  lua_setglobal(L, "stage");

  lua_pushnumber(L, screen_width);
  lua_setglobal(L, "screen_width");

  lua_pushnumber(L, screen_height);
  lua_setglobal(L, "screen_height");

  lua_pop(L, lua_gettop(L));
}

World::World(void* _universe)
  : L(NULL), bWorld(b2Vec2(0, -50)),
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
  free_thread(&pre_render);
  free_thread(&post_render);

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

  step_thread(&pre_render, NULL, NULL);
  cameras.foreach([this](Camera* camera) -> int {
      this->components.foreach([=](Component* comp) -> int {
          comp->render(camera);
          return 0;
        });
      camera->enqueue();
      return 0;
    });
  step_thread(&post_render, NULL, NULL);
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
OBJECT_METHOD(World, create_go, GO*, ());

Object* World::create_object(TypeInfo* type) {
  Object* obj = type->makeInstance(this);
  obj->reference_count = 0; // disown
  return obj;
}
OBJECT_METHOD(World, create_object, Object*, (TypeInfo*));

RevJoint* World::create_joint(GO* ga, Vector la, GO* gb, Vector lb) {
  b2RevoluteJoint *joint;

  //car class constructor
  b2RevoluteJointDef jointDef;
  jointDef.bodyA = ga->body;
  jointDef.enableLimit = true;
  jointDef.lowerAngle = 0;//with both these at zero...
  jointDef.upperAngle = 0;//...the joint will not move
  jointDef.localAnchorA.Set(la->x / BSCALE, la->y / BSCALE);

  jointDef.bodyB = gb->body;
  jointDef.localAnchorB.Set(lb->x / BSCALE, lb->y / BSCALE);

  joint = (b2RevoluteJoint*)bWorld.CreateJoint( &jointDef );
  RevJoint* rj = new RevJoint(joint);
  rj->world = this;
  return rj;
}
OBJECT_METHOD(World, create_joint, RevJoint*, (GO*, Vector, GO*, Vector));

SpriteAtlas World::atlas(const char* atlas) {
  return universe->atlas(atlas);
}
OBJECT_METHOD(World, atlas, SpriteAtlas, (const char*));

SpriteAtlasEntry World::atlas_entry(const char* atlas_name, const char* entry) {
  return universe->atlas_entry(atlas_name, entry);
}
OBJECT_METHOD(World, atlas_entry, SpriteAtlasEntry, (const char*, const char*));

Animation* World::animation(const char* filename, const char* atlas_name, const char* anim) {
  return universe->animation(filename, atlas(atlas_name), anim);
}
OBJECT_METHOD(World, animation, Animation*, (const char*, const char*, const char*));

Sound* World::get_sound(const char* name, float scale) {
  return universe->sound.get_sync(name, scale);
}
OBJECT_METHOD(World, get_sound, Sound*, (const char*, float));

AudioHandle* World::play_sound(Sound* sound, int channel) {
  return universe->play_sound(sound, channel);
}
OBJECT_METHOD(World, play_sound, AudioHandle*, (Sound*, int));

AudioHandle* World::stream_sound(const char* sound, long start_sample) {
  return universe->stream_sound(sound, start_sample);
}
OBJECT_METHOD(World, stream_sound, AudioHandle*, (const char*, long));

AudioHandle* World::sound_handle(long handle_name) {
  return universe->sound_handle(handle_name);
}
OBJECT_METHOD(World, sound_handle, AudioHandle*, (long));

long World::current_sample() const {
  return audio_current_sample();
}
OBJECT_METHOD(World, current_sample, long, ());

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

OBJECT_IMPL(Universe, Object);
OBJECT_PROPERTY(Universe, stash);
OBJECT_PROPERTY(Universe, lua_path);

Universe::Universe(void* _path)
  : stash(NULL) {
  if(_path) {
    lua_path = malloc_lstring((char*)_path, strlen((char*)_path));
  } else {
    lua_path = NULL;
  }
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
      handle->release();
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
