#!/usr/bin/env python3
"""2026-07-23 前缀改名：非 Markdown 语料/元数据中的 .md 路径重写。

仅匹配以旧根目录名开头、以 .md 结尾的完整路径（避免中文叙述里
“网络/存储”这类枚举误伤），映射根段与二级段。

作用范围：
- 19-故障诊断/10-QA语料/*.yaml   （source / answer 中的路径）
- 35-元数据/corpus-config/ 下的 *.json / *.yaml

用法: python3 31-脚本/maintenance/rewrite-prefix-corpus-20260723.py [--dry-run]
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

_ns = {}
with open(os.path.join(HERE, "rename-prefix-20260723.py"), encoding="utf-8") as f:
    exec(f.read().split("def run(")[0], _ns)
ROOT_MAP = _ns["ROOT_MAP"]
L2_MAP = {(p, o): n for p, pairs in _ns["L2_MAP"].items() for o, n in pairs}

ROOT_ALT = "|".join(re.escape(k) for k in ROOT_MAP)
PATH_RE = re.compile(
    r"(?<![0-9A-Za-z_\-/\u4e00-\u9fff])"
    r"((?:" + ROOT_ALT + r")/[0-9A-Za-z_\-/.\u4e00-\u9fff]+\.md)"
)


def map_old_path(p):
    segs = p.split("/")
    if segs[0] not in ROOT_MAP:
        return p
    old0 = segs[0]
    segs[0] = ROOT_MAP[old0]
    if len(segs) > 1 and (old0, segs[1]) in L2_MAP:
        segs[1] = L2_MAP[(old0, segs[1])]
    return "/".join(segs)


def main():
    dry = "--dry-run" in sys.argv
    targets = sorted(
        glob.glob(os.path.join(ROOT, "19-故障诊断/10-QA语料/*.yaml"))
        + glob.glob(os.path.join(ROOT, "35-元数据/corpus-config/**/*.json"), recursive=True)
        + glob.glob(os.path.join(ROOT, "35-元数据/corpus-config/**/*.yaml"), recursive=True)
    )
    total_files = total_hits = 0
    for path in targets:
        if ".vector-cache" in path:
            continue
        text = open(path, encoding="utf-8").read()
        hits = 0

        def sub(m):
            nonlocal hits
            new = map_old_path(m.group(1))
            if new != m.group(1):
                hits += 1
            return new

        out = PATH_RE.sub(sub, text)
        if hits:
            total_files += 1
            total_hits += hits
            print(f"  {os.path.relpath(path, ROOT)}: {hits}")
            if not dry:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(out)
    print(f"files={total_files} rewrites={total_hits}")


if __name__ == "__main__":
    main()
