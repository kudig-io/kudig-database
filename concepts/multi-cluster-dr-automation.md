---
title: 多集群灾备与自动化
category: concepts
tags:
  - disaster-recovery
  - multi-cluster
  - automation
  - k8s
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# 多集群灾备与自动化

## 概述

随着 Kubernetes 部署从单集群走向多集群、多区域架构，灾备（Disaster Recovery, DR）从传统的"备份-恢复"演进为涵盖应用编排、数据同步、流量切换、自动验证的端到端自动化体系。本文系统梳理跨区域 DR 策略、数据库级灾备、多集群故障切换自动化及 DR 成熟度模型。

---

## 跨区域 DR 四模式

```
RTO ↑
  │
  │  ┌─────────────────────────────────────────────────┐
  │  │          Active-Active / Multi-Active            │  RTO ≈ 0, RPO ≈ 0
  │  ├─────────────────────────────────────────────────┤
  │  │          Warm Standby                           │  RTO: 分钟级, RPO: 秒级
  │  ├─────────────────────────────────────────────────┤
  │  │          Pilot Light                            │  RTO: 10-30 分钟, RPO: 分钟级
  │  ├─────────────────────────────────────────────────┤
  │  │          Backup & Restore                       │  RTO: 小时级, RPO: 小时级
  │  └─────────────────────────────────────────────────┘
  │                                                    → 成本
```

### 模式详解

| 模式 | 架构特征 | RTO | RPO | 成本 | 适用场景 |
|------|----------|-----|-----|------|----------|
| **Backup & Restore** | 定期快照 + 按需恢复 | 小时级 | 小时级 | 最低 | 非关键系统、合规备份 |
| **Pilot Light** | 核心组件保持运行，按需扩容 | 10-30 分钟 | 分钟级 | 较低 | 一般业务系统 |
| **Warm Standby** | 缩减版热备集群持续运行 | 分钟级 | 秒级 | 中等 | 重要业务系统 |
| **Active-Active** | 多区域同时服务流量 | ≈ 0 | ≈ 0 | 最高 | 关键业务、金融交易 |

### 备份工具生态

| 工具 | 特点 |
|------|------|
| Velero | K8s 原生备份，支持 PV 快照、资源对象导出，多云兼容 |
| Kasten K10 | Veeam 旗下，企业级数据保护，应用感知备份 |
| TrilioVault | 多集群统一备份管理，支持增量快照 |
| Backube | 开源 Operator，支持 CSI VolumeSnapshot + rsync |

---

## 数据库级灾备

数据库是 DR 的核心难点，需要在一致性、可用性和延迟之间权衡。

### CloudNativePG 流复制

[[cloudnativepg|CloudNativePG]] 是 CNCF 孵化项目，原生支持跨集群流复制：

```yaml
# DR 集群配置 — 接收主集群 WAL 流
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: app-db-dr
spec:
  instances: 2
  bootstrap:
    pg_basebackup:
      source: app-db-primary  # 引用主集群
  replica:
    enabled: true
    source: app-db-primary
  primaryUpdateStrategy: unsupervised
```

**CloudNativePG DR 架构：**
```
主集群 (us-east-1)                    DR 集群 (us-west-2)
┌─────────────────┐                   ┌─────────────────┐
│  Primary PG     │ ──── WAL 流 ───→ │  Replica PG     │
│  + 2 Replicas   │     (异步/同步)   │  Promote → 新主  │
└─────────────────┘                   └─────────────────┘
        │                                      │
   App 写入                               App 只读 / 待切换
```

### Vitess 跨区域分片

[[vitess|Vitess]] 原生支持跨区域部署：

- **Primary Tablet** 在一个区域，**Replica Tablet** 分布在多个区域
- 利用 VReplication 实现跨 shard/跨集群数据同步
- VTGate 智能路由，读请求可就近发送到本地 Replica
- DR 切换：提升 Replica 为 Primary，重新路由流量

### Redis Enterprise CRDT

Redis Enterprise 使用 CRDT（Conflict-free Replicated Data Types）实现多主复制：

