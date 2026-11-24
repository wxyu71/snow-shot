#!/usr/bin/env python3
"""Offline signing of the two accepted Community EXEs. Never creates a production key."""
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import subprocess
import tempfile
from datetime import datetime, timezone
from pathlib import Path

import community_release as release

FEED = "community-installers.json"
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


def payload_for(directory: Path, version: str, published_at: str) -> bytes:
    release.release_tag(version)
    release.verify_assets(directory, version)
    installers = []
    for variant in ("online", "offline"):
        path = directory / f"snow-shot-{version}-windows-x64-{variant}.exe"
        if path.is_symlink() or not 0 < path.stat().st_size <= 4 * 1024**3:
            raise ValueError("Unsafe installer input")
        with path.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        installers.append({"variant": variant, "name": path.name,
                           "size": path.stat().st_size, "sha256": digest})
    document = {"schema": 2, "product": "snow-shot-community",
                "repository": "wxyu71/snow-shot", "platform": "windows-x64",
                "version": version, "publishedAt": published_at, "installers": installers}
    return json.dumps(document, sort_keys=True, separators=(",", ":")).encode("utf-8")


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
            (dist / f"{stem}.manifest.json").write_text(json.dumps({
                "PackageVersion": version, "Variant": variant, "Architecture": "x64",
                "Platform": "windows-x64", "StaticCrt": True, "StaticQt": True,
                "NativeValidation": {"Passed": True},
                "Installer": {"Sha256": digest, "Path": binary.name},
            }))
        payload = payload_for(dist, version, "2026-11-24T12:00:00Z")
        assert json.loads(payload)["installers"][1]["variant"] == "offline"
        key = root / "ephemeral-test.pem"
        openssl("genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:3072",
                "-pkeyopt", "rsa_keygen_pubexp:65537", "-out", str(key))
        modulus = openssl("rsa", "-in", str(key), "-modulus", "-noout").strip()
        trust = root / "trust.json"
        trust.write_text(json.dumps({"keys": [{"id": "test-only",
            "modulus": encoded(bytes.fromhex(modulus.decode().split("=", 1)[1])),
            "exponent": encoded(b"\x01\x00\x01")}]}))
        assert json.loads(sign(payload, key, "test-only", trust))["keyId"] == "test-only"
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
    parser.add_argument("--directory", type=Path)
    parser.add_argument("--private-key", type=Path)
    parser.add_argument("--key-id")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.selftest:
        selftest()
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
