// leveldbtest -- fixtures for the leveldb run. Provides ONLY what leveldb's real API cannot
// express to Python:
//
//   1. leveldb::DB cannot be bound head-on at all. LevelDB's own CMakeLists.txt forces
//      -fno-rtti (lines 61-76), so the prebuilt archive emits DB's vtable but NOT its
//      std::type_info. nanobind's type registry is RTTI-based: binding DB references
//      "typeinfo for leveldb::DB", which the library never defined -> link failure. See
//      findings_draft/leveldb-fno-rtti-polymorphic-typeinfo-missing.md. So DB is reached
//      only through this non-polymorphic KVStore handle, which owns the REAL leveldb::DB*
//      and forwards verbatim to the genuine DB::Put/Get/Delete/Write.
//
//   2. DB::Open returns the handle through a `DB** dbptr` out-parameter, and DB::Get
//      returns the value through a `std::string* value` out-parameter -- neither is
//      drivable from Python. KVStore::get packages the value + Status into a GetResult.
//
// KVStore wraps NO behavior of its own: every method is a one-line forward to the real DB.
// Status / Slice / WriteBatch / the option structs are bound head-on off leveldb's classes
// and used directly in the differential.
#pragma once

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"

#include <memory>
#include <string>

namespace leveldbtest {

// Result of a DB::Get, packaging the std::string* out-parameter value with its Status.
struct GetResult {
    leveldb::Status status;
    std::string value;
    bool found;
};

// A non-polymorphic owning handle around the real leveldb::DB*. Exists only because DB is
// polymorphic-but-RTTI-absent (see header comment) and because DB::Open/DB::Get use C++
// out-parameters. Every method forwards verbatim to the genuine library DB.
class KVStore {
 public:
    KVStore() = default;
    ~KVStore() { delete db_; }
    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;

    // Front for leveldb::DB::Open (DB** out-parameter). Returns the Open Status; on success
    // the owned DB* is held inside this handle.
    leveldb::Status open(const leveldb::Options& options, const std::string& name) {
        delete db_;
        db_ = nullptr;
        return leveldb::DB::Open(options, name, &db_);
    }

    bool is_open() const { return db_ != nullptr; }

    // Verbatim forward to the real DB::Put. Takes std::string (not Slice) for key/value:
    // leveldb::Slice is a NON-OWNING view, so a Slice built from a transient Python str would
    // dangle by call time (its backing std::string temporary is already freed). The
    // std::string caster keeps the bytes alive for the call; the real Put then copies them.
    leveldb::Status put(const leveldb::WriteOptions& options, const std::string& key,
                        const std::string& value) {
        return db_->Put(options, leveldb::Slice(key), leveldb::Slice(value));
    }

    // Verbatim forward to the real DB::Delete. (Named delete_key, not del/delete, to avoid
    // both the C++ `delete` keyword and the Python `del` keyword.)
    leveldb::Status delete_key(const leveldb::WriteOptions& options, const std::string& key) {
        return db_->Delete(options, leveldb::Slice(key));
    }

    // Verbatim forward to the real DB::Write (atomic WriteBatch apply).
    leveldb::Status write(const leveldb::WriteOptions& options, leveldb::WriteBatch& batch) {
        return db_->Write(options, &batch);
    }

    // Front for leveldb::DB::Get (std::string* out-parameter): packages value + Status.
    GetResult get(const leveldb::ReadOptions& options, const std::string& key) {
        GetResult out;
        out.status = db_->Get(options, leveldb::Slice(key), &out.value);
        out.found = out.status.ok();
        return out;
    }

    // Close (free) the DB handle (e.g. before re-opening to test persistence).
    void close() { delete db_; db_ = nullptr; }

 private:
    leveldb::DB* db_ = nullptr;
};

// Slice-from-std::string fronts for WriteBatch::Put/Delete. WriteBatch is bound head-on
// (Clear/Append/ApproximateSize used directly), but its Put/Delete take leveldb::Slice -- a
// non-owning view -- and a Slice built from a transient Python str dangles by the time
// WriteBatch::Put copies it. These thin fronts take std::string (kept alive by the caster)
// and construct a live Slice, so the real WriteBatch::Put/Delete copy valid bytes.
inline void batch_put(leveldb::WriteBatch& batch, const std::string& key,
                      const std::string& value) {
    batch.Put(leveldb::Slice(key), leveldb::Slice(value));
}

inline void batch_delete(leveldb::WriteBatch& batch, const std::string& key) {
    batch.Delete(leveldb::Slice(key));
}

// Observability front for leveldb::Slice's pointer-dereferencing methods. A Slice built from a
// Python str dangles (its backing temporary is freed after the ctor), so ToString/compare/
// starts_with cannot be safely driven on a held Python Slice. This front constructs a live
// Slice from a std::string kept alive by the caster for the call, drives the REAL Slice methods,
// and returns their results so the differential can compare them.
struct SliceInfo {
    std::size_t size;
    std::string str;
    bool starts_with_prefix;
    int compare_self;
    bool eq_self;
    bool empty;
};

inline SliceInfo slice_info(const std::string& s, const std::string& prefix) {
    leveldb::Slice sl(s);                 // live: s outlives this call
    leveldb::Slice pf(prefix);
    SliceInfo out;
    out.size = sl.size();
    out.str = sl.ToString();
    out.starts_with_prefix = sl.starts_with(pf);
    out.compare_self = sl.compare(leveldb::Slice(s));
    out.eq_self = (leveldb::Slice(s) == leveldb::Slice(s));
    out.empty = sl.empty();
    return out;
}

// Cleanup front for leveldb::DestroyDB (used to reset on-disk state between scenarios).
inline leveldb::Status destroy_db(const std::string& name, const leveldb::Options& options) {
    return leveldb::DestroyDB(name, options);
}

}  // namespace leveldbtest
