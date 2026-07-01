---
title: 70 - RuntimeClass配置
description: '# 70 - RuntimeClass配置'
summary: '# 70 - RuntimeClass配置'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- containerd
- gpu
- cuda
- nvidia
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- RuntimeClass配置 是什么
- 如何 RuntimeClass配置
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- RuntimeClass配置
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---



# 70 - RuntimeClass配置

<!-- chunk: RuntimeClass概述 -->
## RuntimeClass概述

| 字段 | 说明 |
|-----|------|
| `handler` | 运行时处理器名称(与CRI配置对应) |
| `overhead` | 运行时额外资源开销 |
| `scheduling` | 调度约束(nodeSelector/tolerations) |

<!-- chunk: RuntimeClass配置 -->
## RuntimeClass配置

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
scheduling:
  nodeSelector:
    runtime: gvisor
  tolerations:
  - key: runtime
    value: gvisor
    effect: NoSchedule
```

<!-- chunk: 常用RuntimeClass -->
## 常用RuntimeClass

| 名称 | Handler | 用途 | 隔离级别 |
|-----|---------|------|---------|
| runc | runc | 默认运行时 | 进程隔离 |
| gvisor | runsc | 安全沙箱 | 内核隔离 |
| kata | kata-runtime | 轻量级VM | 虚拟化隔离 |
| nvidia | nvidia | GPU容器 | 进程隔离 |
| [[WasmEdge|wasmedge]] | wasmedge | WebAssembly | Wasm沙箱 |

<!-- chunk: containerd配置 -->
## containerd配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes]

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
    runtime_type = "io.containerd.runsc.v1"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
      TypeUrl = "io.containerd.runsc.v1.options"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
    privileged_without_host_devices = true
```

<!-- chunk: gVisor RuntimeClass -->
## gVisor RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
---
# 使用gVisor的Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: nginx
    resources:
      limits:
        cpu: "1"
        memory: 512Mi
```

<!-- chunk: Kata Containers RuntimeClass -->
## Kata Containers RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata-runtime
overhead:
  podFixed:
    cpu: "500m"
    memory: "160Mi"
scheduling:
  nodeSelector:
    kata-runtime: "true"
```

<!-- chunk: 运行时对比 -->
## 运行时对比

| 特性 | runc | gVisor | Kata |
|-----|------|--------|------|
| 启动时间 | <100ms | <500ms | 1-2s |
| 内存开销 | 0 | ~50MB | ~100MB |
| 系统调用兼容性 | 100% | ~90% | ~99% |
| 性能开销 | 0 | 5-30% | 10-20% |
| 安全隔离 | 低 | 高 | 最高 |
| 适用场景 | 通用 | 多租户 | 高安全 |

<!-- chunk: NVIDIA RuntimeClass -->
## NVIDIA RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: nvidia
handler: nvidia
scheduling:
  nodeSelector:
    nvidia.com/gpu.present: "true"
---
# GPU Pod
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
spec:
  runtimeClassName: nvidia
  containers:
  - name: cuda
    image: nvcr.io/nvidia/cuda:12.0-base
    resources:
      limits:
        nvidia.com/gpu: 1
```

<!-- chunk: WebAssembly RuntimeClass -->
## WebAssembly RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmedge
handler: wasmedge
overhead:
  podFixed:
    cpu: "50m"
    memory: "10Mi"
---
# Wasm Pod
apiVersion: v1
kind: Pod
metadata:
  name: wasm-app
spec:
  runtimeClassName: wasmedge
  containers:
  - name: wasm
    image: myregistry/wasm-app:v1
```

<!-- chunk: 验证运行时 -->
## 验证运行时

```bash
# 查看RuntimeClass
kubectl get runtimeclass

# 查看Pod使用的运行时
kubectl get pod <pod-name> -o jsonpath='{.spec.runtimeClassName}'

# 检查节点运行时
crictl info | jq '.config.containerd.runtimes'

# 测试运行时
kubectl run test --image=nginx --runtime-class=gvisor --rm -it -- cat /proc/version
```

<!-- chunk: ACK运行时支持 -->
## ACK运行时支持

| 运行时 | 支持状态 | 说明 |
|-------|---------|------|
| containerd | ✅ 默认 | 标准运行时 |
| 安全沙箱 | ✅ | 基于Kata的隔离 |
| 神龙裸金属 | ✅ | 高性能计算 |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.20 | RuntimeClass GA |
| v1.24 | RuntimeClass overhead改进 |
| v1.27 | 用户命名空间支持 |
| v1.29 | Wasm运行时支持改进 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-02-workloads-applications KUDIG Database — Global MOC
- [[domain-02-workloads-applications/README.md|Domain-4: Kubernetes工作负载管理]]
- index.md|Domain-4 工作负载 — 开源项目索引]]
- 01 - [[concepts/kubernetes-architecture-overview.md|kubernetes architecture overview]]
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 14-sidecar-containers-patterns
- 15-container-runtime-interfaces
- 17-container-images-registry
- 18-node-management-operations
