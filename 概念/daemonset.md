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

## 源码实现分析

### DaemonSet Controller 对账逻辑

```go
// k8s.io/kubernetes/pkg/controller/daemon/daemon_controller.go
func (dsc *DaemonSetsController) syncDaemonSet(ctx context.Context, ds *apps.DaemonSet) error {
    // 1. 获取所有节点
    nodeList := dsc.nodeLister.List(labels.Everything())
    // 2. 过滤符合 nodeSelector/affinity 的节点
    targetNodes := filterTargetNodes(nodeList, ds.Spec.Template.Spec.NodeSelector)
    // 3. 获取每个目标节点上的 DaemonSet Pod
    nodeToDaemonPods := dsc.getDaemonPodsOnNodes(ds, targetNodes)
    // 4. 对每个目标节点检查是否需要创建 Pod
    for _, node := range targetNodes {
        pods := nodeToDaemonPods[node.Name]
        if len(pods) == 0 {
            // 5. 检查节点 Taint 是否被 Pod Toleration 容忍
            if !tolerationsTolerateTaints(ds.Spec.Template.Spec.Tolerations, node.Spec.Taints) {
                continue // 节点有未容忍的 Taint，跳过
            }
            // 6. 创建 DaemonSet Pod（带 nodeName 直接绑定）
            pod := dsc.createDaemonPod(ds, node)
            pod.Spec.NodeName = node.Name // 跳过调度器，直接绑定
            dsc.kubeClient.CoreV1().Pods(ns).Create(ctx, pod)
        }
    }
    // 7. 删除不再符合条件的节点上的 Pod
    for node, pods := range nodeToDaemonPods {
        if !isTargetNode(node, targetNodes) {
            dsc.deleteDaemonPods(pods)
        }
    }
    // 8. 更新 Status
    ds.Status.DesiredNumberScheduled = int32(len(targetNodes))
    ds.Status.CurrentNumberScheduled = countCurrent(nodeToDaemonPods)
    dsc.kubeClient.AppsV1().DaemonSets(ns).UpdateStatus(ctx, ds)
    return nil
}
```

### DaemonSet vs Deployment 调度差异

```
┌──────────────────────────────────────────────────────────┐
│          DaemonSet vs Deployment 调度差异              │
├──────────────────────────────────────────────────────────┤
│  Deployment:                                             │
│    Pod 创建 → 调度器选择节点 → 绑定节点              │
│    副本数由 replicas 字段控制                          │
│    支持 HPA 自动伸缩                                    │
│                                                          │
│  DaemonSet:                                              │
│    Controller 直接指定 nodeName → 跳过调度器          │
│    副本数 = 符合条件的节点数                          │
│    新节点加入 → 自动创建 Pod                          │
│    节点移除 → 自动删除 Pod                            │
│    不支持 HPA（副本数由节点数决定）                  │
│                                                          │
│  关键区别:                                              │
│  • DaemonSet Pod 的 nodeName 在创建时就确定          │
│  • DaemonSet 必须容忍节点 Taint 才能覆盖所有节点    │
│  • DaemonSet 滚动更新用 maxUnavailable 控制节奏      │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：监控 Agent 部署（覆盖所有节点）

```yaml
# 🟡 中风险：创建 DaemonSet 影响所有节点
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 10%  # 大规模集群用百分比
      maxSurge: 0
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      # 必须容忍所有常见 Taint，否则 master/专用节点缺副本
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      - key: node.kubernetes.io/not-ready
        effect: NoExecute
        tolerationSeconds: 300
      - key: dedicated
        operator: Exists  # 容忍所有专用节点 Taint
      hostNetwork: true  # 直接采集宿主机网络指标
      hostPID: true      # 采集宿主机进程信息
      containers:
      - name: exporter
        image: prom/node-exporter:v1.8.0
        args: ["--path.procfs=/host/proc", "--path.sysfs=/host/sys"]
        resources:
          requests: {cpu: 50m, memory: 64Mi}
          limits: {cpu: 200m, memory: 128Mi}  # 必须设 limit！
        ports:
        - containerPort: 9100
          hostPort: 9100  # 注意端口冲突
        volumeMounts:
        - {name: proc, mountPath: /host/proc, readOnly: true}
        - {name: sys, mountPath: /host/sys, readOnly: true}
      volumes:
      - {name: proc, hostPath: {path: /proc}}
      - {name: sys, hostPath: {path: /sys}}
