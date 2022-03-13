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


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static boost::python::tuple
FindClosestPointsHelper()
{
    return boost::python::make_tuple(123, 456);
}

static void
SetFromViewAndProjectionMatrix(
    float viewMatrix,
    float projMatrix,
    float focalLength
)
{
    boost::python::slice::range<const float*> bounds;
    (void)bounds;
}

static boost::python::object
GetPerspectiveHelper(
    const float &self,
    bool isFovVertical
)
{
    return boost::python::object();
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
    typedef GfLine This;

    boost::python::def("FindClosestPoints", FindClosestPointsHelper);

    boost::python::class_<This>("Line", "Line class", boost::python::init<>())
        .def(boost::python::init<const GfVec3d &, const GfVec3d &>())

        .def(TfTypePythonClass())

        .def("Set", &This::Set, boost::python::return_self<>())

        .def(
            "GetDirection",
            &This::GetDirection,
            boost::python::return_value_policy<boost::python::copy_const_reference>()
        )

        .def(
            "SetFromViewAndProjectionMatrix",
            SetFromViewAndProjectionMatrix,
            (
                boost::python::arg("viewMatrix"),
                boost::python::arg("projMatrix"),
                boost::python::arg("focalLength") = 50
            )
        )

        .def(
            "GetPerspective",
            GetPerspectiveHelper,
            (boost::python::args("isFovVertical") = true)
        )

        .add_property(
            "direction",
            boost::python::make_function(
                &This::GetDirection,
                boost::python::return_value_policy
                <boost::python::copy_const_reference>()
            ),
            SetDirectionHelper
        )

        .def(boost::python::self_ns::str(boost::python::self))

        .def(boost::python::self == boost::python::self)

        .def(boost::python::self != boost::python::self)
        ;

    boost::python::implicitly_convertible<float, double>();

    boost::python::class_<This> cls("OtherLine", boost::python::no_init);
}
