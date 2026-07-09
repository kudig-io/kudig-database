---
title: 知识图谱 (Knowledge Map) [metadata]
description: NET_FUND[网络 网络基础] --> K8S_NET[网络
  K8s 网络]
summary: NET_FUND[网络 网络基础] --> K8S_NET[网络
  K8s 网络]
category: general
tags:
- k8s
- docker
- agent
tier: peripheral
created: '2026-05-23'
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
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 知识图谱 (Knowledge Map)

> 知识模块间的依赖关系和学习路径

---

## 核心知识图谱

```mermaid
graph TD
    LINUX[系统基础 Linux] --> DOCKER[容器运行时 Docker]
    NET_FUND[网络 网络基础] --> K8S_NET[网络 K8s 网络]
    STORE_FUND[存储 存储基础] --> K8S_STORE[存储 K8s 存储]
    
    DOCKER --> ARCH[集群基础 架构基础]
    ARCH --> DESIGN[集群基础 设计原理]
    DESIGN --> CTRL[集群基础 控制平面]
    
    CTRL --> WORKLOAD[工作负载 工作负载]
    CTRL --> K8S_NET
    CTRL --> K8S_STORE
    CTRL --> SEC[安全 安全合规]
    
    WORKLOAD --> OBS[可观测性 可观测性]
    K8S_NET --> OBS
    SEC --> OBS
    
    OBS --> PLAT[平台工程 平台运维]
    PLAT --> EXT[专项技术 扩展生态]
    
    PLAT --> TS[故障诊断 故障排查]
    OBS --> TS
    
    EXT --> AI[AI基础设施 AI 基础设施]
```

---

## 方法论关系

```mermaid
graph LR
    FTA[topic-fta FTA 故障树] --> |推理骨架| SKILLS[topic-skills 运维技能]
    FEBM[topic-febm FEBM 取证] --> |证据方法| SKILLS
    FTA --> |演绎法| TS[故障排查]
    FEBM --> |归纳法| TS
    TS12[故障诊断] --> TS
    STS[topic-structural] --> TS
    SKILLS --> |自动化| AGENT[02-ai-agents]
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
| 集群基础 架构 | 容器运行时 Docker | 集群基础 设计原理 |
| 集群基础 设计 | 集群基础 架构 | 集群基础 控制平面 |
| 集群基础 控制平面 | 集群基础 设计 | 工作负载/5/6/7 |
| 网络 网络 | 网络 网络基础 | 故障诊断 排障 |
| 存储 存储 | 存储 存储基础 | 故障诊断 排障 |
| 可观测性 可观测 | 集群基础/4/5 | 平台工程 平台运维 |
| AI基础设施 AI | 工作负载 工作负载 | 02-ai-agents |
| 故障诊断 排障 | domain-1~8 任一 | 故障诊断/topic-fta/skills |
| topic-fta | 故障诊断 排障基础 | topic-skills |
| topic-skills | topic-fta + 故障诊断 | 02-ai-agents |

## Related

- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
