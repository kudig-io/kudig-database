---
title: CNCF 项目选型指南
description: CNCF 云原生项目选型对比矩阵，涵盖 CNI、Service Mesh、存储、监控、安全等类目的选型决策树
summary: CNCF 云原生项目选型对比矩阵，涵盖 CNI、Service Mesh、存储、监控、安全等类目的选型决策树
category: cncf-landscape
tags:
- k8s
- cncf
- selection
- comparison
- cni
- service-mesh
- prometheus
- grafana
- jaeger
- istio
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- DevOps
estimated_read_time: 10min
intent_queries:
- CNCF 项目选型
- CNI 对比选型
- Service Mesh 选型
trigger_keywords:
- CNCF
- 选型
- 对比
- 决策
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- policy-basics
- backup-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNCF 项目选型指南

> **最后更新**: 2026-05 | **适用场景**: 生产环境选型

---

## 1. CNI 网络插件选型

### 1.1 选型决策树

```
你需要什么类型的网络方案？
│
├─ 需要 NetworkPolicy 支持？
│   │
│   ├─ 是 ──▶ 需要 eBPF 加速？
│   │          │
│   │          ├─ 是 ──▶ Cilium
│   │          │
│   │          └─ 否 ──▶ Calico
│   │
│   └─ 否 ──▶ 规模如何？
│              │
│              ├─ 小型/测试 ──▶ Flannel
│              │
│              ├─ 中型生产 ──▶ Calico
│              │
│              └─ 大型/高性能 ──▶ Cilium
│
├─ 需要加密隧道？
│   │
│   └─ 是 ──▶ WireGuard 模式
│              │
│              ├─ Cilium + WireGuard
│              └─ Flannel + WireGuard
│
└─ 需要跨集群网络？
    │
    └─ 是 ──▶ Submariner + Calico/Cilium
```

### 1.2 CNI 功能对比

| 特性 | Cilium | Calico | Flannel | [[Antrea|Antrea]] | OVN-[[Kubernetes|Kubernetes]] |
|:-----|:------:|:------:|:-------:|:------:|:---------------:|
| **成熟度** | Graduated | Graduated | Graduated | Sandbox | Sandbox |
| **eBPF 加速** | ✓ | ✗ | ✗ | ✗ | ✗ |
| **L3/L4 NetworkPolicy** | ✓ | ✓ | ✗ | ✓ | ✓ |
| **L7 NetworkPolicy** | ✓ | ✓ | ✗ | ✗ | ✗ |
| **BGP 路由** | ✓ | ✓ | ✗ | ✓ | ✓ |
| **WireGuard 加密** | ✓ | ✓ | ✓ | ✗ | ✗ |
| **IPv6 Dual Stack** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Windows 支持** | ✓ | ✓ | ✓ | ✓ | ✗ |
| **性能** | 最高 | 高 | 中 | 高 | 高 |
| **学习曲线** | 陡峭 | 中等 | 平缓 | 平缓 | 陡峭 |
| **运维复杂度** | 中 | 中 | 低 | 低 | 高 |

### 1.3 场景推荐

| 场景 | 推荐方案 | 原因 |
|:-----|:---------|:-----|
| 小型集群/测试 | Flannel | 简单易用 |
| 通用生产环境 | Calico | 功能全面 |
| 高性能/大规模 | Cilium | eBPF 加速 |
| Windows 混合集群 | Antrea | Windows 原生支持 |
| 多集群互联 | Calico + Submariner | 成熟稳定 |
| 安全敏感环境 | Cilium | 细粒度策略 |

---

## 2. [[Service|Service]]Service Mesh）|Service Mesh]] 选型

### 2.1 选型决策树

```
需要服务网格吗？
│
├─ 需要 L7 高级特性（mTLS、重试、熔断）
│   │
│   ├─ 需要超高性能（<1ms 延迟）？
│   │   │
│   │   ├─ 是 ──▶ Linkerd
│   │   │
│   │   └─ 否 ──▶ 需要深度可观测性？
│   │              │
│   │              ├─ 是 ──▶ Istio + Kiali
│   │              │
│   │              └─ 否 ──▶ Linkerd
│   │
│   └─ 需要多集群支持？
│       │
│       └─ 是 ──▶ Istio + Ambient Mesh
│
└─ 仅需要 mTLS 和基础观测？
    │
    └─ Linkerd (轻量级)
```

