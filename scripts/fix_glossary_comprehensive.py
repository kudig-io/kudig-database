#!/usr/bin/env python3
"""综合修复脚本：断链修复 + Related补充 + created字段 + 核心概念补建"""
from pathlib import Path
import re

BASE = Path("domain-17-system-foundation/topic-dictionary")
SKIP = {'k8s-glossary.md', 'README.md', 'MOC.md', 'GAP-ANALYSIS.md'}

# ────────────────────────────────────────────────────────────
# P1a: 路径修正映射表
# ────────────────────────────────────────────────────────────
PATH_FIXES = {
    "domain-17-system-foundation/topic-dictionary/fundamentals/pod": "domain-17-system-foundation/topic-dictionary/workloads/pod",
    "domain-17-system-foundation/topic-dictionary/configuration/secret": "domain-17-system-foundation/topic-dictionary/security/secret",
    "domain-17-system-foundation/topic-dictionary/platform-engineering/argo": "domain-17-system-foundation/topic-dictionary/operations/argo",
    "domain-17-system-foundation/topic-dictionary/operations/kubectl": "domain-17-system-foundation/topic-dictionary/tooling/kubectl",
    "domain-17-system-foundation/topic-dictionary/operations/helm": "domain-17-system-foundation/topic-dictionary/tooling/helm",
    "domain-17-system-foundation/topic-dictionary/operations/kustomize": "domain-17-system-foundation/topic-dictionary/tooling/kustomize",
    "domain-17-system-foundation/topic-dictionary/tooling/docker": "domain-17-system-foundation/topic-dictionary/fundamentals/docker",
    "domain-17-system-foundation/topic-dictionary/platform-engineering/flux": "domain-17-system-foundation/topic-dictionary/operations/flux",
    "domain-17-system-foundation/topic-dictionary/platform-engineering/tekton": "domain-17-system-foundation/topic-dictionary/operations/tekton",
    "domain-17-system-foundation/topic-dictionary/tooling/tekton": "domain-17-system-foundation/topic-dictionary/operations/tekton",
    "domain-17-system-foundation/topic-dictionary/security/cert-manager": "domain-17-system-foundation/topic-dictionary/operations/cert-manager",
    "domain-17-system-foundation/topic-dictionary/security/cloud-custodian": "domain-17-system-foundation/topic-dictionary/operations/cloud-custodian",
    "domain-17-system-foundation/topic-dictionary/security/networkpolicy": "domain-17-system-foundation/topic-dictionary/networking/networkpolicy",
    "domain-17-system-foundation/topic-dictionary/specialized-workloads/kata-containers": "domain-17-system-foundation/topic-dictionary/fundamentals/kata-containers",
    "domain-17-system-foundation/topic-dictionary/specialized-workloads/keda": "domain-17-system-foundation/topic-dictionary/scheduling/keda",
    "domain-17-system-foundation/topic-dictionary/specialized-workloads/kubeedge": "domain-17-system-foundation/topic-dictionary/platform-engineering/kubeedge",
    "domain-17-system-foundation/topic-dictionary/storage/configmap": "domain-17-system-foundation/topic-dictionary/configuration/configmap",
    "domain-17-system-foundation/topic-dictionary/storage/etcd": "domain-17-system-foundation/topic-dictionary/fundamentals/etcd",
    "domain-17-system-foundation/topic-dictionary/networking/grpc": "domain-17-system-foundation/topic-dictionary/platform-engineering/grpc",
    # Short references
    "etcd": "domain-17-system-foundation/topic-dictionary/fundamentals/etcd",
    "kubernetes": "domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes",
    "kyverno": "domain-17-system-foundation/topic-dictionary/security/kyverno",
    "opa": "domain-17-system-foundation/topic-dictionary/security/opa",
    "deployment": "domain-17-system-foundation/topic-dictionary/workloads/deployment",
}

