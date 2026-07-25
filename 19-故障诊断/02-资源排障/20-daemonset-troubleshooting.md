---
title: DaemonSet 故障排查
description: '# 20 - DaemonSet 故障排查 (DaemonSet Troubleshooting)'
summary: 'kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.tolerations}''
category: troubleshooting
tags:
- daemonset
- node-agent
- system
- kube-proxy
- fluentd
- node
- controller-manager
- prometheus
- rbac
- operator
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
- DaemonSet 没调度
- 节点缺少系统服务
- DaemonSet 更新卡住
trigger_keywords:
- DaemonSet
- 故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cni-basics
- etcd-basics
- logging-basics
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
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/daemonset-fta.md
  label: '故障树: daemonset'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 20 - [[DaemonSet|DaemonSet]] 故障排查 (DaemonSet Troubleshooting)

---

<!-- chunk: 1. DaemonSet 故障诊断总览 (DaemonSet Diagnosis Overview) -->
## 1. DaemonSet 故障诊断总览 (DaemonSet Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod未调度到节点** | 特定节点缺少系统服务 | 节点功能缺失 | P0 - 紧急 |
| **Pod持续CrashLoopBackOff** | 系统组件反复重启 | 节点服务不可用 | P0 - 紧急 |
| **更新卡住** | 滚动更新不进展 | 服务版本不一致 | P1 - 高 |
| **资源不足** | 节点资源耗尽 | 节点性能下降 | P1 - 高 |
| **污点/容忍度问题** | Pod被驱逐/不调度 | 系统功能中断 | P1 - 高 |

### 1.2 DaemonSet 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     DaemonSet 故障诊断架构                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       集群节点层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Worker    │    │   Master    │    │   Edge      │              │  │
│  │  │   Node-1    │    │   Node-1    │    │   Node-1    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   DaemonSet  │   │   DaemonSet  │   │   DaemonSet  │                   │
│  │   Pod-A1     │   │   Pod-B1     │   │   Pod-C1     │                   │
│  │ (fluentd)    │   │ (kube-proxy) │   │ (monitoring) │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   节点服务   │   │   网络代理   │   │   监控代理   │                   │
│  │ (Logging)   │   │ (Networking)│   │ (Telemetry) │                   │
│  │   收集      │   │   转发      │   │   上报      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                   DaemonSet控制器                                    │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-controller-manager                     │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │ DaemonSet   │  │   Node      │  │   Pod       │           │  │  │
│  │  │  │ Controller  │  │  Informer   │  │  Manager    │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   调度约束   │   │   更新策略   │   │   健康检查   │                   │
│  │ (Taints/    │   │ (Rolling    │   │ (Liveness/  │                   │
│  │ Tolerations)│   │ Update)     │   │ Readiness)  │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. DaemonSet 基础状态检查 (Basic Status Check) -->
## 2. DaemonSet 基础状态检查 (Basic Status Check)

### 2.1 DaemonSet 资源状态验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 基础信息检查 ==========
# 查看所有DaemonSet
kubectl get daemonsets --all-namespaces

# 查看特定DaemonSet详细信息
kubectl describe daemonset <daemonset-name> -n <namespace>

# 检查DaemonSet配置
kubectl get daemonset <daemonset-name> -n <namespace> -o yaml

# ========== 2. Pod分布状态检查 ==========
# 查看DaemonSet Pod分布
kubectl get pods -n <namespace> -l <selector> -o wide

# 检查每个节点上的Pod数量
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{range .items[*]}{.spec.nodeName}{"\n"}{end}' | sort | uniq -c

# 验证期望Pod数量 vs 实际Pod数量
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{
    .status.desiredNumberScheduled,
    .status.currentNumberScheduled,
    .status.numberReady,
    .status.numberAvailable
}'

# ========== 3. 节点亲和性检查 ==========
# 查看节点标签
kubectl get nodes --show-labels

# 检查节点污点
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.taints}{"\n"}{end}'

# 验证DaemonSet节点选择器
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.selector}'
```
### 2.2 调度约束验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 污点和容忍度检查 ==========
# 查看节点污点
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .spec.taints[*]}{.key}={.value}:{.effect}{" "}{end}{"\n"}{end}'

# 查看DaemonSet容忍度
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.tolerations}'

# 验证污点匹配
NODE_TAINTS=$(kubectl get nodes <node-name> -o jsonpath='{.spec.taints[*].key}')
DAEMONSET_TOLERATIONS=$(kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.tolerations[*].key}')

echo "Node taints: $NODE_TAINTS"
echo "DaemonSet tolerations: $DAEMONSET_TOLERATIONS"

# ========== 节点选择器检查 ==========
# 查看DaemonSet节点选择器
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.nodeSelector}'

# 验证节点标签匹配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels}{"\n"}{end}' | grep -E "($(kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.nodeSelector[*]}'))"
```
---

