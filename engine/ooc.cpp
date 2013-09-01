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
#include "ooc.h"

TypeInfo::TypeInfo(const char* name, CtorFn ctor, size_t size, TypeInfo* parent)
  : m_name(name), m_ctor(ctor), m_parent(parent), m_size(size) {
  TypeRegistry::instance().register_type(this);
}

void TypeInfo::register_property(PropertyInfo* property) {
  name_to_property.insert(std::make_pair(property->name(), property));
}

void TypeInfo::register_method(MethodInfo* method) {
  name_to_method.insert(std::make_pair(method->name(), method));
}

const PropertyInfo* TypeInfo::property(const char* name) const {
  NameToProperty::const_iterator iter = name_to_property.find(name);
  if(iter == name_to_property.end()) {
    if(m_parent) return m_parent->property(name);
    return NULL;
  }
  return iter->second;
}

const MethodInfo* TypeInfo::method(const char* name) const {
  NameToMethod::const_iterator iter = name_to_method.find(name);
  if(iter == name_to_method.end()) {
    if(m_parent) return m_parent->method(name);
    return NULL;
  }
  return iter->second;
}

const char* TypeInfo::name() const {
  return m_name;
}

const TypeInfo* TypeInfo::parent() const {
  return m_parent;
}

size_t TypeInfo::size() const {
  return m_size;
}

Object* TypeInfo::makeInstance(void* init) const {
  return m_ctor(init);
}

bool TypeInfo::isInstanceOf(const TypeInfo* other) const {
  // add superclass info
  if(this == other) return true;
  if(!parent()) return false;
  return parent()->isInstanceOf(other);
}

void TypeInfo::print() const {
  fprintf(stderr, "methods:\n");
  for(NameToMethod::const_iterator iter = name_to_method.begin();
      iter != name_to_method.end(); ++iter) {
    fprintf(stderr, "  %s\n", iter->first);
  }

  fprintf(stderr, "properties:\n");
  for(NameToProperty::const_iterator iter = name_to_property.begin();
      iter != name_to_property.end(); ++iter) {
    fprintf(stderr, "  %s\n", iter->first);
  }
}

Object* LCcheck_object(lua_State *L, int pos) {
  if(lua_isnil(L, pos)) {
    return NULL;
  }

  void** ud = (void**)lua_touserdata(L, pos);
  if(ud == NULL) return NULL;
  return (Object*)*ud;
}

static int Lobject_method(lua_State* L) {
  const MethodInfo* method = (const MethodInfo*)lua_touserdata(L, lua_upvalueindex(1));
  return method->LCinvoke(L, 1);
}

static int Lobject_property(lua_State* L) {
  const PropertyInfo* prop = (const PropertyInfo*)lua_touserdata(L, lua_upvalueindex(1));
  Object* obj = LCcheck_object(L, 1);
  if(!obj) luaL_argerror(L, 1, "setting property on null object");

  obj = obj->dereference();
  if(!obj) luaL_argerror(L, 1, "object is no longer valid");

  if(lua_gettop(L) == 1) {
    prop->LCpush_value(obj, L);
    return 1;
  } else {
    prop->LCset_value(obj, L, 2);
    return 0;
  }
}

// modify the metatable on this call so that we don't have to do a
// method lookup more than once
static int Lobject_mutate(lua_State* L) {
  const char* name = luaL_checkstring(L, lua_upvalueindex(1));
  Object* obj = LCcheck_object(L, 1);
  Object* robj = obj->dereference();

  // try finding a method first
  const MethodInfo* method = robj->typeinfo()->method(name);
  if(method) {
    // build our cache
    obj->typeinfo()->push_metatable(L);
    lua_pushstring(L, name);
    lua_pushlightuserdata(L, (void*)method);
    lua_pushcclosure(L, Lobject_method, 1);
    lua_settable(L, -3);
    lua_pop(L, 1);
    return method->LCinvoke(L, 1);
  }

  // now try the property
  const PropertyInfo* prop = robj->typeinfo()->property(name);
  if(prop == NULL) {
    luaL_error(L, "`%s' does not have property `%s'",
               robj->typeinfo()->name(), name);
  }

  // build the cache
  obj->typeinfo()->push_metatable(L);
  lua_pushstring(L, name);
  lua_pushlightuserdata(L, (void*)prop);
  lua_pushcclosure(L, Lobject_property, 1);
  lua_settable(L, -3);
  lua_pop(L, 1);

  if(lua_gettop(L) == 1) {
    prop->LCpush_value(robj, L);
    return 1;
  } else {
    prop->LCset_value(robj, L, 2);
    return 0;
  }
}

