---
title: Pod 异常诊断标准操作流程（SOP/Runbook）
description: Pod 异常诊断的标准化操作流程，包含分状态 SOP、Runbook 模板、升级决策矩阵、证据采集规范及 FTA+FEBM 联合诊断流程
summary: 将 Pod 异常诊断从经验驱动转为流程驱动，提供可重复执行的标准化操作步骤和升级决策依据
category: skill
tags:
- k8s
- pod
- sop
- runbook
- troubleshooting
- incident-response
- escalation
- evidence
sources:
- 故障诊断/高级排障/structural-symptom-mapping-layer.md
- 故障诊断/高级排障/43-symptom-sop-mapping.md
- 故障诊断/FEBM方法论/08-febm-production-quick-start.md
- 概念/Production Troubleshooting Playbook.md
- code/kubernetes-release-1.28/pkg/apis/core/types.go
- code/kubernetes-release-1.34/pkg/apis/core/types.go
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 值班人员
estimated_read_time: 15min
intent_queries:
- Pod 故障排查的标准流程是什么
- Pod 异常应该怎么处理
- 值班遇到 Pod 问题怎么操作
- Pod 故障升级标准是什么
trigger_keywords:
- SOP
- Runbook
- 操作流程
- 标准流程
- 升级
- 值班
- 应急响应
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
skill_id: SKILL-POD-004
skill_name: Pod 异常诊断标准操作流程
version: 1.0.0
agent_execution_mode: L1-guided
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Pod 异常诊断标准操作流程（SOP/Runbook）

> **Skill ID**: SKILL-POD-004
> **适用角色**: 值班 SRE、技术支持、AI Agent
> **响应时间要求**: P0 < 5min | P1 < 15min | P2 < 1h

---

## 1. 通用诊断 SOP（所有 Pod 异常适用）

### 1.1 信息收集（0-2 分钟）

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 确认 Pod 状态
kubectl get pod <pod> -n <ns> -o wide

# Step 2: 查看事件（关键！）
kubectl describe pod <pod> -n <ns> | tail -20
kubectl get events -n <ns> --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp

# Step 3: 查看日志
kubectl logs <pod> -n <ns> --previous --tail=50

# Step 4: 检查退出码
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}'
```

### 1.2 症状识别与路由总表

| # | 症状描述（错误消息/事件原文） | 检测方法 | 置信度 | 排除条件 | 路由 |
|---|---------------------------|---------|:---:|---------|------|
| S1 | STATUS `Pending` + Events `FailedScheduling` | `kubectl get pod` / `describe` Events | 0.95 | 节点批量 NotReady → 转 Node 技能集 | → SOP-2 |
| S2 | STATUS `CrashLoopBackOff` / Events `Back-off restarting failed container` | `kubectl get pod` RESTARTS 递增 | 0.95 | Init 容器崩溃属 Init 分支 | → SOP-3 |
| S3 | Last State `OOMKilled` / Exit Code `137` | `kubectl describe pod` Last State | 0.95 | 节点 MemoryPressure 驱逐属 SOP-5 | → SOP-3 |
| S4 | STATUS `ImagePullBackOff` / Events `Failed to pull image` / `unauthorized` | `kubectl describe pod` Events | 0.95 | 节点 DiskPressure 导致拉取失败 → Node 技能集 | → SOP-4 |
| S5 | STATUS `Evicted` / Events `Evicted: The node was low on resource` | `kubectl get pods -A \| grep Evicted` | 0.90 | 手动 drain/污点驱逐属运维操作 | → SOP-5 |
| S6 | STATUS `Terminating` 超时 + finalizer 残留 | `kubectl get pod -o jsonpath='{.metadata.finalizers}'` | 0.90 | 节点失联导致的 Terminating → 先排节点 | → SOP-6 |
| S7 | Running 但 `Ready=False` / Events `Readiness probe failed` | `kubectl describe pod` Conditions/Events | 0.90 | 应用启动慢属探针参数问题而非故障 | → SOP-7 |
| S8 | Events `FailedMount` / `FailedAttachVolume`（存储类） | `kubectl describe pod` Events | 0.90 | → 转存储技能集（csi-storage） | 跨域转诊 |
| S9 | Events `FailedCreatePodSandBox`（CNI 网络类） | `kubectl describe pod` Events | 0.90 | → 转网络技能集/节点 CNI | 跨域转诊 |
| S10 | Events `admission webhook ... denied the request`（安全类） | 创建失败报错文本 | 0.95 | → 转安全/集群技能集（webhook-admission） | 跨域转诊 |

---

## 2. SOP-2: Pod Pending

**触发条件**: Pod 状态为 Pending 超过 30 秒

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 确认状态
kubectl get pod <pod> -n <ns> -o wide

# Step 2: 查看事件（关键！事件中有调度失败原因）
kubectl describe pod <pod> -n <ns> | tail -20
kubectl get events -n <ns> --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp

# Step 3: 根据事件分支诊断
```

