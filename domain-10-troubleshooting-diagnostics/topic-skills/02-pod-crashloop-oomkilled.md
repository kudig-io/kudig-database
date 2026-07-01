---
title: Pod CrashLoopBackOff & OOMKilled 诊断与修复
description: '## 1. 概述'
category: pod
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- prometheus
- grafana
- istio
- envoy
- coredns
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Pod CrashLoopBackOff & OOMKilled 诊断与修复 是什么
- 如何 Pod CrashLoopBackOff & OOMKilled 诊断与修复
trigger_keywords:
- CrashLoopBackOff
- OOMKilled
- 容器崩溃
- 容器重启
- Pod重启
- exit code 137
- exit code 1
- container killed
- 内存溢出
- 频繁重启
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- monitoring-basics
skill_id: SKILL-02_POD_CRASHLOOP_OOMKILLED-001
skill_name: Pod CrashLoopBackOff & OOMKilled 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
created: "2026-05-23"
---

<!-- condition: kubectl get [[Pods|pods]] -A -o jsonpath='{range .items[?(@.status.containerStatuses[?(@.restartCount>3)])]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示频繁重启的 Pod -->

# Pod CrashLoopBackOff & OOMKilled 诊断与修复

> **[[SKILL|Skill]] ID**: SKILL-POD-001  
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批  
> **预计修复时间**: 5-20 分钟  
> **上次更新**: 2026-03

---

## 1. 概述

**CrashLoopBackOff** 和 **OOMKilled** 是生产环境中最常见的 Pod 级别问题，占 [[Kubernetes|Kubernetes]] 工单总量的 30-40%。

- **CrashLoopBackOff**: 容器反复退出（exit），[[kubelet|kubelet]] 以指数退避（exponential backoff, 10s → 20s → 40s → ... → 5min cap）策略不断尝试重启容器。这是一个**状态描述**，不是根因本身——真正的问题隐藏在容器的 exit code 和日志中。
- **OOMKilled**: Linux 内核的 OOM Killer 终止了容器进程（发送 SIGKILL, exit code 137），通常由容器实际内存用量超过 cgroup memory limit 触发。在 Kubernetes 中，这意味着 `resources.limits.memory` 设置不足或应用存在内存泄漏。

> **版本差异说明 / Version Notes**:
> - v1.28+ **Ephemeral Containers GA**: `kubectl debug` 可直接使用，无需启用 feature gate
> - v1.28+ **Native Sidecar Containers** (beta, v1.32 GA): init container 类型为 `restartPolicy: Always` 时，sidecar 容器与主容器并行运行。sidecar 崩溃不会导致 Pod 进入 CrashLoopBackOff，但可能隐藏 sidecar 自身的问题
> - v1.25+ **cgroup v2 默认启用**: 内存统计使用 `memory.current` 而非 `memory.usage_in_bytes`，`kubectl top pod` 显示的内存值可能与 cgroup v1 环境有差异
> - v1.29+ **PodDisruptionConditions** (GA): OOMKilled 的 Pod 会记录 `DisruptionTarget` condition，可用于关联分析

**典型触发场景**:
1. 应用代码 bug 导致进程启动后立即崩溃，Pod 进入 CrashLoopBackOff
2. Java/Go 应用内存占用超过容器 limits，被 OOMKilled 后持续重启
3. 配置错误（缺少 ConfigMap/Secret、命令参数错误）导致容器无法正常启动

**前置条件**:
- **RBAC 权限**:
  - 最小权限: 对 `pods`, `pods/log`, `pods/status`, `events`, `nodes`, `configmaps`, `secrets` 的 `get/list/watch`
  - 验证命令: `kubectl auth can-i list pods -n <namespace>`
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `jq` >= 1.6（可选，用于 JSON 解析）
- **集群组件**:
  - Metrics Server（`kubectl top` 需要）
  - `kubectl debug` 需要 v1.28+（Ephemeral Containers GA）# Requires v1.28+

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod status 显示 **CrashLoopBackOff** / Pod status shows CrashLoopBackOff | `kubectl get pod <pod> -n <ns>` STATUS 列 | 0.95 | 如果 Pod 处于 Pending → SKILL-POD-002 |
| S2 | Pod RESTARTS 计数快速递增 / Pod restart count increasing rapidly | `kubectl get pod <pod> -n <ns>` RESTARTS 列，短时间内 >3 次 | 0.90 | 如果 RESTARTS=0 且 STATUS=Running → 正常，排除本 Skill |
| S3 | 容器被 OOMKilled 终止 / Container terminated with OOMKilled reason | `kubectl describe pod <pod>` → Last State → Reason: OOMKilled | 0.95 | 如果 Reason 为 Evicted → 节点资源压力，参考 SKILL-NODE-001 |
| S4 | Exit code 137（SIGKILL — 通常为 OOM） / Exit code 137 indicating SIGKILL | `kubectl describe pod <pod>` → Last State → Exit Code: 137 | 0.95 | 极少情况下 137 由外部 `kill -9` 触发而非 OOM |
| S5 | Exit code 1（应用程序错误） / Exit code 1 indicating application error | `kubectl describe pod <pod>` → Last State → Exit Code: 1 | 0.70 | exit code 1 含义取决于应用，需结合日志判断 |
| S6 | Exit code 2（Shell misuse / 参数错误） / Exit code 2 indicating shell misuse | `kubectl describe pod <pod>` → Last State → Exit Code: 2 | 0.65 | Bash 脚本语法错误或错误参数 |
| S7 | Exit code 126/127（命令不可执行 / 命令未找到） / Exit code 126/127 command issues | `kubectl describe pod <pod>` → Last State → Exit Code: 126 或 127 | 0.80 | 通常是镜像 entrypoint/CMD 配置错误 |
| S8 | 容器 Last State 显示 "Error" / Container last state shows Error | `kubectl describe pod <pod>` → Last State → Reason: Error | 0.60 | "Error" 是通用状态，需进一步通过日志确认 |
| S9 | Init 容器陷入 CrashLoopBackOff / Init container stuck in CrashLoopBackOff | `kubectl describe pod <pod>` → Init Containers → State: Waiting (CrashLoopBackOff) | 0.85 | Init 容器问题会阻塞所有主容器启动 |
| S10 | Events 中出现 "Back-off restarting failed container" / BackOff event present | `kubectl get events --field-selector involvedObject.name=<pod>` | 0.85 | 确认 event 时间戳是否为近期 |
| S11 | 容器内存使用率持续逼近 limit / Container memory usage approaching limit | `container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.9` | 0.75 | 可能是内存泄漏的早期信号，尚未触发 OOM |

### 2.2 工单关键词映射

Agent 可通过以下常见工单描述进行 NLP 意图匹配：

**中文工单描述**:
- "Pod 一直在重启，状态显示 CrashLoopBackOff"
- "容器被 OOMKilled 了，内存不够用"
- "应用启动失败，Pod 反复崩溃"
- "Pod 重启次数很高，已经几百次了"
- "容器 exit code 137，是不是被杀了"
- "部署后 Pod 一直起不来"
- "init 容器卡住了，主容器没有启动"
- "应用上线后频繁重启"

**English ticket descriptions**:
- "Pod keeps restarting with CrashLoopBackOff"
- "Container is OOMKilled, need more memory"
- "Application fails to start, pod crashing"
- "High restart count on production pods"
- "Exit code 137, container being killed"
- "Deployment rollout pods not coming up"

### 2.3 排除标准

以下场景**不适用**此 Skill，应路由至对应的 Skill：

| 排除条件 | 表面现象 | 正确路由 |
|---------|---------|---------|
| Pod 处于 Pending 状态 | `STATUS=Pending`, 未进入容器创建阶段 | → SKILL-POD-002 (Pod Pending / 调度失败) |
| Pod 处于 ImagePullBackOff | `STATUS=ImagePullBackOff` 或 `ErrImagePull` | → 镜像拉取问题，非本 Skill 范围 |
| Pod 被 Evicted | `STATUS=Evicted`, 由节点资源压力触发 | → SKILL-NODE-001 (节点资源压力) |
| Pod Running 但服务不通 | `STATUS=Running` 且无重启，但请求超时 | → SKILL-NET-002 (Service 连通性) |
| 整个节点 NotReady 导致 Pod 异常 | 多个 Pod 同时异常，节点状态 NotReady | → SKILL-NODE-001 (先诊断节点) |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径（Blast Radius）：

**Step T1**: 统计受影响 Pod 数量和分布范围

```bash
# 检查同一 Deployment/StatefulSet 下所有 Pod 的状态
kubectl get pods -n <namespace> -l <label-selector> -o wide
```

> **判断规则**:
> - 如果只有单个 Pod 受影响 → 单点问题，影响可控
> - 如果同一 Deployment 的多个/全部副本受影响 → Deployment 级别问题
> - 如果跨多个 Deployment/Namespace 出现相同症状 → 可能是集群级别问题（共享依赖、节点问题）

**Step T2**: 确认服务关键性等级

```bash
# 检查 Pod 所属的 Deployment 和 Namespace
kubectl get pod <pod> -n <namespace> -o jsonpath='{.metadata.ownerReferences[0].kind}/{.metadata.ownerReferences[0].name}'
```

> **判断规则**:
> - 生产环境 + 面向客户的服务（customer-facing） → 高优先级
> - 生产环境 + 内部服务（internal） → 中优先级
> - 开发/测试环境 → 低优先级
> - **关注 namespace 命名惯例**: `prod-*`, `production`, `prd` → 生产环境

**Step T3**: 检查是否存在健康副本（部分宕机 vs 全部宕机）