```

### 场景二：检查 DaemonSet 覆盖情况

```bash
# 🟢 低风险：只读检查
# 检查 DESIRED == CURRENT == READY
kubectl get ds -n monitoring
# 输出示例:
# NAME            DESIRED  CURRENT  READY  UP-TO-DATE  AVAILABLE
# node-exporter   12       12       12     12          12
# 找出缺副本的节点
kubectl get nodes -o name | while read node; do
  pod=$(kubectl get pods -n monitoring -l app=node-exporter -o wide --field-selector spec.nodeName=${node#node/} --no-headers)
  [ -z "$pod" ] && echo "MISSING: ${node#node/}"
done
# 检查为什么某节点缺副本
kubectl describe node <node> | grep -A5 Taints
kubectl get events -n monitoring --field-selector reason=FailedCreate
```

### 场景三：滚动更新 DaemonSet

```bash
# 🟡 中风险：更新影响所有节点
# 更新镜像
kubectl set image ds/node-exporter exporter=prom/node-exporter:v1.9.0 -n monitoring
# 监控滚动进度
kubectl rollout status ds/node-exporter -n monitoring
# 查看更新历史
kubectl rollout history ds/node-exporter -n monitoring
# 异常时回滚
kubectl rollout undo ds/node-exporter -n monitoring  # 🟡 回滚
# 大规模集群加速更新（临时调大 maxUnavailable）
kubectl patch ds/node-exporter -n monitoring --type=merge \
  -p '{"spec":{"updateStrategy":{"rollingUpdate":{"maxUnavailable":"20%"}}}}'
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | DaemonSet 不需要 tolerations | 默认 master/专用节点有 Taint；不容忍则这些节点缺副本（监控盲区） |
| 2 | DaemonSet 可以用 HPA | 副本数由节点数决定，HPA 无效；资源调整用 VPA 或手动 |
| 3 | 不设 limits 也没关系 | DaemonSet 每节点常驻，无 limit 可能吃光节点资源影响业务 Pod |
| 4 | hostPort 不会冲突 | 多 DaemonSet 用同一 hostPort 会 CrashLoopBackOff；规划端口分配 |
| 5 | 默认滚动更新很快 | 默认 maxUnavailable=1，500 节点要数小时；大规模用百分比 |
| 6 | 节点 NotReady 时 Pod 会重建 | DaemonSet Pod 受 NodeCondition 影响；1.27+ 用 unhealthyPodEvictionPolicy 控制 |

## 面试要点

1. **Q: DaemonSet 与 Deployment 的核心区别是什么？**
   A: ① 副本数：Deployment 由 replicas 字段控制；DaemonSet 由符合条件的节点数决定。② 调度：Deployment Pod 经过调度器选择节点；DaemonSet Controller 直接指定 nodeName（跳过调度器）。③ 节点变化：新节点加入 DaemonSet 自动创建 Pod；节点移除自动删除。④ 伸缩：Deployment 支持 HPA；DaemonSet 不支持（副本数=节点数）。⑤ 用途：Deployment 用于无状态服务；DaemonSet 用于每节点一个的 agent（监控/日志/网络）。

2. **Q: 为什么 DaemonSet 必须配置 tolerations？**
   A: 因为控制面节点默认有 node-role.kubernetes.io/control-plane:NoSchedule Taint，专用节点可能有自定义 Taint。不容忍则这些节点上不会创建 DaemonSet Pod，导致监控/日志/网络 agent 覆盖不全（盲区）。生产 DaemonSet 应容忍：control-plane NoSchedule + not-ready NoExecute(300s) + unreachable NoExecute(300s) + 自定义专用节点 Taint。

3. **Q: DaemonSet 的滚动更新如何工作？**
   A: ① 更新 Pod Template（如镜像版本）触发滚动；② 按 maxUnavailable 控制同时更新的节点数（默认 1，大规模用 10-20%）；③ 对每个节点：删除旧 Pod → 创建新 Pod → 等待 Ready；④ maxSurge 控制是否先建新再删旧（默认 0，即先删后建）；⑤ 更新期间部分节点无 agent（监控短暂盲区）。加速：临时调大 maxUnavailable 百分比。

4. **Q: 生产环境 DaemonSet 有哪些关键设计考量？**
   A: ① 资源限制：必须设 requests/limits（每节点常驻，无 limit 可能 OOM 影响业务）；② 端口规划：hostPort 避免冲突（监控 9100、日志 5044、网络 4789）；③ 只读挂载：hostPath 一律 readOnly: true；④ 优先级：设高 priorityClassName 确保资源紧张时不被驱逐；⑤ 健康检查：livenessProbe 自动重启卡死的 agent；⑥ 更新策略：大规模集群用百分比 maxUnavailable 平衡速度与可用性。

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]]
- [[概念/deployments.md|Deployment]]
- [[概念/node-taint.md|节点污点]] — 配合 tolerations
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
