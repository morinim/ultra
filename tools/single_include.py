#!/usr/bin/env python3

# Copyright 2019-2020 Rene Ferdinand Rivera Morell
# Distributed under the Boost Software License, Version 1.0.
# (see https://www.boost.org/LICENSE_1_0.txt)
#
# Original file:
# https://github.com/bfgroup/Lyra/blob/develop/tools/gen_single_include.py
#
# Adapted by Manlio Morini for use in Ultra.

import argparse
import os.path
import re

# Requires 3 command-line arguments:
# * `--src-include` the include file to convert, i.e. to merge its include
#   directives into its body
# * `--dst-include` the output file to write
# * `--src-include-dir` the directory relative to which include files are
#   specified (i.e. an "include search path" of one directory; the script
#   doesn't support the complex mechanism of multiple include paths and search
#   priorities which the C++ compiler offers)
class GenSingleInclude(object):

    def __init__(self):
        # The original r.e. was `r'''#\s*include\s+["<]([^">]+)[">]\s*'''`
        self.pp_re = re.compile(r'''#\s*include\s+["]([^">]+)["]\s*''')
        parser = argparse.ArgumentParser()
        parser.add_argument('--src-include-dir', required=True)
        parser.add_argument('--src-include', required=True)
        parser.add_argument('--dst-include', required=True)
        self.args = parser.parse_args()
        if not os.path.isabs(self.args.src_include_dir):
            self.args.src_include_dir = os.path.abspath(
                os.path.join(os.curdir, self.args.src_include_dir))
        self.parsed = set()
        self.run()

    def run(self):
        with open(self.args.dst_include, "w", encoding="UTF8") as dst_file:
            dst_file.write('''/**
 *  Automatically generated global include file for ULTRA
 *  (https://github.com/morinim/ultra).
 *  This is for convenience only and should not be edited.
 *
 *  Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  License
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */
''')
            self.cpp(dst_file, None, self.args.src_include)

    def resolve_include(self, cur_dir, include_name):
        if os.path.isabs(include_name):
            return include_name
        search_dirs = []
        if cur_dir:
            search_dirs.append(cur_dir)
        search_dirs.append(self.args.src_include_dir)
        search_dirs.append(os.path.join(self.args.src_include_dir, "third_party"))
        for search_dir in search_dirs:
            include_path = os.path.join(search_dir, include_name)
            if os.path.exists(include_path):
                return os.path.abspath(include_path)
        return None

    # See https://stackoverflow.com/a/241506/3235496
    def comment_remover(self, text):
        """ Remove comments.
        text: text with comments (can include newlines)
        returns: text with comments removed
        """

        def replacer(match):
            s = match.group(0)
            if s.startswith('/'):
                # Note: a space and NOT an empty string (otherwise the legal
                # expression `int/**/x=5;` would become `intx=5;` which doesn't
                # compile.
                return " "
            else:
                return s

        pattern = re.compile(
            r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
            re.DOTALL | re.MULTILINE
        )
        return re.sub(pattern, replacer, text)

    def cpp(self, dst_file, cur_include_dir, src_include):
        src_i = self.resolve_include(cur_include_dir, src_include)
        if not src_i:
            search_dirs = [self.args.src_include_dir,
                           os.path.join(self.args.src_include_dir, "third_party")]
            if cur_include_dir:
                search_dirs.insert(0, cur_include_dir)
            raise FileNotFoundError(
                f'cannot resolve include "{src_include}" '
                f'(searched: {", ".join(search_dirs)})')
        if src_i not in self.parsed:
            self.parsed.add(src_i)
            with open(src_i, "r", encoding="UTF8") as raw_src_file:
                src_file = self.comment_remover(raw_src_file.read())

                for line in src_file.splitlines():
                    pp_match = self.pp_re.fullmatch(line)
                    src_n = None
                    if pp_match:
                        src_n = self.cpp(dst_file, os.path.dirname(
                            src_i), pp_match.group(1))
                    if not src_n:
                        dst_file.write(line + os.linesep)
        return src_i


if __name__ == "__main__":
    GenSingleInclude()
