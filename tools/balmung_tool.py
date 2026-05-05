#!/usr/bin/env python3
"""
Balmung tooling entry point.

Provides lightweight project checks plus asset/content workflows that do not
require native engine bindings.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path

from bm_assets import process_assets, scan_overworld_pack, scan_source_pack


def project_root() -> Path:
    return Path(__file__).resolve().parents[1]


def default_overworld_root() -> Path:
    env = os.environ.get("BALMUNG_OVERWORLD_ROOT")
    return Path(env).resolve() if env else project_root()


def default_source_pack_root() -> Path:
    env = os.environ.get("BALMUNG_SOURCE_PACK_ROOT")
    return Path(env).resolve() if env else project_root()


def cmd_info(_: argparse.Namespace) -> int:
    root = project_root()
    print(f"Balmung tools root: {root}")
    print(f"Has docs: {(root / 'docs').exists()}")
    print(f"Has editor: {(root / 'editor').exists()}")
    print(f"Has bindings: {(root / 'bindings').exists()}")
    print(f"Default Overworld root: {default_overworld_root()}")
    print(f"Default source-pack root: {default_source_pack_root()}")
    return 0


def cmd_validate_layout(_: argparse.Namespace) -> int:
    root = project_root()
    required = [
        "docs",
        "include",
        "src",
        "editor",
        "bridge",
        "bindings",
    ]
    missing = [p for p in required if not (root / p).exists()]
    if missing:
        print("Missing paths:")
        for item in missing:
            print(f"- {item}")
        return 1
    print("Project layout looks OK.")
    return 0


def cmd_process_assets(args: argparse.Namespace) -> int:
    return process_assets(dry_run=args.dry_run)



def cmd_scan_overworld_pack(args: argparse.Namespace) -> int:
    pack_root = Path(args.pack_root).resolve() if args.pack_root else default_overworld_root()
    output = Path(args.output).resolve() if args.output else pack_root / "overworld.asset_manifest.json"
    manifest = scan_overworld_pack(pack_root, output)
    print(f"Overworld pack scanned: {manifest['summary']['total_files']} files")
    print(f"Manifest: {output}")
    return 0



def cmd_scan_source_pack(args: argparse.Namespace) -> int:
    pack_root = Path(args.pack_root).resolve() if args.pack_root else default_source_pack_root()
    output = Path(args.output).resolve() if args.output else pack_root / "source_pack.asset_manifest.json"
    project_name = args.project_name or pack_root.name
    consumer_project = args.consumer_project or "Overworld"
    manifest = scan_source_pack(pack_root, output, project_name=project_name, consumer_project=consumer_project)
    print(f"Source pack scanned: {manifest['summary']['total_files']} files")
    print(f"Manifest: {output}")
    return 0



def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="balmung-tool")
    sub = parser.add_subparsers(dest="command", required=True)

    p_info = sub.add_parser("info", help="Show basic project/tooling info")
    p_info.set_defaults(func=cmd_info)

    p_validate = sub.add_parser("validate-layout", help="Check required project folders")
    p_validate.set_defaults(func=cmd_validate_layout)

    p_assets = sub.add_parser("process-assets", help="Process Assets/Raw into Assets/Processed")
    p_assets.add_argument("--dry-run", action="store_true", help="Scan and print without copying files")
    p_assets.set_defaults(func=cmd_process_assets)

    p_overworld = sub.add_parser("scan-overworld-pack", help="Generate a content manifest for the Overworld project")
    p_overworld.add_argument("--pack-root", help="Overworld project root; defaults to BALMUNG_OVERWORLD_ROOT or the current project root")
    p_overworld.add_argument("--output", help="Output path for the generated manifest JSON")
    p_overworld.set_defaults(func=cmd_scan_overworld_pack)

    p_source = sub.add_parser("scan-source-pack", help="Generate a manifest for an external asset source such as Overworlder")
    p_source.add_argument("--pack-root", help="Source pack root; defaults to BALMUNG_SOURCE_PACK_ROOT or the current project root")
    p_source.add_argument("--output", help="Output path for the generated manifest JSON")
    p_source.add_argument("--project-name", help="Project name stored in the generated manifest")
    p_source.add_argument("--consumer-project", help="Game project that consumes the source pack")
    p_source.set_defaults(func=cmd_scan_source_pack)

    return parser



def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())





