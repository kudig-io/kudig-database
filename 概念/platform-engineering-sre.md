---
title: 平台工程与 SRE 的协作模式
description: '| **工具链** | 选择、集成、维护 | 监控、告警、On-call |'
summary: '| **工具链** | 选择、集成、维护 | 监控、告警、On-call |'
category: synthesis
tags:
- platform-engineering
- sre
- devops
- internal-developer-platform
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 平台工程与 SRE 的协作模式 是什么
- 如何 平台工程与 SRE 的协作模式
trigger_keywords:
- 平台工程与
- SRE
- 的协作模式
prerequisites:
- kubectl-basics
relationships:
- target: '[[系统基础/速查卡/k8s.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 平台工程与 SRE 的协作模式

## 概述

平台工程与 SRE（Site Reliability Engineering）的协作是云原生组织成熟度的重要标志。平台工程专注于构建内部开发者平台（IDP），降低开发者的认知负荷和运维门槛；SRE 专注于保障系统的可靠性、性能和可观测性。两者在工具链、流程和目标上有大量交叉，明确职责边界和协作接口是实现高效组织运转的关键。

## 职责边界

| 职责 | 平台工程 | SRE |
|------|---------|-----|
| **基础设施** | 提供标准化的 [[系统基础/速查卡/k8s.md|K8s]] 平台、集群模板、节点池配置 | 确保平台可靠性，定义 SLA/SLO，容量规划 |
| **开发者体验** | 构建 IDP、应用模板、文档门户 | 定义发布规范、错误预算策略、On-call 标准 |
| **工具链** | 选择、集成、维护 CI/CD、GitOps、监控栈 | 配置告警规则、On-call 轮值、事后复盘 |
| **安全** | 平台级安全基线（RBAC、NetworkPolicy 默认值） | 运行时安全监控、漏洞响应、合规审计 |
| **成本** | 资源配额、计费模型、存储治理 | 利用率优化、容量预测、资源碎片整理 |
| **可靠性** | 平台 HA 架构、多集群管理 | SLO 定义、错误预算、故障注入验证 |

## 协作接口

### 平台工程交付物

```
平台工程提供:
  - 标准化的 Namespace/Cluster 模板
    → 预配置 ResourceQuota、LimitRange、NetworkPolicy
    → 预安装监控 Agent、日志采集器
  - 预配置的监控和告警
    → 黄金信号看板（USE/RED 方法）
    → 默认告警规则（PodCrashLoopBackOff、NodeNotReady）
  - 自助式部署流水线
    → GitOps 模板（ArgoCD Application 模板）
    → CI/CD 流水线（Tekton / GitHub Actions）
  - 开发者门户（Backstage）
    → 服务目录、文档中心、Scaffolder
  - 安全基线
    → 默认 Pod Security Standards（restricted）
    → 镜像扫描（Trivy）准入策略
```

### SRE 交付物

```
SRE 定义:
  - 新服务的 SLO 要求
    → 可用性目标（99.9% / 99.95% / 99.99%）
    → 延迟目标（P99 < 200ms）
    → 吞吐量目标
  - 发布检查清单
    → 上线前 SLO 达标验证
    → 容量规划确认
    → DR 演练通过
  - On-call 轮换机制
    → 7x24 响应体系
    → 事后复盘（Postmortem）流程
    → 错误预算消耗追踪
  - 可靠性工程
    → 混沌工程实验设计
    → 容量规划和预测
    → 事故分析和改进跟踪
```

## 共同目标

```
开发者体验        平台可靠性
     ↘              ↙
      内部开发者平台 (IDP)
           ↓
   "开发者可以自助式地、可靠地
    部署和管理他们的服务"

度量指标:
  - 部署频率（平台工程关注）
  - 变更失败率（SRE 关注）
  - 平均恢复时间（SRE 关注）
  - 开发者满意度（双方关注）
```

## 最佳实践

- **建立清晰的上游/下游关系**：平台工程是 SRE 的上游——平台不稳定，SRE 做再多也无法保障可靠性。平台工程应将 SLO 作为平台的核心交付指标
- **共用 Backstage 作为协作平台**：平台工程在 Backstage 中维护服务目录和文档，SRE 在 Backstage 中展示 SLO 状态和 On-call 信息——统一信息来源
- **平台工程嵌入 SRE 值班**：让平台工程师参与 SRE On-call 轮值，亲身体验平台问题对业务的影响——这比任何文档都有效
- **定期联合复盘**：重大事故复盘应包含平台工程和 SRE 双方视角——根因可能在平台设计而非运维操作
- **自动化减少 Toil**：SRE 识别的重复性运维工作（Toil）应反馈给平台工程，通过平台能力自动化消除

