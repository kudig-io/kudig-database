#!/usr/bin/env python3
"""
最终清理：
1. 修复剩余 3 个 broken wikilinks
2. 修复 relationships 字段中的 display text 和无效 type
"""

import re
from pathlib import Path


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def fix_body_broken_links(vault: Path):
    """修复 body 中最后的 broken links。"""
    fixed = []

    # 1. CONTRIBUTING.md 中的 [[页面名]]
    contributing = vault / 'CONTRIBUTING.md'
    if contributing.exists():
        text = contributing.read_text(encoding='utf-8')
        if '[[页面名]]' in text:
            text = text.replace('[[页面名]]', '页面名')
            try:
                contributing.write_text(text, encoding='utf-8')
                fixed.append('CONTRIBUTING.md -> 页面名')
            except PermissionError:
                pass

    # 2. synthesis/ticket-agent-rag.md 中的 _meta 链接
    rag = vault / 'synthesis/ticket-agent-rag.md'
    if rag.exists():
        text = rag.read_text(encoding='utf-8')
        if '[[_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md]]' in text:
            text = text.replace('[[_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md]]',
                                '_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md')
            try:
                rag.write_text(text, encoding='utf-8')
                fixed.append('synthesis/ticket-agent-rag.md -> _meta link')
            except PermissionError:
                pass

    # 3. 工作负载/topic-functions/MOC.md 中的 _meta 链接
    moc = vault / '工作负载/topic-functions/MOC.md'
    if moc.exists():
        text = moc.read_text(encoding='utf-8')
        if '[[_meta/corpus-config/embedding-guide.md]]' in text:
            text = text.replace('[[_meta/corpus-config/embedding-guide.md]]',
                                '_meta/corpus-config/embedding-guide.md')
            try:
                moc.write_text(text, encoding='utf-8')
                fixed.append('工作负载/topic-functions/MOC.md -> _meta link')
            except PermissionError:
                pass

    print(f"Fixed body links: {len(fixed)}")
    for f in fixed:
        print(f"  {f}")


def fix_relationships(vault: Path):
    """修复 relationships 字段中的问题。"""
    allowed_types = {'extends', 'implements', 'contradicts', 'derived_from', 'uses', 'replaces', 'related_to'}
    fixed_files = 0
    fixed_issues = 0

    for p in vault.rglob('*.md'):
        if is_excluded(str(p.relative_to(vault))):
            continue
        if not p.is_file():
            continue

        try:
            text = p.read_text(encoding='utf-8')
        except Exception:
            continue

        original = text

        # 1. 清理 target 中的 display text: [[path|display]] -> [[path]]
        text = re.sub(r'(target:\s*"?)\[\[([^\]|]+)\|[^\]]+\]\]("?)', r'\1[[\2]]\3', text)

        # 2. 将纯文本 target 转为带 [[ ]] 的链接
        # 例如 target: "metrics server" -> target: "[[metrics server]]"
        # 但这种转换风险高，只处理明显是链接的 ticket-case 和 skills/ concepts/
        def fix_plain_text_target(match):
            prefix = match.group(1)
            value = match.group(2).strip()
            suffix = match.group(3)

            # 如果已经是 [[...]] 跳过
            if value.startswith('[[') and value.endswith(']]'):
                return match.group(0)

            # 如果包含空格或路径分隔符，可能是链接
            if (' ' in value or '/' in value) and not value.startswith('http'):
                # 检查是否是允许的 type 值被误判
                if match.group(0).strip().startswith('type:'):
                    return match.group(0)
                # 否则包装为 [[value]]
                return f'{prefix}[[{value}]]{suffix}'

            return match.group(0)

        # 这个正则只能用于 target: 行
        text = re.sub(r'^([ \t]*-\s+target:\s*"?)([^"\n]+?)("?\s*)$', fix_plain_text_target, text, flags=re.MULTILINE)

        # 3. 修复 invalid type: related -> related_to
        text = re.sub(r'^(\s*-\s*type:\s*)related\s*$', r'\1related_to', text, flags=re.MULTILINE)

        if text != original:
            try:
                p.write_text(text, encoding='utf-8')
                fixed_files += 1
                fixed_issues += original.count('target:')  # rough count
            except PermissionError:
                pass

    print(f"Fixed relationships in {fixed_files} files")


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')
    fix_body_broken_links(vault)
    fix_relationships(vault)


if __name__ == "__main__":
    main()
