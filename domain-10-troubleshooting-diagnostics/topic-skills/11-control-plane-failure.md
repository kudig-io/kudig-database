---
title: etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation
description: '## 1. 概述'
category: control-plane
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation 是什么
- 如何 etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation
trigger_keywords:
- etcd unhealthy
- etcd leader lost
- apiserver unavailable
- apiserver high latency
- scheduler not leading
- controller-manager restart
- etcd disk slow
- etcd member lost
- control plane certificate expired
- apiserver throttling
- 控制平面问题
- etcd 不健康
- apiserver 不可用
- apiserver 延迟高
- 调度器异常
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
skill_id: SKILL-11_CONTROL_PLANE_FAILURE-001
skill_name: etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation
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

<!-- condition: kubectl get --raw /healthz 返回非 200 或 kubectl get [[Pods|pods]] -n kube-system -l component=[[etcd|etcd]] 显示非 Running -->

# etcd 与控制平面故障诊断与修复 / etcd & Control Plane Failure Diagnosis & Remediation

---

## 1. 概述

控制平面（Control Plane）是 [[Kubernetes|Kubernetes]] 集群的"大脑"，包括 API Server、etcd、Scheduler、Controller Manager 四大核心组件。控制平面问题是 Kubernetes 中**最严重的问题类型**，直接影响整个集群的可用性。etcd 作为唯一的状态存储，其健康状态更是生死攸关——etcd 集群丢失 quorum 意味着集群将无法进行任何状态变更。

### 典型触发场景

1. **etcd 集群健康问题**: Leader 选举失败、成员丢失、数据不一致、磁盘性能不足导致 WAL 写入延迟、数据库配额耗尽（NOSPACE alarm）
2. **API Server 可用性与性能问题**: 请求限流（APF throttling）、高延迟、Webhook 超时导致请求堆积、内存泄漏导致 OOM
3. **Scheduler / Controller Manager 异常**: Leader 选举失败、组件频繁重启、work queue 深度过大
4. **控制平面证书问题**: CA 证书或组件证书过期导致 TLS 握手失败、证书轮转失败
5. **托管集群控制平面问题**: ACK/EKS/GKE 等托管集群的控制平面不可见问题

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `nodes`, `pods`, `events`, `leases` (coordination.k8s.io), `componentstatuses` 的 `get/list/watch`
  - 如需 etcd 诊断: 控制平面节点本地访问权限（etcd 通常不通过 API Server 暴露）
  - 验证命令: `kubectl auth can-i list nodes`
- **SSH 访问**: 自建集群需要对控制平面节点的 SSH 访问权限；托管集群无需 SSH
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `etcdctl` >= v3.5 # Requires v3.5+ (v3.4 deprecated for v1.28+)
  - `jq` >= 1.6
  - `openssl` >= 1.1.1
- **监控系统**: Prometheus + etcd exporter + kube-state-metrics >= v2.10（用于 trigger_metrics 匹配）

> ⚠️ **重要**: 本 Skill 覆盖自建集群和托管集群的控制平面问题场景。对于托管集群，部分诊断步骤不适用（控制平面不可见），需要通过云厂商控制台或 API 进行排查。所有 etcd 修复操作（🔴⚫级别）执行前**必须备份 etcd 快照**。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | `kubectl` 命令超时或无响应 / kubectl commands timeout or no response | `kubectl get nodes --request-timeout=10s` 超时或返回 "connection refused" | 0.95 | 客户端网络问题（本地 kubeconfig 错误、VPN 断开）；kubectl 版本与集群版本不兼容 |
| SP-02 | etcd 成员报告 ALARM（NOSPACE/CORRUPT）/ etcd member reports ALARM | `etcdctl alarm list` 返回 NOSPACE 或 CORRUPT alarm | 0.98 | 已清除但未刷新的历史 alarm（执行 `etcdctl alarm disarm` 后残留） |
| SP-03 | API Server 返回 429 Too Many Requests / API Server returns 429 | 客户端日志或 kubectl 输出包含 "429 Too Many Requests" | 0.90 | 客户端请求频率过高被正常限流；限流配置过于严格但非问题 |
| SP-04 | API Server 返回 504 Gateway Timeout / API Server returns 504 | kubectl 或客户端返回 "504 Gateway Timeout"；ingress/LB 层超时 | 0.85 | 外部负载均衡器配置问题；网络层超时而非 apiserver 问题 |
| SP-05 | etcd leader 频繁切换 / etcd leader frequently changes | `etcd_server_leader_changes_seen_total` 指标短时间内持续增长；etcd 日志出现 "leader changed" | 0.92 | 集群刚启动期间的初始 leader 选举；计划内 etcd 成员滚动重启 |
| SP-06 | Scheduler/CM 日志中 "lost lease" / "leader election failed" | `kubectl logs -n kube-system kube-scheduler-*` 包含 "lost lease" 或 "failed to acquire lease" | 0.88 | 组件正常启动期间的 leader 选举过程；计划内组件重启 |
| SP-07 | 新建/更新资源延迟异常（>5s）/ Resource create/update latency > 5s | `kubectl apply` 或 `kubectl create` 返回时间 > 5s；`apiserver_request_duration_seconds` P99 > 5s | 0.80 | 创建大型资源（如大 ConfigMap）的正常延迟；Webhook 处理时间长 |
| SP-08 | etcd 数据库大小持续增长 / etcd db size keeps growing | `etcd_mvcc_db_total_size_in_bytes` 持续增长；`etcdctl endpoint status` 显示 DB SIZE 接近 quota | 0.85 | 正常业务增长导致的数据增长；未配置自动压缩导致的历史版本堆积 |
| SP-09 | API Server 审计日志停止写入 / API Server audit log stops writing | 审计日志目录无新文件或文件大小不变；磁盘满导致写入失败 | 0.75 | 审计策略配置为不记录某些请求；日志轮转正在进行 |
| SP-10 | Webhook 超时导致请求堆积 / Webhook timeout causing request queue | `apiserver_admission_webhook_admission_duration_seconds` P99 > 10s；`apiserver_current_inflight_requests` 持续高位 | 0.82 | Webhook 服务正常但处理慢；突发大量请求导致临时堆积 |
| SP-11 | 控制平面组件 CrashLoopBackOff / Control plane component CrashLoopBackOff | `kubectl get pods -n kube-system -l tier=control-plane` 显示 CrashLoopBackOff | 0.95 | 组件正在进行滚动升级；配置变更后的预期重启 |
| SP-12 | etcd 快照/备份失败 / etcd snapshot/backup fails | etcd 备份 CronJob 失败；`etcdctl snapshot save` 返回错误 | 0.70 | 临时磁盘空间不足；备份目标存储不可达 |
| SP-13 | Kubernetes Events 显示 FailedScheduling 且无调度器日志 / FailedScheduling events with no scheduler logs | `kubectl get events` 显示大量 FailedScheduling，但 scheduler Pod 无对应日志 | 0.78 | Scheduler 正常但资源不足导致调度失败；亲和性约束无法满足 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "集群 apiserver 挂了，所有 kubectl 命令都不响应"
- "etcd 报警说磁盘空间不足，NOSPACE"
- "控制平面延迟很高，创建 Pod 要等好几秒"
- "scheduler 一直重启，Pod 调度不上去"
- "控制平面证书过期，集群无法操作"
- "etcd leader 一直在切换，集群不稳定"
- "apiserver 返回 429，被限流了"
- "创建资源超时，apiserver 响应很慢"
- "ACK 集群控制平面有问题，帮忙看下"

**English ticket descriptions**:
- "API server is down, all kubectl commands timeout"
- "etcd NOSPACE alarm, cluster is read-only"
- "Control plane high latency, creating pods takes forever"
- "Scheduler keeps restarting, pods stuck in Pending"
- "Control plane certificates expired"
- "etcd leader keeps changing, cluster unstable"
- "API server returning 429, request throttling"
- "Resource creation timeout, slow apiserver response"
- "EKS control plane issues, need investigation"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| API Server 正常响应，但特定 Pod Pending | SKILL-POD-002 | 调度问题（资源不足、亲和性约束），非控制平面问题 |
| 节点状态 NotReady，但控制平面组件正常 | SKILL-NODE-001 | 节点级问题，非控制平面问题 |
| 证书问题但仅影响 kubelet，控制平面正常 | SKILL-SEC-001 | kubelet 证书问题，非控制平面证书 |
| 网络策略阻止 Pod 通信，控制平面正常 | SKILL-NET-001 | 网络策略问题，非控制平面问题 |
| 仅 Webhook 服务本身问题，apiserver 正常 | 应用层问题 | Webhook 服务需要应用团队修复 |
| 客户端 kubeconfig 配置错误 | 客户端问题 | 非集群问题，修正 kubeconfig 即可 |
| 托管集群计划内维护（有维护通知）| 不适用本 Skill | 计划内维护，等待维护窗口结束 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: API Server 基本可用性检测（10s）
```bash
# 测试 API Server 响应
kubectl cluster-info --request-timeout=10s
# 或直接测试 API 端点
kubectl get --raw /healthz --request-timeout=10s
```
> **判断规则**:
> - 命令超时或 "connection refused" → **立即升级**（API Server 完全不可用，见 3.3）
> - 返回 "ok" 但响应慢（>3s）→ **P1**，继续 T2 深入分析
> - 正常响应（<1s）→ 继续 T2

**Step T2**: 控制平面组件状态检测（30s）
```bash
# 检查 componentstatuses（已废弃但仍可用于快速检查）
kubectl get componentstatuses 2>/dev/null || echo "componentstatuses not supported"

# 检查控制平面 Pod 状态
kubectl get pods -n kube-system -l tier=control-plane -o wide

# 检查最近的关键事件
kubectl get events -n kube-system --sort-by=.lastTimestamp --field-selector type=Warning | head -20
```
> **判断规则**:
> - 多个控制平面组件 NotReady/CrashLoopBackOff → **P0**
> - 单个组件异常（如 scheduler 重启）→ **P1**
> - 有 Warning 事件但组件 Running → **P2**，继续 T3

