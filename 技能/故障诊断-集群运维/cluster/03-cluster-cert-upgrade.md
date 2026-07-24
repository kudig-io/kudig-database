---
title: 集群证书过期与版本升级故障诊断
description: 针对 kubeadm PKI 证书过期、kubelet 证书轮换、集群版本升级失败与回滚的完整诊断技能，含症状识别、快速分级、证据三元组、修复操作与升级前检查清单
summary: 证书过期与升级失败是可预防但高破坏力的集群级故障。本技能提供证书检查/轮换与升级失败回滚的生产级路径
category: skill
tags:
- k8s
- cluster
- certificate
- pki
- kubeadm
- upgrade
- version-skew
- rollback
- troubleshooting
- sop
sources:
- 故障诊断/FTA故障树/kubernetes-fta-full-analysis.md
- 故障诊断-集群运维/cluster-upgrade/
- 故障诊断-集群运维/kubeadm-fta.md
- code/kubeadm-main/
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
- 集群证书过期怎么处理
- kubelet 证书如何轮换
- 集群升级失败如何回滚
- x509 certificate expired 怎么办
- kubeadm 升级步骤
trigger_keywords:
- 证书过期
- certificate expired
- x509
- kubeadm certs
- 证书轮换
- 集群升级
- cluster upgrade
- version skew
- 升级回滚
- kubeadm upgrade
prerequisites:
- kubectl-basics
- cluster-architecture
- kubeadm-basics
skill_id: SKILL-CLUSTER-003
skill_name: 集群证书过期与版本升级故障诊断
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L1-manual-first
fta_path: TE-C -> IE-C.3/IE-C.4 -> BE-C.4/BE-C.5
---

> **生产环境安全提示**
>
> 命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险/只读。
>
> **升级与证书操作警告**：证书轮换与集群升级影响全集群。升级前必须备份 etcd 与 `/etc/kubernetes`；升级须逐节点、按小版本递进（禁止跳级）；证书轮换后需重启相关组件。本技能执行模式 **L1-manual-first**。

# 集群证书过期与版本升级故障诊断

> **Skill ID**: SKILL-CLUSTER-003
> **Agent 执行模式**: L1-manual-first
> **FTA 路径**: TE-C → IE-C.3（证书）/ IE-C.4（升级）→ BE-C.4 / BE-C.5

---

## 1. 概述

Kubernetes 组件间通过 mTLS 通信，证书由 kubeadm 管理（默认有效期 1 年）。证书过期会导致组件间认证失败、apiserver/kubelet 不可用。集群版本升级若跳级或忽略 API 废弃、版本 skew 约束，会导致组件 CrashLoop 或功能异常。

**覆盖范围**：kubeadm PKI 证书检查/轮换、kubelet 客户端证书轮换、集群小版本升级失败与回滚、版本 skew 违规、升级后 API 废弃导致的工作负载异常。

**边界**：apiserver 崩溃 → [01-apiserver-controlplane.md](01-apiserver-controlplane.md)；etcd 证书 → 结合 [02-etcd-troubleshooting.md](02-etcd-troubleshooting.md)。

---

## 2. 症状识别

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 组件报 `x509: certificate has expired` | apiserver/kubelet/controller 日志 | 0.95 | 客户端本地时钟错误也会报 x509 |
| S2 | `kubeadm certs check-expiration` 显示已过期 | kubeadm 命令 | 0.95 | 直接确认过期时间 |
| S3 | 节点 NotReady 且 kubelet 证书过期 | kubelet 日志 TLS 错误 | 0.85 | 与网络分区区分 |
| S4 | 升级后控制面组件 CrashLoop | `crictl logs` 版本/API 错误 | 0.85 | 需确认升级动作时间点 |
| S5 | 升级后工作负载异常（API 废弃） | apiserver 日志 `no matches for kind` | 0.80 | 废弃 API 在新版本移除 |
| S6 | 版本 skew 违规告警 | 组件版本差 > 允许范围 | 0.80 | kubelet 落后 apiserver > 3 版本 |

---

## 3. 快速分级

| 级别 | 判定条件 | 响应时限 | 处置 |
|:---:|---------|:---:|------|
| **P0** | 控制面证书全过期/升级致控制面不可用 | 立即 | 应急轮换证书/回滚升级，双人复核 |
| **P1** | 部分节点证书过期致 NotReady；升级卡在中途 | ≤15min | 逐节点轮换/继续或回滚升级 |
| **P2** | 证书临近过期（<30 天）；升级后个别工作负载异常 | ≤1d | 计划轮换/适配废弃 API |
| **P3** | 证书距过期 >30 天，例行巡检发现 | 排期 | 纳入自动轮换 |

