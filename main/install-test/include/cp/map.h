#pragma once

#include <functional>
#include <map>
#include <memory_resource>
#include <optional>
#include <unordered_map>

namespace cp {

template <class Key, class Value, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, Value>>>
using map = std::map<Key, Value, Compare, Allocator>;

template <class Key, class Value, class Compare = std::less<Key>,
          class Allocator = std::allocator<std::pair<const Key, Value>>>
using multimap = std::multimap<Key, Value, Compare, Allocator>;

template <class Key, class Value, class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, Value>>>
using hash_map = std::unordered_map<Key, Value, Hash, Equal, Allocator>;

template <class Key, class Value>
using pmr_map = std::pmr::map<Key, Value>;

template <class Associative, class Key>
constexpr auto find_value(Associative &values, const Key &key)
    -> typename Associative::mapped_type * {
  const auto found = values.find(key);
  return found == values.end() ? nullptr : &found->second;
}

template <class Associative, class Key>
constexpr auto find_value(const Associative &values, const Key &key)
    -> const typename Associative::mapped_type * {
  const auto found = values.find(key);
  return found == values.end() ? nullptr : &found->second;
}

template <class Associative, class Key>
constexpr auto value_copy(const Associative &values, const Key &key)
    -> std::optional<typename Associative::mapped_type> {
  if (const auto *value = find_value(values, key))
    return *value;
  return std::nullopt;
}

} // namespace cp
