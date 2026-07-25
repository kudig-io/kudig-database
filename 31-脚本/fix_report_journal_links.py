#!/usr/bin/env python3
"""
将核心页面中指向 _reports/ 和 _meta/journal/ 的 wikilink 转换为纯文本。
这些属于报告/日志引用，不应计入核心知识图链接。
"""

import re
from pathlib import Path


def is_core_page(rel: str) -> bool:
    excluded = (
        '_archives/', '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_reports/', '_meta/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
    )
    return not rel.startswith(excluded)


def fix_link_in_file(src_path: Path, target: str) -> bool:
    text = src_path.read_text(encoding='utf-8')
    original = text

    pattern = re.compile(rf'\[\[{re.escape(target)}(?:\|([^\]]*))?\]\]')

    def repl(match):
        display = match.group(1)
        return display if display else target

    text = pattern.sub(repl, text)
    if text != original:
        src_path.write_text(text, encoding='utf-8')
        return True
    return False


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    converted = []
    failed = []

    md_files = [p for p in vault.rglob('*.md') if is_core_page(str(p.relative_to(vault)))]

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', text)

        seen = set()
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()
            if target in seen:
                continue
            seen.add(target)

            if target.lower().startswith('_reports/') or target.lower().startswith('_meta/journal/'):
                if fix_link_in_file(p, target):
                    converted.append((rel, target))
                else:
                    failed.append((rel, target))

    print(f"Converted: {len(converted)}")
    print(f"Failed: {len(failed)}")

    report_out = vault / '_reports/report-journal-links-converted-2026-06-26.md'
    lines = [
        "---",
        "title: 报告/日志链接转换记录（2026-06-26）",
        "description: 将核心页面中指向 _reports/ 和 _meta/journal/ 的 wikilink 转为纯文本",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 报告/日志链接转换记录",
        "",
        f"- 转换链接数: {len(converted)}",
        f"- 失败: {len(failed)}",
        "",
        "## Converted Links",
        "",
        "| Source | Original |",
        "|---|---|",
    ]
    for src, target in converted[:100]:
        lines.append(f"| `{src}` | `[[{target}]]` |")

    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"Report written: {report_out}")


if __name__ == "__main__":
    main()
