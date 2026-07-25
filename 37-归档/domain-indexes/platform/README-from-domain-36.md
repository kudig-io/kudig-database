---
title: 'Domain 36: 平台工程 (Platform Engineering)'
description: (IDP) 的设计与实施，涵盖 Backstage、Kratix、Golden Paths 等核心技术栈。
summary: (IDP) 的设计与实施，涵盖 Backstage、Kratix、Golden Paths 等核心技术栈。
category: general
tags:
- k8s
- serverless
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 36: 平台工程 (Platform Engineering) 是什么'
- '如何 Domain 36: 平台工程 (Platform Engineering)'
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- Domain
- '36:'
- 平台工程
- Platform
- Engineering
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 'Domain 36: 平台工程 (Platform Engineering)'
description: 平台工程 (Platform Engineering) 是 2026 年最热门的技术趋势之一。Gartner 预测到 2026 年，80% 的大型软件工程组织将建立平台团队作为应用交付可重用服务、组件和工具的内部提供者。本领域深入探讨内部开发者平台
  (IDP) 的设计与实施，涵盖 Backstage、Kratix、Golden Paths 等核心技术栈。
category: platform-engineering
tags:
- k8s
- platform-engineering
- developer-experience
- idp
- serverless
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 'Domain 36: 平台工程 (Platform Engineering) 是什么'
- '如何 Domain 36: 平台工程 (Platform Engineering)'
- Kubernetes 36 platform engineering 最佳实践
trigger_keywords:
- Domain
- '36:'
- 平台工程
- Platform
- Engineering
- platform
- engineering

tier: peripheral---

# Domain 36: 平台工程 (Platform Engineering)

> **适用范围**: 内部开发者平台、开发者体验、自助服务 | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-04

## 📋 领域概览

平台工程 (Platform Engineering) 是 2026 年最热门的技术趋势之一。Gartner 预测到 2026 年，80% 的大型软件工程组织将建立平台团队作为应用交付可重用服务、组件和工具的内部提供者。本领域深入探讨内部开发者平台 (IDP) 的设计与实施，涵盖 Backstage、Kratix、Golden Paths 等核心技术栈。

## 📚 文档目录

### 🎯 平台工程基础 (01-02)
- **[01-平台工程概述与成熟度模型](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/01-platform-engineering-overview.md)** - 平台工程定义、成熟度模型、组织架构
- **[02-内部开发者平台设计原则](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/02-idp-design-principles.md)** - IDP 设计原则、用户体验、架构模式

### 🌐 Backstage IDP 深度实践 (03-05)
- **[03-Backstage部署与配置](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/03-backstage-deployment.md)** - Backstage 架构、Kubernetes 部署、身份集成
- **[04-Backstage软件目录与TechDocs](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/04-backstage-catalog-techdocs.md)** - Software Catalog、TechDocs、API 文档
- **[05-Backstage脚手架与模板系统](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/05-backstage-scaffolder-templates.md)** - Scaffolder、模板开发、自动化工作流

### 🔧 平台即代码 (06-07)
- **[06-Kratix平台即代码](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/06-kratix-platform-as-code.md)** - Kratix Promise、平台 API、自助服务
- **[07-Crossplane平台组合](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/07-crossplane-platform-composition.md)** - Crossplane Composition、XRD、多云基础设施

### 📊 开发者体验与度量 (08-10)
- **[08-Golden Paths黄金路径设计](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/01-%E6%9E%84%E5%BB%BA/08-golden-paths-design.md)** - 黄金路径模式、最佳实践、模板设计
- **[09-开发者体验度量](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/04-%E5%BC%80%E5%8F%91%E4%BD%93%E9%AA%8C/09-developer-experience-metrics.md)** - DORA 指标、SPACE 框架、平台 KPI
- **[10-平台团队拓扑与运营](../../../10-%E5%B9%B3%E5%8F%B0%E5%B7%A5%E7%A8%8B/04-%E5%BC%80%E5%8F%91%E4%BD%93%E9%AA%8C/10-platform-team-topology.md)** - Team Topologies、平台运营、支持模式

