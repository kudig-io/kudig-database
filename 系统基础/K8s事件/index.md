---
title: Kubernetes 事件知识体系
description: Kubernetes 事件系统完整知识体系，覆盖事件架构、Pod 生命周期、镜像拉取、探针、调度、节点、工作负载、网络、存储、自动扩缩、安全、GC 等 15 个子领域
summary: K8s 事件知识体系总索引，覆盖 15 个子领域、200+ 事件类型、完整排障流程、监控集成
category: index
tags:
- index
- kubernetes
- events
- troubleshooting
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 开发工程师
---

# Kubernetes 事件知识体系

> 本知识体系覆盖 Kubernetes 事件系统的全域知识，包括事件架构、各资源类型事件详解、故障排查流程、监控集成，是 SRE 和平台工程师排查集群问题的权威参考。

## 领域概述

Kubernetes 事件是集群状态变化的实时记录，是故障排查的第一手证据：

- **事件架构**：Event API、事件生命周期、存储与过期
- **Pod 事件**：创建、调度、启动、运行、终止
- **镜像事件**：拉取、失败、认证、超时
- **探针事件**：Liveness、Readiness、Startup 失败
- **调度事件**：调度成功/失败、抢占、污点
- **节点事件**：Ready/NotReady、资源压力、污点
- **工作负载事件**：Deployment、StatefulSet、Job 状态变化
- **网络事件**：Service、Ingress、DNS 异常
- **存储事件**：PV/PVC 绑定、挂载失败
- **扩缩容事件**：HPA/VPA 触发、扩缩失败
- **安全事件**：准入拒绝、RBAC 失败
- **GC 事件**：资源回收、Finalizer 阻塞

## 文档索引

### 事件架构与基础

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/K8s事件/01-event-system-architecture.md|事件系统架构]] | Event API、存储、过期、监控 | 626 |

### 资源生命周期事件

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/K8s事件/02-pod-container-lifecycle-events.md|Pod/容器生命周期]] | 创建、调度、启动、运行、终止 | 2582 |
| [[系统基础/K8s事件/03-image-pull-events.md|镜像拉取事件]] | Pulling、Pulled、Failed、认证 | 2544 |
| [[系统基础/K8s事件/04-probe-health-check-events.md|探针健康检查]] | Liveness、Readiness、Startup | 1545 |
| [[系统基础/K8s事件/05-scheduling-preemption-events.md|调度与抢占]] | Scheduled、FailedScheduling、Preempting | 1021 |

### 节点与工作负载事件

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/K8s事件/06-node-lifecycle-condition-events.md|节点生命周期]] | Ready、NotReady、资源压力 | 2631 |
| [[系统基础/K8s事件/07-deployment-replicaset-events.md|Deployment/ReplicaSet]] | 滚动更新、扩缩、回滚 | 2446 |
| [[系统基础/K8s事件/08-statefulset-daemonset-events.md|StatefulSet/DaemonSet]] | 有序部署、节点守护 | 2488 |
| [[系统基础/K8s事件/09-job-cronjob-batch-events.md|Job/CronJob]] | 批量任务、定时任务、失败 | 2525 |

### 基础设施事件

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/K8s事件/10-service-networking-events.md|Service/网络事件]] | Endpoint、Ingress、DNS | 2353 |
| [[系统基础/K8s事件/11-storage-volume-events.md|存储/卷事件]] | PV/PVC、挂载、快照 | 2391 |
| [[系统基础/K8s事件/12-autoscaling-events.md|自动扩缩容事件]] | HPA、VPA、KEDA | 2511 |

### 安全与治理事件

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/K8s事件/13-security-admission-rbac-events.md|安全/准入/RBAC]] | 准入拒绝、权限失败 | 1871 |
| [[系统基础/K8s事件/14-namespace-resource-gc-events.md|Namespace/资源/GC]] | 删除、回收、Finalizer | 2897 |
| [[系统基础/K8s事件/15-ecosystem-addon-events.md|生态/插件事件]] | Helm、Operator、CNI | 1456 |

## 常见事件快速参考

### Pod 事件

