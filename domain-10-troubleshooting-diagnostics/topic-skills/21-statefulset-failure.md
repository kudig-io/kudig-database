---
title: StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
description: '- 运维工程师'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- kubelet
- controller-manager
- coredns
- ceph
- redis
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation 是什么
- 如何 StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation 故障排查
- StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation 排障步骤
trigger_keywords:
- StatefulSet
- 故障诊断与修复
- StatefulSet
- Failure
- Diagnosis
- Remediation
- troubleshooting
- diagnostics
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
skill_id: SKILL-21_STATEFULSET_FAILURE-001
skill_name: StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
version: 1.0.0
---

---
skill_id: "SKILL-WORK-002"
skill_name: "StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation"
version: "1.0"
category: "workload"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "15-60min"
risk_level: "high"
agent_execution_mode: "L1-advisory"
trigger_keywords:
  - "StatefulSet"
  - "statefulset"
  - "sts"
  - "PVC pending"
  - "pod not starting"
  - "ordinal"
  - "headless service"
  - "有状态集"
  - "数据库集群"
  - "kafka"
  - "zookeeper"
  - "mysql"
  - "mongodb"
trigger_events:
  - "FailedCreate"
  - "FailedDelete"
  - "SuccessfulDelete"
  - "RecreatingFailedPod"
trigger_metrics:
  - 'kube_statefulset_status_replicas{status="not_ready"}'
  - 'kube_statefulset_replicas - kube_statefulset_status_replicas_ready'
  - 'kube_persistentvolumeclaim_status_phase{phase="Pending"}'
  - 'kube_pod_status_phase{phase!="Running"}'
difficulty: "advanced"
reading_level: "advanced"
audience:
  - SRE
  - 运维工程师
  - 技术支持
estimated_read_time: "15min"
prerequisites:
  - "domain-02-workloads-applications"
  - "domain-04-storage-data"
  - "kubectl-basics"
related_skills:
  - "SKILL-WORK-001"
  - "SKILL-STORE-001"
  - "SKILL-POD-002"
  - "SKILL-NET-001"
  - "SKILL-NET-002"
fta_refs:
  - "domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md"
knowledge_refs:
  - "domain-10-troubleshooting-diagnostics/21-statefulset-troubleshooting.md"
  - "domain-02-workloads-applications/"
  - "domain-04-storage-data/"
cross_refs:
  - type: "fta"
    path: "../domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md"
    label: "StatefulSet 故障树分析"
  - type: "domain"
    path: "../domain-10-troubleshooting-diagnostics/21-statefulset-troubleshooting.md"
    label: "StatefulSet 深度排查"
  - type: "skill"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure.md"
    label: "SKILL-WORK-001 Deployment 故障"
  - type: "skill"
    path: "../domain-10-troubleshooting-diagnostics/topic-skills/07-pvc-storage-failure.md"
    label: "SKILL-STORE-001 PVC 存储故障"
authors:
  - name: KUDIG Team
    role: contributor

tier: peripheral---

# StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation

StatefulSet 是 [[entities/kubernetes|kubernetes]] 中管理有状态应用的核心工作负载控制器。与 Deployment 不同，StatefulSet 为每个 Pod 提供稳定的网络标识（hostname）、稳定的存储（PVC）和有序的部署/扩展/更新保证。这些特性使其故障模式更为复杂：Pod 启动顺序依赖、PVC 与 Pod 的生命周期绑定、Headless Service 依赖、以及分布式一致性要求（如数据库集群的脑裂问题）。

本 Skill 覆盖 Pod 启动顺序卡住、PVC 绑定失败、Headless Service 异常、更新策略阻塞、存储容量不足、分布式脑裂等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| StatefulSet Pod 未按序号顺序启动 | `kubectl get pods -l app=<name>` 观察序号 | 0.95 |
| StatefulSet Pod 处于 Pending/CrashLoopBackOff | `kubectl get pods -l app=<name>` | 0.90 |
| PVC 一直 Pending 状态 | `kubectl get pvc -n <ns>` | 0.95 |
| Headless Service DNS 解析失败 | `nslookup <pod-name>.<svc>` | 0.85 |
| StatefulSet 滚动更新卡住 | `kubectl rollout status sts/<name>` | 0.90 |
| 有状态集群出现多主/脑裂 | 应用级集群状态检查 | 0.85 |

**排除条件**: 纯 Deployment 问题 → SKILL-WORK-001; PVC 通用问题 → SKILL-STORE-001; 节点 NotReady → SKILL-NODE-001; DNS 通用问题 → SKILL-NET-001

