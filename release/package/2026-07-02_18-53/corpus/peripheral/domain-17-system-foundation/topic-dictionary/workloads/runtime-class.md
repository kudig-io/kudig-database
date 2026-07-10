---
title: 运行时类（RuntimeClass）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- containerd
- cri-o
- opa
- rbac
- operator
- gpu
- nvidia
tier: peripheral
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 运行时类（RuntimeClass） 是什么
- 如何 运行时类（RuntimeClass）
trigger_keywords:
- 运行时类
- RuntimeClass
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 运行时类（RuntimeClass）

## 概述

RuntimeClass 是 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 中用于选择容器运行时配置的特性（自 v1.20 起进入 Stable）。它允许用户为不同的 Pod 指定不同的容器运行时配置，从而在性能与安全性之间取得平衡。

## 核心概念/原理

### 设计动机

- **安全隔离**：对于需要高信息安全保障的工作负载，可以将其调度到使用硬件虚拟化的容器运行时（如 Kata Containers、gVisor），以换取更强的隔离性
- **运行时差异化配置**：即使使用同一种容器运行时，也可以通过不同的 RuntimeClass 应用不同的设置

### 配置步骤

使用 RuntimeClass 需要完成两个步骤：

1. **在节点上配置 CRI 实现**：具体的配置取决于所使用的容器运行时（如 [[containerd|containerd]]、CRI-O）
2. **创建 RuntimeClass 资源**：为每种运行时配置创建对应的 RuntimeClass 对象

### RuntimeClass 资源定义

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: myclass          # 引用该 RuntimeClass 时使用的名称
handler: myconfiguration # 对应 CRI 实现中的 handler 名称
```

- `name` 必须是有效的 DNS 子域名
- `handler` 必须是有效的 DNS 标签名
- RuntimeClass 是集群级别的非命名空间资源
- 建议将 RuntimeClass 的写操作限制给集群管理员

### 运行时配置示例

**containerd**：在 `/etc/containerd/config.toml` 中配置
```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.${HANDLER_NAME}]
```

**CRI-O**：在 `/etc/crio/crio.conf` 中配置
```ini
[crio.runtime.runtimes.${HANDLER_NAME}]
  runtime_path = "${PATH_TO_BINARY}"
```

## 关键机制或特性

### Pod 中指定 RuntimeClass

在 Pod 的 `spec` 中通过 `runtimeClassName` 字段指定要使用的 RuntimeClass：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mypod
spec:
  runtimeClassName: myclass
  # ...
```

- 若指定的 RuntimeClass 不存在，或 CRI 无法运行对应的 handler，Pod 将进入 `Failed` 终止状态
- 若未指定 `runtimeClassName`，则使用默认的 RuntimeHandler，等同于禁用 RuntimeClass 功能时的行为

### 调度约束（Scheduling）

通过 RuntimeClass 的 `scheduling` 字段，可以设置约束，确保使用该 RuntimeClass 的 Pod 被调度到支持它的节点上：

- **`nodeSelector`**：与 Pod 的 `nodeSelector` 取交集，确保 Pod 落在具有特定标签的节点上
- **`tolerations`**：与 Pod 的 `tolerations` 取并集，允许 Pod 容忍特定节点的污点

> **注意**：默认情况下，RuntimeClass 假设集群节点配置是同质的；若节点异构，应通过 `scheduling` 字段进行约束。

### Pod 开销（[[domain-17-system-foundation/知识字典/scheduling/pod-overhead.md|Pod Overhead]]）

自 v1.24 起进入 Stable。RuntimeClass 支持通过 `overhead` 字段声明运行 Pod 所需的额外资源开销（如虚拟化层消耗的资源），使调度器和其他组件在决策时能够将其纳入考量：

```yaml
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
```

## 使用场景

- **高安全隔离工作负载**：将敏感应用调度到基于 VM 的容器运行时（如 Kata Containers）
- **GPU 或特定硬件加速运行时**：为需要 NVIDIA GPU 的 Pod 指定 `nvidia` runtime handler
- **多运行时混合集群**：在同一集群中同时运行普通 runc 容器和沙箱容器，通过 RuntimeClass 进行区分

## 最佳实践/注意事项

- **限制 RuntimeClass 的写权限**：由于 RuntimeClass 影响节点级运行时行为，建议仅允许集群管理员创建和修改
- **确保节点标签与 RuntimeClass 的 nodeSelector 匹配**：在异构集群中，避免 Pod 因找不到匹配节点而长时间 Pending
- **准确声明 Pod Overhead**：低估开销可能导致节点资源耗尽，高估则造成资源浪费
- **验证 CRI handler 可用性**：在创建 RuntimeClass 前，确认对应 handler 已在目标节点的容器运行时中正确配置

## 生产 YAML 示例

### 完整的多运行时配置

```yaml
# 1. 默认 runc RuntimeClass（可选，显式声明）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: runc
handler: runc
---
# 2. Kata Containers RuntimeClass（VM 级隔离）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
overhead:
  podFixed:
    cpu: "250m"
    memory: "160Mi"              # Kata VM 本身的资源开销
scheduling:
  nodeSelector:
    runtime.kata: "true"         # 仅调度到安装了 Kata 的节点
  tolerations:
  - key: runtime
    operator: Equal
    value: kata
    effect: NoSchedule
---
# 3. gVisor RuntimeClass（用户空间内核隔离）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "100m"
    memory: "80Mi"
scheduling:
  nodeSelector:
    runtime.gvisor: "true"
---
# 4. NVIDIA GPU RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: nvidia
handler: nvidia
scheduling:
  nodeSelector:
    nvidia.com/gpu.present: "true"
```