---

## 4. 诊断工作流

### Phase 1: 快速定位（只读）

**D1.1**: 证书到期检查（kubeadm 集群）

```bash
# 🟢 低风险：只读（控制面节点执行）
kubeadm certs check-expiration
```

**D1.2**: 手动核对关键证书到期时间

```bash
# 🟢 低风险：只读
for c in apiserver apiserver-kubelet-client front-proxy-client; do
  echo "== $c =="
  openssl x509 -in /etc/kubernetes/pki/$c.crt -noout -enddate
done
```

**D1.3**: 升级场景——确认组件版本 skew

```bash
# 🟢 低风险：只读
kubectl get nodes -o wide   # KUBELET-VERSION 列
kubectl version --short 2>/dev/null || kubectl version
```

### Phase 2: 深度检查（只读）

**D2.1**: 升级后 API 废弃检查（S5 分支）

```bash
# 🟢 低风险：只读
kubectl get --raw='/metrics' | grep apiserver_requested_deprecated_apis
# 或使用 kubent / pluto 扫描废弃 API 引用
```

**D2.2**: kubelet 证书轮换状态

```bash
# 🟢 低风险：只读
ls -l /var/lib/kubelet/pki/
# 确认 kubelet-client-current.pem 指向的证书有效期
```

### 4.6 证据三元组

```promql
# 🟢 证书到期剩余秒数（< 7 天需告警）
apiserver_client_certificate_expiration_seconds_count > 0

# 🟢 使用废弃 API 的请求（升级前必查）
apiserver_requested_deprecated_apis > 0
```

| 维度 | 来源 | 取值 |
|------|------|------|
| Metrics | apiserver /metrics | 证书到期直方图、废弃 API 计数 |
| Logs | 组件日志 | `x509: certificate has expired` / `no matches for kind` |
| Events | `kubectl get events` | 节点 NotReady、kubelet 认证失败 |

---

## 5. 根因分类

| RC-ID | 根因 | 概率 | 关键证据 | FTA | 修复 | 风险 |
|-------|------|------|---------|-----|------|------|
| RC-001 | 控制面证书过期 | 30% | check-expiration 显示 expired | BE-C.4 | kubeadm certs renew all | 🟡 |
| RC-002 | kubelet 证书未自动轮换 | 20% | kubelet TLS 错误 | BE-C.4 | 重启 kubelet 触发轮换 | 🟡 |
| RC-003 | 升级跳级（违反逐版本递进） | 18% | 版本差 >1 小版本 | BE-C.5 | 回滚，按序升级 | 🔴 |
| RC-004 | 升级后 API 废弃/移除 | 15% | `no matches for kind` | BE-C.5 | 迁移清单到新 API 版本 | 🟡 |
| RC-005 | 版本 skew 违规（kubelet 过旧） | 10% | kubelet 落后 >3 版本 | BE-C.5 | 先升 kubelet | 🟡 |
| RC-006 | 升级中途组件配置不兼容 | 5% | 组件启动参数报错 | BE-C.5 | 修正配置/回滚 | 🟡 |
| RC-007 | CA 证书过期（10 年，罕见） | 2% | CA enddate 过期 | BE-C.4 | 重签 CA（重大操作） | 🔴 |

---

## 6. 修复操作

**REM-001（🟡 中风险）：轮换 kubeadm 全部证书**

```bash
# 🟡 中风险：轮换后需重启控制面静态 Pod，逐控制面节点执行
kubeadm certs renew all
# 重启控制面组件（移出再移入 manifest 或重启 kubelet）
systemctl restart kubelet
# 更新 admin.conf（若使用）
cp /etc/kubernetes/admin.conf ~/.kube/config
```

**REM-002（🟡 中风险）：触发 kubelet 证书轮换**

```bash
# 🟡 中风险：确认 kubelet 开启 rotateCertificates: true
systemctl restart kubelet
# 若 CSR 未自动批准，检查 kube-controller-manager 的 --cluster-signing-cert-file
kubectl get csr
```

**REM-003（🔴 高风险，需高级审批）：升级失败回滚**

```bash
# 🔴 高风险：回滚控制面到升级前版本（前置：已备份 etcd 与 /etc/kubernetes）
# 1. 恢复 etcd 快照（若数据已被新版本写入不兼容格式）→ 转 02
# 2. 降级 kubeadm/kubelet/kubectl 二进制到原版本
# 3. kubeadm upgrade apply <原版本> 或恢复原 manifest
```

**升级前检查清单（🟢 预防）**