```bash
# 检查 Deployment 的 ready 副本数
kubectl get deployment <deployment> -n <namespace> -o jsonpath='Ready: {.status.readyReplicas}/{.status.replicas}'
```

> **判断规则**:
> - `readyReplicas > 0` → 部分宕机（Partial Outage），服务仍有容量
> - `readyReplicas = 0` 或字段不存在 → 完全宕机（Total Outage），服务不可用
> - 关注 `readyReplicas` 下降趋势，可能从部分宕机演变为完全宕机

**Step T4**: 检查最近是否有部署变更

```bash
# 检查 Deployment 的最近 rollout 历史
kubectl rollout history deployment/<deployment> -n <namespace> --revision=0
```

> **判断规则**:
> - 如果最近有新 revision → 很可能是部署变更引入的问题，回滚是快速修复路径
> - 如果无最近变更 → 可能是运行时问题（内存泄漏、依赖问题等）

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA |
|------|------|------|-----|
| 生产环境 + customer-facing + 所有副本 CrashLoop / OOMKilled + 无健康副本 | **P1** | 服务完全不可用，直接影响用户 | 响应 5min, 修复 30min |
| 生产环境 + 部分副本 CrashLoop / OOMKilled + 剩余副本承载能力不足 | **P2** | 服务降级（Degraded），容量不足可能导致级联问题 | 响应 15min, 修复 1h |
| 生产环境 + 部分副本问题 + 剩余副本可承载流量 | **P2** | 服务有冗余但风险存在 | 响应 15min, 修复 2h |
| 非关键服务 / 开发测试环境 / 单个 Pod 影响 | **P3** | 影响有限，可在工作时间内处理 | 响应 1h, 修复 4h |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过完整诊断流程，立即启动应急修复或升级至人工**：

- **P1 全面宕机**: 所有副本均处于 CrashLoopBackOff 且 readyReplicas=0，customer-facing 服务 → 先执行 REM-007（紧急回滚）或 REM-009（生产紧急回滚），同时升级
- **级联扩散**: 同一 Namespace 中多个不相关 Deployment 同时出现 CrashLoop → 可能是基础设施级别问题
- **OOMKilled 雪崩**: 节点上多个 Pod 同时被 OOMKilled → 可能是节点内存耗尽，转 SKILL-NODE-001
- **核心组件受影响**: CrashLoop 的 Pod 属于核心组件（kube-system namespace 下的 CoreDNS, kube-proxy 等）

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 在 2 分钟内收集关键现场信息，确定容器退出原因和 exit code，为后续深度诊断指明方向。

**Step D1.1**: 获取 Pod 全局状态

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 关注 STATUS, RESTARTS, AGE, NODE 列
- **判断规则**:
  - 如果 `STATUS=CrashLoopBackOff` → 容器反复崩溃，继续 D1.2
  - 如果 `STATUS=Running` 但 `RESTARTS` 很高 → 容器可能刚好重启成功，仍需排查
  - 如果 `STATUS=Error` → 容器已退出，继续 D1.2
  - 如果 `STATUS=Pending` → **排除此 Skill**，转 SKILL-POD-002
  - 如果 `STATUS=ImagePullBackOff` → **排除此 Skill**，镜像拉取问题
  - 记录 `NODE` 列（后续可能需要检查节点状态）
- **输出示例**:
  ```
  NAME           READY   STATUS             RESTARTS      AGE   IP           NODE
  my-app-7b9f6   0/1     CrashLoopBackOff   8 (2m ago)    15m   10.244.1.5   node-02
  ```

**Step D1.2**: 获取 Pod 详细描述（核心诊断信息）

- **命令**:
  ```bash
  kubectl describe pod <pod> -n <namespace>
  ```
- **超时**: 15s
- **预期输出模式**: 重点关注以下字段
  ```
  Containers:
    <container-name>:
      State:          Waiting (reason: CrashLoopBackOff)
      Last State:     Terminated
        Reason:       OOMKilled / Error
        Exit Code:    137 / 1 / 2 / 126 / 127
        Started:      <timestamp>
        Finished:     <timestamp>
  ```
- **判断规则**:
  - 提取 `Last State → Reason` 和 `Exit Code`
  - 如果 `Reason: OOMKilled` 且 `Exit Code: 137` → 确认为 OOM 问题，跳转 D2.3
  - 如果 `Exit Code: 1` → 应用错误，跳转 D2.1（查看日志）
  - 如果 `Exit Code: 2` → Shell/命令使用错误，跳转 D2.1 + D2.2
  - 如果 `Exit Code: 126` → 命令不可执行（权限问题），跳转 D2.2
  - 如果 `Exit Code: 127` → 命令未找到，跳转 D2.2
  - 如果 `Exit Code: 139` → 段错误（SIGSEGV），跳转 D2.5
  - 如果 `Exit Code: 143` → SIGTERM 正常终止但 Pod 重启，跳转 D2.6
  - 如果 `Exit Code: 0` → 进程正常退出但因 restartPolicy 重启，跳转 D2.6
  - 同时检查 `Events` 部分，是否有 `Killing`, `BackOff`, `Unhealthy`, `FailedMount` 等事件
  - **关注 Init Containers 部分**: 如果 init container 状态异常 → 跳转 D1.4

**Step D1.3**: Exit Code 决策树（核心路由逻辑）

根据 D1.2 提取的 Exit Code，参照以下完整决策表确定下一步：

| Exit Code | Signal | 含义 | 典型原因 | 下一步 |
|-----------|--------|------|---------|--------|
| 0 | — | 进程正常退出（Success） | 进程正常完成但 `restartPolicy: Always` 导致重启；或容器是一次性任务误配置为 Deployment | D2.6 |
| 1 | — | 应用程序通用错误 | 未处理的异常、配置文件解析失败、依赖连接失败、启动脚本报错 | D2.1 |
| 2 | — | Shell misuse / 参数错误 | Bash 语法错误、无效参数、`set -e` 下的命令失败 | D2.1, D2.2 |
| 126 | — | 命令不可执行（Permission denied） | 二进制文件无执行权限、mount 的脚本缺少 `chmod +x`、只读文件系统 | D2.2, D2.9 |
| 127 | — | 命令未找到（Command not found） | 错误的 entrypoint/CMD 路径、镜像缺少必要的二进制文件、$PATH 不正确 | D2.2 |
| 128+n | Signal n | 被信号 n 杀死 | 取决于具体信号 | 根据 n 判断 |
| 134 | SIGABRT(6) | 进程调用 abort() | C/C++ 程序 assertion 失败、内存损坏 | D2.5 |
| 137 | SIGKILL(9) | 被 SIGKILL 强制杀死 | **最常见**: OOMKilled（cgroup 内存限制）；也可能是外部 kill、liveness probe 触发的杀死 | D2.3 |
| 139 | SIGSEGV(11) | 段错误（Segmentation fault） | 空指针、内存越界、binary 架构不匹配（如 amd64 binary 运行在 arm64 节点） | D2.5 |
| 143 | SIGTERM(15) | 优雅终止信号 | preStop hook、Pod 被删除/驱逐、Deployment 滚动更新、手动 scale down | D2.6 |

> **Agent 注意**: Exit Code 137 是最关键的分支点——它在 90%+ 的情况下意味着 OOMKilled，但也可能由 liveness probe 超时触发 `kubectl exec kill`。务必结合 `Last State → Reason` 字段确认。

**Step D1.4**: 检查 Init Containers 状态

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.initContainerStatuses[*]}{"init:"}{.name}{" state:"}{.state}{" restarts:"}{.restartCount}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 init container 存在且状态为 `Waiting/CrashLoopBackOff` → Init 容器问题阻塞了主容器，优先诊断 init container
  - 如果所有 init container 状态为 `Terminated(Completed)` → Init 正常完成，问题在主容器
  - **[v1.28+]** 检查是否使用了 Native Sidecar（init container 带有 `restartPolicy: Always`）:
    ```bash
    kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.initContainers[*]}{.name}{": restartPolicy="}{.restartPolicy}{"\n"}{end}'
    ```
    如果 restartPolicy=Always → 这是 sidecar 容器，其 CrashLoop 不会阻塞主容器启动，但可能影响服务功能

### Phase 2: 深度检查（只读，零风险）

> **目标**: 根据 Phase 1 确定的方向，深入分析具体根因。

**Step D2.1**: 检查容器日志（最重要的诊断信息源）

- **命令**:
  ```bash
  # 查看当前容器日志（如果容器正在运行）
  kubectl logs <pod> -n <namespace> -c <container>
  
  # 查看上一次崩溃的容器日志（CrashLoop 场景必用）
  kubectl logs <pod> -n <namespace> -c <container> --previous
  
  # 如果日志很长，只看最后 100 行
  kubectl logs <pod> -n <namespace> -c <container> --previous --tail=100
  ```
- **超时**: 30s
- **预期输出模式**: 关注以下关键信息
  ```
  - Exception / Error / Fatal / Panic 关键字
  - Stack trace（堆栈追踪）
  - "connection refused" / "connection timeout"（依赖服务不可用）
  - "file not found" / "permission denied"（文件系统问题）
  - "out of memory" / "java.lang.OutOfMemoryError"（应用级别 OOM）
  - "no such file or directory"（命令/配置文件缺失）
  - "bind: address already in use"（端口冲突）
  ```
