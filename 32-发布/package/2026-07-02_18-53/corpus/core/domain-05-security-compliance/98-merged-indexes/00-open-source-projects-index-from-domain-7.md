---
title: Domain-7 安全 — 开源项目索引
description: '# Domain-7 安全 — 开源项目索引'
summary: '# Domain-7 安全 — 开源项目索引'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- prometheus
- opa
- falco
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- Domain-7 安全 — 开源项目索引 是什么
- 如何 Domain-7 安全 — 开源项目索引
- Kubernetes 7 security 最佳实践
trigger_keywords:
- Domain-7
- 安全
- 开源项目索引
- security
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- tls-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-7 安全 — 开源项目索引

> **最后更新**: 2026-04-24

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Falco** | 运行时安全监控 | Graduated | v0.41.0 | 7.5k+ | Apache-2.0 |
| **OPA** | 通用策略引擎 | Graduated | v1.3.0 | 9.5k+ | Apache-2.0 |
| **Kyverno** | K8s 原生策略 | Graduated | v1.14.0 | 5.5k+ | Apache-2.0 |
| **cert-manager** | 自动 TLS | Graduated | v1.17.0 | 12.5k+ | Apache-2.0 |
| **SPIFFE/SPIRE** | 工作负载身份 | Graduated | v1.11.0 | 4k+ | Apache-2.0 |
| **TUF** | 安全更新框架 | Graduated | v4.0.0 | 3k+ | MIT |
| **in-toto** | 供应链完整性 | Graduated | v3.0.0 | 1k+ | Apache-2.0 |
| **Kubescape** | 合规扫描 | Incubating | v3.0.30 | 10k+ | Apache-2.0 |
| **Pod Security Standards** | K8s 原生安全标准 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **Gatekeeper** | OPA K8s 准入控制器 | 非 CNCF | v3.18.0 | 3.5k+ | Apache-2.0 |
| **Falco Sidekick** | Falco 响应引擎 | 社区 | v2.29.0 | 5k+ | MIT |
| **Falco Exporter** | Falco Prometheus 指标 | 社区 | v0.8.0 | 200+ | Apache-2.0 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 安全文档](https://kubernetes.io/docs/concepts/security/)
- [CNCF 安全白皮书](https://github.com/cncf/tag-security/blob/main/security-whitepaper/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-05-security-compliance MOC
- [[domain-05-security-compliance/README.md|Security Domain]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## See Also

- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-25.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/00-open-source-projects-index-from-domain-39.md|00-open-source-projects-index-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-25.md|MOC-from-domain-05-security-compliance]]
- [[domain-05-security-compliance/98-merged-indexes/MOC-from-domain-39.md|MOC-from-domain-05-security-compliance]]

- [[domain-05-security-compliance/README.md|返回目录]]

<!-- risk-assessed -->
