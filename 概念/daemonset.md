---
title: DaemonSet
summary: DaemonSet 确保全部（或部分）节点上运行一个 Pod 的副本。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# DaemonSet

## 概述

DaemonSet 是一种确保**每个（或部分）节点上都恰好运行一个 Pod 副本**的工作负载控制器。当新节点加入集群时，DaemonSet 自动在其上拉起 Pod；当节点被移除时，对应的 Pod 也被回收。DaemonSet 非常适合运行节点级"常驻代理"类负载：日志采集器（Fluent Bit、Filebeat）、网络插件（Calico、Cilium）、监控 Agent（Node Exporter、Datadog Agent）、存储 Daemon（CSI plugin、Longhorn）以及安全 Agent（Falco、Tetragon）。

## 架构与工作原理

```
DaemonSet (apps/v1)
   │ spec.selector + template
   ▼
  ┌─────────── 节点集合 ───────────┐
  │ node-1   node-2   node-3  ... │  每个 Ready 节点 1 个 Pod
  │  ▼        ▼        ▼          │
  │ Pod-1    Pod-2    Pod-3       │  （受 nodeSelector/toleration 过滤）
  └────────────────────────────────┘
        │
        ▼  DaemonSet Controller 持续 watch Node/Pod
   - 新节点 Ready → 创建 Pod
   - 节点删除 → 回收 Pod
   - Pod 崩溃 → 重建（按 restartPolicy）
```

**调度机制**：
- 早期由 DaemonSet Controller 直接绑定到节点（绕过默认调度器）。
- 1.12+ 默认由**默认调度器**调度（带 `node-xxx` 的 nodeAffinity 与自动 toleration），与优先级、抢占、Taint 兼容更好。
- 通过 `nodeSelector` / `affinity.nodeAffinity` 限定目标节点子集；通过 `tolerations` 让 Pod 在被 Taint 的节点（如 master、专用 GPU 节点）上也能跑。

**更新策略（updateStrategy.type）**：
- `RollingUpdate`（默认）：按 `maxUnavailable`（默认 1）滚动更新，支持 `maxSurge`（1.25+）。
- `OnDelete`：手动删除 Pod 才会重建为新版本，需谨慎。

## 关键组件与特性

| 字段 | 作用 |
|------|------|
| `selector` | 匹配 Pod 模板标签（创建后不可变） |
| `template` | Pod 模板 |
| `updateStrategy` | RollingUpdate / OnDelete |
| `rollingUpdate.maxUnavailable` | 滚动期间允许不可用节点数/比例 |
| `rollingUpdate.maxSurge` | 滚动期间可超出 1 个的临时 Pod（1.25+） |
| `template.spec.nodeSelector` | 限定目标节点 |
| `template.spec.tolerations` | 容忍节点 Taint（如 master NoSchedule） |
| `revisionHistoryLimit` | 保留历史版本数 |

## 配置示例

```yaml
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
  labels:
    app: node-exporter
spec:
  selector:
    matchLabels: {app: node-exporter}
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 10%
  template:
    metadata:
      labels: {app: node-exporter}
    spec:
      hostNetwork: true              # 用主机网络，避免 kube-proxy 干扰
      hostPID: true
      serviceAccountName: node-exporter
      tolerations:                   # 关键：允许在所有节点（含 master）运行
      - {key: node-role.kubernetes.io/control-plane, effect: NoSchedule}
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.8.1
        ports:
        - {containerPort: 9100, hostPort: 9100, name: metrics}
        resources:
          requests: {cpu: 50m, memory: 64Mi}
          limits:   {cpu: 200m, memory: 128Mi}
        volumeMounts:
        - {name: proc,  mountPath: /host/proc,  readOnly: true}
        - {name: sys,   mountPath: /host/sys,   readOnly: true}
        - {name: root, mountPath: /host/root,  readOnly: true, mountPropagation: HostToContainer}
      volumes:
      - {name: proc,  hostPath: {path: /proc}}
      - {name: sys,   hostPath: {path: /sys}}
      - {name: root,  hostPath: {path: /}}
```

## 常用操作与命令

```bash
# 查看：期望副本数 = 节点数（过滤后）
kubectl get ds -n monitoring
kubectl describe ds node-exporter -n monitoring

# 节点上缺副本？
kubectl get nodes -o wide
kubectl get pods -n monitoring -o wide -l app=node-exporter

# 滚动更新（改镜像触发）
kubectl set image ds/node-exporter node-exporter=prom/node-exporter:v1.9.0 -n monitoring
kubectl rollout status ds/node-exporter -n monitoring
kubectl rollout history ds/node-exporter -n monitoring

# 回滚
kubectl rollout undo ds/node-exporter -n monitoring

# 只在 GPU 节点运行
kubectl patch ds node-exporter --type=json -p='[{"op":"add","path":"/spec/template/spec/nodeSelector","value":{"hardware/gpu":"nvidia"}}]'
```

## 最佳实践

1. **务必加 tolerations**：默认被 Taint 的 master / 专用节点会缺副本，导致监控/网络盲区。
2. **设资源 limits**：DaemonSet 在每个节点常驻，无 limit 可能吃光节点资源影响业务 Pod。
3. **hostNetwork/hostPort 谨慎**：提升性能与避免端口冲突，但要确保 hostPort 不与业务冲突。
4. **滚动用 maxUnavailable 百分比**：大规模集群（100+ 节点）设 10% 平衡更新速度与可用性。
5. **CI 校验每个节点 1 副本**：用 `kubectl get ds` 的 DESIRED == CURRENT == 节点数巡检，监控缺副本。
6. **hostPath 挂载只读**：日志/指标采集类挂载主机目录一律 `readOnly: true`，降低风险。

## 常见陷阱

- **master 节点缺副本**：忘记容忍 `node-role.kubernetes.io/control-plane:NoSchedule` Taint。
- **hostNetwork 端口冲突**：多 Agent 抢 9100/9090 等，导致 Pod CrashLoopBackOff。
- **资源吃满节点**：未设 limit 的 Agent 在大流量下 OOM 影响业务 Pod。
- **更新太慢**：默认 `maxUnavailable: 1`，500 节点要数小时，改百分比或 maxSurge 加速。
- **节点 NotReady 时 Pod 不重建**：DaemonSet Pod 受 NodeCondition 影响，1.27+ 用 `unhealthyPodEvictionPolicy` 控制。
- **与 HPA 冲突**：DaemonSet 副本数由节点数决定，不要套 HPA/VPA（除非用独立的 VPA）。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]]
- [[概念/node-taint.md|节点污点]] — 配合 tolerations
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
