#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""校验全项目 markdown 链接（wiki + md）是否可解析到磁盘文件。

输出未解析（broken）链接清单，按源文件聚合。仅报告，不修改。
"""
import os
import re
import sys
import urllib.parse
from collections import defaultdict

ROOT = os.getcwd()
ONLY = sys.argv[1] if len(sys.argv) > 1 else ROOT  # 可选：只查某子树

WIKI = re.compile(r"\[\[([^\]\|#]+?)(#[^\]\|]+)?(\|[^\]]*)?\]\]")
MD = re.compile(r"\]\(([^)]+?)\)")

def norm(p):
    return os.path.normpath(p)

broken = defaultdict(list)
checked = 0

for dp, _d, fs in os.walk(ONLY):
    if "/.git" in dp.replace(os.sep, "/"):
        continue
    for fn in fs:
        if not fn.endswith(".md"):
            continue
        path = os.path.join(dp, fn)
        text = open(path, encoding="utf-8").read()

        def test(target, kind):
            global checked
            t = target.strip()
            if not t:
                return
            # 跳过外链/锚点/邮件/图片协议
            if t.startswith(("http://", "https://", "mailto:", "#", "<", "tel:")):
                return
            t = t.split("#", 1)[0].split(" ", 1)[0]
            if not t or not t.endswith(".md"):
                return
            t = urllib.parse.unquote(t)
            checked += 1
            if kind == "wiki" or t.startswith("技能/") or "/" in t and not t.startswith((".", "/")):
                # 相对 vault 根 或 绝对(技能/...) —— 以仓库根解析
                cand_root = os.path.join(ROOT, t)
            else:
                cand_root = None
            cand_rel = os.path.join(dp, t)
            if os.path.exists(cand_rel):
                return
            if cand_root and os.path.exists(cand_root):
                return
            broken[path].append((kind, target.strip()))

        for m in WIKI.finditer(text):
            test(m.group(1), "wiki")
        for m in MD.finditer(text):
            test(m.group(1), "md")

total = sum(len(v) for v in broken.values())
print(f"checked_links={checked} broken={total} files_with_broken={len(broken)}")
for p in sorted(broken):
    print(f"\n{os.path.relpath(p, ROOT)}  ({len(broken[p])})")
    for kind, t in broken[p][:20]:
        print(f"    [{kind}] {t}")
