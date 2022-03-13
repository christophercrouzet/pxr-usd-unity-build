#include <pxr/base/tf/pyFunction.h>

#include <boost/python/object.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

void
wrap()
{
    TfPyFunctionFromPython<void()>();
    TfPyFunctionFromPython<bool()>();
    TfPyFunctionFromPython<int()>();
    TfPyFunctionFromPython<long()>();
    TfPyFunctionFromPython<double()>();
    TfPyFunctionFromPython<std::string()>();
    TfPyFunctionFromPython<boost::python::object()>();
}
