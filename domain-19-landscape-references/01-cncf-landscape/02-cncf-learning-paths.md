---
title: CNCF 学习路径
description: CNCF 云原生技术学习路径，按角色（DevOps/SRE/架构师）和技能水平规划学习顺序
category: cncf-landscape
tags:
- k8s
- cncf
- learning
- path
- devops
- sre
- etcd
- prometheus
- grafana
- jaeger
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- DevOps
- SRE
- 架构师
- 开发者
estimated_read_time: 10min
intent_queries:
- CNCF 学习路径
- 云原生学习路线
- CNCF 认证
trigger_keywords:
- CNCF
- 学习路径
- 云原生
- 认证
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- logging-basics
- tracing-basics
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# CNCF 学习路径

> **最后更新**: 2026-05 | **学习时长**: 3-12 个月

---

<!-- chunk: 1. 学习路径概览 -->## 1. 学习路径概览

```
┌─────────────────────────────────────────────────────────────────┐
│                      CNCF 学习全景图                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐      │
│  │   入门级   │ ──▶ │   进阶级   │ ──▶ │   高级    │      │
│  │  (1-3月)   │     │  (3-6月)   │     │  (6-12月)  │      │
│  └─────────────┘     └─────────────┘     └─────────────┘      │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. 角色学习路径 -->## 2. 角色学习路径

#<!-- chunk: 2.1 DevOps 工程师路径 -->## 2.1 DevOps 工程师路径

```
Week 1-4: 基础 (40h)
├── Docker 容器基础 (10h)
│   ├── 容器概念与原理
│   ├── Dockerfile 编写
│   └── Docker Compose
├── Kubernetes 基础 (20h)
│   ├── 架构与核心概念
│   ├── Pod/Deployment/Service
│   └── ConfigMap/Secret
└── 基础网络 (10h)
    ├── Docker 网络模式
    └── Kubernetes 网络模型

Week 5-8: 核心技能 (40h)
├── Kubernetes 进阶 (15h)
│   ├── Ingress/Helm
│   ├── Storage/PV/PVC
│   └── RBAC/ServiceAccount
├── CI/CD 集成 (15h)
│   ├── Argo CD / Flux
│   ├── GitOps 实践
│   └── Tekton/Pipeline
└── 监控基础 (10h)
    ├── Prometheus 基础
    └── Grafana 可视化

Week 9-12: 运维实践 (40h)
├── 集群管理 (15h)
│   ├── kubeadm/k3s
│   ├── 节点维护
│   └── 升级迁移
├── 日志与追踪 (15h)
│   ├── Fluentd/ELK
│   └── Jaeger/Zipkin
└── 故障排查 (10h)
    └── 常见问题处理
```

**推荐文档**：
- [Kubernetes](./graduated/kubernetes/kubernetes.md)
- [[entities/containerd|containerd]]|containerd]]](./graduated/containerd/containerd.md)
- [[entities/helm|Helm]]](./graduated/helm/helm.md)
- [Prometheus](./graduated/prometheus/prometheus.md)
- [Argo CD](./graduated/argo/argo.md)
- [Flux](./graduated/flux/flux.md)

---

#<!-- chunk: 2.2 SRE 工程师路径 -->## 2.2 SRE 工程师路径

```
Week 1-4: 可观测性 (40h)
├── Prometheus 深度 (15h)
│   ├── PromQL 高级查询
│   ├── Recording Rules
│   └── AlertManager 配置
├── 日志系统 (15h)
│   ├── Fluentd/Fluent Bit
│   ├── Loki/Elasticsearch
│   └── 日志聚合架构
└── 分布式追踪 (10h)
    ├── OpenTelemetry
    └── Jaeger/Zipkin

Week 5-8: 稳定性工程 (40h)
├── 服务网格 (15h)
│   ├── Istio/Linkerd
│   ├── 流量管理
│   └── mTLS 配置
├── 安全监控 (15h)
│   ├── Falco 运行时安全
│   ├── OPA/Gatekeeper
│   └── 审计日志
└── 混沌工程 (10h)
    ├── Chaos Mesh/Litmus
    └── 故障注入

