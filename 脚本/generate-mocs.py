#!/usr/bin/env python3
"""
KUDIG-DATABASE MOC (Maps of Content) 批量生成脚本
为所有 domain/ 和 topic/ 目录生成 MOC.md 导航页，并在根目录生成 Global MOC。

输出:
  - domain-N-*/MOC.md  (每个 domain 一个)
  - topic-*/MOC.md     (每个 topic 一个)
  - MOC.md             (根目录 Global MOC)
"""

import re
import yaml
from pathlib import Path
from datetime import date
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
TODAY = date.today().isoformat()

# 排除目录
EXCLUDE_DIRS = {'.git', '.venv', '.ruff_cache', 'site', 'node_modules',
                '.obsidian', '.zread', '.claude', '.codebuddy', '.comate',
                '.github', '.understand-anything', 'reports', 'scripts',
                'templates', 'docs', 'prompts', 'topic-qa-corpus',
                'topic-index'}

# 知识域概述（简要描述）
DOMAIN_OVERVIEWS = {
    "domain-1-architecture-fundamentals": "Kubernetes 架构基础 — 系统整体设计、核心组件、API 版本、源码结构、集群部署",
    "domain-2-design-principles": "Kubernetes 设计原则 — API 设计理念、声明式 API、控制器模式、渐进式交付",
    "domain-3-control-plane": "控制平面 — etcd、apiserver、scheduler、controller-manager 深度解析",
    "domain-4-workloads": "工作负载 — Pod、Deployment、StatefulSet、DaemonSet、Job、CronJob",
    "domain-5-networking": "网络 — Service、Ingress、CNI、网络策略、DNS、负载均衡",
    "domain-6-storage": "存储 — PV、PVC、StorageClass、CSI、持久化存储",
    "domain-7-security": "安全 — RBAC、NetworkPolicy、PodSecurity、Secret、证书管理",
    "domain-8-observability": "可观测性 — Prometheus、Grafana、指标、日志、追踪",
    "domain-9-platform-ops": "平台运维 — 集群管理、资源管理、调度策略、运维自动化",
    "domain-10-extensions": "扩展 — CRD、Operator、Webhook、API Aggregation",
    "domain-11-ai-infra": "AI 基础设施 — GPU 调度、CUDA、Model Serving、LLM 部署",
    "domain-12-troubleshooting": "故障排查 — 通用方法论、常见故障模式、诊断工具链",
    "domain-13-docker": "Docker — 容器运行时、镜像构建、Docker Compose、最佳实践",
    "domain-14-linux": "Linux 基础 — 系统管理、网络配置、性能调优、安全加固",
    "domain-15-network-fundamentals": "网络基础 — TCP/IP、HTTP、DNS、负载均衡原理",
    "domain-16-storage-fundamentals": "存储基础 — 文件系统、块存储、对象存储原理",
    "domain-17-cloud-provider": "云提供商 — AWS、GCP、Azure、阿里云集成",
    "domain-18-production-operations": "生产运维 — 生产最佳实践、容量规划、变更管理",
    "domain-19-papers": "论文阅读 — Kubernetes 相关学术论文和技术报告",
    "domain-20-enterprise-monitoring-alerting": "企业监控告警 — 监控架构、告警策略、SLO/SLI",
    "domain-21-logging-management-analytics": "日志管理与分析 — 日志采集、存储、分析、可视化",
    "domain-22-container-image-management": "容器镜像管理 — 镜像构建、安全扫描、分发",
    "domain-23-gitops-ci-cd": "GitOps 与 CI/CD — ArgoCD、Flux、Jenkins、GitHub Actions",
    "domain-24-infrastructure-as-code": "基础设施即代码 — Terraform、Pulumi、Crossplane",
    "domain-25-cloud-native-security": "云原生安全 — 供应链安全、运行时安全、合规",
    "domain-26-service-mesh-microservices": "Service Mesh 与微服务 — Istio、Envoy、微服务架构",
    "domain-27-multi-cloud-hybrid": "多云与混合云 — 多云架构、混合云网络、数据同步",
    "domain-28-enterprise-database-middleware": "企业数据库中间件 — MySQL、PostgreSQL、Redis on K8s",
    "domain-29-automated-testing-quality": "自动化测试与质量 — 单元测试、集成测试、e2e 测试",
    "domain-30-disaster-recovery-business-continuity": "灾备与业务连续性 — 备份、恢复、容灾演练",
    "domain-31-hardware": "硬件 — 服务器、网络硬件、存储硬件",
    "domain-32-yaml-manifests": "YAML 清单 — 资源清单编写规范、最佳实践",
    "domain-33-kubernetes-events": "Kubernetes 事件 — 事件模型、事件驱动、事件分析",
    "domain-34-cncf-landscape": "CNCF 全景 — CNCF 项目生态、成熟度模型",
    "domain-35-ebpf-technology": "eBPF 技术 — eBPF 原理、Cilium、网络/安全可观测性",
    "domain-36-platform-engineering": "平台工程 — 内部开发者平台、IDP、Backstage",
    "domain-37-edge-computing": "边缘计算 — KubeEdge、边缘集群、边缘 AI",
    "domain-38-webassembly-cloud-native": "WebAssembly 云原生 — Wasm、WASI、WasmEdge",
    "domain-39-supply-chain-security": "供应链安全 — SBOM、签名、验证、镜像安全",
    "domain-40-cloud-native-api-gateway": "云原生 API 网关 — Higress、Envoy Gateway、Kong",
}

