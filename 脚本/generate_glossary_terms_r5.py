#!/usr/bin/env python3
"""Round 5: 剩余高引用 CN 生态术语批量展开（24个）"""
import os
from pathlib import Path
BASE = Path("系统基础/topic-dictionary")

def w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists(): return False
    tks = "\n".join(f"- {k}" for k in dict.fromkeys([zh, en, "dictionary"]))
    tg = "\n".join(f"- {t}" for t in tags)
    r = rel or "- [[系统基础/topic-dictionary/k8s-glossary|K8s Glossary]]"
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
- kubectl-basics
created: "2026-06-24"
---

# {zh}

> **英文名**: {en}

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

{r}
""")
    return True

T = [
("fundamentals", "docker", "Docker", "Docker",
 ["docker", "container", "oci"],
 "Docker 是最广泛使用的容器平台，包含 Docker Engine（运行时）、Docker CLI 和 Docker Buildx（构建工具）。虽然 Kubernetes 已移除对 Docker 的直接支持（dockershim 弃用），但 Docker 镜像格式仍是 OCI 标准的基础。",
 "### Docker 与 K8s 的关系\n\n| 组件 | K8s 中使用 |\n|------|------------|\n| Docker Engine | 已被 containerd/CRI-O 替代 |\n| Docker Image | 仍然使用（OCI 兼容） |\n| Docker CLI | 开发环境仍广泛使用 |\n| Docker Compose | 本地多容器开发 |\n| Docker Buildx | 构建多架构镜像 |\n\n### OCI 标准\n\nDocker 推动了容器技术的发展，其镜像格式和运行时规范已被 OCI（Open Container Initiative）标准化。",
 "- **Docker Desktop**：macOS/Windows 上的开发环境。\n- **Docker Buildx**：多架构镜像构建（amd64/arm64）。\n- **Docker Compose**：本地多容器编排。\n- **docker save/load**：镜像离线传输。\n- **BuildKit**：Docker 内置的高级构建引擎。",
 "- 开发环境继续使用 Docker Desktop/Docker CLI。\n- 生产 K8s 集群使用 containerd 或 CRI-O 作为运行时。\n- 使用 `docker buildx build --platform linux/amd64,linux/arm64` 构建多架构镜像。\n- 使用 `.dockerignore` 减少构建上下文大小。\n- CI/CD 中使用 Docker-in-Docker 或 Kaniko 构建镜像。",
 "- [Docker Official](https://docs.docker.com/)",
 "- [[系统基础/topic-dictionary/fundamentals/containerd|Containerd]]\n- [[系统基础/topic-dictionary/fundamentals/cri|CRI]]\n- [[系统基础/topic-dictionary/fundamentals/container|Container]]\n- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]\n- [[系统基础/topic-dictionary/fundamentals/cri-o|CRI-O]]"),

("fundamentals", "runc", "runc", "runc",
 ["runc", "oci", "container-runtime"],
 "runc 是 OCI（Open Container Initiative）标准的容器运行时参考实现，负责将 OCI 镜像和配置转换为 Linux 容器进程。它是 containerd 和 CRI-O 底层的实际容器创建引擎。",
 "### 运行时层次\n\n```\nkubelet → CRI → containerd/CRI-O → runc → Linux Kernel (namespaces/cgroups)\n```\n\n### OCI 运行时规范\n\n- 定义了容器进程的创建、启动、停止、删除接口。\n- runc 使用 Linux namespaces（pid、net、mnt、ipc、uts、user）实现隔离。\n- 使用 cgroups 实现资源限制。\n- 使用 capabilities 和 seccomp 实现安全沙箱。",
 "- **轻量级**：runc 仅创建容器，不管理生命周期（由上层 daemon 管理）。\n- **OCI 合规**：完全遵循 OCI Image 和 Runtime 规范。\n- **seccomp 支持**：限制容器可用的系统调用。\n- **rootless 模式**：无 root 权限运行容器。\n- 配置文件：`config.json`（OCI bundle 格式）。",
 "- 通常不需要直接操作 runc，通过 containerd/CRI-O 间接使用。\n- 追求更安全的沙箱可考虑 crun（C 实现，更快）或 gVisor。\n- 调试时可使用 `runc exec` 进入容器。\n- 关注 runc 的 CVE 更新（如 CVE-2024-21626）。\n- rootless 容器适合开发环境的安全隔离。",
 "- [runc GitHub](https://github.com/opencontainers/runc)",
 "- [[系统基础/topic-dictionary/fundamentals/containerd|Containerd]]\n- [[系统基础/topic-dictionary/fundamentals/cri-o|CRI-O]]\n- [[系统基础/topic-dictionary/fundamentals/cri|CRI]]\n- [[系统基础/topic-dictionary/fundamentals/container|Container]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]"),

("tooling", "kustomize", "Kustomize", "Kustomize",
 ["kustomize", "configuration", "gitops"],
 "Kustomize 是 Kubernetes 原生的配置管理工具，通过 overlay 模式对 YAML 资源进行无模板的定制。它已内置到 kubectl（`kubectl apply -k`），是 Kubernetes 官方推荐的配置管理方案之一。",
 "### 核心概念\n\n- **Base**：基础配置（通用 YAML）。\n- **Overlay**：环境特定的修改层（dev/staging/prod）。\n- **kustomization.yaml**：定义 bases、patches、generators 的配置文件。\n\n### 与 Helm 对比\n\n| 特性 | Kustomize | Helm |\n|------|-----------|------|\n| 模板 | 无（YAML 叠加） | Go 模板 |\n| 复杂度 | 低 | 中 |\n| 包管理 | 无 | Chart/Release |\n| 适用场景 | 配置微调 | 应用打包分发 |",
 "- **Patches**：Strategic Merge Patch 和 JSON Patch 两种模式。\n- **Generators**：ConfigMap/Secret 自动生成（带内容哈希）。\n- **Components**：可复用的配置片段。\n- **内置到 kubectl**：`kubectl apply -k <dir>` 直接使用。\n- **Transformers**：全局修改 labels、namespaces、name prefixes。",
 "- 多环境配置管理使用 Kustomize overlay。\n- 配合 Argo CD/Flux 实现 GitOps 配置渲染。\n- 为 ConfigMap/Secret 使用 Generator 自动加 hash 触发滚动更新。\n- 使用 Components 提取跨环境共享的配置片段。\n- 复杂应用打包考虑 Helm，精细配置调整使用 Kustomize。",
 "- [Kustomize Official](https://kubectl.docs.kubernetes.io/guides/introduction/kustomize/)",
 "- [[系统基础/topic-dictionary/tooling/helm|Helm]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/gitops|GitOps]]\n- [[系统基础/topic-dictionary/platform-engineering/manifest|Manifest]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]"),

("security", "gatekeeper", "Gatekeeper", "OPA Gatekeeper",
 ["gatekeeper", "opa", "policy", "security"],
 "Gatekeeper 是 OPA（Open Policy Agent）的 Kubernetes 原生实现，通过 CRD 在集群中执行准入策略和审计。它将 Rego 策略封装为 ConstraintTemplate，让非 OPA 专家也能定义和执行策略。",
 "### 核心资源\n\n| 资源 | 功能 |\n|------|------|\n| ConstraintTemplate | 参数化的 Rego 策略模板 |\n| Constraint | ConstraintTemplate 的实例化（指定参数和目标） |\n| Config | 同步 K8s 资源到 OPA 缓存 |\n\n### 执行模式\n\n- **Deny**：拒绝不符合策略的请求（准入控制）。\n- **Warn**：允许但生成警告。\n- **Dryrun**：仅审计，不阻止。\n- **Audit**：定期扫描已有资源的合规性。",
 "- **Admission Webhook**：拦截 API 请求进行策略检查。\n- **Mutation**：自动修改不合规资源（alpha）。\n- **External Data**：引用外部数据源辅助策略决策。\n- **Library**：社区贡献的 ConstraintTemplate 库。\n- 与 CI/CD 集成进行部署前策略检查（gator CLI）。",
 "- 使用 Gatekeeper 替代 PSP 实施 Pod 安全策略。\n- 定义约束：禁止 latest 标签、要求 resource limits、限制特权容器。\n- 启用 Audit 定期扫描集群中的违规资源。\n- 使用 gator CLI 在 CI 流水线中测试策略合规性。\n- 考虑 Kyverno 作为更简单的替代方案（YAML 策略）。",
 "- [Gatekeeper Official](https://open-policy-agent.github.io/gatekeeper/)",
 "- [[系统基础/topic-dictionary/security/opa|OPA]]\n- [[系统基础/topic-dictionary/security/kyverno|Kyverno]]\n- [[系统基础/topic-dictionary/security/admission-controller|Admission Controller]]\n- [[系统基础/topic-dictionary/security/pod-security-policy|Pod Security Policy]]\n- [[系统基础/topic-dictionary/security/webhook|Webhook]]"),

("networking", "linkerd", "Linkerd", "Linkerd",
 ["linkerd", "service-mesh", "cncf"],
 "Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂 Istio 功能的中小规模服务网格场景。",
 "### 核心架构\n\n- **Linkerd Proxy**：Rust 编写的超轻量 sidecar（~10MB 内存）。\n- **Linkerd Control Plane**：管理代理配置和证书。\n- **Linkerd Viz**：可观测性 Dashboard。\n\n### 与 Istio 对比\n\n| 特性 | Linkerd | Istio |\n|------|---------|-------|\n| Proxy | Rust (轻量) | Envoy (功能丰富) |\n| 复杂度 | 低 | 高 |\n| mTLS | 内置 | 内置 |\n| L7 策略 | 有限 | 丰富 |\n| 资源开销 | 极低 | 较高 |",
 "- **mTLS**：自动为所有服务间通信启用 mTLS。\n- **负载均衡**：P2C（Power of Two Choices）算法。\n- **重试和超时**：应用级别的重试策略。\n- **流量拆分**：金丝雀发布和 A/B 测试。\n- **Multi-cluster**：跨集群服务通信。",
 "- 需要服务网格但希望最小运维复杂度时选择 Linkerd。\n- 使用 Linkerd 的 mTLS 实现零信任网络。\n- 启用 Linkerd Viz 监控服务网格指标。\n- 配合 Flagger 实现自动化金丝雀发布。\n- 使用 `linkerd check` 验证安装和配置。",
 "- [Linkerd Official](https://linkerd.io/)",
 "- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]"),

("networking", "consul", "Consul", "Consul",
 ["consul", "service-mesh", "service-discovery"],
 "Consul 是 HashiCorp 开源的服务网格和服务发现解决方案。它提供服务发现、健康检查、KV 存储和服务网格（通过 Envoy sidecar）等功能，支持多数据中心和多云部署。",
 "### 核心功能\n\n| 功能 | 说明 |\n|------|------|\n| Service Discovery | 服务注册和 DNS/HTTP 发现 |\n| Health Checking | 多维度健康检查 |\n| KV Store | 分布式键值存储 |\n| Service Mesh | 基于 Envoy 的 L7 流量管理 |\n| Multi-DC | 多数据中心联邦 |\n\n### 与 K8s Service 对比\n\nConsul 可补充 K8s 的服务发现：跨集群、非 K8s 服务、多数据中心场景。",
 "- **Consul Connect**：基于 Envoy 的 mTLS 服务网格。\n- **Catalog Sync**：K8s Service 与 Consul Catalog 双向同步。\n- **Intentions**：声明式的服务间访问控制策略。\n- **Mesh Gateway**：跨数据中心的服务网格通信。\n- 支持 Terraform 管理 Consul 配置。",
 "- 混合云/多云场景使用 Consul 统一服务发现。\n- 非 K8s 服务（VM、裸金属）需要纳入服务网格时使用 Consul。\n- 使用 Consul KV 存储应用配置。\n- 配合 Vault 实现服务间证书管理。\n- 使用 `consul-k8s` CLI 安装到 Kubernetes。",
 "- [Consul Official](https://www.consul.io/)",
 "- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/coredns|CoreDNS]]\n- [[系统基础/topic-dictionary/security/vault|Vault]]"),

("networking", "metallb", "MetalLB", "MetalLB",
 ["metallb", "loadbalancer", "networking", "bare-metal"],
 "MetalLB 是裸金属（Bare Metal）Kubernetes 集群的负载均衡器实现。它为不支持云厂商 LoadBalancer 的环境（如 on-premises）提供 LoadBalancer 类型的 Service 支持，是本地 K8s 集群的必备组件。",
 "### 工作模式\n\n| 模式 | 说明 | 适用场景 |\n|------|------|----------|\n| Layer 2 | ARP/NDP 应答 | 简单场景，单节点故障转移 |\n| BGP | 与路由器对等 | 大规模，多路径，快速故障转移 |\n\n### 工作原理\n\n```\nExternal Client → LoadBalancer IP → [MetalLB ARP/BGP] → Node → kube-proxy → Pod\n```",
 "- **IP Address Pool**：定义可分配的 IP 地址范围。\n- **L2 Advertisement**：通过 ARP 通告 VIP。\n- **BGP Advertisement**：通过 BGP 协议通告路由。\n- **speaker** DaemonSet：每节点运行，负责 IP 通告。\n- **controller**：分配 IP 和管理配置。",
 "- 裸金属集群必须安装 MetalLB 支持 LoadBalancer Service。\n- 简单场景使用 Layer 2 模式。\n- 大规模生产环境使用 BGP 模式配合 ToR 交换机。\n- 为不同 Service 分配不同的 IP Pool。\n- 监控 MetalLB 的 BGP 会话状态和 IP 分配情况。",
 "- [MetalLB Official](https://metallb.universe.tf/)",
 "- [[系统基础/topic-dictionary/networking/loadbalancer|LoadBalancer]]\n- [[系统基础/topic-dictionary/networking/service|Service]]\n- [[系统基础/topic-dictionary/networking/nodeport|NodePort]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/fundamentals/kube-proxy|Kube-proxy]]"),

("scheduling", "cluster-autoscaler", "Cluster Autoscaler", "Cluster Autoscaler",
 ["cluster-autoscaler", "autoscaling", "node"],
 "Cluster Autoscaler（CA）是 Kubernetes 官方的节点级自动扩缩容组件。当 Pod 因资源不足无法调度时自动扩容节点，当节点资源长期空闲时自动缩容节点，优化集群成本。",
 "### 与 HPA/VPA/KEDA 对比\n\n| 工具 | 扩缩目标 | 触发条件 |\n|------|----------|----------|\n| HPA | Pod 副本数 | CPU/Memory/Custom Metrics |\n| VPA | Pod 资源请求 | 历史资源使用 |\n| KEDA | Pod 副本数 | 外部事件源 |\n| **Cluster Autoscaler** | **节点数量** | **Pending Pods / 空闲节点** |\n\n### 扩容流程\n\nPending Pod → CA 检测 → 请求云厂商创建节点 → 节点加入集群 → Pod 调度",
 "- **Scale-Up**：检测 Pending Pod，模拟调度找到合适的节点组扩容。\n- **Scale-Down**：节点利用率低于阈值（默认 50%）持续 10 分钟后缩容。\n- **Node Group**：定义节点池的大小范围（min/max）和实例类型。\n- **Expander**：扩容策略（random/most-pods/least-waste/priority）。",
 "- 云环境集群必须配置 Cluster Autoscaler 实现成本优化。\n- 为不同工作负载定义不同的 Node Group（GPU/CPU/大内存）。\n- 使用 PDB（PodDisruptionBudget）保护关键 Pod 不被驱逐。\n- 设置合理的 `--scale-down-delay-after-add` 避免频繁扩缩。\n- 配合 Karpenter（AWS）获得更灵活的节点供应。",
 "- [Cluster Autoscaler](https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler)",
 "- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/scheduling/vpa|VPA]]\n- [[系统基础/topic-dictionary/scheduling/keda|KEDA]]\n- [[系统基础/topic-dictionary/operations/pdb|PDB]]\n- [[系统基础/topic-dictionary/fundamentals/node|Node]]"),

("tooling", "podman", "Podman", "Podman",
 ["podman", "container", "docker-alternative"],
 "Podman 是 Red Hat 开发的无守护进程（daemonless）容器引擎，兼容 Docker CLI 但无需 Docker daemon。它支持 rootless 运行容器，是 Linux 系统上 Docker 的安全替代方案。",
 "### 与 Docker 对比\n\n| 特性 | Podman | Docker |\n|------|--------|--------|\n| 架构 | 无 daemon（fork-exec） | Client-Daemon |\n| Rootless | 原生支持 | 需要额外配置 |\n| Pod 概念 | 原生（类似 K8s Pod） | 无 |\n| Docker API | 兼容大部分命令 | 原生 |\n| Compose | podman-compose | docker-compose |\n\n### Pod 概念\n\nPodman 原生支持 Pod（一组共享网络和存储的容器），类似 Kubernetes Pod。",
 "- **podman generate kube**：将容器/Pod 转换为 K8s YAML。\n- **podman play kube**：运行 K8s YAML（本地调试）。\n- **Quadlet**：systemd 集成管理容器。\n- **Podman Desktop**：跨平台 GUI 管理容器。\n- 支持 Docker 镜像格式和 OCI 镜像格式。",
 "- Linux 服务器使用 Podman 替代 Docker 提升安全性（rootless）。\n- 使用 `podman generate kube` 快速将容器配置转为 K8s YAML。\n- 使用 `podman play kube` 本地测试 K8s 配置。\n- 配合 systemd Quadlet 管理生产容器。\n- 注意 Podman 与 Docker 的细微差异（网络、卷挂载等）。",
 "- [Podman Official](https://podman.io/)",
 "- [[系统基础/topic-dictionary/fundamentals/docker|Docker]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|Containerd]]\n- [[系统基础/topic-dictionary/fundamentals/container|Container]]\n- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]"),

("platform-engineering", "rancher", "Rancher", "Rancher",
 ["rancher", "multi-cluster", "management"],
 "Rancher 是 SUSE 的企业级 Kubernetes 管理平台，提供多集群管理、安全策略、应用目录和运维工具的统一界面。它降低了管理多个 K8s 集群的复杂度，是企业多集群运维的主流方案之一。",
 "### 核心功能\n\n| 功能 | 说明 |\n|------|------|\n| Multi-Cluster | 统一管理 EKS/AKS/GKE/自建集群 |\n| RKE2/K3s | 内置轻量级 K8s 发行版 |\n| App Catalog | Helm Chart 应用市场 |\n| Security | 全局 RBAC + OPA Gatekeeper |\n| Monitoring | Prometheus + Grafana 一键启用 |\n| Logging | 集中式日志收集 |\n\n### Rancher 架构\n\n```\nRancher Server → 管理多个 Downstream Clusters\n                    ├── EKS\n                    ├── AKS\n                    ├── RKE2 (on-prem)\n                    └── K3s (edge)\n```",
 "- **Fleet**：大规模多集群 GitOps 部署引擎。\n- **Harvester**：HCI 超融合基础设施管理。\n- **Longhorn**：内置分布式块存储。\n- **NeuVector**：容器安全扫描和运行时保护。\n- **Elemental**：边缘节点的 OS 管理。",
 "- 企业多集群管理统一使用 Rancher。\n- 使用 Fleet 实现跨集群的 GitOps 部署。\n- 边缘场景使用 K3s + Rancher 统一管理。\n- 启用 Rancher 的 Monitoring 和 Logging 快速搭建可观测性。\n- 配置全局安全策略确保所有集群一致性。",
 "- [Rancher Official](https://www.rancher.com/)",
 "- [[系统基础/topic-dictionary/tooling/k3s|K3s]]\n- [[系统基础/topic-dictionary/storage/longhorn|Longhorn]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

("storage", "minio", "MinIO", "MinIO",
 ["minio", "storage", "s3", "object-storage"],
 "MinIO 是高性能的 S3 兼容对象存储系统，可在任何基础设施上部署。在 Kubernetes 中常用作 Thanos、Loki、Velero 等工具的对象存储后端，是云存储的私有化部署首选。",
 "### 核心特性\n\n| 特性 | 说明 |\n|------|------|\n| S3 兼容 | 完全兼容 AWS S3 API |\n| 高性能 | 单节点可达 100GB/s+ 吞吐 |\n| 纠删码 | 数据冗余和自愈 |\n| 加密 | 服务端加密（SSE-S3/SSE-KMS） |\n| 多租户 | 支持多租户隔离 |\n\n### K8s 中使用场景\n\n- Thanos 长期存储后端\n- Loki 日志存储\n- Velero 备份目标\n- Harbor 镜像存储",
 "- **Erasure Coding**：自动数据冗余，容忍多磁盘故障。\n- **Bucket Notification**：对象变更事件通知（Webhook/Kafka）。\n- **Replication**：跨集群/跨站点数据复制。\n- **Site Replication**：多站点双活部署。\n- **Console**：Web UI 管理存储桶和对象。",
 "- 需要私有 S3 兼容存储时部署 MinIO。\n- 作为 Thanos/Loki/Velero 的对象存储后端。\n- 使用 MinIO Operator 在 K8s 中管理 MinIO 集群。\n- 配置纠删码确保数据可靠性。\n- 启用 TLS 加密和 IAM 策略控制访问。",
 "- [MinIO Official](https://min.io/)",
 "- [[系统基础/topic-dictionary/observability/thanos|Thanos]]\n- [[系统基础/topic-dictionary/observability/loki|Loki]]\n- [[系统基础/topic-dictionary/operations/velero|Velero]]\n- [[系统基础/topic-dictionary/tooling/harbor|Harbor]]\n- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]"),

("observability", "datadog", "Datadog", "Datadog",
 ["datadog", "observability", "saas"],
 "Datadog 是企业级全栈可观测性 SaaS 平台，提供 Metrics、Logs、Traces、APM、Security 等一站式功能。在云原生环境中，Datadog Agent 部署为 DaemonSet 采集集群的指标、日志和追踪数据。",
 "### 核心产品\n\n| 产品 | 功能 |\n|------|------|\n| Infrastructure | 指标收集和可视化 |\n| APM | 分布式追踪 |\n| Logs | 日志管理 |\n| RUM | 真实用户体验监控 |\n| Synthetics | API/浏览器测试 |\n| Security | 运行时威胁检测 |\n\n### 与开源方案对比\n\n| 特性 | Datadog | 开源（Prom+Grafana+Loki+Tempo） |\n|------|---------|--------------------------------|\n| 部署 | SaaS | 自建 |\n| 成本 | 高（按主机计费） | 低（硬件成本） |\n| 维护 | 零运维 | 需运维 |",
 "- **Datadog Agent**：DaemonSet 部署，采集 Metrics + Logs + Traces。\n- **Cluster Agent**：集群级事件和外部指标。\n- **Autodiscovery**：自动发现新 Pod 并配置采集。\n- **DogStatsD**：兼容 StatsD 的指标聚合代理。\n- 丰富的集成（500+）：K8s、Istio、Redis、PostgreSQL 等。",
 "- 企业有预算且希望零运维可观测性时选择 Datadog。\n- 使用 Datadog Agent DaemonSet 自动采集集群数据。\n- 配置 Autodiscovery 自动为新 Pod 启用监控。\n- 使用 Datadog 的 APM 替代自建的 Jaeger/Tempo。\n- 注意成本控制：合理设置指标保留期和日志采样率。",
 "- [Datadog Official](https://www.datadoghq.com/)",
 "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/loki|Loki]]\n- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/observability/alertmanager|Alertmanager]]"),

("tooling", "k3s", "K3s", "K3s",
 ["k3s", "lightweight-k8s", "edge", "cncf"],
 "K3s 是 Rancher（现 SUSE）开发的轻量级 Kubernetes 发行版，现为 CNCF 沙箱项目。它将整个 K8s 控制平面打包为单个二进制文件（<100MB），特别适合边缘计算、IoT 和资源受限环境。",
 "### 与标准 K8s 对比\n\n| 特性 | K3s | 标准 K8s |\n|------|-----|----------|\n| 安装 | 单命令（curl） | kubeadm 多步 |\n| 二进制大小 | <100MB | ~1GB |\n| 内存占用 | ~512MB | ~2GB+ |\n| 默认 CNI | Flannel | 无 |\n| 默认存储 | Local Path | 无 |\n| 数据库 | SQLite/etcd | etcd |\n\n### 内置组件\n\nFlannel (CNI)、CoreDNS、Traefik (Ingress)、Local Path Provisioner、Klipper (Service LB)。",
 "- **单二进制**：所有组件编译为单一二进制文件。\n- **自动 TLS**：所有组件间通信自动启用 TLS。\n- **Server/Agent 模式**：Server 运行控制平面，Agent 运行工作负载。\n- **Helm Controller**：通过 CRD 声明式管理 Helm Chart。\n- 支持 ARM64 架构（树莓派等）。",
 "- 边缘/IoT 场景使用 K3s 部署轻量级 K8s。\n- 开发/测试环境快速搭建使用 K3s。\n- CI/CD 流水线中使用 K3s 运行集成测试。\n- 配合 Rancher 统一管理大规模 K3s 集群。\n- 生产环境考虑替换 Flannel 为 Cilium 提升网络性能。",
 "- [K3s Official](https://k3s.io/)",
 "- [[系统基础/topic-dictionary/fundamentals/kubernetes|Kubernetes]]\n- [[系统基础/topic-dictionary/tooling/minikube|Minikube]]\n- [[系统基础/topic-dictionary/tooling/kubeadm|Kubeadm]]\n- [[系统基础/topic-dictionary/platform-engineering/rancher|Rancher]]\n- [[系统基础/topic-dictionary/platform-engineering/kubeedge|KubeEdge]]"),

("operations", "flagger", "Flagger", "Flagger",
 ["flagger", "canary", "progressive-delivery"],
 "Flagger 是 Weaveworks 开源的渐进式发布工具，自动化金丝雀发布、A/B 测试和蓝绿部署。它集成 Prometheus、Istio、Linkerd 等，基于指标分析自动推进或回滚发布。",
 "### 核心概念\n\n- **Canary**：渐进式发布的 CRD 定义。\n- **Traffic Management**：通过 Istio/Linkerd/Nginx 控制流量比例。\n- **Metrics Analysis**：基于 Prometheus 指标自动判断发布是否健康。\n- **Webhooks**：自定义的准入/通知/确认钩子。\n\n### 金丝雀流程\n\n```\n1% 流量 → 指标分析 → 5% → 分析 → 10% → ... → 100%（完成）\n                                ↓ 异常\n                          自动回滚到 0%\n```",
 "- **自动推进**：根据错误率和延迟自动增加流量比例。\n- **自动回滚**：指标超过阈值时自动回滚。\n- **多种流量管理**：支持 Istio、Linkerd、Nginx、Traefik、Gateway API。\n- **A/B Testing**：基于 Header 的流量路由。\n- 支持 Slack/Teams/Discord 通知。",
 "- 配合 Istio/Linkerd 使用 Flagger 实现自动化金丝雀发布。\n- 定义关键的 SLI 指标（错误率、延迟）作为发布门禁。\n- 配置合理的 step 和 interval 控制发布速度。\n- 使用 Webhook 集成手动确认步骤。\n- 配合 Argo CD 实现 GitOps + 渐进式发布的完整流水线。",
 "- [Flagger Official](https://flagger.app/)",
 "- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/rolling-update|Rolling Update]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/networking/linkerd|Linkerd]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

("security", "external-secrets", "External Secrets Operator", "External Secrets Operator",
 ["external-secrets", "secrets-management", "security"],
 "External Secrets Operator（ESO）是 Kubernetes 原生的密钥同步工具，从外部密钥管理系统（Vault、AWS Secrets Manager、Azure Key Vault 等）自动同步密钥到 K8s Secret 资源。",
 "### 核心资源\n\n| 资源 | 功能 |\n|------|------|\n| SecretStore | 命名空间级的外部密钥源配置 |\n| ClusterSecretStore | 集群级的外部密钥源配置 |\n| ExternalSecret | 声明式的外部密钥同步定义 |\n| ClusterExternalSecret | 集群范围的密钥同步 |\n\n### 支持的 Backend\n\nHashiCorp Vault、AWS Secrets Manager、AWS Parameter Store、Azure Key Vault、GCP Secret Manager、1Password、Akeyless 等 20+。",
 "- **自动同步**：外部密钥变更时自动更新 K8s Secret。\n- **Template**：自定义 Secret 的 key 名称和数据格式。\n- **Push Secret**：将 K8s Secret 推送到外部存储。\n- **Refresh Interval**：配置同步频率。\n- 支持假删除（Deletion Policy）保护。",
 "- 使用 ESO 替代手动管理 K8s Secret。\n- 配合 Vault 实现集中式密钥管理。\n- 使用 ClusterSecretStore 统一管理所有命名空间的密钥源。\n- 为 CI/CD 生成的密钥配置自动同步到 Vault。\n- 监控 ESO 的同步状态和错误指标。",
 "- [External Secrets Operator](https://external-secrets.io/)",
 "- [[系统基础/topic-dictionary/security/vault|Vault]]\n- [[系统基础/topic-dictionary/configuration/secret|Secret]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/security/service-account|Service Account]]\n- [[系统基础/topic-dictionary/operations/cert-manager|cert-manager]]"),

("platform-engineering", "backstage", "Backstage", "Backstage",
 ["backstage", "developer-portal", "platform-engineering", "cncf"],
 "Backstage 是 Spotify 开源的开发者门户框架，现为 CNCF 孵化项目。它通过统一的界面集成服务目录、文档、模板和插件，帮助平台工程团队构建内部开发者体验（IDP）。",
 "### 核心功能\n\n| 功能 | 说明 |\n|------|------|\n| Software Catalog | 所有服务和组件的统一目录 |\n| Software Templates | 标准化的项目脚手架 |\n| TechDocs | 文档即代码（Markdown → 文档站） |\n| Plugins | 丰富的插件生态（150+） |\n| Search | 跨所有信息的统一搜索 |\n\n### 架构\n\nBackstage 是 React + Node.js 应用，通过 Plugin 架构扩展功能。K8s 插件可展示集群中服务的实时状态。",
 "- **Service Catalog**：以 YAML 描述每个服务的元数据和所有者。\n- **Scaffolder**：一键创建新服务（基于模板）。\n- **Kubernetes Plugin**：在 Portal 中查看 Pod/Deployment 状态。\n- **API 文档**：自动生成 OpenAPI/gRPC 文档。\n- **Scorecards**：服务质量和安全合规评分。",
 "- 平台团队使用 Backstage 构建内部开发者门户。\n- 使用 Software Templates 标准化新服务的创建流程。\n- 集成 K8s 插件让开发者在 Portal 中查看服务状态。\n- 使用 TechDocs 将服务文档与代码仓库同步。\n- 配合 Scorecards 跟踪服务的技术债务和安全合规。",
 "- [Backstage Official](https://backstage.io/)",
 "- [[系统基础/topic-dictionary/platform-engineering/crossplane|Crossplane]]\n- [[系统基础/topic-dictionary/operations/argo|Argo]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/security/rbac|RBAC]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

("observability", "mimir", "Mimir", "Mimir",
 ["mimir", "prometheus", "observability", "grafana"],
 "Grafana Mimir 是 Grafana Labs 开源的大规模 Prometheus 兼容指标存储和查询系统。它是 Cortex 的下一代替代品，提供水平扩展、多租户和长期指标存储能力。",
 "### 与 Cortex/Thanos 对比\n\n| 特性 | Mimir | Cortex | Thanos |\n|------|-------|--------|--------|\n| 状态 | 活跃开发 | 维护模式 | 活跃 |\n| 架构 | 单体微服务混合 | 纯微服务 | Sidecar |\n| 查询 | PromQL 兼容 | PromQL 兼容 | PromQL 兼容 |\n| 多租户 | 原生 | 原生 | 需额外 |\n\n### 核心组件\n\nDistributor、Ingester、Querier、Query-Frontend、Compactor、Store-Gateway、Ruler。",
 "- **水平扩展**：每个组件可独立扩缩容。\n- **PromQL 兼容**：完全兼容 Prometheus 查询语言。\n- **Ruler**：分布式规则评估和告警。\n- **对象存储**：TSDB 数据存储在 S3/GCS/MinIO。\n- 支持 Remote Write 接收指标数据。",
 "- 大规模 Prometheus 部署使用 Mimir 替代 Thanos。\n- 多租户环境使用 Mimir 的租户隔离功能。\n- 配合 Grafana 构建统一的指标可视化。\n- 使用 Remote Write 将多个 Prometheus 实例的数据汇聚到 Mimir。\n- 配置 Compactor 的保留策略管理存储成本。",
 "- [Mimir Official](https://grafana.com/oss/mimir/)",
 "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/thanos|Thanos]]\n- [[系统基础/topic-dictionary/observability/promql|PromQL]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]\n- [[系统基础/topic-dictionary/observability/alertmanager|Alertmanager]]"),

("networking", "envoy-gateway", "Envoy Gateway", "Envoy Gateway",
 ["envoy-gateway", "gateway-api", "ingress", "cncf"],
 "Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy 官方的网关方案，将 Envoy 作为独立的数据平面，通过 Gateway API 标准化管理入站流量。",
 "### 核心架构\n\n- **Envoy Gateway Controller**：监听 Gateway API 资源并配置 Envoy。\n- **Envoy Proxy**：数据平面，处理实际流量。\n- **EnvoyProxy CRD**：自定义 Envoy 部署和配置。\n\n### Gateway API 概念\n\n| 资源 | 功能 |\n|------|------|\n| GatewayClass | 网关实现类型 |\n| Gateway | 入口点和监听器定义 |\n| HTTPRoute | HTTP 路由规则 |\n| TLSRoute | TLS 路由规则 |\n| GRPCRoute | gRPC 路由规则 |",
 "- **Gateway API 原生**：完全遵循 Kubernetes Gateway API 标准。\n- **Envoy Extension**：支持 Envoy 的 Wasm/Lua 扩展。\n- **Rate Limiting**：内置限流功能。\n- **Security Policy**：JWT 验证、CORS、ExtAuth 等。\n- **Traffic Splitting**：基于权重的流量分割（金丝雀）。",
 "- 新集群使用 Envoy Gateway 替代传统 Ingress Controller。\n- 使用 Gateway API 标准化入站流量管理。\n- 配合 cert-manager 自动管理 TLS 证书。\n- 使用 EnvoyProxy CRD 自定义 Envoy 部署参数。\n- 关注 Gateway API 的 GAMMA 倡议（服务间流量管理）。",
 "- [Envoy Gateway Official](https://gateway.envoyproxy.io/)",
 "- [[系统基础/topic-dictionary/networking/envoy|Envoy]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]\n- [[系统基础/topic-dictionary/networking/traefik|Traefik]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]"),

("tooling", "strimzi", "Strimzi", "Strimzi",
 ["strimzi", "kafka", "streaming", "cncf"],
 "Strimzi 是 CNCF 孵化项目，在 Kubernetes 上提供 Apache Kafka 的原生部署和管理能力。它通过 Operator 模式自动化 Kafka 集群的部署、扩缩容、升级和监控。",
 "### 核心 CRD\n\n| 资源 | 功能 |\n|------|------|\n| Kafka | Kafka 集群定义 |\n| KafkaTopic | Topic 管理 |\n| KafkaUser | 用户和 ACL 管理 |\n| KafkaConnect | Kafka Connect 集群 |\n| KafkaBridge | HTTP 桥接 |\n| KafkaMirrorMaker | 跨集群镜像 |\n\n### 部署模式\n\n- **Ephemeral**：临时存储（测试）。\n- **Persistent**：持久化存储（生产）。\n- **JBOD**：多磁盘存储。",
 "- **Operator 管理**：自动化 Kafka 集群生命周期。\n- **Cruise Control**：自动分区重平衡。\n- **Tiered Storage**：热/温/冷分层存储。\n- **mTLS**：内置客户端和服务端加密。\n- **OAuth/OIDC**：企业级认证集成。",
 "- K8s 中部署 Kafka 优先使用 Strimzi。\n- 生产环境使用 Persistent 模式配置 3 副本。\n- 配合 Cruise Control 实现分区自动平衡。\n- 使用 KafkaUser CRD 管理客户端认证和 ACL。\n- 监控 Kafka 的 lag 指标和分区状态。",
 "- [Strimzi Official](https://strimzi.io/)",
 "- [[系统基础/topic-dictionary/platform-engineering/operator-pattern|Operator Pattern]]\n- [[系统基础/topic-dictionary/storage/persistent-volume|Persistent Volume]]\n- [[系统基础/topic-dictionary/security/certificate|Certificate]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/workloads/statefulset|StatefulSet]]"),

("specialized-workloads", "seldon", "Seldon", "Seldon",
 ["seldon", "ml", "inference", "mlops"],
 "Seldon 是 ML 模型部署和推理管理平台，提供 Seldon Core（K8s 原生推理引擎）和 Seldon Deploy（企业级 ML 部署管理）。它支持多框架模型的可扩展部署和 A/B 测试。",
 "### 核心概念\n\n- **SeldonDeployment**：模型推理服务的 CRD。\n- **Inference Graph**：多模型编排（串行/并行/路由/组合）。\n- **Pre-packaged Servers**：预构建的模型服务（SKLearn/XGBoost/TensorFlow 等）。\n\n### 与 KServe 对比\n\n| 特性 | Seldon | KServe |\n|------|--------|--------|\n| 成熟度 | 成熟 | CNCF 孵化 |\n| 编排 | 丰富的 Graph | 简单 |\n| 企业功能 | Seldon Deploy | 开源 |",
 "- **推理图**：组合多个模型（预处理→推理→后处理）。\n- **A/B 测试**：基于权重的流量分配到不同模型版本。\n- **Metrics**：内置推理延迟和请求量指标。\n- **Explainer**：模型可解释性集成（Alibi）。\n- 支持自定义 Python/Java 推理容器。",
 "- 需要复杂模型编排（多模型组合）时选择 Seldon。\n- 使用 Inference Graph 构建 ML Pipeline 的推理阶段。\n- 配合 Seldon Deploy 管理大规模模型部署。\n- 使用 A/B 测试验证新模型版本的效果。\n- 考虑 KServe 作为轻量替代方案。",
 "- [Seldon Official](https://www.seldon.io/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]\n- [[系统基础/topic-dictionary/specialized-workloads/ray|Ray]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]"),

("specialized-workloads", "openfaas", "OpenFaaS", "OpenFaaS",
 ["openfaas", "serverless", "faas"],
 "OpenFaaS（Functions as a Service）是开源的 Serverless 框架，支持在 Kubernetes 和 Docker 上运行函数。它将函数打包为容器镜像，通过 HTTP 触发，支持自动扩缩容和缩到零。",
 "### 核心组件\n\n| 组件 | 功能 |\n|------|------|\n| Gateway | API 网关和函数管理 |\n| faas-cli | 命令行工具 |\n| Function | 函数定义（容器镜像） |\n| Queue Worker | 异步函数处理 |\n\n### 与 Knative 对比\n\n| 特性 | OpenFaaS | Knative |\n|------|----------|--------|\n| 复杂度 | 低 | 高 |\n| 依赖 | 少 | 需 Knative Serving |\n| 缩到零 | 支持 | 支持 |\n| 语言支持 | 任意（容器） | 任意（容器） |",
 "- **模板**：预构建的函数模板（Python/Node/Go 等）。\n- **异步调用**：通过 NATS 队列异步执行函数。\n- **Auto-scaling**：基于 RPS 或 CPU 的自动扩缩。\n- **Scale-to-Zero**：无调用时缩到零。\n- 支持私有函数和认证。",
 "- 轻量级 Serverless 需求选择 OpenFaaS。\n- 使用 faas-cli 快速创建和部署函数。\n- 配合 CronJob 实现定时触发的函数执行。\n- 使用异步模式处理后台任务。\n- 为函数设置合理的超时和资源限制。",
 "- [OpenFaaS Official](https://www.openfaas.com/)",
 "- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/specialized-workloads/keda|KEDA]]\n- [[系统基础/topic-dictionary/workloads/deployment|Deployment]]\n- [[系统基础/topic-dictionary/scheduling/hpa|HPA]]\n- [[系统基础/topic-dictionary/networking/ingress|Ingress]]"),

("fundamentals", "kata-containers", "Kata Containers", "Kata Containers",
 ["kata-containers", "sandbox", "security", "oci"],
 "Kata Containers 是 OpenInfra Foundation 的开源项目，通过轻量级虚拟机提供容器级别的隔离。它兼容 OCI 运行时规范，可作为 runc 的安全替代方案，特别适合多租户和高安全要求的场景。",
 "### 隔离级别对比\n\n| 运行时 | 隔离方式 | 安全级别 | 开销 |\n|--------|----------|----------|------|\n| runc | Namespace + Cgroup | 中 | 极低 |\n| gVisor | 用户态内核 | 高 | 低 |\n| Kata Containers | 轻量 VM | 极高 | 中 |\n| Firecracker | microVM | 极高 | 中 |\n\n### 工作原理\n\n每个 Pod 运行在独立的轻量 VM 中，VM 内运行 Linux 内核和容器进程。",
 "- **OCI 兼容**：可作为 containerd/CRI-O 的 OCI Runtime。\n- **RuntimeClass**：通过 K8s RuntimeClass 选择性使用。\n- **多种 VMM**：支持 QEMU、Cloud Hypervisor、Firecracker。\n- **Direct-Attached Volume**：PV 直通到 VM 中。\n- 与 Kubernetes 完全集成（Pod、Service、NetworkPolicy）。",
 "- 多租户集群使用 Kata 提供 VM 级别的租户隔离。\n- 运行不可信代码时使用 Kata 替代 runc。\n- 通过 RuntimeClass 为特定 Pod 指定 Kata 运行时。\n- 注意 Kata 的额外资源开销（每 Pod ~30MB VM 开销）。\n- 监控 Kata Pod 的启动时间（比 runc 慢 1-2 秒）。",
 "- [Kata Containers Official](https://katacontainers.io/)",
 "- [[系统基础/topic-dictionary/fundamentals/runc|runc]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|Containerd]]\n- [[系统基础/topic-dictionary/fundamentals/cri-o|CRI-O]]\n- [[系统基础/topic-dictionary/security/security-context|Security Context]]\n- [[系统基础/topic-dictionary/fundamentals/pod|Pod]]"),
]

def main():
    created, skipped = [], []
    for t in T:
        cat, fn, zh, en, tags, ov, core, mech, use, refs, rel = t
        ok = w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel)
        (created if ok else skipped).append(f"{cat}/{fn}.md")
    print(f"新创建: {len(created)}")
    for f in created: print(f"  + {f}")
    if skipped:
        print(f"跳过: {len(skipped)}")
        for f in skipped: print(f"  ~ {f}")

if __name__ == '__main__':
    main()