Week 9-12: 高级主题 (40h)
├── 多集群管理 (15h)
│   ├── Karmada
│   ├── Federation
│   └── 跨集群服务发现
├── 成本优化 (10h)
│   ├── KEDA 弹性伸缩
│   └── OpenCost
└── SLA/SLO 设计 (15h)
    ├── 指标定义
    └── 告警策略

Week 13-16: 生产实践 (40h)
├── 高可用架构 (20h)
│   ├── 集群高可用
│   ├── 数据备份
│   └── 灾备方案
└── 容量规划 (20h)
    ├── 性能测试
    ├── 资源配额
    └── 扩展性设计
```

**推荐文档**：
- [Prometheus](./graduated/prometheus/prometheus.md)
- [Istio](./graduated/istio/istio.md)
- [[entities/linkerd|Linkerd]]](./graduated/linkerd/linkerd.md)
- [OpenTelemetry](./incubating/opentelemetry/opentelemetry.md)
- [Falco](./graduated/falco/falco.md)
- [Chaos Mesh](./incubating/chaos-mesh/chaos-mesh.md)
- [KEDA](./graduated/keda/keda.md)
- [Karmada](./incubating/karmada/karmada.md)

---

#<!-- chunk: 2.3 架构师路径 -->## 2.3 架构师路径

```
Month 1-2: 战略基础 (80h)
├── 云原生架构原则 (20h)
│   ├── 12-Factor App
│   ├── 云原生设计模式
│   └── 微服务架构
├── 安全架构 (30h)
│   ├── 零信任安全
│   ├── SPIFFE/SPIRE
│   ├── cert-manager
│   └── 供应链安全
└── 数据架构 (30h)
    ├── TiKV/Vitess
    ├── Rook/CubeFS
    └── 消息队列 NATS

Month 3-4: 高级网络 (80h)
├── CNI 与网络策略 (25h)
│   ├── Cilium/eBPF
│   ├── Calico/Flannel
│   └── NetworkPolicy
├── 服务网格深度 (30h)
│   ├── Istio 高级特性
│   ├── Ambient Mesh
│   └── WASM 扩展
└── 多集群网络 (25h)
    ├── Submariner
    ├── Kubeslice
    └── ClusterMesh

Month 5-6: 平台工程 (80h)
├── 平台构建 (30h)
│   ├── Crossplane
│   ├── CDK8s/KCL
│   └── Terraform/OpenTofu
├── GitOps 平台 (25h)
│   ├── Argo CD 高级特性
│   ├── Rollouts/Flagger
│   └── 策略即代码
└── 成本与治理 (25h)
    ├── FinOps 实践
    ├── 资源配额设计
    └── 合规与审计

Month 7-8: 行业解决方案 (80h)
├── AI/ML 平台 (30h)
│   ├── Kubeflow/KServe
│   ├── Volcano 调度
│   └── GPU 虚拟化
├── 边缘计算 (25h)
│   ├── KubeEdge
│   ├── OpenYurt
│   └── K3s 部署
└── 混合云架构 (25h)
    ├── Anthos/Arc
    ├── Cluster API
    └── GitOps 多集群
```

**推荐文档**：
- [Kubernetes](./graduated/kubernetes/kubernetes.md)
- [Cilium](./graduated/cilium/cilium.md)
- [Istio](./graduated/istio/istio.md)
- [Crossplane](./graduated/crossplane/crossplane.md)
- [SPIFFE](./graduated/spiffe/spiffe.md)
- [TiKV](./graduated/tikv/tikv.md)
- [Rook](./graduated/rook/rook.md)
- [Kubeflow](./incubating/kubeflow/kubeflow.md)
- [KubeEdge](./graduated/kubeedge/kubeedge.md)

---

#<!-- chunk: 2.4 安全工程师路径 -->## 2.4 安全工程师路径

```
Month 1: 基础安全 (40h)
├── 容器安全 (15h)
│   ├── 镜像扫描 (Trivy)
│   ├── 安全最佳实践
│   └── 最小权限原则
├── Kubernetes 安全 (15h)
│   ├── RBAC 配置
│   ├── NetworkPolicy
│   └── Pod Security
└── 供应链安全 (10h)
    ├── Sigstore/cosign
    ├── SLSA 标准
    └── SBOM 生成

