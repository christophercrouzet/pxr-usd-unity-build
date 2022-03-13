#include <string>
#include <utility>

using namespace std;

void
foo()
{
    auto make_test_pair = []() -> pair<int, string>
    {
        return make_pair(3, "C");
    };

    (void)make_test_pair;
}
