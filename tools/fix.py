#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Run the refactoring tool."""

from argparse import ArgumentParser
from os import (
    pardir,
    sep,
    walk,
)
from os.path import (
    abspath,
    dirname,
    join,
    split,
    splitext,
)
from subprocess import run
from re import compile as re_compile


ROOT_DIR = abspath(join(dirname(__file__), pardir))
EXECUTABLE_DIR = join(ROOT_DIR, "build", "bin")

VERSIONED_FILE = re_compile(r"_v\d+$")


def contains_slice(seq, slice):
    slice_len = len(slice)
    for i in range(len(seq) - slice_len + 1):
        if all(seq[i + j] == slice[j] for j in range(slice_len)):
            return True

    return False


def filter_file_common(file_path):
    dir_path, file_name = split(file_path)
    base_name, ext = splitext(file_name)
    dir_path = dir_path.strip(sep).split(sep)
    return (
        ext in (".h", ".cpp")
        and not base_name.endswith(".template")
        and not base_name.startswith("ilmbase_")
        and not contains_slice(dir_path, ("testenv",))
        and not contains_slice(dir_path, ("base", "js", "rapidjson"))
        and not (
            contains_slice(dir_path, ("base", "tf"))
            and file_name == "type_Impl.h"
        )
        and not (
            contains_slice(dir_path, ("usd", "ar"))
            and VERSIONED_FILE.search(base_name) is not None
        )
        and not (
            contains_slice(dir_path, ("usd", "sdf"))
            and file_name == "pathNode.h"
        )
        and not (
            contains_slice(dir_path, ("usd", "pcp"))
            and file_name == "dynamicFileFormatDependencyData.h"
        )
        and not (
            contains_slice(dir_path, ("usd", "usd", "codegenTemplates"))
        )
        and not (
            contains_slice(dir_path, ("usd", "usd"))
            and file_name in (
                "crateDataTypes.h",
                "crateValueInliners.h",
                "examples.cpp",
            )
        )
        and not (
            contains_slice(dir_path, ("usd", "plugin", "usdDraco"))
            and file_name == "flag.h"
        )
        and not (
            contains_slice(dir_path, ("imaging", "garch"))
            and file_name != "wrapPlatformDebugContext.cpp"
        )
        and not (
            contains_slice(dir_path, ("imaging", "hdSt"))
            and file_name in (
                "glConversions.h",
                "glslProgram.h",
                "points.h",
            )
        )
        and not (
            contains_slice(dir_path, ("imaging", "hgiInterop"))
        )
        and not (
            contains_slice(dir_path, ("imaging", "hgiMetal"))
        )
        and not (
            contains_slice(dir_path, ("imaging", "hgiVulkan"))
        )
    )


def filter_file_disambiguate_symbols(file_path):
    dir_path, file_name = split(file_path)
    base_name, ext = splitext(file_name)
    dir_path = dir_path.strip(sep).split(sep)
    return (
        filter_file_common(file_path)
        and not contains_slice(dir_path, ("base", "js"))
    )


def filter_file_inline_namespaces(file_path):
    dir_path, file_name = split(file_path)
    base_name, ext = splitext(file_name)
    dir_path = dir_path.strip(sep).split(sep)
    return (
        filter_file_common(file_path)
        and not (
            contains_slice(dir_path, ("base", "vt"))
            and file_name == "pyOperators.h"
        )
        and not (
            contains_slice(dir_path, ("usd", "pcp"))
            and file_name == "dynamicFileFormatContext.cpp"
        )
    )


FILTER_FILE_FN = {
    "disambiguate-symbols": filter_file_disambiguate_symbols,
    "inline-namespaces": filter_file_inline_namespaces,
}


def main(tool, path, modules):
    filter_file = FILTER_FILE_FN[tool]

    files = []
    for module in modules:
        for root, _, file_names in walk(join(path, module)):
            file_paths = (join(root, x) for x in sorted(file_names))
            file_paths = (x for x in file_paths if filter_file(x))
            files.extend(x for x in file_paths if x not in files)

    cmd = []
    cmd.append(join(EXECUTABLE_DIR, tool))
    cmd.extend(("-p", join(path, "build")))
    cmd.append("--overwrite")
    cmd.extend(("--root", path))

    if tool == "inline-namespaces":
        cmd.extend(("--file-pattern", join(path, "*")))

    cmd.extend(files)

    run(cmd)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "--tool",
        required=True,
        help="Either ‘inline-namespaces’ or ‘disambiguate-symbols’."
    )
    parser.add_argument(
        "--path",
        required=True,
        help="Path to USD's root directory."
    )
    parser.add_argument(
        "modules",
        nargs="*",
        default=["pxr"],
        help="USD modules to refactor, e.g.: ‘pxr/usd’ or ‘pxr/base/gf’",
    )
    args = parser.parse_args()

    main(args.tool, args.path, args.modules)