Month 2: 运行时安全 (40h)
├── 威胁检测 (20h)
│   ├── Falco 规则
│   ├── 异常检测
│   └── 审计日志
├── 策略执行 (15h)
│   ├── OPA/Rego
│   ├── Gatekeeper
│   └── Kyverno
└── 机密计算 (5h)
    └── Confidential Containers

Month 3: 身份与零信任 (40h)
├── 服务身份 (15h)
│   ├── SPIFFE/SPIRE
│   ├── mTLS 配置
│   └── 证书管理
├── 零信任网络 (15h)
│   ├── BeyondCorp
│   ├── ZTA 实施
│   └── 服务网格集成
└── 合规自动化 (10h)
    ├── OPA/Conftest
    └── OSCAL 标准
```

**推荐文档**：
- [OPA](./graduated/opa/opa.md)
- [Falco](./graduated/falco/falco.md)
- [SPIFFE](./graduated/spiffe/spiffe.md)
- [[entities/spire|SPIRE]]](./graduated/spire/spire.md)
- [Kyverno](./incubating/kyverno/kyverno.md)
- [in-toto](./graduated/in-toto/in-toto.md)
- [[entities/operator-framework|The Update Framework (TUF)]]|TUF]]](./graduated/tuf/tuf.md)

---

<!-- chunk: 3. 专项学习路径 -->## 3. 专项学习路径

#<!-- chunk: 3.1 网络专项 -->## 3.1 网络专项

```
阶段一：基础网络 (20h)
├── Kubernetes 网络模型
├── CNI 基础 (Flannel/Calico)
└── Service/Ingress

阶段二：高级网络 (30h)
├── Cilium/eBPF 深度
├── VXLAN/Geneve 隧道
└── BGP 路由

阶段三：服务网格 (30h)
├── Istio 完整学习
├── Linkerd 对比
└── Envoy 高级特性

阶段四：多集群网络 (20h)
├── Submariner
├── ClusterMesh
└── Network Service Mesh
```

**推荐文档**：
- [Cilium](./graduated/cilium/cilium.md)
- [Istio](./graduated/istio/istio.md)
- [Linkerd](./graduated/linkerd/linkerd.md)
- [Envoy](./graduated/envoy/envoy.md)
- [CoreDNS](./graduated/coredns/coredns.md)
- [Submariner](./sandbox/submariner/submariner.md)

---

#<!-- chunk: 3.2 存储专项 -->## 3.2 存储专项

```
阶段一：存储基础 (15h)
├── Kubernetes 存储概念
├── PV/PVC/StorageClass
└── 本地存储 vs 网络存储

阶段二：块存储 (20h)
├── Rook/Ceph
├── Longhorn
└── 云厂商存储集成

阶段三：文件存储 (15h)
├── CubeFS
├── NFS Ganesha
└── GlusterFS

阶段四：软件定义存储 (20h)
├── MinIO/S3
├── 分布式存储架构
└── 数据保护与备份
```

**推荐文档**：
- [Rook](./graduated/rook/rook.md)
- [Longhorn](./incubating/longhorn/longhorn.md)
- [CubeFS](./graduated/cubefs/cubefs.md)

---

#<!-- chunk: 3.3 可观测性专项 -->## 3.3 可观测性专项

```
阶段一：监控 (25h)
├── Prometheus 深度
├── PromQL 高级
├── AlertManager
└── Thanos 扩展

阶段二：日志 (20h)
├── ELK/Loki 架构
├── Fluentd/Fluent Bit
└── 日志聚合

阶段三：追踪 (20h)
├── OpenTelemetry
├── Jaeger/Zipkin
└── 关联分析

