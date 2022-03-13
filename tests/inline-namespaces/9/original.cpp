#include <functional>
#include <string>
#include <iostream>

void
hello(
    const std::string& s
)
{
    std::cout << "hello " << s << '\n';
}
 
int
main()
{
    namespace ph = std::placeholders;
    std::function<void(const std::string&)> f
        = std::bind(&hello, ph::_1);
}
