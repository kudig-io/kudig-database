---
title: 可用区故障恢复手册
description: '□ 监控告警: AZ 级别不可用'
summary: '□ 监控告警: AZ 级别不可用'
category: domain
tags:
- disaster-recovery
- az-failure
- runbook
- sre
- istio
- hpa
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 可用区故障恢复手册

## 触发条件

```
# 🟢 低风险：只读/信息收集，通常无副作用
□ 某个 AZ 的所有 Pod 变为 NotReady
□ 该 AZ 的负载均衡器健康检查全部失败
□ 监控告警: AZ 级别不可用
```
## 恢复流程

### Step 1: 确认问题 (0-2 分钟)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 AZ 状态
kubectl get nodes -L topology.kubernetes.io/zone

# 检查 Pod 分布
kubectl get pods -o wide -n production | grep $FAILED_AZ
```
### Step 2: 流量切换 (2-5 分钟)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

## 检测与告警配置

### PrometheusRule AZ 故障告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: az-failure-alerts
  namespace: monitoring
spec:
  groups:
    - name: az.failure.rules
      rules:
        # 单 AZ 多节点 NotReady
        - alert: AZMultipleNodesNotReady
          expr: |
            count by (topology.kubernetes.io/zone) (
              kube_node_status_condition{condition="Ready", status="false"} == 1
            ) >= 2
          for: 2m
          labels:
            severity: critical
            scenario: az-failure
          annotations:
            summary: "🚨 可用区 {{ $labels.topology_kubernetes_io_zone }} 多个节点 NotReady"
            runbook: "https://runbooks.example.com/az-failure"

        # AZ 内 Pod 大量失败
        - alert: AZPodFailureSpike
          expr: |
            count by (topology.kubernetes.io/zone) (
              kube_pod_status_phase{phase="Failed"} == 1
            ) > 10
          for: 1m
          labels:
            severity: critical
            scenario: az-failure
          annotations:
            summary: "可用区 {{ $labels.topology_kubernetes_io_zone }} Pod 失败数量激增"

        # AZ 内负载均衡器健康检查失败
        - alert: AZLoadBalancerUnhealthy
          expr: |
            aws_alb_target_group_unhealthy_host_count > 5
          for: 2m
          labels:
            severity: critical
            scenario: az-failure
          annotations:
            summary: "可用区 {{ $labels.zone }} 负载均衡器健康检查失败"

        # 跨 AZ 延迟异常
        - alert: CrossAZLatencyHigh
          expr: |
            histogram_quantile(0.99,
              rate(istio_request_duration_milliseconds_bucket{source_workload_namespace="production"}[5m])
            ) > 1000
          for: 5m
          labels:
            severity: warning
            scenario: az-degradation
          annotations:
            summary: "跨 AZ 延迟异常升高"
```

### 检测脚本

```bash
#!/bin/bash
# 🟢 低风险：AZ 健康检测脚本
set -euo pipefail

echo "=== AZ 健康检测 $(date) ==="

# 1. 检查各 AZ 节点状态
echo "[1] 节点状态:"
kubectl get nodes -L topology.kubernetes.io/zone --no-headers | \
  awk '{print $NF}' | sort | uniq -c | while read count zone; do
    NOT_READY=$(kubectl get nodes -l topology.kubernetes.io/zone=$zone --no-headers | grep -c NotReady || true)
    echo "  $zone: $count 节点, $NOT_READY NotReady"
  done

# 2. 检查各 AZ Pod 分布
echo "[2] Pod 分布:"
kubectl get pods -n production -o wide --no-headers | \
  awk '{print $7}' | cut -d'-' -f1-2 | sort | uniq -c

# 3. 检查各 AZ 服务可用性
echo "[3] 服务可用性:"
for az in az1 az2 az3; do
  HEALTHY=$(kubectl get pods -n production -o wide --no-headers | \
    grep $az | grep -c Running || true)
  TOTAL=$(kubectl get pods -n production -o wide --no-headers | grep -c $az || true)
  echo "  $az: $HEALTHY/$TOTAL Running"
done

echo "=== 检测完成 ==="
```

## 自动化恢复脚本

### 完整恢复脚本