| 事件 | 类型 | 含义 | 排查方向 |
|------|------|------|----------|
| Scheduled | Normal | Pod 已调度到节点 | - |
| FailedScheduling | Warning | 调度失败 | 资源不足/污点/亲和性 |
| Pulling | Normal | 开始拉取镜像 | - |
| Pulled | Normal | 镜像拉取成功 | - |
| Failed | Warning | 镜像拉取失败 | 网络/认证/不存在 |
| Created | Normal | 容器已创建 | - |
| Started | Normal | 容器已启动 | - |
| BackOff | Warning | 容器反复崩溃 | 应用错误/配置错误 |
| OOMKilling | Warning | 内存超限被杀 | 增加 memory limit |
| Killing | Normal | 容器正在终止 | - |
| Unhealthy | Warning | 探针失败 | 检查探针配置/应用状态 |
| Evicted | Warning | Pod 被驱逐 | 节点资源压力 |
| Preempting | Normal | 正在抢占资源 | - |
| FailedMount | Warning | 卷挂载失败 | 检查 PV/StorageClass |
| FailedAttachVolume | Warning | 卷附加失败 | 检查 CSI 驱动 |

### 节点事件

| 事件 | 类型 | 含义 | 排查方向 |
|------|------|------|----------|
| NodeReady | Normal | 节点就绪 | - |
| NodeNotReady | Warning | 节点未就绪 | kubelet/网络 |
| NodeHasDiskPressure | Warning | 磁盘压力 | 清理磁盘 |
| NodeHasMemoryPressure | Warning | 内存压力 | 检查内存使用 |
| NodeHasPIDPressure | Warning | PID 压力 | 检查进程数 |
| NodeNotSchedulable | Normal | 节点不可调度 | 检查 cordon/taint |
| RegisteredNode | Normal | 节点注册 | - |
| RemovingNode | Normal | 节点移除 | - |

### Deployment 事件

| 事件 | 类型 | 含义 | 排查方向 |
|------|------|------|----------|
| ScalingReplicaSet | Normal | 扩缩 ReplicaSet | - |
| SuccessfulCreate | Normal | 创建 Pod 成功 | - |
| FailedCreate | Warning | 创建 Pod 失败 | 资源/配额 |
| SuccessfulDelete | Normal | 删除 Pod 成功 | - |
| DeploymentRollingUpdate | Normal | 滚动更新中 | - |
| DeploymentCompleted | Normal | 部署完成 | - |
| DeploymentFailed | Warning | 部署失败 | 检查 Pod 状态 |

## 事件查询命令

```bash
# 查看集群事件（按时间排序）
kubectl get events -A --sort-by=.metadata.creationTimestamp

# 查看特定命名空间事件
kubectl get events -n production --sort-by=.lastTimestamp

# 查看特定 Pod 事件
kubectl describe pod <pod-name> -n <ns>
kubectl get events -n <ns> --field-selector involvedObject.name=<pod-name>

# 查看 Warning 事件
kubectl get events -A --field-selector type=Warning

# 查看特定类型事件
kubectl get events -A --field-selector reason=FailedScheduling
kubectl get events -A --field-selector reason=OOMKilling
kubectl get events -A --field-selector reason=FailedMount

# 查看节点事件
kubectl describe node <node-name>
kubectl get events -A --field-selector involvedObject.kind=Node

# 事件统计
kubectl get events -A -o json | jq '.items[].reason' | sort | uniq -c | sort -rn

# 最近 1 小时事件
kubectl get events -A --field-selector 'lastTimestamp>2026-07-02T10:00:00Z'
```

## 故障排查流程

### Pod 启动失败排查

```
1. 查看 Pod 事件
   kubectl describe pod <name> -n <ns>
   → 关注 Events 部分的 Warning 事件

2. 根据事件类型分支：
   - FailedScheduling → 检查资源/污点/亲和性
   - Failed (image pull) → 检查镜像名/认证/网络
   - BackOff → 查看日志 kubectl logs --previous
   - Unhealthy → 检查探针配置/应用启动时间
   - FailedMount → 检查 PV/PVC/StorageClass
   - OOMKilling → 增加 memory limit

3. 查看容器日志
   kubectl logs <pod> -n <ns> --previous

4. 检查节点状态
   kubectl describe node <node-name>
```

### 节点 NotReady 排查

```
1. 查看节点事件
   kubectl describe node <node-name>

2. 检查 kubelet 状态
   systemctl status kubelet
   journalctl -u kubelet --since="10 min ago"

3. 检查节点条件
   kubectl get node <name> -o jsonpath='{.status.conditions}'

4. 常见原因：
   - kubelet 崩溃 → 重启 kubelet
   - 证书过期 → 检查/轮换证书
   - 网络断开 → 检查 CNI/网络
   - 磁盘满 → 清理磁盘/镜像
```

## 事件监控集成

### Prometheus 事件导出