# ────────────────────────────────────────────────────────────
# P1a: 修复 Related 中的断链路径
# ────────────────────────────────────────────────────────────
def fix_broken_links():
    fixed = 0
    for f in sorted(BASE.rglob("*.md")):
        if f.name in SKIP:
            continue
        text = f.read_text(encoding="utf-8", errors="ignore")
        new_text = text
        for wrong, correct in PATH_FIXES.items():
            if wrong in new_text and wrong != correct:
                # Only fix in Related section
                new_text = new_text.replace(f"[[{wrong}|", f"[[{correct}|")
                new_text = new_text.replace(f"[[{wrong}]]", f"[[{correct}]]")
        if new_text != text:
            f.write_text(new_text, encoding="utf-8")
            fixed += 1
    print(f"P1a 路径修正: {fixed} 个文件")
    return fixed

# ────────────────────────────────────────────────────────────
# P1b: 创建 8 个缺失的核心概念文件
# ────────────────────────────────────────────────────────────
CORE_TERMS = [
    ("fundamentals", "container", "容器", "Container",
     ["fundamentals", "runtime", "oci"],
     "容器是一种轻量级的操作系统级虚拟化技术，通过 Linux 内核的 namespace 和 cgroup 实现进程隔离和资源限制，是 Docker/Containerd 等运行时的核心构建单元。",
     "- **Linux Namespace**：PID/Network/Mount/UTS/IPC/User 六种隔离维度\n- **Cgroup**：CPU/Memory/IO/PID 等资源限制\n- **OCI 标准**：运行时规范和镜像规范\n- **UnionFS**：分层文件系统实现",
     "- 容器 = 进程 + namespace + cgroup + rootfs\n- 镜像是只读层，容器是读写层\n- 容器运行时（runc）负责创建和管理\n- 容器间共享内核，隔离通过 namespace 实现\n- 生命周期：create → start → running → stop → remove\n- 资源限制通过 cgroup v1/v2 配置\n- 健康检查通过进程探针实现",
     "- 应用容器化（微服务部署）\n- CI/CD 构建环境隔离\n- 多租户安全隔离\n- 资源配额和限流\n- 不可变基础设施\n- 最佳实践：单进程、非 root、只读 rootfs、健康检查",
     "- https://kubernetes.io/docs/concepts/containers/\n- https://opencontainers.org/",
     "- [[domain-17-system-foundation/topic-dictionary/fundamentals/docker|Docker]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/runc|runc]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd|containerd]]"),

    ("fundamentals", "cluster", "Kubernetes 集群", "Cluster",
     ["fundamentals", "cluster", "architecture"],
     "Kubernetes 集群是由一组节点（Node）组成的计算资源池，包含控制平面（Control Plane）和工作节点（Worker Node），提供容器编排、调度和生命周期管理能力。",
     "- **控制平面**：API Server/Scheduler/Controller Manager/etcd\n- **工作节点**：kubelet/kube-proxy/Container Runtime\n- **高可用**：多 Master + etcd 集群\n- **可扩展**：CRD/Operator/Webhook 扩展点",
     "- 集群 = Control Plane + Worker Nodes\n- API Server 是唯一入口（所有操作经此）\n- etcd 存储集群状态（Raft 共识）\n- Scheduler 决定 Pod 放置\n- kubelet 管理节点上的 Pod\n- kube-proxy 维护网络规则\n- 集群联邦（Federation）管理多集群",
     "- 生产环境高可用部署（3+ Master）\n- 多租户集群隔离\n- 集群升级和证书轮转\n- 集群网络安全加固\n- 多区域/多可用区部署\n- 最佳实践：托管 K8s（EKS/AKS/GKE）降低运维负担",
     "- https://kubernetes.io/docs/concepts/cluster-administration/\n- https://kubernetes.io/docs/setup/",
     "- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes|Kubernetes]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/namespace|Namespace]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/etcd|etcd]]"),

    ("fundamentals", "namespace", "命名空间", "Namespace",
     ["fundamentals", "multi-tenancy", "isolation"],
     "Namespace 是 Kubernetes 的逻辑隔离机制，将集群资源划分为虚拟的子集群，实现多团队/多环境/多租户的资源隔离和访问控制。",
     "- **逻辑隔离**：同一集群内的资源分组\n- **资源配额**：限制每个命名空间的资源用量\n- **RBAC 边界**：基于命名空间的访问控制\n- **默认命名空间**：default/kube-system/kube-public/kube-node-lease",
     "- Namespace 隔离资源名称（同命名空间内唯一）\n- ResourceQuota 限制 CPU/Memory/PVC/对象数量\n- LimitRange 设置默认的资源请求和限制\n- NetworkPolicy 控制跨命名空间网络访问\n- RBAC Role/RoleBinding 限定命名空间权限\n- 集群级资源（Node/PV/ClusterRole）不受命名空间约束\n- 4 个系统命名空间有特定用途",
     "- 多团队/多项目的资源隔离\n- 开发/测试/生产环境分离\n- 资源配额和成本分摊\n- 最小权限的 RBAC 设计\n- 最佳实践：避免 default 命名空间、命名规范、配合 NetworkPolicy",
     "- https://kubernetes.io/docs/concepts/overview/working-with-objects/namespaces/\n- https://kubernetes.io/docs/concepts/policy/resource-quotas/",
     "- [[domain-17-system-foundation/topic-dictionary/security/rbac|RBAC]]\n- [[domain-17-system-foundation/topic-dictionary/networking/networkpolicy|NetworkPolicy]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster|Cluster]]"),

    ("configuration", "env", "环境变量配置", "Environment Variables",
     ["configuration", "env", "configmap", "secret"],
     "环境变量（Env）是 Kubernetes Pod/Container 级别的配置注入机制，通过 env/envFrom 字段将配置值、ConfigMap 和 Secret 注入到容器中，是 12-Factor App 的配置管理实践。",
     "- **env**：逐个定义环境变量（支持 value/valueFrom）\n- **envFrom**：批量导入 ConfigMap/Secret 所有键值\n- **valueFrom**：引用 ConfigMapKeyRef/SecretKeyRef/FieldRef/ResourceFieldRef\n- **12-Factor**：配置与代码分离的标准实践",
     "- `env.name` + `env.value` 静态定义\n- `env.valueFrom.configMapKeyRef` 引用 ConfigMap\n- `env.valueFrom.secretKeyRef` 引用 Secret\n- `env.valueFrom.fieldRef` 引用 Pod 元数据（name/namespace/ip）\n- `env.valueFrom.resourceFieldRef` 引用资源限制\n- `envFrom.configMapRef` 批量导入\n- 环境变量变更需要重启 Pod（不同于 Volume 挂载的热更新）",
     "- 应用配置的外部化注入\n- 数据库连接串和 API Key 的安全传递\n- 多环境（dev/staging/prod）的配置差异化\n- Pod 元数据注入（Downward API）\n- 最佳实践：敏感信息用 Secret、批量配置用 envFrom、避免硬编码",
     "- https://kubernetes.io/docs/tasks/inject-data-application/define-environment-variable-container/\n- https://kubernetes.io/docs/concepts/configuration/configmap/",
     "- [[domain-17-system-foundation/topic-dictionary/configuration/configmap|ConfigMap]]\n- [[domain-17-system-foundation/topic-dictionary/security/secret|Secret]]\n- [[domain-17-system-foundation/topic-dictionary/configuration/helm-values|Helm Values]]"),

    ("networking", "headless-service", "Headless Service 无头服务", "Headless Service",
     ["networking", "service", "dns"],
     "Headless Service 是 clusterIP 设为 None 的特殊 Service，不分配虚拟 IP，而是通过 DNS 直接返回后端 Pod 的 IP 地址列表，适用于需要客户端直接连接 Pod 的场景。",
     "- **clusterIP: None**：不分配 ClusterIP\n- **DNS 记录**：为每个 Pod 创建 A/AAAA 记录\n- **直接连接**：客户端通过 DNS 获取 Pod IP 直连\n- **有状态应用**：StatefulSet 的标准网络方案",
     "- `clusterIP: None` 定义 Headless Service\n- DNS 格式：`pod-name.svc-name.namespace.svc.cluster.local`\n- 有 selector 时返回匹配 Pod 的 IP 列表\n- 无 selector 时配合 EndpointSlice 手动管理\n- StatefulSet 必须使用 Headless Service\n- 与 Service Mesh 的集成（Istio 自动处理）\n- DNS SRV 记录支持端口发现",
     "- StatefulSet（数据库集群）的网络标识\n- 服务发现的客户端直连模式\n- 需要知道具体后端地址的场景\n- gRPC 客户端的 DNS 负载均衡\n- 最佳实践：配合 StatefulSet 使用、DNS TTL 调优",
     "- https://kubernetes.io/docs/concepts/services-networking/service/#headless-services\n- https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/",
     "- [[domain-17-system-foundation/topic-dictionary/networking/service|Service]]\n- [[domain-17-system-foundation/topic-dictionary/networking/dns|DNS]]\n- [[domain-17-system-foundation/topic-dictionary/workloads/statefulset|StatefulSet]]"),

    ("observability", "logging", "日志体系", "Logging",
     ["observability", "logging", "fluentd", "loki"],
     "Kubernetes 日志体系涵盖容器标准输出、节点级日志采集和集中式日志平台的完整链路，是可观测性三大支柱之一，为故障诊断和安全审计提供基础数据。",
     "- **三层架构**：应用日志 → 节点采集 → 集中存储\n- **标准输出**：stdout/stderr 被容器运行时捕获\n- **日志采集**：Fluent Bit/Fluentd/Promtail 等\n- **日志存储**：Elasticsearch/Loki/S3 等",
     "- 容器 stdout/stderr → /var/log/containers/*.log\n- 节点级日志采集 DaemonSet\n- Fluent Bit（轻量采集）+ Fluentd（聚合路由）\n- OpenTelemetry Collector（统一采集）\n- 结构化日志（JSON）优于纯文本\n- 日志索引和保留策略\n- 日志关联（trace_id/span_id 贯穿链路）",
     "- 应用错误的快速定位\n- 安全审计和合规日志\n- 性能问题的日志分析\n- 多租户日志隔离\n- 最佳实践：结构化 JSON、保留策略、日志级别控制、敏感信息脱敏",
     "- https://kubernetes.io/docs/concepts/cluster-administration/logging/\n- https://opentelemetry.io/",
     "- [[domain-17-system-foundation/topic-dictionary/observability/fluentd|Fluentd]]\n- [[domain-17-system-foundation/topic-dictionary/observability/loki|Loki]]\n- [[domain-17-system-foundation/topic-dictionary/observability/logging-operator|Logging Operator]]"),

    ("platform-engineering", "custom-resource", "自定义资源 CRD", "Custom Resource",
     ["platform-engineering", "crd", "extension"],
     "Custom Resource（CR）是 Kubernetes 的核心扩展机制，通过 CRD（CustomResourceDefinition）注册自定义资源类型，将任意领域模型纳入 K8s 的声明式 API 管理体系。",
     "- **CRD**：CustomResourceDefinition 定义新资源类型\n- **CR**：Custom Resource 是 CRD 的实例\n- **声明式 API**：CR 遵循 K8s 的声明式管理范式\n- **Operator 模式**：CR + Controller = Operator",
     "- CRD YAML 定义资源 schema（OpenAPI v3）\n- API Group/Version/Kind 注册到 API Server\n- 验证（Validation）通过 OpenAPI schema\n- 子资源（status/scale）支持\n- Webhook（准入控制和转换）\n- Finalizers 生命周期管理\n- 版本管理和转换（conversion webhook）",
     "- 平台能力的 API 化（数据库/消息队列/证书）\n- 运维自动化（备份策略/巡检任务）\n- 业务模型的 K8s 化（工单/配置中心）\n- Operator 开发的基础\n- 最佳实践：版本演进、向后兼容、Status 子资源、条件（Conditions）",
     "- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/\n- https://book.kubebuilder.io/",
     "- [[domain-17-system-foundation/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]\n- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubebuilder|Kubebuilder]]\n- [[domain-17-system-foundation/topic-dictionary/fundamentals/kubernetes|Kubernetes]]"),

    ("tooling", "skaffold", "Skaffold 开发工具", "Skaffold",
     ["tooling", "development", "google"],
     "Skaffold 是 Google 开源的 K8s 开发工具，自动化构建/推送/部署的完整循环，支持文件监控、端口转发和调试模式，是 K8s 开发者的标准效率工具。",
     "- **开发循环**：代码变更 → 构建 → 推送 → 部署的自动化\n- **Google 出品**：Cloud Code 的底层引擎\n- **多构建器**：Docker/Jib/Buildpacks/Kaniko/ko\n- **调试模式**：端口转发和远程调试支持",
     "- `skaffold dev` 开发模式（文件监控 + 自动重部署）\n- `skaffold run` 单次构建部署\n- `skaffold debug` 调试模式（端口转发）\n- 支持 Docker/Jib/Buildpacks/Kaniko/ko 构建器\n- Helm/Kustomize/Kpt/raw YAML 部署\n- 多模块（Artifacts）并行构建\n- Profile 环境切换（dev/staging/prod）",
     "- K8s 应用的日常开发循环\n- 微服务的联调环境\n- CI/CD Pipeline 的本地验证\n- 团队的标准化开发工具\n- 最佳实践：dev profile + prod profile、build concurrency、port-forward",
     "- https://skaffold.dev/\n- https://github.com/GoogleContainerTools/skaffold",
     "- [[domain-17-system-foundation/topic-dictionary/tooling/devspace|DevSpace]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/telepresence|Telepresence]]\n- [[domain-17-system-foundation/topic-dictionary/tooling/helm|Helm]]"),
]

