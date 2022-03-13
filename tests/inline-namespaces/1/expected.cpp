#include <chrono>
#include <iostream>
#include <string>


std::string HELLO = "hello";
std::string WORLD = "world!";

void
hello(
    const std::string &s
)
{
    std::cout << HELLO << " " << s << std::endl;
}

int
main()
{
    std::chrono::time_point<std::chrono::steady_clock> t1
        = std::chrono::steady_clock::now();

    std::chrono::time_point<std::chrono::steady_clock> t2
        = std::chrono::steady_clock::now();

    std::chrono::time_point<std::chrono::steady_clock> t3
        = std::chrono::steady_clock::now();

    (void)t1;
    (void)t2;
    (void)t3;

    hello(WORLD);

    return 0;
}
