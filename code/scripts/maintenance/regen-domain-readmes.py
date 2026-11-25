#!/usr/bin/env python3
"""Regenerate the body section of each Chinese-domain README.md to reflect
the post-restructure 2nd-level directories. Frontmatter is preserved.

Usage: python3 scripts/maintenance/regen-domain-readmes.py [--apply]
Without --apply, only prints what would change.
"""
from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
WIKI = REPO / "wiki"

DOMAINS = {
    "01-集群基础与系统": {
        "title": "集群基础与系统 Cluster & System",
        "desc": "K8s 集群架构、控制平面、升级路径，以及 Linux、硬件、事件、速查卡与生产就绪指南。",
        "tag": "domain/cluster-system",
    },
    "02-工作负载": {
        "title": "工作负载 Workloads",
        "desc": "Kubernetes 核心工作负载与 Java-on-K8s 等垂直栈实践。",
        "tag": "domain/workloads-applications",
    },
    "03-清单模式": {
        "title": "清单模式 Manifests & Patterns",
        "desc": "YAML 参考、Kustomize 模式与 Helm 值模式。",
        "tag": "domain/manifests-patterns",
    },
    "04-应用模式": {
        "title": "应用模式 Application Patterns",
        "desc": "子模式、行业架构与生产模式。",
        "tag": "domain/application-patterns",
    },
    "05-网络": {
        "title": "网络 Networking",
        "desc": "K8s 网络核心、系统与 K8s 网络基础、服务网格、API 网关、eBPF 与 Terway。",
        "tag": "domain/networking-traffic",
    },
    "06-存储与数据": {
        "title": "存储与数据 Storage & Data",
        "desc": "K8s 存储原语、存储基础、分布式存储，以及数据库、缓存、消息队列等数据中间件。",
        "tag": "domain/storage-data",
    },
    "07-安全": {
        "title": "安全 Security",
        "desc": "身份与访问、网络/运行时安全、策略治理、供应链与合规审计。",
        "tag": "domain/security-compliance",
    },
    "08-可观测性": {
        "title": "可观测性 Observability",
        "desc": "指标、日志、链路追踪、告警、SLO/SLI 与工具集。",
        "tag": "domain/observability",
    },
    "09-发布变更": {
        "title": "发布变更 Release & Change",
        "desc": "GitOps、IaC、变更管理、测试质量、部署方案与迁移方案（含迁移三件套主场）。",
        "tag": "domain/release-change-management",
    },
    "10-可靠性": {
        "title": "可靠性 Reliability",
        "desc": "备份恢复、灾难恢复、容量规划、混沌工程、事后复盘、SRE 实践与性能测试。",
        "tag": "domain/reliability-engineering",
    },
    "11-生产运维": {
        "title": "生产运维 Production Ops",
        "desc": "成本治理、集群治理、事件响应、绿色计算、工单案例与回复话术。",
        "tag": "domain/production-operations",
    },
    "12-平台工程": {
        "title": "平台工程 Platform Engineering",
        "desc": "构建、运维、治理、开发体验与平台代码分析。",
        "tag": "domain/platform-engineering",
    },
    "13-容器运行时": {
        "title": "容器运行时 Container Runtime",
        "desc": "Docker、containerd/CRI-O、镜像管理、镜像构建与运行时迁移。",
        "tag": "domain/container-runtime",
    },
    "14-AI基础设施": {
        "title": "AI 基础设施 AI Infra",
        "desc": "AI 基础设施、AI-Agents、Agent 运行时与 AI 编码。",
        "tag": "domain/ai-ml-infra",
    },
    "15-专项技术": {
        "title": "专项技术 Specialized",
        "desc": "边缘计算、WebAssembly、扩展机制与无服务器。",
        "tag": "domain/specialized-tech",
    },
    "16-云厂商": {
        "title": "云厂商 Cloud Providers",
        "desc": "阿里云、AWS-EKS、Google-GKE、Azure-AKS、腾讯云 TKE、华为云 CCE、多云混合、其他云与云厂商发行版实体。",
        "tag": "domain/cloud-providers",
    },
    "17-故障诊断": {
        "title": "故障诊断 Troubleshooting",
        "desc": "核心/资源/基础设施/高级排障、JVM 调优、FTA 故障树、FEBM 方法论、QA 语料、工具与多故障场景。",
        "tag": "domain/troubleshooting-diagnostics",
    },
    "18-最佳实践": {
        "title": "最佳实践 Best Practices",
        "desc": "通用/基础设施/部署/运维/安全/可观测/场景化最佳实践与生产卓越清单。",
        "tag": "domain/best-practices",
    },
    "19-生态参考": {
        "title": "生态参考 Ecosystem",
        "desc": "CNCF 全景、论文、开源项目与领域索引。",
        "tag": "domain/landscape-references",
    },
    "20-概念": {
        "title": "概念 Concepts",
        "desc": "Kubernetes 概念深度主场（核心架构、工作负载、网络、存储、调度与资源等）。",
        "tag": "domain/concepts",
    },
    "21-知识字典": {
        "title": "知识字典 Knowledge Dictionary",
        "desc": "术语速查条目：工作负载、网络、存储、安全、运维等分类字典。",
        "tag": "domain/knowledge-dictionary",
    },
    "22-实体": {
        "title": "实体 Entities",
        "desc": "K8s 核心组件、网络/存储/可观测/安全等项目实体卡。",
        "tag": "domain/entities",
    },
    "23-综合与研究": {
        "title": "综合与研究 Synthesis & Research",
        "desc": "综合提炼、研究与消化的二手提炼层。",
        "tag": "domain/synthesis-research",
    },
    "24-标签": {
        "title": "标签 Tags",
        "desc": "按标签聚合的横切导航。",
        "tag": "domain/tags",
    },
    "25-资产": {
        "title": "资产 Assets",
        "desc": "演示、培训与可复用资产。",
        "tag": "domain/assets",
    },
    "26-源码分析": {
        "title": "源码分析 Source Analysis",
        "desc": "Kubernetes 与云原生项目源码分析。",
        "tag": "domain/source-analysis",
    },
    "32-故障案例库": {
        "title": "故障案例库 Failure Cases",
        "desc": "kuba 故障案例库（subtree 并入的真实故障案例集）。",
        "tag": "domain/failure-cases",
    },
}


