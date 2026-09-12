#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace cp::db {

using value = std::variant<std::nullptr_t, std::int64_t, double, bool, std::string>;
using row = std::map<std::string, value, std::less<>>;

struct result {
  std::vector<row> rows;
  std::size_t affected = 0;
  std::int64_t inserted_id = 0;
};

class error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class statement {
public:
  virtual ~statement() = default;
  virtual statement &bind(std::size_t index, value parameter) = 0;
  virtual result execute() = 0;
};

class transaction {
public:
  virtual ~transaction() = default;
  virtual void commit() = 0;
  virtual void rollback() = 0;
};

class connection {
public:
  virtual ~connection() = default;
  virtual std::unique_ptr<statement> prepare(std::string_view sql) = 0;
  virtual result execute(std::string_view sql) = 0;
  virtual std::unique_ptr<transaction> begin() = 0;
  virtual bool healthy() const noexcept = 0;
};

template <class T> const T &get(const row &record, std::string_view column) {
  auto found = record.find(column);
  if (found == record.end())
    throw error("database column not found: " + std::string(column));
  auto field = std::get_if<T>(&found->second);
  if (!field)
    throw error("database column has unexpected type: " + std::string(column));
  return *field;
}

template <class T>
std::optional<T> get_optional(const row &record, std::string_view column) {
  auto found = record.find(column);
  if (found == record.end() || std::holds_alternative<std::nullptr_t>(found->second))
    return std::nullopt;
  if (auto field = std::get_if<T>(&found->second))
    return *field;
  throw error("database column has unexpected type: " + std::string(column));
}

// A small deterministic store for tests, tools, and embedded programs. Database
// drivers can implement connection for SQLite, PostgreSQL, or another engine.
class memory_table {
  std::vector<row> rows_;

public:
  void insert(row record) { rows_.push_back(std::move(record)); }
  std::size_t size() const noexcept { return rows_.size(); }
  bool empty() const noexcept { return rows_.empty(); }
  const std::vector<row> &rows() const noexcept { return rows_; }

  std::vector<row> where(std::string_view column, const value &expected) const {
    std::vector<row> matches;
    for (const auto &record : rows_) {
      auto found = record.find(column);
      if (found != record.end() && found->second == expected)
        matches.push_back(record);
    }
    return matches;
  }

  std::size_t erase_where(std::string_view column, const value &expected) {
    const auto old_size = rows_.size();
    for (auto iterator = rows_.begin(); iterator != rows_.end();) {
      auto found = iterator->find(column);
      if (found != iterator->end() && found->second == expected)
        iterator = rows_.erase(iterator);
      else
        ++iterator;
    }
    return old_size - rows_.size();
  }
};

} // namespace cp::db
