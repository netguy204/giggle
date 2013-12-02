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
#include "giggle.h"
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
  LString *result = (LString*)GIGGLE->renderer->alloc(sizeof(LString) + length + 1);
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
OBJECT_PROPERTY(Camera, viewport);

Camera::Camera(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), world2camera(NULL) {
  memset(renderables, 0, sizeof(renderables));
  memset(baseLayers, 0, sizeof(baseLayers));
  memset(layers, 0, sizeof(layers));
  memset(particles, 0, sizeof(particles));
  memset(testRects, 0, sizeof(testRects));

  // reasonable default
  world2camera.orthographic_proj(0.0f, GIGGLE->renderer->screen_width,
                                 0.0f, GIGGLE->renderer->screen_height,
                                 -1.0f, 1.0f);

  go->world->cameras.add_head(this);
}

Camera::~Camera() {
  go->world->cameras.remove(this);
  free_thread(&pre_render);
  free_thread(&post_render);
}

void Camera::addRenderable(int layer, Renderable* renderable, void* args) {
  RenderableCommand* command = (RenderableCommand*)GIGGLE->renderer->alloc(sizeof(RenderableCommand));
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

class SetCameraParams : public Renderable {
public:
  SetCameraParams(Matrix44& m, const Rect_& vp)
    : Renderable(NULL), matrix(&m), vp(vp) {
  }

  virtual void render(void* args) {
    GIGGLE->renderer->orthographic_projection = matrix;
    glViewport(vp.minx, vp.miny, vp.maxx, vp.maxy);
  }

  Matrix44 matrix;
  Rect_ vp;
};

void Camera::enqueue() {
  step_thread(&pre_render, go, this);

  // set transform
  void* stfbuff = GIGGLE->renderer->alloc(sizeof(SetCameraParams));
  SetCameraParams *params = new(stfbuff) SetCameraParams(world2camera, viewport);
  GIGGLE->renderer->enqueue(params, NULL);

  for(int ii = 0; ii < LAYER_MAX; ++ii) {
    if(renderables[ii]) {
      GIGGLE->renderer->enqueue(renderables[ii]);
      renderables[ii] = NULL;
    }
    if(baseLayers[ii]) {
      GIGGLE->renderer->enqueue(GIGGLE->renderer->basespritelist_renderer,
                                baseLayers[ii]);
    }
    if(layers[ii]) {
      GIGGLE->renderer->enqueue(GIGGLE->renderer->spritelist_renderer,
                                layers[ii]);
    }
    if(particles[ii]) {
      GIGGLE->renderer->enqueue(GIGGLE->renderer->coloredspritelist_renderer,
                                particles[ii]);
    }
    while(testRects[ii]) {
      GIGGLE->renderer->enqueue(GIGGLE->renderer->filledrect_renderer,
                                testRects[ii]);
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
OBJECT_PROPERTY(GO, world);

GO::GO(void* _world)
  : world((World*)_world), stash(NULL), delete_me(0), _handle(NULL) {
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
  if(_handle) {
    _handle->go = NULL;
    _handle->release();
  }

  DLLNode node = components.head;
  while(node) {
    DLLNode next = node->next;
    Component* comp = components.to_element(node);
    delete comp;
    node = next;
  }

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

struct LowerPriority {
  Component* comp;
  LowerPriority(Component* comp)
    : comp(comp) {
  }

  bool operator()(Component* other) {
    return comp->priority < other->priority;
  }
};

void GO::update(float dt) {
  // initialize new components
  DLLNode node = uninitialized_components.head;
  while(node) {
    DLLNode next = node->next;
    Component* comp = uninitialized_components.to_element(node);
    comp->init();
    uninitialized_components.remove(comp);

    components.insert_before_when(comp, LowerPriority(comp));
    world->components.insert_before_when(comp, LowerPriority(comp));
    node = next;
  }
}
OBJECT_METHOD(GO, update, void, (float));

void GO::messages_received() {
  // notify components
  DLLNode node = components.head;
  while(node) {
    DLLNode next = node->next;
    Component* comp = components.to_element(node);
    comp->messages_received();
    node = next;
  }

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

inline void b2ToVector(Vector v, const b2Vec2* b2) {
  v->x = b2->x * BSCALE;
  v->y = b2->y * BSCALE;
}

inline void vectorToB2(b2Vec2* b2, const Vector v) {
  b2->x = v->x / BSCALE;
  b2->y = v->y / BSCALE;
}

void GO::pos(Vector pos) {
  const b2Vec2& bPos = body->GetPosition();
  b2ToVector(pos, &bPos);
}

void GO::vel(Vector vel) {
  const b2Vec2& bVel = body->GetLinearVelocity();
  b2ToVector(vel, &bVel);
}

void GO::set_pos(Vector pos) {
  b2Vec2 bPos;
  vectorToB2(&bPos, pos);
  body->SetTransform(bPos, body->GetAngle());
}

void GO::set_vel(Vector vel) {
  b2Vec2 bVel;
  vectorToB2(&bVel, vel);
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

void GO::apply_impulse(Vector_ imp) {
  b2Vec2 bImp;
  bImp.x = imp.x / BSCALE;
  bImp.y = imp.y / BSCALE;
  body->ApplyLinearImpulse(bImp, body->GetWorldCenter());
}
OBJECT_METHOD(GO, apply_impulse, void, (Vector_));

void GO::apply_angular_impulse(float angimp) {
  body->ApplyAngularImpulse(angimp);
}
OBJECT_METHOD(GO, apply_angular_impulse, void, (float));

void GO::apply_torque(float torque) {
  body->ApplyTorque(torque);
}
OBJECT_METHOD(GO, apply_torque, void, (float));

void GO::apply_force(Vector_ force) {
  b2Vec2 bForce;
  bForce.x = force.x / BSCALE;
  bForce.y = force.y / BSCALE;

  body->ApplyForceToCenter(bForce);
}
OBJECT_METHOD(GO, apply_force, void, (Vector_));

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

GOHandle* GO::handle() {
  if(!_handle) {
    _handle = new GOHandle(this);
  }
  return _handle;
}

Message* GO::create_message(int kind, const char* content, size_t nbytes) {
  Message* msg = (Message*)GIGGLE->renderer->alloc(sizeof(Message) + nbytes);
  msg->source = handle();
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

OBJECT_IMPL(GOHandle, Object);

GOHandle::GOHandle(void* _go)
  : go((GO*)_go) {
}

Object* GOHandle::dereference() {
  return go;
}

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
OBJECT_METHOD(Component, init, void, ());

void Component::update(float dt) {
}
OBJECT_METHOD(Component, update, void, (float));

void Component::messages_received() {
}
OBJECT_METHOD(Component, messages_received, void, ());

void Component::render(Camera* camera) {
}
OBJECT_METHOD(Component, render, void, (Camera*));

Fixture::Fixture()
  : comp(NULL), fixture(NULL) {
}

Fixture::Fixture(Component* comp)
  : comp(comp), fixture(NULL) {
}

OBJECT_IMPL(CSensor, Component);
OBJECT_PROPERTY(CSensor, fixture);
OBJECT_PROPERTY(CSensor, kind);

CSensor::CSensor(void* _go)
  : Component((GO*)_go, PRIORITY_THINK),
    fixture(this), kind(MESSAGE_COLLIDING) {
}

CSensor::~CSensor() {
  if(fixture.fixture) {
    go->world->unregister_fixture(fixture.fixture);
    go->body->DestroyFixture(fixture.fixture);
  }
}

void CSensor::update(float dt) {
}

void CSensor::beginContact(GO* other) {
  const char* msg = "BEGIN";
  go->send_message(other->create_message(kind, msg, strlen(msg)));
}

void CSensor::endContact(GO* other) {
  const char* msg = "END";
  go->send_message(other->create_message(kind, msg, strlen(msg)));
}

LuaThread::LuaThread()
  : state(NULL), refid(-1), is_initialized(0) {
}

OBJECT_IMPL(CScripted, Component);
OBJECT_PROPERTY(CScripted, update_thread);
OBJECT_PROPERTY(CScripted, message_thread);
OBJECT_PROPERTY(CScripted, frame_skip);
OBJECT_PROPERTY(CScripted, current_frame);

CScripted::CScripted(void* go)
  : Component((GO*)go, PRIORITY_ACT), frame_skip(1), current_frame(0) {
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
  if(((current_frame++) % frame_skip) == 0) {
    step_thread(&update_thread, go, this);
  }

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
  Game* game;
  float value;
};

struct LuaSIData {
  void* fnkey;
  Game* game;
  SpatialInput value;
};

static void handle_lua_key(void* _kd) {
  LuaKeyData* kd = (LuaKeyData*)_kd;
  lua_pushlightuserdata(kd->game->L, kd->fnkey);
  lua_gettable(kd->game->L, LUA_REGISTRYINDEX);
  lua_pushnumber(kd->game->L, kd->value);
  lua_call(kd->game->L, 1, 0);
  kd->game->lk_alloc.free(kd);
}

struct LuaUnregisterData {
  /* sizeof(this) <= sizeof(LuaKeyData) */
  void* fnkey;
  Game* game;;
};

static void unregister_lua_fn(void* _ud) {
  LuaUnregisterData* ud = (LuaUnregisterData*)_ud;
  lua_pushlightuserdata(ud->game->L, ud->fnkey);
  lua_pushnil(ud->game->L);
  lua_settable(ud->game->L, LUA_REGISTRYINDEX);
}

LuaKeyBinding::LuaKeyBinding(KeyNumber keyn, Game* game)
  : game(game), keyn(keyn) {
  game->keybindings.add_head(this);
}

LuaKeyBinding::~LuaKeyBinding() {
  LuaUnregisterData* ud = (LuaUnregisterData*)game->lk_alloc.alloc();
  ud->fnkey = this;
  ud->game = game;
  game->enqueue_command(unregister_lua_fn, ud);
  game->keybindings.remove(this);
}

void LuaKeyBinding::activate(float value) {
  LuaKeyData* kd = (LuaKeyData*)game->lk_alloc.alloc();
  kd->fnkey = this; // we know this by prior arrangement
  kd->value = value;
  kd->game = game;
  game->enqueue_command(handle_lua_key, kd);
}

LuaSIBinding::LuaSIBinding(SpatialInputNumber keyn, Game* game)
  : game(game), keyn(keyn) {
  game->sibindings.add_head(this);
}

LuaSIBinding::~LuaSIBinding() {
  LuaUnregisterData* ud = (LuaUnregisterData*)game->lk_alloc.alloc();
  ud->fnkey = this;
  ud->game = game;
  game->enqueue_command(unregister_lua_fn, ud);
  game->sibindings.remove(this);
}

static void handle_si_key(void* _kd) {
  LuaSIData* kd = (LuaSIData*)_kd;
  lua_pushlightuserdata(kd->game->L, kd->fnkey);
  lua_gettable(kd->game->L, LUA_REGISTRYINDEX);
  lua_pushnumber(kd->game->L, kd->value.absolute.x);
  lua_pushnumber(kd->game->L, kd->value.absolute.y);
  lua_pushnumber(kd->game->L, kd->value.relative.x);
  lua_pushnumber(kd->game->L, kd->value.relative.y);
  lua_call(kd->game->L, 4, 0);
  kd->game->lk_alloc.free(kd);
}

void LuaSIBinding::activate(SpatialInput* value) {
  LuaSIData* kd = (LuaSIData*)game->lk_alloc.alloc();
  kd->fnkey = this; // we know this by prior arrangement
  kd->value = *value;
  kd->game = game;
  game->enqueue_command(handle_si_key, kd);
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

int GO::add_component(lua_State* L, int pos) {
  TypeInfo* type;
  LCcheck(L, &type, pos + 1);
  Component* comp = add_component(type);

  // argument 3 should be an optional table
  if(lua_gettop(L) != pos + 2) {
    LCpush(L, (Component*)comp);
    return 1;
  }

  if(!lua_istable(L, pos + 2)) {
    luaL_error(L, "argument 3 should be a table");
    return 0;
  }

  LCconfigure_object(L, comp, pos + 2);

  LCpush(L, (Component*)comp);
  return 1;
}
OBJECT_LUA_METHOD(GO, add_component);

int GO::create_message(lua_State* L, int pos) {
  int kind = luaL_checkinteger(L, pos + 1);
  const char* content = NULL;
  size_t nbytes = 0;
  if(lua_isstring(L, pos + 2)) {
    content = lua_tolstring(L, pos + 2, &nbytes);
  }

  Message* message = create_message(kind, content, nbytes);
  LCpush(L, message);
  return 1;
}
OBJECT_LUA_METHOD(GO, create_message);

int GO::has_message(lua_State* L, int pos) {
  int type = luaL_checkinteger(L, pos + 1);
  int rvalues = 0;

  DLLNode node = inbox.head;
  while(node) {
    DLLNode next = node->next;
    Message* msg = inbox.to_element(node);

    if(msg->kind == type) {
      if(rvalues == 0) {
        // create the return table
        lua_newtable(L);
      }

      LCpush(L, msg);
      lua_rawseti(L, -2, ++rvalues);
    }

    node = next;
  }

  if(rvalues > 0) {
    return 1;
  } else {
    return 0;
  }
}
OBJECT_LUA_METHOD(GO, has_message);

OBJECT_IMPL(World, Object);
OBJECT_PROPERTY(World, dt);
OBJECT_ACCESSOR(World, time_scale, get_time_scale, set_time_scale);
OBJECT_ACCESSOR(World, gravity, get_gravity, set_gravity);
OBJECT_PROPERTY(World, pre_render);
OBJECT_PROPERTY(World, post_render);
OBJECT_PROPERTY(World, delete_me);
OBJECT_PROPERTY(World, stage);
OBJECT_PROPERTY(World, render_disabled);

void init_lua(Game* game) {
  lua_State* L = luaL_newstate();
  game->L = L;

  luaL_openlibs(L);
  LCprepare_ooc(L);

  if(game->lua_path) {
    lua_getglobal(L, "package");
    LCpush(L, game->lua_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
  }

  static const luaL_Reg world_m[] = {
    {"__gc", Lobject_special_gc},
    {NULL, NULL}};
  LClink_metatable(L, world_m, World::Type);

  static const luaL_Reg comp_m[] = {
    {"__gc", Lobject_special_gc},
    {NULL, NULL}};
  LClink_metatable(L, comp_m, Component::Type);

  LCpush(L, game);
  lua_setglobal(L, "game");

  World* default_world = game->create_world();
  LCpush(L, default_world);
  lua_setglobal(L, "world");

  LCpush(L, default_world->stage);
  lua_setglobal(L, "stage");

  Camera* default_camera = (Camera*)default_world->stage->find_component(&Camera::Type, NULL);
  LCpush(L, default_camera);
  lua_setglobal(L, "camera");

  game->load_script("resources/init.lua");

  lua_pop(L, lua_gettop(L));
}

typedef std::map<b2Fixture*, CSensor*> FixtureToSensor;

class GlobalContactListener : public b2ContactListener {
 public:
  FixtureToSensor sensors;

  virtual void BeginContact(b2Contact* contact) {
    FixtureToSensor::iterator iter = sensors.find(contact->GetFixtureA());
    if(iter != sensors.end()) {
      iter->second->beginContact((GO*)contact->GetFixtureB()->GetBody()->GetUserData());
    }

    iter = sensors.find(contact->GetFixtureB());
    if(iter != sensors.end()) {
      iter->second->beginContact((GO*)contact->GetFixtureA()->GetBody()->GetUserData());
    }
  }

  virtual void EndContact(b2Contact* contact) {
    FixtureToSensor::iterator iter = sensors.find(contact->GetFixtureA());
    if(iter != sensors.end()) {
      iter->second->endContact((GO*)contact->GetFixtureB()->GetBody()->GetUserData());
    }

    iter = sensors.find(contact->GetFixtureB());
    if(iter != sensors.end()) {
      iter->second->endContact((GO*)contact->GetFixtureA()->GetBody()->GetUserData());
    }
  }
};

World::World(void* _game)
  : game((Game*)_game),
    saved_time_delta(0), max_delta(5),
    bWorld(b2Vec2(0, -50)),
    contact_listener(new GlobalContactListener()),
    delete_me(0),
    render_disabled(0) {

  bWorld.SetContactListener(contact_listener);
  clock = GIGGLE->clock_make();
  stage = create_go();
  Camera* camera = (Camera*)stage->add_component(&Camera::Type);
  Rect_ vp = {0, 0, (float)GIGGLE->renderer->screen_width,
              (float)GIGGLE->renderer->screen_height};
  camera->viewport = vp;
  random_init(&rgen, 1234);
}

struct DeleteGO {
  bool operator()(GO* go) {
    delete go;
    return 0;
  }
};

World::~World() {
  game_objects.foreach(DeleteGO());

  free_thread(&pre_render);
  free_thread(&post_render);

  GIGGLE->clock_free(clock);

  delete contact_listener;

  game->worlds.remove(this);
}

void World::update(long delta) {
  saved_time_delta += delta;

  while(saved_time_delta > 0) {
    long step_delta = MIN(saved_time_delta, max_delta);
    saved_time_delta -= step_delta;
    update_step(step_delta);
  }

  if(render_disabled) return;

  // issue render requests
  step_thread(&pre_render, NULL, NULL);
  DLLNode node = cameras.head;
  while(node) {
    DLLNode next = node->next;
    Camera* camera = cameras.to_element(node);

    DLLNode cnode = components.head;
    while(cnode) {
      DLLNode cnext = cnode->next;
      Component* comp = components.to_element(cnode);
      comp->render(camera);
      cnode = cnext;
    }
    camera->enqueue();
    node = next;
  }
  step_thread(&post_render, NULL, NULL);
}

void World::update_step(long delta) {
  this->dt = clock->update(delta / 1000.0);;

  // do an integration step
  bWorld.Step(dt, 6, 2);

  // let the game objects initialize any new components
  DLLNode node = game_objects.head;
  while(node) {
    DLLNode next = node->next;
    GO* go = game_objects.to_element(node);
    if(go->delete_me) {
      delete go;
    } else {
      go->update(dt);
    }
    node = next;
  }

  // update the comonents
  node = components.head;
  while(node) {
    DLLNode next = node->next;
    Component* comp = components.to_element(node);
    if(comp->delete_me) {
      delete(comp);
    } else {
      comp->update(dt);
    }
    node = next;
  }

  // run go messages until all messages have been handled
  while(have_waiting_messages.head) {
    node = have_waiting_messages.head;
    while(node) {
      DLLNode next = node->next;
      GO* go = have_waiting_messages.to_element(node);
      // move the pending messages into the inbox and remove from
      // waiting list
      go->inbox = go->inbox_pending;
      go->inbox_pending.zero();
      have_waiting_messages.remove(go);

      // tell GO to consume messages
      go->messages_received();
      node = next;
    }
  }
}

GO* World::create_go() {
  GO* go = new GO(this);
  return go;
}
OBJECT_METHOD(World, create_go, GO*, ());

float World::random_gaussian() {
  return random_next_gaussian(&rgen);
}
OBJECT_METHOD(World, random_gaussian, float, ());

RevJoint* World::create_joint(GO* ga, Vector_ la, GO* gb, Vector_ lb) {
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

  joint = (b2RevoluteJoint*)bWorld.CreateJoint( &jointDef );
  RevJoint* rj = new RevJoint(joint);
  rj->world = this;
  return rj;
}
OBJECT_METHOD(World, create_joint, RevJoint*, (GO*, Vector_, GO*, Vector_));

class RayCallbackListener : public b2RayCastCallback {
public:
  lua_State* L;
  RayCastKind kind;
  int next_idx;

  b2Fixture* closest_fixture;
  b2Vec2 closest_point;
  b2Vec2 closest_normal;
  float32 closest_fraction;

  RayCallbackListener(lua_State* L, RayCastKind kind)
    : L(L), kind(kind), next_idx(1), closest_fraction(-1) {
  }

  void push_result(b2Fixture* fixture, const b2Vec2& point,
                   const b2Vec2& normal, float32 fraction) {
    lua_createtable(L, 4, 0);

    GO* go = (GO*)fixture->GetBody()->GetUserData();
    LCpush(L, go);
    lua_rawseti(L, -2, 1);

    Vector_ p;
    b2ToVector(&p, &point);
    LCpush(L, p);
    lua_rawseti(L, -2, 2);

    Vector_ n;
    b2ToVector(&n, &normal);
    LCpush(L, n);
    lua_rawseti(L, -2, 3);

    lua_pushnumber(L, fraction);
    lua_rawseti(L, -2, 4);
  }

  virtual float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point,
                                const b2Vec2& normal, float32 fraction) {
    if(kind == RAYCAST_CLOSEST) {
      if(closest_fraction < 0 || fraction < closest_fraction) {
        closest_fixture = fixture;
        closest_point = point;
        closest_normal = normal;
        closest_fraction = fraction;
      }
      return fraction;
    } else {
      push_result(fixture, point, normal, fraction);

      // add it to our result table
      lua_rawseti(L, -2, next_idx++);
      if(kind == RAYCAST_EVERYTHING) {
        return 1;
      } else {
        return fraction;
      }
    }
  }
};

int World::raycast(lua_State* L, int pos) {
  Vector_ origin, dest;
  int kind;

  LCcheck(L, &origin, pos + 1);
  LCcheck(L, &dest, pos + 2);
  LCcheck(L, &kind, pos + 3);

  b2Vec2 origin2, dest2;
  vectorToB2(&origin2, &origin);
  vectorToB2(&dest2, &dest);


  if(kind == RAYCAST_CLOSEST) {
    RayCallbackListener listener(L, (RayCastKind)kind);
    bWorld.RayCast(&listener, origin2, dest2);
    if(listener.closest_fraction > 0) {
      listener.push_result(listener.closest_fixture, listener.closest_point,
                           listener.closest_normal, listener.closest_fraction);
    } else {
      lua_pushnil(L);
    }
    return 1;
  } else {
    // result is in the table that the listener left on the stack
    lua_newtable(L);
    RayCallbackListener listener(L, (RayCastKind)kind);
    bWorld.RayCast(&listener, origin2, dest2);
    return 1;
  }
}
OBJECT_LUA_METHOD(World, raycast);

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

struct SendMessage {
  GO* sender;
  int kind;
  const char* content;
  size_t nbytes;

  SendMessage(GO* sender, int kind, const char* content, size_t nbytes)
    : sender(sender), kind(kind), content(content), nbytes(nbytes) {
  }

  bool operator()(GO* go) {
    go->send_message(sender->create_message(kind, content, nbytes));
    return 0;
  }
};

void World::register_fixture(b2Fixture* fixture, CSensor* sensor) {
  contact_listener->sensors.insert(std::make_pair(fixture, sensor));
}

void World::unregister_fixture(b2Fixture* fixture) {
  contact_listener->sensors.erase(fixture);
}

OBJECT_IMPL(Game, Object);
OBJECT_PROPERTY(Game, stash);
OBJECT_PROPERTY(Game, lua_path);

Game::Game(void* _path)
  : stash(NULL),
    lk_alloc(MAX(sizeof(LuaKeyData), sizeof(LuaSIData)),
             MAX_INFLIGHT_INPUTS, "lk_alloc"),
    cmd_alloc(sizeof(Command), MAX_INFLIGHT_INPUTS*2, "cmd_alloc") {
  if(_path) {
    lua_path = malloc_lstring((char*)_path, strlen((char*)_path));
  } else {
    lua_path = NULL;
  }
  init_lua(this);
}

struct NullKeyBinding {
  bool operator()(LuaKeyBinding* kb) {
    set_key_binding(kb->keyn, NULL);
    return 0;
  }
};

struct NullSIBinding {
  bool operator()(LuaSIBinding* kb) {
    set_si_binding(kb->keyn, NULL);
    return 0;
  }
};

Game::~Game() {
  if(stash) free_lstring(stash);

  // shutdown lua first
  lua_close(L);

  // remove keybindings
  keybindings.foreach(NullKeyBinding());
  sibindings.foreach(NullSIBinding());

  // delete worlds
  DLLNode node = worlds.head;
  while(node) {
    DLLNode next = node->next;
    World* world = worlds.to_element(node);
    delete world;
    node = next;
  }

  // free our spriter and atlas caches
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

void Game::load_script(const char* script) {
  if(!luaL_dofile(L, script)) {
    lua_getglobal(L, "game_init");
    if(!lua_isnil(L, -1)) {
      lua_call(L, 0, 0);
    } else {
      lua_pop(L, 1);
      fail_exit("`game_init' was not defined after loading %s", script);
    }
  } else {
    const char* error = lua_tostring(L, -1);
    fail_exit("script loading failed: %s", error);
  }
}

World* Game::create_world() {
  World* world = new World(this);
  worlds.add_tail(world);
  return world;
}
OBJECT_METHOD(Game, create_world, World*, ());

Object* Game::create_object(TypeInfo* type) {
  Object* obj = type->makeInstance(this);
  obj->reference_count = 0; // disown
  return obj;
}
OBJECT_METHOD(Game, create_object, Object*, (TypeInfo*));

void Game::show_mouse_cursor(int show) {
  GIGGLE->renderer->show_mouse(show);
}
OBJECT_METHOD(Game, show_mouse_cursor, void, (int));

int Game::set_keybinding(lua_State* L, int pos) {
  const char* keyname = luaL_checkstring(L, pos + 1);
  int keyn = find_keynumber(keyname);
  if(keyn < 0) {
    lua_pushnil(L);
    return 1;
  }

  LuaKeyBinding* kb = new LuaKeyBinding((KeyNumber)keyn, this);

  // store the provided closure, using the kb as the registry key
  lua_pushlightuserdata(L, kb);
  lua_pushvalue(L, pos + 2); // the closure
  lua_settable(L, LUA_REGISTRYINDEX);
  set_key_binding((KeyNumber)keyn, kb);

  lua_pushboolean(L, true);
  return 1;
}
OBJECT_LUA_METHOD(Game, set_keybinding);

int Game::set_sibinding(lua_State* L, int pos) {
  const char* keyname = luaL_checkstring(L, pos + 1);
  int keyn = find_sinumber(keyname);
  if(keyn < 0) {
    lua_pushnil(L);
    return 1;
  }

  LuaSIBinding* kb = new LuaSIBinding((SpatialInputNumber)keyn, this);

  // store the provided closure, using the kb as the registry key
  lua_pushlightuserdata(L, kb);
  lua_pushvalue(L, pos + 2); // the closure
  lua_settable(L, LUA_REGISTRYINDEX);
  set_si_binding((SpatialInputNumber)keyn, kb);

  lua_pushboolean(L, true);
  return 1;
}
OBJECT_LUA_METHOD(Game, set_sibinding);

SpriteAtlas Game::atlas(const char* atlas_name) {
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
OBJECT_METHOD(Game, atlas, SpriteAtlas, (const char*));

SpriteAtlasEntry Game::atlas_entry(SpriteAtlas atlas, const char* entry) {
  return spriteatlas_find(atlas, entry);
}

SpriteAtlasEntry Game::atlas_entry(const char* atlas_name, const char* entry) {
  return spriteatlas_find(atlas(atlas_name), entry);
}
OBJECT_METHOD(Game, atlas_entry, SpriteAtlasEntry, (const char*, const char*));

Entity* Game::scml_entity(const char* filename, SpriteAtlas atlas) {
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

void Game::update(long delta) {
  // handle incoming commands
  evaluate_commands();

  // allow all worlds to update
  DLLNode node = worlds.head;
  while(node) {
    DLLNode next = node->next;
    World* world = worlds.to_element(node);
    if(world->delete_me) {
      delete world;
    } else {
      world->update(delta);
    }
    node = next;
  }
}

Animation* Game::animation(const char* filename, const char* atlasname, const char* anim) {
  return spriter_find(scml_entity(filename, atlas(atlasname)), anim);
}
OBJECT_METHOD(Game, animation, Animation*, (const char*, const char*, const char*));

Sound* Game::get_sound(const char* name, float scale) {
  return GIGGLE->audio->get_sound(name, scale);
}
OBJECT_METHOD(Game, get_sound, Sound*, (const char*, float));

AudioHandle* Game::play_sound(Sound* s, int channel)  {
  return GIGGLE->audio->play_sound(s, channel);
}
OBJECT_METHOD(Game, play_sound, AudioHandle*, (Sound*, int));

AudioHandle* Game::stream_sound(const char* fname, long start_sample) {
  return GIGGLE->audio->stream_sound(fname, start_sample);
}
OBJECT_METHOD(Game, stream_sound, AudioHandle*, (const char*, long));

AudioHandle* Game::sound_handle(long handle) {
  return GIGGLE->audio->sound_handle(handle);
}
OBJECT_METHOD(Game, sound_handle, AudioHandle*, (long));

long Game::current_sample() const {
  return GIGGLE->audio->current_sample();
}
OBJECT_METHOD(Game, current_sample, long, ());

void Game::enqueue_command(CommandFunction fn, void* data) {
  Command* command = (Command*)cmd_alloc.alloc();
  command->function = fn;
  command->data = data;
  command_queue.enqueue(command);
}

void Game::evaluate_commands() {
  Command* cmd;
  while((cmd = command_queue.dequeue_noblock()) != NULL) {
    cmd->function(cmd->data);
    cmd_alloc.free(cmd);
  }
}
