#!/usr/bin/env python3
"""
Generate 98-merged-indexes/index.md for KUDIG Kubernetes corpus domains.
"""

import os
import re
from pathlib import Path

DOMAINS = [
    ("domain-01-cluster-fundamentals", "集群基础", "01"),
    ("domain-02-workloads-applications", "工作负载与应用", "02"),
    ("domain-03-networking-traffic", "网络与流量", "03"),
    ("domain-04-storage-data", "存储与数据", "04"),
    ("domain-07-platform-engineering", "平台工程", "07"),
    ("domain-08-release-change-management", "发布与变更管理", "08"),
    ("domain-09-reliability-engineering", "可靠性工程", "09"),
    ("domain-11-production-operations", "生产运维", "11"),
    ("domain-12-cloud-providers", "云厂商", "12"),
    ("domain-13-container-runtime", "容器运行时", "13"),
    ("domain-14-ai-ml-infra", "AI/ML 基础设施", "14"),
    ("domain-15-specialized-tech", "专项技术", "15"),
    ("domain-16-database-middleware", "数据库与中间件", "16"),
    ("domain-17-system-foundation", "系统基础", "17"),
    ("domain-18-manifests-patterns", "清单与模式", "18"),
    ("domain-19-landscape-references", "生态与参考", "19"),
    ("domain-20-application-patterns", "应用场景与架构", "20"),
]

DOMAIN_MAP = {d[0]: (d[1], d[2]) for d in DOMAINS}

RELATED = {
    "domain-01-cluster-fundamentals": ["domain-02-workloads-applications", "domain-17-system-foundation"],
    "domain-02-workloads-applications": ["domain-01-cluster-fundamentals", "domain-03-networking-traffic"],
    "domain-03-networking-traffic": ["domain-02-workloads-applications", "domain-04-storage-data"],
    "domain-04-storage-data": ["domain-03-networking-traffic", "domain-16-database-middleware"],
    "domain-07-platform-engineering": ["domain-08-release-change-management", "domain-11-production-operations"],
    "domain-08-release-change-management": ["domain-07-platform-engineering", "domain-09-reliability-engineering"],
    "domain-09-reliability-engineering": ["domain-08-release-change-management", "domain-11-production-operations"],
    "domain-11-production-operations": ["domain-07-platform-engineering", "domain-09-reliability-engineering"],
    "domain-12-cloud-providers": ["domain-01-cluster-fundamentals", "domain-07-platform-engineering"],
    "domain-13-container-runtime": ["domain-01-cluster-fundamentals", "domain-17-system-foundation"],
    "domain-14-ai-ml-infra": ["domain-02-workloads-applications", "domain-20-application-patterns"],
    "domain-15-specialized-tech": ["domain-14-ai-ml-infra", "domain-03-networking-traffic"],
    "domain-16-database-middleware": ["domain-04-storage-data", "domain-02-workloads-applications"],
    "domain-17-system-foundation": ["domain-01-cluster-fundamentals", "domain-13-container-runtime"],
    "domain-18-manifests-patterns": ["domain-01-cluster-fundamentals", "domain-08-release-change-management"],
    "domain-19-landscape-references": ["domain-01-cluster-fundamentals", "domain-07-platform-engineering"],
    "domain-20-application-patterns": ["domain-02-workloads-applications", "domain-14-ai-ml-infra"],
}

SKIP_DIRS = {"_archived-release-notes", ".git", ".venv", ".obsidian", ".claude", ".ruff_cache", ".understand-anything", ".comate", ".codebuddy"}

