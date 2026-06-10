// TC-0016 minimal: the implicit tag from a typedef inside a linkage-spec
// block has semantic ctx = enclosing scope but lexical ctx = the block;
// stepping it via the semantic chain walked BACKWARD and silently ENDED the
// TU enumeration. Everything after the block vanished from members_of(^^::).
// Field shape: Python.h's pytypedefs.h truncated the global walk mid-header,
// hiding every later global decl (box2d's free operators never bound).
#include <experimental/meta>
extern "C" { typedef struct PMD PMD; }
struct After { int y; };
consteval bool sees_after() {
    for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
        if (std::meta::is_type(m) && std::meta::is_class_type(m)
            && std::meta::has_identifier(m)
            && std::meta::identifier_of(m) == "After")
            return true;
    return false;
}
static_assert(sees_after());   // FAILS at base (walk truncated); passes fixed
int main() {}