def list_subdirs(domain: str) -> list[str]:
    p = WIKI / domain
    if not p.is_dir():
        return []
    return sorted(
        [e.name for e in p.iterdir() if e.is_dir() and not e.name.startswith(".")],
        key=lambda n: (not n[0].isascii(), n),
    )


def build_body(domain: str, meta: dict, subs: list[str]) -> str:
    lines = [
        f"# {meta['title']}",
        "",
        f"> {meta['desc']}",
        "",
        "## 二级子目录",
        "",
    ]
    for s in subs:
        lines.append(f"- [[{domain}/{s}/README|{s}]]")
    lines.append("")
    lines.append("## 跨域导航")
    lines.append("")
    others = sorted(d for d in DOMAINS if d != domain)
    for o in others:
        lines.append(f"- [[{o}/README|{o}]]")
    lines.append("")
    return "\n".join(lines)


def split_frontmatter(text: str) -> tuple[str, str]:
    if not text.startswith("---"):
        return "", text
    end = text.find("\n---", 3)
    if end < 0:
        return "", text
    fm = text[: end + 4]
    body = text[end + 4:].lstrip("\n")
    return fm, body


def main() -> int:
    apply = "--apply" in sys.argv
    changed = 0
    for domain, meta in DOMAINS.items():
        subs = list_subdirs(domain)
        readme = WIKI / domain / "README.md"
        new_body = build_body(domain, meta, subs)
        if readme.exists():
            text = readme.read_text(encoding="utf-8")
            fm, old_body = split_frontmatter(text)
        else:
            fm = (
                "---\n"
                f"title: {meta['title']}\n"
                f"category: domain\n"
                f"tags:\n- domain\n"
                f"tier: core\n"
                "---\n"
            )
            old_body = ""
        new_text = (fm + "\n" + new_body).rstrip() + "\n"
        if new_text != (fm + "\n" + old_body if old_body else fm + "\n"):
            if apply:
                readme.write_text(new_text, encoding="utf-8")
                print(f"WRITE {domain}/README.md ({len(subs)} subs)")
            else:
                print(f"WOULD {domain}/README.md ({len(subs)} subs)")
            changed += 1
        else:
            print(f"ok    {domain}/README.md")
    print(f"total={len(DOMAINS)} changed={changed} apply={apply}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
