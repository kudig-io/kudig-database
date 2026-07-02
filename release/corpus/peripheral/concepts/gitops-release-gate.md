---
title: GitOps 与发布门控的协同
description: 代码提交 → CI 构建 → 安全扫描 → 单元测试 → 集成测试
summary: 代码提交 → CI 构建 → 安全扫描 → 单元测试 → 集成测试
category: synthesis
tags:
- gitops
- release-management
- argocd
- sre
- ci-cd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GitOps 与发布门控的协同 是什么
- 如何 GitOps 与发布门控的协同
trigger_keywords:
- GitOps
- 与发布门控的协同
prerequisites:
- kubectl-basics
relationships:
- target: '[[domain-17-system-foundation/topic-cheat-sheet/gitops.md]]'
  type: related_to
---



# [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|GitOps]] 与发布门控的协同

## 发布流水线

```
代码提交 → CI 构建 → 安全扫描 → 单元测试 → 集成测试
                                              ↓
                                    SLO 预算检查
                                              ↓
                                    ┌───────────────┐
                                    │  Argo CD      │
                                    │  同步到集群   │
                                    └───────────────┘
                                              ↓
                                    金丝雀发布 (1%)
                                              ↓
                                    SLO 验证 (5min)
                                              ↓
                                    ├─ 通过 → 扩大流量
                                    └─ 失败 → 自动回滚
```

## Argo Rollouts 集成

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: order-service
spec:
  strategy:
    canary:
      steps:
      - setWeight: 10
      - pause: {duration: 5m}
      - analysis:
          templates:
          - templateName: slo-check
          args:
          - name: service
            value: order-service
      - setWeight: 50
      - pause: {duration: 10m}
      - analysis:
          templates:
          - templateName: slo-check
      - setWeight: 100
```

## 相关 Domain

- domain-08-release-change-management/01-gitops/01-gitops-principles
- [[domain-09-reliability-engineering/07-sre-practices/02-release-gate-slo-based.md|02 release gate slo based]]
- domain-05-security-compliance/01-security-baseline/01-security-scanning-ci-cd
## Related

- [[entities/argo.md|Argo Workflows]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|Git 速查卡]]
