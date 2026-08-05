---
title: Kubernetes 生产事故响应 Runbook 模板
description: 覆盖事故严重等级定义、Incident Commander/Communicator 角色分工、War Room 流程、证据保全、沟通模板与无责复盘模板的生产级事故响应手册
summary: 覆盖事故严重等级定义、Incident Commander/Communicator 角色分工、War Room 流程、证据保全、沟通模板与无责复盘模板的生产级事故响应手册
category: production-operations
tags:
- production
- best-practices
- playbook
- incident-response
- sre
- war-room
- postmortem
- communication
- severity
- on-call
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 生产事故响应 Runbook 模板 是什么
- 事故严重等级怎么分
- Incident Commander 职责
- War Room 流程
- 事故沟通模板
- 无责复盘模板
trigger_keywords:
- incident response
- war room
- severity
- incident commander
- communicator
- postmortem
- on-call
- escalation
- communication template
prerequisites:
- kubectl-basics
- sre-practices
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 生产事故响应 Runbook 模板

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 为 Kubernetes 生产环境事故响应提供标准化模板，定义严重等级、关键角色（IC/Communicator）、War Room 运作流程、证据保全方法、内外部沟通模板以及无责复盘模板。事故响应的核心目标不是“快速背锅”，而是在最短时间内恢复服务、保护证据、控制影响范围，并通过复盘沉淀系统性改进。

---

## 1. 适用场景与范围

- **服务不可用**：API Server、核心平台组件、关键业务服务中断或严重降级。
- **安全事件**：证书泄露、特权提升、容器逃逸、挖矿、勒索软件、敏感数据外泄。
- **容量与性能事件**：节点大面积 NotReady、资源耗尽、网络分区、存储后端故障。
- **变更引发事故**：升级、发布、配置变更导致服务异常。
- **外部依赖故障**：云厂商 AZ/Region 故障、DNS、CDN、第三方 API 不可用。

---

## 2. 前置条件与工具

### 2.1 组织前提

- 已建立 on-call 值班表与升级矩阵。
- 已配置 PagerDuty/Opsgenie 告警路由。
- 已创建 Slack/飞书/Teams 事故频道模板。
- 已定义业务等级与关键用户旅程。

### 2.2 必备工具

| 工具 | 用途 |
|------|------|
| 告警平台 | PagerDuty / Opsgenie / 阿里云 ARMS |
| 通信频道 | Slack / 飞书 / Teams War Room |
| 会议桥 | Zoom / 腾讯会议 / Google Meet |
| 状态页 | Statuspage / 阿里云健康状态页 |
| 证据收集 | `kubectl logs/events`, `velero backup`, `etcd snapshot` |
| 复盘文档 | 无责复盘模板、Jira/禅道改进项跟踪 |

---

## 3. 标准操作流程

### 3.1 严重等级分类

| 等级 | 别名 | 判定标准 | 响应时间 | 通报范围 |
|------|------|----------|----------|----------|
| SEV 1 | P0 紧急 | 生产核心服务完全不可用，或数据丢失/泄露 | 5 分钟 | IC + Communicator + VP 工程 + 客服 + 安全 |
| SEV 2 | P1 高 | 生产服务严重降级，存在 workaround | 15 分钟 | IC + Communicator + 工程经理 + 客服 |
| SEV 3 | P2 中 | 非生产或部分功能受影响，无业务中断 | 30 分钟 | IC + 相关团队 |
| SEV 4 | P3 低 | 咨询、轻微缺陷、优化建议 | 按队列 | 工单/邮件 |

严重等级由第一个响应的 on-call 工程师初步判定，IC 接手可调整。等级一旦上升，必须立即触发升级。

### 3.2 角色与职责

#### Incident Commander（IC）

- **唯一决策权威**：决定缓解措施、回滚、升级、是否启动灾备。
- **任务分配**：将排查、验证、沟通任务分配给具体人员。
- **状态维护**：维护事故时间线与当前假设。
- **不直接执行操作**：优先协调，避免成为瓶颈。

#### Communicator（沟通负责人）

- **内部通报**：每 15/30 分钟在 War Room 与管理层频道同步进展。
- **外部通报**：维护状态页、客服话术、用户通知。
- **记录决策**：确保所有关键决策有文字记录。

#### Scribe（记录员）

- 记录时间线、命令、输出、决策人、恢复动作。
- 截图保存关键指标与日志。

#### SME（技术专家）

- 执行具体排查与修复命令。
- 向 IC 提供数据支撑的建议。

### 3.3 War Room 流程

#### 启动阶段（0–5 分钟）

1. on-call 工程师确认告警并创建事故频道，命名规范：`#inc-20260701-001-sev1`。
2. 指定 IC（若 on-call 工程师能力不足，立即升级）。
3. IC 召集相关 SME（网络、存储、应用、安全）。
4. 启动会议桥，要求视频/语音在线。

#### 控制阶段（5–30 分钟）

1. **止损优先**：回滚、扩容、切换流量、禁用异常特性开关。
2. **信息采集**：
   ```bash
   kubectl get nodes
   kubectl get pods -A | grep -v Running
   kubectl get events --all-namespaces --sort-by='.lastTimestamp' | tail -n 100
   kubectl logs -n <ns> deployment/<app> --tail=500 --previous
   ```
