---
title: Velero × Disaster Recovery
summary: Velero 与灾难恢复的交叉：基于 CRD 的 Kubernetes 备份/恢复工作流如何支撑 RPO/RTO 目标。
category: synthesis
tags:
- velero
- disaster-recovery
- backup
- restore
- kubernetes
tier: supporting
sources:
- 系统基础/知识字典/operations/velero.md
- 概念/velero-disaster-recovery.md
- 概念/data-protection-k8s.md
- 概念/multi-cluster-dr-automation.md
- 实体/cloudnativepg.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.74
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# Velero × Disaster Recovery

## The Connection

灾难恢复（Disaster Recovery, DR）关注在故障后以可接受的 RPO（数据丢失量）和 RTO（恢复时间）重建服务。Velero 是 Kubernetes 原生的备份/恢复工具，通过 CRD（`Backup`/`Restore`/`Schedule`）声明式地捕获集群资源（YAML）与持久卷数据（通过 CSI 快照或 Restic/Kopia 文件级备份），并把制品推到对象存储（S3/OSS/MinIO）。Velero 把 K8s 的 DR 从"手工脚本"变成"GitOps 式声明工作流"——备份是 CRD 对象而非一次性命令，恢复是声明式操作而非手动编排。在架构上，Velero 的控制器监听 `Backup` 请求后，依次执行 API 对象导出（通过 kube-apiserver list/watch）、PV 快照调用（通过 CSI snapshotter 或 Restic daemonset）、制品打包上传（到对象存储），恢复时逆向执行：下载制品 → 资源 YAML 按依赖顺序创建 → PV 数据按卷恢复。这一自动化链路使得 DR 演练从"季度大项目"变为"日常可执行的 cron 任务"。^[inferred]

## Where They Co-occur

- **资源 + 数据分离备份**：Velero `Backup` 同时捕获命名空间内的 K8s 对象（Deployment/Service/ConfigMap）与 PV 数据（CSI 快照或文件级备份），二者合为一份完整备份制品。
- **Schedule 定期备份**：`Schedule` CRD 实现 cron 化备份，直接对应 RPO（频率越高，RPO 越小）；建议生产环境按 RPO 目标设置 Schedule 间隔并监控执行成功率。
- **跨集群恢复**：把 Backup 制品从 A 集群的对象存储拉到 B 集群 `Restore`，是整体迁移/灾备的核心动作；恢复时需注意 CRD 版本兼容性和命名空间映射。
- **Restic/Kopia 文件级备份**：对不支持快照的存储（如 Local PV、NFS）或需跨存储类型恢复时，用文件级备份兜底；Kopia 是 Velero 1.12+ 的新后端，替代老旧的 Restic。
- **Hooks 联动应用**：PreHook/PostHook 在备份前后冻结数据库（如 `pg_freeze`、`FLUSH TABLES WITH READ LOCK`）、刷盘，保证应用级一致性而非 crash-consistent 快照。
- **与 CloudNativePG/Kanister 协作**：应用级一致性备份需数据库感知，Velero 通过插件或 Operator 协调——CloudNativePG 的 `VolumeSnapshot` 与 Velero Backup 联动实现 WAL + 基础备份一体化。
- **多集群 DR 自动化**：结合 GitOps 与 Velero，实现"定期备份 → 异地存储 → 故障时一键恢复"的自动化链路；Velero 的 `--namespace-mapping` 支持恢复时重映射命名空间。
- **Backup 保留策略**：Velero 支持 `--ttl` 和 ` retentionPolicy`（配合 Schedule），自动清理过期备份以控制对象存储成本，避免"备份桶无限膨胀"。
- **备份验证（Backup Verification）**：Velero 1.14+ 支持 `--verify` 标志，备份完成后自动校验制品完整性（PV 快照存在性、资源数量一致性），避免"备份成功"但恢复时才发现制品损坏。
- **命名空间映射**：Velero Restore 支持 `--namespace-mappings` 参数，恢复时将源 namespace 重映射到目标 namespace，支持"从 staging 备份恢复到 dev"的非破坏性恢复演练。
- **VolumeSnapshotClass 与 CSI 快照**：恢复时如果目标集群的 StorageClass 或 VolumeSnapshotClass 名称不同，需通过 `--snapshot-volumes` 和 `ChangeStorageClass` 插件做存储类映射。
- **Velero BSL/VSL 多目标存储**：Backup Storage Location (BSL) 支持多个配置（如 primary S3 + secondary OSS），Volume Snapshot Location (VSL) 支持 per-region CSI 快照存储——实现"资源 YAML 存一处，PV 快照存多区"的分层备份策略。
- **Velero Plugin 生态**：AWS/Azure/GCP/阿里云 CSI 驱动各自提供 Velero VolumeSnapshotter 插件，处理 provider-specific 的快照创建/删除/检查逻辑——专有云需自定义或使用通用 CSI 插件。
- **恢复后健康检查**：Velero `Restore` 完成后应自动触发 readiness probe 检查和应用级 smoke test（如数据库可连接性验证），确认恢复的 workload 不仅是"Pod Running"而是"服务可用"。
- **Velero 命名空间隔离**：生产推荐为 Velero 分配专用 namespace（`velero`）并配合 ResourceQuota 限制其 CPU/内存——Restic daemonset 在大规模集群中可能消耗大量节点资源。

