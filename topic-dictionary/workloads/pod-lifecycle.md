---
title: Pod Lifecycle
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod Lifecycle 是什么
- 如何 Pod Lifecycle
trigger_keywords:
- Pod
- Lifecycle
- dictionary
title_en: Pods
---


# Pod Lifecycle

## 概述
Pod 遵循一个确定的生命周期，从 `Pending` 阶段开始，如果至少一个主容器正常启动则进入 `Running`，最终根据容器终止情况进入 `Succeeded` 或 `Failed` 阶段。Pod 被视为相对短暂（ephemeral）的实体。

## 核心概念/原理
- **Pod Phase（阶段）**：`Pending` → `Running` → `Succeeded`/`Failed`/`Unknown`。
  - `Pending`：已被集群接受，但容器尚未全部就绪（包括调度、镜像拉取时间）。
  - `Running`：已绑定到节点，至少有一个容器仍在运行。
  - `Succeeded`：所有容器成功终止且不会重启。
  - `Failed`：所有容器终止，且至少有一个失败退出。
  - `Unknown`：无法获取 Pod 状态（通常是与节点通信失败）。
- **容器状态**：`Waiting`、`Running`、`Terminated`。
- **Restart Policy**：
  - `Always`（默认）：任何终止都重启。
  - `OnFailure`：仅在非零退出码时重启。
  - `Never`：不自动重启。
- **CrashLoopBackOff**：容器反复崩溃时，kubelet 会应用指数退避延迟（10s、20s、40s…，上限 300s）。

## 关键机制或特性
- **Pod Conditions**：包括 `PodScheduled`、`Initialized`、`ContainersReady`、`Ready`、`PodReadyToStartContainers`、`DisruptionTarget`、`PodResizePending`、`PodResizeInProgress`。
- **Readiness Gates**：允许应用向 PodStatus 注入额外的就绪条件，Pod 只有在所有自定义条件为 `True` 时才被视为 `Ready`。
- **Pod 终止流程**：
  1. 设置 `deletionTimestamp` 和优雅期（默认 30s）。
  2. 执行 `preStop` Hook。
  3. 发送 TERM（SIGTERM）信号。
  4. 控制平面将终止中的 Pod 从 EndpointSlice 中移除（`ready=false`）。
  5. 优雅期过后发送 KILL（SIGKILL）信号，强制清理。
- **强制终止**：`--grace-period=0 --force` 可立即从 API Server 删除 Pod。
- **Sidecar 容器终止顺序**：Sidecar 容器在主容器完全终止后才接收 TERM 信号，并按定义顺序的反向终止。
- **容器级 Restart Policy（Beta）**：`ContainerRestartRules` 特性门控启用后，可为单个容器指定 `restartPolicy` 和 `restartPolicyRules`。
- **Pod 原地重启（Alpha）**：`RestartAllContainersOnContainerExits` 允许通过规则触发整个 Pod 的原地重启（保留 UID、IP、Volume）。

## 使用场景
- 需要理解 Pod 健康状态和故障排查（如 `CrashLoopBackOff`）。
- 配置优雅关闭流程，确保应用在删除 Pod 时有时间处理未完成请求。
- 使用 Sidecar 容器时，需理解其特殊的启动和终止顺序。

## 最佳实践/注意事项
- 区分 `Status`（kubectl 显示字段）与 `phase`（API 数据模型）。
- 为需要长时间关闭的应用设置足够的 `terminationGracePeriodSeconds`。
- 如果 `preStop` Hook 执行时间较长，务必相应增加 `terminationGracePeriodSeconds`。
- 设置 `activeDeadlineSeconds` 防止 Init 容器无限期失败（但注意该字段在 Init 容器完成后仍然生效）。
- 调试 `CrashLoopBackOff` 时，优先查看容器日志和 `kubectl describe pod` 事件。

## Pod 生命周期流程图

```
                    ┌──────────────────────────────────────────────────┐
                    │                  Pod 生命周期                      │
                    └──────────────────────────────────────────────────┘

  创建 Pod          调度到节点         容器启动              容器终止
    │                  │                │                    │
    ▼                  ▼                ▼                    ▼
┌─────────┐      ┌─────────┐     ┌──────────┐      ┌───────────────┐
│ Pending  │ ──→  │ Pending  │ ──→ │ Running  │ ──→  │  Succeeded    │
│(未调度)  │      │(调度中)  │     │          │      │  (所有容器    │
│          │      │(拉取镜像)│     │          │      │   成功退出)   │
└─────────┘      └─────────┘     └──────────┘      └───────────────┘
                                       │
                                       │ 容器失败
                                       ▼
                                 ┌───────────────┐
                                 │    Failed      │
                                 │ (至少一个容器  │
                                 │  非零退出)     │
                                 └───────────────┘
```

**容器终止详细流程**:
```
1. API Server 设置 deletionTimestamp
2. kubelet 发现 Pod 需要终止
3. 执行 preStop Hook（如果配置了）
4. 发送 SIGTERM 给容器主进程
5. 控制平面从 EndpointSlice 移除 Pod
6. 等待 terminationGracePeriodSeconds
7. 发送 SIGKILL 强制终止
8. 清理容器和 Pod 沙箱
```

