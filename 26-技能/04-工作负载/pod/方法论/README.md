---
title: FTA 诊断方法论与技能建设
description: FTA 故障树诊断方法论、诊断执行引擎、症状匹配、Runbook 自动化、Agent 编排与技能建设规范
summary: Kubernetes 诊断方法论中枢 — FTA 核心原理、诊断引擎、症状向量匹配、Top Events 索引、Agent 编排模式与技能编写规范
category: methodology
tags:
- fta
- methodology
- diagnostics
- agent-orchestration
tier: core
audience:
- SRE
- 平台工程师
- 技能维护者
estimated_read_time: 20min
---

# FTA 诊断方法论与技能建设

> 本目录集中承载跨专题的诊断方法论、诊断引擎与技能建设规范。以 Pod 为核心锚点（FTA Top Events 以工作负载为中心），供全部 8 大专题共同引用。

## 方法论与引擎

| 文档 | 说明 |
|:---|:---|
| [FTA Methodology and Core Principles.md](FTA%20Methodology%20and%20Core%20Principles.md) | FTA 故障树分析方法论与核心原则 |
| [FTA Diagnostic Execution Engine.md](FTA%20Diagnostic%20Execution%20Engine.md) | 诊断执行引擎 |
| [Symptom Vector Matching Engine.md](Symptom%20Vector%20Matching%20Engine.md) | 症状向量匹配引擎 |
| [FTA-Driven Runbook Automation.md](FTA-Driven%20Runbook%20Automation.md) | FTA 驱动的 Runbook 自动化 |
| [Kubernetes FTA Top Events Index.md](Kubernetes%20FTA%20Top%20Events%20Index.md) | K8s FTA 顶事件索引 |
| [Kubernetes Diagnostic Skills Overview.md](Kubernetes%20Diagnostic%20Skills%20Overview.md) | 诊断技能总览 |

## 技能参考资料

| 文档 | 说明 |
|:---|:---|
| [skill-reference-diagnostic-workflow.md](skill-reference-diagnostic-workflow.md) | 诊断工作流参考 |
| [skill-reference-remediation-playbook.md](skill-reference-remediation-playbook.md) | 修复手册参考 |
| [skill-reference-root-cause-catalog.md](skill-reference-root-cause-catalog.md) | 根因目录参考 |
| [skill-MOC.md](skill-MOC.md) / [skill-README.md](skill-README.md) / [skills-run-README.md](skills-run-README.md) | 技能地图与运行说明 |

## Agent 编排 `agent/`

| 文档 | 说明 |
|:---|:---|
| [agent/Agent Orchestration Patterns.md](agent/Agent%20Orchestration%20Patterns.md) | Agent 编排模式 |
| [agent/kudig-agent-specs-collection.md](agent/kudig-agent-specs-collection.md) | Agent 规格集合 |
| [agent/kudig-prompts-catalog.md](agent/kudig-prompts-catalog.md) | 提示词目录 |

## 技能建设规范

| 文档 | 说明 |
|:---|:---|
| [技能建设最佳实践.md](技能建设最佳实践.md) | 技能文件编写规范与最佳实践 |

## Related

- [[02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[27-标签/06-AI与专项/research|research]]
