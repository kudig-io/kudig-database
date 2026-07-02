---
title: 供应链安全、YAML 配置清单与速查表
description: '# 供应链安全、YAML 配置清单与速查表'
summary: '1. **SBOM（Software Bill of Materials）**：软件物料清单'
category: reference
tags:
- k8s
- supply-chain-security
- sbom
- slsa
- sigstore
- yaml
- cheat-sheet
- docker
- ingress
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 供应链安全、YAML 配置清单与速查表 是什么
- 如何 供应链安全、YAML 配置清单与速查表
trigger_keywords:
- 供应链安全
- YAML
- 配置清单与速查表
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 供应链安全、YAML 配置清单与速查表

## 供应链安全

三道防线：

1. **SBOM（Software Bill of Materials）**：软件物料清单
   - 工具：Syft, Trivy SBOM, SPDX/CycloneDX 格式
2. **SLSA（Supply-chain Levels for Software Artifacts）**：构建安全等级
   - Level 1-4，从基本到最高保障
3. **Sigstore**：无密钥签名
   - Cosign（镜像签名）、Rekor（透明日志）、Fulcio（证书颁发）

## YAML 配置清单

KUDIG 提供全资源类型的 YAML 字段参考：
- 核心资源：Pod, Service, Deployment, ConfigMap, Secret
- 存储：PV, PVC, StorageClass
- 网络：Ingress, NetworkPolicy
- RBAC：Role, RoleBinding, ServiceAccount
- 扩展：CRD, Webhook

## 速查表

覆盖六大工具链：
- **kubectl**：常用命令 Top 50
- **Linux**：排障命令集
- **Docker**：镜像/容器操作
- **PromQL**：指标查询语法
- **Git**：分支管理
- **SQL**：基础查询

---

> 来源：.zread/wiki/drafts/28-*.md, .zread/wiki/drafts/29-*.md, .zread/wiki/drafts/30-*.md

## Related

- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — 纵深防御 x 供应链安全
- [[docker]] — Docker
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[entities/trivy.md|trivy]] — Trivy


<!-- risk-assessed -->
