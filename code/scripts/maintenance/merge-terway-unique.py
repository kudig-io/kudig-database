#!/usr/bin/env python3
"""terway A 裁决执行：05/01 旧版系列独有节回填 05/06 主场，随后旧版落桩。

裁决（2026-11-18）：全部按主场 = 05-网络/06-Terway/；05/01 独有节回填。
清单经逐主题同主题覆盖核对（只搬主场确实没有的主题节；同题不同行文的
“技术细节”容器节不整块搬运，避免复述式重复）。

回填规则：
- 主场以「交叉引用」H2 为插入锚（无则 Related / 文末）；新节续接其编号，
  锚节编号顺延；源 H3 提升为 H2，节内更深层级原样保留
- 幂等：主场已含同名（归一化）标题则跳过
- 执行后 42-49 旧版文件落桩指向主场，QA 语料 source 同步改写

Usage: python3 code/scripts/maintenance/merge-terway-unique.py [--dry-run]
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
ROOT = Path(__file__).resolve().parents[3]

STUB_MARKER = "内容已并入主场"
STUB_TEMPLATE = "\n> **{marker}**：[[{canonical}|{title}]]（本页为重排去重桩，正文见主场）\n"
QA_DIR = ROOT / "wiki" / "19-故障诊断" / "10-QA语料"

S = "wiki/05-网络/01-K8s网络核心/"
C = "wiki/05-网络/06-Terway/"

# (旧版, 主场, [(层级, 节题), ...])
MANIFEST = [
    (S + "42-terway-product-overview.md", C + "01-product.md",
     [("h2", "故障排查表"), ("h2", "监控告警"), ("h2", "生产最佳实践"), ("h2", "容量规划计算")]),
    (S + "43-terway-architecture-deep-dive.md", C + "02-architecture.md",
     [("h3", "架构验证命令")]),
    (S + "44-terway-usage-guide.md", C + "03-usage.md",
     [("h3", "多网卡配置 (Multus)"), ("h3", "Service 集成")]),
    (S + "45-terway-crd-operations.md", C + "03b-crd-operations.md",
     [("h3", "NetworkAttachmentDefinition (Multus)"), ("h3", "CRD 故障排查")]),
    (S + "46-terway-operations-manual.md", C + "04-operations.md",
     [("h3", "日志管理"), ("h3", "备份与恢复")]),
    (S + "47-terway-testing-validation.md", C + "05-testing.md",
     [("h3", "验证检查清单")]),
    (S + "48-terway-performance-tuning.md", C + "06-performance.md",
     [("h3", "调优检查清单")]),
    (S + "49-terway-troubleshooting-fta.md", C + "07-troubleshooting-fta.md",
     [("h2", "日志分析"), ("h2", "网络连通性测试"), ("h2", "紧急恢复"), ("h2", "快速诊断脚本")]),
]


def norm(t: str) -> str:
    t = re.sub(r"[\U0001f300-\U0001f9ff]", "", t)
    t = re.sub(r"^[\d一二三四五六七八九十]+[.、．]\s*", "", t.strip())
    return re.sub(r"\s+", "", t).lower()


def front_title(text: str) -> str:
    m = re.search(r"^title:\s*(.+)$", text, re.M)
    if m:
        return m.group(1).strip().strip("'\"")
    m = re.search(r"^#\s+(.+)$", text, re.M)
    return m.group(1).strip() if m else "原文"


def extract_block(src_text: str, level: str, title: str) -> str | None:
    hashes = "##" if level == "h2" else "###"
    target = norm(title)
    for m in re.finditer(rf"^{hashes}\s+(.+)$", src_text, re.M):
        if norm(m.group(1)) != target:
            continue
        start = m.start()
        # 块 = 本标题行起，至下一同级或更高级标题
        deeper = "####" if level == "h2" else hashes
        nxt = re.search(rf"^({hashes}|##)\s+", src_text[m.end():], re.M)
        if level == "h2":
            nxt = re.search(r"^##\s+", src_text[m.end():], re.M)
        else:
            nxt = re.search(r"^(###|##)\s+", src_text[m.end():], re.M)
        end = m.end() + nxt.start() if nxt else len(src_text)
        return src_text[start:end].rstrip() + "\n"
    return None


def canonical_title(heading_line: str) -> str:
    return re.sub(r"^#+\s*", "", heading_line).strip()


def insert_into(canon_text: str, blocks: list[tuple[str, str]]) -> tuple[str, list[str]]:
    lines = canon_text.splitlines(keepends=True)
    # 锚：最后一个「交叉引用」H2，其次 Related，否则文末
    anchor_idx = None
    anchor_num = None
    for i in range(len(lines) - 1, -1, -1):
        m = re.match(r"^##\s+(\d+)\.\s*交叉引用\s*$", lines[i])
        if m:
            anchor_idx, anchor_num = i, int(m.group(1))
            break
        if re.match(r"^##\s+交叉引用\s*$", lines[i]) or re.match(r"^##\s+Related\s*$", lines[i]):
            if anchor_idx is None:
                anchor_idx = i  # 备选，继续找交叉引用
    if anchor_idx is None:
        anchor_idx = len(lines)

    existing = {norm(re.sub(r"^#+\s*", "", l)) for l in lines if re.match(r"^#{2,3}\s+", l)}
    max_n = max([int(m.group(1)) for l in lines
                 for m in [re.match(r"^##\s+(\d+)\.", l)] if m] or [0])

    added, out_blocks = [], []
    if anchor_num is not None:
        next_num = anchor_num
    else:
        next_num = max_n + 1

    for _level, body in blocks:
        first = body.splitlines()[0]
        title = canonical_title(first)
        if norm(title) in existing:
            continue
        rest = body.splitlines(keepends=True)[1:]
        out_blocks.append(f"## {next_num}. {title}\n" + "".join(rest).rstrip() + "\n\n")
        added.append(title)
        existing.add(norm(title))
        next_num += 1

    if not added:
        return canon_text, []

    if anchor_num is not None:
        # 锚编号顺延
        lines[anchor_idx] = re.sub(r"^##\s+\d+\.", f"## {next_num}.", lines[anchor_idx])

    new_lines = []
    for i, line in enumerate(lines):
        if i == anchor_idx:
            for blk in out_blocks:
                new_lines.extend(blk.splitlines(keepends=True))
        new_lines.append(line)
    if anchor_idx >= len(lines):
        for blk in out_blocks:
            new_lines.extend(blk.splitlines(keepends=True))
    return "".join(new_lines), added


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    stub_targets: list[tuple[str, str]] = []
    for src_rel, canon_rel, items in MANIFEST:
        src_p, canon_p = ROOT / src_rel, ROOT / canon_rel
        src_txt = src_p.read_text(encoding="utf-8")
        canon_txt = canon_p.read_text(encoding="utf-8")

        blocks = []
        for level, title in items:
            blk = extract_block(src_txt, level, title)
            if blk is None:
                print(f"  ! 未找到 {src_rel} 中的 {level} 节「{title}」——跳过该项")
                continue
            blocks.append((level, blk))

        if args.dry_run:
            exists = {norm(re.sub(r'^#+\s*', '', l)) for l in canon_txt.splitlines() if re.match(r'^#{2,3}\s+', l)}
            todo = [t for _, t in blocks if norm(t) not in exists]
            print(f"DRY {canon_rel}: 拟回填 {len(todo)} 节 {todo}")
            continue

        new_txt, added = insert_into(canon_txt, blocks)
        if added:
            canon_p.write_text(new_txt, encoding="utf-8")
        stub_targets.append((src_rel, canon_rel))
        print(f"  {canon_rel}: 回填 {len(added)} 节 {added}")

    if args.dry_run:
        return

    # 旧版落桩 + QA source 改写
    for src_rel, canon_rel in stub_targets:
        src_p = ROOT / src_rel
        if STUB_MARKER in src_p.read_text(encoding="utf-8"):
            print(f"  ~ 已是桩: {src_rel}")
            continue
        canon_txt = (ROOT / canon_rel).read_text(encoding="utf-8")
        text = src_p.read_text(encoding="utf-8")
        fm = re.match(r"^---\n.*?\n---\n?", text, re.S)
        head = fm.group(0) if fm else ""
        src_p.write_text(
            head + STUB_TEMPLATE.format(marker=STUB_MARKER, canonical=canon_rel,
                                         title=front_title(canon_txt)),
            encoding="utf-8")
        print(f"  桩: {src_rel} -> {canon_rel}")
        for yml in QA_DIR.glob("*.yaml"):
            t = yml.read_text(encoding="utf-8")
            if src_rel in t:
                yml.write_text(t.replace(src_rel, canon_rel), encoding="utf-8")
                print(f"     QA 改写: {yml.name}")


if __name__ == "__main__":
    main()
