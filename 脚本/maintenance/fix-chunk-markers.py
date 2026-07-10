#!/usr/bin/env python3
"""
清理生成器残留的 chunk 标记。

模式：
  #<!-- chunk: 核心特性 -->## 核心特性    ->  ## 核心特性
  ##<!-- chunk: 配置 -->## 配置            ->  ## 配置
  #<!-- chunk: 章节描述 -->（行尾）         ->  删除整行

逻辑：去掉行首的 `#{1,6}<!-- chunk:...-->` 前缀，保留其后的标题文本；
若替换后整行为空则删除该行。
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXCLUDE_DIRS = {
    "_archives", "_archived-release-notes", "node_modules", ".venv",
    ".git", ".ruff_cache", "__pycache__", "site", "web",
}

CHUNK_RE = re.compile(r"^(#{1,6})<!-- chunk:[^>]*-->")


def iter_md_files() -> list[Path]:
    out = []
    for p in ROOT.rglob("*.md"):
        if any(part in EXCLUDE_DIRS for part in p.parts):
            continue
        out.append(p)
    return sorted(out)


def process(path: Path, write: bool) -> int:
    lines = path.read_text(encoding="utf-8").split("\n")
    new_lines = []
    removed = 0
    for line in lines:
        m = CHUNK_RE.match(line)
        if not m:
            new_lines.append(line)
            continue
        rest = line[m.end():]
        if rest.strip() == "":
            removed += 1
            continue
        removed += 1
        new_lines.append(rest)
    if removed == 0:
        return 0
    new_text = "\n".join(new_lines)
    if write and new_text != path.read_text(encoding="utf-8"):
        path.write_text(new_text, encoding="utf-8")
    return removed


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    total = 0
    files = 0
    for p in iter_md_files():
        n = process(p, write=False)
        if n > 0:
            files += 1
            total += n
            if not args.dry_run:
                process(p, write=True)
    mode = "DRY-RUN" if args.dry_run else "EXECUTED"
    print(f"=== {mode}: {files} 文件, {total} 处 chunk 标记清理 ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
