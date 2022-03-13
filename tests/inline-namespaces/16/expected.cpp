#include <string>
#include <utility>


void
foo()
{
    auto make_test_pair = []() -> std::pair<int, std::string>
    {
        return std::make_pair(3, "C");
    };

    (void)make_test_pair;
}
