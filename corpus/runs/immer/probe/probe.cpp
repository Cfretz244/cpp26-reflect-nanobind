#include <immer/vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <string>

int main() {
    immer::vector<int> vi;
    immer::vector<std::string> vs;
    immer::map<int, int> mi;
    immer::set<int> si;
    (void)vi; (void)vs; (void)mi; (void)si;
    return 0;
}
