#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import Counter
from hashlib import sha1
from pathlib import Path
from typing import Iterable

from bm_common import ensure_dir, project_paths, write_json


SUPPORTED = {".png": "texture", ".wav": "audio", ".fbx": "mesh"}
HASH_EXTENSIONS = {
    ".json",
    ".mcmeta",
    ".png",
    ".frag",
    ".vert",
    ".fsh",
    ".vsh",
    ".txt",
    ".lang",
    ".material",
    ".texture_set",
}


def iter_assets(root: Path) -> Iterable[Path]:
    if not root.exists():
        return []
    return (p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in SUPPORTED)


def file_sha1(path: Path) -> str:
    h = sha1()
    with path.open("rb") as f:
        while True:
            chunk = f.read(65536)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()


def process_assets(dry_run: bool = False) -> int:
    paths = project_paths()
    ensure_dir(paths.assets_processed)

    items = []
    for src in sorted(iter_assets(paths.assets_raw), key=lambda p: str(p).lower()):
        rel = src.relative_to(paths.assets_raw)
        dst = paths.assets_processed / rel
        kind = SUPPORTED[src.suffix.lower()]
        items.append(
            {
                "kind": kind,
                "source": str(src),
                "relative_path": str(rel).replace("\\", "/"),
                "size_bytes": src.stat().st_size,
                "sha1": file_sha1(src),
                "staged_path": str(dst),
            }
        )
        if not dry_run:
            ensure_dir(dst.parent)
            dst.write_bytes(src.read_bytes())

    manifest = {
        "schema_version": 1,
        "asset_count": len(items),
        "items": items,
    }
    if not dry_run:
        write_json(paths.assets_processed / "asset_manifest.json", manifest)

    print(f"Processed {len(items)} assets ({'dry-run' if dry_run else 'written'}).")
    return 0


def _scan_tree(root: Path, spotlight_limit: int = 64) -> tuple[Counter, Counter, list[dict[str, object]]]:
    top_level = Counter()
    extension_counts = Counter()
    spotlight: list[dict[str, object]] = []

    for path in sorted((p for p in root.rglob("*") if p.is_file()), key=lambda p: str(p).lower()):
        rel = path.relative_to(root)
        rel_text = str(rel).replace("\\", "/")
        extension = path.suffix.lower() or "<none>"
        extension_counts[extension] += 1
        top_level[rel.parts[0] if rel.parts else "."] += 1

        if len(spotlight) < spotlight_limit and (path.suffix.lower() in HASH_EXTENSIONS or path.name in {"pack.mcmeta", "manifest.json"}):
            spotlight.append(
                {
                    "path": rel_text,
                    "size_bytes": path.stat().st_size,
                    "sha1": file_sha1(path),
                }
            )

    return top_level, extension_counts, spotlight


def scan_source_pack(
    pack_root: Path,
    output: Path | None = None,
    *,
    project_name: str,
    consumer_project: str | None = None,
) -> dict:
    pack_root = pack_root.resolve()
    if not pack_root.exists():
        raise FileNotFoundError(f"Pack root does not exist: {pack_root}")

    files = sorted((p for p in pack_root.rglob("*") if p.is_file()), key=lambda p: str(p).lower())
    top_level, extension_counts, spotlight = _scan_tree(pack_root)

    nested_groups: dict[str, dict[str, int]] = {}
    for child in sorted((p for p in pack_root.iterdir() if p.is_dir()), key=lambda p: p.name.lower()):
        child_groups = Counter()
        for path in child.rglob("*"):
            if not path.is_file():
                continue
            rel = path.relative_to(child)
            child_groups[rel.parts[0] if rel.parts else "."] += 1
        nested_groups[child.name] = dict(sorted(child_groups.items()))

    manifest = {
        "schema_version": 1,
        "project": {
            "name": project_name,
            "kind": "asset pack",
            "content_pack_root": str(pack_root),
        },
        "integration": {
            "used_by": consumer_project or "unassigned",
            "usage": "external source pack",
        },
        "summary": {
            "total_files": len(files),
            "top_level_groups": dict(sorted(top_level.items())),
            "nested_groups": nested_groups,
            "extension_counts": dict(sorted(extension_counts.items())),
        },
        "spotlight_files": spotlight,
    }

    if output is not None:
        write_json(output, manifest)

    return manifest


def scan_overworld_pack(pack_root: Path, output: Path | None = None) -> dict:
    manifest = scan_source_pack(
        pack_root,
        output=None,
        project_name="Overworld",
        consumer_project="Overworld",
    )

    manifest["project"]["kind"] = "game project"
    manifest["integration"] = {
        "engine": "Balmung Engine",
        "engine_version": "2.4.0",
        "engine_kind": "custom engine / framework",
    }

    minecraft_groups = Counter()
    pack_root = pack_root.resolve()
    for path in sorted((p for p in pack_root.rglob("*") if p.is_file()), key=lambda p: str(p).lower()):
        rel = path.relative_to(pack_root)
        if rel.parts and rel.parts[0] == "minecraft" and len(rel.parts) > 1:
            minecraft_groups[rel.parts[1]] += 1

    manifest["summary"]["minecraft_groups"] = dict(sorted(minecraft_groups.items()))
    manifest["shader_pipeline"] = {
        "chunk_vertex_shader": "minecraft/assets/shaders/chunk_shader.vert",
        "chunk_fragment_shader": "minecraft/assets/shaders/chunk_shader.frag",
        "backend_target": ["Vulkan", "DirectX", "OpenGL compatibility layer"],
    }

    if output is not None:
        write_json(output, manifest)

    return manifest



def main() -> int:
    parser = argparse.ArgumentParser(description="Balmung asset processing helper (Python)")
    parser.add_argument("--dry-run", action="store_true", help="Scan and print without copying files")
    parser.add_argument("--scan-overworld-pack", metavar="PACK_ROOT", help="Scan an Overworld content pack and emit a manifest")
    parser.add_argument("--scan-source-pack", metavar="PACK_ROOT", help="Scan a generic source pack and emit a manifest")
    parser.add_argument("--project-name", default="Source Pack", help="Project name used when writing a generic source-pack manifest")
    parser.add_argument("--consumer-project", help="Optional game project that consumes the source pack")
    parser.add_argument("--output", help="Optional output path for generated manifest")
    args = parser.parse_args()

    if args.scan_overworld_pack:
        output = Path(args.output).resolve() if args.output else None
        manifest = scan_overworld_pack(Path(args.scan_overworld_pack), output)
        print(f"Scanned Overworld pack: {manifest['summary']['total_files']} files")
        if output:
            print(f"Manifest written to: {output}")
        return 0

    if args.scan_source_pack:
        output = Path(args.output).resolve() if args.output else None
        manifest = scan_source_pack(
            Path(args.scan_source_pack),
            output,
            project_name=args.project_name,
            consumer_project=args.consumer_project,
        )
        print(f"Scanned source pack: {manifest['summary']['total_files']} files")
        if output:
            print(f"Manifest written to: {output}")
        return 0

    return process_assets(dry_run=args.dry_run)


if __name__ == "__main__":
    raise SystemExit(main())






