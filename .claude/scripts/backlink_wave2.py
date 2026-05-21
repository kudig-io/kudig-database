#!/usr/bin/env python3
"""
Wave 2 backlinking:
1. Index pages with high outgoing but 0 incoming -> get backlinks from targets
2. Release notes -> link from their component entity pages
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


def get_body(content: str) -> str:
    if not content.startswith("---"):
        return content
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    return parts[2]


def normalize_link(link_text: str) -> str:
    link = link_text.split("|")[0].strip()
    return link.split("/")[-1].strip().lower()


def get_frontmatter(content: str) -> dict:
    if not content.startswith("---"):
        return {}
    parts = content.split("---", 2)
    if len(parts) < 3:
        return {}
    try:
        return yaml.safe_load(parts[1]) or {}
    except Exception:
        return {}


def add_related_link(target_path: Path, source_rel: str, source_title: str) -> bool:
    try:
        content = target_path.read_text(encoding="utf-8")
    except Exception:
        return False

    source_filename = Path(source_rel).stem
    checks = [
        f"[[{source_rel.replace('.md', '')}",
        f"[[{source_filename}",
        f"[[{source_title}",
    ]
    for check in checks:
        if check in content:
            return False

    link_line = f"- [[{source_rel.replace('.md', '')}|{source_title}]]\n"

    if "## Related" in content:
        idx = content.find("## Related")
        section_start = idx + len("## Related")
        next_header = content.find("\n## ", section_start)
        if next_header == -1:
            if not content.endswith("\n"):
                content += "\n"
            content += link_line
        else:
            content = content[:next_header] + link_line + content[next_header:]
    else:
        if not content.endswith("\n"):
            content += "\n"
        content += "\n## Related\n\n" + link_line

    target_path.write_text(content, encoding="utf-8")
    return True


def main():
    output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"

    with open(output_dir / "page_stats.json", "r", encoding="utf-8") as f:
        page_stats = json.load(f)

    with open(output_dir / "orphans.json", "r", encoding="utf-8") as f:
        orphans = json.load(f)

    # Build mappings
    all_pages = {}
    filename_to_paths = defaultdict(list)
    path_to_filename = {}

    for root, dirs, files in os.walk(VAULT_ROOT):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for f in files:
            if f.endswith(".md"):
                md_path = Path(root) / f
                rel_path = str(md_path.relative_to(VAULT_ROOT))
                if rel_path in ("index.md", "log.md", "hot.md"):
                    continue
                all_pages[rel_path] = md_path
                filename = md_path.stem.lower()
                filename_to_paths[filename].append(rel_path)
                path_to_filename[rel_path] = md_path.stem

    backlinks_added = 0
    pages_modified = set()

    # Strategy 1: Index pages (0 incoming, high outgoing) get backlinks
    index_pages = [p for p in page_stats if "/topic-index/" in p or "/merged-indexes/" in p]
    index_zero_in = [p for p in index_pages if page_stats[p]["incoming"] == 0]
    print(f"Index pages with 0 incoming: {len(index_zero_in)}")

    for idx_path in index_zero_in:
        md_path = all_pages.get(idx_path)
        if not md_path:
            continue
        content = md_path.read_text(encoding="utf-8")
        body = get_body(content)
        links = WIKILINK_RE.findall(body)
        fm = get_frontmatter(content)
        idx_title = fm.get("title", "") or md_path.stem

        for link in links:
            normalized = normalize_link(link)
            target_paths = filename_to_paths.get(normalized, [])
            for target_rel in target_paths:
                if target_rel == idx_path:
                    continue
                target_md = all_pages.get(target_rel)
                if not target_md:
                    continue
                if add_related_link(target_md, idx_path, idx_title):
                    backlinks_added += 1
                    pages_modified.add(target_rel)

    print(f"Backlinks from index pages: {backlinks_added}")

    # Strategy 2: Release notes -> link from component entity pages
    # Extract component name from title
    release_note_orphans = [p for p in orphans if "/topic-release-notes/" in p]
    print(f"Release note orphans: {len(release_note_orphans)}")

    entity_linked = 0
    for rel_path in release_note_orphans:
        md_path = all_pages.get(rel_path)
        if not md_path:
            continue
        content = md_path.read_text(encoding="utf-8")
        fm = get_frontmatter(content)
        title = fm.get("title", "") or md_path.stem

        # Extract component name from title
        # "opa v0.68 Release Notes" -> "opa"
        # "prometheus v2.55 Release Notes" -> "prometheus"
        # "opentelemetry-collector v0.101 Release Notes" -> "opentelemetry-collector"
        match = re.match(r'^([^v\d]+)', title.strip())
        if not match:
            continue
        component = match.group(1).strip().lower()
        # Handle common variations
        component = component.replace(' ', '-')

        # Find entity page
        target_paths = filename_to_paths.get(component, [])
        if not target_paths:
            # Try without trailing words
            component_base = component.split('-')[0]
            target_paths = filename_to_paths.get(component_base, [])

        if not target_paths:
            continue

        for target_rel in target_paths:
            if target_rel == rel_path:
                continue
            target_md = all_pages.get(target_rel)
            if not target_md:
                continue
            if add_related_link(target_md, rel_path, title):
                entity_linked += 1
                pages_modified.add(target_rel)
                break  # Only link from first matching entity

    print(f"Release notes linked from entities: {entity_linked}")
    print(f"Total pages modified: {len(pages_modified)}")

    report = {
        "index_backlinks": backlinks_added,
        "release_note_links": entity_linked,
        "pages_modified": len(pages_modified),
    }
    with open(output_dir / "backlink_wave2_report.json", "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
