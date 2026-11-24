#!/usr/bin/env python3
"""重排 P0 盘点：生成跨目录重复清单（机器可读）。

三类信号：
- duplicate_basenames   同名文件（跨目录，wiki+skills，排除归档）
- identical_content     正文完全相同（去 frontmatter 后哈希，抓改名副本）
- title_collisions      标题相同但正文不同（同题异版，需人工合并评审）

结构性文件（README/index/MOC 等自动生成物）单独分桶，不计入内容重复。

Usage:
  python3 code/scripts/maintenance/reorg-inventory.py            # 写入 metadata JSON + 打印摘要
  python3 code/scripts/maintenance/reorg-inventory.py --check    # 只打印摘要（CI 用）
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SCAN_ROOTS = [ROOT / "wiki", ROOT / "skills"]
EXCLUDE_PARTS = {"31-归档", ".git", "node_modules", "__pycache__"}
# 模板目录：frontmatter title 为占位符（如「<公司> <一句话故障标题>」），
# 属模板语义而非真实同题，不进重复信号（评审单 B 节豁免，2026-11-24）
TEMPLATE_PREFIXES = ("wiki/32-故障案例库/templates/",)
STRUCTURAL_NAMES = {"readme.md", "index.md", "moc.md", "summary.md", "changelog.md", "license.md"}
OUTPUT = ROOT / "maintaining" / "02-元数据" / "metadata" / "reorg-inventory.json"

FM_RE = re.compile(r"^---\n(.*?)\n---\n?(.*)$", re.S)
STUB_MARKER = "内容已并入主场"


def split_doc(text: str) -> tuple[dict, str]:
    m = FM_RE.match(text.lstrip("\ufeff"))
    if not m:
        return {}, text
    fm = {}
    for line in m.group(1).splitlines():
        if line.startswith("title:"):
            fm["title"] = line.split(":", 1)[1].strip().strip("'\"")
    return fm, m.group(2)


def iter_docs() -> list[dict]:
    docs = []
    for base in SCAN_ROOTS:
        for p in sorted(base.rglob("*.md")):
            if EXCLUDE_PARTS & set(p.parts):
                continue
            if p.relative_to(ROOT).as_posix().startswith(TEMPLATE_PREFIXES):
                continue  # 模板占位标题不进重复信号
            try:
                text = p.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            if STUB_MARKER in text:
                continue  # P1 去重桩不计入重复信号
            fm, body = split_doc(text)
            norm = re.sub(r"\s+", " ", body).strip()
            docs.append({
                "path": str(p.relative_to(ROOT)),
                "name": p.name,
                "structural": p.name.lower() in STRUCTURAL_NAMES,
                "title": (fm.get("title") or "").strip(),
                "body_hash": hashlib.sha256(norm.encode()).hexdigest()[:16],
                "lines": body.count("\n") + 1,
            })
    return docs


def group(docs: list[dict], key) -> dict:
    buckets = defaultdict(list)
    for d in docs:
        k = key(d)
        if k:
            buckets[k].append(d)
    return {k: v for k, v in buckets.items() if len(v) > 1}


def summarize(name: str, groups: dict) -> dict:
    content, structural = {}, {}
    for k, members in groups.items():
        dirs = {m["path"].rsplit("/", 1)[0] for m in members}
        entry = {
            "count": len(members),
            "cross_dir": len(dirs) > 1,
            "paths": [m["path"] for m in members],
        }
        if all(m["structural"] for m in members):
            structural[k] = entry
        else:
            content[k] = entry
    return {
        "signal": name,
        "content_groups": len(content),
        "content_files": sum(e["count"] for e in content.values()),
        "structural_groups": len(structural),
        "groups": dict(sorted(content.items(), key=lambda kv: -kv[1]["count"])),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true", help="只打印摘要，不写文件")
    ap.add_argument("--gate", action="store_true",
                    help="P4 防回归门禁：与库内基线比对三信号，任一上升 exit 1（只读）")
    args = ap.parse_args()

    docs = iter_docs()
    basename_groups = group(docs, lambda d: d["name"])
    hash_groups = group(docs, lambda d: d["body_hash"])
    title_groups = group(docs, lambda d: d["title"].casefold())

    result = {
        "generated_at": "2026-11-18",
        "scope": "wiki/ (排除 31-归档) + skills/",
        "files_scanned": len(docs),
        "duplicate_basenames": summarize("duplicate_basenames", basename_groups),
        "identical_content": summarize("identical_content", hash_groups),
        "title_collisions": summarize("title_collisions", title_groups),
    }

    print(f"scanned: {len(docs)} md files")
    for k in ("duplicate_basenames", "identical_content", "title_collisions"):
        s = result[k]
        print(f"  {k}: {s['content_groups']} content groups / {s['content_files']} files "
              f"(+{s['structural_groups']} structural groups)")

    if not (args.check or args.gate):
        OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=1), encoding="utf-8")
        print(f"wrote: {OUTPUT.relative_to(ROOT)}")

    if args.gate:
        baseline = json.loads(OUTPUT.read_text(encoding="utf-8"))
        regressions, tightened = [], []
        for k in ("duplicate_basenames", "identical_content", "title_collisions"):
            now = result[k]["content_groups"]
            base = baseline[k]["content_groups"]
            if now > base:
                regressions.append(f"{k}: {base} -> {now}")
            elif now < base:
                tightened.append(f"{k}: {base} -> {now}")
        if regressions:
            print("重复信号回归（超过库内基线，需先人工评审/落桩）: " + "; ".join(regressions))
            print("::error::reorg gate FAILED")
            sys.exit(1)
        if tightened:
            print("基线已收紧，无参运行可刷新 reorg-inventory.json: " + "; ".join(tightened))
        print(f"reorg gate OK（基线 generated_at={baseline.get('generated_at')}）")


if __name__ == "__main__":
    main()
