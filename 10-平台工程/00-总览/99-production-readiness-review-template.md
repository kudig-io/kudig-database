---
title: 生产就绪评审（PRR）模板
description: 面向 Kubernetes 平台工程团队的生产就绪评审模板，包含检查清单、风险矩阵、上线门控、回滚标准与多方会签表
summary: 面向 Kubernetes 平台工程团队的生产就绪评审模板，包含检查清单、风险矩阵、上线门控、回滚标准与多方会签表，适用于平台组件首次上线、重大版本升级与架构变更。
category: platform-engineering
tags:
- production
- best-practices
- playbook
- platform-engineering
- prr
- readiness
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 架构师
estimated_read_time: 25min
intent_queries:
- 生产就绪评审模板是什么
- 如何进行 Kubernetes PRR
- PRR 检查清单与风险矩阵怎么做
trigger_keywords:
- PRR
- 生产就绪评审
- readiness review
- 上线门控
- 风险矩阵
- rollback criteria
prerequisites:
- kubectl-basics
- platform-engineering-basics
- risk-management-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 生产就绪评审（PRR）模板

> **适用范围**: Kubernetes 平台工程组件、IDP 服务、关键基础设施在上线前的生产就绪评审。  
> **目标读者**: SRE、平台工程师、产品经理、安全代表、架构师。  
> **最后更新**: 2026-07-01

本模板提供一份可裁剪的 **Production Readiness Review（PRR）** 检查清单、风险矩阵、上线门控、回滚标准与会签表。PRR 是平台工程变更管理的关键门控，位于架构评审之后、生产发布之前。评审结果应作为变更管理系统的附件存档，并在上线后 30 天内进行首次复盘。

---

## 1. 适用场景与范围

- **适用场景**: 新平台组件首次上线（GitOps 控制器、自动扩缩容、Secret 管理、服务网格控制面等）；重大版本升级或架构变更（如 Karpenter 大版本、Argo CD 分片改造）；多租户模板、ResourceQuota、准入策略变更。
- **不适用场景**: 纯业务应用发布（请参考 [[11-发布变更/README.md|发布与变更管理域]]）；仅修改文档或非生产配置且不影响 SLO 的变更。

---

## 2. 前置条件与工具

