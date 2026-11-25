---
title: 备份/恢复异常故障树分析
description: Kubernetes 集群备份恢复异常的故障树诊断技能，覆盖 etcd 快照、Velero 应用级备份、存储后端、加密校验、恢复流程与依赖调度等故障场景
summary: 备份恢复故障树，覆盖 etcd 快照/Velero/存储后端/加密/恢复流程/调度 6 大类 20+ 底事件
category: skill
tags:
- k8s
- backup
- restore
- velero
- etcd
- snapshot
- disaster-recovery
- fta
- troubleshooting
sources:
- 故障诊断/FTA故障树/list/backup-restore-fta.md
- 故障诊断/FTA故障树/list/etcd-fta.md
created: '2026-05-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Velero 备份失败怎么排查
- 集群备份恢复异常诊断
- etcd 快照失败原因分析
- 备份数据不一致如何处理
- 恢复后资源冲突怎么解决
trigger_keywords:
- 备份失败
- 恢复失败
- Velero
- 快照异常
- 数据不一致
- RPO
- RTO
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
fta_id: FTA-BACKUP_RESTORE-001
component: Backup/Restore
severity: critical
---

> **内容已并入主场**：[[wiki/17-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]（本页为重排去重桩，正文见主场）
