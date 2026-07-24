---
title: Production Operations Best Practices
description: Production Operations Best Practices — Kubernetes 生产运维知识库
summary: Production Operations Best Practices — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- production
- sre
- operations
- capacity-planning
- change-management
- prometheus
- falco
- rag
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Production Operations Best Practices 是什么
- 如何 Production Operations Best Practices
trigger_keywords:
- Production
- Operations
- Best
- Practices
prerequisites:
- kubectl-basics
- prometheus-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/k8s-production-operations.md|Production Operations]]ernetes 生产环境运维最佳实践字典|Operations Best Practices]]

## Production Readiness Checklist

- [ ] HA architecture deployed (minimum 3 control plane nodes)
- [ ] Complete monitoring and alerting system (99.9% coverage)
- [ ] Regular backup and recovery verification (monthly drills)
- [ ] Security compliance baseline check (CIS benchmark passed)
- [ ] Cost governance mechanism established (budget alerts set)
- [ ] Disaster recovery plan complete (RTO < 4 hours, RPO < 15 minutes)

## SLI/SLO Reference Targets

| Availability Metric | Target |
|--------------------|--------|
| API Server availability | 99.95% |
| Node availability | 99.9% |
| Pod scheduling success rate | 99.5% |

| Performance Metric | Target |
|--------------------|--------|
| API Server P99 latency | < 1 second |
| Pod startup time | < 30 seconds |
| Network latency | < 10ms |

| Capacity Metric | Target |
|----------------|--------|
| Resource utilization | 60-80% |
| Cost deviation | < 10% |

## SRE Practices

**SLI/SLO/Error Budget**: Define Service Level Indicators (what to measure), Service Level Objectives (targets), and Error Budgets (allowed failure time). When error budget is exhausted, halt feature deployments and focus on reliability.

**Blameless Post-Mortems**: After every incident, conduct a blameless post-mortem within 48 hours. Document root cause, timeline, and preventive actions. Update runbooks and detection rules.

**Incident Response Flow**:
1. Detection: Alert from monitoring (Prometheus, Falco, Trivy)
2. Triage: Classify severity (Critical/High/Medium/Low), determine blast radius
3. Containment: Isolate affected workloads, scale down compromised deployments
4. Remediation: Deploy fixes, rotate credentials, update detection rules
5. Post-incident: Blameless post-mortem, document, share lessons learned

## Change Management

RFC (Request for Change) process for production changes:
- Document change scope, risk assessment, rollback plan
- Use gray release / canary deployment for gradual rollout
- Monitor metrics during rollout, auto-rollback on failure
- Post-change verification and documentation

## Capacity Planning and Forecasting

- Monitor resource utilization trends (CPU, memory, storage, network)
- Set utilization alert thresholds (warning at 70%, critical at 85%)
- Plan capacity 3-6 months ahead based on growth trajectory
- Maintain 20% headroom for burst capacity

## 源码实现分析

### SLO 计算与 Error Budget 烧尽告警

```go
// Prometheus SLO 计算示例（multi-window multi-burn-rate）
// 快速烧尽：1小时窗口内错误率 > 14.4x SLO（2% 的 error budget 在 1h 内耗尽）
// 慢速烧尽：6小时窗口内错误率 > 6x SLO

// Alertmanager 告警规则
// slo:api_availability:ratio_rate5m = success_requests / total_requests
// 快速烧尽告警（Critical - 立即响应）
// slo:api_availability:ratio_rate5m < (1 - 14.4 * 0.001) for 2m
// 慢速烧尽告警（Warning - 工作时间内处理）
// slo:api_availability:ratio_rate1h < (1 - 6 * 0.001) for 15m
```

### 生产运维成熟度模型

