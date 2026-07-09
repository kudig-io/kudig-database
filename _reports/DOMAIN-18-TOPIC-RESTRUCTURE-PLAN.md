---
title: Domain-18 生产运维 Topic 重组计划
description: 从生产环境维度对 domain-18-production-operations 进行 topic 整合与结构调整的完整方案
summary: 从生产环境维度对 domain-18-production-operations 进行 topic 整合与结构调整的完整方案
category: report
tags:
- k8s
- production-operations
- restructuring
- topic
- architecture
- prometheus
- grafana
- rag
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
- Domain-18 生产运维 Topic 重组计划 是什么
- 如何 Domain-18 生产运维 Topic 重组计划
trigger_keywords:
- Domain-18
- 生产运维
- Topic
- 重组计划
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-18 生产运维 Topic 重组计划

> **报告版本**: 1.0
> **分析日期**: 2026-05-21
> **目标领域**: `domain-18-production-operations`
> **文档数量**: 32 篇
> **执行状态**: ✅ 已完成（两阶段：8 topic → 6 topic）

---

## 一、问题诊断

### 1.1 文档全部平铺，缺乏模块化组织

`domain-18` 现有 32 篇文档全部平铺在根目录，没有像 `domain-12-troubleshooting` 那样按 **topic 子目录** 聚合：

| Domain | 组织方式 | 效果 |
|--------|----------|------|
| `domain-12-troubleshooting` | `故障诊断/topic-febm/`、`故障诊断/topic-fta/`、`故障诊断/topic-structural-trouble-shooting/` | 模块化、按方法论聚类、易于导航 |
| `domain-18-production-operations` | 全部平铺 | 查找困难、边界模糊、认知负担高 |

### 1.2 与独立 Domain 大量重叠

`domain-18` 中多篇文档与其他已拆分的独立 Domain 存在内容交叉，形成事实上的重复维护：

| domain-18 文档 | 重叠的独立 Domain | 建议分工 |
|----------------|-------------------|----------|
| `04-企业级监控体系` / `06-APM` | `domain-20-enterprise-monitoring-alerting` | domain-18 保留架构原则，domain-20 负责工具实现 |
| `05-日志收集分析平台` | `domain-21-logging-management-analytics` | domain-18 保留体系设计，domain-21 负责平台部署 |
| `10-GitOps流水线实践` | `domain-23-gitops-ci-cd` | domain-18 保留流程规范，domain-23 负责工具链配置 |
| `11-基础设施即代码` | `domain-24-infrastructure-as-code` | domain-18 保留设计原则，domain-24 负责具体实现 |
| `07-零信任安全` / `08-CIS合规` / `09-SBOM` | `domain-25-[[系统基础/topic-dictionary/security/cloud-native-security.md|cloud-native-security]]` | domain-18 保留安全架构框架，domain-25 负责工具与合规 |
| `16-备份` / `17-灾备演练` / `18-跨区域容灾` | `domain-30-disaster-recovery-business-continuity` | domain-18 保留策略与流程，domain-30 负责技术实现 |

### 1.3 与根目录 Topic 的交叉关系未厘清

- `topic-best-practices`：按场景（部署、扩缩容、灾备、安全、可观测性）聚合所有 Domain 的最佳实践摘要
- `topic-skills`：按问题场景聚合所有 Domain 的操作卡片
- `domain-18`：本应聚焦生产运维领域的完整知识体系

当前三者缺乏明确的分层约定，存在内容重复沉淀的风险。

### 1.4 `99-*` 深度指南散落

7 篇 `99-*` 前缀的深度指南/架构蓝图平铺在一起，未能与对应主题归类，导航效率低。

### 1.5 过度拆分问题（第二轮优化发现）

第一轮重组为 8 个 topic 后，发现 3 篇的小 topic 多达 5 个（observability-ops、security-compliance、automation-platform、disaster-recovery、operations-management），认知切换成本与内容量不匹配。

---

## 二、重组方案

### 2.1 最终目录结构（6 topic）

参照 `domain-12-troubleshooting` 的 `topic-*` 子目录模式，基于 SRE 可靠性工程框架，在 `domain-18` 内建立以下 **6 个 topic**：

