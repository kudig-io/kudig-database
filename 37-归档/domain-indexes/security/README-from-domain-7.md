---
title: Security Domain
description: 'description: ''### 🔰 基础安全概念 (01-04)'''
summary: 'description: ''### 🔰 基础安全概念 (01-04)'''
category: general
tags:
- k8s
- opa
- falco
- rbac
- networkpolicy
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Security Domain 是什么
- 如何 Security Domain
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- Security
- Domain
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Security Domain
description: '### 🔰 基础安全概念 (01-04)'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- opa
- falco
- networkpolicy
- webhook
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Security Domain 是什么
- 如何 Security Domain
- Kubernetes 7 security 最佳实践
trigger_keywords:
- Security
- Domain
- security
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'

tier: peripheral---

# Security Domain

> **文档数量**: 21 篇 | **最后更新**: 2026-04 | **维护者**: Production Security Team | **适用版本**: Kubernetes 1.25 - 1.33+

## 📚 学习路径与文档结构

### 🔰 基础安全概念 (01-04)
理解Kubernetes安全的基础架构和核心概念

| 序号 | 文档名称 | 内容概要 | 学习时长 | 难度 |
|-----|---------|---------|---------|------|
| 01 | [认证授权体系详解](../../../08-%E5%AE%89%E5%85%A8/01-%E8%BA%AB%E4%BB%BD%E4%B8%8E%E8%AE%BF%E9%97%AE/01-authentication-authorization-system.md) | Kubernetes认证机制、RBAC授权、Webhook集成 | 2小时 | ⭐⭐ |
| 02 | [网络安全策略](../../../08-安全/02-网络安全/01-network-security-policies.md) | NetworkPolicy、CNI插件、服务网格安全 | 2小时 | ⭐⭐⭐ |
| 03 | [运行时安全防护](../../../08-安全/03-运行时安全/04-runtime-security-defense.md) | 容器安全上下文、运行时防护、威胁检测 | 2小时 | ⭐⭐⭐ |
| 04 | [审计日志与合规](../../../08-安全/06-合规审计/03-audit-logging-compliance.md) | 审计策略配置、日志收集、合规检查 | 1.5小时 | ⭐⭐ |

### 🛡️ 安全标准与规范 (05-09)
掌握企业级安全标准和最佳实践

| 序号 | 文档名称 | 内容概要 | 学习时长 | 难度 |
|-----|---------|---------|---------|------|
| 05 | [策略验证工具](../../../08-安全/04-策略治理/02-policy-validation-tools.md) | OPA/Gatekeeper、Kyverno等策略引擎使用 | 2小时 | ⭐⭐⭐ |
| 06 | [Pod安全标准](08-安全/04-策略治理/03-pod-security-standards.md) | PSP替代方案、Pod安全准入控制 | 1小时 | ⭐⭐ |
| 07 | [RBAC权限矩阵](../../../08-安全/01-身份与访问/06-rbac-matrix-configuration.md) | 权限设计、角色规划、最小权限原则 | 2小时 | ⭐⭐⭐ |
| 08 | [安全最佳实践](../../../08-安全/06-合规审计/05-security-best-practices.md) | CIS基准、安全配置清单、防护建议 | 1.5小时 | ⭐⭐ |
| 09 | [生产环境加固](../../../08-安全/06-合规审计/06-security-hardening-production.md) | 内核参数调优、组件安全配置、加固脚本 | 2小时 | ⭐⭐⭐⭐ |

### 🔐 身份与密钥管理 (10-11)
深入学习证书和密钥管理

| 序号 | 文档名称 | 内容概要 | 学习时长 | 难度 |
|-----|---------|---------|---------|------|
| 10 | [证书管理与TLS](../../../08-安全/06-合规审计/07-certificate-management.md) | PKI体系、cert-manager、证书轮换 | 3小时 | ⭐⭐⭐⭐ |
| 11 | [密钥管理工具](../../../08-安全/01-身份与访问/07-secret-management-tools.md) | External Secrets、Vault集成、加密存储 | 2.5小时 | ⭐⭐⭐⭐ |

### 📋 合规与扫描 (12-17)
合规性检查和安全扫描

