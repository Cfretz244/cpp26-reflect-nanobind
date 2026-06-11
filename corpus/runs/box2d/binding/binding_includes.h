// Plain C++ includes for the box2d run (parsed by BOTH compilers: the P2996
// toolchain for the constexpr lane / emit generator, and Apple Clang + system
// libc++ for the emit module). box2d's public types live in the GLOBAL
// namespace; the single umbrella header pulls the whole v2.4.2 C++ API.
// No ^^, no annotations.
#pragma once
#include <box2d/box2d.h>
