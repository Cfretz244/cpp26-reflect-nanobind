// Plain C++ includes for the yaml-cpp run (parsed by BOTH compilers: the P2996
// reflection toolchain for the constexpr/generator lanes, and Apple Clang for
// the emit lane's generated TU). No `^^`, no `[[=...]]` annotations.
//
// yamlfix.h pulls yaml-cpp's own umbrella header (yaml.h) plus the member-template
// forwarders (Node::as<T>/operator[]) and the std::string Load front.
#pragma once
#include "yaml-cpp/yaml.h"
#include "yamlfix.h"