- **判断规则**:
  - 如果看到明确的应用错误/异常 → RC-001（应用代码错误），跳转修复
  - 如果看到 `connection refused` / `timeout` → RC-005（依赖服务不可用），跳转 D2.4
  - 如果看到 `java.lang.OutOfMemoryError` → RC-011（Java Heap OOM），跳转 D2.3
  - 如果看到 `no such file or directory` → RC-004 或 RC-006，跳转 D2.2, D2.4
  - 如果日志为空（无输出） → 容器可能在启动阶段就崩溃了，跳转 D2.2
  - 如果看到 `killed` 但无 OOM 标记 → 检查 liveness probe，跳转 D2.6

**Step D2.2**: 检查容器启动命令和参数

- **命令**:
  ```bash
  # 查看 Pod spec 中的 command 和 args
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  command: "}{.command}{"\n  args: "}{.args}{"\n  image: "}{.image}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 `command` 覆盖了镜像的 ENTRYPOINT → 检查命令路径是否正确
  - 如果 `args` 中包含环境变量引用 `$(VAR_NAME)` → 检查变量是否已注入
  - 如果命令路径以 `/bin/sh -c` 开头 → 检查内联脚本语法
  - 如果未设置 command（使用镜像默认 ENTRYPOINT） → 确认镜像版本是否正确
  - 常见错误模式:
    - `command: ["/bin/bash", "-c", "..."]` 但镜像中没有 bash（alpine 镜像只有 sh）
    - `command: ["./app"]` 但工作目录不正确
    - args 中的路径引用了不存在的配置文件

**Step D2.3**: OOM 深度诊断路径

> **触发条件**: D1.3 确认 Exit Code 137 + Reason: OOMKilled，或日志中出现 `java.lang.OutOfMemoryError`

- **命令组**:
  ```bash
  # D2.3.1: 查看容器的 memory requests/limits 配置
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  requests.memory: "}{.resources.requests.memory}{"\n  limits.memory: "}{.resources.limits.memory}{"\n"}{end}'
  
  # D2.3.2: 查看容器实际内存使用量（需要 Metrics Server）
  kubectl top pod <pod> -n <namespace> --containers
  
  # D2.3.3: 查看同 Deployment 下其他 Pod 的内存用量（判断是否普遍偏高）
  kubectl top pods -n <namespace> -l <label-selector> --containers
  
  # D2.3.4: 检查节点级别的 OOM 事件
  kubectl describe node <node-name> | grep -A5 "OOMKilling"
  
  # D2.3.5: 检查节点内存使用情况
  kubectl top node <node-name>
  ```
- **超时**: 各命令 15s
- **判断规则**:
  
  **场景 A — limits 设置过低（RC-002）**:
  - `kubectl top` 显示的内存用量 接近 limits 值（>85%）
  - 其他同类 Pod 内存用量也偏高
  - 应用在正常负载下就需要更多内存
  - → 修复: REM-001（调整 memory limits）
  
  **场景 B — 应用内存泄漏（RC-003）**:
  - `kubectl top` 显示内存持续增长，不会稳定
  - Pod 启动后内存从低点逐渐增长直至被 OOMKilled
  - 历史 metrics 显示锯齿形内存曲线（增长 → OOMKilled → 重启 → 再增长）
  - → 修复: 短期 REM-001（临时增大 limits）+ 长期需要应用层面排查内存泄漏
  
  **场景 C — Java Heap 与容器内存不匹配（RC-011）**:
  - Java 应用设置了 `-Xmx`/`-Xms`，但未考虑 JVM 非堆内存（Metaspace, native memory, thread stacks, GC overhead）
  - **经验法则**: Container memory limit 应为 Java Heap 的 1.5-2 倍
  - 检查 JVM 参数:
    ```bash
    kubectl logs <pod> -n <namespace> --previous | grep -i "MaxHeapSize\|Xmx\|Xms\|HeapSize"
    ```
  - 如果 `-Xmx=512m` 但 `limits.memory=512Mi` → 几乎必然 OOMKilled
  - **[v1.29+]** 如果使用了 In-Place Pod Resource Resize（alpha → beta），可能动态调整过 limits
  
  **场景 D — Go 应用 GOMEMLIMIT 未设置（RC-002 的变种）**:
  - Go 1.19+ 支持 `GOMEMLIMIT` 环境变量控制 GC 目标
  - 如果未设置 `GOMEMLIMIT`，Go runtime 默认使用 `GOGC=100`（堆翻倍时触发 GC），可能导致内存飙升
  - 检查环境变量:
    ```bash
    kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*].env[*]}{.name}={.value}{"\n"}{end}' | grep -i "GOMEMLIMIT\|GOGC"
    ```

**Step D2.4**: 检查环境变量、ConfigMap、Secret

- **命令**:
  ```bash
  # 查看容器环境变量（包括来自 ConfigMap/Secret 的引用）
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[0].env[*]}{.name}{"="}{.value}{.valueFrom}{"\n"}{end}'
  
  # 查看 envFrom 引用的 ConfigMap/Secret
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[0].envFrom[*]}{"configMapRef: "}{.configMapRef.name}{"  secretRef: "}{.secretRef.name}{"\n"}{end}'
  
  # 检查引用的 ConfigMap 是否存在
  kubectl get configmap <configmap-name> -n <namespace>
  
  # 检查引用的 Secret 是否存在
  kubectl get secret <secret-name> -n <namespace>
  
  # 检查 volume mounts 中引用的 ConfigMap/Secret
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.volumes[*]}{"volume: "}{.name}{" configMap: "}{.configMap.name}{" secret: "}{.secret.secretName}{"\n"}{end}'
  ```
- **超时**: 15s
- **判断规则**:
  - 如果引用的 ConfigMap/Secret 不存在 → RC-006
  - 如果 ConfigMap 存在但 key 不对 → RC-006
  - 如果环境变量 valueFrom 引用了不存在的 key → RC-006
  - Events 中出现 `FailedMount` 或 `MountVolume.SetUp failed for volume` → RC-006
  - 关注 optional 字段: 如果 `optional: false`（默认）且资源不存在 → Pod 将无法启动

**Step D2.5**: 检查架构不匹配和段错误

- **命令**:
  ```bash
  # 检查 Pod 运行的节点架构
  kubectl get node <node-name> -o jsonpath='arch={.status.nodeInfo.architecture} os={.status.nodeInfo.operatingSystem}'
  
  # 检查镜像的目标平台（如果使用 multi-arch 镜像）
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].image}'
  # 然后在支持的环境检查:
  # docker manifest inspect <image> 或 crane manifest <image>
  ```
- **超时**: 15s
- **判断规则**:
  - 如果节点架构为 `arm64` 但镜像仅支持 `amd64`（或反之） → RC-007
  - Exit Code 139 + 架构不匹配 → 高度确认 RC-007
  - Exit Code 139 + 架构匹配 → 应用本身的内存安全问题（C/C++ 程序居多），需要应用层面排查

**Step D2.6**: 检查 liveness/readiness/startup Probe 配置

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  livenessProbe: "}{.livenessProbe}{"\n  readinessProbe: "}{.readinessProbe}{"\n  startupProbe: "}{.startupProbe}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  
  **liveness probe 杀死健康容器（RC-008, 常见误诊）**:
  - 如果 `livenessProbe` 存在且配置了较短的 `initialDelaySeconds`（如 <30s）
  - 而应用实际启动时间较长（如 Java 应用需要 60-120s 启动）
  - 则 liveness probe 在应用还未完成初始化时就标记其不健康，kubelet 杀掉容器
  - **解决方案**: 使用 `startupProbe` 替代在 liveness probe 中设置长 `initialDelaySeconds`
  - 关键参数检查:
    - `initialDelaySeconds`: 应大于应用启动时间
    - `periodSeconds`: 检查频率
    - `failureThreshold`: 连续失败多少次才标记不健康
    - `timeoutSeconds`: 单次探测超时
  - **计算公式**: 应用启动后的最大容忍不健康时间 = `failureThreshold × periodSeconds`
  
  **Exit Code 0 但 Pod 不断重启**:
  - 如果容器正常退出（exit 0）但 `restartPolicy: Always` → 容器将被持续重启
  - 常见于: 一次性任务（Job）被错误部署为 Deployment，或 init 脚本正常结束但被当作主进程
  
  **Exit Code 143 (SIGTERM) 重启循环**:
  - 如果 liveness probe 持续失败，kubelet 会先发送 SIGTERM，等待 `terminationGracePeriodSeconds` 后发送 SIGKILL
  - 检查 Events 中是否有 `Unhealthy` → `Killing` 序列

**Step D2.7**: 检查 Resource Requests 与 Limits 配置

- **命令**:
  ```bash
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  requests: cpu="}{.resources.requests.cpu}{" memory="}{.resources.requests.memory}{"\n  limits: cpu="}{.resources.limits.cpu}{" memory="}{.resources.limits.memory}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 `limits.memory` 未设置 → 容器可以使用节点全部内存，不会被 cgroup OOMKilled（但可能触发节点级 OOM Killer）
  - 如果 `requests.memory` >> `limits.memory` → 配置错误（requests 不应大于 limits）
  - 如果 `limits.cpu` 设置过低 → 虽然不会导致 CrashLoop，但可能导致启动缓慢，间接触发 liveness probe 超时
  - 检查 LimitRange 和 ResourceQuota:
    ```bash
    kubectl get limitrange -n <namespace>
    kubectl get resourcequota -n <namespace>
    ```
  - 如果 namespace 有 LimitRange 且 Pod 未设置资源 → 将使用 LimitRange 的默认值，可能过低

**Step D2.8**: 检查 Sidecar 容器问题 **[v1.28+]**

