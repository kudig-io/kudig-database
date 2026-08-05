---
title: Kubernetes v1.33 速查卡
description: '| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅
  默认启用 |'
summary: '| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅ 默认启用
  |'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- istio
- containerd
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.33 速查卡 是什么
- 如何 Kubernetes v1.33 速查卡
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.33
- 速查卡
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- service-mesh-basics
- etcd-basics
- gpu-scheduling-basics
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
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] v1.33 速查卡

> **一页纸速查**: v1.29 → v1.33 所有关键变更  
> **最后更新**: 2026-04-24

---

<!-- chunk: 🚀 v1.33 核心变更 (最新) -->
## 🚀 v1.33 核心变更 (最新)

| 特性 | 状态 | 一句话说明 | 是否启用 |
|:---|:---|:---|:---|
| **Sidecar 容器** | **GA** | init 容器支持 `restartPolicy: Always`，自动重启 | ✅ 默认启用 |
| **DRA** | **GA** | GPU/FPGA 动态资源分配，替代 Device Plugin | ⚠️ 需显式启用 FG |
| **TopologyManager Per Pod** | **GA** | Pod 级 NUMA 拓扑策略 | ⚠️ 需显式启用 FG |
| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅ 默认启用 |
| **[[kubelet|Kubelet]] Resource Metrics** | **Beta** | `/metrics/resource` 端点，替代 Summary API | ✅ 默认启用 |
| **In-Place Pod Resize** | **Alpha** | 原地调整 Pod 资源，无需重启 | ❌ 需启用 FG |
| **Cross-Namespace PVC** | **Alpha** | PVC 跨命名空间引用数据源 | ❌ 需启用 FG |
| **PodIndexLabel** | **GA** | [[StatefulSet|StatefulSet]] 自动生成 `apps.kubernetes.io/pod-index` | ✅ 默认启用 |
| **Windows HostProcess** | **GA** | Windows 容器 HostProcess 模式稳定 | ✅ 默认启用 |

---

<!-- chunk: 📈 版本演进时间线 -->
## 📈 版本演进时间线

```
v1.29 (2023.12) ──► v1.30 (2024.04) ──► v1.31 (2024.08) ──► v1.32 (2024.12) ──► v1.33 (2025.04)
    │                    │                    │                    │                    │
    ├── Sidecar Beta     ├── CEL Admission GA ├── AppArmor GA      ├── DRA Beta         ├── Sidecar GA
    ├── ReadWriteOncePod ├── SchedulingGates  ├── Parallel Pulls   ├── TopologyManager  ├── DRA GA
    │   GA               │   GA               │   默认启用         │   Per Pod Beta     ├── Queueing Hints
    └── KMS v2 GA        └── BoundSA Token    └── nftables Alpha   └── Pod-level        │   Beta
                           GA                    └── OpenTelemetry    Resources Alpha    └── Kubelet
                                                  Tracing GA                            Metrics Beta
```

---

<!-- chunk: ⚡ 快速启用新特性 -->
## ⚡ 快速启用新特性

### Sidecar 容器 (GA, 立即可用)

```yaml
spec:
  initContainers:
  - name: proxy
    image: istio/proxyv2:1.24
    restartPolicy: Always      # ← 这就是全部
```

### DRA (GA, 需启用 Feature Gate)

```bash
# kube-apiserver, kube-scheduler, kubelet
--feature-gates=DynamicResourceAllocation=true
```

### In-Place Resize (Alpha, 实验性)

```bash
# kubelet
--feature-gates=InPlacePodVerticalScaling=true
```

```yaml
metadata:
  annotations:
    resize.policy/container.app: "RestartNotRequired"
```

---

<!-- chunk: 🔧 kubectl 快捷命令 -->
## 🔧 kubectl 快捷命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 版本检查
kubectl version

# 查看 Feature Gates
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | jq '.kubeletconfig.featureGates'

# 检查已弃用 API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 查看 ValidatingAdmissionPolicy
kubectl get validatingadmissionpolicies

# Sidecar 容器检查
kubectl get pods -A -o json | jq '.items[].spec.initContainers[]? | select(.restartPolicy == "Always") | .name'

# 节点日志 (v1.30+, Alpha)
kubectl alpha node-logs NODE --service=kubelet

# 调试 Profile (v1.32+)
kubectl debug POD --profile=netadmin
```
---

<!-- chunk: 🔄 升级路径 -->
## 🔄 升级路径

```
当前版本 → 目标版本
    │
    ├── ≤v1.29 → 立即升级到 v1.33
    ├── v1.30  → 升级到 v1.33
    ├── v1.31  → 升级到 v1.33
    ├── v1.32  → 评估后升级到 v1.33
    └── v1.33  → 保持，等待 v1.34
```

---

<!-- chunk: 📋 生产检查清单 -->
## 📋 生产检查清单

- [ ] 集群版本 ≥ v1.32 (v1.33 推荐)
- [ ] 所有节点 containerd ≥ 1.7.18
- [ ] etcd ≥ 3.5.15
- [ ] CSI 驱动已安装 (in-tree 驱动已弃用)
- [ ] CCM 已部署 (kubelet --cloud-provider 已弃用)
- [ ] 无已弃用 API 使用
- [ ] Pod Security Admission 已配置
- [ ] ServiceAccount Token 自动轮转正常
- [ ] 匿名用户未绑定 cluster-admin

---

<!-- chunk: 📚 相关文档 -->
## 📚 相关文档

| 文档 | 内容 |
|:---|:---|
| [99-kubernetes-v1.29-v1.33-features-guide.md](15-kubernetes-v1.29-v1.33-features-guide.md) | 按版本详解 |
| [99-kubernetes-core-components-v1.29-v1.33-update.md](12-kubernetes-core-components-v1.29-v1.33-update.md) | 按组件速查 |
| [99-kubernetes-v1.33-upgrade-guide.md](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/06-upgrade-paths/01-kubernetes-v1.33-upgrade-guide.md) | 升级实操 |
| [99-kubectl-v1.29-v1.33-new-commands-guide.md](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/05-kubectl/02-kubectl-v1.29-v1.33-new-commands-guide.md) | kubectl 新命令 |
| [99-kubernetes-v1.33-production-best-practices.md](19-kubernetes-v1.33-production-best-practices.md) | 生产最佳实践 |
| [99-kubernetes-version-lifecycle-support-policy.md](32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-01-cluster-fundamentals/04-api-versions/03-kubernetes-version-lifecycle-support-policy.md) | 版本生命周期 |
| [99-kubernetes-v1.33-ecosystem-compatibility-matrix.md](17-kubernetes-v1.33-ecosystem-compatibility-matrix.md) | 兼容性矩阵 |

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubernetes-v1.33-practical-cookbook
- 99-kubernetes-v1.33-production-best-practices
- 99-kubernetes-v1.33-upgrade-guide
- 99-kubernetes-version-lifecycle-support-policy


<!-- risk-assessed -->
