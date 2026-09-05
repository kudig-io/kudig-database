---
title: Ingress 网关故障诊断
category: reference
tags:
- reference
tier: supporting
created: '2026-07-27'
---

# Ingress 网关故障诊断

## 概述

Ingress Controller（Nginx Ingress、Higress）路由失败、TLS 终止异常、后端健康检查、注解配置错误的故障树诊断。

## 文件索引

| 文件 | 覆盖场景 |
|:---|:---|
| [ingress-fta.md](ingress-fta.md) | Ingress 通用 FTA 故障树（Controller/TLS/路由/后端/注解） |
| [nginx-ingress-fta.md](nginx-ingress-fta.md) | Nginx Ingress Controller 专项故障树 |
| [higress-fta.md](higress-fta.md) | Higress 云原生网关专项故障树 |

## 相关链接

- [[26-技能/05-网络/gateway-api/gateway-api-fta.md|Gateway API 故障树]]
- [[26-技能/05-网络/service/service-fta.md|Service 故障树]]

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