## 快速分级（2 分钟内完成）

```
影响范围 + 数据风险
├── 数据库主节点（如 mysql-0 / kafka-0）故障 ────→ P0（立即处理）
├── 有状态集群多数节点故障─────────────────────→ P0（数据一致性风险）
├── 单副本故障（非主节点）─────────────────────→ P1（1h 内修复）
├── 更新策略卡住但不影响当前服务───────────────→ P2（4h 内修复）
└── 新扩容副本无法启动─────────────────────────→ P2（4h 内修复）
```

**立即升级条件**（跳过所有诊断步骤）：
- 有状态集群出现脑裂（多主）或数据不一致
- 主节点故障且无法自动故障转移
- 所有副本同时故障（可能存储后端问题）
- PVC 数据丢失风险（如存储系统故障）

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.7
│ Phase 2      │    内容: 深度分析（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测/修复
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 主动探测（低风险，可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~008
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```

## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | StatefulSet Pod 未按序号启动（如 -1 存在但 -0 不存在） | `kubectl get pods -l app=<name>` | 0.95 | 手动删除 → 检查操作记录 |
| S2 | Pod 序号中断（如 -0, -2 存在但 -1 不存在） | `kubectl get pods -l app=<name>` | 0.95 | 无 |
| S3 | PVC 一直 Pending | `kubectl get pvc -n <ns>` | 0.95 | 通用 PVC 问题 → SKILL-STORE-001 |
| S4 | Headless Service 无 Endpoints | `kubectl get endpoints <svc>` | 0.85 | Service 配置错误 |
| S5 | 滚动更新卡在特定序号 | `kubectl rollout status sts/<name>` | 0.90 | 应用启动慢 → SKILL-POD-001 |
| S6 | Pod 删除后新 Pod 无法创建 | `kubectl get events` | 0.85 | 节点资源不足 → SKILL-POD-002 |
| S7 | DNS 解析 `<pod>.<svc>` 失败 | `nslookup` from test Pod | 0.85 | CoreDNS 故障 → SKILL-NET-001 |
| S8 | 有状态集群应用报告节点不一致 | 应用日志/状态检查 | 0.80 | 应用自身 bug |

### 2.2 工单关键词映射

- "MySQL 集群 mysql-1 启动不了，mysql-0 和 mysql-2 正常"
- "StatefulSet 的 PVC 一直 Pending"
- "Kafka broker 没有按顺序启动"
- "Headless Service 解析不到 Pod IP"
- "StatefulSet 更新卡在 partition"
- "MongoDB 副本集显示多个 primary"
- "Pod 被删除后重新创建失败"
- "Zookeeper 集群选主失败"

### 2.3 排除标准

- 纯 Deployment 无状态应用问题 → 使用 SKILL-WORK-001
- 通用 PVC 存储问题（非 StatefulSet 特有）→ 使用 SKILL-STORE-001
- 节点状态 NotReady → 使用 SKILL-NODE-001
- DNS 通用解析问题 → 使用 SKILL-NET-001
- 应用自身的业务逻辑 bug → 不在本 Skill 范围

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 检查 StatefulSet 整体状态
```bash
kubectl get statefulset <name> -n <namespace>
# 关注: DESIRED, CURRENT, READY 列
```
> **判断规则**: READY < DESIRED → 有副本未就绪；CURRENT < DESIRED → 有副本未创建

**Step T2**: 检查 Pod 启动顺序状态
```bash
kubectl get pods -n <namespace> -l <statefulset-selector> -o json | \
  jq -r '.items[].metadata.name' | sort -V
