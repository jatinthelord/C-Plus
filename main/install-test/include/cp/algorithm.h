#pragma once
#include <algorithm>
#include <numeric>
#include <ranges>

namespace cp {
using std::ranges::all_of;
using std::ranges::any_of;
using std::ranges::copy;
using std::ranges::count;
using std::ranges::find;
using std::ranges::for_each;
using std::ranges::none_of;
using std::ranges::sort;

template <std::ranges::input_range Range> auto sum(const Range &values) {
  using Value = std::ranges::range_value_t<Range>;
  return std::accumulate(std::ranges::begin(values), std::ranges::end(values),
                         Value{});
}
} // namespace cp
