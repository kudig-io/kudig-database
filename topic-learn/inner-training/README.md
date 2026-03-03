# ACK/ACR/K8S 内部培训 1 个月学习计划

> **目标人群**: 内部运维工程师、技术支持人员 | **投入**: 4+ 小时/天 | **知识库**: kudig-database (668+ 篇)

---

## 快速导航

| 周次 | 主题 | 目录 |
|------|------|------|
| Week 1 | ACK/ACR 基础与集群生命周期 | [week-1-ack-acr-lifecycle/](./week-1-ack-acr-lifecycle/) |
| Week 2 | 安全认证与监控运维 | [week-2-security-monitoring/](./week-2-security-monitoring/) |
| Week 3 | 节点与工作负载管理 | [week-3-node-workload/](./week-3-node-workload/) |
| Week 4 | 网络与存储 | [week-4-network-storage/](./week-4-network-storage/) |
| - | 实践项目 | [projects/](./projects/) |
| - | 补充资源 | [resources/](./resources/) |

---

## 整体学习路径

```
Week 1: ACK/ACR 基础       Week 2: 安全认证与监控    Week 3: 节点与工作负载    Week 4: 网络与存储
├─ ACK/ACR 管控 SR        ├─ RBAC 权限配置          ├─ Node 节点管理          ├─ Service 基础
├─ ACK SDK & API          ├─ RAM 账号集成           ├─ 节点池管理             ├─ Ingress 路由
├─ ACK/ACR 控制台         ├─ 漏洞 & 风险防范        ├─ Pod 容器组管理         ├─ Terway/Flannel CNI
├─ 集群创建               ├─ 集群审计               ├─ K8S 组件运维           ├─ 存储卷管理
├─ 集群删除               ├─ 集群监控                                        ├─ 综合复习
└─ 集群升级/证书           └─ 配额 & License
    |                         |                         |                         |
    v                         v                         v                         v
 产出: 集群全生命周期      产出: 安全体系+监控基础   产出: 节点池+Pod运维能力  产出: 网络+存储实操能力
```

---

## 知识依赖关系

```
ACK/ACR 管控层 ──> ACK SDK/API ──> 控制台操作
       │
       v
集群生命周期 (创建/删除/升级/证书)
       │
       v
安全认证 (RBAC + RAM) ──> 漏洞 & 风险 ──> 审计 & 监控
       │
       v
节点管理 (Node + NodePool) ──> 工作负载 (Pod + 组件)
       │
       v
网络 (Service + Ingress + CNI) ──> 存储 (PV/PVC)
```

---

## 学习方法论

### 1. 费曼学习法 (每日)
每天学完一个模块后，用自己的语言向"虚拟初学者"复述，检测理解漏洞。

### 2. 间隔重复 (每周)
- 每周第一天用 15 分钟回顾上周关键概念
- 每周末复习本周 10 个核心术语

### 3. 主动回忆 (每节)
先合上文档，尝试回答: "这个功能做什么？它和哪些服务交互？出故障了怎么排查？"

### 4. 实践优先原则
理论文档读完后，立刻动手复现。每天 4 小时中: 理论 <= 1.5h，实践 >= 2.5h

### 5. 结构化记录
每个主题学完后，产出一张思维导图或笔记摘要，形成个人知识图谱。

---

## 每周目标与产出

| 周次 | 核心产出 | 完成评估标准 |
|------|----------|--------------|
| Week 1 | 独立完成集群全生命周期操作 | 能通过控制台/SDK/API 三种方式完成集群创建、升级、删除 |
| Week 2 | 安全体系配置 + 监控基础搭建 | 能配置 RBAC 和 RAM 集成，搭建基础监控告警 |
| Week 3 | 节点池运维 + Pod 问题排查 | 能管理节点池扩缩容，独立排查 Pod 常见问题 |
| Week 4 | 网络和存储配置 + 综合实操 | 能配置 Service/Ingress/CNI，管理存储卷 |

---

## 实践项目清单

| # | 项目名称 | 周 | 详情 |
|---|----------|---|------|
| P1 | ACK 集群全生命周期管理 | Week 1 | [p1-ack-cluster-lifecycle.md](./projects/p1-ack-cluster-lifecycle.md) |
| P2 | 安全认证与监控体系搭建 | Week 2 | [p2-security-monitoring-setup.md](./projects/p2-security-monitoring-setup.md) |
| P3 | 节点与工作负载运维实战 | Week 3 | [p3-node-workload-management.md](./projects/p3-node-workload-management.md) |
| P4 | 网络与存储综合实践 | Week 4 | [p4-network-storage-practice.md](./projects/p4-network-storage-practice.md) |
| P5 | 毕业综合实践项目 | Week 4 | [p5-graduation-project.md](./projects/p5-graduation-project.md) |

---

## 关键文件索引

### ACK/ACR 核心文档
- `../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
- `../../domain-17-cloud-provider/04-alicloud-ack/service-ack-practical-guide.md`
- `../../domain-17-cloud-provider/04-alicloud-ack/243-ack-ram-authorization.md`

### 集群架构与组件
- `../../domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md`
- `../../domain-1-architecture-fundamentals/02-core-components-deep-dive.md`

### 故障排查体系
- `../../domain-12-troubleshooting/` (42篇)
- `../../topic-structural-trouble-shooting/README.md`

### 速查手册
- `../../topic-cheat-sheet/k8s.md`
- `../../domain-13-docker/99-docker-commands-reference.md`
- `../../domain-14-linux/99-linux-commands-reference.md`

---

## 如何使用本学习计划

1. **按周顺序学习**: 从 Week 1 开始，按 Day 1 -> Day 7 顺序推进
2. **每日任务**: 每个 day 文件包含理论阅读、实践任务、费曼复述三个环节
3. **周末检验**: 每周末完成 `checkpoint.md` 中的自测题
4. **项目驱动**: 每周末完成一个实践项目，巩固所学知识
5. **记录成长**: 在 `resources/knowledge-map.md` 中记录个人知识图谱

开始你的 ACK/ACR/K8S 内部培训之旅吧!
