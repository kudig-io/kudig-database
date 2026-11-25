#!/usr/bin/env python3
"""S-2：各域 production-readiness 运维指南的骨架校验与编号规范化。

背景：20 份按域指南正文深度域特异（不可全文生成），漂移发生在共享骨架层。
本工具只做结构治理，绝不改写正文：
  --check  校验（CI 可用，结构问题退出码 1）：
           frontmatter / H1 / 六大核心章节齐备且不重复 / 编号风格
  --apply  规范化编号：中文数字（一、二、…）→ 阿拉伯（N. ；
           无编号的核心章节按其前序已编号 H2 续编；其余正文原样

模板参照：code/scripts/templates/production-readiness-template.md
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GUIDE_GLOBS = ["*/00-总览/*production-readiness-operations-guide.md",
               "*/08-生产就绪/*production-readiness-operations-guide.md",
               "*/08-数据库中间件总览/*production-readiness-operations-guide.md"]

CORE_SECTIONS = [
    "生产环境检查清单",
    "关键风险与缓解措施",
    "日常运维操作",
    "故障排查速查",
    "与其他域的协作边界",
    "推荐阅读",
]

CN_DIGITS = {"一": 1, "二": 2, "三": 3, "四": 4, "五": 5,
             "六": 6, "七": 7, "八": 8, "九": 9, "十": 10}

H2_RE = re.compile(r"^##\s+(.+)$", re.M)
PREFIX = r"(?:\d+[.、]\s*|[一二三四五六七八九十]+、\s*)?"
CORE_RE = re.compile(r"^" + PREFIX + r"({titles})\s*$".format(
    titles="|".join(CORE_SECTIONS)))


def parse_cn(s: str) -> int | None:
    """中文数字（一…九十九）→ int。"""
    if not s or any(c not in CN_DIGITS for c in s):
        return None
    if "十" not in s:
        return CN_DIGITS[s] if len(s) == 1 else None
    left, _, right = s.partition("十")
    tens = CN_DIGITS.get(left, 1) if left else 1
    ones = CN_DIGITS.get(right, 0) if right else 0
    if right and ones is None:
        return None
    return tens * 10 + ones


def core_title(h2: str) -> str | None:
    m = CORE_RE.match(h2.strip())
    return m.group(1) if m else None


def check_file(path: Path) -> tuple[list[str], list[str]]:
    text = path.read_text(encoding="utf-8")
    fails, warns = [], []

    if not text.lstrip().startswith("---"):
        fails.append("缺少 frontmatter")
    else:
        fm = re.match(r"^---\n(.*?)\n---", text, re.S)
        if fm and not re.search(r"^title:", fm.group(1), re.M):
            fails.append("frontmatter 缺 title")

    if not re.search(r"^#\s+.*生产就绪运维指南", text, re.M):
        fails.append("H1 非『…生产就绪运维指南』")

    h2s = [m.group(1).strip() for m in H2_RE.finditer(text)]
    found, dupes = [], []
    for h in h2s:
        t = core_title(h)
        if t:
            if t in found:
                dupes.append(t)
            found.append(t)
    missing = [s for s in CORE_SECTIONS if s not in found]
    if missing:
        fails.append(f"缺核心章节: {', '.join(missing)}")
    if dupes:
        fails.append(f"核心章节重复: {', '.join(dupes)}")

    for h in h2s:
        if re.match(r"^[一二三四五六七八九十]+、", h):
            warns.append(f"中文编号: ## {h}")
            break
    for h in h2s:
        if core_title(h) and not re.match(r"^\d+[.、]", h):
            warns.append(f"核心章节无编号: ## {h}")
            break

    return fails, warns


def apply_normalize(path: Path) -> int:
    text = path.read_text(encoding="utf-8")
    changes = 0

    # 1) 中文数字编号 → 阿拉伯
    def cn_to_num(m: re.Match) -> str:
        nonlocal changes
        n = parse_cn(m.group(1))
        if n is None:
            return m.group(0)
        changes += 1
        return f"## {n}. {m.group(2)}"

    text = re.sub(r"^## ([一二三四五六七八九十]+)、\s*(.+)$", cn_to_num, text, flags=re.M)

    # 2) 无编号核心章节：按其前面已编号 H2 数量续编
    out, numbered = [], 0
    for line in text.splitlines(keepends=True):
        m = re.match(r"^##\s+(.+?)(\n?)$", line)
        if m:
            title = m.group(1).strip()
            if re.match(r"^\d+[.、]\s*", title):
                numbered += 1
            elif core_title(title):
                numbered += 1
                line = f"## {numbered}. {title}{m.group(2)}"
                changes += 1
        out.append(line)

    if changes:
        path.write_text("".join(out), encoding="utf-8")
    return changes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true", help="规范化编号")
    args = ap.parse_args()

    guides = sorted({g for pat in GUIDE_GLOBS for g in ROOT.glob(f"wiki/{pat}")})
    if not guides:
        print("no guides found"); sys.exit(2)

    fails_total = warns_total = fixed = 0
    for g in guides:
        rel = g.relative_to(ROOT)
        if args.apply:
            fixed += apply_normalize(g)
        fails, warns = check_file(g)
        status = "FAIL" if fails else ("WARN" if warns else "OK")
        fails_total += len(fails)
        warns_total += len(warns)
        if status != "OK":
            print(f"{status:4} {rel}")
            for f in fails:
                print(f"       - {f}")
            for w in warns:
                print(f"       ~ {w}")

    print(f"\nguides: {len(guides)}  fails: {fails_total}  warns: {warns_total}"
          + (f"  normalized: {fixed}" if args.apply else ""))
    sys.exit(1 if fails_total else 0)


if __name__ == "__main__":
    main()
