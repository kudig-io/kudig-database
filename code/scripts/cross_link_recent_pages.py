#!/usr/bin/env python3
"""
为最近新增的 orphan 页面添加交叉链接。
策略：
1. 只处理最近 24 小时内新增/修改的页面
2. 只处理 orphan 页面（0 incoming links）
3. 基于标题关键词匹配 vault 中已有的页面
4. 添加 Related 章节（非内联，避免破坏正文）
5. 只应用高置信度（EXTRACTED/INFERRED）链接
"""

import re
from pathlib import Path
from datetime import datetime, timedelta
import yaml


def extract_frontmatter(text: str):
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None, text
    return fm_match.group(1), text[fm_match.end():]


def parse_frontmatter(fm_text: str) -> dict:
    try:
        return yaml.safe_load(fm_text) or {}
    except Exception:
        return {}


def build_registry(vault: Path):
    """构建页面注册表。"""
    registry = {}
    md_files = [p for p in vault.rglob('*.md') if not str(p.relative_to(vault)).startswith(('_archives/', '.git/', '.venv/', '.ruff_cache/', '.obsidian/'))]

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = extract_frontmatter(text)
        fm = parse_frontmatter(fm_text) if fm_text else {}

        title = fm.get('title', p.stem)
        aliases = fm.get('aliases', []) or []
        tags = fm.get('tags', []) or []
        category = fm.get('category', '') or str(p.parent.relative_to(vault)).split('/')[0]

        # 注册多个名称变体
        names = [p.stem, title] + aliases
        for name in names:
            key = name.lower().strip()
            if key and key not in registry:
                registry[key] = {
                    'path': rel,
                    'title': title,
                    'aliases': aliases,
                    'tags': tags,
                    'category': category,
                }

    return registry, md_files


def count_incoming_links(md_files: list, vault: Path) -> dict:
    """计算每个页面的入链数。"""
    incoming = {}
    path_map = {}

    for p in md_files:
        rel = str(p.relative_to(vault))
        incoming[rel] = 0
        path_map[rel.lower()] = rel

    for p in md_files:
        text = p.read_text(encoding='utf-8', errors='ignore')
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', text)
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            # 尝试多种匹配
            if target in path_map:
                incoming[path_map[target]] += 1
            else:
                # 尝试后缀匹配
                for rel_lower, rel in path_map.items():
                    if rel_lower.endswith('/' + target) or rel_lower[:-3] == target:
                        incoming[rel] += 1
                        break

    return incoming


def find_link_candidates(page_path: Path, title: str, tags: list, registry: dict, vault: Path) -> list:
    """为给定页面查找候选链接目标。"""
    title_lower = title.lower()
    candidates = []

    # 基于标题关键词匹配
    for key, info in registry.items():
        if info['path'] == str(page_path.relative_to(vault)):
            continue
        score = 0

        # 标题中的关键词出现在页面标题中
        if key in title_lower:
            score += 4

        # 共享标签
        shared_tags = set(tags or []) & set(info.get('tags', []) or [])
        if len(shared_tags) >= 2:
            score += 2
        elif len(shared_tags) == 1:
            score += 1

        # 同目录/同 category
        page_cat = str(page_path.parent.relative_to(vault)).split('/')[0]
        if info.get('category', '').split('/')[0] == page_cat:
            score += 1

        if score >= 3:
            candidates.append((score, info))

    # 去重并排序
    seen = set()
    unique = []
    for score, info in sorted(candidates, key=lambda x: -x[0]):
        if info['path'] not in seen:
            unique.append((score, info))
            seen.add(info['path'])

    return unique[:8]


def determine_relationship_type(source_title: str, target_title: str, shared_tags: set) -> str:
    """推断关系类型。"""
    if any(word in source_title.lower() for word in ['guide', 'guide', 'manual', 'playbook']) and target_title.lower() in source_title.lower():
        return 'implements'
    if shared_tags:
        return 'related_to'
    return 'related_to'


def add_related_section(page_path: Path, links: list, vault: Path):
    """为页面添加 Related 章节。"""
    text = page_path.read_text(encoding='utf-8')
    fm_text, body = extract_frontmatter(text)

    related_lines = ["\n## Related\n"]
    for score, info in links:
        target_path = info['path']
        target_title = info['title']
        # 使用最短路径
        display = target_title
        # 如果目标路径包含斜杠，使用 [[path|display]]
        if '/' in target_path:
            wikilink = f"[[{target_path}|{display}]]"
        else:
            wikilink = f"[[{target_path[:-3]}|{display}]]"
        related_lines.append(f"- {wikilink}")

    # 移除已有的空 Related 章节或追加
    if '## Related' in body:
        # 在已有 Related 后追加
        parts = body.split('## Related', 1)
        new_body = parts[0] + '## Related' + parts[1] + '\n'.join(related_lines[2:]) + '\n'
    else:
        new_body = body.rstrip() + '\n' + '\n'.join(related_lines) + '\n'

    new_text = f"---\n{fm_text}\n---{new_body}"
    page_path.write_text(new_text, encoding='utf-8')


