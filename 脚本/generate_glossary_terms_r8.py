#!/usr/bin/env python3
"""Round 8: 剩余高频引用缺失术语批量展开（25个）"""
from pathlib import Path
BASE = Path("系统基础/topic-dictionary")

def w(cat, fn, zh, en, tags, ov, core, mech, use, refs, rel=""):
    fp = BASE / cat / f"{fn}.md"
    if fp.exists():
        return False
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

{r}
""", encoding="utf-8")
    return True

TERMS = [
    # ── Fundamentals / Runtime ──
    ("fundamentals", "kuasar", "Kuasar 多沙箱运行时", "Kuasar",
     ["fundamentals", "container-runtime", "sandbox"],
     "Kuasar 是华为开源的 CNCF Sandbox 项目，提供多沙箱容器运行时管理，统一 containerd 与多种沙箱运行时（Kata/microVM/Wasm/AppKernel）的集成，简化异构运行时的部署和管理。",
     "- **多沙箱统一**：一套 sandboxer 管理多种沙箱类型\n- **containerd 集成**：通过 Sandboxer API 与 containerd 无缝对接\n- **CNCF Sandbox**：华为开源，社区活跃\n- **异构支持**：Kata Containers、WasmEdge、Quark、gVisor 等",
     "- Sandboxer 插件架构（每种沙箱一个 sandboxer 实现）\n- 通过 containerd runtime handler 选择沙箱类型\n- 支持 Kata/microVM/Wasm/AppKernel 四种沙箱\n- 统一的沙箱生命周期管理\n- 轻量级管理进程，资源开销低\n- 与 Kubernetes RuntimeClass 集成",
     "- 需要多种容器运行时共存的集群\n- Kata Containers + Wasm 混合工作负载\n- 安全隔离要求不同的混合工作负载\n- 边缘设备的异构运行时管理\n- containerd 生态的运行时扩展",
     "- https://kuasar.io/\n- https://github.com/kuasar-io/kuasar",
     "- [[系统基础/topic-dictionary/fundamentals/runc|runc]]\n- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/fundamentals/youki|youki]]"),

    ("fundamentals", "urunc", "urunc 微库运行时", "urunc",
     ["fundamentals", "container-runtime", "unikernel"],
     "urunc 是 Nubificus 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Unikernel 和轻量虚拟机，利用 Unikernel 的极小攻击面和快速启动特性，为安全敏感工作负载提供超轻量隔离方案。",
     "- **Unikernel 支持**：在 K8s 上运行 Unikernel（Unikraft/MirageOS/OSv）\n- **超轻量隔离**：每个容器运行在独立的微型内核中\n- **快速启动**：毫秒级启动时间\n- **CNCF Sandbox**：Nubificus 主导",
     "- 支持 Unikraft、MirageOS、OSv 等 Unikernel\n- 基于 Firecracker/QEMU 的轻量 VM 隔离\n- OCI 兼容的镜像格式\n- 与 containerd shim 集成\n- 极低内存开销（MB 级）\n- Rum 命令行工具",
     "- 安全敏感工作负载的强隔离\n- Serverless 函数的快速启动容器\n- 边缘设备的超轻量运行时\n- 零信任架构中的工作负载隔离\n- 替代 gVisor/Kata 的轻量方案",
     "- https://urunc.io/\n- https://github.com/nubificus/urunc",
     "- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/fundamentals/runc|runc]]\n- [[系统基础/topic-dictionary/fundamentals/kuasar|Kuasar]]"),

    # ── Tooling ──
    ("tooling", "podman-desktop", "Podman Desktop 图形界面", "Podman Desktop",
     ["tooling", "gui", "container"],
     "Podman Desktop 是 Red Hat 开源的容器管理图形界面工具，提供容器、镜像、Pod 和 Kubernetes 的可视化管理，是 Docker Desktop 的开源替代方案。",
     "- **图形界面**：容器全生命周期的可视化管理\n- **多引擎支持**：支持 Podman、Docker、Lima 等多种容器引擎\n- **K8s 集成**：一键部署容器到本地 Kubernetes\n- **Red Hat 开源**：Docker Desktop 的免费替代",
     "- 容器/Pod/镜像的可视化管理\n- 多引擎切换（Podman/Docker/Lima）\n- Compose 文件支持和执行\n- 一键部署到 K8s（生成 K8s YAML）\n- Kind/Minikube/K3s 本地集群管理\n- 扩展插件（OpenShift Local/Docker 扩展）",
     "- 开发者日常容器管理\n- Docker Desktop 的开源替代\n- 容器到 K8s 的迁移辅助\n- 教学环境的容器可视化管理\n- 多引擎环境的统一管理",
     "- https://podman-desktop.io/\n- https://github.com/containers/podman-desktop",
     "- [[系统基础/topic-dictionary/tooling/podman|Podman]]\n- [[系统基础/topic-dictionary/fundamentals/docker|Docker]]\n- [[系统基础/topic-dictionary/tooling/minikube|Minikube]]"),

    ("tooling", "copa", "Copa 容器补丁工具", "Copa",
     ["tooling", "security", "container"],
     "Copa（Container Patching）是微软开源的 CNCF Sandbox 项目，无需访问源代码或 Dockerfile 即可直接修补容器镜像中的 OS 包漏洞，大幅降低容器漏洞修复的门槛。",
     "- **无源码修补**：直接修补已有镜像中的 OS 包漏洞\n- **Trivy 集成**：使用 Trivy 扫描结果驱动修补\n- **CNCF Sandbox**：微软主导，社区活跃\n- **零重建**：无需重新构建镜像即可修复漏洞",
     "- `copa patch` 根据扫描报告修补镜像\n- 支持 Debian/Ubuntu/Alpine/RHEL/Amazon Linux\n- Trivy SARIF/JSON 格式扫描报告输入\n- 修补后的镜像验证（重新扫描确认修复）\n- 支持自定义包源和镜像 Registry\n- 批量修补（batch patching）",
     "- 紧急漏洞的快速修复（无需等待上游重建）\n- 遗留镜像的漏洞修补\n- CI/CD Pipeline 中的自动漏洞修补\n- 合规要求下的漏洞 SLA 管理\n- 第三方镜像的安全加固",
     "- https://project-copa.dev/\n- https://github.com/project-copacetic/copacetic",
     "- [[系统基础/topic-dictionary/security/trivy|Trivy]]\n- [[系统基础/topic-dictionary/tooling/docker|Docker]]\n- [[系统基础/topic-dictionary/security/supply-chain-security|供应链安全]]"),

    ("tooling", "eraser", "Eraser 镜像清理", "Eraser",
     ["tooling", "operations", "cleanup"],
     "Eraser 是微软开源的 CNCF Sandbox 项目，自动清理 Kubernetes 节点上未使用的容器镜像，释放磁盘空间，支持基于漏洞扫描结果的自动镜像删除。",
     "- **自动清理**：定时清理节点上未被使用的镜像\n- **漏洞驱动**：基于漏洞扫描结果删除有问题镜像\n- **CNCF Sandbox**：微软主导的轻量运维工具\n- **DaemonSet 部署**：每个节点自动运行清理任务",
     "- ImageJob CRD 定义清理任务\n- 支持 Trivy 漏洞扫描集成\n- 可配置的保留策略（按年龄/大小/名称）\n- 定时调度（CronJob 式）\n- 非使用镜像自动识别和删除\n- Prometheus 指标导出",
     "- 节点磁盘空间管理\n- 自动化镜像垃圾回收\n- 安全合规的镜像生命周期管理\n- 大规模集群的镜像清理自动化\n- 开发环境的定期空间回收",
     "- https://eraser-dev.github.io/eraser/\n- https://github.com/eraser-dev/eraser",
     "- [[系统基础/topic-dictionary/tooling/docker|Docker]]\n- [[系统基础/topic-dictionary/security/trivy|Trivy]]\n- [[系统基础/topic-dictionary/operations/k8sgpt|K8sGPT]]"),

    # ── Networking ──
    ("networking", "loxilb", "LoxiLB eBPF 负载均衡", "LoxiLB",
     ["networking", "load-balancer", "ebpf"],
     "LoxiLB 是基于 eBPF 的高性能外部负载均衡器，专为 Kubernetes 设计，提供 L4/L7 负载均衡和 NAT，可替代 MetalLB + kube-proxy + external LB 的组合。",
     "- **eBPF 驱动**：使用 eBPF/XDP 实现高性能数据面\n- **多模式**：L4/L7 负载均衡、NAT、FW、Egress\n- **K8s 原生**：Operator 模式部署，自动感知 Service\n- **轻量级**：单进程，资源占用极低",
     "- Service Type LoadBalancer 自动分配\n- kube-proxy 替代（eBPF 模式）\n- L4/L7 负载均衡（IPVS 替代）\n- 多集群负载均衡\n- SCTP 支持（5G/Telco 场景）\n- 健康检查和故障转移\n- Prometheus 指标导出",
     "- 裸金属/边缘环境的 LoadBalancer 实现\n- MetalLB + kube-proxy 的统一替代\n- 5G/Telco 的 SCTP 负载均衡\n- 需要 eBPF 高性能的网络方案\n- 轻量级外部负载均衡",
     "- https://loxilb.io/\n- https://github.com/loxilb-io/loxilb",
     "- [[系统基础/topic-dictionary/networking/metallb|MetalLB]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]\n- [[系统基础/topic-dictionary/networking/kube-vip|kube-vip]]"),

    # ── Security ──
    ("security", "bank-vaults", "Bank Vaults Vault 集成", "Bank Vaults",
     ["security", "vault", "secrets"],
     "Bank Vaults（vault-secrets-webhook + vault-operator）是 Banzai Cloud 开源的 HashiCorp Vault Kubernetes 集成工具集，通过 Webhook 自动注入 Vault 密钥到 Pod 环境变量和 Volume 中。",
     "- **自动注入**：通过 Admission Webhook 自动从 Vault 拉取密钥\n- **Vault Operator**：在 K8s 上管理 Vault 实例的生命周期\n- **零改造**：应用无需修改代码即可使用 Vault 密钥\n- **Banzai Cloud 出品**：活跃的 Vault K8s 集成方案",
     "- vault-secrets-webhook：环境变量和 ConfigMap/Secret 的 Vault 引用替换\n- vault-operator：Vault 集群的 K8s Operator（HA、备份、配置）\n- 支持 Vault Agent Sidecar 注入\n- 支持 Vault PKI 证书自动轮转\n- 支持 Kubernetes Auth Method\n- 与 External Secrets 互补使用",
     "- Vault 密钥的 K8s 原生集成\n- 无需修改应用代码的密钥注入\n- Vault 集群的自动化运维\n- 合规要求下的密钥轮转和审计\n- 多环境密钥管理的统一方案",
     "- https://github.com/bank-vaults/vault-secrets-webhook\n- https://bank-vaults.dev/",
     "- [[系统基础/topic-dictionary/security/vault|Vault]]\n- [[系统基础/topic-dictionary/security/external-secrets|External Secrets]]\n- [[系统基础/topic-dictionary/security/sops|SOPS]]"),

    ("security", "tuf", "TUF 更新框架", "TUF",
     ["security", "supply-chain", "cncf"],
     "The Update Framework（TUF）是 CNCF 毕业项目，为软件更新提供密码学安全框架，防止更新过程中的篡改、回滚攻击和密钥泄露，是软件供应链安全的基础设施。",
     "- **安全更新**：通过签名验证和元数据机制确保软件更新的安全性\n- **密钥轮转**：支持在线/离线密钥分离和定期轮转\n- **CNCF 毕业**：经过大规模生产验证\n- **广泛采用**：PyPI、Notary、Sigstore 等均使用 TUF",
     "- 四级密钥层次（Root/Targets/Snapshot/Timestamp）\n- 在线/离线密钥分离（降低密钥泄露风险）\n- 版本号和过期时间管理\n- 委托（Delegation）机制支持多签名者\n- 参考实现（python-tuf / go-tuf / rust-tuf）\n- Sigstore 的 TUF Root 信任链",
     "- 软件分发系统的安全更新机制\n- 容器 Registry 的内容完整性保障\n- OTA（Over-the-Air）更新的安全验证\n- 供应链中的信任链建立\n- 与 Notary/Sigstore 集成的综合安全方案",
     "- https://theupdateframework.io/\n- https://github.com/theupdateframework/specification",
     "- [[系统基础/topic-dictionary/security/notary-project|Notary Project]]\n- [[系统基础/topic-dictionary/security/in-toto|in-toto]]\n- [[系统基础/topic-dictionary/security/supply-chain-security|供应链安全]]"),

    ("security", "spire", "SPIRE 身份框架", "SPIRE",
     ["security", "identity", "spiffe"],
     "SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目，实现 SPIFFE 规范的生产级参考实现，为工作负载提供通用的加密身份框架，自动签发和管理短期 X.509 证书和 JWT。",
     "- **SPIFFE 实现**：SPIFFE 标准的生产级参考实现\n- **自动身份**：基于节点和工作负载属性自动分配身份\n- **短期凭证**：自动签发和轮转短期 X.509 SVID 和 JWT-SVID\n- **CNCF 毕业**：经过大规模生产验证",
     "- Server + Agent 分布式架构\n- Node Attestation（节点证明）多种插件\n- Workload Attestation（工作负载证明）\n- SVID 自动签发和轮转（X.509 / JWT）\n- Federation API 跨域联邦\n- 支持 Kubernetes、AWS、GCP 等多平台\n- 与 Envoy SDS API 集成",
     "- 微服务间的 mTLS 自动管理\n- 零信任网络中的工作负载身份\n- 多集群/多云的身份联邦\n- Kubernetes 工作负载的身份认证\n- 与 Istio/Envoy 集成的服务网格身份",
     "- https://spiffe.io/spire/\n- https://github.com/spiffe/spire",
     "- [[系统基础/topic-dictionary/security/spiffe-spire-identity|SPIFFE/SPIRE]]\n- [[系统基础/topic-dictionary/security/cert-manager|cert-manager]]\n- [[系统基础/topic-dictionary/networking/istio|Istio]]"),

    # ── Observability ──
    ("observability", "opencost", "OpenCost 成本监控", "OpenCost",
     ["observability", "cost", "cncf"],
     "OpenCost 是 CNCF Sandbox 项目，为 Kubernetes 提供开源的成本分配和监控能力，精确计算每个 Pod/Namespace/Cluster 的资源成本，帮助企业优化云支出。",
     "- **成本分配**：将云厂商账单精确拆分到 K8s 资源维度\n- **多厂商**：支持 AWS/GCP/Azure/Alibaba 等云厂商\n- **CNCF Sandbox**：Kubecost 开源核心\n- **Prometheus 集成**：基于 Prometheus 指标计算成本",
     "- 实时成本分配（Pod/Namespace/Cluster/Label 维度）\n- 云厂商价格 API 集成\n- 自定义价格表（私有云/裸金属）\n- 成本异常检测\n- OpenCost UI 可视化看板\n- Kubecost API 兼容\n- Helm Chart 一键部署",
     "- Kubernetes 集群的成本可视化\n- 多租户环境的成本分摊\n- 资源利用率优化\n- 云支出预算和告警\n- FinOps 实践的底层数据源",
     "- https://www.opencost.io/\n- https://github.com/opencost/opencost",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/kepler|Kepler]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]"),

    ("observability", "perses", "Perses 云原生仪表盘", "Perses",
     ["observability", "dashboard", "cncf"],
     "Perses 是 CNCF Sandbox 项目，云原生可观测性仪表盘工具，旨在提供 GitOps 友好的仪表盘管理方式，支持声明式定义仪表盘并通过 Git 进行版本管理。",
     "- **GitOps 仪表盘**：声明式 YAML 定义仪表盘，通过 Git 管理\n- **Prometheus 优先**：深度集成 Prometheus/Thanos 数据源\n- **CNCF Sandbox**：Grafana 的声明式替代方案\n- **可扩展**：插件式面板和主题",
     "- Dashboard CRD 声明式仪表盘\n- Datasource CRD 数据源管理\n- 支持 Prometheus/Thanos/Cortex 数据源\n- 变量（Variables）和模板系统\n- 面板（Panels）插件生态\n- Perses CLI 和 Web UI\n- 与 Perses Operator 集成",
     "- GitOps 方式的仪表盘管理\n- Grafana 的声明式替代\n- 多环境仪表盘的一致性管理\n- 可观测性平台的标准仪表盘\n- 仪表盘代码审查和版本控制",
     "- https://perses.dev/\n- https://github.com/perses/perses",
     "- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/observability/thanos|Thanos]]\n- [[系统基础/topic-dictionary/observability/grafana|Grafana]]"),

    ("observability", "pixie", "Pixie 自动可观测性", "Pixie",
     ["observability", "ebpf", "cncf"],
     "Pixie 是 New Relic 开源的 CNCF Sandbox 项目，利用 eBPF 技术实现 Kubernetes 应用的零插桩自动可观测性，无需修改应用代码即可获取请求追踪、性能指标和日志。",
     "- **eBPF 驱动**：自动采集应用级指标，无需代码改造\n- **零插桩**：自动追踪 HTTP/gRPC/MySQL/Redis 等协议\n- **本地分析**：数据在集群内处理，无需外传\n- **CNCF Sandbox**：New Relic 主导",
     "- Auto-telemetry：自动追踪 HTTP/gRPC/DNS/MySQL/PostgreSQL/Redis/Kafka\n- PxL 查询语言（类似 SQL 的数据查询）\n- Pixie Live View 实时数据查看\n- 脚本化分析（Scripted Analysis）\n- 数据保留策略（默认 24h 热数据）\n- 与 OpenTelemetry 导出集成",
     "- 无代码改造的应用可观测性\n- 微服务的请求级追踪\n- 性能瓶颈的快速定位\n- 遗留系统的可观测性接入\n- 开发环境的实时调试",
     "- https://px.dev/\n- https://github.com/pixie-io/pixie",
     "- [[系统基础/topic-dictionary/observability/opentelemetry|OpenTelemetry]]\n- [[系统基础/topic-dictionary/observability/jaeger|Jaeger]]\n- [[系统基础/topic-dictionary/networking/cilium|Cilium]]"),

    # ── Operations ──
    ("operations", "kube-burner", "kube-burner 性能测试", "kube-burner",
     ["operations", "performance", "testing"],
     "kube-burner 是 Cloud-Bulldozer 开源的 Kubernetes 性能测试和压力测试工具，通过声明式配置定义测试场景，用于评估 K8s 集群的规模性能和调度器行为。",
     "- **声明式测试**：YAML 定义测试场景（创建/删除/修补资源）\n- **大规模模拟**：支持创建数千个 Pod/Deployment 等\n- **指标收集**：自动采集 Prometheus 指标和 K8s 事件\n- **Cloud-Bulldozer**：Red Hat 性能测试工具集",
     "- Job 定义测试步骤（Create/Measure/Delete/Patch）\n- 模板化资源定义（Go template）\n- 内置指标采集（Prometheus/Grafana 集成）\n- 并发和速率控制\n- OpenShift/Kubernetes 兼容\n- 结果导出到 Elasticsearch/本地文件",
     "- Kubernetes 集群的基准性能测试\n- 调度器性能评估和优化\n- 大规模集群的容量规划\n- 升级前后的性能对比\n- CI/CD 中的性能回归测试",
     "- https://kube-burner.github.io/kube-burner/\n- https://github.com/kube-burner/kube-burner",
     "- [[系统基础/topic-dictionary/scheduling/scheduler|Scheduler]]\n- [[系统基础/topic-dictionary/observability/prometheus|Prometheus]]\n- [[系统基础/topic-dictionary/operations/chaos-engineering|混沌工程]]"),

    ("operations", "konveyor", "Konveyor 应用现代化", "Konveyor",
     ["operations", "migration", "modernization"],
     "Konveyor 是 Red Hat 开源的 CNCF Sandbox 项目，为应用现代化和迁移提供工具链，包括应用评估、代码分析和迁移规划，帮助企业将传统应用迁移到 Kubernetes 和云原生架构。",
     "- **应用评估**：评估应用的云原生就绪度和迁移复杂度\n- **代码分析**：自动扫描代码中的迁移问题\n- **CNCF Sandbox**：Red Hat MTA 的开源核心\n- **迁移规划**：生成详细的迁移路径和优先级",
     "- Tackle（Hub）：应用清单和迁移项目管理\n- Analyzer：基于规则的代码静态分析\n- Pathfinder：应用评估和风险评估\n- Move2Kube：自动化迁移工具\n- 丰富的规则集（Java/Spring/Jakarta EE 等）\n- 与 AI 集成的迁移建议",
     "- 传统 Java/Spring 应用到 K8s 的迁移\n- 应用组合分析和迁移优先级排序\n- 代码级别的迁移问题检测\n- 大规模应用现代化项目\n- 从 VM/传统部署到容器的转换",
     "- https://konveyor.io/\n- https://github.com/konveyor/konveyor",
     "- [[系统基础/topic-dictionary/tooling/buildpacks|Buildpacks]]\n- [[系统基础/topic-dictionary/platform-engineering/backstage|Backstage]]\n- [[系统基础/topic-dictionary/operations/k8sgpt|K8sGPT]]"),

    # ── Scheduling / AI ──
    ("scheduling", "kaito", "KAITO AI 推理调度", "KAITO",
     ["scheduling", "ai-ml", "inference"],
     "KAITO（Kubernetes AI Toolchain Operator）是微软开源的 CNCF Sandbox 项目，通过 Operator 简化 AI/ML 模型在 Kubernetes 上的部署和推理服务管理，自动化 GPU 资源分配和模型服务。",
     "- **模型部署**：声明式 CRD 定义 AI 模型推理服务\n- **自动化 GPU**：自动选择和配置 GPU 资源\n- **CNCF Sandbox**：微软主导\n- **预置模型**：内置主流开源模型的优化配置",
     "- Workspace CRD 定义推理工作空间\n- 预置模型模板（LLaMA/Falcon/Mistral/Phi 等）\n- 自动 GPU 配置（型号/内存/并发数）\n- 推理端点自动暴露\n- 模型版本管理和更新\n- 与 KEDA 集成的自动扩缩",
     "- LLM 推理服务的快速部署\n- GPU 资源的自动化管理\n- AI 模型服务的高可用部署\n- 多模型的统一管理平台\n- AI 开发团队的自助服务",
     "- https://github.com/Azure/kaito\n- https://kaito.sh/",
     "- [[系统基础/topic-dictionary/specialized-workloads/kserve|KServe]]\n- [[系统基础/topic-dictionary/specialized-workloads/ray|Ray]]\n- [[系统基础/topic-dictionary/specialized-workloads/kubeflow|Kubeflow]]"),

    # ── Storage ──
    ("storage", "composefs", "ComposeFS 只读文件系统", "ComposeFS",
     ["storage", "filesystem", "security"],
     "ComposeFS 是 Linux 内核的只读文件系统，基于内容寻址（content-addressed）存储，为容器镜像和不可变系统提供安全、高效的文件访问，与 OSTree 和 Podman 深度集成。",
     "- **内容寻址**：基于文件内容哈希的去重存储\n- **只读安全**：不可修改的文件系统，防止运行时篡改\n- **内核级**：Linux 内核模块，性能优异\n- **容器优化**：Podman/Buildah 的镜像存储后端",
     "- 基于 EROFS 的只读文件系统\n- 文件级去重（相同内容共享存储）\n- fs-verity 完整性验证\n- 与 OSTree 集成（Flatcar/Fedora CoreOS）\n- Podman ComposeFS 存储驱动\n- 支持 Overlayfs 作为底层",
     "- 不可变容器的安全文件系统\n- 容器镜像的存储优化（去重）\n- 不可变基础设施的根文件系统\n- 安全合规环境的防篡改存储\n- 大规模镜像拉取的性能优化",
     "- https://github.com/containers/composefs\n- https://docs.kernel.org/filesystems/composefs.html",
     "- [[系统基础/topic-dictionary/tooling/podman|Podman]]\n- [[系统基础/topic-dictionary/tooling/bootc|bootc]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|containerd]]"),

    # ── Platform Engineering ──
    ("platform-engineering", "cloudevents", "CloudEvents 事件标准", "CloudEvents",
     ["platform-engineering", "events", "cncf"],
     "CloudEvents 是 CNCF 毕业项目，定义了事件数据的通用格式规范，使不同系统和平台之间的事件交换标准化，是事件驱动架构和 Serverless 的基础设施标准。",
     "- **事件标准**：统一事件数据的格式（JSON/Protobuf/Avro）\n- **协议无关**：支持 HTTP、Kafka、AMQP、MQTT 等传输\n- **CNCF 毕业**：经过大规模生产验证\n- **广泛采用**：Knative/Azure/Google 等均采用",
     "- 事件属性：source/type/specversion/id/time/data\n- 多种数据编码（JSON/XML/Protobuf/Binary）\n- SDK 支持 Go/Java/JavaScript/Python/Rust/C#\n- 传输绑定（HTTP/Kafka/AMQP/MQTT/NATS）\n- CloudEvents Discovery 服务发现\n- 与 Knative Eventing 深度集成",
     "- Serverless 函数的事件触发\n- 微服务间的事件驱动通信\n- 多云事件路由和编排\n- IoT 设备事件的标准化\n- Knative Eventing 的事件源",
     "- https://cloudevents.io/\n- https://github.com/cloudevents/spec",
     "- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/platform-engineering/nats|NATS]]\n- [[系统基础/topic-dictionary/platform-engineering/dapr|Dapr]]"),

    ("platform-engineering", "openfeature", "OpenFeature 特性标志", "OpenFeature",
     ["platform-engineering", "feature-flags", "cncf"],
     "OpenFeature 是 CNCF 孵化项目，定义了特性标志（Feature Flags）的通用 API 标准，使应用代码与特性标志提供商解耦，支持 LaunchDarkly/Flagsmith/GO Feature Flag 等多种后端。",
     "- **API 标准**：统一的特性标志 API（不绑定特定提供商）\n- **多后端**：支持 LaunchDarkly/Flagsmith/GO Feature Flag/CloudBees 等\n- **CNCF 孵化**：社区驱动的特性标志标准化\n- **多语言 SDK**：Go/Java/JavaScript/Python/.NET 等",
     "- Client API（评估特性标志值）\n- Provider 接口（对接不同后端）\n- Evaluation Context（用户/环境上下文）\n- Hooks（日志/指标/追踪集成）\n- Targeting Rules（基于上下文的动态规则）\n- OFREP（OpenFeature Remote Evaluation Protocol）",
     "- 应用中的特性标志管理\n- A/B 测试和渐进式发布\n- 多提供商的特性标志统一管理\n- 开发者自助的特性控制\n- 与 CI/CD 集成的发布策略",
     "- https://openfeature.dev/\n- https://github.com/open-feature/spec",
     "- [[系统基础/topic-dictionary/operations/flagger|Flagger]]\n- [[系统基础/topic-dictionary/platform-engineering/argo|Argo]]\n- [[系统基础/topic-dictionary/operations/pipecd|PipeCD]]"),

    # ── Storage / DB ──
    ("storage", "tikv", "TiKV 分布式 KV 存储", "TiKV",
     ["storage", "database", "cncf"],
     "TiKV 是 PingCAP 开源的 CNCF 毕业项目，分布式事务键值存储引擎，为 TiDB 提供底层存储，同时也可独立使用，支持强一致性和水平扩展。",
     "- **分布式事务**：支持 ACID 事务（基于 Percolator 模型）\n- **Raft 共识**：数据多副本强一致性\n- **CNCF 毕业**：TiDB 生态的核心组件\n- **水平扩展**：自动分片和负载均衡",
     "- Multi-Raft Group 架构\n- MVCC 多版本并发控制\n- Coprocessor 下推计算\n- Raw KV（无事务的低延迟访问）\n- Titan（大 Value 优化存储引擎）\n- PD（Placement Driver）元数据管理",
     "- TiDB 的分布式存储后端\n- 需要强一致 KV 的微服务\n- 元数据存储和管理\n- 配置中心的底层存储\n- 替代 etcd 的大规模 KV 场景",
     "- https://tikv.org/\n- https://github.com/tikv/tikv",
     "- [[系统基础/topic-dictionary/storage/etcd|etcd]]\n- [[系统基础/topic-dictionary/storage/ceph|Ceph]]\n- [[系统基础/topic-dictionary/storage/vineyard|Vineyard]]"),

    ("storage", "vitess", "Vitess MySQL 分片", "Vitess",
     ["storage", "database", "cncf"],
     "Vitess 是 PlanetScale 开源的 CNCF 毕业项目，为 MySQL 提供水平扩展和分片能力，通过透明分片让应用无需修改即可扩展到多个 MySQL 实例，是 YouTube 的数据库基础设施。",
     "- **MySQL 兼容**：100% 兼容 MySQL 协议，应用无需修改\n- **透明分片**：自动将查询路由到正确的分片\n- **CNCF 毕业**：YouTube/GitHub/Slack 等使用\n- **在线迁移**：支持在线分片和数据迁移",
     "- VTGate（查询路由代理）\n- VTTablet（分片管理代理）\n- VSchema（分片规则定义）\n- MoveTables/Reshard（在线数据迁移）\n- 连接池和查询缓存\n- 自动故障转移和备份恢复",
     "- MySQL 数据库的水平扩展\n- 从单库到分片的在线迁移\n- 大规模 MySQL 集群管理\n- 需要 MySQL 兼容性的云原生数据库\n- 多租户数据库的分片隔离",
     "- https://vitess.io/\n- https://github.com/vitessio/vitess",
     "- [[系统基础/topic-dictionary/storage/tikv|TiKV]]\n- [[系统基础/topic-dictionary/storage/cloudnativepg|CloudNativePG]]\n- [[系统基础/topic-dictionary/storage/persistent-volumes|PV/PVC]]"),

    # ── Specialized Workloads ──
    ("specialized-workloads", "spinkube", "SpinKube WASM 运行时", "SpinKube",
     ["specialized-workloads", "wasm", "serverless"],
     "SpinKube 是 Fermyon 开源的 CNCF Sandbox 项目，在 Kubernetes 上运行 Spin WebAssembly 应用，通过 RuntimeClass 将 Wasm 工作负载与容器工作负载统一调度。",
     "- **Wasm on K8s**：在 K8s 上原生运行 WebAssembly 组件\n- **Spin 框架**：基于 Spin SDK 的 Serverless Wasm 应用\n- **CNCF Sandbox**：Fermyon 主导\n- **RuntimeClass**：通过 Kwasm 运行时类集成",
     "- SpinApp CRD 定义 Wasm 应用\n- 基于 Spin SDK 的多语言支持（Rust/Go/Python/JS）\n- Kwasm Operator 自动安装 Wasm runtime\n- 毫秒级冷启动\n- 与 K8s Service/Ingress 集成\n- 资源占用极低（KB 级内存）",
     "- Serverless 函数的 Wasm 运行时\n- 边缘计算的超轻量工作负载\n- 安全隔离的插件执行环境\n- 多语言微服务的统一运行时\n- 快速启动的 API 网关和中间件",
     "- https://www.spinkube.dev/\n- https://github.com/spinkube/spin-operator",
     "- [[系统基础/topic-dictionary/fundamentals/kata-containers|Kata Containers]]\n- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/specialized-workloads/openfaas|OpenFaaS]]"),

    ("specialized-workloads", "openyurt", "OpenYurt 边缘计算", "OpenYurt",
     ["specialized-workloads", "edge", "cncf"],
     "OpenYurt 是阿里巴巴开源的 CNCF Sandbox 项目，将 Kubernetes 能力扩展到边缘计算场景，解决云边网络不可靠、边缘自治和大规模边缘节点管理等挑战。",
     "- **云边协同**：云端管控 + 边缘自治的混合架构\n- **边缘自治**：云边断连时边缘节点独立运行\n- **CNCF Sandbox**：阿里巴巴主导\n- **无侵入**：对原生 K8s 零修改，渐进式扩展",
     "- YurtHub：边缘节点代理（缓存 + 自治）\n- YurtTunnel：云边安全通信通道\n- NodePool：边缘节点池管理\n- Raven：跨节点池网络打通\n- YurtAppSet：边缘应用分发\n- 与 KubeEdge 互补的边缘方案",
     "- CDN/IoT/零售等边缘场景\n- 云边网络不可靠环境的 K8s 管理\n- 大规模边缘节点（数千节点）管理\n- 边缘应用的统一分发和更新\n- 混合云/多云的边缘扩展",
     "- https://openyurt.io/\n- https://github.com/openyurtio/openyurt",
     "- [[系统基础/topic-dictionary/specialized-workloads/kubeedge|KubeEdge]]\n- [[系统基础/topic-dictionary/tooling/k3s|K3s]]\n- [[系统基础/topic-dictionary/platform-engineering/karmada|Karmada]]"),

    # ── Workloads / Serverless ──
    ("workloads", "serverless-workflow", "Serverless Workflow 编排", "Serverless Workflow",
     ["workloads", "serverless", "orchestration"],
     "Serverless Workflow 是 CNCF Sandbox 项目，定义了事件驱动工作流的声明式规范，使用 YAML/JSON 描述工作流逻辑，支持多种 Serverless 平台的执行。",
     "- **工作流标准**：定义事件驱动工作流的通用规范\n- **声明式**：YAML/JSON 描述工作流状态和转换\n- **CNCF Sandbox**：厂商中立的编排标准\n- **多平台**：可在 Knative/Apache Kogito/Azure 等平台执行",
     "- State/Transition 工作流模型\n- Event 触发和过滤\n- Action 执行（函数调用/事件发送/子流程）\n- Parallel/Foreach/Switch 控制流\n- Error/Retry/Timeout 处理\n- Compensation 补偿事务\n- SDK（Go/Java/TypeScript）",
     "- Serverless 应用的业务流程编排\n- 微服务间的复杂工作流协调\n- 事件驱动架构的流程管理\n- 长事务的 Saga 模式实现\n- 多云工作流的可移植定义",
     "- https://serverlessworkflow.io/\n- https://github.com/serverlessworkflow/specification",
     "- [[系统基础/topic-dictionary/specialized-workloads/knative|Knative]]\n- [[系统基础/topic-dictionary/platform-engineering/dapr|Dapr]]\n- [[系统基础/topic-dictionary/platform-engineering/nats|NATS]]"),

    # ── Tooling / Dev ──
    ("tooling", "kube-rs", "kube-rs Rust Operator SDK", "kube-rs",
     ["tooling", "operator", "rust"],
     "kube-rs 是 Rust 生态的 Kubernetes 客户端和 Operator 开发框架，提供类型安全的 K8s API 交互和 Controller 运行时，是 Rust 社区开发 K8s Operator 的首选工具。",
     "- **Rust 原生**：类型安全的 K8s API 客户端\n- **Controller 运行时**：提供 Informer/Reconciler 模式\n- **代码生成**：kube-derive 宏自动生成 CRD 代码\n- **社区活跃**：Rust K8s 生态的核心库",
     "- Client：类型安全的 K8s API 客户端（基于 k8s-openapi）\n- Controller：Reconciler 框架（类似 controller-runtime）\n- Runtime：Informer + 缓存管理\n- kube-derive：CRD 类型自动生成\n- 支持 Watch/List/Apply/Patch 等操作\n- 异步运行时（tokio）",
     "- Rust 编写 Kubernetes Operator\n- K8s API 的 Rust 客户端应用\n- 需要高性能的 K8s 控制器\n- CRD 的 Rust 类型生成\n- Rust 微服务与 K8s 的集成",
     "- https://kube.rs/\n- https://github.com/kube-rs/kube",
     "- [[系统基础/topic-dictionary/platform-engineering/operator-framework|Operator Framework]]\n- [[系统基础/topic-dictionary/tooling/kustomize|Kustomize]]\n- [[系统基础/topic-dictionary/fundamentals/youki|youki]]"),

    # ── Networking / Gateway ──
    ("networking", "kgateway", "KGateway API 网关", "KGateway",
     ["networking", "gateway", "envoy"],
     "KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于 Envoy Proxy，完整支持 Gateway API，提供丰富的流量管理和安全功能。",
     "- **Envoy 驱动**：基于 Envoy 的高性能网关\n- **Gateway API**：完整支持 Kubernetes Gateway API\n- **多协议**：HTTP/gRPC/WebSocket/TCP\n- **Solo.io**：企业级网关方案",
     "- Gateway API 完整实现\n- 路由规则和流量分割\n- 速率限制和熔断\n- TLS 终止和 mTLS\n- WAF（Web Application Firewall）集成\n- AI Gateway 功能（LLM 路由/Token 管理）\n- 与 Grafana/Prometheus 可观测性集成",
     "- Kubernetes 入口流量管理\n- API 网关和反向代理\n- 微服务的统一入口\n- Gateway API 的生产部署\n- AI 应用的 API 网关",
     "- https://kgateway.dev/\n- https://github.com/kgateway-dev/kgateway",
     "- [[系统基础/topic-dictionary/networking/envoy-gateway|Envoy Gateway]]\n- [[系统基础/topic-dictionary/networking/contour|Contour]]\n- [[系统基础/topic-dictionary/networking/traefik|Traefik]]"),

    # ── Configuration ──
    ("configuration", "composefs", "ComposeFS 镜像文件系统", "ComposeFS",
     ["configuration", "container", "filesystem"],
     "ComposeFS 是 Linux 内核级只读文件系统，通过内容寻址存储实现容器镜像的高效去重和完整性验证，是容器运行时和不可变系统的下一代存储基础。",
     "- **内容寻址**：基于文件内容哈希的去重存储\n- **只读安全**：内核级不可修改\n- **fs-verity**：集成文件完整性验证\n- **容器优化**：Podman/Buildah 原生支持",
     "- 基于 EROFS 的只读文件系统层\n- 文件级去重（共享相同内容）\n- fs-verity 签名验证\n- 与 OSTree 集成（系统镜像）\n- Podman ComposeFS 存储驱动\n- Overlay 友好的只读底层",
     "- 容器镜像的存储优化\n- 不可变系统的根文件系统\n- 安全环境的防篡改文件存储\n- 大规模部署的存储去重\n- 容器运行时的底层存储后端",
     "- https://github.com/containers/composefs\n- https://docs.kernel.org/filesystems/composefs.html",
     "- [[系统基础/topic-dictionary/tooling/podman|Podman]]\n- [[系统基础/topic-dictionary/tooling/bootc|bootc]]\n- [[系统基础/topic-dictionary/fundamentals/containerd|containerd]]"),
]

# Deduplicate (composefs appears twice, keep first)
seen = set()
unique_terms = []
for t in TERMS:
    key = t[1]  # filename
    if key not in seen:
        seen.add(key)
        unique_terms.append(t)
    else:
        print(f"  ! 跳过重复: {t[1]}")

TERMS = unique_terms

created, skipped = 0, 0
for t in TERMS:
    r = w(*t)
    if r:
        created += 1
        print(f"  + {t[0]}/{t[1]}.md")
    else:
        skipped += 1
        print(f"  = {t[0]}/{t[1]}.md (已存在)")

print(f"\n新创建: {created}, 跳过: {skipped}")
