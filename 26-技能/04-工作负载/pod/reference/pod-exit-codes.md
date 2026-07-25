---
title: 容器退出码完整参考与诊断映射
description: Linux/Kubernetes 容器退出码（Exit Code）的完整参考手册，包含每个退出码的含义、常见原因、诊断命令和修复方向
summary: 容器退出码是 Pod 故障诊断的核心路由依据，本参考覆盖 0-255 所有常见退出码及其在 Kubernetes 中的诊断映射
category: reference
tags:
- k8s
- pod
- exit-code
- reference
- troubleshooting
- linux
- signals
sources:
- 故障诊断/topic-skills/02-pod-crashloop-oomkilled.md
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/kubernetes-release-1.28/pkg/apis/core/types.go
- code/kubernetes-release-1.34/pkg/apis/core/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: supporting
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 容器退出码是什么意思
- Exit Code 137 代表什么
- Pod 退出码对照表
- 如何根据退出码排查问题
trigger_keywords:
- exit code
- 退出码
- 137
- 139
- 143
- SIGKILL
- SIGSEGV
- SIGTERM
- OOMKilled
prerequisites:
- linux-basics
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 容器退出码完整参考与诊断映射

## 1. 退出码获取方法

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方法 1: 通过 describe 查看
kubectl describe pod <pod> -n <ns> | grep -A 5 "Last State"

# 方法 2: 通过 jsonpath 精确获取
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}'

# 方法 3: 获取完整终止信息
kubectl get pod <pod> -n <ns> -o jsonpath='{range .status.containerStatuses[*]}{"container: "}{.name}{"\n  exitCode: "}{.lastState.terminated.exitCode}{"\n  reason: "}{.lastState.terminated.reason}{"\n  signal: "}{.lastState.terminated.signal}{"\n  finishedAt: "}{.lastState.terminated.finishedAt}{"\n"}{end}'
```

---

## 2. 退出码速查表

### 2.1 正常退出

| Exit Code | 含义 | 说明 |
|-----------|------|------|
| **0** | 成功退出 | 进程正常完成，无错误 |

### 2.2 常见错误退出码

| Exit Code | 信号 | 含义 | 常见原因 | 诊断方向 |
|-----------|------|------|---------|---------|
| **1** | — | 通用应用错误 | 配置错误/依赖不可用/代码异常/未捕获异常 | 查看应用日志 |
| **2** | — | Shell misuse | Bash 脚本语法错误/错误参数/内置命令失败 | 检查启动脚本 |
| **126** | — | 命令不可执行 | 文件权限不足/非 ELF 格式/架构不匹配 | 检查文件权限和格式 |
| **127** | — | 命令未找到 | entrypoint/CMD 路径错误/依赖库缺失 | 检查镜像构建 |
| **128** | — | 无效退出参数 | `exit` 命令使用了非法参数 | 检查脚本逻辑 |
| **130** | SIGINT (2) | Ctrl+C 中断 | 用户中断/优雅关闭 | 通常非异常 |
| **132** | SIGILL (4) | 非法指令 | CPU 架构不兼容/二进制损坏 | 检查镜像平台 |
| **134** | SIGABRT (6) | 进程自终止 | `abort()` 调用/断言失败/堆栈溢出 | 查看 core dump |
| **137** | SIGKILL (9) | 强制终止 | **OOMKilled（90%+）**/liveness probe kill/手动 kill -9 | 检查内存 + 探针 |
| **139** | SIGSEGV (11) | 段错误 | 空指针/架构不匹配/内存越界 | 检查架构兼容性 |
| **143** | SIGTERM (15) | 优雅终止 | Pod 删除/滚动更新/preStop | 通常正常 |
| **255** | — | 退出码越界 | 脚本返回了 >255 的值 | 检查脚本 |

### 2.3 Kubernetes 特定 Reason

| Reason | 对应 Exit Code | 含义 | 诊断 |
|--------|---------------|------|------|
| **OOMKilled** | 137 | 容器内存超过 cgroup limit | 增加 limits / 修复泄漏 |
| **Error** | 非 0 | 通用错误退出 | 查看日志 |
| **Completed** | 0 | 正常完成 | 无需处理 |
| **ContainerCannotRun** | — | 容器无法启动 | 检查镜像/运行时 |

---

## 3. Exit Code 137 深度分析

Exit Code 137 = 128 + 9 (SIGKILL)，是生产环境中最高频的异常退出码。

### 3.1 触发场景

| 场景 | 概率 | 确认方法 |
|------|------|---------|
| **OOMKilled** | 90%+ | `kubectl describe pod` → Reason: OOMKilled |
| **Liveness Probe 超时** | 5% | Events: "Liveness probe failed" + "Killing container" |
| **手动 kill -9** | 3% | 审计日志/操作记录 |
| **节点 OOM Killer** | 2% | `dmesg | grep -i oom` 节点级日志 |

### 3.2 诊断命令

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认是否为 OOMKilled
kubectl describe pod <pod> -n <ns> | grep -B2 -A5 "Last State"

# 检查内存配置
kubectl get pod <pod> -n <ns> -o jsonpath='{range .spec.containers[*]}{"limits.memory: "}{.resources.limits.memory}{"\n"}{end}'

# 检查内存使用趋势（Prometheus）
# container_memory_working_set_bytes{pod="<pod>", namespace="<ns>"}

# 检查节点级 OOM 事件
kubectl get events -n <ns> --field-selector reason=OOMKilling
```

