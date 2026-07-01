#!/usr/bin/env python3
"""
工单样本去重与差异化审查。
对重复主题的样本进行标注，保留最具代表性的样本，其余标记为 duplicate。
"""

import re
from pathlib import Path
from collections import defaultdict


def extract_frontmatter_and_title(text: str):
    fm_match = re.search(r'^---\n(.*?)\n---', text, re.DOTALL)
    fm = fm_match.group(1) if fm_match else ""
    title_match = re.search(r'^title:\s*(.+)$', fm, re.MULTILINE)
    incident_match = re.search(r'^incident_id:\s*(.+)$', fm, re.MULTILINE)
    priority_match = re.search(r'^priority:\s*(.+)$', fm, re.MULTILINE)
    return {
        "frontmatter": fm,
        "title": title_match.group(1).strip('"') if title_match else "",
        "incident_id": incident_match.group(1).strip() if incident_match else "",
        "priority": priority_match.group(1).strip() if priority_match else "",
    }


def normalize_topic(title: str) -> str:
    """简单归一化主题，用于聚类。"""
    t = title.lower()
    # 去掉常见前缀
    t = re.sub(r'^阿里云专有云\s*', '', t)
    t = re.sub(r'^["\']', '', t)
    t = re.sub(r'["\']$', '', t)
    # 提取核心关键词
    if 'ingress' in t or '404/502' in t or '502' in t:
        return 'ingress-controller-404-502'
    if 'statefulset' in t and ('pvc' in t or '未绑定' in t):
        return 'statefulset-pvc-unbound'
    if 'diskpressure' in t or '磁盘压力' in t:
        return 'node-diskpressure'
    if 'pending' in t and ('资源' in t or 'cpu' in t or '内存' in t):
        return 'pod-pending-resource'
    if 'daemonset' in t or 'daemonset' in t:
        return 'daemonset-not-ready'
    if 'cronjob' in t or 'job' in t:
        return 'cronjob-job-failure'
    if 'kube-proxy' in t or 'kubeproxy' in t:
        return 'kubeproxy-service-unreachable'
    return t[:60]


def main():
    cases_dir = Path('domain-11-production-operations/ticket-cases')
    files = sorted(cases_dir.glob('ticket-case-*.md'))

    groups = defaultdict(list)
    for f in files:
        text = f.read_text(encoding='utf-8')
        info = extract_frontmatter_and_title(text)
        info['path'] = f
        info['size'] = len(text)
        topic = normalize_topic(info['title'])
        groups[topic].append(info)

    duplicates = {topic: items for topic, items in groups.items() if len(items) > 1}

    print(f"总样本数: {len(files)}")
    print(f"重复主题组: {len(duplicates)}")
    print()

    report_lines = [
        "---",
        "title: 工单样本去重与差异化审查报告（2026-06-26）",
        "description: 本轮 50 个工单样本的重复主题识别与处理建议",
        "category: reports",
        "tags:",
        "- ticket-agent",
        "- audit",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 工单样本去重与差异化审查报告",
        "",
        f"- 总样本数: {len(files)}",
        f"- 重复主题组: {len(duplicates)}",
        f"- 重复样本数: {sum(len(v) for v in duplicates.values()) - len(duplicates)}",
        "",
        "## 重复主题组详情",
        "",
    ]

    for topic, items in sorted(duplicates.items(), key=lambda x: -len(x[1])):
        # 按文件大小和内容完整性排序，保留最大的作为 representative
        items_sorted = sorted(items, key=lambda x: -x['size'])
        representative = items_sorted[0]

        report_lines.append(f"### {topic}")
        report_lines.append("")
        report_lines.append(f"- 样本数量: {len(items)}")
        report_lines.append(f"- 建议保留代表: `{representative['path'].name}` ({representative['incident_id']})")
        report_lines.append(f"- 代表标题: {representative['title']}")
        report_lines.append("")
        report_lines.append("| 文件 | incident_id | 优先级 | 字数 | 处理建议 |")
        report_lines.append("|---|---|---|---|---|")

        for item in items_sorted:
            is_rep = item == representative
            suggestion = "保留为代表" if is_rep else "标记为 duplicate_of 代表样本"
            report_lines.append(
                f"| `{item['path'].name}` | {item['incident_id']} | {item['priority']} | {item['size']} | {suggestion} |"
            )

            # 对非代表样本添加 duplicate_of 标注
            if not is_rep:
                text = item['path'].read_text(encoding='utf-8')
                fm = item['frontmatter']
                # 在 frontmatter 末尾添加字段
                new_fm = fm.rstrip() + f"\nduplicate_of: {representative['incident_id']}\nstatus: duplicate\nduplication_reason: 与 {representative['incident_id']} 主题重复，内容角度相似，降低 RAG 权重\n"
                text = text.replace(f"---\n{fm}\n---", f"---\n{new_fm}\n---", 1)
                item['path'].write_text(text, encoding='utf-8')

        report_lines.append("")

    report_path = Path('_reports/ticket-cases-dedup-review-2026-06-26.md')
    report_path.write_text("\n".join(report_lines), encoding='utf-8')
    print(f"审查报告已写入: {report_path}")


if __name__ == "__main__":
    main()
