---
title: HPA/VPA 故障排查
description: '# 17 - HPA/VPA 故障排查 (HPA/VPA Troubleshooting)'
summary: 'kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.conditions}''
category: troubleshooting
tags:
- hpa
- vpa
- autoscaling
- metrics-server
- replica
- controller-manager
- prometheus
- istio
- pdb
- statefulset
tier: core
created: '2026-05-23'
last_updated: '2026-07-02'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- HPA 不扩缩容
- VPA 不更新资源
- Metrics Server 不工作
- 副本数不变
trigger_keywords:
- HPA
- VPA
- 故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- cni-basics
- etcd-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/hpa-fta.md
  label: '故障树: hpa'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 17 - HPA/VPA 故障排查 (HPA/VPA Troubleshooting)

---

<!-- chunk: 1. 自动扩缩容故障诊断总览 (Autoscaling Diagnosis Overview) -->
## 1. 自动扩缩容故障诊断总览 (Autoscaling Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **HPA不触发扩缩容** | 副本数不变化 | 资源利用率不合理 | P2 - 中 |
| **VPA不更新资源** | Request/Limit不调整 | 资源浪费/不足 | P2 - 中 |
| **指标获取失败** | Metrics Server异常 | 所有自动扩缩容失效 | P0 - 紧急 |
| **扩缩容震荡** | 频繁扩缩容 | 系统不稳定 | P1 - 高 |
| **配置冲突** | HPA/VPA同时作用 | 行为不可预测 | P1 - 高 |

### 1.2 自动扩缩容架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   HPA/VPA 故障诊断架构                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        监控指标收集层                                 │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │ Metrics API │    │ Custom      │    │ External    │              │  │
│  │  │   Server    │    │ Metrics     │    │ Metrics     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │    CPU/Mem   │   │ Application  │   │   Business   │                   │
│  │   Metrics    │   │   Metrics    │   │   Metrics    │                   │
│  │ (内置指标)   │   │ (自定义指标) │   │ (外部指标)   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    自动扩缩容控制器                                  │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   HPA       │    │    VPA      │    │  Cluster    │              │  │
│  │  │ Controller  │    │ Controller  │    │ Autoscaler  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Deployment│    │ StatefulSet │    │   DaemonSet │                   │
│  │   (Replica) │    │  (Replica)  │    │   (Nodes)   │                   │
│  │   调整      │    │   调整      │    │   调整      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Pod实例层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-1     │    │   Pod-2     │    │   Pod-3     │              │  │
│  │  │ CPU: 60%    │    │ Mem: 70%    │    │ CPU: 40%    │              │  │
│  │  │ RAM: 512MB  │    │ CPU: 200m   │    │ RAM: 256MB  │              │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. HPA故障排查 (HPA Troubleshooting) -->
## 2. HPA故障排查 (HPA Troubleshooting)

### 2.1 HPA状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 基础状态检查 ==========
# 查看所有HPA资源
kubectl get hpa --all-namespaces

# 查看详细状态
kubectl describe hpa <hpa-name> -n <namespace>

# 检查HPA条件状态
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.conditions}'

# ========== 2. 指标获取检查 ==========
# 检查Metrics Server状态
kubectl get pods -n kube-system | grep metrics-server
kubectl logs -n kube-system -l k8s-app=metrics-server --tail=50

# 验证指标API可用性
kubectl top nodes
kubectl top pods --all-namespaces

# 检查特定Pod指标
kubectl get --raw "/apis/metrics.k8s.io/v1beta1/namespaces/<namespace>/pods/<pod-name>"
```
### 2.2 HPA配置验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 配置检查 ==========
# 查看HPA配置详情
kubectl get hpa <hpa-name> -n <namespace> -o yaml

# 检查目标资源是否存在
kubectl get deploy,statefulset,replicaset -n <namespace> | grep <target-name>

# 验证资源请求设置
kubectl get deploy <deployment-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}'

# ========== 常见配置问题检查 ==========
# 检查最小/最大副本数
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.minReplicas}{" "}{.spec.maxReplicas}'

# 检查指标类型配置
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[*].type}'

# 检查目标值配置
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[*].resource.target}'
```
### 2.3 HPA不工作常见原因

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 指标类型问题 ==========
# Resource指标检查 (CPU/Memory)
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[?(@.type=="Resource")].resource}'

# Pods指标检查 (自定义Pod指标)
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[?(@.type=="Pods")].pods}'

# Object指标检查 (其他对象指标)
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[?(@.type=="Object")].object}'

# External指标检查 (外部指标)
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[?(@.type=="External")].external}'

# ========== 2. 权限问题检查 ==========
# 检查HPA控制器权限
kubectl auth can-i get pods --as=system:serviceaccount:kube-system:horizontal-pod-autoscaler