### 2.2 Service Mesh 对比

| 特性 | Istio | Linkerd | Cilium | Kuma |
|:-----|:-----:|:-------:|:------:|:----:|
| **成熟度** | Graduated | Graduated | Graduated | Sandbox |
| **架构** | Sidecar | Sidecar-less | Sidecar-less | Sidecar |
| **数据平面** | Envoy | Rust | eBPF | Envoy |
| **控制平面** | Go/Envoy | Go | Go/eBPF | Go |
| **延迟开销** | 1-3ms | <1ms | <0.5ms | 1-2ms |
| **资源消耗** | 高 | 低 | 极低 | 中 |
| **L7 策略** | 完整 | 基础 | 完整 | 基础 |
| **mTLS** | 完整 | 完整 | 完整 | 完整 |
| **多集群** | ✓ | ✓ | ✓ | ✓ |
| **WASM 扩展** | ✓ | ✗ | ✓ | ✗ |
| **学习曲线** | 陡峭 | 平缓 | 中等 | 平缓 |

### 2.3 场景推荐

| 场景 | 推荐方案 | 原因 |
|:-----|:---------|:-----|
| 高性能要求 | Cilium | eBPF 极低延迟 |
| 简单场景 | Linkerd | 轻量易用 |
| 企业级功能 | Istio | 完整特性集 |
| 多云/混合云 | Istio + Anthos | 生态完善 |
| 已有 Cilium | Cilium Ambient | 无 Sidecar |
| 资源受限 | Kuma | 轻量 |

---

## 3. 存储选型

### 3.1 选型决策树

```
需要什么类型的存储？
│
├─ 块存储（数据库）？
│   │
│   ├─ 需要高可用 ──▶ Rook/Ceph 或 Longhorn
│   │
│   └─ 云环境 ──▶ 云厂商内置存储
│
├─ 文件存储（NFS）？
│   │
│   ├─ 大规模 ──▶ CubeFS 或 Rook/CephFS
│   │
│   └─ 小规模 ──▶ NFS Ganesha
│
└─ 对象存储（S3）？
    │
    └─ MinIO（自建）或 云厂商 S3
```

### 3.2 存储方案对比

| 特性 | Rook/Ceph | Longhorn | CubeFS | OpenEBS | HwameiStor |
|:-----|:----------:|:--------:|:------:|:-------:|:----------:|
| **成熟度** | Graduated | Incubating | Graduated | Sandbox | Sandbox |
| **存储类型** | 块/文件/对象 | 块 | 块/文件/对象 | 块/文件 | 块 |
| **数据复制** | 多副本 | 多副本 | 多副本 | 多副本 | 多副本 |
| **快照支持** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **在线扩容** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **异地备份** | ✓ | ✓ | ✓ | ✓ | ✗ |
| **GUI 管理** | ✓ | ✓ | ✓ | ✓ | ✓ |
| **资源消耗** | 高 | 中 | 中 | 低 | 低 |
| **最小节点** | 3 | 1 | 3 | 1 | 1 |
| **适用场景** | 企业存储 | 通用存储 | 云原生存储 | 小规模 | 本地存储 |

### 3.3 场景推荐

| 场景 | 推荐方案 | 原因 |
|:-----|:---------|:-----|
| 企业级全功能 | Rook/Ceph | 功能最全面 |
| 简化运维 | Longhorn | UI 友好 |
| 超大规模 | CubeFS | 字节跳动生产验证 |
| 开发测试 | OpenEBS Maya | 轻量快速 |
| 本地 SSD | HwameiStor | 高性能本地 |

---

## 4. 可观测性选型

### 4.1 监控方案对比

| 特性 | [[Prometheus|Prometheus]] | Thanos | Cortex | Mimir |
|:-----|:----------:|:------:|:------:|:-----:|
| **成熟度** | Graduated | Incubating | Incubating | Sandbox |
| **多租户** | ✗ | ✗ | ✓ | ✓ |
| **长期存储** | 受限 | ✓ | ✓ | ✓ |
| **压缩** | 本地 | ✓ | ✓ | ✓ |
| **对象存储** | ✗ | ✓ | ✓ | ✓ |
| **查询性能** | 优 | 优 | 中 | 优 |
| **资源消耗** | 中 | 高 | 高 | 中 |
| **运维复杂度** | 低 | 高 | 高 | 中 |