| 条件 | 要求 |
|---|---|
| 文档 | 已提交架构设计、运维 Runbook、回滚方案 |
| 环境 | staging 环境完成至少一次全量演练 |
| 监控 | 关键指标已接入 Prometheus/Grafana，告警规则已生效 |
| 安全 | 安全团队已完成 RBAC、NetworkPolicy、Secret、镜像签名审计 |
| 备份 | 配置与数据已具备可恢复备份 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl version --client
helm version
argocd version --client  # 如使用 Argo CD
```
---

## 3. 核心概念/架构

PRR 的核心目标是：在变更进入生产环境前识别架构、运维、安全、可观测性风险；确保所有关键检查项有明确的验收标准、验证方法与责任人；建立可审计的上线门控与回滚决策依据。SRE 负责评估变更对 SLO 的影响，确认监控、告警、容量、回滚方案是否到位，会签表示同意进入发布窗口。

---

## 4. 标准操作流程

### 4.1 发起 PRR

1. 变更负责人填写本模板前半部分（适用场景、前置条件、检查清单自检）。
2. 收集架构设计文档、运维 Runbook、监控 Dashboard 链接、安全审计报告。
3. 在变更管理系统中创建 PRR 工单，并邀请 SRE、安全、架构、网络四方评审。

### 4.2 检查清单评审

#### 架构与高可用

| 编号 | 检查项 | 验收标准 | 验证命令/方法 | 结果 |
|---|---|---|---|---|
| A1 | 组件多副本部署 | replicas ≥ 2，跨可用区分布 | `kubectl get pods -o wide` | □ 通过 □ 遗留 |
| A2 | PodDisruptionBudget | minAvailable ≥ 1 | `kubectl get pdb -n <ns>` | □ 通过 □ 遗留 |
| A3 | 依赖服务降级方案 | 明确下游故障时的降级策略 | 设计文档 | □ 通过 □ 遗留 |
| A4 | 数据持久化与备份 | 关键配置/数据可恢复 | 备份与恢复演练记录 | □ 通过 □ 遗留 |

#### 可观测性

| 编号 | 检查项 | 验收标准 | 验证命令/方法 | 结果 |
|---|---|---|---|---|
| O1 | RED/USE 指标覆盖 | 请求、错误、延迟、饱和度可监控 | `kubectl get servicemonitor -n <ns>` | □ 通过 □ 遗留 |
| O2 | 告警分级与路由 | critical 告警 5 分钟内到达 On-Call | Alertmanager 配置 | □ 通过 □ 遗留 |
| O3 | Dashboard 就绪 | 每个 critical 告警对应 Dashboard | Grafana 链接 | □ 通过 □ 遗留 |
| O4 | 日志采集 | 日志接入 Loki/Elasticsearch/SLS | `kubectl logs` / 日志平台 | □ 通过 □ 遗留 |

#### 安全与合规

| 编号 | 检查项 | 验收标准 | 验证命令/方法 | 结果 |
|---|---|---|---|---|
| S1 | RBAC 最小权限 | 无 cluster-admin 滥用 | `kubectl auth can-i --list` | □ 通过 □ 遗留 |
| S2 | NetworkPolicy | 默认拒绝 + 白名单 | `kubectl get networkpolicy -n <ns>` | □ 通过 □ 遗留 |
| S3 | Secret 管理 | 使用 External Secrets/Sealed Secrets/Vault | `kubectl get externalsecrets -n <ns>` | □ 通过 □ 遗留 |
| S4 | 镜像签名/扫描 | 生产镜像经扫描与签名 | Harbor/ACR 扫描报告 | □ 通过 □ 遗留 |

#### 变更与回滚

| 编号 | 检查项 | 验收标准 | 验证命令/方法 | 结果 |
|---|---|---|---|---|
| C1 | GitOps 同步 | 配置纳入 GitOps 管理 | `argocd app list` | □ 通过 □ 遗留 |
| C2 | 历史版本保留 | 保留 ≥ 10 个版本 | `helm history <release>` | □ 通过 □ 遗留 |
| C3 | 回滚命令验证 | 已在 staging 验证回滚 | 回滚演练记录 | □ 通过 □ 遗留 |
| C4 | 变更窗口 | 已确认低峰期窗口 | 变更单 | □ 通过 □ 遗留 |

### 4.3 风险矩阵评审

| 风险项 | 影响范围 | 发生概率 | 风险等级 | 缓解措施 | 责任人 | 状态 |
|---|---|---|---|---|---|---|
| 控制器单点故障 | 集群级 | 中 | 高 | 多副本 + PDB + 反亲和 | SRE-A | □ |
| 证书过期导致服务中断 | 组件级 | 低 | 高 | cert-manager + 30 天告警 | SRE-B | □ |
| GitOps 配置漂移 | 集群级 | 中 | 中 | 禁止手动 edit，启用 drift detection | 平台工程师 | □ |
| 升级导致 API 废弃 | 应用级 | 中 | 中 | 升级前扫描废弃 API | 架构师 | □ |
| 监控缺失导致误发 | 事件响应 | 低 | 中 | 每个告警附带 Runbook | SRE-A | □ |
| 成本超支 | 财务 | 低 | 低 | ResourceQuota + FinOps 标签 | 平台工程师 | □ |

**风险等级定义**: 高 = 必须解决或取得风险接受方可上线；中 = 需有缓解措施并登记遗留项；低 = 可接受，但需持续观察。

### 4.4 上线门控确认

- [ ] **Gate 1: 自检完成** — 负责人填写本 PRR 检查清单并提交证据。
- [ ] **Gate 2: 跨团队评审** — SRE、安全、架构、网络四方评审通过。
- [ ] **Gate 3: 风险接受** — 所有高/中风险已有关闭或经管理层书面接受的遗留项。
- [ ] **Gate 4: 演练通过** — staging 环境完成升级、回滚、故障切换演练。
- [ ] **Gate 5: 变更单审批** — 变更管理系统中审批完成，窗口已确认。
- [ ] **Gate 6: 监控就绪** — critical 告警、Dashboard、Runbook 全部就位。

### 4.5 会签与归档

所有相关方在会签表签字后，PRR 通过。评审记录、风险矩阵、检查清单扫描件归档至变更管理系统。

---

## 5. 关键检查点与验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 Pod 分布
kubectl get pods -n <ns> -o wide --sort-by='.spec.nodeName'

# 验证 PDB
kubectl get pdb -n <ns>

# 验证 ServiceAccount 权限
kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa> | grep -E 'create|delete|patch'

# 验证 NetworkPolicy
kubectl get networkpolicy -n <ns> -o yaml

# 验证 Helm 历史版本
helm history <release> -n <ns>

# 验证 Argo CD 同步状态
argocd app get <app-name>

# 验证告警规则
kubectl get prometheusrules -n <ns>
```
---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| PRR 检查清单大量遗留 | 文档或演练不到位 | 逐项复核 | 补充文档、补做演练后重新评审 |
| 风险等级争议 | 影响范围或概率评估不一致 | 参考历史故障数据 | 引入第三方 SRE/架构师仲裁 |
| 上线门控未通过 | 监控/告警/回滚未就绪 | `kubectl get pods` / `argocd app get` | 推迟发布，补齐门控 |
| 回滚演练超时 | 依赖复杂或命令未脚本化 | 计时演练记录 | 将回滚步骤脚本化并优化 |
| 评审后配置漂移 | 手动修改未同步 GitOps | `argocd app diff` | 禁止手动修改，启用 drift detection |

