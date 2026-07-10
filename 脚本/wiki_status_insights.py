#!/usr/bin/env python3
"""
Wiki Status Insights 生成脚本。
生成 _insights.md，包含 anchor pages、tag cohesion、tier suggestions 等。
"""

import re
import json
from pathlib import Path
from collections import defaultdict, Counter


def is_core_page(rel: str) -> bool:
    excluded = (
        '_archives/', '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_raw/', '_staging/',
    )
    return not rel.startswith(excluded)


def extract_frontmatter(text: str):
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None
    return fm_match.group(1)


def parse_tags(fm_text: str) -> set:
    tags = set()
    m = re.search(r'^tags:\n((?:\s*- .+\n?)+)', fm_text, re.MULTILINE)
    if m:
        tags = set(re.findall(r'-\s*(.+)', m.group(1)))
    return tags


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')
    print("Building graph...")

    md_files = [p for p in vault.rglob('*.md') if is_core_page(str(p.relative_to(vault)))]

    pages = {}
    incoming = defaultdict(int)
    outgoing = defaultdict(int)
    tags_of = {}
    category_of = {}
    title_of = {}

    # First pass: collect basic info
    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text = extract_frontmatter(text)
        tags = parse_tags(fm_text) if fm_text else set()
        category = str(p.parent.relative_to(vault)).split('/')[0]

        # Extract title
        title = p.stem
        if fm_text:
            m = re.search(r'^title:\s*(.+)$', fm_text, re.MULTILINE)
            if m:
                title = m.group(1).strip('"')

        pages[rel] = {'text': text, 'title': title, 'tags': tags, 'category': category}
        tags_of[rel] = tags
        category_of[rel] = category
        title_of[rel] = title

    # Build link index
    link_index = defaultdict(list)  # target -> [sources]
    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()
            link_index[target.lower()].append(rel)
        outgoing[rel] = len(set(links))

    # Count incoming
    valid_targets = {}
    for rel in pages:
        valid_targets[rel.lower()] = rel
        valid_targets[rel.lower()[:-3]] = rel
        valid_targets[Path(rel).stem.lower()] = rel
        valid_targets[Path(rel).name.lower()] = rel

    for rel in pages:
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', pages[rel]['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in valid_targets:
                incoming[valid_targets[target]] += 1

    # Anchor pages
    print("Computing anchor pages...")
    anchor_pages = sorted(pages.keys(), key=lambda x: -incoming[x])[:20]

    # Tag cohesion
    print("Computing tag cohesion...")
    tag_pages = defaultdict(list)
    for rel, tags in tags_of.items():
        for tag in tags:
            tag_pages[tag].append(rel)

    cohesion_scores = []
    for tag, rels in tag_pages.items():
        if len(rels) < 5:
            continue
        n = len(rels)
        # Sample for efficiency
        sample = rels[:50]
        actual = 0
        for i in range(len(sample)):
            for j in range(i + 1, len(sample)):
                target_j = sample[j][:-3] if sample[j].endswith('.md') else sample[j]
                target_i = sample[i][:-3] if sample[i].endswith('.md') else sample[i]
                if target_j in pages[sample[i]]['text'] or target_i in pages[sample[j]]['text']:
                    actual += 1
        max_links = len(sample) * (len(sample) - 1) / 2
        cohesion = actual / max_links if max_links > 0 else 0
        cohesion_scores.append((tag, n, cohesion))

    cohesion_scores.sort(key=lambda x: -x[2])
    top_cohesive = cohesion_scores[:10]
    bottom_cohesive = sorted(cohesion_scores, key=lambda x: x[2])[:10]

    # Orphan-adjacent: pages linked from top hub but 0 outgoing
    print("Computing orphan-adjacent...")
    top_hub = anchor_pages[0] if anchor_pages else None
    orphan_adjacent = []
    if top_hub:
        hub_text = pages[top_hub]['text']
        hub_links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', hub_text)
        for link in hub_links[:50]:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in valid_targets:
                target_rel = valid_targets[target]
                if outgoing[target_rel] == 0:
                    orphan_adjacent.append((target_rel, title_of[target_rel]))

    # Tier suggestions
    print("Computing tier suggestions...")
    tier_promotions = []
    tier_demotions = []
    for rel in pages:
        inc = incoming[rel]
        # Check existing tier
        fm_text = extract_frontmatter(pages[rel]['text'])
        tier = None
        if fm_text:
            m = re.search(r'^tier:\s*(\S+)', fm_text, re.MULTILINE)
            if m:
                tier = m.group(1)

        if inc >= 5 and tier in (None, 'supporting'):
            tier_promotions.append((rel, title_of[rel], inc, tier))
        elif inc <= 1 and tier in (None, 'supporting'):
            tier_demotions.append((rel, title_of[rel], inc, tier))

    tier_promotions = sorted(tier_promotions, key=lambda x: -x[2])[:10]

    # Graph snapshot
    print("Building graph snapshot...")
    snapshot_nodes = [rel for rel in anchor_pages[:30]]
    snapshot_edges = []
    seen_edges = set()
    for rel in anchor_pages[:30]:
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', pages[rel]['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in valid_targets:
                target_rel = valid_targets[target]
                edge = tuple(sorted([rel, target_rel]))
                if edge not in seen_edges:
                    snapshot_edges.append(list(edge))
                    seen_edges.add(edge)

    # Read previous snapshot for delta
    prev_insights = vault / '_meta/_insights.md'
    prev_nodes = set()
    prev_edges = set()
    if prev_insights.exists():
        text = prev_insights.read_text(encoding='utf-8', errors='ignore')
        m = re.search(r'<!-- GRAPH_SNAPSHOT: (.*?) -->', text, re.DOTALL)
        if m:
            try:
                prev = json.loads(m.group(1))
                prev_nodes = set(prev.get('nodes', []))
                prev_edges = set(tuple(e) for e in prev.get('edges', []))
            except Exception:
                pass

    current_nodes = set(snapshot_nodes)
    current_edges = set(tuple(e) for e in snapshot_edges)
    new_nodes = current_nodes - prev_nodes
    lost_nodes = prev_nodes - current_nodes
    new_edges = current_edges - prev_edges
    lost_edges = prev_edges - current_edges

    # Write insights
    print("Writing _insights.md...")
    lines = [
        "---",
        "title: KUDIG Wiki Insights",
        "category: meta",
        "tags: [meta, insights, visibility/public]",
        "sources: [Vault Scan 2026-06-26]",
        "created: \"2026-05-24\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# Wiki Insights — 2026-06-26",
        "",
        "> 基于 wikilink 图分析的仓库结构洞察。",
        "",
        "## 统计摘要",
        "",
        f"- **总页面数**: {len(pages)}",
        f"- **总 wikilink**: {sum(outgoing.values())}",
        f"- **孤儿页面数**: {sum(1 for rel in pages if incoming[rel] == 0)} ({sum(1 for rel in pages if incoming[rel] == 0)/len(pages)*100:.1f}%)",
        f"- **平均入站链接**: {sum(incoming.values())/len(pages):.2f}" if pages else "- **平均入站链接**: 0",
        f"- **平均出站链接**: {sum(outgoing.values())/len(pages):.2f}" if pages else "- **平均出站链接**: 0",
        "",
        "## Anchor Pages（Top 20 Hubs）",
        "",
        "| 排名 | 页面 | 入站链接 | 出站链接 | 类型 |",
        "|---|---|---|---|---|",
    ]

    for idx, rel in enumerate(anchor_pages, 1):
        inc = incoming[rel]
        out = outgoing[rel]
        note = "connector hub" if out > 5 else "hub" if inc > 100 else "sink hub"
        title = title_of[rel]
        lines.append(f"| {idx} | [[{title}\|{title}]] | {inc} | {out} | {note} |")

    lines.extend([
        "",
        "## Tag Cluster Cohesion",
        "",
        "### 最紧密的 Cluster（Top 10）",
        "",
        "| 标签 | 页面数 | 紧密度 |",
        "|---|---|---|",
    ])
    for tag, n, cohesion in top_cohesive:
        lines.append(f"| #{tag} | {n} | {cohesion:.2f} |")

    lines.extend([
        "",
        "### 最松散的 Cluster（Bottom 10，需 cross-linker 关注）",
        "",
        "| 标签 | 页面数 | 紧密度 | 状态 |",
        "|---|---|---|---|",
    ])
    for tag, n, cohesion in bottom_cohesive:
        lines.append(f"| #{tag} | {n} | {cohesion:.2f} | ⚠️ 需关注 |")

    lines.extend([
        "",
        "## Orphan-Adjacent（Hub 引用但无出站的死胡同）",
        "",
        "| 页面 | 说明 |",
        "|---|---|",
    ])
    for rel, title in orphan_adjacent[:10]:
        lines.append(f"| [[{title}\|{title}]] | 被 top hub 引用但 0 出站 |")

    lines.extend([
        "",
        "## Tier Suggestions",
        "",
    ])
    if tier_promotions:
        for rel, title, inc, tier in tier_promotions:
            current = tier or 'unset'
            lines.append(f"↑ core    [[{title}\|{title}]] — {inc} incoming links, currently tier={current}")
    if tier_demotions:
        for rel, title, inc, tier in tier_demotions[:5]:
            current = tier or 'unset'
            lines.append(f"↓ peripheral [[{title}\|{title}]] — {inc} incoming, currently tier={current}")
    if not tier_promotions and not tier_demotions:
        lines.append("Tier assignments look healthy — no changes suggested.")

    lines.extend([
        "",
        "## Graph Delta Since Last Run",
        "",
        f"- +{len(new_nodes)} new anchor nodes, -{len(lost_nodes)} lost anchor nodes",
        f"- +{len(new_edges)} new edges, -{len(lost_edges)} lost edges",
    ])
    if new_nodes:
        lines.append(f"- Newly connected: {', '.join(list(new_nodes)[:5])}")
    if lost_nodes:
        lines.append(f"- Lost from anchor set: {', '.join(list(lost_nodes)[:5])}")

    lines.extend([
        "",
        "## Questions Worth Asking",
        "",
        f"1. Link: {len([r for r in pages if incoming[r] == 0])} pages have zero incoming links — what should reference them?",
        f"2. Audit: Should the most fragmented tags be split or cross-linked?",
        "3. Explore: Why do top hub pages attract so many links, and do they need better outbound connections?",
        "",
        f"<!-- GRAPH_SNAPSHOT: {json.dumps({'nodes': snapshot_nodes, 'edges': snapshot_edges}, ensure_ascii=False)} -->",
    ])

    insights_path = vault / '_meta/_insights.md'
    insights_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"\nInsights written: {insights_path}")

    # Update log.md
    log_path = vault / 'log.md'
    if log_path.exists():
        log_text = log_path.read_text(encoding='utf-8')
        log_entry = f"\n- [2026-06-26T12:00:00+08:00] STATUS_INSIGHTS anchors=20 cohesion_checked={len(cohesion_scores)} tier_suggestions={len(tier_promotions) + len(tier_demotions)} delta=\"+{len(new_nodes)} nodes +{len(new_edges)} edges\"\n"
        # Insert after frontmatter
        fm_end = log_text.find('\n---\n') + 5
        new_log = log_text[:fm_end] + log_entry + log_text[fm_end:]
        log_path.write_text(new_log, encoding='utf-8')
        print("log.md updated")


if __name__ == "__main__":
    main()