---

## 4. Exit Code 决策树

```
容器退出
    │
    ├── Exit Code = 0 → 正常退出，非故障
    │
    ├── Exit Code = 1 → 应用层错误
    │       └── 查看日志: kubectl logs --previous
    │
    ├── Exit Code = 2 → Shell/参数错误
    │       └── 检查启动命令和脚本
    │
    ├── Exit Code = 126 → 权限/格式问题
    │       └── 检查文件权限: ls -la /entrypoint.sh
    │
    ├── Exit Code = 127 → 命令不存在
    │       └── 检查 PATH 和镜像内容
    │
    ├── Exit Code = 137 → SIGKILL
    │       ├── Reason: OOMKilled → 内存问题
    │       │       ├── limits 过低 → 增加 limits
    │       │       └── 内存泄漏 → 开发介入
    │       └── 非 OOM → Liveness Probe 超时
    │               └── 调整探针配置
    │
    ├── Exit Code = 139 → SIGSEGV 段错误
    │       ├── 架构不匹配 → 使用正确平台镜像
    │       └── 应用 bug → 开发介入
    │
    └── Exit Code = 143 → SIGTERM 正常终止
            └── 通常非异常（Pod 删除/更新）
```

---

## 5. 信号编号对照表

| 信号编号 | 信号名 | Exit Code (128+N) | 默认行为 |
|---------|--------|-------------------|---------|
| 1 | SIGHUP | 129 | 终端挂起 |
| 2 | SIGINT | 130 | 中断（Ctrl+C） |
| 3 | SIGQUIT | 131 | 退出并 core dump |
| 4 | SIGILL | 132 | 非法指令 |
| 6 | SIGABRT | 134 | 异常终止 |
| 9 | SIGKILL | 137 | 强制终止（不可捕获） |
| 11 | SIGSEGV | 139 | 段错误 |
| 13 | SIGPIPE | 141 | 管道破裂 |
| 14 | SIGALRM | 142 | 定时器超时 |
| 15 | SIGTERM | 143 | 优雅终止（可捕获） |

---

## 6. 版本差异说明

> 以下标 ✅ 的条目经 `code/` 目录 `pkg/apis/core/types.go` 源码直接证实；标 [存疑] 的条目因本仓库缺少对应版本快照或为 kubelet 行为（非 API 层）而无法直接证实。

| Kubernetes 版本 | 变更 | 证据状态 |
|----------------|------|---------|
| v1.28+ | Native Sidecar（init container 带 `restartPolicy: Always`）— 其退出不影响主容器；types.go 中 init 容器新增 `RestartPolicy *ContainerRestartPolicy`（`+featureGate=SidecarContainers`） | ✅ 1.28 源码证实 |
| v1.34+ | 容器 `StopSignal`（`+featureGate=ContainerStopSignals`）— 可自定义停止信号，影响 SIGTERM(143)/SIGKILL(137) 的实际触发来源 | ✅ 1.34 源码证实 |
| v1.34+ | `ContainerRestartRules`（按退出码决定是否重启）— 特定 Exit Code 可配置为不触发 CrashLoop | ✅ 1.34 源码证实 |
| v1.29+ | `terminationGracePeriodSeconds` 相关行为优化 | [存疑：此版本行为变更为 kubelet 层逻辑且本仓库无 1.29 快照，无法直接核实] |
| v1.30+ | OOM 事件的 Event 记录更详细（包含内存使用量） | [存疑：此为 kubelet/事件记录行为且本仓库无 1.30 快照，需以实际集群 Event 输出核实] |

**退出码本身的版本无关性**：本文 0–255 退出码及信号映射（128+N）由 Linux 内核与 POSIX 信号规范决定，在 Kubernetes 1.18–1.36 全版本一致，与集群版本无关。版本差异仅影响退出码的**上报方式与重启策略**，不影响退出码含义本身。

完整版本矩阵与字段演进见 [pod-version-differences.md](pod-version-differences.md)。

---

## 相关链接

- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff & OOMKilled 诊断]]
- [[26-技能/04-工作负载/pod/04-pod-sop-runbook.md|Pod SOP/Runbook]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
