---
title: 控制平面不可用（kube-apiserver）诊断与修复
description: 针对 kube-apiserver 不可用、控制面组件崩溃、认证/准入失败、APF 限流等集群级故障的完整诊断技能，含症状识别、快速分级、诊断工作流、证据三元组与修复操作
summary: 控制平面是集群的中枢，apiserver 不可用会导致整个集群失控。本技能提供从症状识别到根因确认、恢复验证的生产级诊断路径
category: skill
tags:
- k8s
- cluster
- controlplane
- apiserver
- kube-controller-manager
- kube-scheduler
- apf
- troubleshooting
- sop
- runbook
sources:
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- code/apiserver-master/
- code/kube-controller-manager-master/
- code/kube-scheduler-master/
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- apiserver 连不上怎么排查
- 控制平面不可用如何恢复
- kubectl 全部超时什么原因
- apiserver 频繁重启怎么办
- API 请求被限流 429 怎么处理
trigger_keywords:
- apiserver
- kube-apiserver
- 控制平面
- control plane
- kubectl 超时
- 429 Too Many Requests
- APF
- controller-manager
- 集群失控
prerequisites:
- kubectl-basics
- cluster-architecture
- troubleshooting-methodology
skill_id: SKILL-CLUSTER-001
skill_name: 控制平面不可用（kube-apiserver）诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L1-manual-first
fta_path: TE-C -> IE-C.1 -> BE-C.1
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险/只读。
>
> **控制平面操作警告**：apiserver / etcd 属于全集群影响面。所有写操作前必须完成 etcd 快照，变更由高级工程师双人复核。本技能默认执行模式为 **L1-manual-first**——诊断只读命令可自动执行，任何恢复动作均需人工确认。

# 控制平面不可用（kube-apiserver）诊断与修复

> **Skill ID**: SKILL-CLUSTER-001
> **Agent 执行模式**: L1-manual-first — 只读诊断自动执行，恢复操作强制人工审批
> **FTA 路径**: TE-C → IE-C.1 → BE-C.1

---

## 1. 概述

kube-apiserver 是 Kubernetes 集群的唯一入口与状态中枢，所有组件（kubelet、controller-manager、scheduler、kubectl）都通过它读写 etcd。apiserver 不可用意味着**整个集群失控**：无法调度、无法自愈、无法运维。

**覆盖范围**：apiserver 进程崩溃/OOM、无法启动、频繁重启、响应超时、APF 限流（429）、认证/准入 Webhook 阻断、controller-manager / scheduler 选主失败。

**前置条件**：具备控制平面节点 SSH 访问权限与集群 admin kubeconfig。

**边界**：etcd 本身故障 → 转 [02-etcd-troubleshooting.md](02-etcd-troubleshooting.md)；证书过期 → 转 [03-cluster-cert-upgrade.md](03-cluster-cert-upgrade.md)。

---

## 2. 症状识别

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | kubectl 所有请求超时/拒绝连接 | `kubectl get ns` 报 `connection refused`/`i/o timeout` | 0.90 | 本地 kubeconfig/网络问题需先排除 |
| S2 | apiserver 静态 Pod 反复重启 | `crictl ps -a \| grep apiserver` RESTARTS 递增 | 0.90 | 需登录控制面节点 |
| S3 | `/readyz` 返回非 ok | `kubectl get --raw=/readyz?verbose` | 0.85 | 部分子检查失败不代表完全不可用 |
| S4 | 请求返回 429 Too Many Requests | apiserver 响应头 `Retry-After` | 0.85 | APF 限流，非崩溃 |
| S5 | 认证/准入失败，所有写被拒 | `kubectl apply` 报 admission webhook 错误 | 0.80 | Webhook 后端不可用导致 fail-closed |
| S6 | controller-manager/scheduler 无 leader | `kubectl get lease -n kube-system` renewTime 停滞 | 0.80 | 选主超时通常伴随 apiserver 慢 |
| S7 | apiserver 日志 OOM / etcd 超时 | `crictl logs`/`journalctl` | 0.85 | 需结合 etcd 侧确认 |

---

## 3. 快速分级

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 所有 apiserver 实例不可用，集群完全失控 | 立即 | 应急响应，双人复核，优先恢复单实例 |
| **P1** | 多实例中部分不可用/频繁重启，服务降级 | ≤15min | 摘除异常实例，定位根因 |
| **P2** | apiserver 可用但 APF 限流/延迟高 | ≤1h | 调整 APF/客户端限流，定位高频调用方 |
| **P3** | 单次偶发超时，已自愈 | ≤1d | 观察 apiserver 延迟指标 |

> HA 集群优先判定存活实例数：`≥1` 存活则集群仍可运维（P1）；`0` 存活为 P0。

---

## 4. 诊断工作流

### Phase 1: 快速定位（只读）

**D1.1**: 从客户端侧确认 apiserver 可达性

