#include <string>

using std::string;

int
main()
{
    string name = "hello!";
    int value = 0;

    auto fn = [&name, &value]()
    {
        (void)name;
        (void)value;
        string foo;
        (void)foo;
    };

    (void)fn;
    return 0;
}
