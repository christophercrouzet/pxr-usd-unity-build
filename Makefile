ALEMBIC_DIR    := /opt/alembic/1.7.10
BISON_DIR      := /opt/bison/2.4.1
BOOST_DIR      := /opt/boost/1.70.0
DRACO_DIR      := /opt/draco/1.5.2
EMBREE_DIR     := /opt/embree/3.2.2
FLEX_DIR       := /opt/flex/2.5.39
MATERIALX_DIR  := /opt/materialx/1.38.0
OCIO_DIR       := /opt/opencolorio/1.1.0
OIIO_DIR       := /opt/openimageio/2.1.16.0
OPENEXR_DIR    := /opt/openexr/2.3.0
OPENSUBDIV_DIR := /opt/opensubdiv/3.4.3
OPENVDB_DIR    := /opt/openvdb/8.1.0
OSL_DIR        := /opt/openshadinglanguage/1.11.17.0
PTEX_DIR       := /opt/ptex/2.1.33
TBB_DIR        := /opt/tbb/2018.U6

# ------------------------------------------------------------------------------

PROJECT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

# ------------------------------------------------------------------------------

GENERATED_MAKEFILE := CMakeFiles/Makefile2

# ------------------------------------------------------------------------------

BUILD_DIR     := $(PROJECT_DIR)/build

USD_SUBDIR    := deps/USD
USD_DIR       := $(PROJECT_DIR)/$(USD_SUBDIR)
USD_BUILD_DIR := $(USD_DIR)/build

USD_MAKEFILE  := $(USD_BUILD_DIR)/$(GENERATED_MAKEFILE)

# ------------------------------------------------------------------------------

# Forward rules to the generated Makefiles.
# $(1): build directory.
# $(2): rules.
# $(3): thread count to use.
define forward_rule =
$(MAKE) -C "$(1)" -f "$(1)/$(GENERATED_MAKEFILE)" -s $(2) --jobs=$(3)
endef

# ------------------------------------------------------------------------------

# Configure USD.
# $(1): build directory.
# $(2): unity build.
# $(3): cxx flags.
define configure_usd =
mkdir -p "$(1)"                                                                \
&& cd "$(1)"                                                                   \
&& cmake                                                                       \
    -DALEMBIC_DIR="$(ALEMBIC_DIR)"                                             \
    -DBISON_EXECUTABLE="$(BISON_DIR)/bin/bison"                                \
    -DBOOST_ROOT="$(BOOST_DIR)"                                                \
    -DDRACO_ROOT="$(DRACO_DIR)"                                                \
    -DEMBREE_LOCATION="$(EMBREE_DIR)"                                          \
    -DFLEX_EXECUTABLE="$(FLEX_DIR)/bin/flex"                                   \
    -DMATERIALX_ROOT="$(MATERIALX_DIR)"                                        \
    -DOCIO_LOCATION="$(OCIO_DIR)"                                              \
    -DOIIO_LOCATION="$(OIIO_DIR)"                                              \
    -DOPENEXR_LOCATION="$(OPENEXR_DIR)"                                        \
    -DOPENSUBDIV_ROOT_DIR="$(OPENSUBDIV_DIR)"                                  \
    -DOPENVDB_LOCATION="$(OPENVDB_DIR)"                                        \
    -DOSL_LOCATION="$(OSL_DIR)"                                                \
    -DPTEX_LOCATION="$(PTEX_DIR)"                                              \
    -DTBB_ROOT_DIR="$(TBB_DIR)"                                                \
                                                                               \
    -DBUILD_SHARED_LIBS=ON                                                     \
    -DCMAKE_BUILD_TYPE=Release                                                 \
    -DCMAKE_CXX_COMPILER="$(CXX)"                                              \
    -DCMAKE_CXX_FLAGS="$(3)"                                                   \
                                                                               \
    -DBoost_NO_BOOST_CMAKE=ON                                                  \
    -DBoost_NO_SYSTEM_PATHS=ON                                                 \
    -DTBB_USE_DEBUG_BUILD=OFF                                                  \
                                                                               \
    -DPXR_PREFER_SAFETY_OVER_SPEED=ON                                          \
    -DPXR_VALIDATE_GENERATED_CODE=ON                                           \
                                                                               \
    -DPXR_ENABLE_HDF5_SUPPORT=ON                                               \
    -DPXR_ENABLE_MATERIALX_SUPPORT=ON                                          \
    -DPXR_ENABLE_OPENVDB_SUPPORT=ON                                            \
    -DPXR_ENABLE_OSL_SUPPORT=ON                                                \
    -DPXR_ENABLE_PTEX_SUPPORT=ON                                               \
    -DPXR_ENABLE_PYTHON_SUPPORT=ON                                             \
                                                                               \
    -DPXR_USE_PYTHON_3=ON                                                      \
    -DPXR_USE_DEBUG_PYTHON=OFF                                                 \
                                                                               \
    -DPXR_BUILD_DOCUMENTATION=OFF                                              \
    -DPXR_BUILD_EXAMPLES=OFF                                                   \
    -DPXR_BUILD_TESTS=OFF                                                      \
    -DPXR_BUILD_TUTORIALS=OFF                                                  \
                                                                               \
    -DPXR_BUILD_ALEMBIC_PLUGIN=ON                                              \
    -DPXR_BUILD_EMBREE_PLUGIN=ON                                               \
    -DPXR_BUILD_DRACO_PLUGIN=ON                                                \
    -DPXR_BUILD_OPENIMAGEIO_PLUGIN=ON                                          \
    -DPXR_BUILD_OPENCOLORIO_PLUGIN=ON                                          \
    -DPXR_BUILD_PRMAN_PLUGIN=OFF                                               \
                                                                               \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON                                         \
                                                                               \
    -DPXR_ENABLE_UNITY_BUILD=$(2)                                              \
                                                                               \
    "$(USD_DIR)"
