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
- **证书过期**：参考 [[01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook.md|Kubernetes 证书与 PKI 生命周期运维 Runbook]]。
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

## 9. K8s 特定事故场景 Runbook

### 场景 A: 控制平面不可用

```bash
# 🟢 快速诊断
# 1. API Server 状态
kubectl get --raw /healthz?verbose
kubectl -n kube-system get pod -l component=kube-apiserver

# 2. etcd 状态
kubectl -n kube-system exec etcd-master-0 -- etcdctl endpoint health --cluster
kubectl -n kube-system exec etcd-master-0 -- etcdctl endpoint status --cluster -w table

# 3. 证书检查
kubeadm certs check-expiration

# 🟡 应急措施
# API Server 无法启动: 检查静态 Pod 配置
kubectl -n kube-system logs kube-apiserver-master-0 --tail=100

# etcd 数据损坏: 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restore
```

### 场景 B: 节点大面积 NotReady

```bash
# 🟢 诊断
kubectl get nodes | grep -v Ready
kubectl describe node <node> | grep -A 5 Conditions

# 检查 kubelet 状态
kubectl debug node/<node> -it --image=busybox -- \
  chroot /host systemctl status kubelet

# 检查 CNI
kubectl -n kube-system get pod -l k8s-app=calico-node -o wide | grep <node>
kubectl -n kube-system logs -l k8s-app=calico-node --tail=50

# 🟡 应急: 隔离故障节点
kubectl cordon <node>
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --timeout=300s

# 扩容健康节点
# 云厂商: 增加节点池实例数
```

### 场景 C: DNS 解析失败

```bash
# 🟢 诊断
kubectl -n kube-system get pod -l k8s-app=kube-dns
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=50

# 测试 DNS 解析
kubectl run dns-test --rm -it --restart=Never --image=busybox:1.36 -- \
  nslookup kubernetes.default.svc.cluster.local

# 检查 CoreDNS 配置
kubectl -n kube-system get configmap coredns -o yaml

# 🟡 应急: 重启 CoreDNS
kubectl -n kube-system rollout restart deployment/coredns

# 检查 NodeLocal DNSCache
kubectl -n kube-system get ds node-local-dns
```

### 场景 D: 存储卷挂载失败

```bash
# 🟢 诊断
kubectl get pvc -A | grep -v Bound
kubectl describe pvc <pvc> -n <ns>

# 检查 CSI 驱动
kubectl get csidrivers
kubectl -n kube-system get pod -l app=csi-provisioner

# 检查 PV 状态
kubectl get pv | grep -v Available | grep -v Bound

# 🟡 应急: 强制删除 Terminating PVC
kubectl patch pvc <pvc> -n <ns> --type='json' \
  -p='[{"op": "remove", "path": "/metadata/finalizers"}]'
```

---

## 10. 事故度量与 KPI

### 核心指标

| 指标 | 定义 | 目标 | 计算 |
|------|------|------|------|
| MTTD | 平均发现时间 | < 5 分钟 | 告警触发 - 故障发生 |
| MTTA | 平均响应时间 | < 10 分钟 | IC 接手 - 告警触发 |
| MTTR | 平均恢复时间 | < 60 分钟 | 服务恢复 - 故障发生 |
| 事故频率 | 每月 SEV1/2 数量 | 下降趋势 | 月度统计 |
| 重复率 | 同根因重复发生 | < 5% | 复盘跟踪 |
| 改进项完成率 | 复盘改进项按时关闭 | > 90% | Jira 跟踪 |

### 事故报告模板

```markdown
## 月度事故报告 - 2026-XX

### 概览
- SEV1: X 起 | SEV2: X 起 | SEV3: X 起
- MTTR: XX 分钟 (上月: XX)
- 可用性: 99.XX%

### 事故列表
| 编号 | 等级 | 摘要 | MTTR | 根因类别 |
|------|------|------|------|----------|
| INC-001 | SEV1 | xxx | 45min | 发布 |

### 根因分布
- 发布变更: 40%
- 容量: 25%
- 外部依赖: 20%
- 配置错误: 15%

### 改进项跟踪
| 改进项 | Owner | 截止日期 | 状态 |
|--------|-------|----------|------|
| xxx | @xx | 2026-XX-XX | 进行中 |
```

---

## 11. 事故演练机制

### GameDay 计划

| 频率 | 演练内容 | 目标 |
|------|----------|------|
| 每月 | 单场景演练（DNS/存储/网络） | 验证 Runbook |
| 每季度 | 全链路事故模拟 | 验证响应流程 |
| 每半年 | 混沌工程 + 事故响应 | 发现未知风险 |

### 演练检查单

```markdown
## GameDay 演练记录

- **日期**: 2026-XX-XX
- **场景**: CoreDNS 完全不可用
- **注入方式**: kubectl scale deployment/coredns --replicas=0 -n kube-system
- **预期 MTTD**: < 2 分钟
- **实际 MTTD**: X 分钟
- **预期 MTTR**: < 10 分钟
- **实际 MTTR**: X 分钟
- **发现问题**:
  1. 告警延迟 3 分钟
  2. Runbook 缺少 NodeLocal DNS 检查步骤
- **改进项**:
  1. 优化 CoreDNS 告警阈值
  2. 更新 Runbook
```

---

## 9. 相关 Runbook / 推荐阅读

- [[13-生产运维/00-总览/01-production-readiness-operations-guide.md|生产运维 生产就绪运维指南]]
- [[19-故障诊断/00-总览/02-production-readiness-operations-guide.md|故障诊断 生产就绪运维指南]]
- [[13-生产运维/03-事件响应/01-escalation-matrix-severity-levels.md|升级矩阵与严重等级]]
- [[13-生产运维/03-事件响应/02-war-room-coordination-procedures.md|War Room 协调流程]]
- [[13-生产运维/03-事件响应/03-communication-templates-stakeholder.md|事故沟通模板]]
- [[12-可靠性/05-事后复盘/03-incident-postmortem-template|无责复盘模板]]
- [[13-生产运维/03-事件响应/10-incident-response-handling.md|事故响应处理]]


<!-- risk-assessed -->