def update_relationships_frontmatter(page_path: Path, links: list, vault: Path):
    """更新 frontmatter 中的 relationships 字段。"""
    text = page_path.read_text(encoding='utf-8')
    fm_text, body = extract_frontmatter(text)
    if not fm_text:
        return

    fm = parse_frontmatter(fm_text)
    source_tags = set(fm.get('tags', []) or [])

    relationships = fm.get('relationships', []) or []
    existing_targets = {r.get('target', '') for r in relationships}

    for score, info in links:
        target_path = info['path']
        target_title = info['title']
        target_wikilink = f"[[{target_path}]]"
        if target_wikilink in existing_targets:
            continue
        shared_tags = source_tags & set(info.get('tags', []) or [])
        rel_type = determine_relationship_type(fm.get('title', ''), target_title, shared_tags)
        relationships.append({
            'target': target_wikilink,
            'type': rel_type
        })
        existing_targets.add(target_wikilink)

    if relationships:
        fm['relationships'] = relationships
        new_fm = yaml.safe_dump(fm, allow_unicode=True, sort_keys=False, default_flow_style=False)
        new_text = f"---\n{new_fm}---{body}"
        page_path.write_text(new_text, encoding='utf-8')


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')
    cutoff = datetime.now() - timedelta(hours=24)

    print("Building page registry...")
    registry, md_files = build_registry(vault)
    print(f"  Registry size: {len(registry)}")
    print(f"  Total pages: {len(md_files)}")

    print("\nCounting incoming links...")
    incoming = count_incoming_links(md_files, vault)

    # 筛选最近新增且为 orphan 的页面
    recent_orphans = []
    for p in md_files:
        rel = str(p.relative_to(vault))
        if datetime.fromtimestamp(p.stat().st_mtime) >= cutoff and incoming[rel] == 0:
            recent_orphans.append(p)

    print(f"\nRecent orphan pages: {len(recent_orphans)}")

    links_added = 0
    pages_modified = 0
    details = []

    for p in recent_orphans:
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, _ = extract_frontmatter(text)
        fm = parse_frontmatter(fm_text) if fm_text else {}
        title = fm.get('title', p.stem)
        tags = fm.get('tags', []) or []

        candidates = find_link_candidates(p, title, tags, registry, vault)
        extracted = [(s, i) for s, i in candidates if s >= 6]
        inferred = [(s, i) for s, i in candidates if 3 <= s < 6]
        selected = extracted + inferred[:3]

        if selected:
            add_related_section(p, selected, vault)
            update_relationships_frontmatter(p, selected, vault)
            links_added += len(selected)
            pages_modified += 1
            confidence = 'EXTRACTED' if extracted else 'INFERRED'
            details.append((str(p.relative_to(vault)), len(selected), confidence, [i['path'] for _, i in selected]))

    print(f"\nLinks added: {links_added}")
    print(f"Pages modified: {pages_modified}")
    print("\nDetails:")
    for rel, count, conf, targets in details[:30]:
        print(f"  {rel}: +{count} ({conf}) -> {targets}")
    if len(details) > 30:
        print(f"  ... and {len(details) - 30} more")

    # 写入报告
    report_path = vault / '_reports/cross-linker-recent-2026-06-26.md'
    lines = [
        "---",
        "title: 近期新增页面 Cross-Link 报告（2026-06-26）",
        "description: 为最近 24 小时新增的 orphan 页面添加交叉链接",
        "category: reports",
        "tags:",
        "- cross-linker",
        "- wiki-maintenance",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 近期新增页面 Cross-Link 报告",
        "",
        f"- 扫描页面总数: {len(md_files)}",
        f"- 最近 24 小时 orphan 页面: {len(recent_orphans)}",
        f"- 修改页面数: {pages_modified}",
        f"- 新增链接数: {links_added}",
        "",
        "## Details",
        "",
        "| Page | Links Added | Confidence | Targets |",
        "|---|---|---|---|",
    ]
    for rel, count, conf, targets in details:
        target_str = ', '.join(targets[:3]) + ('...' if len(targets) > 3 else '')
        lines.append(f"| `{rel}` | {count} | {conf} | {target_str} |")
    report_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nReport written: {report_path}")


if __name__ == "__main__":
    main()
