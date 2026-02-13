#!/usr/bin/env python3

#
#  Copyright (C) 2025 EOS di Manlio Morini.
#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this file,
#  You can obtain one at http://mozilla.org/MPL/2.0/
#
#  A command line utility to merge ULTRA summary XML files.
#
#  It combines run counts, elapsed time, success rate, and pooled fitness
#  statistics, and rewrites best/solutions run indices with the proper offset.
#  The output is re-signed with a CRC32 checksum.
#
#  \see https://ultraevolution.org/blog/combine_test_batches/
#

import xml.etree.ElementTree as ET
import math
from pathlib import Path
import sys
import zlib


CHECKSUM_LENGTH = 8
CHECKSUM_TAG = "checksum"


class UltraParseError(RuntimeError):
    """Raised when an ULTRA summary XML is missing required content."""


# ---------------------------------------------------------------------
#  CRC32 identical to the C++ code (ISO 3309 polynomial)
# ---------------------------------------------------------------------

def compute_crc32(data: str) -> str:
    crc = zlib.crc32(data.encode("utf-8")) & 0xFFFFFFFF
    return f"{crc:0{CHECKSUM_LENGTH}X}"


def embed_xml_signature(xml: str) -> str:
    try:
        root = ET.fromstring(xml)
    except ET.ParseError as e:
        raise UltraParseError(f"Generated XML is not well-formed: {e}") from e

    node = root.find(CHECKSUM_TAG)

    if node is None:
        raise UltraParseError(f"No <{CHECKSUM_TAG}> tag found in generated XML")

    node.text = "0" * CHECKSUM_LENGTH

    temp = ET.tostring(root, encoding="unicode")

    # The output matches our C++ code exactly: Python's zlib.crc32 implements
    # the same ISO 3309 / IEEE 802.3 polynomial (0xEDB88320), with identical
    # initial/final XORs when masked with `0xFFFFFFFF`.
    node.text = compute_crc32(temp)

    return ET.tostring(root, encoding="unicode")


# ---------------------------------------------------------------------
#  Helpers for combining mean and standard deviation
# ---------------------------------------------------------------------

def combine_mean(m1: float, n1: int, m2: float, n2: int) -> float:
    """Pooled mean."""
    N = n1 + n2
    if N <= 0:
        raise ValueError("Cannot combine means with total count 0")
    return (n1 * m1 + n2 * m2) / N


def combine_std(m1: float, s1: float, n1: int,
                m2: float, s2: float, n2: int) -> float:
    """
    Unbiased pooled standard deviation.

    Defined only when total N >= 2 (denominator N-1).
    For N == 1, std is not identifiable; we return 0.0 by convention.
    """
    N = n1 + n2
    if N <= 0:
        raise ValueError("Cannot combine std dev with total count 0")
    if N == 1:
        return 0.0

    M = combine_mean(m1, n1, m2, n2)
    # If a batch has n=0, its (n-1)*s^2 term is 0 by definition here.
    num = (
        (max(n1 - 1, 0)) * (s1 * s1) +
        (max(n2 - 1, 0)) * (s2 * s2) +
        n1 * (m1 - M) ** 2 +
        n2 * (m2 - M) ** 2
    )
    return (num / (N - 1)) ** 0.5


# ---------------------------------------------------------------------
#  Parsing utilities
# ---------------------------------------------------------------------

def _require_node(parent: ET.Element, path: str, *, file: Path) -> ET.Element:
    node = parent.find(path)
    if node is None:
        raise UltraParseError(f"{file}: missing required node '{path}'")
    return node


def _require_text(parent: ET.Element, path: str, *, file: Path) -> str:
    node = _require_node(parent, path, file=file)
    if node.text is None:
        raise UltraParseError(f"{file}: node '{path}' has no text")
    text = node.text.strip()
    if not text:
        raise UltraParseError(f"{file}: node '{path}' is empty")
    return text


def _require_int(parent: ET.Element, path: str, *, file: Path) -> int:
    text = _require_text(parent, path, file=file)
    try:
        return int(text)
    except ValueError as e:
        raise UltraParseError(f"{file}: node '{path}' is not an int: {text!r}") from e


def _require_float(parent: ET.Element, path: str, *, file: Path) -> float:
    text = _require_text(parent, path, file=file)
    try:
        return float(text)
    except ValueError as e:
        raise UltraParseError(f"{file}: node '{path}' is not a float: {text!r}") from e


