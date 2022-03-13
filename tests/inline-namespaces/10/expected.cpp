#include "pxr/pxr.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/regTest.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

PXR_NAMESPACE_USING_DIRECTIVE


PXR_NAMESPACE_USING_DIRECTIVE

static size_t
Tf_CountFileMatches(const std::string& pattern)
{
    // Temporary file exists.
    std::vector<std::string> matches = TfGlob(pattern, 0);
    std::cout << "TfGlob('" << pattern << "') => " << matches << std::endl;
    return matches.size();
}
