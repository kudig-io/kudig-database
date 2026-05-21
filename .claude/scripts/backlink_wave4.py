#!/usr/bin/env python3
"""
Wave 4: Final cleanup for the last 49 orphans.
"""

import os
import json
import re
from pathlib import Path
from collections import defaultdict

VAULT_ROOT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
EXCLUDE_DIRS = {
    ".git", ".obsidian", ".claude", ".venv", "_archives",
    ".understand-anything", ".ruff_cache", ".codebuddy", ".comate",
    ".wiki-meta", ".zread", "_staging", "assets", "web"
}


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

    with open(output_dir / "orphans.json", "r", encoding="utf-8") as f:
        orphans = json.load(f)

    all_pages = {}
    filename_to_paths = defaultdict(list)

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

    backlinks_added = 0
    pages_modified = set()

    for rel_path in orphans:
        md_path = all_pages.get(rel_path)
        if not md_path:
            continue
        content = md_path.read_text(encoding="utf-8")
        title = md_path.stem

        # Strategy: find a parent directory README or index to link from
        parts = rel_path.split("/")
        linked = False

        # Try linking from known hub pages based on path patterns
        if "kubernetes/CHANGELOG" in rel_path:
            targets = filename_to_paths.get("kubernetes", [])
            for target in targets:
                if target != rel_path and add_related_link(all_pages[target], rel_path, title):
                    backlinks_added += 1
                    pages_modified.add(target)
                    linked = True
                    break
        elif "minikube" in rel_path.lower():
            targets = filename_to_paths.get("minikube", [])
            if not targets:
                targets = filename_to_paths.get("kubernetes", [])
            for target in targets:
                if target != rel_path and add_related_link(all_pages[target], rel_path, title):
                    backlinks_added += 1
                    pages_modified.add(target)
                    linked = True
                    break
        elif "openrouter" in rel_path.lower():
            # Link from ai-agent-README or similar
            targets = filename_to_paths.get("ai-agent-README", []) + filename_to_paths.get("ai-agent-MOC", [])
            for target in targets:
                if target != rel_path and add_related_link(all_pages[target], rel_path, title):
                    backlinks_added += 1
                    pages_modified.add(target)
                    linked = True
                    break
        elif "opencode" in rel_path.lower():
            targets = filename_to_paths.get("ai-agent-README", []) + filename_to_paths.get("ai-agent-MOC", [])
            for target in targets:
                if target != rel_path and add_related_link(all_pages[target], rel_path, title):
                    backlinks_added += 1
                    pages_modified.add(target)
                    linked = True
                    break

        # Fallback: link from parent directory README.md or index.md
        if not linked:
            for i in range(len(parts) - 1, 0, -1):
                parent = "/".join(parts[:i])
                for candidate in ["README.md", "index.md", "MOC.md"]:
                    candidate_path = parent + "/" + candidate if parent else candidate
                    if candidate_path in all_pages and candidate_path != rel_path:
                        if add_related_link(all_pages[candidate_path], rel_path, title):
                            backlinks_added += 1
                            pages_modified.add(candidate_path)
                            linked = True
                            break
                if linked:
                    break

    print(f"Final wave backlinks added: {backlinks_added}")
    print(f"Pages modified: {len(pages_modified)}")

    report = {
        "final_backlinks": backlinks_added,
        "pages_modified": len(pages_modified),
    }
    with open(output_dir / "backlink_wave4_report.json", "w", encoding="utf-8") as f:
        json.dump(report, f, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
