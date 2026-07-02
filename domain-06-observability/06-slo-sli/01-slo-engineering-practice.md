---
title: SLO 工程实践：定义、衡量与报告
description: 面向阿里云/专有云 K8s 的 SLO 工程实践，讲解 SLO 的定义方法、SLI 选择、错误预算、报告机制与持续改进。
summary: 面向阿里云/专有云 K8s 的 SLO 工程实践，讲解 SLO 的定义方法、SLI 选择、错误预算、报告机制与持续改进。
category: observability
tags:
- k8s
- slo
- sli
- error-budget
- reliability
- observability
- sre
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 产品经理
estimated_read_time: 25min
intent_queries:
- SLO 工程实践
- SLO 定义衡量报告
- K8s SRE SLO 实施
trigger_keywords:
- SLO
- SLI
- error budget
- 服务等级目标
- 可靠性
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- sre-practices
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SLO 工程实践：定义、衡量与报告

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，系统讲解 SLO 的定义、衡量、报告与持续改进方法。

## 目录

1. [SLO/SLI/SLA 关系](#sloslislaa-关系)
2. [SLI 选择方法](#sli-选择方法)
3. [SLO 设定原则](#slo-设定原则)
4. [错误预算机制](#错误预算机制)
5. [SLO 衡量与报告](#slo-衡量与报告)
6. [SLO 驱动决策](#slo-驱动决策)
7. [阿里云/专有云场景](#阿里云专有云场景)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. SLO/SLI/SLA 关系

### 1.1 概念定义

| 术语 | 全称 | 定义 | 约束方 |
|:---|:---|:---|:---|
| SLI | Service Level Indicator | 服务等级指标，可量化的服务质量指标 | 内部测量 |
| SLO | Service Level Objective | 服务等级目标，SLI 的目标值 | 内部承诺 |
| SLA | Service Level Agreement | 服务等级协议，对客户的法律承诺 | 外部合同 |
| Error Budget | 错误预算 | SLO 允许的失败额度 | 内部管理 |

### 1.2 关系图

```
SLI（测量） → SLO（目标） → SLA（合同）
                ↓
          Error Budget（可用额度）
```

---

## 2. SLI 选择方法

### 2.1 用户旅程映射

从用户视角识别关键路径：

| 用户旅程 | 关键 SLI | 测量点 |
|:---|:---|:---|
| 浏览商品 | 页面加载延迟 | CDN / Ingress |
| 下单支付 | 支付成功率 | 支付服务 |
| 查看订单 | 查询延迟 | 订单服务 |
| 接收通知 | 消息到达率 | 消息服务 |

### 2.2 常见 SLI 类型

| 类型 | 示例 | 典型目标 |
|:---|:---|:---|
| 可用性 | HTTP 成功率 | 99.9% |
| 延迟 | P95 响应时间 | < 200ms |
| 吞吐量 | 每秒请求数 | > 10000 RPS |
| 正确性 | 数据一致性率 | 99.99% |
| 新鲜度 | 数据更新延迟 | < 5min |
| 覆盖率 | 监控覆盖率 | 100% |

---

## 3. SLO 设定原则

### 3.1 SMART 原则

| 原则 | 说明 |
|:---|:---|
| Specific | 明确到具体服务和指标 |
| Measurable | 可通过监控数据测量 |
| Achievable | 基于历史数据可达 |
| Relevant | 与业务价值相关 |
| Time-bound | 明确时间窗口 |

### 3.2 SLO 声明格式

```
<服务> 在 <时间窗口> 内，<SLI> 达到 <目标值>
```

示例：
- 订单服务在 30 天内，P95 延迟低于 200ms 的比例达到 99.9%
- 支付服务在 7 天内，成功率达到 99.95%

### 3.3 SLO 目标层级

| 服务等级 | 可用性 SLO | 适用服务 |
|:---|:---:|:---|
| 关键核心 | 99.99% | 支付、订单 |
| 重要业务 | 99.9% | 商品、用户 |
| 内部工具 | 99% | 管理后台 |

---

## 4. 错误预算机制

### 4.1 错误预算计算

```
错误预算 = (1 - SLO) × 测量窗口
```

| SLO | 月度错误预算（分钟） |
|:---|---:|
| 99% | 432 |
| 99.9% | 43.2 |
| 99.99% | 4.32 |
| 99.999% | 0.43 |

### 4.2 错误预算政策

| 剩余预算 | 行动 |
|:---|:---|
| > 50% | 正常发布 |
| 10% - 50% | 谨慎发布，增加评审 |
| < 10% | 暂停非紧急发布 |
| 已耗尽 | 仅允许修复性变更 |

---

## 5. SLO 衡量与报告

### 5.1 Prometheus 记录规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-recording-rules
  namespace: monitoring
spec:
  groups:
    - name: slo.availability
      interval: 60s
      rules:
        - record: slo:availability_ratio_30d
          expr: |
            1 - (
              sum(rate(http_requests_total{status=~"5.."}[30d]))
              /
              sum(rate(http_requests_total[30d]))
            )
        - record: slo:latency_p95_ratio_30d
          expr: |
            1 - (
              sum(rate(http_request_duration_seconds_bucket{le="0.2"}[30d]))
              /
              sum(rate(http_request_duration_seconds_count[30d]))
            )
```

### 5.2 SLO 报告模板

```markdown
# SLO 月度报告

## 服务：order-service

| SLI | SLO | 实际值 | 状态 |
|:---|---:|---:|:---:|
| 可用性 | 99.9% | 99.95% | ✅ |
| P95 延迟 | < 200ms | 150ms | ✅ |
| 错误率 | < 0.1% | 0.05% | ✅ |

## 错误预算消耗
- 月度错误预算：43.2 分钟
- 已消耗：12 分钟
- 剩余：31.2 分钟（72%）

## 主要事件
- 2026-06-15 网络抖动导致 5 分钟可用性下降
- 2026-06-20 数据库慢查询导致延迟升高

## 改进措施
- 优化数据库索引
- 增加网络冗余
```

---

## 6. SLO 驱动决策

### 6.1 发布决策

| 错误预算状态 | 发布策略 |
|:---|:---|
| 充足 | 正常发布 |
| 紧张 | 严格审批，只接受低变更 |
| 耗尽 | 冻结发布，只修复故障 |

### 6.2 优先级分配

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看各服务 SLO 达成情况，优先改进不达标服务
kubectl exec -it thanos-query -n monitoring -- curl -s \
  "http://prometheus:9090/api/v1/query?query=slo:availability_ratio_30d"
```
---

## 7. 阿里云/专有云场景

### 7.1 阿里云 SLB/云监控 SLO

阿里云提供云产品级别的 SLA，但应用层 SLO 仍需自建：

```bash
# 查询 SLB 可用性
aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId <lb-id>
```

### 7.2 专有云监控对接

- 使用专有云 Prometheus 采集应用指标
- 通过 Grafana 展示 SLO 仪表盘
- 告警对接专有云告警中心

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| SLI 已定义 | 每个核心服务 2-4 个 SLI | SLO 文档 |
| SLO 已设定 | 有明确目标值和时间窗口 | SLO 文档 |
| 错误预算计算 | 自动化计算 | PrometheusRule |
| SLO 仪表盘 | 实时展示 | Grafana |
| 月度报告 | 定期输出 | 报告邮件 |
| 发布决策 | 参考错误预算 | 变更流程 |

---

## SLO 与错误预算的沟通

SLO 不仅是技术指标，也是团队间沟通的语言。通过错误预算，可以在可靠性与交付速度之间建立共识。

### 错误预算沟通示例

| 场景 | 沟通话术 |
|:---|:---|
| 预算充足 | “本月错误预算充足，可以按计划发布新功能。” |
| 预算紧张 | “本月错误预算已用 80%，建议暂缓非紧急发布。” |
| 预算耗尽 | “错误预算已耗尽，暂停发布并优先修复稳定性问题。” |

### SLO 例外流程

特殊情况下可申请临时调整 SLO，需经容量委员会或 SRE 负责人审批，并记录原因与恢复时间。

```markdown
# SLO 例外申请
- 服务：{{service}}
- 申请原因：{{reason}}
- 临时 SLO：{{temporary_slo}}
- 有效期：{{duration}}
- 审批人：{{approver}}
```

### SLO 文档化

每个 SLO 应以代码或文档形式保存，包含 SLI 定义、计算公式、数据源、负责人与 review 周期。

## SLO 文化建设

SLO 体系要真正落地，需要技术与业务团队共同认可。建议从以下方面推动：

1. **从小范围试点**：选择 2-3 个核心服务先行试点。
2. **公开透明**：将 SLO 面板嵌入团队日常工具。
3. **与发布联动**：将错误预算作为发布门禁。
4. **奖励改进**：对持续提升 SLO 的团队给予认可。
5. **容忍失败**：错误预算是为了允许合理风险，不要追求 100% SLO。

### SLO 文档模板

```markdown
# {{service}} SLO

## SLI
- 可用性：成功请求 / 总请求
- 延迟：P95 请求耗时

## SLO
- 可用性 ≥ 99.9%（30 天）
- P95 延迟 < 200ms（30 天）

## 数据源
- Prometheus: http_requests_total, http_request_duration_seconds

## 负责人
- SRE: {{sre_name}}
- 开发: {{dev_name}}

## Review 周期
- 每月
```

## 典型工单场景与处理

**场景**：业务方要求所有服务都达到 99.999% 可用性。

处理步骤：
1. 解释 SLO 需与业务价值、成本匹配。
2. 提供历史可用性数据作为参考。
3. 建议核心服务采用严格 SLO，非核心服务放宽。
4. 通过错误预算说明过高 SLO 对发布频率的影响。

## SLO 落地路径

1. **识别关键服务**：从用户旅程中找出影响最大的 3-5 个服务。
2. **定义 SLI**：为每个服务选择 2-3 个核心指标。
3. **设定 SLO**：基于历史数据，设定 30 天或 7 天目标。
4. **建立监控**：在 Prometheus/Grafana 中实现 SLI 计算与展示。
5. **配置告警**：设置错误预算 burn rate 告警。
6. **纳入发布流程**：将错误预算作为发布门禁。
7. **定期复盘**：每月 review SLO 达成情况并持续优化。

### SLO 与业务价值

| SLO 提升 | 业务价值 |
|:---|:---|
| 可用性 99.9% → 99.99% | 减少停机损失，提升用户信任 |
| 延迟 P95 降低 50% | 提升转化率与用户体验 |
| 错误预算管理 | 平衡稳定性与交付速度 |

### SLO 常见误区

| 误区 | 正确理解 |
|:---|:---|
| SLO 越高越好 | 过高 SLO 会显著增加成本 |
| 只关注可用性 | 延迟、正确性同样影响用户体验 |
| 设定后不再调整 | SLO 应随业务与技术水平演进 |

## Related

- [[domain-06-observability/06-slo-sli/18-slo-sli-system.md|SLO/SLI体系建设与管理]]
- [[domain-09-reliability-engineering/04-slo-sli/01-sli-definition-selection.md|SLI 定义与选择]]

## See Also

- [[domain-06-observability/06-slo-sli/02-error-budget-policy.md|错误预算政策与 burn rate alert]]
- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring.md|Prometheus 企业监控]]


<!-- risk-assessed -->
