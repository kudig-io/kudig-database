---
title: Pod CrashLoopBackOff & OOMKilled 诊断与修复
description: 针对 Pod CrashLoopBackOff 和 OOMKilled 两大高频故障的完整诊断技能，包含症状识别、快速分级、Exit Code 决策树、分阶段诊断流程、证据三元组、根因分类、修复操作、验证确认与升级协议
summary: 生产环境中 30-40% 的 Pod 工单为 CrashLoopBackOff/OOMKilled，本技能提供从症状识别到根因确认、修复验证的完整生产级诊断路径
category: skill
tags:
- k8s
- pod
- troubleshooting
- crashloop
- oomkilled
- exit-code
- memory
- sop
- runbook
- observability
- slo
sources:
- 故障诊断/topic-skills/02-pod-crashloop-oomkilled.md
- 故障诊断/FTA故障树/list/pod-fta.md
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/kubernetes-release-1.28/
- code/kubernetes-release-1.34/
- code/kubernetes-1.36.2/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- Pod CrashLoopBackOff 怎么排查
- OOMKilled 如何解决
- 容器反复重启什么原因
- Exit Code 137 是什么意思
- Pod 频繁崩溃怎么诊断
trigger_keywords:
- CrashLoopBackOff
- OOMKilled
- 容器崩溃
- 容器重启
- Pod重启
- exit code 137
- exit code 1
- 内存溢出
- 频繁重启
- Back-off restarting
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
skill_id: SKILL-POD-001
skill_name: Pod CrashLoopBackOff & OOMKilled 诊断与修复
version: 2.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-2 -> IE-2.1 -> BE-2.1/BE-2.3
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod CrashLoopBackOff & OOMKilled 诊断与修复

> **Skill ID**: SKILL-POD-001
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批
> **预计修复时间**: 5-20 分钟
> **FTA 路径**: TE-2 → IE-2.1 → BE-2.1 (CrashLoop) / BE-2.3 (OOMKilled)

---

## 1. 概述

**CrashLoopBackOff** 和 **OOMKilled** 是生产环境中最常见的 Pod 级别问题，占 Kubernetes 工单总量的 30-40%。

- **CrashLoopBackOff**：容器启动后立即退出，kubelet 按指数退避策略反复重启（10s→20s→40s…最长 300s）
- **OOMKilled**：容器内存使用超过 cgroup limits，被 Linux OOM Killer 终止（Exit Code 137）

**覆盖范围**：容器启动即崩溃、反复重启、内存超限被杀、Init 容器阻塞、探针误杀、镜像入口错误、架构不匹配。

**前置条件**：具备 kubectl 只读权限；了解 Pod 生命周期与容器退出码语义。

**排除边界**（不在本技能范围）：

- `STATUS=Pending`（未调度）→ 转 [02-pod-pending-scheduling.md](02-pod-pending-scheduling.md)
- `STATUS=ImagePullBackOff`（镜像拉取失败）→ 转 [03-pod-imagepull-container.md](03-pod-imagepull-container.md)

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Pod status 显示 **CrashLoopBackOff** | `kubectl get pod <pod> -n <ns>` STATUS 列 | 0.95 | 如果 Pod 处于 Pending → 转 02-pod-pending |
| S2 | RESTARTS 计数快速递增 | `kubectl get pod` RESTARTS 列，短时间内 >3 次 | 0.90 | RESTARTS=0 且 STATUS=Running → 正常 |
| S3 | 容器被 OOMKilled 终止 | `kubectl describe pod` → Last State → Reason: OOMKilled | 0.95 | Reason 为 Evicted → 节点资源压力 |
| S4 | Exit code 137（SIGKILL — 通常为 OOM） | `kubectl describe pod` → Last State → Exit Code: 137 | 0.95 | 极少情况由外部 `kill -9` 触发 |
| S5 | Exit code 1（应用程序错误） | `kubectl describe pod` → Last State → Exit Code: 1 | 0.70 | 含义取决于应用，需结合日志 |
| S6 | Exit code 2（Shell misuse / 参数错误） | `kubectl describe pod` → Last State → Exit Code: 2 | 0.65 | Bash 脚本语法错误 |
| S7 | Exit code 126/127（命令不可执行/未找到） | `kubectl describe pod` → Last State → Exit Code: 126/127 | 0.80 | 镜像 entrypoint/CMD 配置错误 |
| S8 | Init 容器陷入 CrashLoopBackOff | `kubectl describe pod` → Init Containers → Waiting | 0.85 | Init 容器问题阻塞主容器启动 |
| S9 | Events 出现 "Back-off restarting failed container" | `kubectl get events --field-selector involvedObject.name=<pod>` | 0.85 | 确认 event 时间戳为近期 |
| S10 | 容器内存使用率持续逼近 limit | `container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.9` | 0.75 | 内存泄漏早期信号 |

