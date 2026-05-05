#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
from pathlib import Path

from bm_common import ensure_dir, project_paths, write_json


SUPPORTED_TARGET_PLATFORMS = ("Windows", "Linux", "macOS", "Android", "iOS", "Web")


def normalize_target_platform(value: str) -> str:
    key = (value or "Windows").strip().lower()
    aliases = {
        "win": "Windows",
        "windows": "Windows",
        "linux": "Linux",
        "mac": "macOS",
        "macos": "macOS",
        "osx": "macOS",
        "android": "Android",
        "ios": "iOS",
        "iphone": "iOS",
        "web": "Web",
        "wasm": "Web",
    }
    if key not in aliases:
        raise ValueError(f"Unsupported target platform '{value}'. Choose one of: {', '.join(SUPPORTED_TARGET_PLATFORMS)}")
    return aliases[key]


def safe_copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    ensure_dir(dst.parent)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def export_project(build_name: str, target_platform: str) -> int:
    paths = project_paths()
    target_platform = normalize_target_platform(target_platform)
    out_dir = ensure_dir(paths.exports / build_name)

    safe_copy_tree(paths.scenes, out_dir / "Scenes")
    safe_copy_tree(paths.assets_processed, out_dir / "Assets" / "Processed")
    safe_copy_tree(paths.mods, out_dir / "mods")

    manifest = {
        "schema_version": 1,
        "build_name": build_name,
        "target_platform": target_platform,
        "includes": [
            "Scenes",
            "Assets/Processed",
            "mods",
        ],
        "exe_found": False,
    }
    write_json(out_dir / "export_manifest.json", manifest)
    print(f"Export package created for {target_platform}: {out_dir}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Balmung standalone export helper (Python)")
    parser.add_argument("build_name", nargs="?", default="Build1")
    parser.add_argument("--target-platform", choices=SUPPORTED_TARGET_PLATFORMS, default="Windows")
    args = parser.parse_args()
    return export_project(args.build_name, args.target_platform)


if __name__ == "__main__":
    raise SystemExit(main())