endef

# ------------------------------------------------------------------------------

$(BUILD_DIR)/Makefile:
	@ mkdir -p "$(BUILD_DIR)"
	@ cd "$(BUILD_DIR)"                                                        \
	    && cmake "$(PROJECT_DIR)"

# Configure the Makefile to build the tools.

configure: $(BUILD_DIR)/Makefile

.PHONY: configure

# ------------------------------------------------------------------------------

# Build the tools.

build: configure
	@ $(call forward_rule,$(BUILD_DIR),all,1)

.PHONY: build

# ------------------------------------------------------------------------------

# Fetch the remote USD repository.

usd-fetch:
	@ git submodule update --init --remote --force

.PHONY: usd-fetch

# ------------------------------------------------------------------------------

USD_INIT_HEAD_FILES_CMD := \
	sed -n 's/^.PHONY : \(.*_headerfiles\)$$/\1/p' $(USD_MAKEFILE) | tr '\n' ' '

# Generate USD's compilation database and build its header files targets.
#
# This is only useful to set-up the required environment before running
# the “usd-inline-namespaces” and “usd-disambiguate-symbols” rules,
# without having to compile the whole project.

usd-init: usd-clean
	@ git submodule foreach git reset --hard

	@ $(call configure_usd,$(USD_BUILD_DIR),OFF,)

	@ python3 "$(PROJECT_DIR)/tools/update-compile-commands.py"                \
	    --path="$(USD_DIR)"

	@ $(call forward_rule,$(USD_BUILD_DIR),$$($(USD_INIT_HEAD_FILES_CMD)),1)

.PHONY: usd-init

# ------------------------------------------------------------------------------

# Address a couple of compilation issues with Clang.
#
# Compiling with Clang is needed when wanting to profile the compilation process
# using Clang's “-ftime-trace” compilation flag and “ClangBuildAnalyzer”.
#
# Pull request: https://github.com/PixarAnimationStudios/USD/pull/1696

usd-patch-clang:
	@ cd "$(USD_DIR)"                                                          \
	    && git apply "$(PROJECT_DIR)/usd-clang.patch"

.PHONY: usd-patch-clang

# ------------------------------------------------------------------------------

# Run the “inline-namespaces” tool.
#
# It's a tool built on top of the Clang's AST API that automatically removes
# the “using” declarations and fully declares the namespace for the symbols
# that were relying on it.
#
# Options:
#   target
#     Directory to run the tool on (default: "pxr").
#
# Usage:
#   make usd-inline-namespaces
#   make usd-inline-namespaces target=pxr/base
#   make usd-inline-namespaces target=pxr/base/arch

ifdef target
    USD_INLINE_NAMESPACES_TARGET := "$(target)"
