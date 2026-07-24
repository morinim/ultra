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

from __future__ import annotations

import argparse
from dataclasses import dataclass
import math
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
from checksum_summary import sign_etree_xml


class UltraParseError(RuntimeError):
    """Raised when an ULTRA summary XML is missing required content."""


# ---------------------------------------------------------------------
#  Helpers for combining mean and standard deviation.
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
#  Parsing utilities.
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


def _parse_percentile_attr(elite_node: ET.Element, *, file: Path) -> float:
    """
    Returns elite fraction in [0, 1]. Accepts either:
      - "0.05" (fraction)
      - "5" or "5.0" (percent)
    """
    raw = elite_node.get("percentile")
    if raw is None:
        raise UltraParseError(
            f"{file}: <elite> missing required attribute 'percentile'")
    try:
        v = float(raw)
    except ValueError as e:
        raise UltraParseError(
            f"{file}: <elite percentile> is not numeric: {raw!r}") from e

    # If it looks like a percentage (e.g. 5), convert to fraction.
    frac = v / 100.0 if v > 1.0 else v
    if not 0.0 <= frac <= 1.0:
        raise UltraParseError(f"{file}: elite percentile out of range: {raw!r}")
    return frac


def _format_percentile_attr(frac: float) -> str:
    """
    Writes as a human-friendly percentage number (e.g. 5 for 5%),
    matching your example XML.
    """
    pct = frac * 100.0
    # Avoid trailing .0 where possible.
    return f"{pct:.12g}"


@dataclass(frozen=True)
class EliteRun:
    id: int
    fitness: float | None
    accuracy: float | None

    def offset(self, amount: int) -> EliteRun:
        return EliteRun(self.id + amount, self.fitness, self.accuracy)

    def sort_key(self) -> tuple[float, float]:
        """Returns a key that places missing or non-finite values last."""
        fitness = (self.fitness if self.fitness is not None
                   and math.isfinite(self.fitness) else float("-inf"))
        accuracy = (self.accuracy if self.accuracy is not None
                    and math.isfinite(self.accuracy) else float("-inf"))
        return fitness, accuracy


@dataclass(frozen=True)
class Elite:
    percentile: float
    runs: list[EliteRun]

    @classmethod
    def parse(cls, summary: ET.Element, runs: int, *,
              file: Path) -> Elite | None:
        elite_node = summary.find("elite")
        if elite_node is None:
            return None

        items: list[EliteRun] = []
        for run_node in elite_node.findall("run"):
            raw_id = run_node.get("id")
            if raw_id is None:
                raise UltraParseError(
                    f"{file}: <elite><run> missing required attribute 'id'")
            try:
                run_id = int(raw_id)
            except ValueError as e:
                raise UltraParseError(
                    f"{file}: elite run id is not an int: {raw_id!r}") from e

            if run_id < 0 or run_id >= runs:
                raise UltraParseError(
                    f"{file}: elite run id out of range: {run_id} "
                    f"(runs={runs})")

            fitness = cls._optional_float(run_node, "fitness", file)
            accuracy = cls._optional_float(run_node, "accuracy", file)
            items.append(EliteRun(run_id, fitness, accuracy))

        ids = [item.id for item in items]
        if len(set(ids)) != len(ids):
            raise UltraParseError(f"{file}: duplicate ids in <elite>")

        return cls(_parse_percentile_attr(elite_node, file=file), items)

    @staticmethod
    def _optional_float(node: ET.Element, name: str,
                        file: Path) -> float | None:
        value = node.find(name)
        if value is None or value.text is None or not value.text.strip():
            return None
        try:
            return float(value.text.strip())
        except ValueError as e:
            raise UltraParseError(
                f"{file}: elite/{name} is not a float: {value.text!r}") from e

    @classmethod
    def merge(cls, first: Elite | None, second: Elite | None, *,
              offset: int, total_runs: int) -> Elite | None:
        if first is None and second is None:
            return None

        if first is not None:
            percentile = first.percentile
        else:
            assert second is not None
            percentile = second.percentile
        if (first is not None and second is not None
                and abs(first.percentile - second.percentile) > 1e-12):
            raise UltraParseError(
                "Elite percentile mismatch: "
                f"{first.percentile} vs {second.percentile}")

        candidates = list(first.runs) if first is not None else []
        if second is not None:
            candidates.extend(run.offset(offset) for run in second.runs)

        if percentile == 0.0:
            return cls(percentile, [])

        count = max(1, min(int(total_runs * percentile), total_runs))
        candidates.sort(key=EliteRun.sort_key, reverse=True)

        selected: list[EliteRun] = []
        seen: set[int] = set()
        for run in candidates:
            if run.id in seen:
                continue
            seen.add(run.id)
            selected.append(run)
            if len(selected) >= count:
                break

        return cls(percentile, selected)

    def append_xml(self, summary: ET.Element) -> None:
        elite = ET.SubElement(summary, "elite")
        elite.set("percentile", _format_percentile_attr(self.percentile))
        for run in self.runs:
            run_node = ET.SubElement(elite, "run", id=str(run.id))
            if run.fitness is not None:
                ET.SubElement(run_node, "fitness").text = (
                    f"{run.fitness:.12g}"
                )
            if run.accuracy is not None:
                ET.SubElement(run_node, "accuracy").text = (
                    f"{run.accuracy:.12g}"
                )


