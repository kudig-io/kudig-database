---
title: Agent QA 对语料库
description: |
  命令输出→诊断 I-O 对语料 + 结构化 QA pairs，供 SRE Agent 推理使用
summary: 命令输出→诊断 I-O 对语料 + 结构化 QA pairs，供 SRE Agent 推理使用
category: general
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- agent
- qa
- corpus
- rag
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent 开发者
- SRE
- 运维工程师
estimated_read_time: 10min
intent_queries:
- Agent 命令输出诊断语料库
- kubectl 输出解读语料
- SRE Agent I-O pairs
- Kubernetes 故障诊断训练数据
trigger_keywords:
- Agent
- QA
- 命令输出
- 诊断
- troubleshooting
- diagnostics
- qa
- corpus
- I-O pairs
prerequisites:
- kubectl-basics
- troubleshooting-methodology
---

# Agent QA 对语料库

> **生成日期**: 2026-11-25
> **QA 对总数**: 18244

## 用途

- RAG 检索质量评测
- Agent fine-tuning 数据
- 检索相关性验证

## 文件索引

| 文件 | 领域 | QA 数量 |
|------|------|--------|
| AI基础设施-qa.yaml | AI基础设施 | 747 |
| topic-application-architecture-qa.yaml | topic-application-architecture | 426 |
| topic-fta-qa.yaml | topic-fta | 88 |
| topic-skills-qa.yaml | topic-skills | 541 |
| 专项技术-qa.yaml | 专项技术 | 285 |
| 云厂商-qa.yaml | 云厂商 | 508 |
| 发布变更-qa.yaml | 发布变更 | 320 |
| 可观测性-qa.yaml | 可观测性 | 361 |
| 可靠性-qa.yaml | 可靠性 | 354 |
| 存储-qa.yaml | 存储 | 291 |
| 存储与数据-qa.yaml | 存储与数据 | 558 |
| 安全-qa.yaml | 安全 | 344 |
| 实体-qa.yaml | 实体 | 2373 |
| 容器运行时-qa.yaml | 容器运行时 | 311 |
| 工作负载-qa.yaml | 工作负载 | 216 |
| 平台工程-qa.yaml | 平台工程 | 751 |
| 应用模式-qa.yaml | 应用模式 | 550 |
| 故障案例库-qa.yaml | 故障案例库 | 515 |
| 故障诊断-qa.yaml | 故障诊断 | 1181 |
| 数据库中间件-qa.yaml | 数据库中间件 | 276 |
| 最佳实践-qa.yaml | 最佳实践 | 303 |
| 标签-qa.yaml | 标签 | 110 |
| 概念-qa.yaml | 概念 | 1508 |
| 清单模式-qa.yaml | 清单模式 | 473 |
| 源码分析-qa.yaml | 源码分析 | 524 |
| 生产运维-qa.yaml | 生产运维 | 553 |
| 生态参考-qa.yaml | 生态参考 | 289 |
| 知识字典-qa.yaml | 知识字典 | 2549 |
| 系统基础-qa.yaml | 系统基础 | 3013 |
| 综合与研究-qa.yaml | 综合与研究 | 334 |
| 网络-qa.yaml | 网络 | 760 |
| 资产-qa.yaml | 资产 | 121 |
| 集群基础-qa.yaml | 集群基础 | 505 |
| 集群基础与系统-qa.yaml | 集群基础与系统 | 920 |
