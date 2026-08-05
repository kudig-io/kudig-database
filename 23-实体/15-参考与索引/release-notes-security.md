---
title: 发布说明索引 — 安全
description: '# 发布说明索引 — 安全'
summary: '# 发布说明索引 — 安全'
category: references
tags:
- k8s
- release-notes
- security
- cert-manager
- falco
- gatekeeper
- opa
- trivy
- crd
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — 安全 是什么
- 如何 发布说明索引 — 安全
trigger_keywords:
- 发布说明索引
- 安全
prerequisites:
- kubectl-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — 安全

> 本文档汇总安全领域 5 个核心项目的发布说明索引，共覆盖 **218 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| cert-manager | 37 | v1.20 | v1.5 | TLS 证书自动化管理 |
| Falco | 43 | v0.43 | v0.43 | 运行时威胁检测 |
| Gatekeeper | 24 | v3.22 | v3.22 | 准入策略控制 |
| OPA | 86 | v1.15 | v1.8 | 通用策略引擎 |
| Trivy | 28 | v0.69 | v0.23 | 容器安全扫描 |

---

## 项目详情

### cert-manager

- **实体页面**: [[cert-manager|cert-manager]]
- **最新版本**: v1.20
- **发布说明目录**: `生态参考/_archived-release-notes/security/cert-manager/`
- **版本覆盖**: v0.1 → v1.20（37 个版本）
- **Breaking Changes 提醒**:
  - v1.5: CRD API 版本从 v1alpha2/v1alpha3 升级到 v1
- **升级要点**: v1.x 系列稳定，建议保持最新补丁版本以获取安全修复

### Falco

- **实体页面**: [[falco|Falco]]
- **最新版本**: v0.43
- **发布说明目录**: `生态参考/_archived-release-notes/security/falco/`
- **版本覆盖**: v0.1 → v0.43（43 个版本）
- **Breaking Changes 提醒**:
  - v0.43: 规则语法和驱动模型变更
- **升级要点**: 引入插件架构，支持多种事件源

### Gatekeeper

- **实体页面**: Gatekeeper
- **最新版本**: v3.22
- **发布说明目录**: `生态参考/_archived-release-notes/security/gatekeeper/`
- **版本覆盖**: v0.1 → v3.22（24 个版本）
- **Breaking Changes 提醒**:
  - v3.22: 策略模板评估行为变更
- **升级要点**: 持续优化审计性能和约束模板功能

### OPA

- **实体页面**: [[opa|OPA]]
- **最新版本**: v1.15
- **发布说明目录**: `生态参考/_archived-release-notes/security/opa/`
- **版本覆盖**: v0.1 → v1.15（86 个版本）
- **Breaking Changes 提醒**:
  - v1.8: Rego 语言特性变更
- **升级要点**: v1.x 引入内置函数和性能优化

### Trivy

- **实体页面**: [[23-实体/06-安全/trivy.md|Trivy]]
- **最新版本**: v0.69
- **发布说明目录**: `生态参考/_archived-release-notes/security/trivy/`
- **版本覆盖**: v0.1 → v0.69（28 个版本）
- **Breaking Changes 提醒**:
  - v0.23: 扫描输出格式变更
- **升级要点**: 支持 IaC 扫描、SBOM 生成和漏洞数据库自动更新

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v1.5 | cert-manager | CRD API 版本升级到 v1 |
| v0.43 | Falco | 规则语法和驱动模型变更 |
| v3.22 | Gatekeeper | 策略模板评估行为变更 |
| v1.8 | OPA | Rego 语言特性变更 |
| v0.23 | Trivy | 扫描输出格式变更 |

---

## 相关导航

- [[22-概念/12-研究/security-tool-evolution.md|安全工具演进]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[23-实体/15-参考与索引/kudig-contribution-guide.md|kudig-contribution-guide]] — 贡献指南、项目概览与版本发布说明
- [[opa]] — OPA (Open Policy Agent)
- [[falco]] — Falco
- [[23-实体/06-安全/trivy.md|trivy]] — Trivy
- [[cert-manager]] — cert-manager


<!-- risk-assessed -->