- **Active-Active**：多个区域同时接受写入，CRDT 自动解决冲突
- **支持的数据结构**：String（Last-Writer-Wins）、Set/Hash/Sorted Set（并集合并）
- **RPO ≈ 0**：异步复制 + CRDT 保证最终一致
- **适用场景**：会话缓存、购物车、排行榜等可容忍短暂不一致的场景

---

## 多集群故障切换自动化

### ArgoCD ApplicationSets

[[argocd|ArgoCD]] 的 ApplicationSet 实现多集群应用编排：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-cluster-app
spec:
  generators:
    - clusters:
        selector:
          matchLabels:
            env: production
  template:
    metadata:
      name: '{{name}}-app'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/app-config
        targetRevision: main
        kustomize:
          namePrefix: '{{metadata.labels.region}}-'
      destination:
        server: '{{server}}'
        namespace: app
      syncPolicy:
        automated:
          prune: true
        syncOptions:
          - CreateNamespace=true
```

**DR 切换流程：**
1. 检测主集群不可用（健康检查 / 外部监控）
2. 更新 ApplicationSet generator 中的集群标签，排除故障集群
3. ArgoCD 自动同步应用到 DR 集群
4. 更新 DNS/GSLB 指向 DR 集群

### Submariner 跨集群网络

[[submariner|Submariner]] 打通多集群网络：

- **跨集群 Pod 互通**：基于 IPsec/WireGuard 的加密隧道
- **Service Discovery**：通过 Lighthouse 实现跨集群 Service 发现
- **GlobalNet**：解决跨集群 Pod CIDR 冲突
- **DR 场景**：应用无需感知多集群拓扑，通过统一 Service 访问

### Cluster API (CAPI)

Cluster API 实现集群生命周期自动化：

```yaml
# 灾难场景：快速重建 DR 集群
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: dr-cluster
  labels:
    region: us-west-2
    role: disaster-recovery
spec:
  topology:
    class: aws-quickstart
    version: v1.31.0
    workers:
      machineDeployments:
        - class: default-machine-class
          name: md-0
          replicas: 3
          metadata:
            labels:
              node-role: application
```

**CAPI + ArgoCD DR 自动化闭环：**
```
故障检测 → CAPI 创建/扩容 DR 集群 → ArgoCD 部署应用 →
数据同步就绪验证 → 流量切换 → 业务恢复确认
```

---

## 备份验证自动化

备份不验证等于没有备份。自动化验证是 DR 可靠性的关键保障。

### Tekton 验证流水线

```yaml
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: backup-verification
spec:
  params:
    - name: backup-id
    - name: verification-cluster
  tasks:
    - name: restore-backup
      taskRef:
        name: velero-restore
      params:
        - name: backup-name
          value: $(params.backup-id)
        - name: target-cluster
          value: $(params.verification-cluster)
    - name: health-check
      taskRef:
        name: app-health-check
      runAfter: [restore-backup]
    - name: data-integrity
      taskRef:
        name: db-integrity-check
      runAfter: [restore-backup]
    - name: generate-report
      taskRef:
        name: compliance-report
      runAfter: [health-check, data-integrity]
```

### Argo Workflows 验证

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: dr-drill
spec:
  entrypoint: dr-verification
  templates:
    - name: dr-verification
      dag:
        tasks:
          - name: restore
            template: velero-restore
          - name: app-check
            template: smoke-test
            dependencies: [restore]
          - name: data-check
            template: data-validation
            dependencies: [restore]
          - name: report
            template: compliance-report
            dependencies: [app-check, data-check]
```

### 合规报告要素

| 报告项 | 说明 |
|--------|------|
| 备份完整性 | 各资源类型恢复成功率 |
| RTO 验证 | 实际恢复时间 vs 目标 RTO |
| RPO 验证 | 恢复数据的时间点 vs 事件发生时间 |
| 应用健康 | 恢复后 smoke test 结果 |
| 数据一致性 | 数据库校验和、行数比对 |
| 签名与时间戳 | 审计合规，不可篡改 |

---

## DR 成熟度模型

