// Single-stage bindings for leveldb 1.23 (Tier 4: a real embedded key/value store).
//
// Bound head-on off leveldb's own classes:
//   leveldb::Status       -- the result type (ok/IsNotFound/IsCorruption/.../ToString).
//   leveldb::Slice        -- the key/value view (data/size/empty/ToString/compare/starts_with
//                            + ==/!= dunders), constructible from a Python str via std::string.
//   leveldb::Options      -- the open-time option struct (create_if_missing/error_if_exists/...).
//   leveldb::ReadOptions  -- read-op options (verify_checksums/fill_cache).
//   leveldb::WriteOptions -- write-op options (sync).
//   leveldb::WriteBatch   -- the atomic-update buffer (Put/Delete/Clear/Append/ApproximateSize).
//   leveldb::CompressionType -- the block-compression enum (kNoCompression/kSnappyCompression).
//
// leveldb::DB itself cannot be bound head-on: LevelDB is built -fno-rtti (its CMakeLists.txt
// lines 61-76), so the archive emits DB's vtable but NOT its std::type_info, and nanobind's
// RTTI-based registry then fails to link ("typeinfo for leveldb::DB"). See
// findings_draft/leveldb-fno-rtti-polymorphic-typeinfo-missing.md. The DB is therefore reached
// through leveldbtest::KVStore -- a non-polymorphic owning handle whose every method forwards
// verbatim to the genuine DB::Put/Get/Delete/Write, and which also bridges DB::Open's DB**
// and DB::Get's std::string* out-parameters (un-drivable from Python). No behavior is wrapped.
//
// The exclude_ pack makes the unbound surface opaque on every path (BINDER-0014): the
// option structs' forward-declared collaborator pointers (Comparator/Env/Cache/FilterPolicy/
// Logger), the polymorphic Snapshot handle (ReadOptions::snapshot -- itself RTTI-absent),
// and WriteBatch::Handler (a pure-virtual visitor needing a Python override we do not surface).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>

#include "leveldbtest.h"

namespace nb = nanobind;

NB_MODULE(leveldb_ext, m) {
    nb::reflect_<^^leveldb::Status,
                 ^^leveldb::Slice,
                 ^^leveldb::Options,
                 ^^leveldb::ReadOptions,
                 ^^leveldb::WriteOptions,
                 ^^leveldb::WriteBatch,
                 ^^leveldb::CompressionType,
                 ^^leveldbtest,
                 ^^nb::exclude_<^^leveldb::Comparator,
                                ^^leveldb::Env,
                                ^^leveldb::Cache,
                                ^^leveldb::FilterPolicy,
                                ^^leveldb::Logger,
                                ^^leveldb::Snapshot,
                                ^^leveldb::WriteBatch::Handler>>(m);
}