else
    USD_INLINE_NAMESPACES_TARGET := "pxr"
endif

usd-inline-namespaces: build
	@ python3 "$(PROJECT_DIR)/tools/fix.py"                                    \
	    --tool="inline-namespaces"                                             \
	    --path="$(USD_DIR)"                                                    \
	    $(USD_INLINE_NAMESPACES_TARGET)

.PHONY: usd-inline-namespaces

# ------------------------------------------------------------------------------

# Apply manual fixes to the “inline-namespaces” pass.
#
# Warning:
#   Needs to be run after the rule “usd-inline-namespaces”.

usd-patch-inline-namespaces:
	@ cd "$(USD_DIR)"                                                          \
	    && git apply "$(PROJECT_DIR)/usd-inline-namespaces.patch"

.PHONY: usd-patch-inline-namespaces

# ------------------------------------------------------------------------------

# Run the “disambiguate-symbols” tool.
#
# It's a tool built on top of the Clang's AST API that automatically sets a name
# to all the anonymous namespaces found and declares the namespace for
# the symbols that were relying on it.
#
# Warning:
#   Needs to be run after the rule “usd-patch-inline-namespaces”.
#
# Options:
#   target
#     Directory to run the tool on (default: "pxr").
#
# Usage:
#   make usd-disambiguate-symbols
#   make usd-disambiguate-symbols target=pxr/base
#   make usd-disambiguate-symbols target=pxr/base/arch

ifdef target
    USD_DISAMBIGUATE_SYMBOLS_TARGET := "$(target)"
else
    USD_DISAMBIGUATE_SYMBOLS_TARGET := "pxr"
endif

usd-disambiguate-symbols: build
	@ python3 "$(PROJECT_DIR)/tools/fix.py"                                    \
	    --tool="disambiguate-symbols"                                          \
	    --path="$(USD_DIR)"                                                    \
	    $(USD_DISAMBIGUATE_SYMBOLS_TARGET)

.PHONY: usd-disambiguate-symbols

# ------------------------------------------------------------------------------

# Apply manual fixes to the “disambiguate-symbols” pass.
#
# Warning:
#   Needs to be run after the rule “usd-disambiguate-symbols”.

usd-patch-disambiguate-symbols:
	@ cd "$(USD_DIR)"                                                          \
	    && git apply "$(PROJECT_DIR)/usd-disambiguate-symbols.patch"

.PHONY: usd-patch-disambiguate-symbols

# ------------------------------------------------------------------------------

# Apply a bunch of miscellaneous manual touches.
#
# Warning:
#   Needs to be run after the rule “usd-patch-disambiguate-symbols”.

usd-patch-misc:
	@ cd "$(USD_DIR)"                                                          \
	    && git apply "$(PROJECT_DIR)/usd-misc.patch"

.PHONY: usd-patch-misc

# ------------------------------------------------------------------------------

LOCAL_USD_BUILD_DIR := "$(BUILD_DIR)/usd"
USD_BUILD_CXX_FLAGS := -DTBB_USE_GLIBCXX_VERSION=90000

ifdef target
    USD_BUILD_TARGET := "$(target)"
else
    USD_BUILD_TARGET := "all"
endif

ifeq ($(unity),ON)
    USD_BUILD_UNITY := ON
else
    USD_BUILD_UNITY := OFF
endif

ifeq ($(trace),ON)
    USD_BUILD_CXX_FLAGS := $(USD_BUILD_CXX_FLAGS) -ftime-trace
endif

ifdef jobs
    USD_BUILD_JOBS := $(jobs)
else
    USD_BUILD_JOBS := 1
endif

$(LOCAL_USD_BUILD_DIR)/Makefile:
	@ $(call configure_usd,$(LOCAL_USD_BUILD_DIR),$(USD_BUILD_UNITY),$(USD_BUILD_CXX_FLAGS))

# Build USD.
#
# Options:
#   target
#     Target to build (default: "all").
#   trace
#     Whether to compile with the “-ftime-trace” flag (Clang only) (default: ON).
#   jobs
#     Number of threads to use (default: 1).
#
# Usage:
#   make usd-build
#   make usd-build target=pxr/base/all trace=ON jobs=4

