// Fixture for the yaml-cpp run. Supplies ONLY what the real YAML::Node API cannot
// express to Python through reflection:
//
//   * scalar coercion -- YAML::Node::as<T>() is a MEMBER TEMPLATE; the binder's
//     all-defaulted-member-template rule cannot instantiate it (T is a free,
//     non-defaulted type parameter), so there is no reflected entry point for the
//     library's central typed accessor. We forward over node.as<int/double/bool/
//     std::string>() one concrete instantiation each.
//
//   * keyed / indexed access -- YAML::Node::operator[](const Key&) is likewise a
//     member template (and the non-template operator[](const Node&) overload is a
//     spliced-signature shape the binder skips); we forward over the string-key and
//     integer-index instantiations so the differential can walk a parsed map/seq.
//
//   * a fallback-coercion observer -- as<T>(fallback) (the two-parameter member
//     template) so the "bad conversion returns the fallback" behavior is observable.
//
// Everything else (Type/IsNull/IsScalar/IsSequence/IsMap/IsDefined/size/Scalar/Tag/
// SetTag/Style/SetStyle/reset, the operator== free function, Clone, Load, Dump, the
// enums, the exception types) is bound head-on off the real library declarations.
#pragma once
#include "yaml-cpp/yaml.h"
#include <string>

namespace yamlfix {

// --- scalar coercion forwarders (one concrete `as<T>` instantiation each) ---
inline int          as_int(const YAML::Node& n)    { return n.as<int>(); }
inline double       as_double(const YAML::Node& n) { return n.as<double>(); }
inline bool         as_bool(const YAML::Node& n)   { return n.as<bool>(); }
inline std::string  as_str(const YAML::Node& n)    { return n.as<std::string>(); }

// fallback overload (two-parameter member template) -- observes BadConversion-swallowing
inline int          as_int_or(const YAML::Node& n, int fb)    { return n.as<int>(fb); }
inline std::string  as_str_or(const YAML::Node& n, const std::string& fb) { return n.as<std::string>(fb); }

// --- parse entry point ---
// YAML::Load is an overload SET (const std::string& / const char* / std::istream&);
// passing ^^YAML::Load directly to reflect_ is an ambiguous reflection. Forward over the
// single std::string overload so Python has one unambiguous parse front. (Dump/Clone are
// not overloaded and ARE bound head-on off the real declarations.)
inline YAML::Node load(const std::string& input) { return YAML::Load(input); }

// --- keyed / indexed access forwarders (member-template operator[]) ---
inline YAML::Node   get_key(const YAML::Node& n, const std::string& k) { return n[k]; }
inline YAML::Node   get_idx(const YAML::Node& n, int i)                { return n[i]; }
inline bool         has_key(const YAML::Node& n, const std::string& k) { return static_cast<bool>(n[k]); }

}  // namespace yamlfix
