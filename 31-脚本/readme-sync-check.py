#!/usr/bin/env python3
"""readme-sync-check.py — README 与实际目录一致性校验

校验两类"虚假承诺"：
1. 根 README.md 目录结构图中列出的顶层目录是否真实存在（反向：实际存在的
   NN- 顶层目录是否在目录图中有条目）
2. 各域 README.md「二级子目录」表格中声明的子目录/链接目标是否真实存在

用法:
    python3 31-脚本/readme-sync-check.py            # 校验，发现问题退出码 1
    python3 31-脚本/readme-sync-check.py --quiet    # 仅输出问题
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
# 顶层目录豁免：不要求出现在根 README 目录图中
TREE_EXEMPT = {'30-站点', '33-源码'}  # .gitignore 忽略的目录
# 冻结目录：只增不改，不参与域 README 校验
FROZEN = {'32-发布', '36-报告', '37-归档'}
WIKI = re.compile(r'\[\[([^\[\]]+?)\]\]')


def check_root_readme(problems):
    """根 README 目录图 vs 实际顶层 NN- 目录（仅解析树形图行，支持 A/ … B/ 区间折叠）"""
    readme = (ROOT / 'README.md').read_text(encoding='utf-8')
    declared = set()
    ranges = []  # (起始编号, 结束编号)
    for line in readme.splitlines():
        if '├──' not in line and '└──' not in line:
            continue
        rng = re.search(r'(\d{2})-[^\s/]+/\s*…\s*(\d{2})-[^\s/]+/', line)
        if rng:
            ranges.append((int(rng.group(1)), int(rng.group(2))))
            continue
        declared.update(re.findall(r'(\d{2}-[^\s/`│├└─|]+)/', line))
    actual = {p.name for p in ROOT.iterdir()
              if p.is_dir() and re.match(r'^\d{2}-', p.name)}

    def covered(d):
        if d in declared:
            return True
        n = int(d[:2])
        return any(lo <= n <= hi for lo, hi in ranges)

    for d in sorted(actual - TREE_EXEMPT):
        if not covered(d):
            problems.append(f"[根README] 实际存在但目录图未列出: {d}/")
    for d in sorted(declared - actual):
        problems.append(f"[根README] 目录图列出但实际不存在: {d}/")


def check_domain_readmes(problems):
    """各域 README 中 wikilink 指向的本域文件是否存在"""
    for readme in sorted(ROOT.glob('[0-9][0-9]-*/README.md')):
        domain = readme.parent.name
        if domain in FROZEN:
            continue
        text = readme.read_text(encoding='utf-8')
        in_fence = False
        for line in text.splitlines():
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            masked = re.sub(r'`[^`]*`', '', line)
            for m in WIKI.finditer(masked):
                target = re.split(r'[|#]', m.group(1).replace('\\|', '|'))[0].strip()
                if not target or target.startswith('http'):
                    continue
                # 只强校验带路径的链接（含 / 的），纯 stem 链接由 quality.yml 校验
                if '/' not in target:
                    continue
                rel = target if target.endswith('.md') else target + '.md'
                if not (ROOT / rel).exists():
                    problems.append(f"[{domain}/README] 链接目标不存在: {target}")
        # 声明的本域子目录（NN-域名/NN-子目录/ 形式）必须真实存在
        for sub in set(re.findall(rf'{re.escape(domain)}/(\d{{2}}-[^\s/`|\]]+)/', text)):
            if not (readme.parent / sub).is_dir():
                problems.append(f"[{domain}/README] 声明的子目录不存在: {domain}/{sub}/")


def main():
    quiet = '--quiet' in sys.argv
    problems = []
    check_root_readme(problems)
    check_domain_readmes(problems)
    if problems:
        print(f"README 一致性校验失败，共 {len(problems)} 个问题：")
        for p in problems:
            print("  " + p)
        sys.exit(1)
    if not quiet:
        print("README 一致性校验通过（根目录图 + 各域子目录声明）")


if __name__ == '__main__':
    main()