```
生产运维/
├── topic-production-architecture/          # 架构与设计（6 篇）
│   ├── 01-production-architecture-design-principles.md
│   ├── 02-multi-cloud-hybrid-deployment-strategy.md
│   ├── 03-edge-computing-production-deployment.md
│   ├── 99-kubernetes-production-architecture-blueprint.md
│   ├── 99-kubernetes-deployment-patterns-architecture.md
│   └── 99-kubernetes-multi-tenant-architecture.md
├── topic-observability-performance/        # 可观测性与性能（8 篇）
│   ├── 04-enterprise-monitoring-system.md
│   ├── 05-logging-collection-analysis-platform.md
│   ├── 06-apm-application-performance-monitoring.md
│   ├── 19-cluster-performance-tuning.md
│   ├── 20-network-performance-optimization.md
│   ├── 21-storage-performance-optimization.md
│   ├── 99-karpenter-node-autoscaling-guide.md
│   └── 99-keda-event-driven-autoscaling-guide.md
├── topic-security-compliance/              # 安全与合规（3 篇）
│   ├── 07-zero-trust-security-architecture.md
│   ├── 08-cis-benchmark-compliance-audit.md
│   └── 09-software-bill-of-materials.md
├── topic-automation-platform/              # 运维自动化（3 篇）
│   ├── 10-gitops-pipeline-practices.md
│   ├── 11-infrastructure-as-code.md
│   └── 12-automated-operations-toolchain.md
├── topic-cost-governance/                  # 成本与治理（5 篇）
│   ├── 13-kubernetes-cost-governance.md
│   ├── 14-resource-quota-management.md
│   ├── 15-green-computing-sustainability.md
│   ├── 99-finops-cost-optimization-guide.md
│   └── 99-greenops-sustainable-computing-guide.md
└── topic-reliability-operations/           # 可靠性与运营（6 篇）
    ├── 16-enterprise-backup-strategy.md
    ├── 17-disaster-recovery-drills.md
    ├── 18-cross-region-disaster-recovery.md
    ├── 22-change-management-process.md
    ├── 23-incident-response-handling.md
    └── 24-capacity-planning-forecasting.md
```

**整合逻辑**：

| 合并操作 | 来源 | 新 Topic | 文档数 | 理由 |
|----------|------|----------|--------|------|
| 合并 | `topic-observability-ops` + `topic-performance-tuning` | `topic-observability-performance` | 8 | "监控发现瓶颈 → 调优/扩容解决问题" 天然闭环 |
| 合并 | `topic-disaster-recovery` + `topic-operations-management` | `topic-reliability-operations` | 6 | 备份、灾备、变更管理、事件响应、容量规划同属 SRE 可靠性工程 |

### 2.2 跨域分层约定

建立以下分层边界，解决与独立 Domain 的重叠问题：

| 层级 | 负责方 | 内容定位 | 示例 |
|------|--------|----------|------|
| **框架/原则层** | `domain-18` | "为什么这么做"、体系设计原则、流程规范、架构蓝图 | 监控体系架构设计、SLI/SLO 制定、告警分层策略 |
| **工具/实现层** | 独立 Domain (`domain-20`~`domain-30`) | "用什么工具做"、具体配置和命令、平台部署细节 | Prometheus/Grafana/Thanos 的具体部署配置和调优 |

在 `domain-18` 的文档中增加 `cross_refs` 指向独立 Domain，形成 **"原则 → 实践"** 的知识链路。

### 2.3 根目录 Topic 定位澄清

- `topic-best-practices`：跨域横向切片，按场景聚合所有 Domain 的最佳实践**摘要**
- `topic-skills`：跨域横向切片，按问题场景聚合所有 Domain 的**操作卡片**
- `domain-18/topic-*`：纵向领域深度，聚焦生产运维的**完整知识体系**

三者关系应为 **引用（cross-ref）** 而非 **重复**。

---

## 三、执行步骤

1. **创建目录结构**：在 `生产运维/` 下创建 8 个 `topic-*` 子目录
2. **文件迁移**：按映射表将 31 篇文档（不含 `00-open-source-projects-index.md`）移入对应 topic 目录
3. **MOC 重构**：重写 `MOC.md`，按 topic 分组展示文档清单，更新知识图谱
4. **README 重构**：重写 `README.md`，按 topic 分组展示目录结构，更新学习路径
5. **链接修复**：扫描全库，修复所有指向 `domain-18` 文档的 wikilink 路径
6. **交叉引用增强**：在 `domain-18` 各 topic 的文档中增加指向独立 Domain 的 `cross_refs`
7. **验证**：确认所有链接有效，目录结构一致，MOC/READEME 信息准确

---

## 四、风险评估

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| 文件移动导致 git 历史中断 | 中 | 使用 `git mv` 保留历史；在 commit message 中明确说明重组范围 |
| 跨域 wikilink 断裂 | 高 | 执行步骤 5 全库扫描修复；重组后运行链接检查脚本 |
| MOC/READEME 生成脚本不兼容新结构 | 中 | 手动重构 MOC/READEME；后续更新 `scripts/generate-mocs.py` 适配 topic 子目录 |
| 与独立 Domain 的内容边界模糊 | 低 | 在本文档中明确分层约定；后续逐步精简重叠内容 |

---

## 五、预期收益

1. **导航效率提升**：从 32 篇平铺文档 → 8 个模块化 topic，查找路径缩短
2. **与 troubleshooting 风格统一**：全库_domain_组织风格一致，降低认知成本
3. **知识边界清晰**：纵向领域（domain-18）与横向切片（生产运维/topic-best-practices/skills）、工具实现（独立 domain）三层分离
4. **可维护性增强**：新增文档有明确的 topic 归属，避免继续平铺膨胀
5. **`99-*` 指南归位**：深度指南与对应主题聚合，形成"基础 + 进阶"的完整学习路径

---

*本报告由分析 agent 生成，作为 domain-18-production-operations topic 重组的执行依据。*


<!-- risk-assessed -->