@dataclass(frozen=True)
class Best:
    fitness: float
    accuracy: float
    run: int
    code: str

    def offset(self, amount: int) -> Best:
        return Best(self.fitness, self.accuracy, self.run + amount, self.code)


@dataclass(frozen=True)
class Summary:
    runs: int
    elapsed: int
    success: float
    mean: float
    std: float
    best: Best
    solutions: list[int]
    elite: Elite | None

    @classmethod
    def parse(cls, path: Path) -> Summary:
        try:
            tree = ET.parse(path)
        except FileNotFoundError as e:
            raise UltraParseError(f"{path}: file not found") from e
        except OSError as e:
            raise UltraParseError(f"{path}: cannot read file: {e}") from e
        except ET.ParseError as e:
            raise UltraParseError(f"{path}: XML parse error: {e}") from e

        summary = tree.getroot().find("summary")
        if summary is None:
            raise UltraParseError(
                f"{path}: missing required node 'summary'")

        runs = _require_int(summary, "runs", file=path)
        if runs <= 0:
            raise UltraParseError(f"{path}: runs must be positive")

        elapsed = _require_int(summary, "elapsed_time", file=path)
        if elapsed < 0:
            raise UltraParseError(
                f"{path}: elapsed_time must be non-negative")

        success = _require_float(summary, "success_rate", file=path)
        if not 0.0 <= success <= 1.0:
            raise UltraParseError(
                f"{path}: success_rate out of range: {success}")

        mean = _require_float(
            summary, "distributions/fitness/mean", file=path)
        if not math.isfinite(mean):
            raise UltraParseError(
                f"{path}: mean must be finite, got {mean}")

        std = _require_float(
            summary, "distributions/fitness/standard_deviation", file=path)
        if not math.isfinite(std) or std < 0.0:
            raise UltraParseError(
                f"{path}: standard_deviation must be finite and "
                f"non-negative, got {std}")

        best = cls._parse_best(summary, runs, path)
        solutions = cls._parse_solutions(summary, runs, path)
        return cls(runs, elapsed, success, mean, std, best, solutions,
                   Elite.parse(summary, runs, file=path))

    @staticmethod
    def _parse_best(summary: ET.Element, runs: int, path: Path) -> Best:
        fitness = _require_float(summary, "best/fitness", file=path)
        if not math.isfinite(fitness):
            raise UltraParseError(
                f"{path}: best/fitness must be finite, got {fitness}")

        accuracy = _require_float(summary, "best/accuracy", file=path)
        if not math.isfinite(accuracy) or not 0.0 <= accuracy <= 1.0:
            raise UltraParseError(
                f"{path}: best/accuracy out of range: {accuracy} "
                "(expected 0..1)")

        run = _require_int(summary, "best/run", file=path)
        if run < 0 or run >= runs:
            raise UltraParseError(
                f"{path}: best/run out of range: {run} (runs={runs})")

        return Best(fitness, accuracy, run,
                    _require_text(summary, "best/code", file=path))

    @staticmethod
    def _parse_solutions(summary: ET.Element, runs: int,
                         path: Path) -> list[int]:
        parent = summary.find("solutions")
        if parent is None:
            raise UltraParseError(
                f"{path}: missing required node 'solutions'")

        solutions = []
        for node in parent.findall("run"):
            if node.text is None or not node.text.strip():
                raise UltraParseError(
                    f"{path}: empty <solutions><run> entry")
            try:
                solutions.append(int(node.text.strip()))
            except ValueError as e:
                raise UltraParseError(
                    f"{path}: non-integer solution run: {node.text!r}") from e

        if any(run < 0 for run in solutions):
            raise UltraParseError(
                f"{path}: solutions contains negative run index")
        if any(run >= runs for run in solutions):
            raise UltraParseError(
                f"{path}: solutions contains run index >= runs")
        if len(set(solutions)) != len(solutions):
            raise UltraParseError(
                f"{path}: duplicate run indices in solutions")
        return solutions

    def merge(self, other: Summary) -> Summary:
        runs = self.runs + other.runs
        if runs == 0:
            raise UltraParseError(
                "Merged runs is 0; cannot compute merged statistics")

        best = (self.best if self.best.fitness >= other.best.fitness
                else other.best.offset(self.runs))
        solutions = self.solutions + [
            run + self.runs for run in other.solutions
        ]
        return Summary(
            runs=runs,
            elapsed=self.elapsed + other.elapsed,
            success=(self.success * self.runs
                     + other.success * other.runs) / runs,
            mean=combine_mean(
                self.mean, self.runs, other.mean, other.runs),
            std=combine_std(
                self.mean, self.std, self.runs,
                other.mean, other.std, other.runs),
            best=best,
            solutions=solutions,
            elite=Elite.merge(
                self.elite, other.elite,
                offset=self.runs, total_runs=runs),
        )

    def to_xml(self) -> str:
        ultra = ET.Element("ultra")
        summary = ET.SubElement(ultra, "summary")
        ET.SubElement(summary, "runs").text = str(self.runs)
        ET.SubElement(summary, "elapsed_time").text = str(self.elapsed)
        ET.SubElement(summary, "success_rate").text = f"{self.success:.12g}"

        distributions = ET.SubElement(summary, "distributions")
        fitness = ET.SubElement(distributions, "fitness")
        ET.SubElement(fitness, "mean").text = f"{self.mean:.12g}"
        ET.SubElement(fitness, "standard_deviation").text = (
            f"{self.std:.12g}"
        )

        best = ET.SubElement(summary, "best")
        ET.SubElement(best, "fitness").text = f"{self.best.fitness:.12g}"
        ET.SubElement(best, "accuracy").text = f"{self.best.accuracy:.12g}"
        ET.SubElement(best, "run").text = str(self.best.run)
        ET.SubElement(best, "code").text = self.best.code

        solutions = ET.SubElement(summary, "solutions")
        for run in self.solutions:
            ET.SubElement(solutions, "run").text = str(run)

        if self.elite is not None:
            self.elite.append_xml(summary)

        ET.indent(ultra, space="  ")
        return sign_etree_xml(
            ET.tostring(ultra, encoding="unicode"), create=True)


