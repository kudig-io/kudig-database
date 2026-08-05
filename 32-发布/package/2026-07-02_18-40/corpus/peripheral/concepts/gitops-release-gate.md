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
- target: '[[domain-17-system-foundation/速查卡/gitops.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[domain-17-system-foundation/速查卡/gitops.md|GitOps]] 与发布门控的协同

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
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-09-reliability-engineering/04-sre-practices/01-release-gate-slo-based|02 release gate slo based]]
- domain-05-security-compliance/01-security-baseline/01-security-scanning-ci-cd
## Related

- [[entities/argo.md|Argo Workflows]]
- [[domain-17-system-foundation/速查卡/git.md|Git 速查卡]]


<!-- risk-assessed -->
