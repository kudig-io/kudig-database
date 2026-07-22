---
title: 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
description: '# 工作负载管理'
summary: 'Init Containers → Main Containers → Sidecar Containers 执行顺序。'
category: reference
tags:
- k8s
- workloads
- pod
- scheduling
- hpa
- vpa
- autoscaling
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工作负载管理：Pod 生命周期、调度策略与弹性伸缩 是什么
- 如何 工作负载管理：Pod 生命周期、调度策略与弹性伸缩
trigger_keywords:
- 工作负载管理：Pod
- 生命周期
- 调度策略与弹性伸缩
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 工作负载管理

> **类别**: Core Concept | **适用版本**: Kubernetes 1.27+

## 概述

工作负载管理是 Kubernetes 的核心能力之一，涵盖 Pod 生命周期管理、调度策略配置、资源管理和弹性伸缩四大领域。Kubernetes 通过声明式 API 让开发者定义期望的工作负载状态（如 Deployment、StatefulSet、DaemonSet），由控制器自动驱动实际状态向期望状态收敛。理解 Pod 生命周期、探针机制、调度策略和弹性伸缩原理是构建可靠生产级应用的基础。Kubernetes 调度器负责将 Pod 分配到合适的节点，而 HPA/VPA/Cluster Autoscaler 则根据负载动态调整资源供给。

## 核心能力

- **Pod 生命周期管理**: 通过 Init Container、Main Container、Sidecar Container 有序管理容器启动
- **探针机制**: livenessProbe（存活检测）、readinessProbe（就绪检测）、startupProbe（启动检测）
- **调度策略**: nodeSelector、nodeAffinity、podAffinity/AntiAffinity、taints & tolerations、topologySpreadConstraints
- **QoS 管理**: Guaranteed、Burstable、BestEffort 三级服务质量保证
- **弹性伸缩**: HPA（水平）、VPA（垂直）、Cluster Autoscaler（节点）、KEDA（事件驱动）
- **优先级与抢占**: PriorityClass 定义 Pod 优先级，资源不足时驱逐低优先级 Pod

## 架构

工作负载管理涉及多个 Kubernetes 核心组件协作：

- **kube-scheduler**: 监听未调度的 Pod，根据过滤（Filter）和打分（Score）两阶段算法选择最佳节点
- **kube-controller-manager**: 运行 Deployment、ReplicaSet、StatefulSet、DaemonSet 等控制器
- **kubelet**: 管理节点上的 Pod 生命周期，执行探针检查和容器重启
- **metrics-server / Prometheus Adapter**: 为 HPA 提供 CPU/Memory 和自定义指标
- **Cluster Autoscaler**: 监听 Pending Pod，触发云厂商 API 扩展节点

Pod 状态流转：`Pending → Running → Succeeded / Failed / Unknown`

## K8s 集成

工作负载管理是 Kubernetes 原生核心能力，通过内置控制器和调度器实现，无需额外安装。通过 kubectl、Helm 或 GitOps 工具（ArgoCD/Flux）管理各类工作负载 CRD。HPA 可通过 `kubectl autoscale` 或 Prometheus Adapter 集成自定义指标。生产环境推荐配合 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 ResourceQuota、LimitRange 和 PodDisruptionBudget 使用。

## 生产场景

1. **微服务滚动发布**: 使用 Deployment + maxSurge/maxUnavailable 实现零停机滚动更新
2. **有状态数据库部署**: StatefulSet + 持久化存储 + 有序启停，管理 MySQL、PostgreSQL 等数据库
3. **GPU/AI 推理弹性伸缩**: KEDA 基于 Prometheus 指标或消息队列深度，自动伸缩推理 Pod
4. **全局负载均衡**: topologySpreadConstraints 确保 Pod 跨可用区均匀分布

## 安装

```bash
# HPA 基础 — 安装 metrics-server
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# KEDA — 事件驱动伸缩
helm repo add kedacore https://kedacore.github.io/charts
helm install keda kedacore/keda --namespace keda-system --create-namespace

# Prometheus Adapter — 自定义指标 HPA
helm install prometheus-adapter prometheus-community/prometheus-adapter -n monitoring
```

## 运维操作