<!-- chunk: 3. Pod调度问题排查 (Pod Scheduling Issues) -->
## 3. Pod调度问题排查 (Pod Scheduling Issues)

### 3.1 Pod未调度到节点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 调度失败原因分析 ==========
# 查看未调度Pod的事件
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod,reason=FailedScheduling

# 检查特定Pod的调度事件
kubectl describe pod -n <namespace> -l <daemonset-selector> | grep -A10 "Events:"

# 分析调度失败的常见原因
kubectl get pods -n <namespace> -l <selector> --field-selector=status.phase!=Running -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.phase
}{
        "\t"
}{
        .status.conditions[?(@.type=="PodScheduled")].reason
}{
        "\n"
}{
    end
}'

# ========== 2. 资源约束检查 ==========
# 检查节点资源使用情况
kubectl describe nodes | grep -A5 "Allocated resources"

# 验证Pod资源请求
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].resources}'

# 检查节点可分配资源
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl describe node $node | grep -A10 "Allocatable"
done

# ========== 3. 节点亲和性问题 ==========
# 检查节点亲和性配置
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.affinity}'

# 验证反亲和性规则
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.affinity.podAntiAffinity}'
```
### 3.2 污点导致的调度问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 污点问题诊断 ==========
# 列出所有节点污点
kubectl get nodes -o go-template='{{range .items}}{{printf "%s:\n" .metadata.name}}{{range .spec.taints}}{{printf "  %s=%s:%s\n" .key .value .effect}}{{end}}{{end}}'

# 检查关键污点
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .spec.taints[*]}{.key}={.value}:{.effect}{" "}{end}{"\n"}{end}' | grep -E "(node\.kubernetes\.io/not-ready|node\.kubernetes\.io/unreachable)"

# ========== 添加必要容忍度 ==========
# 为DaemonSet添加容忍度示例
cat <<EOF | kubectl patch daemonset <daemonset-name> -n <namespace> --patch '
spec:
  template:
    spec:
      tolerations:
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
        tolerationSeconds: 300
      - key: node.kubernetes.io/unreachable
        operator: Exists
        effect: NoExecute
        tolerationSeconds: 300
      - key: node.kubernetes.io/disk-pressure
        operator: Exists
        effect: NoSchedule
EOF

# ========== 特定场景容忍度配置 ==========
# CNI网络插件容忍度
cat <<EOF > cni-daemonset-tolerations.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: cni-plugin
  namespace: kube-system
spec:
  template:
    spec:
      tolerations:
      # 允许在master节点运行
      - key: node-role.kubernetes.io/master
        operator: Exists
        effect: NoSchedule
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
      # 允许在未就绪节点运行
      - key: node.kubernetes.io/not-ready
        operator: Exists
        effect: NoExecute
        tolerationSeconds: 300
      # 允许在网络不可达节点运行
      - key: node.kubernetes.io/network-unavailable
        operator: Exists
        effect: NoSchedule
EOF
```
---

<!-- chunk: 4. Pod运行时问题排查 (Pod Runtime Issues) -->
## 4. Pod运行时问题排查 (Pod Runtime Issues)

### 4.1 CrashLoopBackOff问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 容器启动失败分析 ==========
# 查看Pod状态和重启次数
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.containerStatuses[*].restartCount
}{
        "\t"
}{
        .status.containerStatuses[*].lastState.terminated.reason
}{
        "\n"
}{
    end
}'

# 查看容器日志
kubectl logs -n <namespace> -l <selector> --previous --tail=100

# 检查容器启动命令
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].command}'

# ========== 2. 资源限制问题 ==========
# 检查OOMKilled状态
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.containerStatuses[*].lastState.terminated.reason
}{
        "\n"
}{
    end
}' | grep OOMKilled

# 验证资源请求和限制
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{
    .spec.template.spec.containers[*].resources.requests.cpu
}{
    "\t"
}{
    .spec.template.spec.containers[*].resources.requests.memory
}{
    "\t"
}{
    .spec.template.spec.containers[*].resources.limits.cpu
}{
    "\t"
}{
    .spec.template.spec.containers[*].resources.limits.memory
}'

