#!/usr/bin/env python3
"""
Analyze wikilinks in the vault: count incoming/outgoing links and identify true orphans.
"""

import os
import json
import re
import yaml
from pathlib import Path
from collections import defaultdict

VAULT_ROOT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
EXCLUDE_DIRS = {
    ".git", ".obsidian", ".claude", ".venv", "_archives",
    ".understand-anything", ".ruff_cache", ".codebuddy", ".comate",
    ".wiki-meta", ".zread", "_staging", "assets", "web"
}

WIKILINK_RE = re.compile(r'\[\[([^\]|]+)(?:\|[^\]]+)?\]\]')


def extract_frontmatter(content: str) -> dict:
    if not content.startswith("---"):
        return {}
    parts = content.split("---", 2)
    if len(parts) < 3:
        return {}
    try:
        fm = yaml.safe_load(parts[1])
        if not isinstance(fm, dict):
            return {}
        return fm or {}
    except Exception:
        return {}


def get_body(rel_path: str, content: str) -> str:
    """Get body text after frontmatter."""
    if not content.startswith("---"):
        return content
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    return parts[2]


def normalize_link(link_text: str) -> str:
    """Normalize a wikilink target to a page name."""
    # Remove alias display part
    link = link_text.split("|")[0].strip()
    # Take basename if path is given
    link = link.split("/")[-1].strip()
    return link.lower()


def main():
    all_files = []
    for root, dirs, files in os.walk(VAULT_ROOT):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for f in files:
            if f.endswith(".md"):
                all_files.append(Path(root) / f)

    # Build filename → path mapping
    filename_to_paths = defaultdict(list)
    path_to_filename = {}
    pages = {}

    for md_path in all_files:
        rel_path = str(md_path.relative_to(VAULT_ROOT))
        if rel_path in ("index.md", "log.md", "hot.md"):
            continue

        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception:
            continue

        fm = extract_frontmatter(content)
        title = fm.get("title", "") or md_path.stem
        filename = md_path.stem

        filename_to_paths[filename.lower()].append(rel_path)
        path_to_filename[rel_path] = filename

        pages[rel_path] = {
            "path": rel_path,
            "filename": filename,
            "title": title,
            "incoming": set(),
            "outgoing": set(),
            "incoming_count": 0,
            "outgoing_count": 0,
        }

    # Count outgoing links
    for md_path in all_files:
        rel_path = str(md_path.relative_to(VAULT_ROOT))
        if rel_path not in pages:
            continue
        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception:
            continue

        body = get_body(rel_path, content)
        links = WIKILINK_RE.findall(body)
        for link in links:
            normalized = normalize_link(link)
            if normalized == pages[rel_path]["filename"].lower():
                continue  # Skip self-reference
            pages[rel_path]["outgoing"].add(normalized)

    # Count incoming links (body only, skip frontmatter relationships)
    for md_path in all_files:
        rel_path = str(md_path.relative_to(VAULT_ROOT))
        if rel_path not in pages:
            continue
        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception:
            continue

        body = get_body(rel_path, content)
        links = WIKILINK_RE.findall(body)
        for link in links:
            normalized = normalize_link(link)
            if normalized == pages[rel_path]["filename"].lower():
                continue
            # Find target page
            target_paths = filename_to_paths.get(normalized, [])
            for target_path in target_paths:
                if target_path != rel_path:
                    pages[target_path]["incoming"].add(pages[rel_path]["filename"].lower())

    # Count and identify orphans
    orphans = []
    hub_pages = []
    total_pages = len(pages)

    for rel_path, info in pages.items():
        info["outgoing_count"] = len(info["outgoing"])
        info["incoming_count"] = len(info["incoming"])
        if info["incoming_count"] == 0 and info["outgoing_count"] == 0:
            orphans.append(rel_path)
        if info["incoming_count"] >= 8 or info["outgoing_count"] >= 8:
            hub_pages.append({
                "path": rel_path,
                "incoming": info["incoming_count"],
                "outgoing": info["outgoing_count"],
            })

    # Cohesion: ratio of connected pages to total pages
    connected = total_pages - len(orphans)
    cohesion = connected / total_pages if total_pages > 0 else 0

    print(f"=== Link Analysis ===")
    print(f"Total pages: {total_pages}")
    print(f"Orphan pages: {len(orphans)} ({len(orphans)/total_pages*100:.1f}%)")
    print(f"Cohesion: {cohesion:.4f}")
    print(f"Hub pages (≥8 links): {len(hub_pages)}")
    print(f"\nTop 20 most connected pages:")
    for p in sorted(hub_pages, key=lambda x: -(x["incoming"] + x["outgoing"]))[:20]:
        print(f"  {p['path']}: in={p['incoming']}, out={p['outgoing']}")

    # Save orphans list
    output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"
    with open(output_dir / "orphans.json", "w", encoding="utf-8") as f:
        json.dump(orphans, f, ensure_ascii=False, indent=2)

    # Save page stats
    stats = {k: {"incoming": v["incoming_count"], "outgoing": v["outgoing_count"]} for k, v in pages.items()}
    with open(output_dir / "page_stats.json", "w", encoding="utf-8") as f:
        json.dump(stats, f, ensure_ascii=False, indent=2)

    print(f"\nSaved orphans.json ({len(orphans)} orphans) and page_stats.json")


if __name__ == "__main__":
    main()
