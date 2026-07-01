#!/usr/bin/env python3
"""
为碎片化 tag clusters 创建 hub 页面并添加交叉链接。
处理选定的 medium-sized fragmented tags。
"""

import re
import yaml
from pathlib import Path
from collections import defaultdict


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


def dump_fm(fm: dict) -> str:
    def str_representer(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
        return dumper.represent_scalar('tag:yaml.org,2002:str', data)
    yaml.add_representer(str, str_representer)
    return yaml.dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)


def get_tags(p: Path) -> list:
    text = p.read_text(encoding='utf-8', errors='ignore')
    fm_text, _ = parse_frontmatter(text)
    if not fm_text:
        return []
    fm = load_fm(fm_text)
    tags = fm.get('tags', []) or []
    return [t for t in tags if isinstance(t, str)]


def add_related_link(p: Path, link_text: str) -> bool:
    """在页面的 Related 部分添加链接，如果没有 Related 则在末尾添加。"""
    text = p.read_text(encoding='utf-8', errors='ignore')
    original = text

    # 检查是否已存在
    if link_text in text:
        return False

    # 找 Related 部分
    related_match = re.search(r'\n## Related\n', text, re.IGNORECASE)
    if related_match:
        # 在 Related 部分开头添加
        pos = related_match.end()
        text = text[:pos] + f"\n{link_text}\n" + text[pos:]
    else:
        # 在文件末尾添加
        text = text.rstrip() + f"\n\n## Related\n\n{link_text}\n"

    if text != original:
        try:
            p.write_text(text, encoding='utf-8')
            return True
        except PermissionError:
            return False
    return False


def create_tag_hub(vault: Path, tag: str, pages: list) -> Path:
    """为 tag 创建 hub 页面。"""
    hub_rel = f"tags/{tag.replace('/', '-').replace('#', '').replace(' ', '-').lower()}.md"
    hub_path = vault / hub_rel
    hub_path.parent.mkdir(parents=True, exist_ok=True)

    safe_tag = tag.lstrip('#')
    title = safe_tag.replace('-', ' ').replace('_', ' ').title()

    lines = [
        "---",
        f"title: #{safe_tag} Tag Hub",
        "category: tags",
        f"tags: [{safe_tag}, meta, visibility/public]",
        "sources: []",
        f"created: \"2026-06-26\"",
        f"updated: \"2026-06-26\"",
        f"summary: \"Hub page collecting all pages tagged with #{safe_tag}.\"",
        "tier: supporting",
        "---",
        "",
        f"# #{safe_tag} Tag Hub",
        "",
        f"> 共 {len(pages)} 个页面带有 `#{safe_tag}` 标签。",
        "",
        "## Pages",
        "",
    ]

    for rel, title in sorted(pages, key=lambda x: x[1]):
        lines.append(f"- [[{rel[:-3]}\|{title}]]")

    lines.extend([
        "",
        "## Related Tags",
        "",
        f"- #{safe_tag}",
    ])

    hub_path.write_text('\n'.join(lines), encoding='utf-8')
    return hub_path


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    # 选定的 fragmented tags
    target_tags = ['research', 'deep-dive', 'papers', 'reference', 'visibility/public']

    # 扫描 tag -> pages
    tag_pages = defaultdict(list)
    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    for p in md_files:
        rel = str(p.relative_to(vault))
        tags = get_tags(p)
        for tag in tags:
            # 提取标题
            text = p.read_text(encoding='utf-8', errors='ignore')
            fm_text, _ = parse_frontmatter(text)
            title = Path(rel).stem
            if fm_text:
                fm = load_fm(fm_text)
                title = fm.get('title', title)
            if tag in target_tags:
                tag_pages[tag].append((rel, title))

    # 创建 hub 并添加链接
    total_modified = 0
    hubs_created = []

    for tag, pages in tag_pages.items():
        if len(pages) < 3:
            continue

        hub_path = create_tag_hub(vault, tag, pages)
        hubs_created.append(str(hub_path.relative_to(vault)))

        hub_link_text = f"- [[{hub_path.stem}\|#{tag} Hub]] — tag hub"

        for rel, title in pages:
            p = vault / rel
            if add_related_link(p, hub_link_text):
                total_modified += 1

    print(f"Created {len(hubs_created)} tag hubs")
    print(f"Modified {total_modified} pages")
    for hub in hubs_created:
        print(f"  {hub}")

    # 报告
    report_out = vault / '_reports/fragmented-tags-cross-link-2026-06-26.md'
    lines = [
        "---",
        "title: 碎片化 Tag Clusters Cross-Link 报告（2026-06-26）",
        "description: 为选定 fragmented tags 创建 hub 页面并添加交叉链接",
        "category: reports",
        "tags:",
        "- cross-linker",
        "- tags",
        "- maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 碎片化 Tag Clusters Cross-Link 报告",
        "",
        f"- **创建 hub 数**: {len(hubs_created)}",
        f"- **修改页面数**: {total_modified}",
        "",
        "## Created Hubs",
        "",
    ]
    for hub in hubs_created:
        lines.append(f"- `{hub}`")

    lines.extend([
        "",
        "## Target Tags",
        "",
    ])
    for tag in target_tags:
        count = len(tag_pages.get(tag, []))
        lines.append(f"- `#{tag}` — {count} pages")

    report_out.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_out}")


if __name__ == "__main__":
    main()