def create_core_terms():
    created = 0
    for cat, fn, zh, en, tags, ov, core, mech, use, refs, rel in CORE_TERMS:
        fp = BASE / cat / f"{fn}.md"
        if fp.exists():
            print(f"  = {cat}/{fn}.md (已存在)")
            continue
        tks = "\n".join(f"- {k}" for k in dict.fromkeys([zh, en, "dictionary"]))
        tg = "\n".join(f"- {t}" for t in tags)
        fp.parent.mkdir(parents=True, exist_ok=True)
        fp.write_text(f"""---
title: {zh}
description: '{ov[:80]}...'
category: dictionary
tags:
- k8s
- glossary
{tg}
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- {zh} 是什么
- {en} 详解
trigger_keywords:
{tks}
prerequisites:
- kubernetes
created: 2026-06
---

# {zh}（{en}）

## 概述

{ov}

## 核心概念/原理

{core}

## 关键机制或特性

{mech}

## 使用场景与最佳实践

{use}

## 参考链接

{refs}

## Related

{rel}
""", encoding="utf-8")
        created += 1
        print(f"  + {cat}/{fn}.md")
    print(f"P1b 核心概念补建: {created} 个新文件")
    return created

# ────────────────────────────────────────────────────────────
# P2: 为缺少 Related 段的旧文件补充 Related
# ────────────────────────────────────────────────────────────
def get_category_peers(category_dir, self_stem):
    """获取同分类下的其他文件作为 Related 候选"""
    peers = []
    if not category_dir.exists():
        return peers
    for f in sorted(category_dir.glob("*.md")):
        if f.name in SKIP or f.stem == self_stem:
            continue
        rel = str(f.relative_to(Path("."))).replace(".md", "")
        peers.append((f.stem, rel))
    return peers