### 2.2 工单关键词映射

工单/告警中出现以下关键词 → 触发本技能：`CrashLoopBackOff`、`OOMKilled`、`容器反复重启`、`Exit Code 137/1`、`Back-off restarting`、`内存溢出`、`频繁重启`。

### 2.3 常见错误消息与事件日志速查

> 以下错误消息和事件日志是生产环境中 CrashLoopBackOff/OOMKilled 场景的高频诊断线索。Agent 在 Phase 1 采集 Events 后，可直接匹配本表快速路由到对应根因。

#### 关键 Events（`kubectl describe pod` / `kubectl get events`）

| 事件 Reason | 事件 Message 模式 | 含义 | 检测命令 | 路由 |
|-------------|------------------|------|---------|------|
| `BackOff` | `Back-off restarting failed container <name> in pod <pod>` | 容器反复崩溃，kubelet 进入退避循环 | `kubectl get events -n <ns> --field-selector reason=BackOff,involvedObject.name=<pod>` | → D1.2 查 Exit Code |
| `OOMKilling` | `Memory cgroup out of memory: Killed process <pid> (<process>)` | cgroup 内存超限，OOM Killer 终止进程 | `kubectl get events -n <ns> --field-selector reason=OOMKilling` | → D2.1 OOM 诊断 |
| `Unhealthy` | `Liveness probe failed: HTTP probe failed with statuscode: 503` | 存活探针失败，kubelet 将杀死容器 | `kubectl get events -n <ns> --field-selector reason=Unhealthy,involvedObject.name=<pod>` | → D2.4 探针检查 |
| `Unhealthy` | `Liveness probe failed: Get "http://<ip>:<port>/healthz": context deadline exceeded` | 探针超时（应用响应慢） | 同上 | → D2.4 探针检查 |
| `Unhealthy` | `Readiness probe failed: connection refused` | 就绪探针失败（不影响重启，但影响流量） | 同上 | → 检查应用启动顺序 |
| `Started` | `Started container <name>` | 容器成功启动（正常事件，用于确认重启时间点） | `kubectl get events -n <ns> --field-selector reason=Started` | 辅助时间线对齐 |
| `Killing` | `Stopping container <name>` | kubelet 正在停止容器（探针/驱逐/删除触发） | `kubectl get events -n <ns> --field-selector reason=Killing` | → 区分主动停止 vs 被动杀死 |
| `FailedKillPod` | `Error killing pod: failed to kill container` | kubelet 无法终止容器（运行时异常） | `kubectl get events -n <ns> --field-selector reason=FailedKillPod` | → 检查容器运行时状态 |

#### 容器 Last State 错误消息（`kubectl describe pod` → Last State）

