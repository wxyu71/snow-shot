#!/usr/bin/env python3
"""Run genuine Clang semantic analysis using CMake's compilation database.

Only production translation units in the configured native CMake graph can
be checked. Changed files outside that graph are reported, never counted as
covered.
"""
from __future__ import annotations

import argparse
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CXX_SUFFIXES = {".cpp", ".cxx", ".cc"}
COMPILER = re.compile(r"clang\+\+(?:-\d+)?$")
SKIP_ONE = {"-c", "-MD", "-MMD", "-MP"}
SKIP_WITH_VALUE = {"-o", "-MF", "-MT", "-MQ"}


def source_in_repo(path: str, directory: Path, root: Path) -> str | None:
    candidate = Path(path)
    if not candidate.is_absolute():
        candidate = directory / candidate
    try:
        return candidate.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return None


def production_cpp(path: str) -> bool:
    return path.startswith("snow_shot/src/") and Path(path).suffix in CXX_SUFFIXES


def changed_production_sources(base_ref: str | None) -> set[str]:
    if not base_ref:
        return set()
    proc = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=ACMR", "-z", base_ref,
         "HEAD", "--", "snow_shot/src"],
        cwd=REPO, capture_output=True, check=True,
    )
    return {path for raw in proc.stdout.split(b"\0") if raw
            if production_cpp(path := raw.decode("utf-8"))}


def syntactic_command(entry: dict) -> list[str]:
    """Keep CMake defines/includes/flags; suppress object and dep file output."""
    if isinstance(entry.get("arguments"), list):
        args = entry["arguments"]
    elif isinstance(entry.get("command"), str):
        args = shlex.split(entry["command"])
    else:
        raise ValueError("compile database row has no arguments or command")
    if not args or not COMPILER.fullmatch(Path(args[0]).name):
        raise ValueError(f"expected genuine Clang++ compile command, got: {args[:1]}")
    kept = [args[0], "-fsyntax-only"]
    index = 1
    while index < len(args):
        arg = args[index]
        if arg in SKIP_WITH_VALUE:
            if index + 1 >= len(args):
                raise ValueError(f"missing value after {arg}")
            index += 2
            continue
        if arg in SKIP_ONE or any(arg.startswith(opt) and len(arg) > len(opt)
                                  for opt in ("-MF", "-MT", "-MQ")):
            index += 1
            continue
        if arg.startswith("-o") and len(arg) > 2 and not arg.startswith("-openmp"):
            index += 1
            continue
        kept.append(arg)
        index += 1
    if len(kept) < 3:
        raise ValueError("no source arguments remain in Clang command")
    return kept


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--database", type=Path, required=True)
    ap.add_argument("--base-ref", help="PR base commit for changed-source priority/coverage")
    ap.add_argument("--report", type=Path, required=True)
    ap.add_argument("--min-production", type=int, default=3)
    args = ap.parse_args()
    try:
        rows = json.loads(args.database.read_text(encoding="utf-8"))
        if not isinstance(rows, list):
            raise ValueError("compile_commands.json must be a JSON array")
        changed = changed_production_sources(args.base_ref)
        relevant = []
        for row in rows:
            directory = Path(row["directory"]).resolve()
            file = source_in_repo(row["file"], directory, REPO)
            if file is not None and production_cpp(file):
                relevant.append((file, row, directory))
        if len(set(file for file, _, _ in relevant)) < args.min_production:
            raise ValueError("compile database has too few real production translation units")
        relevant.sort(key=lambda item: (item[0] not in changed, item[0]))
        covered = {file for file, _, _ in relevant}
        missing = sorted(changed - covered)
        checked = []
        for file, row, directory in relevant:
            argv = syntactic_command(row)
            print(f"[clang -fsyntax-only] {file}"
                  f" ({'changed' if file in changed else 'dependency coverage'})", flush=True)
            proc = subprocess.run(argv, cwd=directory, capture_output=True, text=True,
                                  timeout=90, check=False)
            if proc.stdout:
                print(proc.stdout, end="" if proc.stdout.endswith("\n") else "\n")
            if proc.stderr:
                print(proc.stderr, file=sys.stderr,
                      end="" if proc.stderr.endswith("\n") else "\n")
            checked.append({"source": file, "changed": file in changed,
                            "passed": proc.returncode == 0})
        successes = sum(bool(row["passed"]) for row in checked)
        report = {
            "database": str(args.database),
            "checked_count": len(checked),
            "passed_count": successes,
            "changed_production_sources": len(changed),
            "changed_sources_checked": sorted(changed & covered),
            "changed_sources_not_in_compile_db": missing,
            "checked": checked,
        }
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                               encoding="utf-8")
        print(f"Clang semantic result: {successes}/{len(checked)} production "
              f"compile commands passed; {len(changed & covered)}/{len(changed)} "
              "changed production sources covered by the lightweight graph.")
        if missing:
            print("::notice::Lightweight Clang DB does not cover "
                  f"{len(missing)} changed production files; see clang-syntax-report.json. "
                  "Full Windows Release compilation remains required.")
            for name in missing[:12]:
                print(f"  not covered: {name}")
        return 0 if successes == len(checked) else 1
    except (OSError, KeyError, ValueError, subprocess.SubprocessError) as exc:
        print(f"Clang syntax preflight failed closed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