```bash
# 🟢 低风险：只读
# 1. 备份 etcd + /etc/kubernetes
# 2. 扫描废弃 API：pluto detect-all-in-cluster
# 3. 确认逐版本递进（不跳小版本）
# 4. 确认 kubelet 版本 skew 合规
kubeadm upgrade plan
```

---

## 7. 验证确认

| 阶段 | 判据 | 通过标准 |
|------|------|---------|
| 即时验证 | `kubeadm certs check-expiration` | 全部证书有效期 > 300 天 |
| 短期监控 | 组件健康 | apiserver `/readyz` ok，节点 Ready |
| 解决标准 | 升级后全组件版本一致且运行 | `kubectl get nodes` 版本符合预期 |
| 回归检测 | 废弃 API 计数 | `apiserver_requested_deprecated_apis` 为 0 |

---

## 8. 升级协议

- 控制面证书全过期 / 升级致控制面不可用 → P0，立即升级平台专家。
- 需回滚升级或重签 CA → 高级审批 + 变更记录 + 备份确认。
- 交接信息包：`check-expiration` 输出、组件版本表、`kubeadm upgrade plan` 输出、废弃 API 扫描结果、备份路径。

---

## 9. 版本兼容矩阵

> 基于 `code/kubeadm-main` 快照。

| 项 | 约束 | 说明 |
|----|------|------|
| kubeadm 升级路径 | 逐小版本递进 | 1.28→1.29→1.30，禁止 1.28→1.30 跳级 |
| kubelet 版本 skew | 落后 apiserver ≤ 3 小版本（1.28+） | 早期为 2 版本，1.28 起放宽到 3 |
| kubectl 版本 skew | 与 apiserver 相差 ≤ 1 小版本 | 过大差异可能命令不兼容 |
| 证书默认有效期 | 1 年（叶子证书） / 10 年（CA） | kubeadm 升级会自动续期叶子证书 |

> [存疑：kubelet skew 从 2 放宽到 3 版本的精确起始版本以官方版本 skew 策略文档为准；本仓库 kubeadm-main 为主干快照]

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊 | 纠正 |
|------|------|
| x509 报错一律判过期 | 需排除客户端时钟不同步（NTP） |
| 升级卡住就回滚 | 先看是否为 API 废弃/配置问题，多数可继续修复而非回滚 |
| 只轮换控制面证书 | kubelet 证书需单独确认自动轮换是否生效 |

### 10.2 生产案例

**案例: 集群升级跳级导致控制面 CrashLoop**

| 时间 | 事件 |
|------|------|
| T0 | 运维直接从 1.28 升级到 1.30（跳过 1.29） |
| T1 | kube-controller-manager CrashLoop，存储版本不兼容 |
| 根因 | 违反逐小版本递进约束（RC-003） |
| 修复 | 🔴 回滚到 1.28，改为 1.28→1.29→1.30 逐级升级 |

### 10.3 混沌验证

| 注入场景 | 方法（测试集群） | 应命中 | 验证标准 |
|---------|----------------|-------|---------|
| 证书过期 | 用短有效期证书签发并等待过期 | RC-001 | check-expiration 显示 expired |
| 废弃 API | 部署使用旧 API 版本的清单后升级 | RC-004 | `no matches for kind` |

---

## 11. 云厂商特异性

| 厂商 | 差异 |
|------|------|
| 阿里云 ACK | 托管控制面证书由平台自动轮换；升级走控制台一键升级并含前置检查 |
| AWS EKS | 控制面升级托管；节点组升级需用户触发，注意 kubelet skew |
| 自建 kubeadm | 证书轮换与升级完全自管，需自建备份与巡检 |

---

## 12. 自动化集成接口

```json
{
  "skill_id": "SKILL-CLUSTER-003",
  "symptom": "certificate_expired",
  "expired_certs": ["apiserver", "apiserver-kubelet-client"],
  "root_cause": "RC-001",
  "action": "renew_certs",
  "requires_approval": true,
  "risk": "medium"
}
```

- 🟢 自动执行：证书到期检查、版本 skew 检查、废弃 API 扫描
- 🔴 禁止自动：证书轮换、升级 apply、回滚、重签 CA

---

## 相关链接

- [[技能/故障诊断-集群运维/cluster/README.md|Cluster 集群级故障诊断技能集]]
- [[技能/故障诊断-集群运维/cluster/01-apiserver-controlplane.md|控制平面不可用诊断]]
- [[技能/故障诊断-集群运维/cluster/02-etcd-troubleshooting.md|etcd 故障诊断]]
- [[技能/故障诊断-集群运维/kubeadm-fta.md|kubeadm 故障树分析]]
- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[kubeadm]] — 集群引导工具
- [[kube-apiserver]] — API Server
