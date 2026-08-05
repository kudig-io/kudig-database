---
title: 小白 K8s 学习路线图（多路径版）
description: 面向完全零基础学习者的多路径学习路线图，包含周末自学版、在职速成版、开发转运维版、运维转 SRE 版，以及 CKA 考证路线
summary: 面向完全零基础学习者的多路径学习路线图，包含周末自学版、在职速成版、开发转运维版、运维转 SRE 版，以及 CKA 考证路线
category: learning
tags:
- roadmap
- beginner
- learning-path
- CKA
- career
- etcd
- prometheus
- grafana
- helm
- argocd
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: beginner
reading_level: beginner
audience:
- 零基础初学者
- 在职想转型者
- 培训管理者
estimated_read_time: 15min
intent_queries:
- K8s 小白怎么学
- Kubernetes 学习路线
- 从零开始学 K8s
- CKA 备考路线
trigger_keywords:
- 学习路线
- 路线图
- roadmap
- 小白学习
- 零基础
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- redis-basics
- mysql-basics
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 小白 K8s 学习路线图（多路径版）

> **适用对象**: 完全零基础（无容器经验、无云原生背景、无云厂商资源）  
> **核心原则**: **零成本起步 → 本地实验 → 概念打通 → 项目实战 → 认证加持 → 职业落地**  
> **版本**: v1.0 | 2026-05-21

---

## 快速选择你的路径

