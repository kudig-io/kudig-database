---
title: Pods
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- prometheus
- pdb
- statefulset
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pods 是什么
- 如何 Pods
trigger_keywords:
- Pods
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pods

## 概述
Pod 是 [[Kubernetes|Kubernetes]] 中最小的可部署计算单元，它是一组共享存储和网络资源、并协同运行的一个或多个容器的集合。Pod 中的容器始终被共位（co-located）和共调度（co-scheduled），在共享上下文中运行，相当于一个应用专属的"逻辑主机"。

## 核心概念/原理
- **单容器 Pod**：最常见的使用方式，Kubernetes 直接管理 Pod，而不是直接管理容器。
- **多容器 Pod**：当多个容器需要紧密耦合、共享资源时，可放入同一个 Pod。例如主应用容器 + Sidecar 容器。
- **共享资源**：Pod 内的容器共享网络命名空间（IP 地址、端口空间）和存储卷（Volumes），可通过 `localhost` 互相通信。
- **Pod 模板（PodTemplate）**：工作负载控制器（如 Deployment、Job）通过 Pod 模板来创建和管理 Pod。
- **Static Pods**：由 [[kubelet|kubelet]] 直接管理，不经过 API Server，常用于自托管控制平面。

## 关键机制或特性
- **Pod OS**：通过 `.spec.os.name` 指定 `linux` 或 `windows`。
- **Pod 更新与替换**：直接修改运行中 Pod 的字段有限制；工作负载控制器通常通过创建新 Pod 来应用模板更新。
- **Pod 子资源**：包括 `resize`（调整资源）、`ephemeralContainers`（临时容器）、`status`、`binding`。
- **Pod generation**：`metadata.generation` 在 spec 变更时递增，`status.observedGeneration` 用于跟踪状态同步。
- **容器探针（Probes）**：支持 `livenessProbe`、`readinessProbe`、`startupProbe`，由 kubelet 定期执行诊断。

## 使用场景
- 运行单一应用容器（最常见）。
- 需要多个紧密耦合容器协同工作的场景（如 Web 服务器 + 日志收集 Sidecar）。
- 需要共享网络或存储卷的微服务组件。

## 最佳实践/注意事项
- 通常情况下不要直接创建 Pod，而是通过 Deployment、StatefulSet、Job 等工作负载资源来管理。
- Pod 名称需符合 DNS 子域名规范，建议遵循更严格的 DNS Label 规则。
- 需要横向扩展时，应使用多个 Pod 副本，而不是在同一个 Pod 内运行多个相同容器。
- 注意 CPU limit 的权衡：可防止 noisy neighbor，但也可能在节点有空闲 CPU 时导致限流。

## 实战 YAML 示例

以下为一个生产级 Pod 配置，包含资源管理、探针、安全上下文和优雅终止等最佳实践：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: production-app
  namespace: prod
  labels:
    app: myapp
    version: v1.2.0
    environment: production
  annotations:
    prometheus.io/scrape: "true"      # Prometheus 自动发现
    prometheus.io/port: "8080"
spec:
  terminationGracePeriodSeconds: 60   # 优雅终止等待时间
  serviceAccountName: myapp-sa        # 使用专用 ServiceAccount
  securityContext:
    runAsNonRoot: true                 # 强制非 root 运行
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault            # 启用 seccomp 沙箱
  containers:
  - name: app
    image: myregistry.com/myapp:v1.2.0
    imagePullPolicy: IfNotPresent
    ports:
    - containerPort: 8080
      name: http
      protocol: TCP
    resources:
      requests:                        # 调度依据，必须设置
        cpu: "250m"
        memory: "256Mi"
      limits:                          # 防止资源逃逸
        cpu: "1000m"
        memory: "512Mi"
    startupProbe:                      # 保护慢启动应用
      httpGet:
        path: /healthz
        port: http
      initialDelaySeconds: 5
      periodSeconds: 5
      failureThreshold: 30            # 最长等待 150 秒启动
    livenessProbe:                     # 检测进程僵死
      httpGet:
        path: /healthz
        port: http
      periodSeconds: 15
      timeoutSeconds: 3
      failureThreshold: 3
    readinessProbe:                    # 控制流量切入
      httpGet:
        path: /ready
        port: http
      periodSeconds: 10
      timeoutSeconds: 3
      failureThreshold: 3
    env:
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name     # Downward API 注入 Pod 名称
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    volumeMounts:
    - name: config
      mountPath: /etc/app/config
      readOnly: true
    - name: tmp
      mountPath: /tmp
    securityContext:
      readOnlyRootFilesystem: true     # 只读根文件系统
      allowPrivilegeEscalation: false
      capabilities:
        drop: ["ALL"]
    lifecycle:
      preStop:                         # 优雅终止钩子
        exec:
          command: ["/bin/sh", "-c", "sleep 5 && kill -SIGTERM 1"]
  volumes:
  - name: config
    configMap:
      name: myapp-config
  - name: tmp
    emptyDir: {}
  topologySpreadConstraints:           # 跨可用区均匀分布
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
    labelSelector:
      matchLabels:
        app: myapp
