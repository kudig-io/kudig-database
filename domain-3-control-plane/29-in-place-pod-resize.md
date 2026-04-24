# 29 - 原地 Pod 资源调整 (In-Place Pod Resize)

> **适用版本**: Kubernetes v1.27 Beta, v1.32+ 更完善 | **最后更新**: 2026-04 | **文档类型**: 控制平面特性文档

---

## 目录

1. [特性概述](#1-特性概述)
2. [与传统资源调整方式的对比](#2-与传统资源调整方式的对比)
3. [前置条件与 Feature Gate](#3-前置条件与-feature-gate)
4. [API 字段详解](#4-api-字段详解)
5. [完整 YAML 示例](#5-完整-yaml-示例)
6. [内部机制](#6-内部机制)
7. [限制条件](#7-限制条件)
8. [使用场景](#8-使用场景)
9. [与 VPA 集成](#9-与-vpa-集成)
10. [故障排查](#10-故障排查)

---

## 1. 特性概述

### 1.1 什么是原地 Pod 资源调整

原地 Pod 资源调整（In-Place Pod Vertical Scaling）允许用户在**不重启容器、不重建 Pod**的情况下，在线调整运行中 Pod 的 CPU 和内存资源（requests 和 limits）。

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        传统方式 vs 原地调整                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   传统方式 (Rolling Update / Recreate)                                       │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐ │
│   │  修改资源    │───▶│  重建 Pod   │───▶│  拉取镜像    │───▶│  启动容器   │ │
│   │  (Deployment)│    │  (终止旧Pod) │    │  (可能耗时)  │    │  (初始化)   │ │
│   └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘ │
│         │                                                        │          │
│         └──────────────────── 服务中断 ────────────────────────────┘          │
│                                                                              │
│   原地调整 (In-Place Resize)                                                 │
│   ┌─────────────┐    ┌────────────────────────────────────┐                  │
│   │  修改资源    │───▶│  kubelet 调整 cgroup 限制          │                  │
│   │  (Patch Pod)│    │  容器继续运行，无中断               │                  │
│   └─────────────┘    └────────────────────────────────────┘                  │
│         │                                                                    │
│         └──────────────────── 零中断 ────────────────────────────────────────┘│
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 核心能力

| 能力 | 支持情况 | 说明 |
|:---|:---|:---|
| **CPU requests 调整** | ✅ 支持 | 可增大或缩小 |
| **CPU limits 调整** | ✅ 支持 | 可增大或缩小 |
| **内存 requests 调整** | ✅ 支持 | 可增大或缩小 |
| **内存 limits 调整** | ⚠️ 仅增大 | 运行时限制，不能缩小 |
| **同时调整多个容器** | ✅ 支持 | Pod 内多容器独立配置 |
| **回滚资源** | ✅ 支持 | CPU 和内存 requests 可回滚 |

---

## 2. 与传统资源调整方式的对比

### 2.1 三种方式详细对比

| 维度 | 原地调整 (In-Place) | 滚动更新 (Rolling Update) | 重建 (Recreate) |
|:---|:---|:---|:---|
| **服务中断** | 零中断 | 无（有新旧共存期）| 有中断 |
| **调整速度** | 秒级（cgroup 更新）| 分钟级（取决于副本数）| 分钟级 |
| **容器重启** | 否 | 否 | 是 |
| **Pod 重建** | 否 | 是 | 是 |
| **IP 保持** | 是 | 否 | 否 |
| **节点调度** | 原地 | 可能调度到其他节点 | 可能调度到其他节点 |
| **内存 limits 缩小** | ❌ 不支持 | ✅ 支持 | ✅ 支持 |
| **适用场景** | 在线业务紧急扩容 | 常规配置变更 | 允许中断的维护 |

### 2.2 决策流程

```
                    ┌─────────────────┐
                    │ 需要调整资源?    │
                    └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ 是否允许中断?    │
                    └────────┬────────┘
                             │
            ┌────────────────┼────────────────┐
            │ 是              │ 否              │
            ▼                ▼                ▼
    ┌───────────────┐ ┌───────────────┐ ┌───────────────┐
    │ 允许重建 Pod?  │ │ 原地调整       │ │ 滚动更新       │
    └───────┬───────┘ │ (In-Place)    │ │ (Rolling)     │
            │         └───────────────┘ └───────────────┘
    ┌───────┴───────┐
    │ 是      │ 否   │
    ▼         ▼      │
┌────────┐ ┌────────┐│
│Recreate│ │Rolling ││
└────────┘ └────────┘│
```
---

## 3. 前置条件与 Feature Gate

### 3.1 Feature Gate 配置

原地 Pod 资源调整通过 `InPlacePodVerticalScaling` Feature Gate 控制。

| 版本 | 状态 | 说明 |
|:---|:---|:---|
| v1.27 | Alpha | 首次引入，默认关闭 |
| v1.28-v1.31 | Alpha | 持续改进，仍默认关闭 |
| **v1.32+** | **Beta** | **默认开启，更完善** |

### 3.2 启用 Feature Gate

```bash
# kube-apiserver 配置
--feature-gates=InPlacePodVerticalScaling=true

# kubelet 配置
--feature-gates=InPlacePodVerticalScaling=true
```

或在 KubeletConfiguration 中配置：

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  InPlacePodVerticalScaling: true
```

### 3.3 运行时要求

| 运行时 | 最低版本 | 支持状态 |
|:---|:---|:---|
| **containerd** | v1.6.9+ | ✅ 完全支持 |
| **CRI-O** | v1.25+ | ✅ 完全支持 |
| **docker** | 已弃用 | ❌ 不支持 |
---

## 4. API 字段详解

### 4.1 resizePolicy 字段

`resizePolicy` 定义在每个容器的 spec 中，控制资源调整时容器是否需要重启。

| 字段 | 类型 | 可选值 | 说明 |
|:---|:---|:---|:---|
| `resizePolicy[].resourceName` | string | `cpu` / `memory` | 资源类型 |
| `resizePolicy[].restartPolicy` | string | `RestartNotRequired` / `RestartContainer` | 调整时是否重启容器 |

#### restartPolicy 取值说明

| 值 | 含义 | 适用场景 |
|:---|:---|:---|
| **`RestartNotRequired`** | 调整资源时**不重启**容器 | 在线业务，要求零中断 |
| **`RestartContainer`** | 调整资源时**重启**容器 | 应用需要感知资源变化 |

### 4.2 状态字段

调整过程中，Pod 的 `status` 会反映资源调整的状态。

| 字段路径 | 类型 | 说明 |
|:---|:---|:---|
| `status.containerStatuses[].resources` | ResourceRequirements | 容器**当前实际**资源分配 |
| `status.containerStatuses[].allocatedResources` | ResourceRequirements | kubelet **已分配**的资源 |
| `status.resize` | string | Pod 级别调整状态 |

#### resize 状态值

| 状态 | 含义 |
|:---|:---|
| `""` (空) | 无调整进行 |
| `Proposed` | 已提出调整请求 |
| `InProgress` | 调整进行中 |
| `Deferred` | 调整被延迟（节点资源不足）|
| `Infeasible` | 调整不可行 |

### 4.3 容器调整状态条件

在 `status.conditions` 中可能新增与资源调整相关的条件。

| 条件类型 | 状态 | 原因 | 说明 |
|:---|:---|:---|:---|
| `ContainerResizePending` | True | `ResizePending` | 等待 kubelet 处理调整 |
| `ContainerResizeInProgress` | True | `ResizeInProgress` | kubelet 正在执行调整 |
---

## 5. 完整 YAML 示例

### 5.1 创建带 resizePolicy 的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx-inplace-resize
  namespace: default
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    ports:
    - containerPort: 80
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "200m"
        memory: "256Mi"
    # 定义资源调整策略
    resizePolicy:
    - resourceName: cpu
      restartPolicy: RestartNotRequired
    - resourceName: memory
      restartPolicy: RestartNotRequired
```

### 5.2 使用 patch 调整资源

```bash
# 方法 1: 使用 kubectl patch
kubectl patch pod nginx-inplace-resize --patch "
{
  "spec": {
    "containers": [
      {
        "name": "nginx",
        "resources": {
          "requests": {
            "cpu": "500m",
            "memory": "256Mi"
          },
          "limits": {
            "cpu": "1000m",
            "memory": "512Mi"
          }
        }
      }
    ]
  }
}"

# 方法 2: 使用 kubectl apply（需要完整 YAML）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: nginx-inplace-resize
spec:
  containers:
  - name: nginx
    image: nginx:1.25
    resources:
      requests:
        cpu: "500m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "512Mi"
    resizePolicy:
    - resourceName: cpu
      restartPolicy: RestartNotRequired
    - resourceName: memory
      restartPolicy: RestartNotRequired
EOF
```

### 5.3 查看调整状态

```bash
# 查看 Pod 详细状态
kubectl get pod nginx-inplace-resize -o yaml

# 重点查看字段
kubectl get pod nginx-inplace-resize -o jsonpath="
资源请求 (spec):      {.spec.containers[0].resources.requests}{\n}
资源限制 (spec):      {.spec.containers[0].resources.limits}{\n}
已分配资源:           {.status.containerStatuses[0].allocatedResources}{\n}
当前实际资源:         {.status.containerStatuses[0].resources}{\n}
调整状态:             {.status.resize}{\n}
"
```

### 5.4 多容器 Pod 示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-container-resize
spec:
  containers:
  - name: app
    image: myapp:v1
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
    resizePolicy:
    - resourceName: cpu
      restartPolicy: RestartNotRequired
    - resourceName: memory
      restartPolicy: RestartNotRequired

  - name: sidecar
    image: fluent-bit:latest
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "200m"
        memory: "256Mi"
    resizePolicy:
    - resourceName: cpu
      restartPolicy: RestartNotRequired
    - resourceName: memory
      restartPolicy: RestartContainer  # sidecar 重启以感知新资源
```

### 5.5 状态输出示例

```yaml
status:
  phase: Running
  resize: "InProgress"                    # Pod 级别调整状态
  containerStatuses:
  - name: nginx
    state:
      running:
        startedAt: "2026-04-23T08:00:00Z"
    resources:                            # 当前实际资源 (调整中或已生效)
      limits:
        cpu: 1000m
        memory: 512Mi
      requests:
        cpu: 500m
        memory: 256Mi
    allocatedResources:                   # kubelet 已分配资源
      limits:
        cpu: 1000m
        memory: 512Mi
      requests:
        cpu: 500m
        memory: 256Mi
  conditions:
  - type: ContainerResizeInProgress
    status: "True"
    reason: "ResizeInProgress"
    message: "Container resource resize is in progress"
    lastTransitionTime: "2026-04-23T08:05:00Z"
```
---

## 6. 内部机制

### 6.1 整体架构流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        In-Place Pod Resize 架构流程                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  User / Controller                                                           │
│       │                                                                      │
│       │ PATCH Pod Spec (resources.requests/limits)                           │
│       ▼                                                                      │
│  ┌─────────────────┐                                                         │
│  │   API Server    │  ← 验证资源合法性、更新 Pod Spec                        │
│  └────────┬────────┘                                                         │
│           │ Watch Event                                                      │
│           ▼                                                                  │
│  ┌─────────────────┐                                                         │
│  │     kubelet     │  ← 接收 Pod 更新事件                                    │
│  │                 │                                                         │
│  │  ┌───────────┐  │  1. 比较 allocatedResources vs spec.resources          │
│  │  │  Resize   │  │  2. 检查节点资源是否充足                                │
│  │  │  Manager  │  │  3. 调用 CRI UpdateContainerResources                  │
│  │  └─────┬─────┘  │                                                         │
│  └────────┼────────┘                                                         │
│           │ CRI (Container Runtime Interface)                                 │
│           │ UpdateContainerResources                                          │
│           ▼                                                                  │
│  ┌─────────────────┐                                                         │
│  │ containerd/     │  ← 接收 CRI 调用                                        │
│  │ CRI-O           │                                                         │
│  └────────┬────────┘                                                         │
│           │                                                                  │
│           ▼                                                                  │
│  ┌─────────────────┐                                                         │
│  │   runc / crun   │  ← 通过 cgroup v1/v2 更新资源限制                       │
│  └────────┬────────┘                                                         │
│           │                                                                  │
│           ▼                                                                  │
│  ┌─────────────────┐                                                         │
│  │  cgroup (fs)    │  ← cpu.shares, cpu.max, memory.max 等                  │
│  │                 │                                                         │
│  │  /sys/fs/cgroup/.../pod-xxx/                                             │
│  └─────────────────┘                                                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 kubelet 处理流程

```
Pod Spec Update (resources changed)
         │
         ▼
┌──────────────────────────────┐
│   kubelet SyncPod()          │
│                              │
│  1. 读取 spec.resources      │
│  2. 对比 allocatedResources  │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   资源调整决策                │
│                              │
│  节点资源充足?                │
│       ├─ 否 → Deferred       │
│       └─ 是 → 继续           │
│                              │
│  resizePolicy 允许?          │
│       ├─ RestartRequired →   │
│       │   标记待重启          │
│       └─ NotRequired → 继续  │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   CRI UpdateContainer        │
│   Resources()                │
└──────────────┬───────────────┘
               │
               ▼
┌──────────────────────────────┐
│   更新 Pod Status            │
│                              │
│  - allocatedResources        │
│  - containerStatuses[].resources│
│  - resize 状态               │
└──────────────────────────────┘
```

### 6.3 cgroup 调整流程

#### CPU 调整

| cgroup v2 文件 | 对应资源 | 说明 |
|:---|:---|:---|
| `cpu.max` | CPU limits | 格式: "quota period"，如 "100000 100000" |
| `cpu.weight` | CPU requests | 默认值 100，与 shares 对应 |

#### 内存调整

| cgroup v2 文件 | 对应资源 | 说明 |
|:---|:---|:---|
| `memory.max` | memory limits | 硬限制，不可超过 |
| `memory.min` | memory requests | 保证内存，内存不足时优先回收 |
| `memory.high` | 水位线 | 触发内存回收的压力线 |

```bash
# 查看 Pod 的 cgroup 路径（containerd + cgroup v2）
ls /sys/fs/cgroup/kubepods.slice/kubepods-pod<pod_uid>.slice/

# 查看容器的 CPU 限制
cat /sys/fs/cgroup/kubepods.slice/.../cpu.max

# 查看容器的内存限制
cat /sys/fs/cgroup/kubepods.slice/.../memory.max
```

### 6.4 状态转换图

```
                        ┌─────────────┐
                        │    Idle     │
                        │   (无调整)   │
                        └──────┬──────┘
                               │ 用户 PATCH 资源
                               ▼
                        ┌─────────────┐
                        │  Proposed   │
                        │  (已提出)   │
                        └──────┬──────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ▼                ▼                ▼
       ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
       │ InProgress  │ │  Deferred   │ │ Infeasible  │
       │ (调整中)    │ │ (延迟)      │ │ (不可行)    │
       └──────┬──────┘ └─────────────┘ └─────────────┘
              │
              │ 调整完成
              ▼
       ┌─────────────┐
       │   Idle      │
       │ (调整完成)  │
       └─────────────┘
```
---

## 7. 限制条件

### 7.1 运行时限制

| 限制 | 说明 | 影响 |
|:---|:---|:---|
| **内存 limits 不能缩小** | Linux cgroup 内存限制一旦提升，运行时无法安全缩小 | 只能增大内存 limits，缩小需重建 Pod |
| **节点资源不足** | 目标节点剩余资源不足以满足新的 requests | 调整被标记为 Deferred |
| **CPU 拓扑约束** | NUMA 绑定的 Pod 调整可能受拓扑限制 | 可能导致调整失败 |

### 7.2 QoS 类限制

| QoS 类 | 原地调整支持 | 说明 |
|:---|:---|:---|
| **Guaranteed** | ✅ 支持 | requests == limits，调整时需同时修改两者 |
| **Burstable** | ✅ 支持 | 最灵活，可独立调整 requests 和 limits |
| **BestEffort** | ⚠️ 有限 | 无 resources 定义，不适用 |

### 7.3 其他限制

| 限制 | 说明 |
|:---|:---|
| **Init 容器** | ❌ 不支持，Init 容器运行结束后无法调整 |
| **静态 Pod** | ⚠️ 有限支持，取决于 kubelet 配置 |
| **Windows 容器** | ❌ 当前不支持 |
| **设备资源** | ❌ GPU 等设备资源不支持原地调整 |
| **Pod 终止中** | ❌ Terminating 状态的 Pod 不可调整 |

---

## 8. 使用场景

### 8.1 场景一：在线业务负载突增紧急扩容

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         紧急扩容场景                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  流量突增                                                                    │
│      │                                                                       │
│      ▼                                                                       │
│  HPA 指标异常 / 告警触发                                                     │
│      │                                                                       │
│      ▼                                                                       │
│  运维人员 / 自动化脚本                                                        │
│      │                                                                       │
│      ├── 水平扩容 (HPA) ──▶ 创建新 Pod（分钟级，可能来不及）                 │
│      │                                                                       │
│      └── 原地垂直扩容 ─────▶ 秒级提升 CPU/Memory，立即生效                    │
│                              │                                               │
│                              ▼                                               │
│                         业务恢复正常，零中断                                  │
│                                                                              │
│  优势:                                                                       │
│  - 无需等待 Pod 创建和初始化                                                 │
│  - 保持现有连接和会话                                                        │
│  - IP 和节点位置不变                                                         │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.2 场景二：开发测试环境动态调整

```bash
# 白天开发高峰期扩容
kubectl patch pod dev-app --patch "
{
  "spec": {
    "containers": [
      {
        "name": "app",
        "resources": {
          "requests": {
            "cpu": "1",
            "memory": "2Gi"
          },
          "limits": {
            "cpu": "2",
            "memory": "4Gi"
          }
        }
      }
    ]
  }
}"

# 夜间低峰期缩容回原始配置（CPU 可缩，内存 limits 不可缩）
kubectl patch pod dev-app --patch "
{
  "spec": {
    "containers": [
      {
        "name": "app",
        "resources": {
          "requests": {
            "cpu": "200m",
            "memory": "512Mi"
          },
          "limits": {
            "cpu": "500m",
            "memory": "4Gi"
          }
        }
      }
    ]
  }
}"
```

### 8.3 场景三：VPA 的 in-place 模式

详见第 9 节 [与 VPA 集成](#9-与-vpa-集成)。
---

## 9. 与 VPA 集成

### 9.1 VPA 模式对比

Vertical Pod Autoscaler (VPA) 支持多种更新模式，原地调整让 Auto 模式更加高效。

| VPA 模式 | 行为 | 是否使用原地调整 | 适用场景 |
|:---|:---|:---|:---|
| **Off** | 仅推荐，不执行 | - | 观察和学习 |
| **Initial** | 仅在创建时应用 | - | 启动时配置 |
| **Recreate** | 重建 Pod 应用 | ❌ | 允许短暂中断 |
| **Auto** | 自动选择最佳方式 | ✅ v1.32+ | 优先原地，不可行时重建 |

### 9.2 VPA + 原地调整架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    VPA + In-Place Resize 架构                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌─────────────┐                                                            │
│   │  Metrics    │  ← Prometheus / Metrics Server                            │
│   │  Server     │                                                            │
│   └──────┬──────┘                                                            │
│          │ 容器资源使用指标                                                    │
│          ▼                                                                  │
│   ┌─────────────┐     ┌─────────────┐                                       │
│   │  VPA        │────▶│  VPA        │                                       │
│   │  Recommender│     │  Updater    │                                       │
│   │  (推荐算法)  │     │  (执行调整)  │                                       │
│   └─────────────┘     └──────┬──────┘                                       │
│                              │                                              │
│          ┌───────────────────┼───────────────────┐                         │
│          │                   │                   │                         │
│          ▼                   ▼                   ▼                         │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│   │ In-Place    │    │ Evict +     │    │ Recreate    │                   │
│   │ Resize      │    │ Reschedule  │    │ (fallback)  │                   │
│   │ (零中断)    │    │ (节点不变)  │    │ (允许中断)  │                   │
│   └─────────────┘    └─────────────┘    └─────────────┘                   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 VPA 配置示例

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: "Auto"  # Auto 模式优先尝试原地调整
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: "100m"
        memory: "128Mi"
      maxAllowed:
        cpu: "4"
        memory: "8Gi"
      controlledResources: ["cpu", "memory"]
```

### 9.4 VPA 与原地调整的协同工作

| 步骤 | VPA Recommender | VPA Updater | kubelet |
|:---|:---|:---|:---|
| 1. 采集指标 | 收集容器资源使用数据 | - | - |
| 2. 计算推荐 | 生成推荐 resources | - | - |
| 3. 决策调整 | - | 选择调整策略 | - |
| 4. 执行调整 | - | PATCH Pod | - |
| 5. 应用变更 | - | - | 更新 cgroup |
| 6. 验证状态 | - | 监控 resize 状态 | 上报状态 |

---

## 10. 故障排查

### 10.1 常见问题速查

| 现象 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Pod status.resize = Deferred | 节点资源不足 | 检查节点资源，或调度到其他节点 |
| Pod status.resize = Infeasible | 违反限制条件 | 检查 resizePolicy 和 QoS 类 |
| 调整后资源未生效 | Feature Gate 未启用 | 确认 apiserver 和 kubelet 均启用 |
| 容器被重启 | resizePolicy 为 RestartContainer | 改为 RestartNotRequired 或预期内重启 |
| 内存 limits 无法缩小 | 运行时限制 | 需重建 Pod |
| VPA 不执行原地调整 | 版本过低或模式不对 | 升级到 v1.32+，使用 Auto 模式 |

### 10.2 排查命令

```bash
# 1. 确认 Feature Gate 已启用
kubectl get nodes -o jsonpath="{.items[0].status.nodeInfo.kubeletVersion}"

# 2. 查看 Pod 调整状态
kubectl get pod <pod-name> -o json | jq "{spec: .spec.containers[].resources, status: .status.containerStatuses[].allocatedResources}"

# 3. 查看 kubelet 日志
journalctl -u kubelet -f | grep -i resize

# 4. 检查 cgroup 实际值
crictl inspect <container-id> | grep -A 5 linux.resources

# 5. 查看节点资源压力
kubectl describe node <node-name> | grep -A 10 Allocated
```

### 10.3 事件分析

```bash
# 查看与资源调整相关的事件
kubectl get events --field-selector reason=ContainerResizePending
kubectl get events --field-selector reason=ContainerResizeInProgress

# 查看 Pod 事件
kubectl describe pod <pod-name> | grep -A 5 Events
```

### 10.4 调试清单

| 检查项 | 命令 | 预期结果 |
|:---|:---|:---|
| Feature Gate | `kubectl get --raw /api/v1/nodes/<node>/proxy/configz` | `InPlacePodVerticalScaling: true` |
| 运行时版本 | `crictl version` | containerd >= 1.6.9 或 CRI-O >= 1.25 |
| resizePolicy | `kubectl get pod <pod> -o yaml` | 包含 cpu/memory 的 resizePolicy |
| cgroup 版本 | `stat -fc %T /sys/fs/cgroup/` | cgroup2fs (cgroup v2) |
| 调整状态 | `kubectl get pod <pod> -o jsonpath="{.status.resize}"` | 空 或 InProgress |

---

> **参考文档**
> - [KEP-1287: In-Place Update of Pod Resources](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/1287-in-place-update-pod-resources)
> - [Kubernetes Docs: Resize CPU and Memory Resources assigned to Containers](https://kubernetes.io/docs/tasks/configure-pod-container/resize-container-resources/)
> - [VPA 官方文档](https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler)
