---
title: Kubernetes Knowledge Map
description: NET_FUND[domain-03-networking-traffic 网络基础] --> K8S_NET[domain-03-networking-traffic
  K8s 网络]
summary: NET_FUND[domain-03-networking-traffic 网络基础] --> K8S_NET[domain-03-networking-traffic
  K8s 网络]
category: reference
tags:
- k8s
- knowledge-graph
- learning-path
- dependency-matrix
- docker
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Knowledge Map 是什么
- 如何 Kubernetes Knowledge Map
trigger_keywords:
- Kubernetes
- Knowledge
- Map
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes Knowledge Map

> 知识模块间的依赖关系和学习路径

---

## 核心知识图谱

```mermaid
graph TD
    LINUX[domain-17-system-foundation Linux] --> DOCKER[domain-13-container-runtime Docker]
    NET_FUND[domain-03-networking-traffic 网络基础] --> K8S_NET[domain-03-networking-traffic K8s 网络]
    STORE_FUND[domain-04-storage-data 存储基础] --> K8S_STORE[domain-04-storage-data K8s 存储]

    DOCKER --> ARCH[domain-01-cluster-fundamentals 架构基础]
    ARCH --> DESIGN[domain-01-cluster-fundamentals 设计原理]
    DESIGN --> CTRL[domain-01-cluster-fundamentals 控制平面]

    CTRL --> WORKLOAD[domain-02-workloads-applications 工作负载]
    CTRL --> K8S_NET
    CTRL --> K8S_STORE
    CTRL --> SEC[domain-05-security-compliance 安全合规]

    WORKLOAD --> OBS[domain-06-observability 可观测性]
    K8S_NET --> OBS
    SEC --> OBS

    OBS --> PLAT[domain-07-platform-engineering 平台运维]
    PLAT --> EXT[domain-15-specialized-tech 扩展生态]

    PLAT --> TS[domain-10-troubleshooting-diagnostics 故障排查]
    OBS --> TS

    EXT --> AI[domain-14-ai-ml-infra AI 基础设施]
```

---

## 方法论关系

```mermaid
graph LR
    FTA[topic-fta FTA 故障树] --> |推理骨架| SKILLS[topic-skills 运维技能]
    FEBM[topic-febm FEBM 取证] --> |证据方法| SKILLS
    FTA --> |演绎法| TS[故障排查]
    FEBM --> |归纳法| TS
    TS12[domain-10-troubleshooting-diagnostics] --> TS
    STS[topic-structural] --> TS
    SKILLS --> |自动化| AGENT[topic-ai-agent]
```

---

## 学习路径

```mermaid
graph LR
    W1[Week 1 基础] --> W2[Week 2 核心]
    W2 --> W3[Week 3 运维]
    W3 --> W4[Week 4 进阶]

    W1 --- L1[Linux + Docker + 架构]
    W2 --- L2[控制平面 + 网络 + 存储]
    W3 --- L3[安全 + 可观测 + 排障]
    W4 --- L4[GitOps + FTA + 最佳实践]
```

---

## 模块间依赖矩阵

| 模块 | 前置依赖 | 推荐后续 |
|:---|:---|:---|
| domain-01-cluster-fundamentals 架构 | domain-13-container-runtime Docker | domain-01-cluster-fundamentals 设计原理 |
| domain-01-cluster-fundamentals 设计 | domain-01-cluster-fundamentals 架构 | domain-01-cluster-fundamentals 控制平面 |
| domain-01-cluster-fundamentals 控制平面 | domain-01-cluster-fundamentals 设计 | domain-02-workloads-applications/5/6/7 |
| domain-03-networking-traffic 网络 | domain-03-networking-traffic 网络基础 | domain-10-troubleshooting-diagnostics 排障 |
| domain-04-storage-data 存储 | domain-04-storage-data 存储基础 | domain-10-troubleshooting-diagnostics 排障 |
| domain-06-observability 可观测 | domain-01-cluster-fundamentals/4/5 | domain-07-platform-engineering 平台运维 |
| domain-14-ai-ml-infra AI | domain-02-workloads-applications 工作负载 | topic-ai-agent |
| domain-10-troubleshooting-diagnostics 排障 | domain-1~8 任一 | domain-10-troubleshooting-diagnostics/topic-fta/skills |
| topic-fta | domain-10-troubleshooting-diagnostics 排障基础 | topic-skills |
| topic-skills | topic-fta + domain-10-troubleshooting-diagnostics | topic-ai-agent |

---

## 相关索引

- [[entities/k8s-difficulty-index.md|难度分级索引]]
- [[MOC|学习路径导航]]
- [[entities/kubectl Scenario Quick Reference.md|kubectl 场景速查]]
- [[entities/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]]
- [[entities/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]

## Related

- [[entities/k8s-difficulty-index.md|k8s-difficulty-index]] — Kubernetes Difficulty Index
- [[INDEX]] — Wiki Index
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
