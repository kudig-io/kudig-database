---
title: Keylime (entities)
description: '## 概述'
summary: 'Keylime 是一个基于 TPM (Trusted Platform Module) 的远程引导完整性验证和运行时完整性监控系统。它利用硬件 TPM 芯片提供加密度量，持续验证节点的引导过程和运行时状态是否被篡改，适用于零信任安全架构中的节点信任验证。'
category: entities
tags:
- k8s
- cncf
- security
- keylime
- argocd
- containerd
- rook
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
- Keylime 是什么
- 如何 Keylime
trigger_keywords:
- Keylime
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Keylime

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust, Python

## 概述

Keylime 是一个 CNCF 沙箱项目，由 MIT Lincoln Laboratory 和 Red Hat 联合开发，是一个基于 TPM（Trusted Platform Module）的节点完整性验证框架。它通过硬件信任根（TPM 2.0）远程验证计算节点的系统完整性，确保节点在启动和运行过程中未被篡改。Keylime 特别适合零信任架构中的节点准入控制——只有通过完整性验证的节点才能加入集群。项目已被 Red Hat OpenShift、ACAS（US Army）等采用。

## Key Features（核心能力）

- **TPM 2.0 远程证明**：通过硬件 TPM 验证节点启动链完整性
- **运行时完整性监控**：通过 IMA（Integrity Measurement Architecture）持续监控文件完整性
- **密钥分发**：仅向已验证节点分发加密密钥
- **VTPM 支持**：支持虚拟 TPM 用于 VM 和容器环境
- **多租户**：通过 Tenant API 管理多个节点的验证策略
- **REST API**：提供 RESTful API 管理验证器和节点

## 架构与工作原理

Keylime 由三个核心组件构成：Verifier（验证器）定期收集节点的 TPM Quote 并验证 PCR 值；Registrar（注册器）管理节点的 TPM 身份注册和 EK 证书验证；Tenant（租户 API）提供管理接口，定义节点的完整性策略和密钥分发规则。Agent（keylime_agent）运行在被验证节点上，提供 TPM 访问接口和 IMA 日志。验证通过后，Verifier 触发密钥分发，将节点加入受信集合。

## K8s 集成

Keylime 可集成到 Kubernetes 节点准入流程中。Node Bootstrapping 阶段，新节点需通过 Keylime 验证后才能加入集群。Keylime Agent 以 DaemonSet 或系统服务运行在每个节点上。通过 ValidatingWebhook 在 Pod 创建时验证节点完整性状态，拒绝将工作负载调度到未验证节点。与 Cluster API 集成可实现自动化的节点验证和准入。

## 生产用例

- **零信任集群**：确保所有 K8s 节点通过硬件级完整性验证
- **合规要求**：满足 NIST 800-155 等远程证明标准
- **供应链安全**：验证节点启动链未被篡改
- **机密计算**：为安全敏感工作负载提供可信执行环境验证

## 安装与快速开始

```bash
# 安装 Keylime
pip3 install keylime
# 启动 Verifier
keylime_verifier
# 启动 Registrar
keylime_registrar
# 节点上启动 Agent
keylime_agent
```

## 对比替代方案

相比软件级安全工具（Falco/Tracee），Keylime 基于硬件 TPM 提供更底层的完整性保证。相比 AWS Nitro Enclaves，Keylime 是开源的且不锁定云厂商。

## Related

- [[02-containerd-v2-features]] — [[containerd|containerd]]rd 2.0 新特性|containerd 2.0 新特性]]
- [[karmada]] — Karmada
- [[rook]] — Rook
- [[microcks]] — Microcks
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- keylime


<!-- risk-assessed -->
