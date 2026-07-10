---
title: Advanced Pod Configuration
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- coredns
- operator
- gpu
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Advanced Pod Configuration 是什么
- 如何 Advanced Pod Configuration
trigger_keywords:
- Advanced
- Pod
- Configuration
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Advanced Pod Configuration

## 概述
本页涵盖 Pod 的高级配置主题，包括 PriorityClass、RuntimeClass、安全上下文（security context）以及影响 Pod 调度的相关机制。

## 核心概念/原理
- **PriorityClass**：集群范围的 API 对象，将优先级名称映射为整数值。数值越高优先级越高。当资源不足时，kube-scheduler 可抢占（驱逐）低优先级 Pod 以调度高优先级 Pod。
  - 内置类：`system-cluster-critical`（集群关键系统组件）、`system-node-critical`（节点关键组件，最高优先级）。
- **RuntimeClass**：允许为 Pod 指定低级别容器运行时，适用于需要不同隔离级别或运行时特性的场景（如 Kata Containers、gVisor）。
- **安全上下文（Security Context）**：
  - **Pod 级**：`pod.spec.securityContext` 应用于整个 Pod，可设置 `runAsUser`、`runAsGroup`、`fsGroup`、SELinux、seccomp 等。
  - **容器级**：`container.securityContext` 可对单个容器进行更细粒度的控制，如 `capabilities` 增删、`allowPrivilegeEscalation`、`runAsNonRoot`、AppArmor 等。
- **调度影响机制**：
  - `nodeSelector`：最简单的节点选择约束。
  - `nodeAffinity`：基于节点标签的复杂约束（优先/强制）。
  - `podAffinity` / `podAntiAffinity`：基于其他 Pod 标签的 placement 约束。
  - `tolerations`：允许 Pod 调度到带有匹配 taint 的节点上。
- **[[系统基础/知识字典/scheduling/pod-overhead.md|Pod Overhead]]**：记录 Pod 基础设施本身消耗的资源（超出容器请求/限制的部分），由 RuntimeClass 定义。

## 关键机制或特性
- **特权模式（Privileged Mode）**：`securityContext` 中可启用特权模式，但会覆盖许多其他安全设置，应尽量避免，优先使用细粒度权限配置。
- **Windows HostProcess**：通过 `windowsOptions.hostProcess` 在 Windows 上运行特权容器。

## 使用场景
- 需要保证关键业务优先调度时使用 PriorityClass。
- 对安全隔离有更高要求时，使用 RuntimeClass 切换至沙箱运行时。
- 需要 Pod 运行在特定硬件（GPU、SSD）或特定拓扑区域时，使用 Affinity 和 Taints/Tolerations。
- 多租户环境中通过 Security Context 加固容器安全。

## 最佳实践/注意事项
- 尽量避免使用特权容器；使用 capabilities、seccomp、AppArmor 等细粒度控制。
- 为系统组件预留 `system-cluster-critical` 或 `system-node-critical` 优先级。
- 使用 `podAntiAffinity` 将同一应用的副本分散到不同节点/可用区，提高容错性。
- 配置 RuntimeClass 前，需确保集群管理员已在相应节点上安装并配置好底层运行时。

## 生产 YAML 示例

### PriorityClass + SecurityContext + RuntimeClass 综合配置

```yaml
# 1. 定义 PriorityClass
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority-app
value: 1000000                     # 高于默认（0），低于系统级
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "业务关键应用优先级"
---
# 2. 使用 RuntimeClass（可选，沙箱运行时）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata
overhead:
  podFixed:
    cpu: "250m"
    memory: "160Mi"
scheduling:
  nodeSelector:
    runtime: kata
---
# 3. 综合 Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: production
spec:
  priorityClassName: high-priority-app
  runtimeClassName: kata-containers      # 可选：使用沙箱运行时
  securityContext:                        # Pod 级安全上下文
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: registry.example.com/apps/secure-service:v2.1
    securityContext:                      # 容器级安全上下文
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
        add: ["NET_BIND_SERVICE"]        # 仅添加必要的 capability
    ports:
    - containerPort: 8443
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1"
        memory: "1Gi"
    volumeMounts:
    - name: tmp
      mountPath: /tmp
    - name: cache
      mountPath: /app/cache
  volumes:
  - name: tmp
    emptyDir: {}
  - name: cache
    emptyDir:
      sizeLimit: 100Mi
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "secure-workloads"
    effect: "NoSchedule"
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: topology.[[entities/kubernetes.md|kubernetes]].io/zone
            operator: In
            values: ["us-east-1a", "us-east-1b"]
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchLabels:
              app: secure-app
          topologyKey: kubernetes.io/hostname    # 分散到不同节点
```

