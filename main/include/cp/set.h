#pragma once

#include <functional>
#include <memory_resource>
#include <set>
#include <unordered_set>

namespace cp {

template <class Key, class Compare = std::less<Key>,
          class Allocator = std::allocator<Key>>
using set = std::set<Key, Compare, Allocator>;

template <class Key, class Compare = std::less<Key>,
          class Allocator = std::allocator<Key>>
using multiset = std::multiset<Key, Compare, Allocator>;

template <class Key, class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>, class Allocator = std::allocator<Key>>
using hash_set = std::unordered_set<Key, Hash, Equal, Allocator>;

template <class Key>
using pmr_set = std::pmr::set<Key>;

template <class Set, class Value>
constexpr bool contains(const Set &values, const Value &value) {
  return values.find(value) != values.end();
}

template <class Set>
Set set_union(const Set &left, const Set &right) {
  Set result = left;
  result.insert(right.begin(), right.end());
  return result;
}

template <class Set>
Set set_intersection(const Set &left, const Set &right) {
  Set result;
  for (const auto &value : left)
    if (contains(right, value))
      result.insert(value);
  return result;
}

} // namespace cp
