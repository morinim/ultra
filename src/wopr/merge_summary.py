#!/usr/bin/env python3

#
#  Copyright (C) 2025 EOS di Manlio Morini.
#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this file,
#  You can obtain one at http://mozilla.org/MPL/2.0/
#
#  A command line utility to simplify xrff preprocessing.
#
#  \see https://ultraevolution.org/blog/combine_test_batches/
#

import xml.etree.ElementTree as ET
from pathlib import Path
import sys
import zlib


CHECKSUM_LENGTH = 8
CHECKSUM_TAG = "checksum"


# ---------------------------------------------------------------------
#  CRC32 identical to the C++ code (ISO 3309 polynomial)
# ---------------------------------------------------------------------

def compute_crc32(data: str) -> str:
    crc = zlib.crc32(data.encode("utf-8")) & 0xFFFFFFFF
    return f"{crc:0{CHECKSUM_LENGTH}X}"


def embed_xml_signature(xml: str) -> str:
    root = ET.fromstring(xml)
    node = root.find(CHECKSUM_TAG)

    if node is None:
        raise ValueError("No <" + CHECKSUM_TAG + "> tag found")

    node.text = "0" * CHECKSUM_LENGTH

    temp = ET.tostring(root, encoding="unicode")

    # The output matches our C++ code exactly: Python's zlib.crc32 implements
    # the same ISO 3309 / IEEE 802.3 polynomial (0xEDB88320), with identical
    # initial/final XORs when masked with `0xFFFFFFFF`.
    crc = compute_crc32(temp)

    node.text = crc

    return ET.tostring(root, encoding="unicode")


# ---------------------------------------------------------------------
#  Helpers for combining mean and standard deviation
# ---------------------------------------------------------------------

def combine_mean(m1, n1, m2, n2):
    """Pooled mean."""
    return (n1 * m1 + n2 * m2) / (n1 + n2)


def combine_std(m1, s1, n1, m2, s2, n2):
    """Unbiased pooled standard deviation."""
    M = combine_mean(m1, n1, m2, n2)
    num = (
        (n1 - 1) * s1 * s1 +
        (n2 - 1) * s2 * s2 +
        n1 * (m1 - M)**2 +
        n2 * (m2 - M)**2
    )
    return (num / (n1 + n2 - 1)) ** 0.5


# ---------------------------------------------------------------------
#  Parsing utilities
# ---------------------------------------------------------------------

def parse_ultra_file(path: Path):
    tree = ET.parse(path)
    root = tree.getroot()
    summary = root.find("summary")

    runs = int(summary.find("runs").text)
    elapsed = int(summary.find("elapsed_time").text)
    success = float(summary.find("success_rate").text)

    fitness_node = summary.find("distributions/fitness")
    mean = float(fitness_node.find("mean").text)
    std = float(fitness_node.find("standard_deviation").text)

    best = summary.find("best")
    best_fitness = float(best.find("fitness").text)
    best_accuracy = float(best.find("accuracy").text)
    best_run = int(best.find("run").text)
    best_code = best.find("code").text.strip()

    solutions = [int(node.text) for node in summary.find("solutions")]

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
    elapsed = A["elapsed"] + B["elapsed"]

    success = (A["success"] * A["runs"] + B["success"] * B["runs"]) / runs

    mean = combine_mean(A["mean"], A["std"], A["runs"],
                        B["mean"], B["std"], B["runs"])

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

    merge_ultra_files(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))
    print("Merged file written.")