```bash
# 🟢 查看工作负载状态
kubectl get deployments,statefulsets,daemonsets -A
kubectl get pods -o wide --sort-by=.status.startTime

# 🟢 查看 HPA 状态
kubectl get hpa -A
kubectl describe hpa <name> -n <ns>

# 🟡 手动扩缩容
kubectl scale deployment/<name> --replicas=5 -n <ns>

# 🟡 滚动更新
kubectl set image deployment/<name> <container>=<image>:<tag> -n <ns>
kubectl rollout status deployment/<name> -n <ns>

# 🟡 回滚部署
kubectl rollout undo deployment/<name> -n <ns>
kubectl rollout history deployment/<name> -n <ns>

# 🟢 查看调度事件
kubectl get events --field-selector reason=FailedScheduling -A
kubectl describe pod <pod-name> -n <ns> | grep -A5 Events

# 🟢 查看节点资源分配
kubectl describe nodes | grep -A10 "Allocated resources"
kubectl top nodes

# 🔴 驱逐节点上的 Pod（维护）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
kubectl uncordon <node>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod Pending | 资源不足/调度约束 | `kubectl describe pod` | 检查节点资源、亲和性、污点 |
| HPA 不触发 | 指标未就绪 | `kubectl get hpa` | 检查 metrics-server 和指标注册 |
| 滚动更新卡住 | 新 Pod 未就绪 | `kubectl rollout status` | 检查 readinessProbe 配置 |
| Pod 频繁重启 | livenessProbe 过严 | `kubectl get events` | 调整探针参数 |
| 节点压力驱逐 | 资源超用 | `kubectl describe node` | 检查 requests/limits 配置 |
| 抢占发生 | 优先级冲突 | `kubectl get events --field-selector reason=Preempted` | 调整 PriorityClass |

### 排查流程

```
Pod 异常 → kubectl describe pod 查看状态和事件
  ├─ Pending → 检查调度失败原因
  │   ├─ Insufficient cpu/memory → 扩容节点或调整 requests
  │   ├─ node(s) didn't match affinity → 检查亲和性配置
  │   └─ node(s) had taint → 添加 toleration 或移除 taint
  ├─ ContainerCreating → 检查镜像拉取/存储挂载
  ├─ CrashLoopBackOff → 检查容器日志
  └─ Running 但异常 → 检查探针配置和应用日志
```

## 生产案例

### 案例1: HPA 无法触发扩容

**场景**: 流量突增但 HPA 未触发扩容，服务响应变慢  
**排查**: `kubectl get hpa` 显示 `<unknown>` 指标，metrics-server Pod CrashLoop  
**方案**: 修复 metrics-server（添加 --kubelet-insecure-tls），配置多指标 HPA  
**效果**: HPA 正常工作，扩容延迟 < 30s  

### 案例2: 滚动更新零停机失败

**场景**: 滚动更新期间部分用户收到 502 错误  
**排查**: 新 Pod 未完全就绪就接收流量，readinessProbe 配置不当  
**方案**: 添加 preStop hook (sleep 5) + 调整 readinessProbe initialDelaySeconds  
**效果**: 实现真正零停机滚动更新  

## 对比

| 伸缩组件 | 维度 | 触发条件 | 适用场景 |
|----------|------|----------|----------|
| HPA | Pod 副本数 | CPU/Memory/自定义指标 | Web 服务、API |
| VPA | Pod 资源配置 | 历史使用量分析 | 资源调优 |
| Cluster Autoscaler | 节点数 | Pending Pod | 集群扩缩容 |
| KEDA | 事件驱动 | 消息队列/外部事件 | 消费者、批处理 |

## 生产部署要点

QoS 优先级（OOM 时驱逐顺序）：
1. **BestEffort**: 未设置 requests/limits → 最先被驱逐
2. **Burstable**: requests < limits → 次优先被驱逐
3. **Guaranteed**: requests = limits → 最后被驱逐

HPA 经典公式：`目标副本数 = ceil(当前副本数 × (当前指标值 / 目标指标值))`

## 检查清单

- [ ] 所有生产 Pod 配置 requests 和 limits
- [ ] 配置 livenessProbe + readinessProbe + startupProbe
- [ ] 关键服务配置 PodDisruptionBudget
- [ ] 使用 topologySpreadConstraints 跨可用区分布
- [ ] 配置 HPA 并验证指标采集正常
- [ ] 滚动更新配置 maxSurge/maxUnavailable
- [ ] 配置 preStop hook 确保优雅关闭
- [ ] 关键服务使用 Guaranteed QoS

---

> 来源：.zread/wiki/drafts/8-gong-zuo-fu-zai-guan-li-pod-sheng-ming-zhou-qi-diao-du-ce-lue-yu-dan-xing-shen-suo.md

## Related

- [[keda]] — KEDA
- [[平台工程/代码分析/deployment-create/08-hpa-integration.md|Deployment 与 HPA 集成源码分析]]
- [[实体/kube-scheduler.md|kube-scheduler]] — K8s 调度器
- [[概念/controller-pattern.md|controller-pattern]] — 控制器模式

<!-- risk-assessed -->
