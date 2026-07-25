---
title: Kubernetes 生产运维实战训练营
description: 'title: Kubernetes 生产运维实战训练营'
summary: 'title: Kubernetes 生产运维实战训练营'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- scheduler
- prometheus
- grafana
- helm
- argocd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 生产运维实战训练营 是什么
- 如何 Kubernetes 生产运维实战训练营
trigger_keywords:
- Kubernetes
- 生产运维实战训练营
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- etcd-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<div align="center">

```yaml
---
title: Kubernetes 生产运维实战训练营
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "Kubernetes运维培训"
  - "28天训练营"
  - "SRE工程师培训"
  - "云原生运维课程"
trigger_keywords:
  - "K8s培训"
  - "28天课程"
  - "SRE训练营"
  - "云原生"
  - "生产运维"
  - "故障排查"
  - "监控告警"
  - "GitOps"
reading_level: beginner
audience:
  - sre工程师
  - devops工程师
  - 运维工程师
  - 开发工程师转型
estimated_read_time: 15min
related_domains:
  - 集群基础
  - 工作负载
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/quick-start
  - 生产运维/topic-learn/public-training/[[06-存储/README.md|README]]
id: PUBLIC-TRAINING-BOOT-001
topic: training
type: landing-page
tags: [training, bootcamp, 28-days, k8s, sre, devops, k8s-1.28-1.33]
---
```

# 🔥 Kubernetes 生产运维实战训练营 🔥

### ━━━━━━━━ 28 天，从入门到全栈运维 ━━━━━━━━

<br/>

> **"别再只是看文档了。28 天后，你就是团队里那个能扛事儿的人。"**

<br/>

---

## ⏰ 每晚 20:00 - 21:00 | 直播授课 + 实时答疑

---

</div>

<br/>

## 你是不是也有这些困扰？

- 😫 看了一堆 K8s 文档，遇到生产问题还是手足无措？
- 😫 被问"这个问题怎么排查"，只能支支吾吾？
- 😫 想系统学 K8s，但找不到一条清晰的学习路线？
- 😫 自学效率低，遇到问题没人问，卡住就放弃？

**这个训练营，就是为你准备的。**

<br/>

---

<div align="center">

## 🎯 训练营亮点

</div>

|  |  |
|:---:|:---:|
| **📚 两万篇知识库全开放** | **🎙️ 每晚直播，不是录播** |
| 背后是 kudig-database 海量沉淀 | 讲师在线授课，实时互动 |
| **💬 随时提问，当场解答** | **🛠️ 学完就能用，拒绝纸上谈兵** |
| 没有"下课再说"，问题当晚清零 | 每周交付一个生产级实践项目 |

<br/>

---

<div align="center">

## 📅 28 天极速成长路径

</div>

```
# 🟢 低风险：只读/信息收集，通常无副作用
    ┌──────────────────────────────────────────────────────────────────────┐
    │                                                                      │
    │   Week 1                Week 2               Week 3              Week 4    │
    │   地基建设              核心技术             运维作战            企业进阶   │
    │                                                                      │
    │   Docker 基础           控制平面精读         安全合规体系        企业监控    │
    │   Linux 基础            工作负载深潜         可观测性构建        GitOps     │
    │   K8s 架构全貌          网络栈精通           故障排查方法论      生产实践    │
    │   kubectl 实战          存储体系             平台运维实践        毕业项目    │
    │       │                    │                    │                   │       │
    │       ▼                    ▼                    ▼                   ▼       │
    │   🏗️ 搭建集群          📦 应用编排          📊 监控+排障       🚀 流水线   │
    │                                                                      │
    └──────────────────────────────────────────────────────────────────────┘
```
<br/>

---

<div align="center">

## 🗓️ 每周你将收获什么

</div>

### Week 1 · 地基建设期 `Day 1 ~ Day 7`

> **从容器到集群，一周搞定底层基础**

| 天数 | 主题 | 你将学会 |
|:---:|------|---------|
| Day 1-2 | Docker 容器全栈 | 镜像构建、网络模型、存储挂载、安全加固 |
| Day 3-4 | Linux 核心技能 | 进程管理、网络调试、文件系统、性能调优 |
| Day 5-6 | K8s 架构精读 | API Server、[[etcd|etcd]]、Scheduler 全组件拆解 |
| Day 7 | 综合实战 | **亲手搭建一个可运行的 K8s 集群** |

> **🎁 产出**: 拥有你自己的 K8s 集群 + 完整架构认知图

---

### Week 2 · 核心技术构建期 `Day 8 ~ Day 14`

> **深入 K8s 技术腹地，掌握核心能力**

| 天数 | 主题 | 你将学会 |
|:---:|------|---------|
| Day 8-9 | 控制平面深潜 | etcd Raft 协议、API 请求链、调度算法 |
| Day 10-11 | 工作负载实战 | Deployment / [[StatefulSet|StatefulSet]] / [[DaemonSet|DaemonSet]] / HPA |
| Day 12-13 | 网络栈精通 | CNI 原理、Service 四种类型、Ingress 路由 |
| Day 14 | 存储 + 综合实践 | PV/PVC/CSI 全链路 + 生产级应用编排 |