- **命令**:
  ```bash
  # 列出所有容器（包括 sidecar）的状态
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.containerStatuses[*]}{"container: "}{.name}{" ready: "}{.ready}{" restartCount: "}{.restartCount}{" state: "}{.state}{"\n"}{end}'
  
  # [v1.28+] 检查 native sidecar containers
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.initContainers[?(@.restartPolicy=="Always")]}{"sidecar: "}{.name}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - **[v1.28+]** Native Sidecar Containers 使用 `initContainers` + `restartPolicy: Always` 定义
  - 如果 sidecar 容器 CrashLoop → 可能影响主容器的网络（如 Istio sidecar）或日志收集
  - 如果非 sidecar 的 init container CrashLoop → 会阻塞主容器启动（RC-010）
  - 检查 sidecar 依赖: 某些应用依赖 sidecar proxy（如 Istio [[envoy|envoy]]）才能正常通信

**Step D2.9**: 检查文件系统和权限问题

- **命令**:
  ```bash
  # 检查 Pod 的 securityContext
  kubectl get pod <pod> -n <namespace> -o jsonpath='{"podSecurityContext: "}{.spec.securityContext}{"\n"}{range .spec.containers[*]}{"container: "}{.name}{" securityContext: "}{.securityContext}{"\n"}{end}'
  
  # 检查 volume mounts 的 readOnly 配置
  kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[0].volumeMounts[*]}{"mount: "}{.mountPath}{" readOnly: "}{.readOnly}{"\n"}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 `readOnlyRootFilesystem: true` 且应用需要写入临时文件 → RC-009
  - 如果 `runAsUser` / `runAsGroup` 与应用期望的用户不匹配 → RC-009
  - 如果 `fsGroup` 未设置但 volume 需要特定组权限 → RC-009

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: Exec 进入运行中的容器

- **前提**: 容器当前处于 Running 状态（CrashLoop 间隙或另一个未崩溃的副本）
- **风险等级**: 🟢 低（只读操作）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查文件系统中配置文件是否存在
  kubectl exec <pod> -n <namespace> -c <container> -- ls -la /path/to/config/
  
  # 检查进程列表
  kubectl exec <pod> -n <namespace> -c <container> -- ps aux
  
  # 检查 DNS 解析（依赖服务连通性）
  kubectl exec <pod> -n <namespace> -c <container> -- nslookup <dependency-service>
  
  # 检查依赖服务端口连通性
  kubectl exec <pod> -n <namespace> -c <container> -- nc -zv <service-host> <port>
  
  # 检查内存使用详情（/proc/meminfo）
  kubectl exec <pod> -n <namespace> -c <container> -- cat /proc/meminfo
  
  # 检查 cgroup 内存限制（cgroup v2）
  kubectl exec <pod> -n <namespace> -c <container> -- cat /sys/fs/cgroup/memory.max
  ```
- **超时**: 30s per command
- **判断规则**:
  - 如果配置文件不存在 → RC-006
  - 如果依赖服务 DNS 解析失败 → RC-005 + 参考 SKILL-NET-001
  - 如果端口不通 → RC-005
  - 如果 `/proc/meminfo` 显示可用内存极低 → 确认内存压力

**Step D3.2**: 使用 Ephemeral Debug 容器

- **前提**: 容器已崩溃无法 exec，或容器使用 distroless/scratch 镜像无 shell
- **风险等级**: 🟢 低
- **命令**:
  ```bash
  # 在目标 Pod 中启动临时 debug 容器
  kubectl debug <pod> -n <namespace> -it --image=busybox:latest --target=<container>
  
  # [v1.28+] 使用自定义 debug profile
  kubectl debug <pod> -n <namespace> -it --image=nicolaka/netshoot --profile=general --target=<container>
  
  # 在 debug 容器中检查共享的进程命名空间
  ps aux
  ls /proc/1/root/  # 查看目标容器的文件系统
  cat /proc/1/maps  # 查看目标进程的内存映射
  ```
- **超时**: 60s
- **版本差异**:
  - **[v1.28]**: Custom Debug Profiles beta, `--profile` 参数可用
  - **[v1.30+]**: Custom Debug Profiles GA, 全部 profile 稳定可用
  - 可用 profiles: `general`, `baseline`, `restricted`, `netadmin`, `sysadmin`

**Step D3.3**: 检查应用 Health Endpoint

- **前提**: 应用暴露了 HTTP health endpoint
- **风险等级**: 🟢 低
- **命令**:
  ```bash
  # 通过 port-forward 直接访问应用 health endpoint
  kubectl port-forward <pod> -n <namespace> <local-port>:<container-port> &
  curl -s http://localhost:<local-port>/healthz
  kill %1  # 终止 port-forward
  
  # 或者在另一个 Pod 中 curl
  kubectl run debug-curl --rm -it --image=curlimages/curl --restart=Never -- \
    curl -s http://<service-name>.<namespace>.svc.cluster.local:<port>/healthz
  ```
- **超时**: 30s
- **判断规则**:
  - 如果返回 200 → 应用本身是健康的，问题可能在启动过程或 probe 配置
  - 如果返回 500+ 或超时 → 应用内部问题（确认 RC-001 或 RC-005）
  - 如果连接拒绝 → 容器端口未正确监听，检查应用监听地址（常见错误: 监听 127.0.0.1 而非 0.0.0.0）

---

### Phase 4: 应用级内存分析

> **触发条件**: 确认 OOMKilled（Exit Code 137 + Reason: OOMKilled）且需要深入分析应用内存使用模式
> **目标**: 通过应用级别的 memory profiling 定位内存问题根因
> **前提**: 应用需要暴露 profiling 端点或支持 profiling 工具
> **预计耗时**: 10-30 分钟

**Step D4.1**: Go 应用 — pprof heap profiling
- **前提**: Go 应用已启用 `net/http/pprof` 或 runtime pprof
- **命令**:
  ```bash
  # 通过 port-forward 访问 pprof 端点
  kubectl port-forward <pod> -n <namespace> 6060:6060 &

  # 采集 heap profile
  go tool pprof http://localhost:6060/debug/pprof/heap

  # 在 pprof 交互式界面中：
  # top 20          -- 查看内存占用最高的 20 个函数
  # list <func>     -- 查看具体函数的内存分配
  # web             -- 生成可视化图表（需要 graphviz）

  # 或者直接下载 profile 文件
  curl -o heap.pb.gz http://localhost:6060/debug/pprof/heap
  go tool pprof heap.pb.gz

  # 对比两个时间点的 heap（定位内存增长）
  go tool pprof -base heap1.pb.gz heap2.pb.gz
  ```
- **判断规则**:
  - inuse_space 持续增长且不下降 → 内存泄漏
  - alloc_space 很高但 inuse_space 正常 → 内存分配频繁但有正常 GC、可能需要设置 GOMEMLIMIT
  - 某个函数占用内存异常高 → 检查该函数的数据结构和缓存逻辑
  - goroutine 数量异常高 → goroutine 泄漏，检查 `/debug/pprof/goroutine`

**Step D4.2**: Java 应用 — JFR/jmap heap dump
- **前提**: 容器内有 JDK 工具（jcmd/jmap），或可以使用 ephemeral debug container
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 方式 1: 使用 jcmd 生成 heap dump
  kubectl exec <pod> -n <namespace> -- jcmd 1 GC.heap_dump /tmp/heap.hprof

  # 将 heap dump 复制出来
  kubectl cp <namespace>/<pod>:/tmp/heap.hprof ./heap.hprof

  # 方式 2: 使用 jmap（如果 jcmd 不可用）
  kubectl exec <pod> -n <namespace> -- jmap -dump:format=b,file=/tmp/heap.hprof 1

  # 方式 3: 启用 JFR 进行实时 profiling
  kubectl exec <pod> -n <namespace> -- jcmd 1 JFR.start duration=60s filename=/tmp/recording.jfr
  # 等待 60 秒
  kubectl cp <namespace>/<pod>:/tmp/recording.jfr ./recording.jfr

  # 分析工具: 
  # - Eclipse MAT (Memory Analyzer Tool) 分析 .hprof
  # - JDK Mission Control 分析 .jfr
  # - VisualVM

  # 快速检查堆内存概览
  kubectl exec <pod> -n <namespace> -- jcmd 1 GC.heap_info
  ```
- **判断规则**:
  - Old Gen 使用率持续高且 Full GC 频繁 → 内存泄漏或 -Xmx 设置过低
  - Metaspace 增长异常 → 类加载泄漏（常见于动态类加载场景）
  - 大量相同类型对象 → 缓存未清理或集合类未释放
  - byte[] 或 char[] 占用异常高 → 字符串或二进制数据缓存问题
  - **Java Heap OOM vs Container OOM**: 
    - `java.lang.OutOfMemoryError: Java heap space` → 调整 -Xmx 或优化堆使用
    - Container OOMKilled 无 Java 堆错误 → Native memory / Direct buffer / Metaspace 问题

