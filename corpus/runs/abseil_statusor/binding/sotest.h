// Fixture namespace for the Abseil StatusOr run. StatusOr<T>'s value/converting
// constructors are all templates (enable_if'd U&&) which the binder skips by
// design, so these factories supply construction -- the same pattern as
// abseil_time's timetest (Duration/Time have no public integer ctors). The
// returned objects are real absl::StatusOr<T>; everything observed on them in
// the tests goes through the directly bound ok()/status()/value().
// Shared by binding/binding.cpp and tests/oracle_native.cpp.
#pragma once
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace sotest {

inline absl::StatusOr<int> ok_int(int v) { return v; }
inline absl::StatusOr<int> err_int(absl::StatusCode c, std::string_view msg) {
    return absl::Status(c, msg);
}

inline absl::StatusOr<std::string> ok_str(std::string_view v) {
    return std::string(v);
}
inline absl::StatusOr<std::string> err_str(absl::StatusCode c, std::string_view msg) {
    return absl::Status(c, msg);
}

inline absl::StatusOr<double> ok_dbl(double v) { return v; }
inline absl::StatusOr<double> err_dbl(absl::StatusCode c, std::string_view msg) {
    return absl::Status(c, msg);
}

// StatusOr's value()/operator*/operator-> are declared in internal_statusor::
// OperatorBase<T> -- a PRIVATE base -- and re-exported into StatusOr with
// using-declarations. Neither is visible to the binder: using-declarations are
// not enumerable via members_of, and private bases are (correctly) not
// flattened. value access therefore goes through these accessors; they call the
// real value(), so the throw-on-error path (BadStatusOrAccess) is the real one.
inline int         get_int(const absl::StatusOr<int>& s) { return s.value(); }
inline std::string get_str(const absl::StatusOr<std::string>& s) { return s.value(); }
inline double      get_dbl(const absl::StatusOr<double>& s) { return s.value(); }

}  // namespace sotest