3. **根因假设**：列出 3 个最可能根因，按证据排序。
4. **每 15 分钟同步**：IC 在频道发布状态更新。

#### 恢复阶段（30 分钟–恢复）

1. 执行修复并验证。
2. 持续观察错误预算、SLO、业务指标。
3. 确认稳定后，IC 宣布服务恢复。

#### 收尾阶段（恢复后 24 小时内）

1. 收集完整证据包。
2. 48 小时内召开无责复盘。
3. 输出改进项，指定 Owner 与截止日期。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 目的 |
|--------|------|------|
| 集群整体状态 | `kubectl get nodes,pods -A` | 快速识别大面积异常 |
| 近期事件 | `kubectl get events --all-namespaces --sort-by='.lastTimestamp'` | 发现触发事件 |
| 资源压力 | `kubectl top nodes` / `kubectl top pods -A` | 识别资源耗尽 |
| 网络连通 | `kubectl run -it --rm debug --image=nicolaka/netshoot -- curl <svc>` | 验证服务可达 |
| DNS 解析 | `kubectl run -it --rm debug --image=busybox -- nslookup kubernetes.default` | 验证 CoreDNS |
| 证书状态 | `kubeadm certs check-expiration` | 排除证书过期 |
| 变更关联 | `kubectl rollout history deployment/<app> -n <ns>` | 定位最近变更 |

---

## 5. 回滚/应急方案

- **发布导致事故**：立即回滚最近发布。
  ```bash
  kubectl rollout undo deployment/<app> -n <ns>
  # 或 Argo CD
  argocd app rollback <app> <revision>
  ```
- **节点大面积 NotReady**：隔离异常节点并扩容健康节点池。
  ```bash
  kubectl cordon <node>
  kubectl drain <node> --ignore-daemonsets --force
  ```
- **证书过期**：参考 [[32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/03-control-plane/01-certificate-pki-lifecycle-runbook|Kubernetes 证书与 PKI 生命周期运维 Runbook]]。
- **安全事件**：立即隔离受感染 Pod/节点，保留镜像与日志，通知安全团队。
- **外部依赖故障**：切换 DNS/流量至备用区域或降级模式。

---

## 6. 风险与注意事项

1. **IC 必须唯一**：多人同时决策会导致误操作，必须明确指挥链。
2. **先止损后根因**：不要因根因未明而拒绝回滚，恢复服务是最高优先级。
3. **证据保全优先于恢复**：在回滚前尽量收集日志、事件、指标截图，避免现场丢失。
4. **避免疲劳决策**：SEV 1 事故超过 2 小时必须启动交接，避免 on-call 疲劳导致误判。
5. **所有操作留痕**：War Room 中的关键命令应粘贴到频道，便于复盘与审计。

---

## 7. 沟通模板

### 7.1 内部状态更新

```
[INC-20260701-001] SEV 1 - 支付服务延迟升高
状态：调查中 / 缓解中 / 已恢复
影响：支付成功率从 99.9% 降至 82%，预计影响 12k 用户
根因假设（按置信度）：1. 数据库连接池耗尽 2. 新发布引入 N+1 查询
已采取措施：回滚 v1.23.4 → v1.23.3，支付成功率回升至 96%
下一步：验证 SLO 恢复，收集数据库慢日志
IC：张三 | Communicator：李四 | 时间：2026-07-01 14:32 UTC+8
```

### 7.2 外部状态页更新

```
[监控中] 支付服务延迟升高
我们的监控系统发现支付接口响应时间出现异常。技术团队已介入排查，预计 30 分钟内提供更新。给您带来的不便敬请谅解。
```

### 7.3 恢复通知

```
[已恢复] 支付服务已恢复正常
经过回滚与扩容，支付接口成功率已恢复至 99.9% 以上。我们正在开展无责复盘，后续将通过状态页发布根因分析与改进项。
```

---

## 8. 无责复盘模板

- **事故编号**：INC-20260701-001
- **时间线**：发现时间、响应时间、止血时间、恢复时间、对外通知时间
- **影响范围**：用户数、请求失败数、收入影响、数据完整性
- **根因**：5 Whys 分析
- **触发因素**：发布、配置变更、外部依赖、容量、安全
- **缓解措施**：已采取的动作与效果
- **改进项**：
  - 可检测性改进
  - 可恢复性改进
  - 流程改进
  - 架构改进
- **Owner 与截止日期**：每项改进项指定唯一 Owner
- **经验教训**：可推广到其他团队的知识点

---

## 9. 相关 Runbook / 推荐阅读

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-11-production-operations/10-production-readiness-operations-guide|生产运维 生产就绪运维指南]]
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-11-production-operations/10-production-readiness-operations-guide|故障诊断 生产就绪运维指南]]
- [[domain-11-production-operations/事件响应/01-escalation-matrix-severity-levels.md|升级矩阵与严重等级]]
- [[domain-11-production-operations/事件响应/02-war-room-coordination-procedures.md|War Room 协调流程]]
- [[domain-11-production-operations/事件响应/03-communication-templates-stakeholder.md|事故沟通模板]]
- [[domain-11-production-operations/事件响应/04-incident-postmortem-template.md|无责复盘模板]]
- [[32-发布/package/2026-07-02_18-53/corpus/peripheral/domain-11-production-operations/02-incident-response/01-incident-response-handling|事故响应处理]]


<!-- risk-assessed -->
