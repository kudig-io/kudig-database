#!/usr/bin/env python3
"""
为工单样本补充 recommended frontmatter 字段。
"""

import re
from pathlib import Path


def extract_frontmatter(text: str):
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    if not fm_match:
        return None
    return fm_match.group(1)


def infer_difficulty(priority: str, severity: str) -> str:
    p = (priority or '').upper()
    if 'P0' in p:
        return 'advanced'
    return 'intermediate'


def infer_estimated_read_time(size: int) -> str:
    if size < 5000:
        return '5min'
    elif size < 10000:
        return '8min'
    return '10min'


def infer_prerequisites(title: str, tags: list) -> list:
    title_lower = title.lower()
    prereqs = ['kubectl-basics']
    if 'storage' in tags or 'pvc' in title_lower or 'statefulset' in title_lower:
        prereqs.append('k8s-storage')
    if 'network' in tags or 'service' in title_lower or 'ingress' in title_lower or 'dns' in title_lower:
        prereqs.append('k8s-networking')
    if 'security' in tags or 'rbac' in title_lower or 'certificate' in title_lower:
        prereqs.append('k8s-security')
    if 'backup' in title_lower or 'etcd' in title_lower:
        prereqs.append('k8s-backup')
    if 'aliyun' in title_lower or 'terway' in title_lower or 'slb' in title_lower:
        prereqs.append('alicloud-basics')
    return prereqs


def enhance_frontmatter(fm: str, title: str, size: int) -> str:
    # 提取 priority
    priority_match = re.search(r'^priority:\s*(.+)$', fm, re.MULTILINE)
    priority = priority_match.group(1).strip() if priority_match else 'P2'

    severity_match = re.search(r'^severity:\s*(.+)$', fm, re.MULTILINE)
    severity = severity_match.group(1).strip() if severity_match else 'medium'

    tags_match = re.search(r'^tags:\n((?:\s*- .+\n?)+)', fm, re.MULTILINE)
    tags = []
    if tags_match:
        tags = re.findall(r'-\s*(.+)', tags_match.group(1))

    additions = []

    if 'difficulty:' not in fm:
        additions.append(f"difficulty: {infer_difficulty(priority, severity)}")
    if 'reading_level:' not in fm:
        additions.append(f"reading_level: {infer_difficulty(priority, severity)}")
    if 'audience:' not in fm:
        additions.append("audience:\n- AI Agent\n- SRE\n- 运维工程师")
    if 'estimated_read_time:' not in fm:
        additions.append(f"estimated_read_time: {infer_estimated_read_time(size)}")
    if 'intent_queries:' not in fm:
        additions.append(f"intent_queries:\n- {title} 如何处理")
    if 'trigger_keywords:' not in fm:
        kw = [t.strip() for t in tags[:5]]
        if not kw:
            kw = [title.split()[0]]
        additions.append("trigger_keywords:\n" + "\n".join(f"- {k}" for k in kw))
    if 'prerequisites:' not in fm:
        prereqs = infer_prerequisites(title, tags)
        additions.append("prerequisites:\n" + "\n".join(f"- {p}" for p in prereqs))
    if 'k8s_versions:' not in fm:
        additions.append("k8s_versions:\n- '1.28'\n- '1.29'\n- '1.30'\n- '1.31'\n- '1.32'")
    if 'authors:' not in fm:
        additions.append("authors:\n- name: KUDIG Team\n  role: contributor")

    if additions:
        return fm.rstrip() + "\n" + "\n".join(additions) + "\n"
    return fm


def main():
    cases_dir = Path('domain-11-production-operations/ticket-cases')
    files = sorted(cases_dir.glob('ticket-case-*.md'))
    enhanced = 0

    for path in files:
        text = path.read_text(encoding='utf-8')
        fm = extract_frontmatter(text)
        if not fm:
            continue

        title_match = re.search(r'^title:\s*(.+)$', fm, re.MULTILINE)
        title = title_match.group(1).strip('"') if title_match else path.stem

        new_fm = enhance_frontmatter(fm, title, len(text))
        if new_fm != fm:
            text = text.replace(f"---\n{fm}\n---", f"---\n{new_fm}\n---", 1)
            path.write_text(text, encoding='utf-8')
            enhanced += 1

    print(f"已增强 frontmatter 的工单样本: {enhanced}/{len(files)}")


if __name__ == "__main__":
    main()
