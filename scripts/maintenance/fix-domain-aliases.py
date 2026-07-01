#!/usr/bin/env python3
"""
修正 wikilink alias 里的旧域编号。

模式：[[domain-07-platform-engineering/README.md|Domain 36: 平台工程]]
路径 domain-07 是权威，alias 显示 "Domain 36"（迁移前旧编号）错误。
修正为：[[domain-07-platform-engineering/README.md|Domain 07: 平台工程]]

安全约束：仅处理 alias 段（| 与 ]] 之间）不含额外 [[ 的链接，避免触碰
已损坏的嵌套 wikilink（那些需人工重建）。
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

# [[domain-XX-name/...| <alias 不含[> Domain NN:
PAT = re.compile(r"(\[\[domain-(\d{2})-[^\]|]+\|)([^\[\]]*?Domain )\d{1,2}(:)")


def iter_md_files() -> list[Path]:
    out = []
    for p in ROOT.rglob("*.md"):
        if any(part in EXCLUDE_DIRS for part in p.parts):
            continue
        out.append(p)
    return sorted(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    total = 0
    files = 0
    for p in iter_md_files():
        t = p.read_text(encoding="utf-8")
        o = t
        samples = []

        def repl(m):
            nonlocal total
            total += 1
            if len(samples) < 3:
                samples.append(f"  {m.group(0)[:70]} -> Domain {m.group(2)}:")
            return m.group(1) + m.group(3) + m.group(2) + m.group(4)

        t = PAT.sub(repl, t)
        if t != o:
            files += 1
            if args.verbose and samples:
                print(f"{p.relative_to(ROOT)}:")
                for s in samples:
                    print(s)
            if not args.dry_run:
                try:
                    p.write_text(t, encoding="utf-8")
                except PermissionError:
                    print(f"  [LOCKED] {p} (uchg, 跳过)")
    mode = "DRY-RUN" if args.dry_run else "EXECUTED"
    print(f"=== {mode}: {files} 文件, {total} 处域编号 alias 修正 ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
