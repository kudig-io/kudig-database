---
title: SPIRE (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- spire
- kubelet
- istio
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIRE 是什么
- 如何 SPIRE
trigger_keywords:
- SPIRE
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SPIRE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

SPIRE（SPIFFE Runtime Environment）是 CNCF 毕业项目 SPIFFE 的参考实现，由 HPE 主导开发。它是一个生产级的工作负载身份管理平台，实现了 SPIFFE 规范定义的 Workload API 和身份验证机制。SPIRE 为云原生环境中的工作负载提供自动化的加密身份颁发、轮转和验证，无需人工干预。它支持 Kubernetes、Docker、VM、裸机等多种部署环境，是零信任架构（Zero Trust Architecture）的关键基础设施。

## Key Features（核心能力）

- **身份自动颁发**：工作负载启动时自动获取短期 SVID（X.509 或 JWT）
- **Agent-Server 架构**：Server 集群管理信任域，Agent 以 DaemonSet 运行在每个节点
- **节点认证**：支持多种节点认证方式（K8s PSAT、AWS IID、GCP、Azure）
- **可插拔架构**：通过插件扩展密钥存储、节点认证、工作负载注册器
- **SVID 自动轮转**：短期凭证到期前自动轮转，无需应用感知
- **联邦信任**：支持跨信任域的联邦身份验证

## 架构与工作原理

SPIRE 采用 Server-Agent 架构。SPIRE Server 负责签发 SVID、管理信任域和签名密钥，支持集群部署实现 HA。Server 通过 KeyManager 插件管理私钥（支持内存、Disk、AWS KMS、HashiCorp Vault 等）。SPIRE Agent 运行在每个节点，通过 Node Attestor 证明节点身份，通过 Workload Registrar 注册工作负载身份映射。Agent 暴露 Workload API（Unix Domain Socket），工作负载通过它获取 SVID。

## K8s 集成

在 Kubernetes 中，SPIRE Server 通过 StatefulSet 部署实现 HA，使用 PVC 存储数据库。SPIRE Agent 以 DaemonSet 形式部署，通过 K8s ServiceAccount Token 进行节点认证。工作负载通过 Pod 的 ServiceAccount 和 Namespace 自动映射到 SPIFFE 身份。可集成 Envoy 的 SPIFFE 校验过滤器实现透明 mTLS。

## 生产用例

- **零信任服务网格**：无需 Istio 即可实现服务间 mTLS 双向认证
- **跨集群信任**：在不同 K8s 集群间建立统一的工作负载身份信任
- **合规审计**：提供细粒度的工作负载身份和访问审计
- **混合云安全**：统一管理 K8s、VM、Serverless 工作负载的身份

## 安装与快速开始

```bash
helm repo add spiffe https://spiffe.github.io/helm-charts/
helm install spire-crds spiffe/spire-crds -n spire-server
helm install spire spiffe/spire -n spire-server
```

## 对比替代方案

相比 HashiCorp Vault（密钥管理），SPIRE 专注于工作负载身份而非密钥。相比 Istio 自带的 mTLS，SPIRE 提供跨平台、跨网格的身份标准化。

## Related

- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[carina]] — Carina
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spire
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
