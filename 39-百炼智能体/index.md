---
title: 阿里云百炼智能体
description: 阿里云百炼智能体平台运维实践目录索引 — 智能体部署、定时巡检、本地集群接入等场景的配置指南
summary: 百炼智能体目录索引：沉淀智能体「部署」定时巡检配置、本地 kind 集群 MCP 接入等运维实践，巡检标准与知识库值班手册单一事实源对齐。
category: index
tags:
- index
- alibaba-cloud
- bailian
- agent
tier: supporting
created: '2026-08-26'
last_updated: '2026-08-26'
---

# 阿里云百炼智能体

> 百炼智能体平台运维实践知识轨：将 KUDIG 知识库的巡检标准、排障技能交给云端智能体定时执行，实现 Agentic Ops 闭环。

## 文档索引

| # | 文档 | 说明 | 状态 |
|---|------|------|------|
| 01 | [01-每日巡检部署配置.md](01-每日巡检部署配置.md) | 部署表单、初始消息模板、调度配置、前置检查 | ✅ |
| 02 | [02-本地集群接入方案.md](02-本地集群接入方案.md) | kind 集群 MCP + 隧道拉模式 / 推送 API 推模式 | ✅ |

## 核心场景

```mermaid
graph TB
    subgraph "百炼智能体运维闭环"
        direction TB

        subgraph "配置层"
            Deploy["部署（定时/手动触发）"]
            Env["环境 + 密钥"]
            MCP["MCP 工具绑定"]
        end

        subgraph "执行层"
            Agent["智能体 AIGURU"]
            KB["KUDIG 知识库<br/>巡检标准/排障技能"]
        end

        subgraph "目标层"
            Kind["本地 kind（验证）"]
            ACK["生产 ACK（演进）"]
        end

        subgraph "送达层"
            Report["巡检报告"]
            Ding["值班群推送"]
        end
    end

    Deploy --> Agent
    Env --> Agent
    MCP --> Agent
    Agent --> KB
    Agent --> Kind
    Kind -.->|演进| ACK
    Agent --> Report
    Report --> Ding

    style Deploy fill:#FF9800,stroke:#FF9800,color:#fff
    style Agent fill:#326CE5,stroke:#326CE5,color:#fff
    style KB fill:#9C27B0,stroke:#9C27B0,color:#fff
    style ACK fill:#4CAF50,stroke:#4CAF50,color:#fff
```

## 设计原则

| 原则 | 说明 |
|------|------|
| **单一事实源** | 巡检阈值（CPU<80%、内存<85%、Restart<3）统一来自知识库值班手册，初始消息不另立标准 |
| **只读底线** | 智能体工具仅授予只读权限，巡检不执行变更类命令 |
| **先验证后定时** | 手动触发验证链路 → 再开启每日定时 |
| **测试到生产** | kind 跑通链路 → 生产换 OpenAPI/STAROps 工具，无需隧道 |

## 相关目录

- [[18-云厂商/01-阿里云/公共云/index.md|公共云/]] — 产品选型与架构方案
- [[18-云厂商/01-阿里云/公有云-ACK/index.md|公有云-ACK/]] — ACK 容器服务运维
- [[13-生产运维/07-运维手册/01-production-sre-daily-ops.md|生产环境日常巡检与值班手册]] — 巡检标准来源
- [[09-可观测性/00-总览/05-cluster-health-check.md|集群健康检查指南]] — 健康检查命令矩阵

---

> **文档版本**: v1.0  
> **最后更新**: 2026-08-26  
> **维护者**: 云原生架构师知识库
