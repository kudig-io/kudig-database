#!/usr/bin/env python3
"""
Fix unidirectional links and index-page orphans by adding backlinks.

Strategy:
1. For pages with outgoing >= 5 and incoming == 0 (link emitters),
   ask their link targets to link back.
2. For index pages (topic-index/, merged-indexes/, README.md in domains),
   add backlinks from linked concepts.
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


def rebuild_frontmatter(fm: dict, content: str) -> str:
    """Rebuild content with updated frontmatter."""
    parts = content.split("---", 2)
    if len(parts) < 3:
        return content
    new_fm = yaml.dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
    return f"---\n{new_fm}---{parts[2]}"


def add_related_link(target_path: Path, source_rel_path: str, source_title: str) -> bool:
    """Add a backlink in the target page's ## Related section."""
    try:
        content = target_path.read_text(encoding="utf-8")
    except Exception:
        return False

    # Check if already linked
    if f"[[{source_rel_path.replace('.md', '')}" in content:
        return False
    if f"[[{source_title}" in content:
        return False

    # Also check by filename
    source_filename = Path(source_rel_path).stem
    if f"[[{source_filename}" in content:
        return False

    body = get_body(content)

    related_section = "\n## Related\n\n"
    link_line = f"- [[{source_rel_path.replace('.md', '')}|{source_title}]] — Cross-reference\n"

    if "## Related" in content:
        # Append to existing section (before next ## or end of file)
        idx = content.find("## Related")
        section_start = idx + len("## Related")
        next_header = content.find("\n## ", section_start)
        if next_header == -1:
            # Append at end
            if not content.endswith("\n"):
                content += "\n"
            content += link_line
        else:
            # Insert before next header
            content = content[:next_header] + link_line + content[next_header:]
    else:
        # Add new section at end
        if not content.endswith("\n"):
            content += "\n"
        content += related_section + link_line

    target_path.write_text(content, encoding="utf-8")
    return True


def main():
    # Load stats
    output_dir = VAULT_ROOT / ".claude" / "scripts" / "output"
    with open(output_dir / "page_stats.json", "r", encoding="utf-8") as f:
        page_stats = json.load(f)

    # Build filename → paths mapping
    filename_to_paths = defaultdict(list)
    path_to_filename = {}
    all_pages = {}

    for root, dirs, files in os.walk(VAULT_ROOT):
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for f in files:
            if f.endswith(".md"):
                md_path = Path(root) / f
                rel_path = str(md_path.relative_to(VAULT_ROOT))
                if rel_path in ("index.md", "log.md", "hot.md"):
                    continue
                filename = md_path.stem.lower()
                filename_to_paths[filename].append(rel_path)
                path_to_filename[rel_path] = md_path.stem
                all_pages[rel_path] = md_path

    # Identify link emitters: outgoing >= 5, incoming == 0
    emitters = []
    for rel_path, stats in page_stats.items():
        if stats["outgoing"] >= 5 and stats["incoming"] == 0:
            emitters.append(rel_path)

    print(f"Link emitters found: {len(emitters)}")

    # Also identify index pages
    index_pages = [p for p in all_pages if "/topic-index/" in p or "/merged-indexes/" in p]
    print(f"Index pages found: {len(index_pages)}")

    backlinks_added = 0
    pages_modified = set()

    # Process emitters
    for emitter_path in emitters:
        md_path = all_pages.get(emitter_path)
        if not md_path or not md_path.exists():
            continue

        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception:
            continue

        body = get_body(content)
        links = WIKILINK_RE.findall(body)
        fm = get_frontmatter(content)
        emitter_title = fm.get("title", "") or md_path.stem

        for link in links:
            normalized = normalize_link(link)
            target_paths = filename_to_paths.get(normalized, [])
            for target_rel in target_paths:
                if target_rel == emitter_path:
                    continue
                target_md = all_pages.get(target_rel)
                if not target_md:
                    continue
                if add_related_link(target_md, emitter_path, emitter_title):
                    backlinks_added += 1
                    pages_modified.add(target_rel)

    print(f"Backlinks added from emitters: {backlinks_added} across {len(pages_modified)} pages")

    # Process index pages: they should be linked from their targets
    index_backlinks = 0
    for idx_path in index_pages:
        md_path = all_pages.get(idx_path)
        if not md_path or not md_path.exists():
            continue

        try:
            content = md_path.read_text(encoding="utf-8")
        except Exception:
            continue

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
                    index_backlinks += 1
                    pages_modified.add(target_rel)

    print(f"Backlinks added from index pages: {index_backlinks}")
    print(f"Total pages modified: {len(pages_modified)}")

    # Save report
    report = {
        "emitters_count": len(emitters),
        "index_pages_count": len(index_pages),
        "backlinks_added": backlinks_added + index_backlinks,
        "pages_modified": len(pages_modified),
    }
    with open(output_dir / "backlink_report.json", "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
