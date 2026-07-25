#pragma once

#include <ei_types.h>

class EiSystem {
public:
  bool setup();
  bool startup();
private:
  void loop();
};

extern EiSystem eiSystem;