| Reason 字段 | Exit Code | Message 模式 | 含义 | 验证方法 |
|-------------|-----------|-------------|------|----------|
| `OOMKilled` | 137 | （无 Message，Reason 即结论） | cgroup 内存超限 | `kubectl get pod -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'` |
| `Error` | 137 | （无 Message） | 非 OOM 的 SIGKILL（通常为探针超时杀死） | 结合 Events 中 `Unhealthy` 事件确认 |
| `Error` | 1 | 应用日志中的 panic/fatal/exception | 应用程序异常退出 | `kubectl logs <pod> --previous --tail=50` |
| `Error` | 2 | `Syntax error` / `unexpected token` | Shell 脚本语法错误 | `kubectl logs <pod> --previous` |
| `Error` | 126 | `Permission denied` / `cannot execute binary file` | 入口文件无执行权限或格式错误 | `kubectl exec` 检查文件权限 |
| `Error` | 127 | `not found` / `No such file or directory` | entrypoint/CMD 路径不存在 | 检查 Dockerfile ENTRYPOINT/CMD |
| `Error` | 139 | `Segmentation fault` / `exec format error` | 段错误或 CPU 架构不匹配 | `kubectl get node -o jsonpath='{.status.nodeInfo.architecture}'` |
| `Error` | 143 | （正常 SIGTERM） | 优雅终止（通常非 CrashLoop 原因） | 检查 preStop hook 和 terminationGracePeriodSeconds |
| `ContainerCannotRun` | 128 | `OCI runtime create failed` / `runc create failed` | 容器运行时无法创建容器 | 检查 SecurityContext / RuntimeClass |

#### 应用日志高频错误模式（`kubectl logs --previous`）

| 日志模式 | 含义 | 对应根因 | 修复方向 |
|---------|------|---------|----------|
| `panic: runtime error: invalid memory address or nil pointer dereference` | Go 应用空指针崩溃 | RC-001 代码 bug | 回滚 + 开发修复 |
| `java.lang.OutOfMemoryError: Java heap space` | JVM 堆内存不足 | RC-002 limit 过低 / RC-003 泄漏 | 调整 -Xmx 或增加 limit |
| `java.lang.OutOfMemoryError: Metaspace` | JVM 元空间不足 | RC-002 | 增加 -XX:MaxMetaspaceSize |
| `FATAL: could not create shared memory segment` | PostgreSQL 共享内存不足 | RC-002 | 增加 shared_buffers 或 limit |
| `Error loading shared library` / `cannot open shared object file` | 动态链接库缺失/版本不匹配 | RC-001 镜像问题 | 回滚镜像 |
| `exec format error` | 二进制架构不匹配（amd64/arm64） | RC-007 | 使用 multi-arch 镜像 |
| `Connection refused` / `Connection timed out` | 依赖服务不可达 | RC-005 | 检查依赖 Service/网络 |
| `FATAL: role "xxx" does not exist` / `password authentication failed` | 数据库凭证/配置错误 | RC-001 配置错误 | 修正 ConfigMap/Secret |
| `bind: address already in use` | 端口冲突（多容器/残留进程） | RC-001 | 检查端口配置 |
| `Killed` （无其他日志，突然中断） | 被 SIGKILL（OOM 或外部 kill） | RC-002/RC-004 | 结合 Events 区分 OOM vs 探针 |

#### 节点级 OOM 事件（dmesg / kubelet 日志）

```bash
# 🟢 低风险：只读/信息收集（需节点 SSH 权限）
# 检查节点内核 OOM Killer 日志
ssh <node-ip> "dmesg -T | grep -i 'oom\|out of memory\|killed process' | tail -20"

# 检查 kubelet 记录的 OOM 事件
ssh <node-ip> "journalctl -u kubelet --since '1 hour ago' | grep -i 'oom\|evict'"
```

| dmesg 日志模式 | 含义 | 与 cgroup OOM 的区别 |
|---------------|------|---------------------|
| `Memory cgroup out of memory: Killed process <pid>` | cgroup 级 OOM（容器超 limit） | 仅影响该容器 |
| `Out of memory: Killed process <pid>` （无 cgroup 前缀） | 节点级 OOM（整机内存耗尽） | 可能杀死任意进程，包括 kubelet |
| `oom-kill:constraint=CONSTRAINT_MEMCG` | cgroup 约束触发 | 对应 Pod Events 中 OOMKilled |
| `oom-kill:constraint=CONSTRAINT_NONE` | 无约束（节点级） | 需检查节点整体内存分配 |

