---
title: "备份 × 多云 × 灾难恢复策略"
summary: "Velero 跨云备份、多云 DR 架构设计与 RTO/RPO 目标达成，恢复演练自动化将灾难恢复从'纸面计划'转为'可验证能力'"
category: synthesis
tags:
- backup
- multi-cloud
- disaster-recovery
- velero
- rto
- rpo
- recovery-drill
tier: supporting
sources:
- 概念/velero-disaster-recovery.md
- 概念/data-protection-k8s.md
- 概念/multi-cluster-dr-automation.md
- 概念/cross-cloud-migration-playbook.md
- 实体/rook.md
- 概念/high-availability-patterns.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 备份 × 多云 × 灾难恢复策略

## The Connection（为什么这两个领域交叉）

灾难恢复（DR）的终极目标是：在任何单点故障（节点、可用区、区域、云厂商）后，以可接受的 RTO（恢复时间目标）和 RPO（恢复点目标）恢复业务。单一云厂商的区域级故障虽罕见但确实发生（如 AWS us-east-1 2021 年 12 月宕机 7 小时），多云 DR 是应对"云厂商级故障"的最终保障。

备份是 DR 的基础——没有备份就没有恢复。Velero 作为 K8s 原生备份工具，将集群资源（YAML）和持久卷数据（CSI 快照/文件级备份）打包为可移植的制品，存储在对象存储中。跨云备份意味着备份制品存储在不同于生产环境的云厂商（如生产在 AWS，备份在阿里云 OSS），确保单一云厂商故障不丢失备份。

三者的交叉形成完整的 DR 体系：备份（数据保护）→ 多云（基础设施冗余）→ 恢复（业务连续性）。备份解决"数据不丢"，多云解决"基础设施可用"，恢复策略解决"业务多快能回来"。缺少任何一环，DR 都是不完整的。

## Where They Co-occur（生产中的交叉场景）

### 场景一：跨云备份存储

生产集群在 AWS EKS，Velero 备份制品存储在阿里云 OSS（或 Azure Blob）。AWS 区域级故障时，备份制品不受影响，可在阿里云 ACK 上恢复。需要 Velero 的 Backup Storage Location (BSL) 配置跨云存储后端，以及对应的 VolumeSnapshotter 插件。

### 场景二：Active-Passive 多云 DR

主集群在 AWS（Active），灾备集群在阿里云（Passive，最小规格运行）。正常时灾备集群只运行核心服务的最小副本。主集群故障时：(1) DNS 切换到灾备集群；(2) Velero Restore 恢复最新备份；(3) 扩容灾备集群到生产规格。RTO 目标：30 分钟内完成切换。

### 场景三：Active-Active 多区域

两个区域（如 us-east + eu-west）同时运行，各承担 50% 流量。单区域故障时，另一区域承接 100% 流量。数据层通过跨区复制（数据库主从、对象存储跨区同步）保持一致。K8s 层面通过 Karmada 或 GitOps 确保两区域部署一致。

### 场景四：恢复演练自动化

每季度 DR 演练不再是"大项目"，而是自动化流水线：创建隔离环境 → 从备份恢复 → 运行 Smoke Test → 验证 RTO/RPO → 生成报告 → 清理环境。Argo Workflows 编排整个演练流程，结果自动记录到合规系统。

### 场景五：数据库级 DR

K8s 资源（Deployment/Service）恢复快（秒级），但数据库恢复是 DR 的瓶颈。CloudNativePG 的流复制 + WAL 归档实现 RPO < 1 分钟；跨云场景下 WAL 归档到对象存储（跨区），灾备集群从 WAL 重放到最新状态。

### 场景六：合规驱动的备份保留

金融/医疗行业要求备份保留 7 年且不可篡改。Velero 备份制品存储在启用 WORM（Write Once Read Many）的对象存储桶中，配合生命周期策略自动管理保留期。审计时可直接从归档中恢复特定时间点的备份验证完整性。

## Production Patterns（生产模式与架构）

### 模式一：多云 DR 架构