---

## 7. 风险与注意事项

1. **PRR 不是形式流程**：未真实执行的检查清单会留下生产隐患，评审会议应逐条核对证据。
2. **高风险遗留必须升级**：任何高风险遗留项未取得管理层书面接受，不得进入发布窗口。
3. **回滚方案必须可执行**：回滚命令需在 staging 真实执行并计时，不能仅停留在文档层面。
4. **监控告警需经过告警测试**：未经过测试的告警规则不得在 PRR 中标记为通过。
5. **PRR 记录应长期保存**：建议保留至组件下线后至少一年，以满足审计与复盘需求。
6. **上线后复盘不可省略**：上线后 30 天内应进行首次复盘，验证假设、更新风险矩阵。


### 4.6 评审输出与持续改进

PRR 评审完成后，应在变更工单中记录评审结论、遗留项清单、上线门控状态与回滚命令。上线后 30 天内应召开复盘会，验证 SLO 是否达成、是否有新增告警、回滚方案是否仍然有效。复盘结论应反馈到本模板，持续优化检查清单与风险矩阵，形成组织级知识沉淀。

---

## 8. 自动化 PRR 检查脚本

### 8.1 一键生产就绪检查

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# PRR 自动化检查脚本 — 在评审会前执行，生成报告

NAMESPACE=${1:-platform-system}
REPORT="/tmp/prr-check-$(date +%Y%m%d-%H%M).txt"

echo "=== PRR 自动化检查报告 ===" | tee "$REPORT"
echo "命名空间: $NAMESPACE" | tee -a "$REPORT"
echo "时间: $(date)" | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 1. 高可用检查
echo "--- [架构与高可用] ---" | tee -a "$REPORT"
REPLICAS=$(kubectl get deploy -n "$NAMESPACE" -o json | jq '[.items[] | select(.spec.replicas < 2)] | length')
if [ "$REPLICAS" -gt 0 ]; then
  echo "❌ 存在单副本 Deployment:" | tee -a "$REPORT"
  kubectl get deploy -n "$NAMESPACE" -o json | jq -r '.items[] | select(.spec.replicas < 2) | "  \(.metadata.name) replicas=\(.spec.replicas)"' | tee -a "$REPORT"
else
  echo "✅ 所有 Deployment 副本数 ≥ 2" | tee -a "$REPORT"
fi

PDB_COUNT=$(kubectl get pdb -n "$NAMESPACE" --no-headers 2>/dev/null | wc -l)
DEPLOY_COUNT=$(kubectl get deploy -n "$NAMESPACE" --no-headers | wc -l)
if [ "$PDB_COUNT" -lt "$DEPLOY_COUNT" ]; then
  echo "⚠️  PDB 覆盖不完整: $PDB_COUNT/$DEPLOY_COUNT" | tee -a "$REPORT"
else
  echo "✅ PDB 覆盖完整" | tee -a "$REPORT"
fi

# 2. 可观测性检查
echo "" | tee -a "$REPORT"
echo "--- [可观测性] ---" | tee -a "$REPORT"
SM_COUNT=$(kubectl get servicemonitor -n "$NAMESPACE" --no-headers 2>/dev/null | wc -l)
if [ "$SM_COUNT" -eq 0 ]; then
  echo "❌ 无 ServiceMonitor，指标未接入 Prometheus" | tee -a "$REPORT"
