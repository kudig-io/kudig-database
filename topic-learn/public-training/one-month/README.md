# Kubernetes 生产运维 1 个月学习计划

> **目标人群**: 入门级 -> 全栈运维 | **投入**: 4+ 小时/天 | **知识库**: kudig-database (668+ 篇)

---

## 快速导航

| 周次 | 主题 | 目录 |
|------|------|------|
| Week 1 | 地基建设期 | [week-1-foundation/](./week-1-foundation/) |
| Week 2 | 核心技术构建期 | [week-2-core-tech/](./week-2-core-tech/) |
| Week 3 | 运维作战能力期 | [week-3-operations/](./week-3-operations/) |
| Week 4 | 企业级进阶期 | [week-4-enterprise/](./week-4-enterprise/) |
| - | 实践项目 | [projects/](./projects/) |
| - | 补充资源 | [resources/](./resources/) |

---

## 整体学习路径

```
Week 1: 地基建设期     Week 2: 核心技术构建期    Week 3: 运维作战能力期    Week 4: 企业级进阶期
├─ Docker 基础        ├─ 控制平面精读           ├─ 安全合规体系           ├─ 企业监控/日志平台
├─ Linux 基础         ├─ 工作负载深潜           ├─ 可观测性构建           ├─ GitOps & CI/CD
├─ K8s 架构全貌       ├─ 网络栈精通             ├─ 故障排查方法论         ├─ FTA/FEBM 专题
└─ kubectl 实战       └─ 存储体系               └─ 平台运维实践           └─ 生产最佳实践综合
    |                     |                         |                         |
    v                     v                         v                         v
 产出: K8s 集群搭建    产出: 生产级应用编排      产出: 监控大盘+排障手册   产出: GitOps 流水线
```

---

## 知识依赖关系

```
Domain13(Docker) ─┐
Domain14(Linux)  ─┼─> Domain1(架构) ─> Domain3(控制平面) ─> Domain9(平台运维)
Domain15(网络基础)┘       │                 │                     │
                          v                 v                     v
                     Domain4(工作负载)   Domain5(网络)      Domain12(故障排查)
                          │              Domain6(存储)           │
                          v                 │                    v
                     Domain7(安全)  <───────┘             topic-fta/febm
                     Domain8(可观测性)
                          │
                          v
                 Domain18-33(企业级专题)
```

---

## 学习方法论

### 1. 费曼学习法 (每日)
每天学完一个模块后，用自己的语言向"虚拟初学者"复述，检测理解漏洞。

### 2. 间隔重复 (每周)
- 每周第一天用 15 分钟回顾上周关键概念
- 每周末复习本周 10 个核心术语

### 3. 主动回忆 (每节)
先合上文档，尝试回答: "这个组件做什么？它和哪些组件交互？出故障了怎么排查？"

### 4. 实践优先原则
理论文档读完后，立刻动手复现。每天 4 小时中: 理论 <= 1.5h，实践 >= 2.5h

### 5. 结构化记录
每个 Domain 学完后，产出一张思维导图或 README 摘要，形成个人知识图谱。

---

## 每周目标与产出

| 周次 | 核心产出 | 完成评估标准 |
|------|----------|--------------|
| Week 1 | K8s 集群环境 + 架构图 | 能独立搭建集群，能解释所有组件职责 |
| Week 2 | 生产级应用 YAML 编排 | 能完整部署含网络/存储的多层应用 |
| Week 3 | 监控告警体系 + 排障手册 | 能独立构建监控栈，30分钟内定位故障 |
| Week 4 | GitOps 流水线 + Playbook | 任何变更都通过 Git PR 触发部署，有文档化 SOP |

---

## 实践项目清单

| # | 项目名称 | 周 | 详情 |
|---|----------|---|------|
| P1 | 从零搭建 K8s 集群 | Week 1 | [p1-k8s-cluster-setup.md](./projects/p1-k8s-cluster-setup.md) |
| P2 | 生产级应用全栈编排 | Week 2 | [p2-production-app-orchestration.md](./projects/p2-production-app-orchestration.md) |
| P3 | 可观测性体系 + 故障演练 | Week 3 | [p3-observability-fault-drill.md](./projects/p3-observability-fault-drill.md) |
| P4 | GitOps 流水线 | Week 4 | [p4-gitops-pipeline.md](./projects/p4-gitops-pipeline.md) |
| P5 | 毕业综合实践项目 | Week 4 | [p5-graduation-project.md](./projects/p5-graduation-project.md) |

---

## 关键文件索引

### 核心架构文档
- `../domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md`
- `../domain-1-architecture-fundamentals/02-core-components-deep-dive.md`

### 故障排查体系
- `../topic-fta/23-fta-production-quick-start.md`
- `../topic-febm/08-febm-production-quick-start.md`
- `../domain-12-troubleshooting/` (42篇)

### 生产运维实践
- `../domain-18-production-operations/23-incident-response-handling.md`
- `../domain-18-production-operations/22-change-management-process.md`

### 速查手册
- `../topic-cheat-sheet/k8s.md`
- `../domain-13-docker/99-docker-commands-reference.md`
- `../domain-14-linux/99-linux-commands-reference.md`

---

## 如何使用本学习计划

1. **按周顺序学习**: 从 Week 1 开始，按 Day 1 -> Day 7 顺序推进
2. **每日任务**: 每个 day 文件包含理论阅读、实践任务、费曼复述三个环节
3. **周末检验**: 每周末完成 `checkpoint.md` 中的自测题
4. **项目驱动**: 每周末完成一个实践项目，巩固所学知识
5. **记录成长**: 在 `resources/knowledge-map.md` 中记录个人知识图谱

开始你的 Kubernetes 全栈运维之旅吧!
