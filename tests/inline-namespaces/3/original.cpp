#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/env.h"
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/arch/vsnprintf.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <memory>
#include <utility>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <alloca.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <unistd.h>

PXR_NAMESPACE_OPEN_SCOPE

using std::pair;
using std::string;
using std::set;


namespace {

enum TokenType { Dot, DotDot, Elem };

typedef pair<string::const_iterator, string::const_iterator> Token;
typedef pair<string::reverse_iterator, string::reverse_iterator> RToken;

template <class Iter>
inline pair<Iter, Iter>
_NextToken(Iter i, Iter end)
{
    pair<Iter, Iter> t;
    for (t.first = i;
         t.first != end && *t.first == '/'; ++t.first) {}
    for (t.second = t.first;
         t.second != end && *t.second != '/'; ++t.second) {}
    return t;
}

template <class Iter>
inline TokenType
_GetTokenType(pair<Iter, Iter> t) {
    size_t len = distance(t.first, t.second);
    if (len == 1 && t.first[0] == '.')
        return Dot;
    if (len == 2 && t.first[0] == '.' && t.first[1] == '.')
        return DotDot;
    return Elem;
}

string
_NormPath(
    string const &inPath
)
{
    string path(inPath);

    Token t = _NextToken(inPath.begin(), inPath.end());

    const size_t numLeadingSlashes = distance(inPath.begin(), t.first);
    size_t writeIdx = numLeadingSlashes >= 3 ? 1 : numLeadingSlashes;

    size_t firstWriteIdx = writeIdx;
    
    for (; t.first != inPath.end(); t = _NextToken(t.second, inPath.end()))
    {
        switch (_GetTokenType(t))
        {
            case Dot:
            case Elem:
            case DotDot:
            {
                string::reverse_iterator
                    rstart(path.begin() + firstWriteIdx),
                    rwrite(path.begin() + writeIdx);
                RToken backToken = _NextToken(rwrite, rstart);
                _GetTokenType(backToken);
            }

            break;
        };
    }
    
    return path;
}
} // anon

template <class Mapping>
static inline Mapping
Arch_MapFileImpl(
    FILE *file,
    std::string *errMsg
)
{
    using PtrType = typename Mapping::pointer;
    constexpr bool isConst =
        std::is_const<typename Mapping::element_type>::value;

    auto length = ArchGetFileLength(file);
    if (length < 0)
    {
        return Mapping();
    }

    auto m = mmap(
        nullptr,
        length,
        isConst ? PROT_READ : PROT_READ | PROT_WRITE,
        MAP_PRIVATE,
        fileno(file),
        0
    );
    Mapping ret(
        m == MAP_FAILED
            ? nullptr
            : static_cast<PtrType>(m),
        Arch_Unmapper(length)
    );
    return ret;
}

ArchConstFileMapping
ArchMapFileReadOnly(
    FILE *file,
    std::string *errMsg
)
{
    return Arch_MapFileImpl<ArchConstFileMapping>(file, errMsg);
}

ArchMutableFileMapping
ArchMapFileReadWrite(
    FILE *file,
    std::string *errMsg
)
{
    return Arch_MapFileImpl<ArchMutableFileMapping>(file, errMsg);
}

namespace
{
    
struct _Fcloser
{
    void
    operator()(
        FILE *f
    ) const
    {
        if (f)
        {
            fclose(f);
        }
    }
};

using _UniqueFILE = std::unique_ptr<FILE, _Fcloser>;

} // end anonymous namespace

template <class Mapping>
static inline Mapping
Arch_MapFileImpl(
    std::string const& path,
    std::string *errMsg
)
{
    _UniqueFILE f(ArchOpenFile(path.c_str(), "rb"));
    if (!f)
    {
        if (errMsg)
        {
            *errMsg = ArchStrerror();
        }

        return Mapping();
    }

    return Arch_MapFileImpl<Mapping>(f.get(), errMsg);
}

ArchConstFileMapping
ArchMapFileReadOnly(
    std::string const& path,
    std::string *errMsg
)
{
    return Arch_MapFileImpl<ArchConstFileMapping>(_NormPath(path), errMsg);
}

ArchMutableFileMapping
ArchMapFileReadWrite(
    std::string const& path,
    std::string *errMsg
)
{
    return Arch_MapFileImpl<ArchMutableFileMapping>(_NormPath(path), errMsg);
}

PXR_NAMESPACE_CLOSE_SCOPE
