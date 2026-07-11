---
title: Tokenetes (entities)
description: '## 概述'
summary: 'Tokenetes（也称为 Vault CRD Operator）是一个 Kubernetes Operator，用于将 HashiCorp Vault 中的密钥自动同步到 Kubernetes [[Secrets|Secrets]]。它通过自定义资源 (CRD) 简化了 Vault 与 Kubernetes 的集成，支持多种认证方式和密钥类型，'
category: entities
tags:
- k8s
- cncf
- security
- tokenetes
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
- Tokenetes 是什么
- 如何 Tokenetes
trigger_keywords:
- Tokenetes
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tokenetes

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Java / Go

## 概述

Tokenetes 是一个 CNCF 沙箱项目，旨在为微服务架构提供自动化的事务令牌（Transaction Token）管理。它通过短期、范围受限的令牌替代长期 API Key，实现服务间调用的最小权限和可审计访问。Tokenetes 自动拦截和注入服务间通信的认证令牌，无需修改应用代码，实现透明的新一代服务间认证。

## Key Features（核心能力）

- **自动令牌注入**：通过 Sidecar/Init Container 自动拦截和注入认证令牌
- **短期令牌**：令牌有效期极短（分钟级），降低泄露风险
- **范围限制**：令牌绑定到特定操作和资源路径
- **Sidecar 拦截**：通过 iptables/网络代理透明拦截 HTTP 请求注入令牌
- **审计追踪**：记录每个令牌的颁发和使用，支持全链路审计
- **策略引擎**：基于属性和上下文的动态令牌策略

## 架构与工作原理

Tokenetes 由 Token Controller 和 Sidecar Proxy 组成。Token Controller 管理令牌策略和密钥。Sidecar Proxy 部署在每个服务 Pod 中，拦截出站 HTTP 请求，自动从 Token Controller 获取短期令牌并注入到请求头；拦截入站请求，验证令牌有效性并执行授权策略。令牌通过非对称加密签名，Proxy 本地缓存公钥进行快速验证。

## K8s 集成

Tokenetes 通过 Mutating Webhook 自动将 Sidecar Proxy 注入到标记的 Pod 中。Sidecar 以 init-container + sidecar 模式运行，通过 iptables 规则拦截 Pod 的所有网络流量。TokenController 通过 Deployment 部署，以 CRD（TokenPolicy）管理令牌策略。与 K8s ServiceAccount 集成，利用 Workload Identity 简化 Pod 身份认证。

## 生产用例

- **微服务间零信任**：替代长期 API Key 的服务间认证
- **合规审计**：记录每次服务间调用的认证和授权
- **最小权限实施**：为每个服务调用动态授予最小必要权限
- **API 安全加固**：为遗留 API 增加透明的事务级认证

## 安装与快速开始

```bash
kubectl apply -f https://github.com/tokenetes/tokenetes/releases/latest/download/tokenetes.yaml
```

## 对比替代方案

相比 SPIFFE/SPIRE（身份认证），Tokenetes 更关注事务级的授权令牌。相比 Service Mesh mTLS（传输层认证），Tokenetes 在应用层提供更细粒度的访问控制。

## Related

- [[kuma]] — Kuma
- [[kuberhealthy]] — Kuberhealthy
- [[实体/trivy.md|[[Trivy|trivy]]]] — Trivy
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tokenetes
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
