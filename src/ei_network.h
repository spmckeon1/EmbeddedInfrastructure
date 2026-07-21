#pragma once
//
//  network.hpp
//  
//
//  Created by Stephen McKeon on 7/18/26.
//

#include <ei_types.h>

struct NetworkCredentials {
    bool dirty = false;
    String ssid;
    String password;
};


class Network {
public:
  bool startup();

private:
  NetworkCredentials _credentials;
  bool connect(String& ssid, String& pwd, int from);
  void logInfo(const String& msg);
  void logError(const String& msg);

};

extern Network network;
