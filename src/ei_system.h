#pragma once

#include <ei_types.h>

class EiSystem {
public:
  bool bootStrap();
  bool setup();
  bool startup();
  void loop();
private:
};

extern EiSystem eiSystem;
