---
title: Krkn
description: '## 概述'
summary: 'Krkn（原名 Kraken）是一个面向 Kubernetes 的混沌工程工具，通过向集群注入各种问题场景来测试系统的弹性和可靠性。它支持节点问题、Pod 中断、网络混沌、CPU/内存压力、时间偏移等多种混沌场景，并提供基于 Cerberus 的健康检查和告警机制，帮助团队在生产环境之前发现系统弱点。'
category: entities
tags:
- k8s
- cncf
- chaos
- krkn
- etcd
- prometheus
- grafana
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
- Krkn 是什么
- 如何 Krkn
trigger_keywords:
- Krkn
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Krkn

> **CNCF 状态**: Sandbox | **类别**: Chaos | **主要语言**: Python

## 概述

Krkn（原 krkube）是一个 CNCF 沙箱项目，由 Red Hat 开源，是专为 OpenShift/Kubernetes 设计的混沌工程工具。它专注于基础设施级故障注入——模拟节点宕机、网络中断、API Server 压力等大规模故障场景。Krkn 特别适合验证 OpenShift/K8s 生产集群的容灾能力和恢复机制。与 Chaos Mesh 专注于 Pod 级故障不同，Krkn 更关注节点和集群级别的混沌实验。

## Key Features（核心能力）

- **节点混沌**：模拟节点 NotReady、关机、网络隔离等故障
- **网络混沌**：注入集群级网络延迟、丢包、DNS 故障
- **API Server 压力**：模拟 API Server 过载和响应延迟
- **Pod 混沌**：大规模 Pod kill 和 IO 干扰
- **Scenario 框架**：通过 YAML 定义可复用的混沌场景
- **与 Prow 集成**：支持 CI/CD 流水线中的自动化混沌测试

## 架构与工作原理

Krkn 采用 Python 实现的 Scenario 驱动架构：每个 Scenario 以 YAML 配置定义故障类型、目标范围和持续时间。Krkn 核心引擎解析 Scenario 配置，通过 K8s API（如 cordon/uncordon node、delete pod）或系统命令（如 iptables、tc）执行故障注入。执行完成后自动收集指标和日志用于分析。支持与 Chaos Mesh 互补使用。

## K8s 集成

Krkn 直接通过 Kubernetes API 执行混沌操作：通过 cordon/uncordon 模拟节点故障；通过 delete pod 验证工作负载韧性；通过 NetworkPolicy 和 iptables 规则注入网络故障。Krkn 以 Job 或 CronJob 方式在 K8s 集群中运行，通过 ServiceAccount 获取所需的 API 权限。与 Prometheus 集成收集故障期间的系统指标。

## 生产用例

- **集群容灾测试**：验证多节点故障场景下的集群可用性
- **OpenShift 认证测试**：PaaS 级混沌验证
- **API Server 韧性**：验证控制平面在高负载下的表现
- **灾备演练**：模拟数据中心级故障验证 DR 方案

## 安装与快速开始

```bash
pip3 install krkn
# 运行混沌场景
krkn --config kraken-config.yaml --scenario scenarios/node_scenario.yaml
```

## 对比替代方案

相比 Chaos Mesh（Pod 级混沌），Krkn 更关注节点和集群级故障注入。相比 LitmusChaos，Krkn 更专注于 OpenShift/K8s 基础设施混沌。

## Related

- [[devfile]] — Devfile
- [[cohdi]] — Cohdi
- [[koordinator]] — Koordinator
- [[oxia]] — Oxia
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- krkn
- [[实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
