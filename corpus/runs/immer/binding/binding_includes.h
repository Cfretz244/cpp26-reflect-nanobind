// Plain C++ includes for the immer run (parsed by BOTH the P2996 toolchain and
// the production Apple-Clang compile of the generated source). No `^^`, no
// `[[=...]]` annotations -- just the library headers the bound specializations
// (immer::vector<int>/<std::string>, immer::map<int,int>, immer::set<int>) need.
#pragma once
#include <immer/vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>

#include <string>
