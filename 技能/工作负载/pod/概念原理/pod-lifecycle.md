---
title: Pod Lifecycle (concepts)
description: '- [[概念/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
summary: '- [[概念/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合'
category: concepts
tags:
- k8s
- pod
- lifecycle
- containers
- probes
- kubelet
- statefulset
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Lifecycle 是什么
- 如何 Pod Lifecycle
trigger_keywords:
- Pod
- Lifecycle
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Lifecycle

## State Machine

[[Pods|Pods]] transition through these phases:

| Phase | Meaning |
|-------|---------|
| **Pending** | Pod accepted by API Server but not yet running (waiting for scheduling, image pull, or volume mount) |
| **Running** | At least one container is running |
| **Succeeded** | All containers exited successfully (code 0) |
| **Failed** | At least one container exited with non-zero code |
| **Unknown** | Pod state cannot be determined (usually node communication failure) |

## Conditions

Each Pod has four condition types that describe its internal state:
- **PodScheduled**: Pod assigned to a node
- **Initialized**: All init containers completed
- **ContainersReady**: All containers passed readiness
- **Ready**: Pod can accept traffic (subset of ContainersReady + network ready)

## Container Startup Sequence

1. **Init Containers** run sequentially (each must succeed before next starts)
2. **Main Containers** start in parallel after all init containers complete
3. **Sidecar Containers** (v1.28+) can run alongside init containers in parallel

## Health Probes

| Probe | Purpose | Trigger | Impact |
|-------|---------|---------|--------|
| **startupProbe** | Allow slow startup | Runs until first success | Disables other probes during startup |
| **livenessProbe** | Detect deadlocked/stuck processes | Periodic check | Container restart |
| **readinessProbe** | Determine if container can accept traffic | Periodic check | Remove from [[Service|Service]] endpoints |

Each probe can use HTTP GET, TCP Socket, or Exec commands.

## Termination Flow

1. Pod marked for deletion
2. **PreStop hook** executes (if defined)
3. **SIGTERM** sent to all containers
4. Wait for `terminationGracePeriodSeconds` (default 30s)
5. **SIGKILL** sent if still running
6. Resources cleaned up by [[kubelet|kubelet]]

## 源码实现分析

### kubelet Pod 启动流程

```go
// kubernetes/pkg/kubelet/kubelet.go
func (kl *Kubelet) syncPod(pod *v1.Pod, podStatus *kubecontainer.PodStatus) {
    // 1. 创建 Pod 数据目录
    kl.makePodDataDirs(pod)
    
    // 2. 挂载 Volume（CSI）
    kl.volumeManager.WaitForAttachAndMount(pod)
    
    // 3. 拉取 Secret/ConfigMap
    kl.secretManager.GetSecrets(pod)
    
    // 4. 调用 CRI 创建 Pod Sandbox + 容器
    result := kl.containerRuntime.SyncPod(pod, podStatus)
    // 内部: RunPodSandbox → PullImage → CreateContainer → StartContainer
    
    // 5. 启动探针监控
    kl.probeManager.AddPod(pod)
    // startupProbe → livenessProbe → readinessProbe
}
```

### Pod 状态机

```
┌─────────┐   调度成功   ┌─────────┐   容器启动   ┌─────────┐
│ Pending │───────────►│ Running │───────────►│ Running │
│(未调度) │            │(启动中) │            │(就绪)   │
└─────────┘            └─────────┘            └────┬────┘
                                                  │ 删除/失败
                                                  ▼
┌─────────┐   优雅终止   ┌──────────────┐
│ Succeeded│◄───────────│ Terminating  │
│ / Failed │            │(PreStop+TERM)│
└─────────┘            └──────────────┘
```

## 使用场景

### 场景一：优雅终止配置

```yaml
apiVersion: v1
kind: Pod
spec:
  terminationGracePeriodSeconds: 60  # 给应用 60s 清理
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 5 && /app/graceful-shutdown"]
          # sleep 5: 等待 kube-proxy 更新 iptables 规则
          # graceful-shutdown: 排干存量请求
    readinessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
```