### 4.2 日志方案对比

| 特性 | ELK Stack | Loki | Elastic Cloud | OpenSearch |
|:-----|:---------:|:----:|:-------------:|:----------:|
| **成熟度** | - | Incubating | - | Sandbox |
| **存储成本** | 高 | 低 | 中 | 中 |
| **查询语法** | Lucene | LogQL | Lucene | Lucene |
| **K8s 原生** | 中 | 高 | 中 | 中 |
| **集成生态** | 广 | 中 | 广 | 广 |
| **运维复杂度** | 高 | 低 | 低 | 中 |

### 4.3 追踪方案对比

| 特性 | Jaeger | Zipkin | Tempo | SigNoz |
|:-----|:------:|:------:|:-----:|:-------:|
| **成熟度** | Graduated | - | Incubating | Sandbox |
| **存储后端** | 多 | 多 | 对象存储 | PostgreSQL |
| **查询性能** | 优 | 优 | 优 | 中 |
| **资源消耗** | 中 | 中 | 中 | 中 |
| **OTLP 支持** | ✓ | ✓ | ✓ | ✓ |

### 4.4 场景推荐

| 场景 | 推荐方案 |
|:-----|:---------|
| 小规模监控 | Prometheus + Grafana |
| 大规模/多租户 | Prometheus + Thanos 或 Mimir |
| 低成本日志 | Loki + Grafana |
| 复杂日志分析 | ELK Stack |
| 全栈可观测性 | OTel Collector + Tempo + Loki + Prometheus |

---

## 5. GitOps 与 CD 选型

### 5.1 GitOps 工具对比

| 特性 | Argo CD | Flux | Jenkins X | Argo Rollouts |
|:-----|:-------:|:----:|:---------:|:-------------:|
| **成熟度** | Graduated | Graduated | Sandbox | Sandbox |
| **架构** | Pull-based | Pull-based | Pull-based | Progressive |
| **多集群** | ✓ | ✓ | ✓ | ✓ |
| **RBAC** | ✓ | ✓ | ✓ | ✓ |
| **UI** | 完整 | 基础 | 基础 | 基础 |
| **Helm 支持** | ✓ | ✓ | ✓ | ✓ |
| **Kustomize** | ✓ | ✓ | ✓ | ✓ |
| **金丝雀发布** | ✗ | ✗ | ✗ | ✓ |
| **学习曲线** | 平缓 | 中等 | 陡峭 | 平缓 |

### 5.2 场景推荐

| 场景 | 推荐方案 |
|:-----|:---------|
| 纯 GitOps | Argo CD 或 Flux |
| 需要渐进发布 | Argo CD + Rollouts |
| 已有 Jenkins | Jenkins X |
| 多集群管理 | Argo CD ApplicationSet |

---

## 6. 安全工具选型

### 6.1 策略引擎对比

| 特性 | OPA/Gatekeeper | Kyverno | KubeBench | Falco |
|:-----|:--------------:|:-------:|:---------:|:-----:|
| **类型** | 策略即代码 | 策略即代码 | 安全扫描 | 运行时检测 |
| **策略语言** | Rego | YAML | Go | YAML |
| **CRD 方式** | OPA Constraint | Kubernetes 原生 | Benchmark | Rule |
| **Webhook 拦截** | ✓ | ✓ | ✗ | ✗ |
| **审计能力** | ✓ | ✓ | ✓ | ✓ |
| **学习曲线** | 陡峭 | 平缓 | 平缓 | 中等 |

### 6.2 场景推荐

| 场景 | 推荐方案 |
|:-----|:---------|
| 通用策略引擎 | OPA + Gatekeeper |
| Kubernetes 原生策略 | Kyverno |
| 合规检查 | KubeBench + KubeHunter |
| 运行时安全 | Falco + OPA |

---

## 7. 混沌工程选型

### 7.1 混沌工具对比

