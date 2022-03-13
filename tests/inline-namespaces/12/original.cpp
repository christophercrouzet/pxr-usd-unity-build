#include <pxr/base/tf/notice.h>

using namespace std;

template <class T>
struct TfTest_SingletonFactory
{
    T*
    New()
    {
        return &T::GetInstance();
    }
};
