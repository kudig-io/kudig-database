---
title: 结构化排障方法论：配置优先、全组件排障指南
description: '## 配置优先原则'
summary: '## 配置优先原则'
category: reference
tags:
- k8s
- troubleshooting
- structured-troubleshooting
- configuration-first
- diagnostic
- etcd
- kubelet
- scheduler
- coredns
- containerd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 结构化排障方法论：配置优先、全组件排障指南 是什么
- 如何 结构化排障方法论：配置优先、全组件排障指南
- 结构化排障方法论：配置优先、全组件排障指南 故障排查
- 结构化排障方法论：配置优先、全组件排障指南 排障步骤
trigger_keywords:
- 结构化排障方法论：配置优先
- 全组件排障指南
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 结构化排障方法论

> **CNCF 状态**: 方法论 | **类别**: Troubleshooting | **主要语言**: Markdown, YAML

## 概述

Kubernetes 结构化故障排查是一套系统化的故障诊断方法论，为 K8s 生产环境提供标准化的排障流程。它将复杂的分布式系统故障分解为可管理的诊断步骤，涵盖分层排查（Pod → Node → Network → Control Plane）、证据收集、假设验证和根因定位。该方法论整合了 SRE 最佳实践、K8s 资源模型知识和运维工具链，帮助工程师在面对复杂故障时保持清晰的诊断思路，避免盲目试错。

## Key Features（核心能力）

- **分层排查模型**：从应用层到基础设施层逐层诊断（Pod → Service → Network → Node → Control Plane）
- **证据收集清单**：标准化的诊断命令和检查项清单
- **时间线分析**：基于 Events 时间线重建故障发生过程
- **假设验证框架**：系统化的假设生成和验证流程
- **自动化工具集成**：与 kubectl、k9s、HolmesGPT 等工具集成
- **知识库积累**：将故障案例转化为可复用的诊断模式

## 架构与工作原理

结构化排查方法论遵循 PDCA（Plan-Do-Check-Act）循环：Plan 阶段根据故障现象确定排查方向和优先级；Do 阶段执行诊断命令收集证据（Pod Status、Events、日志、指标）；Check 阶段分析证据验证或排除假设；Act 阶段确定根因并执行修复。排查从最上层（用户感知的问题）开始，逐步向下钻取到根本原因。每一步的证据都记录在诊断报告中，便于协作和复盘。

## K8s 集成

排查流程直接操作 K8s API 对象：从 kubectl describe pod 检查 Pod Status 和 Events 开始；通过 kubectl logs 查看应用日志；通过 kubectl get events 重建事件时间线；通过 kubectl exec 进入 Pod 测试网络连通性；通过 kubectl top 和 kubectl get nodes 检查资源使用和节点健康。对于控制平面问题，检查 kube-apiserver、etcd、scheduler 的日志和指标。

## 生产用例

- **Pod 启动失败**：系统化排查 ImagePullBackOff、CrashLoopBackOff、OOMKilled 等常见问题
- **网络不可达**：DNS 解析、Service Endpoints、NetworkPolicy 的分层诊断
- **性能降级**：从应用延迟到资源争用的性能瓶颈定位
- **控制平面异常**：API Server 延迟、etcd 性能、调度器问题的诊断

## 安装与快速开始

```bash
# 标准排查命令链
kubectl get pods -n <ns> -o wide
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous
kubectl get events -n <ns> --sort-by=.lastTimestamp
kubectl top pods -n <ns>
```

## 对比替代方案

相比「试错式」排障，结构化方法论提供可重复、可追溯的诊断流程。相比纯自动化工具（HolmesGPT），方法论指导人工和 AI 协同排查。

## Related

- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd


<!-- risk-assessed -->
