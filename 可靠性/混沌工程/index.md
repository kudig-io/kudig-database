---
title: Chaos Engineering
description: 混沌工程知识域 — Chaos Mesh/Litmus 实验设计、GameDay 演练、爆炸半径控制、自动化混沌
summary: 混沌工程子目录索引，涵盖混沌工程原理、Chaos Mesh 部署、实验设计方法论、Litmus 实践、GameDay 作战手册、爆炸半径控制
category: subdomain
tags:
- chaos-engineering
- chaos-mesh
- litmus
- gameday
- resilience
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 混沌工程 Chaos Engineering

> 通过主动注入故障验证系统韧性，发现潜在弱点。

## 核心概念

| 概念 | 说明 |
|------|------|
| 稳态假设 | 定义系统正常运行的可量化指标 |
| 实验变量 | 注入的故障类型（网络/Pod/节点/IO） |
| 爆炸半径 | 故障影响的范围控制 |
| 自动回滚 | 实验失败时的自动恢复机制 |

## 文档索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[可靠性/混沌工程/01-chaos-engineering-overview.md\|混沌工程概述]] | 原理、原则、成熟度模型 | beginner |
| [[可靠性/混沌工程/02-chaos-mesh-deployment.md\|Chaos Mesh 部署]] | 架构、安装、CRD 详解 | intermediate |
| [[可靠性/混沌工程/03-chaos-experiment-design.md\|实验设计]] | 假设驱动、实验设计方法论 | advanced |
| [[可靠性/混沌工程/04-litmus-practices.md\|Litmus 实践]] | LitmusChaos 部署与实验 | intermediate |
| [[可靠性/混沌工程/05-chaos-experiment-automation.md\|实验自动化]] | CI/CD 集成、定时实验 | advanced |
| [[可靠性/混沌工程/06-game-day-runbook-template.md\|GameDay 手册]] | 五阶段模板、角色、Checklist | advanced |
| [[可靠性/混沌工程/07-blast-radius-control.md\|爆炸半径控制]] | 命名空间/标签/百分比控制 | advanced |

## 工具对比

| 工具 | 类型 | 故障类型 | K8s 集成 |
|------|------|----------|----------|
| Chaos Mesh | CNCF | Pod/网络/IO/内核 | 原生 CRD |
| LitmusChaos | CNCF | Pod/节点/应用 | Operator |
| Gremlin | 商业 | 全栈 | Agent |
| AWS FIS | 云服务 | AWS 资源 | IAM |

## Related

- [[可靠性/SRE实践/index.md|SRE 实践]] — SLO/Error Budget
- [[可靠性/灾难恢复/index.md|灾难恢复]] — DR Playbook
- [[可观测性/告警/index.md|告警]] — 实验监控

