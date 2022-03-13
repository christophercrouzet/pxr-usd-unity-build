#include <pxr/base/tf/pyUtils.h>

#include <boost/function.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/converter/registered.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
struct TfPyFunctionFromPython;

template <typename Ret, typename... Args>
struct TfPyFunctionFromPython<Ret (Args...)>
{

    TfPyFunctionFromPython()
    {
        RegisterFunctionType<boost::function<Ret (Args...)>>();
        RegisterFunctionType<std::function<Ret (Args...)>>();
    }

    template <typename FuncType>
    static void
    RegisterFunctionType()
    {
        boost::python::converter::registry::insert(
            &convertible, &construct<FuncType>, boost::python::type_id<FuncType>()
        );
    }

    static void *convertible(
        PyObject *obj
    )
    {
        return ((obj == Py_None) || PyCallable_Check(obj)) ? obj : 0;
    }

    template <typename FuncType>
    static void construct(
        PyObject *src,
        boost::python::converter::rvalue_from_python_stage1_data *data
    )
    {
    }
};

PXR_NAMESPACE_CLOSE_SCOPE