```
> **判断规则**: 序号不连续或有缺失 → 启动顺序问题（RC-001）

**Step T3**: 检查 PVC 状态
```bash
kubectl get pvc -n <namespace> -l <statefulset-selector>
```
> **判断规则**: 有 PVC Pending → 存储问题（RC-002/003）

**Step T4**: 检查有状态集群健康状态（应用级）
```bash
# MySQL
kubectl exec -n <ns> <mysql-pod> -- mysql -e "SHOW STATUS LIKE 'Slave_IO_Running';" 2>/dev/null
# Kafka
kubectl exec -n <ns> <kafka-pod> -- kafka-broker-api-versions.sh --bootstrap-server localhost:9092 2>/dev/null
# MongoDB
kubectl exec -n <ns> <mongo-pod> -- mongosh --eval "rs.status()" 2>/dev/null | grep -i state
```
> **判断规则**: 应用级状态异常 → 可能是 RC-009（脑裂）或 RC-010（应用配置）

### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| 主节点（ordinal 0 或集群主）故障 | P0 | 15min 内修复 |
| 有状态集群多数节点不可用 | P0 | 30min 内修复 |
| 单非主节点故障 | P1 | 1h 内修复 |
| 更新策略卡住但不影响服务 | P2 | 4h 内修复 |
| 新扩容副本无法启动 | P2 | 4h 内修复 |

### 3.3 立即升级触发条件

- 有状态数据库集群脑裂（多主）
- 主节点故障且无自动故障转移
- PVC 数据丢失或存储后端故障
- 所有副本同时无法启动

## 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 StatefulSet、Pod、PVC 和 Service 状态。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 StatefulSet 概览
- **命令**:
  ```bash
  kubectl get statefulset <name> -n <namespace> -o wide
  kubectl describe statefulset <name> -n <namespace> | head -50
  ```
- **超时**: 10s
- **预期输出模式**: DESIRED/CURRENT/READY 列和 Events
- **判断规则**:
  - READY < DESIRED → 有 Pod 未就绪
  - CURRENT < DESIRED → 有 Pod 未创建（可能是顺序启动卡住）
  - Events 包含 `FailedCreate` → RC-001/002/003
  - Events 包含 `FailedDelete` → 删除/重建问题
- **版本差异**: 无

**Step D1.2**: 检查 Pod 状态和序号
- **命令**:
  ```bash
  kubectl get pods -n <namespace> -l <statefulset-label-selector> \
    -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,READY:.status.containerStatuses[*].ready,RESTARTS:.status.containerStatuses[*].restartCount,NODE:.spec.nodeName
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表及状态
- **判断规则**:
  - Pod 序号不连续（如 -0, -2 但无 -1）→ RC-001（启动顺序异常）
  - 高序号 Pod 存在但低序号不存在 → 异常情况，可能手动操作导致
  - Pod 状态 Pending → RC-002/003（PVC/调度问题）
  - Pod 状态 CrashLoopBackOff → 应用启动失败（RC-010 或 SKILL-POD-001）
- **版本差异**: 无

**Step D1.3**: 检查 PVC 绑定状态
- **命令**:
  ```bash
  kubectl get pvc -n <namespace> -l <statefulset-label-selector>
  kubectl describe pvc <pvc-name> -n <namespace> | grep -A 5 "Events:"
  ```
- **超时**: 10s
- **预期输出模式**: PVC 列表和状态
- **判断规则**:
  - PVC Pending → RC-002（PVC 绑定失败）
  - PVC Bound 但 Pod 未挂载 → RC-003（挂载问题）
  - 无 PVC（StatefulSet 未定义 volumeClaimTemplate）→ 正常（无状态部分）
- **版本差异**: 无

**Step D1.4**: 检查 Headless Service
- **命令**:
  ```bash
  kubectl get service <service-name> -n <namespace>
  kubectl get endpoints <service-name> -n <namespace>
  kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.clusterIP}'
  ```
- **超时**: 10s
- **预期输出模式**: Service 类型和 Endpoints
- **判断规则**:
  - `clusterIP` 不为 `None` → RC-004（Service 不是 Headless）
  - Endpoints 为空 → RC-004（Service selector 不匹配 Pod 标签）
  - Service 不存在 → RC-004（Service 未创建）
- **版本差异**: 无

**Step D1.5**: 检查 StatefulSet 更新策略
- **命令**:
  ```bash
  kubectl get statefulset <name> -n <namespace> -o jsonpath='{.spec.updateStrategy}' | jq .
  kubectl rollout status statefulset <name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: updateStrategy 配置和 rollout 状态
- **判断规则**:
  - `partition` 设置非 0 且更新卡住 → RC-005（Partition 阻塞更新）
  - `type: OnDelete` 且用户期望自动更新 → 预期行为，需手动删除 Pod
  - `rollingUpdate` 参数不合理（如 maxUnavailable 为 0 且只有 1 副本）→ RC-005
- **版本差异**:
  - **[v1.24+]**: StatefulSet 开始支持 `maxUnavailable`（之前仅支持 partition）

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入分析 Pod 事件、存储状态和网络标识。
> **预计耗时**: 5-15 分钟

**Step D2.1**: 分析 Pod 创建事件
- **命令**:
  ```bash
  kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> \
    --sort-by=.lastTimestamp | tail -20
  kubectl describe pod <pod-name> -n <namespace> | grep -A 30 "Events:"
  ```
- **超时**: 15s
- **预期输出模式**: Pod 事件列表
- **判断规则**:
  - `FailedScheduling` → RC-002（调度失败，可能是资源不足）
  - `FailedMount` → RC-003（存储挂载失败）
  - `FailedCreatePodSandBox` → CNI/网络问题
  - `BackOff` + 容器启动失败 → 应用问题（RC-010）
  - `RecreatingFailedPod` → 前一 Pod 失败，StatefulSet 尝试重建
- **版本差异**: 无

**Step D2.2**: 检查 PVC 和 PV 详情
- **命令**:
  ```bash
  kubectl describe pvc <pvc-name> -n <namespace>
  kubectl get pv <pv-name> -o yaml
  kubectl get storageclass <sc-name> -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: PVC/PV/StorageClass 详情
