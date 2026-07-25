---
title: 渐进式交付模式
summary: 研究 Kubernetes 上的渐进式交付策略（金丝雀、蓝绿、A/B 测试、Feature Flag），对比 Argo Rollouts、Flagger、Istio 流量管理的生产实践。
category: research
tags:
- research
- progressive-delivery
- canary
- argo-rollouts
- flagger
tier: supporting
created: '2026-07-21'
updated: '2026-07-21'
last_updated: '2026-07-21'
status: done
---

# 渐进式交付模式

## 研究背景

传统的滚动更新（RollingUpdate）缺乏流量控制和自动回滚能力。渐进式交付通过逐步将流量切换到新版本，结合自动化指标分析，实现零停机、低风险的应用发布。

## 核心问题

1. 金丝雀、蓝绿、A/B 测试各适用什么场景？
2. Argo Rollouts vs Flagger 的架构差异和选型？
3. 如何定义自动回滚的指标阈值？
4. 渐进式交付如何与 GitOps 流水线集成？

## 调研发现

### 发现一：交付策略对比

| 策略 | 流量切换 | 回滚速度 | 资源开销 | 适用场景 |
|------|----------|----------|----------|----------|
| 滚动更新 | 逐步替换 Pod | 慢 (重新部署) | 低 | 无状态、低风险 |
| 蓝绿 | 一次性切换 | 即时 (切回) | 2x 资源 | 需要即时回滚 |
| 金丝雀 | 渐进式 (5%→25%→100%) | 快 (流量切回) | 低 | 高风险变更 |
| A/B 测试 | 按用户特征路由 | 快 | 低 | 功能验证 |
| Feature Flag | 运行时开关 | 即时 | 无 | 功能灰度 |

### 发现二：Argo Rollouts vs Flagger

| 维度 | Argo Rollouts | Flagger |
|------|---------------|---------|
| 实现方式 | 自定义 CRD (Rollout) | 注解现有 Deployment |
| 流量管理 | Istio/Nginx/ALB/SMI | Istio/Linkerd/Nginx/Gateway API |
| 分析集成 | Prometheus/Datadog/自定义 | Prometheus/自定义 Webhook |
| 实验支持 | 支持 (Experiment) | 支持 (Canary) |
| GitOps 集成 | ArgoCD 原生 | 独立运行 |
| 学习曲线 | 中等 | 低 |

### 发现三：自动回滚指标设计

| 指标类型 | 示例 | 阈值建议 |
|---------|------|----------|
| 错误率 | HTTP 5xx 比例 | > 1% 回滚 |
| 延迟 | P99 响应时间 | > 500ms 回滚 |
| 成功率 | 业务成功率 | < 99% 回滚 |
| 资源 | CPU/内存异常 | > 2x 基线回滚 |
| 自定义 | 业务指标 | 按业务定义 |

## 落地方案

### 推荐架构

```
Git Push → ArgoCD Sync → Argo Rollout
                              │
                    ┌─────────┼─────────┐
                    ▼         ▼         ▼
               Step 1     Step 2    Step 3
               (5%)      (25%)     (100%)
                    │         │         │
                    ▼         ▼         ▼
              Prometheus Analysis (每步验证)
                    │
                    ▼ (失败)
              Auto Rollback
```

## 参考资源

- [Argo Rollouts](https://argoproj.github.io/argo-rollouts/)
- [Flagger](https://docs.flagger.app/)
- [Progressive Delivery](https://redmonk.com/jgovernor/2018/08/06/towards-progressive-delivery/)

## Related Tags

- [[27-标签/gitops|gitops]]
- [[27-标签/production|production]]
- [[27-标签/best-practices|best-practices]]