**Step T3**: etcd 集群健康快检（60s，需要 etcdctl）
```bash
# 在控制平面节点上执行，或通过 kubectl exec 进入 etcd Pod
# 自建集群方式：
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  endpoint health --cluster

# 通过 kubectl exec 方式：
kubectl exec -n kube-system etcd-<control-plane-node> -- \
  etcdctl endpoint health --cluster \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key

# 检查 etcd alarm
kubectl exec -n kube-system etcd-<control-plane-node> -- \
  etcdctl alarm list \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key
```
> **判断规则**:
> - 任何成员 "unhealthy" → **P0**（etcd 集群降级）
> - 存在 NOSPACE 或 CORRUPT alarm → **P0**
> - 所有成员 "healthy" 但延迟高（>100ms）→ **P1**
> - 托管集群无法执行此步骤 → 跳过，依赖 T1/T2 结果

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| API Server 完全不可用（kubectl 命令超时）**或** etcd 丢失 quorum | **P0** | 集群级灾难，无法进行任何操作。etcd 丢失 quorum 意味着集群变为只读或完全不可用 | 立即响应，15min 内确认根因，1h 内恢复 |
| 控制平面部分组件异常 **或** etcd 成员降级（<quorum 但仍可用）**或** API 延迟 >5s | **P1** | 控制平面降级运行，集群功能受限。新工作负载无法调度或创建延迟严重 | 15min 内响应，30min 内恢复 |
| API 延迟升高（2-5s）**或** 单个非关键组件异常 **或** etcd 磁盘性能警告 | **P2** | 控制平面性能问题，功能正常但体验下降。需要关注但不紧急 | 30min 内响应，4h 内修复 |
| 监控指标异常但用户无感知 **或** 日志中有 Warning 但功能正常 | **P3** | 潜在问题，需要预防性处理 | 工作日内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **集群完全不可用**: 所有 kubectl 命令超时或返回 "connection refused"
- **etcd 丢失 quorum**: 3 节点 etcd 集群中 2+ 节点不可用，或 5 节点集群中 3+ 节点不可用
- **etcd 数据损坏**: `etcdctl alarm list` 返回 CORRUPT alarm
- **多控制平面节点问题**: 超过 50% 的控制平面节点不可用
- **证书链完全失效**: 所有控制平面组件报告 TLS 握手失败
- **托管集群控制平面异常**: ACK/EKS/GKE 控制台显示控制平面不健康且无法自愈

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 控制平面健康快检（只读，零风险）

> **目标**: 通过 kubectl 和 API 端点远程检查控制平面组件状态。所有命令均为只读操作。
> **预计耗时**: 3-5 分钟
> **适用范围**: 自建集群和托管集群

**Step D1.1**: API Server 存活与就绪检查
- **命令**:
  ```bash
  # 存活检查
  kubectl get --raw /livez?verbose --request-timeout=30s
  
  # 就绪检查
  kubectl get --raw /readyz?verbose --request-timeout=30s
  ```
- **超时**: 30s
- **预期输出模式**: 各检查项返回 `[+]xxx ok` 或 `[-]xxx failed`
- **判断规则**:
  - 所有项返回 `[+]...ok` → API Server 健康，继续 D1.2
  - `[-]etcd ok` 失败 → etcd 连接问题（RC-001, RC-002），跳转 Phase 2
  - `[-]poststarthook/...` 失败 → 启动钩子问题，检查 D1.4 日志
  - 命令超时 → API Server 严重问题，尝试直接 SSH 到控制平面节点
- **版本差异**:
  - **[v1.29+]**: `/readyz` 包含更多检查项，如 `informer-sync`
  - **[v1.31+]**: 新增 `shutdown` 检查项，用于优雅关闭状态检测

**Step D1.2**: etcd 集群状态检查
- **命令**:
  ```bash
  # 通过 kubectl exec 检查 etcd 状态
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
  
  kubectl exec -n kube-system ${ETCD_POD} -- etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w table
  ```
- **超时**: 15s
- **预期输出模式**: 表格输出包含 ENDPOINT, ID, VERSION, DB SIZE, IS LEADER, IS LEARNER, RAFT TERM, RAFT INDEX
- **判断规则**:
  - 所有成员显示且有唯一 IS LEADER=true → etcd 健康
  - 无 LEADER 或多个 LEADER → leader 选举问题（RC-007）
  - 某成员缺失 → 成员丢失（RC-002）
  - DB SIZE 接近 quota（默认 2GB）→ 数据库配额问题（RC-004）
  - RAFT TERM 不一致 → 可能有脑裂（RC-010）
- **版本差异**: 无

