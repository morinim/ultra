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
import sys
from typing import Callable, Optional


def die(msg: str, code: int = 1) -> None:
    print(msg, file=sys.stderr)
    sys.exit(code)


def make_version(parts: list[str]) -> str:
    if len(parts) == 1:
        # Accept: 1.2.3, v1.2.3, V01.002.0003, with optional spaces.
        regex = r"^[vV]?\s*0*(\d+)\s*\.\s*0*(\d+)\s*\.\s*0*(\d+)\s*$"
        m = re.match(regex, parts[0])
        if not m:
            die("Invalid version format (expected MAJOR.MINOR.PATCH or vMAJOR.MINOR.PATCH)")
        return f"{m.group(1)}.{m.group(2)}.{m.group(3)}"

    if len(parts) == 3 and all(p.isdigit() for p in parts):
        return f"{int(parts[0])}.{int(parts[1])}.{int(parts[2])}"

    die("Invalid version format (use either: version OR major minor patch)")
    return ""  # unreachable


def file_process(path: str, rule: Callable[[str, str], Optional[str]],
                 rule_arg: str) -> None:
    print(f"--- Processing {os.path.basename(path)}")

    with open(path, "r", encoding="utf-8") as f:
        original = f.read()

    updated = rule(original, rule_arg)
    if updated is None or updated == original:
        print("No changes.")
        return

    print(f"Writing {path}")
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(updated)


def changelog_rule(data: str, new_version: str) -> Optional[str]:
    today = datetime.date.today().isoformat()

    # Insert new section after "Unreleased"
    data2, n1 = re.subn(
        r"^## \[Unreleased\]\s*$",
        f"## [Unreleased]\n\n## [{new_version}] - {today}",
        data,
        count=1,
        flags=re.MULTILINE,
    )
    if n1 != 1:
        return None

    # Update compare links at bottom
    # [Unreleased]: ...compare/vX.Y.Z...HEAD
    # [X.Y.Z]: ...compare/vOLD...vNEW
    data3, n2 = re.subn(
        r"^\[Unreleased\]:\s*(https://github\.com/morinim/ultra/compare/v)(\S+)(\.\.\.HEAD)\s*$",
        rf"[Unreleased]: \g<1>{new_version}\g<3>\n[{new_version}]: \g<1>\g<2>...v{new_version}",
        data2,
        count=1,
        flags=re.MULTILINE,
    )

    return data3 if n2 == 1 else None


def doxygen_rule(data: str, new_version: str) -> Optional[str]:
    # Replace the whole line, keep spacing stable.
    regex = r"^(PROJECT_NUMBER\s*=\s*).*$"
    subst = r"\g<1>" + new_version

    result = re.subn(regex, subst, data, flags=re.MULTILINE)
    return result[0] if result[1] == 1 else None


def get_cmd_line_options() -> argparse.ArgumentParser:
    description = "Helps to set up a new version of Ultra"
    usage = "\n1. %(prog)s [-h] [-v] version\n2. %(prog)s [-h] major minor patch"
    parser = argparse.ArgumentParser(description=description, usage=usage)

    parser.add_argument("version", nargs="+")
    return parser


def main() -> None:
    args = get_cmd_line_options().parse_args()
    version = make_version(args.version)

    dirname = os.path.dirname(__file__)

    # file_process(os.path.join(dirname, "../NEWS.md"), changelog_rule,
    #              version)
    file_process(os.path.join(dirname, "../doc/doxygen/ultra"),
                 doxygen_rule, version)


    print("\n\nRELEASE NOTE\n")
    print("1. Build.  cmake --preset=clang_sanitize_address src/ ; cmake --build build/")
    print("2. Check.  cd build/test ; ctest")
    print(f'3. Commit. git commit -am "docs: update version to v{version}"')
    print(f'4. Tag.    git tag -a v{version} -m "tag message"')
    print("\nRemember to 'git push' both code and tag. For the tag:\n")
    print("   git push origin [tagname]\n")


if __name__ == "__main__":
    main()
