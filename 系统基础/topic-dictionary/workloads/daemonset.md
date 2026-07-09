---
title: DaemonSet
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- cilium
- flannel
- calico
- docker
- opa
- pdb
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DaemonSet 是什么
- 如何 DaemonSet
trigger_keywords:
- DaemonSet
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
- cilium-basics
- cni-basics
- policy-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DaemonSet

## 概述
DaemonSet 确保所有（或部分）节点上都运行一个 Pod 副本。当节点加入集群时，Pod 会被自动创建；当节点从集群移除时，Pod 会被垃圾回收。删除 DaemonSet 会清理其创建的所有 Pod。

## 核心概念/原理
- **节点级服务**：DaemonSet 用于提供节点本地设施，类似于传统 Unix 服务器上的系统守护进程。
- **Pod 模板**：与 Deployment 类似，`spec.template` 是必需的，且 `restartPolicy` 必须为 `Always`（或未指定，默认即 Always）。
- **选择器**：`spec.selector` 用于匹配 Pod 标签，创建后不可变。
- **节点筛选**：可通过 `nodeSelector` 或 `nodeAffinity` 限制 DaemonSet 仅在符合条件的节点上创建 Pod。

## 关键机制或特性
- **调度方式**：DaemonSet 控制器会为每个目标节点设置 `spec.affinity.nodeAffinity`，将 Pod 绑定到特定节点。默认调度器随后会处理实际的节点绑定，必要时可基于 Pod 优先级抢占现有 Pod。
- **自动容忍（Tolerations）**：DaemonSet 控制器会自动为 Pod 添加一组容忍，使其能在不健康的节点上运行：
  - `node.[[Kubernetes|kubernetes]].io/not-ready`（NoExecute）
  - `node.kubernetes.io/unreachable`（NoExecute）
  - `node.kubernetes.io/disk-pressure`（NoSchedule）
  - `node.kubernetes.io/memory-pressure`（NoSchedule）
  - `node.kubernetes.io/pid-pressure`（NoSchedule）
  - `node.kubernetes.io/unschedulable`（NoSchedule）
  - `node.kubernetes.io/network-unavailable`（NoSchedule，仅对 `hostNetwork: true` 的 Pod）
- **更新策略**：支持滚动更新（RollingUpdate），可配置 `maxUnavailable` 和 `maxSurge`。
- **高优先级**：建议为关键 DaemonSet 设置较高的 PriorityClass，以确保在资源竞争时能成功调度。

## 使用场景
- 集群网络插件（如 Calico、Flannel、[[Cilium|Cilium]]）。
- 节点监控代理（如 [[Prometheus|Prometheus]] Node Exporter）。
- 日志收集代理（如 [[fluentd|[[Fluentd]]]]、Fluent Bit）。
- 存储驱动或设备插件（如 CSI 节点插件）。

## 最佳实践/注意事项
- 如果 DaemonSet 提供集群网络等关键功能，请确保其具有足够的优先级和 tolerations，以避免与节点就绪状态形成死锁。
- 可通过 `hostPort`、DNS 或 Service 等方式与 DaemonSet Pod 通信。
- 修改 DaemonSet 的 Pod 后，控制器下次在节点上创建 Pod 时仍会使用原始模板；某些字段不支持原地更新。
- 删除 DaemonSet 时使用 `--cascade=orphan` 可保留节点上的 Pod；后续创建相同选择器的 DaemonSet 会收养这些 Pod。

## 实战 YAML 示例

以下为生产级 Prometheus Node Exporter DaemonSet 配置：

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
  labels:
    app: node-exporter
spec:
  selector:
    matchLabels:
      app: node-exporter
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1                    # 每次最多 1 个节点不可用
  template:
    metadata:
      labels:
        app: node-exporter
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9100"
    spec:
      hostNetwork: true                     # 使用主机网络，暴露节点指标
      hostPID: true                         # 访问主机进程信息
      priorityClassName: system-node-critical  # 确保节点级别高优先调度
      tolerations:
      - operator: Exists                    # 容忍所有 taint，确保每个节点都运行
      serviceAccountName: node-exporter
      securityContext:
        runAsNonRoot: true
        runAsUser: 65534                    # nobody 用户
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.8.0
        args:
        - '--path.procfs=/host/proc'
        - '--path.sysfs=/host/sys'
        - '--path.rootfs=/host/root'
        - '--collector.filesystem.mount-points-exclude=^/(dev|proc|sys|var/lib/docker/.+)($|/)'
        ports:
        - containerPort: 9100
          hostPort: 9100
          name: metrics
        resources:
          requests:
            cpu: "50m"
            memory: "64Mi"
          limits:
            cpu: "200m"
            memory: "128Mi"
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: sys
          mountPath: /host/sys
          readOnly: true
        - name: root
          mountPath: /host/root
          mountPropagation: HostToContainer
          readOnly: true
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
      - name: root
        hostPath:
          path: /