---

## 3. 快速分级

在深入诊断前，先用 **≤1 分钟** 完成影响面评估与严重性分级，决定响应节奏与是否立即升级。

### 3.1 严重性分级（P0-P3）

| 级别 | 判定条件 | 响应时限 | 处置策略 |
|:---:|---------|:---:|---------|
| **P0** | 多副本/多 Deployment 同时 CrashLoop；核心服务全部不可用 | 立即 | 立即升级，怀疑集群级变更（配置/镜像仓库/节点），启动应急 |
| **P1** | 单个关键服务全部副本 CrashLoop，无健康端点 | ≤15min | 优先回滚最近变更，并行定位根因 |
| **P2** | 部分副本 CrashLoop，服务仍有健康端点（降级可用） | ≤1h | 按标准流程定位根因后修复 |
| **P3** | 单 Pod 偶发重启，RESTARTS 缓慢增长，无用户影响 | ≤1d | 排期观察，收集内存泄漏/探针证据 |

### 3.2 影响面评估命令

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 统计该 Deployment 下 CrashLoop 副本占比
kubectl get pods -n <namespace> -l app=<app> \
  -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{range .status.containerStatuses[*]}{.restartCount}{end}{"\n"}{end}'

# 是否为集群级（跨 namespace 大面积 CrashLoop → 疑似集群变更）
kubectl get pods -A --field-selector=status.phase!=Running \
  | grep -E "CrashLoopBackOff|Error" | wc -l
```

- 跨多个 namespace 大面积 CrashLoop → 判定 **P0**，优先排查集群级变更（镜像仓库不可用、准入 Webhook、节点批量异常）。

---

## 4. 诊断工作流

### Phase 1: 快速定位（只读，零风险，< 2 分钟）

**D1.1**: 获取 Pod 全局状态

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o wide
```

- **判断规则**:
  - `STATUS=CrashLoopBackOff` → 容器反复崩溃，继续 D1.2
  - `STATUS=Running` 但 `RESTARTS` 很高 → 容器可能刚好重启成功，仍需排查
  - `STATUS=Pending` → **排除此 Skill**，转 [02-pod-pending-scheduling.md](02-pod-pending-scheduling.md)
  - `STATUS=ImagePullBackOff` → **排除此 Skill**，转 [03-pod-imagepull-container.md](03-pod-imagepull-container.md)

**D1.2**: 获取容器退出码和上次状态

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.containerStatuses[*]}{"container: "}{.name}{"\n  restartCount: "}{.restartCount}{"\n  lastState: "}{.lastState.terminated.reason}{"\n  exitCode: "}{.lastState.terminated.exitCode}{"\n  finishedAt: "}{.lastState.terminated.finishedAt}{"\n"}{end}'
```

**D1.3**: Exit Code 决策树（核心路由逻辑）

> **Agent 注意**: Exit Code 137 是最关键的分支点——它在 90%+ 的情况下意味着 OOMKilled，但也可能由 liveness probe 超时触发。务必结合 `Last State → Reason` 字段确认（OOMKilled vs Error）。

| Exit Code | 含义 | 最可能根因 | 下一步 |
|-----------|------|-----------|--------|
| **137** | SIGKILL (128+9) | OOMKilled（90%+）或 liveness probe kill | → D2.1 OOM 诊断 |
| **1** | 通用应用错误 | 配置错误/依赖不可用/代码 bug | → D2.2 日志分析 |
| **2** | Shell misuse | 脚本语法错误/参数错误 | → D2.2 日志分析 |
| **126** | 命令不可执行 | 权限不足/文件格式错误 | → D2.3 镜像检查 |
| **127** | 命令未找到 | entrypoint/CMD 路径错误 | → D2.3 镜像检查 |
| **139** | SIGSEGV (128+11) | 段错误/架构不匹配 | → D2.5 架构检查 |
| **143** | SIGTERM (128+15) | 正常终止信号（通常非 CrashLoop 原因） | → 检查 preStop/优雅关闭 |

**D1.4**: 检查 Init Containers 状态

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .status.initContainerStatuses[*]}{"init:"}{.name}{" state:"}{.state}{" restarts:"}{.restartCount}{"\n"}{end}'
```

