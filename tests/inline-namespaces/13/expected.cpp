#include <string>


int
main()
{
    std::string name = "hello!";
    int value = 0;

    auto fn = [&name, &value]()
    {
        (void)name;
        (void)value;
        std::string foo;
        (void)foo;
    };

    (void)fn;
    return 0;
}