TOPIC_OVERVIEWS = {
    "topic-ai-agent": "AI Agent — AI 智能体架构、工具调用、Agent 工作流",
    "topic-ai-coding": "AI 编程 — AI 辅助编程工具、最佳实践",
    "topic-application-architecture": "应用架构 — 云原生应用设计模式、架构决策",
    "topic-cheat-sheet": "速查卡 — 常用命令、配置、模板速查",
    "topic-deployment": "部署 — 部署策略、发布模式、滚动更新",
    "topic-dictionary": "运维术语词典 — K8s 运维专业术语解释",
    "topic-febm": "FEBM 取证 — 故障事件取证方法文档",
    "topic-fta": "FTA 故障树 — 故障树分析文档集合",
    "topic-functions": "函数 — 运维脚本常用函数库",
    "topic-index": "深度研究入口 — 语料库索引与向量检索",
    "domain-java-kubernetes": "Java on Kubernetes — Java 应用部署与调优",
    "topic-learn": "学习计划 — 系统学习路径与考核",
    "topic-migration": "迁移 — 数据迁移、应用迁移、版本升级",
    "topic-presentations": "演示文稿 — 技术分享与培训 PPT",
    "topic-publish": "发布 — 内容发布流程与规范",
    "topic-qa-corpus": "QA 语料库 — Agent 评测问答对",
    "topic-release-notes": "版本发布说明 — Kubernetes 各版本变更",
    "topic-skills": "操作技能 — 场景化运维操作卡片",
    "topic-structural-trouble-shooting": "结构化故障排查 — 系统性排障方法论",
    "topic-terway": "Terway — 阿里云 CNI 插件深度解析",
}


def parse_frontmatter(content: str) -> dict:
    """Parse YAML frontmatter from markdown content."""
    content = content.lstrip()
    if not content.startswith("---"):
        return {}
    end = content.find("---", 3)
    if end == -1:
        return {}
    try:
        fm = yaml.safe_load(content[3:end].strip())
        return fm if fm else {}
    except Exception:
        return {}


