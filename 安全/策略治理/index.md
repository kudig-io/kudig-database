---
title: Policy & Governance
description: 策略治理知识域 — OPA Gatekeeper/Kyverno 策略引擎、Pod 安全标准、策略验证工具、合规即代码
category: subdomain
tags:
- opa
- kyverno
- pod-security
- policy-as-code
- admission-control
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 策略治理 Policy & Governance

> 以 Policy-as-Code 实现 Kubernetes 集群的自动化合规治理。

## 策略引擎对比

| 引擎 | 语言 | 优势 | 适用 |
|------|------|------|------|
| OPA Gatekeeper | Rego | 通用策略引擎、CNCF 毕业 | 复杂策略/多平台 |
| Kyverno | YAML | K8s 原生、无需学习新语言 | K8s 专用策略 |
| Pod Security Admission | 内置 | 无额外组件、K8s 原生 | 基础 Pod 安全 |
| ValidatingAdmissionPolicy | CEL | K8s 1.30+ 原生 | 轻量级验证 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[安全/策略治理/04-kyverno-enterprise-policy-management.md\|Kyverno 企业策略]] | 策略编写/测试/部署 | advanced |
| [[安全/策略治理/05-policy-validation-tools.md\|策略验证工具]] | conftest/kube-score/CI 集成 | intermediate |
| [[安全/策略治理/06-pod-security-standards.md\|Pod 安全标准]] | PSS/PSA 三级安全模型 | intermediate |
| [[安全/策略治理/09-opa-gatekeeper-policy.md\|OPA Gatekeeper]] | ConstraintTemplate/审计 | advanced |
| [[安全/策略治理/14-policy-engines-opa-kyverno.md\|策略引擎对比]] | OPA vs Kyverno 选型 | intermediate |
| [[安全/策略治理/99-kyverno-policy-guide.md\|Kyverno 指南]] | 完整实践指南 | advanced |
| [[安全/策略治理/99-opa-gatekeeper-policy-guide.md\|OPA 指南]] | 完整实践指南 | advanced |

## 策略治理检查清单

- [ ] 启用 Pod Security Admission（restricted 级别）
- [ ] 部署策略引擎（OPA/Kyverno）强制合规
- [ ] CI/CD 管道集成策略检查（Shift-Left）
- [ ] 策略变更走 GitOps 审批流程
- [ ] 定期审计策略覆盖率与违规事件

## Related

- [[安全/合规审计/index.md|合规审计]]
- [[安全/身份与访问/index.md|身份与访问]]
- [[清单模式/index.md|清单模式 Manifests]]
