#!/usr/bin/env python3
"""P0-1 收尾：标题式 wikilink 重映射（2026-07-27）

剩余断链多为"用文档标题而非文件名"的链接（如 [[Topic 应用层架构设计最佳实践]]）。
本脚本从 frontmatter title 与 H1 标题建立 标题→文件 索引，唯一匹配时重写为
[[相对路径|原显示文本]]。多义/无匹配的目标输出报告，人工处理。

用法:
    python3 remap-title-links-20260727.py --dry-run   # 预览
    python3 remap-title-links-20260727.py             # 执行
"""
import collections
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
EXCL = {'node_modules', '.venv', '.git', '__pycache__', '30-站点', '32-发布', '33-源码'}
SCAN_EXCL = EXCL | {'37-归档', '36-报告', '35-元数据', '28-资产', '31-脚本'}
WIKI = re.compile(r'\[\[([^\[\]]+?)\]\]')
# 标题清洗：去掉 [entities]/[concepts] 等后缀标记与首尾空白
TITLE_SUFFIX = re.compile(r'\s*\[(entities|concepts|synthesis|skills|research)\]\s*$')


def norm_title(t: str) -> str:
    t = TITLE_SUFFIX.sub('', t.strip().strip('\'"'))
    return re.sub(r'\s+', ' ', t)


def build_indexes():
    names = set()
    title_map = collections.defaultdict(set)  # norm title -> {relpath(no ext)}
    for p in ROOT.rglob('*.md'):
        if any(x in p.parts for x in EXCL) or p.relative_to(ROOT).parts[0].startswith('.'):
            continue
        rel = str(p.relative_to(ROOT).with_suffix(''))
        names.add(p.stem)
        names.add(p.name)
        names.add(rel)
        try:
            text = p.read_text(encoding='utf-8')
        except Exception:
            continue
        # frontmatter title
        if text.startswith('---'):
            end = text.find('\n---', 3)
            if end > 0:
                m = re.search(r'^title:\s*(.+)$', text[:end], re.M)
                if m:
                    title_map[norm_title(m.group(1))].add(rel)
        # 首个 H1
        m = re.search(r'^#\s+(.+)$', text, re.M)
        if m:
            title_map[norm_title(m.group(1))].add(rel)
    return names, title_map


def parse_inner(inner: str):
    escaped = '\\|' in inner
    work = inner.replace('\\|', '\x01')
    if '|' in work:
        target, alias = work.split('|', 1)
    elif '\x01' in work:
        target, alias = work.split('\x01', 1)
    else:
        target, alias = work, None
    heading = None
    if '#' in target:
        target, heading = target.split('#', 1)
    return target.strip().rstrip('\\'), heading, (alias.replace('\x01', '|') if alias else None), escaped


def main():
    dry = '--dry-run' in sys.argv
    names, title_map = build_indexes()
    changed_files = 0
    unresolved = collections.Counter()
    ambiguous = {}

    for p in sorted(ROOT.rglob('*.md')):
        if any(x in p.parts for x in SCAN_EXCL) or p.relative_to(ROOT).parts[0].startswith('.'):
            continue
        text = p.read_text(encoding='utf-8')
        lines = text.splitlines(keepends=True)
        in_fence = False
        modified = False
        out_lines = []
        for line in lines:
            if line.lstrip().startswith('```'):
                in_fence = not in_fence
                out_lines.append(line)
                continue
            if in_fence:
                out_lines.append(line)
                continue

            def repl(m):
                nonlocal modified
                inner = m.group(1)
                target, heading, alias, escaped = parse_inner(inner)
                if not target or target.startswith('http'):
                    return m.group(0)
                stem = target.rsplit('/', 1)[-1]
                if target in names or stem in names or (stem + '.md') in names:
                    return m.group(0)
                key = norm_title(target)
                cands = title_map.get(key, set())
                if len(cands) == 1:
                    new_target = next(iter(cands))
                    display = alias if alias else target
                    sep = '\\|' if (escaped or '|' in line and line.strip().startswith('|')) else '|'
                    frag = f'#{heading}' if heading else ''
                    modified = True
                    changes.append((str(p.relative_to(ROOT)), target, new_target))
                    return f'[[{new_target}{frag}{sep}{display}]]'
                elif len(cands) > 1:
                    ambiguous[target] = sorted(cands)
                else:
                    unresolved[target] += 1
                return m.group(0)

            # 行内代码掩码保护
            code_spans = [(m.start(), m.end()) for m in re.finditer(r'`[^`]*`', line)]

            def in_code(pos):
                return any(s <= pos < e for s, e in code_spans)

            new_line = WIKI.sub(lambda m: m.group(0) if in_code(m.start()) else repl(m), line)
            out_lines.append(new_line)

        if modified:
            changed_files += 1
            if not dry:
                p.write_text(''.join(out_lines), encoding='utf-8')

    print(f"{'[DRY-RUN] ' if dry else ''}修改文件={changed_files} 重映射链接={len(changes)}")
    print(f"\n--- 无法解析（无标题匹配，共 {len(unresolved)} 目标 / {sum(unresolved.values())} 处）---")
    for t, c in unresolved.most_common(40):
        print(f"{c:3d}  {t}")
    if ambiguous:
        print(f"\n--- 多义目标（{len(ambiguous)} 个，未改动）---")
        for t, cs in list(ambiguous.items())[:20]:
            print(f"  {t} -> {cs}")


changes = []
if __name__ == '__main__':
    main()
