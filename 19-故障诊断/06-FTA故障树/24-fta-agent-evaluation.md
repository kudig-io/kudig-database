---
title: 'FTA Agent 评测集设计：诊断准确率 / 路径完整率 / 误报率'
description: 定义 FTA 驱动的 AI Agent 诊断能力评测指标体系与评测基准集（20 条），使"FTA 作为 Agent 知识骨架"从方法论变为可度量、可迭代的工程实践。
summary: FTA-Agent 评测闭环：三指标定义、评测基准集（引用 10-QA语料 与 13-生产运维 工单案例）、评分规则与迭代流程
category: fta
tags:
- fta
- agent
- evaluation
- benchmark
- qa-corpus
- troubleshooting
tier: core
created: '2026-08-13'
last_updated: 2026-08
difficulty: advanced
reading_level: advanced
audience:
- AI Agent 开发者
- SRE
- 运维工程师
- 平台架构师
estimated_read_time: 10min
intent_queries:
- FTA Agent 评测
- Agent 诊断准确率如何度量
- 故障树评测集
- 路径完整率
- FTA 误报率
trigger_keywords:
- 评测
- benchmark
- 准确率
- 误报率
- 路径完整率
- FTA
- Agent
prerequisites:
- troubleshooting-methodology
- monitoring-basics
- fta-basics
fta_id: FTA-EVAL-001
component: FTA Agent Evaluation
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# FTA Agent 评测集设计

> **所属模块**: FTA 故障树模块 · Agent 评测闭环
> **关联文档**: [第十五章：FTA 质量评估与优化](./15-fta-quality-assessment.md)（质量指标总览）· [第九章：FTA 作为 AI Agent 的知识骨架](./09-fta-as-agent-knowledge-skeleton.md)（Agent 执行引擎）· [第二十三章：生产环境快速启动](./23-fta-production-quick-start.md)
> **上一文档**: [第二十三章：FTA 生产环境快速启动与 SRE 集成指南](./23-fta-production-quick-start.md)

---

## 24.1 评测目标

第 8-13 章给出了"FTA 作为 Agent 知识骨架"的落地路径，但缺少**如何评测 Agent 按 FTA 诊断是否正确**的闭环。本文档定义三指标评测体系，使 FTA-Agent 从方法论变为可度量、可迭代的工程实践。

评测回答三个问题：

1. **诊断方向是否正确** —— 顶事件（TE）是否命中？
2. **排查路径是否完整** —— TE → IE → BE 链路是否走通到根因？
3. **是否存在误判** —— 无问题场景是否被误报、非根因是否被错报？

## 24.2 核心评测指标

