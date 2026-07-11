---
title: Confidential Containers (CoCo)
description: '## 概述'
summary: 'Confidential Containers (CoCo) 是一个为 Kubernetes 提供机密计算能力的项目，使容器工作负载能够在硬件 TEE（可信执行环境）中运行。通过利用 AMD SEV、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据免受云提供商、管理员和其他特权软件的访问。'
category: entities
tags:
- k8s
- cncf
- security
- confidential-containers
- opa
- crd
- operator
- agent
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
- Confidential Containers (CoCo) 是什么
- 如何 Confidential Containers (CoCo)
trigger_keywords:
- Confidential
- Containers
- CoCo
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Confidential Containers (CoCo)

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Rust, Go

## 概述

Confidential Containers（CoCo）是 CNCF Sandbox 项目，为 Kubernetes 提供机密计算（Confidential Computing）能力，使容器工作负载能够在硬件 TEE（Trusted Execution Environment）中运行。通过利用 AMD SEV-SNP、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据（Data-in-Use）免受云提供商、管理员和其他特权软件的访问。它解决了云原生场景下"数据在处理时"的安全保护问题。

## 核心特性

- **硬件 TEE 隔离**: 支持 AMD SEV-SNP、Intel TDX、IBM SE 等硬件隔离技术
- **加密镜像**: 容器镜像在 TEE 内解密，防止主机窥探镜像内容
- **远程证明**: 启动前验证 TEE 可信状态（Attestation）
- **KBS 密钥管理**: Key Broker Service 安全分发镜像解密密钥
- **策略框架**: 基于 OPA 的证明策略评估
- **标准 K8s 接口**: 通过 RuntimeClass 集成，不改变用户工作负载 API

## 架构

CoCo 架构分为 Guest 侧和 Host 侧。Host 侧通过自定义 RuntimeClass（如 `kata-qemu`）与 CRI 兼容的运行时集成。当 Pod 指定 CoCo RuntimeClass 时，containerd/shim 将 Pod 启动在基于 Kata Containers 的 TEE VM 中。Guest 侧包括：confidential-image-rs（拉取并解密镜像）、attestation-agent（执行远程证明获取密钥）、kata-agent（管理容器生命周期）。KBS（Key Broker Service）作为密钥代理，仅在远程证明通过后分发解密密钥。

## Kubernetes 集成

CoCo 通过 Kubernetes RuntimeClass 集成。用户在 Pod Spec 中指定 `runtimeClassName: kata-qemu` 即可将 Pod 运行在 TEE 中。节点需配置相应的硬件支持和 Kata Containers runtime。CoCo Operator 自动管理节点上的运行时安装和配置。加密镜像通过标准的 OCI Distribution 分发，密钥通过 KBS 在证明通过后注入。支持标准的 Kubernetes Pod API，用户无需修改工作负载定义。

## 生产使用场景

1. **金融数据处理**: 在公有云上安全处理敏感金融数据，TEE 保证数据不泄露
2. **医疗数据隐私**: 符合 HIPAA 合规要求，在云端安全处理医疗记录
3. **多方安全计算**: 多个组织在不暴露原始数据的情况下进行联合分析
4. **AI 模型保护**: 保护专有 AI 模型权重不被云提供商或攻击者获取

## 安装

```bash
# 安装 CoCo Operator
kubectl apply -k "github.com/confidential-containers/operator/config/release?ref=v0.11.0"
# 部署 CoCo Runtime
kubectl apply -f - <<EOF
apiVersion: confidentialcontainers.org/v1beta1
kind: CcRuntime
metadata: { name: coco-runtime }
spec:
  runtimeName: kata
EOF
# 使用 TEE 运行 Pod
kubectl run secret-app --image=encrypted-app:latest --overrides='{"spec":{"runtimeClassName":"kata-qemu"}}'
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **CoCo** | K8s 原生、标准化 | 性能开销、硬件依赖 |
| Enarx | 架构灵活、跨平台 | 尚不成熟、社区较小 |
| Gramine | 不需要硬件 TEE | 仅 x86、非 K8s 原生 |
| Occlum | 高性能 LibOS | 仅 Intel SGX |

## 架构定位

在 CNCF 生态中，CoCo 属于 **Security** 类别，是机密计算在 Kubernetes 上的标准化入口。它与 Kata Containers、OPA、SPIFFE 等项目协同工作。

## 参考链接

- [[deployment]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- confidential-containers
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[实体/tetragon.md|Tetragon]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