```yaml
# kube-eventer 或 event-exporter 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: event-exporter
  namespace: monitoring
spec:
  template:
    spec:
      containers:
        - name: event-exporter
          image: registry.k8s.io/event-exporter:v0.1.0
          args:
            - --sink=prometheus://metrics:9102
```

### 告警规则示例

```yaml
# PrometheusRule: Pod 频繁重启
- alert: PodCrashLooping
  expr: increase(kube_pod_container_status_restarts_total[1h]) > 3
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"

# PrometheusRule: 调度失败
- alert: PodSchedulingFailed
  expr: kube_pod_status_unschedulable > 0
  for: 10m
  labels:
    severity: warning
  annotations:
    summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 调度失败"

# PrometheusRule: 节点 NotReady
- alert: NodeNotReady
  expr: kube_node_status_condition{condition="Ready",status="true"} == 0
  for: 5m
  labels:
    severity: critical
  annotations:
    summary: "节点 {{ $labels.node }} NotReady"
```

## 事件生命周期

```
事件产生 → 存储到 etcd → API Server 提供查询 → 默认 1h 后过期删除
     │              │              │                    │
  组件上报     Event 对象    kubectl get events    kube-apiserver
  (kubelet/   (v1.Event)   --sort-by=.lastTimestamp  --event-ttl=1h
  scheduler/                                      (可配置)
  controller)
```

**关键参数**：
- `--event-ttl`: 事件保留时间（默认 1h）
- 事件不会持久化，重启后丢失
- 生产环境建议部署事件导出器（event-exporter/kube-eventer）

## 事件字段详解

```yaml
# Event 对象核心字段
apiVersion: v1
kind: Event
metadata:
  name: myapp-16d9f8b7c5-x2k4j.17a2b3c4d5e6f7a8
  namespace: production
involvedObject:          # 关联对象
  kind: Pod
  name: myapp-16d9f8b7c5-x2k4j
  namespace: production
  uid: 12345678-1234-1234-1234-123456789012
reason: BackOff          # 事件原因（简短标识）
message: "Back-off restarting failed container"  # 详细描述
source:                  # 事件来源
  component: kubelet
  host: worker-node-1
type: Warning            # Normal 或 Warning
count: 5                 # 发生次数
firstTimestamp: "2026-07-02T10:00:00Z"
lastTimestamp: "2026-07-02T10:05:00Z"
```

## 事件类型分类

### 按严重程度

| 类型 | 含义 | 处理优先级 |
|------|------|----------|
| Normal | 正常状态变化 | 无需处理 |
| Warning | 异常/需关注 | 及时排查 |

### 按来源组件

| 组件 | 产生事件 | 典型场景 |
|------|----------|----------|
| kubelet | Pod 生命周期、探针、镜像 | 容器启动失败 |
| kube-scheduler | 调度决策 | 调度失败 |
| kube-controller-manager | 控制器操作 | 扩缩容、GC |
| deployment-controller | Deployment 状态 | 滚动更新 |
| statefulset-controller | StatefulSet 状态 | 有序部署 |
| job-controller | Job 状态 | 任务完成/失败 |
| hpa-controller | 自动扩缩 | 扩缩触发 |
| node-controller | 节点状态 | NotReady |
| endpoint-controller | Endpoint 更新 | 服务发现 |
| pv-controller | 存储卷操作 | 绑定/释放 |
| attachdetach-controller | 卷附加/分离 | 挂载失败 |
| namespace-controller | Namespace 操作 | 删除/GC |
| garbage-collector | 资源回收 | 级联删除 |
| admission-webhook | 准入拒绝 | 策略拦截 |

## 生产事件分析模式

### 事件聚合分析

```bash
# 按 reason 统计事件数量
kubectl get events -A -o json | \
  jq -r '.items[] | "\(.reason)\t\(.type)"' | \
  sort | uniq -c | sort -rn | head -20

# 查看特定节点的 Warning 事件
kubectl get events -A --field-selector type=Warning,source.host=worker-1

# 查看最近 30 分钟的调度失败
kubectl get events -A --field-selector reason=FailedScheduling \
  -o custom-columns='TIME:.lastTimestamp,NS:.metadata.namespace,MSG:.message'

# 查看 OOM 事件
kubectl get events -A --field-selector reason=OOMKilling \
  -o custom-columns='TIME:.lastTimestamp,POD:.involvedObject.name,NS:.metadata.namespace'
```

### 事件关联分析

