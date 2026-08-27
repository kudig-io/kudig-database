---
title: Disaster Recovery
description: 灾难恢复知识域 — 多区域 DR 架构、自动化 Playbook、Velero 备份、演练方法、RTO/RPO
summary: 灾难恢复子目录索引，涵盖多区域 DR 架构、自动化恢复 Playbook、Velero/K8up 备份、DR 演练、跨区域容灾
category: subdomain
tags:
- disaster-recovery
- backup
- velero
- rto-rpo
- multi-region
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 灾难恢复 Disaster Recovery

> 确保业务连续性 — 备份、恢复、容灾、演练。

## 核心指标

| 指标 | 说明 | 典型目标 |
|------|------|----------|
| RTO | 恢复时间目标 | < 15 分钟 |
| RPO | 恢复点目标 | < 5 分钟 |
| MTTR | 平均修复时间 | < 30 分钟 |

## 文档索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[12-可靠性/02-灾难恢复/01-multi-region-dr-architecture.md\|多区域 DR 架构]] | 主备/双活/多活架构设计 | advanced |
| [[12-可靠性/02-灾难恢复/03-dr-automation-playbook.md\|DR 自动化 Playbook]] | 自动化切换流程、脚本 | advanced |
| [[12-可靠性/02-灾难恢复/08-kubernetes-backup-restore-deep-dive.md\|K8s 备份恢复]] | Velero/etcd 快照/PV 备份 | intermediate |
| [[12-可靠性/02-灾难恢复/18-disaster-recovery-drills.md\|DR 演练]] | 演练计划、执行、评估 | advanced |
| [[12-可靠性/02-灾难恢复/20-cross-region-disaster-recovery.md\|跨区域容灾]] | 跨 Region 数据同步、切换 | advanced |
| [[12-可靠性/02-灾难恢复/26-velero-backup-recovery-guide.md\|Velero 指南]] | Velero 部署、备份、恢复 | intermediate |

## DR 架构模式

```
主备模式 (Active-Standby)
  └── 成本低，切换时间较长

双活模式 (Active-Active)
  └── 资源利用率高，数据一致性强

多活模式 (Multi-Active)
  └── 最高可用性，复杂度最高
```

## Related

- [[12-可靠性/01-备份恢复/index.md|备份恢复]] — Velero/etcd 备份
- [[12-可靠性/04-混沌工程/index.md|混沌工程]] — DR 验证
- [[12-可靠性/06-SRE实践/index.md|SRE 实践]] — 事件响应

