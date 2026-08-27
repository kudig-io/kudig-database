---
title: Change Management
description: 变更管理知识域 — 变更窗口与审批、金丝雀发布策略、回滚 Playbook、密钥轮换、变更流程
category: subdomain
tags:
- change-management
- canary
- rollback
- approval
- release-window
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 变更管理 Change Management

> 生产变更的全流程管控，平衡发布速度与系统稳定性。

## 变更风险分级

| 级别 | 影响范围 | 审批要求 | 示例 |
|------|----------|----------|------|
| 标准变更 | 低风险 | 自动审批 | 配置更新、日志级别 |
| 普通变更 | 中风险 | Peer Review | 服务版本升级 |
| 重大变更 | 高风险 | CAB 审批 | 数据库迁移、架构变更 |
| 紧急变更 | 任意 | 事后审批 | 生产故障修复 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[11-发布变更/04-变更管理/01-change-window-and-approval.md\|变更窗口与审批]] | 变更时间窗口/冻结期/审批流 | intermediate |
| [[11-发布变更/04-变更管理/02-canary-release-strategy.md\|金丝雀发布]] | 渐进式发布策略 | intermediate |
| [[11-发布变更/04-变更管理/03-change-rollback-playbook.md\|回滚 Playbook]] | 变更回滚操作手册 | advanced |
| [[11-发布变更/04-变更管理/04-secret-rotation-cicd.md\|密钥轮换]] | CI/CD 中的密钥轮换 | advanced |
| [[11-发布变更/04-变更管理/08-change-management-process.md\|变更流程]] | 端到端变更管理流程 | intermediate |

## 变更管理检查清单

- [ ] 变更有明确的回滚方案
- [ ] 变更在非高峰期执行
- [ ] 变更前后有监控对比
- [ ] 重大变更经过演练验证
- [ ] 变更记录可审计追溯

## Related

- [[11-发布变更/03-Progressive-Delivery/index.md|Progressive Delivery]]
- [[11-发布变更/05-测试质量/index.md|测试质量]]
- [[12-可靠性/index.md|可靠性]]