## 常见陷阱

- **职责模糊导致推诿**：当监控告警出现时，"这是平台问题还是应用问题"的争论浪费时间——应在服务上线时就定义清晰的告警 owner
- **平台过度抽象增加复杂度**：平台工程追求"对开发者透明"，但过度抽象会导致问题排查困难——需要在抽象和透明之间找到平衡
- **SRE 变成高级运维**：如果 SRE 花大量时间处理工单而非工程改进，说明组织没有真正实施 SRE——SRE 应该有至少 50% 的时间用于工程改进

## 源码实现分析

### 平台工程与 SRE 协作架构

```go
// 内部开发者平台 (IDP) 核心组件交互
// Backstage + ArgoCD + Prometheus + PagerDuty 集成示例

// 服务上线流程（平台工程视角）
type ServiceOnboarding struct {
    // 1. Backstage 服务目录注册
    CatalogEntity  string // apiVersion: backstage.io/v1alpha1, kind: Component
    // 2. 脚手架生成（Golden Path）
    ScaffoldTemplate string // Cookiecutter/Crossplane 模板
    // 3. CI/CD 流水线自动创建
    PipelineConfig  string // Tekton/GitHub Actions 模板
    // 4. 监控告警自动配置
    SLODefinition   string // Prometheus rules + Grafana dashboard
    // 5. On-call 轮值自动注册
    OncallSchedule  string // PagerDuty/OpsGenie 集成
}

// SRE 可靠性工程循环
type SREReliabilityLoop struct {
    SLI   []ServiceLevelIndicator // 可用性/延迟/吞吐/错误率
    SLO   []ServiceLevelObjective // 99.95% / P99<500ms
    ErrorBudget float64           // 1 - SLO = 允许失败空间
    ToilReduction []Automation    // 自动化消除重复劳动
}
```

### 平台工程与 SRE 职责边界

```
┌──────────────────────────────────────────────────────────┐
│         平台工程 vs SRE 职责边界与协作                │
├──────────────────────────────────────────────────────────┤
│  平台工程 (Platform Engineering)                        │
│  │ • 内部开发者平台 (IDP) 建设与维护                │
│  │ • Golden Path 脚手架 / 自助服务                    │
│  │ • CI/CD 流水线模板 / 部署自动化                  │
│  │ • 服务目录 / 文档中心 / 开发者体验              │
│  │ • 度量：部署频率、开发者满意度                  │
│  │                                                    │
│  │        ──── 协作界面 ────                       │
│  │                                                    │
│  SRE (Site Reliability Engineering)                    │
│  │ • SLO/SLI/Error Budget 定义与监控                │
│  │ • 事故响应 / On-call / Post-Mortem               │
│  │ • 容量规划 / 性能优化 / 混沌工程              │
│  │ • Toil 识别与自动化反馈                          │
│  │ • 度量：MTTR、变更失败率、可用性              │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：服务上线自助流程（平台工程）

```yaml
# 🟡 中风险：创建服务基础设施
# Backstage 服务目录实体
apiVersion: backstage.io/v1alpha1
kind: Component
metadata:
  name: payment-service
  annotations:
    github.com/project-slug: org/payment-service
    argocd/app-name: payment-service
    prometheus.io/slo: "99.95"
spec:
  type: service
  lifecycle: production
  owner: team-payments
  providesApis: [payment-api]
  dependsOn: [resource:default/payment-db, component:default/fraud-detection]
```

### 场景二：SLO 定义与 Error Budget 监控（SRE）

```yaml
# 🟡 中风险：创建告警规则影响值班响应
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: payment-slo-alerts
  namespace: monitoring
spec:
  groups:
  - name: payment.slo
    rules:
    - record: slo:payment:availability
      expr: |
        sum(rate(http_requests_total{service="payment",code!~"5.."}[5m]))
        / sum(rate(http_requests_total{service="payment"}[5m]))
    - alert: PaymentSLOFastBurn
      expr: slo:payment:availability < 0.9980  # 14.4x burn rate
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Payment SLO 快速烧尽，需立即响应"
        runbook: "https://wiki.internal/runbooks/payment-slo-breach"
