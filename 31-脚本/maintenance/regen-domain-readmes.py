#!/usr/bin/env python3
"""Regenerate the body section of each Chinese-domain README.md to reflect
the post-restructure 2nd-level directories. Frontmatter is preserved.

Usage: python3 scripts/maintenance/regen-domain-readmes.py [--apply]
Without --apply, only prints what would change.
"""
from __future__ import annotations

import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

DOMAINS = {
    "集群基础": {
        "title": "集群基础 Cluster Fundamentals",
        "desc": "Kubernetes 集群架构总览、设计原则、控制平面、API 版本、kubectl 与升级路径。",
        "tag": "domain/cluster-fundamentals",
    },
    "工作负载": {
        "title": "工作负载 Workloads",
        "desc": "Kubernetes 核心工作负载与 Java-on-K8s 等垂直栈实践。",
        "tag": "domain/workloads-applications",
    },
    "网络": {
        "title": "网络 Networking",
        "desc": "K8s 网络核心、基础协议、服务网格、API 网关、eBPF 与 Terway。",
        "tag": "domain/networking-traffic",
    },
    "存储": {
        "title": "存储 Storage",
        "desc": "K8s 存储原语、存储基础、分布式存储与有状态应用存储。",
        "tag": "domain/storage-data",
    },
    "安全": {
        "title": "安全 Security",
        "desc": "身份与访问、网络/运行时安全、策略治理、供应链与合规审计。",
        "tag": "domain/security-compliance",
    },
    "可观测性": {
        "title": "可观测性 Observability",
        "desc": "指标、日志、链路追踪、告警、SLO/SLI 与工具集。",
        "tag": "domain/observability",
    },
    "平台工程": {
        "title": "平台工程 Platform Engineering",
        "desc": "构建、运维、治理、开发体验与平台代码分析。",
        "tag": "domain/platform-engineering",
    },
    "发布变更": {
        "title": "发布变更 Release & Change",
        "desc": "GitOps、IaC、变更管理、测试质量、部署方案与迁移方案。",
        "tag": "domain/release-change-management",
    },
    "可靠性": {
        "title": "可靠性 Reliability",
        "desc": "备份恢复、灾难恢复、容量规划、混沌工程、事后复盘、SRE 实践与性能测试。",
        "tag": "domain/reliability-engineering",
    },
    "故障诊断": {
        "title": "故障诊断 Troubleshooting",
        "desc": "资源/基础设施/高级/核心排障、JVM/性能调优、FEBM 方法论、FTA 故障树、技能体系与多故障场景。",
        "tag": "domain/troubleshooting-diagnostics",
    },
    "生产运维": {
        "title": "生产运维 Production Ops",
        "desc": "成本治理、集群治理、事件响应、绿色计算、工单案例与回复话术。",
        "tag": "domain/production-operations",
    },
    "云厂商": {
        "title": "云厂商 Cloud Providers",
        "desc": "阿里云、AWS-EKS、Google-GKE、Azure-AKS、腾讯云 TKE、华为云 CCE、多云混合与其他云。",
        "tag": "domain/cloud-providers",
    },
    "容器运行时": {
        "title": "容器运行时 Container Runtime",
        "desc": "Docker、containerd/CRI-O、镜像管理、镜像构建与运行时迁移。",
        "tag": "domain/container-runtime",
    },
    "AI基础设施": {
        "title": "AI 基础设施 AI Infra",
        "desc": "AI 基础设施、AI-Agents、Agent 运行时与 AI 编码。",
        "tag": "domain/ai-ml-infra",
    },
    "专项技术": {
        "title": "专项技术 Specialized",
        "desc": "边缘计算、WebAssembly、扩展机制与无服务器。",
        "tag": "domain/specialized-tech",
    },
    "数据库中间件": {
        "title": "数据库中间件 Database & Middleware",
        "desc": "数据库、缓存、消息队列、时序数据库、Operator 管理与数据流。",
        "tag": "domain/database-middleware",
    },
    "系统基础": {
        "title": "系统基础 System Foundation",
        "desc": "Linux、硬件、K8s 事件、速查卡与知识字典。",
        "tag": "domain/system-foundation",
    },
    "清单模式": {
        "title": "清单模式 Manifests & Patterns",
        "desc": "YAML 参考、Kustomize 模式与 Helm 值模式。",
        "tag": "domain/manifests-patterns",
    },
    "生态参考": {
        "title": "生态参考 Ecosystem",
        "desc": "CNCF 全景、论文与领域索引。",
        "tag": "domain/landscape-references",
    },
    "应用模式": {
        "title": "应用模式 Application Patterns",
        "desc": "子模式、行业架构与生产模式。",
        "tag": "domain/application-patterns",
    },
}


def list_subdirs(domain: str) -> list[str]:
    p = REPO / domain
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
        readme = REPO / domain / "README.md"
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