# ========== 3. 健康检查失败 ==========
# 检查存活探针配置
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].livenessProbe}'

# 检查就绪探针配置
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].readinessProbe}'

# 查看探针失败事件
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod,reason=Unhealthy
```
### 4.2 权限和安全上下文问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 安全上下文检查 ==========
# 检查Pod安全上下文
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.securityContext}'

# 验证容器安全上下文
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].securityContext}'

# 检查特权模式
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[*].securityContext.privileged}'

# ========== ServiceAccount权限验证 ==========
# 检查ServiceAccount
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.serviceAccountName}'

# 验证RBAC权限
SERVICE_ACCOUNT=$(kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.template.spec.serviceAccountName}')
kubectl auth can-i get nodes --as=system:serviceaccount:<namespace>:$SERVICE_ACCOUNT

# 检查RoleBinding
kubectl get rolebindings -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.subjects[*].name}{"\n"}{end}' | grep $SERVICE_ACCOUNT
```
---

<!-- chunk: 5. 更新和滚动策略问题 (Update and Rolling Strategy Issues) -->
## 5. 更新和滚动策略问题 (Update and Rolling Strategy Issues)

### 5.1 更新卡住问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 更新状态检查 ==========
# 查看DaemonSet更新状态
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{
    .status.updatedNumberScheduled,
    .status.numberAvailable,
    .status.desiredNumberScheduled
}'

# 检查更新策略
kubectl get daemonset <daemonset-name> -n <namespace> -o jsonpath='{.spec.updateStrategy}'

# 查看更新历史
kubectl rollout history daemonset <daemonset-name> -n <namespace>

# ========== 2. 卡住原因分析 ==========
# 检查未就绪Pod
kubectl get pods -n <namespace> -l <selector> --field-selector=status.phase=Running -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.conditions[?(@.type=="Ready")].status
}{
        "\n"
}{
    end
}' | grep False

# 检查节点状态
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.conditions[?(@.type=="Ready")].status
}{
        "\n"
}{
    end
}' | grep -v True

# 查看更新事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<daemonset-name>,involvedObject.kind=DaemonSet
```
### 5.2 滚动更新配置优化

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 推荐的更新策略配置 ==========
cat <<EOF > daemonset-update-strategy.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: optimized-daemonset
  namespace: kube-system
spec:
  updateStrategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1  # 每次最多1个不可用
  minReadySeconds: 30   # 新Pod就绪后等待30秒
  revisionHistoryLimit: 10  # 保留10个历史版本
  
  template:
    spec:
      terminationGracePeriodSeconds: 30  # 优雅终止时间
      containers:
      - name: daemon-container
        image: my-daemon:latest
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 3
EOF

# ========== 更新策略对比 ==========
# OnDelete策略 (需要手动删除Pod)
kubectl patch daemonset <daemonset-name> -n <namespace> -p '{"spec":{"updateStrategy":{"type":"OnDelete"}}}'

# RollingUpdate策略 (自动滚动更新)
kubectl patch daemonset <daemonset-name> -n <namespace> -p '{"spec":{"updateStrategy":{"type":"RollingUpdate","rollingUpdate":{"maxUnavailable":1}}}}'
```
---

<!-- chunk: 6. 监控和告警配置 (Monitoring and Alerting) -->
## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 关键指标监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== DaemonSet状态监控 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: daemonset-monitor
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
      regex: 'kube_daemonset.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: daemonset-alerts
  namespace: monitoring
spec:
  groups:
  - name: daemonset.rules
    rules:
    - alert: DaemonSetNotScheduled
      expr: kube_daemonset_status_number_misscheduled > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "DaemonSet pods are misscheduled (namespace {{ \$labels.namespace }} daemonset {{ \$labels.daemonset }})"
        
    - alert: DaemonSetUnavailable
      expr: kube_daemonset_status_number_unavailable > 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "DaemonSet has unavailable pods (namespace {{ \$labels.namespace }} daemonset {{ \$labels.daemonset }})"
        
    - alert: DaemonSetUpdateStuck
      expr: kube_daemonset_status_updated_number_scheduled != kube_daemonset_status_desired_number_scheduled
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "DaemonSet update is stuck (namespace {{ \$labels.namespace }} daemonset {{ \$labels.daemonset }})"
        
    - alert: NodeWithoutDaemonSet
      expr: count(kube_node_status_condition{condition="Ready",status="true"}) - count(kube_pod_status_ready{condition="true"} * on(pod) group_left(node) kube_pod_info{daemonset!="",node!=""}) > 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Nodes without required DaemonSet pods"