### 按安全等级分配运行时

```yaml
# 不受信任的工作负载 → Kata Containers（VM 级隔离）
apiVersion: v1
kind: Pod
metadata:
  name: untrusted-tenant-app
  namespace: tenant-a
spec:
  runtimeClassName: kata-containers
  securityContext:
    runAsNonRoot: true
    runAsUser: 65534
  containers:
  - name: app
    image: registry.tenant-a.com/app:v1.0
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1"
        memory: "1Gi"
---
# 受信任的内部服务 → 默认 runc（最低开销）
apiVersion: v1
kind: Pod
metadata:
  name: internal-api
  namespace: platform
spec:
  # runtimeClassName 不设置，使用默认 handler
  containers:
  - name: api
    image: registry.example.com/internal/api:v3.0
    resources:
      requests:
        cpu: "200m"
        memory: "256Mi"
```

## 运行时隔离级别对比

| 运行时 | 隔离级别 | 性能开销 | Pod Overhead | 适用场景 |
|--------|---------|---------|--------------|----------|
| runc | 进程级（namespaces + cgroups） | 最低 | 无 | 受信任的内部服务 |
| gVisor (runsc) | 用户空间内核 | 低~中 | ~80-100Mi | 多租户、不受信任代码 |
| Kata Containers | VM 级（轻量虚拟机） | 中 | ~160-250Mi | 高安全要求、合规场景 |
| Firecracker | microVM | 中 | ~128Mi | Serverless、FaaS |
| WASM (spin/wasmtime) | 沙箱 | 极低 | ~10-30Mi | 轻量函数、边缘计算 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 进入 Failed 状态，Events 显示 handler 未找到 | 节点未安装对应运行时或 handler 名称不匹配 | 检查 containerd/CRI-O 配置中的 handler 名称 |
| Pod Pending 且调度失败 | RuntimeClass 的 nodeSelector 与节点标签不匹配 | `kubectl get nodes --show-labels` 确认标签；`kubectl describe rc <name>` |
| Pod 资源使用超预期 | Pod Overhead 未正确设置 | `kubectl get runtimeclass -o yaml` 检查 overhead 值 |
| 性能下降（高延迟） | 使用了 VM 级运行时但应用对延迟敏感 | 评估是否需要该隔离级别；考虑降级到 gVisor 或 runc |
| 节点 NotReady | 运行时组件崩溃 | `systemctl status containerd`；检查 handler 进程状态 |

## 生产检查清单

- [ ] 每个 RuntimeClass 的 `handler` 名称与 CRI 配置中的 handler 完全匹配
- [ ] 支持该 handler 的节点已正确打标签（与 `scheduling.nodeSelector` 对应）
- [ ] `overhead.podFixed` 值准确反映运行时实际资源消耗
- [ ] RuntimeClass 的写权限仅限集群管理员（RBAC）
- [ ] 多租户场景下通过准入策略（OPA/Kyverno）强制不受信任命名空间使用沙箱运行时
- [ ] 监控不同运行时节点的资源利用率和 Pod 启动延迟
- [ ] 升级运行时组件前在非生产环境验证兼容性
- [ ] 异构集群中 RuntimeClass 的 tolerations 与节点 taints 一致

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有 RuntimeClass
kubectl get runtimeclasses

# 查看 RuntimeClass 详情（含 overhead 和 scheduling）
kubectl get runtimeclass kata-containers -o yaml

# 查看 Pod 使用的 RuntimeClass
kubectl get pod <name> -o jsonpath='{.spec.runtimeClassName}'

# 检查节点上可用的 handler
# containerd:
sudo cat /etc/containerd/config.toml | grep -A2 'runtimes\.'
# CRI-O:
sudo cat /etc/crio/crio.conf | grep -A2 '\[crio.runtime.runtimes'

# 验证节点标签匹配
kubectl get nodes -l runtime.kata=true

# 查看 Pod Overhead 对调度的影响
kubectl describe node <name> | grep -A5 "Allocated resources"

# 创建测试 Pod 验证运行时
kubectl run test-kata --image=busybox --runtime=kata-containers -- sleep 3600
kubectl exec test-kata -- uname -r    # Kata 会显示 guest kernel 版本
```
## 交叉引用

- [容器运行时接口](container-runtime-interface-cri.md) — CRI handler 的底层配置
- [高级 Pod 配置](advanced-pod-configuration.md) — PriorityClass 和 SecurityContext 的配合
- [容器镜像](images.md) — 按 RuntimeClass 拉取镜像的特性（Alpha）
- [Spot 与可抢占工作负载](spot-and-preemptible-workloads.md) — 运行时与节点类型的配合

## 参考链接

- [Kubernetes 官方文档：RuntimeClass](https://kubernetes.io/docs/concepts/containers/runtime-class/)
- [RuntimeClass API 参考](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/runtime-class-v1/)
- [Pod Overhead 概念文档](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-overhead/)
- [Assigning Pods to Nodes](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)

## Related

- [[domain-17-system-foundation/知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[domain-17-system-foundation/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[domain-17-system-foundation/知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