```bash
#!/bin/bash
# 🔴 高风险：AZ 故障自动恢复脚本
set -euo pipefail

FAILED_AZ=${1:?"Usage: $0 <failed-az>"}
NAMESPACE=${2:-production}

echo "=== AZ 故障恢复: $FAILED_AZ ==="

# 1. 确认故障
echo "[1] 确认故障..."
NOT_READY=$(kubectl get nodes -l topology.kubernetes.io/zone=$FAILED_AZ --no-headers | grep -c NotReady || true)
if [ "$NOT_READY" -eq 0 ]; then
  echo "✓ $FAILED_AZ 节点正常，无需恢复"
  exit 0
fi
echo "  检测到 $NOT_READY 个 NotReady 节点"

# 2. 流量切换
echo "[2] 流量切换..."
# 使用 Istio DestinationRule 排除故障 AZ
kubectl apply -f - <<EOF
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: az-failover-$FAILED_AZ
  namespace: $NAMESPACE
spec:
  host: "*.svc.cluster.local"
  trafficPolicy:
    outlierDetection:
      consecutiveErrors: 1
      interval: 10s
      baseEjectionTime: 300s
      maxEjectionPercent: 100
    loadBalancer:
      localityLbSetting:
        enabled: true
        failover:
          - from: $FAILED_AZ
            to: $(kubectl get nodes -L topology.kubernetes.io/zone --no-headers | awk '{print $NF}' | grep -v $FAILED_AZ | head -1)
EOF

# 3. 扩容健康 AZ
echo "[3] 扩容健康 AZ..."
for deploy in $(kubectl get deploy -n $NAMESPACE -o name); do
  CURRENT=$(kubectl get $deploy -n $NAMESPACE -o jsonpath='{.spec.replicas}')
  TARGET=$((CURRENT * 3 / 2))  # 扩容 50%
  kubectl scale $deploy -n $NAMESPACE --replicas=$TARGET
  echo "  $deploy: $CURRENT → $TARGET"
done

# 4. 等待 Pod 就绪
echo "[4] 等待 Pod 就绪..."
for deploy in $(kubectl get deploy -n $NAMESPACE -o name); do
  kubectl rollout status $deploy -n $NAMESPACE --timeout=300s
done

# 5. 验证
echo "[5] 验证恢复..."
sleep 30
ERROR_RATE=$(curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))' | jq -r '.data.result[0].value[1]')
echo "  当前错误率: $ERROR_RATE"

echo "=== 恢复完成 ==="
```

### 回切脚本

```bash
#!/bin/bash
# 🟡 中风险：AZ 恢复后回切脚本
set -euo pipefail

RECOVERED_AZ=${1:?"Usage: $0 <recovered-az>"}
NAMESPACE=${2:-production}

echo "=== AZ 回切: $RECOVERED_AZ ==="

# 1. 验证 AZ 健康
echo "[1] 验证 AZ 健康..."
NOT_READY=$(kubectl get nodes -l topology.kubernetes.io/zone=$RECOVERED_AZ --no-headers | grep -c NotReady || true)
if [ "$NOT_READY" -gt 0 ]; then
  echo "❌ $RECOVERED_AZ 仍有 $NOT_READY 个 NotReady 节点，中止回切"
  exit 1
fi
echo "  ✓ 所有节点 Ready"

# 2. 逐步恢复流量
echo "[2] 逐步恢复流量..."
# 先恢复 10% 流量
kubectl patch destinationrule az-failover-$RECOVERED_AZ -n $NAMESPACE --type='json' \
  -p='[{"op": "replace", "path": "/spec/trafficPolicy/outlierDetection/maxEjectionPercent", "value": 90}]'
sleep 60

# 检查错误率
ERROR_RATE=$(curl -s 'http://prometheus:9090/api/v1/query?query=sum(rate(http_requests_total{status=~"5.."}[1m]))/sum(rate(http_requests_total[1m]))' | jq -r '.data.result[0].value[1]')
if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
  echo "❌ 错误率异常，回滚回切"
  kubectl patch destinationrule az-failover-$RECOVERED_AZ -n $NAMESPACE --type='json' \
    -p='[{"op": "replace", "path": "/spec/trafficPolicy/outlierDetection/maxEjectionPercent", "value": 100}]'
  exit 1
fi

# 3. 完全恢复
echo "[3] 完全恢复..."
kubectl delete destinationrule az-failover-$RECOVERED_AZ -n $NAMESPACE

# 4. 恢复原始副本数
echo "[4] 恢复原始副本数..."
# 根据记录恢复

echo "=== 回切完成 ==="
```