| 事件关键词 | 根因 | 修复方向 |
|-----------|------|---------|
| `Insufficient cpu/memory` | 节点资源不足 | 扩容节点 / 调整 requests |
| `node(s) didn't match selector` | NodeSelector 不匹配 | 检查标签 / 修改选择器 |
| `node(s) had taint` | Taint/Toleration 不匹配 | 添加 Toleration / 移除 Taint |
| `persistentvolumeclaim not found` | PVC 未绑定 | 检查 PVC 状态和 StorageClass |
| `Unschedulable` | 节点不可调度 | 检查节点 SchedulingDisabled |

---

## 3. SOP-3: CrashLoopBackOff / OOMKilled

**触发条件**: Pod 状态为 CrashLoopBackOff 或 Last State 为 OOMKilled

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看上一次日志（关键！）
kubectl logs <pod> -n <ns> --previous --tail=100

# Step 2: 检查容器退出码
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}'

# Step 3: 检查探针配置
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].livenessProbe}'
```

| Exit Code | 含义 | 下一步 |
|-----------|------|--------|
| 137 | OOMKilled / SIGKILL | 检查内存 limits，增加或修复泄漏 |
| 1 | 应用错误 | 分析日志，修复配置/代码 |
| 126/127 | 命令问题 | 检查镜像 entrypoint |
| 139 | 段错误 | 检查架构兼容性 |

---

## 4. SOP-4: ImagePullBackOff

**触发条件**: Pod 状态为 ImagePullBackOff 或 ErrImagePull

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查镜像名称和 tag
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].image}'

# Step 2: 检查 imagePullSecrets
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.imagePullSecrets}'
kubectl get secret <secret> -n <ns> -o jsonpath='{.type}'

# Step 3: 检查事件中的详细错误
kubectl describe pod <pod> -n <ns> | grep -A 5 "Failed"
```

| 错误信息 | 根因 | 修复 |
|---------|------|------|
| `repository does not exist` | 镜像名称错误 | 核实镜像地址 |
| `unauthorized` | 认证失败 | 检查 imagePullSecret |
| `manifest unknown` | Tag 不存在 | 核实 Tag 是否已推送 |
| `timeout` | 网络不通 | 检查节点到 Registry 的网络连通性 |

---

## 5. SOP-5: Pod Evicted

**触发条件**: Pod 状态为 Evicted

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看驱逐原因
kubectl describe pod <pod> -n <ns> | grep -A 5 "Message"

# Step 2: 检查节点资源压力
kubectl describe node <node> | grep -A 5 "Conditions"