- **判断规则**:
  - PVC 等待 PV 绑定且 StorageClass 不存在 → RC-002（StorageClass 缺失）
  - PV 容量不足 → RC-007（存储容量不足）
  - VolumeBindingMode: WaitForFirstConsumer 但 Pod 未调度 → 调度问题
- **版本差异**: 无

**Step D2.3**: 测试 Headless Service DNS 解析
- **命令**:
  ```bash
  # 从测试 Pod 解析 StatefulSet Pod DNS
  kubectl run -n <namespace> dns-test --image=busybox:1.36 --rm -it --restart=Never -- \
    nslookup <statefulset-name>-0.<service-name>.<namespace>.svc.cluster.local
  # 解析 SRV 记录
  kubectl run -n <namespace> dns-test --image=busybox:1.36 --rm -it --restart=Never -- \
    nslookup -type=SRV <service-name>.<namespace>.svc.cluster.local
  ```
- **超时**: 30s
- **预期输出模式**: DNS 解析结果
- **判断规则**:
  - 解析失败 → RC-004（Headless Service 或 DNS 问题）
  - 解析成功但 IP 不正确 → Pod IP 变更后 DNS 未更新（短暂问题）
  - SRV 记录缺失 → Service 未正确配置
- **版本差异**: 无

**Step D2.4**: 检查 Pod 到 PVC 的挂载关系
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.volumes[*].persistentVolumeClaim.claimName}'
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.conditions[?(@.type=="Ready")]}' | jq .
  ```
- **超时**: 10s
- **预期输出模式**: PVC 名称和 Pod 条件
- **判断规则**:
  - Pod 引用的 PVC 不存在 → RC-002（PVC 未创建）
  - Pod Ready=False，原因 `ContainersNotReady` + 挂载问题 → RC-003
- **版本差异**: 无

**Step D2.5**: 检查 StatefulSet Controller 日志
- **命令**:
  ```bash
  kubectl logs -n kube-system <kube-controller-manager-pod> | \
    grep -iE 'statefulset|StatefulSet' | grep <statefulset-name> | tail -20
  ```
- **超时**: 15s
- **预期输出模式**: Controller 相关日志
- **判断规则**:
  - 日志包含 `Failed to create pod` → RC-001（创建失败）
  - 日志包含 `Waiting for pod to be deleted` → 删除/重建延迟
- **版本差异**: 无

**Step D2.6**: 检查有状态集群应用状态（应用级诊断）
- **命令**:
  ```bash
  # MySQL/MariaDB
  kubectl exec -n <ns> <pod> -- mysql -e "SHOW SLAVE STATUS\G" 2>/dev/null
  # PostgreSQL
  kubectl exec -n <ns> <pod> -- pg_isready 2>/dev/null
  # Redis
  kubectl exec -n <ns> <pod> -- redis-cli info replication 2>/dev/null
  # MongoDB
  kubectl exec -n <ns> <pod> -- mongosh --eval "rs.status()" 2>/dev/null | grep -E '"state"|"name"'
  # Kafka
  kubectl exec -n <ns> <pod> -- kafka-metadata-quorum.sh --bootstrap-server localhost:9092 describe --status 2>/dev/null
  # etcd
  kubectl exec -n <ns> <pod> -- etcdctl endpoint status --cluster 2>/dev/null
  ```
- **超时**: 15s
- **预期输出模式**: 应用级状态信息
- **判断规则**:
  - MySQL Slave_IO_Running: No → RC-008（复制中断）
  - MongoDB 多个 PRIMARY → RC-009（脑裂）
  - Redis 连接主节点失败 → RC-008（复制/同步问题）
  - etcd 成员不健康 → RC-009（集群一致性）
- **版本差异**: 无

**Step D2.7**: 检查节点亲和性和存储拓扑
- **命令**:
  ```bash
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity}' | jq .
  kubectl get pv <pv-name> -o jsonpath='{.spec.nodeAffinity}' | jq .
  kubectl get nodes -l <topology-labels> -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 亲和性配置和节点匹配
