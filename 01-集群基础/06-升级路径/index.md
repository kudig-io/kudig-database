---
title: Upgrade Paths
description: 升级路径知识域 — K8s 版本升级策略、滚动升级、版本偏差、升级检查清单
summary: 升级路径子目录索引，涵盖集群升级策略、版本偏差控制、滚动升级流程、升级回滚、版本兼容性
category: subdomain
tags:
- upgrade
- migration
- version
- rolling-update
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 升级路径 Upgrade Paths

> 安全、平滑地完成 Kubernetes 版本升级。

## 升级原则

| 原则 | 说明 |
|------|------|
| 版本偏差 | API Server 与 kubelet 最多相差 2 个 minor 版本 |
| 顺序升级 | 先控制平面，后工作节点 |
| 滚动升级 | 逐个节点升级，保持可用性 |
| 回滚准备 | 升级前备份 etcd，准备回滚方案 |

## 文档索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[01-集群基础/06-升级路径/06-cluster-configuration-parameters.md\|集群配置参数]] | 升级相关配置参数详解 | intermediate |
| [[01-集群基础/06-升级路径/07-upgrade-paths-strategy.md\|升级策略]] | 升级路径规划、版本选择 | advanced |
| [[01-集群基础/06-升级路径/18-upgrade-migration-strategy.md\|升级迁移策略]] | 大规模集群升级、迁移 | advanced |
| [[01-集群基础/06-升级路径/99-kubernetes-v1.33-upgrade-guide.md\|v1.33 升级指南]] | 最新版本升级实操 | intermediate |
| [[01-集群基础/03-控制平面/35-cluster-upgrade-runbook.md\|升级 Runbook]] | 完整升级操作手册 | advanced |

## 升级检查清单

```
□ 备份 etcd 快照
□ 检查版本兼容性
□ 阅读 Release Notes
□ 在测试环境验证
□ 更新 API 版本（废弃 API）
□ 升级控制平面
□ 升级工作节点（滚动）
□ 验证集群状态
□ 更新监控/告警
```

## Related

- [[01-集群基础/03-控制平面/index.md|控制平面]] — 控制平面升级
- [[11-发布变更/04-变更管理/index.md|变更管理]] — 变更审批流程
- [[12-可靠性/01-备份恢复/index.md|备份恢复]] — etcd 备份