**Step D4.3**: Python 应用 — tracemalloc / memory_profiler
- **前提**: 应用代码中已集成 tracemalloc 或可以注入 profiler
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 方式 1: 如果应用已启用 tracemalloc，连接到应用的调试接口
  # 通常需要应用提供 HTTP 端点来获取 tracemalloc snapshot

  # 方式 2: 使用 py-spy 进行内存采样（在容器内或 debug container）
  kubectl exec <pod> -n <namespace> -- pip install py-spy
  kubectl exec <pod> -n <namespace> -- py-spy record -o profile.svg --pid 1

  # 方式 3: 使用 memory_profiler（需要重启应用）
  # 在 Dockerfile 中添加: pip install memory_profiler
  # 运行: python -m memory_profiler your_script.py

  # 方式 4: 检查进程内存映射
  kubectl exec <pod> -n <namespace> -- cat /proc/1/smaps | grep -E "^(Size|Rss|Pss|Shared|Private)" | head -40
  ```
- **判断规则**:
  - Rss 持续增长 → 内存泄漏
  - 某个模块/函数分配内存异常高 → 检查该模块的数据结构
  - 大量小对象 → 考虑使用 `__slots__` 或优化数据结构
  - C 扩展内存泄漏 → 需要使用 valgrind 等工具

**Step D4.4**: Node.js 应用 — --inspect + Chrome DevTools / clinic.js
- **前提**: Node.js 应用启用了 `--inspect` 标志或可以修改启动参数
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 方式 1: 使用 --inspect 启动 Node.js并连接 Chrome DevTools
  # 确保应用以 node --inspect=0.0.0.0:9229 启动
  kubectl port-forward <pod> -n <namespace> 9229:9229 &
  # 在 Chrome 中访问 chrome://inspect，连接到 localhost:9229
  # 使用 Memory tab 进行 heap snapshot

  # 方式 2: 使用 clinic.js 进行综合诊断
  kubectl exec <pod> -n <namespace> -- npm install -g clinic
  kubectl exec <pod> -n <namespace> -- clinic heapprofiler -- node /app/index.js

  # 方式 3: 生成 heap snapshot
  kubectl exec <pod> -n <namespace> -- node -e "require('v8').writeHeapSnapshot('/tmp/heap.heapsnapshot')"
  kubectl cp <namespace>/<pod>:/tmp/heap.heapsnapshot ./heap.heapsnapshot
  # 在 Chrome DevTools 的 Memory tab 中加载分析

  # 方式 4: 检查 V8 内存统计
  kubectl exec <pod> -n <namespace> -- node -e "console.log(process.memoryUsage())"
  ```
- **判断规则**:
  - heapUsed 持续增长 → 内存泄漏
  - external 异常高 → Native 模块内存问题（如 Buffer）
  - arrayBuffers 异常高 → 二进制数据处理问题
  - 大量 Detached DOM 节点 → 前端渲染内存泄漏
  - EventEmitter listener 堆积 → 事件监听器未移除

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | 应用程序代码错误导致进程崩溃 / Application code error causing crash | 高 | D2.1 日志中出现 Exception/Panic/Fatal；Exit Code 1 | pod-fta:BE-APP-CRASH |
| RC-002 | OOMKilled — 内存 limits 设置过低 / Memory limits too low | 高 | D1.3 Exit Code 137 + D2.3.1 limits 值明显低于实际需求 + D2.3.2 正常运行内存已接近 limits | pod-fta:BE-OOM-LIMIT |
| RC-003 | OOMKilled — 应用内存泄漏 / Application memory leak | 中 | D2.3.2 内存持续增长；历史 metrics 呈锯齿形；增大 limits 后仍会 OOM（只是延迟） | pod-fta:BE-OOM-LEAK |
| RC-004 | 启动命令/参数错误 / Incorrect command or arguments | 中 | D2.2 command/args 配置与镜像不匹配；Exit Code 126/127；日志为空或仅一行 | pod-fta:BE-CMD-ERR |
| RC-005 | 依赖服务不可用导致启动失败 / Dependency service unavailable | 中 | D2.1 日志中出现 connection refused/timeout；D3.1 依赖服务不可达 | pod-fta:BE-DEP-FAIL |
| RC-006 | ConfigMap/Secret 缺失或格式错误 / Missing or malformed ConfigMap/Secret | 中 | D2.4 引用的 ConfigMap/Secret 不存在或 key 不匹配；Events 中有 FailedMount | pod-fta:BE-CONFIG-ERR |
| RC-007 | 镜像架构不匹配 / Image architecture mismatch (amd64 vs arm64) | 低 | D2.5 节点架构与镜像架构不一致；Exit Code 139（exec format error） | pod-fta:BE-ARCH-MISMATCH |
| RC-008 | 存活探针配置过于激进 / Liveness probe too aggressive | 中 | D2.6 livenessProbe 的 initialDelaySeconds < 应用启动时间；Events 中有 Unhealthy → Killing 序列 | pod-fta:BE-PROBE-AGGR |
| RC-009 | 文件系统只读或权限不足 / Read-only filesystem or insufficient permissions | 低 | D2.9 readOnlyRootFilesystem=true 且应用需写文件；D2.1 日志中有 permission denied / read-only | pod-fta:BE-FS-PERM |
| RC-010 | Init 容器失败阻塞主容器启动 / Init container failure blocking main container | 中 | D1.4 init container 状态为 CrashLoopBackOff；主容器 state 为 PodInitializing 或 Blocked | pod-fta:BE-INIT-FAIL |
| RC-011 | Java 堆内存 vs 容器内存限制不匹配 / Java heap vs container memory mismatch | 中 | D2.3 Java 应用 -Xmx 接近 limits.memory；Exit Code 137 + 日志中无 OOM 但容器级 OOMKilled | pod-fta:BE-JAVA-HEAP |
| RC-012 | PID 1 僵尸进程问题 / PID 1 zombie process issue | 低 | D3.1 `ps aux` 显示大量 zombie/defunct 进程；容器无 init 系统（如 tini/dumb-init） | pod-fta:BE-PID1-ZOMBIE |
| RC-013 | **cgroup v2 内存限制行为差异** — cgroup v1 与 v2 在内存会计、swap 处理上的差异导致意外的 OOM 行为 | ~6% | 节点使用 cgroup v2（检查 `/sys/fs/cgroup/cgroup.controllers` 是否存在）；内存限制位于 `memory.max` 而非 `memory.limit_in_bytes`；swap 行为受 `memory.swap.max` 控制 | pod-fta:BE-CGROUP-V2 |
| RC-014 | **preStop Hook 执行超时导致 SIGKILL** — preStop hook 执行时间超过 `terminationGracePeriodSeconds`，导致容器被强制杀死 | ~5% | Pod 配置了 preStop hook；Exit Code 137（SIGKILL）但 Reason 不是 OOMKilled；Events 中出现 `Killing` 且时间与 `terminationGracePeriodSeconds` 一致；日志显示 preStop 未完成 | pod-fta:BE-PRESTOP-TIMEOUT |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 调整容器内存 Limits

- **适用根因**: RC-002（limits 设置过低）
- **前置检查**:
  ```bash
  # 确认当前 limits 值
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].resources.limits.memory}'
  
  # 确认当前实际内存用量
  kubectl top pods -n <namespace> -l <label-selector> --containers
  
  # 确认 namespace 的 ResourceQuota 是否有剩余
  kubectl describe resourcequota -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方式 1: 使用 kubectl patch（推荐，可追溯）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "<new-limit>"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "<new-request>"}
  ]'
  
  # 方式 2: 使用 kubectl set resources
  kubectl set resources deployment <deployment> -n <namespace> \
    -c <container> --limits=memory=<new-limit> --requests=memory=<new-request>
  ```
  > **计算建议**: 新 limit = 当前峰值内存 × 1.5（留出 50% buffer）
  > **Java 应用**: 新 limit = -Xmx × 2（留出 Metaspace、Native Memory、Thread Stacks 空间）
- **后置验证**:
  ```bash
  # 等待新 Pod 启动
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=120s
  
  # 验证新 Pod 正常运行
  kubectl get pods -n <namespace> -l <label-selector> -o wide
  
  # 验证内存用量在新 limits 下安全
  kubectl top pods -n <namespace> -l <label-selector> --containers
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-002: 修复 ConfigMap/Secret 引用

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  # 确认缺失的 ConfigMap/Secret 名称
  kubectl get events -n <namespace> --field-selector involvedObject.name=<pod> | grep -i "mount\|configmap\|secret"
  
  # 列出 namespace 中现有的 ConfigMap
  kubectl get configmap -n <namespace>
  
  # 列出 namespace 中现有的 Secret
  kubectl get secret -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 如果是 ConfigMap 名称拼写错误，修正 Deployment 引用
  kubectl edit deployment <deployment> -n <namespace>
  # 或者创建缺失的 ConfigMap（需确认内容）
  kubectl create configmap <name> -n <namespace> --from-literal=<key>=<value>
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=120s
  kubectl get pods -n <namespace> -l <label-selector>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-003: 删除并重建异常 Pod

- **适用根因**: 一次性临时问题（如依赖服务短暂不可用后已恢复）
- **前置检查**:
  ```bash
  # 确认 Pod 由 Deployment/StatefulSet 管理（会自动重建）
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.metadata.ownerReferences[0].kind}'
  
  # 确认依赖服务已恢复
  kubectl get endpoints <dependency-service> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  kubectl delete pod <pod> -n <namespace>
  ```
- **后置验证**:
  ```bash
  # 等待新 Pod 正常启动
  kubectl get pods -n <namespace> -l <label-selector> -w --timeout=60s
  
  # 验证新 Pod 状态
  kubectl get pod -n <namespace> -l <label-selector> -o wide
  ```
- **回滚命令**:
  ```bash
  # 无需回滚（Controller 自动重建 Pod）
  # 如果新 Pod 仍然 CrashLoop，问题不是临时性的，需进一步诊断
  ```

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-004: 更新 Deployment 镜像或命令

- **适用根因**: RC-004（命令错误）, RC-007（架构不匹配）
- **影响说明**: 修改 Deployment spec 将触发滚动更新，短暂减少可用副本数
- **审批提示**: "建议将 Deployment `<deployment>` 的 image/command 从 `<old>` 更新为 `<new>`，将触发滚动更新（maxUnavailable=25%），是否批准？"
- **前置检查**:
  ```bash
  # 确认当前 image/command
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].image}'
  
  # 确认滚动更新策略
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.strategy}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 更新镜像
  kubectl set image deployment/<deployment> -n <namespace> <container>=<new-image>:<tag>
  
  # 或更新命令（需要 edit 或 patch）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/command", "value": ["/bin/sh", "-c", "<correct-command>"]}
  ]'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=180s
  kubectl get pods -n <namespace> -l <label-selector> -o wide
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-005: 调整 Liveness Probe 参数