static int Lobject_index(lua_State* L) {
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

static int Lobject_gc(lua_State* L) {
  Object* obj = LCcheck_object(L, 1);
  obj->release();
  return 0;
}

static int Lobject_metaclass_object(lua_State* L) {
  Object* obj;
  MetaclassObject* mco;

  LCcheck(L, &obj, 1);
  mco = obj->metaclass_object(L);
  mco->reference_count = 0; // disown
  LCpush(L, mco);
  return 1;
}

void TypeInfo::push_metatable(lua_State* L) const {
  if(luaL_newmetatable(L, name())) {
    static const luaL_Reg object_m[] = {
      {"__index", Lobject_index},
      {NULL, NULL}};

    static const luaL_Reg defaults_m[] = {
      {"__tostring", Lobject_tostring},
      {"__eq", Lobject_eq},
      {"__gc", Lobject_gc},
      {"key", Lobject_key},
      {"metaclass_object", Lobject_metaclass_object},
      {NULL, NULL}};
    luaL_setfuncs(L, defaults_m, 0);

    // always get __gc from our parent if it's available
    if(parent()) {
      parent()->push_metatable(L);
      lua_pushstring(L, "__gc");
      lua_pushstring(L, "__gc");
      lua_gettable(L, -3);
      if(lua_isnil(L, -1)) {
        lua_pop(L, 3);
      } else {
        lua_settable(L, -4);
        lua_pop(L, 1);
      }
    }

    // property lookups should hit main metatable first
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    // build the metatable of the metatable as the backup plan
    lua_newtable(L);
    luaL_setfuncs(L, object_m, 0);
    lua_setmetatable(L, -2);
  }
}

void LClink_metatable(lua_State* L, const luaL_Reg* funcs, TypeInfo& info) {
  info.push_metatable(L);
  luaL_setfuncs(L, funcs, 0);
  lua_pop(L, 1);
}

static int Lmco_override(lua_State* L) {
  MetaclassObject *mco;
  const char* name;

  LCcheck(L, &mco, 1);
  LCcheck(L, &name, 2);

  if(!lua_isfunction(L, 3)) {
    luaL_argerror(L, 3, "expected function");
  }

  lua_pushvalue(L, 3);
  long refid = luaL_ref(L, LUA_REGISTRYINDEX);
  mco->override(name, refid);
  return 0;
}

static int Lmco_spawn(lua_State* L) {
  MetaclassObject *mco;
  Object *arg;

  LCcheck(L, &mco, 1);
  LCcheck(L, &arg, 2);

  Object* result = mco->spawn(arg->dereference());
  result->reference_count = 0; // disown
  LCpush(L, result);
  return 1;
}

void LCprepare_ooc(lua_State* L) {
  static const luaL_Reg mco_m[] = {
    {"override", Lmco_override},
    {"spawn", Lmco_spawn},
    {NULL, NULL}};

  LClink_metatable(L, mco_m, MetaclassObject::Type);
}

PropertyInfo::PropertyInfo(TypeInfo* type, const char* name, size_t size)
  : m_type(type), m_size(size), m_name(name) {
  m_type->register_property(this);
}

const char* PropertyInfo::name() const {
  return m_name;
}

size_t PropertyInfo::size() const {
  return m_size;
}

bool cmp_str::operator()(char const *a, char const *b) const {
  return strcmp(a, b) < 0;
}

TypeRegistry& TypeRegistry::instance() {
  static TypeRegistry registry;
  return registry;
}

void TypeRegistry::register_type(TypeInfo* type) {
  name_to_type.insert(std::make_pair(type->name(), type));
}

TypeInfo* TypeRegistry::find_type(const char* name) {
  NameToType::iterator iter = name_to_type.find(name);
  if(iter == name_to_type.end()) return NULL;
  return iter->second;
}

OBJECT_BIMPL(Object, NULL);

Object::~Object() {
}

Object* Object::CreateInstance(void* init) {
  return new Object();
}

MetaclassObject* Object::metaclass_object(lua_State* L) const {
  return NULL;
}

void Object::destroy() {
  delete this;
}

Object* Object::dereference() {
  return this;
}

OBJECT_BIMPL(MetaclassObject, &Object::Type);

Object* MetaclassObject::spawn(Object* arg) {
  fail_exit("MetaclassObject::spawn not implemented");
  return NULL;
}

void MetaclassObject::override(const char* name, long refid) {
  fail_exit("MetaclassObject::override not implemented");
}

MethodInfo::MethodInfo(TypeInfo* type, const char* name)
  : m_type(type), m_name(name), m_voffset(-1) {
  m_type->register_method(this);
}

const char* MethodInfo::name() const {
  return m_name;
}

int Lobject_special_gc(lua_State* L) {
  // used by GO and Component, which do not participate in reference
  // counting
  return 0;
}

void LCpush_lut(lua_State *L, Object* ut) {
  if(!ut) {
    lua_pushnil(L);
  } else {
    void** p = (void**)lua_newuserdata(L, sizeof(void*));
    ut->typeinfo()->push_metatable(L);
    lua_setmetatable(L, -2);
    *p = ut->retain();
  }
}