def add_missing_related():
    fixed = 0
    for f in sorted(BASE.rglob("*.md")):
        if f.name in SKIP:
            continue
        text = f.read_text(encoding="utf-8", errors="ignore")
        # Check if Related section exists
        if re.search(r"## Related\s*\n", text):
            # Check if it has actual links
            m = re.search(r"## Related\s*\n(.*?)(?=\n## |\Z)", text, re.DOTALL)
            if m and re.findall(r"\[\[", m.group(1)):
                continue  # Has valid Related links
        # Get category peers
        cat_dir = f.parent
        peers = get_category_peers(cat_dir, f.stem)
        if len(peers) < 2:
            # Also get glossary as fallback
            glossary_link = "domain-17-system-foundation/topic-dictionary/k8s-glossary"
        # Pick up to 3 peers
        related_links = []
        for stem, path in peers[:3]:
            # Get title from file
            peer_file = BASE / cat_dir.name / f"{stem}.md"
            title = stem.replace("-", " ").title()
            if peer_file.exists():
                pt = peer_file.read_text(encoding="utf-8", errors="ignore")
                tm = re.search(r"^title:\s*(.+)$", pt, re.MULTILINE)
                if tm:
                    title = tm.group(1).strip()
            related_links.append(f"- [[{path}|{title}]]")

        if not related_links:
            related_links.append("- [[domain-17-system-foundation/topic-dictionary/k8s-glossary|K8s Glossary]]")

        related_section = "\n".join(related_links)

        # Check if Related section already exists but is empty
        if re.search(r"## Related\s*\n", text):
            # Replace empty Related section
            text = re.sub(
                r"(## Related\s*\n)(.*?)(?=\n## |\Z)",
                f"## Related\n\n{related_section}\n",
                text,
                flags=re.DOTALL
            )
        else:
            # Append Related section at the end
            text = text.rstrip() + f"\n\n## Related\n\n{related_section}\n"

        f.write_text(text, encoding="utf-8")
        fixed += 1

    print(f"P2 Related段补充: {fixed} 个文件")
    return fixed