```bash
# 🟢 低风险：只读
kubectl get --raw='/readyz?verbose'
kubectl get --raw='/livez?verbose'
# 若完全不可达，登录控制面节点排查
```

**D1.2**: 控制面节点上确认 apiserver 进程/容器

```bash
# 🟢 低风险：只读（控制面节点执行）
crictl ps -a | grep -E "kube-apiserver|etcd"
crictl logs --tail=100 $(crictl ps -a --name kube-apiserver -q | head -1)
```

**D1.3**: 检查静态 Pod manifest 与资源

```bash
# 🟢 低风险：只读
cat /etc/kubernetes/manifests/kube-apiserver.yaml | grep -E "image:|--etcd-servers|--tls"
# 检查节点资源
free -h; df -h /var/lib/etcd
```

### Phase 2: 深度检查（只读）

**D2.1**: APF 限流分析（S4 分支）

```bash
# 🟢 低风险：只读
kubectl get --raw '/metrics' | grep -E "apiserver_flowcontrol_rejected_requests_total|apiserver_flowcontrol_current_inqueue_requests"
kubectl get flowschemas
kubectl get prioritylevelconfigurations
```

**D2.2**: 准入 Webhook 阻断分析（S5 分支）

```bash
# 🟢 低风险：只读
kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations
# failurePolicy=Fail 且后端不可用 → 写操作全阻断
```

**D2.3**: 选主状态（S6 分支）

```bash
# 🟢 低风险：只读
kubectl get lease -n kube-system kube-controller-manager kube-scheduler -o yaml | grep -E "holderIdentity|renewTime"
```

### Phase 3: 主动探测（需审批）

**D3.1**: 直接连 etcd 确认后端（需控制面权限）

```bash
# 🟡 中风险：需控制面权限，只读 etcd
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key endpoint status -w table
```

### 4.6 证据三元组