# 检查Metrics Server权限
kubectl auth can-i get pods/metrics --as=system:serviceaccount:kube-system:metrics-server

# ========== 3. 时间窗口问题 ==========
# 检查HPA同步周期
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.metadata.annotations.autoscaling\.alpha\.kubernetes\.io/conditions}'

# 默认同步周期为15-30秒，可通过以下参数调整:
# --horizontal-pod-autoscaler-sync-period
# --horizontal-pod-autoscaler-downscale-stabilization
```
---

<!-- chunk: 3. VPA故障排查 (VPA Troubleshooting) -->
## 3. VPA故障排查 (VPA Troubleshooting)

### 3.1 VPA组件状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. VPA组件检查 ==========
# 检查VPA相关Pod
kubectl get pods -n kube-system | grep vpa

# 检查VPA Admission Controller
kubectl get pods -n kube-system | grep vpa-admission-controller

# 检查VPA Recommender
kubectl get pods -n kube-system | grep vpa-recommender

# 检查VPA Updater
kubectl get pods -n kube-system | grep vpa-updater

# ========== 2. VPA资源配置检查 ==========
# 查看VPA资源
kubectl get vpa --all-namespaces

# 查看详细VPA状态
kubectl describe vpa <vpa-name> -n <namespace>

# 检查VPA条件状态
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.status.conditions}'
```
### 3.2 VPA配置验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== VPA配置检查 ==========
# 查看VPA完整配置
kubectl get vpa <vpa-name> -n <namespace> -o yaml

# 检查更新模式
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.updatePolicy.updateMode}'

# 检查资源策略
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.resourcePolicy}'

# 检查目标引用
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.targetRef}'

# ========== 推荐值检查 ==========
# 查看VPA推荐的资源值
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.status.recommendation.containerRecommendations}'

# 检查历史推荐值
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.status.recommendation.containerRecommendations[*].lowerBound}'

# 检查推荐的时间戳
kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.status.recommendation.containerRecommendations[*].lastRecommendationTime}'
```
### 3.3 VPA不工作常见原因

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 更新模式问题 ==========
# 检查更新模式配置
UPDATE_MODE=$(kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.updatePolicy.updateMode}')
echo "Update Mode: $UPDATE_MODE"

# 不同模式的行为:
# "Auto" - 自动更新Pod
# "Initial" - 只在创建时应用
# "Off" - 不更新，仅提供推荐

# ========== 2. 资源冲突检查 ==========
# 检查是否存在资源配额限制
kubectl get resourcequota -n <namespace>

# 检查LimitRange配置
kubectl get limitrange -n <namespace>

# 检查Pod Disruption Budget
kubectl get pdb -n <namespace>

# ========== 3. 控制器兼容性 ==========
# 检查工作负载控制器类型
WORKLOAD_TYPE=$(kubectl get vpa <vpa-name> -n <namespace> -o jsonpath='{.spec.targetRef.kind}')
echo "Target Workload Type: $WORKLOAD_TYPE"

# VPA支持的工作负载:
# Deployment, StatefulSet, ReplicaSet, ReplicationController
# 不支持: DaemonSet, Job, CronJob
```
---

<!-- chunk: 4. 指标相关问题 (Metrics Issues) -->
## 4. 指标相关问题 (Metrics Issues)

### 4.1 Metrics Server故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. Metrics Server状态检查 ==========
# 检查Metrics Server Pod状态
kubectl get pods -n kube-system | grep metrics-server

# 查看Metrics Server日志
kubectl logs -n kube-system -l k8s-app=metrics-server --tail=100

# 检查Metrics Server服务
kubectl get svc -n kube-system | grep metrics-server

# ========== 2. 指标API验证 ==========
# 验证API组可用性
kubectl api-versions | grep metrics

# 测试节点指标API
kubectl get --raw "/apis/metrics.k8s.io/v1beta1/nodes" | jq .

# 测试Pod指标API
kubectl get --raw "/apis/metrics.k8s.io/v1beta1/pods" | jq .

# ========== 3. 常见Metrics Server问题 ==========
# 证书问题
kubectl get pods -n kube-system -l k8s-app=metrics-server -o jsonpath='{.items[*].status.containerStatuses[*].state.waiting.reason}'

# 权限问题
kubectl auth can-i get nodes/metrics --as=system:serviceaccount:kube-system:metrics-server

# 网络策略阻断
kubectl get networkpolicy -n kube-system
```
### 4.2 自定义指标配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== Custom Metrics Adapter检查 ==========
# 检查Custom Metrics API服务
kubectl get apiservice | grep custom.metrics

# 检查Custom Metrics Adapter Pod
kubectl get pods -n monitoring | grep adapter

# 验证自定义指标可用性
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta2/" | jq .

# ========== 指标适配器配置 ==========
# Prometheus Adapter配置检查
kubectl get cm -n monitoring adapter-config -o yaml

# 检查指标规则配置
kubectl get cm -n monitoring adapter-config -o jsonpath='{.data.config\.yaml}' | yq

# 验证特定指标存在
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta2/namespaces/<namespace>/pods/*/http_requests_per_second" | jq .
```
---

