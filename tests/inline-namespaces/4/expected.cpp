#include <pxr/base/js/converter.h>

#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

static boost::python::dict
Foo()
{
    boost::python::dict result;
    result["abc"] = JsConvertToContainerType<boost::python::object, boost::python::dict>(JsValue());
    return result;
}
