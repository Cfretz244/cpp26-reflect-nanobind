// Plain C++ includes for the sqlitecpp run (parsed by BOTH compilers: the P2996
// reflection toolchain for the constexpr/generator lanes, and Apple Clang for the
// emit lane's generated TU). No `^^`, no `[[=...]]` annotations.
//
// SQLiteCpp.h pulls SQLite::Database/Statement/Column/Transaction/Exception and the
// open-mode / type-code namespace-scope constants; sqlfix.h surfaces those loose
// constants as inline free functions in a reflected namespace (the binder reflects a
// namespace/class, not loose namespace-scope `extern const int` variables).
#pragma once
#include "SQLiteCpp/SQLiteCpp.h"
#include "sqlfix.h"
