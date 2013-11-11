#include "task.h"

Task::Task() {
}

int Task::internal_runner(void* _task) {
  Task* task = (Task*)_task;
  task->initialize();

  while(!task->stop_requested.has_fired() && task->step());

  task->stop_acknowledged.notify();
  return 1;
}

void Task::initialize() {
  if(!initialized.has_fired()) {
    initializer();
    initialized.notify();
  }
}

void Task::run() {
  stop_requested.reset();
  stop_acknowledged.reset();

  internal_runner(this);
}

void Task::start() {
  stop_requested.reset();
  stop_acknowledged.reset();

  thread_create(&Task::internal_runner, this);
}

void Task::await_initialization() {
  initialized.wait();
}

void Task::request_stop() {
  stop_requested.notify();
}

void Task::await_stop() {
  stop_acknowledged.wait();
}