- 如果 init container 状态为 `Waiting/CrashLoopBackOff` → Init 容器问题阻塞主容器
- **[v1.28+]** 检查 Native Sidecar（`restartPolicy: Always` 的 init container）

### Phase 2: 深度检查（只读，零风险，< 10 分钟）

**D2.1**: OOMKilled 深度诊断

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 OOMKilled
kubectl describe pod <pod> -n <namespace> | grep -A 5 "Last State"

# 检查内存配置
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"container: "}{.name}{"\n  requests.memory: "}{.resources.requests.memory}{"\n  limits.memory: "}{.resources.limits.memory}{"\n"}{end}'

# 检查节点级 OOM 事件
kubectl get events -n <namespace> --field-selector reason=OOMKilling --sort-by=.lastTimestamp
```

- **判断规则**:
  - `limits.memory` 未设置 → 容器可使用节点全部内存，不会被 cgroup OOMKilled（但可能触发节点级 OOM Killer）
  - `requests.memory` >> `limits.memory` → 配置错误
  - 内存使用持续增长 → 疑似内存泄漏

**D2.2**: 应用日志分析

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看上次崩溃日志（--previous 是 CrashLoop 排查的关键）
kubectl logs <pod> -n <namespace> --previous --tail=100

# 查看当前日志
kubectl logs <pod> -n <namespace> --tail=50

# 检查特定错误模式
kubectl logs <pod> -n <namespace> --previous | grep -iE "error|fatal|panic|exception|refused|timeout|denied"
```

**D2.3**: 镜像与入口点检查

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].image}'
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].command}'
kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].args}'
```

**D2.4**: 探针配置检查

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <pod> -n <namespace> -o jsonpath='{range .spec.containers[*]}{"liveness: "}{.livenessProbe}{"\nreadiness: "}{.readinessProbe}{"\nstartup: "}{.startupProbe}{"\n"}{end}'
```

- liveness probe 超时 → 容器被 kubelet 杀死 → Exit Code 137（非 OOM，Reason=Error）
- `initialDelaySeconds` 过短 → 应用未启动完就被探测

**D2.5**: 架构不匹配检查

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node <node-name> -o jsonpath='arch={.status.nodeInfo.architecture} os={.status.nodeInfo.operatingSystem}'
```

- 节点 `arm64` 但镜像仅支持 `amd64` → Exit Code 139 + exec format error

### Phase 3: 主动探测（需审批）

**D3.1**: 临时调试容器（不改动崩溃容器本身）

```bash
# 🟡 中风险：会在集群创建临时资源，执行前确认目标 namespace 与授权
# 使用 ephemeral container 进入运行中 Pod 排查（v1.25+ GA）
kubectl debug <pod> -n <namespace> -it --image=busybox --target=<container>
```

### 4.6 证据三元组（诊断结论必须可溯源）

每个根因结论必须同时具备 **Metrics + Logs/Events** 证据，且时间窗对齐故障时刻 ±5 分钟：

```promql
# 🟢 容器重启频率判据（对应 CrashLoopBackOff）
rate(kube_pod_container_status_restarts_total{namespace="<ns>",pod="<pod>"}[15m]) > 0

# 🟢 OOMKilled 判据（对应 Exit 137 / OOM）
kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1

