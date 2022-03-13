#include <boost/compressed_pair.hpp>
#include <boost/iterator/iterator_facade.hpp>

using namespace boost;

template<class ElementType, class UnderlyingIterator>
class Foo:
    public iterator_facade<
        Foo<ElementType, UnderlyingIterator>,
        ElementType,
        bidirectional_traversal_tag
    >
{
private:
    friend class iterator_core_access;
};
