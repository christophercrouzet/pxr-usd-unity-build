#include <pxr/base/gf/line.h>
#include <pxr/base/tf/wrapTypeHelpers.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/tuple.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static tuple
FindClosestPointsHelper()
{
    using namespace boost;
    return python::make_tuple(123, 456);
}

static void
SetFromViewAndProjectionMatrix(
    float viewMatrix,
    float projMatrix,
    float focalLength
)
{
    slice::range<const float*> bounds;
    (void)bounds;
}

static api::object
GetPerspectiveHelper(
    const float &self,
    bool isFovVertical
)
{
    return object();
}

static void
SetDirectionHelper(
    GfLine &self,
    const GfVec3d &dir
)
{
    self.Set(self.GetPoint(0.0), dir);
}

} // anonymous namespace 

void wrapLine()
{
    using namespace boost;
    typedef GfLine This;

    def("FindClosestPoints", FindClosestPointsHelper);

    class_<This>("Line", "Line class", init<>())
        .def(init<const GfVec3d &, const GfVec3d &>())

        .def(TfTypePythonClass())

        .def("Set", &This::Set, return_self<>())

        .def(
            "GetDirection",
            &This::GetDirection,
            return_value_policy<copy_const_reference>()
        )

        .def(
            "SetFromViewAndProjectionMatrix",
            SetFromViewAndProjectionMatrix,
            (
                arg("viewMatrix"),
                arg("projMatrix"),
                arg("focalLength") = 50
            )
        )

        .def(
            "GetPerspective",
            GetPerspectiveHelper,
            (boost::python::args("isFovVertical") = true)
        )

        .add_property(
            "direction",
            make_function(
                &This::GetDirection,
                return_value_policy
                <python::copy_const_reference>()
            ),
            SetDirectionHelper
        )

        .def(str(self))

        .def(python::self_ns::self == self)

        .def(self != python::self_ns::self)
        ;

    implicitly_convertible<float, double>();

    boost::python::class_<This> cls("OtherLine", no_init);
}