- **判断规则**:
  - Pod 有节点亲和性但无匹配节点 → RC-002（调度失败）
  - PV 有节点亲和性约束但 Pod 被调度到其他节点 → RC-003（拓扑不匹配）
- **版本差异**: 无

### Phase 3: 主动探测（低风险，可能需审批）

> ⚠️ 以下步骤涉及 Pod 操作或应用命令执行，在 L1-advisory 模式下需人工确认。

**Step D3.1**: 强制删除卡住的 Pod（StatefulSet 会重建）
- **命令**:
  ```bash
  kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0
  ```
- **超时**: 15s
- **风险级别**: 🟡 中（强制删除可能导致数据不一致，需确认应用支持）
- **预期输出模式**: Pod 删除成功
- **判断规则**:
  - Pod 删除后 StatefulSet 成功重建 → RC-001（启动顺序问题，强制重建可解）
  - Pod 删除后仍无法重建 → 根因未解决
- **版本差异**: 无
- **⚠️ 警告**: 强制删除有状态 Pod 可能导致数据不一致，仅在其他副本健康时使用

**Step D3.2**: 临时扩容存储（云环境）
- **命令**:
  ```bash
  # 确认 PVC 容量请求
  kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.resources.requests.storage}'
  # 云环境扩容（示例：AWS EBS）
  kubectl patch pvc <pvc-name> -n <namespace> -p '{"spec":{"resources":{"requests":{"storage":"<new-size>"}}}}'
  ```
- **超时**: 10s
- **风险级别**: 🟡 中（扩容存储可能影响 I/O 性能）
- **预期输出模式**: PVC 更新成功
- **判断规则**:
  - 扩容后 Pod 正常运行 → RC-007（存储容量不足）
- **版本差异**:
  - **[v1.11+]**: PVC 在线扩容支持（需 StorageClass 允许）
  - **[v1.24+]**: 恢复性扩容支持（Recovered Expansion）

**Step D3.3**: 手动调整 StatefulSet partition
- **命令**:
  ```bash
  # 检查当前 partition
  kubectl get statefulset <name> -n <namespace> -o jsonpath='{.spec.updateStrategy.rollingUpdate.partition}'
  # 临时调整 partition 以继续更新
  kubectl patch statefulset <name> -n <namespace> -p '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'
  ```
- **超时**: 10s
- **风险级别**: 🟡 中（可能导致未准备好的 Pod 被更新）
- **预期输出模式**: partition 更新成功
- **判断规则**:
  - partition 调整后更新继续 → RC-005（Partition 设置问题）
- **版本差异**: 无

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | Pod 启动顺序卡住（前一 Pod 未 Ready，后续不启动） | 高 | D1.2 序号不连续；D2.1 Events | ordinal_startup_blocked |
| RC-002 | PVC 绑定失败（StorageClass 缺失/后端故障） | 高 | D1.3 PVC Pending；D2.2 详情 | pvc_binding_failure |
| RC-003 | 存储挂载失败（PV 拓扑/权限/格式问题） | 中 | D2.1 FailedMount；D2.4 挂载关系 | volume_mount_failure |
| RC-004 | Headless Service 配置错误 | 中 | D1.4 clusterIP!=None 或 Endpoints 为空；D2.3 DNS 失败 | headless_service_misconfig |
| RC-005 | 更新策略阻塞（Partition 设置不当） | 中 | D1.5 partition>0；D3.3 调整后恢复 | update_strategy_blocked |
| RC-006 | Pod 被删除后重建失败（节点/PVC 问题） | 中 | D2.1 RecreatingFailedPod；D3.1 重建失败 | pod_recreate_failure |
| RC-007 | 存储容量不足 | 低 | D2.2 PV 容量满；D3.2 扩容后恢复 | storage_capacity_exhausted |
| RC-008 | 有状态集群复制/同步中断 | 中 | D2.6 应用级状态异常 | replication_failure |
| RC-009 | 有状态集群脑裂（多主） | 低 | D2.6 多个 PRIMARY；应用报告不一致 | split_brain |
| RC-010 | 应用配置错误导致启动失败 | 高 | D2.1 BackOff；D2.6 应用日志错误 | application_misconfig |