EOF
```
### 6.2 性能和健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== DaemonSet健康检查脚本 ==========
cat <<'EOF' > daemonset-health-check.sh
#!/bin/bash

NAMESPACE=${1:-kube-system}
CHECK_INTERVAL=${2:-60}

echo "Starting DaemonSet health check for namespace: $NAMESPACE"
echo "Check interval: $CHECK_INTERVAL seconds"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 检查所有DaemonSet状态
    kubectl get daemonsets -n $NAMESPACE -o jsonpath='{
        range .items[*]
    }{
            .metadata.name
    }{
            "\t"
    }{
            .status.desiredNumberScheduled
    }{
            "\t"
    }{
            .status.currentNumberScheduled
    }{
            "\t"
    }{
            .status.numberReady
    }{
            "\t"
    }{
            .status.numberMisscheduled
    }{
            "\n"
    }{
        end
    }' | while read name desired current ready misscheduled; do
        echo "$TIMESTAMP - DaemonSet: $name"
        echo "  Desired: $desired, Current: $current, Ready: $ready, Misscheduled: $misscheduled"
        
        # 告警条件检查
        if [ "$desired" != "$current" ]; then
            echo "  WARNING: Scheduled count mismatch"
        fi
        
        if [ "$current" != "$ready" ]; then
            echo "  WARNING: Not all pods are ready"
        fi
        
        if [ "$misscheduled" != "0" ]; then
            echo "  CRITICAL: Pods are misscheduled"
        fi
    done
    
    sleep $CHECK_INTERVAL
done
EOF

chmod +x daemonset-health-check.sh

# ========== 节点覆盖率检查 ==========
cat <<'EOF' > node-coverage-check.sh
#!/bin/bash

DAEMONSET_NAME=$1
NAMESPACE=${2:-kube-system}

echo "Checking node coverage for DaemonSet: $DAEMONSET_NAME in namespace: $NAMESPACE"

# 获取DaemonSet选择器
SELECTOR=$(kubectl get daemonset $DAEMONSET_NAME -n $NAMESPACE -o jsonpath='{.spec.selector.matchLabels}' | jq -r 'to_entries[] | "\(.key)=\(.value)"' | paste -sd "," -)

# 获取所有就绪节点
READY_NODES=$(kubectl get nodes -o jsonpath='{.items[?(@.status.conditions[?(@.type=="Ready")].status=="True")].metadata.name}')

# 获取运行DaemonSet Pod的节点
POD_NODES=$(kubectl get pods -n $NAMESPACE -l $SELECTOR -o jsonpath='{.items[*].spec.nodeName}')

echo "Ready nodes: $(echo $READY_NODES | wc -w)"
echo "Nodes with DaemonSet pods: $(echo $POD_NODES | wc -w)"

# 找出缺失Pod的节点
for node in $READY_NODES; do
    if ! echo $POD_NODES | grep -q $node; then
        echo "MISSING: Node $node does not have DaemonSet pod"
        
        # 检查节点污点
        TAINTS=$(kubectl get node $node -o jsonpath='{.spec.taints[*].key}')
        echo "  Node taints: $TAINTS"
        
        # 检查DaemonSet容忍度
        TOLERATIONS=$(kubectl get daemonset $DAEMONSET_NAME -n $NAMESPACE -o jsonpath='{.spec.template.spec.tolerations[*].key}')
        echo "  DaemonSet tolerations: $TOLERATIONS"
    fi
done
EOF

chmod +x node-coverage-check.sh
```
---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 故障诊断 KUDIG Database — Global MOC
- [[19-故障诊断/README.md|Domain-12 故障排查 (Troubleshooting)]]
- index.md|Domain-12 故障排查 — 开源项目索引]]
- [[19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting.md|[[API Server 故障排查|API Server 故障排查]]]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[19-故障诊断/01-核心排障/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[19-故障诊断/02-资源排障/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[19-故障诊断/02-资源排障/18-cronjob-troubleshooting.md|18-cronjob-troubleshooting]]
- [[19-故障诊断/02-资源排障/19-configmap-secret-troubleshooting.md|19-configmap-secret-troubleshooting]]
- [[19-故障诊断/02-资源排障/21-statefulset-troubleshooting.md|21-statefulset-troubleshooting]]
- [[19-故障诊断/02-资源排障/22-job-troubleshooting.md|22-job-troubleshooting]]


<!-- risk-assessed -->
