---
title: 可用区故障恢复手册
description: '□ 监控告警: AZ 级别不可用'
category: domain
tags:
- disaster-recovery
- az-failure
- runbook
- sre
- istio
- hpa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 可用区故障恢复手册 是什么
- 如何 可用区故障恢复手册
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 可用区故障恢复手册
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- service-mesh-basics
created: "2026-05-23"
---

# 可用区故障恢复手册

## 触发条件

```
□ 某个 AZ 的所有 Pod 变为 NotReady
□ 该 AZ 的负载均衡器健康检查全部失败
□ 监控告警: AZ 级别不可用
```

## 恢复流程

### Step 1: 确认问题 (0-2 分钟)

```bash
# 检查 AZ 状态
kubectl get nodes -L topology.kubernetes.io/zone

# 检查 Pod 分布
kubectl get pods -o wide -n production | grep $FAILED_AZ
```

### Step 2: 流量切换 (2-5 分钟)

```bash
# 从负载均衡器中移除问题 AZ
# AWS ALB
aws elbv2 modify-target-group \
  --target-group-arn $TG_ARN \
  --health-check-port 8081  # 指向不可用的端口，强制标记为不健康

# 或使用流量权重调整
# Istio
kubectl apply -f - <<EOT
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: az-dr
spec:
  host: order-service
  trafficPolicy:
    outlierDetection:
      consecutiveErrors: 1
      interval: 10s
      baseEjectionTime: 30s
EOT
```

### Step 3: 扩容健康 AZ (5-10 分钟)

```bash
# HPA 自动扩容
# 或手动调整副本数
kubectl scale deployment/order-service --replicas=30 -n production

# 确保新 Pod 调度到健康 AZ
kubectl get pods -o wide -n production | grep Running
```

### Step 4: 验证恢复 (10-15 分钟)

```bash
# 检查错误率
# 检查 P99 延迟
# 检查业务核心流程
```

### Step 5: 问题 AZ 恢复后

```bash
# 逐步将流量切回
# 恢复原始副本数
# 监控是否稳定
```

## 相关

- [[domain-09-reliability-engineering/09-disaster-recovery-playbooks/01-dr-scenarios-catalog]]
