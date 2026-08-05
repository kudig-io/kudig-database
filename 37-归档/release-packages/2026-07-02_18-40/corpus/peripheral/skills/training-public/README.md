---
title: K8s 学习与培训体系
description: Kubernetes 全链路学习平台，涵盖基础概念讲解、系统培训路径、实操练习、On-Call 场景与故障排查决策树
summary: Kubernetes 全链路学习平台，涵盖基础概念讲解、系统培训路径、实操练习、On-Call 场景与故障排查决策树
category: learning
tags:
- k8s
- training
- learning-path
- hands-on
- lecturer
- oncall
- troubleshooting
- hpa
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
- 培训师
- SRE / Ops 工程师
estimated_read_time: 5min
intent_queries:
- K8s 学习路径有哪些
- Kubernetes 培训体系介绍
- 如何系统学习 K8s
trigger_keywords:
- K8s 学习
- 培训体系
- 学习路径
- topic-learn
- 运维培训
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 学习与培训体系

> **版本**: v2.0
> **创建日期**: 2026-05-15
> **更新日期**: 2026-05-21
> **定位**: [[Kubernetes|Kubernetes]] 全链路学习平台
> **覆盖**: 基础概念 → 系统培训 → 实操练习 → On-Call 场景 → 故障排查

---

## 概述

本主题提供 Kubernetes 从入门到精通的完整学习路径，整合了数字人讲师概念讲解、系统培训体系、实操练习手册、On-Call 快速问答与故障排查决策树，形成闭环学习生态。

### 内容架构

```
domain-11-production-operations/topic-learn/
├── 00-learning-gaps-analysis.md      # 内容缺口分析（本库自检报告）
├── 00-beginner-learning-roadmap.md   # 多路径小白学习路线图
├── fundamentals/          # 15 课基础概念讲解（数字人讲师课件）
├── beginner-guides/       # 🆕 小白补充教程（零成本实验/认证/项目）
│   ├── 01-cloud-native-evolution-story.md  # 云原生演进故事
│   ├── 02-local-lab-environment.md         # 本地零成本实验环境
│   ├── 03-end-to-end-project.md            # 端到端完整项目案例
│   └── 04-cka-exam-prep-guide.md           # CKA 认证备考指南
├── quick-start/           # 快速入门指南（新人上手）
├── public-training/       # 公开培训资源（通用版本）
│   ├── one-month/         # 一个月生产运维训练营
│   └── week-2-4/          # 四周实操培训（Day 8-28）
├── inner-training/        # 内部培训材料（阿里云 ACK 版本）
├── oncall-qa/             # On-Call 快速问答（20 个场景）
├── troubleshooting/       # 故障排查决策树（Mermaid 可视化）
└── resources/             # 学习辅助资源
    ├── lecturer-persona.md    # 讲师角色设定
    ├── analogy-dictionary.md  # 概念类比词典
    ├── commands-cheatsheet.md # 命令速查表
    ├── knowledge-map.md       # 知识图谱
    └── reading-sequence.md    # 阅读顺序
```

---

## 学习路径推荐

### 路径一：零基础系统学习（推荐）

```
Step 0: 建立认知（新增 🆕）
  ├── 00-learning-gaps-analysis.md        # 了解本库内容结构
  └── beginner-guides/01-cloud-native-evolution-story.md  # 为什么需要 K8s

Step 1: 快速上手
  ├── quick-start/                        # 1-2 天完成新人 checklist
  └── beginner-guides/02-local-lab-environment.md         # 本地搭建实验环境

Step 2: 基础概念
  └── fundamentals/         # 15 课，每课 15-25 分钟
      ├── 01-what-is-kubernetes.md
      ├── 02-pod-basics.md
      ├── 03-deployment-basics.md
      ├── 04-service-basics.md
      ├── 05-ingress-basics.md
      ├── 06-configmap-secret.md
      ├── 07-namespace-resource-quota.md
      ├── 08-pv-pvc-basics.md
      ├── 09-hpa-basics.md
      ├── 10-health-check.md
      ├── 11-job-cronjob.md
      ├── 12-common-problems.md
      ├── 13-daemonset-basics.md
      ├── 14-statefulset-basics.md
      └── 15-scheduling-basics.md

Step 3: 系统培训（四选一）
  ├── public-training/one-month/          # 通用一个月训练营
  ├── public-training/week-2-4/           # 四周实操培训
  └── beginner-guides/03-end-to-end-project.md            # 端到端项目实战

Step 4: 认证与职业（新增 🆕）
  ├── beginner-guides/04-cka-exam-prep-guide.md           # CKA 备考指南
  └── 00-beginner-learning-roadmap.md     # 职业路径与面试准备

Step 5: On-Call 实战
  ├── oncall-qa/oncall-quick-qa.md                    # 20 个快速问答
  └── troubleshooting/decision-tree-mermaid.md        # 10 个决策树
```

### 路径二：有经验工程师进阶

```
Step 1: 查漏补缺
  └── fundamentals/         # 快速浏览不熟悉的基础概念

Step 2: 深度培训
  └── inner-training/       # 阿里云 ACK 深度培训（如适用）
      或 public-training/one-month/week-3-4  # 进阶内容

Step 3: 故障排查专项
  ├── oncall-qa/oncall-quick-qa.md
  └── troubleshooting/decision-tree-mermaid.md
```

### 路径三：培训师/数字人开发者

```
Step 1: 讲师设定
  └── resources/lecturer-persona.md    # 角色设定与场景规范

Step 2: 类比词典
  └── resources/analogy-dictionary.md  # 生活化类比素材

Step 3: 课件内容
  └── fundamentals/                    # 15 课完整课件

Step 4: 场景化 Q&A
  ├── oncall-qa/oncall-quick-qa.md
  └── troubleshooting/decision-tree-mermaid.md
```