# Step 3: 检查节点资源使用
kubectl top node <node>
```

| 驱逐原因 | 修复 |
|---------|------|
| MemoryPressure | 扩容节点/提高 Pod QoS 等级 |
| DiskPressure | 清理镜像和日志/扩容磁盘 |
| PIDPressure | 检查进程泄漏/调整 PID limit |

---

## 6. SOP-6: Pod Terminating 卡住

**触发条件**: Pod 处于 Terminating 超过 5 分钟

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查 Finalizers
kubectl get pod <pod> -n <ns> -o jsonpath='{.metadata.finalizers}'

# Step 2: 检查 Volume 卸载
kubectl describe pod <pod> -n <ns> | grep -i volume

# Step 3: 检查 preStop hook
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].lifecycle.preStop}'
```

**修复（高风险，需审批）**:
```bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl delete pod <pod> -n <ns> --grace-period=0 --force
```

---

## 7. SOP-7: Pod Ready=False

**触发条件**: Pod Running 但 Ready 为 0/1

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查 readiness probe
kubectl describe pod <pod> -n <ns> | grep -A 10 "Readiness"

# Step 2: 手动测试探针端点
kubectl exec <pod> -n <ns> -- curl -s localhost:<probe-port><probe-path>

# Step 3: 检查应用日志
kubectl logs <pod> -n <ns> --tail=30
```

---

## 8. 升级决策矩阵

| 级别 | 条件 | 响应时间 | 动作 |
|------|------|---------|------|
| **P0** | 核心服务 Pod 全部不可用 | < 5 min | 立即回滚 + On-call SRE + Tech Lead |
| **P1** | 部分 Pod 异常，服务降级 | < 15 min | 按 SOP 诊断 + On-call SRE |
| **P2** | 单个 Pod 偶发重启 | < 1 h | 记录工单 + 优化配置 |

### 升级触发条件

- SOP 步骤执行完毕但根因不明 → 升级
- 需要执行 🔴 高风险操作 → 升级审批
- 批量 Pod 同时异常 → 立即升级 P0
- 诊断耗时超过 30 分钟 → 升级

---

## 9. 证据采集规范

### P0 必须采集

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Pod YAML 快照
kubectl get pod <pod> -n <ns> -o yaml > /incident/pod-snapshot.yaml

# Deployment YAML 快照
kubectl get deployment <deploy> -n <ns> -o yaml > /incident/deploy-snapshot.yaml

# 最近 30 分钟 Events
kubectl get events -n <ns> --sort-by='.lastTimestamp' | tail -30 > /incident/events.txt

# 容器日志（最后 200 行）
kubectl logs <pod> -n <ns> -c <container> --previous --tail=200 > /incident/logs-previous.txt
```

### P1 高价值

- Prometheus 指标趋势（问题前后 1 小时）
- K8s 审计日志（配置变更）
- 网络连通性测试
- 资源使用率（CPU/内存/磁盘）

---

## 10. FTA + FEBM 联合诊断流程

```
0-5 分钟（FTA 阶段）:
├── 收集基础症状（pod 状态、日志最后 20 行、退出码）
├── 匹配 FTA 决策树，找到最可能的 2-3 个根因
├── 执行快速验证命令（每个根因 < 1 分钟）
├── 如果根因明确 → 执行标准处置 → 结束
└── 如果根因不明 → 启动 FEBM 取证

5-30 分钟（FEBM 阶段）:
├── 根据 FTA 缩小的范围，采集针对性证据
├── 重建事件时间线（精确到分钟）
├── 跨层关联证据（应用+网络+系统）
├── 形成完整证据链
└── 确认根因并实施修复

30+ 分钟（反馈阶段）:
├── 将新发现的模式添加到 FTA
├── 更新 Runbook 库
├── 编写事后分析报告（Postmortem）
└── 识别预防措施并跟踪实施
```

---

## 11. Runbook 模板