## Cross-cutting Insight

DR 的本质不是"能不能备份"，而是"能不能在 RTO 内可信地恢复"。Velero 的价值在于把"恢复"变成可演练、可声明、可 Git 化的动作——团队应把 `Restore` 当作日常演练对象，而非仅靠"备份成功"的告警自慰。未演练的备份等于没有备份：恢复时才发现 CRD 已删除、PV 存储类不兼容、命名空间依赖缺失，这些故障模式只有在真实恢复演练中才会暴露。更深层的挑战在于"恢复的粒度"：全集群恢复风险极高（可能覆盖正在运行的正常资源），按 namespace/label 精准恢复更安全但需要预先设计好恢复边界。生产级 DR 策略应包含分层恢复预案——资源 YAML 优先恢复（快速重建控制面拓扑），PV 数据按优先级分批恢复（关键数据库先恢复），配置漂移检测在恢复后立即执行（确保 GitOps 期望状态与恢复状态一致）。^[inferred]

## Tensions and Trade-offs

| 维度 | Velero 备份侧重 | DR 目标侧重 | 结合注意事项 |
|---|---|---|---|
| RPO | 取决于 Schedule 频率 | 越小越好成本越高 | 频率 vs 对象存储成本 |
| 一致性 | 默认 crash-consistent | 业务需 app-consistent | 必须配 Hooks/插件 |
| 恢复范围 | 按 namespace/label | 需精准爆炸半径 | 全量恢复风险大 |
| 存储依赖 | 依赖对象存储可用 | DR 存储本身需容灾 | 备份存储需跨区/跨云 |
| 演练 | 备份易、恢复少验证 | RTO 靠演练度量 | 需常态化恢复演习 |
| 跨集群恢复 | 按制品内容恢复 | 目标集群拓扑可能不同 | 需 namespace/storageClass 映射 |
| 资源依赖 | 资源按 list 顺序恢复 | CRD/Operator 有先后依赖 | 恢复需按依赖分层编排 |

## Open Questions

- CSI 快照与 Velero 文件级备份同时存在时，如何避免数据不一致与成本翻倍？是否应按存储类型自动选择备份策略？
- 多集群 DR 自动化中，如何让 `Restore` 在目标集群不覆盖已有且更新的资源？是否需要 `--existing-resource-policy=update` 之外的保护机制？
- GitOps 管理的"期望状态"与 Velero 备份的"历史状态"在恢复时如何裁决优先级？GitOps 重建是否应先于 Velero 恢复？
- 当 Velero 备份的 CRD 在目标集群已不存在时，恢复流程如何优雅降级？是否应预置 CRD 版本兼容性检查？

## Related

- [[系统基础/知识字典/operations/velero.md|Velero]]
- [[概念/velero-disaster-recovery.md|Velero 灾难恢复]]
- [[概念/data-protection-k8s.md|K8s 数据保护]]
- [[概念/multi-cluster-dr-automation.md|多集群 DR 自动化]]
- [[实体/cloudnativepg.md|CloudNativePG]]
- [[实体/kanister.md|Kanister]]
- [[实体/k8up.md|K8up]]
- [[综合/argocd-gitops.md|ArgoCD × GitOps]]


<!-- risk-assessed -->