```promql
# 🟢 apiserver 可用性
apiserver_request_total{code=~"5.."} / apiserver_request_total > 0.1

# 🟢 APF 拒绝请求
rate(apiserver_flowcontrol_rejected_requests_total[5m]) > 0

# 🟢 apiserver 到 etcd 请求延迟（P99）
histogram_quantile(0.99, rate(etcd_request_duration_seconds_bucket[5m])) > 1
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | apiserver /metrics | 5xx 比例、APF 拒绝、etcd 延迟 |
| Logs | `crictl logs kube-apiserver` | `etcdserver: request timed out` / `OOMKilled` / TLS 错误 |
| Events | `kubectl get events -n kube-system` | apiserver Pod 重启事件 |

---

## 5. 根因分类

| RC-ID | 根因 | 概率 | 关键证据 | FTA | 修复 | 风险 |
|-------|------|------|---------|-----|------|------|
| RC-001 | etcd 慢/不可用拖垮 apiserver | 30% | 日志 `etcdserver: request timed out` | BE-C.1 | 转 02-etcd | 🔴 |
| RC-002 | apiserver OOM / 资源不足 | 18% | Exit 137，节点内存满 | BE-C.1 | 调 limit/扩控制面 | 🟡 |
| RC-003 | 证书过期 | 15% | x509 certificate expired | BE-C.4 | 转 03-cert | 🟡 |
| RC-004 | APF 限流（高频客户端） | 12% | flowcontrol_rejected 增长 | BE-C.1 | 调 APF/限流客户端 | 🟡 |
| RC-005 | 准入 Webhook fail-closed | 10% | admission webhook 报错 | BE-C.1 | 临时移除/修 Webhook | 🔴 |
| RC-006 | manifest/参数错误（如 etcd-servers） | 8% | apiserver 启动即崩 | BE-C.1 | 修正 manifest | 🟡 |
| RC-007 | 磁盘满/inode 耗尽 | 5% | df 100%，写失败 | BE-C.1 | 清理磁盘 | 🟡 |
| RC-008 | 网络/负载均衡异常（HA VIP） | 2% | 单实例可达，VIP 不可达 | BE-C.1 | 修复 LB/VIP | 🟡 |

---

## 6. 修复操作

**REM-005（🔴 高风险，需高级审批）：临时禁用阻断写操作的 Webhook**

```bash
# 🔴 高风险：移除准入 Webhook 会绕过安全策略，仅限恢复期间
kubectl delete validatingwebhookconfiguration <name>   # 恢复后必须重建
```

**REM-002（🟡 中风险）：提升 apiserver 资源限制**

```bash
# 🟡 中风险：编辑静态 Pod manifest（保存后 kubelet 自动重建）
# 编辑 /etc/kubernetes/manifests/kube-apiserver.yaml 的 resources.limits
```

**REM-006（🟡 中风险）：修正 manifest 后由 kubelet 重建**

```bash
# 🟡 中风险：修正参数后移出再移入 manifest 触发重建
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/  # 等待停止
# 修正后移回
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
```

> 🔴 涉及 etcd 的恢复（RC-001）必须转 [02-etcd-troubleshooting.md](02-etcd-troubleshooting.md)，切勿盲目重启 apiserver。

---

## 7. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `/readyz` | 返回 `ok` |
| 短期监控 | apiserver 5xx 比例 | 15min 内 < 1% |
| 解决标准 | 全组件 Lease 正常续租 | controller-manager/scheduler 有 leader |
| 回归检测 | APF 拒绝率 | 持续为 0 |

---

## 8. 升级协议

### 8.1 SLO 驱动升级

| 燃烧率 | 升级级别 |
|:---:|:---:|
| ≥ 14.4x（apiserver 可用性 SLO） | **P0** |
| ≥ 6x | **P1** |
| ≥ 3x | **P2** |

### 8.2 升级决策

- 涉及 etcd 数据/quorum → 立即升级 etcd 专家 + 转 02。
- 需移除安全 Webhook → 高级审批 + 变更记录 + 恢复后重建。
- 交接信息包：存活 apiserver 实例数、`/readyz` 输出、apiserver 日志关键行、etcd endpoint status、最近变更记录。

---

## 9. 版本兼容矩阵

> 基于 `code/apiserver-master`、`code/kube-controller-manager-master`、`code/kube-scheduler-master` 主干快照。

| 特性 | 1.28 | 1.30 | 1.34 | 1.36 | 诊断影响 |
|------|:----:|:----:|:----:|:----:|---------|
| API Priority & Fairness (APF) | ✅ GA(1.29 前 beta) | ✅ | ✅ | ✅ | 429 限流诊断查 flowcontrol 指标全版本通用 |
| Leader 选举 `coordination.k8s.io/Lease` | ✅ | ✅ | ✅ | ✅ | 选主诊断查 Lease renewTime 全版本通用 |
| `--enable-admission-plugins` 默认集 | 随版本增删 | — | — | — | 升级后默认准入插件变化可能改变准入行为 |

> [存疑：APF 精确 GA 版本（普遍认为 1.29 GA）依赖官方 Release Notes，本仓库主干快照无法直接证实各小版本状态]

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| apiserver 崩就重启 apiserver | 若根因是 etcd 慢，重启无效且掩盖问题，需先查 etcd |
| 429 当作 apiserver 故障 | APF 限流是保护机制，应定位高频调用方 |
| Webhook 报错以为是网络 | fail-closed Webhook 后端不可用会阻断所有写 |

### 10.2 生产案例

**案例: 准入 Webhook 后端下线导致全集群写阻断**

| 时间 | 事件 |
|------|------|
| T0 | 某 admission webhook 服务被误删 |
| T1 | 所有 `kubectl apply` 报 `failed calling webhook ... connection refused` |
| 根因 | webhook `failurePolicy: Fail` 且后端不可用（RC-005） |
| 修复 | 🔴 临时删除该 webhook 配置恢复写操作，修复后端后重建 |

### 10.3 混沌验证

| 注入场景 | 方法（测试集群） | 应命中 | 验证标准 |
|---------|----------------|-------|---------|
| apiserver OOM | 降低 apiserver memory limit + 高压查询 | RC-002 | Exit 137，`/readyz` 失败 |
| Webhook fail-closed | 部署指向失效后端的 webhook | RC-005 | 写操作全阻断 |

---

## 11. 云厂商特异性

| 厂商 | 差异 |
|------|------|
| 阿里云 ACK | 托管控制面用户不可直接登录，apiserver 故障走工单/事件中心；APF 与审计日志在控制台可见 |
| AWS EKS | 控制面完全托管，用户仅能查 CloudWatch 控制面日志 |
| 自建 kubeadm | 控制面为静态 Pod，可 SSH 直接操作 manifest |

---

## 12. 自动化集成接口

```json
{
  "skill_id": "SKILL-CLUSTER-001",
  "symptom": "apiserver_unavailable",
  "alive_instances": 0,
  "root_cause": "RC-001",
  "confidence": 0.9,
  "evidence": {"logs": "etcdserver: request timed out"},
  "action": "escalate_to_etcd",
  "requires_approval": true,
  "risk": "critical"
}
```

- 🟢 自动执行：所有 Phase 1/2 只读命令
- 🔴 禁止自动：重启 apiserver、删除 Webhook、任何 etcd 写操作

---

## 相关链接

- [[26-技能/01-集群运维/cluster/README.md|Cluster 集群级故障诊断技能集]]
- [[26-技能/01-集群运维/cluster/02-etcd-troubleshooting.md|etcd 故障诊断]]
- [[26-技能/01-集群运维/cluster/03-cluster-cert-upgrade.md|证书与升级]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[kube-apiserver]] — API Server
- [[etcd]] — 集群数据存储
