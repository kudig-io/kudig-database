#!/usr/bin/env python3
"""Batch add all topic-release-notes/ files to .manifest.json"""
import json
import hashlib
import os
from datetime import datetime, timezone
from pathlib import Path

VAULT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
MANIFEST_PATH = VAULT / ".manifest.json"
RELEASE_NOTES_DIR = VAULT / "topic-release-notes"

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return f"sha256:{h.hexdigest()}"

def main():
    # Load existing manifest
    with open(MANIFEST_PATH, "r") as f:
        manifest = json.load(f)
    
    existing = set(manifest.get("sources", {}).keys())
    now = datetime.now(timezone.utc).isoformat()
    
    added = 0
    skipped = 0
    
    for md_file in sorted(RELEASE_NOTES_DIR.rglob("*.md")):
        rel = str(md_file.relative_to(VAULT))
        
        if rel in existing:
            skipped += 1
            continue
        
        stat = md_file.stat()
        manifest["sources"][rel] = {
            "ingested_at": now,
            "size_bytes": stat.st_size,
            "modified_at": datetime.fromtimestamp(stat.st_mtime, tz=timezone.utc).isoformat(),
            "content_hash": sha256_file(md_file),
            "source_type": "release-notes",
            "project": None
        }
        added += 1
    
    # Write manifest
    with open(MANIFEST_PATH, "w") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
    
    print(f"Added: {added}")
    print(f"Skipped (already in manifest): {skipped}")
    print(f"Total sources in manifest: {len(manifest['sources'])}")

if __name__ == "__main__":
    main()
