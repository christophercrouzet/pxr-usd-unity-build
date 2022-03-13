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

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

using namespace std;

PXR_NAMESPACE_USING_DIRECTIVE

static size_t
Tf_CountFileMatches(const string& pattern)
{
    // Temporary file exists.
    vector<string> matches = TfGlob(pattern, 0);
    cout << "TfGlob('" << pattern << "') => " << matches << endl;
    return matches.size();
}