```
┌──────────────────────────────────────────────────────────┐
│            生产运维成熟度模型 (5级)                    │
├──────────────────────────────────────────────────────────┤
│  L5 │ 自愈    │ AIOps 自动修复、混沌工程常态化        │
│  L4 │ 优化    │ 容量预测、成本优化、自动化变更        │
│  L3 │ 标准化  │ SLO体系、变更管理、定期演练          │
│  L2 │ 可观测  │ 完整监控告警、日志集中、链路追踪    │
│  L1 │ 被动    │ 故障后响应、手动操作、无文档        │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：生产环境健康巡检

```bash
# 🟢 低风险：只读巡检
# 控制面健康
kubectl get componentstatuses 2>/dev/null || kubectl get --raw='/healthz?verbose'
# 节点状态
kubectl get nodes -o wide | grep -v Ready  # 检查非 Ready 节点
kubectl top nodes --sort-by=cpu | head -5   # CPU 使用率 Top5
# 工作负载健康
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded
# 资源利用率
kubectl top pods -A --sort-by=memory | head -10
# PVC 容量
kubectl get pvc -A -o json | jq '.items[] | select(.status.phase!="Bound")'
```

### 场景二：变更发布流程（金丝雀）

```bash
# 🟡 中风险：生产发布
# Step 1: 金丝雀发布 10% 流量
kubectl set image deployment/api api=registry/api:v2.1.0 -n production
kubectl rollout status deployment/api -n production --timeout=300s
# Step 2: 观察关键指标 15分钟
# - 错误率 < 0.1%
# - P99 延迟 < 500ms
# - 无新增 CrashLoopBackOff
# Step 3: 异常时立即回滚
kubectl rollout undo deployment/api -n production  # 🔴 回滚操作
# Step 4: 确认回滚成功
kubectl rollout status deployment/api -n production
```

### 场景三：容量规划与资源分析

```bash
# 🟢 低风险：只读分析
# 集群整体资源利用率
kubectl top nodes -o json | jq '[.items[] | .usage.cpu[:-1] | tonumber] | add'
# 命名空间资源配额使用情况
kubectl get resourcequota -A -o custom-columns=NS:.metadata.namespace,CPU-USED:.status.used.requests\\.cpu,CPU-LIMIT:.status.hard.requests\\.cpu
# 识别资源浪费（request 远大于实际使用）
kubectl top pods -A --sort-by=cpu -o json | jq '.items[:10]'
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | 100% 可用性是目标 | 100% 不现实且成本极高；99.95% 已允许 ~4.4h/年停机，根据业务选择 |
| 2 | 告警越多越安全 | 告警风暴导致“告警笫劳”；只告警可操作的事件，分级处理 |
| 3 | 变更窗口内就安全 | 变更窗口只是降低影响；仍需金丝雀+自动回滚+监控 |
| 4 | 备份 = 可恢复 | 备份必须定期验证恢复（月度演练）；未验证的备份等于没有 |
| 5 | 资源利用率越高越好 | >85% 利用率无 burst 空间，突发流量导致 OOM/调度失败；保持 60-80% |
| 6 | 故障复盘是追责 | Blameless Post-Mortem 目的是改进系统，而非追责个人 |

## 面试要点

1. **Q: SLO/Error Budget 如何驱动工程决策？**
   A: SLO 定义可靠性目标（如 99.95%）；Error Budget = 1 - SLO = 0.05%（允许 ~22min/月不可用）。当 budget 充足时可快速发布新功能；当 budget 耗尽时冻结发布、专注可靠性。Multi-window multi-burn-rate 告警避免误报：快速烧尽（1h, 14.4x）立即响应，慢速烧尽（6h, 6x）工作时间内处理。

2. **Q: 生产环境变更管理的核心原则？**
   A: ① 可观测：变更前后关键指标对比；② 可回滚：每次变更必须有回滚方案（< 5min 完成）；③ 渐进式：金丝雀 10% → 50% → 100%；④ 可审计：所有变更记录 who/what/when/why；⑤ 时间窗口：避开业务高峰、周五下午、大促前。

3. **Q: 如何设计一个完整的生产就绪检查清单？**
   A: 六大维度：① 高可用（控制面 3 节点、工作负载 ≥2 副本、PDB、反亲和）；② 可观测（指标/日志/追踪 100% 覆盖、SLO 定义）；③ 安全（CIS Benchmark、镜像扫描、RBAC 审计）；④ 备份恢复（etcd 每日快照、恢复演练）；⑤ 容量（资源配额、HPA、预留 20% headroom）；⑥ 变更管理（GitOps、金丝雀、自动回滚）。

4. **Q: 故障响应中的“止血”与“根治”如何平衡？**
   A: 优先止血（恢复服务）：回滚、扩容、限流、降级、切流。止血后再根治：分析根因、修复代码、补充测试、更新告警。关键原则：① 止血操作必须简单可靠（回滚 > 热修）；② 保留现场证据（日志/快照）供后续分析；③ 止血后 48h 内完成 Post-Mortem。

## Related

- radius — radius
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/microservice-resilience-patterns.md|microservice-resilience-patterns]] — Microservice Resilience Patterns
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/gitops-principles.md|GitOps Principles]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[概念/microservice-resilience-patterns.md|Microservice Resilience Patterns]]
- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[技能/工作负载/pod/方法论/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[技能/工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[技能/工作负载/pod/方法论/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]
- [[技能/工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]

- 17-production-operations-best-practices

<!-- risk-assessed -->
