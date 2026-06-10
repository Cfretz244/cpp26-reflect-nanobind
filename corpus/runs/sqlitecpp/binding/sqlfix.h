// sqlfix -- a thin fixture for the SQLiteCpp run, shared by binding.cpp and the native oracle.
//
// SQLiteCpp exposes its open-mode flags and column-type codes as namespace-scope
// `extern const int` variables (SQLite::OPEN_READWRITE, SQLite::INTEGER, ...). The reflection
// binder reflects classes/enums/namespaces, not loose namespace-scope `extern const int`
// variables, so these are surfaced here as inline free functions in a reflected namespace
// (free functions in a reflected namespace DO bind). They return the library's own real
// constants verbatim -- no values are invented. This is the ONLY thing the fixture provides;
// every Database/Statement/Column/Transaction/Exception behavior is bound head-on off the
// real classes.
#pragma once

#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Column.h"

namespace sqlfix {

// Open-mode flag ints (SQLite::OPEN_*). The differential opens an in-memory db with
// OPEN_READWRITE | OPEN_CREATE.
inline int open_readonly()  { return SQLite::OPEN_READONLY; }
inline int open_readwrite() { return SQLite::OPEN_READWRITE; }
inline int open_create()    { return SQLite::OPEN_CREATE; }
inline int open_memory()    { return SQLite::OPEN_MEMORY; }

// Column-type codes (SQLite::INTEGER/FLOAT/TEXT/BLOB/Null) returned by Column::getType().
inline int type_integer() { return SQLite::INTEGER; }
inline int type_float()   { return SQLite::FLOAT; }
inline int type_text()    { return SQLite::TEXT; }
inline int type_blob()    { return SQLite::BLOB; }
inline int type_null()    { return SQLite::Null; }

}  // namespace sqlfix
