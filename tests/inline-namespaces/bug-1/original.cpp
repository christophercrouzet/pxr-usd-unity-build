#include <string>

using namespace std;

int
main()
{
    // typeLoc() should match `const string &` but it doesn't, see
    // https://github.com/llvm/llvm-project/issues/53209
    for (const string& k : {"key_a", "key_b"})
    {
        (void)k;
    }
}
