// serialize_poc.cpp — an automatic serializer driven entirely by C++26 static
// reflection (WG21 P2996), no per-type boilerplate and no macros.
//
// Build with the repo-local toolchain (from the umbrella repo root):
//   TC=$PWD/toolchain
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" \
//     -nostdinc++ -isystem $TC/include/c++/v1 \
//     -L $TC/lib -Wl,-rpath,$TC/lib serialize_poc.cpp -o serialize_poc
//
// Scope (POC): aggregates of arithmetic/bool fields, nested arbitrarily. Both
// the writer and the reader are generated from the SAME reflection over each
// type's nonstatic data members, so a binary round-trip is correct by
// construction — there is no second, hand-maintained schema to drift out of
// sync. A human-readable JSON writer is included to make the output legible.

#if __has_include(<meta>)
#include <meta>
#else
#include <experimental/meta>
#endif
#include <type_traits>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <print>

namespace serde {
using namespace std::meta;

// The set of fields to walk for a type T: its nonstatic data members. Lifted
// into a static array so a `template for` can expand over it. Every algorithm
// below shares this one definition of "what are the fields of T".
template <typename T>
consteval auto fields_of() {
  return define_static_array(
      nonstatic_data_members_of(^^T, access_context::current()));
}

// A field is "scalar" (memcpy-able leaf) if arithmetic; otherwise we recurse
// into it as a nested aggregate.
template <typename T>
constexpr bool is_scalar = std::is_arithmetic_v<T>;

// ---------------------------------------------------------------------------
// JSON writer (human-readable). Recurses structurally over the reflection.
// ---------------------------------------------------------------------------
template <typename T> void to_json(std::string& out, const T& obj);

template <typename T>
void json_value(std::string& out, const T& v) {
  if constexpr (std::is_same_v<T, bool>) {
    out += v ? "true" : "false";
  } else if constexpr (std::is_arithmetic_v<T>) {
    out += std::to_string(v);
  } else if constexpr (std::is_class_v<T>) {
    to_json(out, v);
  } else {
    static_assert(false, "serde: unsupported field type");
  }
}

template <typename T>
void to_json(std::string& out, const T& obj) {
  out += '{';
  bool first = true;
  template for (constexpr auto member : fields_of<T>()) {
    if (!first) out += ',';
    first = false;
    out += '"';
    out += identifier_of(member);
    out += "\":";
    json_value(out, obj.[:member:]);
  }
  out += '}';
}

template <typename T>
std::string to_json(const T& obj) {
  std::string out;
  to_json(out, obj);
  return out;
}

// ---------------------------------------------------------------------------
// Binary writer / reader. Both expansions are generated from `fields_of<T>()`,
// so they visit fields in the same order with the same types — the round-trip
// cannot disagree about the layout.
// ---------------------------------------------------------------------------
template <typename T>
void write(std::vector<std::uint8_t>& buf, const T& obj) {
  template for (constexpr auto member : fields_of<T>()) {
    const auto& field = obj.[:member:];
    using F = std::remove_cvref_t<decltype(field)>;
    if constexpr (is_scalar<F>) {
      const auto* p = reinterpret_cast<const std::uint8_t*>(&field);
      buf.insert(buf.end(), p, p + sizeof(F));
    } else {
      write(buf, field);            // recurse into nested aggregate
    }
  }
}

template <typename T>
void read(const std::uint8_t*& cur, const std::uint8_t* end, T& obj) {
  template for (constexpr auto member : fields_of<T>()) {
    auto& field = obj.[:member:];
    using F = std::remove_cvref_t<decltype(field)>;
    if constexpr (is_scalar<F>) {
      std::memcpy(&field, cur, sizeof(F));
      cur += sizeof(F);
    } else {
      read(cur, end, field);        // recurse into nested aggregate
    }
  }
}

template <typename T>
std::vector<std::uint8_t> to_bytes(const T& obj) {
  std::vector<std::uint8_t> buf;
  write(buf, obj);
  return buf;
}

template <typename T>
T from_bytes(const std::vector<std::uint8_t>& buf) {
  T obj{};
  const std::uint8_t* cur = buf.data();
  read(cur, buf.data() + buf.size(), obj);
  return obj;
}

// Structural equality, also reflection-driven — handy for verifying round-trips.
template <typename T>
bool equal(const T& a, const T& b) {
  bool eq = true;
  template for (constexpr auto member : fields_of<T>()) {
    const auto& fa = a.[:member:];
    const auto& fb = b.[:member:];
    using F = std::remove_cvref_t<decltype(fa)>;
    if constexpr (is_scalar<F>) {
      eq = eq && (fa == fb);
    } else {
      eq = eq && equal(fa, fb);
    }
  }
  return eq;
}
} // namespace serde

// ---------------------------------------------------------------------------
// Demo: define a couple of plain structs and serialize them with zero
// per-type code. Nothing below "registers" a type with the serializer.
// ---------------------------------------------------------------------------
struct Vec3   { double x; double y; double z; };
struct Entity { std::int32_t id; bool alive; Vec3 position; float health; };

int main() {
  Entity e{1337, true, Vec3{1.5, -2.0, 0.25}, 87.5f};

  std::println("JSON:  {}", serde::to_json(e));

  auto bytes = serde::to_bytes(e);
  std::println("Bytes: {} ({} bytes)", "binary blob", bytes.size());

  Entity back = serde::from_bytes<Entity>(bytes);
  std::println("Round-trip JSON: {}", serde::to_json(back));
  std::println("Round-trip equal: {}", serde::equal(e, back) ? "yes" : "no");
}
