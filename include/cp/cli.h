#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace cp {
struct Flag {
  std::string name;
  std::string value;
  bool present = false;
};

inline Flag find_flag(int argc, char **argv, std::string_view name) {
  Flag flag{std::string(name), {}, false};
  const std::string needle = "--" + flag.name;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == needle) {
      flag.present = true;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        flag.value = argv[++i];
      return flag;
    }
    if (arg.rfind(needle + "=", 0) == 0) {
      flag.present = true;
      flag.value = std::string(arg.substr(needle.size() + 1));
      return flag;
    }
  }
  return flag;
}

inline std::vector<std::string> positional(int argc, char **argv) {
  std::vector<std::string> out;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg.rfind("-", 0) == 0)
      continue;
    out.emplace_back(arg);
  }
  return out;
}
} // namespace cp
