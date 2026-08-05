---
title: 自建 Kubernetes 迁移至阿里云 ACK 生产实践指南 [migration]
description: '# 自建 Kubernetes 迁移至阿里云 ACK 生产实践指南'
summary: 'aliyun ram ListPoliciesForUser --UserName <your-user>'
category: migration
tags:
- k8s
- migration
- modernization
- etcd
- helm
- ceph
- redis
- mysql
- kafka
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 自建 Kubernetes 迁移至阿里云 ACK 生产实践指南 是什么
- 如何 自建 Kubernetes 迁移至阿里云 ACK 生产实践指南
trigger_keywords:
- 自建
- Kubernetes
- 迁移至阿里云
- ACK
- 生产实践指南
- migration
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 自建 [[Kubernetes|Kubernetes]] 迁移至阿里云 ACK 生产实践指南

> **适用版本**: Kubernetes v1.24 - v1.32 → 阿里云 ACK | **文档类型**: 端到端迁移专题 | **最后更新**: 2026-03 | **关键词**: 自建 [[domain-17-system-foundation/速查卡/k8s.md|[[Kubernetes 生产环境速查卡|k8s]]]], kubeadm, ACK, 迁移, Velero, Terway, 零停机, 灰度切流

---

## 概述

本专题提供从**云下自建 Kubernetes 集群（kubeadm / 二进制 / RKE 等）迁移至阿里云 ACK 托管版**的完整生产级指南。覆盖迁移评估、目标集群设计、工作负载搬迁、存储/网络/有状态服务迁移、可观测性与安全体系重建、灰度切流与验收、旧集群退役全流程。

**设计原则**:
- **生产级标准**: 所有方案均经过真实生产环境验证，包含完整回滚方案
- **零停机优先**: 默认采用双集群并行 + 灰度切流策略，确保业务连续性
- **可操作性**: 每一步都附带完整命令、YAML、预期输出和验证方法
- **问题预案**: 每个阶段均包含故障排查和回滚步骤

---

## 迁移决策树

```
Q1: 你的自建集群是什么类型？
│
├── kubeadm 搭建              → 标准迁移流程（本指南主线）
├── 二进制部署                → 先梳理组件配置，再走标准流程
├── RKE / Rancher 管理        → 参考 03 应用迁移，额外处理 Rancher CRD
└── k3s / MicroK8s            → 轻量迁移，重点关注存储和网络差异
│
Q2: 业务中断容忍度？
│
├── 零停机（金融/电商）       → 双集群并行 + DNS 灰度切流（推荐）
├── 可接受短暂维护窗口        → 蓝绿切换（维护窗口 < 30min）
└── 可接受较长停机            → 直接迁移（停机 1-4h）
│
Q3: 有状态服务复杂度？
│
├── 纯无状态                  → 03 应用迁移 → 05 网络切流 → 完成
├── 有 MySQL/Redis/ES 等      → 额外参考 06 有状态迁移
└── 有自建存储（Ceph/GlusterFS）→ 额外参考 04 存储迁移
```

---

## 迁移阶段总览

```
Phase 0          Phase 1          Phase 2          Phase 3          Phase 4
迁移评估         目标集群搭建      工作负载迁移      灰度切流          旧集群退役
(1-2 周)         (1-2 周)          (2-4 周)         (1-2 周)          (1 周)
─────────────────────────────────────────────────────────────────────────────
│ 集群现状分析 │ ACK 集群设计   │ 无状态应用迁移 │ DNS 灰度切流   │ 流量确认归零 │
│ 兼容性评估   │ VPC/网络规划   │ ConfigMap/Secret│ 监控对比验证   │ 备份快照     │
│ 风险矩阵     │ 节点池配置     │ 存储/数据迁移  │ 全量切换       │ 资源释放     │
│ 迁移计划     │ 监控/日志基线  │ 有状态服务迁移 │ 回滚演练       │ 文档归档     │
─────────────────────────────────────────────────────────────────────────────
```

---

## 文档目录

