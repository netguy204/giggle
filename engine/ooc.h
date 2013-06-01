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
#ifndef OOC_H
#define OOC_H

#include <lua.hpp>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <map>
#include <string>
#include <typeinfo>

class Object;

typedef Object*(*CtorFn)(void*);

struct cmp_str {
  bool operator()(char const *a, char const *b) const;
};

class PropertyInfo;
class MethodInfo;

typedef std::map<const char*, PropertyInfo*, cmp_str> NameToProperty;
typedef std::map<const char*, MethodInfo*, cmp_str> NameToMethod;
typedef std::map<const char*, std::string> TypeTags;

class TypeInfo {
public:
  TypeInfo(const char* name, CtorFn ctor, TypeInfo* parent);
  void register_property(PropertyInfo* property);
  void register_method(MethodInfo* method);

  const PropertyInfo* property(const char* name) const;
  const MethodInfo* method(const char* name) const;

  const char* name() const;
  const TypeInfo* parent() const;
  Object* makeInstance(void*) const;
  bool isInstanceOf(const TypeInfo* other) const;

  const char* metatable() const;
  void set_metatable_flag(bool flag);

private:
  NameToProperty name_to_property;
  NameToMethod name_to_method;
  bool mt_flag;

  const char* m_name;
  CtorFn m_ctor;
  TypeInfo* m_parent;
};

class PropertyType;

class PropertyInfo {
 public:
  PropertyInfo(TypeInfo* type, const char* name, size_t size);

  const char* name() const;
  size_t size() const;

  virtual void set_value(Object* obj, void* value) const = 0;
  virtual void get_value(Object* obj, void* value) const = 0;
  virtual void LCset_value(Object* obj, lua_State* L,  int pos) const = 0;
  virtual void LCpush_value(Object* obj, lua_State* L) const = 0;

  TypeInfo* m_type;
  size_t m_size;
  const char* m_name;
};

template <typename T>
class PropertyTypeImpl : public PropertyInfo {
public:
  PropertyTypeImpl(TypeInfo* type, const char* name, size_t offset)
    : PropertyInfo(type, name, sizeof(T)), m_offset(offset) {
  }

  virtual void set_value(Object* obj, void* value) const {
    memcpy((char*)obj + m_offset, value, sizeof(T));
  }

  virtual void get_value(Object* obj, void* value) const {
    memcpy(value, (char*)obj + m_offset, sizeof(T));
  }

  virtual void LCset_value(Object* obj, lua_State* L, int pos) const {
    T value;
    get_value(obj, &value);
    LCcheck(L, &value, pos);
    set_value(obj, &value);
  }

  virtual void LCpush_value(Object* obj, lua_State* L) const {
    T value;
    get_value(obj, &value);
    LCpush(L, value);
  }

  size_t m_offset;
};

template <typename Sig>
class PropertyMethodImpl;

template <typename CLASS, typename T>
class PropertyMethodImpl<void (CLASS::*)(T)> : public PropertyInfo {
public:
  typedef T (CLASS::*Getter)();
  typedef void (CLASS::*Setter)(T);

  PropertyMethodImpl(TypeInfo* type, const char* name,
                     Getter getter, Setter setter)
    : PropertyInfo(type, name, sizeof(T)), m_setter(setter), m_getter(getter) {
  }

  virtual void set_value(Object* _obj, void* _value) const {
    CLASS* obj = (CLASS*)_obj;
    T* value = (T*)_value;
    (obj->*m_setter)(*value);
  }

  virtual void get_value(Object* _obj, void* _value) const {
    CLASS* obj = (CLASS*)_obj;
    T* value = (T*)_value;
    *value = (obj->*m_getter)();
  }

  virtual void LCset_value(Object* obj, lua_State* L, int pos) const {
    T value;
    get_value(obj, &value);
    LCcheck(L, &value, pos);
    set_value(obj, &value);
  }

  virtual void LCpush_value(Object* obj, lua_State* L) const {
    T value;
    get_value(obj, &value);
    LCpush(L, value);
  }

