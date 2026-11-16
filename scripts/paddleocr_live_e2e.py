#!/usr/bin/env python3
"""AI Studio PaddleOCR v2: JSONL contract self-tests and optional real-token E2E.

Run without credentials: python scripts/paddleocr_live_e2e.py --selftest
Run with a token: PADDLEOCR_ACCESS_TOKEN=... python scripts/paddleocr_live_e2e.py --live
Never prints a token or writes a screenshot/result outside the ephemeral test area.
"""
from __future__ import annotations

import argparse
import io
import json
import os
import re
import tempfile
import time
from pathlib import Path
from urllib.parse import urlparse

JOB_URL = "https://paddleocr.aistudio-app.com/api/v2/ocr/jobs"
MODELS = ("PP-OCRv6", "PP-StructureV3", "PaddleOCR-VL-1.6")


def extract_markdown(jsonl: bytes, model: str) -> str:
    """Official JSONL contains result.ocrResults or result.layoutParsingResults."""
    if model not in MODELS:
        raise ValueError("Unsupported PaddleOCR model")
    pages = []
    for line in jsonl.splitlines():
        if not line.strip():
            continue
        try:
            result = json.loads(line)["result"]
        except (ValueError, KeyError, TypeError) as exc:
            raise ValueError("Invalid PaddleOCR JSONL result") from exc
        if model == "PP-OCRv6":
            for item in result.get("ocrResults", ()):
                values = item.get("prunedResult", {}).get("rec_texts", ())
                if not isinstance(values, list) or not all(isinstance(v, str) for v in values):
                    raise ValueError("Invalid OCR recognized text array")
                if values:
                    pages.append("\n".join(values))
        else:
            for item in result.get("layoutParsingResults", ()):
                value = item.get("markdown", {}).get("text", "")
                if not isinstance(value, str):
                    raise ValueError("Invalid document Markdown")
                if value.strip():
                    pages.append(value)
    if not pages:
        raise ValueError("PaddleOCR returned no text or Markdown")
    return "\n\n".join(pages).strip()


def error_hint(status: int, payload: dict) -> str:
    code = payload.get("code", -1)
    if status == 401 or code == 401:
        return "Access Token invalid (401)"
    if status == 429 or code == 12002:
        return "AI Studio rate limited this request (12002/429)"
    if status == 403 or code == 12001:
        return "AI Studio daily quota exhausted (12001/403)"
    return f"AI Studio error HTTP {status} code {code}"


def test_contract() -> None:
    ocr = {"result": {"ocrResults": [
        {"prunedResult": {"rec_texts": ["中文 Hello", "42"]}}]}}
    doc = {"result": {"layoutParsingResults": [
        {"markdown": {"text": "# Table\n\n| A | B |\n| --- | --- |\n| 1 | 2 |\n\n$$E=mc^2$$"}}]}}
    assert extract_markdown((json.dumps(ocr) + "\n").encode(), MODELS[0]) == "中文 Hello\n42"
    for model in MODELS[1:]:
        md = extract_markdown((json.dumps(doc) + "\n").encode(), model)
        assert "| 1 | 2 |" in md and "$$E=mc^2$$" in md
    try:
        extract_markdown(b"{broken\n", MODELS[0])
        raise AssertionError("invalid JSONL accepted")
    except ValueError as exc:
        assert "Invalid" in str(exc)
    assert "401" in error_hint(401, {})
    assert "12002" in error_hint(429, {"code": 12002})
    assert "12001" in error_hint(403, {"code": 12001})
    print("AI Studio JSONL/parser/auth/quota/rate-limit offline contract: PASS")


def fixture_png() -> bytes:
    from PIL import Image, ImageDraw, ImageFont

    img = Image.new("RGB", (1500, 1050), "white")
    d = ImageDraw.Draw(img)
    paths = (
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
    )
    font_path = next((p for p in paths if Path(p).exists()), None)
    font = ImageFont.truetype(font_path, 40) if font_path else ImageFont.load_default()
    large = ImageFont.truetype(font_path, 58) if font_path else font
    d.text((80, 50), "Invoice 2026 - Mixed Layout", font=large, fill="black")
    d.text((80, 160), "Table of purchases", font=font, fill="black")
    x = [70, 650, 1200]
    y = [240, 330, 420, 510]
    for i in x:
        d.line([(i, y[0]), (i, y[-1])], fill="black", width=4)
    for i in y:
        d.line([(x[0], i), (x[-1], i)], fill="black", width=4)
    for col, row, value in [
        (0, 0, "Item"), (1, 0, "Quantity"),
        (0, 1, "Book"), (1, 1, "3"),
        (0, 2, "Pen"), (1, 2, "2"),
    ]:
        d.text((x[col] + 20, y[row] + 14), value, fill="black", font=font)
    d.text((80, 600), "Formula:", font=font, fill="black")
    d.text((80, 690), "E = m c²", font=large, fill="black")
    d.text((80, 820), "End of mixed layout. Total 42.", font=font, fill="black")
    out = io.BytesIO()
    img.save(out, format="PNG")
    return out.getvalue()


