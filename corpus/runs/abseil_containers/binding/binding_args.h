// The reflect_ pack, defined ONCE for every backend consumer (P2996-only).
// The two explicit specialization listings become explicit with_ tuples on
// one instantiate_ rule -- the corpus's with_ exercise: a mixed type+NTTP
// tuple (val_<size_t> matches InlinedVector's size_t N exactly) with the
// allocator parameter filled by default-argument substitution; a tuple that
// failed to substitute would hard-error (instantiate_with_rule_failed), the
// loud half of the with_/product_ contract. The minted <int,4>/<double,2>
// specs are the identical reflections the old listings named (same Python
// names, same surface -- the unchanged legacy suite is the proof); the
// <std::string,4> spec is new surface (non-trivial element type).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS \
    ^^mirrorbind::instantiate_<^^absl::InlinedVector, \
        mirrorbind::with_<^^int, mirrorbind::val_<std::size_t(4)>>, \
        mirrorbind::with_<^^double, mirrorbind::val_<std::size_t(2)>>, \
        mirrorbind::with_<^^std::string, mirrorbind::val_<std::size_t(4)>>>
