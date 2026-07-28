#!/usr/bin/env python3
"""2026-07-23 前缀改名后的全库链接重写（解析式）。

处理三类引用（.md 文件）：
1. wikilink  [[域/子目录/文件...]]      —— 视为库根相对路径，按段映射
2. md 链接   ](relative/or/../path)    —— 按源文件旧位置解析→映射→按新位置回算相对路径
3. frontmatter  path: "../域/..."      —— 同 md 链接的解析逻辑

跳过：.git .venv node_modules 30-站点 33-源码 及冻结目录 32-发布 36-报告 37-归档。

用法: python3 31-脚本/maintenance/rewrite-prefix-links-20260723.py [--dry-run]
"""
import os
import posixpath
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# 从改名脚本导入映射
_ns = {}
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "rename-prefix-20260723.py"), encoding="utf-8") as f:
    exec(f.read().split("def run(")[0], _ns)
ROOT_MAP = _ns["ROOT_MAP"]
L2_MAP_RAW = _ns["L2_MAP"]

L2_MAP = {}      # (parent_old, child_old) -> child_new
for parent, pairs in L2_MAP_RAW.items():
    for c_old, c_new in pairs:
        L2_MAP[(parent, c_old)] = c_new

INV_ROOT = {v: k for k, v in ROOT_MAP.items()}
INV_L2 = {(parent, c_new): c_old for (parent, c_old), c_new in L2_MAP.items()}

SKIP_DIRS = {".git", ".venv", "node_modules", "__pycache__", ".obsidian", ".ruff_cache",
             ".zread", ".qoder", ".understand-anything", ".claude", ".comate",
             ".codebuddy", ".mimocode", ".zcode", ".github",
             "30-站点", "33-源码", "32-发布", "36-报告", "37-归档"}

stats = {"files": 0, "wiki": 0, "md": 0, "fm": 0, "skip_pct": 0}


def map_old_path(p):
    """旧库根相对路径 -> 新库根相对路径。不匹配则原样返回。"""
    segs = p.split("/")
    if not segs or segs[0] not in ROOT_MAP:
        return p
    old0 = segs[0]
    segs[0] = ROOT_MAP[old0]
    if len(segs) > 1 and (old0, segs[1]) in L2_MAP:
        segs[1] = L2_MAP[(old0, segs[1])]
    return "/".join(segs)


def inv_map_path(p):
    """新库根相对路径 -> 旧库根相对路径（用于回算源文件旧位置）。"""
    segs = p.split("/")
    if not segs or segs[0] not in INV_ROOT:
        return p
    new0 = segs[0]
    old0 = INV_ROOT[new0]
    segs[0] = old0
    if len(segs) > 1 and (old0, segs[1]) in INV_L2:
        segs[1] = INV_L2[(old0, segs[1])]
    return "/".join(segs)


def rewrite_relative(href, old_src_dir, new_src_dir):
    """解析相对 href：旧目录解析→映射→新目录回算。返回新 href 或 None（不变）。"""
    if not href or href.startswith(("http://", "https://", "mailto:", "#", "obsidian://")):
        return None
    if "%" in href:
        stats["skip_pct"] += 1
        return None
    # 拆锚点
    anchor = ""
    path_part = href
    if "#" in href:
        path_part, anchor = href.split("#", 1)
        anchor = "#" + anchor
    if not path_part:
        return None
    trailing = "/" if path_part.endswith("/") and path_part != "/" else ""
    if path_part.startswith("/"):
        old_t = path_part.lstrip("/")
        new_t = map_old_path(old_t)
        if new_t == old_t:
            return None
        return "/" + new_t + trailing + anchor
    old_target = posixpath.normpath(posixpath.join(old_src_dir, path_part))
    if old_target.startswith(".."):
        return None
    new_target = map_old_path(old_target)
    old_expected = posixpath.normpath(posixpath.join(new_src_dir, path_part))
    # 若旧解析目标映射后 == 从新位置解析出的路径，则链接天然仍正确，无需改
    new_href = posixpath.relpath(new_target, new_src_dir)
    if new_href == posixpath.normpath(path_part) and old_expected == new_target:
        return None
    if posixpath.normpath(new_href) == posixpath.normpath(path_part):
        return None
    return new_href + trailing + anchor


WIKI_RE = re.compile(r"(!?\[\[)([^\]\[|#]+)((?:#[^\]\[|]*)?(?:\|[^\]\[]*)?\]\])")
# 啅形嵌套 wikilink（别名里又套 [[..]]）：只重写 | 前的目标段
WIKI_TARGET_RE = re.compile(r"(!?\[\[)([^\]\[|#\n]+)(\|)")
MDLINK_RE = re.compile(r"(\]\()([^)\s]+)(\))")
FMPATH_RE = re.compile(r'(path:\s*")([^"]+)(")')


def rewrite_file(new_rel):
    abspath = os.path.join(ROOT, new_rel)
    try:
        text = open(abspath, encoding="utf-8").read()
    except (UnicodeDecodeError, OSError):
        return
    old_rel = inv_map_path(new_rel)
    old_dir = posixpath.dirname(old_rel) or "."
    new_dir = posixpath.dirname(new_rel) or "."

    def wiki_sub(m):
        target = m.group(2).strip()
        if "/" not in target:
            return m.group(0)
        if target.startswith(("./", "../")):
            new_href = rewrite_relative(target, old_dir, new_dir)
            if new_href is None:
                return m.group(0)
            stats["wiki"] += 1
            return m.group(1) + new_href + m.group(3)
        mapped = map_old_path(target)
        if mapped == target:
            return m.group(0)
        stats["wiki"] += 1
        return m.group(1) + mapped + m.group(3)

    def md_sub(m):
        new_href = rewrite_relative(m.group(2), old_dir, new_dir)
        if new_href is None:
            return m.group(0)
        stats["md"] += 1
        return m.group(1) + new_href + m.group(3)

    def fm_sub(m):
        new_href = rewrite_relative(m.group(2), old_dir, new_dir)
        if new_href is None:
            return m.group(0)
        stats["fm"] += 1
        return m.group(1) + new_href + m.group(3)

    def wiki_target_sub(m):
        target = m.group(2).strip()
        if "/" not in target:
            return m.group(0)
        mapped = map_old_path(target)
        if mapped == target:
            return m.group(0)
        stats["wiki"] += 1
        return m.group(1) + mapped + m.group(3)

    out = WIKI_RE.sub(wiki_sub, text)
    out = WIKI_TARGET_RE.sub(wiki_target_sub, out)
    out = MDLINK_RE.sub(md_sub, out)
    out = FMPATH_RE.sub(fm_sub, out)
    if out != text:
        stats["files"] += 1
        if "--dry-run" not in sys.argv:
            with open(abspath, "w", encoding="utf-8") as f:
                f.write(out)


def main():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        rel = os.path.relpath(dirpath, ROOT)
        parts = rel.split(os.sep)
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        if parts[0] in SKIP_DIRS:
            continue
        for fn in filenames:
            if fn.endswith(".md"):
                p = fn if rel == "." else rel.replace(os.sep, "/") + "/" + fn
                rewrite_file(p)
    print(f"changed files={stats['files']} wiki={stats['wiki']} "
          f"md={stats['md']} fm={stats['fm']} skipped-pct-encoded={stats['skip_pct']}")


if __name__ == "__main__":
    main()
