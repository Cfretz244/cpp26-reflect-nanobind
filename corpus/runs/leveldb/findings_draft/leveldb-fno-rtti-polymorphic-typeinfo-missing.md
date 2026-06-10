dedup_key: library-fno-rtti-polymorphic-class-typeinfo-missing
layer: LIB
smallest_trigger: nb::reflect_<^^leveldb::DB>(m) where leveldb is built -fno-rtti

# Binding a polymorphic class from a `-fno-rtti` library fails to link (missing typeinfo)

## Summary

`nb::reflect_<^^leveldb::DB>(m)` compiles cleanly but fails at **link** with:

```
Undefined symbols for architecture arm64:
  "typeinfo for leveldb::DB", referenced from:
      nanobind::detail::reflect_class<leveldb::DB, ...> in leveldb_ext.o
      nanobind::detail::reflect_method_binder<leveldb::DB, ...Put...> in leveldb_ext.o
      ... (every method binder + the class registration)
ld: symbol(s) not found for architecture arm64
```

## Root cause (library build, not the binder)

LevelDB's own `CMakeLists.txt` (lines 61-76 of `corpus/libs/leveldb/CMakeLists.txt`) forces
RTTI **off**:

```cmake
# Disable RTTI.
string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
```

With `-fno-rtti`, the compiler emits the **vtable** for each polymorphic class but NOT the
`std::type_info` object. Confirmed against the prebuilt merged archive:

```
$ nm -m build/leveldb-install/lib/libleveldb_merged.a | grep '2DBE'
0000000000007670 (__DATA,__const) external __ZTVN7leveldb2DBE     # vtable present
# (no __ZTIN7leveldb2DBE, no __ZTSN7leveldb2DBE)                  # typeinfo absent
```

Same for `leveldb::Iterator` and `leveldb::Snapshot` (all the polymorphic handles).

The nanobind binding module is compiled WITH RTTI (the reflect path's default), and nanobind's
type registry is fundamentally RTTI-based: registering a class and every method binder over it
takes `typeid(leveldb::DB)`, which for a *polymorphic* type must resolve to the library's
weak-external `__ZTIN7leveldb2DBE`. Because the library never emitted it, the reference is
unresolved.

## Why this is not (only) a binder bug, and what would help

- It is primarily a **library-build property**: any `-fno-rtti` C++ library exposing
  polymorphic types this way is unbindable head-on by an RTTI-based binder. This will recur for
  every corpus library built `-fno-rtti` (protobuf-lite, many Google C++ libs default this way).
- Non-polymorphic classes are unaffected: `Status`, `Slice`, `Options`, `ReadOptions`,
  `WriteOptions`, `WriteBatch`, `CompressionType` all bind + link cleanly, because the module
  synthesizes their (weak, local) typeinfo itself — there is no library vtable forcing the
  typeinfo to live in the library TU.
- Possible binder-side mitigations to consider (not attempted here, per the no-binder-edits
  rule): (a) detect at bind time that a class is polymorphic-but-RTTI-absent and emit a clear
  static_assert / diagnostic instead of a raw linker error; (b) document that `-fno-rtti`
  libraries must be rebuilt with RTTI for head-on polymorphic binding.

## Repro (smallest)

```cpp
#include <nanobind/nb_reflect.h>
#include "leveldb/db.h"
namespace nb = nanobind;
NB_MODULE(x, m) { nb::reflect_<^^leveldb::DB>(m); }   // links: "typeinfo for leveldb::DB" missing
```

Compiles; fails only at link. Dropping `^^leveldb::DB` (binding only the non-polymorphic
Status/Slice/Options/WriteBatch surface) links cleanly — confirming the polymorphic typeinfo is
the sole blocker.

## Workaround used in this run

`leveldb::DB` is not bound head-on. The leveldbtest fixture owns the real `leveldb::DB*` behind
a thin, non-polymorphic `KVStore` handle that forwards to the genuine DB::Put/Get/Delete/Write
and re-open. The store's real behavior is still driven and differentially checked; only the
DB *handle type* could not be presented to Python directly. Status / Slice / WriteBatch remain
bound head-on and are used throughout the differential.
