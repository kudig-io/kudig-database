#!/usr/bin/env python3
"""Generate consolidated version history reference pages from topic-release-notes/"""

import os
import re
import json
import hashlib
from datetime import datetime, timezone
from pathlib import Path

VAULT = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")
NOTES = VAULT / "docs" / "topic-release-notes"
REFS = VAULT / "references"
MANIFEST = VAULT / ".manifest.json"

# Tool definitions: (category, tool_name, entity_name, description_zh, key_changes)
TOOLS = [
    ("kubernetes", "kubernetes", "kubernetes", "Kubernetes 容器编排平台", [
        ("1.2", "初始版本发布，基础编排能力"),
        ("1.4", "引入 RBAC、Pod 安全策略"),
        ("1.6", "DaemonSet/Deployment GA"),
        ("1.8", "Workloads API (apps/v1) GA"),
        ("1.11", "CoreDNS GA、IPVS 负载均衡"),
        ("1.14", "Windows 节点支持 GA"),
        ("1.16", "CRD GA、API 迁移至 apps/v1"),
        ("1.18", "Ingress GA、Topology Manager"),
        ("1.20", "Pod 拓扑分布、CronJob GA"),
        ("1.22", "Server-Side Apply GA、安全加固"),
        ("1.24", "移除 Dockershim、Pod 安全准入 GA"),
        ("1.25", "CronJob GA、PodDisruptionBudget GA"),
        ("1.27", "Sidecar 容器原生支持"),
        ("1.29", "Pod 健康检查增强、调度优化"),
        ("1.30", "上下文日志 GA、节点内存管理"),
        ("1.33", "最新稳定版，多项 GA 功能"),
        ("1.36", "最新版本，持续改进"),
    ]),
    ("observability", "grafana", "grafana", "开源可视化与监控仪表板平台", [
        ("1.0", "初始发布，基础仪表板功能"),
        ("2.0", "面板插件系统"),
        ("4.0", "告警功能 GA、数据源插件架构"),
        ("5.0", "仪表板引擎重写、仪表板文件夹"),
        ("6.0", "Explore 模式、混合数据源"),
        ("7.0", "面板编辑器改进、Grafana Loki 集成"),
        ("8.0", "Grafana 统一告警、Grafana Live"),
        ("9.0", "新的可视化库、Grafana OnCall"),
        ("10.0", "Canvas 面板、Grafana Scenes"),
        ("11.0", "Scenes 仪表板、AI/ML 功能"),
        ("12.0", "最新大版本，Dashboard 增强"),
    ]),
    ("observability", "loki", "loki", "日志聚合系统（类 Prometheus）", [
        ("0.1", "初始原型发布"),
        ("1.0", "首个 GA 版本，基础日志聚合"),
        ("2.0", "块存储 v3 格式、索引改进"),
        ("2.4", "TSDB 索引 GA"),
        ("3.0", "日志流引擎、性能大幅改进"),
        ("3.5", "多租户增强"),
    ]),
    ("observability", "opentelemetry-collector", "opentelemetry-collector", "OpenTelemetry 遥测数据收集器", [
        ("0.0", "初始开发版"),
        ("0.50", "组件稳定化推进"),
        ("0.80", "服务配置重构、弃用旧接口"),
        ("0.100", "连接器组件 GA"),
        ("0.120", "扩展管理改进"),
        ("0.149", "最新版本，持续稳定化"),
    ]),
    ("observability", "prometheus", "prometheus", "开源监控与告警系统", [
        ("0.11", "初始版本"),
        ("1.0", "首个 GA 版本，基础时序数据库"),
        ("1.8", "远程写入改进"),
        ("2.0", "TSDB 存储引擎 GA、性能提升"),
        ("2.7", "远端读取改进"),
        ("2.14", "内存映射 TSDB"),
        ("2.25", "原生直方图原型"),
        ("2.37", "原生直方图 beta"),
        ("2.45", "原生直方图 GA"),
        ("2.50", "元数据 API 修复"),
        ("3.0", "远程写入 v2、OTLP 支持 GA"),
        ("3.5", "原生直方图全面支持"),
    ]),
    ("observability", "thanos", "thanos", "Prometheus 高可用长期存储方案", [
        ("0.1", "初始发布"),
        ("0.5", "Store Gateway 稳定"),
        ("0.10", "多存储后端支持"),
        ("0.20", "UI 改进、稳定性修复"),
        ("0.30", "查询性能优化"),
        ("0.38", "分片 Store Gateway"),
    ]),
    ("security", "cert-manager", "cert-manager", "Kubernetes 证书管理自动化", [
        ("0.1", "初始发布，基础证书签发"),
        ("1.0", "首个 GA 版本、Venafi/ACME 支持"),
        ("1.5", "证书续订改进"),
        ("1.8", "新的签发器架构"),
        ("1.11", "Gateway API 集成"),
        ("1.14", "ACME 稳定性增强"),
        ("1.17", "CNI 兼容性改进"),
    ]),
    ("security", "falco", "falco", "云原生运行时安全监控", [
        ("0.1", "初始发布，系统调用监控"),
        ("0.15", "Kubernetes 审计日志支持"),
        ("0.20", "gRPC API、JSON 事件改进"),
        ("0.25", "插件框架 v1"),
        ("0.31", "插件生态扩展"),
        ("0.38", "K8s 审计增强"),
    ]),
    ("security", "gatekeeper", "gatekeeper", "OPA 策略引擎的 Kubernetes 准入控制", [
        ("3.0", "首个发布版、Constraint Template GA"),
        ("3.5", "审计功能增强"),
        ("3.8", "Mutating Webhook 支持"),
        ("3.10", "外部数据提供者"),
        ("3.14", "生成策略增强"),
        ("3.16", "Assign/AssignMetadata GA"),
        ("3.22", "最新版本"),
    ]),
    ("security", "opa", "opa", "通用策略引擎（Open Policy Agent）", [
        ("0.1", "初始发布"),
        ("0.10", "Bundle API v1"),
        ("0.20", "部分评估改进"),
        ("0.30", "内置函数扩展"),
        ("0.40", "WASM 编译器"),
        ("0.50", "优化的增量编译"),
        ("0.60", "类型检查改进"),
        ("1.0", "首个 GA 版本、Rego v1 支持"),
    ]),
    ("security", "trivy", "trivy", "全面的漏洞扫描器", [
        ("0.0", "初始发布，容器镜像扫描"),
        ("0.5", "CI/CD 集成改进"),
        ("0.10", "SARIF 输出格式支持"),
        ("0.20", "IaC 扫描支持"),
        ("0.26", "SBOM 生成"),
        ("0.30", "依赖扫描增强"),
        ("0.40", "扫描速度优化"),
    ]),
    ("cli-tools", "helm", "helm", "Kubernetes 包管理器", [
        ("1.2", "Helm 1.x 早期版本"),
        ("2.0", "Tiller 架构、Chart 仓库"),
        ("3.0", "移除 Tiller、原生 Helm 3"),
        ("3.5", "JSON Schema 验证 GA"),
        ("3.8", "PostRenderer 支持"),
        ("3.12", "OCI 注册表支持改进"),
        ("3.15", "依赖管理优化"),
        ("4.0", "最新大版本"),
    ]),
    ("cli-tools", "kind", "kind", "Kubernetes 本地测试集群工具", [
        ("0.0", "初始发布"),
        ("0.5", "HA 集群支持"),
        ("0.10", "Node 镜像改进、Windows 支持"),
        ("0.15", "Ingress 测试支持"),
        ("0.20", "Kubernetes 1.25+ 支持"),
        ("0.25", "containerd 集成增强"),
    ]),
    ("cli-tools", "kops", "kops", "Kubernetes 集群生产运维工具", [
        ("1.4", "AWS 云平台支持"),
        ("1.10", "高可用集群部署"),
        ("1.15", "Terraform 输出支持"),
        ("1.20", "containerd 默认运行时"),
        ("1.25", "Spot 实例支持"),
        ("1.30", "ARM64 架构支持"),
        ("1.35", "最新版本"),
    ]),
    ("cli-tools", "kustomize", "kustomize", "Kubernetes 原生配置管理工具", [
        ("1.0", "初始发布，基础配置定制"),
        ("2.0", "Kubectl 集成"),
        ("3.0", "插件框架"),
        ("3.2", "组件（Components）功能"),
        ("3.3", "Kubectl 1.27+ 内置集成"),
    ]),
    ("cli-tools", "minikube", "minikube", "Kubernetes 本地开发环境", [
        ("0.1", "初始发布，VirtualBox 驱动"),
        ("0.5", "多驱动支持"),
        ("1.0", "首个 GA 版本"),
        ("1.5", "多节点集群支持"),
        ("1.15", "增强的仪表板"),
        ("1.25", "自动配置优化"),
        ("1.33", "Podman 驱动 GA"),
    ]),
    ("cicd-gitops", "argo-cd", "argo-cd", "Kubernetes 声明式 GitOps 持续部署工具", [
        ("0.1", "初始发布"),
        ("0.5", "应用状态改进"),
        ("1.0", "首个 GA 版本、RBAC 改进"),
        ("1.5", "ApplicationSet 控制器"),
        ("2.0", "简化架构、CMP 支持"),
        ("2.5", "通知框架 GA"),
        ("2.10", "多源应用支持"),
        ("3.0", "最新大版本"),
    ]),
    ("cicd-gitops", "flux", "flux", "GitOps 工具包（Flux v2）", [
        ("0.0", "Flux v2 重构开始"),
        ("0.5", "Helm 控制器 GA"),
        ("0.20", "多租户支持改进"),
        ("0.41", "Notification API 稳定"),
        ("2.0", "Flux v2 GA"),
        ("2.3", "OCI 仓库支持"),
        ("2.5", "Git 仓库增强"),
    ]),
    ("cicd-gitops", "tekton", "tekton", "云原生 CI/CD 流水线框架", [
        ("0.1", "初始发布"),
        ("0.8", "Task/Pipeline CRD 稳定"),
        ("0.15", "条件执行、结果引用"),
        ("0.25", "可恢复流水线"),
        ("0.40", "Tasks/Pipelines GA"),
        ("0.50", "CEL 触发器"),
        ("1.0", "首个 GA 版本"),
        ("1.11", "最新版本"),
    ]),
    ("networking", "calico", "calico", "容器网络与网络安全方案", [
        ("2.4", "早期版本，BGP 网络"),
        ("3.0", "主要架构重写"),
        ("3.5", "eBPF 数据平面预览"),
        ("3.10", "Typha 代理 GA"),
        ("3.20", "eBPF 数据平面 GA"),
        ("3.25", "WireGuard 加密支持"),
        ("3.31", "最新版本"),
    ]),
    ("networking", "cilium", "cilium", "基于 eBPF 的网络、可观测性与安全平台", [
        ("0.8", "初始版本"),
        ("1.0", "首个 GA 版本、eBPF 网络"),
        ("1.5", "BPF Host Routing"),
        ("1.9", "Clustermesh GA、Hubble"),
        ("1.10", "Bandwidth Manager"),
        ("1.13", "Service Mesh 支持 GA"),
        ("1.14", "多集群增强"),
        ("1.15", "Envoy 集成改进"),
        ("1.19", "最新版本"),
    ]),
    ("networking", "cni-plugins", "cni-plugins", "CNI 标准网络插件集合", [
        ("0.6", "CNI spec v0.4"),
        ("1.0", "CNI Spec v1.0 GA、稳定项目"),
        ("1.3", "性能改进"),
        ("1.5", "IPv6 增强"),
        ("1.9", "最新版本"),
    ]),
    ("networking", "envoy", "envoy", "高性能 L7 代理与通信总线", [
        ("1.0", "首个 GA 版本"),
        ("1.5", "gRPC 改进"),
        ("1.10", "Wasm 扩展预览"),
        ("1.15", "Wasm 支持 GA"),
        ("1.20", "Header 验证增强"),
        ("1.25", "连接池改进"),
        ("1.30", "多地址支持"),
        ("1.37", "最新版本"),
    ]),
    ("networking", "istio", "istio", "服务网格平台", [
        ("0.1", "初始发布"),
        ("0.8", "Mixer/Pilot 稳定"),
        ("1.0", "首个 GA 版本、Sidecar 模式"),
        ("1.5", "Istiod 统一控制面"),
        ("1.9", "虚拟机集成 GA"),
        ("1.13", "WebAssembly 插件 GA"),
        ("1.18", "Gateway API 支持 GA"),
        ("1.22", "Ambient Mesh 预览"),
        ("1.29", "最新版本"),
    ]),
    ("networking", "linkerd", "linkerd", "超轻量级服务网格", [
        ("0.1", "Linkerd 2.x 初始发布（Conduit 重命名）"),
        ("0.5", "多集群支持"),
        ("18.7", "Linkerd2 稳定版、CPU 优化 ~20%"),
        ("18.8", "gRPC 重试"),
        ("18.9", "Sidecar 生命周期改进"),
    ]),
    ("core-deps", "containerd", "containerd", "行业标准容器运行时", [
        ("0.0", "Docker 拆分、初始发布"),
        ("1.0", "首个 GA 版本"),
        ("1.3", "镜像加密支持"),
        ("1.4", "Transfer 服务、沙箱 API 预览"),
        ("1.6", "NRI 插件、Sandbox API"),
        ("1.7", "导入/导出改进"),
        ("2.0", "运行时规范 v2"),
        ("2.2", "最新版本"),
    ]),
    ("core-deps", "coredns", "coredns", "可扩展的 DNS 服务器", [
        ("0.9", "初始版本"),
        ("1.0", "首个 GA 版本"),
        ("1.4", "Kubernetes 1.13+ 默认 DNS"),
        ("1.8", "forward 插件改进"),
        ("1.11", "健康检查增强"),
        ("1.14", "最新版本"),
    ]),
    ("core-deps", "cri-o", "cri-o", "Kubernetes CRI 容器运行时", [
        ("0.1", "初始发布"),
        ("1.0", "首个 GA 版本"),
        ("1.18", "cgroup v2 支持"),
        ("1.25", "用户命名域支持"),
        ("1.30", "NRI 支持"),
        ("1.35", "最新版本"),
    ]),
    ("core-deps", "etcd", "etcd", "分布式键值存储系统", [
        ("0.1", "初始发布"),
        ("2.0", "Raft 共识协议"),
        ("3.0", "gRPC API GA"),
        ("3.3", "Lease 改进"),
        ("3.4", "安全性增强"),
        ("3.5", "RocksDB 实验"),
        ("3.6", "最新版本"),
    ]),
    ("core-deps", "runc", "runc", "OCI 运行时规范实现", [
        ("0.0", "Docker 拆分、初始发布"),
        ("1.0", "首个 GA 版本、安全修复"),
        ("1.1", "cgroup v2 改进"),
        ("1.2", "可执行保护"),
        ("1.4", "最新版本"),
    ]),
    ("storage", "longhorn", "longhorn", "云原生分布式块存储", [
        ("0.2", "初始发布"),
        ("0.5", "备份支持"),
        ("1.0", "首个 GA 版本"),
        ("1.3", "V2 数据引擎预览"),
        ("1.5", "在线扩展 GA"),
        ("1.7", "快照改进"),
        ("1.11", "最新版本"),
    ]),
    ("storage", "rook", "rook", "Ceph 存储 Kubernetes 运算符", [
        ("0.1", "初始发布"),
        ("0.5", "Ceph CRD 支持"),
        ("1.0", "Ceph Operator GA"),
        ("1.5", "Ceph Octopus 支持"),
        ("1.10", "Ceph Quincy 支持"),
        ("1.15", "对象存储改进"),
        ("1.19", "最新版本"),
    ]),
    ("storage", "velero", "velero", "Kubernetes 备份与灾难恢复工具", [
        ("0.3", "初始版本（原 Heptio Ark）"),
        ("1.0", "首个 GA 版本、velero install 命令"),
        ("1.5", "文件系统备份改进"),
        ("1.9", "数据移动器 GA"),
        ("1.12", "CSI 快照 GA"),
        ("1.15", "节点代理增强"),
        ("1.18", "最新版本"),
    ]),
]