- **适用根因**: RC-008（探针过于激进）
- **影响说明**: 修改 probe 参数将触发 Pod 重建；如果调整不当，可能无法检测到真正的不健康状态
- **审批提示**: "建议调整 Deployment `<deployment>` 的 liveness probe：增加 initialDelaySeconds 到 `<value>`s，增加 failureThreshold 到 `<value>`，是否批准？"
- **前置检查**:
  ```bash
  # 确认当前 probe 配置
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].livenessProbe}'
  
  # 确认应用实际启动时间（通过日志）
  kubectl logs <pod> -n <namespace> --previous | head -20
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 推荐: 添加 startupProbe（最佳实践）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "add", "path": "/spec/template/spec/containers/0/startupProbe", "value": {
      "httpGet": {"path": "/healthz", "port": <port>},
      "initialDelaySeconds": 10,
      "periodSeconds": 10,
      "failureThreshold": 30,
      "timeoutSeconds": 5
    }},
    {"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds", "value": 0},
    {"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/failureThreshold", "value": 3}
  ]'
  
  # 或仅调整 liveness probe 参数
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds", "value": <new-delay>},
    {"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/failureThreshold", "value": <new-threshold>},
    {"op": "replace", "path": "/spec/template/spec/containers/0/livenessProbe/timeoutSeconds", "value": <new-timeout>}
  ]'
  ```
  > **最佳实践**: 使用 `startupProbe` 处理慢启动，`livenessProbe` 仅处理运行时健康检查
  > - `startupProbe.failureThreshold × startupProbe.periodSeconds` ≥ 最大启动时间
  > - `livenessProbe.initialDelaySeconds` 可以设为 0（startupProbe 成功后才开始 liveness 检查）
- **后置验证**:
  ```bash
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=300s
  kubectl get pods -n <namespace> -l <label-selector>
  # 等待超过原来的 initialDelaySeconds，确认 Pod 不再被杀
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```

#### REM-006: 临时扩容副本数以维持服务

- **适用根因**: 任何根因（作为临时缓解措施）
- **影响说明**: 增加副本数会占用更多集群资源，确保节点有足够容量
- **审批提示**: "建议将 Deployment `<deployment>` 从 `<current>` 副本扩至 `<new>` 副本，以维持服务可用性，同时继续排查根因，是否批准？"
- **前置检查**:
  ```bash
  # 确认当前副本数和就绪数
  kubectl get deployment <deployment> -n <namespace>
  
  # 确认集群是否有足够资源
  kubectl top nodes
  ```
- **执行命令**:
  ```bash
  kubectl scale deployment <deployment> -n <namespace> --replicas=<new-count>
  ```
- **后置验证**:
  ```bash
  kubectl get deployment <deployment> -n <namespace>
  kubectl get pods -n <namespace> -l <label-selector> -o wide
  ```
- **回滚命令**:
  ```bash
  kubectl scale deployment <deployment> -n <namespace> --replicas=<original-count>
  ```

#### REM-010: 基于 Memory Profiling 的内存泄漏修复
- **适用根因**: RC-003（应用内存泄漏）
- **风险等级**: 🟡 中
- **影响说明**: 根据 profiling 结果调整应用配置或资源限制。可能需要重启应用或修改代码。短期内可以通过调整 limits 缓解，但根本修复需要应用层面修改。
- **审批提示**: "已通过 profiling 确认内存泄漏模式，建议：1) 临时增大 memory limits 到 `<new-limit>`；2) 安排应用团队修复泄漏点 `<leak-location>`。是否批准？"
- **前置检查**:
  ```bash
  # 确认内存增长模式（持续增长 vs 突增）
  # 查看最近 24h 的内存趋势（需要 Prometheus）
  # PromQL: container_memory_working_set_bytes{pod="<pod>", container="<container>"}

  # 确认 profiling 结果
  # 确保已通过 D4.1-D4.4 进行了应用级内存分析
  # 记录关键发现：泄漏点、增长速率、受影响的数据结构
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # Step 1: 临时增大 memory limits（赠送时间给应用团队修复）
  # 建议: 新 limit = 当前峰值内存 × 2（根据泄漏速率调整）
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "<new-limit>"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "<new-request>"}
  ]'

  # Step 2: 对于 Go 应用，设置 GOMEMLIMIT 限制 GC 压力
  kubectl patch deployment <deployment> -n <namespace> --type='json' -p='[
    {"op": "add", "path": "/spec/template/spec/containers/0/env/-", "value": {"name": "GOMEMLIMIT", "value": "<soft-limit>"}}
  ]'

  # Step 3: 对于 Java 应用，调整 JVM 参数
  # 在 JAVA_OPTS 中设置:
  # -Xmx<heap-size> -Xms<heap-size> -XX:MaxMetaspaceSize=<meta-size>
  # 终极缓解: 启用 NativeMemoryTracking 排查 native 内存
  # -XX:NativeMemoryTracking=summary

  # Step 4: 记录修复工单，通知应用团队
  # 包含: profiling 结果、泄漏点位置、建议的代码修复方向
  ```
- **后置验证**:
  ```bash
  # 等待新 Pod 启动
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=180s

  # 观察 24h 内存趋势
  # 使用 Prometheus/Grafana 监控 container_memory_working_set_bytes
  # 预期: 内存使用率应小于新 limits 的 70%

  # 确认无再次 OOM
  kubectl get events -n <namespace> --field-selector reason=OOMKilled --sort-by='.lastTimestamp' | tail -5

  # 检查容器重启次数
  kubectl get pod -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,RESTARTS:.status.containerStatuses[0].restartCount
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl rollout undo deployment/<deployment> -n <namespace>
  ```
- **长期修复建议**:
  - 向应用团队提供 profiling 报告和建议的修复方向
  - 对于 Go: 检查未关闭的 channel、goroutine 泄漏、全局缓存
  - 对于 Java: 检查未关闭的资源（Connection、Stream）、集合类未清理
  - 对于 Python: 检查循环引用、未关闭的文件句柄
  - 对于 Node.js: 检查 EventEmitter 监听器、未释放的 Buffer
  - 建议在 CI/CD 中集成内存泄漏检测（如 Go 的 goleak）

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-007: 回滚 Deployment 至上一版本

- **适用根因**: RC-001, RC-004, RC-007（部署变更引入的问题）
- **影响说明**: 回滚将丢弃当前版本的所有变更（包括代码、配置、镜像更新），所有 Pod 将被替换；如果有数据库 migration 已执行，可能造成不兼容
- **操作步骤**:
  1. 确认回滚目标版本:
     ```bash
     kubectl rollout history deployment/<deployment> -n <namespace>
     ```
  2. 查看目标版本的详细信息:
     ```bash
     kubectl rollout history deployment/<deployment> -n <namespace> --revision=<target-revision>
     ```
  3. 执行回滚:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     # 回滚到上一版本
     kubectl rollout undo deployment/<deployment> -n <namespace>
     
     # 或回滚到指定版本
     kubectl rollout undo deployment/<deployment> -n <namespace> --to-revision=<target-revision>
     ```
  4. 监控回滚进度:
     ```bash
     kubectl rollout status deployment/<deployment> -n <namespace> --timeout=180s
     ```
- **安全检查**:
  ```bash
  # 确认回滚后 Pod 正常运行
  kubectl get pods -n <namespace> -l <label-selector> -o wide
  
  # 确认服务端点正常
  kubectl get endpoints <service> -n <namespace>
  ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 如果回滚后仍有问题，再次回滚到之前版本
  kubectl rollout undo deployment/<deployment> -n <namespace>
  
  # 或回滚到已知良好版本
  kubectl rollout undo deployment/<deployment> -n <namespace> --to-revision=<known-good-revision>
  ```

#### REM-008: 修改 Namespace 级别 ResourceQuota

- **适用根因**: RC-002（当 ResourceQuota 阻止了 limits 调整）
- **影响说明**: 修改 ResourceQuota 会影响整个 namespace 内所有 workload 的资源配额上限，可能允许 namespace 过度消耗集群资源
- **操作步骤**:
  1. 查看当前 ResourceQuota:
     ```bash
     kubectl describe resourcequota -n <namespace>
     ```
  2. 评估是否有足够的集群资源支持增大配额:
     ```bash
     kubectl top nodes
     kubectl describe nodes | grep -A5 "Allocated resources"
     ```
  3. 修改 ResourceQuota（需 cluster-admin 或 namespace-admin 权限）:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     kubectl patch resourcequota <quota-name> -n <namespace> --type='json' -p='[
       {"op": "replace", "path": "/spec/hard/limits.memory", "value": "<new-quota>"}
     ]'
     ```
- **安全检查**:
  ```bash
  kubectl describe resourcequota -n <namespace>
  ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch resourcequota <quota-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/hard/limits.memory", "value": "<original-quota>"}
  ]'
  ```

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-009: 生产环境紧急镜像回滚