### 场景二：诊断 Pod 状态

```bash
# 🟢 低风险 - 查看 Pod 事件时间线
kubectl describe pod <pod> | grep -A 30 Events

# 🟢 低风险 - 查看容器状态转换
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[*].state}'

# 🟢 低风险 - 查看 Pod 条件
kubectl get pod <pod> -o jsonpath='{.status.conditions}' | jq .
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Pod 删除立即终止 | 先标记 Terminating，执行 PreStop + SIGTERM，等待 grace period |
| readinessProbe 失败重启容器 | 失败只从 Service Endpoints 移除，不重启（liveness 才重启） |
| startupProbe 和 initialDelay 相同 | startupProbe 更灵活：失败多次才杀，不影响运行后探针 |
| Pod IP 在重启后不变 | 容器重启 IP 不变，但 Pod 删除重建后 IP 会变 |
| initContainer 失败不影响 Pod | init 失败导致 Pod 停在 Init 状态，不会启动主容器 |
| terminationGracePeriod 总是等待 | 超过 grace period 后 SIGKILL 强杀，不管是否完成清理 |

## 面试要点

1. **Pod 从创建到运行的完整链路？** — API Server 写入 etcd → Scheduler Watch 到未调度 Pod → Filter+Score 选节点 → Bind → kubelet Watch 到新 Pod → 创建 Volume → CRI 创建 Sandbox(pause) → CNI 配置网络 → 拉取镜像 → 创建容器 → 启动探针监控 → Ready。

2. **优雅终止为什么需要 sleep？** — Pod 删除与 Endpoints 更新是并行的。sleep 5 等待 kube-proxy 更新 iptables/IPVS 规则，确保新流量不再路由到该 Pod，然后再排干存量请求。否则最后几个请求可能失败。

3. **三种探针的作用和区别？** — startupProbe：启动期检测，失败则重启（替代 initialDelaySeconds）；livenessProbe：运行期健康检测，失败则重启容器；readinessProbe：就绪检测，失败则从 Service 移除（不重启）。

4. **Pod 的 QoS 如何影响生命周期？** — Guaranteed（requests=limits）最后被驱逐；BestEffort（无 requests）最先被驱逐。节点内存压力时，kubelet 按 QoS 优先级驱逐 Pod。

## Related
- [[概念/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合
- [[概念/Pod 生命周期 × Secret 管理.md|Pod 生命周期 × Secret 管理]] — 综合
- [[概念/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]] — 综合

- [[技能/工作负载/pod/诊断排障/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — Troubleshoot Pod Issues
- [[概念/node-lifecycle-management.md|node-lifecycle-management]] — 节点生命周期管理
- [[实体/kubelet.md|kubelet]] — kubelet
- [[概念/high-availability-patterns.md|high-availability-patterns]] — High Availability Patterns
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|configure-health-probes]] — Configure Health Probes
- [[deployment|Deployment]]
- [[实体/statefulset.md|StatefulSet]]
- [[概念/high-availability-patterns.md|High Availability Patterns]]
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]
- [[实体/kubelet.md|kubelet]]

- Pod 生命周期事件表
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[实体/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[实体/workloads-terms.md|K8s 工作负载术语参考]] — Cross-reference
- [[实体/k8s-architecture-domain-guide.md|Kubernetes Architecture Domain Guide]] — Cross-reference
- [[实体/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- [[概念/scheduling-algorithm.md|Scheduling Algorithm]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-inner-training.md|Kubernetes 培训：Inner Training]] — Cross-reference
- [[技能/节点/node/运维操作/kubelet-eviction-mechanism.md|kubelet 资源驱逐机制]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-public-training.md|Kubernetes 培训：Public Training]] — Cross-reference
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference
- [[实体/container-runtime.md|Container Runtime]] — Cross-reference
- [[实体/clusterpedia.md|Clusterpedia]] — Cross-reference
- [[生态参考/领域索引/pod-index.md|Pod 知识图谱索引]]


<!-- risk-assessed -->