阶段四：可视化 (15h)
├── Grafana 高级
├── 自定义 Dashboard
└── Alerting 策略
```

**推荐文档**：
- [Prometheus](./graduated/prometheus/prometheus.md)
- [Thanos](./incubating/thanos/thanos.md)
- [Grafana](./graduated/grafana/grafana.md)
- [Fluentd](./graduated/fluentd/fluentd.md)
- [Jaeger](./graduated/jaeger/jaeger.md)
- [OpenTelemetry](./incubating/opentelemetry/opentelemetry.md)

---

<!-- chunk: 4. 认证路径 -->## 4. 认证路径

#<!-- chunk: 4.1 CKA (Kubernetes 管理员) -->## 4.1 CKA (Kubernetes 管理员)

**考试内容**：
- 集群架构与运维 (25%)
- 工作负载与调度 (15%)
- 服务与网络 (20%)
- 存储 (10%)
- 故障排查 (30%)

**准备时间**：40-60h

**推荐文档**：
- [Kubernetes](./graduated/kubernetes/kubernetes.md)
- [etcd](./graduated/etcd/etcd.md)
- [containerd](./graduated/containerd/containerd.md)

---

#<!-- chunk: 4.2 CKS (Kubernetes 安全专家) -->## 4.2 CKS (Kubernetes 安全专家)

**考试内容**：
- 集群架构 (10%)
- 集群强化 (15%)
- 系统强化 (15%)
- 微服务强化 (20%)
- 供应链安全 (20%)
- 运行时安全 (20%)

**前置条件**：CKA

**准备时间**：30-40h

**推荐文档**：
- [Falco](./graduated/falco/falco.md)
- [OPA](./graduated/opa/opa.md)
- [SPIFFE](./graduated/spiffe/spiffe.md)
- [Kyverno](./incubating/kyverno/kyverno.md)

---

#<!-- chunk: 4.3 KCNA (Kubernetes 云原生助理) -->## 4.3 KCNA (Kubernetes 云原生助理)

**考试内容**：
- 容器编排基础 (30%)
- Kubernetes 基础 (25%)
- 云原生架构 (20%)
- 云原生可观测性 (10%)
- 云原生应用 (15%)

**准备时间**：20-30h

---

#<!-- chunk: 4.4 PCA (Prometheus 认证专家) -->## 4.4 PCA (Prometheus 认证专家)

**考试内容**：
- Prometheus 基础 (20%)
- 数据模型与标签 (15%)
- PromQL (25%)
- 告警配置 (20%)
- Pushgateway/Exporters (20%)

**准备时间**：20-30h

**推荐文档**：
- [Prometheus](./graduated/prometheus/prometheus.md)

---

<!-- chunk: 5. 学习资源汇总 -->## 5. 学习资源汇总

#<!-- chunk: 5.1 官方文档 -->## 5.1 官方文档

| 项目 | 文档链接 |
|:-----|:---------|
| Kubernetes | https://kubernetes.io/docs/ |
| Prometheus | https://prometheus.io/docs/ |
| Istio | https://istio.io/latest/docs/ |
| Cilium | https://docs.cilium.io/ |
| Argo CD | https://argoproj.github.io/argo-cd/ |

#<!-- chunk: 5.2 实践平台 -->## 5.2 实践平台

| 平台 | 用途 |
|:-----|:-----|
| Killercoda | 免费 K8s 实验环境 |
| Play with Kubernetes | Docker Playground |
| Katacoda | 交互式学习 |
| Azure Kubernetes Workshop | 微软官方实验 |

#<!-- chunk: 5.3 社区资源 -->## 5.3 社区资源

| 资源 | 链接 |
|:-----|:-----|
| CNCF Blog | https://www.cncf.io/blog/ |
| Kubernetes Blog | https://kubernetes.io/blog/ |
| DevOps Weekly | 周刊订阅 |
| Cloud Native Weekly | 周刊订阅 |

---

<!-- chunk: 6. 学习时间估算 -->## 6. 学习时间估算

| 角色 | 基础 (80h) | 进阶 (120h) | 专家 (200h+) |
|:-----|:----------:|:-----------:|:------------:|
| DevOps | 1-2 月 | 2-3 月 | 6+ 月 |
| SRE | 1-2 月 | 3-4 月 | 8+ 月 |
| 架构师 | 2-3 月 | 4-6 月 | 12+ 月 |
| 安全 | 1-2 月 | 3-4 月 | 6+ 月 |

> 注：以上为估算时间，实际学习速度因人而异

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-19-landscape-references MOC
- [[domain-19-landscape-references/README|Domain-34: CNCF Landscape 开源项目]]
- Domain-34 CNCF Landscape — 开源项目索引
- CNCF 集成实践指南
- CNCF 项目选型指南
- CNCF 项目 FTA 索引

## See Also

- 04-cncf-fta-index
- 01-cncf-integration-guide
- 03-cncf-selection-guide
- 04-cncf-fta-index
