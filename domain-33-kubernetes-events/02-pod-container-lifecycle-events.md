# 02 - Pod 与容器生命周期事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档全面覆盖 kubelet 产生的 Pod 和容器生命周期事件，包括容器创建、启动、终止、重启、沙箱管理等全流程事件，是生产环境排查 Pod 启动失败、CrashLoopBackOff、驱逐等问题的核心参考。**

---

## 目录

- [一、事件总览](#一事件总览)
- [二、Pod 生命周期状态与事件关系](#二pod-生命周期状态与事件关系)
- [三、容器状态与事件映射](#三容器状态与事件映射)
- [四、正常生命周期事件](#四正常生命周期事件)
- [五、失败与重启事件](#五失败与重启事件)
- [六、沙箱管理事件](#六沙箱管理事件)
- [七、生命周期钩子事件](#七生命周期钩子事件)
- [八、驱逐与抢占事件](#八驱逐与抢占事件)
- [九、综合排查案例](#九综合排查案例)
- [十、生产环境最佳实践](#十生产环境最佳实践)

---

## 一、事件总览

### 1.1 本文档覆盖的事件列表

| 事件原因 (Reason) | 类型 | 生产频率 | 适用版本 | 简要说明 |
|:---|:---|:---|:---|:---|
| `Created` | Normal | 高频 | v1.0+ | 容器创建成功 |
| `Started` | Normal | 高频 | v1.0+ | 容器启动成功 |
| `Killing` | Normal | 中频 | v1.0+ | 发送信号终止容器 |
| `BackOff` | Warning | 高频 | v1.0+ | 容器重启退避 (CrashLoopBackOff) |
| `Failed` | Warning | 中频 | v1.0+ | 容器创建或启动失败 |
| `ExceededGracePeriod` | Warning | 低频 | v1.0+ | 容器超过优雅终止期 |
| `Preempting` | Warning | 低频 | v1.15+ | 抢占低优先级容器 |
| `FailedCreatePodSandBox` | Warning | 中频 | v1.0+ | Pod 沙箱创建失败 |
| `FailedPodSandBoxStatus` | Warning | 低频 | v1.8+ | 获取沙箱状态失败 |
| `SandboxChanged` | Normal | 低频 | v1.6+ | Pod 沙箱变更，需重建 |
| `FailedCreatePodContainer` | Warning | 低频 | v1.0+ | 无法确保 Pod 容器存在 |
| `FailedSync` | Warning | 中频 | v1.0+ | Pod 同步失败 |
| `FailedValidation` | Warning | 低频 | v1.0+ | Pod 验证失败 |
| `NetworkNotReady` | Warning | 低频 | v1.9+ | 网络插件未就绪 |
| `FailedPostStartHook` | Warning | 低频 | v1.0+ | PostStart 钩子执行失败 |
| `FailedPreStopHook` | Warning | 低频 | v1.0+ | PreStop 钩子执行失败 |
| `Evicted` | Warning | 中频 | v1.9+ | Pod 被驱逐 |
| `TopologyAffinityError` | Warning | 罕见 | v1.22+ | 拓扑亲和性错误 |

**事件来源**: 所有事件均由 **kubelet** 产生 (`source.component: kubelet`)

### 1.2 快速索引

| 问题场景 | 关注事件 | 跳转章节 |
|:---|:---|:---|
| 容器无法启动 | `Failed`, `BackOff` | [五.1](#51-backoff---容器重启退避) [五.2](#52-failed---容器创建或启动失败) |
| CrashLoopBackOff | `BackOff`, `Failed` | [五.1](#51-backoff---容器重启退避) |
| Pod 一直 Pending | `FailedCreatePodSandBox`, `NetworkNotReady` | [六.1](#61-failedcreatepodsandbox---pod-沙箱创建失败) |
| 容器无法正常终止 | `ExceededGracePeriod` | [五.3](#53-exceededgraceperiod---容器超过优雅终止期) |
| PostStart 钩子失败 | `FailedPostStartHook` | [七.1](#71-failedpoststarthook---poststart-钩子执行失败) |
| Pod 被驱逐 | `Evicted` | [八.1](#81-evicted---pod-被驱逐) |

---

## 二、Pod 生命周期状态与事件关系

### 2.1 Pod Phase 与事件时间线

```
Pod 生命周期阶段               产生的主要事件
════════════════════════════════════════════════════════════════════

┌─────────────┐
│   Pending   │ ◀─── Pod 已创建，等待调度和容器启动
└──────┬──────┘
       │
       │ [调度器完成调度]
       │
       ├──▶ FailedCreatePodSandBox    (沙箱创建失败)
       ├──▶ NetworkNotReady            (网络插件未就绪)
       ├──▶ FailedValidation           (Pod 验证失败)
       │
┌──────▼──────┐
│  Running    │ ◀─── 至少一个容器正在运行
└──────┬──────┘
       │
       ├──▶ Created                    (容器创建成功)
       ├──▶ Started                    (容器启动成功)
       ├──▶ BackOff                    (容器崩溃，进入退避重启)
       ├──▶ Failed                     (容器启动失败)
       ├──▶ FailedPostStartHook        (PostStart 钩子失败)
       ├──▶ SandboxChanged             (沙箱变更)
       ├──▶ FailedSync                 (同步失败)
       │
┌──────▼──────┐
│  Succeeded  │ ◀─── 所有容器成功终止 (Job/CronJob 常见)
└──────┬──────┘
       │
       └──▶ Killing                    (正常终止容器)
       
┌─────────────┐
│   Failed    │ ◀─── 至少一个容器非零退出
└──────┬──────┘
       │
       └──▶ Failed                     (容器退出码非零)
            ExceededGracePeriod        (强制终止)

┌─────────────┐
│   Unknown   │ ◀─── 无法获取 Pod 状态 (节点失联)
└─────────────┘

┌─────────────┐
│  Evicted    │ ◀─── Pod 被驱逐 (资源不足或违反策略)
└──────┬──────┘
       │
       └──▶ Evicted                    (驱逐事件)
```

### 2.2 Pod 生命周期完整事件流

**正常启动流程**:
```
1. [Scheduler] → Scheduled           # 调度成功 (见 05-scheduling-preemption-events.md)
2. [kubelet]   → SandboxChanged      # (可选) 沙箱需重建
3. [kubelet]   → Pulling             # 开始拉取镜像 (见 03-image-pull-events.md)
4. [kubelet]   → Pulled              # 镜像拉取成功
5. [kubelet]   → Created             # 容器创建成功
6. [kubelet]   → Started             # 容器启动成功
7. [kubelet]   → (无事件)            # 容器运行中
```

**异常启动流程 (CrashLoopBackOff)**:
```
1. [Scheduler] → Scheduled
2. [kubelet]   → Pulled
3. [kubelet]   → Created
4. [kubelet]   → Started
5. [容器崩溃]
6. [kubelet]   → BackOff             # 第1次退避 (10s)
7. [kubelet]   → Pulled
8. [kubelet]   → Created
9. [kubelet]   → Started
10. [容器崩溃]
11. [kubelet]  → BackOff             # 第2次退避 (20s)
    ... 退避时间指数增长至最大 5 分钟 ...
```

**优雅终止流程**:
```
1. [用户删除] kubectl delete pod xxx --grace-period=30
2. [kubelet]   → Killing             # 发送 SIGTERM
3. [等待 30s]
4. [kubelet]   → Killing             # (可选) 发送 SIGKILL (如超时)
5. [kubelet]   → ExceededGracePeriod # (可选) 如容器未终止
```

---

## 三、容器状态与事件映射

### 3.1 容器三态与事件关系

Kubernetes 中容器有三种状态，每种状态对应不同的事件触发时机：

| 容器状态 | 说明 | 可能的 Reason | 对应事件 |
|:---|:---|:---|:---|
| **Waiting** | 容器尚未运行 | ContainerCreating | `Pulling`, `Pulled`, `Created` |
| | | CrashLoopBackOff | `BackOff`, `Failed` |
| | | ImagePullBackOff | `Failed`, `BackOff` (见 03-image-pull-events.md) |
| | | CreateContainerConfigError | `Failed` |
| | | ErrImagePull | `Failed` (见 03-image-pull-events.md) |
| **Running** | 容器正在运行 | - | `Started` |
| **Terminated** | 容器已终止 | Completed | `Killing` |
| | | Error | `Failed`, `Killing` |
| | | OOMKilled | `OOMKilling` (见 04-probe-health-check-events.md) |
| | | ContainerCannotRun | `Failed` |

### 3.2 容器重启策略与事件关系

| restartPolicy | 容器退出情况 | kubelet 行为 | 产生事件 |
|:---|:---|:---|:---|
| **Always** | 任何退出 | 始终重启 | `BackOff` (如崩溃) |
| **OnFailure** | 退出码非零 | 重启 | `BackOff` |
| | 退出码为零 | 不重启 | 无 |
| **Never** | 任何退出 | 不重启 | `Failed` (如非零退出) |

---

## 四、正常生命周期事件

### 4.1 `Created` - 容器创建成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

容器运行时（如 containerd、CRI-O、Docker）已成功创建容器实例，但尚未启动（进程尚未执行）。这是容器生命周期的第二阶段（第一阶段是镜像拉取）。

在此阶段：
- 容器的文件系统层已准备完毕
- 容器的网络命名空间已创建（或加入 Pod 沙箱的网络命名空间）
- 容器的挂载卷已完成
- 容器的 entrypoint/command 已配置，但尚未执行

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type    Reason   Age   From     Message
  ----    ------   ----  ----     -------
  Normal  Pulled   30s   kubelet  Successfully pulled image "nginx:1.25" in 2.5s
  Normal  Created  30s   kubelet  Created container nginx
  Normal  Started  29s   kubelet  Started container nginx
```

#### 影响面说明

- **用户影响**: 无，这是正常操作的一部分
- **服务影响**: 容器尚未启动，服务不可用
- **集群影响**: 无
- **关联事件链**: `Pulled` → `Created` → `Started`

#### 排查建议

**该事件本身不需要排查，但如果看到 `Created` 但没有后续 `Started` 事件，说明容器启动阶段失败。**

```bash
# 检查 Pod 状态
kubectl get pod my-app-7d5bc-xyz12 -o wide

# 查看容器详细状态
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.containerStatuses[*]}'

# 检查是否有后续失败事件
kubectl get events --field-selector involvedObject.name=my-app-7d5bc-xyz12,type=Warning
```

#### 解决建议

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Created 后无 Started | 容器 entrypoint 配置错误 | 检查 Pod spec 的 `command` 和 `args` 字段 |
| Created 后立即 Failed | PostStart 钩子失败 | 检查 `FailedPostStartHook` 事件 |
| Created 后长时间无响应 | 容器运行时故障 | 检查节点上的 containerd/docker 日志 |

---

### 4.2 `Started` - 容器启动成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

容器进程已成功启动并开始执行。这是容器生命周期的第三阶段。容器的主进程（PID 1）已经运行。

在此阶段：
- 容器的 entrypoint/command 已开始执行
- PostStart 钩子（如果配置）已执行成功（或正在执行）
- 容器状态从 `Waiting` 转为 `Running`
- 如果配置了 startupProbe，开始执行探测
- 如果配置了 livenessProbe/readinessProbe，开始执行探测

**重要**: `Started` 事件并不意味着容器健康或服务就绪，仅表示进程已启动。

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type    Reason   Age   From     Message
  ----    ------   ----  ----     -------
  Normal  Created  30s   kubelet  Created container app
  Normal  Started  29s   kubelet  Started container app
```

#### 影响面说明

- **用户影响**: 无，容器已启动，等待健康检查通过
- **服务影响**: 如果没有配置 readinessProbe，容器会立即接收流量；如果配置了，需等待探测成功
- **集群影响**: 无
- **关联事件链**: `Created` → `Started` → (健康检查事件，见 04-probe-health-check-events.md)

#### 排查建议

```bash
# 检查容器是否真正运行中
kubectl get pod my-app -o jsonpath='{.status.containerStatuses[?(@.name=="app")].state}'

# 检查容器启动时间
kubectl get pod my-app -o jsonpath='{.status.containerStatuses[?(@.name=="app")].state.running.startedAt}'

# 查看容器日志
kubectl logs my-app -c app

# 如果容器立即崩溃，查看上一个容器的日志
kubectl logs my-app -c app --previous

# 检查健康检查状态
kubectl get pod my-app -o jsonpath='{.status.containerStatuses[?(@.name=="app")]}'
```

#### 解决建议

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Started 后立即崩溃 | 应用配置错误 | 检查容器日志 `kubectl logs --previous` |
| Started 但 Not Ready | readinessProbe 失败 | 检查 `Unhealthy` 事件 (见 04-probe-health-check-events.md) |
| Started 后 OOMKilled | 内存限制过低 | 增加 `resources.limits.memory` |

---

### 4.3 `Killing` - 终止容器

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

kubelet 正在向容器发送终止信号，开始优雅终止流程。这是容器正常关闭或被重启前的标准操作。

**终止流程**:
1. 如果配置了 PreStop 钩子，先执行钩子
2. 向容器主进程（PID 1）发送 SIGTERM 信号
3. 等待 `terminationGracePeriodSeconds` 时长（默认 30 秒）
4. 如果容器仍未退出，发送 SIGKILL 强制终止

**触发场景**:
- 用户主动删除 Pod (`kubectl delete pod`)
- Deployment 滚动更新
- Pod 被驱逐（节点资源不足）
- Node 维护（drain 操作）
- 容器探针失败需要重启（livenessProbe 失败）
- Pod 被抢占（高优先级 Pod 需要资源）

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type    Reason   Age   From     Message
  ----    ------   ----  ----     -------
  Normal  Killing  10s   kubelet  Stopping container app
  Normal  Killing  10s   kubelet  Killing container with id docker://abc123:Need to kill Pod
```

**message 格式变体**:
- `"Stopping container <container-name>"` - 标准终止
- `"Killing container with id <cri>://<container-id>:<reason>"` - 带原因的终止
- `"Container <container-name> failed liveness probe, will be restarted"` - 探针失败导致的重启

#### 影响面说明

- **用户影响**: 
  - 如果 Pod 仅有一个副本，会导致服务短暂不可用
  - 如果 Pod 有多副本且配合 PodDisruptionBudget，影响可控
- **服务影响**: 
  - 容器停止接收新流量（Endpoint 会被移除）
  - 已有连接需在 terminationGracePeriodSeconds 内处理完成
- **集群影响**: 无
- **关联事件链**: 
  - 主动删除: `Killing` → (容器退出)
  - 探针重启: `Unhealthy` → `Killing` → `Started`
  - 滚动更新: `Killing` (旧 Pod) + `Started` (新 Pod)

#### 排查建议

```bash
# 查看 Pod 的删除原因
kubectl get pod my-app -o jsonpath='{.status.reason}' 
# 可能的值: Evicted, NodeLost, UnexpectedAdmissionError 等

# 查看 Pod 的删除时间戳
kubectl get pod my-app -o jsonpath='{.metadata.deletionTimestamp}'

# 查看 Pod 的优雅终止期设置
kubectl get pod my-app -o jsonpath='{.spec.terminationGracePeriodSeconds}'

# 检查是否因为探针失败而重启
kubectl get events --field-selector involvedObject.name=my-app,reason=Unhealthy

# 检查是否因为驱逐而终止
kubectl get events --field-selector involvedObject.name=my-app,reason=Evicted

# 查看容器退出码和原因
kubectl get pod my-app -o jsonpath='{.status.containerStatuses[*].lastState.terminated}'
```

#### 解决建议

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| 频繁 Killing 后重启 | livenessProbe 配置过于严格 | 调整探针参数 `failureThreshold`, `timeoutSeconds` |
| Killing 后容器无法终止 | 应用未处理 SIGTERM | 在应用中实现信号处理，或减少 `terminationGracePeriodSeconds` |
| Killing 伴随 Evicted | 节点资源不足 | 增加节点资源或减少 Pod 的资源请求 |
| Killing 后超时 | PreStop 钩子耗时过长 | 优化 PreStop 钩子或增加 `terminationGracePeriodSeconds` |
| 大量 Pod 同时 Killing | 节点 NotReady 或被 drain | 检查节点状态 `kubectl describe node` |

---

## 五、失败与重启事件

### 5.1 `BackOff` - 容器重启退避

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 ⚠️ |

#### 事件含义

容器在启动后崩溃或退出，kubelet 尝试重启容器，但采用**指数退避策略**（Exponential Backoff）来避免过于频繁的重启操作。这是生产环境中最常见的 Warning 事件之一。

**CrashLoopBackOff 详解**:

当你看到 `kubectl get pods` 显示容器状态为 `CrashLoopBackOff` 时，实际上是 kubelet 正在执行退避重启，对应的就是 `BackOff` 事件。

**退避时间计算**:
- 第 1 次重启: 立即（0 秒）
- 第 2 次重启: 10 秒后
- 第 3 次重启: 20 秒后
- 第 4 次重启: 40 秒后
- 第 5 次重启: 80 秒后
- 第 6 次重启: 160 秒后（2 分 40 秒）
- 第 7 次及以后: 300 秒后（5 分钟，最大退避时间）

**触发条件**:
- 容器主进程非零退出（退出码 1-255）
- 容器因信号终止（如 SIGSEGV 段错误）
- 容器启动后立即退出
- 容器运行一段时间后崩溃（如果 `restartPolicy` 允许重启）

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason   Age                   From     Message
  ----     ------   ----                  ----     -------
  Normal   Pulled   5m10s                 kubelet  Successfully pulled image "myapp:v1"
  Normal   Created  5m10s                 kubelet  Created container app
  Normal   Started  5m10s                 kubelet  Started container app
  Warning  BackOff  4m50s                 kubelet  Back-off restarting failed container app in pod my-app-7d5bc-xyz12_default(abc-123)
  Normal   Pulled   4m40s (x2 over 5m)    kubelet  Container image "myapp:v1" already present on machine
  Normal   Created  4m40s (x2 over 5m)    kubelet  Created container app
  Normal   Started  4m40s (x2 over 5m)    kubelet  Started container app
  Warning  BackOff  4m20s (x2 over 4m50s) kubelet  Back-off restarting failed container app in pod my-app-7d5bc-xyz12_default(abc-123)
  Warning  BackOff  3m50s (x3 over 4m50s) kubelet  Back-off restarting failed container app in pod my-app-7d5bc-xyz12_default(abc-123)
```

**message 格式**:
```
Back-off restarting failed container <container-name> in pod <pod-name>_<namespace>(<pod-uid>)
```

**Pod 状态显示**:
```bash
$ kubectl get pod my-app-7d5bc-xyz12

NAME                 READY   STATUS             RESTARTS      AGE
my-app-7d5bc-xyz12   0/1     CrashLoopBackOff   5 (2m ago)    10m
```

#### 影响面说明

- **用户影响**: **高** - 服务完全不可用，Pod 无法正常运行
- **服务影响**: **严重** - 该 Pod 无法提供服务，如果所有副本都 CrashLoopBackOff，服务完全中断
- **集群影响**: 低 - 不影响其他 Pod，但会占用 CPU/内存资源进行反复重启
- **关联事件链**: `Started` → (容器崩溃) → `BackOff` → `Pulled` → `Created` → `Started` → (崩溃) → `BackOff` (循环)

#### 排查建议

**1. 查看容器日志（最关键）**

```bash
# 查看当前容器日志
kubectl logs my-app-7d5bc-xyz12 -c app

# 查看上一个容器的日志（更重要！因为当前容器可能还在退避）
kubectl logs my-app-7d5bc-xyz12 -c app --previous

# 查看最近 20 行日志
kubectl logs my-app-7d5bc-xyz12 -c app --previous --tail=20

# 持续监控日志（如果容器短暂运行）
kubectl logs my-app-7d5bc-xyz12 -c app -f
```

**2. 检查容器退出码**

```bash
# 查看容器上一次终止的退出码和原因
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.containerStatuses[?(@.name=="app")].lastState.terminated}'

# 常见退出码含义:
# 0   - 正常退出（但 restartPolicy=Always 仍会重启）
# 1   - 应用错误退出
# 2   - 命令使用错误
# 126 - 命令无法执行（权限问题）
# 127 - 命令未找到
# 130 - 容器被 Ctrl+C 终止 (SIGINT)
# 137 - 容器被 SIGKILL 终止（OOMKilled 或强制终止）
# 139 - 段错误 (SIGSEGV)
# 143 - 容器被 SIGTERM 终止
```

**3. 检查容器配置**

```bash
# 查看容器 command 和 args
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].command}'
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].args}'

# 查看环境变量
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].env}'

# 查看卷挂载
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].volumeMounts}'

# 查看资源限制
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].resources}'
```

**4. 检查 PostStart 钩子**

```bash
# 如果配置了 PostStart 钩子，检查是否失败
kubectl get events --field-selector involvedObject.name=my-app-7d5bc-xyz12,reason=FailedPostStartHook

# 查看 PostStart 钩子配置
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.containers[?(@.name=="app")].lifecycle.postStart}'
```

**5. 快速诊断脚本**

```bash
#!/bin/bash
POD="my-app-7d5bc-xyz12"
CONTAINER="app"

echo "=== Pod 状态 ==="
kubectl get pod $POD

echo ""
echo "=== 容器状态详情 ==="
kubectl get pod $POD -o jsonpath='{.status.containerStatuses[?(@.name=="'$CONTAINER'")]}'

echo ""
echo "=== 上一次容器终止信息 ==="
kubectl get pod $POD -o jsonpath='{.status.containerStatuses[?(@.name=="'$CONTAINER'")].lastState.terminated}' | jq .

echo ""
echo "=== 重启次数 ==="
kubectl get pod $POD -o jsonpath='{.status.containerStatuses[?(@.name=="'$CONTAINER'")].restartCount}'

echo ""
echo "=== 最近事件 ==="
kubectl get events --field-selector involvedObject.name=$POD --sort-by='.lastTimestamp' | tail -10

echo ""
echo "=== 上一个容器日志（最后 30 行）==="
kubectl logs $POD -c $CONTAINER --previous --tail=30
```

#### 解决建议

| 退出码/原因 | 常见场景 | 排查方向 | 解决方案 |
|:---|:---|:---|:---|
| **退出码 1** | 应用启动失败 | 检查容器日志 | 修复应用配置/代码错误 |
| | 缺少环境变量 | 检查 `env`, `envFrom` | 添加必需的环境变量 |
| | 配置文件错误 | 检查 ConfigMap/Secret 挂载 | 修正配置内容 |
| | 数据库连接失败 | 检查网络和服务地址 | 修正连接字符串，检查 Service |
| **退出码 127** | 命令未找到 | 检查 `command` 字段 | 修正容器 entrypoint 路径 |
| | 可执行文件不存在 | 检查镜像内容 | 重新构建镜像，确保文件存在 |
| **退出码 137** | OOMKilled | 检查 `kubectl describe pod` 的 Last State | 增加 `resources.limits.memory` |
| | 被强制终止 | 检查节点日志 | 调查节点是否有内存压力 |
| **退出码 139** | 段错误 (SIGSEGV) | 检查应用日志 | 修复代码内存访问错误 |
| | 依赖库版本不兼容 | 检查镜像基础镜像 | 更新依赖库版本 |
| **退出码 126** | 权限问题 | 检查文件权限 | 添加 `chmod +x` 到 Dockerfile |
| | SecurityContext 限制 | 检查 `securityContext` | 调整 `runAsUser`, `fsGroup` |
| **FailedPostStartHook** | PostStart 钩子失败 | 查看 `FailedPostStartHook` 事件 | 修复钩子脚本或增加超时 |

**典型案例解决方案**:

**案例 1: 应用配置文件路径错误**
```bash
# 现象: 容器日志显示 "config file not found"
# 解决: 检查 ConfigMap 挂载路径和应用读取路径是否一致

kubectl get pod my-app -o jsonpath='{.spec.containers[0].volumeMounts}'
kubectl get pod my-app -o jsonpath='{.spec.volumes}'
```

**案例 2: 数据库密码错误**
```bash
# 现象: 容器日志显示 "authentication failed"
# 解决: 检查 Secret 内容是否正确

kubectl get secret db-password -o jsonpath='{.data.password}' | base64 -d
```

**案例 3: 端口冲突**
```bash
# 现象: 容器日志显示 "address already in use"
# 解决: 检查同一个 Pod 中是否有多个容器监听相同端口

kubectl get pod my-app -o jsonpath='{.spec.containers[*].ports}'
```

---

### 5.2 `Failed` - 容器创建或启动失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

容器在创建或启动阶段遇到错误，无法进入运行状态。这个事件涵盖多种失败场景，具体原因需要结合事件消息（message 字段）来判断。

**与 BackOff 的区别**:
- `Failed`: 容器**创建或启动**阶段失败（进程尚未运行或刚启动就失败）
- `BackOff`: 容器**运行中**崩溃，正在退避重启

**常见失败原因分类**:
1. **容器运行时错误** - 容器引擎无法创建容器
2. **容器配置错误** - spec 配置不合法
3. **资源限制** - 无法分配所需资源
4. **安全策略限制** - SecurityContext 或 PodSecurityPolicy 阻止
5. **PostStart 钩子失败** - 生命周期钩子执行失败

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason   Age   From     Message
  ----     ------   ----  ----     -------
  Warning  Failed   10s   kubelet  Error: failed to create containerd task: failed to create shim task: OCI runtime create failed: ...
  Warning  Failed   10s   kubelet  Error: failed to start container "app": Error response from daemon: OCI runtime create failed: ...
  Warning  Failed   10s   kubelet  Failed to create container: rpc error: code = Unknown desc = failed to start container ...
```

**message 格式变体（常见）**:

| message 前缀 | 含义 | 典型场景 |
|:---|:---|:---|
| `Error: failed to create containerd task` | 容器运行时创建任务失败 | OCI runtime 错误，通常是底层配置问题 |
| `Error: failed to start container` | 容器启动失败 | entrypoint 错误、权限问题 |
| `Error: CreateContainerConfigError` | 容器配置错误 | ConfigMap/Secret 不存在或键名错误 |
| `Error: ImagePullBackOff` | 镜像拉取失败 | 详见 03-image-pull-events.md |
| `FailedPostStartHook` | PostStart 钩子执行失败 | 钩子脚本返回非零退出码 |
| `InvalidImageName` | 镜像名称不合法 | 镜像名称格式错误 |

#### 影响面说明

- **用户影响**: **高** - 容器无法启动，服务不可用
- **服务影响**: **严重** - 该 Pod 无法提供服务
- **集群影响**: 低 - 仅影响该 Pod，不影响其他工作负载
- **关联事件链**: 
  - 配置错误: `Failed` → (不会重启)
  - 镜像问题: `Failed` + `ErrImagePull` → `ImagePullBackOff`
  - 钩子失败: `Started` → `FailedPostStartHook` → `Failed`

#### 排查建议

**1. 查看完整错误消息**

```bash
# 查看 Pod 事件，重点看 Failed 事件的 message
kubectl describe pod my-app-7d5bc-xyz12 | grep -A 5 "Failed"

# 查看容器状态中的错误信息
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.containerStatuses[?(@.name=="app")].state.waiting.message}'
```

**2. 检查容器配置引用**

```bash
# 检查 ConfigMap 是否存在
kubectl get pod my-app -o jsonpath='{.spec.volumes[*].configMap.name}' | xargs -n1 kubectl get configmap

# 检查 Secret 是否存在
kubectl get pod my-app -o jsonpath='{.spec.volumes[*].secret.secretName}' | xargs -n1 kubectl get secret

# 检查环境变量引用的 ConfigMap/Secret
kubectl get pod my-app -o jsonpath='{.spec.containers[*].env[?(@.valueFrom)]}' | jq .
```

**3. 检查安全上下文**

```bash
# 查看 Pod 级别安全上下文
kubectl get pod my-app -o jsonpath='{.spec.securityContext}' | jq .

# 查看容器级别安全上下文
kubectl get pod my-app -o jsonpath='{.spec.containers[?(@.name=="app")].securityContext}' | jq .

# 检查是否有 PodSecurityPolicy 限制（已废弃，v1.25+）
kubectl get psp
```

**4. 检查节点容器运行时日志**

```bash
# 如果是 containerd
ssh <node> "sudo journalctl -u containerd -n 100 | grep my-app"

# 如果是 Docker (v1.24 前)
ssh <node> "sudo journalctl -u docker -n 100 | grep my-app"

# 如果是 CRI-O
ssh <node> "sudo journalctl -u crio -n 100 | grep my-app"
```

#### 解决建议

| 错误消息关键词 | 问题原因 | 排查方向 | 解决方案 |
|:---|:---|:---|:---|
| **CreateContainerConfigError** | ConfigMap/Secret 不存在 | 检查资源是否存在 | 创建缺失的 ConfigMap/Secret |
| | 引用的 key 不存在 | 检查 `env[].valueFrom` 的 key | 修正 key 名称或添加到 ConfigMap |
| **runC create failed: unable to start container** | SecurityContext 限制 | 检查 `securityContext` | 调整 `runAsUser`, `fsGroup`, `capabilities` |
| | AppArmor/SELinux 限制 | 检查节点安全策略 | 调整 Pod 注解或节点策略 |
| **OCI runtime create failed: container_linux.go: starting container process caused: exec: ...** | entrypoint 文件不存在 | 检查 `command` 字段 | 修正可执行文件路径或重建镜像 |
| | 权限不足（非可执行） | 检查文件权限 | 在 Dockerfile 中添加 `RUN chmod +x` |
| **FailedPostStartHook** | PostStart 钩子超时 | 检查钩子逻辑 | 优化钩子脚本，减少执行时间 |
| | 钩子命令不存在 | 检查 `lifecycle.postStart.exec.command` | 修正命令路径 |
| **invalid reference format** | 镜像名称格式错误 | 检查 `image` 字段 | 使用正确格式: `registry/repo:tag` |
| **Error: ErrImagePull** | 镜像拉取失败 | 见 03-image-pull-events.md | 检查镜像仓库认证和网络 |

**典型案例解决方案**:

**案例 1: CreateContainerConfigError - ConfigMap key 不存在**
```bash
# 错误: Error: CreateContainerConfigError: configmap "app-config" not found
# 排查:
kubectl get configmap app-config -n <namespace>

# 解决: 创建 ConfigMap
kubectl create configmap app-config --from-literal=APP_ENV=production
```

**案例 2: CreateContainerConfigError - Secret key 不存在**
```yaml
# 错误: Error: CreateContainerConfigError: couldn't find key "DB_PASSWORD" in Secret "db-secret"
# 排查:
kubectl get secret db-secret -o jsonpath='{.data}' | jq .

# 解决: 添加缺失的 key
kubectl create secret generic db-secret \
  --from-literal=DB_PASSWORD=mysecret \
  --dry-run=client -o yaml | kubectl apply -f -
```

**案例 3: OCI runtime create failed - 权限问题**
```bash
# 错误: OCI runtime create failed: container_linux.go:380: starting container process caused: exec: "/app/start.sh": permission denied

# 解决: 在 Dockerfile 中添加可执行权限
RUN chmod +x /app/start.sh

# 或者在 Pod spec 中调整 securityContext
securityContext:
  runAsUser: 0  # 临时方案，不推荐生产使用
```

---

### 5.3 `ExceededGracePeriod` - 容器超过优雅终止期

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

容器在收到 SIGTERM 信号后，在 `terminationGracePeriodSeconds` 时间内没有正常退出，kubelet 被迫发送 SIGKILL 强制终止容器。

**优雅终止流程回顾**:
1. kubelet 向容器发送 SIGTERM 信号
2. 等待 `terminationGracePeriodSeconds` 秒（默认 30 秒）
3. 如果容器仍在运行，发送 SIGKILL 强制终止
4. 产生 `ExceededGracePeriod` 事件

**为什么容器不能优雅退出**:
- 应用未捕获 SIGTERM 信号
- 应用清理逻辑耗时过长（如持久化大量数据）
- 应用在等待长连接关闭
- 应用被阻塞（如等待锁或 I/O）
- PreStop 钩子执行时间过长

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason              Age   From     Message
  ----     ------              ----  ----     -------
  Normal   Killing             45s   kubelet  Stopping container app
  Warning  ExceededGracePeriod 15s   kubelet  Container app exceeded grace period
```

**message 格式**:
```
Container <container-name> exceeded grace period
```

**Pod 事件时间线**:
```
T+0s   [User] kubectl delete pod my-app --grace-period=30
T+0s   [kubelet] Killing (发送 SIGTERM)
T+30s  [kubelet] ExceededGracePeriod (容器未退出，发送 SIGKILL)
T+30s  [容器] 进程被强制终止
```

#### 影响面说明

- **用户影响**: **中** - 容器被强制终止，可能导致数据丢失或状态不一致
- **服务影响**: 
  - 长连接被强制断开（如 WebSocket、gRPC stream）
  - 未完成的事务可能回滚
  - 缓存数据可能丢失
- **集群影响**: 无
- **关联事件链**: `Killing` → (等待 30s) → `ExceededGracePeriod` → (容器强制终止)

#### 排查建议

```bash
# 1. 查看 Pod 的优雅终止期设置
kubectl get pod my-app -o jsonpath='{.spec.terminationGracePeriodSeconds}'

# 2. 查看容器是否配置了 PreStop 钩子
kubectl get pod my-app -o jsonpath='{.spec.containers[?(@.name=="app")].lifecycle.preStop}'

# 3. 检查容器日志，查看应用是否收到 SIGTERM
kubectl logs my-app -c app --previous | grep -i "sigterm\|shutdown\|graceful"

# 4. 查看容器退出码（应为 137 = SIGKILL）
kubectl get pod my-app -o jsonpath='{.status.containerStatuses[?(@.name=="app")].lastState.terminated.exitCode}'

# 5. 测试应用是否正确处理 SIGTERM
# 在本地容器中测试
docker run --rm myapp:v1 &
PID=$!
kill -TERM $PID  # 发送 SIGTERM
sleep 5
kill -0 $PID && echo "Process still running" || echo "Process terminated"
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 | 代码示例 |
|:---|:---|:---|:---|
| **应用未捕获 SIGTERM** | 应用未实现信号处理 | 在应用中添加 SIGTERM 处理逻辑 | 见下方代码 |
| **PreStop 钩子超时** | PreStop 钩子执行时间过长 | 优化钩子逻辑或增加 `terminationGracePeriodSeconds` | - |
| **长连接无法快速关闭** | 应用等待所有连接关闭 | 设置连接强制关闭超时 | 见下方代码 |
| **数据持久化耗时长** | 应用在退出时保存大量数据 | 使用后台线程异步持久化，或增加优雅期 | - |
| **应用阻塞在某个操作** | 如等待锁、I/O | 在 SIGTERM 处理器中设置标志位，中断阻塞操作 | - |

**应用代码修复示例**:

**Python 示例（Flask 应用）**:
```python
import signal
import sys
from flask import Flask

app = Flask(__name__)
is_shutting_down = False

def graceful_shutdown(signum, frame):
    global is_shutting_down
    print(f"Received signal {signum}, shutting down gracefully...")
    is_shutting_down = True
    
    # 停止接收新请求
    # Flask 内置服务器不支持优雅关闭，生产应使用 gunicorn
    
    # 清理资源
    # close_database_connections()
    # flush_cache()
    
    sys.exit(0)

# 注册 SIGTERM 处理器
signal.signal(signal.SIGTERM, graceful_shutdown)
signal.signal(signal.SIGINT, graceful_shutdown)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

**Go 示例（HTTP 服务）**:
```go
package main

import (
    "context"
    "net/http"
    "os"
    "os/signal"
    "syscall"
    "time"
)

func main() {
    srv := &http.Server{Addr: ":8080"}
    
    // 启动服务器
    go func() {
        if err := srv.ListenAndServe(); err != http.ErrServerClosed {
            log.Fatalf("Server error: %v", err)
        }
    }()
    
    // 等待 SIGTERM 信号
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGTERM, syscall.SIGINT)
    <-quit
    
    log.Println("Shutting down server...")
    
    // 优雅关闭，设置 25 秒超时（小于 terminationGracePeriodSeconds）
    ctx, cancel := context.WithTimeout(context.Background(), 25*time.Second)
    defer cancel()
    
    if err := srv.Shutdown(ctx); err != nil {
        log.Fatalf("Server forced to shutdown: %v", err)
    }
    
    log.Println("Server exited")
}
```

**Node.js 示例（Express 应用）**:
```javascript
const express = require('express');
const app = express();
const server = app.listen(8080);

// 优雅关闭处理
process.on('SIGTERM', () => {
  console.log('SIGTERM signal received: closing HTTP server');
  server.close(() => {
    console.log('HTTP server closed');
    // 清理资源
    // db.close();
    // cache.flush();
    process.exit(0);
  });
  
  // 设置强制退出超时（小于 terminationGracePeriodSeconds）
  setTimeout(() => {
    console.error('Could not close connections in time, forcefully shutting down');
    process.exit(1);
  }, 25000);
});
```

**Pod spec 调整示例**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  terminationGracePeriodSeconds: 60  # 增加优雅终止期到 60 秒
  containers:
    - name: app
      image: myapp:v1
      lifecycle:
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 停止接收新流量（可选，如果应用有健康检查端点）
                touch /tmp/shutting-down
                # 等待现有请求处理完成
                sleep 10
```

---

## 六、沙箱管理事件

### 6.1 `FailedCreatePodSandBox` - Pod 沙箱创建失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

kubelet 无法创建 Pod 沙箱（Sandbox），导致 Pod 无法启动。**Pod 沙箱是 Pod 中所有容器共享的基础运行环境**，包括网络命名空间、IPC 命名空间等。

**Pod 沙箱的作用**:
- 创建 Pod 的网络命名空间（所有容器共享同一个 IP 地址）
- 创建 IPC 命名空间（容器间共享内存通信）
- 运行 pause 容器（也称 infra 容器，用于占据命名空间）
- 为容器提供共享的卷挂载点

**失败原因分类**:
1. **网络插件问题** - CNI 插件配置错误或不可用（最常见 ~70%）
2. **容器运行时问题** - containerd/CRI-O 故障
3. **节点资源问题** - 节点磁盘、内存、PID 资源耗尽
4. **内核参数问题** - 如 `net.ipv4.ip_forward` 未开启
5. **SELinux/AppArmor 限制** - 安全策略阻止

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason                  Age   From     Message
  ----     ------                  ----  ----     -------
  Warning  FailedCreatePodSandBox  10s   kubelet  Failed to create pod sandbox: rpc error: code = Unknown desc = failed to setup network for sandbox "abc123": plugin type="calico" failed (add): error getting ClusterInformation: connection refused
  Warning  FailedCreatePodSandBox  10s   kubelet  Failed to create pod sandbox: rpc error: code = Unknown desc = failed to reserve sandbox name "my-app_default_abc-123_0": name "my-app_default_abc-123_0" is reserved for "def456"
  Warning  FailedCreatePodSandBox  10s   kubelet  Failed to create pod sandbox: rpc error: code = Unknown desc = failed to create containerd task: too many open files
```

**message 格式变体**:

| message 关键词 | 根本原因 | 典型触发场景 |
|:---|:---|:---|
| `failed to setup network for sandbox` | CNI 插件错误 | CNI 插件未安装、配置错误、API Server 不可达 |
| `failed to reserve sandbox name` | 沙箱名称冲突 | 上一个同名 Pod 的沙箱未清理干净 |
| `too many open files` | 节点文件描述符耗尽 | 节点上运行过多容器或进程泄漏 |
| `no space left on device` | 节点磁盘空间不足 | /var/lib/containerd 或 /var/lib/kubelet 磁盘满 |
| `cannot allocate memory` | 节点内存不足 | 节点内存耗尽 |
| `failed to start sandbox container` | pause 容器启动失败 | pause 镜像不存在或拉取失败 |

#### 影响面说明

- **用户影响**: **高** - Pod 完全无法启动，卡在 Pending 状态
- **服务影响**: **严重** - 该 Pod 无法提供服务，可能导致服务容量不足
- **集群影响**: **可能扩散** - 如果是节点级问题（如 CNI 故障），该节点上的所有新 Pod 都会失败
- **关联事件链**: 
  - CNI 问题: `Scheduled` → `FailedCreatePodSandBox` (循环重试)
  - 节点问题: 多个 Pod 同时出现 `FailedCreatePodSandBox`

#### 排查建议

**1. 查看完整错误信息**

```bash
# 查看 Pod 事件
kubectl describe pod my-app-7d5bc-xyz12 | grep -A 5 "FailedCreatePodSandBox"

# 查看 Pod 状态
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.phase}'
# 应显示: Pending

# 查看 Pod 的 Conditions
kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.conditions}' | jq .
```

**2. 检查节点上的 CNI 配置**

```bash
# 登录到 Pod 所在节点
NODE=$(kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.spec.nodeName}')
ssh $NODE

# 检查 CNI 配置文件是否存在
sudo ls -la /etc/cni/net.d/

# 常见 CNI 配置文件:
# - 10-calico.conflist (Calico)
# - 10-flannel.conflist (Flannel)
# - 10-cilium.conf (Cilium)
# - 10-weave.conflist (Weave)

# 检查 CNI 二进制文件
sudo ls -la /opt/cni/bin/

# 检查 CNI 插件日志（Calico 示例）
sudo journalctl -u calico-node -n 50
```

**3. 检查容器运行时状态**

```bash
# 检查 containerd 状态
sudo systemctl status containerd

# 查看 containerd 日志
sudo journalctl -u containerd -n 100 | grep -i "sandbox\|error"

# 检查 CRI 接口是否正常
sudo crictl version
sudo crictl pods | head
```

**4. 检查节点资源**

```bash
# 检查磁盘空间
df -h /var/lib/containerd
df -h /var/lib/kubelet

# 检查文件描述符限制
ulimit -n
cat /proc/sys/fs/file-nr  # 已用 / 未用 / 最大

# 检查内存
free -h

# 检查 PID 限制
cat /proc/sys/kernel/pid_max
ps aux | wc -l
```

**5. 检查 pause 容器镜像**

```bash
# 查看 kubelet 配置的 pause 镜像
ps aux | grep kubelet | grep pod-infra-container-image

# 常见 pause 镜像:
# - registry.k8s.io/pause:3.9
# - k8s.gcr.io/pause:3.8

# 手动拉取 pause 镜像测试
sudo crictl pull registry.k8s.io/pause:3.9
```

**6. 检查网络配置**

```bash
# 检查 IP forwarding 是否开启（必需）
sysctl net.ipv4.ip_forward
# 应输出: net.ipv4.ip_forward = 1

# 检查 bridge-nf-call-iptables（必需）
sysctl net.bridge.bridge-nf-call-iptables
# 应输出: net.bridge.bridge-nf-call-iptables = 1

# 如果输出错误，说明 br_netfilter 模块未加载
sudo modprobe br_netfilter
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 排查命令 | 解决方案 |
|:---|:---|:---|:---|
| **failed to setup network** | CNI 插件错误 | `kubectl get pods -n kube-system \| grep cni` | 重启 CNI 插件 Pod |
| | CNI 配置错误 | `cat /etc/cni/net.d/*.conf` | 修正 CNI 配置文件 |
| | CNI 无法连接 API Server | `kubectl get --raw /healthz` | 检查 API Server 可达性 |
| **failed to reserve sandbox name** | 沙箱名称冲突 | `sudo crictl sandboxes \| grep my-app` | 手动删除旧沙箱: `crictl rmsandbox <id>` |
| **too many open files** | 文件描述符耗尽 | `cat /proc/sys/fs/file-nr` | 增加 `fs.file-max`: `sysctl -w fs.file-max=2097152` |
| **no space left on device** | 磁盘空间不足 | `df -h /var/lib` | 清理容器镜像/日志，或扩容磁盘 |
| **cannot allocate memory** | 内存不足 | `free -h` | 驱逐低优先级 Pod 或添加节点 |
| **failed to start sandbox container** | pause 镜像不存在 | `sudo crictl images \| grep pause` | 手动拉取 pause 镜像 |

**典型案例解决方案**:

**案例 1: Calico CNI 插件故障**
```bash
# 错误: failed to setup network for sandbox: plugin type="calico" failed

# 排查:
kubectl get pods -n kube-system | grep calico
# 如果 calico-node Pod 不是 Running 状态，说明 CNI 插件有问题

# 查看 calico-node 日志
kubectl logs -n kube-system calico-node-xyz12 -c calico-node

# 解决: 重启 calico-node DaemonSet
kubectl rollout restart daemonset/calico-node -n kube-system
```

**案例 2: 沙箱名称冲突**
```bash
# 错误: failed to reserve sandbox name "my-app_default_abc-123_0": name is reserved

# 排查:
ssh <node>
sudo crictl sandboxes | grep my-app

# 解决: 删除旧沙箱
sudo crictl rmsandbox <sandbox-id>

# 验证
kubectl delete pod my-app-7d5bc-xyz12  # 让控制器重建 Pod
```

**案例 3: 文件描述符耗尽**
```bash
# 错误: too many open files

# 排查:
cat /proc/sys/fs/file-nr
# 输出: 123456  0  131072  (已用 / 未用 / 最大)
# 如果第一个数字接近第三个，说明耗尽

# 解决: 增加文件描述符限制
sudo sysctl -w fs.file-max=2097152
sudo sysctl -w fs.nr_open=2097152

# 永久生效
echo "fs.file-max = 2097152" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# 同时修改 systemd 服务的限制
sudo mkdir -p /etc/systemd/system/containerd.service.d
sudo tee /etc/systemd/system/containerd.service.d/override.conf <<EOF
[Service]
LimitNOFILE=1048576
EOF
sudo systemctl daemon-reload
sudo systemctl restart containerd
```

**案例 4: IP forwarding 未开启**
```bash
# 错误: 网络不通，Pod 无法通信

# 排查:
sysctl net.ipv4.ip_forward
# 输出: net.ipv4.ip_forward = 0  ❌

# 解决:
sudo sysctl -w net.ipv4.ip_forward=1
sudo sysctl -w net.bridge.bridge-nf-call-iptables=1

# 永久生效
echo "net.ipv4.ip_forward = 1" | sudo tee -a /etc/sysctl.conf
echo "net.bridge.bridge-nf-call-iptables = 1" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

---

### 6.2 `FailedPodSandBoxStatus` - 获取沙箱状态失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.8+ |
| **生产频率** | 低频 |

#### 事件含义

kubelet 无法从容器运行时获取 Pod 沙箱的状态信息。这通常表明容器运行时（containerd/CRI-O）出现故障或响应缓慢。

**与 FailedCreatePodSandBox 的区别**:
- `FailedCreatePodSandBox`: 创建沙箱失败（沙箱不存在）
- `FailedPodSandBoxStatus`: 沙箱存在，但无法查询其状态（运行时故障）

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason                   Age   From     Message
  ----     ------                   ----  ----     -------
  Warning  FailedPodSandBoxStatus   10s   kubelet  Failed to get pod sandbox status: rpc error: code = Unavailable desc = connection error: ...
  Warning  FailedPodSandBoxStatus   10s   kubelet  Failed to get pod sandbox status: rpc error: code = DeadlineExceeded desc = context deadline exceeded
```

#### 影响面说明

- **用户影响**: 中 - Pod 可能仍在运行，但 kubelet 无法管理
- **服务影响**: 中 - 如果沙箱实际已损坏，容器会逐渐异常
- **集群影响**: **可能严重** - 通常表明节点容器运行时有问题，影响该节点所有 Pod
- **关联事件链**: `FailedPodSandBoxStatus` → `FailedSync` (kubelet 同步失败)

#### 排查建议

```bash
# 1. 检查容器运行时状态
ssh <node>
sudo systemctl status containerd
sudo journalctl -u containerd -n 100 | grep -i error

# 2. 检查 CRI 接口响应
sudo crictl --timeout=10s pods  # 如果超时，说明运行时响应慢

# 3. 检查沙箱列表
sudo crictl sandboxes | grep my-app

# 4. 查看节点资源（运行时故障常因资源不足）
top
df -h
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| connection error | containerd/CRI-O 进程崩溃 | 重启容器运行时: `systemctl restart containerd` |
| context deadline exceeded | 容器运行时响应超时 | 检查节点负载，考虑驱逐部分 Pod |
| 节点资源耗尽 | 磁盘/内存/PID 耗尽导致运行时故障 | 清理资源或标记节点不可调度 |

---

### 6.3 `SandboxChanged` - Pod 沙箱变更

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.6+ |
| **生产频率** | 低频 |

#### 事件含义

kubelet 检测到 Pod 的沙箱配置发生变化（如网络模式、IPC 模式、主机名等），需要销毁旧沙箱并重新创建。**这会导致 Pod 中所有容器重启**。

**触发场景**:
- Pod 的 `hostNetwork`, `hostIPC`, `hostPID` 字段被修改（不可变字段，实际不会发生）
- 节点重启后沙箱状态不一致
- 容器运行时升级后沙箱格式变更
- CNI 插件配置变更

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type    Reason         Age   From     Message
  ----    ------         ----  ----     -------
  Normal  SandboxChanged 10s   kubelet  Pod sandbox changed, it will be killed and re-created.
```

#### 影响面说明

- **用户影响**: 中 - Pod 中所有容器会重启
- **服务影响**: 中 - 服务短暂不可用（重启期间）
- **集群影响**: 无
- **关联事件链**: `SandboxChanged` → `Killing` (所有容器) → `FailedCreatePodSandBox` / `Created` → `Started`

#### 排查建议

```bash
# 这是正常的运维操作，通常不需要特别处理
# 如果频繁出现，检查:

# 1. 节点是否频繁重启
kubectl get events --field-selector involvedObject.kind=Node,reason=Rebooted

# 2. CNI 插件是否稳定
kubectl get pods -n kube-system | grep cni

# 3. 容器运行时是否频繁重启
ssh <node> "systemctl status containerd"
```

#### 解决建议

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| 节点重启后大量 SandboxChanged | 正常现象 | 无需处理，等待 Pod 重建 |
| 频繁出现且伴随网络问题 | CNI 配置不稳定 | 检查 CNI 插件配置和日志 |

---

### 6.4 `FailedCreatePodContainer` - 无法确保 Pod 容器存在

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

kubelet 无法确保 Pod 容器的存在。这是一个较为宽泛的错误，通常是容器运行时底层错误的封装。

#### 典型事件消息

```bash
Events:
  Type     Reason                    Age   From     Message
  ----     ------                    ----  ----     -------
  Warning  FailedCreatePodContainer  10s   kubelet  unable to ensure pod container exists: ...
```

#### 排查建议

查看完整错误消息中的具体原因，通常会引用其他更具体的错误（如 `FailedCreatePodSandBox`, `Failed` 等）。

---

## 七、生命周期钩子事件

### 7.1 `FailedPostStartHook` - PostStart 钩子执行失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

容器的 PostStart 生命周期钩子执行失败。PostStart 钩子在容器启动后**立即**执行（与主进程并发），如果钩子失败，**容器会被终止并重启**。

**PostStart 钩子的特性**:
- 在容器的 `ENTRYPOINT` 执行后立即触发（几乎同时）
- 钩子执行期间，容器状态为 `Running`，但 Pod 可能还是 `Pending`
- 如果钩子失败，容器会被杀死并根据 `restartPolicy` 重启
- 钩子没有执行顺序保证，可能在 `ENTRYPOINT` 之前完成

**钩子失败原因**:
- 钩子命令不存在或路径错误
- 钩子脚本返回非零退出码
- 钩子执行超时（默认无超时，但会受 kubelet 的 `event-qps` 限制）
- 钩子中的 HTTP 请求失败（如果使用 httpGet 钩子）

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason                Age   From     Message
  ----     ------                ----  ----     -------
  Normal   Started               30s   kubelet  Started container app
  Warning  FailedPostStartHook   30s   kubelet  Exec lifecycle hook ([/app/init.sh]) for Container "app" in Pod "my-app_default(abc-123)" failed - error: command '/app/init.sh' exited with 1: ...
  Warning  FailedPostStartHook   30s   kubelet  PostStart handler failed: HTTPGet http://localhost:8080/_poststart: dial tcp 127.0.0.1:8080: connect: connection refused
```

**message 格式变体**:

| 钩子类型 | message 格式 | 示例 |
|:---|:---|:---|
| **Exec 钩子** | `Exec lifecycle hook ([<command>]) failed - error: command '<cmd>' exited with <code>` | 脚本返回非零退出码 |
| **HTTP 钩子** | `PostStart handler failed: HTTPGet <url>: <error>` | HTTP 请求失败或返回非 2xx |

#### 影响面说明

- **用户影响**: **高** - 容器无法正常启动，会进入 CrashLoopBackOff
- **服务影响**: **严重** - Pod 无法提供服务
- **集群影响**: 无
- **关联事件链**: `Started` → `FailedPostStartHook` → `Killing` → `BackOff` → (重试)

#### 排查建议

```bash
# 1. 查看 PostStart 钩子配置
kubectl get pod my-app -o jsonpath='{.spec.containers[?(@.name=="app")].lifecycle.postStart}'

# 2. 查看完整错误消息
kubectl describe pod my-app | grep -A 5 "FailedPostStartHook"

# 3. 如果是 Exec 钩子，尝试手动执行命令
kubectl exec my-app -c app -- /app/init.sh
# 查看退出码
echo $?

# 4. 查看容器日志（钩子可能有输出）
kubectl logs my-app -c app

# 5. 如果是 HTTP 钩子，测试端点
kubectl exec my-app -c app -- curl -v http://localhost:8080/_poststart
```

#### 解决建议

| 问题场景 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **Exec 钩子失败** | 命令不存在 | `kubectl exec -- which <cmd>` | 修正命令路径或在镜像中安装命令 |
| | 脚本返回非零 | 手动执行脚本查看输出 | 修复脚本逻辑，确保成功时返回 0 |
| | 权限不足 | 检查文件权限和 securityContext | 添加可执行权限或调整 runAsUser |
| **HTTP 钩子失败** | 端口未监听 | `curl` 测试端点 | 确保应用在 PostStart 前已监听端口（使用 readinessProbe 代替） |
| | 请求超时 | 检查应用响应时间 | 优化端点响应速度或使用异步初始化 |
| | 返回非 2xx | 查看 HTTP 响应码 | 修复端点逻辑，确保返回 200-299 |

**典型案例解决方案**:

**案例 1: PostStart 脚本返回非零**
```yaml
# 错误配置
lifecycle:
  postStart:
    exec:
      command: ["/bin/sh", "-c", "echo 'Initializing' && exit 1"]

# 修正: 确保脚本成功执行
lifecycle:
  postStart:
    exec:
      command: ["/bin/sh", "-c", "echo 'Initializing' && /app/init.sh || true"]
      # 或者修复 init.sh 使其成功时返回 0
```

**案例 2: PostStart HTTP 端点过早调用**
```yaml
# 问题: PostStart 在应用启动前就调用，导致连接拒绝
lifecycle:
  postStart:
    httpGet:
      path: /_poststart
      port: 8080

# 解决方案 1: 在 PostStart 中添加等待逻辑
lifecycle:
  postStart:
    exec:
      command:
        - /bin/sh
        - -c
        - |
          # 等待应用监听端口
          for i in {1..30}; do
            if nc -z localhost 8080; then
              curl -f http://localhost:8080/_poststart && exit 0
            fi
            sleep 1
          done
          exit 1

# 解决方案 2: 使用 readinessProbe 代替（更推荐）
# PostStart 不适合用于等待应用就绪，应使用 readinessProbe
readinessProbe:
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 10
```

**案例 3: PostStart 钩子命令不存在**
```yaml
# 错误: 命令路径错误
lifecycle:
  postStart:
    exec:
      command: ["/usr/local/bin/init.sh"]  # 文件不存在

# 解决: 在 Dockerfile 中确保文件存在
# Dockerfile:
COPY init.sh /usr/local/bin/init.sh
RUN chmod +x /usr/local/bin/init.sh

# 或者使用 shell 查找命令
lifecycle:
  postStart:
    exec:
      command: ["/bin/sh", "-c", "which init.sh && init.sh"]
```

---

### 7.2 `FailedPreStopHook` - PreStop 钩子执行失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

容器的 PreStop 生命周期钩子执行失败。PreStop 钩子在容器终止前执行，用于优雅关闭前的清理工作。

**PreStop 钩子的特性**:
- 在 kubelet 向容器发送 SIGTERM 之前执行
- 钩子执行期间，容器仍处于 `Running` 状态
- 钩子执行时间计入 `terminationGracePeriodSeconds`
- **即使钩子失败，容器仍会被终止**（与 PostStart 不同）

**钩子失败影响**:
- PreStop 失败**不会阻止容器终止**
- 可能导致资源未正确清理（如连接未关闭、缓存未刷新）
- 产生 Warning 事件，但不影响 Pod 删除流程

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason             Age   From     Message
  ----     ------             ----  ----     -------
  Normal   Killing            30s   kubelet  Stopping container app
  Warning  FailedPreStopHook  30s   kubelet  PreStop lifecycle hook ([/app/cleanup.sh]) for Container "app" in Pod "my-app_default(abc-123)" failed - error: command '/app/cleanup.sh' exited with 1: ...
```

#### 影响面说明

- **用户影响**: 低 - 容器仍会正常终止
- **服务影响**: 低 - 可能有资源泄漏或状态不一致（取决于钩子的作用）
- **集群影响**: 无
- **关联事件链**: `Killing` → `FailedPreStopHook` → (继续执行 SIGTERM)

#### 排查建议

```bash
# 1. 查看 PreStop 钩子配置
kubectl get pod my-app -o jsonpath='{.spec.containers[?(@.name=="app")].lifecycle.preStop}'

# 2. 查看容器日志（钩子可能有输出）
kubectl logs my-app -c app --previous

# 3. 检查钩子执行时间（避免超时）
# PreStop 钩子必须在 terminationGracePeriodSeconds 内完成
kubectl get pod my-app -o jsonpath='{.spec.terminationGracePeriodSeconds}'
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| 钩子脚本返回非零 | 清理逻辑失败 | 修复脚本，或添加错误处理 `|| true` |
| 钩子执行超时 | 清理操作耗时过长 | 增加 `terminationGracePeriodSeconds` 或优化清理逻辑 |
| 命令不存在 | 脚本路径错误 | 修正路径或确保文件存在 |

**PreStop 钩子最佳实践**:
```yaml
lifecycle:
  preStop:
    exec:
      command:
        - /bin/sh
        - -c
        - |
          # 1. 停止接收新流量（标记为 Not Ready）
          touch /tmp/shutdown
          
          # 2. 等待已有请求处理完成
          sleep 10
          
          # 3. 清理资源（确保不失败）
          /app/cleanup.sh || true
          
          # 4. 成功退出
          exit 0
```

---

## 八、驱逐与抢占事件

### 8.1 `Evicted` - Pod 被驱逐

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.9+ |
| **生产频率** | 中频 |

#### 事件含义

Pod 被 kubelet 的**驱逐管理器（Eviction Manager）**主动终止，通常是因为节点资源不足或违反了资源配额策略。

**驱逐触发条件**:

| 资源类型 | 驱逐信号 | 触发条件 | 优先驱逐 |
|:---|:---|:---|:---|
| **内存** | `memory.available` | 可用内存 < 阈值（默认 100Mi） | BestEffort → Burstable → Guaranteed |
| **磁盘** | `nodefs.available` | 节点文件系统可用空间 < 阈值（默认 10%） | 按磁盘使用量排序 |
| | `nodefs.inodesFree` | 节点文件系统 inode 可用 < 阈值（默认 5%） | 按 inode 使用量排序 |
| | `imagefs.available` | 镜像文件系统可用空间 < 阈值（默认 15%） | 按镜像磁盘使用量排序 |
| | `imagefs.inodesFree` | 镜像文件系统 inode 可用 < 阈值 | 按 inode 使用量排序 |
| **PID** | `pid.available` | 可用 PID < 阈值（默认 10%） | 按 PID 使用量排序 |

**驱逐等级**:
- **Soft Eviction** (软驱逐): 超过阈值后等待一段时间（grace period）才驱逐
- **Hard Eviction** (硬驱逐): 立即驱逐，无优雅终止期

**QoS 驱逐优先级** (从高到低):
1. **BestEffort**: 未设置 requests 和 limits 的 Pod
2. **Burstable**: 设置了 requests 但超出使用的 Pod
3. **Guaranteed**: requests = limits 的 Pod

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason   Age   From     Message
  ----     ------   ----  ----     -------
  Warning  Evicted  30s   kubelet  The node was low on resource: memory. Container app was using 1024Mi, which exceeds its request of 512Mi.
  Warning  Evicted  30s   kubelet  The node had condition: [DiskPressure].
  Warning  Evicted  30s   kubelet  Pod ephemeral local storage usage exceeds the total limit of containers 1Gi.
```

**message 格式变体**:

| 驱逐原因 | message 关键词 | 说明 |
|:---|:---|:---|
| **内存压力** | `low on resource: memory` | 节点内存不足 |
| **磁盘压力** | `low on resource: ephemeral-storage` | 节点磁盘空间不足 |
| | `had condition: [DiskPressure]` | 节点处于 DiskPressure 状态 |
| **PID 不足** | `low on resource: pid` | 节点 PID 资源耗尽 |
| **临时存储超限** | `ephemeral local storage usage exceeds` | Pod 使用的临时存储超过限制 |

**Pod 状态变化**:
```bash
$ kubectl get pod my-app-7d5bc-xyz12

NAME                 READY   STATUS    RESTARTS   AGE
my-app-7d5bc-xyz12   0/1     Evicted   0          5m

# Pod 不会自动重启，需要手动删除或由控制器（Deployment 等）重建
$ kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.reason}'
Evicted

$ kubectl get pod my-app-7d5bc-xyz12 -o jsonpath='{.status.message}'
The node was low on resource: memory.
```

#### 影响面说明

- **用户影响**: **高** - Pod 被强制终止，服务中断
- **服务影响**: **严重** - 如果多个副本在同一节点，可能导致服务容量大幅下降
- **集群影响**: **警示** - 表明节点资源不足，可能影响其他 Pod
- **关联事件链**: 
  - 节点压力: `NodeHasMemoryPressure` (Node 事件) → `Evicted` (多个 Pod)
  - Deployment: `Evicted` → (Deployment Controller 创建新 Pod)

#### 排查建议

**1. 查看 Pod 的驱逐原因**

```bash
# 查看 Pod 状态中的详细驱逐原因
kubectl get pod my-app -o jsonpath='{.status.message}'

# 查看 Pod 的资源使用情况
kubectl top pod my-app

# 查看 Pod 的资源配置
kubectl get pod my-app -o jsonpath='{.spec.containers[*].resources}'
```

**2. 查看节点资源状态**

```bash
# 查看节点资源使用情况
kubectl top node <node-name>

# 查看节点状态（是否有压力条件）
kubectl describe node <node-name> | grep -A 5 "Conditions:"
# 关注: MemoryPressure, DiskPressure, PIDPressure

# 查看节点的驱逐阈值配置
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/configz" | jq .kubeletconfig.evictionHard
```

**3. 查看节点上的所有被驱逐 Pod**

```bash
# 查看集群中所有 Evicted 状态的 Pod
kubectl get pods -A --field-selector status.phase=Failed | grep Evicted

# 按节点统计驱逐的 Pod 数量
kubectl get pods -A -o json | jq -r '.items[] | select(.status.reason=="Evicted") | .spec.nodeName' | sort | uniq -c
```

**4. 登录节点检查资源**

```bash
ssh <node>

# 检查内存使用
free -h
# 查看进程内存占用
ps aux --sort=-%mem | head -20

# 检查磁盘空间
df -h
# 查看大文件
du -ah /var/lib/kubelet | sort -rh | head -20
du -ah /var/lib/containerd | sort -rh | head -20

# 检查 inode 使用
df -i

# 检查 PID 使用
cat /proc/sys/kernel/pid_max
ps aux | wc -l
```

**5. 分析驱逐历史**

```bash
# 查看节点最近的驱逐事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>,reason=EvictionThresholdMet

# 查看 kubelet 日志
ssh <node> "sudo journalctl -u kubelet -n 200 | grep -i evict"
```

#### 解决建议

| 驱逐原因 | 根本原因 | 排查方法 | 短期解决方案 | 长期解决方案 |
|:---|:---|:---|:---|:---|
| **内存压力** | 节点内存不足 | `kubectl top node` | 驱逐低优先级 Pod | 增加节点内存或添加节点 |
| | Pod 内存 limits 设置过高 | 检查 Pod limits | 减少 Pod limits | 优化应用内存使用 |
| | 内存泄漏 | 查看 Pod 内存趋势 | 重启 Pod | 修复应用内存泄漏 |
| **磁盘压力** | 容器日志过多 | `du -h /var/log/pods` | 配置日志轮转 | 使用日志收集系统（如 EFK） |
| | 镜像占用过多 | `crictl images` | 清理未使用镜像 | 配置镜像自动清理策略 |
| | 临时文件积累 | `du -h /var/lib/kubelet` | 手动清理 | 配置 emptyDir 限制 |
| **PID 不足** | 进程泄漏 | `ps aux \| wc -l` | 重启泄漏的容器 | 修复应用进程泄漏 |
| | PID 限制过低 | 检查 `pid_max` | 增加 PID 限制 | 调整 kubelet `--pod-max-pids` |
| **临时存储超限** | emptyDir 使用过多 | `kubectl exec -- df -h` | 清理临时文件 | 使用 PV 或对象存储 |
| | 容器写层过大 | 检查容器写入量 | 重启容器 | 优化应用 I/O 模式 |

**典型案例解决方案**:

**案例 1: 内存压力导致驱逐**
```bash
# 现象: 节点频繁驱逐 BestEffort Pod
# 排查:
kubectl describe node <node> | grep -A 10 "Allocated resources"
# 发现内存 request 已达 90%

# 短期解决:
# 1. 标记节点不可调度，避免新 Pod 调度到该节点
kubectl cordon <node>

# 2. 手动驱逐低优先级 Pod
kubectl delete pod <low-priority-pod> --grace-period=30

# 长期解决:
# 1. 为所有 Pod 设置合理的 memory requests 和 limits
apiVersion: v1
kind: Pod
spec:
  containers:
    - name: app
      resources:
        requests:
          memory: "256Mi"
        limits:
          memory: "512Mi"

# 2. 使用 ResourceQuota 限制命名空间的总资源使用
apiVersion: v1
kind: ResourceQuota
metadata:
  name: mem-quota
spec:
  hard:
    requests.memory: "10Gi"
    limits.memory: "20Gi"

# 3. 添加更多节点或增加节点内存
```

**案例 2: 磁盘压力导致驱逐**
```bash
# 现象: 节点 DiskPressure，大量 Pod 被驱逐
# 排查:
ssh <node>
df -h /var/lib/kubelet
# 输出: /var/lib/kubelet 使用 92%

du -sh /var/lib/kubelet/pods/*  | sort -rh | head -10
# 发现某些 Pod 的日志占用数 GB

# 短期解决:
# 1. 清理容器日志
sudo sh -c "truncate -s 0 /var/lib/kubelet/pods/*/volumes/kubernetes.io~empty-dir/logs/*"

# 2. 清理未使用的容器镜像
sudo crictl rmi --prune

# 3. 清理已终止的容器
sudo crictl rm $(sudo crictl ps -a -q)

# 长期解决:
# 1. 配置日志轮转（kubelet 参数）
--container-log-max-size=10Mi
--container-log-max-files=5

# 2. 使用集中式日志系统，避免日志存储在节点
# 部署 Fluentd/Fluent Bit DaemonSet

# 3. 配置镜像 GC 策略
--image-gc-high-threshold=80
--image-gc-low-threshold=70

# 4. 为 emptyDir 设置 sizeLimit
volumes:
  - name: cache
    emptyDir:
      sizeLimit: "1Gi"
```

**案例 3: 临时存储超限**
```bash
# 现象: Pod 消息显示 "ephemeral local storage usage exceeds the total limit"
# 排查:
kubectl get pod my-app -o jsonpath='{.spec.containers[*].resources.limits}'
# 输出: {"ephemeral-storage":"1Gi"}

kubectl exec my-app -- df -h /
# 输出: / 使用了 1.2Gi

# 解决:
# 1. 增加 ephemeral-storage limit
apiVersion: v1
kind: Pod
spec:
  containers:
    - name: app
      resources:
        limits:
          ephemeral-storage: "5Gi"

# 2. 将临时文件存储到 emptyDir
apiVersion: v1
kind: Pod
spec:
  containers:
    - name: app
      volumeMounts:
        - name: tmp
          mountPath: /tmp
  volumes:
    - name: tmp
      emptyDir: {}

# 3. 清理应用临时文件（在代码中定期清理）
```

**清理被驱逐的 Pod**:
```bash
# 被驱逐的 Pod 不会自动删除，会占用 etcd 空间，需要手动清理

# 删除当前命名空间的所有 Evicted Pod
kubectl get pods --field-selector status.phase=Failed -o json | \
  jq -r '.items[] | select(.status.reason=="Evicted") | .metadata.name' | \
  xargs kubectl delete pod

# 删除所有命名空间的 Evicted Pod
kubectl get pods -A --field-selector status.phase=Failed -o json | \
  jq -r '.items[] | select(.status.reason=="Evicted") | "\(.metadata.namespace) \(.metadata.name)"' | \
  xargs -n2 sh -c 'kubectl delete pod -n $0 $1'

# 自动化清理脚本（添加到 CronJob）
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cleanup-evicted-pods
  namespace: kube-system
spec:
  schedule: "0 * * * *"  # 每小时执行一次
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: evicted-pod-cleaner
          containers:
            - name: kubectl
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  kubectl get pods -A --field-selector status.phase=Failed -o json | \
                  jq -r '.items[] | select(.status.reason=="Evicted") | "\(.metadata.namespace) \(.metadata.name)"' | \
                  xargs -n2 sh -c 'kubectl delete pod -n $0 $1 --ignore-not-found'
          restartPolicy: OnFailure
```

---

### 8.2 `Preempting` - 抢占低优先级容器

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.15+ |
| **生产频率** | 低频 |

#### 事件含义

高优先级 Pod 需要资源，kubelet 正在抢占（终止）低优先级 Pod 的容器以释放资源。这是 Kubernetes 优先级与抢占机制的一部分。

**抢占流程**:
1. 高优先级 Pod 无法调度（节点资源不足）
2. kube-scheduler 选择一个节点，并决定抢占该节点上的低优先级 Pod
3. kube-scheduler 向 API Server 更新 Pod 的 `nominatedNodeName` 字段
4. kubelet 收到抢占指令，终止低优先级 Pod
5. 产生 `Preempting` 事件
6. 资源释放后，高优先级 Pod 被调度到该节点

**优先级判断**:
- 基于 Pod 的 `priorityClassName` 字段
- 如果没有设置，默认优先级为 0
- 数值越大，优先级越高

#### 典型事件消息

```bash
Events:
  Type     Reason      Age   From     Message
  ----     ------      ----  ----     -------
  Warning  Preempting  10s   kubelet  Preempting container app to make room for critical pod system/high-priority-pod
```

#### 影响面说明

- **用户影响**: 高 - 低优先级 Pod 被强制终止
- **服务影响**: 中 - 低优先级服务中断，高优先级服务获得资源
- **集群影响**: 无 - 这是正常的调度机制
- **关联事件链**: (高优先级 Pod) `FailedScheduling` → (低优先级 Pod) `Preempting` → `Killing`

#### 排查建议

```bash
# 查看 Pod 的优先级配置
kubectl get pod <pod> -o jsonpath='{.spec.priorityClassName}'

# 查看 PriorityClass 定义
kubectl get priorityclass

# 查看哪些 Pod 被抢占
kubectl get events --field-selector reason=Preempting -A
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| 关键服务被抢占 | 优先级设置过低 | 为关键服务设置高优先级 PriorityClass |
| 频繁抢占 | 节点资源不足 | 增加节点或减少 Pod 的资源 requests |
| 低优先级任务无法运行 | 高优先级 Pod 占用所有资源 | 使用 ResourceQuota 限制高优先级 Pod 的总量 |

**PriorityClass 配置示例**:
```yaml
# 定义高优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 1000000
globalDefault: false
description: "High priority for critical services"

---
# 在 Pod 中使用
apiVersion: v1
kind: Pod
metadata:
  name: critical-app
spec:
  priorityClassName: high-priority
  containers:
    - name: app
      image: myapp:v1
```

---

## 九、综合排查案例

### 案例 1: CrashLoopBackOff 完整排查

**现象**:
```bash
$ kubectl get pods
NAME                 READY   STATUS             RESTARTS      AGE
web-app-7d5bc-xyz    0/1     CrashLoopBackOff   5 (2m ago)    10m
```

**排查步骤**:

```bash
# 1. 查看 Pod 事件
kubectl describe pod web-app-7d5bc-xyz | grep -A 10 "Events:"
# 输出: Warning BackOff Back-off restarting failed container

# 2. 查看容器状态
kubectl get pod web-app-7d5bc-xyz -o jsonpath='{.status.containerStatuses[0]}' | jq .
# 输出: lastState.terminated.exitCode: 1, reason: "Error"

# 3. 查看容器日志
kubectl logs web-app-7d5bc-xyz --previous --tail=50
# 输出: Error: ENOENT: no such file or directory, open '/app/config.json'

# 4. 检查 ConfigMap 挂载
kubectl get pod web-app-7d5bc-xyz -o jsonpath='{.spec.volumes}' | jq .
# 发现 configMap 名称拼写错误

# 5. 修正 Deployment
kubectl edit deployment web-app
# 修正 configMapRef 名称

# 6. 验证修复
kubectl rollout status deployment web-app
kubectl get pods
```

### 案例 2: FailedCreatePodSandBox 网络故障

**现象**:
```bash
$ kubectl get pods
NAME                 READY   STATUS    RESTARTS   AGE
api-server-abc123    0/1     Pending   0          5m
```

**排查步骤**:

```bash
# 1. 查看 Pod 事件
kubectl describe pod api-server-abc123 | grep -A 5 "FailedCreatePodSandBox"
# 输出: failed to setup network for sandbox: plugin type="calico" failed

# 2. 检查节点
kubectl get nodes
NODE=$(kubectl get pod api-server-abc123 -o jsonpath='{.spec.nodeName}')
kubectl describe node $NODE

# 3. 检查 Calico 插件状态
kubectl get pods -n kube-system | grep calico
# 输出: calico-node-xyz  1/2  CrashLoopBackOff

# 4. 查看 Calico 日志
kubectl logs -n kube-system calico-node-xyz -c calico-node
# 输出: Failed to connect to etcd

# 5. 检查 Calico 配置
kubectl get configmap -n kube-system calico-config -o yaml
# 发现 etcd 端点配置错误

# 6. 修复配置后重启 Calico
kubectl rollout restart daemonset/calico-node -n kube-system

# 7. 验证修复
kubectl delete pod api-server-abc123  # 让控制器重建
kubectl get pods
```

---

## 十、生产环境最佳实践

### 10.1 事件监控与告警

**必须监控的事件**:
```yaml
# Prometheus AlertManager 告警规则
groups:
  - name: pod-lifecycle-events
    rules:
      - alert: HighBackOffRate
        expr: |
          increase(
            kube_event_count{reason="BackOff", type="Warning"}[5m]
          ) > 10
        for: 5m
        annotations:
          summary: "大量容器进入 CrashLoopBackOff"
          
      - alert: PodEvictedHigh
        expr: |
          increase(
            kube_event_count{reason="Evicted"}[10m]
          ) > 5
        for: 5m
        annotations:
          summary: "节点资源不足，频繁驱逐 Pod"
          
      - alert: FailedCreatePodSandBoxHigh
        expr: |
          increase(
            kube_event_count{reason="FailedCreatePodSandBox"}[5m]
          ) > 3
        for: 5m
        annotations:
          summary: "Pod 沙箱创建失败，可能是网络或节点问题"
```

### 10.2 容器最佳实践

**1. 正确配置资源**:
```yaml
apiVersion: v1
kind: Pod
spec:
  containers:
    - name: app
      resources:
        requests:
          memory: "256Mi"
          cpu: "100m"
        limits:
          memory: "512Mi"
          cpu: "500m"
          ephemeral-storage: "2Gi"
```

**2. 实现优雅关闭**:
```yaml
spec:
  terminationGracePeriodSeconds: 60
  containers:
    - name: app
      lifecycle:
        preStop:
          exec:
            command: ["/bin/sh", "-c", "sleep 10"]  # 等待请求处理完成
```

**3. 避免使用 PostStart**:
```yaml
# ❌ 不推荐: PostStart 失败会导致容器重启
lifecycle:
  postStart:
    exec:
      command: ["/app/init.sh"]

# ✅ 推荐: 使用 init container 或在应用启动时初始化
initContainers:
  - name: init
    image: myapp:v1
    command: ["/app/init.sh"]
```

**4. 设置合理的重启策略**:
```yaml
spec:
  restartPolicy: Always  # 生产环境推荐
  # OnFailure: 适用于 Job
  # Never: 仅用于调试
```

### 10.3 节点资源管理

**1. 配置驱逐阈值**:
```yaml
# kubelet 配置 (/var/lib/kubelet/config.yaml)
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"
```

**2. 配置镜像和容器 GC**:
```yaml
# kubelet 参数
--image-gc-high-threshold=80
--image-gc-low-threshold=70
--maximum-dead-containers-per-container=1
--maximum-dead-containers=100
```

**3. 配置日志轮转**:
```yaml
--container-log-max-size=10Mi
--container-log-max-files=5
```

### 10.4 快速诊断工具

**Pod 生命周期事件诊断脚本**:
```bash
#!/bin/bash
# pod-lifecycle-diagnosis.sh

POD=$1
NAMESPACE=${2:-default}

if [ -z "$POD" ]; then
  echo "Usage: $0 <pod-name> [namespace]"
  exit 1
fi

echo "=== Pod 基本信息 ==="
kubectl get pod $POD -n $NAMESPACE -o wide

echo -e "\n=== Pod 状态 ==="
kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.status.phase}'
echo ""

echo -e "\n=== 容器状态详情 ==="
kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.status.containerStatuses}' | jq .

echo -e "\n=== 最近事件（按时间排序）==="
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD --sort-by='.lastTimestamp' | tail -20

echo -e "\n=== Warning 事件统计 ==="
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD,type=Warning -o json | jq -r '[.items[].reason] | group_by(.) | map({reason: .[0], count: length})'

echo -e "\n=== 容器日志（最后 30 行）==="
kubectl logs $POD -n $NAMESPACE --tail=30 2>/dev/null || echo "无日志输出"

echo -e "\n=== 上一个容器日志（最后 30 行）==="
kubectl logs $POD -n $NAMESPACE --previous --tail=30 2>/dev/null || echo "无上一个容器"

echo -e "\n=== 资源配置 ==="
kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.spec.containers[*].resources}' | jq .

echo -e "\n=== 重启次数 ==="
kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.status.containerStatuses[*].restartCount}'
echo ""

echo -e "\n=== 节点资源使用情况 ==="
NODE=$(kubectl get pod $POD -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
if [ -n "$NODE" ]; then
  kubectl top node $NODE 2>/dev/null || echo "metrics-server 未安装"
fi
```

**使用方法**:
```bash
chmod +x pod-lifecycle-diagnosis.sh
./pod-lifecycle-diagnosis.sh my-app-7d5bc-xyz default
```

---

## 相关文档交叉引用

- **[01-event-system-architecture.md](./01-event-system-architecture.md)** - 事件系统基础架构和 API 参考
- **[03-image-pull-events.md](./03-image-pull-events.md)** - 镜像拉取相关事件（Pulling, Pulled, ErrImagePull, ImagePullBackOff）
- **[04-probe-health-check-events.md](./04-probe-health-check-events.md)** - 健康检查探针事件（Unhealthy, ProbeError, OOMKilling）
- **[05-scheduling-preemption-events.md](./05-scheduling-preemption-events.md)** - 调度和抢占事件（Scheduled, FailedScheduling）
- **[06-node-lifecycle-condition-events.md](./06-node-lifecycle-condition-events.md)** - 节点生命周期事件（NodeReady, NodeNotReady, NodeHasDiskPressure）
- **[11-storage-volume-events.md](./11-storage-volume-events.md)** - 存储卷挂载事件（FailedMount, FailedAttachVolume）
- **[Domain-4: 工作负载 - Pod 生命周期](../domain-4-workloads/02-pod-lifecycle.md)** - Pod 生命周期详细说明
- **[Topic: 结构化故障排查 - Pod 故障排查](../topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md)** - Pod 故障排查完整流程

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 02/15