def get_doc_info(filepath: Path, base_dir: Path) -> dict:
    """Extract metadata from a single markdown file."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return {"title": filepath.stem, "difficulty": "", "tags": [],
                "read_time": "", "path": str(filepath.relative_to(base_dir))}

    fm = parse_frontmatter(content)
    rel = str(filepath.relative_to(base_dir))

    # Extract first H1/H2 heading if no title
    title = fm.get("title", filepath.stem)
    heading_match = re.search(r'^#{1,2}\s+(.+?)(?:\s*\{.*\})?$', content, re.MULTILINE)
    if heading_match and title == filepath.stem:
        title = re.sub(r'[\U0001f300-\U0001f9ff]', '', heading_match.group(1)).strip()

    return {
        "title": title,
        "difficulty": fm.get("difficulty", ""),
        "tags": fm.get("tags", []),
        "read_time": fm.get("estimated_read_time", ""),
        "path": rel,
    }


def difficulty_label(diff: str) -> str:
    """Map difficulty to emoji label."""
    mapping = {
        "beginner": "入门",
        "intermediate": "进阶",
        "advanced": "高级",
        "expert": "专家",
    }
    return mapping.get(diff, diff) if diff else ""


def generate_domain_moc(domain_dir: Path) -> str:
    """Generate MOC.md content for a domain directory."""
    domain_name = domain_dir.name
    overview = DOMAIN_OVERVIEWS.get(domain_name, f"{domain_name} 知识域")

    # Collect all .md files (exclude README.md and MOC.md itself)
    md_files = sorted(domain_dir.glob("*.md"))
    docs = []
    for f in md_files:
        if f.name in ("README.md", "MOC.md"):
            continue
        info = get_doc_info(f, BASE_DIR)
        docs.append(info)

    doc_count = len(docs)
    if doc_count == 0:
        return ""

    # Difficulty distribution
    diff_counts = defaultdict(int)
    for d in docs:
        diff_counts[d["difficulty"]] += 1

    # Build document table
    table_lines = []
    for i, doc in enumerate(docs, 1):
        diff_str = difficulty_label(doc["difficulty"])
        tags_str = ", ".join([t for t in doc["tags"][:3] if t])
        table_lines.append(
            f"| {i} | [[{doc['path']}|{doc['title']}]] "
            f"| {diff_str} | {tags_str} | {doc['read_time']} |"
        )
    doc_table = "\n".join(table_lines)

    # Mermaid subgraph with first 6 docs
    core_docs = [d["title"] for d in docs[:6]]
    mermaid_nodes = "\n    ".join(
        f'{chr(65 + i)}["{t}"]' for i, t in enumerate(core_docs)
    )
    mermaid_links = "\n    ".join(
        f"A --> {chr(65 + i)}" for i in range(1, min(len(core_docs), 6))
    )

    # Determine primary tag
    all_tags = defaultdict(int)
    for d in docs:
        for t in d["tags"]:
            all_tags[t] += 1
    primary_tag = max(all_tags, key=all_tags.get) if all_tags else "k8s"

    return f"""---
title: "{domain_name} MOC"
description: "{domain_name} 知识域导航页，覆盖 {doc_count} 篇文档"
category: moc
tags: [k8s, moc, {primary_tag}]
moc_scope: "{domain_name}"
moc_type: "domain"
moc_coverage:
  total_docs: {doc_count}
last_updated: "{TODAY}"
---

# {domain_name} MOC

> **MOC 版本**: 1.0
> **知识域**: {domain_name}
> **文档数量**: {doc_count} 篇
> **最后更新**: {TODAY}
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

{overview}

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | {domain_name} |
| **文档数量** | {doc_count} 篇 |
| **难度分布** | 入门 {diff_counts.get('beginner', 0)} / 进阶 {diff_counts.get('intermediate', 0)} / 高级 {diff_counts.get('advanced', 0)} / 专家 {diff_counts.get('expert', 0)} |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
{doc_table}

---

## 知识图谱

