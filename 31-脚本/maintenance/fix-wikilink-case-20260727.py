#!/usr/bin/env python3
"""
2026-07-27 wikilink 大小写修复（对应 P0-1 项）：
将 [[Kubernetes]] 这类与真实文件名大小写不一致的 wikilink 改写为
[[kubernetes|Kubernetes]]，保留显示文本与 #heading，表格行使用 \\| 转义。

规则：
- 仅处理内容层（20 个域目录 + 22~29 提炼层）
- 跳过代码围栏 ``` 与行内代码 `...`（避免误改 shell [[ ]] 测试语法）
- 候选文件多个时优先 22-概念 > 23-实体 > 其余（按路径排序取首个）

用法: python3 31-脚本/maintenance/fix-wikilink-case-20260727.py [--dry-run]
"""
import collections
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
NAME_EXCL = {'node_modules', '.venv', '.git', '__pycache__', '30-站点', '33-源码', '32-发布'}
SCAN_EXCL = NAME_EXCL | {'37-归档', '36-报告', '35-元数据', '28-资产', '31-脚本'}
CONTENT = {'22-概念', '23-实体', '26-技能', '24-综合', '29-文档', '20-最佳实践', '25-研究', '27-标签'}
DOMAINS = {'01-集群基础', '02-工作负载', '05-网络', '06-存储', '08-安全', '09-可观测性',
           '10-平台工程', '11-发布变更', '12-可靠性', '19-故障诊断', '13-生产运维', '18-云厂商',
           '14-容器运行时', '15-AI基础设施', '16-专项技术', '07-数据库中间件', '17-系统基础',
           '03-清单模式', '21-生态参考', '04-应用模式'}
PREFER = ('22-概念', '23-实体')

WIKI = re.compile(r'\[\[([^\[\]]+?)\]\]')
INLINE_CODE = re.compile(r'`[^`]*`')


def build_index():
    exact = set()
    lower = collections.defaultdict(set)
    for p in ROOT.rglob('*.md'):
        rel = p.relative_to(ROOT)
        if any(x in rel.parts for x in NAME_EXCL) or rel.parts[0].startswith('.'):
            continue
        exact.add(p.stem)
        lower[p.stem.lower()].add((0 if rel.parts[0] in PREFER else 1, p.stem))
    best = {}
    for k, cands in lower.items():
        best[k] = sorted(cands)[0][1]
    return exact, best


def parse_inner(inner: str):
    """拆解 wikilink 内部：target、heading、alias、是否表格转义。"""
    escaped = False
    alias = None
    m = re.search(r'\\\|', inner)
    if m:
        escaped = True
        target, alias = inner[:m.start()], inner[m.end():]
    elif '|' in inner:
        target, alias = inner.split('|', 1)
    else:
        target = inner
    heading = None
    if '#' in target:
        target, heading = target.split('#', 1)
    return target.strip(), heading, alias, escaped


def main() -> None:
    dry = '--dry-run' in sys.argv
    exact, best = build_index()
    n_files = n_links = 0

    def is_content(rel):
        return rel.parts and (rel.parts[0] in DOMAINS or rel.parts[0] in CONTENT)

    for p in sorted(ROOT.rglob('*.md')):
        rel = p.relative_to(ROOT)
        if any(x in rel.parts for x in SCAN_EXCL) or not is_content(rel):
            continue
        lines = p.read_text(encoding='utf-8', errors='ignore').split('\n')
        in_fence = False
        changed = False

        for i, line in enumerate(lines):
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                continue
            if in_fence or '[[' not in line:
                continue
            is_table = line.lstrip().startswith('|')
            masked = INLINE_CODE.sub(lambda m: '\x00' * len(m.group()), line)

            def fix(m):
                nonlocal changed
                inner = m.group(1)
                if '\x00' in inner:
                    return m.group(0)
                target, heading, alias, escaped = parse_inner(inner)
                stem = target.split('/')[-1]
                if stem.endswith('.md'):
                    stem = stem[:-3]
                if not stem or stem in exact or stem.lower() not in best:
                    return m.group(0)
                real = best[stem.lower()]
                if real == stem:
                    return m.group(0)
                changed = True
                new_target = real if '/' not in target else target.rsplit('/', 1)[0] + '/' + real
                if heading:
                    new_target += f'#{heading}'
                if alias is None:
                    alias = target.split('/')[-1] + (f'#{heading}' if heading else '')
                sep = '\\|' if (escaped or is_table) else '|'
                return f'[[{new_target}{sep}{alias}]]'

            # 用 masked 定位、原文替换：逐个 match 处理保持行内代码不动
            out, pos = [], 0
            for m in WIKI.finditer(masked):
                out.append(line[pos:m.start()])
                out.append(fix(m) if '\x00' not in m.group(1) else line[m.start():m.end()])
                pos = m.end()
            out.append(line[pos:])
            new_line = ''.join(out)
            if new_line != line:
                n_links += sum(1 for _ in WIKI.finditer(new_line)) and 1
                lines[i] = new_line

        if changed:
            n_files += 1
            if not dry:
                p.write_text('\n'.join(lines), encoding='utf-8')
            print(f"[fix] {rel}")

    print(f"\n修改文件数: {n_files}")


if __name__ == '__main__':
    main()