**Step D1.3**: 控制平面 Pod 状态检查
- **命令**:
  ```bash
  # 获取所有控制平面组件 Pod 状态
  kubectl get pods -n kube-system -l tier=control-plane -o wide
  
  # 或按组件分别检查
  kubectl get pods -n kube-system -l component=kube-apiserver
  kubectl get pods -n kube-system -l component=kube-scheduler
  kubectl get pods -n kube-system -l component=kube-controller-manager
  kubectl get pods -n kube-system -l component=etcd
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表，关注 STATUS 和 RESTARTS 列
- **判断规则**:
  - 所有 Pod 为 Running 且 RESTARTS=0（或稳定）→ 组件运行正常
  - CrashLoopBackOff → 组件反复崩溃（RC-008, RC-011）
  - 高 RESTARTS 数 → 组件不稳定，检查日志（D1.4）
  - Pending → 控制平面节点资源不足或调度问题
  - 托管集群无此 Pod → 正常，托管集群控制平面不可见
- **版本差异**: 无

**Step D1.4**: 组件日志快扫
- **命令**:
  ```bash
  # API Server 最近日志
  kubectl logs -n kube-system -l component=kube-apiserver --tail=50 --timestamps
  
  # Scheduler 最近日志
  kubectl logs -n kube-system -l component=kube-scheduler --tail=30 --timestamps
  
  # Controller Manager 最近日志
  kubectl logs -n kube-system -l component=kube-controller-manager --tail=30 --timestamps
  
  # etcd 最近日志
  kubectl logs -n kube-system -l component=etcd --tail=50 --timestamps
  ```
- **超时**: 20s（每个组件）
- **预期输出模式**: 日志条目
- **判断规则**:
  - API Server 日志包含 `etcd` + `connection refused` 或 `timeout` → etcd 连接问题（RC-001）
  - API Server 日志包含 `TLS handshake error` → 证书问题（RC-006）
  - API Server 日志包含 `request throttled` → 限流触发（RC-005）
  - Scheduler/CM 日志包含 `lost lease` → leader 选举问题（RC-008）
  - etcd 日志包含 `disk too slow` 或 `slow fdatasync` → 磁盘性能问题（RC-001）
  - etcd 日志包含 `leader changed` → leader 频繁切换（RC-007）
- **版本差异**:
  - **[v1.30+]**: API Server 日志可能包含 APF 相关详细信息

**Step D1.5**: 证书有效期检查
- **命令**:
  ```bash
  # 使用 kubeadm 检查证书（自建集群）
  kubeadm certs check-expiration
  
  # 或手动检查各证书
  for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
    echo "=== $cert ===" 
    openssl x509 -in $cert -noout -dates -subject 2>/dev/null || echo "Failed to read"
  done
  ```
- **超时**: 15s
- **预期输出模式**: 证书到期时间列表
- **判断规则**:
  - 任何证书 `notAfter` 早于当前时间 → 证书已过期（RC-006）
  - 任何证书 7 天内过期 → 需要紧急续期
  - 所有证书有效期 > 30 天 → 证书正常
  - 托管集群无法执行此检查 → 跳过
- **版本差异**:
  - **[v1.29+]**: kubeadm 支持 `--config` 参数指定配置文件

**Step D1.6**: API Server 请求延迟检查
- **命令**:
  ```bash
  # 获取 API Server metrics（需要直接访问 metrics 端点）
  kubectl get --raw /metrics 2>/dev/null | grep apiserver_request_duration_seconds
  
  # 或使用 kubectl 测量实际延迟
  time kubectl get nodes >/dev/null 2>&1
  time kubectl get pods -A >/dev/null 2>&1
  ```
- **超时**: 30s
- **预期输出模式**: 延迟数据或命令执行时间
- **判断规则**:
  - 实际延迟 < 1s → 正常
  - 实际延迟 1-5s → 性能降级（P2），检查 Webhook 和 etcd
  - 实际延迟 > 5s → 严重问题（P1），深度诊断 Phase 2
  - `apiserver_request_duration_seconds` P99 > 1s → 需要关注
- **版本差异**: 无

---

### Phase 2: etcd 深度诊断（只读，零风险，需 SSH 或 kubectl exec）

> **目标**: 深度检查 etcd 集群健康状态、性能指标和数据一致性。所有命令为只读操作。
> **前提**: 需要对控制平面节点的 SSH 访问权限，或能够 kubectl exec 进入 etcd Pod
> **预计耗时**: 5-10 分钟
> **适用范围**: 仅自建集群

**Step D2.1**: etcd 成员列表检查
- **命令**:
  ```bash
  # 获取完整成员列表
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    member list -w table
  ```
- **超时**: 10s
- **预期输出模式**: 表格包含 ID, STATUS, NAME, PEER ADDRS, CLIENT ADDRS, IS LEARNER
- **判断规则**:
  - 所有成员 STATUS=started → 成员正常
  - 成员 STATUS=unstarted → 成员未启动或不可达（RC-002）
  - 成员数量与预期不符 → 成员丢失或多余（RC-002）
  - PEER ADDRS 不正确 → 配置问题
- **版本差异**: 无

**Step D2.2**: etcd 性能指标检查
- **命令**:
  ```bash
  # 获取详细端点状态
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w table
  ```
- **超时**: 15s
- **预期输出模式**: DB SIZE, IS LEADER, RAFT TERM, RAFT INDEX, ERRORS
- **判断规则**:
  - DB SIZE > 2GB（默认 quota）或接近配置的 quota → 配额警告（RC-004）
  - DB SIZE 各成员差异 > 100MB → 数据不一致风险
  - RAFT TERM 各成员不一致 → 脑裂风险（RC-010）
  - ERRORS 列有内容 → 需要分析具体错误
- **版本差异**: 无

**Step D2.3**: WAL fsync 延迟检查
- **命令**:
  ```bash
  # 执行性能检查
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    check perf
  
  # 或从 metrics 获取详细数据（如果有 metrics 端口）
  curl -s http://127.0.0.1:2381/metrics | grep etcd_disk_wal_fsync_duration_seconds
  ```
- **超时**: 60s（perf check 需要时间）
- **预期输出模式**: 性能报告或 histogram metrics
- **判断规则**:
  - 60 OP/s PASS → 磁盘性能充足
  - 60 OP/s FAIL 或 fsync P99 > 10ms → 磁盘性能不足（RC-001）
  - fsync P99 > 100ms → 严重磁盘问题，可能导致 leader 选举失败
- **版本差异**: 无

**Step D2.4**: 后端提交延迟检查
- **命令**:
  ```bash
  # 从 metrics 获取后端提交延迟
  curl -s http://127.0.0.1:2381/metrics | grep etcd_disk_backend_commit_duration_seconds
  ```
- **超时**: 10s
- **预期输出模式**: histogram metrics
- **判断规则**:
  - P99 < 25ms → 正常
  - P99 25-100ms → 需要关注
  - P99 > 100ms → 后端存储问题，可能是磁盘 I/O 或碎片化（RC-001）
- **版本差异**: 无

**Step D2.5**: etcd 数据库碎片化检查
- **命令**:
  ```bash
  # 获取数据库大小详情
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize, dbSizeInUse: .Status.dbSizeInUse}'
  ```
- **超时**: 10s
- **预期输出模式**: JSON 输出包含 dbSize 和 dbSizeInUse
- **判断规则**:
  - dbSize / dbSizeInUse < 1.5 → 碎片化正常
  - dbSize / dbSizeInUse 1.5-2.0 → 轻度碎片化，建议 defrag
  - dbSize / dbSizeInUse > 2.0 → 严重碎片化（RC-004 变种），需要 defrag
- **版本差异**: 无

**Step D2.6**: etcd 快照状态检查
- **命令**:
  ```bash
  # 检查最近的快照
  ls -la /var/lib/etcd/member/snap/
  
  # 检查特定快照的状态
  ETCDCTL_API=3 etcdctl snapshot status /var/lib/etcd/member/snap/db -w table
  ```
- **超时**: 10s
- **预期输出模式**: 快照文件列表和状态信息
- **判断规则**:
  - 有最近的快照文件 → 正常
  - 无快照文件或文件损坏 → 数据持久化风险
  - 快照大小与 DB SIZE 差异大 → 可能有问题
- **版本差异**: 无

**Step D2.7**: etcd 网络延迟检查
- **命令**:
  ```bash
  # 从 metrics 获取成员间网络延迟
  curl -s http://127.0.0.1:2381/metrics | grep etcd_network_peer_round_trip_time_seconds
  ```
- **超时**: 10s
- **预期输出模式**: histogram metrics
- **判断规则**:
  - P99 < 10ms → 网络延迟正常
  - P99 10-50ms → 需要关注，可能影响 leader 选举
  - P99 > 50ms → 网络延迟过高，可能导致 leader 频繁切换（RC-007）
- **版本差异**: 无

**Step D2.8**: etcd MVCC 修订版本检查
- **命令**:
  ```bash
  # 检查修订版本增长情况
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status -w json | jq '.[].Status.header.revision'
  
  # 等待 10 秒后再次检查
  sleep 10
  # 重复上述命令
  ```
- **超时**: 20s
- **预期输出模式**: revision 数字
- **判断规则**:
  - 10s 内增长 < 1000 → 正常
  - 10s 内增长 1000-10000 → 写入频率较高，关注是否正常业务
  - 10s 内增长 > 10000 → 写入频率异常高，可能是控制循环风暴
- **版本差异**: 无

---

### Phase 3: API Server 与调度器深度诊断（只读，零风险）

> **目标**: 深度检查 API Server 性能、Webhook 影响和调度器状态。
> **预计耗时**: 5-10 分钟
> **适用范围**: 自建集群和托管集群（部分命令）

**Step D3.1**: API Server 请求分析
- **命令**:
  ```bash
  # 获取 API Server 请求统计
  kubectl get --raw /metrics 2>/dev/null | grep -E "apiserver_request_total|apiserver_request_duration"
  
  # 分析请求分布
  kubectl get --raw /metrics 2>/dev/null | grep apiserver_request_total | \
    grep -v "^#" | \
    awk -F'{' '{print $2}' | \
    awk -F'}' '{print $1}' | \
    sort | uniq -c | sort -rn | head -20
  ```
- **超时**: 30s
- **预期输出模式**: 请求指标数据
- **判断规则**:
  - 大量 code="429" → 限流触发（RC-005）
  - 大量 code="5xx" → 服务端错误，需要分析具体原因
  - 特定 verb/resource 请求量异常高 → 可能是客户端行为问题
  - LIST 请求过多 → 可能是 watch 重连或 client 问题
- **版本差异**:
  - **[v1.29+]**: 新增 `apiserver_request_sli_duration_seconds` 指标

**Step D3.2**: 审计日志分析
- **命令**:
  ```bash
  # 检查审计日志目录
  ls -la /var/log/kubernetes/audit/
  
  # 分析最近的审计日志（高频请求）
  cat /var/log/kubernetes/audit/audit.log | \
    jq -r '.user.username + " " + .verb + " " + .objectRef.resource' | \
    sort | uniq -c | sort -rn | head -20
  
  # 分析大体积请求
  cat /var/log/kubernetes/audit/audit.log | \
    jq -r 'select(.responseStatus.code >= 400) | .user.username + " " + .verb + " " + .objectRef.resource + " " + (.responseStatus.code|tostring)' | \
    sort | uniq -c | sort -rn | head -20
  ```
- **超时**: 30s
- **预期输出模式**: 请求统计
- **判断规则**:
  - 单一 user 请求量远超其他 → 可能是行为不当的 controller 或脚本
  - 大量 4xx/5xx 错误集中在特定资源 → 权限或资源问题
  - ConfigMap/Secret 大量 GET → 可能是 Pod 频繁重启或配置问题
- **版本差异**: 无

**Step D3.3**: Webhook 延迟分析
- **命令**:
  ```bash
  # 获取 Webhook 延迟指标
  kubectl get --raw /metrics 2>/dev/null | grep apiserver_admission_webhook_admission_duration_seconds
  
  # 列出所有 Webhook 配置
  kubectl get validatingwebhookconfigurations -o wide
  kubectl get mutatingwebhookconfigurations -o wide
  
  # 检查 Webhook 服务健康状态
  for webhook in $(kubectl get validatingwebhookconfigurations -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== $webhook ==="
    kubectl get validatingwebhookconfiguration $webhook -o jsonpath='{.webhooks[*].clientConfig.service.name}'
    echo ""
  done
  ```
- **超时**: 20s
- **预期输出模式**: Webhook 列表和延迟数据
- **判断规则**:
  - Webhook 延迟 P99 > 10s → Webhook 服务严重延迟（RC-003）
  - Webhook 服务 Pod 不健康 → Webhook 服务问题
  - failurePolicy=Fail 的 Webhook 超时 → 会阻塞请求
  - failurePolicy=Ignore 的 Webhook 超时 → 不会阻塞但功能缺失
- **版本差异**:
  - **[v1.30+]**: 支持 Webhook 匹配条件表达式

**Step D3.4**: 客户端限流（APF）分析
- **命令**:
  ```bash
  # 获取 APF 配置
  kubectl get flowschemas
  kubectl get prioritylevelconfigurations
  
  # 获取 APF 指标
  kubectl get --raw /metrics 2>/dev/null | grep -E "apiserver_flowcontrol"
  
  # 检查是否有请求被拒绝
  kubectl get --raw /metrics 2>/dev/null | grep apiserver_flowcontrol_rejected_requests_total
  ```
- **超时**: 15s
- **预期输出模式**: APF 配置和指标
- **判断规则**:
  - `rejected_requests_total` 持续增长 → 限流触发（RC-005）
  - 特定 FlowSchema 的拒绝率高 → 该类请求被限流
  - PriorityLevel 的 queue depth 持续高 → 请求积压
- **版本差异**:
  - **[v1.28+]**: APF GA，默认启用
  - **[v1.29+]**: 新增 `borrowing` 语义
  - **[v1.31+]**: APF 配置更灵活

**Step D3.5**: Scheduler 调度延迟检查
- **命令**:
  ```bash
  # 获取 Scheduler 指标
  kubectl get --raw /api/v1/namespaces/kube-system/pods/$(kubectl get pods -n kube-system -l component=kube-scheduler -o jsonpath='{.items[0].metadata.name}')/proxy/metrics 2>/dev/null | grep scheduler_scheduling
  
  # 检查 Scheduler leader 选举
  kubectl get endpoints kube-scheduler -n kube-system -o yaml
  kubectl get leases kube-scheduler -n kube-system -o yaml
  ```
- **超时**: 15s
- **预期输出模式**: 调度指标和 leader 信息
- **判断规则**:
  - `scheduler_scheduling_algorithm_duration_seconds` P99 > 1s → 调度算法慢
  - 无 leader 或 leader 频繁切换 → leader 选举问题（RC-008）
  - Scheduler Pod 频繁重启 → 组件不稳定
- **版本差异**:
  - **[v1.29+]**: 调度框架增强
  - **[v1.31+]**: 新的调度插件和指标

**Step D3.6**: Controller Manager work queue 深度检查
- **命令**:
  ```bash
  # 获取 Controller Manager 指标
  kubectl get --raw /api/v1/namespaces/kube-system/pods/$(kubectl get pods -n kube-system -l component=kube-controller-manager -o jsonpath='{.items[0].metadata.name}')/proxy/metrics 2>/dev/null | grep workqueue
  
  # 检查 CM leader 选举
  kubectl get leases kube-controller-manager -n kube-system -o yaml
  ```
- **超时**: 15s
- **预期输出模式**: work queue 指标
- **判断规则**:
  - `workqueue_depth` 持续 > 100 → 控制器处理不过来
  - `workqueue_adds_total` 增长异常快 → 可能有控制循环风暴
  - CM leader 丢失 → leader 选举问题（RC-008）
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 风险 | 诊断证据 | FTA 映射 |
|--------|------|------|------|---------|---------|
| RC-001 | **etcd 磁盘性能不足（fsync 延迟 >10ms）** — etcd 数据目录所在磁盘 I/O 性能不足，导致 WAL 写入延迟，影响 leader 选举和数据持久化 | ~20% | 🟡 | D2.3 fsync P99 > 10ms；etcd 日志包含 "slow fdatasync"；D2.4 后端提交延迟高 | cp-fta: BE-etcd-disk-slow |
| RC-002 | **etcd 成员丢失/不可达** — etcd 集群成员因节点问题、网络问题或配置错误而不可用，导致集群降级或丢失 quorum | ~15% | 🔴 | D2.1 成员 STATUS=unstarted；D1.2 显示缺少成员；etcd 日志包含 "peer unreachable" | cp-fta: BE-etcd-member-lost |
| RC-003 | **API Server Webhook 级联延迟** — Admission Webhook 服务响应慢或不可用，导致 API 请求堆积和超时 | ~12% | 🟡 | D3.3 Webhook 延迟 P99 > 10s；D1.4 日志包含 webhook timeout；D3.1 大量 5xx | cp-fta: BE-webhook-slow |
| RC-004 | **etcd 数据库配额耗尽（NOSPACE alarm）** — etcd 数据库大小达到配置的 quota 上限，触发 NOSPACE alarm，集群变为只读 | ~10% | 🔴 | T3 alarm list 包含 NOSPACE；D2.2 DB SIZE 接近 quota；D1.2 显示 ERRORS | cp-fta: BE-etcd-quota |
| RC-005 | **API Server 请求限流配置不当** — APF（API Priority and Fairness）配置过于严格或不合理，导致正常请求被限流 | ~8% | 🟡 | D3.4 rejected_requests 增长；D3.1 大量 429；日志包含 "request throttled" | cp-fta: BE-apf-throttle |
| RC-006 | **控制平面证书过期** — CA 证书或组件证书（apiserver、etcd、scheduler、controller-manager）过期，导致 TLS 握手失败 | ~7% | 🔴 | D1.5 证书已过期；D1.4 日志包含 "x509: certificate has expired"；组件无法启动 | cp-fta: BE-cert-expired |
| RC-007 | **etcd leader 选举风暴** — etcd 成员间网络延迟过高或时钟不同步，导致 leader 频繁切换，影响集群稳定性 | ~6% | 🔴 | D2.7 网络延迟 P99 > 50ms；etcd 日志包含 "leader changed"；`leader_changes_seen` 指标持续增长 | cp-fta: BE-etcd-leader-churn |
| RC-008 | **Scheduler/CM leader 选举失败** — kube-scheduler 或 kube-controller-manager 无法获得或维持 leader lease | ~5% | 🟡 | D1.4 日志包含 "lost lease"；D3.5/D3.6 无 leader；组件频繁重启 | cp-fta: BE-component-leader |
| RC-009 | **审计日志爆满导致 API Server OOM** — 审计日志配置不当，大量审计数据占用内存，导致 API Server OOM | ~5% | 🟡 | D3.2 审计日志巨大；apiserver Pod OOMKilled；内存使用持续增长 | cp-fta: BE-apiserver-oom |
| RC-010 | **etcd 数据不一致/损坏** — etcd 数据文件损坏或成员间数据不一致，可能由磁盘问题或非正常关机导致 | ~4% | ⚫ | T3 alarm 包含 CORRUPT；D2.5 成员间 DB SIZE 差异大；etcd 日志包含 corruption 错误 | cp-fta: BE-etcd-corrupt |
| RC-011 | **API Server 内存泄漏** — API Server 存在内存泄漏，导致内存使用持续增长直至 OOM | ~3% | 🔴 | apiserver Pod 内存使用线性增长；最终 OOMKilled；重启后短期恢复 | cp-fta: BE-apiserver-memleak |
| RC-012 | **大量 Watch 连接导致 API Server 负载过高** — 客户端创建过多 watch 连接，导致 API Server 内存和 CPU 负载过高 | ~3% | 🟡 | `apiserver_watch_events_total` 极高；内存使用与 watch 数相关；LIST 请求过多 | cp-fta: BE-watch-overload |
| RC-013 | **托管集群控制平面底层问题** — ACK/EKS/GKE 等托管集群的控制平面不可见问题，需要云厂商介入 | ~2% | 🟡 | 托管集群控制台显示异常；无法 SSH 到控制平面；云厂商 API 返回错误 | cp-fta: BE-managed-cp |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 清理 API Server 审计日志
- **适用根因**: RC-009
- **前置检查**:
  ```bash
  # 确认审计日志占用空间
  du -sh /var/log/kubernetes/audit/
  # 确认 API Server 状态
  kubectl get pods -n kube-system -l component=kube-apiserver
  ```
- **执行命令**:
  ```bash
  # 保留最近的审计日志，清理旧日志
  find /var/log/kubernetes/audit/ -name "*.log" -mtime +7 -delete
  find /var/log/kubernetes/audit/ -name "*.log.gz" -mtime +3 -delete
  
  # 如果空间仍然不足，压缩当前日志（不删除）
  gzip -k /var/log/kubernetes/audit/audit.log
  ```
- **后置验证**:
  ```bash
  # 确认空间已释放
  du -sh /var/log/kubernetes/audit/
  # 确认 API Server 正常
  kubectl get --raw /healthz
  ```
- **回滚命令**:
  ```bash
  # 审计日志清理不可逆，但不影响集群功能
  # 如果需要历史审计数据，应从备份恢复
  ```

#### REM-002: 禁用/修复异常 Webhook
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认问题 Webhook
  kubectl get validatingwebhookconfigurations -o wide
  kubectl get mutatingwebhookconfigurations -o wide
  
  # 检查 Webhook 服务状态
  kubectl get svc -A | grep webhook
  kubectl get pods -A | grep webhook
  ```
- **执行命令**:
  ```bash
  # 方式1: 临时禁用问题 Webhook（添加 namespaceSelector 排除）
  kubectl patch validatingwebhookconfiguration <webhook-name> \
    --type='json' \
    -p='[{"op": "add", "path": "/webhooks/0/namespaceSelector", "value": {"matchExpressions": [{"key": "webhook-disabled", "operator": "Exists"}]}}]'
  
  # 方式2: 将 failurePolicy 改为 Ignore（临时措施）
  kubectl patch validatingwebhookconfiguration <webhook-name> \
    --type='json' \
    -p='[{"op": "replace", "path": "/webhooks/0/failurePolicy", "value": "Ignore"}]'
  
  # 方式3: 直接删除 Webhook（谨慎使用）
  kubectl delete validatingwebhookconfiguration <webhook-name>
  ```
- **后置验证**:
  ```bash
  # 测试 API 延迟恢复
  time kubectl get nodes
  # 确认请求不再堆积
  kubectl get --raw /metrics | grep apiserver_current_inflight_requests
  ```
- **回滚命令**:
  ```bash
  # 恢复 Webhook 配置
  kubectl apply -f <webhook-backup.yaml>
  # 或重新安装 Webhook 服务
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-003: etcd 数据库碎片整理 (defrag)
- **适用根因**: RC-004（碎片化变种）, RC-001（空间不足）
- **影响说明**: defrag 操作会**暂时阻塞**被 defrag 的 etcd 成员，在此期间该成员无法处理请求。在 3 节点集群中，逐个 defrag 是安全的，但如果同时 defrag 多个成员可能导致 quorum 丢失。
- **审批提示**: "建议对 etcd 集群执行碎片整理 (defrag) 操作，以回收磁盘空间。操作将逐个成员进行，每个成员在 defrag 期间（通常 10-60s）无法处理请求。是否批准？"
- **前置检查**:
  ```bash
  # ⚠️ 必须先备份 etcd
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    snapshot save /backup/etcd-snapshot-$(date +%Y%m%d%H%M%S).db
  
  # 确认碎片化程度
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize, dbSizeInUse: .Status.dbSizeInUse}'
  ```
- **执行命令**:
  ```bash
  # 逐个成员执行 defrag（从 follower 开始，leader 最后）
  # 获取成员列表
  ENDPOINTS=$(ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    member list -w json | jq -r '.members[].clientURLs[0]' | tr '\n' ',')
  
  # 对每个 follower 执行 defrag
  for ep in $(echo $ENDPOINTS | tr ',' '\n' | grep -v leader); do
    echo "Defragmenting $ep"
    ETCDCTL_API=3 etcdctl \
      --endpoints=$ep \
      --cacert=/etc/kubernetes/pki/etcd/ca.crt \
      --cert=/etc/kubernetes/pki/etcd/peer.crt \
      --key=/etc/kubernetes/pki/etcd/peer.key \
      defrag
    echo "Completed, waiting 30s before next..."
    sleep 30
  done
  
  # 最后对 leader 执行 defrag
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    defrag
  ```
- **后置验证**:
  ```bash
  # 确认碎片化改善
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w table
  
  # 确认集群健康
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint health --cluster
  ```
- **回滚命令**:
  ```bash
  # defrag 不可逆，但如果出现问题可以从备份恢复
  # 参见 REM-009
  ```

#### REM-004: etcd 配额调整与 alarm 清除
- **适用根因**: RC-004
- **影响说明**: 调整 etcd 配额需要重启 etcd 成员。清除 NOSPACE alarm 后集群恢复可写，但需要同时处理数据清理或扩容，否则 alarm 会再次触发。
- **审批提示**: "etcd 触发了 NOSPACE alarm，需要调整配额并清除 alarm。此操作需要修改 etcd 配置并可能需要重启 etcd 成员。是否批准？"
- **前置检查**:
  ```bash
  # ⚠️ 必须先备份 etcd
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    snapshot save /backup/etcd-snapshot-$(date +%Y%m%d%H%M%S).db
  
  # 确认当前配额和使用情况
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status --cluster -w table
  
  # 确认 alarm 状态
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    alarm list
  ```
- **执行命令**:
  ```bash
  # Step 1: 执行压缩以删除历史版本（需要先确定安全的 revision）
  # 获取当前 revision
  REVISION=$(ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    endpoint status -w json | jq '.[0].Status.header.revision')
  
  # 压缩到当前 revision 之前的所有数据
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    compact $REVISION
  
  # Step 2: 执行 defrag 以回收空间
  # 参见 REM-003
  
  # Step 3: 清除 alarm
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    alarm disarm
  
  # Step 4: （可选）调整配额（需要修改 etcd 启动参数并重启）
  # 编辑 /etc/kubernetes/manifests/etcd.yaml
  # 添加或修改 --quota-backend-bytes=8589934592 (8GB)
  ```
- **后置验证**:
  ```bash
  # 确认 alarm 已清除
  ETCDCTL_API=3 etcdctl \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/peer.crt \
    --key=/etc/kubernetes/pki/etcd/peer.key \
    alarm list
  # 预期: 无输出
  
  # 测试写入
  kubectl create configmap test-write --from-literal=test=ok -n kube-system
  kubectl delete configmap test-write -n kube-system
  ```
- **回滚命令**:
  ```bash
  # 如果清除 alarm 后集群仍有问题，从备份恢复
  # 参见 REM-009
  ```

#### REM-005: API Server 限流参数调优
- **适用根因**: RC-005
- **影响说明**: 调整 APF 配置不需要重启 API Server，但错误的配置可能导致更严重的限流或放开限制导致过载。
- **审批提示**: "建议调整 API Server 的限流配置（APF）以缓解限流问题。此操作不需要重启 API Server，但需要仔细评估配置。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前 APF 配置
  kubectl get flowschemas -o wide
  kubectl get prioritylevelconfigurations -o wide
  
  # 确认哪些请求被限流
  kubectl get --raw /metrics | grep apiserver_flowcontrol_rejected_requests_total
  ```
