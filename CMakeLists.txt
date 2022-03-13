cmake_minimum_required(VERSION 3.1)

project(
    UsdUnityBuild
        VERSION 0.1.0
)

# ------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------

SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

find_package(
    Clang
        REQUIRED
)

# ------------------------------------------------------------------------------

add_executable(
    disambiguate-symbols
        src/Helpers.cpp
        src/Locations.cpp
        src/Replacements.cpp
        src/disambiguate-symbols/DisambiguateSymbols.cpp
        src/disambiguate-symbols/tool/DisambiguateSymbols.cpp
)
set_target_properties(
    disambiguate-symbols
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY bin
)
target_include_directories(
    disambiguate-symbols
        PRIVATE
            "${CLANG_INCLUDE_DIRS}"
)
target_link_libraries(
    disambiguate-symbols
        PRIVATE
            clangTooling
)

# ------------------------------------------------------------------------------

add_executable(
    inline-namespaces
        src/Helpers.cpp
        src/Locations.cpp
        src/Replacements.cpp
        src/inline-namespaces/InlineNamespaces.cpp
        src/inline-namespaces/tool/InlineNamespaces.cpp
)
set_target_properties(
    inline-namespaces
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY bin
)
target_include_directories(
    inline-namespaces
        PRIVATE
            "${CLANG_INCLUDE_DIRS}"
)
target_link_libraries(
    inline-namespaces
        PRIVATE
            clangTooling
)
