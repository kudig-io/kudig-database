---
title: Domain-29 自动化测试与质量 — 开源项目索引
description: '| **Snyk** | 安全扫描平台 | Snyk | - | - | 商业 |'
summary: '| **Snyk** | 安全扫描平台 | Snyk | - | - | 商业 |'
category: automated-testing-quality
tags:
- k8s
- testing
- quality
- automation
- grafana
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- QA 工程师
- SRE
- 开发工程师
estimated_read_time: 5min
intent_queries:
- Domain-29 自动化测试与质量 — 开源项目索引 是什么
- 如何 Domain-29 自动化测试与质量 — 开源项目索引
- Kubernetes 29 automated testing quality 最佳实践
trigger_keywords:
- Domain-29
- 自动化测试与质量
- 开源项目索引
- automated
- testing
- quality
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- monitoring-basics
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-29 自动化测试与质量 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **SonarQube** | 代码质量分析 | SonarSource | v25.2.0 | 9k+ | LGPL-3.0 |
| **SonarQube Scanner** | K8s / CI 扫描 | SonarSource | v7.0.0 | - | LGPL-3.0 |
| **Trivy** | 漏洞与合规扫描 | Aqua | v0.61.0 | 24k+ | Apache-2.0 |
| **Snyk** | 安全扫描平台 | Snyk | - | - | 商业 |
| **Checkov** | IaC 安全扫描 | Bridgecrew | v3.2.0 | 7k+ | Apache-2.0 |
| **Kube-bench** | CIS K8s Benchmark | Aqua | v0.10.0 | 7k+ | Apache-2.0 |
| **Kube-hunter** | K8s 渗透测试 | Aqua | v0.6.0 | 5k+ | Apache-2.0 |
| **Polaris** | K8s 最佳实践验证 | Fairwinds | v9.0.0 | 4k+ | Apache-2.0 |
| **Pluto** | 废弃 API 检测 | Fairwinds | v5.0.0 | 1k+ | Apache-2.0 |
| **Nova** | Helm Chart 过期检测 | Fairwinds | v3.0.0 | 500+ | Apache-2.0 |
| **Selenium** | Web UI 测试 | Selenium | v4.27.0 | 31k+ | Apache-2.0 |
| **Cypress** | 前端 E2E 测试 | Cypress.io | v14.0.0 | 48k+ | MIT |
| **k6** | 负载测试 | Grafana | v0.56.0 | 26k+ | AGPL-3.0 |
| **Locust** | Python 负载测试 | Locust | v2.32.0 | 25k+ | MIT |
| **JMeter** | 性能测试 | Apache | v5.6.0 | 8k+ | Apache-2.0 |
| **Goss** | 服务器验证 | 社区 | v0.4.0 | 5k+ | Apache-2.0 |
| **Terratest** | 基础设施测试 | Gruntwork | v0.48.0 | 8k+ | Apache-2.0 |
| **Testcontainers** | 集成测试容器 | AtomicJar | v1.20.0 | 8k+ | MIT |
| **Kuttl** | K8s 测试工具 | K8s SIG | v0.21.0 | 1k+ | Apache-2.0 |
| **Chainsaw** | K8s 声明式测试 | Kyverno | v0.6.0 | 500+ | Apache-2.0 |
| **Popeye** | K8s 集群卫生检查 | Derailed | v0.22.0 | 5k+ | Apache-2.0 |
| **Kube-score** | K8s 对象静态分析 | Zegl | v1.19.0 | 3k+ | MIT |
| **SonarQube** | 代码质量分析 | SonarSource | v25.2.0 | 9k+ | LGPL-3.0 |
| **OWASP Dependency-Check** | 依赖漏洞扫描 | OWASP | v12.0.0 | 6k+ | Apache-2.0 |

---

## 参考链接

- [Trivy 文档](https://aquasecurity.github.io/trivy/)
- [Checkov 文档](https://www.checkov.io/)
- [Polaris 文档](https://polaris.docs.fairwinds.com/)
- [k6 文档](https://grafana.com/docs/k6/)

---

## Obsidian 相关文档

- domain-29-automated-testing-quality MOC
- [[发布变更/README.md|Domain 08: 自动化测试与质量保障 (Automated Testing & Quality Assurance...]]
- Selenium 企业级自动化测试平台
- JUnit 5 企业级单元测试框架深度实践
- 企业级AI测试与质量保障深度实践
- Cypress Enterprise Automation Testing 深度实践
- Playwright Enterprise Automation Testing 深度实践


<!-- risk-assessed -->
