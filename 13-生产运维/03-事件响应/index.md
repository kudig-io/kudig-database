---
title: Incident Response
description: 事件响应知识域 — 升级矩阵、War Room 协调、沟通模板、事后复盘、Runbook 模板
category: subdomain
tags:
- incident-response
- escalation
- war-room
- runbook
- on-call
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 事件响应 Incident Response

> 生产事件的全流程响应体系，从检测到恢复的标准化操作。

## 事件响应流程

| 阶段 | 活动 | 负责人 |
|------|------|--------|
| 检测 | 告警触发/用户报告 | 监控系统 |
| 分级 | 严重度评估/升级 | On-Call |
| 响应 | War Room/止血操作 | IC + 团队 |
| 恢复 | 服务恢复验证 | SRE |
| 复盘 | Postmortem/改进 | 全员 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[13-生产运维/03-事件响应/01-escalation-matrix-severity-levels.md\|升级矩阵]] | 严重度分级与升级路径 | intermediate |
| [[13-生产运维/03-事件响应/02-war-room-coordination-procedures.md\|War Room]] | 作战室协调流程 | advanced |
| [[13-生产运维/03-事件响应/03-communication-templates-stakeholder.md\|沟通模板]] | 干系人沟通模板 | intermediate |
| [[12-可靠性/05-事后复盘/03-incident-postmortem-template\|复盘模板]] | 事件复盘结构化模板 | intermediate |
| [[13-生产运维/03-事件响应/10-incident-response-handling.md\|事件处理]] | 端到端事件处理流程 | advanced |
| [[13-生产运维/03-事件响应/11-incident-response-runbook-template.md\|Runbook 模板]] | 响应操作手册模板 | advanced |

## Related

- [[12-可靠性/05-事后复盘/index.md|事后复盘]]
- [[09-可观测性/05-告警/index.md|告警 Alerting]]
- [[13-生产运维/02-集群治理/index.md|集群治理]]

## 文档

- [[13-生产运维/03-事件响应/04-on-call-playbook.md|03-on-call-playbook]]
- [[13-生产运维/03-事件响应/05-incident-response-template.md|04-incident-response-template]]
- [[13-生产运维/03-事件响应/escalation-playbook.md|escalation-playbook]]