```mermaid
graph TD
    subgraph {domain_name}
        {mermaid_nodes}
    end

    {mermaid_links}

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|
| [[../topic-fta/MOC.md|FTA 故障树]] | {domain_name} 相关故障树分析 |
| [[../topic-skills/MOC.md|Skills 技能]] | {domain_name} 相关操作技能 |
| [[../topic-index/README.md|深度研究入口]] | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | {doc_count} |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 {TODAY}。*
"""


def generate_topic_moc(topic_dir: Path) -> str:
    """Generate MOC.md content for a topic directory."""
    topic_name = topic_dir.name
    overview = TOPIC_OVERVIEWS.get(topic_name, f"{topic_name} 专题")

    # Collect .md files recursively for topics (they may have subdirs)
    md_files = sorted(topic_dir.rglob("*.md"))
    docs = []
    for f in md_files:
        if f.name in ("README.md", "MOC.md"):
            continue
        # Skip deeply nested generated files
        info = get_doc_info(f, BASE_DIR)
        docs.append(info)

    doc_count = len(docs)
    if doc_count == 0:
        return ""

    # Difficulty distribution
    diff_counts = defaultdict(int)
    for d in docs:
        diff_counts[d["difficulty"]] += 1

    # Build document table (limit to first 50 for readability)
    table_lines = []
    for i, doc in enumerate(docs[:50], 1):
        diff_str = difficulty_label(doc["difficulty"])
        tags_str = ", ".join([t for t in doc["tags"][:3] if t])
        table_lines.append(
            f"| {i} | [[{doc['path']}|{doc['title']}]] "
            f"| {diff_str} | {tags_str} | {doc['read_time']} |"
        )
    if len(docs) > 50:
        table_lines.append(f"| ... | 共 {len(docs)} 篇文档 | | | |")
    doc_table = "\n".join(table_lines)

    # Determine primary tag
    all_tags = defaultdict(int)
    for d in docs:
        for t in d["tags"]:
            all_tags[t] += 1
    primary_tag = max(all_tags, key=all_tags.get) if all_tags else "topic"

    truncated = len(docs) > 50
    return f"""---
title: "{topic_name} MOC"
description: "{topic_name} 专题导航页，覆盖 {doc_count} 篇文档"
category: moc
tags: [k8s, moc, {primary_tag}]
moc_scope: "{topic_name}"
moc_type: "topic"
moc_coverage:
  total_docs: {doc_count}
last_updated: "{TODAY}"
---

# {topic_name} MOC

> **MOC 版本**: 1.0
> **专题**: {topic_name}
> **文档数量**: {doc_count} 篇
> **最后更新**: {TODAY}
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

{overview}

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | {topic_name} |
| **文档数量** | {doc_count} 篇{"（展示前 50 篇）" if truncated else ""} |
| **难度分布** | 入门 {diff_counts.get('beginner', 0)} / 进阶 {diff_counts.get('intermediate', 0)} / 高级 {diff_counts.get('advanced', 0)} / 专家 {diff_counts.get('expert', 0)} |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
{doc_table}

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | {doc_count} |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 {TODAY}。*
"""


def generate_global_moc(domain_mocs: list, topic_mocs: list) -> str:
    """Generate the root Global MOC."""
    domain_rows = []
    for d in sorted(domain_mocs, key=lambda x: x["name"]):
        domain_rows.append(
            f"| [[{d['dir']}/MOC.md|{d['name']} MOC]] | {d['overview']} | {d['count']} |"
        )

    topic_rows = []
    for t in sorted(topic_mocs, key=lambda x: x["name"]):
        topic_rows.append(
            f"| [[{t['dir']}/MOC.md|{t['name']} MOC]] | {t['overview']} | {t['count']} |"
        )

    total_docs = sum(d["count"] for d in domain_mocs) + sum(t["count"] for t in topic_mocs)

    return f"""---
title: "KUDIG Database — Global MOC"
title_en: "KUDIG Database — Global Map of Content"
description: "Kubernetes 生产运维全域知识库导航，40 个知识域 + 21 个专题，共 {total_docs}+ 篇文档"
category: moc
tags: [k8s, moc, global, navigation, knowledge-graph]
moc_scope: "global"
moc_type: "global"
last_updated: "{TODAY}"
---

# KUDIG Database — Global MOC

> **Kubernetes Production Operations Knowledge Base**
> **全局导航**: 40 个知识域 + 21 个专题
> **文档总量**: {total_docs}+ 篇
> **最后更新**: {TODAY}

---

<div align="center">

<pre align="center">
╔══════════════════════════════════════════════════════════════════════════╗
║   KUDIG — Global Map of Content (MOC)                                   ║
║   40 Domains  │  21 Topics  │  {total_docs}+ Documents                          ║
╚══════════════════════════════════════════════════════════════════════════╝
</pre>

</div>

---

## 知识域导航 (40 Domains)

| MOC | 概述 | 文档数 |
|---|---|---|
{chr(10).join(domain_rows)}

---

## 专题导航 (21 Topics)

| MOC | 概述 | 文档数 |
|---|---|---|
{chr(10).join(topic_rows)}

---

## 快速入口

| 入口 | 说明 |
|---|---|
| [[topic-fta/MOC.md|FTA 故障树]] | 67+ 篇故障树分析文档 |
| [[topic-skills/MOC.md|Skills 技能]] | 34+ 篇操作技能卡片 |
| [[topic-cheat-sheet/MOC.md|速查卡]] | 9 张速查卡 |
| [[topic-index/README.md|深度研究入口]] | 语料库索引与向量检索 |
| [[topic-learn/MOC.md|学习计划]] | 系统学习路径 |
| [[topic-release-notes/MOC.md|版本发布说明]] | Kubernetes 版本变更历史 |
| [[topic-dictionary/MOC.md|运维术语词典]] | 运维专业术语解释 |

---

## 知识图谱概览

```mermaid
graph TD
    subgraph 控制平面
        A[domain-3-control-plane]
    end
    subgraph 工作负载
        B[domain-4-workloads]
    end
    subgraph 网络
        C[domain-5-networking]
    end
    subgraph 存储
        D[domain-6-storage]
    end
    subgraph 安全
        E[domain-7-security]
    end
    subgraph 可观测性
        F[domain-8-observability]
    end
    subgraph 故障排查
        G[domain-12-troubleshooting]
    end
    subgraph AI 基础设施
        H[domain-11-ai-infra]
    end

    A --> B
    A --> C
    B --> D
    C --> E
    D --> E
    E --> F
    F --> G
    H --> B

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
    style C fill:#f59e0b,stroke:#b45309,color:#fff
    style D fill:#a855f7,stroke:#6b21a8,color:#fff
    style E fill:#ef4444,stroke:#b91c1c,color:#fff
    style F fill:#06b6d4,stroke:#0891b2,color:#fff
    style G fill:#f97316,stroke:#c2410c,color:#fff
    style H fill:#8b5cf6,stroke:#6d28d9,color:#fff
