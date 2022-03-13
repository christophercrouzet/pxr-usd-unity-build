#include <pxr/base/js/converter.h>

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static dict
Foo()
{
    dict result;
    result["abc"] = JsConvertToContainerType<object, dict>(JsValue());
    return result;
}
