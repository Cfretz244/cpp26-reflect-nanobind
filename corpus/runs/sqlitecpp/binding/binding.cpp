// Single-stage bindings for SQLiteCpp 3.3.3 (Tier 4: RAII wrapper classes + move-only types +
// shared-statement Column handles + a real exception). Bound head-on, no wrappers around the
// library's own behavior:
//   SQLite::Database     -- the RAII connection (exec/tableExists/execAndGet/getColumnCount...)
//   SQLite::Statement    -- the prepared statement (bind overloads/executeStep/getColumn...)
//   SQLite::Column       -- the result cell (getInt/getDouble/getText/getName/getType...)
//   SQLite::Transaction  -- commit/rollback with RAII rollback-on-destruct
//   SQLite::Exception    -- derives std::runtime_error; surfaces on the error path
// plus the TransactionBehavior scoped enum and the sqlfix namespace (OPEN_*/type-code int
// constants as free functions).
//
// The bind set + exclude_ pack live in binding_args.h (shared with the emit generator).
#include <mirrorbind/reflect.h>
#include "binding_args.h"  // bind set defined once (shared with the emit generator)
#include <nanobind/stl/string.h>

namespace nb = nanobind;
namespace mb = mirrorbind;

NB_MODULE(sqlitecpp_ext, m) {
    mb::reflect_<CORPUS_REFLECT_ARGS>(m);
}