# 🟢 内存逼近 limit 判据（对应内存泄漏/limit 过低）
container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.9
```

| 证据维度 | 采集来源 | CrashLoop/OOM 场景取值 |
|---------|---------|----------------------|
| Metrics | Prometheus / cAdvisor | 上述 PromQL 命中 |
| Logs | `kubectl logs --previous` | OOM 场景常无应用日志（被 SIGKILL）；应用错误场景有 panic/fatal |
| Events | `kubectl get events` | `Back-off restarting failed container` / `OOMKilling` |

---

## 5. 根因分类

### 5.1 根因分类表

| RC-ID | 根因 | 概率 | 关键证据 | FTA 映射 | 修复 | 风险 |
|-------|------|------|---------|---------|------|------|
| RC-001 | 应用代码 bug / 配置错误 | 30% | logs 有 panic/fatal，Exit 1 | BE-2.1 | REM-001 回滚 | 🟡 |
| RC-002 | 内存 limit 过低 | 22% | Reason=OOMKilled，working_set≈limit | BE-2.3 | REM-002 调 limit | 🟡 |
| RC-003 | 内存泄漏 | 14% | 内存单调增长至 limit 后 OOM | BE-2.3 | REM-003 开发介入 | 🟢 |
| RC-004 | Liveness probe 配置不当 | 10% | Exit 137 但 Reason=Error，probe 超时 | BE-2.4 | REM-004 调探针 | 🟡 |
| RC-005 | 依赖服务不可用 | 8% | logs 有 refused/timeout/DNS | BE-2.5 | REM-005 修依赖 | 🟡 |
| RC-006 | 镜像 entrypoint/CMD 错误 | 5% | Exit 126/127 | BE-2.6 | REM-006 修镜像 | 🟡 |
| RC-007 | 架构不匹配 (amd64/arm64) | 3% | Exit 139 + exec format error | BE-2.7 | REM-007 multi-arch | 🟡 |
| RC-008 | Init/Sidecar 容器阻塞 | 5% | Init 容器 CrashLoop，主容器 Waiting | BE-2.8 | REM-001/依赖修复 | 🟡 |
| RC-009 | ConfigMap/Secret 挂载缺失或错误 | 3% | logs 有 config not found / mount 失败 | BE-2.9 | 修正引用 | 🟡 |

---

## 6. 修复操作

修复操作按 **四级风险** 组织：低风险可 Agent 自动执行，中风险需审批，高风险需指导，严重需高级审批。

**REM-002（🟡 中风险）：增加内存 limits**

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment <deployment> -n <namespace> -p \
  '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"limits":{"memory":"2Gi"}}}]}}}}'

# 验证
kubectl rollout status deployment/<deployment> -n <namespace>
```

**REM-001（🟡 中风险）：回滚到上一版本**

```bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout undo deployment/<deployment> -n <namespace>

# 验证
kubectl get pods -n <namespace> -l app=<app> -w
```

**REM-004（🟡 中风险）：调整探针参数**

```bash
# 🟡 中风险：调整 initialDelaySeconds / timeoutSeconds / failureThreshold
kubectl patch deployment <deployment> -n <namespace> --type='json' -p \
  '[{"op":"replace","path":"/spec/template/spec/containers/0/livenessProbe/initialDelaySeconds","value":30}]'
```

**REM-003（🟢 低风险，需开发介入）：内存泄漏取证**

采集 heapdump/pprof 证据包（不改动线上状态），升级至开发团队。

---

## 7. 验证确认

修复后必须完成 **四阶段验证闭环**，禁止"改完即认为已解决"：

| 阶段 | 检查项 | 命令/判据 | 通过标准 |
|------|-------|----------|---------|
| **即时验证** | Pod 重新拉起且不再崩溃 | `kubectl get pod -w` | STATUS=Running 且新 RESTARTS 不再增长 |
| **短期监控** | 15 分钟内无新增重启 | `rate(kube_pod_container_status_restarts_total[15m]) == 0` | 值为 0 |
| **解决标准** | 内存稳定在 limit 70% 以下 | `container_memory_working_set_bytes / limit < 0.7` | 持续 30min 达标 |
| **回归检测** | 同类工单在下一周期未复发 | 告警系统 / 工单系统 | 无复发 |

---

## 8. 升级协议

### 8.1 SLO/错误预算驱动的升级分级

结合服务 SLO 错误预算燃烧率决定升级级别（而非仅看重启次数）：

