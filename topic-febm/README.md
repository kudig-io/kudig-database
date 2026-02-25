# topic-febm: FEBM 法医鉴定循证方法论深度解析

> **文档数量**: 10 篇（8章 + 1主文档 + 1 PDF原文） | **最后更新**: 2026-02

---

## 专题概述

本专题提供 FEBM（Forensic Evidence-Based Methodology）法医鉴定循证方法论的完整知识体系，从传统法医学的洛卡德交换原理到云原生 Kubernetes 环境中的数字取证实践，涵盖方法论理论、技术实现、最佳实践、AI Agent 工单处理集成、体系建设方法论及生产环境快速落地。

FEBM 与 FTA 形成方法论互补：FTA 采用**演绎法**（自上而下，从假设到验证），FEBM 采用**归纳法**（自下而上，从证据到结论）。两者在成熟运维体系中共同构成完整的故障诊断和安全调查能力。

**适用读者**：SRE、安全工程师、取证分析师、平台架构师、AIOps 工程师

---

## 快速导航

| 目标 | 推荐起点 |
|:---|:---|
| **快速了解 FEBM** | [第一章：FEBM 方法论原理与理论基础](./1_febm_theory_foundations.md) |
| **快速落地 FEBM** | [第八章：生产环境快速启动与 K8s 故障取证手册](./8_febm_production_quick_start.md) |
| **技术实现深度** | [第二章：FEBM 技术实现体系](./2_febm_technical_implementation.md) |
| **AI Agent 工单处理** | [第四章：FEBM 对云平台工单智能体托管的意义](./4_febm_agent_ticket_processing.md) |
| **FTA vs FEBM** | [FTA-vs-FEBM.pdf](./FTA-vs-FEBM.pdf)（原始论文） |
| **总纲概览** | [FEBM 方法论深度解析](./febm_methodology_deep_dive.md) |