> **🎁 产出**: 一套完整的生产级多层应用 YAML 编排方案

---

### Week 3 · 运维作战能力期 `Day 15 ~ Day 21`

> **安全、监控、排障 —— 生产环境三把利剑**

| 天数 | 主题 | 你将学会 |
|:---:|------|---------|
| Day 15-16 | 安全合规体系 | RBAC 精细控制、Pod 安全标准、密钥管理 |
| Day 17-18 | 可观测性构建 | Prometheus + Grafana + Loki + 分布式追踪 |
| Day 19-20 | 故障排查实战 | **FTA 故障树分析 + FEBM 取证循证** (核心方法论!) |
| Day 21 | 平台运维实践 | 集群升级、资源管理、自动化运维 |

> **🎁 产出**: 监控告警大盘 + 一本你自己的故障排查手册

---

### Week 4 · 企业级进阶期 `Day 22 ~ Day 28`

> **对标大厂 SRE 标准，完成最后跃迁**

| 天数 | 主题 | 你将学会 |
|:---:|------|---------|
| Day 22-23 | 企业监控 + GitOps | Thanos 跨集群监控、ArgoCD 持续部署 |
| Day 24-25 | 安全合规 + 生产实践 | 云原生安全体系、变更管理、事故响应 SOP |
| Day 26-27 | 方法论深化 + 扩展 | FTA/FEBM 生产落地、Helm/Operator 生态 |
| Day 28 | **毕业项目答辩** | 综合实践 + 成果展示 + 职业规划建议 |

> **🎁 产出**: GitOps 全自动部署流水线 + 生产事故响应 Playbook

<br/>

---

<div align="center">

## 💪 5 大实战项目，学完直接写进简历

</div>

| # | 项目名称 | 对应能力 |
|:-:|----------|---------|
| P1 | 从零搭建 K8s 集群 | 集群部署与架构理解 |
| P2 | 生产级应用全栈编排 | 工作负载 + 网络 + 存储 |
| P3 | 可观测性体系 + 故障演练 | 监控告警 + 排障能力 |
| P4 | GitOps 自动化流水线 | CI/CD + 变更管理 |
| P5 | **毕业综合实践** | **全栈运维能力证明** |

<br/>

---

<div align="center">

## 🏆 学习方式：不只是听课

</div>

```
   ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐
   │  20:00  │────▶│  20:30  │────▶│  20:45  │────▶│  21:00  │
   │         │     │         │     │         │     │         │
   │ 精讲核心 │     │ 实操演示 │     │ 自由提问 │     │ 今日总结 │
   │ 知识点   │     │ 跟做练习 │     │ 实时答疑 │     │ 明日预告 │
   └─────────┘     └─────────┘     └─────────┘     └─────────┘
```

- **费曼学习法**: 每节课后用自己的话复述，真正内化
- **间隔重复**: 周末回顾本周核心概念，防止遗忘
- **主动回忆**: 先想再看，刻意训练思维链路
- **实践优先**: 理论 30% + 动手 70%，拒绝空谈

<br/>

---

<div align="center">

## 🤔 适合谁来学？

</div>

| 适合你 | 不适合你 |
|--------|---------|
| 想系统学习 K8s 运维的开发/运维工程师 | 只想看看不想动手的 |
| 想往 SRE/DevOps 方向转型的同学 | 期望一天速成的 |
| 生产环境遇到问题想提升排障能力的 | 不愿意花 1 小时/天的 |
| 准备 CKA/CKS 认证考试的 | — |
| 想在简历上加一笔"生产级 K8s 经验"的 | — |

<br/>

---

<div align="center">

## 📊 你将获得的能力对比

</div>

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
能力维度              学习前                        28天后
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
集群管理        ❌ 不会部署              ✅ 独立搭建生产级集群
应用编排        ❌ 只会 copy YAML        ✅ 手写多层应用完整编排
网络排障        ❌ ping 不通就懵了       ✅ CNI/Service/DNS 逐层定位
监控告警        ❌ 没有监控体系          ✅ Prometheus+Grafana 全栈
故障排查        ❌ 靠猜靠运气            ✅ FTA/FEBM 系统化排障
安全合规        ❌ 什么是 RBAC?          ✅ 完整安全策略落地
GitOps          ❌ 手动 kubectl apply    ✅ ArgoCD 全自动部署
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
<br/>

---

<div align="center">

## 🌟 往期学员反馈

</div>

> *"之前看了三个月文档，不如这 28 天跟着练一遍。现在线上出问题，我是组里第一个能定位问题的人。"*

> *"每天一小时不多不少，关键是有人答疑，卡住的点当晚就解决了。"*

> *"毕业项目写进简历后，面试通过率直接翻倍。"*

<br/>

---

<div align="center">

## 🚀 现在就加入

<br/>

### 📅 每晚 20:00 - 21:00 准时开讲

### 💬 课上交流 + 实时答疑 + 课后社群

### 📖 668+ 篇知识库全程辅助

<br/>

---

**28 天不长，但足以改变你的技术轨迹。**

**别等"有空再学"，最好的时间就是今天的 20:00。**

---

<br/>

### ⬇️ 扫码 / 点击链接，立即报名 ⬇️

*名额有限，满员即止*

</div>

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