TOPIC_NAMES = {
    "01-architecture-overview": "架构概览",
    "02-design-principles": "设计原则",
    "03-control-plane": "控制平面",
    "04-api-versions": "API 版本",
    "05-kubectl": "Kubectl 工具",
    "06-upgrade-paths": "升级路径",
    "07-performance-tuning": "性能调优",
    "00-core-workloads": "核心工作负载",
    "topic-functions": "功能专题",
    "topic-java-kubernetes": "Java on Kubernetes",
    "00-core-k8s-networking": "核心 K8s 网络",
    "01-fundamentals": "网络基础",
    "02-service-mesh": "服务网格",
    "03-api-gateway": "API 网关",
    "04-ebpf": "eBPF 技术",
    "topic-terway": "Terway 网络",
    "01-k8s-storage": "K8s 存储",
    "02-storage-fundamentals": "存储基础",
    "03-distributed-storage": "分布式存储",
    "build": "平台构建",
    "developer-experience": "开发者体验",
    "governance": "治理与管控",
    "operate": "平台运维",
    "topic-code-analysis": "代码分析专题",
    "01-gitops": "GitOps 与 CI/CD",
    "02-iac": "基础设施即代码",
    "03-change-management": "变更管理",
    "04-testing-quality": "测试与质量",
    "topic-deployment": "部署专题",
    "topic-migration": "迁移专题",
    "01-backup-recovery": "备份恢复",
    "02-disaster-recovery": "灾难恢复",
    "03-capacity-planning": "容量规划",
    "04-slo-sli": "SLO/SLI 管理",
    "05-chaos-engineering": "混沌工程",
    "06-postmortem": "事后复盘",
    "07-sre-practices": "SRE 实践",
    "08-performance-testing": "性能测试",
    "09-disaster-recovery-playbooks": "灾难恢复手册",
    "01-finops": "FinOps 成本管理",
    "02-governance": "治理管理",
    "03-incident-response": "事件响应",
    "04-green-computing": "绿色计算",
    "01-alibaba-cloud": "阿里云",
    "01-aws-eks": "AWS EKS",
    "02-google-cloud-gke": "Google GKE",
    "03-azure-aks": "Azure AKS",
    "04-alicloud-ack": "阿里云 ACK",
    "05-tencent-tke": "腾讯云 TKE",
    "06-huawei-cce": "华为云 CCE",
    "06-multi-cloud": "多云管理",
    "07-ucloud-uk8s": "UCloud UK8S",
    "08-ibm-iks": "IBM IKS",
    "09-oracle-oke": "Oracle OKE",
    "10-volcengine-vek": "火山引擎 VEK",
    "11-ctyun-tke": "天翼云 TKE",
    "12-ecloud-cke": "移动云 CKE",
    "13-alicloud-apsara-ack": "阿里云专有云",
    "01-docker": "Docker 技术",
    "02-image-management": "镜像管理",
    "01-ai-infra": "AI 基础设施",
    "02-ai-agents": "AI Agent 系统",
    "topic-ai-agent": "AI Agent 专题",
    "topic-ai-coding": "AI 编程工具",
    "01-edge-computing": "边缘计算",
    "02-webassembly": "WebAssembly",
    "03-extensions": "K8s 扩展",
    "01-databases": "数据库",
    "03-message-queues": "消息队列",
    "04-time-series-db": "时序数据库",
    "05-operator-management": "Operator 管理",
    "06-data-streaming": "数据流处理",
    "01-linux": "Linux 系统",
    "02-hardware": "硬件技术",
    "03-kubernetes-events": "K8s 事件系统",
    "topic-cheat-sheet": "速查手册",
    "topic-dictionary": "术语词典",
    "01-yaml-reference": "YAML 参考手册",
    "01-cncf-landscape": "CNCF 生态全景",
    "02-papers": "技术论文",
    "topic-index": "主题索引",
    "topic-release-notes": "发布说明索引",
    "01-reference-architectures": "参考架构",
    "topic-application-architecture": "应用架构专题",
}

SUBTOPIC_NAMES = {
    "configuration": "配置管理",
    "fundamentals": "基础概念",
    "multi-cloud": "多云与边缘",
    "networking": "网络",
    "observability": "可观测性",
    "operations": "运维实践",
    "platform-engineering": "平台工程",
    "scheduling": "调度",
    "security": "安全",
    "specialized-workloads": "专项工作负载",
    "storage": "存储",
    "tooling": "工具链",
    "workloads": "工作负载",
    "cluster-cert": "集群证书",
    "cluster-create": "集群创建",
    "cluster-delete": "集群删除",
    "deployment-create": "Deployment 创建",
    "node-create": "节点创建",
    "graduated": "Graduated 项目",
    "incubating": "Incubating 项目",
    "sandbox": "Sandbox 项目",
    "openclaw-workspace": "OpenClaw 工作空间",
}


def get_display_name(dirname):
    return TOPIC_NAMES.get(dirname, dirname.replace("-", " ").replace("_", " ").title())


def get_subtopic_name(dirname):
    return SUBTOPIC_NAMES.get(dirname, dirname.replace("-", " ").replace("_", " ").title())


def clean_basename(path):
    base = os.path.basename(path)
    if base == "index.md":
        return None
    if base.endswith(".md"):
        return base[:-3]
    return base


def get_link_text(basename):
    cleaned = re.sub(r'^\d+[a-z]?[-_]', '', basename)
    cleaned = cleaned.replace('-', ' ').replace('_', ' ')
    if cleaned:
        cleaned = cleaned[0].upper() + cleaned[1:]
    return cleaned


def should_skip_dir(part):
    return part in SKIP_DIRS or part.startswith('.')