### 🌐 前端部署平台 (11)
- **[11-Vercel前端部署平台](./[[10-平台工程/01-构建/11-vercel-frontend-deployment-platform.md|11-vercel-frontend-deployment-platform]].md)** - Vercel 平台深度指南，涵盖零配置部署、Serverless/Edge Functions、企业级安全与性能优化

## 🎯 学习路径建议

### 🔰 平台工程入门
1. **01-平台工程概述** → 理解平台工程核心概念
2. **02-IDP设计原则** → 掌握设计原则与模式
3. **08-黄金路径设计** → 了解开发者体验最佳实践

### ⭐ Backstage 平台工程师
1. **03-Backstage部署** → 部署与配置 Backstage
2. **04-软件目录** → 建立服务目录与文档
3. **05-脚手架模板** → 实现自助服务工作流

### 🏗️ 平台架构师
1. **06-Kratix** → 平台即代码实践
2. **07-Crossplane** → 多云基础设施抽象
3. **09-开发者体验度量** → 建立平台 KPI 体系

### 🌐 前端平台工程师
1. **11-Vercel平台** → 掌握 Vercel 部署与 Serverless 开发
2. **08-黄金路径设计** → 标准化前端交付流程
3. **09-开发者体验度量** → 衡量平台效能

## 📊 技术深度对比

| 文档 | 技术深度 | 实践价值 | 适用场景 | 复杂度 |
|------|----------|----------|----------|--------|
| 01-平台工程概述 | ⭐⭐⭐⭐ | 很高 | 战略规划、组织设计 | 中 |
| 02-IDP设计原则 | ⭐⭐⭐⭐ | 高 | 架构设计 | 中 |
| 03-Backstage部署 | ⭐⭐⭐⭐⭐ | 很高 | IDP 实施 | 中高 |
| 04-软件目录 | ⭐⭐⭐⭐ | 很高 | 服务治理 | 中 |
| 05-脚手架模板 | ⭐⭐⭐⭐⭐ | 很高 | 自动化工作流 | 中高 |
| 06-Kratix | ⭐⭐⭐⭐⭐ | 高 | 平台即代码 | 高 |
| 07-Crossplane | ⭐⭐⭐⭐⭐ | 高 | 多云基础设施 | 高 |
| 08-黄金路径 | ⭐⭐⭐⭐ | 很高 | 开发者体验 | 中 |
| 09-度量体系 | ⭐⭐⭐⭐ | 很高 | 平台运营 | 中 |
| 10-团队拓扑 | ⭐⭐⭐⭐ | 高 | 组织设计 | 中 |
| 11-Vercel平台 | ⭐⭐⭐⭐ | 很高 | 前端部署、全栈应用 | 低中 |

## 🔧 核心技术栈

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 内部开发者平台
Backstage (CNCF Incubating)     # 开发者门户
Kratix                          # 平台即代码
Port/Humanitec                  # 商业 IDP 方案

# 前端部署平台
Vercel                          # Next.js 深度集成，全球 Edge Network
Netlify                         # JAMstack 部署平台
Cloudflare Pages                # 边缘优先部署

# 基础设施抽象
Crossplane (CNCF Incubating)    # 多云组合
Terraform                       # 基础设施即代码

# 度量与分析
DORA Metrics                    # 交付效能
SPACE Framework                 # 开发者体验
```
## 📚 相关领域链接

- **[Domain-9: 平台运维](../domain-9-platform-operations)** - Kubernetes 平台运维
- **[Domain-19: 高级论文](../domain-19-papers)** - 平台工程与 IDP 深度实践
- **[Domain-23: GitOps CI/CD](../发布变更)** - 持续交付实践

---
*本文档由云原生技术专家团队维护，内容基于 2026 年平台工程最新实践。*


<!-- risk-assessed -->
