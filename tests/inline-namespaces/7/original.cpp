#include <boost/operators.hpp>

class Foo
    : boost::totally_ordered<Foo>
{
};