def collect_md_files(domain_dir):
    """Collect .md files grouped by subdirectory."""
    root = Path(domain_dir)
    groups = {}
    root_files = []

    for p in sorted(root.rglob('*.md')):
        parts = p.parts
        rel_parts = list(p.relative_to(root).parts)
        if should_skip_dir(rel_parts[0]):
            continue
        rel = p.relative_to(root)
        basename = clean_basename(str(rel))
        if basename is None:
            continue

        if len(rel.parts) == 1:
            root_files.append(basename)
        else:
            first_dir = rel.parts[0]
            if first_dir not in groups:
                groups[first_dir] = {}
            
            # For deeply nested dirs, use second-level as subkey
            if len(rel.parts) >= 3:
                second_dir = rel.parts[1]
                subkey = f"{first_dir}/{second_dir}"
            else:
                subkey = first_dir
            
            if subkey not in groups[first_dir]:
                groups[first_dir][subkey] = []
            groups[first_dir][subkey].append((str(rel.parent), basename))

    for first_dir in groups:
        for subkey in groups[first_dir]:
            groups[first_dir][subkey].sort(key=lambda x: x[0] + '/' + x[1])

    root_files.sort()
    return groups, root_files


def build_index(domain_dir, domain_name, domain_num):
    groups, root_files = collect_md_files(domain_dir)
    lines = []
    lines.append("---")
    lines.append(f'title: "Domain {domain_num} 内容索引"')
    lines.append(f"category: {domain_dir}")
    lines.append(f'tags: ["index", "{domain_dir}", "navigation"]')
    lines.append('sources: ["auto-generated"]')
    lines.append("created: 2026-05-21")
    lines.append("updated: 2026-05-21")
    lines.append("---")
    lines.append("")
    lines.append(f"# Domain {domain_num} 内容索引")
    lines.append("")
    lines.append(f"> 本索引汇总了 {domain_dir} 下的所有文档，按主题分组。")
    lines.append("")
    lines.append("## 概述")
    lines.append(f"- [[README]] — Domain 总览")
    lines.append("")

    link_count = 1

    non_readme_root = [f for f in root_files if f != 'README']
    if non_readme_root:
        lines.append("## 根目录文档")
        for f in non_readme_root:
            desc = get_link_text(f)
            lines.append(f"- [[{f}]] — {desc}")
            link_count += 1
        lines.append("")

    if groups:
        lines.append("## 按主题分组")
        lines.append("")

        for group_name in sorted(groups.keys()):
            subgroups = groups[group_name]
            display = get_display_name(group_name)
            
            # If only one subgroup and it equals group_name, no subgroup heading
            if len(subgroups) == 1 and group_name in subgroups:
                lines.append(f"### {display}")
                lines.append("")
                for parent, basename in subgroups[group_name]:
                    desc = get_link_text(basename)
                    if parent == group_name:
                        lines.append(f"- [[{basename}]] — {desc}")
                    else:
                        rel_path = parent.replace(group_name + '/', '')
                        if rel_path:
                            lines.append(f"- [[{parent}/{basename}|{basename}]] — {desc}")
                        else:
                            lines.append(f"- [[{basename}]] — {desc}")
                    link_count += 1
                lines.append("")
            else:
                # Has subgroups
                lines.append(f"### {display}")
                lines.append("")
                for subkey in sorted(subgroups.keys()):
                    sub_name = subkey.split('/')[-1] if '/' in subkey else subkey
                    sub_display = get_subtopic_name(sub_name)
                    lines.append(f"#### {sub_display}")
                    lines.append("")
                    for parent, basename in subgroups[subkey]:
                        desc = get_link_text(basename)
                        if parent == group_name:
                            lines.append(f"- [[{basename}]] — {desc}")
                        else:
                            rel_path = parent.replace(group_name + '/', '')
                            if rel_path:
                                lines.append(f"- [[{parent}/{basename}|{basename}]] — {desc}")
                            else:
                                lines.append(f"- [[{basename}]] — {desc}")
                        link_count += 1
                    lines.append("")

    related = RELATED.get(domain_dir, [])
    if related:
        lines.append("## 相关 Domain")
        for rd in related:
            if rd in DOMAIN_MAP:
                name, num = DOMAIN_MAP[rd]
                lines.append(f'- [[{rd}/98-merged-indexes/index|Domain {num} {name} 索引]]')
        lines.append("")

    return "\n".join(lines), link_count


def main():
    results = []
    for domain_dir, domain_name, domain_num in DOMAINS:
        idx_dir = Path(domain_dir) / "98-merged-indexes"
        idx_dir.mkdir(parents=True, exist_ok=True)

        content, link_count = build_index(domain_dir, domain_name, domain_num)
        idx_path = idx_dir / "index.md"
        idx_path.write_text(content, encoding='utf-8')

        line_count = len(content.splitlines())
        results.append((str(idx_path), link_count, line_count))
        print(f"Created: {idx_path} ({link_count} links, {line_count} lines)")

    print("\n" + "=" * 60)
    print("Summary:")
    total_links = 0
    for path, count, lines in results:
        print(f"  {path}: {count} links, {lines} lines")
        total_links += count
    print(f"\nTotal: {len(results)} files, {total_links} links")


if __name__ == "__main__":
    main()