| 序号 | 文档 | 内容概要 | 适用角色 | 阅读耗时 |
|:---:|------|---------|---------|---------|
| 01 | [迁移评估与规划](./01-migration-assessment-planning.md) | 集群现状分析、兼容性评估、风险矩阵、迁移计划模板 | 架构师、项目经理 | 30min |
| 02 | [ACK 目标集群设计](./02-ack-target-cluster-design.md) | 集群类型选择、VPC/CIDR 规划、节点池设计、Addon 配置 | 架构师、运维工程师 | 40min |
| 03 | [应用工作负载迁移](./03-application-workload-migration.md) | Deployment/Service/Ingress/ConfigMap 导出适配、ACK 注解映射 | 运维工程师、开发 | 45min |
| 04 | [存储与数据迁移](./04-storage-data-migration.md) | PV/PVC 迁移、CSI 适配、数据同步（Velero/rsync）、阿里云存储对接 | 运维工程师、DBA | 40min |
| 05 | [网络迁移与流量切换](./05-network-migration-traffic-cutover.md) | CNI 差异处理、[[Ingress|Ingress]]/Gateway 迁移、DNS 灰度切流、SLB/NLB/ALB 适配 | 运维工程师、网络工程师 | 45min |
| 06 | [有状态服务迁移](./06-stateful-services-migration.md) | MySQL/Redis/ES/Kafka/etcd 迁移策略、数据一致性校验 | DBA、运维工程师 | 50min |
| 07 | [可观测性与安全迁移](./07-01-observability-architecture-overview-security-migration.md) | 监控/日志/链路追踪迁移、RBAC/证书/NetworkPolicy 重建 | SRE、安全工程师 | 35min |
| 08 | [验收、切换与旧集群退役](./08-validation-cutover-decommission.md) | 功能/性能验证清单、全量切换 SOP、旧集群安全退役流程 | 全团队 | 30min |
| 09 | [迁移工具链参考](./09-migration-toolchain.md) | Velero、kubectl-neat、yq、迁移脚本集、自动化流水线 | 运维工程师 | 25min |
| 10 | [生产迁移实战案例](./10-real-world-case-study.md) | 完整案例复盘：50+ 微服务、3 套有状态中间件、零停机迁移全记录 | 全团队 | 40min |

---

## 前置条件

### 人员与权限

| 角色 | 职责 | 所需权限 |
|------|------|---------|
| 迁移项目负责人 | 整体计划、风险管控、进度跟踪 | 自建集群 cluster-admin、阿里云 RAM 管理员 |
| 运维工程师 | 集群搭建、资源迁移、流量切换 | 自建集群 cluster-admin、ACK FullAccess |
| DBA | 有状态服务数据迁移与校验 | 数据库 root/admin、RDS/Redis 管理权限 |
| 开发负责人 | 应用适配验证、功能回归测试 | 业务 Namespace 读写 |
| 网络工程师 | DNS 切换、负载均衡配置 | DNS 管理、SLB/NLB/ALB 管理 |

### 工具准备

| 工具 | 最低版本 | 用途 | 安装方式 |
|------|---------|------|---------|
| kubectl | 与集群版本 ±1 | K8s 资源操作 | `brew install kubectl` |
| aliyun CLI | 3.0+ | 阿里云 API 操作 | `brew install aliyun-cli` |
| helm | 3.10+ | Chart 部署 | `brew install helm` |
| velero | 1.12+ | 集群资源备份迁移 | `brew install velero` |
| yq | 4.x | YAML 处理 | `brew install yq` |
| kubectl-neat | latest | 清理导出 YAML | `kubectl krew install neat` |
| jq | 1.6+ | JSON 处理 | `brew install jq` |
| rsync | 3.x | 数据同步 | 系统自带 |

### 阿里云资源准备

```bash
# 确认 aliyun CLI 已配置
aliyun configure list

# 确认 RAM 权限
aliyun ram ListPoliciesForUser --UserName <your-user>

# 确认区域与可用区
aliyun ecs DescribeRegions --output cols=RegionId,LocalName
aliyun ecs DescribeZones --RegionId cn-hangzhou --output cols=ZoneId,LocalName
```

---

## 关联文档索引

| 类别 | 文档路径 | 说明 |
|------|---------|------|
| ACK 服务概览 | `domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md` | ACK 产品架构与最佳实践 |
| ACK Service 实战 | `domain-12-cloud-providers/04-alicloud-ack/service-ack-practical-guide.md` | Service 类型与 SLB 集成 |
| ACK VPC 网络 | `domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md` | VPC/CIDR 规划详解 |
| ACK ECS 计算 | `domain-12-cloud-providers/04-alicloud-ack/240-ack-ecs-compute.md` | 节点规格选型 |
| 升级迁移策略 | `domain-01-cluster-fundamentals/18-upgrade-migration-strategy.md` | 通用升级迁移理论 |
| API 网关迁移 | `domain-03-networking-traffic/09-nginx-ingress-migration-guide.md` | Ingress 控制器迁移 |
| 生产运维实践 | `domain-11-production-operations/` | 生产环境运维全集 |
| 故障排查大全 | `domain-10-troubleshooting-diagnostics/` | 各类故障排查手册 |
| 灾备恢复 | `domain-09-reliability-engineering/` | 备份与恢复策略 |

---

## 快速开始

**第一次做迁移？** 按顺序阅读 → [01-迁移评估与规划](./01-migration-assessment-planning.md)

**已评估完成，准备动手？** → [02-ACK 目标集群设计](./02-ack-target-cluster-design.md)

**只需迁移应用工作负载？** → [03-应用工作负载迁移](./03-application-workload-migration.md)

**需要完整案例参考？** → [10-生产迁移实战案例](./10-real-world-case-study.md)

---

*本专题为 kudig-database 项目原创内容，基于多个生产环境迁移经验总结。*

## Related

- [[entities/kubernetes.md|kubernetes]]
- [[entities/cni.md|cni]]
- [[entities/networkpolicy.md|networkpolicy]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
