---
title: 知识图谱 (Knowledge Map)
description: NET_FUND[domain-15 网络基础] --> K8S_NET[domain-5 K8s 网络]
category: general
tags:
- k8s
- docker
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 知识图谱 (Knowledge Map) 是什么
- 如何 知识图谱 (Knowledge Map)
trigger_keywords:
- 知识图谱
- Knowledge
- Map
---

# 知识图谱 (Knowledge Map)

> 知识模块间的依赖关系和学习路径

---

## 核心知识图谱

```mermaid
graph TD
    LINUX[domain-14 Linux] --> DOCKER[domain-13 Docker]
    NET_FUND[domain-15 网络基础] --> K8S_NET[domain-5 K8s 网络]
    STORE_FUND[domain-16 存储基础] --> K8S_STORE[domain-6 K8s 存储]
    
    DOCKER --> ARCH[domain-1 架构基础]
    ARCH --> DESIGN[domain-2 设计原理]
    DESIGN --> CTRL[domain-3 控制平面]
    
    CTRL --> WORKLOAD[domain-4 工作负载]
    CTRL --> K8S_NET
    CTRL --> K8S_STORE
    CTRL --> SEC[domain-7 安全合规]
    
    WORKLOAD --> OBS[domain-8 可观测性]
    K8S_NET --> OBS
    SEC --> OBS
    
    OBS --> PLAT[domain-9 平台运维]
    PLAT --> EXT[domain-10 扩展生态]
    
    PLAT --> TS[domain-12 故障排查]
    OBS --> TS
    
    EXT --> AI[domain-11 AI 基础设施]
```

---

## 方法论关系

```mermaid
graph LR
    FTA[topic-fta FTA 故障树] --> |推理骨架| SKILLS[topic-skills 运维技能]
    FEBM[topic-febm FEBM 取证] --> |证据方法| SKILLS
    FTA --> |演绎法| TS[故障排查]
    FEBM --> |归纳法| TS
    TS12[domain-12] --> TS
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
| domain-1 架构 | domain-13 Docker | domain-2 设计原理 |
| domain-2 设计 | domain-1 架构 | domain-3 控制平面 |
| domain-3 控制平面 | domain-2 设计 | domain-4/5/6/7 |
| domain-5 网络 | domain-15 网络基础 | domain-12 排障 |
| domain-6 存储 | domain-16 存储基础 | domain-12 排障 |
| domain-8 可观测 | domain-3/4/5 | domain-9 平台运维 |
| domain-11 AI | domain-4 工作负载 | topic-ai-agent |
| domain-12 排障 | domain-1~8 任一 | topic-fta/skills |
| topic-fta | domain-12 排障基础 | topic-skills |
| topic-skills | topic-fta + domain-12 | topic-ai-agent |
