---
title: 基于 SLO 的发布门控
description: 50-75%         正常发布 + 加强监控
category: domain
tags:
- sre
- slo
- release-management
- ci-cd
- gate
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 基于 SLO 的发布门控 是什么
- 如何 基于 SLO 的发布门控
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 基于
- SLO
- 的发布门控
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

# 基于 SLO 的发布门控

## 发布决策矩阵

```
错误预算剩余    发布策略
─────────────────────────────────
> 75%          正常发布
50-75%         正常发布 + 加强监控
25-50%         仅发布关键修复
< 25%          发布冻结（紧急修复除外）
0%             完全冻结
─────────────────────────────────
```

## CI/CD 集成

```yaml
# GitLab CI 示例
slo_gate:
  stage: deploy
  script:
    - |
      BURN_RATE=$(curl -s "$SLO_API/burn_rate?service=order-service")
      if (( $(echo "$BURN_RATE > 0.75" | bc -l) )); then
        echo "❌ 错误预算已消耗超过 75%，发布被拒绝"
        exit 1
      fi
      echo "✅ 错误预算充足，允许发布"
  only:
    - main
```

## 自动化发布策略

```
金丝雀发布 + SLO 监控:
  1. 发布 1% 流量
  2. 监控 SLO 5 分钟
  3. SLO 达标 → 扩大到 10%
  4. 监控 SLO 10 分钟
  5. SLO 达标 → 扩大到 50%
  6. 监控 SLO 15 分钟
  7. SLO 达标 → 全量发布
  8. SLO 不达标 → 自动回滚
```

## 相关

- [[domain-09-reliability-engineering/04-slo-sli/03-error-budget-management]]
- [[domain-09-reliability-engineering/04-slo-sli/04-burn-rate-alerting]]