# ────────────────────────────────────────────────────────────
# P3: 补全缺失的 created 字段
# ────────────────────────────────────────────────────────────
def add_missing_created():
    fixed = 0
    for f in sorted(BASE.rglob("*.md")):
        if f.name in SKIP:
            continue
        text = f.read_text(encoding="utf-8", errors="ignore")
        if "created:" in text[:500]:
            continue
        # Determine created date from file
        # R1-R4 files don't have created: 2026-06 marker, use 2026-05
        created_val = "2026-05"
        # Add created field before the --- closing of frontmatter
        # Find the last line before closing ---
        lines = text.split("\n")
        new_lines = []
        in_frontmatter = False
        added = False
        for i, line in enumerate(lines):
            if line.strip() == "---" and not in_frontmatter:
                in_frontmatter = True
                new_lines.append(line)
                continue
            if line.strip() == "---" and in_frontmatter and not added:
                # Insert created before closing ---
                new_lines.append(f"created: {created_val}")
                added = True
                in_frontmatter = False
            new_lines.append(line)
        if added:
            f.write_text("\n".join(new_lines), encoding="utf-8")
            fixed += 1

    print(f"P3 created字段补全: {fixed} 个文件")
    return fixed

# ────────────────────────────────────────────────────────────
# Main
# ────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("=" * 60)
    print("KUDIG Dictionary 综合修复")
    print("=" * 60)

    p1a = fix_broken_links()
    p1b = create_core_terms()
    p2 = add_missing_related()
    p3 = add_missing_created()

    print()
    print(f"总计修复:")
    print(f"  P1a 路径修正: {p1a} 文件")
    print(f"  P1b 核心概念: {p1b} 新文件")
    print(f"  P2  Related段: {p2} 文件")
    print(f"  P3  created字段: {p3} 文件")