else
  echo "✅ ServiceMonitor 已配置: $SM_COUNT 个" | tee -a "$REPORT"
fi

RULE_COUNT=$(kubectl get prometheusrules -n "$NAMESPACE" --no-headers 2>/dev/null | wc -l)
if [ "$RULE_COUNT" -eq 0 ]; then
  echo "❌ 无 PrometheusRule，告警未配置" | tee -a "$REPORT"
else
  echo "✅ PrometheusRule 已配置: $RULE_COUNT 个" | tee -a "$REPORT"
fi

# 3. 安全检查
echo "" | tee -a "$REPORT"
echo "--- [安全与合规] ---" | tee -a "$REPORT"
NP_COUNT=$(kubectl get networkpolicy -n "$NAMESPACE" --no-headers 2>/dev/null | wc -l)
if [ "$NP_COUNT" -eq 0 ]; then
  echo "❌ 无 NetworkPolicy，网络未隔离" | tee -a "$REPORT"
else
  echo "✅ NetworkPolicy 已配置: $NP_COUNT 个" | tee -a "$REPORT"
fi

# 检查是否有 Pod 使用 default ServiceAccount
DEFAULT_SA=$(kubectl get pods -n "$NAMESPACE" -o json | jq '[.items[] | select(.spec.serviceAccountName == "default" or .spec.serviceAccountName == null)] | length')
if [ "$DEFAULT_SA" -gt 0 ]; then
  echo "⚠️  $DEFAULT_SA 个 Pod 使用 default ServiceAccount" | tee -a "$REPORT"
else
  echo "✅ 所有 Pod 使用专用 ServiceAccount" | tee -a "$REPORT"
fi

# 4. 变更与回滚检查
echo "" | tee -a "$REPORT"
echo "--- [变更与回滚] ---" | tee -a "$REPORT"
if command -v argocd &>/dev/null; then
  ARGO_STATUS=$(argocd app list -l namespace="$NAMESPACE" --format json 2>/dev/null | jq -r '.[].status.sync.status' 2>/dev/null)
  if [ -n "$ARGO_STATUS" ]; then
    echo "✅ Argo CD 管理: $ARGO_STATUS" | tee -a "$REPORT"
  fi
fi

HELM_RELEASES=$(helm list -n "$NAMESPACE" --no-headers 2>/dev/null | wc -l)
if [ "$HELM_RELEASES" -gt 0 ]; then
  echo "✅ Helm Release 数: $HELM_RELEASES" | tee -a "$REPORT"
  helm list -n "$NAMESPACE" -o json | jq -r '.[] | "  \(.name) revision=\(.revision) status=\(.status)"' | tee -a "$REPORT"
fi

echo "" | tee -a "$REPORT"
echo "=== 检查完成，报告已保存至: $REPORT ===" | tee -a "$REPORT"
```

### 8.2 容量规划验证

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# 容量规划验证 — 确认资源预留充足

NAMESPACE=${1:-platform-system}

echo "=== 容量规划检查 ==="

# 节点资源余量
echo "--- 节点资源余量 ---"
kubectl top nodes --sort-by=cpu 2>/dev/null | head -5
echo ""
kubectl top nodes --sort-by=memory 2>/dev/null | head -5

# 命名空间资源使用
echo ""
echo "--- 命名空间资源使用 ---"
kubectl top pods -n "$NAMESPACE" --sort-by=cpu 2>/dev/null | head -10

# ResourceQuota 使用率
echo ""
echo "--- ResourceQuota 使用率 ---"
kubectl get resourcequota -n "$NAMESPACE" -o json | jq -r '
  .items[] |
  "\(.metadata.name):",
  (.status.hard // {} | to_entries[] | "  \(.key): used=\(.value)"),
  ""
'

# HPA 状态
echo "--- HPA 状态 ---"
kubectl get hpa -n "$NAMESPACE" -o custom-columns=\
NAME:.metadata.name,\
MIN:.spec.minReplicas,\
MAX:.spec.maxReplicas,\
CURRENT:.status.currentReplicas,\
CPU%:.status.currentMetrics[0].resource.current.averageUtilization
```

## 9. SLI/SLO 验证检查单

### 9.1 上线前 SLO 确认