- **执行命令**:
  ```bash
  # 方式1: 增加特定 PriorityLevel 的并发数
  kubectl patch prioritylevelconfiguration <pl-name> \
    --type='merge' \
    -p='{"spec":{"limited":{"nominalConcurrencyShares":100}}}'
  
  # 方式2: 为特定客户端创建豁免 FlowSchema
  cat <<EOF | kubectl apply -f -
  apiVersion: flowcontrol.apiserver.k8s.io/v1
  kind: FlowSchema
  metadata:
    name: exempt-important-client
  spec:
    priorityLevelConfiguration:
      name: exempt
    rules:
    - subjects:
      - kind: ServiceAccount
        serviceAccount:
          name: important-client
          namespace: my-namespace
      resourceRules:
      - apiGroups: ["*"]
        clusterScope: true
        namespaces: ["*"]
        resources: ["*"]
        verbs: ["*"]
  EOF
  ```
- **后置验证**:
  ```bash
  # 确认限流减少
  kubectl get --raw /metrics | grep apiserver_flowcontrol_rejected_requests_total
  # 监控 5 分钟内拒绝数是否下降
  ```
- **回滚命令**:
  ```bash
  # 删除自定义 FlowSchema
  kubectl delete flowschema exempt-important-client
  
  # 恢复 PriorityLevel 配置
  kubectl patch prioritylevelconfiguration <pl-name> \
    --type='merge' \
    -p='{"spec":{"limited":{"nominalConcurrencyShares":<original-value>}}}'
  ```