| 序号 | 文档名称 | 内容概要 | 学习时长 | 难度 |
|-----|---------|---------|---------|------|
| 12 | [合规认证指南](../../../08-安全/06-合规审计/09-compliance-certification.md) | SOC2、ISO27001、PCI-DSS等认证要求 | 2小时 | ⭐⭐⭐ |
| 13 | [镜像安全扫描](../../../08-%E5%AE%89%E5%85%A8/05-%E4%BE%9B%E5%BA%94%E9%93%BE/13-image-security-scanning.md) | Trivy、Clair、Anchore等工具使用 | 1.5小时 | ⭐⭐ |
| 14 | [策略引擎详解](../../../08-安全/04-策略治理/05-policy-engines-opa-kyverno.md) | OPA Rego语言、Kyverno策略编写 | 2.5小时 | ⭐⭐⭐⭐ |
| 15 | [运行时安全检测](../../../08-安全/03-运行时安全/05-runtime-security-detection.md) | Falco/KubeArmor配置、威胁情报集成 | 2.5小时 | ⭐⭐⭐ |
| 16 | [合规审计实践](../../../08-安全/06-合规审计/10-compliance-audit-practices.md) | CIS基准测试、安全审计、漏洞评估 | 2小时 | ⭐⭐⭐ |
| 17 | [综合安全扫描](../../../08-安全/06-合规审计/11-comprehensive-security-scanning.md) | Trivy、Grype、Kubescape等全栈扫描 | 3小时 | ⭐⭐⭐⭐ |

### 🏗️ 高级安全架构 (18-21)
企业级安全架构设计与实施

| 序号 | 文档名称 | 内容概要 | 学习时长 | 难度 |
|-----|---------|---------|---------|------|
| 18 | [网络安全纵深防御](../../../08-安全/02-网络安全/03-network-defense-depth.md) | 多层防护体系、CNI安全配置、微分段 | 3小时 | ⭐⭐⭐⭐⭐ |
| 19 | [零信任架构实施](../../../08-安全/02-网络安全/04-zero-trust-architecture.md) | SPIFFE/SPIRE、身份联合、动态访问控制 | 4小时 | ⭐⭐⭐⭐⭐ |
| 20 | [事件响应流程](32-发布/package/2026-07-02_18-29/corpus/core/domain-05-security-compliance/02-incident-response/01-incident-response-process.md) | SOC建设、事件处理、取证分析 | 3小时 | ⭐⭐⭐⭐ |
| 21 | [多集群安全管理](../../../08-安全/02-网络安全/05-multicluster-security.md) | 联邦认证、统一策略、集中监控 | 4小时 | ⭐⭐⭐⭐⭐ |

## 🎯 学习建议

### 📖 初学者路径 (1-2周)
```
01 → 02 → 08 → 06 → 07 → 10
```

### 👨‍💻 进阶工程师路径 (2-3周)
```
01 → 02 → 03 → 04 → 09 → 11 → 14 → 16
```

### 🏢 企业安全专家路径 (4-6周)
```
全部文档 + 实践项目
重点关注: 18, 19, 20, 21
```

## 🛠️ 实践项目推荐

### 项目1: 基础安全加固 (初级)
- 实施RBAC权限体系
- 配置NetworkPolicy
- 部署基础审计日志

### 项目2: 企业级安全平台 (中级)
- 部署OPA/Gatekeeper策略引擎
- 集成Vault密钥管理
- 实施CI/CD安全扫描

### 项目3: 零信任架构 (高级)
- 部署SPIFFE/SPIRE身份体系
- 实施微分段网络策略
- 建立SOC监控体系

## 📊 技能评估矩阵

| 技能领域 | 初级 | 中级 | 高级 | 专家级 |
|---------|------|------|------|--------|
| 认证授权 | ☑️ | ☑️ | ☑️ | ☑️ |
| 网络安全 | ☑️ | ☑️ | ☑️ | ☑️ |
| 运行时安全 | ☐ | ☑️ | ☑️ | ☑️ |
| 合规审计 | ☐ | ☑️ | ☑️ | ☑️ |
| 策略管理 | ☐ | ☐ | ☑️ | ☑️ |
| 零信任架构 | ☐ | ☐ | ☐ | ☑️ |
| 多集群管理 | ☐ | ☐ | ☐ | ☑️ |

## 🔄 更新记录

| 版本 | 日期 | 更新内容 | 贡献者 |
|------|------|---------|--------|
| v2.1 | 2026-02 | 新增网络安全纵深防御、零信任架构等4篇高级文档 | Security Team |
| v2.0 | 2026-01 | 重构文档结构，完善基础安全内容 | Platform Team |
| v1.0 | 2025-12 | 初始版本发布 | Initial Release |

---
> **注意**: 本文档持续更新中，建议定期查看最新版本

## Related

- 相关知识域: 集群基础
- 相关知识域: 可观测性
- [[17-系统基础/05-速查卡/tls-pki.md|速查卡: tls-pki]]

- [[08-安全/README.md|返回目录]]

<!-- risk-assessed -->
