// TC-0020: parameter identifier_of was instantiation-state-dependent when a
// member's declaration and out-of-line definition name the parameter
// differently.
//
//   $TC/bin/clang++ -std=c++26 -freflection-latest -stdlib=libc++ \
//     -isysroot "$(xcrun --show-sdk-path)" -nostdinc++ \
//     -isystem $TC/include/c++/v1 [-DFORCE_DEFINITION] -fsyntax-only repro.cpp
//
// Pre-fix: without -DFORCE_DEFINITION the query returned the in-class
// declaration's name ("value"); with it (an explicit instantiation of the
// definition before the query) the SAME query on the SAME entity returned
// "val" -- the parameters were REPLACED in place on the instantiated
// FunctionDecl, so the consistency walk saw only one name. Two TUs
// reflecting the same entity disagreed.
//
// Post-fix: the consistency walk uses the template pattern's full
// declaration chain, so the inconsistently-named parameter deterministically
// has NO identifier in BOTH variants, and a consistently-named sibling
// keeps its name.
//
// Field shape: Eigen declares DenseBase::setConstant(const Scalar& value)
// and defines it with `val`; a constexpr binding TU and a pure codegen TU
// rendered different Python keyword names for the same method -- the only
// surface divergence across a 36-library corpus.
#include <experimental/meta>
#include <string_view>
using namespace std::meta;

template <class T> struct S {
  void f(int value);       // inconsistent: definition says "val"
  void g(int width);       // consistent across decl + definition
};
template <class T> void S<T>::f(int val) {}
template <class T> void S<T>::g(int width) {}

#ifdef FORCE_DEFINITION
template void S<int>::f(int);   // instantiate the DEFINITION before the query
template void S<int>::g(int);
#endif

consteval info param0(std::string_view name) {
    for (auto m : members_of(^^S<int>, access_context::unchecked()))
        if (is_function(m) && has_identifier(m) && identifier_of(m) == name)
            return parameters_of(m)[0];
    return {};
}

// Inconsistently-named parameter: deterministically unnamed, regardless of
// instantiation state.
static_assert(!has_identifier(param0("f")));
// Consistently-named parameter: keeps its name.
static_assert(has_identifier(param0("g")));
static_assert(identifier_of(param0("g")) == "width");
int main() {}
