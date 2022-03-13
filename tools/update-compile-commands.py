#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Prepend USD's source directory into the list of included directories.

This is not necessary but it is helpful for debugging, in case of mishaps from
the refactoring tool which could try in some cases to fix a different file from
the one being currently matched, in which case Clang would apply the fix to
that different file using the include paths given in the ‘compile_commands.json’
file, thus leading to some header files from USD's build directory being
silently edited instead of the ones in USD's source directory.
"""

from argparse import ArgumentParser
import json
from os.path import (
    abspath,
    dirname,
    join,
    split,
    splitext,
)

def main(path):
    path = abspath(path)
    build_path = join(path, "build")
    file_path = join(build_path, "compile_commands.json")

    with open(file_path, "r", encoding="utf-8") as file:
        data = json.load(file)

    src_include_flag = "-I{}".format(path)
    build_include_flag = "-I{}".format(join(build_path, "include"))
    for entry in data:
        command = entry["command"].split()
        idx = None
        for i, part in enumerate(command):
            if part == build_include_flag:
                idx = i
                break

        if idx is not None:
            command.insert(idx, src_include_flag)

        entry["command"] = " ".join(command)

    with open(file_path, "w", encoding="utf-8") as file:
        json.dump(data, file, indent=4)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "-p",
        "--path",
        required=True,
        help="Path to USD's root directory."
    )
    args = parser.parse_args()

    main(args.path)
