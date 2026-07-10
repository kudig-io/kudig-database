#!/usr/bin/env python3
"""
检查本轮新增 Markdown 文件中的 wikilink 是否指向不存在的文件。
输出 broken links 报告，便于后续修复。
"""

import re
from pathlib import Path


def extract_wikilinks(text: str) -> list:
    """提取 [[path|display]] 或 [[path]] 中的 path 部分。"""
    pattern = r"\[\[([^\]|]+)(?:\|[^\]]*)?\]\]"
    return re.findall(pattern, text)


def normalize_target(target: str) -> str:
    """规范化 wikilink 目标。"""
    target = target.split("#")[0]
    target = target.split("?")[0]
    return target.strip()


def build_file_index(project_root: Path) -> set:
    """构建项目文件索引，用于快速判断目标是否存在。"""
    index = set()
    for p in project_root.rglob("*"):
        if p.is_file():
            rel = p.relative_to(project_root)
            index.add(str(rel))
            index.add(str(rel.with_suffix("")))
            index.add(p.name)
            index.add(p.stem)
    return index


def target_exists(target: str, file_index: set, project_root: Path) -> bool:
    """检查目标文件是否存在。"""
    target = normalize_target(target)
    if not target:
        return False

    if target in file_index:
        return True
    if (target + ".md") in file_index:
        return True

    direct_path = project_root / target
    if direct_path.exists():
        return True
    if (direct_path.with_suffix(".md")).exists():
        return True

    return False


def main():
    project_root = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")

    new_files = [
        "_meta/projects/kudig-ticket-agent-corpus-improvement-plan.md",
        "_meta/corpus-config/profiles/rag-ticket-agent-profile.yaml",
        "存储/03-distributed-storage/01-velero-backup-recovery.md",
        "存储/03-distributed-storage/02-rook-ceph-production.md",
        "存储/03-distributed-storage/03-longhorn-production.md",
        "存储/04-stateful-app-storage/01-stateful-app-storage-patterns.md",
        "发布变更/01-gitops/99-helm-production-guide.md",
        "生产运维/ticket-routing-rules.md",
        "生产运维/escalation-playbook.md",
        "生产运维/reply-templates/README.md",
        "故障诊断/topic-skills/skill-set/k8s-node-notready/SKILL-DEEP-DIVE.md",
        "故障诊断/topic-skills/skill-set/k8s-pod-crashloop/SKILL-DEEP-DIVE.md",
        "故障诊断/topic-skills/skill-set/k8s-service-unreachable/SKILL-DEEP-DIVE.md",
        "云厂商/01-alibaba-cloud/apsara-stack-components.md",
        "_reports/ticket-agent-corpus-execution-summary-2026-06-26.md",
        "_reports/ticket-agent-corpus-execution-summary-2026-06-26-final.md",
    ]

    ticket_cases = list((project_root / "生产运维/ticket-cases").glob("ticket-case-*.md"))
    new_files.extend([str(p.relative_to(project_root)) for p in ticket_cases])

    print("正在构建文件索引...")
    file_index = build_file_index(project_root)
    print(f"文件索引完成，共 {len(file_index)} 个条目")

    broken_links = []
    total_links = 0
    checked_files = 0

    for rel_path in new_files:
        path = project_root / rel_path
        if not path.exists() or path.suffix != ".md":
            continue
        checked_files += 1
        text = path.read_text(encoding="utf-8")
        links = extract_wikilinks(text)
        for link in links:
            total_links += 1
            if not target_exists(link, file_index, project_root):
                broken_links.append((rel_path, link))

    print(f"\n检查文件数: {checked_files}")
    print(f"总 wikilink 数: {total_links}")
    print(f"Broken links: {len(broken_links)}")

    if broken_links:
        print("\n详细列表（前 50）：")
        for src, target in broken_links[:50]:
            print(f"  {src} -> [[{target}]]")
        if len(broken_links) > 50:
            print(f"  ... 还有 {len(broken_links) - 50} 个")

    # 写入报告
    report_path = project_root / "_reports/new-wikilink-audit-2026-06-26.md"
    report_lines = [
        "---",
        "title: 新增文档 Wikilink 质量审计（2026-06-26）",
        "description: 本轮新增 Markdown 文件的 wikilink 指向检查",
        "category: reports",
        "tags:",
        "- wiki-lint",
        "- audit",
        "created: \"2026-06-26\"",
        "updated: \"2026-06-26\"",
        "---",
        "",
        "# 新增文档 Wikilink 质量审计",
        "",
        f"- 检查文件数: {checked_files}",
        f"- 总 wikilink 数: {total_links}",
        f"- Broken links: {len(broken_links)}",
        "",
    ]
    if broken_links:
        report_lines.append("## Broken Links")
        report_lines.append("")
        for src, target in broken_links:
            report_lines.append(f"- `{src}` -> `[[{target}]]`")
    else:
        report_lines.append("未发现 broken wikilink。")
    report_path.write_text("\n".join(report_lines), encoding="utf-8")
    print(f"\n报告已写入: {report_path}")


if __name__ == "__main__":
    main()
