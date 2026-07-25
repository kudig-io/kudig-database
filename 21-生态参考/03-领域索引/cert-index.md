---
title: Certificate / TLS 证书知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- certificate
- tls
- pki
- cert-manager
- etcd
- apiserver
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Certificate TLS 证书知识图谱 是什么
- 如何 Certificate TLS 证书知识图谱
trigger_keywords:
- Certificate
- TLS
- 证书
- 知识图谱
- PKI
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Certificate / TLS 证书知识图谱索引

> 知识图谱：按主题 **Certificate / TLS** 聚合相关文档，按关联度分层级组织。

---

## 一、核心文档 (直接相关)

> 这些文档以证书/TLS为主题或直接面向证书运维场景。

### 深度技术

- 证书管理与 TLS 配置
- [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]]

### 证书工具

- cert-manager 自动证书管理实践指南
- [[17-系统基础/05-速查卡/tls-pki.md|TLS/SSL 与 PKI 速查表]]

### 故障排查

- 证书故障排查 (Certificate Troubleshooting)
- [[19-故障诊断/04-高级排障/06-security-auth/02-certificate-troubleshooting.md|Kubernetes 证书故障排查指南]]

### 技能卡片

- [[19-故障诊断/08-技能体系/06-certificate-expiry.md|证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis]]

### 集群证书

- [[10-平台工程/06-代码分析/functions-cluster-cert/06-cert-rotation.md|证书轮换机制源码分析]]
- [[10-平台工程/06-代码分析/functions-cluster-cert/07-service-account-keys.md|ServiceAccount 密钥对源码分析]]
- [[10-平台工程/06-代码分析/functions-cluster-cert/13-cert-config.md|kubeadm 配置对证书生成的影响]]
- [[10-平台工程/06-代码分析/functions-cluster-cert/17-pki-security-best-practices.md|Kubernetes PKI 安全最佳实践]]

---

## 二、关联文档 (K8s 集成)

> 这些文档涉及证书但以其他 K8s 组件为主题。

### 网络安全

- 网络加密与mTLS
- Ingress TLS 与证书管理

### 控制平面

- [[19-故障诊断/04-高级排障/01-control-plane/02-etcd-troubleshooting.md|etcd 故障排查指南]]
- [[19-故障诊断/04-高级排障/01-control-plane/01-apiserver-troubleshooting.md|API Server 故障排查指南]]

### 安全

- [[19-故障诊断/04-高级排障/06-security-auth/01-rbac-troubleshooting.md|RBAC 与认证故障排查指南]]
- [[19-故障诊断/04-高级排障/01-control-plane/07-control-plane-security-troubleshooting.md|控制平面安全加固故障排查指南]]

### 术语词典

- [[17-系统基础/06-知识字典/operations/certificates.md|Certificates（PKI 证书与要求）]]
- [[17-系统基础/06-知识字典/configuration/secrets.md|Secrets]]
- [[17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane.md|Communication between Nodes and the Control Plane]]

---

## 三、扩展参考

> 以下为 K8s 全域参考，证书运维可参考安全、网络等章节。

### CNCF 生态

- cert-manager
- SPIFFE
- SPIRE

### 相关工具

- HashiCorp Vault Enterprise Secrets Management


<!-- risk-assessed -->
