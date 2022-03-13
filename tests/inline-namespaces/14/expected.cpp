#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/singleton.h>


PXR_NAMESPACE_OPEN_SCOPE

class Tf_NoticeRegistry
{
public:
    typedef TfNotice::_DelivererList _DelivererList;
    typedef std::pair<_DelivererList*, _DelivererList::iterator>
        _DelivererListEntry;
};

PXR_NAMESPACE_CLOSE_SCOPE