#### REM-006: Scheduler/CM 重启恢复 leader 选举
- **适用根因**: RC-008
- **影响说明**: 重启 Scheduler 或 Controller Manager 会导致短暂的调度或控制器中断。在 HA 配置中，其他实例会接管 leader。
- **审批提示**: "建议重启 kube-scheduler/kube-controller-manager 以恢复 leader 选举。此操作会导致短暂（约 30s）的调度/控制器中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认 leader 选举状态
  kubectl get leases -n kube-system kube-scheduler kube-controller-manager -o yaml
  
  # 确认组件状态
  kubectl get pods -n kube-system -l component=kube-scheduler
  kubectl get pods -n kube-system -l component=kube-controller-manager
  ```
- **执行命令**:
  ```bash
  # 重启 Scheduler（如果使用 static pod）
  kubectl delete pod -n kube-system -l component=kube-scheduler
  
  # 重启 Controller Manager（如果使用 static pod）
  kubectl delete pod -n kube-system -l component=kube-controller-manager
  
  # 如果使用 systemd 管理
  ssh <control-plane-node> "systemctl restart kube-scheduler"
  ssh <control-plane-node> "systemctl restart kube-controller-manager"
  ```
- **后置验证**:
  ```bash
  # 等待 30s
  sleep 30
  
  # 确认组件恢复
  kubectl get pods -n kube-system -l component=kube-scheduler
  kubectl get pods -n kube-system -l component=kube-controller-manager
  
  # 确认 leader 选举成功
  kubectl get leases -n kube-system kube-scheduler kube-controller-manager -o jsonpath='{range .items[*]}{.metadata.name}: {.spec.holderIdentity}{"\n"}{end}'
  ```
- **回滚命令**:
  ```bash
  # 组件重启为幂等操作
  # 如果重启后问题未解决，需要深度排查配置或升级
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-007: etcd 成员替换（移除问题成员 + 添加新成员）
- **适用根因**: RC-002
- **影响说明**: 替换 etcd 成员涉及移除问题成员和添加新成员。在操作期间集群仍可用（假设仍有 quorum），但操作不当可能导致数据丢失或集群不可用。**此操作必须在 etcd 快照备份后执行**。
- **操作步骤**:
  1. **创建 etcd 快照备份**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       snapshot save /backup/etcd-snapshot-before-member-replace-$(date +%Y%m%d%H%M%S).db
     
     # 验证快照
     ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-snapshot-*.db -w table
     ```
  2. **识别问题成员**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       member list -w table
     # 记录问题成员的 ID
     ```
  3. **移除问题成员**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       member remove <member-id>
     ```
  4. **准备新成员**:
     ```bash
     # 在新节点上准备 etcd 数据目录
     rm -rf /var/lib/etcd/member
     
     # 复制证书（从现有控制平面节点）
     scp /etc/kubernetes/pki/etcd/* new-node:/etc/kubernetes/pki/etcd/
     ```
  5. **添加新成员**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       member add <new-member-name> --peer-urls=https://<new-node-ip>:2380
     ```
  6. **在新节点上启动 etcd**:
     ```bash
     # 修改 /etc/kubernetes/manifests/etcd.yaml
     # 设置 --initial-cluster-state=existing
     # 更新 --initial-cluster 列表
     ```
  7. **验证集群状态**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       endpoint health --cluster
     
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       member list -w table
     ```
- **安全检查**:
  - ⚠️ 操作前必须有可用的 etcd 快照备份
  - 确认 etcd 集群仍有 quorum（N/2+1 个健康成员）
  - 新节点网络与现有成员连通
  - 新节点时钟与集群同步
- **回滚方案**:
  ```bash
  # 如果添加成员失败，从快照恢复
  # 参见 REM-009
  ```

#### REM-008: 控制平面证书紧急续期
- **适用根因**: RC-006
- **影响说明**: 证书续期需要重启控制平面组件。在续期期间，集群可能短暂不可用。**此操作应在非高峰期执行，并提前通知用户**。
- **操作步骤**:
  1. **备份现有证书**:
     ```bash
     cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d%H%M%S)
     ```
  2. **检查证书状态**:
     ```bash
     kubeadm certs check-expiration
     ```
  3. **续期所有证书**:
     ```bash
     kubeadm certs renew all
     ```
  4. **重启控制平面组件**:
     ```bash
     # 触发 static pod 重启（kubelet 会检测配置变化）
     # 方式1: 移动 manifest 再恢复
     mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
     sleep 10
     mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
     
     # 对其他组件重复
     for component in kube-controller-manager kube-scheduler; do
       mv /etc/kubernetes/manifests/${component}.yaml /tmp/
       sleep 10
       mv /tmp/${component}.yaml /etc/kubernetes/manifests/
     done
     ```
  5. **更新 kubeconfig 文件**:
     ```bash
     kubeadm init phase kubeconfig all
     
     # 更新用户 kubeconfig
     cp /etc/kubernetes/admin.conf ~/.kube/config
     ```
  6. **验证证书有效**:
     ```bash
     kubeadm certs check-expiration
     kubectl get nodes
     ```
- **安全检查**:
  - 备份现有证书
  - 在非高峰期执行
  - 通知可能受影响的用户
  - 确保 kubeadm 版本与集群版本匹配
- **回滚方案**:
  ```bash
  # 恢复备份的证书
  rm -rf /etc/kubernetes/pki
  cp -r /etc/kubernetes/pki.bak.<timestamp> /etc/kubernetes/pki
  
  # 重启控制平面组件
  ```

#### REM-009: etcd 从快照恢复
- **适用根因**: RC-010, RC-002（quorum 完全丢失）
- **影响说明**: 从快照恢复会**覆盖**当前 etcd 数据。快照后的所有变更将丢失。这是**破坏性操作**，只应在其他方法都失败时使用。
- **操作步骤**:
  1. **停止所有 etcd 成员**:
     ```bash
     # 在所有控制平面节点上
     mv /etc/kubernetes/manifests/etcd.yaml /tmp/
     ```
  2. **清理旧数据目录**:
     ```bash
     # 在所有控制平面节点上
     rm -rf /var/lib/etcd/member
     ```
  3. **在第一个节点上从快照恢复**:
     ```bash
     ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
       --name=<member-name> \
       --data-dir=/var/lib/etcd \
       --initial-cluster=<member1>=https://<ip1>:2380,<member2>=https://<ip2>:2380,<member3>=https://<ip3>:2380 \
       --initial-cluster-token=etcd-cluster \
       --initial-advertise-peer-urls=https://<this-node-ip>:2380
     ```
  4. **在其他节点上重复恢复**:
     ```bash
     # 使用相同的快照，但不同的 --name 和 --initial-advertise-peer-urls
     ```
  5. **启动 etcd**:
     ```bash
     # 在所有控制平面节点上
     mv /tmp/etcd.yaml /etc/kubernetes/manifests/
     ```
  6. **验证恢复**:
     ```bash
     ETCDCTL_API=3 etcdctl \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/peer.crt \
       --key=/etc/kubernetes/pki/etcd/peer.key \
       endpoint health --cluster
     
     kubectl get nodes
     kubectl get pods -A
     ```
- **安全检查**:
  - ⚠️ 这是**破坏性操作**，会丢失快照后的所有数据
  - 仅在 etcd 集群完全不可用且其他方法失败时使用
  - 必须在所有 etcd 成员上执行相同的恢复操作
  - 恢复后检查是否有资源不一致（如 Pod 存在于 etcd 但节点上已不存在）
- **回滚方案**:
  ```bash
  # 从快照恢复后无法再回滚到"恢复前"的状态
  # 只能使用另一个快照再次恢复
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: etcd 集群完全重建
- **适用根因**: RC-010（数据严重损坏），灾难恢复
- **审批要求**: 需要高级 SRE + 架构师 + 管理层审批
- **数据备份**: 必须有可用的 etcd 快照，否则将丢失所有集群数据
- **操作步骤**:
  1. **评估损坏程度**: 确认无法通过成员替换或快照恢复修复
  2. **准备新的 etcd 集群**: 可能需要新的控制平面节点
  3. **从快照恢复或重新初始化**: 
     - 如果有快照: 参见 REM-009
     - 如果无快照: 需要重新初始化集群并重建所有资源
  4. **重建控制平面组件**: 更新所有组件配置指向新 etcd
  5. **验证并协调**: 处理可能的资源不一致
- **回滚方案**: 完全重建后无法回滚，需要仔细规划

#### REM-011: 控制平面组件完全重新部署
- **适用根因**: RC-011（严重内存泄漏），重大配置损坏
- **审批要求**: 需要高级 SRE 审批
- **操作步骤**:
  1. **备份现有配置**:
     ```bash
     cp -r /etc/kubernetes/manifests /etc/kubernetes/manifests.bak.$(date +%Y%m%d%H%M%S)
     cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d%H%M%S)
     cp /etc/kubernetes/*.conf /tmp/kubeconfig-backup/
     ```
  2. **使用 kubeadm 重新生成配置**:
     ```bash
     kubeadm init phase control-plane all --config=<kubeadm-config.yaml>
     ```
  3. **重启控制平面**:
     ```bash
     # kubelet 会自动检测 manifest 变化并重启 pod
     ```
  4. **验证**:
     ```bash
     kubectl get componentstatuses
     kubectl get nodes
     ```
- **回滚方案**:
  ```bash
  # 恢复备份的 manifest
  rm -rf /etc/kubernetes/manifests
  cp -r /etc/kubernetes/manifests.bak.<timestamp> /etc/kubernetes/manifests
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 2-5 分钟内）

```bash
# V1: etcd 集群健康检查
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  endpoint health --cluster
# 预期: 所有成员显示 "is healthy: true"

# V2: API Server 就绪检查
kubectl get --raw /readyz?verbose
# 预期: 所有检查项返回 [+]xxx ok

# V3: 控制平面组件状态
kubectl get pods -n kube-system -l tier=control-plane
# 预期: 所有 Pod 为 Running 状态，RESTARTS 为 0 或稳定

# V4: API 响应延迟
time kubectl get nodes >/dev/null 2>&1
# 预期: 响应时间 < 1s

# V5: 基本集群操作
kubectl create configmap test-verify --from-literal=test=ok -n kube-system
kubectl delete configmap test-verify -n kube-system
# 预期: 创建和删除成功，无延迟

# V6: 证书有效期（如果修复了证书问题）
kubeadm certs check-expiration
# 预期: 所有证书有效期 > 30 天
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| etcd leader 稳定 | `etcd_server_leader_changes_seen_total` | 无增长 | 30min 内 >2 次 leader 切换 |
| etcd 磁盘延迟 | `etcd_disk_wal_fsync_duration_seconds` P99 | < 10ms | P99 > 50ms |
| etcd 数据库大小 | `etcd_mvcc_db_total_size_in_bytes` | 稳定或下降（如果做了压缩） | 持续增长接近 quota |
| API 请求延迟 | `apiserver_request_duration_seconds` P99 | < 1s | P99 > 5s |
| API 请求限流 | `apiserver_flowcontrol_rejected_requests_total` | 无增长 | 持续增长 |
| 控制平面组件重启 | `kubectl get pods -n kube-system -l tier=control-plane` | RESTARTS 不变 | 有组件重启 |
| Webhook 延迟 | `apiserver_admission_webhook_admission_duration_seconds` P99 | < 5s | P99 > 30s |
| work queue 深度 | `workqueue_depth` | < 100 | 持续 > 500 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] etcd 集群所有成员健康（`endpoint health` 全部 `is healthy: true`）
- [ ] etcd 无 ALARM（`alarm list` 无输出）
- [ ] API Server `/readyz?verbose` 所有检查通过
- [ ] 所有控制平面组件 Pod 状态为 Running
- [ ] API 请求延迟恢复正常（P99 < 1s）
- [ ] 无新的限流拒绝（429 响应）
- [ ] Scheduler 和 Controller Manager 有 leader 且稳定
- [ ] 可以正常创建/更新/删除资源

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| etcd leader 稳定性 | `etcd_server_leader_changes_seen_total` 增长率 | 每小时 | leader 频繁切换 → 检查网络和磁盘性能 |
| etcd 磁盘空间 | `etcd_mvcc_db_total_size_in_bytes` 趋势 | 每小时 | 持续增长 → 检查压缩配置或清理旧资源 |
| API Server OOM | apiserver Pod 重启次数 | 每 4 小时 | 重启 → 检查内存泄漏和审计配置 |
| 证书有效期 | `kubeadm certs check-expiration` | 每日 | <7 天 → 预防性续期 |
| 控制平面组件健康 | 组件 Pod 状态和日志 | 每 4 小时 | Warning 日志 → 深度分析 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 或 Phase 3 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V6 验证失败 |
| **严重性升级** | 初始分级为 P1/P2 但情况恶化（如 etcd 成员持续减少） | 诊断过程中状况恶化 |
| **未知根因** | 完成所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常发现 |
| **操作权限不足** | 无法 SSH 到控制平面节点执行 etcd 诊断 | Phase 2 需要 SSH 但无权限 |
| **etcd 数据风险** | etcd 报告 CORRUPT alarm 或成员间数据不一致 | 任何时候发现数据一致性问题 |
| **托管集群** | 托管集群控制平面问题，用户端无法修复 | ACK/EKS/GKE 控制台显示控制平面异常 |

### 8.2 升级消息模板

```
【{severity}】控制平面问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {summary}
- 影响范围: 
  - 集群类型: {cluster_type} (自建/ACK/EKS/GKE)
  - API Server 状态: {apiserver_status}
  - etcd 状态: {etcd_status} ({healthy_members}/{total_members} 成员健康)
  - Scheduler/CM 状态: {scheduler_cm_status}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 etcd 深度诊断: {phase2_summary}
  - Phase 3 API Server 诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 数据风险:
  - etcd 备份状态: {etcd_backup_status}
  - 最近备份时间: {last_backup_time}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-CP-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   # etcd 状态
   etcdctl endpoint status --cluster -w table > etcd-status.txt
   etcdctl member list -w table >> etcd-status.txt
   etcdctl alarm list >> etcd-status.txt
   
   # 控制平面组件日志
   kubectl logs -n kube-system -l component=kube-apiserver --tail=200 > apiserver-logs.txt
   kubectl logs -n kube-system -l component=etcd --tail=200 > etcd-logs.txt
   
   # 事件
   kubectl get events -n kube-system --sort-by=.lastTimestamp > kube-system-events.txt
   ```
5. **事件时间线**: 最近 1 小时内的关键事件按时间排列
6. **备份状态**: etcd 快照位置和最后备份时间

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| etcd 默认版本 | 3.5.9 | 3.5.10 | 3.5.12 | 3.5.13 | 3.5.15 |
| API Priority and Fairness (APF) | GA | GA | GA（改进借用语义）| GA | GA |
| `/readyz` verbose 检查项 | 基础 | 增加 informer-sync | 增加更多检查 | 新增 shutdown | 稳定 |
| kubeadm certs 命令 | 基础 | 改进 | GA | GA | GA |
| etcd 健康检查端点 | `/health` | `/health` | `/health` + `/livez` | 同左 | 同左 |
| Leader Election 租约超时 | 15s | 15s | 15s | 可配置 | 可配置 |
| Scheduler Component Config | v1 | v1 | v1 | v1 | v1 |
| 审计日志动态配置 | beta | beta | GA | GA | GA |
| API Server 优雅关闭 | beta | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get --raw /readyz?verbose` | 支持 | 新增检查项 | 更多检查项 | 新增 shutdown | 同左 |
| `kubectl get --raw /livez?verbose` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `etcdctl endpoint status` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubeadm certs check-expiration` | 支持 | 改进输出 | GA | GA | GA |
| `kubeadm certs renew` | 支持 | 支持 | 支持 | 支持 | 支持 |
| etcdctl 版本 | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| FlowSchema | flowcontrol.apiserver.k8s.io/v1 | v1 | v1 | v1 | v1 |
| PriorityLevelConfiguration | flowcontrol.apiserver.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Lease | coordination.k8s.io/v1 | v1 | v1 | v1 | v1 |
| ValidatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1 | v1 | v1 | v1 |
| MutatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1 | v1 | v1 | v1 |

### 9.4 托管集群差异

| 特性 | 阿里云 ACK | AWS EKS | Google GKE |
|------|-----------|---------|------------|
| 控制平面访问 | 不可见 | 不可见 | 不可见 |
| etcd 管理 | 托管 | 托管 | 托管 |
| 控制平面日志 | 可通过 SLS 查看 | 可通过 CloudWatch 查看 | 可通过 Cloud Logging 查看 |
| 证书管理 | 自动 | 自动 | 自动 |
| API Server 扩展 | 通过 ACK 控制台 | 通过 EKS 控制台 | 通过 GKE 控制台 |
| 控制平面升级 | 控制台操作 | 控制台/CLI | 控制台/CLI |
| SLA | 99.95% | 99.95% | 99.95% |
| 故障排查 | 联系阿里云支持 | 联系 AWS 支持 | 联系 Google 支持 |

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 Webhook 延迟误判为 etcd 慢** | API 请求延迟高，怀疑 etcd 性能问题 | 实际是 Admission Webhook 服务响应慢，etcd 指标正常 | 在 D2.3 检查 etcd 性能的同时，用 D3.3 检查 Webhook 延迟。如果 etcd fsync < 10ms 但 API 仍慢，优先排查 Webhook |
| **将网络分区误判为 etcd 成员问题** | etcd 成员显示 unhealthy，怀疑成员崩溃 | 实际是网络问题导致成员间通信失败，但各成员本地进程正常 | 在判断成员问题前，先检查 D2.7 网络延迟。如果能 SSH 到"问题"成员且 etcd 进程运行正常，优先排查网络 |
| **将 APF 限流误判为 API Server 过载** | 收到 429 响应，怀疑 API Server 负载过高 | 实际是 APF 配置限制了特定客户端，API Server 本身资源充足 | 用 D3.4 检查 APF 配置和拒绝指标。如果只有特定 FlowSchema 被限流，问题在配置而非负载 |
| **将 Scheduler 限流误判为 Scheduler 问题** | Pod 长期 Pending，Scheduler 日志有限流信息 | 实际是 Scheduler 被 APF 限流，而非 Scheduler 本身问题 | 检查 Scheduler 作为 API 客户端是否被限流。APF 可能限制了 Scheduler 的 LIST/WATCH 请求 |
| **将证书即将过期误判为已过期** | TLS 错误，怀疑证书已过期 | 实际是客户端时钟偏差导致认为证书"未来"才有效 | 同时检查证书有效期和系统时钟。时钟偏差可能导致即使证书有效也出现 TLS 错误 |
| **将 etcd 碎片化误判为配额不足** | etcd DB SIZE 接近 quota | 实际是碎片化导致 dbSize 虚高，实际使用量（dbSizeInUse）远低于 quota | 用 D2.5 同时检查 dbSize 和 dbSizeInUse。如果 dbSize >> dbSizeInUse，先 defrag 而非扩容 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| API Server 架构与内部机制 | `domain-10-troubleshooting-diagnostics/01-control-plane-apiserver-troubleshooting.md` | 深度理解 API Server 请求处理、限流、Webhook 集成 |
| etcd 故障排查深度指南 | `domain-10-troubleshooting-diagnostics/02-control-plane-etcd-troubleshooting.md` | 深度理解 etcd 集群运维、数据恢复、性能调优 |
| 控制平面组件架构 | `domain-01-cluster-fundamentals/` | 理解 Scheduler、Controller Manager 工作原理 |
| Kubernetes 架构基础 | `domain-01-cluster-fundamentals/` | 控制平面整体架构和组件交互 |
| 证书管理与安全 | `SKILL-SEC-001` | 证书过期的详细诊断与修复 |
| 节点故障排查 | `SKILL-NODE-001` | 控制平面节点 NotReady 的排查 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 13 个根因、11 个修复操作 | 基于 top 工单分析确定控制平面问题为高优先级场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **etcd 加密静态数据**: KMS 集成和加密密钥轮转相关问题
2. **多集群联邦控制平面**: Kubernetes Federation v2 的控制平面问题
3. **API Server 高可用负载均衡**: HAProxy/Keepalived/云 LB 层面的问题
4. **etcd operator 管理模式**: 使用 etcd-operator 的集群的特定故障模式
5. **边缘场景**: 边缘集群中控制平面与边缘节点的连接问题
6. **AIoT 场景**: 大规模设备接入导致的 API Server 负载问题

---

## 附录 A：自动化诊断脚本

### A.1 控制平面快速健康检查 (diagnose-cp-quick.sh)

```bash
#!/bin/bash
# =============================================================================
# 控制平面快速健康检查脚本
# Usage: bash diagnose-cp-quick.sh [--etcd-endpoints <endpoints>]
# Risk: NONE (read-only operations)
# Source: SKILL-CP-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

控制平面快速健康检查脚本 - 检查 API Server、etcd、Scheduler、Controller Manager 状态

Options:
    --etcd-endpoints    etcd 端点地址 (默认: https://127.0.0.1:2379)
    --etcd-cacert       etcd CA 证书路径
    --etcd-cert         etcd 客户端证书路径
    --etcd-key          etcd 客户端密钥路径
    --help, -h          显示帮助信息

Examples:
    $(basename "$0")
    $(basename "$0") --etcd-endpoints https://10.0.0.1:2379
EOF
    exit 0
}

# --- 默认参数 ---
ETCD_ENDPOINTS="https://127.0.0.1:2379"
ETCD_CACERT="/etc/kubernetes/pki/etcd/ca.crt"
ETCD_CERT="/etc/kubernetes/pki/etcd/peer.crt"
ETCD_KEY="/etc/kubernetes/pki/etcd/peer.key"

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --etcd-endpoints) ETCD_ENDPOINTS="$2"; shift 2 ;;
        --etcd-cacert)    ETCD_CACERT="$2"; shift 2 ;;
        --etcd-cert)      ETCD_CERT="$2"; shift 2 ;;
        --etcd-key)       ETCD_KEY="$2"; shift 2 ;;
        --help|-h)        usage ;;
        *)                error "未知参数: $1"; usage ;;
    esac
done

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    error "kubectl 未安装或不在 PATH 中"
    exit 1
fi

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  控制平面快速健康检查${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "  时间: $(date -u '+%Y-%m-%d %H:%M:%S UTC')\n"

# --- Step 1: API Server 健康检查 ---
info "[1/6] 检查 API Server 健康状态..."
HEALTHZ=$(kubectl get --raw /healthz --request-timeout=10s 2>/dev/null || echo "unreachable")
if "$HEALTHZ" == "ok"; then
    success "API Server /healthz: ok"
else
    error "API Server /healthz: $HEALTHZ"
fi

READYZ=$(kubectl get --raw /readyz --request-timeout=10s 2>/dev/null || echo "unreachable")
if "$READYZ" == "ok"; then
    success "API Server /readyz: ok"
else
    warn "API Server /readyz: $READYZ"
fi

LIVEZ=$(kubectl get --raw /livez --request-timeout=10s 2>/dev/null || echo "unreachable")
if "$LIVEZ" == "ok"; then
    success "API Server /livez: ok"
else
    warn "API Server /livez: $LIVEZ"
fi

# --- Step 2: etcd 集群健康检查 ---
info "[2/6] 检查 etcd 集群健康..."
if command -v etcdctl &>/dev/null && -f "$ETCD_CACERT"; then
    ETCD_HEALTH=$(ETCDCTL_API=3 etcdctl \
        --endpoints="$ETCD_ENDPOINTS" \
        --cacert="$ETCD_CACERT" \
        --cert="$ETCD_CERT" \
        --key="$ETCD_KEY" \
        endpoint health --cluster 2>/dev/null || echo "check failed")
    
    if echo "$ETCD_HEALTH" | grep -q "is healthy"; then
        success "etcd 集群健康"
        echo "$ETCD_HEALTH" | while read line; do echo "    $line"; done
    else
        error "etcd 集群状态异常:"
        echo "$ETCD_HEALTH" | while read line; do echo "    $line"; done
    fi
    
    # 检查 alarm
    ETCD_ALARM=$(ETCDCTL_API=3 etcdctl \
        --endpoints="$ETCD_ENDPOINTS" \
        --cacert="$ETCD_CACERT" \
        --cert="$ETCD_CERT" \
        --key="$ETCD_KEY" \
        alarm list 2>/dev/null || true)
    
    if -n "$ETCD_ALARM"; then
        error "etcd 存在 ALARM:"
        echo "$ETCD_ALARM" | while read line; do echo "    $line"; done
    else
        success "etcd 无 ALARM"
    fi
else
    warn "跳过 etcd 检查 (etcdctl 不可用或证书不存在)"
fi

# --- Step 3: Scheduler/Controller Manager Leader 选举状态 ---
info "[3/6] 检查 Scheduler/Controller Manager Leader 选举状态..."
SCHED_LEASE=$(kubectl get lease kube-scheduler -n kube-system -o jsonpath='{.spec.holderIdentity}' 2>/dev/null || true)
if -n "$SCHED_LEASE"; then
    success "kube-scheduler leader: $SCHED_LEASE"
else
    warn "kube-scheduler leader 未找到"
fi

CM_LEASE=$(kubectl get lease kube-controller-manager -n kube-system -o jsonpath='{.spec.holderIdentity}' 2>/dev/null || true)
if -n "$CM_LEASE"; then
    success "kube-controller-manager leader: $CM_LEASE"
else
    warn "kube-controller-manager leader 未找到"
fi

# --- Step 4: 控制平面 Pod 状态 ---
info "[4/6] 检查控制平面 Pod 状态..."
CP_PODS=$(kubectl get pods -n kube-system -l tier=control-plane --no-headers 2>/dev/null || true)
if -n "$CP_PODS"; then
    NOT_RUNNING=$(echo "$CP_PODS" | grep -v "Running" || true)
    if -n "$NOT_RUNNING"; then
        error "控制平面 Pod 异常:"
        echo "$NOT_RUNNING" | while read line; do echo "    $line"; done
    else
        success "控制平面 Pod 全部 Running"
    fi
else
    warn "未找到控制平面 Pod (可能是托管集群)"
fi

# --- Step 5: 证书过期时间检查 ---
info "[5/6] 检查证书过期时间..."
if command -v kubeadm &>/dev/null; then
    CERT_INFO=$(kubeadm certs check-expiration 2>/dev/null || true)
    if -n "$CERT_INFO"; then
        # 检查是否有6天内过期
        if echo "$CERT_INFO" | grep -qE "[0-9]+d|invalid"; then
            EXPIRING=$(echo "$CERT_INFO" | grep -E "([0-6]d|invalid|expired)" || true)
            if -n "$EXPIRING"; then
                warn "以下证书即将过期或已过期:"
                echo "$EXPIRING" | while read line; do echo "    $line"; done
            else
                success "证书有效期正常"
            fi
        else
            success "证书检查完成"
        fi
    fi
else
    warn "跳过证书检查 (kubeadm 不可用)"
fi

# --- Step 6: 输出控制平面健康摘要 ---
info "[6/6] 生成控制平面健康摘要..."
echo -e "\n${CYAN}${BOLD}── 控制平面健康摘要 ──${NC}"
cat <<EOF
{
  "timestamp": "$(date -u '+%Y-%m-%dT%H:%M:%SZ')",
  "apiserver": {
    "healthz": "$HEALTHZ",
    "readyz": "$READYZ",
    "livez": "$LIVEZ"
  },
  "scheduler_leader": "${SCHED_LEASE:-unknown}",
  "controller_manager_leader": "${CM_LEASE:-unknown}",
  "etcd_healthy": $(echo "$ETCD_HEALTH" | grep -q "is healthy" && echo "true" || echo "false")
}
EOF

echo -e "\n${GREEN}控制平面健康检查完成${NC}"
```

### A.2 etcd 性能诊断脚本 (diagnose-etcd-perf.sh)

```bash
#!/bin/bash
# =============================================================================
# etcd 性能诊断脚本
# Usage: bash diagnose-etcd-perf.sh [OPTIONS]
# Risk: NONE (read-only, except perf check has minor impact)
# Source: SKILL-CP-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
success() { echo -e "${GREEN}[OK]${NC} $*"; }

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

etcd 性能诊断脚本 - 检查 etcd 性能指标

Options:
    --endpoints     etcd 端点地址 (默认: https://127.0.0.1:2379)
    --cacert        etcd CA 证书路径
    --cert          etcd 客户端证书路径
    --key           etcd 客户端密钥路径
    --run-perf      执行性能测试 (etcdctl check perf)
    --help, -h      显示帮助信息
EOF
    exit 0
}

# --- 默认参数 ---
ETCD_ENDPOINTS="https://127.0.0.1:2379"
ETCD_CACERT="/etc/kubernetes/pki/etcd/ca.crt"
ETCD_CERT="/etc/kubernetes/pki/etcd/peer.crt"
ETCD_KEY="/etc/kubernetes/pki/etcd/peer.key"
RUN_PERF=false

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --endpoints)  ETCD_ENDPOINTS="$2"; shift 2 ;;
        --cacert)     ETCD_CACERT="$2"; shift 2 ;;
        --cert)       ETCD_CERT="$2"; shift 2 ;;
        --key)        ETCD_KEY="$2"; shift 2 ;;
        --run-perf)   RUN_PERF=true; shift ;;
        --help|-h)    usage ;;
        *)            error "未知参数: $1"; usage ;;
    esac
done

# --- 前置检查 ---
if ! command -v etcdctl &>/dev/null; then
    error "etcdctl 未安装或不在 PATH 中"
    exit 1
fi

if ! -f "$ETCD_CACERT"; then
    error "etcd CA 证书不存在: $ETCD_CACERT"
    exit 1
fi

# --- etcdctl 通用参数 ---
ETCDCTL_OPTS=(--endpoints="$ETCD_ENDPOINTS" --cacert="$ETCD_CACERT" --cert="$ETCD_CERT" --key="$ETCD_KEY")

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  etcd 性能诊断${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "  时间: $(date -u '+%Y-%m-%d %H:%M:%S UTC')\n"

# --- Step 1: etcd member list ---
info "[1/5] 检查 etcd 成员列表..."
MEMBER_LIST=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" member list -w table 2>/dev/null || true)
if -n "$MEMBER_LIST"; then
    success "etcd 成员列表:"
    echo "$MEMBER_LIST" | while read line; do echo "    $line"; done
else
    error "无法获取 etcd 成员列表"
fi

# --- Step 2: endpoint status ---
info "[2/5] 检查 etcd 端点状态..."
ENDPOINT_STATUS=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" endpoint status --cluster -w table 2>/dev/null || true)
if -n "$ENDPOINT_STATUS"; then
    success "etcd 端点状态:"
    echo "$ENDPOINT_STATUS" | while read line; do echo "    $line"; done
    
    # 检查 DB 大小
    DB_SIZE=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" endpoint status -w json 2>/dev/null | \
              jq -r '.[0].Status.dbSize' 2>/dev/null || echo "0")
    DB_SIZE_MB=$((DB_SIZE / 1024 / 1024))
    if $DB_SIZE_MB -gt 2048; then
        warn "etcd DB 大小: ${DB_SIZE_MB}MB (超过默认 quota 2GB)"
    else
        success "etcd DB 大小: ${DB_SIZE_MB}MB"
    fi
else
    error "无法获取 etcd 端点状态"
fi

# --- Step 3: 检查 alarm 状态 ---
info "[3/5] 检查 etcd alarm 状态..."
ALARM_LIST=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" alarm list 2>/dev/null || true)
if -z "$ALARM_LIST"; then
    success "etcd 无 alarm"
else
    error "etcd 存在 alarm:"
    echo "$ALARM_LIST" | while read line; do echo "    $line"; done
fi

# --- Step 4: 检查碎片化比例 ---
info "[4/5] 检查 etcd 数据库碎片化..."
DB_INFO=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" endpoint status --cluster -w json 2>/dev/null || echo "[]")
if "$DB_INFO" != "[]"; then
    echo "$DB_INFO" | jq -r '.[] | "\(.Endpoint): dbSize=\(.Status.dbSize) dbSizeInUse=\(.Status.dbSizeInUse // .Status.dbSize)"' 2>/dev/null | \
    while read line; do
        ENDPOINT=$(echo "$line" | cut -d: -f1-2)
        DB_SIZE=$(echo "$line" | grep -oP 'dbSize=\K[0-9]+')
        DB_IN_USE=$(echo "$line" | grep -oP 'dbSizeInUse=\K[0-9]+')
        if -n "$DB_SIZE" && -n "$DB_IN_USE" && "$DB_IN_USE" -gt 0; then
            FRAG_RATIO=$(echo "scale=2; $DB_SIZE / $DB_IN_USE" | bc 2>/dev/null || echo "1")
            if (( $(echo "$FRAG_RATIO > 2.0" | bc -l 2>/dev/null || echo 0) )); then
                warn "$ENDPOINT 碎片化比例: $FRAG_RATIO (建议 defrag)"
            else
                success "$ENDPOINT 碎片化比例: $FRAG_RATIO"
            fi
        fi
    done
fi

# --- Step 5: 运行性能测试 (可选) ---
if "$RUN_PERF" == "true"; then
    info "[5/5] 运行 etcd 性能测试 (60s)..."
    PERF_RESULT=$(ETCDCTL_API=3 etcdctl "${ETCDCTL_OPTS[@]}" check perf 2>&1 || true)
    if echo "$PERF_RESULT" | grep -q "PASS"; then
        success "etcd 性能测试通过:"
    else
        warn "etcd 性能测试结果:"
    fi
    echo "$PERF_RESULT" | while read line; do echo "    $line"; done
else
    info "[5/5] 跳过性能测试 (使用 --run-perf 启用)"
fi

echo -e "\n${GREEN}etcd 性能诊断完成${NC}"
```

### A.3 控制平面修复后验证 (verify-control-plane.sh)

```bash
#!/bin/bash
# =============================================================================
# 控制平面修复后验证脚本
# Usage: bash verify-control-plane.sh [OPTIONS]
# Risk: NONE (read-only operations)
# Source: SKILL-CP-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# --- 输出函数 ---
info()    { echo -e "${BLUE}[INFO]${NC} $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $*"; }
success() { echo -e "${GREEN}[PASS]${NC} $*"; ((PASS_COUNT++)); }
fail()    { echo -e "${RED}[FAIL]${NC} $*"; ((FAIL_COUNT++)); }

# --- 统计 ---
PASS_COUNT=0
FAIL_COUNT=0

# --- 帮助信息 ---
usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

控制平面修复后验证脚本 - 验证控制平面恢复正常

Options:
    --etcd-endpoints    etcd 端点地址
    --etcd-cacert       etcd CA 证书路径
    --etcd-cert         etcd 客户端证书路径
    --etcd-key          etcd 客户端密钥路径
    --help, -h          显示帮助信息
EOF
    exit 0
}

# --- 默认参数 ---
ETCD_ENDPOINTS="https://127.0.0.1:2379"
ETCD_CACERT="/etc/kubernetes/pki/etcd/ca.crt"
ETCD_CERT="/etc/kubernetes/pki/etcd/peer.crt"
ETCD_KEY="/etc/kubernetes/pki/etcd/peer.key"

# --- 参数解析 ---
while $# -gt 0; do
    case "$1" in
        --etcd-endpoints) ETCD_ENDPOINTS="$2"; shift 2 ;;
        --etcd-cacert)    ETCD_CACERT="$2"; shift 2 ;;
        --etcd-cert)      ETCD_CERT="$2"; shift 2 ;;
        --etcd-key)       ETCD_KEY="$2"; shift 2 ;;
        --help|-h)        usage ;;
        *)                warn "未知参数: $1"; shift ;;
    esac
done

# --- 前置检查 ---
if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}kubectl 未安装${NC}"
    exit 1
fi

echo -e "\n${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}${BOLD}  控制平面修复后验证${NC}"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════${NC}\n"

# --- V1: 验证控制平面组件 Running ---
info "[V1] 验证控制平面组件状态..."
CP_PODS=$(kubectl get pods -n kube-system -l tier=control-plane --no-headers 2>/dev/null || true)
if -n "$CP_PODS"; then
    NOT_RUNNING=$(echo "$CP_PODS" | grep -v "Running" || true)
    if -z "$NOT_RUNNING"; then
        success "控制平面组件全部 Running"
    else
        fail "控制平面组件存在异常"
    fi
else
    info "未找到控制平面 Pod (托管集群跳过)"
fi

# --- V2: 验证 etcd 集群 healthy ---
info "[V2] 验证 etcd 集群健康..."
if command -v etcdctl &>/dev/null && -f "$ETCD_CACERT"; then
    ETCD_HEALTH=$(ETCDCTL_API=3 etcdctl \
        --endpoints="$ETCD_ENDPOINTS" \
        --cacert="$ETCD_CACERT" \
        --cert="$ETCD_CERT" \
        --key="$ETCD_KEY" \
        endpoint health --cluster 2>&1 || true)
    
    if echo "$ETCD_HEALTH" | grep -q "is healthy" && ! echo "$ETCD_HEALTH" | grep -q "is unhealthy"; then
        success "etcd 集群全部 healthy"
    else
        fail "etcd 集群存在 unhealthy 成员"
    fi
else
    info "跳过 etcd 检查"
fi

# --- V3: 验证 API Server 请求延迟正常 ---
info "[V3] 验证 API Server 请求延迟..."
START_TIME=$(date +%s%3N)
kubectl get nodes &>/dev/null
END_TIME=$(date +%s%3N)
LATENCY=$((END_TIME - START_TIME))

if $LATENCY -lt 1000; then
    success "API Server 延迟: ${LATENCY}ms (<1s)"
else
    fail "API Server 延迟: ${LATENCY}ms (超过 1s)"
fi

# --- V4: 验证证书有效期 > 30 天 ---
info "[V4] 验证证书有效期..."
if command -v kubeadm &>/dev/null; then
    CERT_INFO=$(kubeadm certs check-expiration 2>/dev/null || true)
    if -n "$CERT_INFO"; then
        # 检查是否有30天内过期的证书
        EXPIRING_SOON=$(echo "$CERT_INFO" | grep -E "[0-2][0-9]d" | grep -vE "[3-9][0-9]d" || true)
        if -z "$EXPIRING_SOON"; then
            success "所有证书有效期 > 30 天"
        else
            fail "部分证书将在30天内过期"
        fi
    else
        info "无法获取证书信息"
    fi
else
    info "跳过证书检查 (kubeadm 不可用)"
fi

# --- V5: 验证集群操作正常 ---
info "[V5] 验证集群操作正常..."
TEST_CM="cp-verify-test-$(date +%s)"
if kubectl create configmap "$TEST_CM" --from-literal=test=ok -n kube-system &>/dev/null; then
    kubectl delete configmap "$TEST_CM" -n kube-system &>/dev/null || true
    success "集群读写操作正常"
else
    fail "集群读写操作失败"
fi

# --- 输出验证结果 ---
echo -e "\n${BOLD}════════════════════════════════════════════════════════${NC}"
TOTAL=$((PASS_COUNT + FAIL_COUNT))
if $FAIL_COUNT -eq 0; then
    echo -e "${GREEN}${BOLD}验证结果: 全部通过 ($PASS_COUNT/$TOTAL)${NC}"
    exit 0
else
    echo -e "${RED}${BOLD}验证结果: 存在失败 (通过: $PASS_COUNT, 失败: $FAIL_COUNT)${NC}"
    exit 1
fi
```
