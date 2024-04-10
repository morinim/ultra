#!/usr/bin/env python3
#
#  Copyright (C) 2024 EOS di Manlio Morini.
#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this file,
#  You can obtain one at http://mozilla.org/MPL/2.0/
#
#  A python program that helps to set up a new version of Ultra.
#

import argparse
import datetime
import os
import re


def make_version(l):
    if len(l) == 1:
        regex = r"^[vV]?[0\s]*(\d+)\s*\.[0\s]*(\d+)\s*\.[0\s]*(\d+)\s*$"

        if not re.match(regex, l[0]):
            print("Invalid version format")
            exit(1)

        subst = r"\g<1>.\g<2>.\g<3>"
        return re.sub(regex, subst, l[0])
    elif len(l) == 3:
        if l[0].isdigit() and l[1].isdigit() and l[2].isdigit():
            return l[0] + "." + l[1] + "." + l[2]

        print("Invalid version format")
        exit(1)

    print("Invalid number of arguments")
    exit(1)


def file_process(name, rule, args):
    print("--- Processing " + os.path.basename(name))
    with open(name) as source:
        data = rule(source.read(), args)

    if not data:
        return

    print("Writing " + name)
    with open(name) as dest:
        dest = open(name, "w")
        dest.write(data)


def changelog_rule(data, new_version):
    regex = r"## \[Unreleased\]"
    subst = r"## [Unreleased]\n\n## [" + new_version + r"] - " + datetime.date.today().isoformat()

    result = re.subn(regex, subst, data)
    if result[1] != 1:
        return None

    regex = r"(\[Unreleased)(\]: https://github.com/morinim/ultra/compare/v)(.+)(\.\.\.HEAD)"
    subst = r"\g<1>\g<2>" + new_version + r"\g<4>\n[" + new_version + r"\g<2>\g<3>...v" + new_version

    result = re.subn(regex, subst, result[0])

    return result[0] if result[1] == 1 else None


def doxygen_rule(data, new_version):
    regex = r"([\s]+)(\*[\s]+\\mainpage ULTRA v)([\d]+)\.([\d]+)\.([\d]+)([\s]*)"
    subst = r"\g<1>\g<2>" + new_version + r"\g<6>"

    result = re.subn(regex, subst, data)

    return result[0] if result[1] > 0 else None


def get_cmd_line_options():
    description = "Helps to set up a new version of Ultra"
    usage = "\n1. %(prog)s [-h] [-v] version\n2. %(prog)s [-h] [-v] major minor maintenance"
    parser = argparse.ArgumentParser(description = description, usage = usage)

    parser.add_argument("-v", "--verbose", action = "store_true",
                        help = "Turn on verbose mode")

    # Now the positional arguments.
    parser.add_argument("version", nargs="+")
    return parser


def main():
    args = get_cmd_line_options().parse_args()

    version = make_version(args.version);

    dirname = os.path.dirname(__file__)

    #file_process(os.path.join(dirname, "../NEWS.md"), changelog_rule, version)
    file_process(os.path.join(dirname, "../doc/doxygen/doxygen.h"),
                 doxygen_rule, version)

    print("\n\nRELEASE NOTE\n")
    print("1. Build.  cmake -B build/ src/ ; cmake --build build/")
    print("2. Check.  cd build/test ; ./tests")
    print('3. Commit. git commit -am "doc: change revision number v'
          + version + '"')
    print("4. Tag.    git tag -a v" + version + " -m \"tag message\"")
    print("\nRemember to 'git push' both code and tag. For the tag:\n")
    print("   git push origin [tagname]\n")


if __name__ == "__main__":
    main()
