// Single-stage bindings for SQLiteCpp 3.3.3 (Tier 4: RAII wrapper classes + move-only types +
// shared-statement Column handles + a real exception). Bound head-on, no wrappers around the
// library's own behavior:
//   SQLite::Database     -- the RAII connection (exec/tableExists/execAndGet/getColumnCount...)
//   SQLite::Statement    -- the prepared statement (bind overloads/executeStep/getColumn...)
//   SQLite::Column       -- the result cell (getInt/getDouble/getText/getName/getType...)
//   SQLite::Transaction  -- commit/rollback with RAII rollback-on-destruct
//   SQLite::Exception    -- derives std::runtime_error; surfaces on the error path
// plus the TransactionBehavior scoped enum.
//
// The sqlfix namespace surfaces ONLY the SQLite::OPEN_* / type-code namespace-scope int
// constants (free functions; the binder reflects classes/namespaces, not loose extern consts).
// The exclude_ pack (also in meta.toml's reflect_args) makes the raw sqlite3 C-API handles,
// the std::ostream operator<< front, and std::filesystem::path opaque so methods mentioning
// them are skipped (BINDER-0014).
#include <nanobind/nb_reflect.h>
#include <nanobind/stl/string.h>

#include "SQLiteCpp/SQLiteCpp.h"
#include "sqlfix.h"

namespace nb = nanobind;

NB_MODULE(sqlitecpp_ext, m) {
    nb::reflect_<^^SQLite::Database,
                 ^^SQLite::Statement,
                 ^^SQLite::Column,
                 ^^SQLite::Transaction,
                 ^^SQLite::Exception,
                 ^^SQLite::TransactionBehavior,
                 ^^sqlfix,
                 ^^nb::exclude_<^^sqlite3, ^^sqlite3_stmt, ^^sqlite3_context,
                                ^^sqlite3_value, ^^std::basic_ostream,
                                ^^std::filesystem::path,
                                // Column::getBlob() returns const void* -- a
                                // pointer-to-void the binder does not gracefully skip
                                // (it reaches nanobind's void* caster as a const-mismatch
                                // hard error; see findings_draft). Excluded as an individual
                                // member so the rest of Column binds head-on.
                                ^^SQLite::Column::getBlob,
                                // SQLite::Header is returned by Database::getHeaderInfo,
                                // which exists as BOTH a const member AND a static overload
                                // of the same name; the binder binds them as a same-named
                                // instance method + staticmethod, which nanobind cannot
                                // represent and ABORTS on at import (see findings_draft).
                                // Excluding the Header type makes both getHeaderInfo
                                // overloads skip (they mention an excluded type), which is
                                // also the right call semantically: getHeaderInfo parses a
                                // file-on-disk header, out of scope for the in-memory run.
                                ^^SQLite::Header>>(m);
}