```yaml
runbook:
  id: "RB-POD-001"
  title: "Pod CrashLoopBackOff 快速恢复"
  trigger:
    symptom: "Pod CrashLoopBackOff"
    confidence_min: 0.85
  check_items:
    - "kubectl describe pod {ns}/{pod} | grep -A5 'Last State'"
    - "kubectl top pod {ns}/{pod} --containers"
    - "kubectl logs {ns}/{pod} --previous"
  fix_steps:
    - step: 1
      action: "确认 OOMKilled 后，增加内存 limit"
      risk: "medium"
    - step: 2
      action: "如非 OOM，检查应用配置错误"
      risk: "low"
  rollback: "kubectl rollout undo deployment/{deployment} -n {ns}"
  validation:
    - "Pod 状态为 Running"
    - "重启次数不再增长"
    - "应用日志无新错误"
    - "健康检查通过"
```

---

## 版本差异与 SOP 适配（基于 code/ 源码实证）

> 基于 `code/kubernetes-release-1.28`、`-1.34` 的 `pkg/apis/core/types.go` 比对，影响 SOP 证据采集与升级决策的版本敏感点。

| 特性 / Condition | 1.28 | 1.34 | 1.36 | 对 SOP 的影响 |
|------------------|:----:|:----:|:----:|--------------|
| `DisruptionTarget` Pod Condition | ✅ | ✅ | ✅ | 排查 Terminating/被驱逐时，可通过该 Condition 区分抢占/驱逐/GC 来源 |
| `.status.resize` (`PodResizeStatus`) | 🅰 | ⚠️ 废弃 | ⚠️ 废弃 | 原地扩缩容证据采集：1.34+ 改采 `PodResizePending`/`PodResizeInProgress` Condition |
| 容器 `StopSignal` 上报 | ❌ | 🅰 alpha | 🅰 alpha | Terminating 排查时可确认实际停止信号来源 |
| `SchedulingGates`/`SchedulingGated` | 🅱 | ✅ GA | ✅ | Pending 升级决策：需区分"被门控阻塞"与"真实调度失败" |

**证据采集适配要点**：

- 本 SOP 的证据采集命令（`kubectl describe`、`kubectl logs --previous`、`kubectl get events`）在 1.18–1.36 全版本通用。
- 采集扩缩容证据时：≤ 1.32 查 `.status.resize`；1.34+ 改查两个 Resize Condition。
- FTA+FEBM 联合诊断流程与升级决策矩阵与 K8s 版本无关，全版本适用。

> [存疑：各 Condition/字段的精确毕业版本因仓库缺少部分中间版本快照而仅能根据相邻快照推断，实际以目标集群 `kubectl version` 与官方 Release Notes 为准]

完整版本矩阵见 [reference/pod-version-differences.md](reference/pod-version-differences.md)。

---

## SLO/错误预算驱动的升级分级

所有 Pod 异常 SOP 的升级决策统一按 SLO 错误预算燃烧率分级：

| 燃烧率 | 含义 | 升级级别 |
|:---:|------|:---:|
| ≥ 14.4x | 1 小时内耗尽 2% 月度预算 | **P0** 立即升级 |
| ≥ 6x | 6 小时燃烧窗口 | **P1** |
| ≥ 3x | 24 小时燃烧窗口 | **P2** |
| < 1x | 无 SLO 影响 | **P3** 排期 |

## 证据三元组（SOP 通用）

每个 SOP 分支的结论均需同时附 Metrics + Logs/Events 证据，且时间窗对齐故障时刻 ±5 分钟：

```promql
# 🟢 重启频率
rate(kube_pod_container_status_restarts_total[15m]) > 0
# 🟢 OOMKilled
kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
# 🟢 Pending / 拉取失败
kube_pod_status_phase{phase="Pending"} == 1
```

---

## 相关链接

- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/01-pod-crashloop-oomkilled.md|CrashLoopBackOff 诊断]]
- [[26-技能/04-工作负载/pod/02-pod-pending-scheduling.md|Pod Pending 诊断]]
- [[26-技能/04-工作负载/pod/03-pod-imagepull-container.md|镜像拉取诊断]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[kube-scheduler]] — kube-scheduler
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]