```

### 场景三：Toil 识别与自动化（SRE → 平台工程反馈）

```bash
# 🟢 低风险：只读分析
# 识别重复性运维工作（Toil）
# 1. 统计过去 30 天告警类型分布
kubectl get events -A --field-selector reason=BackOff -o json | \
  jq '[.items[] | .involvedObject.name] | group_by(.) | map({name: .[0], count: length}) | sort_by(-.count)[:10]'
# 2. 统计手动干预次数（PagerDuty/OpsGenie API）
# 3. 识别 Top Toil 并反馈给平台工程自动化
# 示例：CrashLoopBackOff 自动重启 + 根因分析
# 示例：PVC 扩容自动化（基于使用率告警）
# 示例：证书轮换自动化（cert-manager）
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 平台工程和 SRE 是同一团队 | 职责不同：平台工程关注开发者体验和部署效率；SRE 关注可靠性和事故响应 |
| 2 | SRE 就是高级运维 | SRE 应花 ≥50% 时间做工程改进（自动化/工具），而非处理工单 |
| 3 | 平台越抽象越好 | 过度抽象导致问题排查困难；需在抽象和透明之间平衡 |
| 4 | SLO 是 SRE 单方面定义的 | SLO 应与产品/业务共同定义；技术 SLO 需对齐业务影响 |
| 5 | 自动化能解决所有 Toil | 部分 Toil 需要架构改进而非自动化；自动化本身也有维护成本 |
| 6 | 事故复盘是追责 | Blameless Post-Mortem 目的是改进系统，而非追责个人 |

## 面试要点

1. **Q: 平台工程和 SRE 的核心区别和协作点是什么？**
   A: 平台工程：构建内部开发者平台（IDP），关注开发者体验、部署频率、自助服务能力。SRE：保障生产可靠性，关注 SLO/MTTR/变更失败率。协作点：① 平台工程是 SRE 的上游（平台不稳定 SRE 无法保障可靠性）；② SRE 识别的 Toil 反馈给平台工程自动化；③ 共用 Backstage 作为服务目录和 SLO 展示平台；④ 重大事故联合复盘。

2. **Q: 如何衡量平台工程的成功？**
   A: DORA 四指标 + 开发者体验：① 部署频率（Deploy Frequency）；② 变更前置时间（Lead Time for Changes）；③ 变更失败率（Change Failure Rate）；④ 服务恢复时间（MTTR）；⑤ 开发者满意度（NPS 调查）；⑥ 自助服务比例（无需平台团队介入的部署占比）。目标：让开发者“铺好路”而非“设路障”。

3. **Q: SRE 的 Error Budget 如何影响发布决策？**
   A: Error Budget = 1 - SLO。当 budget 充足时：正常发布新功能、可以接受更高风险的变更。当 budget 耗尽时：冻结功能发布、专注可靠性改进、增加测试覆盖、修复技术债务。这是 SRE 与开发团队之间的“契约”：用数据而非感觉决定“能不能发布”。

4. **Q: 如何设计一个有效的 Toil 消除策略？**
   A: ① 识别：统计 On-call 工单类型、手动操作频率、重复性任务时间占比；② 分类：可自动化（脚本/Operator）vs 需架构改进（消除根因）；③ 优先级：按频率×耗时排序，优先消除高频高耗时 Toil；④ 实施：平台工程提供自动化能力（cert-manager/external-secrets/auto-scaling）；⑤ 度量：Toil 占比应从 >50% 降至 <30%（SRE 时间分配目标）。

## 相关 Domain

- 平台工程/01-idp/01-internal-developer-platform
- [[可靠性/SRE实践/04-toil-reduction-automation.md|04 toil reduction automation]]

## 相关页面

- [[概念/backstage-platform-catalog.md|Backstage 平台目录]] — IDP 核心组件
- [[概念/slo-monitoring-integration.md|SLO 与监控集成]] — SLO 工程实践
- [[概念/observability-finops.md|可观测性与 FinOps]] — 成本治理协作

## Related

- [[概念/platform-engineering-idp|平台工程 × IDP(开发者平台视角)]]
- [[系统基础/知识字典/security/runtime-security.md|运行时安全]]


<!-- risk-assessed -->