```

## 故障排查

### Pod 持续 Pending
- **症状**: Pod 状态一直为 `Pending`，未被调度到任何节点。
- **常见原因**: 资源不足（CPU/内存 requests 超过集群可用量）、nodeSelector/affinity 无匹配节点、PVC 未绑定。
- **诊断命令**:
  ```bash
  # 查看 Pod 事件，定位调度失败原因
  kubectl describe pod <pod-name> -n <namespace> | grep -A 20 "Events"
  # 检查集群可用资源
  kubectl top nodes
  # 查看集群是否有 Unschedulable 节点
  kubectl get nodes -o wide | grep -i "SchedulingDisabled"
  ```
- **解决方案**: 扩容节点池、调整 requests、修正标签选择器或确保 PVC 已绑定。

### Pod 反复 CrashLoopBackOff
- **症状**: Pod 频繁重启，状态在 `CrashLoopBackOff` 和 `Running` 间切换。
- **常见原因**: 应用启动失败（配置错误、依赖服务不可达）、OOMKilled（内存不足）、Liveness Probe 配置不当。
- **诊断命令**:
  ```bash
  # 查看上一次容器退出日志
  kubectl logs <pod-name> --previous -n <namespace>
  # 查看 Pod 状态详情（退出码和原因）
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState}'
  # 检查是否 OOMKilled
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'
  ```
- **解决方案**: 修复应用配置、增大 memory limits、调整 Liveness Probe 的 `initialDelaySeconds` 和 `failureThreshold`。

### Pod 状态为 Running 但无法接收流量
- **症状**: Pod 运行中但 Service 不转发流量到该 Pod。
- **常见原因**: Readiness Probe 失败、Pod 标签与 Service selector 不匹配、Endpoint 未注册。
- **诊断命令**:
  ```bash
  # 检查 Pod 的 Ready 条件
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  # 查看 Endpoints 是否包含该 Pod
  kubectl get endpoints <service-name> -n <namespace>
  # 检查 Service selector 与 Pod labels 是否匹配
  kubectl get svc <service-name> -n <namespace> -o jsonpath='{.spec.selector}'
  ```

### ImagePullBackOff
- **症状**: Pod 无法拉取容器镜像。
- **常见原因**: 镜像名称/标签错误、私有仓库认证失败、网络不通。
- **诊断命令**:
  ```bash
  kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "Warning.*Failed"
  # 验证 imagePullSecrets 是否正确配置
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'
  ```

## 生产就绪检查清单

- [ ] `resources.requests` 和 `resources.limits` 已为所有容器配置
- [ ] `livenessProbe` 和 `readinessProbe` 已正确配置（Liveness 仅检测进程自身）
- [ ] `startupProbe` 已为慢启动应用配置（大模型加载、Java 应用等）
- [ ] `securityContext` 已设置: `runAsNonRoot`、`readOnlyRootFilesystem`、`capabilities.drop: ALL`
- [ ] `terminationGracePeriodSeconds` 已根据应用需求设置（默认 30 秒可能不够）
- [ ] `preStop` 生命周期钩子已配置，确保优雅终止
- [ ] Pod 通过 Deployment/StatefulSet 管理，而非裸 Pod
- [ ] `PodDisruptionBudget (PDB)` 已创建，保障滚动更新和节点维护时的可用性
- [ ] `topologySpreadConstraints` 或 `podAntiAffinity` 已配置，实现跨节点/可用区分布
- [ ] 日志输出到 stdout/stderr，便于集中采集
- [ ] Prometheus 监控注解已添加（如适用）
- [ ] `imagePullPolicy` 生产环境建议 `IfNotPresent`，搭配不可变镜像标签

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pod 列表及状态
kubectl get pods -n <namespace> -o wide

# 查看 Pod 详细信息和事件
kubectl describe pod <pod-name> -n <namespace>

# 查看 Pod 实时日志（-f 跟踪模式）
kubectl logs -f <pod-name> -c <container-name> -n <namespace>

# 进入 Pod 容器执行命令
kubectl exec -it <pod-name> -c <container-name> -n <namespace> -- /bin/sh

# 注入临时调试容器（适用于 distroless 镜像）
kubectl debug -it <pod-name> --image=busybox:latest --target=<container-name> -n <namespace>

# 查看 Pod 资源使用量
kubectl top pod <pod-name> -n <namespace> --containers

# 批量查看所有异常 Pod
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# 导出 Pod 完整 YAML（排查配置问题）
kubectl get pod <pod-name> -n <namespace> -o yaml
```
## 交叉引用

- [Pod 生命周期事件深度解析](../../domain-02-workloads-applications/11-pod-lifecycle-events.md)
- [高级 Pod 运维模式](../../domain-02-workloads-applications/12-advanced-pod-patterns.md)
- [工作负载概览与架构](../../domain-02-workloads-applications/01-workload-overview-architecture.md)
- [Pod 综合故障排查手册](../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md)
- [Pod 故障树分析 (FTA)](../../domain-10-troubleshooting-diagnostics/FTA故障树/list/pod-fta.md)
- [Pod Pending 诊断](../../domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis.md)
- [OOM 内存诊断](../../domain-10-troubleshooting-diagnostics/07-oom-memory-diagnosis.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/

## Related

- [[domain-19-landscape-references/领域索引/pod-index.md|Pod 知识图谱索引]]


<!-- risk-assessed -->
