---
title: Deployment 故障诊断与发布技能
category: reference
tags:
- reference
tier: supporting
created: '2026-07-27'
---

# Deployment 故障诊断与发布技能

## 概述

Deployment 滚动更新失败、副本异常、选择器冲突、发布策略（金丝雀/蓝绿）相关的故障诊断与操作技能。

## 文件索引

| 文件 | 覆盖场景 |
|:---|:---|
| [deployment-fta.md](deployment-fta.md) | Deployment FTA 故障树（滚动更新/副本管理/选择器/镜像拉取） |
| [deployment-rolling-update.md](deployment-rolling-update.md) | 滚动更新策略配置与故障排查 |
| [deployment-canary-and-bluegreen.md](deployment-canary-and-bluegreen.md) | 金丝雀与蓝绿发布实践 |
| [deployment-workload-selection.md](deployment-workload-selection.md) | 工作负载控制器选型指南 |

## 相关链接

- [[26-技能/04-工作负载/pod/README.md|Pod 异常诊断技能集]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/deployment/最佳实践/k8s-deployment-strategies-guide.md|发布策略最佳实践]]

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