  Getter m_getter;
  Setter m_setter;
};

typedef std::map<const char*, TypeInfo*, cmp_str> NameToType;

class TypeRegistry {
 public:
  static TypeRegistry& instance();

  void register_type(TypeInfo* type);
  TypeInfo* find_type(const char* name);

  NameToType name_to_type;
};

#define OBJECT_PROTO(name)                            \
  virtual const TypeInfo* typeinfo();                 \
  static TypeInfo Type;                               \
  static Object* CreateInstance(void*)


#define OBJECT_BIMPL(name, ptype)                               \
  TypeInfo name::Type(#name, name::CreateInstance, ptype);      \
  const TypeInfo* name::typeinfo() {                            \
    return &(name::Type);                                       \
  }

#define OBJECT_IMPL(name, pname)                        \
  Object* name::CreateInstance(void* init) {            \
    return new name(init);                              \
  }                                                     \
  OBJECT_BIMPL(name, &pname::Type)

#define STRINGIFY(name) #name

#define OP(name, member_name) name ## _ ## member_name ## _ ## Property

#define OBJECT_PROPERTY(name, member_name)                              \
  static PropertyTypeImpl<decltype(((name*)0)->member_name)>            \
    OP(name, member_name)(                                              \
      &name::Type,                                                      \
      #member_name,                                                     \
      offsetof(name, member_name));

#define OBJECT_ACCESSOR(name, member_name, getter, setter)           \
  static PropertyMethodImpl<decltype(&name::setter)>                 \
    OP(name, member_name)(                                           \
      &name::Type,                                                   \
      #member_name,                                                  \
      &name::getter,                                                 \
      &name::setter)


class VoidFunction {
 public:
  virtual void invoke() = 0;
};

class MethodInfo {
 public:
  MethodInfo(TypeInfo* type, const char* name);

  const char* name() const;
  virtual int LCinvoke(lua_State* L, int pos) const = 0;
  virtual VoidFunction* LCframebind(lua_State* L, int pos) const = 0;
  virtual VoidFunction* framebind(Object* obj, ...) const = 0;

  TypeInfo* m_type;
  const char* m_name;
};

// now for some serious pre-processor black magic.
#include "cpp.h"

#define LLC($, X) NOT(ISEMPTY(TAIL(X)))
#define LLU($, X) CONS(INC(HEAD(X)), TAIL(TAIL(X)))
#define LLF($, X)

#define GENVARS2(TYPE, OFFSET)                               \
  TYPE JOIN(var, OFFSET);
#define GENVARS1($, X) GENVARS2(HEAD(TAIL(X)), HEAD(X))
#define GENVARS(X) IF(NOT(ISEMPTY(X)), JOIN(RECR_D, 0)(1, LLC, GENVARS1, LLU, LLF, CONS(1, X)))

#define GENCHECKS2(TYPE, OFFSET)                               \
  LCcheck(L, &JOIN(var, OFFSET), pos + OFFSET);
#define GENCHECKS1($, X) GENCHECKS2(HEAD(TAIL(X)), HEAD(X))
#define GENCHECKS(X) IF(NOT(ISEMPTY(X)), JOIN(RECR_D, 0)(1, LLC, GENCHECKS1, LLU, LLF, CONS(1, X)))

#define CHECK_FLOAT(X) CHECK(JOIN(CHECK_FLOAT_, X))
#define CHECK_FLOAT_float ~, 1,

#define FLOAT2DOUBLE(TYPE) IF_ELSE(CHECK_FLOAT(TYPE), double, TYPE)

#define GENVAARGS2(TYPE, OFFSET)                               \
  JOIN(var, OFFSET) = va_arg(*args, FLOAT2DOUBLE(TYPE));
#define GENVAARGS1($, X) GENVAARGS2(HEAD(TAIL(X)), HEAD(X))
#define GENVAARGS(X) IF(NOT(ISEMPTY(X)), JOIN(RECR_D, 0)(1, LLC, GENVAARGS1, LLU, LLF, CONS(1, X)))

#define VARNAME0(item, pos1) JOIN(var, pos1)
#define VARNAME(item, pos) VARNAME0(var, INC(pos))

#define CHECK_VOID(X) CHECK(JOIN(CHECK_VOID_, X))
#define CHECK_VOID_void ~, 1,

#define RETURNS(TYPE, ARGS, METHOD)              \
  TYPE r = obj->METHOD MAP(VARNAME, ARGS);       \
  LCpush(L, r);                                  \
  return 1;

#define NORETURN(ARGS, METHOD)                   \
  obj->METHOD MAP(VARNAME, ARGS);                \
  return 0;

#define LOP(name, method_name) L ## name ## method_name
#define LOPC(name, method_name) LCLASS ## name ## method_name
#define FLOPC(name, method_name) FLCLASS ## name ## method_name

#define OBJECT_BASE_METHOD(CLASS, METHOD, RTYPE, ARGS)                  \
  class FLOPC(CLASS, METHOD) : public VoidFunction {                    \
  public:                                                               \
    CLASS* obj;                                                         \
    GENVARS(ARGS);                                                      \
                                                                        \
    void invoke() {                                                     \
      obj->METHOD MAP(VARNAME, ARGS);                                   \
    }                                                                   \
                                                                        \
    void LCbind(lua_State* L, int pos) {                                \
      LCcheck(L, &obj, pos);                                            \
      GENCHECKS(ARGS);                                                  \
    }                                                                   \
                                                                        \
    void bind(va_list* args) {                                          \
      GENVAARGS(ARGS);                                                  \
    }                                                                   \
  };                                                                    \
  class LOPC(CLASS, METHOD) : public MethodInfo {                       \
  public:                                                               \
                                                                        \
  LOPC(CLASS, METHOD)()                                                 \
    : MethodInfo(&CLASS::Type, STRINGIFY(METHOD)) {                     \
    }                                                                   \
                                                                        \
    virtual int LCinvoke(lua_State* L, int pos) const;                  \
                                                                        \
    virtual VoidFunction* LCframebind(lua_State* L, int pos) const {    \
      void* buffer = frame_alloc(sizeof(FLOPC(CLASS,METHOD)));          \
      FLOPC(CLASS, METHOD) *bound = new(buffer) FLOPC(CLASS, METHOD)(); \
      bound->LCbind(L, pos);                                            \
      return bound;                                                     \
    }                                                                   \
    virtual VoidFunction* framebind(Object* obj, ...) const {           \
      va_list args;                                                     \
      va_start(args, obj);                                              \
      void* buffer = frame_alloc(sizeof(FLOPC(CLASS,METHOD)));          \
      FLOPC(CLASS, METHOD) *bound = new(buffer) FLOPC(CLASS, METHOD)(); \
      bound->obj = (CLASS*)obj;                                         \
      bound->bind(&args);                                               \
      return bound;                                                     \
    }                                                                   \
  };                                                                    \
  static LOPC(CLASS, METHOD) LOP(CLASS, METHOD);                        \
                                                                        \
  int LOPC(CLASS, METHOD)::LCinvoke(lua_State* L, int pos) const

#define OBJECT_METHOD(CLASS, METHOD, RTYPE, ARGS)                       \
  OBJECT_BASE_METHOD(CLASS, METHOD, RTYPE, ARGS) {                      \
    CLASS* obj;                                                         \
    GENVARS(ARGS);                                                      \
    LCcheck(L, &obj, pos);                                              \
    GENCHECKS(ARGS);                                                    \
    IF_ELSE(CHECK_VOID(RTYPE),                                          \
            NORETURN(ARGS, METHOD),                                     \
            RETURNS(RTYPE, ARGS, METHOD))                               \
  }

#define DEFERRED_OBJECT_METHOD(TARGET, CLASS, METHOD, RTYPE, ARGS)      \
  OBJECT_BASE_METHOD(CLASS, METHOD, RTYPE, ARGS) {                      \
    TARGET(LCframebind(L, pos));                                        \
    return 0;                                                           \
  }

#define DEFERRED_INVOKE(TARGET, OBJECT, METHOD, ...)                    \
  do {                                                                  \
    const MethodInfo* info = OBJECT->typeinfo()->method(STRINGIFY(METHOD)); \
    VoidFunction* fn = info->framebind(OBJECT, __VA_ARGS__);            \
    TARGET(fn);                                                         \
  } while(false)


class Object {
public:
  OBJECT_PROTO(Object);

  // a newly constructed object will have a reference count of 1
  inline Object()
    : reference_count(1) {
  }

  inline Object* retain() {
    ++reference_count;
    return this;
  }

  inline void release() {
    --reference_count;
    if(reference_count <= 0) {
      destroy();
    }
  }

  virtual void destroy();

  int reference_count;
};


#define LUT_OBJECT "Object"

void LClink_metatable(lua_State *L, const luaL_Reg* table, TypeInfo& type);
void LCinit_object_metatable(lua_State* L);

// use this to override __gc if you don't want LUA to participate in
// reference counting. This is necessary to break some cyclic
// references and to ensure that certain Object destruction happens
// independently of Lua's references.
int Lobject_special_gc(lua_State* L);

void LCpush_lut(lua_State *L, const char* metatable, Object* ut);
Object* LCcheck_object(lua_State *L, int pos);

template<typename T>
inline void LCpush(lua_State* L, T value) {
  if(value == NULL) {
    lua_pushnil(L);
    return;
  }

  // assume that it's an object, errors here mean that a new template
  // specialization is required for the type since this assumption is
  // invalid.
  const TypeInfo* info = value->typeinfo();
  LCpush_lut(L, info->metatable(), value);
}

template<typename T>
inline void LCcheck(lua_State* L, T* target, int pos) {
  if(lua_isnil(L, pos)) {
    *target = NULL;
    return;
  }

  // we assume that it is an object if it has a metatable. Otherwise,
  // it's probably primitive or lightuserdata and we don't know how to
  // handle that here. Those cases mean that a new template
  // specialization is required.
  if(!lua_isuserdata(L, pos)) {
    luaL_error(L, "object at %d is not userdata. need specialized check handler", pos);
  }

  if(!lua_getmetatable(L, pos)) {
    luaL_error(L, "object at %d does not have a metatable. need specialized check handler",
               pos);
  }
  lua_pop(L, 1);

  // assume object
  Object* obj = LCcheck_object(L, pos);

  // verify this is a type of what we're looking for via static member
  // so this deref is safe
  TypeInfo* tgt_type = &((*target)->Type);
  if(obj->typeinfo()->isInstanceOf(tgt_type)) {
    *target = (T)obj;
  } else {
    luaL_error(L, "%s is not a kind of %s", obj->typeinfo()->name(), tgt_type->name());
  }
}

// specialize the lua getters and setters for the primitives here
// since we'll always need those.
template<>
inline void LCpush<int>(lua_State* L, int val) {
  lua_pushinteger(L, val);
}

template<>
inline void LCcheck(lua_State* L, int* target, int pos) {
  *target = luaL_checkinteger(L, pos);
}

template<>
inline void LCpush<long>(lua_State* L, long val) {
  lua_pushinteger(L, val);
}

template<>
inline void LCcheck<long>(lua_State* L, long* target, int pos) {
  *target = luaL_checkinteger(L, pos);
}

template<>
inline void LCpush<float>(lua_State* L, float val) {
  lua_pushnumber(L, val);
}

template<>
inline void LCcheck<float>(lua_State* L, float* target, int pos) {
  *target = luaL_checknumber(L, pos);
}

template<>
inline void LCpush<const char*>(lua_State* L, const char* str) {
  lua_pushstring(L, str);
}

template<>
inline void LCcheck<const char*>(lua_State* L, const char** str, int pos) {
  *str = luaL_checkstring(L, pos);
}

#endif
