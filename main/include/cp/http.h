#pragma once

#include <string>
#include <string_view>

#include <cp/socket.h>

namespace cp::http {

struct request {
  std::string method = "GET";
  std::string target = "/";
  std::string host;
  std::string body;
};

struct response {
  int status = 0;
  std::string body;
};

inline std::string request_line(const request &value) {
  return value.method + " " + value.target + " HTTP/1.1\r\nHost: " +
         value.host + "\r\nConnection: close\r\n\r\n";
}

inline bool successful(const response &value) noexcept {
  return value.status >= 200 && value.status < 300;
}

} // namespace cp::http
