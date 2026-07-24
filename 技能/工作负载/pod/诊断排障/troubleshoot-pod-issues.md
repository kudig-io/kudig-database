---
title: Troubleshoot Pod Issues
description: Troubleshoot Pod Issues — Kubernetes 生产运维知识库
summary: Troubleshoot Pod Issues — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- troubleshooting
- pod
- pending
- crashloop
- debugging
- diagnosis
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Troubleshoot Pod Issues 是什么
- 如何 Troubleshoot Pod Issues
- Troubleshoot Pod Issues 故障排查
- Troubleshoot Pod Issues 排障步骤
trigger_keywords:
- Troubleshoot
- Pod
- Issues
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Troubleshoot Pod Issues

## Diagnostic Workflow

### Step 1: Check Pod Status

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -o wide
kubectl describe pod <pod-name>
kubectl get pod <pod-name> -o jsonpath='{.status.conditions}'
```
Key indicators:
- **Phase**: Pending, Running, Failed, Unknown
- **Conditions**: PodScheduled, Initialized, ContainersReady, Ready
- **Container Status**: waiting, running, terminated (with reason/exit code)

### Step 2: Analyze Events

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod <pod-name> | grep -A5 Events
kubectl get events --sort-by='.lastTimestamp'
```
Common events and their meaning:
- `FailedScheduling`: No suitable node found (check resources, taints, node selectors)
- `PullBackOff` / `ErrImagePull`: Image pull failure (check image name, tag, registry credentials)
- `CrashLoopBackOff`: Container keeps crashing (check logs, resource limits, application errors)

### Step 3: Check Logs

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs <pod-name>                    # Current container
kubectl logs <pod-name> --previous          # Previous crashed container
kubectl logs <pod-name> -c <container-name> # Specific container
```
### Step 4: Common Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| **Pending** | Insufficient resources, taints, PVC unbound | Check `kubectl describe pod`, increase resources, fix PVC |
| **CrashLoopBackOff** | App crash, bad config, OOM | Check logs, validate config, increase memory limits |
| **ImagePullBackOff** | Wrong image/tag, no pull secret | Fix image reference, create imagePullSecret |
| **OOMKilled** | Memory limit too low, memory leak | Increase limits, fix leak, configure JVM -XX:MaxRAMPercentage |
| **Init:Error** | Init container failure | Check init container logs, fix init logic |

### Step 5: Debug with Ephemeral Containers

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl debug -it <pod-name> --image=busybox --target=<container-name>
```
## JVM-Specific Guidance

For Java applications in containers:
- Set `-XX:MaxRAMPercentage=75.0` to limit JVM heap to container memory
- Set `-XX:+UseContainerSupport` (enabled by default since Java 10)
- Account for off-heap memory: Metaspace, direct buffers, thread stacks, code cache

## 生产案例

### 案例 1: Pod ImagePullBackOff——私有仓库凭据过期

| 时间 | 事件 |
|------|------|
| 09:00 | 新 Pod 启动失败，状态 ImagePullBackOff |
| 09:05 | `kubectl describe pod` 显示 "unauthorized: authentication required" |
| 09:08 | imagePullSecrets 中的 Docker registry token 过期 |
| 09:10 | 🟡 更新 Secret，Pod 自动重试拉取 |

**根因**: 私有镜像仓库的访问 token 有有效期，未配置自动刷新。

### 案例 2: Pod OOMKilled——内存 limit 设置过低

**现象**: Pod 运行 1h 后 OOMKilled，重启后再次 OOM。

**诊断**: `kubectl describe pod` 显示 Last State: OOMKilled，memory limit 256Mi

**修复**: 🟢 分析实际内存需求，调整 limit 到 1Gi

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 核心服务 Pod 全部不可用 | 立即回滚 + 检查 |
| P1 | 部分 Pod 异常 | 检查日志和事件 |
| P2 | 偶发重启 | 优化资源配置 |

## 面试要点

1. **Q: Pod 故障排查的标准流程？**
   A: ① `kubectl get pod` 查看状态 ② `kubectl describe pod` 查看事件 ③ `kubectl logs --previous` 查看崩溃日志 ④ `kubectl exec` 进入容器调试 ⑤ 检查节点状态 ⑥ 检查网络和存储。

2. **Q: 常见 Pod 异常状态及原因？**
   A: Pending(资源不足/调度失败)、ImagePullBackOff(镜像/凭据问题)、CrashLoopBackOff(应用启动失败)、OOMKilled(内存超限)、Evicted(节点资源压力)、Terminating(卸载/finalizer 阻塞)。

3. **Q: 如何预防 Pod 故障？**
   A: ① 合理设置 request/limit ② 配置健康探针 ③ 使用 PDB 保护 ④ 镜像拉取策略 IfNotPresent ⑤ 配置资源告警 ⑥ 定期审查 Pod 状态。

## Related

- [[技能/可观测性/monitoring/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — [[技能/可观测性/monitoring/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|configure-health-probes]] — [[技能/工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[技能/工作负载/pod/方法论/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[pod-lifecycle|Pod Lifecycle]]
- [[概念/resource-management.md|Resource Management]]
- [[技能/工作负载/pod/运维操作/configure-health-probes.md|Configure Health Probes]]
- [[技能/可观测性/monitoring/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]


<!-- risk-assessed -->
