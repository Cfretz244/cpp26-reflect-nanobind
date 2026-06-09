// Gate 1 probe for fmtlib/fmt (11.2.0), header-only mode. Includes the public headers
// the binding uses, under the p2996 toolchain flags; -fsyntax-only.
//
// NOTE: <cstdlib> is included FIRST to work around a latent fmt bug (LIB-0001): fmt's
// detail::allocator<T> calls unqualified global malloc()/free() in <fmt/format.h> without
// including <cstdlib> and without std:: qualification. Strict C++ two-phase name lookup in
// clang-p2996 rejects the non-dependent unqualified call unless the name is declared before
// the template definition. Making <cstdlib> visible first satisfies that (no fmt edit needed).
#include <cstdlib>
#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
