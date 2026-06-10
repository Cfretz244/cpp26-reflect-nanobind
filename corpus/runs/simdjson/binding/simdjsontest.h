// Fixture for the simdjson DOM run.
//
// The simdjson DOM API is a value-or-error monad: nearly every navigation/coercion returns
// simdjson_result<T> (a template the binder has no caster for) and the entry points live on a
// MOVE-ONLY parser whose returned element/array/object alias buffers owned by the parser. None
// of that is expressible to Python directly. This fixture is the ONLY thing Python cannot drive
// head-on, and it does exactly two things:
//
//   1. Owns a parser + the parsed buffer so DOM handles stay valid for the module's lifetime
//      (Doc), and exposes the document root as a bound dom::element.
//   2. Unwraps simdjson_result<T> to a value (throwing simdjson::simdjson_error -> a Python
//      exception on error, with the raw error_code reachable for the differential).
//
// The element / array / object / element_type / error_code types themselves are bound HEAD-ON
// in binding.cpp; this fixture returns and accepts those real library types, never wrappers.

#pragma once

#include "simdjson.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// NOTE: the namespace aliases live OUTSIDE simdjsontest on purpose. A namespace alias is a
// member of its enclosing namespace; if `namespace dom = simdjson::dom;` were declared inside
// simdjsontest, reflecting ^^simdjsontest would enumerate that alias and the binder's bind-set
// fixpoint would follow it into the ENTIRE simdjson namespace -- including the ondemand API,
// whose ondemand::field : std::pair<raw_json_string, value> derails the bind (and, with field
// excluded, ICEs the deduction-guide mangler). Declared here, they are members of the global
// namespace (never reflected) and only shorten the spellings below.
namespace sjt_sd = simdjson;
namespace sjt_dom = simdjson::dom;

namespace simdjsontest {

// Short local handles for the bodies. These are NOT namespace aliases (which would be members
// of simdjsontest and leak the whole library to the binder's namespace walk); they are the
// global aliases, referenced fully-qualified below. We keep using `sd::`/`dom::` spellings via
// a function-style shim only where needed -- see the explicit qualifications in each body.

// Raised to Python when a simdjson navigation/coercion fails; carries the raw error_code so the
// differential can assert error paths (e.g. NO_SUCH_FIELD, INCORRECT_TYPE) by value.
struct QueryError : std::runtime_error {
  ::sjt_sd::error_code code;
  explicit QueryError(::sjt_sd::error_code c)
      : std::runtime_error(::sjt_sd::error_message(c)), code(c) {}
};

template <typename T>
static T unwrap(::sjt_sd::simdjson_result<T> r) {
  // simdjson_result_base<T>::get(T&) is rvalue-ref-qualified; move the result to call it.
  T value;
  auto err = std::move(r).get(value);
  if (err != ::sjt_sd::SUCCESS) { throw QueryError(err); }
  return value;
}

// A parsed JSON document. Holds the parser + a padded copy of the source so every element/array/
// object handle it hands out remains valid. dom::parser is move-only and parse() yields a
// simdjson_result<element> aliasing the parser's internal buffers -- exactly what Python cannot
// own; Doc is the lifetime anchor.
class Doc {
public:
  explicit Doc(const std::string &json) : source_(json) {
    parser_ = std::make_unique<::sjt_dom::parser>();
    auto r = parser_->parse(source_);
    error_ = r.error();
    if (error_ == ::sjt_sd::SUCCESS) { root_ = r.value_unsafe(); }
  }

  // Whether the parse succeeded.
  bool ok() const noexcept { return error_ == ::sjt_sd::SUCCESS; }
  // The raw parse error_code (SUCCESS when ok()).
  ::sjt_sd::error_code error() const noexcept { return error_; }
  // The document root element (only meaningful when ok()).
  ::sjt_dom::element root() const { if (!ok()) throw QueryError(error_); return root_; }

private:
  std::string source_;
  std::unique_ptr<::sjt_dom::parser> parser_;
  ::sjt_dom::element root_{};
  ::sjt_sd::error_code error_{::sjt_sd::UNINITIALIZED};
};

// ---- navigation / coercion helpers: real library types in and out, results unwrapped ----

// object key lookup (NO_SUCH_FIELD / INCORRECT_TYPE -> QueryError).
inline ::sjt_dom::element at_key(::sjt_dom::element e, const std::string &key) {
  return unwrap<::sjt_dom::element>(e.at_key(key));
}
// JSON-pointer navigation (RFC 6901).
inline ::sjt_dom::element at_pointer(::sjt_dom::element e, const std::string &ptr) {
  return unwrap<::sjt_dom::element>(e.at_pointer(ptr));
}
// array index (INDEX_OUT_OF_BOUNDS -> QueryError).
inline ::sjt_dom::element at_index(::sjt_dom::element e, size_t i) {
  return unwrap<::sjt_dom::element>(e.at(i));
}

// element -> array / object (INCORRECT_TYPE -> QueryError).
inline ::sjt_dom::array as_array(::sjt_dom::element e) { return unwrap<::sjt_dom::array>(e.get_array()); }
inline ::sjt_dom::object as_object(::sjt_dom::element e) { return unwrap<::sjt_dom::object>(e.get_object()); }

// scalar coercions (INCORRECT_TYPE / NUMBER_OUT_OF_RANGE -> QueryError).
inline int64_t as_int64(::sjt_dom::element e) { return unwrap<int64_t>(e.get_int64()); }
inline uint64_t as_uint64(::sjt_dom::element e) { return unwrap<uint64_t>(e.get_uint64()); }
inline double as_double(::sjt_dom::element e) { return unwrap<double>(e.get_double()); }
inline bool as_bool(::sjt_dom::element e) { return unwrap<bool>(e.get_bool()); }
inline std::string as_string(::sjt_dom::element e) {
  ::sjt_sd::simdjson_result<std::string_view> r = e.get_string();
  std::string_view sv;
  auto err = std::move(r).get(sv);
  if (err != ::sjt_sd::SUCCESS) { throw QueryError(err); }
  return std::string(sv);
}

// array element access by index (no result for size()).
inline ::sjt_dom::element array_at(::sjt_dom::array a, size_t i) {
  return unwrap<::sjt_dom::element>(a.at(i));
}
// object value at key.
inline ::sjt_dom::element object_at(::sjt_dom::object o, const std::string &key) {
  return unwrap<::sjt_dom::element>(o.at_key(key));
}

// Materialize an array's children as a vector of bound elements (Python list).
inline std::vector<::sjt_dom::element> array_values(::sjt_dom::array a) {
  std::vector<::sjt_dom::element> out;
  for (::sjt_dom::element child : a) { out.push_back(child); }
  return out;
}
// Materialize an object's keys in document order (Python list of str).
inline std::vector<std::string> object_keys(::sjt_dom::object o) {
  std::vector<std::string> out;
  // *iterator yields a key_value_pair whose .key is a std::string_view data member.
  for (auto field : o) { out.emplace_back(field.key); }
  return out;
}

} // namespace simdjsontest
