// The reflect_ pack for the leveldb run, defined ONCE for every backend consumer
// (binding.cpp's NB_MODULE and gen_emit.cpp's generator). P2996-only.
//
// leveldb's real result/view/batch/option surface is bound head-on: Status, Slice,
// the three option structs, WriteBatch, and the CompressionType enum. leveldb::DB
// cannot be bound head-on (LevelDB is built -fno-rtti, so the archive lacks DB's
// std::type_info -- LIB-0003); it is reached through the non-polymorphic
// leveldbtest::KVStore forwarding handle.
//
// exclude_ makes the unbound surface opaque on every path (BINDER-0014): the option
// structs' forward-declared collaborator pointers (Comparator/Env/Cache/FilterPolicy/
// Logger), the polymorphic Snapshot handle (ReadOptions::snapshot -- itself RTTI-absent),
// and WriteBatch::Handler (a pure-virtual visitor needing a Python override we do not
// surface).
#pragma once
#include <mirrorbind/reflect.h>
#include "binding_includes.h"
#define CORPUS_REFLECT_ARGS                                                       \
    ^^leveldb::Status, ^^leveldb::Slice, ^^leveldb::Options,                      \
    ^^leveldb::ReadOptions, ^^leveldb::WriteOptions, ^^leveldb::WriteBatch,       \
    ^^leveldb::CompressionType, ^^leveldbtest,                                    \
    ^^mirrorbind::exclude_<^^leveldb::Comparator, ^^leveldb::Env, ^^leveldb::Cache, \
                         ^^leveldb::FilterPolicy, ^^leveldb::Logger,              \
                         ^^leveldb::Snapshot, ^^leveldb::WriteBatch::Handler>