## 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 创建缺失的 Headless Service
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get service <service-name> -n <namespace>
  # 确认 Service 不存在
  ```
- **执行命令**:
  ```bash
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: Service
  metadata:
    name: <service-name>
    namespace: <namespace>
    labels:
      app: <app-label>
  spec:
    ports:
    - port: <port>
      name: <port-name>
    clusterIP: None
    selector:
      app: <app-label>
  EOF
  ```
- **后置验证**:
  ```bash
  kubectl get service <service-name> -n <namespace>
  kubectl get endpoints <service-name> -n <namespace>
  ```
- **回滚命令**:
  ```bash
  kubectl delete service <service-name> -n <namespace>
  ```

#### REM-002: 修正 Service selector 匹配 Pod 标签
- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}'
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.labels}'
  ```
- **执行命令**:
  ```bash
  kubectl patch service <service-name> -n <namespace> -p \
    '{"spec":{"selector":{"app":"<correct-label>"}}}'
  ```
- **后置验证**:
  ```bash
  kubectl get endpoints <service-name> -n <namespace>
  ```
- **回滚命令**:
  ```bash
  kubectl patch service <service-name> -n <namespace> -p \
    '{"spec":{"selector":{"app":"<original-label>"}}}'
  ```

#### REM-003: 调整 StatefulSet partition 以继续更新
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get statefulset <name> -n <namespace> -o jsonpath='{.spec.updateStrategy.rollingUpdate.partition}'
  ```
- **执行命令**:
  ```bash
  # 重置 partition 为 0（全部更新）
  kubectl patch statefulset <name> -n <namespace> -p \
    '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":0}}}}'
  ```
- **后置验证**:
  ```bash
  kubectl rollout status statefulset <name> -n <namespace>
  kubectl get pods -n <namespace> -l <selector>
  ```
- **回滚命令**:
  ```bash
  kubectl patch statefulset <name> -n <namespace> -p \
    '{"spec":{"updateStrategy":{"rollingUpdate":{"partition":<original-value>}}}}'
  ```

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-004: 删除并重建卡住的 Pod
- **适用根因**: RC-001/006
- **影响说明**: 删除有状态 Pod 可能导致短暂服务中断。StatefulSet 会按相同序号重建，PVC 会重新挂载。
- **审批提示**: "建议删除 StatefulSet Pod <pod-name>，StatefulSet 控制器将自动重建。是否批准？"
- **前置检查**:
  ```bash
  # 确认其他副本健康（如适用）
  kubectl get pods -n <namespace> -l <selector>
  # 确认 PVC 存在
  kubectl get pvc -n <namespace> | grep <pod-name>
  ```
- **执行命令**:
  ```bash
  # 正常删除（有优雅终止期）
  kubectl delete pod <pod-name> -n <namespace>
  # 或强制删除（Pod 处于 Terminating 卡住时）
  kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n <namespace> -l <selector> -w
  # 等待新 Pod 进入 Running
  kubectl get pod <new-pod-name> -n <namespace>
  ```
- **回滚命令**: Pod 删除后 StatefulSet 自动重建，无回滚操作
- **注意事项**: 确认应用支持单副本中断（多副本场景）

#### REM-005: 扩容 PVC 存储
- **适用根因**: RC-007
- **影响说明**: PVC 扩容可能导致短暂 I/O 暂停。
- **审批提示**: "建议将 PVC <pvc-name> 从 <current-size> 扩容到 <new-size>。是否批准？"
- **前置检查**:
  ```bash
  kubectl get pvc <pvc-name> -n <namespace>
  kubectl describe sc <storage-class>
  # 确认 StorageClass 支持扩容
  ```
- **执行命令**:
  ```bash
  kubectl patch pvc <pvc-name> -n <namespace> -p \
    '{"spec":{"resources":{"requests":{"storage":"<new-size>"}}}}'
  ```
- **后置验证**:
  ```bash
  kubectl get pvc <pvc-name> -n <namespace>
  kubectl exec -n <namespace> <pod-name> -- df -h
  ```
- **回滚命令**: PVC 扩容通常不可缩容，需提前规划

#### REM-006: 手动触发有状态集群故障转移
- **适用根因**: RC-008/009
- **影响说明**: 手动干预集群拓扑可能导致数据不一致。
- **审批提示**: "建议手动触发 <cluster-type> 集群故障转移，可能影响数据一致性。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前集群状态
  kubectl exec -n <ns> <pod> -- <cluster-status-command>
  ```
