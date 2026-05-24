---
title: Kubernetes 开源项目全景生态图谱
description: '| **Kubernetes** | 编排调度 | 2016.03 | v1.33.0 | 115k+ |'
category: general
tags:
- k8s
- etcd
- scheduler
- prometheus
- grafana
- jaeger
- istio
- envoy
- cilium
- flannel
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 开源项目全景生态图谱 是什么
- 如何 Kubernetes 开源项目全景生态图谱
trigger_keywords:
- Kubernetes
- 开源项目全景生态图谱
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

# Kubernetes 开源项目全景生态图谱

> **最后更新**: 2026-04-24  
> **数据来源**: CNCF Landscape, GitHub, 各项目官方 Release  
> **覆盖范围**: 41 个知识域、200+ 核心开源项目

---

## 📋 目录

- [一、CNCF 项目成熟度总览](#一cncf-项目成熟度总览)
- [二、按知识域分类的项目索引](#二按知识域分类的项目索引)
- [三、核心项目版本速查表](#三核心项目版本速查表)
- [四、2025-2026 重大里程碑](#四2025-2026-重大里程碑)
- [五、项目成熟度定义](#五项目成熟度定义)

---

## 一、CNCF 项目成熟度总览

截至 2026 年 Q1，CNCF 生态系统共托管约 **200+ 个开源项目**，累计 **30 万+ 贡献者**，来自 **190+ 个国家** 的 **11,500+ 组织**。

### 1.1 Graduated 项目 (32 个)

| 项目 | 类别 | 加入 CNCF | 当前版本 (2026.04) | GitHub Stars |
|:---|:---|:---|:---|:---|
| **Kubernetes** | 编排调度 | 2016.03 | v1.33.0 | 115k+ |
| **Prometheus** | 可观测性 | 2016.05 | v3.3.0 | 56k+ |
| **Envoy** | 服务代理 | 2017.09 | v1.33.0 | 25k+ |
| **CoreDNS** | 服务发现 | 2017.02 | v1.12.0 | 11k+ |
| **containerd** | 容器运行时 | 2017.03 | v2.0.4 | 17k+ |
| **Fluentd** | 可观测性 | 2016.11 | v1.17.1 | 12k+ |
| **Jaeger** | 可观测性 | 2017.09 | v2.5.0 | 20k+ |
| **Helm** | 应用定义 | 2018.06 | v3.17.0 | 27k+ |
| **Harbor** | 镜像仓库 | 2018.07 | v2.13.0 | 25k+ |
| **Rook** | 云原生存储 | 2018.01 | v1.16.0 | 12k+ |
| **etcd** | 协调服务 | 2018.12 | v3.5.21 | 48k+ |
| **Vitess** | 数据库 | 2018.02 | v21.0.0 | 18k+ |
| **TiKV** | 数据库 | 2018.08 | v8.5.0 | 15k+ |
| **Linkerd** | 服务网格 | 2017.01 | v2.18.0 | 10k+ |
| **Istio** | 服务网格 | 2022.09 | v1.29.0 | 36k+ |
| **Falco** | 安全 | 2018.10 | v0.41.0 | 7k+ |
| **OPA** | 安全 | 2018.03 | v1.3.0 | 9k+ |
| **SPIFFE/SPIRE** | 密钥管理 | 2018.03 | v1.11.0 | 4k+ |
| **TUF** | 安全 | 2017.10 | v4.0.0 | 3k+ |
| **in-toto** | 安全 | 2019.08 | v3.0.0 | 1k+ |
| **Argo** | CI/CD | 2020.03 | v3.3.8 (CD) | 17k+ |
| **Flux** | CI/CD | 2019.07 | v2.5.0 | 6k+ |
| **Cilium** | 云原生网络 | 2021.10 | v1.17.0 | 21k+ |
| **Dapr** | 应用运行时 | 2021.11 | v1.15.0 | 25k+ |
| **KEDA** | 自动伸缩 | 2020.03 | v2.17.0 | 8k+ |
| **KubeEdge** | 边缘计算 | 2019.03 | v1.20.0 | 7k+ |
| **CloudEvents** | 流式消息 | 2018.05 | v1.0.2 | 4k+ |
| **CRI-O** | 容器运行时 | 2019.04 | v1.33.0 | 5k+ |
| **Crossplane** | 基础设施编排 | 2020.06 | v1.19.0 | 10k+ |
| **CubeFS** | 云原生存储 | 2019.12 | v3.5.0 | 4k+ |
| **Dragonfly** | 镜像分发 | 2018.11 | v2.2.0 | 2k+ |
| **Knative** |  Serverless | 2022.03 | v1.18.0 | 4k+ |
| **[[cert-manager|cert-manager]]** | 安全 | 2020.11 | v1.17.0 | 12k+ |
| **Kyverno** | 安全 | 2020.11 | v1.14.0 | 5k+ |

### 1.2 Incubating 项目 (40+)

| 项目 | 类别 | 加入 CNCF | 当前版本 (2026.04) | 备注 |
|:---|:---|:---|:---|:---|
| **OpenTelemetry** | 可观测性 | 2019.05 | v1.28.0 | 贡献者增长 35%，生产采用率 49% |
| **Backstage** | 开发者门户 | 2020.09 | v1.36.0 | 贡献量翻倍增长 |
| **CNI** | 网络接口 | 2017.05 | v1.6.0 | 52% 生产采用率 |
| **Keycloak** | 身份认证 | 2023.04 | v26.0.5 | 组织支持、TLS 热重载 |
| **Kubeflow** | AI/ML | 2023.07 | v1.10.0 | AI 工作流编排 |
| **KServe** | AI 推理 | 2025.09 | v0.15.0 | 新晋 Incubating |
| **KubeVirt** | 虚拟化 | 2019.09 | v1.5.0 | 网络热插拔、GPU 分配 GA |
| **Longhorn** | 存储 | 2019.10 | v1.8.0 | 企业级分布式块存储 |
| **OpenCost** | 成本优化 | 2022.06 | v1.114.0 | FinOps 实践 |
| **gRPC** | RPC | 2017.02 | v1.71.0 | 44% 生产采用率 |
| **Contour** | Ingress | 2020.07 | v1.30.0 | Envoy 驱动的 Ingress |
| **Emissary-Ingress** | API 网关 | 2021.04 | v3.10.0 | 前身为 Ambassador |
| **Cortex** | 可观测性 | 2018.09 | v1.18.0 | Prometheus 长期存储 |
| **Thanos** | 可观测性 | - | v0.38.0 | 高可用 Prometheus 联邦 |
| **Chaos Mesh** | 混沌工程 | 2020.07 | v2.7.0 | 云原生混沌测试 |
| **Litmus** | 混沌工程 | 2020.06 | v3.12.0 | ChaosNative 主导 |
| **Karmada** | 多集群 | 2021.09 | v1.13.0 | 多云多集群调度 |
| **Fluid** | 数据编排 | 2021.04 | v1.0.6 | 数据集缓存加速 |
| **KubeVela** | 应用交付 | 2021.06 | v1.10.0 | OAM 模型实现 |
| **OpenKruise** | 应用管理 | 2020.11 | v1.8.0 | 高级工作负载 |
| **NATS** | 消息队列 | 2018.03 | v2.11.0 | 轻量级消息系统 |
| **Notary** | 镜像签名 | 2017.10 | v2.0.0 | 内容信任 |
| **Buildpacks** | 镜像构建 | 2018.10 | v0.36.0 | 云原生构建标准 |
| **Lima** | 容器运行时 | 2022.09 | v1.0.0 | macOS/Linux VM |
| **OpenYurt** | 边缘计算 | 2020.09 | v1.5.0 | 阿里云开源 |
| **Flatcar** | OS | 2024.08 | v4081.0.0 | 容器优化 Linux |
| **Cloud Custodian** | 治理 | 2020.06 | v0.9.42 | 多云治理 |
| **metal3-io** | 裸金属 | 2020.09 | v1.9.0 | 裸金属 K8s 管理 |
| **OpenFeature** | 特性开关 | 2022.06 | v1.14.0 | 标准化特性管理 |
| **OpenFGA** | 授权 | 2022.09 | v1.8.0 | 细粒度访问控制 |
| **Kubescape** | 安全 | 2022.12 | v3.0.30 | 合规扫描 |

---

## 二、按知识域分类的项目索引

### domain-1: 架构基础 (Architecture Fundamentals)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Kubernetes | 容器编排核心 | Graduated | v1.33.0 |
| Minikube | 本地单节点 K8s | 非 CNCF | v1.35.0 |
| kind | Docker 中运行 K8s | 非 CNCF | v0.27.0 |
| k3s | 轻量级 K8s 发行版 | 非 CNCF | v1.32.0 |
| kubeadm | 官方集群安装工具 | K8s 子项目 | v1.33.0 |
| KubeEdge | 边缘 K8s 方案 | Graduated | v1.20.0 |

### domain-2: 设计原理 (Design Principles)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| etcd | 分布式键值存储 | Graduated | v3.5.21 |
| CoreDNS | 集群 DNS | Graduated | v1.12.0 |

### domain-10: 扩展与自定义 (Extensions)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Helm | 包管理器 | Graduated | v3.17.0 |
| KubeVirt | 虚拟机管理 | Incubating | v1.5.0 |
| KubeVela | OAM 应用交付 | Incubating | v1.10.0 |
| OpenKruise | 高级工作负载 | Incubating | v1.8.0 |
| Operator SDK | Operator 开发框架 | K8s SIG | v1.39.0 |
| kro (Kube Resource Orchestrator) | CRD 编排 | AWS 开源 | v0.2.0 |
| kubebuilder | K8s API 构建框架 | K8s SIG | v4.5.0 |

### domain-11: AI 基础设施 (AI Infra)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Kubeflow | ML 工作流平台 | Incubating | v1.10.0 |
| KServe | 模型推理服务 | Incubating | v0.15.0 |
| Fluid | 数据集缓存编排 | Incubating | v1.0.6 |
| KubeRay | Ray on K8s | 非 CNCF | v1.3.0 |
| Volcano | 批处理调度 | 非 CNCF | v1.11.0 |
| NCCL / NVIDIA GPU Operator | GPU 调度与管理 | 非 CNCF | v24.9.0 |
| Dragonfly | 镜像 P2P 分发 | Graduated | v2.2.0 |

### domain-12: 问题排查 (Troubleshooting)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| kubectl / kubectx / kubens | 集群管理 CLI | K8s 生态 | - |
| Stern | 多 Pod 日志聚合 | 非 CNCF | v1.32.0 |
| K9s | 终端 K8s UI | 非 CNCF | v0.40.0 |
| Lens (OpenLens) | K8s IDE | 非 CNCF | v6.5.0 |
| kube-state-metrics | K8s 状态指标 | K8s SIG | v2.15.0 |
| node_exporter | 节点指标 | Prometheus | v1.9.0 |
| KubeSkray | 网络抓包分析 | 非 CNCF | v52.0.0 |
| Inspektor Gadget | eBPF 排查工具 | 非 CNCF | v0.38.0 |

### domain-13: Docker

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Docker Engine / Docker Desktop | 容器运行时 | 非 CNCF | v28.0.0 |
| containerd | 行业标准容器运行时 | Graduated | v2.0.4 |
| CRI-O | K8s 专用容器运行时 | Graduated | v1.33.0 |
| Podman | 无守护进程容器工具 | 申请 Sandbox | v5.4.0 |
| Buildah | OCI 镜像构建 | 申请 Sandbox | v1.39.0 |
| Skopeo | 镜像远程操作 | 申请 Sandbox | v1.17.0 |
| nerdctl | containerd CLI | 非 CNCF | v2.0.4 |
| Lima | macOS/Linux 容器 VM | Incubating | v1.0.0 |
| composefs | 只读文件系统层 | 申请 Sandbox | - |
| bootc | OCI 镜像 OS 更新 | 申请 Sandbox | v1.1.0 |

### domain-15: 网络基础

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| CNI | 容器网络接口标准 | Incubating | v1.6.0 |
| Cilium | eBPF 网络与安全 | Graduated | v1.17.0 |
| Calico | L3 网络与策略 | 非 CNCF | v3.29.0 |
| Flannel | 简单 overlay 网络 | 非 CNCF | v0.26.0 |
| OVN-Kubernetes | OVN 网络方案 | Sandbox | v1.0.0 |

### domain-16: 存储基础

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Rook | 云原生存储编排 | Graduated | v1.16.0 |
| Longhorn | 分布式块存储 | Incubating | v1.8.0 |
| CubeFS | 分布式文件存储 | Graduated | v3.5.0 |
| OpenEBS | K8s 原生存储 | 非 CNCF | v4.2.0 |
| Vitess | MySQL 水平扩展 | Graduated | v21.0.0 |

### domain-17: 云厂商 (Cloud Providers)

| 项目 | 作用 | 提供商 | 备注 |
|:---|:---|:---|:---|
| EKS Distro | AWS K8s 发行版 | AWS | 开源发行版 |
| aks-engine / AKS | Azure K8s | Microsoft | 托管服务 |
| GKE / Autopilot | Google K8s | Google | 托管服务 |
| ACK / OpenYurt | 阿里云 K8s | 阿里云 | OpenYurt Incubating |
| TKE / TKEStack | 腾讯云 K8s | 腾讯 | TKEStack 开源 |
| Volcengine veStack | 火山引擎 K8s | 字节跳动 | 企业级发行版 |

### domain-18: 生产运维 (Production Operations)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| KEDA | 事件驱动自动伸缩 | Graduated | v2.17.0 |
| OpenCost | K8s 成本可视化 | Incubating | v1.114.0 |
| Keptn | 应用生命周期编排 | 非 CNCF | v2.4.0 |
| Cluster API | 声明式集群管理 | K8s SIG | v1.9.0 |
| Karpenter | 节点自动伸缩 | AWS 开源 | v1.3.0 |
| Descheduler | Pod 重调度 | K8s SIG | v0.32.0 |
| Vertical Pod Autoscaler | 垂直伸缩 | K8s SIG | v1.3.0 |
| Karmada | 多云多集群调度 | Incubating | v1.13.0 |

### domain-19: 论文与参考 (Papers)

> 本域为学术与最佳实践文档，主要关联项目见其他域。

### domain-20: 企业监控告警 (Enterprise Monitoring)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Prometheus | 时序监控与告警 | Graduated | v3.3.0 |
| Grafana | 可视化与仪表盘 | 非 CNCF | v11.6.0 |
| Thanos | Prometheus 长期存储 | 非 CNCF | v0.38.0 |
| Cortex | 多租户指标存储 | Incubating | v1.18.0 |
| Mimir | Grafana 企业级指标后端 | 非 CNCF | v3.0.0 |
| kube-state-metrics | K8s 资源状态指标 | K8s SIG | v2.15.0 |
| node_exporter | 主机指标导出 | Prometheus | v1.9.0 |
| Alertmanager | 告警路由管理 | Prometheus | v0.28.0 |
| VictoriaMetrics | 高性能时序数据库 | 非 CNCF | v1.115.0 |
| cAdvisor | 容器资源分析 | K8s SIG | v0.51.0 |
| Kiali | 服务网格可视化 | 非 CNCF | v2.7.0 |

### domain-21: 日志管理与分析 (Logging)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Fluentd | 统一日志收集 | Graduated | v1.17.1 |
| Fluent Bit | 轻量级日志转发 | 非 CNCF | v3.2.0 |
| Loki | Grafana 日志聚合 | 非 CNCF | v3.4.0 |
| ELK Stack | Elasticsearch+Logstash+Kibana | 非 CNCF | v8.17.0 |
| OpenSearch | AWS 开源搜索分叉 | 非 CNCF | v2.19.0 |
| Graylog | 企业日志管理 | 非 CNCF | v6.1.0 |
| Vector | 可观测性数据管道 | 非 CNCF | v0.46.0 |
| Pluralith / OTEL Collector | 标准化采集 | CNCF | v0.121.0 |

### domain-22: 容器镜像管理 (Image Management)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Harbor | 企业镜像仓库 | Graduated | v2.13.0 |
| Dragonfly | P2P 镜像分发 | Graduated | v2.2.0 |
| Notary | 镜像内容信任 | Incubating | v2.0.0 |
| cosign (Sigstore) | 镜像签名验证 | OpenSSF | v2.4.0 |
| Syft | SBOM 生成 | Anchore | v1.22.0 |
| Trivy | 镜像漏洞扫描 | Aqua | v0.61.0 |
| Quay | Red Hat 镜像仓库 | Red Hat | v3.14.0 |
| JFrog Artifactory | 通用制品库 | JFrog | 企业版 |
| GitLab Container Registry | 集成镜像仓库 | GitLab | v17.10.0 |
| Amazon ECR | AWS 托管仓库 | AWS | 托管服务 |

### domain-23: GitOps & CI/CD

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Argo CD | 声明式 GitOps CD | Graduated | v3.3.8 |
| Argo Workflows | 工作流引擎 | Graduated | v3.6.0 |
| Argo Rollouts | 渐进式交付 | Graduated | v1.8.0 |
| Argo Events | 事件驱动自动化 | Graduated | v1.9.0 |
| Flux | GitOps 持续交付 | Graduated | v2.5.0 |
| Flagger | 渐进式发布 | Flux 生态 | v1.40.0 |
| Jenkins | CI/CD 服务器 | CDF | v2.492.0 |
| GitLab CI | 集成 CI/CD | GitLab | v17.10.0 |
| GitHub Actions | 托管 CI/CD | GitHub | - |
| Tekton | 云原生 CI/CD 框架 | CDF | v0.65.0 |
| KubeSphere DevOps | 集成 DevOps 平台 | 非 CNCF | v4.1.0 |

### domain-24: 基础设施即代码 (IaC)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Terraform | 多云 IaC | HashiCorp | v1.11.0 |
| OpenTofu | Terraform 开源分叉 | Linux 基金会 | v1.10.0 |
| Crossplane | K8s 原生 IaC | Graduated | v1.19.0 |
| Pulumi | 编程式 IaC | Pulumi | v3.160.0 |
| Ansible | 配置管理 | Red Hat | v2.18.0 |
| Azure Resource Manager | Azure 原生 IaC | Microsoft | - |
| AWS CDK | AWS 云开发套件 | AWS | v2.188.0 |
| Crossplane Providers | 云厂商 Provider | Crossplane | 最新 |

### domain-25: 云原生安全 (Security)

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Falco | 运行时安全监控 | Graduated | v0.41.0 |
| OPA | 通用策略引擎 | Graduated | v1.3.0 |
| Kyverno | K8s 原生策略管理 | Graduated | v1.14.0 |
| cert-manager | 自动化 TLS | Graduated | v1.17.0 |
| SPIFFE/SPIRE | 工作负载身份 | Graduated | v1.11.0 |
| TUF / in-toto | 软件供应链安全 | Graduated | v4.0/v3.0 |
| Vault (OSS) | 密钥管理 | HashiCorp | v1.19.0 |
| External Secrets | 外部密钥同步 | 非 CNCF | v0.15.0 |
| Sealed Secrets | GitOps 加密密钥 | 非 CNCF | v0.28.0 |
| Kubescape | 合规与风险评估 | Incubating | v3.0.30 |
| Trivy | 漏洞扫描 | Aqua | v0.61.0 |
| Snyk | 安全扫描 | Snyk | 企业版 |
| Aqua Enterprise | 容器安全 | Aqua | 企业版 |
| Sysdig | 运行时安全 | Sysdig | 企业版 |
| Notary / cosign | 镜像签名 | CNCF/OpenSSF | v2.0/v2.4 |

### domain-26: 服务网格与微服务

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Istio | 服务网格 | Graduated | v1.29.0 |
| Linkerd | 轻量级服务网格 | Graduated | v2.18.0 |
| Cilium Service Mesh | eBPF 服务网格 | Graduated | v1.17.0 |
| Envoy | L7 代理与网关 | Graduated | v1.33.0 |
| Consul Connect | HashiCorp 服务网格 | HashiCorp | v1.20.0 |
| Dapr | 分布式应用运行时 | Graduated | v1.15.0 |
| Kuma | Envoy 服务网格 | Kong | v2.10.0 |
| NGINX Service Mesh | NGINX 服务网格 | F5 | 已归档 |
| OSM (Open Service Mesh) | SMI 实现 | 非 CNCF | 已归档 |

### domain-27: 多云与混合云

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Karmada | 多云多集群调度 | Incubating | v1.13.0 |
| Cluster API | 声明式集群生命周期 | K8s SIG | v1.9.0 |
| Rancher | 多集群管理平台 | SUSE | v2.10.0 |
| Fleet | Rancher GitOps 多集群 | Rancher | v0.12.0 |
| Kamaji | 托管 K8s 控制平面 | Clastix | v1.0.0 |
| vCluster | 虚拟集群 | Loft | v0.24.0 |
| Admiralty | 多集群调度 | 非 CNCF | v0.15.0 |

### domain-28: 企业数据库与中间件

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| Vitess | MySQL 水平扩展 | Graduated | v21.0.0 |
| TiKV | 分布式 KV 存储 | Graduated | v8.5.0 |
| TiDB | 分布式 HTAP 数据库 | PingCAP | v9.0.0 |
| CockroachDB | 云原生分布式 SQL | Cockroach Labs | v25.1.0 |
| YugabyteDB | 云原生分布式 SQL | Yugabyte | v2024.2.0 |
| CloudNativePG | K8s PostgreSQL 运维 | EDB | v1.25.0 |
| Strimzi | K8s Kafka 运维 | CNCF Sandbox申请 | v0.45.0 |
| Redpanda | K8s-native Kafka 替代 | Redpanda | v24.3.0 |
| Pulsar | 云原生消息流 | Apache | v4.0.0 |
| Apache Cassandra | 分布式宽列存储 | Apache | v5.0.0 |
| Debezium | CDC 变更数据捕获 | Red Hat | v3.0.0 |

### domain-29: 自动化测试与质量

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Trivy | 漏洞与合规扫描 | Aqua | v0.61.0 |
| Checkov | IaC 安全扫描 | Bridgecrew | v3.2.0 |
| Kube-bench | CIS K8s Benchmark | Aqua | v0.10.0 |
| Polaris | K8s 最佳实践验证 | Fairwinds | v9.0.0 |
| Popeye | K8s 集群卫生检查 | Derailed | v0.22.0 |
| Kube-score | K8s 对象静态分析 | Zegl | v1.19.0 |
| k6 | 负载测试 | Grafana | v0.56.0 |
| Locust | Python 负载测试 | Locust | v2.32.0 |
| Testcontainers | 集成测试容器框架 | AtomicJar | v1.20.0 |
| Kuttl | K8s 声明式测试 | K8s SIG | v0.21.0 |
| Chainsaw | K8s 增强测试工具 | Kyverno | v0.6.0 |

### domain-30: 灾备与业务连续性

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Velero | K8s 集群备份与恢复 | VMware | v1.15.0 |
| Kasten K10 | K8s 数据保护 | Veeva | v7.5.0 |
| Stash | K8s 备份恢复 (Restic) | AppsCode | v2024.12.0 |
| etcd-druid | etcd 生命周期与备份 | Gardener | v0.27.0 |
| Chaos Mesh | 混沌工程 | Incubating | v2.7.0 |
| Litmus | 混沌工程 | Incubating | v3.12.0 |

### domain-31: 硬件与裸金属

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| MetalLB | 裸金属 LoadBalancer | 社区 | v0.14.0 |
| kube-vip | 高可用虚拟 IP | 社区 | v0.8.0 |
| Cluster API Provider Metal3 | 裸金属 K8s 管理 | Incubating | v1.9.0 |
| Tinkerbell | 裸金属工作流引擎 | 社区 | v0.10.0 |
| NVIDIA GPU Operator | GPU 驱动与管理 | NVIDIA | v24.9.0 |
| Node Feature Discovery | 硬件特性发现 | K8s SIG | v0.17.0 |
| DPDK | 数据平面开发套件 | Intel/Linux | v24.11.0 |
| SR-IOV Network Operator | SR-IOV 网络虚拟化 | Intel | v1.4.0 |

### domain-32: YAML 配置与清单工具

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| kustomize | K8s 原生配置定制 | K8s SIG | v5.6.0 |
| Helm | 包管理与模板 | Graduated | v3.17.0 |
| Helmfile | 声明式 Helm 部署 | 社区 | v1.0.0 |
| yq | YAML 命令行处理器 | Mike Farah | v4.45.0 |
| kubeconform | K8s YAML 验证 | 社区 | v0.6.7 |
| Conftest | OPA 策略验证 | OPA | v0.57.0 |
| kubectl-neat | 清理 YAML 冗余字段 | 社区 | v2.0.0 |
| cuelang | 配置语言与验证 | CUE | v0.12.0 |
| Jsonnet / Tanka | 数据模板语言 | Google/Grafana | v0.20.0/v0.30.0 |
| DevSpace | K8s 开发工作流 | Loft | v6.3.0 |
| Tilt | 本地 K8s 开发 | Tilt.dev | v0.33.0 |
| Okteto | 云端开发环境 | Okteto | v3.5.0 |
| DevPod | 开源 Codespaces 替代 | Loft | v0.6.0 |
| mirrord | 本地代码接入集群 | MetalBear | v3.0.0 |
| telepresence | 本地开发流量拦截 | Ambassador | v2.22.0 |

### domain-33: K8s 事件与审计

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Kubernetes Events | 原生事件系统 | K8s 核心 | v1.33.0 |
| Sloop | K8s 历史状态查看 | Salesforce | v1.0.0 |
| Kubernetes Event Exporter | 通用事件导出 | Resmo | v1.7.0 |
| Policy Reporter | 策略结果展示 | Kyverno | v3.0.0 |
| Komodor | 变更追踪与事件关联 | Komodor | SaaS |

### domain-35: eBPF 技术

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Cilium | eBPF 网络与安全 | Graduated | v1.17.0 |
| Falco | eBPF 运行时安全 | Graduated | v0.41.0 |
| Pixie | K8s 可观测性 (eBPF) | New Relic | v0.14.0 |
| Inspektor Gadget | eBPF 排查工具集 | 社区 | v0.38.0 |
| Tetragon | eBPF 安全可观测性 | Cilium | v1.3.0 |
| BCC | eBPF 编译工具集 | IOVisor | v0.31.0 |
| bpftrace | eBPF 高级追踪语言 | 社区 | v0.22.0 |
| Grafana Beyla | eBPF 应用自动可观测性 | Grafana | v2.0.0 |
| Parca | 持续性能分析 (eBPF) | Polar Signals | v0.23.0 |
| Caretta | K8s 网络映射 (eBPF) | Groundcover | v1.0.0 |
| L3AF | eBPF 应用框架 | Linux Foundation | v1.0.0 |
| ebpf_exporter | eBPF Prometheus 指标 | Cloudflare | v2.0.0 |

### domain-36: 平台工程

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Backstage | 开发者门户 (IDP) | Incubating | v1.36.0 |
| Crossplane | K8s 原生平台构建 | Graduated | v1.19.0 |
| KubeVela | OAM 应用交付平台 | Incubating | v1.10.0 |
| KusionStack | IDP 平台编排器 | CNCF Sandbox | v0.14.0 |
| Kratix | 平台框架 (K8s-native) | Syntasso | v0.12.0 |
| Score | 工作负载规范 | Humanitec | v0.16.0 |
| CNOE | Cloud Native Operational Excellence | 社区 | - |
| Kubeapps | K8s 应用仪表板 | VMware | v17.0.0 |
| Nitric | 云原生开发框架 | Nitric | v1.0.0 |
| Port | 开发者门户 | Port | SaaS |
| Humanitec | 平台编排引擎 | Humanitec | SaaS |

### domain-37: 边缘计算

| 项目 | 作用 | CNCF 状态 | 版本 |
|:---|:---|:---|:---|
| KubeEdge | 边缘 K8s 方案 | Graduated | v1.20.0 |
| OpenYurt | 阿里云边缘扩展 | Incubating | v1.5.0 |
| SuperEdge | 腾讯边缘容器 | 非 CNCF | v0.8.0 |
| Akri | 边缘设备自动发现 | 非 CNCF | v0.13.0 |
| k3s | 轻量级 K8s 发行版 | 非 CNCF | v1.32.0 |
| EdgeX Foundry | 通用边缘平台 | LF Edge | v4.0.0 |
| WasmEdge | 边缘 WebAssembly 运行时 | Sandbox | v0.14.0 |
| EMQ X | 边缘 MQTT broker | EMQ | v5.8.0 |
| Eclipse Kanto | 边缘设备管理 | Eclipse | v1.0.0 |
| EdgeFarm | 边缘应用平台 | 社区 | v1.0.0 |

### domain-38: WebAssembly 云原生

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| WasmEdge | 轻量级 Wasm 运行时 | CNCF Sandbox | v0.14.0 |
| wasmCloud | 分布式 Wasm 应用平台 | Incubating | v1.5.0 |
| Spin | 开发者友好的 Wasm 框架 | Fermyon | v3.2.0 |
| SpinKube | K8s 上的 Spin 运行时 | SpinKube | v0.6.0 |
| runwasi | containerd Wasm shim | containerd | v0.8.0 |
| WAMR | 轻量 Wasm 微运行时 | Bytecode Alliance | v2.2.0 |
| Fermyon Cloud | Wasm PaaS | Fermyon | SaaS |
| Cosmonic | wasmCloud PaaS | Cosmonic | SaaS |

### domain-39: 供应链安全

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Sigstore | 软件签名生态 | OpenSSF | - |
| cosign | 容器镜像签名 | OpenSSF | v2.4.0 |
| in-toto | 供应链完整性 | Graduated | v3.0.0 |
| TUF | 安全更新框架 | Graduated | v4.0.0 |
| SLSA | 供应链安全框架 | OpenSSF | v1.1.0 |
| Syft | SBOM 生成 | Anchore | v1.22.0 |
| Grype | 漏洞扫描 | Anchore | v0.87.0 |
| GUAC | 软件供应链知识图谱 | OpenSSF | v0.13.0 |
| Scorecard | 开源项目安全评分 | OpenSSF | v5.0.0 |
| Sigstore policy-controller | K8s 签名策略验证 | Sigstore | v0.11.0 |
| Tekton Chains | CI/CD 供应链安全 | CDF | v0.24.0 |
| SOPS | YAML/JSON 加密 | Mozilla | v3.9.0 |
| Checkov | IaC 安全扫描 | Bridgecrew | v3.2.0 |

### domain-40: 云原生 API 网关

| 项目 | 作用 | 归属 | 版本 |
|:---|:---|:---|:---|
| Envoy | L7 代理与网关标准 | Graduated | v1.33.0 |
| Ingress NGINX | K8s Ingress 控制器 | K8s SIG | v1.12.0 |
| Emissary-Ingress | API 网关 | Incubating | v3.10.0 |
| Contour | Envoy Ingress | Incubating | v1.30.0 |
| Envoy Gateway | 官方 Envoy K8s 网关 | Envoy | v1.3.0 |
| Kong Gateway | API 网关 | Kong | v3.9.0 |
| Traefik | 云原生反向代理 | Traefik Labs | v3.3.0 |
| Apache APISIX | 动态 API 网关 | Apache | v3.11.0 |
| Tyk | 开源 API 网关 | Tyk | v5.7.0 |
| Solo Gloo | Envoy 网关 | Solo.io | v1.18.0 |
| Spring Cloud Gateway | Spring 生态网关 | VMware | v4.3.0 |
| Gateway API | K8s 流量管理新标准 | K8s SIG | v1.2.0 |

### 其他重要交叉领域项目

| 项目 | 作用 | 归属 | 版本 | 领域 |
|:---|:---|:---|:---|:---|
| SOPS | YAML/JSON 加密 (GitOps 密钥管理) | Mozilla | v3.9.0 | 安全 / GitOps |
| Kubewarden | Rust 编写 K8s 策略引擎 | Rancher | v1.23.0 | 安全 |
| jsPolicy | JavaScript K8s 策略引擎 | Loft | v0.3.0 | 安全 |
| NeuVector | 容器安全平台 (SUSE) | SUSE | v5.4.0 | 安全 |
| External Secrets | 外部密钥同步到 K8s | 社区 | v0.15.0 | 安全 / GitOps |
| Sealed Secrets | GitOps 加密密钥 | Bitnami | v0.28.0 | 安全 / GitOps |
| Reloader | ConfigMap/Secret 变更自动重启 Pod | Stakater | v1.3.0 | 运维 |
| Descheduler | Pod 重调度优化 | K8s SIG | v0.32.0 | 调度 |
| Kueue | K8s 作业队列管理 | K8s SIG | v0.11.0 | 调度 |
| Scheduler-plugins | K8s 调度器扩展插件 | K8s SIG | v0.30.0 | 调度 |
| Multus | 多 CNI 网络接口 | 社区 | v4.1.0 | 网络 |
| Whereabouts | IPAM CNI 插件 | 社区 | v0.8.0 | 网络 |
| TopoLVM | LVM 本地存储 CSI | Cybozu | v0.30.0 | 存储 |
| SeaweedFS | 分布式对象/文件存储 | 社区 | v3.80.0 | 存储 |
| Spinnaker | 多云持续交付平台 | Netflix/Armory | v1.37.0 | CI/CD |
| Concourse CI | 声明式 CI/CD 管道 | VMware | v7.12.0 | CI/CD |
| Woodpecker CI | 轻量级 CI/CD (Drone fork) | 社区 | v3.0.0 | CI/CD |
| Argo CD Image Updater | Argo CD 镜像自动更新 | Argo | v0.15.0 | GitOps |
| Renovate | 自动化依赖更新 | Mend | v39.0.0 | DevOps |
| Capsule | K8s 多租户框架 | Clastix | v0.7.0 | 多租户 |
| HNC | 层级命名空间 | K8s SIG | v1.2.0 | 多租户 |
| OpenFunction | 云原生函数计算框架 | CNCF Sandbox | v1.2.0 | Serverless |
| Fission | K8s Serverless 框架 | Platform9 | v1.20.0 | Serverless |
| Nuclio | 高性能 Serverless | Iguazio | v1.13.0 | Serverless |
| Kubecost | K8s 成本管理与优化 | Kubecost | v2.7.0 | FinOps |
| Infracost | 云成本估算 (Terraform) | Infracost | v0.11.0 | FinOps |
| Sentry | 错误追踪与监控 | Functional Software | v25.0.0 | 可观测性 |
| Netdata | 实时系统监控 | Netdata | v2.4.0 | 可观测性 |
| Uptrace | APM 与分布式追踪 | Uptrace | v1.7.0 | 可观测性 |
| SigNoz | 开源 APM (替代 Datadog) | SigNoz | v0.76.0 | 可观测性 |
| OPA Gatekeeper | K8s 准入策略控制器 | OPA | v3.18.0 | 安全 |
| ORAS | OCI Registry As Storage | CNCF Sandbox | v1.3.0 | 镜像 / 制品 |
| Artifact Hub | K8s 包发现平台 | Incubating | - | 制品 |
| CloudNativePG | K8s PostgreSQL 运维 | EDB | v1.25.0 | 数据库 |
| Percona XtraDB Operator | MySQL/MariaDB K8s 运维 | Percona | v1.17.0 | 数据库 |
| MongoDB Community Operator | MongoDB K8s 运维 | MongoDB | v0.12.0 | 数据库 |
| Redis Operator (OT-CONTAINER-KIT) | Redis Cluster K8s 运维 | OT-CONTAINER-KIT | v0.19.0 | 数据库 |
| Zalando Postgres Operator | PostgreSQL K8s 运维 | Zalando | v1.14.0 | 数据库 |
| Banzai Cloud Kafka Operator | Kafka K8s 运维 | Cisco | v0.25.0 | 消息队列 |
| RocketMQ Operator | RocketMQ K8s 运维 | Apache | v0.3.0 | 消息队列 |
| Pulsar Operator | Pulsar K8s 运维 | StreamNative | v0.22.0 | 消息队列 |
| AWS App Mesh | AWS 托管服务网格 | AWS | - | 服务网格 |
| Solo Gloo Mesh | Istio 多集群管理 | Solo.io | v2.7.0 | 服务网格 |
| **Higress** | 阿里云云原生 API 网关 (Envoy + WASM) | 阿里云 | v2.1.0 | API 网关 |
| **Kmesh** | eBPF 内核级服务网格 (华为) | 华为 | v1.0.0 | 服务网格 / eBPF |
| **Sermant** | Proxyless 服务网格 (华为) | 华为 | v2.0.0 | 服务网格 |
| **Meshery** | 服务网格管理平台 (CNCF Sandbox) | Layer5 | v0.8.0 | 服务网格 |
| **Kepler** | K8s 能耗监控与可持续计算 (CNCF Sandbox) | 红帽/社区 | v0.7.0 | GreenOps / 可观测性 |
| **Kube-green** | 非工作时间自动关闭开发环境 | 社区 | v0.6.0 | GreenOps / 运维 |
| **Scaphandre** | 软件电力消耗测量 | 社区 | v1.0.0 | GreenOps / 可观测性 |
| **Robusta** | K8s 自动化运维与告警响应 | 社区 | v0.20.0 | 运维 / 可观测性 |
| **Botkube** | K8s 事件通知机器人 (Slack/Teams) | Kubeshop | v1.15.0 | 运维 / 可观测性 |
| **Trivy Operator** | K8s 持续漏洞扫描 | Aqua | v0.26.0 | 安全 |
| **Trust-manager** | cert-manager 信任包分发 | cert-manager | v0.15.0 | 安全 |
| **Validating Admission Policy (VAP)** | K8s 原生准入策略 (v1.30+) | K8s 核心 | v1.33.0 | 安全 |
| **Higress** | 阿里云云原生 API 网关 | 阿里云 | v2.1.0 | API 网关 |
| **Alluxio** | 数据编排层 (类似 Fluid) | Alluxio | v2.9.0 | 数据 / AI |
| **OpenObserve** | 轻量级云原生可观测性平台 | 社区 | v0.14.0 | 可观测性 |
| **Parseable** | 云原生日志分析 (Loki 替代) | 社区 | v1.0.0 | 日志 |
| **Dagger** | 可编程 CI/CD 管道 | Dagger | v0.18.0 | CI/CD / DevOps |
| **Garden** | 开发编排与测试加速 | Garden | v0.13.0 | 开发工具 |
| **Nocalhost** | 云原生开发环境 (中国) | Nocalhost | v0.6.0 | 开发工具 |
| **KubeSphere** | 企业级容器平台 | 青云 | v4.1.0 | 平台 / 运维 |
| **KubeVirt** | VM 作为 K8s 工作负载 (Incubating) | CNCF | v1.5.0 | 虚拟化 / 工作负载 |
| **KrakenD** | 高性能 API 网关 | KrakenD | v2.9.0 | API 网关 |
| **Vcluster** | 虚拟集群 | Loft | v0.24.0 | 多集群 / 多租户 |
| **Liqo** | 多集群资源联邦与共享 | 社区 | v1.0.0 | 多集群 |
| **VirtualCluster** | 多租户扩展 API Server | 社区 | v0.4.0 | 多租户 |
| **K8spin** | Namespace 即服务 | 社区 | v0.1.0 | 多租户 |
| **Apache YuniKorn** | K8s 批处理调度器 | Apache | v1.6.0 | 调度 |
| **Kata Containers** | 轻量级 VM 容器运行时 | Kata | v3.14.0 | 容器运行时 / 安全 |
| **gVisor** | 用户空间内核容器沙箱 | Google | v2025.04.0 | 容器运行时 / 安全 |
| **Firecracker** | AWS MicroVM | AWS | v1.10.0 | 容器运行时 / 虚拟化 |
| **Youki** | Rust 容器运行时 | 社区 | v0.5.0 | 容器运行时 |
| **NAD (Network Attachment Definition)** | Multus 网络定义标准 | K8s SIG | - | 网络 |
| **SR-IOV CNI** | SR-IOV 网络 CNI | Intel | v2.8.0 | 网络 |
| **DANM** | Nokia Telco CNI | Nokia | v4.3.0 | 网络 |
| **Spiderpool** | Underlay CNI for K8s | 社区 | v1.0.0 | 网络 |
| **Merbridge** | eBPF 加速 Istio/Envoy | 社区 | v0.9.0 | 服务网格 / eBPF |
| **Aeraki** | Istio 七层流量管理扩展 | 社区 | v1.4.0 | 服务网格 |
| **Slime** | Istio 智能服务管理框架 | 网易 | v1.0.0 | 服务网格 |
| **EaseMesh** | 基于 Istio 的易用服务网格 | 社区 | v2.0.0 | 服务网格 |
| **Kraken** | Uber P2P Docker 镜像分发 | Uber | v0.2.0 | 镜像分发 |
| **Dragonfly** | 已在 Graduated 列表 | - | - | - |
| **SlimToolkit** (原 DockerSlim) | 容器镜像最小化 | Slim.AI | v1.40.0 | 镜像 / 安全 |
| **Kaniko** | 无守护进程容器构建 | Google | v1.23.0 | 镜像构建 |
| **BuildKit** | Docker 构建引擎 | Docker/Moby | v0.20.0 | 镜像构建 |
| **Dive** | Docker 镜像层分析 | 社区 | v0.13.0 | 镜像分析 |
| **Skaffold** | K8s 本地开发持续构建 | Google | v2.15.0 | 开发工具 |
| **Telepresence** | 已在开发工具列表 | - | - | - |
| **Infra** | 基础设施访问管理 (K8s SSH) | Infra | v0.21.0 | 安全 / 访问 |
| **Teleport** | 安全基础设施访问 | Gravitational | v17.0.0 | 安全 / 访问 |
| **Teller** | 开发者密钥管理工具 | Spectral | v2.0.0 | 安全 / 开发 |
| **TruffleHog** | Git 凭证泄露扫描 | Truffle Security | v3.0.0 | 安全 / 供应链 |
| **Gitleaks** | 保护与发现 Secrets | 社区 | v9.0.0 | 安全 / 供应链 |
| **GitGuardian / GGShield** | 代码安全扫描 | GitGuardian | v1.0.0 | 安全 / 供应链 |
| **Semgrep** | 轻量级静态分析 | Semgrep | v1.0.0 | 安全 / 代码质量 |
| **OpenObserve** | 轻量级云原生可观测性 | OpenObserve | v0.14.0 | 可观测性 |
| **Quickwit** | 云原生搜索引擎 (日志/追踪) | Quickwit | v0.8.0 | 可观测性 / 日志 |
| **OpenShift** | Red Hat 企业 K8s 平台 | Red Hat | v4.18.0 | 平台 / 发行版 |
| **VMware Tanzu** | VMware 应用平台 | VMware | v2.5.0 | 平台 / 发行版 |
| **Mirantis K0s** | 零摩擦 K8s 发行版 | Mirantis | v1.32.0 | 架构 / 发行版 |

---

## 三、核心项目版本速查表

| 项目 | 最新稳定版 | 发布日期 | 下一个主要版本 | K8s 兼容 |
|:---|:---|:---|:---|:---|
| Kubernetes | v1.33.0 | 2026.04 | v1.34 (2026.06) | - |
| Prometheus | v3.3.0 | 2026.03 | v3.4 | v1.29+ |
| Grafana | v11.6.0 | 2026.04 | v12 | - |
| Argo CD | v3.3.8 | 2026.04 | v3.4 (2026.06) | v1.28+ |
| Istio | v1.29.0 | 2026.02 | v1.30 (2026.05) | v1.31-1.35 |
| Cilium | v1.17.0 | 2025.12 | v1.18 | v1.28+ |
| Helm | v3.17.0 | 2025.12 | v4 (2025.11 规划) | v1.28+ |
| Harbor | v2.13.0 | 2025.12 | v2.14 | v1.28+ |
| Dapr | v1.15.0 | 2025.12 | v1.16 | v1.27+ |
| cert-manager | v1.17.0 | 2025.12 | v1.18 | v1.28+ |
| Falco | v0.41.0 | 2025.12 | v0.42 | v1.28+ |
| Kyverno | v1.14.0 | 2025.12 | v1.15 | v1.28+ |
| Knative | v1.18.0 | 2025.12 | v1.19 | v1.29+ |
| Crossplane | v1.19.0 | 2025.12 | v1.20 | v1.28+ |
| KEDA | v2.17.0 | 2025.12 | v2.18 | v1.28+ |
| OpenTelemetry | v1.28.0 | 2026.03 | v1.29 | v1.28+ |
| Backstage | v1.36.0 | 2026.03 | v1.37 | - |
| containerd | v2.0.4 | 2026.03 | v2.1 | v1.30+ |
| etcd | v3.5.21 | 2026.03 | v3.6 | v1.28+ |
| Jaeger | v2.5.0 | 2026.03 | v2.6 | v1.28+ |
| Fluentd | v1.17.1 | 2025.12 | v1.18 | v1.28+ |
| Vault | v1.19.0 | 2025.03 | v1.20 | - |
| Terraform | v1.11.0 | 2025.02 | v1.12 | - |
| Loki | v3.4.0 | 2026.03 | v3.5 | v1.28+ |
| Mimir | v3.0.0 | 2025.11 | - | v1.28+ |

---

## 四、2025-2026 重大里程碑

### 2025-2026 新晋 Graduated 项目
- **Crossplane** (2025.11) — 多云基础设施编排
- **Knative** (2025.10) — Serverless on K8s
- **Dragonfly** (2026.01) — P2P 镜像分发
- **Dapr** (2024.11) — 分布式应用运行时
- **cert-manager** (2024.11) — 自动化证书管理

### 2025-2026 新晋 Incubating 项目
- **KServe** (2025.09) — 云原生模型推理服务
- **Flatcar** (2024.08) — 容器优化 Linux

### 重大版本发布
- **Prometheus 3.0** (2024.11) — 全新 UI、Remote Write 2.0、Native Histograms
- **Grafana Mimir 3.0** (2025.11) — 读写分离架构
- **Jaeger v2** (2024.11) — 基于 OpenTelemetry Collector 架构重写
- **Argo CD 3.0** (2025.05) — 全新 major 版本
- **Helm 4** (2025 启动开发，预计 2025.11 KubeCon 发布)
- **Istio Ambient Mesh GA** (2025) — 无 sidecar 服务网格
- **Cilium 1.16+** — Gateway API GA、BGP 增强
- **K8s Gateway API v1.2** — 正式 GA
- **Podman → CNCF Sandbox** (2024.11 申请) — 含 Buildah、Skopeo、bootc

### 重要变更与弃用
- **Grafana Agent EOL** (2025.11) — 迁移至 Grafana Alloy
- **Flux/Flagger 未来不确定性** — Weaveworks 倒闭，社区接管
- **OPA Gatekeeper → KubeArmor / Kyverno** 趋势

---

## 五、项目成熟度定义

| 级别 | 定义 | 采用建议 |
|:---|:---|:---|
| **Graduated** | 生产级成熟、广泛应用、完善的治理与社区 | **生产环境首选** |
| **Incubating** | 证明可行性、有显著采用、活跃社区 | **可生产采用，需评估风险** |
| **Sandbox** | 早期实验、探索性项目、小规模验证 | **PoC 与评估阶段** |
| **非 CNCF** | 行业主流开源项目或商业开源产品 | **根据社区活跃度判断** |
| **Archived** | 停止维护或已归档 | **不建议新采用** |

---

> 💡 **使用建议**: 各 Domain 目录下的 `00-open-source-projects-index.md` 包含该领域更详细的项目说明、配置示例与最佳实践。
