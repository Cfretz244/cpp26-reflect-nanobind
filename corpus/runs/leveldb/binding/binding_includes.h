// Plain C++ includes for the leveldb run (parsed by BOTH compilers: the P2996
// reflection toolchain for the constexpr/generator lanes, and Apple Clang for
// the emit lane's generated TU). No `^^`, no `[[=...]]` annotations.
//
// leveldbtest.h pulls leveldb's own headers (db/options/slice/status/write_batch)
// plus the KVStore forwarding handle and the Slice/WriteBatch std::string fronts.
#pragma once
#include "leveldbtest.h"