<!-- chunk: 5. 扩缩容行为分析 (Scaling Behavior Analysis) -->
## 5. 扩缩容行为分析 (Scaling Behavior Analysis)

### 5.1 扩缩容算法验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== HPA计算过程分析 ==========
# 获取HPA详细信息
kubectl get hpa <hpa-name> -n <namespace> -o json

# 检查当前指标值
CURRENT_METRIC=$(kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.currentMetrics[0].resource.current.averageUtilization}')
TARGET_METRIC=$(kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.spec.metrics[0].resource.target.averageUtilization}')

echo "Current Utilization: $CURRENT_METRIC%"
echo "Target Utilization: $TARGET_METRIC%"

# 计算期望副本数
CURRENT_REPLICAS=$(kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.currentReplicas}')
DESIRED_REPLICAS=$(kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.desiredReplicas}')

echo "Current Replicas: $CURRENT_REPLICAS"
echo "Desired Replicas: $DESIRED_REPLICAS"

# ========== 扩缩容阈值检查 ==========
# 检查扩缩容延迟配置
kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.metadata.annotations.autoscaling\.alpha\.kubernetes\.io/conditions}' | jq

# 默认行为:
# - 扩容: 立即触发
# - 缩容: 默认稳定窗口5分钟
# - 可通过以下注解调整:
#   autoscaling.alpha.kubernetes.io/tolerance: "0.1"  # 容忍度
#   autoscaling.alpha.kubernetes.io/downscale-stabilization: "5m"  # 缩容稳定期
```
### 5.2 扩缩容震荡问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 震荡检测 ==========
# 监控副本数变化
watch -n 5 "kubectl get hpa <hpa-name> -n <namespace> -o jsonpath='{.status.currentReplicas} {.status.desiredReplicas}'"

# 查看HPA事件历史
kubectl get events -n <namespace> --field-selector involvedObject.name=<hpa-name>,involvedObject.kind=HorizontalPodAutoscaler

# 分析指标波动
kubectl get --raw "/apis/metrics.k8s.io/v1beta1/namespaces/<namespace>/pods" | jq '.items[] | select(.metadata.name | startswith("<pod-prefix>")) | .containers[0].usage.cpu'

# ========== 震荡缓解策略 ==========
# 调整容忍度
kubectl patch hpa <hpa-name> -n <namespace> -p '{"metadata":{"annotations":{"autoscaling.alpha.kubernetes.io/tolerance":"0.2"}}}'

# 增加稳定窗口
kubectl patch hpa <hpa-name> -n <namespace> -p '{"metadata":{"annotations":{"autoscaling.alpha.kubernetes.io/downscale-stabilization":"10m"}}}'

# 设置合理的最小/最大副本数差距
kubectl patch hpa <hpa-name> -n <namespace> -p '{"spec":{"minReplicas":3,"maxReplicas":10}}'
```
---

<!-- chunk: 6. 监控和告警配置 (Monitoring and Alerting) -->
## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 关键指标监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== HPA监控指标 ==========
# HPA状态指标
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: hpa-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-controller-manager
  endpoints:
  - port: http-metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'horizontalpodautoscaler.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: autoscaling-alerts
  namespace: monitoring