| 燃烧率 | 含义 | 升级级别 |
|:---:|------|:---:|
| ≥ 14.4x | 1 小时内耗尽 2% 月度预算 | **P0** 立即升级 |
| ≥ 6x | 6 小时燃烧窗口 | **P1** |
| ≥ 3x | 24 小时燃烧窗口 | **P2** |
| < 1x | 无 SLO 影响 | **P3** 排期 |

### 8.2 升级决策表

| 条件 | 动作 |
|------|------|
| 根因明确且可自动修复（如调整 limits） | Agent 自动执行 + 通知 |
| 需要回滚 Deployment | 人工审批后执行 |
| 疑似内存泄漏（需开发介入） | 收集证据包 → 升级至开发团队 |
| 批量 Pod 同时 CrashLoop | 立即升级 P0，检查集群级变更 |

### 8.3 交接信息包（升级时必须附带）

Pod/Namespace/Deployment 标识、Exit Code 与 Reason、`--previous` 日志片段、内存曲线截图、已执行修复动作、当前 SLO 燃烧率。

---

## 9. 版本兼容矩阵

> 下表基于 `code/kubernetes-release-1.28`、`-1.34`、`kubernetes-1.36.2` 的 `pkg/apis/core/types.go` 直接比对得出，影响 CrashLoop/OOMKilled 诊断的版本敏感点。

| 特性 / 字段 | 1.28 | 1.34 | 1.36 | 诊断影响 |
|------------|:----:|:----:|:----:|---------|
| Sidecar 容器（init container `restartPolicy: Always`） | 🅰 alpha (`SidecarContainers`) | ✅ | ✅ | init 容器可作为常驻 Sidecar；排查 CrashLoop 时需区分 init/常规容器重启语义 |
| `ContainerRestartRules` / `RestartPolicyRules`（按退出码决定重启） | ❌ | 🅰 alpha (`ContainerRestartRules`) | 🅰 alpha | 1.34+ 可配置"特定退出码才重启"，CrashLoop 判断需结合规则 |
| 原地扩缩容 `.status.resize` (`PodResizeStatus`) | 🅰 含 `Proposed`/`InProgress`/`Deferred`/`Infeasible` | ⚠️ 废弃，`Proposed` 值已移除 | ⚠️ 废弃 | 1.34+ 改查 `PodResizePending`/`PodResizeInProgress` 两个 Condition |
| Pod 级资源 `spec.resources` (`PodLevelResources`) | ❌ | 🅰 alpha | 🅰 alpha | 1.34+ 内存限制可在 Pod 级设置，OOM 诊断需同时看 Pod 级与容器级 limits |
| 容器 `StopSignal` 上报 | ❌ | 🅰 alpha (`ContainerStopSignals`) | 🅰 alpha | 可确认实际停止信号，辅助区分正常终止与异常退出 |

**诊断适配要点**：

- **D1.3 Exit Code 决策树** 在 1.18–1.36 全版本通用（Exit Code 语义由 Linux 信号决定，与 K8s 版本无关）。
- **原地扩缩容相关的 OOM 误报**：在 1.34+ 排查"扩容后仍 OOM"时，应查 `PodResizePending`/`PodResizeInProgress` 而非旧的 `.status.resize`。
- **D1.4 Native Sidecar 检查**：仅在 1.28+ 有意义；≤ 1.27 集群不存在 init 容器 `restartPolicy`。

> [存疑：Sidecar 容器从 1.28 alpha 到正式 GA 的精确版本（普遍认为 1.33 GA）因本仓库缺少 1.29/1.31/1.33 源码快照而无法直接证实，需以官方 Release Notes 核实]

完整版本矩阵见 [reference/pod-version-differences.md](reference/pod-version-differences.md)。

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊 | 现象 | 纠正 |
|------|------|------|
| Exit 137 一律判为 OOM | 忽略 Reason 字段 | 必须区分 Reason=OOMKilled vs Error（探针误杀） |
| limit 未设置却报 OOMKilled | 混淆 cgroup OOM 与节点 OOM | 无 limit 时是节点级 OOM Killer，非 cgroup |
| 只看当前日志 | 崩溃后日志已丢 | 必须用 `--previous` 看崩溃前日志 |
| 增大 limit 后仍 OOM | 忽略内存泄漏 | 内存单调增长应转 RC-003 取证，而非无限加 limit |