- **适用根因**: RC-001, RC-004, RC-007（P1 级别紧急情况）
- **审批要求**: 需要 P1 On-Call SRE + 服务 Owner 双重确认
- **数据备份**:
  ```bash
  # 备份当前 Deployment spec
  kubectl get deployment <deployment> -n <namespace> -o yaml > deployment-backup-$(date +%Y%m%d%H%M%S).yaml
  
  # 记录当前版本信息
  kubectl rollout history deployment/<deployment> -n <namespace>
  ```
- **操作步骤**:
  1. **停止正在进行的 rollout**:
     ```bash
     kubectl rollout pause deployment/<deployment> -n <namespace>
     ```
  2. **紧急回滚到已知良好版本**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     kubectl rollout undo deployment/<deployment> -n <namespace> --to-revision=<last-known-good>
     kubectl rollout resume deployment/<deployment> -n <namespace>
     ```
  3. **监控回滚**:
     ```bash
     kubectl rollout status deployment/<deployment> -n <namespace> --timeout=300s
     ```
  4. **验证服务恢复**:
     ```bash
     kubectl get pods -n <namespace> -l <label-selector>
     kubectl get endpoints <service> -n <namespace>
     # 确认用户流量恢复正常（通过监控面板或 curl 健康检查）
     ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 恢复到紧急回滚前的版本（使用备份文件）
  kubectl apply -f deployment-backup-<timestamp>.yaml
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1 分钟内）

