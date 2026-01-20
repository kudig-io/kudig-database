# 102 - Pod Pending 状态深度诊断 (Pod Pending Diagnosis)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级-高级 | **参考**: [Kubernetes Scheduling](https://kubernetes.io/docs/concepts/scheduling-eviction/)

---

## 目录

1. [概述与诊断框架](#1-概述与诊断框架)
2. [诊断决策树](#2-诊断决策树)
3. [资源类问题深度诊断](#3-资源类问题深度诊断)
4. [节点选择问题诊断](#4-节点选择问题诊断)
5. [存储问题诊断](#5-存储问题诊断)
6. [配额与准入控制](#6-配额与准入控制)
7. [调度器问题诊断](#7-调度器问题诊断)
8. [高级调度场景](#8-高级调度场景)
9. [ACK/云环境特定问题](#9-ack云环境特定问题)
10. [自动化诊断工具](#10-自动化诊断工具)
11. [监控告警配置](#11-监控告警配置)
12. [紧急处理流程](#12-紧急处理流程)
13. [版本特定变更](#13-版本特定变更)
14. [多角色视角](#14-多角色视角)
15. [最佳实践](#15-最佳实践)

---

## 1. 概述与诊断框架

### 1.1 Pod Pending 状态定义

Pod 处于 Pending 状态表示 Pod 已被 Kubernetes API Server 接受，但尚未被调度到节点或容器镜像尚未拉取。

| 阶段 | 状态 | 说明 | 诊断入口 |
|------|------|------|---------|
| **调度前** | Pending (无 nodeName) | 等待调度器分配节点 | `kubectl describe pod` Events |
| **调度后** | Pending (有 nodeName) | 已分配节点，等待容器启动 | `kubectl describe pod` Conditions |
| **镜像拉取** | Pending + ImagePullBackOff | 镜像拉取失败 | 容器运行时日志 |
| **初始化** | Pending + Init:X/Y | Init 容器未完成 | Init 容器日志 |

### 1.2 诊断优先级矩阵

| 紧急程度 | Pending 时长 | 影响范围 | 响应时间 | 处理优先级 |
|---------|-------------|---------|---------|-----------|
| **P0 - 紧急** | > 30min | 生产核心服务 | < 15min | 立即处理 |
| **P1 - 高** | > 15min | 生产服务 | < 30min | 优先处理 |
| **P2 - 中** | > 5min | 非核心服务 | < 2h | 计划处理 |
| **P3 - 低** | < 5min | 开发/测试 | < 24h | 常规处理 |

### 1.3 调度流程架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                         Kubernetes Pod Scheduling Pipeline                               │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│  ┌──────────────┐                                                                       │
│  │  Pod Create  │                                                                       │
│  │  (API Server)│                                                                       │
│  └──────┬───────┘                                                                       │
│         │                                                                               │
│         ▼                                                                               │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                        Scheduling Queue (优先级队列)                             │   │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐    │   │
│  │  │ activeQ       │  │ backoffQ      │  │ unschedulableQ│  │ gatedQ(v1.28+)│    │   │
│  │  │ (待调度)      │  │ (退避重试)    │  │ (不可调度)     │  │ (调度门控)    │    │   │
│  │  └───────────────┘  └───────────────┘  └───────────────┘  └───────────────┘    │   │
│  └────────────────────────────────────────────┬────────────────────────────────────┘   │
│                                               │                                         │
│                                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Scheduling Cycle (调度周期)                              │   │
│  │                                                                                  │   │
│  │  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │   │
│  │  │  PreFilter   │───▶│    Filter    │───▶│  PostFilter  │───▶│   PreScore   │  │   │
│  │  │  预过滤       │    │    过滤      │    │  后过滤/抢占 │    │   预评分     │  │   │
│  │  └──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘  │   │
│  │         │                   │                   │                   │           │   │
│  │         │                   │                   │                   │           │   │
│  │         ▼                   ▼                   ▼                   ▼           │   │
│  │  - 资源需求检查       - NodeAffinity      - 触发抢占          - 准备评分状态  │   │
│  │  - 端口冲突检查       - Taints/Tolerations - 选择牺牲者                        │   │
│  │  - 卷拓扑检查         - NodeResourcesFit                                       │   │
│  │                       - PodTopologySpread                                       │   │
│  │                                                                                  │   │
│  │  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                      │   │
│  │  │    Score     │───▶│ NormalizeScore│───▶│   Reserve    │                      │   │
│  │  │    评分      │    │  归一化评分   │    │   预留资源   │                      │   │
│  │  └──────────────┘    └──────────────┘    └──────────────┘                      │   │
│  │         │                   │                   │                               │   │
│  │         ▼                   ▼                   ▼                               │   │
│  │  - ImageLocality      - 加权汇总          - 锁定节点资源                        │   │
│  │  - NodeAffinity       - 选择最高分节点    - 防止并发调度冲突                    │   │
│  │  - BalancedAllocation                                                           │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                               │                                         │
│                                               ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                          Binding Cycle (绑定周期 - 异步)                         │   │
│  │                                                                                  │   │
│  │  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │   │
│  │  │    Permit    │───▶│   PreBind    │───▶│     Bind     │───▶│   PostBind   │  │   │
│  │  │   许可/等待  │    │   绑定前     │    │     绑定     │    │    绑定后    │  │   │
│  │  └──────────────┘    └──────────────┘    └──────────────┘    └──────────────┘  │   │
│  │         │                   │                   │                   │           │   │
│  │         ▼                   ▼                   ▼                   ▼           │   │
│  │  - Gang调度等待       - PVC绑定           - 更新Pod.nodeName  - 清理状态      │   │
│  │  - 资源锁定           - 存储预留          - 通知kubelet                        │   │
│  │                                                                                  │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                             调度失败处理                                         │   │
│  │  Filter 阶段失败 ──▶ 记录 FailedScheduling 事件                                 │   │
│  │  PostFilter 抢占失败 ──▶ 移入 unschedulableQ                                    │   │
│  │  Binding 失败 ──▶ 移入 backoffQ 重试                                            │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 诊断决策树

### 2.1 快速诊断流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Pod Pending 快速诊断决策树                                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  Pod 处于 Pending 状态                                                              │
│          │                                                                           │
│          ▼                                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐            │
│  │ Step 1: kubectl describe pod <name> -n <ns>                         │            │
│  │         查看 Events 和 Conditions                                    │            │
│  └─────────────────────────────────────────────────────────────────────┘            │
│          │                                                                           │
│          ▼                                                                           │
│  ┌─────────────────────────────────────────────────────────────────────┐            │
│  │ Step 2: 检查 spec.nodeName 是否已分配                               │            │
│  └─────────────────────────────────────────────────────────────────────┘            │
│          │                                                                           │
│          ├──────────────────────────────────────────┐                               │
│          │ (未分配 - 调度问题)                       │ (已分配 - 节点启动问题)       │
│          ▼                                          ▼                               │
│  ┌──────────────────┐                     ┌──────────────────────────┐              │
│  │ 查看调度失败原因  │                     │ 检查 kubelet/容器运行时  │              │
│  └────────┬─────────┘                     └──────────────────────────┘              │
│           │                                                                          │
│  ┌────────┴────────────────────────────────────────────────────────────┐            │
│  │                          调度失败原因分类                            │            │
│  ├─────────────────────────────────────────────────────────────────────┤            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ Insufficient cpu/memory/ephemeral-storage                     │  │            │
│  │  │ ──▶ 资源类问题 ──▶ 跳转第3章                                  │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ node(s) didn't match Pod's node selector/affinity            │  │            │
│  │  │ node(s) had taint that pod didn't tolerate                   │  │            │
│  │  │ ──▶ 节点选择问题 ──▶ 跳转第4章                                │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ pod has unbound immediate PersistentVolumeClaims             │  │            │
│  │  │ volume node affinity conflict                                │  │            │
│  │  │ ──▶ 存储问题 ──▶ 跳转第5章                                    │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ exceeded quota / forbidden by LimitRange                     │  │            │
│  │  │ ──▶ 配额问题 ──▶ 跳转第6章                                    │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ pod topology spread constraints not satisfied                │  │            │
│  │  │ ──▶ 拓扑分布问题 ──▶ 跳转第8章                                │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  │  ┌──────────────────────────────────────────────────────────────┐  │            │
│  │  │ 无事件 / 调度器不可用                                         │  │            │
│  │  │ ──▶ 调度器问题 ──▶ 跳转第7章                                  │  │            │
│  │  └──────────────────────────────────────────────────────────────┘  │            │
│  │                                                                      │            │
│  └─────────────────────────────────────────────────────────────────────┘            │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 常见 FailedScheduling 消息速查

| 错误消息 | 原因类别 | 快速诊断 | 解决方案 |
|---------|---------|---------|---------|
| `Insufficient cpu` | 资源不足 | `kubectl describe nodes \| grep -A5 Allocated` | 扩容节点/调整requests |
| `Insufficient memory` | 资源不足 | `kubectl top nodes` | 扩容节点/调整requests |
| `Insufficient nvidia.com/gpu` | GPU不足 | `kubectl get nodes -l gpu=true` | 添加GPU节点 |
| `node(s) didn't match Pod's node selector` | 节点选择 | `kubectl get nodes --show-labels` | 修正标签/选择器 |
| `node(s) didn't match Pod's node affinity/selector` | 亲和性 | 检查 `spec.affinity` | 调整亲和性规则 |
| `node(s) had taint {key} that the pod didn't tolerate` | 污点容忍 | `kubectl describe node \| grep Taint` | 添加tolerations |
| `node(s) had untolerated taint {node.kubernetes.io/not-ready}` | 节点异常 | `kubectl get nodes` | 修复节点状态 |
| `pod has unbound immediate PersistentVolumeClaims` | PVC未绑定 | `kubectl get pvc` | 创建PV/修复SC |
| `volume node affinity conflict` | 卷拓扑冲突 | 检查PV nodeAffinity | 调整卷配置 |
| `pod topology spread constraints not satisfied` | 拓扑分布 | 检查topologySpreadConstraints | 调整maxSkew |
| `Too many pods` | Pod数量限制 | `kubectl describe node \| grep -i pods` | 扩容节点 |
| `exceeded quota` | 配额超限 | `kubectl describe quota` | 调整配额 |
| `no preemption victims found` | 抢占失败 | 检查PriorityClass | 调整优先级 |

---

## 3. 资源类问题深度诊断

### 3.1 资源不足原因分类

| 资源类型 | 检查命令 | 版本变化 | ACK特殊处理 |
|---------|---------|---------|------------|
| **CPU** | `kubectl describe node \| grep -A5 "Allocated"` | 稳定 | 支持弹性调度 |
| **Memory** | `kubectl top nodes` | 稳定 | 内存超售配置 |
| **Ephemeral Storage** | `kubectl describe node \| grep ephemeral` | v1.25+增强 | 云盘自动扩容 |
| **GPU** | `kubectl describe node \| grep nvidia.com/gpu` | 稳定 | GPU共享(cGPU) |
| **Extended Resources** | `kubectl get node -o json \| jq '.status.capacity'` | 稳定 | 自定义资源 |
| **Hugepages** | `kubectl describe node \| grep hugepages` | v1.27+稳定 | 大页内存配置 |
| **Pods** | `kubectl describe node \| grep "Pods:"` | 稳定 | maxPods配置 |

### 3.2 集群资源分析

```bash
#!/bin/bash
# cluster-resource-analysis.sh - 集群资源深度分析

echo "=============================================="
echo "  集群资源分析报告 - $(date)"
echo "=============================================="

echo ""
echo "=== 1. 节点资源总览 ==="
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,STATUS:.status.conditions[?(@.type=="Ready")].status,CPU-ALLOC:.status.allocatable.cpu,MEM-ALLOC:.status.allocatable.memory,PODS-ALLOC:.status.allocatable.pods'

echo ""
echo "=== 2. 节点实时使用率 ==="
kubectl top nodes --use-protocol-buffers 2>/dev/null || kubectl top nodes

echo ""
echo "=== 3. 各节点资源分配详情 ==="
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  echo ""
  echo "--- Node: $node ---"
  kubectl describe node $node | grep -A 10 "Allocated resources:"
done

echo ""
echo "=== 4. 资源请求 Top 10 Pod (CPU) ==="
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.status.phase=="Running") |
  {namespace: .metadata.namespace, name: .metadata.name, cpu: (.spec.containers[].resources.requests.cpu // "0")} |
  "\(.namespace)/\(.name): \(.cpu)"
' | sort -t: -k2 -rn | head -10

echo ""
echo "=== 5. 资源请求 Top 10 Pod (Memory) ==="
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.status.phase=="Running") |
  {namespace: .metadata.namespace, name: .metadata.name, mem: (.spec.containers[].resources.requests.memory // "0")} |
  "\(.namespace)/\(.name): \(.mem)"
' | sort -t: -k2 -rh | head -10

echo ""
echo "=== 6. 节点资源压力状态 ==="
kubectl get nodes -o json | jq -r '
  .items[] | 
  "\(.metadata.name): MemoryPressure=\(.status.conditions[] | select(.type=="MemoryPressure") | .status), DiskPressure=\(.status.conditions[] | select(.type=="DiskPressure") | .status), PIDPressure=\(.status.conditions[] | select(.type=="PIDPressure") | .status)"
'

echo ""
echo "=== 7. 可用资源汇总 ==="
echo "可分配 CPU 总量:"
kubectl get nodes -o json | jq '[.items[].status.allocatable.cpu | gsub("m"; "") | tonumber] | add'
echo ""
echo "已请求 CPU 总量:"
kubectl get pods -A -o json | jq '[.items[] | select(.status.phase=="Running") | .spec.containers[].resources.requests.cpu // "0" | gsub("m"; "") | if . == "0" then 0 else tonumber end] | add'

echo ""
echo "=== 8. GPU 资源状态 (如有) ==="
kubectl get nodes -o json | jq -r '
  .items[] | 
  select(.status.allocatable["nvidia.com/gpu"] != null) |
  "\(.metadata.name): GPU 可分配=\(.status.allocatable["nvidia.com/gpu"]), 已分配=\(.status.capacity["nvidia.com/gpu"])"
'

echo ""
echo "=== 9. Pending Pod 资源需求汇总 ==="
kubectl get pods -A --field-selector=status.phase=Pending -o json | jq -r '
  .items[] | 
  "\(.metadata.namespace)/\(.metadata.name): CPU=\(.spec.containers[].resources.requests.cpu // "未设置"), Memory=\(.spec.containers[].resources.requests.memory // "未设置")"
'
```

### 3.3 资源不足解决方案

| 解决方案 | 适用场景 | 操作复杂度 | 生效时间 | 风险等级 |
|---------|---------|-----------|---------|---------|
| **调整Pod requests** | 资源请求过大 | 低 | 立即 | 低 |
| **驱逐低优先级Pod** | 紧急释放资源 | 中 | 立即 | 中 |
| **节点扩容** | 长期资源不足 | 中 | 5-10min | 低 |
| **Cluster Autoscaler** | 自动扩缩容 | 高(初次配置) | 自动 | 低 |
| **VPA自动调整** | 动态资源调整 | 中 | 按策略 | 中 |
| **资源超售配置** | 提高利用率 | 中 | 立即 | 中 |

```yaml
# 资源优化配置示例
---
# 方案1: 合理设置资源请求 (基于实际使用量)
apiVersion: v1
kind: Pod
metadata:
  name: optimized-app
spec:
  containers:
  - name: app
    image: myapp:latest
    resources:
      requests:
        cpu: 100m       # 基于 P95 使用量
        memory: 256Mi   # 基于稳态使用量 + 20%缓冲
      limits:
        cpu: 500m       # requests 的 2-5 倍
        memory: 512Mi   # requests 的 1.5-2 倍
---
# 方案2: VPA 自动调整 (v1.25+)
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: myapp-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: myapp
  updatePolicy:
    updateMode: Auto  # 或 "Off" 仅推荐
  resourcePolicy:
    containerPolicies:
    - containerName: '*'
      minAllowed:
        cpu: 50m
        memory: 64Mi
      maxAllowed:
        cpu: 4
        memory: 8Gi
      controlledResources: ["cpu", "memory"]
---
# 方案3: PriorityClass 配置抢占
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 1000000
preemptionPolicy: PreemptLowerPriority
globalDefault: false
description: "高优先级业务Pod，可抢占低优先级Pod"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: low-priority-batch
value: -100
preemptionPolicy: Never  # 不触发抢占
globalDefault: false
description: "低优先级批处理任务"
```

---

## 4. 节点选择问题诊断

### 4.1 节点选择机制总览

| 机制 | 作用 | 硬/软约束 | 版本状态 |
|------|------|----------|---------|
| **nodeSelector** | 简单标签匹配 | 硬约束 | 稳定 |
| **nodeAffinity** | 高级节点亲和性 | 硬/软 | 稳定 |
| **podAffinity** | Pod间亲和性 | 硬/软 | 稳定 |
| **podAntiAffinity** | Pod间反亲和性 | 硬/软 | 稳定 |
| **Taints/Tolerations** | 污点容忍 | 硬约束 | 稳定 |
| **topologySpreadConstraints** | 拓扑分布 | 硬/软 | v1.19+ GA |
| **nodeName** | 直接指定节点 | 硬约束 | 稳定 |

### 4.2 节点选择诊断命令

```bash
#!/bin/bash
# node-selection-diagnose.sh - 节点选择问题诊断

POD_NAME=$1
NAMESPACE=${2:-default}

echo "=============================================="
echo "  节点选择问题诊断: $NAMESPACE/$POD_NAME"
echo "=============================================="

echo ""
echo "=== 1. Pod 节点选择配置 ==="
echo "--- nodeSelector ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeSelector}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "--- nodeAffinity ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.affinity.nodeAffinity}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "--- podAffinity ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.affinity.podAffinity}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "--- podAntiAffinity ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.affinity.podAntiAffinity}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "--- topologySpreadConstraints ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.topologySpreadConstraints}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "--- tolerations ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.tolerations}' 2>/dev/null | jq . || echo "(未配置)"

echo ""
echo "=== 2. 集群节点状态 ==="
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,STATUS:.status.conditions[?(@.type=="Ready")].status,SCHEDULABLE:.spec.unschedulable,VERSION:.status.nodeInfo.kubeletVersion'

echo ""
echo "=== 3. 节点标签 ==="
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): \(.metadata.labels | to_entries | map("\(.key)=\(.value)") | join(", "))"'

echo ""
echo "=== 4. 节点污点 ==="
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): \(.spec.taints // [] | map("\(.key)=\(.value):\(.effect)") | join(", "))"'

echo ""
echo "=== 5. 可用区分布 ==="
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): zone=\(.metadata.labels["topology.kubernetes.io/zone"] // "N/A"), region=\(.metadata.labels["topology.kubernetes.io/region"] // "N/A")"'

echo ""
echo "=== 6. 节点匹配分析 ==="
# 获取 Pod 的 nodeSelector
NODE_SELECTOR=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeSelector}' 2>/dev/null)
if [ -n "$NODE_SELECTOR" ] && [ "$NODE_SELECTOR" != "{}" ]; then
    echo "NodeSelector: $NODE_SELECTOR"
    echo "匹配的节点:"
    # 将 nodeSelector 转换为 -l 参数
    LABEL_SELECTOR=$(echo $NODE_SELECTOR | jq -r 'to_entries | map("\(.key)=\(.value)") | join(",")')
    kubectl get nodes -l "$LABEL_SELECTOR" 2>/dev/null || echo "  (无匹配节点)"
else
    echo "NodeSelector: 未配置，所有节点可选"
fi
```

### 4.3 常见节点选择问题解决

```yaml
# 节点选择问题解决方案集

---
# 场景1: 添加缺失的节点标签
# kubectl label node <node-name> app-tier=frontend

---
# 场景2: 使用软亲和性避免调度失败
apiVersion: apps/v1
kind: Deployment
metadata:
  name: flexible-app
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          # 硬约束: 必须在生产节点
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-type
                operator: In
                values: ["production"]
          # 软约束: 优先选择SSD节点
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 80
            preference:
              matchExpressions:
              - key: disk-type
                operator: In
                values: ["ssd"]
          - weight: 20
            preference:
              matchExpressions:
              - key: topology.kubernetes.io/zone
                operator: In
                values: ["cn-hangzhou-h"]

---
# 场景3: 添加污点容忍
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tolerate-all
spec:
  template:
    spec:
      tolerations:
      # 容忍 master 节点污点 (非生产环境)
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
      # 容忍节点压力污点
      - key: node.kubernetes.io/memory-pressure
        operator: Exists
        effect: NoSchedule
      # 容忍自定义污点
      - key: dedicated
        operator: Equal
        value: gpu
        effect: NoSchedule

---
# 场景4: 移除节点污点 (运维操作)
# kubectl taint node <node-name> <key>:<effect>-
# 示例: kubectl taint node node1 dedicated=gpu:NoSchedule-
```

---

## 5. 存储问题诊断

### 5.1 存储问题分类

| 问题类型 | 错误消息 | 诊断方向 | 版本特性 |
|---------|---------|---------|---------|
| **PVC未绑定** | `pod has unbound immediate PersistentVolumeClaims` | 检查PVC状态、SC配置 | 稳定 |
| **卷拓扑冲突** | `volume node affinity conflict` | 检查PV nodeAffinity | v1.17+ |
| **动态供应失败** | PVC Pending无事件 | 检查CSI驱动、SC参数 | 稳定 |
| **WaitForFirstConsumer** | PVC Pending等待Pod | 正常行为，等调度 | v1.17+ GA |
| **CSI驱动异常** | 各种CSI错误 | 检查CSI Pod状态 | 稳定 |
| **存储后端故障** | 超时/连接失败 | 检查存储系统 | - |

### 5.2 存储诊断命令

```bash
#!/bin/bash
# storage-diagnose.sh - 存储问题诊断

POD_NAME=$1
NAMESPACE=${2:-default}

echo "=============================================="
echo "  存储问题诊断: $NAMESPACE/$POD_NAME"
echo "=============================================="

echo ""
echo "=== 1. Pod 卷配置 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.volumes}' | jq .

echo ""
echo "=== 2. PVC 状态 ==="
PVCS=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.volumes[*].persistentVolumeClaim.claimName}' 2>/dev/null)
if [ -n "$PVCS" ]; then
    for pvc in $PVCS; do
        echo ""
        echo "--- PVC: $pvc ---"
        kubectl get pvc $pvc -n $NAMESPACE -o wide
        echo ""
        echo "事件:"
        kubectl describe pvc $pvc -n $NAMESPACE | grep -A 10 "Events:"
        
        # 检查绑定的 PV
        PV=$(kubectl get pvc $pvc -n $NAMESPACE -o jsonpath='{.spec.volumeName}' 2>/dev/null)
        if [ -n "$PV" ]; then
            echo ""
            echo "绑定的 PV: $PV"
            kubectl get pv $PV -o wide
            echo ""
            echo "PV nodeAffinity:"
            kubectl get pv $PV -o jsonpath='{.spec.nodeAffinity}' | jq . 2>/dev/null || echo "(无)"
        fi
    done
else
    echo "(Pod 未使用 PVC)"
fi

echo ""
echo "=== 3. StorageClass 状态 ==="
kubectl get sc -o wide

echo ""
echo "=== 4. 默认 StorageClass ==="
kubectl get sc -o json | jq -r '.items[] | select(.metadata.annotations["storageclass.kubernetes.io/is-default-class"]=="true") | .metadata.name'

echo ""
echo "=== 5. CSI 驱动状态 ==="
kubectl get csidrivers -o wide

echo ""
echo "=== 6. CSI 节点信息 ==="
kubectl get csinodes -o wide

echo ""
echo "=== 7. CSI 相关 Pod ==="
kubectl get pods -A | grep -E "csi|provisioner|attacher|resizer|snapshotter"

echo ""
echo "=== 8. Pending PVC 列表 ==="
kubectl get pvc -A --field-selector=status.phase=Pending

echo ""
echo "=== 9. 可用 PV 列表 ==="
kubectl get pv --field-selector=status.phase=Available
```

### 5.3 ACK 存储特定诊断

```bash
# ACK 云盘 CSI 诊断
echo "=== ACK 云盘 CSI 状态 ==="
kubectl get pods -n kube-system -l app=csi-plugin
kubectl get pods -n kube-system -l app=csi-provisioner

# 云盘 CSI 日志
kubectl logs -n kube-system -l app=csi-provisioner -c csi-provisioner --tail=50

# NAS CSI 诊断
kubectl get pods -n kube-system | grep nas

# 检查云盘绑定状态
kubectl get pv -o json | jq -r '.items[] | select(.spec.csi.driver=="diskplugin.csi.alibabacloud.com") | "\(.metadata.name): \(.status.phase), diskId=\(.spec.csi.volumeHandle)"'
```

### 5.4 存储问题解决方案

```yaml
# 存储问题解决方案集

---
# 场景1: 创建 StorageClass (ACK 云盘)
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-essd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  fsType: ext4
  performanceLevel: PL1  # PL0/PL1/PL2/PL3
reclaimPolicy: Delete
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定

---
# 场景2: 手动创建 PV (静态供应)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv-001
spec:
  capacity:
    storage: 20Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: manual
  csi:
    driver: diskplugin.csi.alibabacloud.com
    volumeHandle: d-xxx  # 云盘ID
    fsType: ext4
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values:
          - cn-hangzhou-h

---
# 场景3: 修改 PVC 存储类
# 注意: PVC 创建后 storageClassName 不可修改，需重建
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-pvc
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-essd  # 指定正确的 SC
  resources:
    requests:
      storage: 20Gi
```

---

## 6. 配额与准入控制

### 6.1 配额限制类型

| 限制类型 | 作用范围 | 检查命令 | 版本特性 |
|---------|---------|---------|---------|
| **ResourceQuota** | 命名空间级别资源总量 | `kubectl describe quota` | 稳定 |
| **LimitRange** | 单个容器/Pod资源范围 | `kubectl describe limitrange` | 稳定 |
| **PodSecurityAdmission** | Pod安全标准 | `kubectl get ns -o yaml` | v1.25+ GA |
| **ValidatingAdmissionWebhook** | 自定义验证 | `kubectl get validatingwebhookconfigurations` | 稳定 |
| **MutatingAdmissionWebhook** | 自定义修改 | `kubectl get mutatingwebhookconfigurations` | 稳定 |

### 6.2 配额诊断命令

```bash
#!/bin/bash
# quota-diagnose.sh - 配额问题诊断

NAMESPACE=${1:-default}

echo "=============================================="
echo "  配额诊断: $NAMESPACE"
echo "=============================================="

echo ""
echo "=== 1. ResourceQuota 状态 ==="
kubectl get resourcequota -n $NAMESPACE -o wide
echo ""
kubectl describe resourcequota -n $NAMESPACE

echo ""
echo "=== 2. LimitRange 配置 ==="
kubectl get limitrange -n $NAMESPACE -o wide
echo ""
kubectl describe limitrange -n $NAMESPACE

echo ""
echo "=== 3. 命名空间资源使用汇总 ==="
echo "已用 CPU requests:"
kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.status.phase=="Running") | .spec.containers[].resources.requests.cpu // "0" | gsub("m";"") | if . == "0" then 0 else tonumber end] | add'
echo ""
echo "已用 Memory requests:"
kubectl get pods -n $NAMESPACE -o json | jq '[.items[] | select(.status.phase=="Running") | .spec.containers[].resources.requests.memory // "0" | gsub("Mi";"") | gsub("Gi";"000") | if . == "0" then 0 else tonumber end] | add' 
echo " Mi"

echo ""
echo "=== 4. Pod 数量 ==="
echo "Running: $(kubectl get pods -n $NAMESPACE --field-selector=status.phase=Running --no-headers | wc -l)"
echo "Pending: $(kubectl get pods -n $NAMESPACE --field-selector=status.phase=Pending --no-headers | wc -l)"
echo "Total: $(kubectl get pods -n $NAMESPACE --no-headers | wc -l)"

echo ""
echo "=== 5. PodSecurityAdmission 配置 ==="
kubectl get ns $NAMESPACE -o json | jq '.metadata.labels | with_entries(select(.key | startswith("pod-security")))'

echo ""
echo "=== 6. 准入 Webhook 配置 ==="
echo "--- ValidatingAdmissionWebhook ---"
kubectl get validatingwebhookconfigurations -o custom-columns='NAME:.metadata.name,WEBHOOKS:.webhooks[*].name'
echo ""
echo "--- MutatingAdmissionWebhook ---"
kubectl get mutatingwebhookconfigurations -o custom-columns='NAME:.metadata.name,WEBHOOKS:.webhooks[*].name'
```

### 6.3 配额问题解决

```yaml
# 配额问题解决方案集

---
# 场景1: 调整 ResourceQuota
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    requests.cpu: "100"      # 总 CPU requests 限制
    requests.memory: 200Gi   # 总内存 requests 限制
    limits.cpu: "200"        # 总 CPU limits 限制
    limits.memory: 400Gi     # 总内存 limits 限制
    pods: "200"              # Pod 数量限制
    persistentvolumeclaims: "50"
    requests.storage: 500Gi
    services.loadbalancers: "5"

---
# 场景2: 配置 LimitRange (设置默认值)
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: production
spec:
  limits:
  - type: Container
    default:            # 默认 limits
      cpu: 500m
      memory: 512Mi
    defaultRequest:     # 默认 requests
      cpu: 100m
      memory: 128Mi
    min:                # 最小值
      cpu: 50m
      memory: 64Mi
    max:                # 最大值
      cpu: 4
      memory: 8Gi
  - type: Pod
    max:
      cpu: 8
      memory: 16Gi

---
# 场景3: 配置 PodSecurity (v1.25+)
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: v1.32
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: v1.32
```

---

## 7. 调度器问题诊断

### 7.1 调度器故障类型

| 故障类型 | 现象 | 诊断方法 | 影响范围 |
|---------|------|---------|---------|
| **调度器不可用** | 新Pod无事件 | 检查scheduler Pod | 全集群 |
| **Leader选举问题** | 调度延迟高 | 检查lease对象 | 全集群 |
| **配置错误** | 特定Pod无法调度 | 检查调度器日志 | 部分Pod |
| **插件异常** | 调度失败+错误日志 | 检查插件状态 | 部分Pod |
| **自定义调度器缺失** | 指定schedulerName无响应 | 检查调度器部署 | 特定Pod |
| **性能问题** | 调度延迟高 | 检查metrics | 全集群 |

### 7.2 调度器诊断命令

```bash
#!/bin/bash
# scheduler-diagnose.sh - 调度器问题诊断

echo "=============================================="
echo "  调度器诊断报告"
echo "=============================================="

echo ""
echo "=== 1. 调度器 Pod 状态 ==="
kubectl get pods -n kube-system -l component=kube-scheduler -o wide

echo ""
echo "=== 2. 调度器健康检查 ==="
# 获取调度器 Pod
SCHEDULER_POD=$(kubectl get pods -n kube-system -l component=kube-scheduler -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$SCHEDULER_POD" ]; then
    kubectl exec -n kube-system $SCHEDULER_POD -- curl -s localhost:10259/healthz 2>/dev/null || echo "无法访问健康检查端点"
fi

echo ""
echo "=== 3. 调度器 Leader 选举 ==="
kubectl get lease kube-scheduler -n kube-system -o yaml 2>/dev/null | grep -E "holderIdentity|acquireTime|renewTime"

echo ""
echo "=== 4. 调度器日志 (最近50行) ==="
kubectl logs -n kube-system -l component=kube-scheduler --tail=50 2>/dev/null | tail -30

echo ""
echo "=== 5. 调度失败事件 (最近10条) ==="
kubectl get events -A --field-selector reason=FailedScheduling --sort-by='.lastTimestamp' 2>/dev/null | tail -10

echo ""
echo "=== 6. Pending Pod 统计 ==="
echo "按命名空间统计:"
kubectl get pods -A --field-selector=status.phase=Pending -o json | jq -r '.items | group_by(.metadata.namespace) | .[] | "\(.[0].metadata.namespace): \(length)"'

echo ""
echo "=== 7. 调度器指标 (如可访问) ==="
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes 2>/dev/null | jq '.items | length' | xargs -I {} echo "Metrics Server 可用，共 {} 个节点"

echo ""
echo "=== 8. 自定义调度器检查 ==="
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.schedulerName != null and .spec.schedulerName != "default-scheduler") | "\(.metadata.namespace)/\(.metadata.name): \(.spec.schedulerName)"' | head -10
```

### 7.3 调度器问题解决

```bash
# 调度器故障紧急处理

# 1. 重启调度器 (kubeadm 集群)
kubectl delete pod -n kube-system -l component=kube-scheduler

# 2. 检查调度器配置
kubectl get pods -n kube-system -l component=kube-scheduler -o yaml | grep -A 50 "containers:"

# 3. 强制 Leader 选举
kubectl delete lease kube-scheduler -n kube-system

# 4. 检查调度器资源使用
kubectl top pod -n kube-system -l component=kube-scheduler

# 5. 修改 Pod 使用默认调度器
kubectl patch deployment <name> -n <namespace> -p '{"spec":{"template":{"spec":{"schedulerName":"default-scheduler"}}}}'
```

---

## 8. 高级调度场景

### 8.1 拓扑分布约束 (TopologySpreadConstraints)

| 参数 | 说明 | 版本 | 最佳实践 |
|------|------|------|---------|
| `maxSkew` | 最大倾斜度 | v1.19+ | 通常设置1-2 |
| `topologyKey` | 拓扑域键 | v1.19+ | `topology.kubernetes.io/zone` |
| `whenUnsatisfiable` | 不满足时行为 | v1.19+ | 生产用 `DoNotSchedule` |
| `labelSelector` | Pod标签选择 | v1.19+ | 必须配置 |
| `minDomains` | 最小域数 | v1.25+ | 控制最小分布域 |
| `nodeAffinityPolicy` | 节点亲和性策略 | v1.26+ | `Honor` / `Ignore` |
| `nodeTaintsPolicy` | 节点污点策略 | v1.26+ | `Honor` / `Ignore` |
| `matchLabelKeys` | 动态标签键 | v1.27+ | 滚动更新场景 |

```yaml
# 拓扑分布最佳实践
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 6
  template:
    spec:
      topologySpreadConstraints:
      # 跨可用区分布 (硬约束)
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-app
        minDomains: 2  # v1.25+ 至少分布在2个zone
      # 跨节点分布 (软约束)
      - maxSkew: 2
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: web-app
```

### 8.2 抢占调度 (Preemption)

```yaml
# 抢占调度配置
---
# 定义优先级类
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: critical-production
value: 1000000
preemptionPolicy: PreemptLowerPriority
globalDefault: false
description: "生产关键服务，可抢占低优先级Pod"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: batch-processing
value: -1000
preemptionPolicy: Never  # 不触发抢占
globalDefault: false
description: "批处理任务，不抢占其他Pod"
---
# 使用优先级类
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-app
spec:
  template:
    spec:
      priorityClassName: critical-production
      containers:
      - name: app
        image: myapp:latest
```

### 8.3 调度门控 (Scheduling Gates - v1.27+ Beta)

```yaml
# 调度门控示例 (延迟调度直到满足条件)
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
  - name: example.com/waiting-for-config
  containers:
  - name: app
    image: myapp:latest
---
# 移除调度门 (允许调度)
# kubectl patch pod gated-pod --type=json -p='[{"op":"remove","path":"/spec/schedulingGates"}]'
```

---

## 9. ACK/云环境特定问题

### 9.1 ACK 特定调度问题

| 问题 | 原因 | 诊断 | 解决方案 |
|------|------|------|---------|
| **弹性节点池扩容慢** | 节点池配置/库存 | 检查节点池事件 | 调整节点池配置 |
| **抢占式实例回收** | 竞价实例被回收 | 检查节点事件 | 混合实例策略 |
| **跨可用区调度失败** | 网络/存储限制 | 检查拓扑标签 | 配置多可用区 |
| **GPU调度异常** | GPU驱动/cGPU配置 | 检查GPU插件 | 重启插件/检查配额 |
| **云盘无法挂载** | 云盘与节点不在同一可用区 | 检查PV nodeAffinity | WaitForFirstConsumer |

### 9.2 ACK 诊断命令

```bash
#!/bin/bash
# ack-pending-diagnose.sh - ACK 特定诊断

echo "=============================================="
echo "  ACK Pod Pending 诊断"
echo "=============================================="

echo ""
echo "=== 1. 节点池状态 ==="
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): pool=\(.metadata.labels["alibabacloud.com/nodepool-id"] // "N/A"), type=\(.metadata.labels["node.kubernetes.io/instance-type"] // "N/A")"'

echo ""
echo "=== 2. 弹性伸缩组件状态 ==="
kubectl get pods -n kube-system | grep -E "cluster-autoscaler|ess-"

echo ""
echo "=== 3. Cluster Autoscaler 日志 ==="
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=30 2>/dev/null

echo ""
echo "=== 4. GPU 节点状态 ==="
kubectl get nodes -l accelerator -o custom-columns='NAME:.metadata.name,GPU:.status.allocatable.nvidia\.com/gpu,CGPU:.status.allocatable.alibabacloud\.com/gpu-mem'

echo ""
echo "=== 5. GPU 调度情况 ==="
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.containers[].resources.requests["nvidia.com/gpu"] != null) | "\(.metadata.namespace)/\(.metadata.name): GPU=\(.spec.containers[].resources.requests["nvidia.com/gpu"])"'

echo ""
echo "=== 6. 云盘 CSI 状态 ==="
kubectl get pods -n kube-system -l app=csi-plugin -o wide
kubectl get pods -n kube-system -l app=csi-provisioner -o wide

echo ""
echo "=== 7. 节点可用区分布 ==="
kubectl get nodes -o json | jq -r '.items | group_by(.metadata.labels["topology.kubernetes.io/zone"]) | .[] | "\(.[0].metadata.labels["topology.kubernetes.io/zone"]): \(length) nodes"'

echo ""
echo "=== 8. 抢占式实例状态 ==="
kubectl get nodes -o json | jq -r '.items[] | select(.metadata.labels["alibabacloud.com/spot-instance"]=="true") | .metadata.name'
```

### 9.3 ACK 自动扩容配置

```yaml
# Cluster Autoscaler 配置 (ACK 托管版自动管理)
# 如需自建，参考以下配置

# 节点池自动扩容触发条件
# - Pod 因资源不足处于 Pending 状态
# - 节点池有扩容空间

# 推荐配置:
# - 启用多可用区节点池
# - 配置合适的最小/最大节点数
# - 使用混合实例 (按量+竞价)
# - 配置扩容冷却时间 (避免抖动)
```

---

## 10. 自动化诊断工具

### 10.1 完整诊断脚本

```bash
#!/bin/bash
# pod-pending-full-diagnose.sh - Pod Pending 完整诊断工具
# 版本: 1.0 | 适用: Kubernetes v1.25-v1.32

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

POD_NAME=${1:-""}
NAMESPACE=${2:-"default"}
OUTPUT_FILE="pod-pending-diagnosis-$(date +%Y%m%d-%H%M%S).txt"

log() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查必要工具
check_prerequisites() {
    for cmd in kubectl jq; do
        if ! command -v $cmd &> /dev/null; then
            error "$cmd 未安装"
            exit 1
        fi
    done
}

# 列出所有 Pending Pod
list_pending_pods() {
    log "当前 Pending 的 Pod 列表:"
    kubectl get pods -A --field-selector=status.phase=Pending -o wide
}

# 诊断单个 Pod
diagnose_pod() {
    local pod=$1
    local ns=$2
    
    echo ""
    echo "============================================================"
    echo "  诊断 Pod: $ns/$pod"
    echo "  时间: $(date)"
    echo "============================================================"
    
    # 1. 基本信息
    echo ""
    echo "=== 1. Pod 基本信息 ==="
    kubectl get pod $pod -n $ns -o wide
    
    # 2. 检查是否已分配节点
    echo ""
    echo "=== 2. 调度状态 ==="
    NODE_NAME=$(kubectl get pod $pod -n $ns -o jsonpath='{.spec.nodeName}' 2>/dev/null)
    if [ -n "$NODE_NAME" ]; then
        log "已分配节点: $NODE_NAME (问题在节点启动阶段)"
    else
        warn "未分配节点 (调度阶段问题)"
    fi
    
    # 3. Events
    echo ""
    echo "=== 3. Pod 事件 ==="
    kubectl describe pod $pod -n $ns | grep -A 30 "Events:" | head -35
    
    # 4. 调度失败原因分析
    echo ""
    echo "=== 4. 调度失败原因分析 ==="
    EVENTS=$(kubectl get events -n $ns --field-selector involvedObject.name=$pod,reason=FailedScheduling -o json 2>/dev/null)
    if [ "$(echo $EVENTS | jq '.items | length')" -gt "0" ]; then
        echo "$EVENTS" | jq -r '.items[-1].message'
        
        # 分析具体原因
        MSG=$(echo "$EVENTS" | jq -r '.items[-1].message')
        
        if echo "$MSG" | grep -q "Insufficient cpu"; then
            warn "诊断: CPU 资源不足"
            echo "建议: 扩容节点或调整 Pod CPU requests"
        fi
        
        if echo "$MSG" | grep -q "Insufficient memory"; then
            warn "诊断: 内存资源不足"
            echo "建议: 扩容节点或调整 Pod Memory requests"
        fi
        
        if echo "$MSG" | grep -q "didn't match Pod's node selector"; then
            warn "诊断: 节点选择器不匹配"
            echo "建议: 检查节点标签或修改 nodeSelector"
        fi
        
        if echo "$MSG" | grep -q "had taint"; then
            warn "诊断: 节点污点未容忍"
            echo "建议: 添加 tolerations 或移除节点污点"
        fi
        
        if echo "$MSG" | grep -q "unbound.*PersistentVolumeClaims"; then
            warn "诊断: PVC 未绑定"
            echo "建议: 检查 PVC 状态和 StorageClass 配置"
        fi
        
        if echo "$MSG" | grep -q "topology spread constraints"; then
            warn "诊断: 拓扑分布约束不满足"
            echo "建议: 调整 maxSkew 或增加节点"
        fi
    else
        warn "未找到 FailedScheduling 事件，可能是调度器问题"
    fi
    
    # 5. 资源请求
    echo ""
    echo "=== 5. Pod 资源请求 ==="
    kubectl get pod $pod -n $ns -o json | jq '.spec.containers[] | {name: .name, requests: .resources.requests, limits: .resources.limits}'
    
    # 6. 节点选择约束
    echo ""
    echo "=== 6. 节点选择约束 ==="
    echo "--- nodeSelector ---"
    kubectl get pod $pod -n $ns -o jsonpath='{.spec.nodeSelector}' | jq . 2>/dev/null || echo "(未配置)"
    echo ""
    echo "--- nodeAffinity ---"
    kubectl get pod $pod -n $ns -o jsonpath='{.spec.affinity.nodeAffinity}' | jq . 2>/dev/null || echo "(未配置)"
    echo ""
    echo "--- tolerations ---"
    kubectl get pod $pod -n $ns -o jsonpath='{.spec.tolerations}' | jq . 2>/dev/null || echo "(未配置)"
    echo ""
    echo "--- topologySpreadConstraints ---"
    kubectl get pod $pod -n $ns -o jsonpath='{.spec.topologySpreadConstraints}' | jq . 2>/dev/null || echo "(未配置)"
    
    # 7. PVC 状态
    echo ""
    echo "=== 7. PVC 状态 ==="
    PVCS=$(kubectl get pod $pod -n $ns -o jsonpath='{.spec.volumes[*].persistentVolumeClaim.claimName}' 2>/dev/null)
    if [ -n "$PVCS" ]; then
        for pvc in $PVCS; do
            echo "PVC: $pvc"
            kubectl get pvc $pvc -n $ns -o wide 2>/dev/null || echo "  (不存在)"
        done
    else
        echo "(Pod 未使用 PVC)"
    fi
    
    # 8. 配额检查
    echo ""
    echo "=== 8. 命名空间配额 ==="
    kubectl describe resourcequota -n $ns 2>/dev/null || echo "(无配额)"
    
    # 9. 节点资源概况
    echo ""
    echo "=== 9. 节点资源概况 ==="
    kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[?(@.type=="Ready")].status,CPU:.status.allocatable.cpu,MEMORY:.status.allocatable.memory,TAINTS:.spec.taints[*].key'
    
    echo ""
    echo "============================================================"
    echo "  诊断完成"
    echo "============================================================"
}

# 主流程
main() {
    check_prerequisites
    
    if [ -z "$POD_NAME" ]; then
        list_pending_pods
        echo ""
        echo "用法: $0 <pod-name> [namespace]"
        exit 0
    fi
    
    # 检查 Pod 是否存在
    if ! kubectl get pod $POD_NAME -n $NAMESPACE &>/dev/null; then
        error "Pod $NAMESPACE/$POD_NAME 不存在"
        exit 1
    fi
    
    # 检查 Pod 是否 Pending
    PHASE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.phase}')
    if [ "$PHASE" != "Pending" ]; then
        warn "Pod 当前状态为 $PHASE (非 Pending)"
    fi
    
    diagnose_pod $POD_NAME $NAMESPACE | tee $OUTPUT_FILE
    
    log "诊断结果已保存到: $OUTPUT_FILE"
}

main "$@"
```

---

## 11. 监控告警配置

### 11.1 Prometheus 告警规则

```yaml
# pod-pending-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-pending-alerts
  namespace: monitoring
  labels:
    release: prometheus
spec:
  groups:
  - name: pod.pending.rules
    interval: 30s
    rules:
    # Pod Pending 超过 5 分钟
    - alert: PodPendingTooLong
      expr: |
        sum by (namespace, pod) (
          kube_pod_status_phase{phase="Pending"} == 1
        ) * on (namespace, pod) group_left()
        (time() - kube_pod_created) > 300
      for: 1m
      labels:
        severity: warning
        category: scheduling
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 5 分钟"
        description: "Pod 调度失败，请检查调度器事件"
        runbook_url: "https://wiki.example.com/runbooks/pod-pending"
    
    # Pod Pending 超过 15 分钟 (严重)
    - alert: PodPendingCritical
      expr: |
        sum by (namespace, pod) (
          kube_pod_status_phase{phase="Pending"} == 1
        ) * on (namespace, pod) group_left()
        (time() - kube_pod_created) > 900
      for: 1m
      labels:
        severity: critical
        category: scheduling
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 15 分钟"
        description: "Pod 长时间无法调度，需立即处理"
    
    # 大量 Pod Pending
    - alert: ManyPodsPending
      expr: |
        count(kube_pod_status_phase{phase="Pending"}) > 10
      for: 5m
      labels:
        severity: warning
        category: scheduling
      annotations:
        summary: "集群有 {{ $value }} 个 Pod 处于 Pending 状态"
        description: "可能存在集群资源不足或调度器问题"
    
    # 命名空间 Pod Pending 过多
    - alert: NamespacePodsPending
      expr: |
        count by (namespace) (kube_pod_status_phase{phase="Pending"}) > 5
      for: 5m
      labels:
        severity: warning
        category: scheduling
      annotations:
        summary: "命名空间 {{ $labels.namespace }} 有 {{ $value }} 个 Pending Pod"
        description: "检查命名空间配额和资源配置"
    
    # 调度器调度失败率过高
    - alert: HighSchedulingFailureRate
      expr: |
        increase(scheduler_schedule_attempts_total{result="error"}[5m]) /
        increase(scheduler_schedule_attempts_total[5m]) > 0.1
      for: 5m
      labels:
        severity: warning
        category: scheduling
      annotations:
        summary: "调度器失败率超过 10%"
        description: "当前失败率: {{ $value | humanizePercentage }}"
    
    # 调度队列积压
    - alert: SchedulerQueueBacklog
      expr: |
        scheduler_pending_pods{queue="unschedulable"} > 50
      for: 10m
      labels:
        severity: warning
        category: scheduling
      annotations:
        summary: "调度队列积压 {{ $value }} 个不可调度 Pod"
        description: "检查集群资源和调度约束"
    
    # 集群 CPU 资源不足预警
    - alert: ClusterCPUResourcesLow
      expr: |
        (
          sum(kube_node_status_allocatable{resource="cpu"}) -
          sum(kube_pod_container_resource_requests{resource="cpu"})
        ) < 4
      for: 10m
      labels:
        severity: warning
        category: capacity
      annotations:
        summary: "集群可用 CPU 资源不足 4 核"
        description: "剩余 CPU: {{ $value }} 核，考虑扩容"
    
    # 集群 Memory 资源不足预警
    - alert: ClusterMemoryResourcesLow
      expr: |
        (
          sum(kube_node_status_allocatable{resource="memory"}) -
          sum(kube_pod_container_resource_requests{resource="memory"})
        ) / 1024 / 1024 / 1024 < 8
      for: 10m
      labels:
        severity: warning
        category: capacity
      annotations:
        summary: "集群可用内存资源不足 8 Gi"
        description: "剩余内存: {{ $value | humanize }}i，考虑扩容"

---
# Grafana Dashboard 配置 (JSON简化版)
# 完整 Dashboard 请参考: https://grafana.com/grafana/dashboards/
```

### 11.2 关键监控指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `kube_pod_status_phase{phase="Pending"}` | Pending Pod 数量 | < 5 | > 10 |
| `scheduler_pending_pods` | 调度队列 Pod 数量 | < 10 | > 50 |
| `scheduler_schedule_attempts_total` | 调度尝试总数 | - | 失败率>10% |
| `scheduler_scheduling_attempt_duration_seconds` | 调度延迟 | P99 < 100ms | P99 > 1s |
| `scheduler_preemption_attempts_total` | 抢占尝试次数 | 增长缓慢 | 快速增长 |

---

## 12. 紧急处理流程

### 12.1 紧急处理命令

```bash
#!/bin/bash
# emergency-pod-pending.sh - Pod Pending 紧急处理

# === 紧急级别判断 ===
echo "=== 紧急级别判断 ==="
PENDING_COUNT=$(kubectl get pods -A --field-selector=status.phase=Pending --no-headers | wc -l)
echo "当前 Pending Pod 数量: $PENDING_COUNT"

if [ $PENDING_COUNT -gt 50 ]; then
    echo "紧急级别: P0 - 大规模调度故障"
elif [ $PENDING_COUNT -gt 20 ]; then
    echo "紧急级别: P1 - 严重调度问题"
elif [ $PENDING_COUNT -gt 5 ]; then
    echo "紧急级别: P2 - 中等调度问题"
else
    echo "紧急级别: P3 - 常规处理"
fi

# === P0/P1 紧急处理命令 ===
echo ""
echo "=== P0/P1 紧急处理命令 ==="

echo "# 1. 检查调度器状态"
echo "kubectl get pods -n kube-system -l component=kube-scheduler"

echo ""
echo "# 2. 重启调度器 (如调度器故障)"
echo "kubectl delete pod -n kube-system -l component=kube-scheduler"

echo ""
echo "# 3. 紧急扩容节点 (ACK)"
echo "# aliyun cs POST /clusters/{ClusterId}/nodepools/{NodepoolId}/nodes --body '{\"count\": 3}'"

echo ""
echo "# 4. 临时移除节点污点"
echo "kubectl taint node <node-name> <key>:<effect>-"

echo ""
echo "# 5. 取消节点不可调度"
echo "kubectl uncordon <node-name>"

echo ""
echo "# 6. 强制删除卡住的 Pod (让控制器重建)"
echo "kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0"

echo ""
echo "# 7. 临时清除调度约束 (仅紧急情况)"
echo "kubectl patch deployment <name> -n <namespace> -p '{\"spec\":{\"template\":{\"spec\":{\"nodeSelector\":null,\"affinity\":null}}}}'"

echo ""
echo "# 8. 驱逐低优先级 Pod 释放资源"
echo "kubectl get pods -A -o json | jq -r '.items[] | select(.spec.priority != null and .spec.priority < 0) | \"\\(.metadata.namespace) \\(.metadata.name)\"' | while read ns pod; do kubectl delete pod \$pod -n \$ns; done"
```

### 12.2 紧急处理流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Pod Pending 紧急处理流程                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  触发告警: Pod Pending > 15min 或 批量 Pending                                       │
│                     │                                                                │
│                     ▼                                                                │
│  ┌──────────────────────────────────────────────────────────────┐                   │
│  │ Step 1: 快速评估 (2min)                                       │                   │
│  │ - kubectl get pods -A --field-selector=status.phase=Pending   │                   │
│  │ - 确定影响范围和紧急级别                                       │                   │
│  └──────────────────────────────────────────────────────────────┘                   │
│                     │                                                                │
│         ┌──────────┴──────────┐                                                     │
│         │                     │                                                     │
│    单个Pod               批量Pod                                                    │
│         │                     │                                                     │
│         ▼                     ▼                                                     │
│  ┌────────────┐       ┌────────────────────────────────────────┐                   │
│  │ 常规诊断   │       │ Step 2: 检查调度器 (1min)               │                   │
│  │ (跳转诊断  │       │ kubectl get pods -n kube-system        │                   │
│  │  决策树)   │       │   -l component=kube-scheduler           │                   │
│  └────────────┘       └────────────────────────────────────────┘                   │
│                                  │                                                  │
│                     ┌────────────┴────────────┐                                    │
│                     │                         │                                    │
│              调度器正常                  调度器异常                                  │
│                     │                         │                                    │
│                     ▼                         ▼                                    │
│  ┌────────────────────────┐    ┌────────────────────────────────┐                 │
│  │ Step 3: 检查节点资源    │    │ 重启调度器                      │                 │
│  │ kubectl top nodes       │    │ kubectl delete pod -n kube-system│                 │
│  │ kubectl describe nodes  │    │   -l component=kube-scheduler   │                 │
│  └────────────────────────┘    └────────────────────────────────┘                 │
│                     │                                                               │
│         ┌──────────┴──────────┐                                                    │
│         │                     │                                                    │
│    资源充足              资源不足                                                   │
│         │                     │                                                    │
│         ▼                     ▼                                                    │
│  ┌────────────────┐   ┌────────────────────────────────────────┐                  │
│  │ 检查调度约束   │   │ Step 4: 紧急扩容                        │                  │
│  │ - nodeSelector │   │ - 触发 Cluster Autoscaler               │                  │
│  │ - taints       │   │ - 手动扩容节点池                        │                  │
│  │ - PVC          │   │ - 驱逐低优先级 Pod                      │                  │
│  └────────────────┘   └────────────────────────────────────────┘                  │
│                                  │                                                  │
│                                  ▼                                                  │
│  ┌──────────────────────────────────────────────────────────────┐                  │
│  │ Step 5: 验证恢复                                              │                  │
│  │ - 确认 Pending Pod 开始调度                                   │                  │
│  │ - 监控新 Pod 调度情况                                         │                  │
│  │ - 记录事件和根因                                              │                  │
│  └──────────────────────────────────────────────────────────────┘                  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 13. 版本特定变更

### 13.1 调度器功能演进

| 版本 | 特性 | 状态 | 影响 |
|------|------|------|------|
| **v1.25** | PodSchedulingReadiness (调度门控) | Alpha | 延迟调度直到满足条件 |
| **v1.26** | NodeInclusionPolicyInPodTopologySpread | Beta | 拓扑分布考虑节点亲和性 |
| **v1.27** | SchedulerQueueingHints | Beta | 智能重调度 |
| **v1.27** | PodSchedulingReadiness | Beta | 调度门控增强 |
| **v1.28** | MatchLabelKeys in TopologySpread | Beta | 滚动更新时的拓扑分布 |
| **v1.29** | Scheduler QueueingHints | GA | 智能重调度稳定 |
| **v1.30** | VolumeCapacityPriority | Beta | 考虑卷容量的调度 |
| **v1.31** | Pod Scheduling Readiness | GA | 调度门控稳定 |
| **v1.32** | DRA (Dynamic Resource Allocation) | Beta | 动态资源分配增强 |

### 13.2 版本特定配置

```yaml
# v1.27+ 调度门控示例
apiVersion: v1
kind: Pod
metadata:
  name: gated-pod
spec:
  schedulingGates:
  - name: example.com/wait-for-config
  containers:
  - name: app
    image: myapp:latest
---
# v1.28+ MatchLabelKeys 示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    metadata:
      labels:
        app: web
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web
        matchLabelKeys:
        - pod-template-hash  # v1.28+ 滚动更新时只考虑同版本Pod
```

---

## 14. 多角色视角

### 14.1 架构师视角

| 关注点 | 设计要点 | 推荐配置 |
|-------|---------|---------|
| **高可用设计** | 跨可用区分布 | topologySpreadConstraints |
| **资源规划** | 预留缓冲资源 | 集群利用率<80% |
| **扩展性** | 自动扩缩容 | Cluster Autoscaler |
| **优先级策略** | 关键服务优先 | PriorityClass 分级 |
| **故障隔离** | Pod反亲和性 | podAntiAffinity |

### 14.2 测试工程师视角

| 测试类型 | 测试目标 | 测试方法 |
|---------|---------|---------|
| **调度功能测试** | 验证调度约束正确性 | 创建各种约束的Pod |
| **资源压力测试** | 验证资源不足处理 | 模拟资源耗尽场景 |
| **扩缩容测试** | 验证自动扩容 | 触发扩容并验证 |
| **故障注入测试** | 验证调度器恢复 | 杀死调度器观察行为 |
| **性能测试** | 调度延迟基准 | 批量创建Pod测量延迟 |

### 14.3 产品经理视角

| SLA指标 | 目标值 | 监控方式 |
|--------|-------|---------|
| **调度成功率** | > 99.9% | scheduler_schedule_attempts_total |
| **调度延迟 P99** | < 1s | scheduler_scheduling_attempt_duration_seconds |
| **Pending Pod数量** | < 10 | kube_pod_status_phase{phase="Pending"} |
| **资源可用率** | > 20% | 集群资源利用率 |

---

## 15. 最佳实践

### 15.1 预防措施清单

- [ ] **资源管理**
  - [ ] 为所有容器设置 requests 和 limits
  - [ ] requests 基于实际使用量 P95 设置
  - [ ] 配置 VPA 自动调整资源
  - [ ] 集群保留 20%+ 缓冲资源

- [ ] **调度策略**
  - [ ] 使用软亲和性避免调度失败
  - [ ] 配置合理的拓扑分布约束
  - [ ] 为关键服务配置 PriorityClass
  - [ ] 避免过于严格的 nodeSelector

- [ ] **存储配置**
  - [ ] 使用 `WaitForFirstConsumer` 绑定模式
  - [ ] 确保 StorageClass 正确配置
  - [ ] 定期检查 CSI 驱动健康状态

- [ ] **监控告警**
  - [ ] 配置 Pod Pending 告警
  - [ ] 监控集群资源使用率
  - [ ] 监控调度器健康状态
  - [ ] 配置自动扩容触发条件

### 15.2 诊断原则

1. **先看 Events**: `kubectl describe pod` 确定根本原因
2. **分类排查**: 资源 → 节点选择 → 存储 → 配额 → 调度器
3. **针对性解决**: 根据具体原因采取对应措施
4. **验证修复**: 确认 Pod 成功调度并运行
5. **记录根因**: 更新运维知识库，防止复发

### 15.3 相关文档

| 主题 | 文档编号 | 说明 |
|------|---------|------|
| 节点问题诊断 | 103-node-notready-diagnosis | 节点 NotReady 排查 |
| 存储问题诊断 | 104-storage-issue-diagnosis | PVC/PV 问题排查 |
| 调度器深度解析 | 164-kube-scheduler-deep-dive | 调度框架详解 |
| Cluster Autoscaler | 45-cluster-autoscaler | 自动扩缩容配置 |
| 资源管理 | 05-resource-management | 资源配置最佳实践 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01 | 版本: v1.25-v1.32