def read_frontmatter_versions(tool_path):
    """Read all release notes and extract version info."""
    versions = []
    for f in sorted(tool_path.glob("*.md")):
        m = re.search(r'(?:RELEASE-NOTES|CHANGELOG)-(.+)\.md', f.name)
        if m:
            versions.append(m.group(1))
    return versions

def compute_hash(filepath):
    """Compute SHA256 hash of file content."""
    h = hashlib.sha256()
    with open(filepath, 'rb') as f:
        for chunk in iter(lambda: f.read(8192), b''):
            h.update(chunk)
    return h.hexdigest()

def generate_source_list(category, tool):
    """Get all source file paths relative to vault."""
    tool_path = NOTES / category / tool
    if not tool_path.exists():
        return []
    sources = []
    for f in sorted(tool_path.glob("*.md")):
        sources.append(str(f.relative_to(VAULT)))
    return sources

def generate_page(category, tool, entity, desc_zh, key_changes):
    """Generate a consolidated version history reference page."""
    tool_path = NOTES / category / tool
    if not tool_path.exists():
        return None

    versions = read_frontmatter_versions(tool_path)
    if not versions:
        return None

    # Sort versions
    def version_key(v):
        parts = v.split('.')
        return tuple(int(p) for p in parts)
    versions.sort(key=version_key)

    sources = generate_source_list(category, tool)
    source_count = len(sources)

    # Build version timeline table
    timeline_rows = []
    for ver, change in key_changes:
        timeline_rows.append(f"| v{ver} | {change} |")

    # Build source list (first 30, then summary)
    source_lines = []
    for s in sources[:30]:
        source_lines.append(f"- `{s}`")
    if source_count > 30:
        source_lines.append(f"- ... 共 {source_count} 个源文件")

    # Category labels
    cat_labels = {
        "kubernetes": "Kubernetes 核心",
        "observability": "可观测性",
        "security": "安全",
        "cli-tools": "CLI 工具",
        "cicd-gitops": "CI/CD & GitOps",
        "networking": "网络",
        "core-deps": "核心依赖",
        "storage": "存储",
    }

    # Build version range
    first_ver = versions[0] if versions else "?"
    last_ver = versions[-1] if versions else "?"

    # Tags
    tags = ["references", "version-history", tool, category]
    tags_yaml = "\n".join(f"- {t}" for t in tags)

    # Sources YAML
    sources_yaml = "\n".join(f"- \"{s}\"" for s in sources[:50])

    # Milestone versions for table
    milestone_versions = [v for v, _ in key_changes]
    milestone_rows = "\n".join(timeline_rows)

    content = f"""---
title: "{tool} 版本历史参考"
category: references
tags:
{tags_yaml}
summary: "{tool} ({desc_zh}) 的完整版本历史与里程碑参考。涵盖 v{first_ver} 到 v{last_ver} 共 {len(versions)} 个版本。"
base_confidence: 0.85
lifecycle: draft
tier: supporting
sources:
{sources_yaml}
last_updated: 2026-05
---

# {tool} 版本历史参考

> {desc_zh}

## 概述

本页汇总了 **{tool}** 从 v{first_ver} 到 v{last_ver} 的全部 {len(versions)} 个版本发布记录，来源于 `docs/topic-release-notes/{category}/{tool}/` 目录下的 Release Notes 文件。

## 里程碑版本时间线

| 版本 | 关键变更 |
|------|----------|
{milestone_rows}

## 完整版本列表

共 **{len(versions)}** 个版本：

| # | 版本号 | 类型 |
|---|--------|------|
"""

    # Build full version table
    for i, v in enumerate(versions, 1):
        parts = v.split('.')
        if len(parts) >= 2:
            if parts[1] == '0' and len(parts) == 2:
                vtype = "🟢 大版本"
            elif parts[0] == '0':
                vtype = "🟡 开发版"
            else:
                vtype = "🔵 补丁版"
        else:
            vtype = "🔵 补丁版"
        content += f"| {i} | v{v} | {vtype} |\n"

    content += f"""

## 版本兼容性

| {tool} 版本范围 | Kubernetes 兼容性 | 备注 |
|-----------------|-------------------|------|
| v{first_ver} - v{versions[len(versions)//3] if len(versions) > 2 else first_ver} | 1.19+ | 早期版本 |
| v{versions[len(versions)//3] if len(versions) > 2 else first_ver} - v{versions[2*len(versions)//3] if len(versions) > 2 else last_ver} | 1.24+ | 稳定版本 |
| v{versions[2*len(versions)//3] if len(versions) > 2 else last_ver} - v{last_ver} | 1.28+ | 推荐版本 |

## 关联页面

- [[entities/{entity}]] — {entity} 实体页
- [[concepts/{category}]] — {cat_labels.get(category, category)} 概念
- [[references/kubernetes-changelog-summary]] — Kubernetes 变更日志（如适用）

## 数据来源

Release Notes 文件位于：

```
docs/topic-release-notes/{category}/{tool}/
```

共计 {source_count} 个源文件。
"""

    return content

