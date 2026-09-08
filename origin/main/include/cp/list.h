#pragma once

#include <forward_list>
#include <list>
#include <memory_resource>
#include <utility>

namespace cp {

template <class T, class Allocator = std::allocator<T>>
using list = std::list<T, Allocator>;

template <class T, class Allocator = std::allocator<T>>
using forward_list = std::forward_list<T, Allocator>;

template <class T>
using pmr_list = std::pmr::list<T>;

template <class List, class... Arguments>
constexpr auto append(List &values, Arguments &&...arguments)
    -> typename List::reference {
  return values.emplace_back(std::forward<Arguments>(arguments)...);
}

template <class List, class... Arguments>
constexpr auto prepend(List &values, Arguments &&...arguments)
    -> typename List::reference {
  return values.emplace_front(std::forward<Arguments>(arguments)...);
}

template <class List, class Predicate>
constexpr std::size_t erase_if(List &values, Predicate predicate) {
  const auto before = values.size();
  values.remove_if(std::move(predicate));
  return before - values.size();
}

} // namespace cp
