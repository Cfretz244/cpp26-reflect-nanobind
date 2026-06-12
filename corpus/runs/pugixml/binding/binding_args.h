// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The hand-written 15-entry exclude_ list is replaced by an exclude_if_
// predicate: glob families cover the pImpl structs (BINDER-0019's home),
// stream types, iterator/walker internals, and the xpath surface 1:1 (plus
// same-family strays the old list never named -- xml_writer_file/_stream,
// xpath_exception, xpath_parse_result, xpath_variable -- all equally
// unbindable). xml_text left the exclusion set and is bound head-on.
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^pugi::xml_node, \
    ^^pugi::xml_attribute, \
    ^^pugi::xml_document, \
    ^^pugi::xml_text, \
    ^^pugi::xml_parse_result, \
    ^^pugi::xml_node_type, \
    ^^pugi::xml_encoding, \
    ^^pugi::xml_parse_status, \
    ^^pugitest, \
    ^^mirrorbind::exclude_if_<mirrorbind::any_of_< \
        mirrorbind::named_<"xpath_*">, \
        mirrorbind::named_<"*_iterator">, \
        mirrorbind::named_<"*_struct">, \
        mirrorbind::named_<"xml_writer*">, \
        mirrorbind::named_<"xml_object_range">, \
        mirrorbind::named_<"xml_tree_walker">, \
        mirrorbind::named_<"basic_?stream">>>
