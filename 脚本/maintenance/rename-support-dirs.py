#!/usr/bin/env python3
"""Batch-7: rename 15 English root support directories to Chinese short names,
and rewrite wikilink / markdown-link references across tracked .md files.

Mapping (kept intentionally 1-to-1 with the plan):
    assets       → 资产
    code         → 源码
    concepts     → 概念
    docs         → 文档
    entities     → 实体
    release      → 发布
    research     → 研究
    scripts      → 脚本
    skills       → 技能
    synthesis    → 综合
    tags         → 标签
    web          → 站点
    _archives    → 归档
    _meta        → 元数据
    _reports     → 报告

Usage:
    python3 scripts/maintenance/rename-support-dirs.py            # dry-run
    python3 scripts/maintenance/rename-support-dirs.py --apply    # execute mv
    python3 scripts/maintenance/rename-support-dirs.py --apply --rewrite   # mv + rewrite links
"""
from __future__ import annotations

import os
import re
import subprocess
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

# Freeze: do NOT rewrite links inside these subtrees (they are immutable snapshots)
FROZEN_PREFIXES = ("发布/package/", "release/package/")

WIKILINK_RE = re.compile(r"\[\[([^\]\|]+)(\|[^\]]+)?\]\]")
MDLINK_RE = re.compile(r"\[([^\]]*)\]\(([^)]+)\)")


def do_rename(*, apply: bool) -> tuple[int, int]:
    ok = fail = 0
    for old, new in RENAME.items():
        src = REPO / old
        dst = REPO / new
        if not src.exists():
            print(f"SKIP {old}/ (not found)")
            continue
        if dst.exists():
            print(f"SKIP {old}/ → {new}/ (target exists)")
            fail += 1
            continue
        if apply:
            r = subprocess.run(["git", "mv", old, new], cwd=REPO,
                               capture_output=True, text=True)
            if r.returncode == 0:
                print(f"OK   {old}/ → {new}/")
                ok += 1
            else:
                print(f"FAIL {old}/ → {new}/ :: {r.stderr.strip()}")
                fail += 1
        else:
            print(f"WOULD {old}/ → {new}/")
            ok += 1
    return ok, fail


def rewrite_path(path: str) -> str:
    """Replace leading English root dir segment with Chinese."""
    # Try leading-segment form: "scripts/foo/bar" → "脚本/foo/bar"
    for old, new in RENAME.items():
        if path == old:
            return new
        if path.startswith(old + "/"):
            return new + path[len(old):]
    # Also handle ../scripts/foo style relative paths (leave ../ intact)
    m = re.match(r"^((?:\.\./)+)(.*)$", path)
    if m:
        prefix, rest = m.group(1), m.group(2)
        for old, new in RENAME.items():
            if rest == old:
                return prefix + new
            if rest.startswith(old + "/"):
                return prefix + new + rest[len(old):]
    return path


def rewrite_file(p: Path) -> int:
    try:
        text = p.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return 0

    changed = 0

    def _wiki(m: re.Match) -> str:
        nonlocal changed
        target = m.group(1)
        alias = m.group(2) or ""
        new = rewrite_path(target)
        if new != target:
            changed += 1
            return f"[[{new}{alias}]]"
        return m.group(0)

    def _md(m: re.Match) -> str:
        nonlocal changed
        label, target = m.group(1), m.group(2)
        # Skip http(s):// and anchors
        if target.startswith(("http://", "https://", "#", "mailto:")):
            return m.group(0)
        new = rewrite_path(target)
        if new != target:
            changed += 1
            return f"[{label}]({new})"
        return m.group(0)

    new_text = WIKILINK_RE.sub(_wiki, text)
    new_text = MDLINK_RE.sub(_md, new_text)

    if new_text != text:
        p.write_text(new_text, encoding="utf-8")
    return changed


def do_rewrite(*, apply: bool) -> tuple[int, int]:
    scanned = changed_files = 0
    for root, _dirs, files in os.walk(REPO):
        rel = os.path.relpath(root, REPO)
        if any(rel.startswith(fp.rstrip("/")) for fp in FROZEN_PREFIXES):
            continue
        for f in files:
            if not f.endswith(".md"):
                continue
            p = Path(root) / f
            rel_p = str(p.relative_to(REPO))
            if any(rel_p.startswith(fp.rstrip("/")) for fp in FROZEN_PREFIXES):
                continue
            scanned += 1
            if apply:
                n = rewrite_file(p)
                if n > 0:
                    changed_files += 1
                    print(f"rewrite {rel_p} ({n} links)")
            else:
                # dry-run: count without writing
                try:
                    text = p.read_text(encoding="utf-8")
                except UnicodeDecodeError:
                    continue
                hits = 0
                for m in WIKILINK_RE.finditer(text):
                    if rewrite_path(m.group(1)) != m.group(1):
                        hits += 1
                for m in MDLINK_RE.finditer(text):
                    if m.group(2).startswith(("http://", "https://", "#", "mailto:")):
                        continue
                    if rewrite_path(m.group(2)) != m.group(2):
                        hits += 1
                if hits > 0:
                    changed_files += 1
                    print(f"WOULD {rel_p} ({hits} links)")
    return scanned, changed_files


def main() -> int:
    apply = "--apply" in sys.argv
    rewrite = "--rewrite" in sys.argv

    print("=== PHASE 1: rename root directories ===")
    ok, fail = do_rename(apply=apply)
    print(f"rename ok={ok} fail={fail} apply={apply}")

    if rewrite:
        print("\n=== PHASE 2: rewrite link references ===")
        scanned, changed = do_rewrite(apply=apply)
        print(f"rewrite scanned={scanned} changed={changed} apply={apply}")

    return 0 if fail == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
