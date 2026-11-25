#!/usr/bin/env python3
"""Offline signing of accepted Community EXEs and transactional update ZIPs. No production key generation."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import subprocess
import tempfile
import zipfile
import stat
from datetime import datetime, timezone
from pathlib import Path

import community_release as release

FEED = "latest-version.json"
MAX_BYTES = 4 * 1024**3
MAX_FILES = 20_000
RECORD = "snow-shot-installation.json"
TRUST = release.ROOT / "snow_shot/resources/update-trusted-keys.json"


def encoded(value: bytes) -> str:
    return base64.b64encode(value).decode("ascii")


def der(tag: int, data: bytes) -> bytes:
    count = len(data)
    length = bytes([count]) if count < 128 else count.to_bytes((count.bit_length() + 7) // 8, "big")
    if count >= 128:
        length = bytes([128 | len(length)]) + length
    return bytes([tag]) + length + data


def integer(value: bytes) -> bytes:
    value = value.lstrip(b"\0") or b"\0"
    return der(2, (b"\0" if value[0] & 128 else b"") + value)


def public_der(key: dict) -> bytes:
    modulus = base64.b64decode(key["modulus"], validate=True)
    exponent = base64.b64decode(key["exponent"], validate=True)
    if len(modulus) != 384 or not modulus[0] & 128 or exponent != b"\x01\x00\x01":
        raise ValueError("Expected an RSA-3072 key with public exponent 65537")
    rsa = der(48, integer(modulus) + integer(exponent))
    algorithm = bytes.fromhex("300d06092a864886f70d0101010500")
    return der(48, algorithm + der(3, b"\0" + rsa))


def openssl(*args: str, payload: bytes | None = None) -> bytes:
    return subprocess.run(["openssl", *args], input=payload, check=True,
                          stdout=subprocess.PIPE).stdout


def export_public_key(public_key: Path, key_id: str) -> dict:
    """Accept only an RSA-3072 public PEM; never read a private PEM."""
    if not re.fullmatch(r"[A-Za-z0-9_.-]{1,80}", key_id):
        raise ValueError("Invalid key identifier")
    if not public_key.is_file() or public_key.stat().st_size > 16384:
        raise ValueError("Expected a small public PEM file")
    pem = public_key.read_bytes().strip()
    if not (pem.startswith(b"-----BEGIN PUBLIC KEY-----") and
            pem.endswith(b"-----END PUBLIC KEY-----") and
            pem.count(b"-----BEGIN ") == 1):
        raise ValueError("Expected only a PUBLIC KEY PEM; never provide a private key here")
    modulus = openssl("rsa", "-pubin", "-in", str(public_key),
                      "-modulus", "-noout").strip()
    if not modulus.startswith(b"Modulus="):
        raise ValueError("Invalid RSA public key")
    key = {"id": key_id,
           "modulus": encoded(bytes.fromhex(modulus.decode("ascii").split("=", 1)[1])),
           "exponent": encoded(b"\x01\x00\x01")}
    actual = openssl("pkey", "-pubin", "-in", str(public_key), "-pubout", "-outform", "DER")
    if actual != public_der(key):
        raise ValueError("Expected RSA-3072 with exponent 65537")
    return key


def file_digest(path: Path) -> str:
    if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= MAX_BYTES:
        raise ValueError("Unsafe release input")
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def inventory_key(path: str) -> str:
    if (not isinstance(path, str) or len(path.encode("utf-16-le")) // 2 > 220
            or not (path.startswith(("bin/", "share/snow-shot/")) or path == RECORD)
            or path.lower().startswith("bin/portable/") or path.lower() == "bin/__data_directory"):
        raise ValueError("Unsafe update path")
    for part in path.split("/"):
        if (not part or part in (".", "..") or part.endswith((".", " "))
                or any(ord(c) < 32 or c in '\\:<>\"|?*' for c in part)
                or re.fullmatch(r"(CON|PRN|AUX|NUL|CONIN\$|CONOUT\$|CLOCK\$|COM[0-9¹²³]|LPT[0-9¹²³])",
                                part.split(".")[0], re.IGNORECASE)):
            raise ValueError("Unsafe update path")
    return path.lower()


def index_inventory(items: list[dict]) -> dict[str, tuple[int, str]]:
    indexed = {}
    lowered = set()
    total = 0
    for item in items:
        path, size, sha = item["path"], item["size"], item["sha256"]
        key = inventory_key(path)
        if key in lowered or type(size) is not int or size < 0 or not re.fullmatch(r"[0-9a-f]{64}", sha):
            raise ValueError("Invalid or duplicate inventory entry")
        lowered.add(key)
        indexed[path] = (size, sha)
        total += size
    if not 0 < len(indexed) <= MAX_FILES or total > MAX_BYTES:
        raise ValueError("Invalid inventory size")
    for key in lowered:
        parts = key.split("/")
        if any("/".join(parts[:i]) in lowered for i in range(1, len(parts))):
            raise ValueError("Conflicting update paths")
    return indexed


def zip_inventory(archive: Path, version: str, variant: str, installer_manifest: dict) -> list[dict]:
    file_digest(archive)
    inventory = []
    total = 0
    with zipfile.ZipFile(archive) as z:
        if not 0 < len(z.infolist()) <= MAX_FILES:
            raise ValueError("Unexpected archive entry count")
        for info in z.infolist():
            inventory_key(info.filename)
            mode = stat.S_IFMT(info.external_attr >> 16)
            if info.is_dir() or mode not in (0, stat.S_IFREG) or info.flag_bits & 1:
                raise ValueError("Archive contains nonregular or encrypted entry")
            total += info.file_size
            if total > MAX_BYTES:
                raise ValueError("Archive exceeds the update payload limit")
            with z.open(info) as stream:
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            inventory.append({"path": info.filename, "size": info.file_size, "sha256": digest})
        actual = index_inventory(inventory)
        required = {"bin/snow_shot.exe", "bin/snow-shot-updater.exe", RECORD}
        if not required <= actual.keys() or actual[RECORD][0] > 8 * 1024**2:
            raise ValueError("Missing required payload or oversized installation record")
        record = json.loads(z.read(RECORD))
        if (record.get("schema") != 1 or record.get("platform") != "windows-x64"
                or record.get("version") != version or record.get("variant") != variant
                or record.get("product", "snow-shot") != "snow-shot"):
            raise ValueError("Archive installation record does not match the release")
        owned = {k: v for k, v in actual.items() if k != RECORD}
        if index_inventory(record["files"]) != owned:
            raise ValueError("Archive ownership inventory does not match its bytes")
    expected = index_inventory([{"path": entry["Path"].replace("\\", "/"),
                                 "size": entry["Bytes"], "sha256": entry["Sha256"].lower()}
                                for entry in installer_manifest["InstallFiles"]])
    if actual != expected:
        raise ValueError("Update ZIP and EXE staging inventory differ")
    return sorted(inventory, key=lambda item: item["path"])


def payload_for(directory: Path, version: str, published_at: str) -> bytes:
    release.release_tag(version)
    release.verify_assets(directory, version, allow_build_intermediates=True)
    packages = []
    for variant in ("online", "offline"):
        stem = f"snow-shot-{version}-windows-x64-{variant}"
        prefix = f"setup/snow-shot_windows-x64-{variant}"
        exe = directory / f"{stem}.exe"
        archive = directory / f"{stem}-update.zip"
        manifest = json.loads((directory / f"{stem}.manifest.json").read_text(encoding="utf-8-sig"))
        packages.append({"variant": variant, "kind": "installer", "path": prefix + ".exe",
                         "size": exe.stat().st_size, "sha256": file_digest(exe)})
        packages.append({"variant": variant, "kind": "update", "path": prefix + "-update.zip",
                         "size": archive.stat().st_size, "sha256": file_digest(archive),
                         "files": zip_inventory(archive, version, variant, manifest)})
    document = {"schema": 3, "product": "snow-shot-community",
                "repository": "wxyu71/snow-shot", "platform": "windows-x64",
                "version": version, "publishedAt": published_at, "packages": packages}
    return json.dumps(document, sort_keys=True, separators=(",", ":")).encode("utf-8")


def local_installer_manifest(directory: Path, version: str, variant: str) -> bytes:
    release.release_tag(version)
    if variant not in ("online", "offline"):
        raise ValueError("Invalid installer variant")
    stem = f"snow-shot-{version}-windows-x64-{variant}"
    archive = directory / f"{stem}-update.zip"
    # The packaging script emits an archive manifest before either EXE exists.
    manifest = json.loads((directory / f"{stem}-update.manifest.json").read_text(encoding="utf-8-sig"))
    if (manifest.get("PackageVersion") != version or manifest.get("Variant") != variant
            or manifest.get("Architecture") != "x64" or manifest.get("Platform") != "windows-x64"
            or manifest.get("NativeValidation", {}).get("Passed") is not True):
        raise ValueError("Update archive failed its production manifest")
    digest = file_digest(archive)
    if manifest.get("Archive", {}).get("Sha256", "").lower() != digest:
        raise ValueError("Archive checksum mismatch")
    files = zip_inventory(archive, version, variant, manifest)
    return json.dumps({"schema": 1, "product": "snow-shot-community", "platform": "windows-x64",
                       "version": version, "variant": variant, "size": archive.stat().st_size,
                       "sha256": digest, "files": files},
                      sort_keys=True, separators=(",", ":")).encode("utf-8")


def sign(payload: bytes, private_key: Path, key_id: str, trust: Path) -> bytes:
    if not re.fullmatch(r"[A-Za-z0-9_.-]{1,80}", key_id):
        raise ValueError("Invalid key identifier")
    if private_key.resolve().is_relative_to(release.ROOT):
        raise ValueError("Keep signing private keys outside the repository")
    keys = json.loads(trust.read_text(encoding="utf-8"))["keys"]
    selected = [key for key in keys if key["id"] == key_id]
    if len(selected) != 1:
        raise ValueError("Signing key is not in the independently reviewed trust store")
    expected = public_der(selected[0])
    actual = openssl("pkey", "-in", str(private_key), "-pubout", "-outform", "DER")
    if actual != expected:
        raise ValueError("Private key does not match the pinned public key")
    options = ("-sigopt", "rsa_padding_mode:pss", "-sigopt", "rsa_pss_saltlen:32",
               "-sigopt", "rsa_mgf1_md:sha256")
    signature = openssl("dgst", "-sha256", "-sign", str(private_key), *options, payload=payload)
    if len(signature) != 384:
        raise ValueError("Invalid signature length")
    with tempfile.TemporaryDirectory() as temp:
        key = Path(temp) / "public.der"
        sig = Path(temp) / "signature.bin"
        key.write_bytes(expected)
        sig.write_bytes(signature)
        openssl("dgst", "-sha256", "-verify", str(key), "-keyform", "DER",
                "-signature", str(sig), *options, payload=payload)
    return json.dumps({"schema": 1, "keyId": key_id, "payload": encoded(payload),
                       "signature": encoded(signature)}, separators=(",", ":")).encode("utf-8") + b"\n"


def selftest() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        dist = root / "dist"
        dist.mkdir()
        version = "1.6.3-beta.2"
        for variant in ("online", "offline"):
            stem = f"snow-shot-{version}-windows-x64-{variant}"
            binary = dist / f"{stem}.exe"
            binary.write_bytes(("fixture-" + variant).encode())
            digest = hashlib.sha256(binary.read_bytes()).hexdigest()
            (dist / f"{stem}.exe.sha256").write_text(f"{digest}  {binary.name}\n")
            contents = {"bin/snow_shot.exe": b"app", "bin/snow-shot-updater.exe": b"helper"}
            owned = [{"path": name, "size": len(value), "sha256": hashlib.sha256(value).hexdigest()}
                     for name, value in contents.items()]
            contents[RECORD] = json.dumps({"schema": 1, "platform": "windows-x64",
                                           "version": version, "variant": variant, "files": owned}).encode()
            with zipfile.ZipFile(dist / f"{stem}-update.zip", "w") as z:
                for name, value in contents.items():
                    z.writestr(name, value)
            files = [{"Path": name, "Bytes": len(value), "Sha256": hashlib.sha256(value).hexdigest()}
                     for name, value in contents.items()]
            (dist / f"{stem}.manifest.json").write_text(json.dumps({
                "PackageVersion": version, "Variant": variant, "Architecture": "x64",
                "Platform": "windows-x64", "StaticCrt": True, "StaticQt": True,
                "NativeValidation": {"Passed": True},
                "Installer": {"Sha256": digest, "Path": binary.name},
                "InstallFiles": files,
            }))
        payload = payload_for(dist, version, "2026-11-24T12:00:00Z")
        assert len(json.loads(payload)["packages"]) == 4
        assert json.loads(payload)["packages"][3]["variant"] == "offline"
        archive = dist / f"snow-shot-{version}-windows-x64-online-update.zip"
        saved = archive.read_bytes()
        with zipfile.ZipFile(archive, "a") as z:
            z.writestr("../foreign.exe", b"bad")
        try:
            payload_for(dist, version, "2026-11-24T12:00:00Z")
        except ValueError:
            pass
        else:
            raise AssertionError("Unsafe archive accepted")
        archive.write_bytes(saved)
        key = root / "ephemeral-test.pem"
        openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:3072",
                "-pkeyopt", "rsa_keygen_pubexp:65537", "-out", str(key))
        modulus = openssl("rsa", "-in", str(key), "-modulus", "-noout").strip()
        trust = root / "trust.json"
        trust.write_text(json.dumps({"keys": [{"id": "test-only",
            "modulus": encoded(bytes.fromhex(modulus.decode().split("=", 1)[1])),
            "exponent": encoded(b"\x01\x00\x01")}]}))
        assert json.loads(sign(payload, key, "test-only", trust))["keyId"] == "test-only"
        public_pem = root / "public.pem"
        public_pem.write_bytes(openssl("pkey", "-in", str(key), "-pubout"))
        assert export_public_key(public_pem, "test-only") == json.loads(trust.read_text())["keys"][0]
        try:
            export_public_key(key, "test-only")
        except ValueError:
            pass
        else:
            raise AssertionError("Private PEM was accepted by public export")
        trust.write_text('{"keys":[]}')
        try:
            sign(payload, key, "test-only", trust)
        except ValueError:
            pass
        else:
            raise AssertionError("Untrusted signing key accepted")
        (dist / f"snow-shot-{version}-windows-x64-online.exe").write_bytes(b"tampered")
        try:
            payload_for(dist, version, "2026-11-24T12:00:00Z")
        except ValueError:
            pass
        else:
            raise AssertionError("Tampered installer accepted")
    print("Community installer signing selftest passed (ephemeral key only)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--local-variant", choices=("online", "offline"))
    parser.add_argument("--directory", type=Path)
    parser.add_argument("--private-key", type=Path)
    parser.add_argument("--export-public-key", type=Path,
                        help="Export only an independently generated PUBLIC KEY PEM as trust JSON")
    parser.add_argument("--key-id")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.selftest:
        selftest()
        return
    if args.export_public_key:
        if not args.key_id or args.private_key or args.local_variant or args.directory:
            parser.error("Public export requires --key-id and a public PEM only")
        data = json.dumps({"keys": [export_public_key(args.export_public_key, args.key_id)]},
                          indent=2) + "\n"
        if args.output:
            with args.output.open("x", encoding="utf-8") as stream:
                stream.write(data)
        else:
            print(data, end="")
        return
    if args.local_variant:
        if not args.directory or not args.output:
            parser.error("Local installer manifest requires --directory and --output")
        with args.output.open("xb") as stream:
            stream.write(local_installer_manifest(args.directory, release.source_version(), args.local_variant))
        return
    if not all((args.directory, args.private_key, args.key_id, args.output)):
        parser.error("Signing requires --directory, --private-key, --key-id and --output")
    if args.output.name != FEED:
        parser.error(f"Output must be named {FEED}")
    payload = payload_for(args.directory, release.source_version(),
                          datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"))
    envelope = sign(payload, args.private_key, args.key_id, TRUST)
    with args.output.open("xb") as stream:
        stream.write(envelope)


if __name__ == "__main__":
    main()
