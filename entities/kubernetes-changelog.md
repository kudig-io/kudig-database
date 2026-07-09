---
title: Kubernetes 变更日志索引
description: '# Kubernetes 变更日志索引'
summary: '此外，还包含 19 个 RELEASE-NOTES 文件（v0.4 - v1.1），记录了 Kubernetes 早期版本的关键变更。'
category: entities
tags:
- k8s
- release-notes
- changelog
- kubernetes
- coredns
- docker
- statefulset
- job
- cronjob
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 变更日志索引 是什么
- 如何 Kubernetes 变更日志索引
trigger_keywords:
- Kubernetes
- 变更日志索引
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 变更日志索引

> 本文档是 `生态参考/_archived-release-notes/kubernetes/` 目录下 Kubernetes 变更日志的索引和摘要 ^[inferred]

## CHANGELOG 文件索引

| K8s 版本 | 文件大小 | 说明 |
|---|---|---|
| v1.2 | 42 KB | 早期版本，多容器 Pod 支持 |
| v1.3 | 85 KB | 企业级功能引入 |
| v1.4 | 137 KB | 自动扩缩增强 |
| v1.5 | 140 KB | [[StatefulSet|StatefulSet]]、RBAC alpha |
| v1.6 | 312 KB | 动态供给、[[CronJob|CronJob]] |
| v1.7 | 317 KB | 核心功能扩展 |
| v1.8 | 320 KB | RBAC/NetworkPolicy GA |
| v1.9 | 322 KB | Apps API GA |
| v1.10 | 351 KB | CSI beta、Windows 支持 |
| v1.11 | 337 KB | CoreDNS GA |
| v1.12 | 302 KB | kubeadm GA |
| v1.13 | 281 KB | 调度改进 |
| v1.14 | 279 KB | kubectl GA |
| v1.15 | 286 KB | CRD Webhooks |
| v1.16 | 354 KB | 15 个 GA API |
| v1.17 | 355 KB | 拓扑感知调度 |
| v1.18 | 383 KB | Ephemeral Containers |
| v1.19 | 502 KB | 大规模版本 |
| v1.20 | 420 KB | Docker 弃用警告 |
| v1.21 | 377 KB | PSP 弃用 |
| v1.22 | 466 KB | PSP 移除 |
| v1.23 | 435 KB | 结构化日志 |
| v1.24 | 485 KB | dockershim 移除 |
| v1.25 | 430 KB | PSA GA |
| v1.26 | 436 KB | Sidecar alpha |
| v1.27 | 478 KB | RWOOP |
| v1.28 | 469 KB | 资源健康检查 |
| v1.29 | 441 KB | CEL 验证 |
| v1.30 | 408 KB | 调度优化 |
| v1.31 | 463 KB | 安全增强 |
| v1.32 | 482 KB | 存储网络 |
| v1.33 | 379 KB | 持续演进 |
| v1.34 | 378 KB | 持续演进 |
| v1.35 | 273 KB | 持续演进 |
| v1.36 | 146 KB | 最新版本 |

## RELEASE-NOTES 索引

此外，还包含 19 个 RELEASE-NOTES 文件（v0.4 - v1.1），记录了 Kubernetes 早期版本的关键变更。

## 使用方式

1. 参考 [[concepts/kubernetes-version-evolution.md|Kubernetes 版本演进]] 了解里程碑版本的关键变更
2. 查看具体 CHANGELOG 文件了解某个版本的完整变更详情
3. 关注弃用和移除的 API，在升级前做好准备

## 来源文档

生态参考/_archived-release-notes/kubernetes/ 目录下全部 54 个文件。

## Related

- [[docker]] — Docker
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
