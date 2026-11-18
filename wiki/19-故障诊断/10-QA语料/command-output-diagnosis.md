---
title: 命令输出解读语料 — Agent 诊断推理核心数据 [故障诊断]
description: kubectl/系统命令输出→诊断结论的结构化映射, 供 Agent 直接用于问题推理
summary: kubectl/系统命令输出→诊断结论的结构化映射, 供 Agent 直接用于问题推理
category: agent-corpus
source: fta-skills  # M3: 来源标注（由 08-技能体系 Skills 与 06-FTA故障树 FTA 自动提取生成）
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- agent
- corpus
- etcd
- apiserver
- kubelet
- scheduler
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-19'
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- kubectl get pods 输出异常怎么解读
- kubectl describe node NotReady 怎么排查
- Pod CrashLoopBackOff 日志怎么看
- kubectl top 显示资源不足怎么处理
- etcd 健康检查失败怎么诊断
trigger_keywords:
- 命令输出
- kubectl
- 诊断
- 故障排查
- 日志解读
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
---

> **内容已并入主场**：[[wiki/19-故障诊断/10-QA语料/command-output-diagnosis.with_actions.md|命令输出解读语料 — Agent 诊断推理核心数据 [故障诊断]]]（本页为重排去重桩，正文见主场）