```

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 {TODAY}。*
"""


def main():
    domain_mocs = []
    topic_mocs = []

    # Generate Domain MOCs
    print("=" * 60)
    print("Generating Domain MOCs...")
    print("=" * 60)
    for domain_dir in sorted(BASE_DIR.glob("domain-*")):
        if not domain_dir.is_dir():
            continue
        content = generate_domain_moc(domain_dir)
        if not content:
            print(f"  SKIP {domain_dir.name} (no docs)")
            continue
        moc_path = domain_dir / "MOC.md"
        moc_path.write_text(content, encoding="utf-8")
        md_files = [f for f in domain_dir.glob("*.md") if f.name not in ("README.md", "MOC.md")]
        domain_mocs.append({
            "name": domain_dir.name,
            "dir": domain_dir.name,
            "overview": DOMAIN_OVERVIEWS.get(domain_dir.name, ""),
            "count": len(md_files),
        })
        print(f"  OK  {domain_dir.name} ({len(md_files)} docs) -> {moc_path}")

    # Generate Topic MOCs
    print()
    print("=" * 60)
    print("Generating Topic MOCs...")
    print("=" * 60)
    for topic_dir in sorted(BASE_DIR.glob("topic-*")):
        if not topic_dir.is_dir():
            continue
        content = generate_topic_moc(topic_dir)
        if not content:
            print(f"  SKIP {topic_dir.name} (no docs)")
            continue
        moc_path = topic_dir / "MOC.md"
        moc_path.write_text(content, encoding="utf-8")
        md_files = [f for f in topic_dir.rglob("*.md") if f.name not in ("README.md", "MOC.md")]
        topic_mocs.append({
            "name": topic_dir.name,
            "dir": topic_dir.name,
            "overview": TOPIC_OVERVIEWS.get(topic_dir.name, ""),
            "count": len(md_files),
        })
        print(f"  OK  {topic_dir.name} ({len(md_files)} docs) -> {moc_path}")

    # Generate Global MOC
    print()
    print("=" * 60)
    print("Generating Global MOC...")
    print("=" * 60)
    global_moc = generate_global_moc(domain_mocs, topic_mocs)
    global_moc_path = BASE_DIR / "MOC.md"
    global_moc_path.write_text(global_moc, encoding="utf-8")
    print(f"  OK  Global MOC -> {global_moc_path}")

    # Summary
    print()
    print("=" * 60)
    print("MOC Generation Complete")
    print(f"  Domain MOCs: {len(domain_mocs)}")
    print(f"  Topic MOCs:  {len(topic_mocs)}")
    print(f"  Global MOC:  1")
    print(f"  Total:       {len(domain_mocs) + len(topic_mocs) + 1}")
    print(f"  Total docs covered: {sum(d['count'] for d in domain_mocs) + sum(t['count'] for t in topic_mocs)}")
    print("=" * 60)


if __name__ == "__main__":
    main()