| 指标 | 定义 | 计算方式 | 目标值 | 数据来源 |
|------|------|---------|-------|---------|
| **诊断准确率（TE 命中率）** | Agent 给出的顶事件与标注 TE 一致的比例 | (TE 命中的评测样本 / 评测样本总数) × 100% | ≥ 90% | 评测基准集执行结果 |
| **路径完整率** | 诊断路径完整覆盖 TE→IE→BE 三层且 BE 正确的比例 | (完整命中路径的样本 / TE 命中样本) × 100% | ≥ 80% | 评测基准集执行结果 |
| **误报率** | 无故障场景中被判定为故障的比例 | (误报样本 / 无故障样本总数) × 100% | ≤ 5% | 负样本集执行结果 |
| **平均诊断深度** | 诊断遍历的平均层数（与 [15.1 质量指标](./15-fta-quality-assessment.md#151-核心质量指标) 对齐） | Sum(诊断深度) / 诊断次数 | 2-4 层 | Agent 诊断日志 |

> 与 [15 章质量指标](./15-fta-quality-assessment.md) 的关系：本文档的三指标是 15.1"诊断准确率 / 首次修复率 / 平均诊断深度"在**评测阶段**的可执行版本；15 章面向生产运行期，本文档面向发布前评测与回归。

## 24.3 评测基准集

评测基准集共 **20 条**，分为三组：

- **正样本（16 条）**：真实工单案例（`13-生产运维/05-工单案例/ticket-case-*`），标注期望 TE 与关键 IE/BE；
- **QA 语料对齐（2 条）**：与 `10-QA语料/topic-fta-qa.yaml`、`故障诊断-qa.yaml` 的 fault_tree 类型题目交叉验证；
- **负样本（2 条）**：无故障/非故障场景，用于度量误报率。

### 24.3.1 正样本基准（工单案例）

| # | 基准用例 | 输入症状（工单标题） | 期望 TE | 期望关键路径 | 来源工单 |
|:---:|:---|:---|:---:|:---|:---|
| 1 | Terway ENI 耗尽 | 节点 NotReady、Pod 无法分配沙箱网络 | TE-9 | IE-9.1 → BE-9.1.x（ENI IP 耗尽） | [ticket-case-001](../../13-生产运维/05-工单案例/ticket-case-001-terway-eni-exhaustion.md) |
| 2 | kubelet 证书过期 | 节点 NotReady、kubelet 证书过期 | TE-7 | IE-7.1 → BE-1.5.3（Kubelet 证书过期） | [ticket-case-005](../../13-生产运维/05-工单案例/ticket-case-005-kubelet-cert-expired.md) |
| 3 | etcd 磁盘满 | API Server 变慢、etcd 磁盘空间耗尽 | TE-1 | IE-1.1 → BE-1.2.1（etcd 磁盘空间耗尽） | [ticket-case-009](../../13-生产运维/05-工单案例/ticket-case-009-etcd-disk-full-apiserver-slow.md) |
| 4 | CoreDNS 转发异常 | Pod 内 DNS 解析失败 | TE-4 | IE-4.1 → BE-4.1（CoreDNS 问题） | [ticket-case-008](../../13-生产运维/05-工单案例/ticket-case-008-coredns-vpc-dns-forward.md) |
| 5 | Java OOM + ESSD IO hang | 应用 OOMKilled、存储 IO hang | TE-5 | IE-5.3 → BE-5.7.x（ESSD 性能异常） | [ticket-case-002](../../13-生产运维/05-工单案例/ticket-case-002-java-oom-essd-iohang.md) |
| 6 | SLB 后端组配置错误 | 服务无法访问、SLB 健康检查失败 | TE-2 | IE-2.2 → BE-4.10（SLB 健康检查失败） | [ticket-case-003](../../13-生产运维/05-工单案例/ticket-case-003-slb-backend-group-misconfig.md) |
| 7 | HPA + metrics-server 异常 | HPA 不扩缩容、metrics-server 不可用 | TE-8 | IE-8.1 → BE-8.x（监控数据采集异常） | [ticket-case-007](../../13-生产运维/05-工单案例/ticket-case-007-hpa-metrics-server-down.md) |
| 8 | NetworkPolicy 拦截 | 业务 Pod 间访问不通 | TE-4 | IE-4.2 → BE-4.4.x（NetworkPolicy 规则） | [ticket-case-010](../../13-生产运维/05-工单案例/ticket-case-010-networkpolicy-blocks-traffic.md) |
| 9 | Ingress 控制器 404/502 | 外部访问 404/502 | TE-2 | IE-2.3 → BE-2.x（Ingress 控制器异常） | [ticket-case-011](../../13-生产运维/05-工单案例/ticket-case-011-ingress-controller-pod-404-502.md) |
| 10 | Pod Pending 资源不足 | Pod Pending、节点资源耗尽 | TE-6 | IE-6.1 → BE-3.x（节点资源不足） | [ticket-case-012](../../13-生产运维/05-工单案例/ticket-case-012-pod-pending-resource-exhaustion.md) |
| 11 | Cluster Autoscaler 扩缩容失败 | 节点不扩容、CA 不工作 | TE-6 | IE-6.1 → BE-6.x（CA 异常） | [ticket-case-020](../../13-生产运维/05-工单案例/ticket-case-020-cluster-autoscaler-scale-failure.md) |
| 12 | RBAC 权限拒绝 | API 访问 403 Forbidden | TE-7 | IE-7.2 → BE-7.x（RBAC 权限问题） | [ticket-case-039](../../13-生产运维/05-工单案例/ticket-case-039-rbac-api-access-denied.md) |
| 13 | CSI 插件缺失 | 扩容后 PVC 无法挂载 | TE-5 | IE-5.2 → BE-5.4（CSI 插件异常） | [ticket-case-004](../../13-生产运维/05-工单案例/ticket-case-004-csi-plugin-missing-after-scale.md) |
| 14 | Prometheus 数据丢失 | 监控数据丢失、查询慢 | TE-16 | IE-16.1 → BE-16.x（指标完整性缺失） | [ticket-case-015](../../13-生产运维/05-工单案例/ticket-case-015-prometheus-data-loss-slow-query.md) |
| 15 | kube-proxy 导致服务不通 | ClusterIP 访问不通 | TE-4 | IE-4.2 → BE-4.x（kube-proxy 异常） | [ticket-case-019](../../13-生产运维/05-工单案例/ticket-case-019-kubeproxy-service-unreachable.md) |
| 16 | StatefulSet PVC Unbound | 有状态应用启动失败 | TE-5 | IE-5.1 → BE-5.1（PVC 绑定失败） | [ticket-case-023](../../13-生产运维/05-工单案例/ticket-case-023-statefulset-pvc-unbound-config-error.md) |

### 24.3.2 QA 语料对齐基准

| # | 基准用例 | 输入 | 期望输出 | 来源 |
|:---:|:---|:---|:---|:---|
| 17 | API Server 不可用 | 「API Server 不可用/性能劣化」告警 | TE-1 + apiserver-fta 根因路径 | [topic-fta-qa.yaml](../10-QA语料/topic-fta-qa.yaml)（fault_tree 类型） |
| 18 | 镜像拉取失败 | Pod ImagePullBackOff | TE-3 + IE-3.2（镜像拉取失败） | [故障诊断-qa.yaml](../10-QA语料/故障诊断-qa.yaml)（fault_tree 类型） |

### 24.3.3 负样本基准（误报率度量）

| # | 基准用例 | 输入 | 期望判定 | 目的 |
|:---:|:---|:---|:---|:---|
| 19 | 正常业务波动 | 单次请求 P99 轻微抖动，无错误率上升 | **不触发**故障树诊断 | 度量误报率 |
| 20 | 非集群问题 | 客户端侧网络故障（本机断网） | 判定为外部因素，不落入 TE 路径 | 度量误报率 |

> 负样本与 [10-QA语料 capability-safety 评测](../10-QA语料/capability-safety/) 配套：前者验证"不误诊"，后者验证"不泄露/不越权"。

## 24.4 评测流程与评分规则

```
输入: 基准用例 (症状描述 + 期望 TE/路径)
步骤:
1. 向被测 Agent 提供症状输入（禁止注入期望答案）
2. 记录 Agent 输出: 判定 TE / 遍历 IE / 定位 BE / 给出修复动作
3. 与标注期望比对，按 24.2 指标计分
4. 汇总生成评测报告（按故障域分组，定位薄弱 FTA 树）
```

| 评分项 | 得分规则 |
|:---|:---|
| TE 命中 | 输出 TE 与期望 TE 一致得 1 分（部分一致 0.5 分，不一致 0 分） |
| 路径完整 | TE 命中前提下，路径覆盖全部关键 IE 且 BE 正确得 1 分；覆盖但 BE 错误 0.5 分；缺失关键 IE 0 分 |
| 误报 | 负样本中被判定为故障记 1 次误报（超阈值即不合格） |

**通过门槛**: 诊断准确率 ≥ 90% 且 路径完整率 ≥ 80% 且 误报率 ≤ 5%，任一不达标则 Agent 版本不得发布。

## 24.5 评测迭代闭环

1. **失败分析**：对未命中样本归类——FTA 树缺失 BE（→ 补树）、Agent 检索失败（→ 调 RAG 索引）、推理错误（→ 修 Agent 逻辑）；
2. **回归**：每次 FTA 树更新（如 [list/README.md](./list/README.md) 新增组件树）后重跑 20 条基准，防止知识变更引入回归；
3. **扩集**：每季度从 `13-生产运维/05-工单案例/` 新工单中挑选典型案例扩充基准（目标 50 条），保持与生产故障分布对齐；
4. **CI 集成**：评测集接入质量流水线（参见 `.github/workflows/quality.yml`），作为 Agent 发布门禁。

## 24.6 与相关模块的关系

| 模块 | 关系 |
|:---|:---|
| [10-QA语料](../10-QA语料/README.md) | 14,080 条 QA pairs / 469 条 I-O 对为评测输入源；`benchmark/benchmark-diagnostic-e2e-v1.yaml` 为端到端基准（本文档聚焦 FTA 诊断环节） |
| [08-技能体系](../08-技能体系/README.md) | 技能型 FTA（node-fta 等）提供 BE 级修复动作，作为"路径完整率"中 BE 正确性的判定依据 |
| [09-多故障场景](../09-多故障场景/index.md) | 级联/共因故障样本可扩展为多故障评测组（AND 门路径） |
| [13-生产运维/05-工单案例](../../13-生产运维/05-工单案例/index.md) | 50 个工单闭环样本为基准集主要来源 |

---

> **导航**: [<< 第二十三章 - 生产环境快速启动](./23-fta-production-quick-start.md) | [FTA 模块入口](./README.md) | [故障树清单索引](./list/README.md) >>

---

## Obsidian 相关文档

- [[19-故障诊断/06-FTA故障树/MOC.md|topic-fta MOC]]
- [[19-故障诊断/06-FTA故障树/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[19-故障诊断/06-FTA故障树/15-fta-quality-assessment.md|第十五章：FTA 质量评估与优化]]
- [[19-故障诊断/06-FTA故障树/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]

## See Also

- [[19-故障诊断/06-FTA故障树/23-fta-production-quick-start.md|23-fta-production-quick-start]]
- [[19-故障诊断/06-FTA故障树/fta-index.md|FTA 故障树完整索引]]
- [[19-故障诊断/10-QA语料/README.md|Agent QA 对语料库]]

<!-- risk-assessed -->