```
故障时间线重建：
1. 获取故障时间点前后所有事件
   kubectl get events -A --sort-by=.lastTimestamp | grep "10:0[0-9]"

2. 按 involvedObject 分组
   kubectl get events -A -o json | jq '.items[] | {time:.lastTimestamp, obj:.involvedObject.name, reason:.reason, msg:.message}'

3. 关联日志
   kubectl logs <pod> --since=1h | grep -i error

4. 关联监控
   查看 Prometheus/Grafana 对应时间点的指标变化
```

## 事件与告警映射

| 事件 | 告警规则 | 严重级 | 响应时间 |
|------|----------|--------|----------|
| OOMKilling | PodOOMKilled | P2 | 15min |
| FailedScheduling | PodUnschedulable | P2 | 30min |
| NodeNotReady | NodeDown | P1 | 5min |
| BackOff (多次) | PodCrashLooping | P2 | 15min |
| FailedMount | VolumeMountFailed | P2 | 15min |
| Evicted | PodEvicted | P3 | 1h |
| FailedCreate | PodCreateFailed | P2 | 15min |
| Unhealthy (Readiness) | PodNotReady | P3 | 30min |
| Unhealthy (Liveness) | PodUnhealthy | P2 | 15min |
| NodeHasDiskPressure | NodeDiskPressure | P2 | 15min |
| NodeHasMemoryPressure | NodeMemoryPressure | P2 | 15min |
| FailedAttachVolume | VolumeAttachFailed | P2 | 15min |
| FailedSync | DeploymentSyncFailed | P2 | 15min |
| FailedKillPod | PodTerminationFailed | P3 | 30min |
| ContainerGCFailed | ContainerGCFailed | P3 | 1h |
| ImageGCFailed | ImageGCFailed | P3 | 1h |
| InvalidDiskCapacity | NodeDiskCapacityInvalid | P2 | 15min |
| Rebooted | NodeRebooted | P1 | 5min |
| HostPortConflict | PortConflict | P2 | 15min |
| FailedToRetrieveImagePullSecret | ImagePullSecretMissing | P2 | 15min |

## 常见事件根因速查

| 事件 | 常见根因 | 快速修复 |
|------|----------|----------|
| FailedScheduling | CPU/内存不足 | 扩容节点/减少 request |
| FailedScheduling | 污点未容忍 | 添加 toleration |
| FailedScheduling | PVC 未绑定 | 检查 StorageClass |
| Failed (image) | 镜像不存在 | 检查镜像名/tag |
| Failed (image) | 认证失败 | 检查 imagePullSecrets |
| Failed (image) | 网络超时 | 检查网络/镜像仓库 |
| BackOff | 应用崩溃 | 查看日志修复应用 |
| BackOff | 配置错误 | 检查 ConfigMap/Secret |
| OOMKilling | 内存不足 | 增加 memory limit |
| Unhealthy | 探针端口错误 | 检查探针配置 |
| Unhealthy | 启动太慢 | 增加 initialDelaySeconds |
| FailedMount | PV 不可用 | 检查 PV/CSI 驱动 |
| Evicted | 节点资源压力 | 清理节点/扩容 |
| NodeNotReady | kubelet 崩溃 | 重启 kubelet |
| NodeNotReady | 证书过期 | 轮换证书 |

## 检查清单

### 事件监控就绪检查

- [ ] 事件导出器已部署（event-exporter/kube-eventer）
- [ ] 事件已接入 Prometheus/Grafana
- [ ] 关键事件告警规则已配置
- [ ] 事件保留时间已调整（生产建议 24h）
- [ ] 事件日志已持久化（Loki/ES）
- [ ] 告警通知渠道已配置（Slack/钉钉/邮件）
- [ ] 定期审查 Warning 事件趋势
- [ ] 事件关联分析流程已建立

## 学习路径

```
入门: 事件系统架构 → Pod 生命周期事件 → 镜像拉取事件
中级: 探针事件 → 调度事件 → 节点事件 → 工作负载事件
高级: 网络/存储事件 → 自动扩缩 → 安全事件 → GC 事件
专家: 事件监控集成 → 自定义事件 → 事件分析平台
```

## 参考链接

- https://kubernetes.io/docs/reference/node/node-status/
- https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/event-v1/
- https://kubernetes.io/docs/tasks/debug/debug-application/
- https://kubernetes.io/docs/concepts/overview/components/
- https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/

## Related

- [[系统基础/知识字典/index.md|知识字典总索引]]
- [[系统基础/速查卡/k8s.md|K8s 速查卡]]
- [[系统基础/速查卡/kubectl-scene-cheatsheet.md|kubectl 场景速查]]