## 最近更新（2026 Q2）
- **取证自动化蓝图**：新增总纲章节 [7.1 取证自动化蓝图（E2E 流程）](./febm_methodology_deep_dive.md#71-取证自动化蓝图e2e-流程)
- **合规落地清单**：新增总纲章节 [7.2 合规落地清单（SOC 2 / ISO 27001 / 等保）](./febm_methodology_deep_dive.md#72-合规落地清单soc-2--iso-27001--等保)
- **落地度量仪表板**：新增总纲章节 [7.3 落地度量仪表板](./febm_methodology_deep_dive.md#73-落地度量仪表板示例指标)

---

## 文档索引

### 基础文档

| 文档 | 说明 |
|:---|:---|
| [febm_methodology_deep_dive.md](./febm_methodology_deep_dive.md) | 总纲文档，六大部分概览（1041行） |
| [FTA-vs-FEBM.pdf](./FTA-vs-FEBM.pdf) | FTA 与 FEBM 在 Kubernetes 运维中的适用性对比研究（原始论文） |

### 分章详解

| # | 文档 | 核心内容 | 行数 |
|:---:|:---|:---|:---:|
| 1 | [FEBM 方法论原理与理论基础](./1_febm_theory_foundations.md) | 洛卡德交换原理、四大支柱（证据中心性/程序规范性/时效敏感性/结论可辩护性）、FEBM vs FTA 认识论差异、认知偏差防范 | 684 |
| 2 | [FEBM 技术实现体系](./2_febm_technical_implementation.md) | 证据生命周期管理、容器检查点（CRIU）、eBPF 遥测、内存取证（Volatility）、时间线重建、网络取证、K8s 审计日志深度解析、多源证据融合 | 3,388 |
| 3 | [FEBM 最佳实践](./3_febm_best_practices.md) | 五层可观测性栈、证据采集策略、NIST SP 800-61 事件响应流程、Forensics as Code、持续取证、证据存储管理、取证环境隔离、常见陷阱与反模式 | 3,163 |
| 4 | [FEBM 对云平台工单智能体托管的意义](./4_febm_agent_ticket_processing.md) | Agent 工单处理架构、七大核心能力模型、FTA+FEBM 融合模式、三个完整案例（连接池耗尽/容器逃逸/静默失败）、人机协同分级、知识进化机制 | 2,690 |
| 5 | [FEBM 体系建设方法论](./5_febm_construction_methodology.md) | 五级成熟度模型、分阶段建设路线（Phase 1-5）、工具链参考、组织角色矩阵、实施建议、预算规划、合规法律考量 | 2,873 |
| 6 | [未来演进方向](./6_febm_future_evolution.md) | AI/ML 增强混合方法、云原生取证基础设施（OSDFIR）、DevSecOps 融合、意图模型协同、数字孪生、量子计算影响、标准化 | 3,916 |
| 7 | [附录](./7_febm_appendix.md) | 50+ 术语表、参考标准与规范（NIST/ISO/RFC）、40+ 工具速查表、K8s 审计策略模板、Falco 检测规则模板、事件响应 Checklist | 1,267 |
| 8 | [生产环境快速启动与 K8s 故障取证手册](./8_febm_production_quick_start.md) | **第一周行动清单**、最小化工具栈部署、6个 K8s 故障取证 Runbook（OOMKilled/CrashLoopBackOff/NodeNotReady/间歇超时/证书过期/配置漂移）、FTA+FEBM 联合诊断、KPI 仪表板、合规快速参考 | 1,600+ |

---

## 核心概念速览

```
FEBM 四大支柱:

  证据中心性           程序规范性           时效敏感性         结论可辩护性
  (Evidence            (Procedural         (Time              (Defensible
   Centricity)          Rigor)              Sensitivity)       Conclusions)
  ┌──────────┐        ┌──────────┐        ┌──────────┐      ┌──────────┐
  │ 所有结论 │        │ 遵循 NIST│        │ 按易失性 │      │ 可审计   │
  │ 必须基于 │        │ /ISO 标准│        │ 优先采集 │      │ 可复现   │
  │ 可验证   │        │ 采集和   │        │ 容器环境 │      │ 可辩护   │
  │ 证据     │        │ 保全证据 │        │ 秒级响应 │      │          │
  └──────────┘        └──────────┘        └──────────┘      └──────────┘
```

```
FTA vs FEBM:

  FTA (演绎法)                          FEBM (归纳法)
  "系统可能在哪里出问题？"              "系统实际发生了什么？"
  自上而下：假设 → 验证                 自下而上：证据 → 结论
  适合：已知故障模式、架构评审           适合：未知故障、安全事件、动态环境
```

---

## 关联专题

| 专题 | 说明 |
|:---|:---|
| [topic-fta](../topic-fta/README.md) | FTA 故障树分析方法论（演绎法视角，与FEBM互补） |
| [domain-7-security](../domain-7-security/) | 安全合规知识域 |
| [domain-8-observability](../domain-8-observability/) | 可观测性体系 |
| [topic-structural-trouble-shooting](../topic-structural-trouble-shooting/README.md) | 结构化故障排查知识库 |

---

## 阅读建议

```
新手路径:      第一章 → 总纲文档 → 第八章(快速启动)
SRE 路径:      第八章(快速启动) → 第三章(最佳实践) → 第四章(工单Agent)
安全工程师:    第二章(技术实现) → 第三章(最佳实践) → 附录(规则模板)
架构师:        第一章 → 第五章(体系建设) → 第六章(未来演进)
Agent 工程师:  第四章(工单Agent) → 第五章(建设方法论) → topic-fta(FTA方法论)
管理者:        总纲文档 → 第五章(成熟度模型/预算) → 第八章(KPI/合规)
```

---

## 统计

| 指标 | 数值 |
|:---|:---|
| 文档总数 | 10 篇 |
| 总行数 | 约 20,600 行 |
| 代码/配置示例 | 150+ 个 |
| ASCII 架构图 | 100+ 个 |
| 对比表格 | 80+ 个 |
| 实战案例 | 15+ 个完整案例 |
