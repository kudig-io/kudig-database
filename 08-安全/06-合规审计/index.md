---
title: Compliance & Audit
description: 合规审计知识域 — K8s 审计日志、CIS Benchmark、加密体系、证书管理、安全加固、合规认证
category: subdomain
tags:
- audit-logging
- cis-benchmark
- encryption
- cert-manager
- compliance
tier: core
created: '2026-07-02'
last_updated: '2026-08-25'
---
# 合规审计 Compliance & Audit

> 从审计日志、基准合规到安全加固的全方位合规体系。

## 合规框架映射

| 框架 | 范围 | K8s 关联 |
|------|------|----------|
| CIS Benchmark | 集群配置安全 | kube-bench 扫描 |
| SOC 2 Type II | 服务控制 | 审计日志 + 访问控制 |
| PCI DSS | 支付卡数据 | 网络隔离 + 加密 |
| GDPR | 个人数据保护 | 数据加密 + 访问审计 |
| 等保 2.0 | 国内合规 | 安全加固 + 日志审计 |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[08-安全/06-合规审计/01-kubernetes-audit-logging-configuration.md\|审计日志配置]] | Audit Policy/Backend/存储 | advanced |
| [[08-安全/06-合规审计/02-encryption-at-rest-transit.md\|加密体系]] | 静态/传输加密配置 | advanced |
| [[08-安全/06-合规审计/03-audit-logging-compliance.md\|审计合规]] | 日志合规与保留策略 | intermediate |
| [[08-安全/06-合规审计/04-cis-benchmark-compliance-audit.md\|CIS Benchmark]] | kube-bench 扫描与修复 | intermediate |
| [[08-安全/06-合规审计/05-security-best-practices.md\|安全最佳实践]] | 生产安全实践汇总 | intermediate |
| [[08-安全/06-合规审计/06-security-hardening-production.md\|安全加固]] | 生产环境加固指南 | advanced |
| [[08-安全/06-合规审计/07-certificate-management.md\|证书管理]] | cert-manager/TLS 自动化 | intermediate |
| [[08-安全/06-合规审计/08-kubernetes-security-hardening.md\|K8s 加固]] | 集群安全加固全景 | advanced |
| [[08-安全/06-合规审计/09-compliance-certification.md\|合规认证]] | 合规认证流程与准备 | intermediate |
| [[08-安全/06-合规审计/10-compliance-audit-practices.md\|审计实践]] | 合规审计操作实践 | advanced |
| [[08-安全/06-合规审计/11-comprehensive-security-scanning.md\|安全扫描]] | 镜像/配置/漏洞扫描 | intermediate |
| [[08-安全/06-合规审计/13-cert-manager-tls-guide.md\|cert-manager 指南]] | TLS 证书自动化指南 | intermediate |
| [[08-安全/06-合规审计/14-java-security-kubernetes-guide.md\|Java 安全指南]] | Java on K8s 安全实践 | intermediate |

## 合规审计检查清单

- [ ] 启用 K8s Audit Log（Metadata 级别以上）
- [ ] 定期运行 kube-bench CIS 扫描
- [ ] 启用 etcd 静态加密 + TLS 传输加密
- [ ] 部署 cert-manager 自动化证书管理
- [ ] 镜像扫描集成 CI/CD（Trivy/Grype）
- [ ] 审计日志保留 ≥ 90 天并防篡改

## Related

- [[08-安全/04-策略治理/index.md|策略治理]]
- [[08-安全/01-身份与访问/index.md|身份与访问]]
- [[13-生产运维/index.md|生产运维]]

## 文档

- [[08-安全/06-合规审计/P3-11-security-incident-sop-compliance-checklist.md|P3-11-security-incident-sop-compliance-checklist]]
