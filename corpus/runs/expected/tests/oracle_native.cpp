// Native C++ ground-truth oracle for the tl::expected binding (Layer-1 differential).
// Drives the EXACT scenarios test_bindings.py drives through the bound module -- the
// same factories, the same swap/copy/equality/monadic sequences -- through native
// tl::expected via the shared extest fixture, and emits every observable as JSON.
// Shared compiler + shared (header-only) library => any divergence is the binding
// layer's.
#include "../binding/extest.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::vector<std::pair<std::string, std::string>> kv;
    auto add_s = [&](const char* k, const std::string& v) {
        std::string e = "\"";
        for (char c : v) {
            if (c == '\\' || c == '"') { e += '\\'; e += c; }
            else if (c == '\n') { e += "\\n"; }
            else { e += c; }
        }
        kv.emplace_back(k, e + "\"");
    };
    auto add_i = [&](const char* k, std::int64_t v) { kv.emplace_back(k, std::to_string(v)); };
    auto add_b = [&](const char* k, bool v) { kv.emplace_back(k, v ? "true" : "false"); };

    using EI = tl::expected<int, std::string>;
    using ES = tl::expected<std::string, int>;

    // --- default construction (the one non-template, non-copy ctor) ---
    EI di;
    add_b("def_int_has", di.has_value());
    add_i("def_int_value", di.value());
    ES ds;
    add_b("def_str_has", ds.has_value());
    add_s("def_str_value", ds.value());

    // --- ok/err factories + the directly bound observers ---
    EI ok = extest::ok_int(42);
    add_b("ok_has", ok.has_value());
    add_b("ok_bool", static_cast<bool>(ok));
    add_i("ok_value", ok.value());
    EI err = extest::err_int("nope");
    add_b("err_has", err.has_value());
    add_b("err_bool", static_cast<bool>(err));
    add_s("err_error", err.error());

    // --- value() on an errored expected: the REAL bad_expected_access throw path ---
    try {
        (void)err.value();
        add_s("throw_what", "<no exception>");
        add_s("throw_error", "<no exception>");
    } catch (const tl::bad_expected_access<std::string>& ex) {
        add_s("throw_what", ex.what());
        add_s("throw_error", ex.error());
    }

    // --- member swap (all-defaulted member template, bound as `swap`) ---
    EI a = extest::ok_int(7);
    EI b = extest::err_int("swapped");
    a.swap(b);
    add_b("swap_a_has", a.has_value());
    add_s("swap_a_error", a.error());
    add_b("swap_b_has", b.has_value());
    add_i("swap_b_value", b.value());

    // --- copy construction from another bound instance ---
    EI copy(ok);
    add_b("copy_has", copy.has_value());
    add_i("copy_value", copy.value());

    // --- unexpected<E>: real non-template ctor + value() ---
    tl::unexpected<std::string> u(std::string("boom"));
    add_s("unexp_value", u.value());
    tl::unexpected<int> ui(17);
    add_i("unexp_int_value", ui.value());

    // --- the real expected(const unexpected<G>&) converting ctor (extest wrapper) ---
    EI ue = extest::from_unexpected(u);
    add_b("from_unexp_has", ue.has_value());
    add_s("from_unexp_error", ue.error());

    // --- role-swapped spec: expected<std::string, int> ---
    ES os = extest::ok_str("hello expected");
    add_b("ok_str_has", os.has_value());
    add_s("ok_str_value", os.value());
    ES es = extest::err_str(404);
    add_b("err_str_has", es.has_value());
    add_i("err_str_error", es.error());

    // --- value_or / equality wrappers (all-template fronts) ---
    add_i("value_or_ok", extest::value_or_int(ok, -1));
    add_i("value_or_err", extest::value_or_int(err, -1));
    add_b("eq_ok_ok", extest::eq_int(extest::ok_int(1), extest::ok_int(1)));
    add_b("eq_ok_err", extest::eq_int(extest::ok_int(1), extest::err_int("x")));
    add_b("eq_err_err", extest::eq_int(extest::err_int("x"), extest::err_int("x")));
    add_b("eq_value", extest::eq_int_value(ok, 42));
    add_b("eq_unexp", extest::eq_int_unexpected(err, tl::unexpected<std::string>(std::string("nope"))));

    // --- monadic wrappers (fixed C++ callables over map/and_then) ---
    add_i("map_ok", extest::map_double(extest::ok_int(21)).value());
    add_s("map_err", extest::map_double(extest::err_int("m")).error());
    add_i("and_then_even", extest::and_then_halve(extest::ok_int(10)).value());
    add_s("and_then_odd", extest::and_then_halve(extest::ok_int(3)).error());
    add_s("and_then_early", extest::and_then_halve(extest::err_int("early")).error());

    // --- bad_expected_access<std::string> as a directly constructed value ---
    tl::bad_expected_access<std::string> bea(std::string("direct"));
    add_s("bea_error", bea.error());
    add_s("bea_what", bea.what());

    std::cout << "{";
    for (size_t i = 0; i < kv.size(); ++i) {
        if (i) std::cout << ",";
        std::cout << "\"" << kv[i].first << "\":" << kv[i].second;
    }
    std::cout << "}" << std::endl;
    return 0;
}
