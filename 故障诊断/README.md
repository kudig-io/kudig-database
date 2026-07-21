---
title: Troubleshooting & Diagnostics
description: 故障诊断知识域 — FTA 故障树、FEBM 方法论、资源/基础设施/高级排障、JVM 调优、多故障场景
summary: 故障诊断知识域入口，涵盖 FTA 故障树分析、FEBM 法医鉴定循证方法论、Pod/节点/网络/存储排障、JVM 调优、工具套件、技能体系
category: domain
tags:
- troubleshooting
- diagnostics
- fta
- febm
- runbook
- incident-response
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: advanced
audience:
- SRE
- 所有工程师
- 平台团队
estimated_read_time: 10min
---
# 故障诊断 Troubleshooting

> FTA 故障树、FEBM 方法论、资源/基础设施/高级排障、JVM 调优、多故障场景与工具套件。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[故障诊断/FTA故障树/README.md\|FTA故障树/]] | FTA | 36 个故障树、Mermaid 图、JSON 工作流 |
| [[故障诊断/FEBM方法论/README.md\|FEBM方法论/]] | FEBM | 法医鉴定循证、证据链、根因分析 |
| [[故障诊断/核心排障/README.md\|核心排障/]] | 核心 | Pod/Deployment/Service 常见故障 |
| [[故障诊断/资源排障/README.md\|资源排障/]] | 资源 | CPU/内存/磁盘/配额问题 |
| [[故障诊断/基础设施排障/README.md\|基础设施排障/]] | 基础设施 | 节点/网络/存储/DNS 故障 |
| [[故障诊断/高级排障/README.md\|高级排障/]] | 高级 | 控制平面/etcd/证书/准入控制 |
| [[故障诊断/多故障场景/README.md\|多故障场景/]] | 多故障 | 级联故障、雪崩、复合问题 |
| [[故障诊断/JVM调优/README.md\|JVM调优/]] | JVM | GC 调优、内存泄漏、线程分析 |
| [[故障诊断/工具/README.md\|工具/]] | 工具 | kubectl-debug、stern、k9s、诊断脚本 |
| [[故障诊断/技能体系/README.md\|技能体系/]] | 技能 | 故障诊断技能树、培训路径 |
| [[故障诊断/QA语料/README.md\|QA语料/]] | QA | 问答语料、知识库训练数据 |

## 跨域导航

- [[AI基础设施/README.md|AI基础设施]]
- [[专项技术/README.md|专项技术]]
- [[云厂商/README.md|云厂商]]
- [[发布变更/README.md|发布变更]]
- [[可观测性/README.md|可观测性]]
- [[可靠性/README.md|可靠性]]
- [[存储/README.md|存储]]
- [[安全/README.md|安全]]
- [[容器运行时/README.md|容器运行时]]
- [[工作负载/README.md|工作负载]]
- [[平台工程/README.md|平台工程]]
- [[应用模式/README.md|应用模式]]
- [[数据库中间件/README.md|数据库中间件]]
- [[清单模式/README.md|清单模式]]
- [[生产运维/README.md|生产运维]]
- [[生态参考/README.md|生态参考]]
- [[系统基础/README.md|系统基础]]
- [[网络/README.md|网络]]
- [[集群基础/README.md|集群基础]]
