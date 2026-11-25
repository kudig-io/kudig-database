#!/usr/bin/env python3
"""P1-F2：按"正文全同 → 按体裁定主场"批量清理 identical_content 副本。

策略（低风险落桩）：
- 每组保留唯一主场（canonical），其余成员正文替换为指向主场的 wikilink 桩
- 不改动任何入链（桩自身可解析），skills 组件层级与 README 章程保持有效
- QA 语料中 source/answer 引用被落桩文件的条目，改写为主场路径
- 桩带 STUB_MARKER，reorg-inventory.py 会跳过，三信号不复报

主场比赛规则（按优先级）：
  R-11    wiki/11-发布变更 优先（部署三件套等，plan S-6 主场）
  R-20    wiki/20 内非 01-best-practices 优先（plan F1 镜像收敛）
  R-CHEAT 非 17/05-速查卡 优先（速查卡内为分类错放副本）
  R-MAIN  主题域优先于提炼层（22-概念/23-实体/24-25/26-标签/17-06-知识字典；
          含 plan S-5：18-云厂商 优先于 23-实体/13）
  R-FUNC  10-平台工程/06-代码分析 内非 functions-* 优先（functions-* 为镜像目录）
  R-TRAIN skills 培训内嵌套目录优先于平铺别名（嵌套带 index 枢纽）
  R-WIKI  wiki 优先于 skills（体裁=知识条目，F2）
  R-ALT   skills 内非 -alt 优先（relocation 合并残件）
  R-SHORT 同分区内路径最短优先（兜底）

Usage:
  python3 code/scripts/maintenance/apply-identical-dedup.py            # dry-run 决策全表
  python3 code/scripts/maintenance/apply-identical-dedup.py --apply    # 执行落桩
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
INVENTORY = ROOT / "maintaining" / "02-元数据" / "metadata" / "reorg-inventory.json"
QA_DIR = ROOT / "wiki" / "19-故障诊断" / "10-QA语料"

STUB_MARKER = "内容已并入主场"
STUB_TEMPLATE = "\n> **{marker}**：[[{canonical}|{title}]]（本页为重排去重桩，正文见主场）\n"


def load_groups() -> dict:
    data = json.loads(INVENTORY.read_text(encoding="utf-8"))
    return data["identical_content"]["groups"]


REFINED_PREFIXES = (
    "wiki/20-概念/", "wiki/22-实体/", "wiki/23-综合与研究/", "wiki/23-综合与研究/研究/",
    "wiki/24-标签/", "wiki/21-知识字典/",
)


def pick_canonical(paths: list[str]) -> tuple[str, str]:
    """返回 (canonical, rule_id)。"""
    wiki = [p for p in paths if p.startswith("wiki/")]
    pool = wiki or [p for p in paths if p.startswith("skills/")] or paths

    if wiki:
        ordered = (
            ("R-11", lambda p: p.startswith("wiki/09-发布变更/")),
            ("R-20", lambda p: p.startswith("wiki/18-最佳实践/") and "/01-best-practices/" not in p),
            ("R-CHEAT", lambda p: "/05-速查卡/" not in p),
            ("R-MAIN", lambda p: not p.startswith(REFINED_PREFIXES)),
            ("R-FUNC", lambda p: "/functions-" not in p),
        )
        for rule_id, pred in ordered:
            hits = [p for p in pool if pred(p)]
            if hits and len(hits) < len(pool):
                return sorted(hits, key=lambda p: (len(p), p))[0], rule_id
        rule = "R-WIKI" if wiki != paths else "R-SHORT"
        return sorted(pool, key=lambda p: (len(p), p))[0], rule

    # skills-only：培训类优先嵌套（层级深），其余最短路径
    if all("/培训/" in p for p in pool):
        deepest = max(len(Path(p).parts) for p in pool)
        deep = [p for p in pool if len(Path(p).parts) == deepest]
        if len(deep) < len(pool):
            return sorted(deep, key=lambda p: (len(p), p))[0], "R-TRAIN"

    non_alt = [p for p in pool if "-alt" not in Path(p).name]
    if non_alt and len(non_alt) < len(pool):
        return sorted(non_alt, key=lambda p: (len(p), p))[0], "R-ALT"
    return sorted(pool, key=lambda p: (len(p), p))[0], "R-SHORT"


def frontmatter_title(text: str) -> str:
    m = re.search(r"^title:\s*(.+)$", text, re.M)
    if m:
        return m.group(1).strip().strip("'\"")
    m = re.search(r"^#\s+(.+)$", text, re.M)
    return m.group(1).strip() if m else "原文"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    groups = load_groups()
    decisions, rule_counts = [], {}
    stub_writes, qa_fixes = [], 0

    for body_hash, entry in sorted(groups.items(), key=lambda kv: -kv[1]["count"]):
        paths = entry["paths"]
        canonical, rule = pick_canonical(paths)
        stubs = [p for p in paths if p != canonical]
        decisions.append({"hash": body_hash, "keep": canonical, "stub": stubs, "rule": rule})
        rule_counts[rule] = rule_counts.get(rule, 0) + 1

        if not args.apply:
            continue
        # 落桩
        title = frontmatter_title((ROOT / canonical).read_text(encoding="utf-8"))
        for sp in stubs:
            fp = ROOT / sp
            text = fp.read_text(encoding="utf-8")
            fm = re.match(r"^---\n.*?\n---\n?", text, re.S)
            head = fm.group(0) if fm else ""
            fp.write_text(head + STUB_TEMPLATE.format(marker=STUB_MARKER, canonical=canonical, title=title),
                          encoding="utf-8")
            stub_writes.append(sp)
        # QA 引用改写：被落桩路径 → 主场
        stub_set = set(stubs)
        for yml in QA_DIR.glob("*.yaml"):
            t = yml.read_text(encoding="utf-8")
            orig = t
            for sp in stubs:
                t = t.replace(sp, canonical)
            if t != orig:
                yml.write_text(t, encoding="utf-8")
                qa_fixes += 1

    total_stub = sum(len(d["stub"]) for d in decisions)
    print(f"groups: {len(decisions)}  keep: {len(decisions)}  stub: {total_stub}")
    print("rules:", dict(sorted(rule_counts.items())))

    if args.apply:
        print(f"stubbed files: {len(stub_writes)}  qa files rewritten: {qa_fixes}")
        # 抽样打印
        for d in decisions[:3]:
            print(f"  keep={d['keep']} <- stub={d['stub'][:2]}")
    else:
        print("\n=== decision table ===")
        for d in decisions:
            print(f"[{d['rule']:8}] keep {d['keep']}  <-  {', '.join(d['stub'])}")


if __name__ == "__main__":
    main()