- **执行命令**:
  ```bash
  # MySQL: 提升从节点为主
  kubectl exec -n <ns> <slave-pod> -- mysql -e "STOP SLAVE; RESET SLAVE ALL;"
  # MongoDB: 强制重新配置
  kubectl exec -n <ns> <pod> -- mongosh --eval "rs.reconfig(...)"
  # Redis: 手动故障转移
  kubectl exec -n <ns> <pod> -- redis-cli CLUSTER FAILOVER
  ```
- **后置验证**:
  ```bash
  # 检查新主节点状态
  kubectl exec -n <ns> <pod> -- <cluster-status-command>
  ```
- **回滚命令**: 复杂，需根据具体集群类型制定

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-007: 重新初始化有状态集群
- **适用根因**: RC-009（严重脑裂）
- **影响说明**: 可能导致数据丢失，需从备份恢复。
- **操作步骤**:
  1. 备份现有数据（如可能）
  2. 按序号逐个删除 Pod 和 PVC
  3. 重建 StatefulSet
  4. 从备份恢复数据
  5. 重新配置集群拓扑
- **安全检查**: 确认有可用备份；确认可接受数据丢失
- **回滚方案**: 从备份恢复

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-008: 删除 StatefulSet 保留 PVC 后重建
- **适用根因**: RC-002/003/006（StatefulSet 严重损坏）
- **审批要求**: 需要数据所有者 + 高级 SRE 双重审批
- **数据备份**: 导出/备份所有 PVC 数据
- **操作步骤**:
  1. 停止写入（如可能）
  2. 备份 PVC 数据
  3. 删除 StatefulSet（使用 `--cascade=orphan` 保留 Pod）
  4. 删除问题 Pod（保留 PVC）
  5. 重新创建 StatefulSet
  6. 验证 Pod 重建和 PVC 挂载
- **回滚方案**: 从备份恢复 PVC 数据

## 验证确认

### 7.1 即时验证（修复后 1 分钟内）

