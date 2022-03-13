#include <boost/compressed_pair.hpp>
#include <boost/iterator/iterator_facade.hpp>


template<class ElementType, class UnderlyingIterator>
class Foo:
    public boost::iterator_facade<
        Foo<ElementType, UnderlyingIterator>,
        ElementType,
        boost::bidirectional_traversal_tag
    >
{
private:
    friend class boost::iterator_core_access;
};