```
┌─────────────────────────────────────────────────────────┐
│  Multi-Cloud DR Architecture                             │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Primary Cloud (AWS us-east-1)                          │
│  ├── EKS Cluster (生产)                                │
│  │   ├── 应用工作负载 (全量)                           │
│  │   ├── 数据库 (CloudNativePG 主)                    │
│  │   └── Velero (备份调度)                             │
│  ├── S3 (应用数据)                                     │
│  └── Route53 (DNS)                                     │
│                                                         │
│  备份制品存储 (跨云)                                    │
│  ├── 阿里云 OSS (Velero 备份制品)                     │
│  │   ├── K8s 资源 YAML                                │
│  │   ├── PV 数据 (CSI 快照 / Kopia)                  │
│  │   └── 保留策略: 日备 30 天, 周备 12 周, 月备 12 月 │
│  └── Azure Blob (数据库 WAL 归档)                     │
│                                                         │
│  DR Cloud (阿里云 cn-hangzhou)                         │
│  ├── ACK Cluster (灾备)                                │
│  │   ├── 核心服务 (最小副本, 热备)                    │
│  │   ├── 数据库 (CloudNativePG 从, 流复制)           │
│  │   └── Velero (恢复执行)                             │
│  ├── OSS (灾备数据)                                    │
│  └── DNS (备用, 健康检查失败时切换)                    │
│                                                         │
│  故障转移流程:                                          │
│  1. 检测: 健康检查失败 > 5min (自动) / 人工决策       │
│  2. DNS: Route53 权重切到阿里云 (TTL 60s)             │
│  3. 恢复: Velero Restore 最新备份到 ACK               │
│  4. 扩容: ACK 节点池扩到生产规格                      │
│  5. 验证: Smoke Test + 数据一致性检查                  │
│  6. 通知: 利益相关者通知 + 事件报告                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：Velero 跨云备份配置

```yaml
# 主集群 Velero 配置: 备份到阿里云 OSS
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: cross-cloud-oss
  namespace: velero
spec:
  provider: alibabacloud
  objectStorage:
    bucket: k8s-backup-prod
    prefix: aws-cluster
  config:
    region: cn-hangzhou
    # 凭证通过 Secret 挂载
---
# 定期备份 Schedule
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: daily-full-backup
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨 2 点
  template:
    includedNamespaces:
    - production
    - staging
    excludedNamespaces:
    - kube-system
    - velero
    storageLocation: cross-cloud-oss
    volumeSnapshotLocations:
    - default
    ttl: 720h  # 30 天保留
    hooks:
      resources:
      - name: database-hook
        includedNamespaces:
        - production
        labelSelector:
          matchLabels:
            app: postgres
        pre:
        - exec:
            container: postgres
            command:
            - /bin/sh
            - -c
            - "pg_basebackup -D /tmp/backup -Ft -z -P"
          timeout: 300s
---
# 灾备集群恢复命令
# velero restore create --from-backup daily-full-backup-20260719
#   --namespace-mappings production=production-dr
#   --restore-volumes=true
#   --storage-location cross-cloud-oss
```

### 模式三：RTO/RPO 分层策略

```
业务分级与 DR 目标:

  Tier 1 (核心交易): RTO < 5min, RPO < 1min
  ├── 策略: Active-Active 双区域
  ├── 数据: 同步复制 (数据库流复制)
  ├── 切换: 自动 (DNS + 健康检查)
  └── 验证: 每分钟健康检查

  Tier 2 (重要服务): RTO < 30min, RPO < 15min
  ├── 策略: Active-Passive 热备
  ├── 数据: 异步复制 + Velero 15min 备份
  ├── 切换: 半自动 (人工确认 + 自动执行)
  └── 验证: 每 5 分钟健康检查

  Tier 3 (一般服务): RTO < 4h, RPO < 1h
  ├── 策略: 备份恢复 (Cold Standby)
  ├── 数据: Velero 每小时备份
  ├── 切换: 手动 (运维团队执行)
  └── 验证: 每 30 分钟健康检查

  Tier 4 (非关键): RTO < 24h, RPO < 24h
  ├── 策略: 每日备份 + 按需恢复
  ├── 数据: Velero 每日备份
  ├── 切换: 手动
  └── 验证: 每日备份成功确认
