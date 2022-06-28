//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_CALLBACK
#define ZAGROS_CALLBACK

#include <string>

class Callback {
 public:
  virtual ~Callback() {  }
  virtual void run() {  }
  virtual std::string description() { return "Default callback"; }
};

class Caller {
 private:
  Callback *_callback;
 public:
  Caller() : _callback(0) {}
  ~Caller() { delCallback(); }
  void delCallback() {
    delete _callback;
    _callback = 0;
  }
  void setCallback(Callback *cb) {
    delCallback();
    _callback = cb;
  }
  void call() { if (_callback) _callback->run(); }
};
#endif //ZAGROS_CALLBACK
