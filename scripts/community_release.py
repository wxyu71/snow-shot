#!/usr/bin/env python3
"""Fail-closed policy for publishing a Windows x64-only Snow Shot Community beta."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BETA_VERSION = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+-beta\.[0-9]+$")
VERSION_LINE = re.compile(r'^\s*set\(SNOW_SHOT_VERSION\s+"([^"]+)"\)', re.MULTILINE)


def source_version(root: Path = ROOT) -> str:
    match = VERSION_LINE.search((root / "CMakeLists.txt").read_text(encoding="utf-8"))
    if not match or not BETA_VERSION.fullmatch(match.group(1)):
        raise ValueError("Expected a prerelease beta version in CMakeLists.txt")
    return match.group(1)


def release_tag(version: str) -> str:
    if not BETA_VERSION.fullmatch(version):
        raise ValueError(f"Expected beta version, got {version!r}")
    return f"v{version}_snow-shot"


def validate_tag(tag: str, version: str) -> None:
    expected = release_tag(version)
    if tag != expected:
        raise ValueError(f"Refusing release: tag {tag!r} must exactly match {expected}")


def render_release_notes(version: str, root: Path = ROOT) -> str:
    template = (root / "docs" / "community-release-notes.md").read_text(encoding="utf-8")
    if template.count("@VERSION@") != 1:
        raise ValueError("Release notes must contain exactly one @VERSION@ placeholder")
    rendered = template.replace("@VERSION@", version)
    if "@VERSION@" in rendered:
        raise ValueError("Unresolved release notes version placeholder")
    return rendered


def filenames(version: str) -> set[str]:
    return {
        f"snow-shot-{version}-windows-x64-{variant}.{extension}"
        for variant in ("online", "offline")
        for extension in ("exe", "exe.sha256", "manifest.json")
    }


def verify_assets(directory: Path, version: str, *, allow_build_intermediates: bool = False) -> None:
    expected = filenames(version)
    actual = {p.name for p in directory.iterdir() if p.is_file()}
    # Packaging also generates update/portable ZIPs and manifests in its build
    # directory. The final upload, in contrast, must have *exactly* six files.
    if allow_build_intermediates:
        found = actual & expected
        if any(p.startswith("snow-shot-mini-") or
               p.startswith(f"snow-shot-{version}-windows-arm64-") for p in actual):
            raise ValueError("Mini or ARM64 build artifacts are not allowed")
    else:
        found = actual
    if found != expected:
        raise ValueError(f"Release assets differ from the x64 Full allowlist: missing={sorted(expected-found)}, unexpected={sorted(found-expected)}")
    for variant in ("online", "offline"):
        stem = f"snow-shot-{version}-windows-x64-{variant}"
        binary = directory / f"{stem}.exe"
        checksum = directory / f"{stem}.exe.sha256"
        manifest = json.loads((directory / f"{stem}.manifest.json").read_text(encoding="utf-8-sig"))
        if binary.stat().st_size <= 0:
            raise ValueError(f"Empty installer: {stem}")
        with binary.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if not re.fullmatch(r"[0-9a-fA-F]{64}\s+\*?" + re.escape(binary.name),
                            checksum.read_text(encoding="utf-8").strip()):
            raise ValueError(f"Malformed SHA-256 sidecar: {checksum.name}")
        if checksum.read_text(encoding="utf-8").split()[0].lower() != digest:
            raise ValueError(f"SHA-256 mismatch: {binary.name}")
        if (manifest.get("PackageVersion") != version
                or manifest.get("Variant") != variant
                or manifest.get("Architecture") != "x64"
                or manifest.get("Platform") != "windows-x64"
                or manifest.get("StaticCrt") is not True
                or manifest.get("StaticQt") is not True
                or manifest.get("NativeValidation", {}).get("Passed") is not True
                or manifest.get("Installer", {}).get("Sha256", "").lower() != digest
                or manifest.get("Installer", {}).get("Path") != binary.name):
            raise ValueError(f"Installer manifest or native probes failed: {stem}")


def selftest() -> None:
    version = "1.5.8-beta.1"
    assert release_tag(version) == "v1.5.8-beta.1_snow-shot"
    validate_tag("v1.5.8-beta.1_snow-shot", version)
    notes = render_release_notes(version)
    assert "Release version: **1.5.8-beta.1**" in notes and "@VERSION@" not in notes
    for tag in ("v1.5.7_snow-shot", "v1.5.8-beta.1", "v1.5.8-beta.1_snow-shot-mini"):
        try:
            validate_tag(tag, version)
        except ValueError:
            pass
        else:
            raise AssertionError(f"Unsafe release tag accepted: {tag}")
    with tempfile.TemporaryDirectory() as temp:
        directory = Path(temp)
        for variant in ("online", "offline"):
            stem = f"snow-shot-{version}-windows-x64-{variant}"
            binary = directory / f"{stem}.exe"
            binary.write_bytes(("test-only-" + variant).encode("ascii"))
            digest = hashlib.sha256(binary.read_bytes()).hexdigest()
            (directory / f"{stem}.exe.sha256").write_text(f"{digest}  {binary.name}\n")
            (directory / f"{stem}.manifest.json").write_text(json.dumps({
                "PackageVersion": version, "Variant": variant, "Architecture": "x64",
                "Platform": "windows-x64", "StaticCrt": True, "StaticQt": True,
                "NativeValidation": {"Passed": True},
                "Installer": {"Sha256": digest, "Path": binary.name},
            }))
        verify_assets(directory, version)
        intermediate = directory / f"snow-shot-{version}-windows-x64-offline-update.manifest.json"
        intermediate.write_text("{}")
        try:
            verify_assets(directory, version)
        except ValueError:
            pass
        else:
            raise AssertionError("Extra release attachment accepted")
        verify_assets(directory, version, allow_build_intermediates=True)
        intermediate.unlink()
        (directory / f"snow-shot-{version}-windows-x64-online.exe").write_bytes(b"tampered")
        try:
            verify_assets(directory, version)
        except ValueError:
            pass
        else:
            raise AssertionError("Tampered binary accepted")
        fixed = directory / f"snow-shot-{version}-windows-x64-online.exe"
        fixed.write_bytes(b"test-only-online")
        (directory / f"snow-shot-{version}-windows-arm64-offline.exe").write_bytes(b"forbidden")
        try:
            verify_assets(directory, version)
        except ValueError:
            pass
        else:
            raise AssertionError("Other architecture accepted")
    presets = json.loads((ROOT / "CMakePresets.json").read_text(encoding="utf-8"))
    release = next(p for p in presets["configurePresets"] if p["name"] == "snow-shot-msvc-release")
    assert release["cacheVariables"]["SNOW_APPS_BUILD_SNOW_SHOT_MINI"] == "OFF"
    assert release["cacheVariables"]["SNOW_APPS_API_BASE_URL"] == ""
    print("Community release policy selftest passed")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "command",
        choices=["version", "tag", "validate-tag", "verify-assets", "render-notes", "selftest"],
    )
    parser.add_argument("--tag", default="")
    parser.add_argument("--directory", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--allow-build-intermediates", action="store_true")
    args = parser.parse_args()
    try:
        if args.command == "selftest":
            selftest()
        elif args.command == "version":
            print(source_version())
        elif args.command == "tag":
            print(release_tag(source_version()))
        elif args.command == "validate-tag":
            validate_tag(args.tag, source_version())
        elif args.command == "render-notes":
            if args.output is None:
                parser.error("--output is required for render-notes")
            args.output.write_text(render_release_notes(source_version()), encoding="utf-8")
        elif args.command == "verify-assets":
            if args.directory is None:
                parser.error("--directory is required for verify-assets")
            verify_assets(args.directory, source_version(),
                          allow_build_intermediates=args.allow_build_intermediates)
    except (OSError, ValueError, AssertionError, KeyError) as error:
        print(f"Community release policy failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