| SLI 指标 | SLO 目标 | 测量方法 | 当前值 | 状态 |
|----------|----------|----------|--------|------|
| 可用性 | 99.95% | `1 - (error_requests / total_requests)` | ___ | □ |
| P99 延迟 | < 500ms | `histogram_quantile(0.99, ...)` | ___ | □ |
| 错误率 | < 0.1% | `rate(http_requests_total{code=~"5.."}[5m])` | ___ | □ |
| 吐吐量 | > 1000 QPS | `sum(rate(http_requests_total[5m]))` | ___ | □ |
| 恢复时间 (RTO) | < 5min | 故障切换演练计时 | ___ | □ |
| 数据丢失 (RPO) | < 1min | 备份频率与同步延迟 | ___ | □ |

### 9.2 告警规则验证

```bash
# 🟢 低风险：验证告警规则是否生效
# 检查 PrometheusRule 是否被加载
kubectl get prometheusrules -n <ns> -o json | jq -r '
  .items[].spec.groups[].rules[] |
  select(.alert != null) |
  "\(.alert) severity=\(.labels.severity // "none") for=\(.for // "0s")"
'

# 模拟告警触发（通过 promtool）
# promtool test rules /tmp/alert-test.yaml
# 确认 Alertmanager 路由正确
# amtool config routes show
```

## 10. 上线后监控与复盘

### 10.1 上线后 72 小时监控检查单

| 时间点 | 检查项 | 命令/方法 | 结果 |
|--------|--------|----------|------|
| +5min | Pod 全部 Ready | `kubectl get pods -n <ns>` | □ |
| +15min | 无 CrashLoopBackOff | `kubectl get pods -n <ns> \| grep -v Running` | □ |
| +30min | 告警无异常触发 | Alertmanager UI | □ |
| +1h | SLO 指标达标 | Grafana Dashboard | □ |
| +4h | 资源使用稳定 | `kubectl top pods -n <ns>` | □ |
| +24h | 无内存泄漏趋势 | Grafana 内存趋势图 | □ |
| +72h | 复盘会召开 | 会议纪要 | □ |

### 10.2 回滚决策矩阵

| 触发条件 | 决策 | 回滚方式 | 时限 |
|----------|------|----------|------|
| 可用性 < 99.5% 持续 5min | 立即回滚 | `helm rollback` / `argocd app rollback` | 5min 内 |
| P99 延迟 > SLO 3倍 持续 10min | 立即回滚 | 同上 | 5min 内 |
| 错误率 > 5% 持续 3min | 立即回滚 | 同上 | 3min 内 |
| 数据不一致 | 评估后决策 | 回滚 + 数据修复 | 30min 内 |
| 资源使用异常但未影响 SLO | 观察 30min | 扩容/调参 | 1h 内 |
| 单一非关键告警 | 记录并跟踪 | 不回滚 | 下一工作日 |

### 10.3 PRR 评分体系

| 维度 | 权重 | 评分标准 | 得分 |
|------|------|----------|------|
| 架构与高可用 | 25% | 多副本+PDB+反亲和+降级方案 | /25 |
| 可观测性 | 25% | RED指标+告警+Dashboard+日志 | /25 |
| 安全与合规 | 20% | RBAC+NetworkPolicy+Secret+镜像 | /20 |
| 变更与回滚 | 20% | GitOps+版本保留+回滚验证+窗口 | /20 |
| 容量与性能 | 10% | 资源预留+HPA+压测报告 | /10 |
| **总分** | **100%** | **≥ 80 分方可上线** | **/100** |

**评分规则**：
- 90-100：优秀，可直接上线
- 80-89：良好，可上线但需跟踪遗留项
- 70-79：待改进，需补充后重新评审
- < 70：不通过，禁止上线

---

## 11. 相关 Runbook / 推荐阅读

- [[10-平台工程/00-总览/99-production-readiness-operations-guide.md|平台工程生产就绪运维指南]]
- [[10-平台工程/02-运维/99-karpenter-node-autoscaling-guide.md|Karpenter 节点自动扩展实践指南]]
- [[10-平台工程/02-运维/99-keda-event-driven-autoscaling-guide.md|KEDA 事件驱动自动缩放实践指南]]
- [[10-平台工程/02-运维/12-automated-operations-toolchain.md|自动化运维工具链]]
- [[13-生产运维/07-运维手册/02-change-management-guide.md|变更管理指南]]
- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]

---

*本模板应根据组织规模与组件重要性裁剪。高风险组件建议采用更严格的评审流程，并在上线后 30 天内进行首次复盘。*


<!-- risk-assessed -->
