// The reflect_ pack for the sqlitecpp run, defined ONCE for every backend consumer
// (binding.cpp's NB_MODULE and gen_emit.cpp's generator). P2996-only.
//
// SQLiteCpp's RAII wrapper classes are bound head-on: Database (the connection),
// Statement (the prepared statement), Column (the result cell), Transaction (RAII
// commit/rollback), Exception (derives std::runtime_error), plus the TransactionBehavior
// scoped enum and the sqlfix namespace (OPEN_*/type-code int constants as free functions).
//
// exclude_ makes the unbindable surface opaque on every path (BINDER-0014): the raw
// sqlite3 C-API handles (forward-declared incomplete structs), the std::ostream
// operator<< front (no caster), std::filesystem::path (the C++17 path ctor overload --
// reached through the const char*/std::string ctors instead), and two
// findings-driven exclusions:
//   ^^SQLite::Column::getBlob -- returns const void*, the cv-void* unbindable shape
//     (BINDER-0023); excluded as an individual member.
//   ^^SQLite::Header -- returned by getHeaderInfo, which exists as BOTH a const member
//     AND a static overload of the same name (the static-shadow shape, BINDER-0024);
//     excluding the type skips both overloads (also right semantically -- a file-on-disk
//     feature out of scope for the in-memory run).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                   \
    ^^SQLite::Database, ^^SQLite::Statement, ^^SQLite::Column,                \
    ^^SQLite::Transaction, ^^SQLite::Exception, ^^SQLite::TransactionBehavior,\
    ^^sqlfix,                                                                 \
    ^^mirrorbind::exclude_<^^sqlite3, ^^sqlite3_stmt, ^^sqlite3_context,        \
                         ^^sqlite3_value, ^^std::basic_ostream,              \
                         ^^std::filesystem::path, ^^SQLite::Column::getBlob,  \
                         ^^SQLite::Header>
