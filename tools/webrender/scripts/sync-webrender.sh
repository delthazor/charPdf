#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WEBRENDER_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${WEBRENDER_DIR}/../.." && pwd)"
SRC_CFG="${REPO_ROOT}/assets/cfg"
DEST_CFG="${WEBRENDER_DIR}/cfg"
DEST_C="${WEBRENDER_DIR}/c"
SITE_CONFIG="${WEBRENDER_DIR}/site.config"

if [[ ! -d "${SRC_CFG}" ]]; then
    echo "error: assets/cfg not found at ${SRC_CFG}" >&2
    exit 1
fi

BASE_URL="${WEBRENDER_BASE_URL:-}"
if [[ -z "${BASE_URL}" && -f "${SITE_CONFIG}" ]]; then
    BASE_URL="$(tr -d '[:space:]' < "${SITE_CONFIG}")"
fi
if [[ -z "${BASE_URL}" ]]; then
    BASE_URL="https://delthazor.github.io/charPdf"
fi
BASE_URL="${BASE_URL%/}"

mkdir -p "${DEST_CFG}" "${DEST_C}"
rm -f "${DEST_CFG}"/*.json "${DEST_C}"/*.html
cp "${SRC_CFG}"/*.json "${DEST_CFG}/"

export WEBRENDER_DIR DEST_CFG DEST_C BASE_URL
python3 << 'PY'
import json
import os
import html
from pathlib import Path

webrender_dir = Path(os.environ["WEBRENDER_DIR"])
dest_cfg = Path(os.environ["DEST_CFG"])
dest_c = Path(os.environ["DEST_C"])
base_url = os.environ["BASE_URL"]

manifest = []

for path in sorted(dest_cfg.glob("char_*.json")):
    with path.open(encoding="utf-8") as f:
        data = json.load(f)

    stem = path.stem  # char_Celdrick
    slug = stem[5:].lower() if stem.startswith("char_") else stem.lower()
    name = data.get("name", slug)
    race = data.get("race", "")
    background = data.get("background", "")

    classes = data.get("classes") or {}
    class_parts = []
    total_level = 0
    for class_id, cls in classes.items():
        level = cls.get("level", 0) or 0
        total_level += level
        if level > 0:
            label = class_id[:1].upper() + class_id[1:] if class_id else class_id
            class_parts.append(f"{label} {level}")
    class_summary = " / ".join(class_parts)

    manifest.append({
        "slug": slug,
        "file": path.name,
        "name": name,
        "race": race,
        "background": background,
        "classSummary": class_summary,
        "totalLevel": total_level,
    })

    og_title = f"{name} — Character Sheet"
    og_desc_short = f"{race} {background} — D&D 5e character sheet".strip()
    og_desc = f"{race} {background} — Level {total_level} {class_summary}".strip(" —")
    og_image = f"{base_url}/img/og-default.png"
    og_url = f"{base_url}/c/{slug}.html"

    page = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="webrender-char" content="{html.escape(slug, quote=True)}">

  <title>{html.escape(og_title)}</title>
  <meta name="description" content="{html.escape(og_desc_short)}">

  <meta property="og:type" content="website">
  <meta property="og:title" content="{html.escape(og_title)}">
  <meta property="og:description" content="{html.escape(og_desc)}">
  <meta property="og:image" content="{html.escape(og_image)}">
  <meta property="og:url" content="{html.escape(og_url)}">

  <meta name="twitter:card" content="summary_large_image">
  <meta name="twitter:title" content="{html.escape(og_title)}">
  <meta name="twitter:description" content="{html.escape(og_desc)}">
  <meta name="twitter:image" content="{html.escape(og_image)}">

  <link rel="stylesheet" href="../css/style.css">
</head>
<body>
  <div id="app"></div>
  <script type="module" src="../js/main.js"></script>
</body>
</html>
"""
    (dest_c / f"{slug}.html").write_text(page, encoding="utf-8")

manifest_path = dest_cfg / "characters.json"
with manifest_path.open("w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2)
    f.write("\n")

print(f"Synced {len(manifest)} character(s) to {dest_cfg}")
print(f"Generated {len(manifest)} share page(s) in {dest_c}")
PY
