#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Evaluate the tests."""

from argparse import ArgumentParser
from collections import namedtuple
from difflib import unified_diff
from os import (
    pardir,
    scandir,
)
from os.path import (
    abspath,
    basename,
    dirname,
    join,
    splitext,
)
from subprocess import (
    DEVNULL,
    PIPE,
    run,
)


ROOT_DIR = abspath(join(dirname(__file__), pardir))
TEST_DIR = join(ROOT_DIR, "tests")
EXECUTABLE = join(ROOT_DIR, "build", "bin", "inline-namespaces")

Test = namedtuple("Test", ("name", "original", "expected"))


def run_test(test, path, verbose):
    cmd = []
    cmd.append(EXECUTABLE)
    cmd.append("--dump")
    cmd.extend(("-p", join(path, "build")))
    cmd.extend(("--file-pattern", join(path, "*")))
    cmd.append(test.original)

    if verbose:
        title = "Running test ‘{}’ ".format(test.name)
        print("\n{} {:=<74}\n".format("=" * 5, title))
        print(" ".join(cmd))
        print("")

    result = run(cmd, stdout=PIPE, stderr=None if verbose else DEVNULL)
    if result.returncode:
        raise RuntimeError("Error while refactoring the file")

    if result.stdout:
        modified = result.stdout.decode("utf-8")
        modified = modified.split("\n")[1:-2]
    else:
        # No changes were made.
        with open(test.original) as file:
            modified = file.read()
            modified = modified.split("\n")

    with open(test.expected) as file:
        expected = file.read()
        expected = expected.split("\n")

    diffs = unified_diff(modified, expected)
    diffs = "\n".join(diffs)
    if not diffs:
        return []

    title = "Diff for test ‘{}’ ".format(test.name)
    return [
        "\n{} {:=<74}\n".format("=" * 5, title),
        "~" * 80,
        diffs,
        "~" * 80,
    ]


def main(path, tool_names, test_names, verbose):
    tools = tuple(
        x for x in scandir(TEST_DIR) if not tool_names or x.name in tool_names
    )

    tests = []
    for tool in tools:
        for entry in scandir(tool):
            if (
                not entry.is_dir()
                or (test_names and entry.name not in test_names)
            ):
                continue

            files = {splitext(basename(x))[0]: x.path for x in scandir(entry)}
            test = Test(name=entry.name, **files)
            tests.append(test)

    outputs = []
    for test in sorted(tests):
        outputs += run_test(test, path, verbose)

    if outputs:
        print("\n".join(outputs))


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "-p",
        "--path",
        required=True,
        help="Path to USD's root directory."
    )
    parser.add_argument(
        "--tool",
        action="append",
        help="Tools to test."
    )
    parser.add_argument(
        "--test",
        action="append",
        help="Test names to run."
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Display verbose logs."
    )
    args = parser.parse_args()

    path = args.path
    tool_names = [] if "*" in args.tool else args.tool
    test_names = [] if "*" in args.test else args.test
    verbose = args.verbose

    main(path, tool_names, test_names, verbose)
