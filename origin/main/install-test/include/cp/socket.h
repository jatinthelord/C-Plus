#pragma once

#include <cstdint>
#include <string_view>

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace cp::net {

struct endpoint {
  std::string_view host;
  std::uint16_t port = 0;
};

inline bool valid(endpoint value) noexcept {
  return !value.host.empty() && value.port != 0;
}

inline int close_socket(int handle) noexcept {
#if defined(_WIN32)
  return ::closesocket(static_cast<SOCKET>(handle));
#else
  return ::close(handle);
#endif
}

} // namespace cp::net