```
┌─────────────────────────────────────────────────────────────────┐
│  L4: 自主/AI 驱动                                               │
│  • AI 预测故障，自动触发预防性切换                               │
│  • 自愈能力：故障修复后自动回切                                  │
│  • 持续混沌工程验证                                              │
├─────────────────────────────────────────────────────────────────┤
│  L3: 全自动切换                                                  │
│  • 故障检测 → 流量切换 → 业务恢复全自动                          │
│  • 定期自动 DR 演练                                              │
│  • 备份验证自动化 + 合规报告                                     │
├─────────────────────────────────────────────────────────────────┤
│  L2: 半自动切换                                                  │
│  • 自动故障检测 + 告警                                           │
│  • 一键切换（人工确认触发）                                      │
│  • 自动化备份 + 手动验证                                         │
├─────────────────────────────────────────────────────────────────┤
│  L1: 手动切换 + 自动化备份                                       │
│  • 自动化备份策略                                                │
│  • 手动故障检测 + 手动切换                                       │
│  • 有 DR 文档但未定期演练                                        │
├─────────────────────────────────────────────────────────────────┤
│  L0: 完全手动                                                    │
│  • 手动备份（或无备份）                                          │
│  • 无 DR 预案                                                    │
│  • 故障时临时恢复                                                │
└─────────────────────────────────────────────────────────────────┘
```

### 各级别关键指标

| 级别 | RTO 目标 | RPO 目标 | 演练频率 | 自动化率 |
|------|----------|----------|----------|----------|
| L0 | 无保证 | 无保证 | 无 | 0% |
| L1 | 小时级 | 小时级 | 年度 | < 30% |
| L2 | 10-30 分钟 | 分钟级 | 季度 | 30-60% |
| L3 | 分钟级 | 秒级 | 月度 | 60-90% |
| L4 | 秒级 | ≈ 0 | 持续（混沌工程）| > 90% |

### 升级路径建议

**L0 → L1：** 先用 Velero 建立基础备份策略，编写 DR 文档

**L1 → L2：** 引入 ArgoCD ApplicationSets 实现多集群应用管理，建立健康检查告警

**L2 → L3：** 实现自动故障检测 + 一键切换脚本，定期 Tekton 流水线验证备份

**L3 → L4：** 引入 Chaos Engineering（Litmus Chaos / Chaos Mesh），AI 异常检测，自愈闭环

---

## 参考架构

```
┌──────────────────────────────────────────────────────────────────┐
│                      流量层                                       │
│  GSLB / Global Accelerator / Multi-cluster Gateway (Istio)       │
├──────────────────────────────────────────────────────────────────┤
│                      编排层                                       │
│  ArgoCD ApplicationSets │ Cluster API │ Submariner               │
├──────────────────────────────────────────────────────────────────┤
│                      应用层                                       │
│  Primary Cluster (us-east-1)  ←→  DR Cluster (us-west-2)        │
│  ┌──────────────┐                 ┌──────────────┐               │
│  │  App Pods    │                 │  App Pods    │               │
│  │  DB Primary  │ ── 同步/异步 ──→│  DB Replica  │               │
│  │  Redis       │    复制         │  Redis CRDT  │               │
│  └──────────────┘                 └──────────────┘               │
├──────────────────────────────────────────────────────────────────┤
│                      数据保护层                                    │
│  Velero │ CloudNativePG Streaming Replication │ Redis CRDT       │
├──────────────────────────────────────────────────────────────────┤
│                      验证层                                       │
│  Tekton/Argo Workflows → 验证 → 合规报告 → 仪表盘               │
└──────────────────────────────────────────────────────────────────┘
```

---

## 相关概念

- [[capacity-planning-cost-optimization|容量规划与成本优化]]
- 多集群管理
- 服务网格
- GitOps
- 平台工程
- 混沌工程

## Related

- [[concepts/storage-data-protection.md|storage data protection]] — 存储数据保护与灾备
- [[concepts/chaos-engineering-platforms.md|chaos engineering platforms]] — 混沌工程平台
- [[concepts/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
