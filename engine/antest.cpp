#include "cpp.h"

#define LLC($, X) NOT(ISEMPTY(TAIL(X)))
#define LLU($, X) CONS(INC(HEAD(X)), TAIL(TAIL(X)))
#define LLF($, X)

#define GENLUAVARS2(TYPE, OFFSET)                               \
  TYPE JOIN(var, OFFSET);                                       \
  LCcheck(L, &JOIN(var, OFFSET), pos + OFFSET);

#define GENLUAVARS1($, X) GENLUAVARS2(HEAD(TAIL(X)), INC(HEAD(X)))

#define GENLUAVARS(X) IF(NOT(ISEMPTY(X)), JOIN(RECR_D, 0)(1, LLC, GENLUAVARS1, LLU, LLF, CONS(0, X)))

#define RETURNS(TYPE, ARGS, METHOD)             \
  TYPE r = obj->METHOD(ARGS);                   \
  LCpush(L, r);                                 \
  return 1;

#define NORETURN(TYPE, ARGS, METHOD)            \
  obj->METHOD(ARGS);                            \
  return 0;

#define LOP(name, method_name) L ## name ## method_name
#define LOPC(name, method_name) LCLASS ## name ## method_name

#define OBJECT_METHOD(CLASS, METHOD, RETURNER, RTYPE, ARGS)             \
  class LOPC(CLASS, METHOD) : public MethodInfo {                       \
  public:                                                               \
                                                                        \
  LOPC(CLASS, METHOD)()                                                 \
    : MethodInfo(&CLASS::Type, STRINGIFY(METHOD)) {                     \
    }                                                                   \
                                                                        \
    virtual int LCinvoke(lua_State* L, int pos) const;                  \
  };                                                                    \
  static LOPC(CLASS, METHOD) LOP(CLASS, METHOD);                        \
                                                                        \
  int LOPC(CLASS,METHOD)::LCinvoke(lua_State* L, int pos) const {       \
    CLASS* obj;                                                         \
    LCcheck(L, &obj, pos);                                              \
    GENLUAVARS(ARGS);                                                   \
    RETURNER(RTYPE, METHOD);                                            \
  }

OBJECT_METHOD(TestClass, method1, RETURNS, float, (int));

OBJECT_METHOD(GO, apply_impulse, NORETURN, void, (Vector));

#define VARNAME(item, pos) JOIN(var, pos)
MAP(VARNAME, (float, int, int))
