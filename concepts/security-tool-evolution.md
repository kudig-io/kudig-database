---
title: 安全工具演进
description: '# 安全工具演进'
summary: 'OPA 是通用策略引擎，Gatekeeper 是其 Kubernetes 特定的实现。'
category: concepts
tags:
- k8s
- release-notes
- falco
- opa
- trivy
- gatekeeper
- cert-manager
- security
- ingress
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全工具演进 是什么
- 如何 安全工具演进
trigger_keywords:
- 安全工具演进
prerequisites:
- kubectl-basics
- iac-basics
- ebpf-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全工具演进

> 本文档综合了 `生态参考/_archived-release-notes/security/` 目录下 5 个安全工具的 218 个版本发布说明 ^[inferred]

## 组件概览

| 组件 | 版本范围 | 定位 |
|---|---|---|
| [[falco|Falco]] | 43 个版本 | 运行时安全与异常检测 |
| [[opa|opa]] | 86 个版本 | 通用策略引擎 |
| Gatekeeper | 24 个版本 | OPA 的 Kubernetes 准入集成 |
| [[entities/trivy.md|[[Trivy|trivy]]]] | 28 个版本 | 容器和 IaC 安全扫描 |
| [[cert-manager|cert-manager]] | 37 个版本 | Kubernetes 证书管理 |

## Falco 版本演进

Falco 是云原生运行时安全项目，通过系统调用和行为检测异常。

### v0.10 关键变更

- **规则目录支持**：Falco 从 `/etc/falco/rules.d` 读取规则文件
- 支持所有系统调用（包括无参数提取的）
- 容器构建使用 gcc 5.0
- USR1 信号支持日志轮转
- 资源使用优化（限制系统调用集合）
- 新增规则：Disallowed SSH Connection、Unexpected K8s NodePort Connection、Unexpected UDP Traffic

### 后续演进

- eBPF 探针支持（替代内核模块）
- 改进的规则引擎
- 更好的 Kubernetes 集成
- 输出到多种后端（[[gRPC|gRPC]]、Webhook 等）^ [inferred]

### Falco 规则体系

Falco 通过规则定义安全策略，每个规则包含：
- 条件（使用系统调用过滤）
- 输出（告警消息）
- 优先级
- 标签

## OPA (Open Policy Agent) 版本演进

OPA 是通用策略引擎，Gatekeeper 是其 Kubernetes 特定的实现。

### v0.10 关键变更

- Hugo 文档发布到 GitHub Pages
- 新增 `array.slice` 内置函数
- 新增 `net.cidr_contains` 和 `net.cidr_intersects`（替代 `net.cidr_overlap`）
- 教程中 kube-mgmt 更新到 v0.8
- AST 集合和对象分配优化
- 新增 Kubernetes Admission Control 指南

### OPA 核心概念

| 概念 | 说明 |
|---|---|
| Rego | OPA 的策略语言 |
| Policy | 用 Rego 编写的规则 |
| Data | 策略评估的输入 |
| Query | 策略评估请求 |
| Decision | 策略评估结果（allow/deny） |

### 后续演进

- Rego 语言持续增强
- 性能优化
- 更好的 Kubernetes 集成
- WebAssembly 支持 ^[inferred]

## Gatekeeper 版本演进

Gatekeeper 将 OPA 集成到 Kubernetes 准入控制流程。

### 核心功能

- ValidatingAdmissionWebhook
- 约束模板（ConstraintTemplate）
- 约束（Constraint）
- 审计功能
- 外部数据源 ^[inferred]

## Trivy 版本演进

Trivy 是 Aqua Security 开发的全能安全扫描工具。

### 扫描能力

- 容器镜像漏洞扫描
- 文件系统扫描
- Git 仓库扫描
- IaC 扫描（Terraform、Kubernetes 等）
- SBOM 生成 ^[inferred]

## cert-manager 版本演进

cert-manager 自动化 Kubernetes 中的 TLS 证书管理。

### 核心功能

- 自动证书颁发和续期
- 支持 ACME（Let's Encrypt）
- 支持自签名和 CA 签发
- Ingress 集成
- Certificate CRD ^[inferred]

## 安全层次

```
供应链安全：Trivy（镜像扫描）+ cert-manager（证书）
    |
准入安全：OPA/Gatekeeper（策略准入）
    |
运行时安全：Falco（系统调用监控）
```

## 来源文档

- 生态参考/_archived-release-notes/security/falco/（43 个文件）
- 生态参考/_archived-release-notes/security/opa/（86 个文件）
- 生态参考/_archived-release-notes/security/gatekeeper/（24 个文件）
- 生态参考/_archived-release-notes/security/trivy/（28 个文件）
- 生态参考/_archived-release-notes/security/cert-manager/（37 个文件）

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[opa]] — OPA (Open Policy Agent)
- [[falco]] — Falco
- [[entities/trivy.md|trivy]] — Trivy
- [[cert-manager]] — cert-manager

- [[系统基础/速查卡/k8s.md|k8s]]

<!-- risk-assessed -->
