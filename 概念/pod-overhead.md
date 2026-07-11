---
title: Pod Overhead
summary: Pod Overhead 是 Kubernetes 中用于计算 Pod 运行时额外资源开销的机制。
category: concepts
tags:
- pod
- runtime
- resource
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# Pod Overhead

## 概述

Pod Overhead 是 Kubernetes 中用于计算 Pod 运行时**额外资源开销**的机制。容器运行时本身（如 containerd 的进程、pause 容器、运行时 shim）会消耗 CPU 和内存，这些开销在传统的 `resources.requests` 中不会被计入。Pod Overhead 通过 RuntimeClass 将这些隐形成本显式化，使调度器的资源计算更加精确。

## 技术原理

### 资源计算公式

调度器和 kubelet 在计算 Pod 总资源需求时，采用以下公式：

```
Pod 总资源需求 = max(所有 container.requests 之和, Pod overhead) + 所有 container.requests 之和
```

更精确地说，每个 Pod 的有效请求值为：

```
effectivePodRequest = sum(container.requests) + podOverhead
```

其中 `podOverhead` 来自 RuntimeClass 中定义的 `overhead` 字段。调度器使用 `effectivePodRequest` 进行节点选择和资源记账，确保节点不会因为忽略了运行时开销而过度超卖。

### 为什么需要 Pod Overhead

| 场景 | 无 Pod Overhead | 有 Pod Overhead |
|------|----------------|----------------|
| Kata Containers | 每个容器额外占用 ~256MB 内存（VM 开销），调度器不可见 | 显式声明 256MB overhead，精确调度 |
| gVisor | 每个沙箱额外 CPU 消耗，节点易超载 | 声明 CPU overhead，避免节点压力 |
| WASM shim | 轻量但仍有固定开销 | 准确计量资源使用 |

## 生产示例

### 定义带 Overhead 的 RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
overhead:                          # 关键字段
  podFixed:
    cpu: "250m"                    # 每个 Pod 固定额外 0.25 核
    memory: "256Mi"                # 每个 Pod 固定额外 256MB
```

### 使用该 RuntimeClass 的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-workload
spec:
  runtimeClassName: kata-containers  # 引用 RuntimeClass
  containers:
    - name: app
      image: myapp:1.0
      resources:
        requests:
          cpu: "500m"
          memory: "1Gi"
```

此 Pod 的**有效资源请求**为：CPU = 500m + 250m = 750m，Memory = 1Gi + 256Mi = 1.25Gi。调度器按 750m / 1.25Gi 进行节点选择和记账。

### 验证 Overhead 计入

```bash
# 🟢 低风险：只读/信息收集
kubectl get pod secure-workload -o jsonpath='{.overhead}' 
# 输出: {"cpu":"250m","memory":"256Mi"}

kubectl describe node <node-name> | grep -A5 "Allocated"
# 确认 Overhead 已计入节点已分配资源
```

## 最佳实践

- **为非标准运行时配置 overhead**：Kata Containers、gVisor 等强隔离运行时必须设置 overhead，否则节点会超卖。标准 runc 容器 overhead 极小（约 10-20MB），通常不需要单独设置
- **基于实测设置数值**：通过 `crictl stats` 和 `/sys/fs/cgroup` 数据测量真实开销，不要估算。不同工作负载模式的开销有差异，建议取 P99 值
- **区分轻量与重量级运行时**：普通 runc 容器开销极小（通常不需要 overhead），安全沙箱运行时开销显著（Kata 约 256MB+，gVisor 约 128MB+）
- **与 Limit Range 配合**：结合 LimitRange 确保 overhead 不会导致 Pod 超出节点可分配范围。在命名空间层面设置资源配额上限
- **定期重新校准**：随着运行时版本更新，overhead 值可能变化——每季度重新测量并更新 RuntimeClass

## 常见陷阱

- **RuntimeClass 不存在导致 Pod 创建失败**：如果引用了不存在的 runtimeClassName，Pod 会一直处于 Pending 状态，报错 `RuntimeClass "xxx" not found`——部署前确认节点上运行时已安装且 RuntimeClass 已创建
- **Overhead 与 Limit 的交互**：Pod Overhead 不计入 container 的 `limits`，但会计入 cgroup 的 pod-level 限制，可能导致 OOM——需要在 container limit 中预留 overhead 余量
- **旧版本兼容性**：Pod Overhead 自 K8s 1.18 进入 beta，1.24 GA——1.18 之前版本不支持。多版本集群需注意兼容性

## 技术深度解析

### Overhead 在资源记账链路中的传递

```
定义链路:
  RuntimeClass.overhead → Pod.spec.overhead → kubelet → containerd → cgroup

记账链路:
  调度器: effectivePodRequest = sum(container.requests) + podOverhead
    → 用于节点选择和资源记账
  kubelet: Pod cgroup limit = sum(container.limits) + podOverhead
    → 用于 cgroup 级别的资源限制
  Eviction Manager: 考虑 overhead 后计算节点可用资源
    → 影响驱逐决策
```

### 不同运行时的 Overhead 参考值

| 运行时 | CPU Overhead | Memory Overhead | 说明 |
|--------|-------------|-----------------|------|
| runc (默认) | ~0 | ~0 | 通常不需要设置 |
| crun | ~0 | ~0 | 比 runc 更轻量 |
| Kata Containers | 250-500m | 256-512Mi | 每个轻量级 VM 的固定开销 |
| gVisor | 100-200m | 128-256Mi | 用户态内核的开销 |
| WASM shim | 50-100m | 32-64Mi | WASM 运行时的开销 |

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/container-runtime-evolution.md|容器运行时演进]] — RuntimeClass 与运行时选型
- [[概念/containerd-pod-lifecycle.md|containerd Pod 生命周期]] — 运行时层资源管理

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
