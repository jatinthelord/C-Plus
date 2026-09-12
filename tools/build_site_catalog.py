#!/usr/bin/env python3
"""Index real C+ packages, libraries, and sources into unique website data files."""
from __future__ import annotations

import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "website"
DATA = WEB / "data"
SKIP_DIRS = {
    ".git",
    "build",
    "main",
    "install-test",
    "node_modules",
    ".edge-auth",
    ".edge-headless",
    ".edge-auth-http",
    ".novagit",
}


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8", "replace")).hexdigest()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")


def first_paragraph(path: Path) -> str:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return ""
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
            comment = stripped.lstrip("/* ").rstrip("*/ ")
            if len(comment) > 12:
                return comment[:240]
        if stripped.startswith("#include") or stripped.startswith("#pragma"):
            continue
        if stripped.startswith("namespace") or stripped.startswith("struct"):
            return f"{path.name} public C+ surface"
    return f"{path.name} in the C+ tree"


def index_packages() -> list[dict]:
    registry = json.loads((ROOT / "packages" / "csx" / "index.json").read_text(encoding="utf-8"))
    write_json(DATA / "csx-index.json", registry)
    packages = []
    for item in registry["packages"]:
        record = {
            "name": item["name"],
            "version": item["version"],
            "description": item["description"],
            "headers": item.get("headers", []),
            "depends": item.get("depends", []),
            "repository": "Core" if item["name"].startswith("csp") else "Extra",
            "arch": "any",
            "license": "MIT",
        }
        packages.append(record)
        write_json(DATA / "packages" / f"{item['name']}.json", record)
    write_json(DATA / "packages.json", {"generated": now(), "packages": packages})
    return packages


def index_libraries() -> list[dict]:
    include = ROOT / "include"
    libraries = []
    for path in sorted(include.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(ROOT).as_posix()
        record = {
            "path": relative,
            "name": f"<{path.relative_to(include).as_posix()}>",
            "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "summary": first_paragraph(path),
        }
        libraries.append(record)
        stem = path.relative_to(include).as_posix().replace("/", "_").replace(".", "_")
        write_json(DATA / "libraries" / f"{stem}.json", record)
    write_json(DATA / "libraries.json", {"generated": now(), "libraries": libraries})
    return libraries


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def index_sources() -> list[dict]:
    roots = ["src", "include", "examples", "cmd", "backend", "website", "packages"]
    extras = ["cspc.cpp", "CMakeLists.txt", "syntax.md", "README.md", "LICENSE"]
    files: list[Path] = []
    for name in roots:
        base = ROOT / name
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file() and not should_skip(path) and "website" in path.parts and "data" in path.parts:
                continue
            if path.is_file() and not should_skip(path) and path.suffix.lower() in {
                ".c",
                ".h",
                ".cpp",
                ".hpp",
                ".cs",
                ".wat",
                ".go",
                ".js",
                ".css",
                ".html",
                ".json",
                ".sql",
                ".md",
                ".csp",
                ".cpsm",
                ".txt",
            }:
                files.append(path)
    for name in extras:
        path = ROOT / name
        if path.is_file():
            files.append(path)
    unique = sorted({path.resolve(): path for path in files}.values(), key=lambda item: item.as_posix())
    sources = []
    for path in unique:
        relative = path.relative_to(ROOT).as_posix()
        record = {
            "file": relative,
            "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        }
        sources.append(record)
        stem = relative.replace("/", "_").replace(".", "_")
        write_json(DATA / "sources" / f"{stem}.json", record)
    write_json(
        DATA / "source-manifest.json",
        {"generated": now(), "files": [item["file"] for item in sources]},
    )
    write_json(DATA / "sources.json", {"generated": now(), "sources": sources})
    return sources


def now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def write_sql(packages: list[dict], libraries: list[dict], sources: list[dict]) -> None:
    lines = [
        "BEGIN;",
        "DELETE FROM packages;",
        "DELETE FROM libraries;",
        "DELETE FROM sources;",
    ]
    for item in packages:
        lines.append(
            "INSERT INTO packages(name, version, description, repository, arch, license) VALUES ("
            f"{sql(item['name'])}, {sql(item['version'])}, {sql(item['description'])}, "
            f"{sql(item['repository'])}, {sql(item['arch'])}, {sql(item['license'])});"
        )
    for item in libraries:
        lines.append(
            "INSERT INTO libraries(path, name, bytes, sha256, summary) VALUES ("
            f"{sql(item['path'])}, {sql(item['name'])}, {item['bytes']}, "
            f"{sql(item['sha256'])}, {sql(item['summary'])});"
        )
    for item in sources:
        lines.append(
            "INSERT INTO sources(file, bytes, sha256) VALUES ("
            f"{sql(item['file'])}, {item['bytes']}, {sql(item['sha256'])});"
        )
    lines.append("COMMIT;")
    (ROOT / "backend" / "seed.sql").parent.mkdir(parents=True, exist_ok=True)
    (ROOT / "backend" / "seed.sql").write_text("\n".join(lines) + "\n", encoding="utf-8")


def sql(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def main() -> None:
    if DATA.exists():
        for path in DATA.rglob("*"):
            if path.is_file():
                path.unlink()
    packages = index_packages()
    libraries = index_libraries()
    sources = index_sources()
    manifest = {
        "generated": now(),
        "packages": len(packages),
        "libraries": len(libraries),
        "sources": len(sources),
        "files": [
            "manifest.json",
            "csx-index.json",
            "packages.json",
            "libraries.json",
            "sources.json",
            "source-manifest.json",
        ],
    }
    write_json(DATA / "manifest.json", manifest)
    write_sql(packages, libraries, sources)
    print(
        f"catalog packages={len(packages)} libraries={len(libraries)} sources={len(sources)}"
    )


if __name__ == "__main__":
    main()