```
┌─────────────────────────────────────────────────────────────────────┐
│                         选择你的背景                                 │
├─────────────────────────────────────────────────────────────────────┤
│  A. 我是纯小白，只有周末时间      →  路径一：周末自学（6 个月）        │
│  B. 我有 1 个月全职学习时间       →  路径二：密集速成（28 天）         │
│  C. 我是开发，想转运维/平台工程    →  路径三：开发转运维（8 周）        │
│  D. 我是运维，想转 SRE/DevOps     →  路径四：运维转 SRE（6 周）       │
│  E. 我要考 CKA 认证               →  路径五：CKA 考证路线（4 周）     │
│  F. 我想快速面试找工作            →  路径六：面试突击（2 周）         │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 路径一：周末自学版（推荐 ⭐）

> **时间投入**: 每周六日各 4 小时，持续 6 个月  
> **总投入**: 约 200 小时  
> **产出目标**: 能独立在本地搭建集群、部署应用、排查常见问题、通过 CKA

### 第一阶段：地基期（Week 1-4）

| 周次 | 周六上午 | 周六下午 | 周日上午 | 周日下午 |
|------|---------|---------|---------|---------|
| W1 | [云原生演进故事](beginner-guides/01-cloud-native-evolution-story.md) | [本地环境搭建](beginner-guides/02-local-lab-environment.md) | Docker 基础（镜像/容器/Dockerfile） | 实践：本地跑第一个容器 |
| W2 | Linux 基础命令（文件/进程/网络） | 实践：在容器里玩 Linux | Git 基础（clone/add/commit/push） | 实践：把代码推到 GitHub |
| W3 | [K8s 是什么](fundamentals/01-what-is-[[Kubernetes|kubernetes]].md) | [Pod 基础](32-发布/package/2026-07-02_18-29/corpus/peripheral/skills/training-lecturer/01-getting-started/01-pod-basics.md) | 实践：本地 kind 集群跑第一个 Pod | kubectl 基础命令练习 |
| W4 | [Deployment 基础](32-发布/package/2026-07-02_18-29/corpus/peripheral/skills/training-lecturer/01-getting-started/02-deployment-basics.md) | [Service 基础](26-技能/05-网络/service/培训/01-service-basics.md) | 实践：部署一个 Nginx 并暴露服务 | [Ingress 基础](26-技能/05-网络/ingress/培训/01-ingress-basics.md) |

**阶段检查点**: 能独立在本地 kind 集群部署一个可访问的 Nginx 网站

### 第二阶段：核心期（Week 5-10）

| 周次 | 重点内容 |
|------|---------|
| W5 | [ConfigMap & Secret](04-configmap-secret.md) + 实践：配置外置化 |
| W6 | [Namespace & 资源配额](26-技能/07-安全/resource-quota/培训/01-namespace-resource-quota.md) + 多租户模拟 |
| W7 | [PV & PVC 基础](26-技能/06-存储/csi-storage/培训/01-pv-pvc-basics.md) + 实践：给 Nginx 加持久化日志 |
| W8 | [HPA 基础](26-技能/04-工作负载/hpa-vpa/培训/01-hpa-basics.md) + [健康检查](26-技能/04-工作负载/pod/培训/05-health-check.md) |
| W9 | [Job & CronJob](26-技能/04-工作负载/job-cronjob/培训/01-job-cronjob.md) + [DaemonSet](26-技能/04-工作负载/daemonset/培训/01-daemonset-basics.md) |
| W10 | [StatefulSet](26-技能/04-工作负载/statefulset/培训/01-statefulset-basics.md) + [调度基础](26-技能/02-控制面/scheduler/培训/01-scheduling-basics.md) |

**阶段检查点**: 能画出 K8s 核心对象的关系图（Pod → Deployment → Service → Ingress → PV）

### 第三阶段：实战期（Week 11-16）

| 周次 | 重点内容 |
|------|---------|
| W11-W12 | [端到端项目案例](beginner-guides/03-end-to-end-project.md)：从代码到生产 |
| W13 | Helm 入门：写第一个 Chart |
| W14 | 中间件部署：Redis + MySQL on K8s |
| W15 | 监控入门：Prometheus + Grafana 看集群指标 |
| W16 | GitOps 入门：ArgoCD 自动同步 |

**阶段检查点**: 拥有一个完整的"代码 → 镜像 → Helm → GitOps → 监控"流水线

### 第四阶段：认证期（Week 17-24）

| 周次 | 重点内容 |
|------|---------|
| W17-W20 | [CKA 备考指南](beginner-guides/04-cka-exam-prep-guide.md) 系统学习 + 模拟题 |
| W21-W22 | 第一次模拟考 + 查漏补缺 |
| W23-W24 | 第二次模拟考 + 预约真考 |

**阶段检查点**: 通过 CKA 认证（或模拟考达到 80 分以上）

---

## 路径二：密集速成版（28 天）

> **适用**: 有 1 个月全职时间（如离职学习、学生暑假）  
> **每日投入**: 6-8 小时  
> **直接采用**: [public-training/one-month/](public-training/one-month/) 现有课程

### 优化建议（在原计划基础上）

| 调整项 | 原内容 | 建议补充 |
|--------|--------|---------|
| Day 1-2 | Docker + Linux | 先读 [云原生演进故事](beginner-guides/01-cloud-native-evolution-story.md) 建立全局观 |
| Day 6 | K8s 集群搭建 | 同时提供 [本地 kind 方案](beginner-guides/02-local-lab-environment.md)，不绑定云厂商 |
| Week 4 | 企业级内容 | 增加 [CKA 模拟题](beginner-guides/04-cka-exam-prep-guide.md) 作为毕业考核 |
| 每日结束 | 无 | 增加"今日概念卡片"——用一张图总结当天核心知识点 |

---

## 路径三：开发转运维版（8 周）

> **你的优势**: 懂代码、懂 Git、有应用开发经验  
> **你的短板**: 可能缺少 Linux 运维、网络、系统调优背景  
> **目标岗位**: DevOps 工程师 / 平台工程师 / SRE

### 可跳过内容
- Git 基础（你已会）
- Docker 基础（如果你用过）
- 应用开发部分（你本身就是开发）

### 重点补强

| 周次 | 重点内容 | 为什么重要 |
|------|---------|-----------|
| W1 | Linux 网络命名空间 / cgroup | K8s 的底层基石 |
| W2 | K8s 核心对象 + 本地实验 | 快速建立直觉 |
| W3 | YAML 编写规范 + Helm 模板语法 | 开发者的日常工具 |
| W4 | CI/CD 流水线（Jenkins/GitLab CI/GitHub Actions）| 从开发到交付的桥梁 |
| W5 | 可观测性（Prometheus/Grafana/ELK）| 生产环境必备 |
| W6 | GitOps（ArgoCD/Flux）| 现代交付标准 |
| W7 | 安全基础（RBAC/NetworkPolicy/Secret）| 不能只做"能跑" |
| W8 | [端到端项目](beginner-guides/03-end-to-end-project.md) + 面试准备 | 把知识串成故事 |

---

## 路径四：运维转 SRE 版（6 周）

> **你的优势**: 懂 Linux、懂网络、有服务器管理经验  
> **你的短板**: 可能缺少云原生思维、自动化意识、开发能力  
> **目标岗位**: SRE / 平台工程师 / 云原生架构师

### 可跳过内容
- Linux 基础命令
- 网络基础（TCP/IP、DNS）
- 基础运维概念

### 重点补强

| 周次 | 重点内容 | 为什么重要 |
|------|---------|-----------|
| W1 | 容器原理（Namespace/Cgroups/UnionFS）| 理解 Docker 不是虚拟机 |
| W2 | K8s 架构与设计理念（声明式 API、控制器模式）| 运维思维 → 平台思维 |
| W3 | K8s 核心对象 + 调度/存储/网络深度 | 知其所以然 |
| W4 | Helm + Kustomize + GitOps | 自动化一切 |
| W5 | 可观测性体系 + SLO/SLI | SRE 核心能力 |
| W6 | 混沌工程 + 故障演练 + [CKA 备考](beginner-guides/04-cka-exam-prep-guide.md) | 证明你能 hold 住生产 |

---

## 路径五：CKA 考证路线（4 周）

> **目标**: 通过 Certified Kubernetes Administrator (CKA)  
> **前提**: 已完成 K8s 核心概念学习（或边学边考）  
> **考试费**: $395（约 ¥2800）

### 4 周冲刺计划

| 周次 | 主题 | 具体内容 |
|------|------|---------|
| W1 | 架构与安装 | 集群架构、etcd 备份恢复、kubeadm 安装升级、高可用 |
| W2 | 工作负载与调度 | Pod、Deployment、DaemonSet、Job、调度规则、资源限制 |
| W3 | 服务与存储 | Service、Ingress、PV/PVC、StorageClass、网络策略 |
| W4 | 排障与模拟 | 节点故障排查、网络故障排查、3 次全真模拟考 |

### CKA 核心资源

- [CKA 备考指南](beginner-guides/04-cka-exam-prep-guide.md) — 本库专属
- [官方课程](https://training.linuxfoundation.org/certification/certified-kubernetes-administrator-cka/) — Linux Foundation
- [Killer.sh](https://killer.sh/) — 模拟考试环境（最接近真实考试）
- [CKAD 备考](https://training.linuxfoundation.org/certification/certified-kubernetes-application-developer-ckad/) — 如果偏开发方向

### 考试技巧

1. **kubectl 自动补全**: 考试第一件事就是配置 `source <(kubectl completion bash)`
2. **文档搜索**: 善用官方文档搜索，不要死记硬背 YAML
3. **时间管理**: 17 道题，2 小时，平均每题 7 分钟，难题先标记跳过
4. **环境切换**: 注意每道题可能要求不同的集群上下文

---

## 路径六：面试突击版（2 周）

> **目标**: 快速通过 K8s 相关岗位面试  
> **适用**: 已有基础，需要快速复习和准备

### Week 1: 知识点速通

| 天数 | 主题 | 关键问题 |
|------|------|---------|
| D1 | Pod 与容器 | Pod 生命周期、Init 容器、Sidecar 模式 |
| D2 | 工作负载 | Deployment 滚动更新策略、StatefulSet 与 Deployment 区别 |
| D3 | 网络 | Service 类型、Ingress 原理、CNI 作用 |
| D4 | 存储 | PV/PVC/StorageClass 关系、emptyDir vs hostPath |
| D5 | 调度 | 节点亲和性、污点容忍、资源限制 vs 请求 |
| D6 | 安全 | RBAC 三要素、NetworkPolicy、Secret 安全 |
| D7 | 综合复习 | 画一张 K8s 全景图，能讲 10 分钟 |

### Week 2: 项目与模拟

| 天数 | 主题 |
|------|------|
| D8 | 准备一个"端到端项目"的故事（说清你在里面做了什么） |
| D9 | 高频面试题背诵（见 `beginner-guides/interview-prep/`） |
| D10 | 模拟面试 1：技术深度 |
| D11 | 模拟面试 2：项目复盘 |
| D12 | 模拟面试 3：场景设计（"如何设计一个高可用集群？"） |
| D13 | 查漏补缺 |
| D14 | 面试心态调整 |

---

## 配套资源索引

| 资源 | 路径 | 用途 |
|------|------|------|
| 概念类比词典 | [resources/analogy-dictionary.md](resources/analogy-dictionary.md) | 用生活化语言理解抽象概念 |
| 命令速查表 | [public-training/one-month/resources/commands-cheatsheet.md](public-training/one-month/resources/commands-cheatsheet.md) | 日常命令快速查找 |
| 知识图谱 | [public-training/one-month/resources/knowledge-map.md](public-training/one-month/resources/knowledge-map.md) | 全局知识导航 |
| 术语表 | [../../domain-17-system-foundation/topic-dictionary/k8s-glossary.md](../../domain-17-system-foundation/知识字典/k8s-glossary.md) | 不懂就查 |
| YAML 参考 | [../../domain-18-manifests-patterns/01-yaml-reference/](../../domain-18-manifests-patterns/YAML参考/) | 写 manifest 时参考 |
| 故障排查 | [../topic-skills/](../技能体系/) | 遇到问题来这找 |

---

## 学习纪律与技巧

### 1. 费曼技巧
每学完一个概念，尝试用一句话讲给"完全不懂技术的朋友"听。说不通 = 没真懂。

### 2. 输出倒逼输入
不要只看不练。建议开一个小博客/笔记，每学完一章就写一篇总结。

### 3. 建立错题本
实验失败的错误、面试答不上来的问题，全部记下来。这是你最宝贵的个性化学习资料。

### 4. 加入学习社群
- CNCF Slack #kubernetes-novice
- 掘金/知乎 K8s 话题
- 本地 K8s Meetup

### 5. 里程碑庆祝
- 🏅 本地跑通第一个 Pod → 奖励自己一杯咖啡
- 🏅 完成端到端项目 → 更新简历
- 🏅 通过 CKA → 把证书挂到 LinkedIn

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0 | 2026-05-21 | 初始版本：6 条学习路径 + CKA 备考 + 面试突击 |

---

**关联文档**:
- [[01-learning-gaps-analysis]] — 本路线图的缺口分析依据
- [[README]] — topic-learn 总入口
- [[skills/training-public/beginner-guides/01-cloud-native-evolution-story.md|01 cloud native evolution story]] — 推荐第一课
- [[skills/training-public/beginner-guides/02-local-lab-environment.md|02 local lab environment]] — 本地实验环境搭建
- [[skills/training-public/beginner-guides/03-end-to-end-project.md|03 end to end project]] — 端到端完整项目
- [[skills/training-public/beginner-guides/04-cka-exam-prep-guide.md|04 cka exam prep guide]] — CKA 备考指南


<!-- risk-assessed -->