| 特性 | Chaos Mesh | Litmus | Kraken | Pumba |
|:-----|:----------:|:------:|:------:|:-----:|
| **成熟度** | Incubating | Incubating | Sandbox | Sandbox |
| **K8s 原生** | ✓ | ✓ | ✓ | ✓ |
| **混沌类型** | 全方位 | 全方位 | OpenShift | Docker/K8s |
| **SLO 追踪** | ✓ | ✗ | ✗ | ✗ |
| **调度能力** | ✓ | ✓ | ✗ | ✗ |
| **社区活跃度** | 高 | 高 | 中 | 低 |

### 7.2 场景推荐

| 场景 | 推荐方案 |
|:-----|:---------|
| 通用混沌工程 | Chaos Mesh |
| CNCF 生态优先 | Litmus |
| OpenShift 环境 | Kraken |
| 容器级别 chaos | Pumba |

---

## 8. 多集群管理选型

### 8.1 多集群方案对比

| 特性 | Karmada | Clusternet | OCM | KubeFed |
|:-----|:-------:|:----------:|:---:|:-------:|
| **成熟度** | Incubating | Sandbox | Sandbox | - |
| **架构** | 原生 K8s API | 聚合 API | 聚合 API | Federation |
| **调度策略** | 高级 | 基础 | 高级 | 基础 |
| **应用分发** | CRD | CRD | CRD | ConfigMap |
| **多租户** | ✓ | ✓ | ✓ | ✓ |
| **学习曲线** | 中等 | 平缓 | 中等 | 陡峭 |

### 8.2 场景推荐

| 场景 | 推荐方案 |
|:-----|:---------|
| 跨云/多集群 | Karmada |
| 简单多集群 | Clusternet |
| 混合云管理 | OCM |

---

## 9. 决策矩阵

### 9.1 按优先级选择

| 优先级 | 场景 | 推荐组合 |
|:-------|:-----|:---------|
| **性能优先** | 高性能网络 | Cilium + eBPF |
| **稳定性优先** | 金融/关键业务 | Calico + Rook/Ceph |
| **成本优先** | 初创/小团队 | Flannel + Longhorn + Loki |
| **功能优先** | 企业级 | Cilium + Istio + Rook + OPA |
| **简单优先** | 快速起步 | k3s + Longhorn + Argo CD |

### 9.2 技术栈推荐

| 场景 | 推荐技术栈 |
|:-----|:----------|
| **微型 (< 10 节点)** | k3s + Flannel + Longhorn + Argo CD |
| **小型 (10-50 节点)** | kubeadm + Calico + Rook + Argo CD + Prometheus |
| **中型 (50-200 节点)** | kubeadm + Cilium + Rook + Argo CD + Prometheus + Thanos |
| **大型 (200+ 节点)** | 多集群 + Cilium + Karmada + Istio + Rook + Mimir |
| **多云/混合云** | Rancher/RKE + Kaleda + Thanos + Istio + Vault |

---

## 10. 升级与迁移路径

### 10.1 CNI 迁移

| 迁移路径 | 风险 | 停机时间 |
|:---------|:----:|:--------:|
| Flannel → Calico | 中 | 滚动更新 |
| Calico → Cilium | 中 | 滚动更新 |
| Flannel → Cilium | 高 | 滚动更新 |

### 10.2 Service Mesh 迁移

| 迁移路径 | 建议 |
|:---------|:-----|
| 无 → Linkerd | 直接部署 |
| 无 → Istio | 使用 Sidecar 注入 |
| Linkerd → Istio | 逐步迁移流量 |
| Cilium → Istio | 并存一段时间 |

### 10.3 存储迁移

| 迁移类型 | 方法 |
|:---------|:-----|
| PV 迁移 | Velero 备份恢复 |
| StorageClass 切换 | 逐步迁移 PVC |
| 集群间迁移 | Restic + Velero |

---

## Obsidian 相关文档

- domain-19-landscape-references MOC
- [[domain-19-landscape-references/README.md|Domain-34: CNCF Landscape 开源项目]]
- Domain-34 CNCF Landscape — 开源项目索引
- CNCF 集成实践指南
- CNCF 学习路径
- CNCF 项目 FTA 索引

## See Also

- 01-cncf-integration-guide
- 02-cncf-learning-paths
- 04-cncf-fta-index
- 01-cncf-integration-guide


<!-- risk-assessed -->
