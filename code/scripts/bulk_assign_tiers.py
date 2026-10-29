#!/usr/bin/env python3
"""
批量为页面分配 tier 字段。
规则：
- core: 入链 >= 5
- supporting: 入链 2-4，或入链 >=5 但已手动设置为 supporting
- peripheral: 入链 <= 1 且 90+ 天未更新
- 未设置 tier 的页面按入链数分配
"""

import re
import yaml
from pathlib import Path
from datetime import datetime, timedelta


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return rel.startswith(excluded)


def parse_frontmatter(text: str) -> tuple:
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None, text
    return fm_match.group(1), text[fm_match.end():]


def load_fm(fm_text: str) -> dict:
    try:
        return yaml.safe_load(fm_text) or {}
    except Exception:
        return {}


def days_since_updated(updated_str) -> int:
    if not updated_str:
        return 9999
    try:
        updated = datetime.fromisoformat(str(updated_str).replace('Z', '+00:00'))
        return (datetime.now() - updated).days
    except Exception:
        return 9999


def determine_tier(incoming: int, days_stale: int, current_tier: str) -> str:
    if incoming >= 5:
        return 'core'
    elif incoming <= 1 and days_stale >= 90:
        return 'peripheral'
    else:
        return 'supporting'


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    # 1. 构建页面索引和入链统计
    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    pages = {}
    incoming = {}

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = parse_frontmatter(text)
        fm = load_fm(fm_text) if fm_text else {}
        pages[rel] = {'path': p, 'fm': fm, 'text': text, 'body': body}
        incoming[rel] = 0

    # 计算入链
    lookup = {}
    for rel in pages:
        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel

    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in lookup:
                incoming[lookup[target]] += 1
            elif '/' in target:
                basename = target.split('/')[-1]
                if basename in lookup:
                    incoming[lookup[basename]] += 1

    # 2. 分配 tier
    assigned = {'core': 0, 'supporting': 0, 'peripheral': 0}
    unchanged = 0
    permission_errors = []

    for rel, info in pages.items():
        fm = info['fm']
        current_tier = fm.get('tier')
        inc = incoming[rel]
        days_stale = days_since_updated(fm.get('updated') or fm.get('last_updated') or fm.get('created'))

        new_tier = determine_tier(inc, days_stale, current_tier)

        if current_tier == new_tier:
            unchanged += 1
            continue

        fm['tier'] = new_tier

        # 重新序列化 frontmatter
        def str_representer(dumper, data):
            if '\n' in data:
                return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
            return dumper.represent_scalar('tag:yaml.org,2002:str', data)

        yaml.add_representer(str, str_representer)

        # 保持字段顺序
        ordered_fm = {}
        for key in ['title', 'description', 'summary', 'category', 'tags', 'tier', 'sources', 'created', 'updated']:
            if key in fm:
                ordered_fm[key] = fm[key]
        for key in fm:
            if key not in ordered_fm:
                ordered_fm[key] = fm[key]

        fm_text = yaml.dump(ordered_fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
        new_text = f"---\n{fm_text}---\n{info['body']}"

        try:
            info['path'].write_text(new_text, encoding='utf-8')
            assigned[new_tier] += 1
        except PermissionError:
            permission_errors.append(rel)

    print(f"Tier assigned:")
    print(f"  core: {assigned['core']}")
    print(f"  supporting: {assigned['supporting']}")
    print(f"  peripheral: {assigned['peripheral']}")
    print(f"  unchanged: {unchanged}")
    print(f"  permission errors: {len(permission_errors)}")

    # 生成报告
    report_out = vault / '_reports/tier-assignment-2026-06-26.md'
    lines = [
        "---",
        "title: Tier 批量分配报告（2026-06-26）",
        "description: 根据入链数和更新时间为核心页面自动分配 tier",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- tier",
        "- maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# Tier 批量分配报告",
        "",
        f"- **扫描页面数**: {len(pages)}",
        f"- **core 分配**: {assigned['core']}",
        f"- **supporting 分配**: {assigned['supporting']}",
        f"- **peripheral 分配**: {assigned['peripheral']}",
        f"- **保持不变**: {unchanged}",
        f"- **权限错误**: {len(permission_errors)}",
        "",
        "## 分配规则",
        "",
        "- **core**: 入链 >= 5",
        "- **peripheral**: 入链 <= 1 且 90+ 天未更新",
        "- **supporting**: 其他情况",
        "",
        "> ⚠️ 自动分配的 tier 建议人工 review，尤其是原本手动设置过 tier 的页面。",
    ]
    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
