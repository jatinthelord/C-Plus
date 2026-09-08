#pragma once

#include <string_view>

#include <cp/http.h>

namespace cp::site {

struct url {
  std::string_view scheme;
  std::string_view host;
  std::string_view path = "/";
};

inline url parse(std::string_view value) noexcept {
  const auto separator = value.find("://");
  if (separator == std::string_view::npos)
    return {"", value, "/"};
  const auto host_start = separator + 3;
  const auto path_start = value.find('/', host_start);
  if (path_start == std::string_view::npos)
    return {value.substr(0, separator), value.substr(host_start), "/"};
  return {value.substr(0, separator), value.substr(host_start, path_start - host_start), value.substr(path_start)};
}

inline cp::net::endpoint endpoint_for(url value, std::uint16_t port = 80) noexcept {
  return {value.host, port};
}

} // namespace cp::site
