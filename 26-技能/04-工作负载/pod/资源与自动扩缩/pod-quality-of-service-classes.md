---
title: Pod Quality of Service Classes
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- vpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Quality of Service Classes 是什么
- 如何 Pod Quality of Service Classes
trigger_keywords:
- Pod
- Quality
- of
- Service
- Classes
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod Quality of [[service|Service]] Classes

## 概述
[[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] 根据 Pod 内容器的资源请求（requests）和限制（limits）为每个 Pod 分配一个服务质量（QoS）等级。该等级用于在节点资源不足时决定驱逐优先级。

## 核心概念/原理
可能的 QoS 等级有三种，按驱逐优先级从高到低排列：
1. **BestEffort**：没有任何容器设置 CPU 或内存的 request/limit，也没有 Pod 级资源设置。节点资源紧张时最优先被驱逐。
2. **Burstable**：不满足 Guaranteed 条件，但至少有一个容器或 Pod 设置了 CPU 或内存的 request/limit。
3. **Guaranteed**：最严格的资源约束，最不容易被驱逐；只有该等级可使用 `static` CPU 管理策略申请独占 CPU。

**Guaranteed 的判定条件**：
- 每个容器（或 Pod 级资源）必须同时设置内存 request 和 limit，且两者相等。
- 每个容器（或 Pod 级资源）必须同时设置 CPU request 和 limit，且两者相等。

## 关键机制或特性
- **节点压力驱逐**：当节点资源不足时，[[kubelet|kubelet]] 优先驱逐 `BestEffort`，其次是 `Burstable`，最后是 `Guaranteed`。仅超出自身 request 的 Pod 才会被驱逐。
- **资源超限处理**：任何容器超出其资源 limit 都会被 kubelet 终止并重启（如 OOM Kill 或 CPU 限流），不影响同一 Pod 内的其他容器。
- **QoS 不变性**：Pod 创建后 QoS 等级终身不变。若进行原地 resize 导致 QoS 变更，则 resize 会被拒绝。
- **Memory QoS（cgroup v2，Alpha）**：利用 `memory.min` 和 `memory.high` 保证内存可用性，与 QoS 等级协同工作但机制不同。

## 使用场景
- 对延迟敏感的负载应设置为 `Guaranteed`，以获得最强的资源保障和最低的驱逐风险。
- 可容忍一定资源波动的批处理或开发测试负载可设置为 `Burstable`。
- 非关键后台任务可使用 `BestEffort`，充分利用节点空闲资源。

## 最佳实践/注意事项
- 生产环境中关键应用建议配置 `Guaranteed` QoS。
- 设置 `Guaranteed` 时务必确保所有容器的 CPU 和内存 request/limit 完全相等。
- 调度器在进行抢占（preemption）时**不考虑** QoS 等级，抢占决策基于优先级和资源需求。
- 使用 Pod 级资源（Beta）简化 Guaranteed 配置时，需确保 Pod 级 request 和 limit 相等。

## 实战 YAML 示例

### Guaranteed QoS（生产关键应用）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: guaranteed-app
  namespace: prod
spec:
  containers:
  - name: app
    image: myregistry.com/critical-app:v1.0
    resources:
      requests:
        cpu: "500m"          # request == limit → Guaranteed
        memory: "512Mi"
      limits:
        cpu: "500m"          # 必须与 request 完全相等
        memory: "512Mi"      # 必须与 request 完全相等
```

### Burstable QoS（一般业务应用）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: burstable-app
  namespace: prod
spec:
  containers:
  - name: app
    image: myregistry.com/web-app:v1.0
    resources:
      requests:
        cpu: "250m"          # request < limit → Burstable
        memory: "256Mi"
      limits:
        cpu: "1000m"         # 允许突发使用更多 CPU
        memory: "1Gi"        # 允许突发使用更多内存
```

### BestEffort QoS（非关键后台任务）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: besteffort-task
  namespace: dev
spec:
  containers:
  - name: task
    image: myregistry.com/batch-task:v1.0
    # 不设置任何 resources → BestEffort
    # 注意：生产环境不建议使用 BestEffort
```

### QoS 等级对比矩阵

| 特性 | Guaranteed | Burstable | BestEffort |
|------|-----------|-----------|------------|
| 资源配置 | request == limit（全设） | 至少设置一项 | 不设置任何资源 |
| 驱逐优先级 | 最低（最后驱逐） | 中等 | 最高（最先驱逐） |
| CPU 限流 | 严格限制在 limit | 可突发到 limit | 无限制 |
| OOM Kill 顺序 | 最后被 Kill | 按 OOM [[score\|Score]] | 最先被 Kill |
| 独占 CPU | 支持（static CPU Manager） | 不支持 | 不支持 |
| 适用场景 | 延迟敏感、关键服务 | 一般业务 | 开发测试、非关键任务 |

## 故障排查

### Pod 被 OOMKilled
- **症状**: Pod 状态显示 `OOMKilled`，容器被反复重启。
- **常见原因**: 内存 limit 设置过低；应用存在内存泄漏；JVM 堆大小未与容器 limit 对齐。
- **诊断命令**:
  ```bash
  # 查看 Pod QoS 等级
  kubectl get pod <pod-name> -n prod -o jsonpath='{.status.qosClass}'
  
  # 检查容器退出原因
  kubectl get pod <pod-name> -n prod -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
  
  # 查看节点上的 OOM 事件
  kubectl describe node <node-name> | grep -A 5 "OOMKilling"
  
  # 查看 Pod 实际内存使用
  kubectl top pod <pod-name> -n prod --containers
  ```
- **解决方案**: 增大内存 limit；修复内存泄漏；Java 应用设置 `-XX:MaxRAMPercentage=75.0`。

### 意外的 QoS 等级
- **症状**: Pod 的 QoS 等级不是期望的 Guaranteed。
- **常见原因**: 多容器 Pod 中某个容器（包括 Sidecar）未设置资源，或 request != limit。
- **诊断命令**:
  ```bash
  # 查看 Pod QoS 等级
  kubectl get pod <pod-name> -n prod -o jsonpath='{.status.qosClass}'
  
  # 检查所有容器的资源配置
  kubectl get pod <pod-name> -n prod -o jsonpath='{range .spec.containers[*]}{.name}: requests={.resources.requests}, limits={.resources.limits}{"\n"}{end}'
  
  # 检查 init 容器（Sidecar）的资源配置
  kubectl get pod <pod-name> -n prod -o jsonpath='{range .spec.initContainers[*]}{.name}: requests={.resources.requests}, limits={.resources.limits}{"\n"}{end}'
  ```

### 节点资源不足导致大量驱逐
- **症状**: 多个 BestEffort/Burstable Pod 同时被驱逐。
- **诊断命令**:
  ```bash
  # 查看节点资源压力
  kubectl describe node <node-name> | grep -A 10 "Conditions"
  
  # 查看被驱逐的 Pod
  kubectl get pods -A --field-selector=status.phase=Failed -o wide | grep Evicted
  
  # 查看节点资源使用率
  kubectl top node <node-name>
  ```

## 生产就绪检查清单

- [ ] 关键服务（数据库、API 网关等）配置为 `Guaranteed` QoS
- [ ] 所有生产 Pod 至少设置了 `resources.requests`（避免 BestEffort）
- [ ] 多容器 Pod（含 Sidecar）的每个容器都设置了资源
- [ ] Java 应用的 JVM 堆大小与容器内存 limit 对齐（建议 `-XX:MaxRAMPercentage=75.0`）
- [ ] 监控节点资源使用率，设置驱逐前告警
- [ ] 了解 CPU limit 的限流权衡（部分团队选择不设置 CPU limit）

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 的 QoS 等级
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.qosClass}'

# 批量查看所有 Pod 的 QoS 等级
kubectl get pods -n <namespace> -o custom-columns='NAME:.metadata.name,QOS:.status.qosClass,CPU_REQ:.spec.containers[0].resources.requests.cpu,MEM_REQ:.spec.containers[0].resources.requests.memory'

# 查找所有 BestEffort Pod（生产环境不应存在）
kubectl get pods -A -o jsonpath='{range .items[?(@.status.qosClass=="BestEffort")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'

# 查看节点驱逐阈值
kubectl describe node <node-name> | grep -A 5 "eviction"
```
## 交叉引用

- [Pod 生命周期](../%E6%A6%82%E5%BF%B5%E5%8E%9F%E7%90%86/pod-lifecycle.md)
- [OOM 内存诊断](../../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/01-%E6%A0%B8%E5%BF%83%E6%8E%92%E9%9A%9C/07-oom-memory-diagnosis.md)
- [高级 Pod 配置](../%E9%85%8D%E7%BD%AE%E4%B8%8E%E5%AD%97%E5%85%B8/advanced-pod-configuration.md)
- [工作负载监控与告警](../../../../02-%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/01-%E6%A0%B8%E5%BF%83%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD/06-workload-monitoring-alerting.md)
- [VPA 垂直自动扩缩](./vertical-pod-autoscaling.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/pod-qos/

## Related

- [[17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[17-系统基础/06-知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[17-系统基础/06-知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
