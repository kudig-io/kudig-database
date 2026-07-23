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

## 源码实现分析

### kubelet Pod Overhead 资源记账

```go
// k8s.io/kubernetes/pkg/kubelet/kuberuntime/kuberuntime_manager.go
func (m *kubeGenericRuntimeManager) computePodOverhead(pod *v1.Pod) *v1.ResourceList {
    // 1. 查找 Pod 引用的 RuntimeClass
    if pod.Spec.RuntimeClassName == nil {
        return nil // 默认 runc 无 overhead
    }
    rc, err := m.runtimeClassLister.Get(*pod.Spec.RuntimeClassName)
    if rc.Overhead == nil {
        return nil
    }
    // 2. 返回 RuntimeClass 定义的 overhead
    return &rc.Overhead.PodFixed
}

// 调度器中的 overhead 计算
// k8s.io/kubernetes/pkg/scheduler/framework/plugins/noderesources/fit.go
func getPodRequest(pod *v1.Pod) *framework.Resource {
    result := &framework.Resource{}
    for _, container := range pod.Spec.Containers {
        result.Add(container.Resources.Requests)
    }
    // 关键：加上 Pod Overhead
    if pod.Spec.Overhead != nil {
        result.Add(pod.Spec.Overhead)
    }
    return result
}
// 节点可分配资源 = Allocatable - 已分配(含 overhead)
// 若忽略 overhead，节点会超卖 → OOM/驱逐
```

### Overhead 在资源链路中的传递

```
┌──────────────────────────────────────────────────────────┐
│          Pod Overhead 资源记账链路                    │
├──────────────────────────────────────────────────────────┤
│  RuntimeClass.overhead.podFixed                          │
│       │                                                  │
│       ▼                                                  │
│  Admission: Pod.spec.overhead 自动填充                  │
│       │                                                  │
│       ├─────────────────────┬────────────────────┐  │
│       ▼                     ▼                    ▼  │
│  Scheduler              kubelet              Eviction │
│  effectiveRequest =     cgroup limit =       Manager  │
│  Σ(containers.req)     Σ(containers.lim)   考虑      │
│  + overhead             + overhead           overhead  │
│  → 节点选择            → 资源限制          → 驱逐决策│
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：配置 Kata Containers RuntimeClass

```yaml
# 🟡 中风险：创建 RuntimeClass 影响调度计算
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata  # 对应 containerd 中的 runtime 名称
overhead:
  podFixed:
    cpu: "250m"      # 每个 Kata VM 的固定 CPU 开销
    memory: "256Mi"  # 每个 Kata VM 的固定内存开销
scheduling:
  nodeSelector:
    runtime.katacontainers.io: "true"  # 只调度到安装了 Kata 的节点
```

### 场景二：验证 Overhead 是否生效

```bash
# 🟢 低风险：只读验证
# 检查 Pod 的 overhead 字段
kubectl get pod secure-app -o jsonpath='{.spec.overhead}'
# 输出: {"cpu":"250m","memory":"256Mi"}
# 检查节点已分配资源（含 overhead）
kubectl describe node kata-node-01 | grep -A10 "Allocated resources"
# 检查实际 cgroup 限制
kubectl exec secure-app -- cat /sys/fs/cgroup/memory.max
# 对比：container limit + overhead = pod cgroup limit
```

### 场景三：测量运行时实际开销

```bash
# 🟢 低风险：只读测量
# 创建一个使用 Kata 的测试 Pod
kubectl run overhead-test --image=busybox --overrides='
  {"spec":{"runtimeClassName":"kata","containers":[{"name":"test","image":"busybox","command":["sleep","3600"],"resources":{"requests":{"cpu":"100m","memory":"64Mi"}}}]}}'
# 测量实际开销
crictl stats | grep overhead-test  # 查看实际 CPU/内存使用
# 对比 request (100m/64Mi) 与实际使用，差值即为 overhead
# 建议取 P99 值作为 RuntimeClass overhead 配置
kubectl delete pod overhead-test  # 🟡 清理测试 Pod
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | runc 容器也需要设置 overhead | runc overhead 极小（~10-20MB），通常不需要；只有安全沙箱运行时需要 |
| 2 | Overhead 计入 container limits | Overhead 不计入 container limits，但计入 pod-level cgroup，可能导致 OOM |
| 3 | 估算 overhead 值即可 | 必须基于实测（crictl stats + cgroup 数据）；不同负载模式开销有差异 |
| 4 | RuntimeClass 不存在只是警告 | 引用不存在的 RuntimeClass 会导致 Pod 永远 Pending（RuntimeClass not found） |
| 5 | Overhead 设置后不需要更新 | 运行时版本更新可能改变开销；应每季度重新测量并更新 |
| 6 | Overhead 只影响调度 | Overhead 影响三个环节：调度器节点选择、kubelet cgroup 限制、Eviction Manager 驱逐决策 |

## 面试要点

1. **Q: Pod Overhead 解决什么问题？为什么需要它？**
   A: 安全沙箱运行时（Kata/gVisor）每个 Pod 有固定的额外资源开销（VM 内核、虚拟设备、用户态内核）。若不计量这部分开销，调度器会超卖节点资源（认为节点还有空间，实际已被运行时开销占用），导致 OOM 或驱逐。Pod Overhead 让调度器、kubelet、Eviction Manager 都能正确计算实际资源使用。

2. **Q: Pod Overhead 在哪些环节生效？**
   A: 三个环节：① 调度器：effectiveRequest = Σ(container.requests) + overhead，用于节点选择和资源记账；② kubelet：Pod cgroup limit = Σ(container.limits) + overhead，用于 cgroup 级别资源限制；③ Eviction Manager：计算节点可用资源时考虑 overhead，影响驱逐决策。注意：overhead 不计入单个 container 的 limits。

3. **Q: 如何确定正确的 overhead 值？**
   A: 基于实测而非估算：① 创建测试 Pod（最小资源请求）；② 用 crictl stats 和 /sys/fs/cgroup 测量实际 CPU/内存使用；③ 实际使用 - 容器请求 = 运行时开销；④ 取 P99 值（考虑波动）；⑤ 不同工作负载模式分别测量；⑥ 定期重新校准（运行时更新后开销可能变化）。参考值：Kata ~250m CPU + 256Mi，gVisor ~100m + 128Mi。

4. **Q: Overhead 与 container limits 的交互可能导致什么问题？**
   A: 问题：Pod cgroup limit = Σ(container.limits) + overhead。若 container limit 设为 512Mi，overhead 为 256Mi，则 pod cgroup = 768Mi。但容器内部看到的 limit 仍是 512Mi，应用可能用满 512Mi，加上运行时 256Mi 开销，总计 768Mi 刚好触及 pod cgroup。若应用稍微超出，触发 pod-level OOM。解决：在 container limit 中预留 overhead 余量，或适当调大 overhead 值。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/container-runtime-evolution.md|容器运行时演进]] — RuntimeClass 与运行时选型
- [[概念/containerd-pod-lifecycle.md|containerd Pod 生命周期]] — 运行时层资源管理

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
