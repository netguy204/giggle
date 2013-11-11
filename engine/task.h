#ifndef TASK_H
#define TASK_H

#include "ooc.h"
#include "threadlib.h"

class Task {
private:
  ThreadEvent initialized;
  ThreadEvent stop_requested;
  ThreadEvent stop_acknowledged;

  static int internal_runner(void* _task);

protected:
  // override these for each task type
  virtual void initializer() = 0; // initialize, utilizing the local thread

public:
  Task();

  virtual bool step() = 0; // provide one loop of service. return false to terminate

  // these are provided
  virtual void initialize(); // only initialize. notify anyone waiting when complete
  virtual void run(); // initialize (if not already done) and run, claiming the calling thread
  virtual void start(); // initialize (if not already done) and run, but in another thread
  virtual void await_initialization();

  virtual void request_stop();
  virtual void await_stop();
};



#endif
