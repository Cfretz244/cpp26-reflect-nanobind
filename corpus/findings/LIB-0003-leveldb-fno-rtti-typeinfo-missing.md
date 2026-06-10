# LIB-0003 — leveldb builds -fno-rtti: polymorphic typeinfo missing, head-on DB binding impossible

- **Status:** RECORDED (library-side; run worked around it).
- **Found via:** corpus/runs/leveldb (wave 2; dedup key
  `library-fno-rtti-polymorphic-class-typeinfo-missing`). leveldb's CMake forces
  `-fno-rtti`; the archive has `vtable for leveldb::DB` but no `typeinfo for leveldb::DB`,
  and nanobind's RTTI-based registry fails to link. The run drives the real DB through a
  thin forwarding handle (KVStore) and still reaches E.
- **Note:** will recur for any -fno-rtti library (protobuf-lite and other Google C++
  libraries). Possible future harness lever: rebuild such archives with `-frtti` appended
  in `build_cmake_lib.sh` (ABI-compatible for these uses); not done this wave to keep the
  vendor build flags honest.
