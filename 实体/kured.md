---
title: Kured (KUbernetes REboot Daemon)
description: '## 概述'
summary: 'Kured (KUbernetes REboot Daemon) 是一个 Kubernetes 守护进程，用于在节点需要重启时安全地执行重启操作。它检测节点上的重启信号 (如 /var/run/reboot-required 文件)，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐工作负载。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kured
- prometheus
- grafana
- coredns
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kured (KUbernetes REboot Daemon) 是什么
- 如何 Kured (KUbernetes REboot Daemon)
trigger_keywords:
- Kured
- KUbernetes
- REboot
- Daemon
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kured|Kured]] (KUbernetes REboot Daemon)

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Kured（KUbernetes REboot Daemon）是由 Weaveworks 开发的 Kubernetes 节点重启守护进程，2020 年加入 CNCF Sandbox。它在节点需要重启时（如内核安全更新）安全地执行重启操作，检测 `/var/run/reboot-required` 信号，协调节点重启以避免同时重启多个节点，并在重启前正确驱逐（Drain）工作负载。Kured 使 Kubernetes 节点的安全补丁管理变得自动化和安全。

## 核心特性

- **自动检测**: 检测 OS 发出的重启信号（reboot-required 文件）
- **协调重启**: 通过 Kubernetes Lease 确保一次只重启一个节点
- **Cordon/Drain**: 重启前自动 cordon 节点并 drain Pod
- **时间窗口**: 支持配置允许重启的时间窗口（如夜间低峰）
- **Prometheus 指标**: 暴露待重启节点数等监控指标
- **通知集成**: 支持 Slack、Teams、Webhook 通知

## 架构

Kured 以 DaemonSet 形式部署在每个节点上。每个 Kured Pod 作为特权容器运行，挂载主机的文件系统以检测 `/var/run/reboot-required` 文件。当检测到重启信号时，Kured 尝试获取集群级的 Kubernetes Lease（作为分布式锁）。获取到锁的节点执行：cordon → drain → reboot → uncordon 流程。其他节点等待锁释放后再依次执行。这确保了同一时间只有一个节点在重启，避免服务中断。

## Kubernetes 集成

Kured 通过 DaemonSet 运行在所有节点上，通过 Kubernetes API 获取和释放分布式锁（Lease 对象）。重启流程利用 `kubectl cordon` 和 `kubectl drain` 原语隔离节点并驱逐 Pod。尊重 PodDisruptionBudget，不会强制驱逐受保护的 Pod。通过 Prometheus 指标（`kured_reboot_required`）暴露状态，支持 AlertManager 集成告警。

## 生产使用场景

1. **安全补丁自动化**: 节点 OS 安装内核安全更新后，Kured 自动安全重启
2. **合规要求**: 满足定期重启/补丁管理的合规审计要求
3. **维护窗口**: 配置低峰期重启窗口，最小化业务影响
4. **大规模集群**: 协调数百个节点的滚动重启

## 安装

```bash
# Helm 安装
helm repo add kubereboot https://kubereboot.github.io/charts/
helm install kured kubereboot/kured \
  --set configuration.startTime="0:00" \
  --set configuration.endTime="4:00" \
  --set slack.enabled=true --set slack.channel=#alerts
# 或一键安装
kubectl apply -f https://github.com/kubereboot/kured/releases/download/1.16.2/kured-1.16.2-dockerhub.yaml
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kured** | 简单可靠、CNCF 项目 | 功能单一（仅重启） |
| Talos Linux | 不可变 OS、自动更新 | 需替换整个 OS |
| Flatcar LinuxUpdate Operator | 与 Flatcar 深度集成 | 仅限 Flatcar |
| AWS SSM Patch Manager | 云厂商集成 | 仅限 AWS、非 K8s 原生 |

## 架构定位

在 CNCF 生态中，Kured 属于 **Node Management / Operations** 类别，是节点生命周期运维的轻量级自动化工具。它与 PodDisruptionBudget、Cluster Autoscaler 等协同工作。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kured
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