```bash
# V1: 检查 StatefulSet 状态
kubectl get statefulset <name> -n <namespace>
# 预期: READY == DESIRED == CURRENT

# V2: 检查所有 Pod 运行状态
kubectl get pods -n <namespace> -l <selector>
# 预期: 所有 Pod Running 且 Ready

# V3: 检查 PVC 绑定
kubectl get pvc -n <namespace> -l <selector>
# 预期: 所有 PVC Bound

# V4: 检查 Headless Service Endpoints
kubectl get endpoints <service-name> -n <namespace>
# 预期: 包含所有 Pod IP

# V5: 测试 DNS 解析
kubectl run -n <namespace> dns-test --image=busybox:1.36 --rm -it --restart=Never -- \
  nslookup <statefulset-name>-0.<service-name>.<namespace>.svc.cluster.local
# 预期: 解析成功
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Pod 状态 | `kube_pod_status_phase{phase="Running"}` | 稳定 Running | 非 Running |
| PVC 状态 | `kube_persistentvolumeclaim_status_phase{phase="Bound"}` | 稳定 Bound | Pending |
| 应用健康 | 应用级健康检查端点 | 健康 | 不健康 |
| 集群状态 | 应用级集群状态指标 | 正常 | 异常 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：
- [ ] StatefulSet READY == DESIRED == CURRENT
- [ ] 所有 Pod Running 且 Ready
- [ ] 所有 PVC Bound
- [ ] Headless Service Endpoints 包含所有 Pod IP
- [ ] DNS 解析正常
- [ ] 应用级集群状态正常（如适用）

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Pod 重启 | `kubectl get pods` 观察 RESTARTS | 每 4h | 若频繁重启 → 检查应用稳定性 |
| PVC 容量 | `kubelet_volume_stats_available_bytes` | 每 4h | 若不足 → 扩容规划 |
| 集群一致性 | 应用级状态检查 | 每 4h | 若异常 → 排查复制/同步 |
| 更新状态 | `kubectl rollout status` | 更新时 | 若卡住 → 检查 partition |

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 45 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 数据风险 | 诊断发现数据丢失或不一致风险 |
| 集群脑裂 | 有状态集群出现脑裂且无法自动恢复 |
| 存储故障 | 存储后端（如 EBS/Ceph）故障 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 故障概述: StatefulSet <name> 在 namespace <ns> 中有 <count> 个副本异常
- 影响范围: <affected-services> 有状态服务受影响
- 已完成诊断: {completed_steps}
- 初步发现: {findings}
- 根因候选: {root_cause_candidates}
- 数据风险: {data_risk_assessment}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. StatefulSet、Pod、PVC、Service 的 YAML 快照
3. 已排除的根因及原因
4. 应用级集群状态输出
5. 存储后端状态（如适用）
6. 数据备份状态（如有）

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| StatefulSet maxUnavailable | 支持 | 支持 | 支持 | 支持 | 支持 |
| PodIndexLabel | beta | beta | GA | GA | GA |
| StatefulSetAutoDeletePVC | alpha | beta | beta | beta | beta |
| 原地 Pod 资源调整 | alpha | beta | beta | beta | beta |

### 9.2 StatefulSetAutoDeletePVC 说明

| 版本 | 行为 |
|------|------|
| v1.28- | 删除 StatefulSet 时默认保留 PVC（需手动清理） |
| v1.29+ | `StatefulSetAutoDeletePVC` feature gate，可配置 PVC 自动删除策略 |

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将 PVC Pending 误判为通用存储问题 | PVC 不绑定 | StatefulSet 的 volumeClaimTemplate 配置错误 | 检查 volumeClaimTemplate vs 独立 PVC |
| 将启动慢误判为启动卡住 | Pod 长时间 ContainerCreating | 镜像大/网络慢 | 观察是否有进度（Events 持续更新） |
| 忽略 Headless Service 重要性 | DNS 不通 | Service 类型不是 Headless | 始终检查 `clusterIP: None` |
| 强制删除所有 Pod | 期望快速重建 | 可能导致集群数据不一致 | 逐个删除，确认应用恢复后再继续 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- StatefulSet 控制器原理 → `domain-02-workloads-applications/`
- 存储故障排查 → `domain-10-troubleshooting-diagnostics/14-pvc-storage-troubleshooting.md`
- 工作负载管理 → `domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure.md`

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-05 | v1.0 | 初始版本 | 补齐 StatefulSet 故障诊断 Skill |

## 云厂商特异性

### 11.1 ACK (Alibaba Cloud)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| 云盘扩容 | `aliyun ecs ResizeDisk` | 需确认文件系统扩展 |
| NAS/OSS 挂载 | 检查挂载参数 | 有状态应用常用 NAS |

### 11.2 EKS (Amazon Web Services)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| EBS 扩容 | `aws ec2 modify-volume` | gp3 支持在线扩容 |
| EFS 共享存储 | 多个 Pod 共享 PVC | 注意文件锁机制 |

### 11.3 GKE (Google Kubernetes Engine)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| Persistent Disk | 支持在线扩容 | Regional PD 支持多可用区 |
| Filestore | NFS 共享存储 | 有状态应用共享数据 |

### 11.4 AKS (Azure Kubernetes Service)

| 差异 | 诊断命令 | 备注 |
|------|---------|------|
| Azure Disk | 支持在线扩容 | 需 Premium SSD |
| Azure Files | SMB/NFS 共享 | 注意权限配置 |

## 自动化集成接口

### 12.1 脚本入口

```bash
# Phase 1: 快速诊断
./scripts/diagnose-statefulset-quick.sh --statefulset <NAME> --namespace <NS>

# Phase 2: 深度诊断
./scripts/diagnose-statefulset-deep.sh --statefulset <NAME> --namespace <NS>

# 验证
./scripts/verify-statefulset.sh --statefulset <NAME> --namespace <NS>
```

### 12.2 Webhook 回调

```yaml
receivers:
- name: skill-statefulset-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-WORK-002'
    send_resolved: true
```

### 12.3 输出 JSON Schema

```json
{
  "skill_id": "SKILL-WORK-002",
  "statefulset_name": "mysql",
  "namespace": "default",
  "findings": [
    { "step": "D1.2", "result": "mysql-1 Pending", "severity": "high" },
    { "step": "D1.3", "result": "data-mysql-1 Pending", "severity": "critical" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-002", "confidence": 0.90, "evidence": ["D1.3", "D2.2"] }
  ],
  "recommended_action": {
    "rem_id": "REM-005",
    "risk_level": "medium",
    "command": "kubectl patch pvc ...",
    "rollback": "N/A"
  }
}
```

---

*文档版本: 1.0*  
*Skill ID: SKILL-WORK-002*  
*创建时间: 2026-05*  
*维护者: Kudig Team*