### 10.2 生产案例

**案例 1: 基础镜像升级导致 CrashLoopBackOff**

| 时间 | 事件 |
|------|------|
| Day 0 | 运维升级基础镜像 alpine:3.18 → alpine:3.19 |
| Day 1 | Pod CrashLoopBackOff，RESTARTS=12 |
| 诊断 | `kubectl logs --previous` 显示 `Error loading shared library libssl.so.3` |
| 根因 | alpine:3.19 升级了 OpenSSL 版本，应用依赖的 .so 文件路径变更（RC-001） |
| 修复 | 🟡 回滚基础镜像版本，等待应用适配后重新升级 |

**案例 2: JVM 堆内存超出容器 limit**

| 时间 | 事件 |
|------|------|
| 09:00 | Pod OOMKilled，Exit Code 137 |
| 诊断 | `limits.memory: 1Gi`，但 JVM `-Xmx2g` |
| 根因 | JVM 堆设置超出容器内存限制（RC-002） |
| 修复 | 🟡 调整 `-Xmx` 为容器 limit 的 70%（`-Xmx700m`），或增加 limit |

### 10.3 混沌工程验证

| 注入场景 | 注入方法（测试集群） | 应命中 | 验证标准 |
|---------|-------------------|-------|---------|
| OOMKilled | 部署内存压测容器超限 `limits.memory` | RC-002 / Exit 137 | Phase 1 即定位，Reason=OOMKilled |
| 探针误杀 | liveness `initialDelaySeconds=1` 但应用启动需 30s | RC-004 | Exit 137 但 Reason=Error |
| 镜像入口错误 | command 指向不存在路径 | RC-006 | Exit 127 |

> ⚠️ 混沌注入仅允许在专用测试集群执行，注入脚本须含自动回滚与超时熔断。

### 10.4 监控告警配置

```yaml
# Prometheus 告警规则
groups:
  - name: pod-crashloop
    rules:
      - alert: PodCrashLooping
        expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"

      - alert: ContainerOOMKilled
        expr: kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
        for: 0m
        labels:
          severity: critical
        annotations:
          summary: "容器 {{ $labels.container }} 被 OOMKilled"
```

---

## 11. 云厂商特异性（可选）

| 厂商 | 差异点 | 排查补充 |
|------|-------|---------|
| 阿里云 ACK | 节点 OOM 事件可在 SLS 日志与事件中心关联查询 | 结合 ACK 事件中心确认节点级 OOM |
| AWS EKS | Fargate Pod 无节点级 OOM Killer，仅 cgroup 限制 | Fargate 上 137 更可能是 cgroup OOM |
| GKE | Autopilot 强制 requests=limits | limit 调整受 Autopilot 约束 |

---

## 12. 自动化集成接口（可选）

### 12.1 结构化输出格式

Agent 完成诊断后按"现象 → 根因 → 修复 → 验证 → 预防"输出：

```json
{
  "skill_id": "SKILL-POD-001",
  "symptom": "CrashLoopBackOff",
  "exit_code": 137,
  "reason": "OOMKilled",
  "root_cause": "RC-002",
  "confidence": 0.92,
  "evidence": {
    "metrics": "container_memory_working_set_bytes/limit=0.99",
    "events": "OOMKilling"
  },
  "remediation": "REM-002",
  "risk": "medium",
  "requires_approval": true
}
```

### 12.2 Agent 执行边界

- 🟢 自动执行：所有 Phase 1/2 只读诊断命令
- 🟡 需审批：REM-001/002/004（修改 Deployment）
- 🔴 禁止自动：删除 Pod/Deployment、修改集群级配置

---

## 相关链接

- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]
- [[26-技能/04-工作负载/pod/reference/pod-exit-codes.md|容器退出码参考]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/pod-fta.md|Pod 异常故障树分析]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
