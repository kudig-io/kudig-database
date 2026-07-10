#!/usr/bin/env python3
"""
KUDIG-DATABASE 场景导航页生成脚本
生成 20 个场景导航页到 topic-scenarios/ 目录。
"""

from pathlib import Path
from datetime import date

BASE_DIR = Path(__file__).parent.parent
SCENARIOS_DIR = BASE_DIR / "topic-scenarios"
SCENARIOS_DIR.mkdir(exist_ok=True)
TODAY = date.today().isoformat()

SCENARIOS = [
    {
        "id": "SC-01",
        "name": "cluster-deployment",
        "title": "集群部署",
        "title_en": "Cluster Deployment",
        "primary_tag": "deployment",
        "description": "Kubernetes 集群从 0 到 1 的部署指南，涵盖裸机、云托管、和混合部署模式",
        "overview": "集群部署是从零开始构建 Kubernetes 生产环境的第一步。本文档汇总了 KUDIG 知识库中所有与集群部署相关的文档、技能和故障树。",
        "docs": [
            ("domain-1-architecture-fundamentals", "12-cluster-deployment-patterns.md"),
            ("domain-1-architecture-fundamentals", "06-cluster-configuration-parameters.md"),
            ("domain-1-architecture-fundamentals", "07-upgrade-paths-strategy.md"),
            ("domain-3-control-plane", "03-plane-high-availability.md"),
            ("domain-9-platform-ops", "README.md"),
            ("topic-deployment", "README.md"),
        ],
        "ftas": ["apiserver-fta.md", "etcd-fta.md", "node-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-02",
        "name": "app-deployment",
        "title": "应用部署",
        "title_en": "Application Deployment",
        "primary_tag": "deployment",
        "description": "在 Kubernetes 上部署和运维应用的完整流程",
        "overview": "应用部署是 Kubernetes 最常见的操作场景。本场景汇总了 Deployment、StatefulSet、DaemonSet 等所有工作负载的部署模式和最佳实践。",
        "docs": [
            ("domain-4-workloads", "02-deployment-production-patterns.md"),
            ("domain-4-workloads", "03-statefulset-complete-guide.md"),
            ("domain-4-workloads", "04-daemonset-and-job.md"),
            ("domain-32-yaml-manifests", "README.md"),
        ],
        "ftas": ["pod-fta.md", "deployment-fta.md", "statefulset-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-03",
        "name": "troubleshooting",
        "title": "故障排查",
        "title_en": "Troubleshooting",
        "primary_tag": "troubleshooting",
        "description": "系统化故障排查方法论，覆盖所有知识域和组件",
        "overview": "故障排查是 SRE 和运维工程师的核心能力。本场景汇总了通用排查方法论、组件级故障树、和操作技能卡片。",
        "docs": [
            ("domain-12-troubleshooting", "README.md"),
            ("topic-structural-trouble-shooting", "README.md"),
            ("domain-1-architecture-fundamentals", "16-troubleshooting-guide.md"),
        ],
        "ftas": [],
        "skills": [],
        "all_ftas": True,
        "all_skills": True,
    },
    {
        "id": "SC-04",
        "name": "performance-tuning",
        "title": "性能调优",
        "title_en": "Performance Tuning",
        "primary_tag": "performance",
        "description": "Kubernetes 集群和应用性能优化，涵盖 CPU、内存、网络、存储",
        "overview": "性能调优涉及集群各个层面的参数调整和资源优化。",
        "docs": [
            ("domain-1-architecture-fundamentals", "13-performance-tuning-guide.md"),
            ("domain-9-platform-ops", "README.md"),
            ("domain-18-production-operations", "README.md"),
        ],
        "ftas": ["hpa-vpa-fta.md", "node-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-05",
        "name": "security-hardening",
        "title": "安全加固",
        "title_en": "Security Hardening",
        "primary_tag": "security",
        "description": "Kubernetes 安全加固，覆盖 RBAC、网络策略、Pod 安全、证书管理",
        "overview": "安全加固是生产环境的基础要求。",
        "docs": [
            ("domain-7-security", "README.md"),
            ("domain-25-cloud-native-security", "README.md"),
            ("domain-39-supply-chain-security", "README.md"),
        ],
        "ftas": ["secret-fta.md", "certificate-fta.md", "network-policy-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-06",
        "name": "monitoring-alerting",
        "title": "监控告警",
        "title_en": "Monitoring & Alerting",
        "primary_tag": "monitoring",
        "description": "Prometheus + Grafana 监控体系搭建和告警策略配置",
        "overview": "监控是生产可观测性的基础。",
        "docs": [
            ("domain-8-observability", "README.md"),
            ("domain-20-enterprise-monitoring-alerting", "README.md"),
            ("domain-21-logging-management-analytics", "README.md"),
        ],
        "ftas": ["metrics-server-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-07",
        "name": "backup-restore",
        "title": "备份恢复",
        "title_en": "Backup & Restore",
        "primary_tag": "backup-restore",
        "description": "etcd 备份恢复、PV 数据备份、集群灾备方案",
        "overview": "备份恢复是业务连续性的保障。",
        "docs": [
            ("domain-30-disaster-recovery-business-continuity", "README.md"),
            ("domain-3-control-plane", "README.md"),
        ],
        "ftas": ["backup-restore-fta.md", "etcd-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-08",
        "name": "upgrade-migration",
        "title": "升级迁移",
        "title_en": "Upgrade & Migration",
        "primary_tag": "migration",
        "description": "Kubernetes 版本升级、集群迁移、数据迁移",
        "overview": "升级迁移需要精心规划和执行。",
        "docs": [
            ("domain-1-architecture-fundamentals", "07-upgrade-paths-strategy.md"),
            ("domain-1-architecture-fundamentals", "18-upgrade-migration-strategy.md"),
            ("topic-migration", "README.md"),
        ],
        "ftas": ["cluster-upgrade-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-09",
        "name": "daily-ops",
        "title": "日常运维",
        "title_en": "Daily Operations",
        "primary_tag": "daily-ops",
        "description": "Kubernetes 日常运维操作手册",
        "overview": "日常运维是保障集群稳定的基础工作。",
        "docs": [
            ("domain-9-platform-ops", "README.md"),
            ("domain-1-architecture-fundamentals", "05-kubectl-commands-reference.md"),
            ("topic-skills", "README.md"),
        ],
        "ftas": [],
        "skills": [],
        "all_skills": True,
    },
    {
        "id": "SC-10",
        "name": "ai-infra-ops",
        "title": "AI 基础设施运维",
        "title_en": "AI Infrastructure Operations",
        "primary_tag": "ai",
        "description": "GPU 调度、模型服务、LLM 部署在 Kubernetes 上的运维",
        "overview": "AI 基础设施是 Kubernetes 的新兴场景。",
        "docs": [
            ("domain-11-ai-infra", "README.md"),
            ("topic-ai-agent", "README.md"),
        ],
        "ftas": [],
        "skills": [],
    },
    {
        "id": "SC-11",
        "name": "network-diagnosis",
        "title": "网络诊断",
        "title_en": "Network Diagnosis",
        "primary_tag": "networking",
        "description": "Kubernetes 网络问题系统化诊断",
        "overview": "网络问题是 K8s 运维中最常见的故障类型。",
        "docs": [
            ("domain-5-networking", "README.md"),
            ("domain-15-network-fundamentals", "README.md"),
            ("topic-terway", "README.md"),
        ],
        "ftas": ["dns-fta.md", "service-fta.md", "ingress-fta.md", "network-policy-fta.md", "kube-proxy-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-12",
        "name": "storage-issues",
        "title": "存储问题排查",
        "title_en": "Storage Issues",
        "primary_tag": "storage",
        "description": "PV/PVC/StorageClass 相关问题的排查和解决",
        "overview": "存储问题直接影响应用的持久化数据。",
        "docs": [
            ("domain-6-storage", "README.md"),
            ("domain-16-storage-fundamentals", "README.md"),
        ],
        "ftas": ["persistentvolume-fta.md", "storageclass-fta.md", "csi-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-13",
        "name": "security-incident",
        "title": "安全事件响应",
        "title_en": "Security Incident Response",
        "primary_tag": "security",
        "description": "安全事件应急响应流程和处置方法",
        "overview": "安全事件需要快速、系统化的响应。",
        "docs": [
            ("domain-7-security", "README.md"),
            ("domain-25-cloud-native-security", "README.md"),
            ("domain-39-supply-chain-security", "README.md"),
        ],
        "ftas": ["secret-fta.md", "certificate-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-14",
        "name": "capacity-planning",
        "title": "容量规划",
        "title_en": "Capacity Planning",
        "primary_tag": "capacity-planning",
        "description": "Kubernetes 集群容量评估、资源规划和扩容策略",
        "overview": "容量规划是成本优化的基础。",
        "docs": [
            ("domain-18-production-operations", "README.md"),
            ("domain-9-platform-ops", "README.md"),
        ],
        "ftas": ["hpa-vpa-fta.md", "cluster-autoscaler-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-15",
        "name": "gitops-workflow",
        "title": "GitOps 工作流",
        "title_en": "GitOps Workflow",
        "primary_tag": "gitops",
        "description": "基于 ArgoCD/Flux 的 GitOps 工作流搭建和运维",
        "overview": "GitOps 是现代化的持续部署方式。",
        "docs": [
            ("domain-23-gitops-ci-cd", "README.md"),
            ("domain-24-infrastructure-as-code", "README.md"),
        ],
        "ftas": ["helm-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-16",
        "name": "mesh-ops",
        "title": "Service Mesh 运维",
        "title_en": "Service Mesh Operations",
        "primary_tag": "mesh",
        "description": "Istio/Envoy Service Mesh 的部署、运维和故障排查",
        "overview": "Service Mesh 为微服务提供服务间治理能力。",
        "docs": [
            ("domain-26-service-mesh-microservices", "README.md"),
        ],
        "ftas": ["gateway-fta.md", "ingress-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-17",
        "name": "multi-cluster",
        "title": "多集群管理",
        "title_en": "Multi-Cluster Management",
        "primary_tag": "cloud",
        "description": "多 Kubernetes 集群的管理、服务发现和统一治理",
        "overview": "多集群是大规模生产环境的常见架构。",
        "docs": [
            ("domain-9-platform-ops", "README.md"),
            ("domain-27-multi-cloud-hybrid", "README.md"),
        ],
        "ftas": [],
        "skills": [],
    },
    {
        "id": "SC-18",
        "name": "edge-ops",
        "title": "边缘计算运维",
        "title_en": "Edge Computing Operations",
        "primary_tag": "edge",
        "description": "KubeEdge 等边缘 Kubernetes 集群的部署和运维",
        "overview": "边缘计算将 K8s 延伸到 IoT 和边缘场景。",
        "docs": [
            ("domain-37-edge-computing", "README.md"),
            ("domain-1-architecture-fundamentals", "09-edge-computing-kubeedge.md"),
        ],
        "ftas": [],
        "skills": [],
    },
    {
        "id": "SC-19",
        "name": "cost-optimization",
        "title": "成本优化",
        "title_en": "Cost Optimization",
        "primary_tag": "cost-optimization",
        "description": "Kubernetes 集群成本分析和优化策略",
        "overview": "成本优化直接影响云基础设施的 ROI。",
        "docs": [
            ("domain-18-production-operations", "README.md"),
            ("domain-9-platform-ops", "README.md"),
        ],
        "ftas": ["hpa-vpa-fta.md"],
        "skills": [],
    },
    {
        "id": "SC-20",
        "name": "compliance-audit",
        "title": "合规审计",
        "title_en": "Compliance & Audit",
        "primary_tag": "compliance",
        "description": "Kubernetes 安全合规审计和策略管理",
        "overview": "合规审计是企业级 K8s 的基础要求。",
        "docs": [
            ("domain-25-cloud-native-security", "README.md"),
            ("domain-39-supply-chain-security", "README.md"),
            ("domain-7-security", "README.md"),
        ],
        "ftas": ["secret-fta.md"],
        "skills": [],
    },
]


def generate_scenario_page(scenario: dict) -> str:
    """Generate a scenario navigation page."""
    s = scenario
    docs_md = ""
    for dir_name, filename in s.get("docs", []):
        docs_md += f"- [[../{dir_name}/{filename}]]\n"

    fta_md = ""
    if s.get("all_ftas"):
        fta_md = "- [[../topic-fta/MOC.md|所有 FTA 故障树]]\n"
    else:
        for fta in s.get("ftas", []):
            fta_md += f"- [[../topic-fta/list/{fta}]]\n"

    skills_md = ""
    if s.get("all_skills"):
        skills_md = "- [[../topic-skills/MOC.md|所有操作技能]]\n"
    else:
        for skill in s.get("skills", []):
            skills_md += f"- [[../topic-skills/{skill}]]\n"

    return f"""---
title: "场景: {s['title']}"
title_en: "Scenario: {s['title_en']}"
description: "{s['description']}"
category: scenario
tags: [k8s, scenario, {s['primary_tag']}]
scenario_id: "{s['id']}"
last_updated: "{TODAY}"
---

# 场景: {s['title']}

> **场景 ID**: {s['id']}
> **英文**: {s['title_en']}
> **最后更新**: {TODAY}

---

## 场景概述

{s['overview']}

---

## 快速决策树

```mermaid
graph TD
    A["{s['title']}"] --> B{{"问题确认"}}
    B -->|"已知问题"| C["参考相关文档"]
    B -->|"未知问题"| D{{"组件定位"}}
    D -->|"控制平面"| E["参考 domain-3-control-plane"]
    D -->|"工作负载"| F["参考 domain-4-workloads"]
    D -->|"网络"| G["参考 domain-5-networking"]
    D -->|"存储"| H["参考 domain-6-storage"]
    D -->|"安全"| I["参考 domain-7-security"]

    C --> J["执行修复"]
    E --> J
    F --> J
    G --> J
    H --> J
    I --> J

    J --> K{{"验证"}}
    K -->|"已解决"| L["记录关闭"]
    K -->|"未解决"| M["升级到专家"]

    style A fill:#ef4444,stroke:#b91c1c,color:#fff
    style L fill:#22c55e,stroke:#166534,color:#fff
    style M fill:#f59e0b,stroke:#b45309,color:#fff
```

---

## 相关文档

{docs_md}

---

## FTA 故障树

{fta_md if fta_md else "暂无专项 FTA\n"}

---

## 操作技能

{skills_md if skills_md else "暂无专项技能卡片\n"}

---

## 关联场景

| 关联场景 | 说明 |
|---|---|
"""


def main():
    print("=" * 60)
    print(f"生成 {len(SCENARIOS)} 个场景导航页...")
    print("=" * 60)

    for sc in SCENARIOS:
        content = generate_scenario_page(sc)
        output = SCENARIOS_DIR / f"{sc['name']}.md"
        output.write_text(content, encoding="utf-8")
        print(f"  OK  {sc['id']} {sc['name']} ({sc['title']})")

    # Generate MOC for scenarios
    toc_lines = []
    for sc in SCENARIOS:
        toc_lines.append(f"| {sc['id']} | [[{sc['name']}.md|{sc['title']}]] | {sc['title_en']} | {sc['description'][:50]} |")

    moc_content = f"""---
title: "场景导航 MOC"
description: "按生产场景组织的知识入口，共 {len(SCENARIOS)} 个场景"
category: moc
tags: [k8s, moc, scenario]
moc_scope: "topic-scenarios"
moc_type: "topic"
last_updated: "{TODAY}"
---

# 场景导航 MOC

> **场景总数**: {len(SCENARIOS)}
> **最后更新**: {TODAY}
> **用途**: 按"生产场景"而非"文档结构"组织知识入口

---

## 场景总览

| ID | 场景 | 英文 | 描述 |
|---|---|---|---|
{chr(10).join(toc_lines)}

---

## 场景分类

### 运维操作类
| 场景 | 描述 |
|---|---|
| [[daily-ops.md|日常运维]] | 日常运维操作手册 |
| [[troubleshooting.md|故障排查]] | 系统化故障排查 |
| [[performance-tuning.md|性能调优]] | 集群和应用性能优化 |
| [[capacity-planning.md|容量规划]] | 资源评估和扩容策略 |
| [[cost-optimization.md|成本优化]] | 成本分析和优化 |

### 部署发布类
| 场景 | 描述 |
|---|---|
| [[cluster-deployment.md|集群部署]] | 从零构建集群 |
| [[app-deployment.md|应用部署]] | 应用部署和运维 |
| [[upgrade-migration.md|升级迁移]] | 版本升级和数据迁移 |
| [[gitops-workflow.md|GitOps 工作流]] | 持续部署工作流 |

### 基础设施类
| 场景 | 描述 |
|---|---|
| [[network-diagnosis.md|网络诊断]] | 网络问题系统化诊断 |
| [[storage-issues.md|存储问题排查]] | PV/PVC 问题排查 |
| [[monitoring-alerting.md|监控告警]] | Prometheus + Grafana |
| [[backup-restore.md|备份恢复]] | etcd 备份和灾备 |

### 安全合规类
| 场景 | 描述 |
|---|---|
| [[security-hardening.md|安全加固]] | 全方位安全加固 |
| [[security-incident.md|安全事件响应]] | 安全应急响应 |
| [[compliance-audit.md|合规审计]] | 安全合规审计 |

### 新兴场景
| 场景 | 描述 |
|---|---|
| [[ai-infra-ops.md|AI 基础设施运维]] | GPU 调度和模型服务 |
| [[mesh-ops.md|Service Mesh 运维]] | Istio/Envoy 运维 |
| [[multi-cluster.md|多集群管理]] | 多集群统一管理 |
| [[edge-ops.md|边缘计算运维]] | KubeEdge 边缘集群 |

---

*本文档由 scripts/generate-scenarios.py 自动生成。*
"""

    moc_path = SCENARIOS_DIR / "MOC.md"
    moc_path.write_text(moc_content, encoding="utf-8")
    print(f"\n  OK  Scenario MOC ({len(SCENARIOS)} scenarios)")
    print(f"\n场景导航页生成完成: {len(SCENARIOS)} + 1 MOC")


if __name__ == "__main__":
    main()