spec:
  groups:
  - name: autoscaling.rules
    rules:
    - alert: HPANotWorking
      expr: kube_horizontalpodautoscaler_status_condition{condition="ScalingActive",status!="true"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "HPA is not scaling (namespace {{ \$labels.namespace }} deployment {{ \$labels.horizontalpodautoscaler }})"
        
    - alert: VPANotUpdating
      expr: kube_verticalpodautoscaler_status_recommendation_total{container!=""} == 0
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "VPA is not providing recommendations (namespace {{ \$labels.namespace }} target {{ \$labels.verticalpodautoscaler }})"
        
    - alert: ScalingStorm
      expr: increase(kube_horizontalpodautoscaler_status_desired_replicas[5m]) > 5
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Excessive scaling activity detected"
        
    - alert: MetricsServerDown
      expr: up{job="metrics-server"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Metrics Server is down"
EOF
```
### 6.2 性能分析工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 扩缩容性能分析 ==========
# 创建性能测试脚本
cat <<'EOF' > autoscaling-perf-test.sh
#!/bin/bash

HPA_NAME=$1
NAMESPACE=$2
TEST_DURATION=${3:-300}  # 默认5分钟

echo "Starting autoscaling performance test for $HPA_NAME in $NAMESPACE"
echo "Test duration: $TEST_DURATION seconds"

# 记录初始状态
INITIAL_REPLICAS=$(kubectl get hpa $HPA_NAME -n $NAMESPACE -o jsonpath='{.status.currentReplicas}')
echo "Initial replicas: $INITIAL_REPLICAS"

# 持续监控
END_TIME=$(($(date +%s) + TEST_DURATION))
while [ $(date +%s) -lt $END_TIME ]; do
    CURRENT=$(kubectl get hpa $HPA_NAME -n $NAMESPACE -o jsonpath='{.status.currentReplicas}')
    DESIRED=$(kubectl get hpa $HPA_NAME -n $NAMESPACE -o jsonpath='{.status.desiredReplicas}')
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    echo "$TIMESTAMP - Current: $CURRENT, Desired: $DESIRED"
    
    # 检查是否有扩缩容活动
    if [ "$CURRENT" != "$INITIAL_REPLICAS" ]; then
        echo "$TIMESTAMP - Scaling activity detected!"
        INITIAL_REPLICAS=$CURRENT
    fi
    
    sleep 10
done

echo "Test completed"
EOF

chmod +x autoscaling-perf-test.sh

# ========== 资源利用率分析 ==========
# 分析Pod资源使用情况
cat <<'EOF' > resource-utilization-analyzer.sh
#!/bin/bash

NAMESPACE=${1:-default}

echo "Analyzing resource utilization in namespace: $NAMESPACE"

# 获取所有Deployment
kubectl get deploy -n $NAMESPACE -o name | cut -d/ -f2 | while read deploy; do
    echo "=== Deployment: $deploy ==="
    
    # 获取HPA配置
    if kubectl get hpa -n $NAMESPACE | grep -q "^${deploy} "; then
        kubectl describe hpa $deploy -n $NAMESPACE | grep -E "(Target|Current)"
    else
        echo "No HPA configured"
    fi
    
    # 分析实际资源使用
    kubectl top pods -n $NAMESPACE -l app=$deploy 2>/dev/null || echo "No metrics available"
    
    # 检查资源请求设置
    kubectl get deploy $deploy -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[*].resources}' | jq .
    
    echo ""
done
EOF

chmod +x resource-utilization-analyzer.sh
```
---

<!-- chunk: 7. 最佳实践和优化建议 (Best Practices and Optimization) -->
## 7. 最佳实践和优化建议 (Best Practices and Optimization)

### 7.1 HPA配置最佳实践

```bash
# ========== 推荐的HPA配置模板 ==========
cat <<EOF > hpa-best-practice.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
  annotations:
    # 设置合理的容忍度 (避免频繁扩缩容)
    autoscaling.alpha.kubernetes.io/tolerance: "0.15"
    # 设置缩容稳定期
    autoscaling.alpha.kubernetes.io/downscale-stabilization: "5m"
    # 设置扩容冷却期
    autoscaling.alpha.kubernetes.io/upscale-delay: "1m"
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      - type: Pods
        value: 4
        periodSeconds: 60
      selectPolicy: Max
EOF
```

### 7.2 VPA配置最佳实践

```bash
# ========== 推荐的VPA配置模板 ==========
cat <<EOF > vpa-best-practice.yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: "apps/v1"
    kind: Deployment
    name: app-deployment
  updatePolicy:
    updateMode: "Auto"  # 或 "Initial" 用于生产环境更安全
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: "2"
        memory: 2Gi
      controlledResources: ["cpu", "memory"]
      mode: "Auto"
    - containerName: istio-proxy  # Sidecar容器特殊处理
      mode: "Off"  # 不自动调整sidecar资源
EOF

# ========== HPA+VPA混合使用策略 ==========
# HPA负责副本数调整，VPA负责单Pod资源调整
# 注意避免冲突，通常采用以下策略:

# 1. VPA使用"Initial"模式，只在Pod创建时调整
# 2. HPA基于CPU使用率，VPA基于内存使用
# 3. 为不同容器设置不同的控制策略
```

---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics KUDIG Database — Global MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- index.md|Domain-12 故障排查 — 开源项目索引]]
- [[domain-10-troubleshooting-diagnostics/核心排障/01-control-plane-apiserver-troubleshooting.md|[[API Server 故障排查|API Server 故障排查]]]]
- [[domain-10-troubleshooting-diagnostics/核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[domain-10-troubleshooting-diagnostics/核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[01-pod-pending-diagnosis|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-oom-memory-diagnosis|OOM 和内存问题诊断]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-pod-comprehensive-troubleshooting|Pod 全面故障排查]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/01-node-comprehensive-troubleshooting|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/07-ingress-troubleshooting|15-ingress-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/01-networkpolicy-troubleshooting|16-networkpolicy-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/03-cronjob-troubleshooting|18-cronjob-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/04-configmap-secret-troubleshooting|19-configmap-secret-troubleshooting]]

## Related

- [[domain-19-landscape-references/领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