### 内置 PriorityClass 参考

| PriorityClass | 优先级值 | 用途 |
|--------------|---------|------|
| `system-node-critical` | 2000001000 | 节点关键组件（如 kube-proxy） |
| `system-cluster-critical` | 2000000000 | 集群关键组件（如 [[CoreDNS|CoreDNS]]） |
| 自定义高优先级 | 1000000 | 业务关键应用 |
| 默认（无设置） | 0 | 普通工作负载 |
| 自定义低优先级 | -100 | 可牺牲的批处理任务 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 因 SecurityContext 失败启动 | `readOnlyRootFilesystem: true` 但应用需要写入特定目录 | 为需写入的目录挂载 emptyDir；`kubectl logs` 查看权限错误 |
| 高优先级 Pod 抢占了关键服务 | PriorityClass 值设置过高 | `kubectl get pc` 审查所有 PriorityClass 的值分布 |
| Pod Pending 且 Events 显示 RuntimeClass 错误 | 节点未安装对应 handler 或 nodeSelector 不匹配 | `kubectl describe pod` 查看调度失败原因；确认目标节点标签 |
| 容器以 root 运行 | `runAsNonRoot` 未设置或镜像 USER 为 root | 在 securityContext 中设置 `runAsNonRoot: true` 和 `runAsUser` |
| Pod 无法调度到特定节点 | toleration 与节点 taint 不匹配 | `kubectl describe node` 查看 taints；对比 Pod tolerations |

## 生产检查清单

- [ ] 所有容器设置 `allowPrivilegeEscalation: false`
- [ ] 生产 Pod 设置 `runAsNonRoot: true` 和明确的 `runAsUser`
- [ ] 启用 `readOnlyRootFilesystem`，必要写入目录用 emptyDir/PVC 挂载
- [ ] capabilities 使用 `drop: ["ALL"]` 后仅添加必要项
- [ ] 配置 `seccompProfile: RuntimeDefault` 或自定义 profile
- [ ] PriorityClass 层次设计合理，避免业务应用使用 system-* 级别
- [ ] 使用 podAntiAffinity 将同一应用副本分散到不同节点/可用区
- [ ] RuntimeClass 使用前确认节点 handler 已安装且 nodeSelector 正确
- [ ] Pod Overhead 值准确反映运行时实际开销

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有 PriorityClass
kubectl get priorityclasses

# 查看 Pod 的安全上下文
kubectl get pod <name> -o jsonpath='{.spec.securityContext}' | jq .

# 查看容器的 capabilities
kubectl get pod <name> -o jsonpath='{.spec.containers[0].securityContext.capabilities}' | jq .

# 查看 RuntimeClass 列表
kubectl get runtimeclasses

# 验证 Pod 运行用户
kubectl exec <pod> -- id

# 测试 readOnlyRootFilesystem（应该失败）
kubectl exec <pod> -- touch /test-write

# 检查节点 taints
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints[*].key'
```
## 交叉引用

- [RuntimeClass](runtime-class.md) — 运行时类的深入配置
- [Pod 生命周期](pod-lifecycle.md) — Pod 各阶段与安全上下文的交互
- [[系统基础/知识字典/workloads/pods.md|Pods]]](pods.md) — Pod 基础概念和配置
- [调度与约束](../scheduling/) — nodeAffinity、tolerations 详解

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/advanced-pod-config/

## Related

- [[系统基础/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[系统基础/知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]
- [[系统基础/知识字典/workloads/container-environment.md|容器环境（Container Environment）]]


<!-- risk-assessed -->