## 数据层恢复

### 数据库 AZ 故障处理

```bash
# 🟢 低风险：检查数据库状态
kubectl exec -n database sts/postgres-0 -- pg_isready
kubectl exec -n database sts/postgres-0 -- psql -c "SELECT pg_is_in_recovery();"

# 检查复制延迟
kubectl exec -n database sts/postgres-0 -- psql -c \
  "SELECT client_addr, state, sent_lsn, replay_lsn FROM pg_stat_replication;"

# 🟡 中风险：如果主库在故障 AZ，触发主从切换
# 1. 确认从库状态
kubectl exec -n database sts/postgres-1 -- psql -c "SELECT pg_is_in_recovery();"

# 2. 提升从库为主库
kubectl exec -n database sts/postgres-1 -- pg_ctl promote

# 3. 更新 Service 指向
kubectl patch svc postgres-primary -n database -p \
  '{"spec":{"selector":{"statefulset.kubernetes.io/pod-name":"postgres-1"}}}'
```

### 缓存 AZ 故障处理

```bash
# 🟢 低风险：检查 Redis 状态
kubectl exec -n cache sts/redis-0 -- redis-cli ping
kubectl exec -n cache sts/redis-0 -- redis-cli info replication

# 🟡 中风险：如果主节点在故障 AZ
# 1. 检查从节点
kubectl exec -n cache sts/redis-1 -- redis-cli info replication

# 2. 手动故障转移
kubectl exec -n cache sts/redis-1 -- redis-cli replicaof no one

# 3. 更新其他从节点
kubectl exec -n cache sts/redis-2 -- redis-cli replicaof redis-1.redis.cache.svc 6379
```

## 验证检查清单

### 恢复后验证

| 序号 | 检查项 | 验证命令 | 通过标准 |
|-----|--------|---------|----------|
| 1 | 所有 Pod Running | `kubectl get pods -n production` | 无 Pending/Failed |
| 2 | 错误率正常 | 检查 Prometheus | < 1% |
| 3 | 延迟正常 | 检查 P99 | < 500ms |
| 4 | 数据库连接正常 | 检查应用日志 | 无连接错误 |
| 5 | 缓存命中率正常 | 检查 Redis 指标 | > 90% |
| 6 | 负载均衡器健康 | 检查 AWS ALB | 所有目标健康 |
| 7 | 业务核心流程 | 手动测试 | 支付/下单正常 |

### 回切后验证

| 序号 | 检查项 | 验证命令 | 通过标准 |
|-----|--------|---------|----------|
| 1 | 流量分布均衡 | 检查各 AZ 流量 | 比例正常 |
| 2 | 副本数恢复 | `kubectl get deploy` | 与故障前一致 |
| 3 | 无残留配置 | 检查 DestinationRule | 已清理 |
| 4 | 监控告警正常 | 检查 Prometheus | 无异常告警 |

## 预防措施

### 拓扑分布约束

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: critical-service
spec:
  replicas: 6
  template:
    spec:
      topologySpreadConstraints:
        # 跨 AZ 均匀分布
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: critical-service
        # 跨节点分布
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app: critical-service
```

### PodDisruptionBudget

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-service-pdb
spec:
  minAvailable: 4  # 至少 4 个可用（总共 6 个，跨 3 AZ）
  selector:
    matchLabels:
      app: critical-service
```

## 演练指南

### AZ 故障演练步骤

```bash
# 🟡 中风险：AZ 故障演练（使用 Chaos Mesh）
# 1. 创建实验
kubectl apply -f - <<EOF
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: az-failure-drill
  namespace: production
spec:
  action: pod-kill
  mode: all
  selector:
    namespaces:
      - production
    labelSelectors:
      app: critical-service
    expressionSelectors:
      - key: topology.kubernetes.io/zone
        operator: In
        values:
          - az1  # 模拟 az1 故障
  duration: "300s"
EOF

# 2. 观察恢复
kubectl get pods -n production -l app=critical-service -w

# 3. 清理
kubectl delete podchaos az-failure-drill -n production
```

## 相关

- [[可靠性/灾难恢复/01-dr-scenarios-catalog.md|01 dr scenarios catalog]]


<!-- risk-assessed -->