def checked_json(response):
    try:
        payload = response.json()
    except ValueError as exc:
        raise RuntimeError("AI Studio returned invalid JSON") from exc
    if response.status_code != 200 or payload.get("code", -1) != 0:
        raise RuntimeError(error_hint(response.status_code, payload))
    return payload["data"]


def safe_result_url(url: str) -> None:
    parsed = urlparse(url)
    if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password:
        raise RuntimeError("AI Studio supplied an unsafe unsigned result URL")


def test_auth(base: str) -> None:
    import requests
    response = requests.post(base, headers={"Authorization": "Bearer definitely-invalid-community-smoke-token"},
                             data={"model": "PP-OCRv6"}, files={"file": ("fixture.png", fixture_png(), "image/png")},
                             timeout=60, allow_redirects=False)
    payload = {}
    try:
        payload = response.json()
    except ValueError:
        pass
    if response.status_code != 401 and payload.get("code") != 401:
        raise RuntimeError("Invalid-token test did not return 401; verify API endpoint")
    print("AI Studio live invalid-token 401: PASS")


def run_model(base: str, token: str, model: str, png: bytes) -> dict:
    import requests
    options = (
        {"useTableRecognition": True, "useFormulaRecognition": True, "prettifyMarkdown": True}
        if model == "PP-StructureV3" else
        {"useLayoutDetection": True, "prettifyMarkdown": True}
        if model == "PaddleOCR-VL-1.6" else
        {"useDocOrientationClassify": False}
    )
    auth = {"Authorization": "Bearer " + token}
    response = requests.post(base, headers=auth,
                             data={"model": model, "optionalPayload": json.dumps(options)},
                             files={"file": ("community-e2e.png", png, "image/png")},
                             timeout=90, allow_redirects=False)
    data = checked_json(response)
    job = str(data["jobId"])
    if not re.fullmatch(r"[A-Za-z0-9_-]{1,128}", job):
        raise RuntimeError("Invalid job identifier from AI Studio")
    result_url = {}
    deadline = time.monotonic() + 600
    while time.monotonic() < deadline:
        time.sleep(5)  # AI Studio example recommends 5s job polling
        response = requests.get(base + "/" + job, headers=auth, timeout=90, allow_redirects=False)
        data = checked_json(response)
        state = data.get("state")
        if state in ("pending", "running"):
            continue
        if state == "failed":
            raise RuntimeError("AI Studio job failed: " + str(data.get("errorMsg", "unknown failure")))
        if state != "done":
            raise RuntimeError("Unexpected AI Studio job state")
        result_url = data.get("resultUrl") or {}
        break
    else:
        raise RuntimeError("AI Studio job polling exceeded 10 minutes")
    json_url = result_url.get("jsonUrl")
    md_url = result_url.get("markdownUrl")
    # Never forward a bearer token to BOS/signed-result URLs.
    target = json_url or md_url
    if not isinstance(target, str) or not target:
        raise RuntimeError("AI Studio returned no result URL")
    safe_result_url(target)
    resp = requests.get(target, timeout=90, allow_redirects=False)
    if resp.status_code != 200 or len(resp.content) > 8 * 1024 * 1024:
        raise RuntimeError("AI Studio result URL download failed")
    markdown = (extract_markdown(resp.content, model) if json_url else resp.text.strip())
    if len(markdown.strip()) < 8:
        raise RuntimeError("AI Studio recognized suspiciously little content")
    table = bool(re.search(r"(?m)^\|.+\|$|<table|\\begin\{tabular\}", markdown))
    formula = bool(re.search(r"\$[^$]+\$|\\frac|\\sqrt|\\\[|mc\^?2", markdown, re.I))
    if model != "PP-OCRv6" and (not table or not formula):
        raise RuntimeError(f"{model} result missing structured table or formula")
    print(f"{model}: PASS; Markdown chars={len(markdown)}, table={table}, formula={formula}")
    return {"model": model, "passed": True, "markdown_length": len(markdown),
            "table": table, "formula": formula}


def main() -> None:
    parser = argparse.ArgumentParser()
    choices = parser.add_mutually_exclusive_group(required=True)
    choices.add_argument("--selftest", action="store_true")
    choices.add_argument("--live", action="store_true")
    args = parser.parse_args()
    test_contract()
    if args.selftest:
        return
    token = os.getenv("PADDLEOCR_ACCESS_TOKEN", "").strip()
    if not token:
        raise SystemExit("LIVE OCR TEST BLOCKED: set PADDLEOCR_ACCESS_TOKEN (GitHub secret)")
    base = os.getenv("PADDLEOCR_API_URL", JOB_URL).rstrip("/")
    if not base.endswith("/api/v2/ocr/jobs"):
        base += "/api/v2/ocr/jobs"
    safe_result_url(base)
    test_auth(base)
    png = fixture_png()
    results = [run_model(base, token, model, png) for model in MODELS]
    print("REAL AI Studio E2E RESULT: " + json.dumps(results, ensure_ascii=False))


if __name__ == "__main__":
    main()