def parse_ultra_file(path: Path):
    try:
        tree = ET.parse(path)
    except FileNotFoundError as e:
        raise UltraParseError(f"{path}: file not found") from e
    except OSError as e:
        raise UltraParseError(f"{path}: cannot read file: {e}") from e
    except ET.ParseError as e:
        raise UltraParseError(f"{path}: XML parse error: {e}") from e

    root = tree.getroot()
    summary = root.find("summary")
    if summary is None:
        raise UltraParseError(f"{path}: missing required node 'summary'")

    runs = _require_int(summary, "runs", file=path)
    if runs <= 0:
        raise UltraParseError(f"{path}: runs must be positive")

    elapsed = _require_int(summary, "elapsed_time", file=path)
    if elapsed < 0:
        raise UltraParseError(f"{path}: elapsed_time must be non-negative")

    success = _require_float(summary, "success_rate", file=path)
    if not (0.0 <= success <= 1.0):
        raise UltraParseError(f"{path}: success_rate out of range: {success}")

    mean = _require_float(summary, "distributions/fitness/mean", file=path)
    if not math.isfinite(mean):
        raise UltraParseError(f"{path}: mean must be finite, got {mean}")

    std  = _require_float(summary, "distributions/fitness/standard_deviation",
                          file=path)
    if not math.isfinite(std) or std < 0.0:
        raise UltraParseError(
            f"{path}: standard_deviation must be finite and non-negative, got {std}")

    best_fitness = _require_float(summary, "best/fitness", file=path)
    best_accuracy = _require_float(summary, "best/accuracy", file=path)

    best_run = _require_int(summary, "best/run", file=path)
    if best_run < 0 or best_run >= runs:
        raise UltraParseError(f"{path}: best/run out of range: {best_run} (runs={runs})")

    best_code = _require_text(summary, "best/code", file=path)

    sol_parent = summary.find("solutions")
    if sol_parent is None:
        raise UltraParseError(f"{path}: missing required node 'solutions'")

    solutions = []
    for node in sol_parent.findall("run"):
        if node.text is None or not node.text.strip():
            raise UltraParseError(f"{path}: empty <solutions><run> entry")
        try:
            solutions.append(int(node.text.strip()))
        except ValueError as e:
            raise UltraParseError(f"{path}: non-integer solution run: {node.text!r}") from e
    if any(s < 0 for s in solutions):
        raise UltraParseError(f"{path}: solutions contains negative run index")
    if any(s >= runs for s in solutions):
        raise UltraParseError(f"{path}: solutions contains run index >= runs")
    if len(set(solutions)) != len(solutions):
        raise UltraParseError(f"{path}: duplicate run indices in solutions")

    return {
        "runs": runs,
        "elapsed": elapsed,
        "success": success,
        "mean": mean,
        "std": std,
        "best_fitness": best_fitness,
        "best_accuracy": best_accuracy,
        "best_run": best_run,
        "best_code": best_code,
        "solutions": solutions,
        "tree": tree,
        "root": root,
    }


# ---------------------------------------------------------------------
#  Main merge function
# ---------------------------------------------------------------------

def merge_ultra_files(path1: Path, path2: Path, output: Path):
    A = parse_ultra_file(path1)
    B = parse_ultra_file(path2)

    # Merged summary.
    runs = A["runs"] + B["runs"]

    if runs == 0:
        raise UltraParseError("Merged runs is 0; cannot compute merged statistics")

    elapsed = A["elapsed"] + B["elapsed"]

    success = (A["success"] * A["runs"] + B["success"] * B["runs"]) / runs

    mean = combine_mean(A["mean"], A["runs"],
                        B["mean"], B["runs"])

    std = combine_std(A["mean"], A["std"], A["runs"],
                      B["mean"], B["std"], B["runs"])

    # Best.
    if A["best_fitness"] >= B["best_fitness"]:
        best_fitness = A["best_fitness"]
        best_accuracy = A["best_accuracy"]
        best_run = A["best_run"]
        best_code = A["best_code"]
    else:
        best_fitness = B["best_fitness"]
        best_accuracy = B["best_accuracy"]
        best_run = B["best_run"] + A["runs"]  # important offset
        best_code = B["best_code"]

    # Solutions: merge + offset for B
    solutions = A["solutions"] + [s + A["runs"] for s in B["solutions"]]

    # -----------------------------------------------------------------
    # Build XML
    # -----------------------------------------------------------------
    ultra = ET.Element("ultra")
    summary = ET.SubElement(ultra, "summary")

    ET.SubElement(summary, "runs").text = str(runs)
    ET.SubElement(summary, "elapsed_time").text = str(elapsed)
    ET.SubElement(summary, "success_rate").text = f"{success:.12g}"

    dist = ET.SubElement(summary, "distributions")
    fit = ET.SubElement(dist, "fitness")
    ET.SubElement(fit, "mean").text = f"{mean:.12g}"
    ET.SubElement(fit, "standard_deviation").text = f"{std:.12g}"

    best = ET.SubElement(summary, "best")
    ET.SubElement(best, "fitness").text = f"{best_fitness:.12g}"
    ET.SubElement(best, "accuracy").text = f"{best_accuracy:.12g}"
    ET.SubElement(best, "run").text = str(best_run)
    ET.SubElement(best, "code").text = best_code

    sol = ET.SubElement(summary, "solutions")
    for s in solutions:
        ET.SubElement(sol, "run").text = str(s)

    # Placeholder checksum
    ET.SubElement(ultra, "checksum").text = "0" * CHECKSUM_LENGTH

    # Convert to string
    ET.indent(ultra, space="  ")
    xml = ET.tostring(ultra, encoding="unicode")

    # Embed real checksum
    xml_signed = embed_xml_signature(xml)

    # Save
    output.write_text(xml_signed, encoding="utf-8")

    return xml_signed


# ---------------------------------------------------------------------
#  Example command-line usage
# ---------------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python merge_summary.py summary1.xml summary2.xml "
              "output.xml",
              file=sys.stderr)
        sys.exit(1)

    try:
        merge_ultra_files(Path(sys.argv[1]), Path(sys.argv[2]),
                          Path(sys.argv[3]))
        print("Merged file written.")
    except UltraParseError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)
    except Exception as e:
        print(f"Error: unexpected failure: {e}", file=sys.stderr)
        sys.exit(3)
