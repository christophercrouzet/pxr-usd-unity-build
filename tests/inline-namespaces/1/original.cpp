#include <chrono>
#include <iostream>
#include <string>

using namespace std;
using namespace std::chrono;

std::string HELLO = "hello";
string WORLD = "world!";

void
hello(
    const std::string &s
)
{
    cout << HELLO << " " << s << std::endl;
}

int
main()
{
    std::chrono::time_point<std::chrono::steady_clock> t1
        = std::chrono::steady_clock::now();

    chrono::time_point<chrono::steady_clock> t2
        = chrono::steady_clock::now();

    time_point<steady_clock> t3
        = steady_clock::now();

    (void)t1;
    (void)t2;
    (void)t3;

    hello(WORLD);

    return 0;
}