```

## 故障排查

### DaemonSet Pod 未在某些节点上调度
- **症状**: `kubectl get pods -o wide` 显示部分节点上没有 DaemonSet Pod。
- **常见原因**: 目标节点有 taint 但 DaemonSet 未配置对应的 toleration；nodeSelector/nodeAffinity 不匹配。
- **诊断命令**:
  ```bash
  # 查看 DaemonSet 的调度状态
  kubectl get daemonset node-exporter -n monitoring
  # desiredNumberScheduled vs currentNumberScheduled vs numberMisscheduled
  
  # 查看缺失 Pod 的节点 taint
  kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints'
  
  # 查看 DaemonSet 的 toleration 配置
  kubectl get daemonset node-exporter -n monitoring -o jsonpath='{.spec.template.spec.tolerations}'
  ```
- **解决方案**: 添加对应的 `tolerations`，或使用 `operator: Exists` 容忍所有 taint。

### DaemonSet 滚动更新卡住
- **症状**: `kubectl rollout status ds/node-exporter -n monitoring` 长时间无进展。
- **常见原因**: 新版本 Pod 启动失败（镜像错误、资源不足）；PDB 限制了同时不可用的 Pod 数量。
- **诊断命令**:
  ```bash
  # 查看更新状态
  kubectl rollout status ds/node-exporter -n monitoring
  # 查看各节点上 Pod 状态
  kubectl get pods -n monitoring -l app=node-exporter -o wide
  # 查看失败 Pod 的事件
  kubectl describe pod <failing-pod> -n monitoring | tail -20
  ```

### DaemonSet Pod 占用过多节点资源
- **症状**: 节点资源告警，DaemonSet Pod 消耗超预期。
- **诊断命令**:
  ```bash
  # 按节点查看 DaemonSet Pod 资源使用
  kubectl top pods -n monitoring -l app=node-exporter --sort-by=memory
  ```
- **解决方案**: 为 DaemonSet Pod 设置合理的 `resources.limits`，检查是否存在配置导致的资源泄漏。

## 生产检查清单

- [ ] `updateStrategy.type: RollingUpdate` 已配置，`maxUnavailable` 设置合理
- [ ] 关键 DaemonSet 使用 `system-node-critical` 或 `system-cluster-critical` PriorityClass
- [ ] `tolerations` 配置覆盖所有目标节点的 taint
- [ ] `resources.requests/limits` 已为所有容器配置
- [ ] `hostNetwork`/`hostPID`/`hostIPC` 仅在必要时启用
- [ ] SecurityContext 已加固（`runAsNonRoot`、`readOnlyRootFilesystem`）
- [ ] 配合 `PodDisruptionBudget` 保障节点维护时的可用性
- [ ] 监控 DaemonSet 的 `desiredNumberScheduled` 和 `numberReady` 指标

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 DaemonSet 状态（含 desired/current/ready 数量）
kubectl get daemonset -n monitoring

# 查看 DaemonSet 滚动更新状态
kubectl rollout status ds/node-exporter -n monitoring

# 回滚 DaemonSet
kubectl rollout undo ds/node-exporter -n monitoring

# 查看各节点上的 DaemonSet Pod
kubectl get pods -n monitoring -l app=node-exporter -o wide

# 查看 DaemonSet 事件
kubectl describe ds node-exporter -n monitoring | tail -20
```
## 交叉引用

- [DaemonSet 管理详解](../../工作负载/04-daemonset-management.md)
- [工作负载概览与架构](../../工作负载/01-workload-overview-architecture.md)
- [DaemonSet 故障树分析 (FTA)](../../故障诊断/topic-fta/list/daemonset-fta.md)
- [节点 NotReady 诊断](../../故障诊断/06-node-notready-diagnosis.md)
- [工作负载监控与告警](../../工作负载/06-workload-monitoring-alerting.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/daemonset/

## Related

- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