```bash
# V1: 确认 Pod 状态为 Running 且 READY
kubectl get pod -n <namespace> -l <label-selector> -o wide
# 预期: STATUS=Running, READY=1/1 (或 N/N), RESTARTS 不再增长

# V2: 确认容器内存使用在安全范围（如果是 OOM 修复）
kubectl top pod -n <namespace> -l <label-selector> --containers
# 预期: MEMORY 列 < limits 的 80%

# V3: 确认无新的异常 Events
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod --sort-by='.lastTimestamp' | tail -10
# 预期: 无新的 BackOff, Killing, OOMKilling, Unhealthy 事件

# V4: 确认 Deployment 的 readyReplicas 达标
kubectl get deployment <deployment> -n <namespace> -o jsonpath='ready={.status.readyReplicas}/{.status.replicas} updated={.status.updatedReplicas}'
# 预期: ready=N/N updated=N
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pod 重启次数 | `kubectl get pod <pod> -n <ns>` RESTARTS 列 | 稳定不增长 | 5min 内再次增加 → 修复失败 |
| 容器内存使用 | `container_memory_working_set_bytes{pod="<pod>"}` | 稳定或周期性波动（GC 模式） | 持续增长且 >85% limits → 内存泄漏 |
| Pod 状态 | `kube_pod_status_phase{pod="<pod>"}` | phase=Running | phase 变为非 Running → 问题复发 |
| 应用请求成功率 | 应用的 HTTP 成功率指标（如 `http_requests_total{code=~"2.."}` / total） | 恢复到正常水平 | 成功率持续低于基线 → 可能有残余问题 |
| Endpoint 数量 | `kubectl get endpoints <service> -n <ns>` | addresses 数量 = replicas 数量 | addresses < replicas → 部分 Pod 未 Ready |

```bash
# 持续监控命令（5 分钟内观察）
kubectl get pods -n <namespace> -l <label-selector> -w
```

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] Pod 状态为 Running 且 READY，持续 5 分钟无重启
- [ ] 容器内存使用率低于 limits 的 80%（如果是 OOM 场景）
- [ ] 应用 Health Endpoint 返回 200
- [ ] 无新的 CrashLoopBackOff / OOMKilled / Unhealthy Events
- [ ] Deployment readyReplicas = desired replicas
- [ ] 关联 Service 的 Endpoints 数量正常

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pod 重启计数 | `kube_pod_container_status_restarts_total` 增速 | 每小时 | rate > 0 → 复查，可能是间歇性问题 |
| 内存增长趋势 | `container_memory_working_set_bytes` 24h 趋势 | 每 4 小时 | 持续上升斜率 → 内存泄漏，需应用层排查 |
| OOMKilled 事件 | `kube_pod_container_status_last_terminated_reason{reason="OOMKilled"}` | 持续告警 | 再次触发 → limits 仍不够或存在泄漏 |
| Pod 健康状态 | `kube_pod_status_ready{condition="true"}` | 每 15 分钟 | 值变为 0 → 问题复发 |
| 关联服务可用性 | 上游服务的错误率 / 延迟指标 | 持续告警 | 异常 → 可能有未发现的残余影响 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 升级级别 |
|------|------|---------|
| 诊断超时 | 诊断工作流执行超过 15 分钟未确认根因 | L2 → 人工 SRE |
| 修复失败 | 同一修复操作执行 2 次仍未通过 V1-V4 验证 | L2 → 高级 SRE |
| 严重性升级 | 初始分级为 P3 但影响面扩大（更多 Pod 受影响） | 重新分级 + 升级 |
| 未知根因 | 完成 Phase 1-2 诊断但无法匹配任何 RC-001 至 RC-012 | L2 → 高级 SRE |
| 多 Skill 交叉 | 诊断过程中发现问题涉及节点级别（SKILL-NODE-001）或网络（SKILL-NET-001）| 转派对应 Skill + 升级 |
| 安全相关 | 日志中发现异常进程、可疑网络连接等安全迹象 | 立即升级安全团队 |

### 8.2 升级消息模板

```
【{severity}】Pod CrashLoopBackOff/OOMKilled - {cluster_name}/{namespace}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {affected_pod_count} 个 Pod 处于 {status}，Exit Code: {exit_code}，Reason: {reason}
- 影响范围: Deployment {deployment_name}，{ready_replicas}/{total_replicas} 副本可用
- 服务影响: {customer_facing ? "客户流量受影响" : "内部服务降级"}
- 已完成诊断: {completed_diagnostic_steps}
- 初步发现: {findings_summary}
- 已尝试修复: {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 关键信息:
  - Node: {node_name}
  - Image: {container_image}
  - Memory: {current_memory_usage} / {memory_limit}
  - Restart Count: {restart_count}
- 工单编号: {ticket_id}
- Agent Skill: SKILL-POD-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下结构化信息包（Handoff Package）：

1. **诊断路径记录**:
   - 已执行的每个 Step ID 及输出摘要
   - 每步的判断结果和跳转路径
   - 诊断耗时

2. **已排除的根因**:
   - 排除的 RC-ID 及排除依据
   - 排除的置信度

3. **可能的根因假设**:
   - 最可能的 RC-ID 及置信度
   - 支持证据和不确定因素

4. **资源快照**:
   ```bash
   # Pod YAML 快照
   kubectl get pod <pod> -n <namespace> -o yaml
   
   # Deployment YAML 快照
   kubectl get deployment <deployment> -n <namespace> -o yaml
   
   # 最近 30 分钟 Events
   kubectl get events -n <namespace> --sort-by='.lastTimestamp' | tail -30
   
   # 容器日志（最后 200 行）
   kubectl logs <pod> -n <namespace> -c <container> --previous --tail=200
   ```

5. **关键时间线**:
   - 问题首次出现时间
   - 最近一次正常运行时间
   - 各诊断步骤时间戳
   - 修复尝试时间和结果

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Native Sidecar Containers (`initContainers` + `restartPolicy: Always`) | alpha (需开启 `SidecarContainers` feature gate) | beta (默认开启) | beta | GA | GA |
| Ephemeral Containers (`kubectl debug`) | GA | GA | GA | GA | GA |
| Custom Debug Profiles (`--profile`) | beta | beta | GA | GA | GA |
| In-Place Pod Vertical Scaling (`resize`) | alpha | alpha | beta | beta | beta |
| Pod Readiness Gates | GA | GA | GA | GA | GA |
| Container Restart Policy on Init Containers | alpha | beta | beta | GA | GA |
| Pod Scheduling Readiness | beta | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl debug --profile` | beta, 支持 `general`, `baseline`, `restricted` | beta, 同上 | GA, 新增 `netadmin`, `sysadmin` | GA | GA |
| `kubectl debug --custom` | 不支持 | alpha | beta | GA | GA |
| `kubectl get pod -o wide` 输出 | 标准列 | 标准列 | 新增 NOMINATED NODE 改进 | 同上 | 同上 |
| `kubectl top pod --containers` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl events` (独立子命令) | beta | GA | GA | GA | GA |

### 9.3 关键 API 版本与行为变更

| 资源/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Pod API | v1 (stable) | v1 | v1 | v1 | v1 |
| Pod `spec.initContainers[].restartPolicy` | alpha (需 feature gate) | beta | beta | GA | GA |
| `containerStatuses[].allocatedResources` (用于 resize) | alpha | alpha | beta | beta | beta |
| Pod Disruption Conditions | beta | GA | GA | GA | GA |
| Memory Swap 支持 (节点级) | alpha | beta | beta | beta | GA |

### 9.4 OOM 相关行为变更

| 行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| cgroup v2 内存控制器 | 默认(大多数发行版) | 默认 | 默认 | 默认 | 默认 |
| cgroup v1 支持 | 支持 | 支持 | 支持(弃用警告) | 支持(弃用) | 支持(弃用) |
| Memory QoS (cgroup v2 memory.high) | alpha | beta | beta | beta | GA |
| OOMKilled 事件报告 | 标准 | 标准 | 增强（包含更多上下文） | 增强 | 增强 |
| Node Swap 对 OOM 的影响 | 不支持 swap | alpha swap 支持 | beta | beta | GA |

### 9.5 Native Sidecar 对本 Skill 的影响

**[v1.28 alpha → v1.29 beta → v1.31 GA]**

Native Sidecar Containers 改变了 Init Container 的诊断逻辑：

- **传统行为**: Init containers 按顺序运行，全部成功后主容器才启动。任何 init container CrashLoop 都会阻塞。
- **[v1.28+] 新行为**: `restartPolicy: Always` 的 init container 被视为 sidecar：
  - 启动后立即允许下一个 init container 和主容器运行
  - CrashLoop 的 sidecar 不会阻塞主容器
  - 但 sidecar 功能异常（如 Istio proxy）可能导致主容器网络问题
- **诊断影响**:
  - D1.4 需要区分 "传统 init container" 和 "native sidecar"
  - Sidecar CrashLoop 可能导致 RC-005（依赖服务不可用的一种形式）
  - 检查 feature gate 是否开启: `kubectl get cm kubelet-config -n kube-system -o jsonpath='{.data.kubelet}' | grep SidecarContainers`

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| Liveness Probe 杀死慢启动应用 | Pod 反复 CrashLoop，exit code 137 (SIGKILL) 或 143 (SIGTERM)，疑似 OOM | liveness probe 的 `initialDelaySeconds` 小于应用启动时间，在应用就绪前就开始检测 | 检查 Events 中是否有 `Unhealthy` → `Killing` 序列（D2.6）；始终优先使用 `startupProbe` 而非增大 liveness `initialDelaySeconds` |
| Java Heap OOM vs Container OOM | 容器 exit code 137，日志中有 `java.lang.OutOfMemoryError: Java heap space` | 混淆两种 OOM: (1) Java Heap OOM 是 JVM 内部的，不一定导致容器被杀；(2) Container OOM 是 cgroup 级别的，由内核触发 SIGKILL | 区分: Java Heap OOM 在日志中可见 + exit code 通常为 1；Container OOMKilled 的 Reason 字段明确显示 "OOMKilled" + exit code 137。两者可能同时发生 |
| Exit Code 137 ≠ 一定是 OOM | Exit code 137，假设一定是内存不足 | 137 = SIGKILL，除了 OOM，还可能是: (1) 手动 `kubectl delete pod --force`; (2) Node 资源压力触发 Eviction 前的 kill; (3) preStop hook 超时后被强制杀 | 务必检查 `Last State → Reason` 字段: `OOMKilled` vs `Error`。如果 Reason 不是 OOMKilled，检查 Events 和节点状态 |
| 内存指标误读 | `container_memory_usage_bytes` 看起来很高，以为接近 OOM | `container_memory_usage_bytes` 包含了 page cache（可回收内存），实际 OOM 判定使用的是 `container_memory_working_set_bytes` | **始终使用 `container_memory_working_set_bytes` 作为实际内存用量指标**。公式: working_set = usage - inactive_file (可回收的 page cache) |
| ConfigMap 更新后 Pod 未重启 | 更新了 ConfigMap 内容但 Pod 仍在使用旧配置 | Kubernetes 不会因为 ConfigMap 变更而自动重启 Pod（subPath mount 甚至不会热更新） | ConfigMap 以 volume mount 的方式（非 subPath）挂载的内容会在约 60s 内自动更新，但应用需要 watch 文件变化或重新加载；使用 env 或 subPath 的必须重启 Pod |
| OOMKilled 但 `kubectl top` 显示正常 | 容器被 OOMKilled，但 `kubectl top` 显示内存用量远低于 limit | `kubectl top` 显示的是当前运行中容器的内存，OOMKilled 后容器已重启，显示的是新容器的初始内存 | 使用 Prometheus 查询历史 metrics，或检查 `kubectl describe pod` 中 Last State 的 Finished timestamp 确认 OOM 发生时间 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 参考文件路径 | 适用根因 |
|------|-----------|---------|
| Pod 生命周期与容器状态机 | `domain-4-workloads-scheduling/` | 所有根因 |
| Linux OOM Killer 机制与 cgroup 内存管理 | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/` | RC-002, RC-003, RC-011 |
| Kubernetes 资源管理 (requests/limits/QoS) | `domain-4-workloads-scheduling/` | RC-002, RC-003 |
| Container Runtime (containerd) 行为 | `domain-10-troubleshooting-diagnostics/` | RC-001, RC-004 |
| Pod FTA 故障树分析 | `domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md` | 所有根因 |
| Java 内存模型与容器化最佳实践 | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/` | RC-011 |
| Init Container 与 Sidecar 行为 | `domain-4-workloads-scheduling/` | RC-010 |
| Probe 配置最佳实践 | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/` | RC-008 |

### 10.3 关键知识点备忘

**内存指标辨析**:

| 指标 | 含义 | 是否用于 OOM 判定 | 备注 |
|------|------|-------------------|------|
| `container_memory_usage_bytes` | 容器内存总用量（含 page cache） | ❌ 不直接用于 OOM 判定 | 偏高，包含可回收内存 |
| `container_memory_working_set_bytes` | 容器工作集内存（不可回收部分） | ✅ 是 cgroup OOM 判定依据 | **使用此指标** |
| `container_memory_rss` | Resident Set Size (物理内存中的匿名页) | ⚠️ 部分参考 | 不包含 tmpfs 和 shared memory |
| `container_memory_cache` | Page cache 使用量 | ❌ 可回收 | 文件系统缓存，压力下可回收 |
| `container_spec_memory_limit_bytes` | 容器内存 limit (cgroup 限制) | — | 用于计算使用率百分比 |

**OOM 事件完整链路**:
```
容器内存增长 → 触及 cgroup memory.max (= limits.memory)
  → 内核尝试回收 page cache
  → 回收不足 → 触发 OOM Killer
  → SIGKILL (exit code 137) 发送给容器进程
  → kubelet 检测到容器退出
  → 记录 Reason: OOMKilled
  → 根据 restartPolicy 决定是否重启
  → 重启 → 如果问题未解决 → 再次 OOM → CrashLoopBackOff (指数退避)
```

**CrashLoopBackOff 退避时间表**:
```
第 1 次重启: 立即
第 2 次重启: 10s 延迟
第 3 次重启: 20s 延迟
第 4 次重启: 40s 延迟
第 5 次重启: 80s 延迟
第 6 次重启: 160s 延迟
第 7+ 次重启: 300s 延迟 (5 分钟上限)
```

### 10.4 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布 | 覆盖 CrashLoopBackOff 和 OOMKilled 两大核心场景 |

## 修复动作

> **本章定位**: 基于 Section 6 修复操作的快速决策摘要，供 Agent 在 QA 语料和运行时直接引用。

### 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-002 内存 limits 过低 | `kubectl patch deployment <deploy> -n <ns> --type='json' -p='[{"op":"replace","path":"/spec/template/spec/containers/0/resources/limits/memory","value":"<new-limit>"}]'` | 🟢 低风险（触发滚动更新） | `kubectl rollout status deployment/<deploy> -n <ns> && kubectl top pod <pod> -n <ns>` |
| RC-006 ConfigMap/Secret 缺失 | `kubectl create configmap <name> -n <ns> --from-literal=<key>=<value>` 或修正 Deployment 引用 | 🟢 低风险 | `kubectl rollout status deployment/<deploy> -n <ns>` |
| RC-003 临时问题 | `kubectl delete pod <pod> -n <ns>`（Controller 自动重建） | 🟢 低风险 | `kubectl get pods -n <ns> -l <label-selector>` |
| RC-004 命令/镜像错误 | `kubectl set image deployment/<deploy> -n <ns> <container>=<new-image>:<tag>` | 🟡 中风险（触发滚动更新） | `kubectl rollout status deployment/<deploy> -n <ns> --timeout=180s` |
| RC-008 探针过于激进 | `kubectl patch deployment <deploy> -n <ns> --type='json' -p='[{"op":"add","path":"/spec/template/spec/containers/0/startupProbe",...}]'` | 🟡 中风险（触发 Pod 重建） | `kubectl get pods -n <ns> -l <label-selector>` |
| RC-006 批量临时问题 | `kubectl scale deployment/<deploy> -n <ns> --replicas=<new-count>` | 🟡 中风险（占用更多集群资源） | `kubectl get deployment/<deploy> -n <ns>` |
| RC-003 内存泄漏（应用层） | 短期扩容 limits + 长期应用修复 | 🟡 中风险（掩盖根因） | `kubectl top pod <pod> -n <ns> --containers` |

### danger_operations 高风险操作标注

以下操作需谨慎评估影响：

```yaml
danger_operations:
  - operation: "kubectl patch deployment ... 修改 resources/limits"
    risk: "设置过高的 limits 可能导致节点资源耗尽，引发节点级 OOM 或调度失败"
    mitigation: "新 limit = 当前峰值 × 1.5；同步确认 namespace ResourceQuota 余量"

  - operation: "kubectl scale deployment --replicas=<high-count>"
    risk: "快速扩容可能耗尽集群资源，影响其他工作负载"
    mitigation: "先确认节点 CPU/内存余量: kubectl top nodes"
```

### 通用验证步骤

```bash
# 1. 确认 Pod 状态恢复正常
kubectl get pod <pod> -n <ns>

# 2. 确认无持续重启
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[0].restartCount}'

# 3. 确认内存用量在安全范围
kubectl top pod <pod> -n <ns> --containers

# 4. 确认 Deployment 滚动更新完成
kubectl rollout status deployment/<deploy> -n <ns> --timeout=120s
```

---

> **文档结束** — SKILL-POD-001 v1.0  
> 如在使用过程中发现未覆盖的根因场景或误诊模式，请通过 Skill 改进记录（Section 10.4）提交反馈。

```