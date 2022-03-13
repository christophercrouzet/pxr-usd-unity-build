#include <pxr/base/arch/defines.h>
#include <pxr/base/arch/error.h>
#include <pxr/base/arch/systemInfo.h>
#include <pxr/base/tf/regTest.h>
#include <pxr/base/tf/bitUtils.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

enum _TestEnum {
    Test0,
    Test1,
};

int
main()
{
    ARCH_AXIOM(ArchGetExecutablePath().find("testArch", 0) != std::string::npos);
    TF_AXIOM(TF_BITS_FOR_VALUES(Test1) == 1);
    return 0;
}