```

### 模式四：恢复演练自动化

```yaml
# Argo Workflow: 自动化 DR 演练
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: dr-drill-quarterly
spec:
  entrypoint: dr-drill
  templates:
  - name: dr-drill
    steps:
    - - name: create-isolated-env
        template: create-namespace  # 创建隔离演练环境
    - - name: restore-from-backup
        template: velero-restore    # 从最新备份恢复
    - - name: wait-for-ready
        template: wait-ready        # 等待所有 Pod Ready
    - - name: smoke-test
        template: run-smoke-test    # 运行 Smoke Test
    - - name: measure-rto
        template: calculate-rto     # 计算实际 RTO
    - - name: verify-data
        template: data-consistency  # 数据一致性验证
    - - name: generate-report
        template: drill-report      # 生成演练报告
    - - name: cleanup
        template: cleanup-env       # 清理演练环境
---
# 演练频率:
# Tier 1: 每月演练
# Tier 2: 每季度演练
# Tier 3: 每半年演练
# Tier 4: 每年演练
```

### 模式五：数据库 DR（CloudNativePG）

```yaml
# 主集群: CloudNativePG 主实例 + WAL 归档到跨云存储
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: prod-db
  namespace: production
spec:
  instances: 3
  storage:
    size: 500Gi
    storageClass: gp3-encrypted
  backup:
    barmanObjectStore:
      destinationPath: "s3://db-backup-prod/wal"
      # 跨云: 同时归档到阿里云 OSS
    retentionPolicy: "30d"
  # 流复制到灾备集群 (异步)
  replica:
    enabled: true
    source: prod-db-primary
---
# 灾备集群: 从 WAL 重放恢复
# 故障时: 提升灾备实例为主 (switchover)
# kubectl cnpg promote prod-db-replica -n production-dr
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | 单云多 AZ | 单云多区域 | 多云 Active-Passive | 多云 Active-Active |
|------|----------|----------|-------------------|-------------------|
| RTO | < 5min | < 15min | < 30min | < 1min |
| RPO | < 1min | < 5min | < 15min | < 1min |
| 成本 | 1x | 1.5-2x | 1.3-1.5x | 2-3x |
| 复杂度 | 低 | 中 | 高 | 极高 |
| 数据一致性 | 强 | 强/最终 | 最终 | 最终 |
| 故障域 | AZ 级 | 区域级 | 云厂商级 | 云厂商级 |
| 适用场景 | 大多数 | 关键业务 | 合规要求 | 金融/电商核心 |

### 备份方案对比

| 维度 | Velero (CSI 快照) | Velero (Kopia 文件级) | 数据库原生备份 | 云快照 |
|------|-----------------|---------------------|-------------|--------|
| 速度 | 快（块级） | 慢（文件级） | 取决于数据量 | 快 |
| 跨云 | 支持（对象存储） | 支持 | 支持（WAL 归档） | 不支持 |
| 一致性 | Crash-consistent | Crash-consistent | App-consistent | Crash-consistent |
| 增量 | 支持（CSI） | 支持（去重） | 支持（WAL） | 支持 |
| 恢复粒度 | PV 级 | 文件级 | 时间点 (PITR) | PV 级 |
| 适用 | 通用 | 无 CSI 快照支持 | 数据库 | 云原生 |

## Anti-patterns & Pitfalls（反模式）

### 反模式一：备份成功 = DR 就绪

Velero 备份显示"Completed"就认为 DR 没问题。实际上备份制品可能损坏、CRD 版本不兼容、目标集群缺少依赖。**正确做法**：定期恢复演练（至少每季度）；备份后自动验证（`velero backup describe --details`）；恢复演练自动化。

### 反模式二：备份存储与生产同区域

备份存在与生产相同的 S3 区域。区域级故障时生产和备份同时不可用。**正确做法**：备份制品存储在跨区域或跨云的对象存储；至少跨可用区。

