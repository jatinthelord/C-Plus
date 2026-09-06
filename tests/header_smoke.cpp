#include <cp/stdlib.h>
#include <color.h>
#include <database.h>
#include <safety.h>
#include <csxStudio.h>
#include <cp/hardware.h>
#include <array.h>
#include <abi.h>
#include <buge.h>
#include <list.h>
#include <map.h>
#include <memory.h>
#include <new.h>
#include <set.h>

int main() {
  struct CSP_ALIGN(64) cache_line { int value; };
  static_assert(alignof(cache_line) == 64);
  static_assert(cp::abi::c_compatible_layout<cache_line>);
  const auto parsed = cp::site::parse("http://example.com/docs");
  const cp::net::endpoint endpoint = cp::site::endpoint_for(parsed);
  cp::map<int, int> values{{1, 4}};
  cp::set<int> unique{2, 3, 3};
  cp::list<int> sequence{1, 2};
  cp::append(sequence, 3);
  const auto fixed = cp::make_array(2, 4, 8);
  cp::arena storage;
  int *number = storage.create<int>(7);
  csx::studio::project project{"demo"};
  const auto build = csx::studio::command(project);
  cp::db::memory_table users;
  users.insert({{"id", std::int64_t{7}}, {"name", std::string{"CSP"}}});
  const auto matches = users.where("id", std::int64_t{7});
  int guarded = 3;
  sg::not_null<int *> safe_pointer(&guarded);
  int raw_buffer[] = {4, 8, 15};
  int *aligned_view = cp::hardware::assume_aligned(raw_buffer, alignof(int));
  bg::buffer checked_buffer(raw_buffer);
  const auto colored = cp::color::decorate("ready", cp::color::basic::green);
  auto nightmare = bugemoan::enter_nightmare();
  auto erased = bugemoan::void_pointer::allocate(nightmare, sizeof(int), alignof(int));
  int *nightmare_value = bugemoan::construct<int>(nightmare, erased, 99);
  return parsed.host == "example.com" && endpoint.port == 80 &&
                 *cp::find_value(values, 1) == 4 && unique.size() == 2 &&
                 sequence.size() == 3 && fixed[2] == 8 && *number == 7 &&
                 !build.empty() && cp::db::get<std::string>(matches[0], "name") == "CSP" &&
                 *safe_pointer == 3 && aligned_view[0] == 4 &&
                 checked_buffer.at(2) == 15 && !colored.empty() &&
                 *nightmare_value == 99
             ? 0
             : 1;
}
