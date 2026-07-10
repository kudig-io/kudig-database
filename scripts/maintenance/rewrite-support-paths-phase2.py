#!/usr/bin/env python3
"""Batch-7 phase 2: catch remaining path references the wikilink rewriter missed.

Handles:
  - YAML frontmatter (between --- fences) path strings
  - Standalone path tokens like `scripts/maintenance/foo.py` in prose
  - Obsidian vault paths in .env / .plist / .sh files

Run:
    python3 scripts/maintenance/rewrite-support-paths-phase2.py [--apply]
"""
from __future__ import annotations

import os
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

RENAME = {
    "assets": "资产",
    "concepts": "概念",
    "docs": "文档",
    "entities": "实体",
    "release": "发布",
    "research": "研究",
    "scripts": "脚本",
    "skills": "技能",
    "synthesis": "综合",
    "tags": "标签",
    "web": "站点",
    "_archives": "归档",
    "_meta": "元数据",
    "_reports": "报告",
}

# Do not touch files under these subtrees (immutable snapshots)
FROZEN_PREFIXES = ("发布/package/", "release/package/")

# Build regex: match any of the old names when they appear as a path segment
# bounded by start-of-string / whitespace / quote / ( and followed by /
OLD_NAMES = sorted(RENAME.keys(), key=len, reverse=True)
# e.g. matches "_archives/" or "scripts/" when preceded by word-boundary-ish context
PATH_TOKEN_RE = re.compile(
    r"(?<![\w\-])(" + "|".join(re.escape(n) for n in OLD_NAMES) + r")/"
)


def rewrite_text(text: str) -> tuple[str, int]:
    count = 0

    def _repl(m: re.Match) -> str:
        nonlocal count
        old = m.group(1)
        count += 1
        return RENAME[old] + "/"

    new_text = PATH_TOKEN_RE.sub(_repl, text)
    return new_text, count


def main() -> int:
    apply = "--apply" in sys.argv
    scanned = changed = total_subs = 0

    targets = []
    for root, _dirs, files in os.walk(REPO):
        rel = os.path.relpath(root, REPO)
        if any(rel.startswith(fp.rstrip("/")) for fp in FROZEN_PREFIXES):
            continue
        for f in files:
            if not f.endswith((".md", ".yaml", ".yml", ".env", ".example",
                               ".sh", ".plist", ".toml", ".json")):
                continue
            p = Path(root) / f
            rel_p = str(p.relative_to(REPO))
            if any(rel_p.startswith(fp.rstrip("/")) for fp in FROZEN_PREFIXES):
                continue
            targets.append((p, rel_p))

    for p, rel_p in targets:
        scanned += 1
        try:
            text = p.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        new_text, n = rewrite_text(text)
        if n > 0:
            if apply:
                p.write_text(new_text, encoding="utf-8")
                print(f"rewrite {rel_p} ({n} subs)")
            else:
                print(f"WOULD {rel_p} ({n} subs)")
            changed += 1
            total_subs += n

    print(f"scanned={scanned} changed={changed} subs={total_subs} apply={apply}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
