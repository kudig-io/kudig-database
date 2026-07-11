---
title: TUF
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- supply-chain
- tuf
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- TUF 是什么
- 如何 TUF
trigger_keywords:
- TUF
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# TUF

> **CNCF 状态**: Graduated | **类别**: Supply Chain | **主要语言**: Python, Go, Rust

## 概述

The Update Framework（TUF）是一个 CNCF 毕业项目，最初由 NYU 安全研究团队开发，灵感来源于 Tor 项目中的软件更新安全研究。TUF 是一个用于保护软件更新系统安全的框架，解决镜像仓库被入侵后的降级攻击、恶意软件分发等安全威胁。它是 Notary 项目的底层规范基础，也是容器镜像签名验证体系的核心组件。TUF 规范已被 Datadog、Docker、Cloudflare、Python PyPI、RubyGems 等组织采用。

## Key Features（核心能力）

- **防降级攻击**：通过版本号和时间戳机制防止攻击者分发旧版本软件
- **防无限数据攻击**：限制元数据大小，防止客户端资源耗尽
- **防快速攻击**：使用时间戳和自动化验证防止恶意镜像快速分发
- **防混合攻击**：组合多种防御机制，覆盖已知攻击向量
- **密钥分离架构**：将根密钥、目标密钥、快照密钥、时间戳密钥分离，降低密钥泄露风险
- **委托机制**：支持多层级角色委派，实现细粒度仓库管理

## 架构与工作原理

TUF 采用分层元数据架构：Root Role 管理顶级密钥；Targets Role 定义目标文件的哈希和大小；Snapshot Role 冻结各角色元数据版本；Timestamp Role 提供最新的仓库快照引用。所有元数据通过密钥签名链式验证。客户端更新流程：获取并验证 Root 元数据 -> Timestamp 元数据 -> Snapshot 元数据 -> Targets 元数据，逐层确认文件完整性和新鲜度。

## K8s 集成

TUF 在 Kubernetes 生态中的主要实现是 Notary / Notation，用于容器镜像签名和验证。K8s 通过 Admission Controller（如 Connaisseur、Sigstore Policy Controller）在 Pod 创建时验证镜像 TUF 签名，确保仅部署经过签名的可信镜像。Containerd 和 CRI-O 均支持基于 TUF/Notary 的镜像签名验证策略。

## 生产用例

- **容器镜像签名**：在 CI/CD 流水线中对构建产物签名，部署时验证
- **软件供应链安全**：防止镜像仓库被入侵后分发恶意镜像
- **IoT 设备更新**：保护物联网设备的 OTA 固件更新安全
- **包管理器安全**：保护 PyPI、NPM 等包管理器的软件分发安全

## 安装与快速开始

```bash
# Notation CLI（基于 TUF 规范）
brew install notation

# Python TUF 参考
pip install tuf
```

## 对比替代方案

相比 Sigstore（Cosign），TUF 提供更强的仓库级完整性保护（防降级攻击），而 Sigstore 更侧重于个体镜像的透明日志签名。TUF 是规范框架，Sigstore 是实现方案。

## Related

- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tuf
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
