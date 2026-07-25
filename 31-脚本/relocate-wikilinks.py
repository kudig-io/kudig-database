#!/usr/bin/env python3
"""
relocate-wikilinks.py
批量重写 Obsidian wikilinks 以匹配目录迁移后的新路径。

用法:
    python3 scripts/relocate-wikilinks.py --dry-run    # 预览变更（不写入）
    python3 scripts/relocate-wikilinks.py --execute    # 执行变更
    python3 scripts/relocate-wikilinks.py --verify     # 验证无残留旧路径
"""

import re
import sys
from pathlib import Path

# 项目根目录（脚本在 scripts/ 下，上一层即根目录）
SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent

# ── 路径映射表 ──────────────────────────────────────────────────────────────
# 旧前缀 → 新前缀（wikilink [[旧/...]] → [[新/...]]）
MAPPINGS = {
    "references":    "entities",
    "synthesis":     "concepts",
    "best-practices": "skills",
    "journal":       "_meta/journal",
    "release-notes": "_reports/release-notes",
    "video-scripts": "scripts/video-scripts",
    "templates":     "scripts/templates",
    "prompts":       "scripts/prompts",
}

# 不参与重写的目录（归档）
EXCLUDE_DIRS = {"_archives", ".git", "node_modules", ".venv", "web"}

# ── 构建正则 ─────────────────────────────────────────────────────────────────
# 匹配 [[old_prefix/rest]] 或 [[old_prefix/rest|display text]]
def build_patterns():
    """为每个映射构建正则，返回 [(pattern, replacement), ...]"""
    patterns = []
    for old, new in MAPPINGS.items():
        # 匹配 [[old/...]]  其中 ... 不能包含 ] ，直到遇到 ]]
        # 组1 = 路径+可选显示文本部分
        pat = re.compile(
            r"\[\[" + re.escape(old) + r"/([^\]]*?)\]\]",
            re.UNICODE,
        )
        repl = f"[[{new}/\\1]]"
        patterns.append((pat, repl, old, new))
    return patterns


def collect_md_files():
    """收集所有 .md 文件，排除 EXCLUDE_DIRS"""
    md_files = []
    for f in ROOT.rglob("*.md"):
        # 检查是否在排除目录中
        rel = f.relative_to(ROOT)
        parts = rel.parts
        if any(p in EXCLUDE_DIRS for p in parts):
            continue
        md_files.append(f)
    return md_files


def dry_run(patterns, md_files):
    """预览变更，打印将要修改的文件和替换内容"""
    total_links = 0
    total_files = 0
    print("=" * 70)
    print("DRY-RUN: 预览 wikilink 重写变更")
    print("=" * 70)

    for fpath in sorted(md_files):
        try:
            content = fpath.read_text(encoding="utf-8")
        except Exception as e:
            print(f"  [SKIP] {fpath.relative_to(ROOT)}: {e}")
            continue

        file_changes = []
        new_content = content
        for pat, repl, old, new in patterns:
            matches = pat.findall(new_content)
            if matches:
                count = len(matches)
                file_changes.append((old, new, count))
                new_content = pat.sub(repl, new_content)

        if file_changes:
            rel = fpath.relative_to(ROOT)
            print(f"\n  FILE: {rel}")
            for old, new, count in file_changes:
                print(f"    [[{old}/...]] → [[{new}/...]]  ({count} links)")
                total_links += count
            total_files += 1

    print("\n" + "=" * 70)
    print(f"SUMMARY: {total_files} files, {total_links} links to rewrite")
    print("=" * 70)
    return total_links


def execute(patterns, md_files):
    """执行变更，原地替换文件内容"""
    total_links = 0
    total_files = 0

    for fpath in md_files:
        try:
            content = fpath.read_text(encoding="utf-8")
        except Exception as e:
            print(f"  [SKIP] {fpath.relative_to(ROOT)}: {e}")
            continue

        new_content = content
        file_link_count = 0
        for pat, repl, old, new in patterns:
            matches = pat.findall(new_content)
            if matches:
                file_link_count += len(matches)
                new_content = pat.sub(repl, new_content)

        if new_content != content:
            fpath.write_text(new_content, encoding="utf-8")
            rel = fpath.relative_to(ROOT)
            print(f"  [OK] {rel}: {file_link_count} links rewritten")
            total_links += file_link_count
            total_files += 1

    print(f"\nDONE: {total_files} files modified, {total_links} links rewritten")
    return total_links


def verify(patterns, md_files):
    """验证无残留旧路径 wikilinks"""
    residual = []
    for fpath in md_files:
        try:
            content = fpath.read_text(encoding="utf-8")
        except Exception:
            continue
        for pat, repl, old, new in patterns:
            matches = pat.findall(content)
            if matches:
                rel = fpath.relative_to(ROOT)
                residual.append((rel, old, len(matches)))

    if residual:
        print("FAIL: 发现残留旧路径 wikilinks：")
        for rel, old, count in residual:
            print(f"  {rel}: [[{old}/...]] × {count}")
        print(f"\n共 {len(residual)} 处残留，请手动修复。")
        sys.exit(1)
    else:
        print("OK: 无残留旧路径 wikilinks，验证通过！")


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("--dry-run", "--execute", "--verify"):
        print(__doc__)
        sys.exit(1)

    mode = sys.argv[1]
    patterns = build_patterns()
    md_files = collect_md_files()
    print(f"扫描到 {len(md_files)} 个 .md 文件（已排除归档和工具目录）\n")

    if mode == "--dry-run":
        dry_run(patterns, md_files)
    elif mode == "--execute":
        execute(patterns, md_files)
    elif mode == "--verify":
        verify(patterns, md_files)


if __name__ == "__main__":
    main()
