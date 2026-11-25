#!/usr/bin/env python3
"""P1-④：title_collisions 人工评审队列生成器。

对 reorg-inventory.json 的 title_collisions 内容组分类：
- LEGIT：成员分属不同提炼层（字典/概念/实体/标签，且无主题域与 skills 成员）
          ——同题不同体裁，章程允许，列表供一次性签认
- REAL ：其余（同层重复、域↔skills 漂移、域内同题……）——真同题异版，
         逐对给出相似度/行数/独有章节/差异摘要/主场建议（复用 P1-① 规则）
- SPECIAL：terway 三向专项（05/01 系列 ↔ 05/06 系列 ↔ 23/04 系列，跨命名不入自动配对）

输出：markdown 评审单（默认 maintaining/01-文档/assessments/title-collision-review-queue.md）

Usage:
  python3 code/scripts/maintenance/review-title-collisions.py            # 写评审单
  python3 code/scripts/maintenance/review-title-collisions.py --stdout   # 打印
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import sys
from datetime import date
from pathlib import Path

# 主场规则与 P1-① 同源：动态加载带连字符的模块文件
_spec = importlib.util.spec_from_file_location(
    "apply_identical_dedup",
    Path(__file__).resolve().parent / "apply-identical-dedup.py",
)
_mod = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_mod)
pick_canonical = _mod.pick_canonical

ROOT = Path(__file__).resolve().parents[3]
INVENTORY = ROOT / "maintaining" / "02-元数据" / "metadata" / "reorg-inventory.json"
OUTPUT = ROOT / "maintaining" / "01-文档" / "assessments" / "title-collision-review-queue.md"

LAYERS = [
    ("17-系统基础/06-知识字典", "字典"),
    ("22-概念", "概念"),
    ("23-实体", "实体"),
    ("26-标签", "标签"),
    ("05-速查卡", "速查卡"),
]

# terway 三向专项：05/01 系列 ↔ 05/06 系列 ↔ 23/04 系列（按主题人工配对）
TERWAY_TOPICS = [
    ("产品概览", "wiki/05-网络/01-K8s网络核心/42-terway-product-overview.md",
     "wiki/05-网络/06-Terway/01-product.md", "wiki/22-实体/04-网络/03-terway-product-overview.md"),
    ("架构深潜", "wiki/05-网络/01-K8s网络核心/43-terway-architecture-deep-dive.md",
     "wiki/05-网络/06-Terway/02-architecture.md", "wiki/22-实体/04-网络/04-terway-architecture-deep-dive.md"),
    ("使用指南", "wiki/05-网络/01-K8s网络核心/44-terway-usage-guide.md",
     "wiki/05-网络/06-Terway/03-usage.md", "wiki/22-实体/04-网络/05-terway-usage-guide.md"),
    ("CRD 操作", "wiki/05-网络/01-K8s网络核心/45-terway-crd-operations.md",
     "wiki/05-网络/06-Terway/03b-crd-operations.md", "wiki/22-实体/04-网络/06-terway-crd-operations.md"),
    ("运维手册", "wiki/05-网络/01-K8s网络核心/46-terway-operations-manual.md",
     "wiki/05-网络/06-Terway/04-operations.md", "wiki/22-实体/04-网络/07-terway-operations-manual.md"),
    ("测试验证", "wiki/05-网络/01-K8s网络核心/47-terway-testing-validation.md",
     "wiki/05-网络/06-Terway/05-testing.md", "wiki/22-实体/04-网络/08-terway-testing-validation.md"),
    ("性能调优", "wiki/05-网络/01-K8s网络核心/48-terway-performance-tuning.md",
     "wiki/05-网络/06-Terway/06-performance.md", "wiki/22-实体/04-网络/09-terway-performance-tuning.md"),
    ("故障排查", "wiki/05-网络/01-K8s网络核心/49-terway-troubleshooting-fta.md",
     "wiki/05-网络/06-Terway/07-troubleshooting-fta.md", "wiki/22-实体/04-网络/10-terway-troubleshooting-fta.md"),
]

STRUCTURAL_NAMES = {"readme.md", "index.md", "moc.md", "summary.md"}


def layer_of(path: str) -> str | None:
    for key, name in LAYERS:
        if key in path:
            return name
    return None


def genre_of(path: str) -> str:
    """structural | layer:<层名> | article（主题域/skills 的文章体裁）。"""
    lay = layer_of(path)
    if lay == "标签":
        return "structural"
    if lay:
        return f"layer:{lay}"
    name = Path(path).name.lower()
    if name in STRUCTURAL_NAMES or name.endswith("-index.md") or "/reference/" in path:
        return "structural"
    return "article"


def read_body(rel: str) -> str:
    p = ROOT / rel
    if not p.exists():
        return ""
    text = p.read_text(encoding="utf-8", errors="replace")
    m = re.match(r"^---\n.*?\n---\n?(.*)$", text, re.S)
    return m.group(1) if m else text


def sections(body: str) -> set[str]:
    return {m.group(2).strip() for m in re.finditer(r"^(#{2,3})\s+(.+)$", body, re.M)}


def front_field(rel: str, key: str) -> str:
    p = ROOT / rel
    if not p.exists():
        return "?"
    m = re.search(rf"^{key}:\s*(.+)$", p.read_text(encoding="utf-8", errors="replace"), re.M)
    return m.group(1).strip().strip("'\"") if m else "-"


def line_count(rel: str) -> int:
    return read_body(rel).count("\n") + 1


def similarity(a: str, b: str) -> float:
    """行级 Jaccard（O(n)，评审摘要够用；difflab 对多千行文件过慢）。"""
    la = {l.strip() for l in a.splitlines() if l.strip()}
    lb = {l.strip() for l in b.splitlines() if l.strip()}
    if not la or not lb:
        return 0.0
    return len(la & lb) / len(la | lb)


def diff_summary(canon_body: str, other_body: str) -> str:
    only_c = sorted(sections(canon_body) - sections(other_body))
    only_o = sorted(sections(other_body) - sections(canon_body))
    parts = []
    if only_o:
        parts.append(f"副本独有 {len(only_o)} 节: {', '.join(only_o[:3])}{'…' if len(only_o) > 3 else ''}")
    if only_c:
        parts.append(f"主场独有 {len(only_c)} 节: {', '.join(only_c[:3])}{'…' if len(only_c) > 3 else ''}")
    if not parts:
        parts.append("章节结构一致，差异在正文措辞/数据")
    return "；".join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stdout", action="store_true")
    args = ap.parse_args()

    data = json.loads(INVENTORY.read_text(encoding="utf-8"))
    groups = data["title_collisions"]["groups"]

    legit, real = [], []
    for title, entry in groups.items():
        genres = [genre_of(p) for p in entry["paths"]]
        article_cnt = sum(1 for g in genres if g == "article")
        # 真同题异版 = ≥2 篇同体裁文章；其余（索引/标签/跨层各一）归签认
        (real if article_cnt >= 2 else legit).append((title, entry))

    pick = pick_canonical

    out = []
    out.append("---")
    out.append("title: 同题异版人工评审单（P1-④）")
    out.append("category: references")
    out.append("tags: [reorg, dedup, review, corpus]")
    out.append("tier: supporting")
    out.append(f"created: '{date.today().isoformat()}'")
    out.append(f"last_updated: '{date.today().isoformat()}'")
    out.append("---")
    out.append("")
    out.append("# 同题异版人工评审单（P1-④）")
    out.append("")
    out.append("> 由 `review-title-collisions.py` 生成，数据源 `reorg-inventory.json`。")
    out.append(f"> 分类：索引/标签/跨层撞题签认 **{len(legit)}** 组（签认即关闭）；真同题异版 **{len(real)}** 组（逐组裁决）；terway 三向专项 **{len(TERWAY_TOPICS)}** 项。")
    out.append("> 裁决填法：在「你的裁决」列填 `按建议` / `保留副本来路径` / `保留全部（同场景不同实例）` / `需看全文 diff`。")
    out.append("> 「提示」列：相似度 <30% 时建议先确认是否为同场景不同实例（如不同工单号），低相似合并需谨慎。")
    out.append("> 相似度口径：行级 Jaccard（保守，跨结构重排的两版会显著偏低）；「差异摘要」以双方独有章节为准更可靠。")
    out.append("")

    out.append("## A. terway 三向专项（F3，plan S-1）")
    out.append("")
    out.append("> plan 主场 = `05-网络/06-Terway/`；本表用于确认各主题的择优方向与 23-实体侧压卡。")
    out.append("")
    out.append("| 主题 | 05/01 系列 (行) | 05/06 主场 (行) | 23/04 (行) | 05/01 vs 主场 相似度 | 差异摘要 | 你的裁决 |")
    out.append("|---|---:|---:|---:|---:|---|---|")
    for name, a, b, c in TERWAY_TOPICS:
        ba, bb = read_body(a), read_body(b)
        sim = similarity(ba, bb) if ba and bb else 0
        out.append(f"| {name} | {line_count(a)} | {line_count(b)} | {line_count(c)} | {sim:.0%} | {diff_summary(bb, ba)} | |")
    out.append("")

    out.append("## B. 真同题异版（逐组裁决）")
    out.append("")
    out.append("| # | 标题 | 成员（主场**加粗**） | 主场建议(规则) | 相似度 vs 主场 | 行数(主/副) | 差异摘要 | 提示 | 你的裁决 |")
    out.append("|---:|---|---|---|---:|---|---|---|---|")
    for i, (title, entry) in enumerate(sorted(real, key=lambda kv: -kv[1]["count"]), 1):
        paths = entry["paths"]
        canon, rule = pick(paths)
        cb = read_body(canon)
        members = "<br>".join(f"**{p}**" if p == canon else p for p in paths)
        sims, lines, updates = [], [], []
        for p in paths:
            if p == canon:
                continue
            sims.append(f"{similarity(cb, read_body(p)):.0%}")
            lines.append(str(line_count(p)))
            updates.append(front_field(p, "last_updated"))
        summary = diff_summary(cb, read_body(paths[1] if paths[0] == canon else paths[0]))
        nums = [float(s.rstrip("%")) for s in sims]
        hint = "⚠️ 低相似(<30%)：先确认是否同场景不同实例" if nums and max(nums) < 30 else ""
        out.append(
            f"| {i} | {title[:40]} | {members} | {rule} | {', '.join(sims) or '-'} | "
            f"{line_count(canon)}/{'/'.join(lines)} | {summary} | {hint} | |"
        )
    out.append("")

    out.append("## C. 索引/标签/跨层撞题（列表签认即关闭）")
    out.append("")
    out.append("> 成员少于 2 篇同体裁文章：索引/README/标签页撞题，或字典/概念/实体/速查卡各一的跨层撞题——章程允许共存。")
    out.append("")
    out.append("| # | 标题 | 成员（行数） | 你的签认 |")
    out.append("|---:|---|---|---|")
    for i, (title, entry) in enumerate(sorted(legit, key=lambda kv: kv[0]), 1):
        members = "<br>".join(f"{p}（{line_count(p)} 行）" for p in entry["paths"])
        out.append(f"| {i} | {title[:40]} | {members} | |")
    out.append("")

    text = "\n".join(out) + "\n"
    if args.stdout:
        print(text)
    else:
        OUTPUT.write_text(text, encoding="utf-8")
        print(f"wrote {OUTPUT.relative_to(ROOT)}  (legit={len(legit)} real={len(real)} terway={len(TERWAY_TOPICS)})")


if __name__ == "__main__":
    main()