### 反模式三：RTO 目标未经验证

纸面上写"RTO < 30 分钟"，但从未实际演练。真正故障时发现：DNS TTL 太长（1 小时）、备份恢复需要 2 小时、节点扩容需要 30 分钟——实际 RTO 远超目标。**正确做法**：定期演练验证 RTO；识别恢复链路中的瓶颈；预先扩容灾备集群。

### 反模式四：忽略有状态服务的 DR

无状态服务（Deployment）恢复简单，但有状态服务（数据库、消息队列、缓存）是 DR 的难点。只备份 K8s YAML 不备份 PV 数据 = 恢复了"空壳"。**正确做法**：有状态服务使用应用级备份（pg_basebackup、mysqldump）+ Velero PV 备份双重保障。

### 反模式五：恢复后不验证数据一致性

恢复完成（Pod Running）就宣布"DR 成功"。实际上数据库可能缺少最后几分钟的事务，缓存与数据库不一致。**正确做法**：恢复后运行数据一致性检查（如订单数对比、余额校验）；应用级 Smoke Test（关键业务流程验证）。

### 反模式六：DR 切换后无法回切

故障恢复后，流量切到灾备集群。但回切（Failback）流程未定义——如何把灾备期间产生的数据同步回主集群？**正确做法**：预先设计 Failback 流程；灾备期间的数据变更需要记录和同步；回切前验证数据一致性。

## Operational Checklist（运维检查清单）

### 备份配置

- [ ] Velero 部署（≥ 2 副本 + PDB）
- [ ] BackupStorageLocation 配置（跨云/跨区域）
- [ ] Schedule 配置（频率匹配 RPO 目标）
- [ ] 备份 Hooks（数据库冻结/刷盘）
- [ ] 保留策略（日/周/月/年分层）
- [ ] 备份加密（对象存储 SSE）
- [ ] 备份验证（自动完整性检查）

### DR 架构

- [ ] 确定业务分级（Tier 1-4）和对应 RTO/RPO
- [ ] 灾备集群预配置（网络、存储、RBAC）
- [ ] DNS 故障转移配置（健康检查 + 低 TTL）
- [ ] 数据库复制配置（流复制/WAL 归档）
- [ ] Secret/ConfigMap 同步方案
- [ ] 灾备集群节点预扩容方案

### 恢复演练

- [ ] 演练频率：Tier 1 每月，Tier 2 每季度
- [ ] 演练自动化（Argo Workflow 编排）
- [ ] 演练包含：恢复 + Smoke Test + RTO 测量
- [ ] 演练报告：实际 RTO/RPO vs 目标
- [ ] 演练发现 → Action Items → 修复 → 验证
- [ ] 年度全量 DR 演练（含 DNS 切换）

### 监控告警

- [ ] 备份成功率监控（< 100% 告警）
- [ ] 备份延迟监控（超过预期时间告警）
- [ ] 灾备集群健康监控（持续可达）
- [ ] 数据库复制延迟监控（> RPO 告警）
- [ ] DNS 健康检查状态监控
- [ ] 备份存储容量监控（接近配额告警）

### 合规

- [ ] 备份保留期满足合规要求
- [ ] 备份加密（传输 + 存储）
- [ ] 备份访问审计日志
- [ ] WORM 存储（不可篡改，金融/医疗）
- [ ] 恢复演练记录（审计证据）

## Related

- [[概念/velero-disaster-recovery.md|Velero 灾难恢复]]
- [[概念/data-protection-k8s.md|K8s 数据保护]]
- [[概念/multi-cluster-dr-automation.md|多集群 DR 自动化]]
- [[概念/cross-cloud-migration-playbook.md|跨云迁移手册]]
- [[实体/rook.md|Rook]]
- [[概念/high-availability-patterns.md|高可用模式]]
- [[综合/velero-disaster-recovery.md|Velero × Disaster Recovery]]
- [[综合/multi-cluster-gitops-federation.md|多集群 × GitOps × 联邦]]
- [[综合/chaos-engineering-sre-resilience.md|混沌工程 × SRE × 弹性]]
