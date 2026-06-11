// GCC-5: matching a partial-specialization matrix over a spliced member
// function type ICEs when the member's noexcept-specifier is a DEPENDENT
// expression that GCC has not yet resolved for the instantiated member
// (internal compiler error: in nothrow_spec_p, at cp/except.cc:1240).
//
// Field shape: nlohmann::basic_json<>::swap(reference) declares
//   noexcept(std::is_nothrow_move_constructible<value_t>::value && ...)
// and the binder's reflect_method_binder<T, fn, Ret(Args...) [noexcept]>
// matrix selection (most_specialized_partial_spec -> nothrow_spec_p) dies.
//
//   g++ -std=c++26 -freflection xfail_gcc5_deferred_noexcept_partial_spec.cpp
//
// Expected: compiles and exits 0. Actual (gcc 16.1.0): ICE as above.
#include <meta>
#include <type_traits>
#include <utility>

template <typename T, std::meta::info fn, typename FnType>
struct binder;

template <typename T, std::meta::info fn, typename Ret, typename... Args>
struct binder<T, fn, Ret(Args...)> {
    static void bind() {
        auto l = [](T& self, Args... args) -> Ret {
            return self.[:fn:](std::forward<Args>(args)...);
        };
        (void)l;
    }
};
template <typename T, std::meta::info fn, typename Ret, typename... Args>
struct binder<T, fn, Ret(Args...) noexcept> {
    static void bind() {
        auto l = [](T& self, Args... args) -> Ret {
            return self.[:fn:](std::forward<Args>(args)...);
        };
        (void)l;
    }
};

template <typename U>
struct W {
    // Dependent noexcept-specifier, like basic_json's swap.
    void swap(W& other) noexcept(std::is_nothrow_move_constructible<U>::value) {
        (void)other;
    }
};

template <typename T, std::meta::info fn>
void bind_method() {
    using FnType = [:std::meta::type_of(fn):];
    binder<T, fn, FnType>::bind();
}

int main() {
    using T = W<int>;
    template for (constexpr auto m : std::define_static_array(
                      std::meta::members_of(^^T,
                          std::meta::access_context::unchecked()))) {
        if constexpr (std::meta::is_function(m)
                      && !std::meta::is_special_member_function(m)
                      && std::meta::has_identifier(m)
                      && std::meta::identifier_of(m) == std::string_view("swap")) {
            bind_method<T, m>();
        }
    };
    return 0;
}
