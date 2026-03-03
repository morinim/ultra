#!/usr/bin/env python3
#
#  Copyright (C) 2026 EOS di Manlio Morini.
#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this file,
#  You can obtain one at http://mozilla.org/MPL/2.0/
#
#  Utilities for verifying/fixing ULTRA summary XML checksums.
#
#  Importable module + argparse CLI.

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
import xml.etree.ElementTree as ET
import zlib

CHECKSUM_LENGTH = 8
CHECKSUM_TAG = "checksum"


class UltraChecksumError(RuntimeError):
    pass


# ---------------------------------------------------------------------
#  CRC32 identical to the C++ code (ISO 3309 polynomial).
# ---------------------------------------------------------------------
def compute_crc32(data: str) -> str:
    crc = zlib.crc32(data.encode("utf-8")) & 0xFFFFFFFF
    return f"{crc:0{CHECKSUM_LENGTH}X}"


# ---------------------------------------------------------------------
#  Canonical (ElementTree-based) signing: use for generated XML.
#  This matches merge_summary.py's historical behaviour.
# ---------------------------------------------------------------------
def sign_etree_xml(xml: str, *, create: bool = False) -> str:
    """
    Signs XML using ElementTree canonicalisation (parse + tostring()).

    Intended for XML you generate programmatically (e.g. merge_summary.py),
    where you accept ElementTree's serialisation as canonical.
    """
    try:
        root = ET.fromstring(xml)
    except ET.ParseError as e:
        raise UltraChecksumError(f"XML not well-formed: {e}") from e

    node = root.find(CHECKSUM_TAG)
    if node is None:
        if not create:
            raise UltraChecksumError(f"Missing <{CHECKSUM_TAG}> tag")
        node = ET.SubElement(root, CHECKSUM_TAG)

    node.text = "0" * CHECKSUM_LENGTH
    temp = ET.tostring(root, encoding="unicode")

    node.text = compute_crc32(temp)
    return ET.tostring(root, encoding="unicode")


# ---------------------------------------------------------------------
#  Text-preserving verify/fix: use for existing files on disk.
# ---------------------------------------------------------------------
_CHECKSUM_RE = re.compile(
    rf"(<{CHECKSUM_TAG}\s*>)(.*?)(</{CHECKSUM_TAG}\s*>)",
    re.DOTALL | re.IGNORECASE,
)


def verify_text(xml: str) -> bool:
    """
    Verifies checksum against the original text, preserving formatting.

    This is the correct choice for validating existing files because any
    parse+tostring normalisation (e.g. '<solutions/>' -> '<solutions />')
    would change the CRC input.
    """
    m = _CHECKSUM_RE.search(xml)
    if not m:
        return False

    declared = m.group(2).strip().upper()
    if len(declared) != CHECKSUM_LENGTH:
        return False

    zeroed = xml[:m.start(2)] + ("0" * CHECKSUM_LENGTH) + xml[m.end(2):]
    return compute_crc32(zeroed) == declared


def fix_text(xml: str) -> str:
    """
    Fixes checksum in the original text, preserving formatting.
    Requires an existing <checksum> tag.
    """
    m = _CHECKSUM_RE.search(xml)
    if not m:
        raise UltraChecksumError(f"Missing <{CHECKSUM_TAG}> tag")

    zeroed = xml[:m.start(2)] + ("0" * CHECKSUM_LENGTH) + xml[m.end(2):]
    crc = compute_crc32(zeroed)

    return xml[:m.start(2)] + crc + xml[m.end(2):]


# ---------------------------------------------------------------------
#  CLI
# ---------------------------------------------------------------------
def _read_text(path: Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except FileNotFoundError as e:
        raise UltraChecksumError(f"{path}: file not found") from e
    except OSError as e:
        raise UltraChecksumError(f"{path}: cannot read file: {e}") from e


def _write_text(path: Path, text: str) -> None:
    try:
        path.write_text(text, encoding="utf-8")
    except OSError as e:
        raise UltraChecksumError(f"{path}: cannot write file: {e}") from e


def build_argparser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="checksum_summary.py",
        description="Verify or fix ULTRA summary XML CRC32 checksums.",
    )

    sub = p.add_subparsers(dest="cmd", required=True)

    v = sub.add_parser("verify", help="verify checksum (text-preserving)")
    v.add_argument("file", type=Path, help="input XML file")

    f = sub.add_parser("fix", help="fix checksum (text-preserving)")
    f.add_argument("file", type=Path, help="input XML file")
    f.add_argument(
        "--in-place",
        action="store_true",
        help="rewrite the input file in place (otherwise print to stdout)",
    )

    return p


def main(argv: list[str] | None = None) -> int:
    args = build_argparser().parse_args(argv)

    try:
        xml = _read_text(args.file)

        if args.cmd == "verify":
            ok = verify_text(xml)
            print("OK" if ok else "INVALID")
            return 0 if ok else 3

        if args.cmd == "fix":
            fixed = fix_text(xml)
            if args.in_place:
                _write_text(args.file, fixed)
                print("Checksum updated.")
            else:
                print(fixed, end="" if fixed.endswith("\n") else "\n")
            return 0

        # argparse should prevent this, but keep it defensive.
        raise UltraChecksumError(f"Unknown command: {args.cmd}")

    except UltraChecksumError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 2
    except Exception as e:
        print(f"Error: unexpected failure: {e}", file=sys.stderr)
        return 99


if __name__ == "__main__":
    raise SystemExit(main())