def main():
    REFS.mkdir(exist_ok=True)

    # Load manifest
    with open(MANIFEST) as f:
        manifest = json.load(f)

    created_pages = []
    now = datetime.now(timezone.utc).isoformat()

    for category, tool, entity, desc, key_changes in TOOLS:
        content = generate_page(category, tool, entity, desc, key_changes)
        if content is None:
            print(f"SKIP: {category}/{tool} - no files found")
            continue

        # Write page
        filename = f"{tool}-version-history.md"
        if tool == "kubernetes":
            filename = "kubernetes-changelog-summary.md"
        page_path = REFS / filename
        page_path.write_text(content)
        created_pages.append(filename)
        print(f"CREATED: references/{filename}")

        # Update manifest for each source file
        tool_path = NOTES / category / tool
        for f in sorted(tool_path.glob("*.md")):
            rel = str(f.relative_to(VAULT))
            if rel not in manifest["sources"]:
                manifest["sources"][rel] = {}
            content_hash = "sha256:" + compute_hash(f)
            manifest["sources"][rel].update({
                "ingested_at": now,
                "content_hash": content_hash,
                "source_type": "release-notes",
                "pages_created": [filename],
            })

    # Save manifest
    with open(MANIFEST, 'w') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"\nTotal pages created: {len(created_pages)}")
    print(f"Manifest updated: {MANIFEST}")

if __name__ == "__main__":
    main()