usd-build: $(LOCAL_USD_BUILD_DIR)/Makefile
	@ time --format="elapsed: %E"                                              \
	    $(call forward_rule,$(LOCAL_USD_BUILD_DIR),$(USD_BUILD_TARGET),$(USD_BUILD_JOBS))

.PHONY: usd-build

# ------------------------------------------------------------------------------

# Analyze the trace data resulting from using the “-ftime-trace” compiler flag.
#
# This writes a file at the root named “profile”.

usd-analyze-trace:
	@ ClangBuildAnalyzer --all $(LOCAL_USD_BUILD_DIR) $(BUILD_DIR)/profile
	@ ClangBuildAnalyzer --analyze $(BUILD_DIR)/profile > $(PROJECT_DIR)/profile

.PHONY: usd-analyze-trace

# ------------------------------------------------------------------------------

# Clean the USD's build directory.

usd-clean:
	@ rm -rf $(USD_BUILD_DIR)

.PHONY: usd-clean

# ------------------------------------------------------------------------------

ifdef tool
    TEST_TOOL := "$(tool)"
else
    TEST_TOOL := "*"
endif

ifdef test
    TEST_TEST := "$(test)"
else
    TEST_TEST := "*"
endif

ifeq ($(verbose),ON)
    TEST_VERBOSE := "--verbose"
else
    TEST_VERBOSE :=
endif

# Run the tests.
#
# Options:
#   tool
#     Which tool to test (default: "*").
#   test
#     Name of the tests to consider (default: "*").
#   verbose
#     Whether to print some debut log (default: OFF).
#
# Usage:
#   make test
#   make test test=foo
#   make test tool=inline-namespaces test=foo verbose=ON

test: build
	@ python3 "$(PROJECT_DIR)/tools/test.py"                                   \
	    --path="$(USD_DIR)"                                                    \
	    --tool="$(TEST_TOOL)"                                                  \
	    --test="$(TEST_TEST)"                                                  \
	    $(TEST_VERBOSE)

.PHONY: test

# ------------------------------------------------------------------------------

# Run the “inline-namespaces” tool on a file “tmp.cpp”.
#
# Warning:
#   The rule “usd-init” needs to have been run once beforehand.

tmp-inline-namespaces: build
	@ $(BUILD_DIR)/bin/inline-namespaces                                       \
	    --root="$(PROJECT_DIR)"                                                \
	    --file-pattern="^$(USD_DIR)/*"                                         \
	    -p="$(USD_BUILD_DIR)"                                                  \
	    "$(PROJECT_DIR)/tmp.cpp"

.PHONY: tmp-inline-namespaces

# ------------------------------------------------------------------------------

# Run the “disambiguate-symbols” tool on a file “tmp.cpp”.
#
# Warning:
#   The rule “usd-init” needs to have been run once beforehand.

tmp-disambiguate-symbols: build
	@ $(BUILD_DIR)/bin/disambiguate-symbols                                    \
	    --root="$(PROJECT_DIR)"                                                \
	    -p="$(USD_BUILD_DIR)"                                                  \
	    "$(PROJECT_DIR)/tmp.cpp"

.PHONY: tmp-disambiguate-symbols

# ------------------------------------------------------------------------------

# Run Clang's AST dump tool on a file “tmp.cpp”.
#
# Warning:
#   The rule “usd-init” needs to have been run once beforehand.

tmp-dump: build
	@ clang++-14                                                               \
	    -Xclang                                                                \
	    -ast-dump                                                              \
	    -fsyntax-only                                                          \
	    -I$(USD_DIR)                                                           \
	    -I$(USD_BUILD_DIR)/include                                             \
	    "$(PROJECT_DIR)/tmp.cpp"

.PHONY: tmp-dump

# ------------------------------------------------------------------------------

# Start Clang's AST query tool on a file “tmp.cpp”.
#
# Warning:
#   The rule “usd-init” needs to have been run once beforehand.

tmp-query:
	@ clang-query-14                                                           \
	    -p="$(USD_BUILD_DIR)"                                                  \
	    "$(PROJECT_DIR)/tmp.cpp"

.PHONY: tmp-query

# ------------------------------------------------------------------------------

# Clean everything.

clean: usd-clean
	@ rm -rf $(BUILD_DIR)

.PHONY: clean

# ------------------------------------------------------------------------------

.DEFAULT_GOAL := build