---

## 内容索引

| 目录 | 说明 | 文件数 | 目标受众 |
|------|------|--------|---------|
| `00-learning-gaps-analysis.md` | 内容缺口分析报告 | 1 | 内容建设者 |
| `00-beginner-learning-roadmap.md` | 多路径学习路线图 | 1 | 所有学习者 |
| [beginner-guides/](beginner-guides/) | 🆕 小白补充教程 | 4 | 零基础/自学者 |
| [fundamentals/](fundamentals/) | 15 课基础概念讲解 | 15 | 零基础学员 |
| [quick-start/](quick-start/) | 快速入门指南 | 5 | 新入职工程师 |
| [public-training/](public-training/) | 公开培训资源 | ~60 | 通用场景学员 |
| [inner-training/](inner-training/) | 内部培训材料 | ~35 | 阿里云 ACK 运维 |
| [oncall-qa/](oncall-qa/) | On-Call 快速问答 | 1 | 值班工程师 |
| [troubleshooting/](troubleshooting/) | 故障排查决策树 | 1 | 故障处理人员 |
| [resources/](resources/) | 学习辅助资源 | 5+ | 所有学习者 |

---

## 核心能力覆盖

| 能力域 | 对应内容 | 目标等级 |
|--------|---------|---------|
| 集群生命周期管理 | inner-training/week-1 | L3 |
| 安全与权限管理 | fundamentals/06-07, inner-training/week-2 | L3 |
| 监控与告警 | fundamentals/10, inner-training/week-2 | L3 |
| 节点与工作负载 | fundamentals/02-03-11-13-14, inner-training/week-3 | L3 |
| 网络与存储 | fundamentals/04-05-08, inner-training/week-4 | L3 |
| 故障排查 | oncall-qa/, troubleshooting/ | L2-L3 |
| 弹性伸缩 | fundamentals/09 | L2-L3 |

---

## 配套资源速查

| 资源 | 路径 | 用途 |
|------|------|------|
| 讲师角色设定 | [resources/lecturer-persona.md](resources/lecturer-persona.md) | 数字人开发参考 |
| 概念类比词典 | [resources/analogy-dictionary.md](resources/analogy-dictionary.md) | 培训讲解素材 |
| 命令速查表 | [public-training/one-month/resources/commands-cheatsheet.md](public-training/one-month/resources/commands-cheatsheet.md) | 日常运维速查 |
| 知识图谱 | [public-training/one-month/resources/knowledge-map.md](public-training/one-month/resources/knowledge-map.md) | 全局知识导航 |
| 考核评估 | [../domain-10-troubleshooting-diagnostics/topic-skills/assessment/]( ../domain-10-troubleshooting-diagnostics/技能体系/assessment/) | 技能自测工具 |
| 术语表 | [../domain-17-system-foundation/topic-dictionary/k8s-glossary.md](../domain-17-system-foundation/知识字典/k8s-glossary.md) | 术语查询 |
| 故障排查手册 | [../domain-10-troubleshooting-diagnostics/]( ../domain-10-troubleshooting-diagnostics/) | 深度故障排查 |

---

## 学习建议

### 新手（无 K8s 经验）

1. 按顺序完成 fundamentals/ 15 课
2. 每课结束后用自己的语言复述核心概念（费曼技巧）
3. 进入 public-training/one-month/ 进行系统训练
4. 每周 checkpoint 达到 60% 以上再继续
5. 综合项目独立完成后再看参考答案

### 有经验（6 个月以上 K8s 经验）

1. 快速浏览 fundamentals/ 查漏补缺
2. 重点学习 public-training/one-month/week-3-4 进阶内容
3. 关注 oncall-qa/ 和 troubleshooting/ 的实战场景
4. 完成 inner-training/ 中不熟悉的领域

### 阿里云 ACK 用户

1. 优先使用 inner-training/ 路径
2. Day 1-7 涵盖 ACK/ACR 特有内容
3. 关注 aliyun CLI 和 API 操作
4. 注意 ACK 与开源 K8s 的差异

---

## 版本历史

| 版本 | 日期 | 变更内容 |
|------|------|---------|
| v1.0 | 2026-05-15 | 初始版本：inner-training + public-training + quick-start |
| v2.0 | 2026-05-21 | 合并 topic-k8s-lecturer，新增 fundamentals/、oncall-qa/、troubleshooting/、resources/，形成完整学习生态 |
| v2.1 | 2026-05-21 | 新增 beginner-guides/（云原生故事/本地实验/CKA备考/端到端项目）、缺口分析、多路径路线图，补齐小白入门最后一公里 |

---

**关联文档**:
- MOC.md](MOC.md) — 本专题完整文档导航
- [00-learning-gaps-analysis.md](32-发布/package/2026-07-02_18-40/corpus/supporting/skills/training-public/01-learning-gaps-analysis.md) — 内容缺口分析
- [00-beginner-learning-roadmap.md](00-beginner-learning-roadmap.md) — 多路径学习路线图
- [beginner-guides/](beginner-guides/) — 🆕 小白补充教程
- [../domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/技能体系/) — 18 个 GA Skill（深度技术细节）
- [../domain-17-system-foundation/topic-dictionary/k8s-glossary.md](../domain-17-system-foundation/知识字典/k8s-glossary.md) — K8s 术语表
- [../domain-10-troubleshooting-diagnostics/](../domain-10-troubleshooting-diagnostics/) — 故障排查深度文档

## Related

- [[kudig-prompts-catalog]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


## 参见

- [[skills/training-lecturer/README.md|讲师版]]


<!-- risk-assessed -->
