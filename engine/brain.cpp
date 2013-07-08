#include "brain.h"

OBJECT_IMPL(Brain, Object);
OBJECT_PROPERTY(Brain, params);

Steering Brain::steering(NULL);

Brain::Brain(void* _world) {
}

void Brain::update(GO* go, float dt) {
}

OBJECT_IMPL(CBrain, Component);
OBJECT_ACCESSOR(CBrain, brain, get_brain, set_brain);

CBrain::CBrain(void* _go)
  : Component((GO*)_go, PRIORITY_THINK), brain(NULL) {
}

CBrain::~CBrain() {
  if(brain) brain->release();
}

void CBrain::update(float dt) {
  if(brain) brain->update(go, dt);
}

void CBrain::set_brain(Brain* _brain) {
  if(brain) brain->release();
  brain = _brain;
  brain->retain();
}

Brain* CBrain::get_brain() {
  return brain;
}

#define IMPL_STEERING_BRAIN(method)             \
  OBJECT_IMPL(method, Brain);                   \
  method::method(void* _world)                  \
    : Brain(_world)                             \
  { }                                           \
  void method::update(GO* go, float dt)

IMPL_STEERING_BRAIN(SeekBrain) {
  Vector_ pos, vel;
  go->pos(&pos);
  go->vel(&vel);

  steering.begin(params);
  steering.seek(tgt, pos, vel);
  steering.complete();
  go->apply_force(steering.force);
}
OBJECT_PROPERTY(SeekBrain, tgt);

IMPL_STEERING_BRAIN(VelocityBrain) {
  Vector_ vel;
  go->vel(&vel);

  steering.begin(params);
  steering.apply_desired_velocity(tgt_vel, vel);
  steering.complete();
  go->apply_force(steering.force);
}
OBJECT_PROPERTY(VelocityBrain, tgt_vel);

OBJECT_IMPL(PursuitBrain, Brain);
OBJECT_ACCESSOR(PursuitBrain, tgt, get_tgt, set_tgt);

PursuitBrain::PursuitBrain(void* _world)
  : Brain(_world), tgt(NULL) {
}

void PursuitBrain::update(GO* go, float dt) {
  Vector_ pos, vel, tpos, tvel;
  go->pos(&pos);
  go->vel(&vel);

  if(tgt && tgt->go) {
    tgt->go->pos(&tpos);
    tgt->go->vel(&tvel);
  } else {
    tpos = pos;
    vector_zero(&tvel);
  }

  steering.begin(params);
  steering.pursuit(tpos, tvel, pos, vel);
  steering.complete();
  go->apply_force(steering.force);
}

void PursuitBrain::set_tgt(GOHandle* gh) {
  if(tgt) tgt->release();
  tgt = gh;
  tgt->retain();
}

GOHandle* PursuitBrain::get_tgt() {
  return tgt;
}
