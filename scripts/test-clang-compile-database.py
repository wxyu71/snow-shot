#!/usr/bin/env python3
"""Prove the Clang gate runs a real compiler using an actual CMake database."""
from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location(
    "clang_preflight", ROOT / "scripts/check-clang-compile-database.py"
)
if spec is None or spec.loader is None:
    raise RuntimeError("Cannot import real Clang semantic preflight")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

compiler = shutil.which("clang++-18")
if compiler is None:
    raise RuntimeError("clang++-18 is required for the semantic gate")
with tempfile.TemporaryDirectory(prefix="snow-clang-sema-") as tmp:
    root = Path(tmp)
    (root / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.24)\n"
        "project(snow_clang_sema_regression LANGUAGES CXX)\n"
        "add_library(real_cpp OBJECT probe.cpp)\n", encoding="utf-8"
    )
    source = root / "probe.cpp"
    source.write_text(
        "struct ScreenshotToolPalette { void expected(); };\n"
        "void ScreenshotToolPalette::createImageConversionActionFamily() {}\n",
        encoding="utf-8"
    )
    subprocess.run(
        ["cmake", "-S", str(root), "-B", str(root / "build"), "-G", "Ninja",
         "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", f"-DCMAKE_CXX_COMPILER={compiler}"],
        check=True, stdout=subprocess.DEVNULL,
    )
    rows = json.loads((root / "build/compile_commands.json").read_text(encoding="utf-8"))
    if len(rows) != 1:
        raise RuntimeError(f"Expected one genuine CMake compile command, got {len(rows)}")
    command = module.syntactic_command(rows[0])
    if "-fsyntax-only" not in command or "-c" in command:
        raise RuntimeError("Compiler was not converted to syntax-only mode")
    failed = subprocess.run(command, cwd=rows[0]["directory"],
                            capture_output=True, text=True)
    if failed.returncode == 0 or "createImageConversionActionFamily" not in failed.stderr:
        raise RuntimeError("Real Clang did not reject the undeclared member:\n"
                           + failed.stderr)
    source.write_text(
        "struct ScreenshotToolPalette { void expected(); };\n"
        "void ScreenshotToolPalette::expected() {}\n",
        encoding="utf-8"
    )
    passed = subprocess.run(command, cwd=rows[0]["directory"],
                            capture_output=True, text=True)
    if passed.returncode != 0:
        raise RuntimeError("Real Clang rejected the repaired member:\n" + passed.stderr)
print("Real Clang negative/positive CMake compilation-database regression passed")