# ---------------------------------------------------------------------
#  Directory-related helpers
# ---------------------------------------------------------------------
def iter_summary_files(d: Path):
    if not d.exists():
        raise UltraParseError(f"{d}: directory does not exist")
    if not d.is_dir():
        raise UltraParseError(f"{d}: not a directory")

    yield from sorted(p for p in d.iterdir()
                      if p.is_file() and p.name.endswith(".summary.xml"))


def index_by_name(d: Path) -> dict[str, Path]:
    out: dict[str, Path] = {}
    for p in iter_summary_files(d):
        name = p.name
        if name in out:
            raise UltraParseError(f"{d}: duplicate filename {name!r}")
        out[name] = p
    return out


# ---------------------------------------------------------------------
#  Main merge functions.
# ---------------------------------------------------------------------

def merge_ultra_files(path1: Path, path2: Path, output: Path) -> str:
    xml = Summary.parse(path1).merge(Summary.parse(path2)).to_xml()
    output.write_text(xml, encoding="utf-8")
    return xml


def merge_ultra_dirs(dir1: Path, dir2: Path, out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    if not out_dir.is_dir():
        raise UltraParseError(f"{out_dir}: cannot create output directory")

    a = index_by_name(dir1)
    b = index_by_name(dir2)

    all_names = sorted(set(a.keys()) | set(b.keys()))
    if not all_names:
        raise UltraParseError(
            "No *.summary.xml files found in either input directory")

    merged = 0
    copied = 0

    for name in all_names:
        out_path = out_dir / name

        if name in a and name in b:
            merge_ultra_files(a[name], b[name], out_path)
            merged += 1
        else:
            src = a.get(name) or b.get(name)
            assert src is not None
            out_path.write_text(
                src.read_text(encoding="utf-8"), encoding="utf-8")
            copied += 1

    print(f"Merged: {merged}, copied: {copied}, total: {len(all_names)}")


# ---------------------------------------------------------------------
#  CLI
# ---------------------------------------------------------------------
def build_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="merge_summary.py",
        description=(
            "Merge two ULTRA summary XML files or matching summary "
            "directories."
        ),
    )
    parser.add_argument("first", type=Path, help="first file or directory")
    parser.add_argument("second", type=Path, help="second file or directory")
    parser.add_argument("output", type=Path, help="output file or directory")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_argparser().parse_args(argv)

    try:
        first_is_dir = args.first.is_dir()
        second_is_dir = args.second.is_dir()

        if first_is_dir and second_is_dir:
            if args.output.exists() and not args.output.is_dir():
                raise UltraParseError(
                    "In directory mode, output must be a directory: "
                    f"{args.output}"
                )
            merge_ultra_dirs(args.first, args.second, args.output)

        elif not first_is_dir and not second_is_dir:
            if args.output.exists() and args.output.is_dir():
                raise UltraParseError(
                    "In file mode, output must be a file path, "
                    f"not a directory: {args.output}"
                )
            merge_ultra_files(args.first, args.second, args.output)
            print("Merged file written.")

        else:
            raise UltraParseError(
                "Mixed input modes are not allowed: both inputs must be files "
                "or both inputs must be directories"
            )

    except UltraParseError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 2
    except Exception as e:
        print(f"Error: unexpected failure: {e}", file=sys.stderr)
        return 3

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
