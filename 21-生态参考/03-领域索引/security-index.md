---
title: Security 安全知识图谱索引
description: '## Security 知识图谱'
summary: '## Security 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- security
- rbac
- network-policy
- pod-security
- opa
- falco
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 20min
intent_queries:
- Security 知识图谱 是什么
- Kubernetes 安全 相关文档
trigger_keywords:
- Security
- 知识图谱
- index
- rbac
prerequisites:
- kubectl-basics
- cncf-ecosystem
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Security 安全知识图谱索引

> 知识图谱：按关键字 **security** 聚合项目内所有相关内容。

## 核心文档 (直接相关)

### 安全知识域

- 01 - Kubernetes认证授权体系详解
- 02 - 网络安全策略与零信任架构
- 03 - 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes 安全加固]]
- 证书管理与 TLS 配置
- 11 - 密钥与敏感信息管理工具
- 14 - 策略引擎与合规
- 18 - 网络安全纵深防御体系
- 19 - 零信任安全架构实施指南
- 20 - 安全事件响应与应急处理流程

### 网络安全

- 01 - NetworkPolicy 深度实践指南
- 83 - 网络加密与mTLS
- [[23-实体/03-运行时/03-containerd-security-hardening.md|03 containerd security hardening]]

### 术语词典

- [[17-系统基础/06-知识字典/security/cloud-native-security-practices.md|09 - 云原生安全专家指南]]
- [[17-系统基础/06-知识字典/security/cloud-native-security.md|云原生安全]]
- [[17-系统基础/06-知识字典/security/controlling-access-to-the-kubernetes-api.md|控制对 Kubernetes API 的访问]]
- [[17-系统基础/06-知识字典/security/good-practices-for-kubernetes-secrets.md|Kubernetes Secrets 最佳实践]]
- [[17-系统基础/06-知识字典/security/pod-security-admission.md|Pod 安全准入]]
- [[17-系统基础/06-知识字典/security/pod-security-standards.md|Pod 安全标准]]
- [[17-系统基础/06-知识字典/security/policy-as-code.md|策略即代码（Policy as Code）]]
- [[17-系统基础/06-知识字典/security/role-based-access-control-good-practices.md|基于角色的访问控制（RBAC）最佳实践]]
- [[17-系统基础/06-知识字典/security/runtime-security.md|运行时安全]]
- [[17-系统基础/06-知识字典/security/secrets-management-deep-dive.md|密钥管理深度指南]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity.md|SPIFFE / SPIRE 与工作负载身份]]
- [[17-系统基础/06-知识字典/security/supply-chain-security.md|软件供应链安全]]
- [[17-系统基础/06-知识字典/networking/network-policies.md|Network Policies]]

## 关联文档 (K8s 集成)

### 故障排查

- [[19-故障诊断/02-资源排障/12-rbac-quota-troubleshooting.md|12 - RBAC与ResourceQuota 故障排查 (RBAC & Quota Troubleshooting)]]
- [[19-故障诊断/02-资源排障/13-certificate-troubleshooting.md|13 - 证书故障排查 (Certificate Troubleshooting)]]
- [[19-故障诊断/03-基础设施排障/32-security-troubleshooting.md|32 - 安全相关故障排查 (Security Troubleshooting)]]
- [[19-故障诊断/04-高级排障/structural-06-security-auth/01-rbac-troubleshooting.md|RBAC 与认证故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-06-security-auth/02-certificate-troubleshooting.md|Kubernetes 证书故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-06-security-auth/03-pod-security-troubleshooting.md|Pod 安全与 SecurityContext 故障排查指南]]
- [[19-故障诊断/04-高级排障/structural-06-security-auth/04-audit-logging-troubleshooting.md|审计日志故障排查指南]]

### YAML 配置参考

- 14 - Secret 全类型 YAML 配置参考
- 20 - Role / RoleBinding YAML 配置参考
- 21 - ClusterRole / ClusterRoleBinding YAML 配置参考
- 22 - NetworkPolicy YAML 配置参考
- 24 - Admission Webhook 配置参考

### 技能卡片

- [[19-故障诊断/08-技能体系/09-rbac-quota-failure.md|RBAC 权限与 ResourceQuota 故障诊断 / RBAC & ResourceQuota Troubleshooting]]
- [[19-故障诊断/08-技能体系/14-configmap-secret-failure.md|ConfigMap/Secret 配置管理故障诊断与修复 / ConfigMap & Secret Configuration Troubleshooting]]
- [[19-故障诊断/08-技能体系/18-security-incident-response.md|安全事件应急响应 / Security Incident Response]]

### FTA 故障树

- [[19-故障诊断/06-FTA故障树/list/psp-scc-fta.md|PSP/SCC 异常 FTA 树]]
- [[19-故障诊断/06-FTA故障树/list/rbac-fta.md|RBAC 异常 FTA 树]]

## 扩展参考

### 云原生安全生态

- Falco云原生安全监控深度实践
- Kyverno Enterprise Policy Management 深度实践
- HashiCorp Vault Enterprise Secrets Management 深度实践
- cert-manager 自动证书管理实践指南
- OPA Gatekeeper 策略即代码实践指南

### 供应链安全

- [[08-安全/05-供应链/05-slsa-levels-implementation.md|05 slsa levels implementation]]
- [[08-安全/05-供应链/07-sigstore-cosign-signing.md|07 sigstore cosign signing]]

### 安全生态项目

- Falco
- OPA
- Kyverno
- SPIFFE
- SPIRE
- cert-manager
- Kubewarden
- KubeArmor


<!-- risk-assessed -->
