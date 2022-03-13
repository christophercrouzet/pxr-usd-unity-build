#include <pxr/base/tf/notice.h>


template <class T>
struct TfTest_SingletonFactory
{
    T*
    New()
    {
        return &T::GetInstance();
    }
};
