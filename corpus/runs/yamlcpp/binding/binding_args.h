// The reflect_ pack for the yaml-cpp run, defined ONCE for every backend consumer
// (binding.cpp's NB_MODULE and gen_emit.cpp's generator). P2996-only.
//
// yaml-cpp's central type YAML::Node is bound head-on (value semantics): Type()->
// NodeType enum, IsNull/IsScalar/IsSequence/IsMap/IsDefined, size(), Scalar(),
// Tag()/SetTag(), Style()/SetStyle(), reset(), is(). YAML::Mark (the source-location
// struct) and the free functions Dump/Clone bind head-on; the yamlfix fixture forwards
// the member-template-only accessors (as<T>/operator[]) and the overloaded Load.
//
// BINDER-0022 collision run: YAML::NodeType::value and YAML::EmitterStyle::value share
// the unqualified Python name "value" (both are `enum value` inside a sibling namespace).
// BOTH are reflected: the first registered (NodeType::value, the differential's primary
// enum via Node::Type()) keeps the bare module attribute "value"; the second
// (EmitterStyle::value, Node::Style()'s return) binds parent-qualified as
// "EmitterStyle_value" via the binder's runtime hasattr fallback (parent_qualified_name)
// instead of silently clobbering the first. The same fallback renders identically in the
// emit lane, so both modules expose the identical pair of names.
//
// exclude_ makes the node-internals + stream/emitter/iterator fronts opaque on every path
// (BINDER-0014): the detail:: pImpl/handle types, the iterator_value proxy, YAML::Emitter,
// and std::ostream/istream (the operator<< / Load(istream) fronts -- no caster; Dump()
// covers serialization head-on).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                    \
    ^^YAML::Node, ^^YAML::Mark, ^^YAML::NodeType::value,                       \
    ^^YAML::EmitterStyle::value, ^^YAML::Dump, ^^YAML::Clone, ^^yamlfix,       \
    ^^mirrorbind::exclude_<^^YAML::detail::node, ^^YAML::detail::node_data,      \
                         ^^YAML::detail::iterator_value,                       \
                         ^^YAML::detail::shared_memory_holder,                 \
                         ^^YAML::Emitter, ^^std::basic_ostream,                \
                         ^^std::basic_istream>