## 实战 YAML 示例

### 配置完整生命周期管理的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: lifecycle-demo
  namespace: prod
spec:
  terminationGracePeriodSeconds: 60        # 优雅终止等待时间
  containers:
  - name: app
    image: myregistry.com/myapp:v1.0
    ports:
    - containerPort: 8080
    # 探针配置影响 Pod Conditions
    startupProbe:                          # 保护慢启动应用
      httpGet:
        path: /healthz
        port: 8080
      periodSeconds: 5
      failureThreshold: 30                 # 最长等待 150 秒启动
    livenessProbe:                         # 触发容器重启
      httpGet:
        path: /healthz
        port: 8080
      periodSeconds: 15
      failureThreshold: 3
    readinessProbe:                        # 控制 Endpoint 注册/摘除
      httpGet:
        path: /ready
        port: 8080
      periodSeconds: 10
    lifecycle:
      postStart:                           # 容器启动后执行
        exec:
          command: ["/bin/sh", "-c", "echo 'App started' >> /var/log/app.log"]
      preStop:                             # 容器终止前执行
        exec:
          command: ["/bin/sh", "-c", "sleep 10 && /app/graceful-shutdown"]
    resources:
      requests:
        cpu: "250m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "512Mi"
  # Readiness Gate: 自定义就绪条件
  readinessGates:
  - conditionType: "custom.io/config-loaded"
```

## 故障排查

### CrashLoopBackOff
- **症状**: Pod 状态在 `CrashLoopBackOff` 和 `Running` 间反复切换，重启间隔越来越长。
- **常见原因**: 应用启动崩溃、配置错误、依赖不可达、OOMKilled。
- **诊断命令**:
  ```bash
  # 查看容器退出原因和退出码
  kubectl describe pod <pod-name> -n prod | grep -A 10 "Last State"
  
  # 查看上一次容器日志
  kubectl logs <pod-name> --previous -n prod
  
  # 常见退出码含义:
  # 0   = 正常退出
  # 1   = 应用错误
  # 137 = SIGKILL (通常是 OOMKilled 或超时被 kill)
  # 143 = SIGTERM (正常终止信号)
  ```

### Pod 卡在 Terminating 状态
- **症状**: Pod 长时间处于 `Terminating` 状态，无法删除。
- **常见原因**: `preStop` Hook 阻塞、finalizer 未清除、kubelet 与 API Server 通信失败。
- **诊断命令**:
  ```bash
  # 查看 Pod 是否有 finalizers
  kubectl get pod <pod-name> -n prod -o jsonpath='{.metadata.finalizers}'
  
  # 查看 Pod 的 deletionTimestamp
  kubectl get pod <pod-name> -n prod -o jsonpath='{.metadata.deletionTimestamp}'
  
  # 检查节点上的 kubelet 状态
  kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  ```
- **解决方案**: 等待优雅期结束；强制删除 `kubectl delete pod <pod-name> --force --grace-period=0`；清除 finalizer。

### Pod 状态为 Unknown
- **症状**: Pod 状态显示 `Unknown`。
- **常见原因**: Pod 所在节点与 API Server 通信中断。
- **诊断命令**:
  ```bash
  # 查看 Pod 所在节点状态
  kubectl get pod <pod-name> -n prod -o jsonpath='{.spec.nodeName}'
  kubectl get node <node-name>
  ```

## 生产就绪检查清单

- [ ] 三种探针全部配置（startupProbe + livenessProbe + readinessProbe）
- [ ] `terminationGracePeriodSeconds` 根据应用实际关闭时间设置
- [ ] `preStop` Hook 已配置，确保优雅关闭（如等待 LB 摘流量）
- [ ] Liveness Probe 仅检测进程自身（不检查外部依赖）
- [ ] 应用正确处理 SIGTERM 信号，实现优雅关闭
- [ ] 了解 CrashLoopBackOff 的指数退避机制（10s→20s→...→300s）

## 命令快速参考

```bash
# 查看 Pod phase 和 conditions
kubectl get pod <pod-name> -n prod -o jsonpath='{.status.phase}'
kubectl get pod <pod-name> -n prod -o jsonpath='{.status.conditions}'

# 查看容器状态详情
kubectl get pod <pod-name> -n prod -o jsonpath='{.status.containerStatuses}'

# 查看 Pod 重启次数
kubectl get pod <pod-name> -n prod -o jsonpath='{.status.containerStatuses[0].restartCount}'

# 查看上一次容器终止信息
kubectl get pod <pod-name> -n prod -o jsonpath='{.status.containerStatuses[0].lastState.terminated}'

# 强制删除卡住的 Pod
kubectl delete pod <pod-name> -n prod --force --grace-period=0
```

## 交叉引用

- [Pods 基础](./pods.md)
- [容器生命周期钩子](./container-lifecycle-hooks.md)
- [Pod QoS 等级](./pod-quality-of-service-classes.md)
- [Pod 综合故障排查手册](../../domain-12-troubleshooting/08-pod-comprehensive-troubleshooting.md)
- [Pod 生命周期事件深度解析](../../domain-4-workloads/11-pod-lifecycle-events.md)
- [Pod 故障树分析 (FTA)](../../topic-fta/list/pod-fta.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/
