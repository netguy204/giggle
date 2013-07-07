#ifndef BRAIN_H
#define BRAIN_H

#include "ooc.h"
#include "gameobject.h"
#include "steering.h"

class Brain : public Object {
 public:
  OBJECT_PROTO(Brain);
  Brain(void *_world);

  virtual void update(GO* go, float dt);
  SteeringParams params;
  static Steering steering;
};

class CBrain : public Component {
 public:
  OBJECT_PROTO(CBrain);

  CBrain(void *_go);
  virtual ~CBrain();

  virtual void update(float dt);

  void set_brain(Brain* brain);
  Brain* get_brain();

  Brain* brain;
};

#define DEF_STEERING_BRAIN(method)              \
  class method : public Brain {                 \
public:                                         \
 OBJECT_PROTO(method);                          \
 method(void *_world);                          \
 virtual void  update(GO* go, float dt);

DEF_STEERING_BRAIN(SeekBrain)
  Vector_ tgt;
};

DEF_STEERING_BRAIN(VelocityBrain)
  Vector_ tgt_vel;
};

DEF_STEERING_BRAIN(PursuitBrain)

  void set_tgt(GOHandle* go);
  GOHandle* get_tgt();

  GOHandle* tgt;
};

#endif
