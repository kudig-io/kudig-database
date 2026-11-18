#!/usr/bin/env python3
"""P1-④ B/C 裁决执行器。

裁决（2026-11-18）：
- B1+B2/B4 按建议：真同题异版中「跨 wiki/skills」或「20域01镜像」组
  按主场规则保留唯一主场，其余成员落桩
- B3a 工单同场景：保留全部，标题加「（工单 NNN）」消歧
- B3b 培训变体：保留全部，讲师版/平铺版加限定词，主版保留原题
- B3① 语料生成物对：主场 = .with_actions（内容超集+动作标注），plain 落桩
- B3② 真同题异版（约 10 组）：本轮不执行，出细摘要待裁决
- C 签认：写入评审单（不改组内文件）

Usage: python3 code/scripts/maintenance/execute-title-review.py [--dry-run]
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def _load(name: str, fname: str):
    spec = importlib.util.spec_from_file_location(name, HERE / fname)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


dedup = _load("apply_identical_dedup", "apply-identical-dedup.py")
review = _load("review_title_collisions", "review-title-collisions.py")

QA_DIR = ROOT / "wiki" / "19-故障诊断" / "10-QA语料"
QA_CORPUS_PREFIX = "wiki/19-故障诊断/10-QA语料/"


def batch_of(paths: list[str]) -> str:
    """与评审单分批一致。"""
    if any(p.startswith("skills/") for p in paths) and any(p.startswith("wiki/") for p in paths):
        return "B1"
    if any("/01-best-practices/" in p or p.startswith("wiki/20-最佳实践/") for p in paths):
        return "B24"
    if any("工单案例" in p for p in paths):
        return "B3a"
    if all(p.startswith(QA_CORPUS_PREFIX) for p in paths):
        return "B3c1"
    if any("培训" in p for p in paths) or any(Path(p).name.startswith(("day-", "inner-", "public-")) for p in paths):
        return "B3b"
    return "B3c2"


def real_groups(data: dict) -> list[tuple[str, dict]]:
    out = []
    for title, entry in data["title_collisions"]["groups"].items():
        if sum(1 for p in entry["paths"] if review.genre_of(p) == "article") >= 2:
            out.append((title, entry))
    return out


def write_stub(path: Path, canonical_rel: str):
    text = path.read_text(encoding="utf-8")
    fm = re.match(r"^---\n.*?\n---\n?", text, re.S)
    head = fm.group(0) if fm else ""
    title = dedup.frontmatter_title((ROOT / canonical_rel).read_text(encoding="utf-8"))
    path.write_text(head + dedup.STUB_TEMPLATE.format(
        marker=dedup.STUB_MARKER, canonical=canonical_rel, title=title), encoding="utf-8")


def retitle(path: Path, suffix_fn, apply: bool = True):
    text = path.read_text(encoding="utf-8")
    m = re.search(r"^title:\s*(.+)$", text, re.M)
    if not m:
        return None
    old = m.group(1).strip().strip("'\"")
    new = suffix_fn(old, str(path.relative_to(ROOT)))
    if new is None or new == old:
        return None
    if apply:
        quoted = f"'{new}'" if ":" in new or "{" in new else new
        path.write_text(text[:m.start(1)] + quoted + text[m.end(1):], encoding="utf-8")
    return f"{old} -> {new}"


def ticket_suffix(old: str, rel: str) -> str:
    m = re.search(r"ticket-case-(\d+)", rel)
    return f"{old}（工单 {m.group(1)}）" if m else None


def train_suffix(old: str, rel: str) -> str | None:
    name = Path(rel).name
    if "/lecturer/" in rel:
        return f"{old}（讲师版）"
    if name.startswith("lecturer-"):
        return f"{old}（讲师版·平铺）"
    if name.startswith("inner-") and "/inner-training/" not in rel:
        return f"{old}（内训版·平铺）"
    if name.startswith("public-"):
        return f"{old}（公开班版·平铺）"
    return None  # 主版保留原题


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    data = json.loads((ROOT / "maintaining/02-元数据/metadata/reorg-inventory.json").read_text(encoding="utf-8"))
    groups = real_groups(data)

    stats = {"B1B24_stub": 0, "corpus_stub": 0, "ticket_retitle": 0, "train_retitle": 0}
    stubbed: list[tuple[str, str]] = []  # (stub_path, canonical)
    retitled: list[str] = []
    deferred = []

    for title, entry in groups:
        paths = entry["paths"]
        batch = batch_of(paths)

        if batch in ("B1", "B24"):
            canon, rule = dedup.pick_canonical(paths)
            for p in paths:
                if p != canon:
                    stubbed.append((p, canon))
                    stats["B1B24_stub"] += 1
            if not args.dry_run:
                for p in paths:
                    if p != canon:
                        write_stub(ROOT / p, canon)

        elif batch == "B3c1":
            with_actions = [p for p in paths if ".with_actions" in p]
            canon = with_actions[0] if with_actions else paths[0]
            for p in paths:
                if p != canon:
                    stubbed.append((p, canon))
                    stats["corpus_stub"] += 1
            if not args.dry_run:
                for p in paths:
                    if p != canon:
                        write_stub(ROOT / p, canon)

        elif batch == "B3a":
            for p in paths:
                r = retitle(ROOT / p, ticket_suffix, apply=not args.dry_run)
                if r:
                    retitled.append(f"{p}: {r}")
                    stats["ticket_retitle"] += 1

        elif batch == "B3b":
            for p in paths:
                r = retitle(ROOT / p, train_suffix, apply=not args.dry_run)
                if r:
                    retitled.append(f"{p}: {r}")
                    stats["train_retitle"] += 1

        else:  # B3c2 真同题异版 → 待裁决
            deferred.append((title, paths, dedup.pick_canonical(paths)))

    # QA source 改写（被落桩路径 → 主场）
    qa_fixed = 0
    if not args.dry_run and stubbed:
        for yml in QA_DIR.glob("*.yaml"):
            t = orig = yml.read_text(encoding="utf-8")
            for sp, canon in stubbed:
                t = t.replace(sp, canon)
            if t != orig:
                yml.write_text(t, encoding="utf-8")
                qa_fixed += 1

    print(f"{'DRY-RUN ' if args.dry_run else ''}stats: {stats}  qa_files_rewritten: {qa_fixed}")
    print(f"deferred (B3② 待裁决): {len(deferred)} 组")
    for t, ps, (canon, rule) in deferred:
        print(f"   [{rule}] {t[:44]}")
        for p in ps:
            print(f"       {'**' if p == canon else '  '}{p}")
    if args.dry_run:
        stub_total = stats['B1B24_stub'] + stats['corpus_stub']
        retitle_total = stats['ticket_retitle'] + stats['train_retitle']
        print(f"\n将落桩 {stub_total} 份，改名 {retitle_total} 处")
        for sp, c in stubbed[:10]:
            print(f"   桩 {sp} -> {c}")
        print("   改名预览:")
        for r in retitled:
            print(f"     {r}")


if __name__ == "__main__":
    main()
