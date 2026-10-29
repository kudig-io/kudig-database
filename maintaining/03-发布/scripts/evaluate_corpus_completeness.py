#!/usr/bin/env python3
"""
评估 llm-wiki 语料内容完整度。
从规模、结构健康度、内容覆盖度、工单智能体适配度、阿里云覆盖等维度评分。
"""

import re
import json
import yaml
from pathlib import Path
from collections import defaultdict, Counter


def is_excluded(rel: str) -> bool:
    excluded = (
        '.git/', '.venv/', '.ruff_cache/', '.obsidian/',
        '_archives/', '_raw/', '_staging/',
        '.comate/', '.claude/', '.codebuddy/', '.qoder/',
        '.understand-anything/', '.zread/',
        'web/node_modules/', 'node_modules/',
        '_reports/', '_meta/',
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


def estimate_tokens(text: str) -> int:
    return len(text) // 4


def main():
    vault = Path('/Users/allengaller/Documents/GitHub/kudig-io/kudig-database')

    md_files = [p for p in vault.rglob('*.md') if not is_excluded(str(p.relative_to(vault)))]

    pages = {}
    incoming = defaultdict(int)
    outgoing = defaultdict(int)
    tag_counts = Counter()
    tier_counts = Counter()
    category_counts = Counter()
    domain_counts = Counter()

    lookup = {}

    for p in md_files:
        rel = str(p.relative_to(vault))
        text = p.read_text(encoding='utf-8', errors='ignore')
        fm_text, body = parse_frontmatter(text)
        fm = load_fm(fm_text) if fm_text else {}

        pages[rel] = {
            'path': p,
            'fm': fm,
            'text': text,
            'body': body,
            'tokens': estimate_tokens(text),
        }

        lookup[rel.lower()] = rel
        lookup[rel.lower()[:-3]] = rel
        lookup[Path(rel).stem.lower()] = rel
        lookup[Path(rel).name.lower()] = rel

        # 统计
        tags = fm.get('tags', []) or []
        for tag in tags:
            if isinstance(tag, str):
                tag_counts[tag] += 1

        tier = fm.get('tier', 'unset')
        tier_counts[tier] += 1

        category = fm.get('category', 'unset')
        category_counts[category] += 1

        # domain 分布
        if rel.startswith('domain-'):
            domain = rel.split('/')[0]
            domain_counts[domain] += 1

    # 入链/出链
    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        outgoing[rel] = len(set(links))
        for link in links:
            target = link.split('#')[0].split('?')[0].strip().lower()
            if target in lookup:
                incoming[lookup[target]] += 1
            elif '/' in target:
                basename = target.split('/')[-1]
                if basename in lookup:
                    incoming[lookup[basename]] += 1

    # 结构问题统计
    missing_summary = sum(1 for p in pages.values() if 'summary' not in p['fm'])
    missing_frontmatter = sum(1 for p in pages.values() if not p['fm'])
    missing_tier = sum(1 for p in pages.values() if 'tier' not in p['fm'])

    broken_links = 0
    for rel, info in pages.items():
        links = re.findall(r'\[\[([^\]|]+)(?:\|[^\]]*)?\]\]', info['text'])
        for link in links:
            target = link.split('#')[0].split('?')[0].strip()
            if re.fullmatch(r"[a-zA-Z0-9_.-]+", target):
                continue
            if target.startswith('http'):
                continue
            target_lower = target.lower()
            exists = target_lower in lookup
            if not exists and '/' in target_lower:
                exists = target_lower.split('/')[-1] in lookup
            if not exists:
                broken_links += 1

    # 内容覆盖度检查
    key_concepts = [
        'kubernetes', 'pod', 'deployment', 'statefulset', 'daemonset', 'service',
        'ingress', 'configmap', 'secret', 'persistent-volume', 'pvc',
        'hpa', 'vpa', 'cluster-autoscaler', 'karpenter',
        'prometheus', 'grafana', 'loki', 'jaeger', 'otel',
        'istio', 'envoy', 'cilium', 'calico', 'cni',
        'etcd', 'kubelet', 'apiserver', 'scheduler', 'controller-manager',
        'helm', 'argocd', 'gitops', 'terraform', 'iac',
        'falco', 'tetragon', 'kyverno', 'opa', 'rbac',
        'velero', 'backup', 'disaster-recovery',
        'containerd', 'docker', 'cri-o',
        'sla', 'slo', 'sli', 'error-budget',
    ]

    concept_hits = 0
    for concept in key_concepts:
        for rel in pages:
            if concept.replace('-', ' ') in rel.lower() or concept in Path(rel).stem.lower():
                concept_hits += 1
                break

    concept_coverage = concept_hits / len(key_concepts)

    # 工单智能体适配度
    ticket_pages = len([p for p in pages if 'ticket' in p.lower()])
    qa_files = list(vault.rglob('**/qa*.json')) + list(vault.rglob('**/qa*.jsonl')) + list(vault.rglob('**/qa*.yaml'))
    qa_pairs = 0
    for qf in qa_files:
        try:
            content = qf.read_text(encoding='utf-8')
            qa_pairs += content.count('"input"') + content.count('input:')
        except Exception:
            pass

    skill_docs = len([p for p in pages if 'skill' in p.lower()])

    # 阿里云覆盖
    aliyun_pages = len([p for p in pages if 'aliyun' in p.lower() or 'alibaba' in p.lower() or 'apsara' in p.lower()])
    ack_pages = len([p for p in pages if 'ack' in p.lower()])

    # RAG 适配度
    pages_with_summary = len(pages) - missing_summary
    pages_with_tags = sum(1 for p in pages.values() if p['fm'].get('tags'))
    pages_with_category = sum(1 for p in pages.values() if p['fm'].get('category'))

    # 评分
    scores = {
        'scale': min(100, len(pages) / 50),  # 5000 pages = 100
        'structure_health': max(0, 100 - (broken_links + missing_frontmatter * 10 + missing_summary / 100)),
        'concept_coverage': concept_coverage * 100,
        'ticket_readiness': min(100, (ticket_pages + qa_pairs / 10 + skill_docs) / 5),
        'aliyun_coverage': min(100, (aliyun_pages + ack_pages) / 3),
        'rag_readiness': (pages_with_summary / len(pages) * 40 + pages_with_tags / len(pages) * 30 + pages_with_category / len(pages) * 30) if pages else 0,
    }

    overall = sum(scores.values()) / len(scores)

    # 生成报告
    report = {
        'timestamp': '2026-06-26T12:00:00+08:00',
        'vault_path': str(vault),
        'overall_score': round(overall, 1),
        'scores': {k: round(v, 1) for k, v in scores.items()},
        'scale': {
            'total_pages': len(pages),
            'total_tokens': sum(p['tokens'] for p in pages.values()),
            'total_chars': sum(len(p['text']) for p in pages.values()),
        },
        'structure': {
            'broken_links': broken_links,
            'missing_frontmatter': missing_frontmatter,
            'missing_summary': missing_summary,
            'missing_tier': missing_tier,
            'orphans': sum(1 for rel in pages if incoming[rel] == 0),
            'core_pages': tier_counts.get('core', 0),
            'supporting_pages': tier_counts.get('supporting', 0),
            'peripheral_pages': tier_counts.get('peripheral', 0),
        },
        'content_coverage': {
            'key_concepts_checked': len(key_concepts),
            'key_concepts_hit': concept_hits,
            'coverage_ratio': round(concept_coverage, 2),
        },
        'ticket_agent': {
            'ticket_pages': ticket_pages,
            'qa_pairs_estimate': qa_pairs,
            'skill_docs': skill_docs,
        },
        'aliyun': {
            'aliyun_pages': aliyun_pages,
            'ack_pages': ack_pages,
        },
        'rag': {
            'pages_with_summary': pages_with_summary,
            'pages_with_tags': pages_with_tags,
            'pages_with_category': pages_with_category,
        },
        'top_categories': dict(category_counts.most_common(15)),
        'top_tags': dict(tag_counts.most_common(15)),
        'top_domains': dict(domain_counts.most_common(15)),
    }

    report_path = vault / '_reports/corpus-completeness-evaluation-2026-06-26.md'
    lines = [
        "---",
        "title: 语料内容完整度评估报告（2026-06-26）",
        "description: KUDIG Database 作为 llm-wiki 语料的内容完整度评估",
        "category: reports",
        "tags:",
        "- corpus",
        "- evaluation",
        "- llm-wiki",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 语料内容完整度评估报告",
        "",
        f"> **综合评分**: {report['overall_score']}/100",
        "",
        "## 评分维度",
        "",
        "| 维度 | 得分 | 说明 |",
        "|---|---|---|",
        f"| 规模 | {scores['scale']:.1f} | {len(pages)} 页面，{sum(p['tokens'] for p in pages.values())/1000:.0f}K tokens |",
        f"| 结构健康度 | {scores['structure_health']:.1f} | broken links={broken_links}, missing frontmatter={missing_frontmatter} |",
        f"| 概念覆盖度 | {scores['concept_coverage']:.1f} | {concept_hits}/{len(key_concepts)} 关键概念 |",
        f"| 工单智能体适配度 | {scores['ticket_readiness']:.1f} | ticket 页={ticket_pages}, QA 对≈{qa_pairs}, skill 文档={skill_docs} |",
        f"| 阿里云覆盖度 | {scores['aliyun_coverage']:.1f} | aliyun 页={aliyun_pages}, ack 页={ack_pages} |",
        f"| RAG 适配度 | {scores['rag_readiness']:.1f} | summary={pages_with_summary}, tags={pages_with_tags}, category={pages_with_category} |",
        "",
        "## 规模统计",
        "",
        f"- **总页面数**: {len(pages)}",
        f"- **总字符数**: {sum(len(p['text']) for p in pages.values()):,}",
        f"- **估算 Tokens**: {sum(p['tokens'] for p in pages.values()):,}",
        "",
        "## 结构健康度",
        "",
        f"- **Broken links**: {broken_links}",
        f"- **Missing frontmatter**: {missing_frontmatter}",
        f"- **Missing summary**: {missing_summary}",
        f"- **Missing tier**: {missing_tier}",
        f"- **Orphans**: {sum(1 for rel in pages if incoming[rel] == 0)}",
        f"- **Core pages**: {tier_counts.get('core', 0)}",
        f"- **Supporting pages**: {tier_counts.get('supporting', 0)}",
        f"- **Peripheral pages**: {tier_counts.get('peripheral', 0)}",
        "",
        "## 内容覆盖度",
        "",
        f"- 检查关键概念: {len(key_concepts)}",
        f"- 命中概念: {concept_hits}",
        f"- 覆盖率: {concept_coverage*100:.1f}%",
        "",
        "## 工单智能体适配度",
        "",
        f"- **Ticket 相关页面**: {ticket_pages}",
        f"- **估算 QA 对数**: {qa_pairs}",
        f"- **Skill 文档**: {skill_docs}",
        "",
        "## 阿里云/专有云覆盖",
        "",
        f"- **阿里云相关页面**: {aliyun_pages}",
        f"- **ACK 相关页面**: {ack_pages}",
        "",
        "## RAG 适配度",
        "",
        f"- **有 summary**: {pages_with_summary} / {len(pages)}",
        f"- **有 tags**: {pages_with_tags} / {len(pages)}",
        f"- **有 category**: {pages_with_category} / {len(pages)}",
        "",
        "## Top 15 Tags",
        "",
    ]
    for tag, count in tag_counts.most_common(15):
        lines.append(f"- `#{tag}` — {count}")

    lines.extend([
        "",
        "## Top 15 Categories",
        "",
    ])
    for cat, count in category_counts.most_common(15):
        lines.append(f"- `{cat}` — {count}")

    lines.extend([
        "",
        "## 评估结论",
        "",
    ])
    if overall >= 80:
        lines.append("综合评分达到 **优秀** 水平，语料规模充足、结构健康、覆盖全面，可以作为 llm-wiki 语料导出使用。")
    elif overall >= 60:
        lines.append("综合评分达到 **良好** 水平，基本满足 llm-wiki 语料要求，但仍有优化空间。")
    else:
        lines.append("综合评分 **一般**，建议继续补充内容和修复结构问题后再导出。")

    report_path.write_text('\n'.join(lines), encoding='utf-8')
    print(f"Report written: {report_path}")
    print(f"\nOverall score: {overall:.1f}/100")

    # 同时输出 JSON
    json_path = vault / '_reports/corpus-completeness-evaluation-2026-06-26.json'
    json_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f"JSON: {json_path}")


if __name__ == "__main__":
    main()
