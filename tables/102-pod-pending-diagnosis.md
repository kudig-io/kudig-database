# 86 - Pod Pending 状态诊断 (Pod Pending Diagnosis)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级

## Pod Pending 诊断流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Pod Pending 状态诊断决策树                                    │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  Pod 处于 Pending 状态                                                              │
│          │                                                                           │
│          ▼                                                                           │
│  ┌─────────────────┐                                                                │
│  │ kubectl describe│                                                                │
│  │ pod <name>      │                                                                │
│  └────────┬────────┘                                                                │
│           │                                                                          │
│           ▼                                                                          │
│  查看 Events 部分中的 FailedScheduling 原因                                          │
│           │                                                                          │
│           ├──────────────────────────────────────────────────────────────┐          │
│           │                                                              │          │
│  ┌────────▼────────┐    ┌────────────────────┐    ┌────────────────────┐│          │
│  │ Insufficient    │    │ node(s) didn't     │    │ node(s) had        ││          │
│  │ cpu/memory      │    │ match selector     │    │ taints             ││          │
│  └────────┬────────┘    └─────────┬──────────┘    └─────────┬──────────┘│          │
│           │                       │                         │           │          │
│           ▼                       ▼                         ▼           │          │
│  ┌─────────────────┐    ┌────────────────────┐    ┌────────────────────┐│          │
│  │ 资源不足        │    │ 节点选择问题       │    │ 污点容忍问题       ││          │
│  │                 │    │                    │    │                    ││          │
│  │ • 扩容节点      │    │ • 检查 nodeSelector│    │ • 添加 tolerations ││          │
│  │ • 调整 requests │    │ • 检查 affinity    │    │ • 移除节点污点     ││          │
│  │ • 驱逐低优先级  │    │ • 添加节点标签     │    │                    ││          │
│  └─────────────────┘    └────────────────────┘    └────────────────────┘│          │
│                                                                          │          │
│           ├──────────────────────────────────────────────────────────────┘          │
│           │                                                                          │
│  ┌────────▼────────┐    ┌────────────────────┐    ┌────────────────────────────┐   │
│  │ pod has unbound │    │ pod topology       │    │ 0/N nodes are              │   │
│  │ PVC(s)          │    │ spread constraints │    │ available                  │   │
│  └────────┬────────┘    └─────────┬──────────┘    └─────────┬──────────────────┘   │
│           │                       │                         │                       │
│           ▼                       ▼                         ▼                       │
│  ┌─────────────────┐    ┌────────────────────┐    ┌────────────────────────────┐   │
│  │ 存储问题        │    │ 拓扑分布约束       │    │ 所有节点不可用             │   │
│  │                 │    │                    │    │                            │   │
│  │ • 检查 PVC 状态 │    │ • 检查 maxSkew     │    │ • 检查节点状态             │   │
│  │ • 检查 SC       │    │ • 调整约束         │    │ • 检查节点资源             │   │
│  │ • 检查 CSI      │    │ • 增加节点         │    │ • 综合排查                 │   │
│  └─────────────────┘    └────────────────────┘    └────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## Pod Pending 常见原因分类

### 资源类问题

| 原因类别 | 具体原因 | 诊断方法 | 解决方案 |
|---------|---------|---------|---------|
| **CPU 不足** | 集群 CPU 资源耗尽 | `kubectl describe node | grep -A 10 "Allocated"` | 扩容节点/调整 requests |
| **内存不足** | 集群内存资源耗尽 | `kubectl top nodes` | 扩容节点/调整 requests |
| **GPU 不足** | GPU 节点不足 | 检查 `nvidia.com/gpu` 资源 | 添加 GPU 节点 |
| **临时存储不足** | ephemeral-storage 不足 | 检查节点磁盘使用 | 清理磁盘/扩容 |
| **Pod 数量限制** | 节点 Pod 数达上限 | `kubectl describe node | grep "Capacity"` | 调整 maxPods |

### 节点选择问题

| 原因类别 | 具体原因 | 诊断方法 | 解决方案 |
|---------|---------|---------|---------|
| **nodeSelector 不匹配** | 无节点有匹配标签 | `kubectl get nodes --show-labels` | 修正标签或选择器 |
| **nodeAffinity 不满足** | 亲和性规则无法满足 | 检查 Pod spec.affinity | 调整亲和性配置 |
| **污点无法容忍** | 节点有污点 Pod 无容忍 | `kubectl describe node | grep Taint` | 添加 tolerations |
| **节点不可调度** | 节点被 cordon | `kubectl get nodes` | uncordon 节点 |

### 存储问题

| 原因类别 | 具体原因 | 诊断方法 | 解决方案 |
|---------|---------|---------|---------|
| **PVC 未绑定** | PVC Pending 状态 | `kubectl get pvc` | 检查 StorageClass/PV |
| **StorageClass 不存在** | 指定的 SC 不存在 | `kubectl get sc` | 创建 SC 或修改 PVC |
| **CSI 驱动异常** | CSI 组件故障 | 检查 CSI Pod 状态 | 修复 CSI 组件 |
| **存储后端故障** | NAS/云盘等后端问题 | 检查存储系统 | 修复存储后端 |

### 配额与限制

| 原因类别 | 具体原因 | 诊断方法 | 解决方案 |
|---------|---------|---------|---------|
| **ResourceQuota 超限** | 命名空间配额用尽 | `kubectl describe quota` | 调整配额或释放资源 |
| **LimitRange 违规** | Pod 资源不符合限制 | `kubectl describe limitrange` | 调整 Pod 资源配置 |
| **PodDisruptionBudget** | PDB 阻止调度 | `kubectl get pdb` | 检查 PDB 配置 |

### 调度器问题

| 原因类别 | 具体原因 | 诊断方法 | 解决方案 |
|---------|---------|---------|---------|
| **调度器不可用** | kube-scheduler 故障 | 检查 scheduler Pod | 恢复调度器 |
| **调度器配置错误** | 调度器配置问题 | 查看调度器日志 | 修复配置 |
| **自定义调度器缺失** | 指定的调度器不存在 | 检查 schedulerName | 部署调度器或修改配置 |

## 快速诊断脚本

```bash
#!/bin/bash
# pod-pending-diagnose.sh - Pod Pending 快速诊断

POD_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <pod-name> [namespace]"
    echo ""
    echo "当前 Pending 的 Pod 列表:"
    kubectl get pods -A --field-selector=status.phase=Pending
    exit 1
fi

echo "=========================================="
echo "Pod Pending 诊断: $NAMESPACE/$POD_NAME"
echo "=========================================="

echo ""
echo "=== 1. Pod 基本信息 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o wide

echo ""
echo "=== 2. Pod 事件 (最近) ==="
kubectl describe pod $POD_NAME -n $NAMESPACE | grep -A 30 "Events:"

echo ""
echo "=== 3. 调度失败事件 ==="
kubectl get events -n $NAMESPACE \
  --field-selector reason=FailedScheduling,involvedObject.name=$POD_NAME \
  --sort-by='.lastTimestamp'

echo ""
echo "=== 4. Pod 资源请求 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='
Containers:
{range .spec.containers[*]}
  - {.name}:
    requests: cpu={.resources.requests.cpu}, memory={.resources.requests.memory}
    limits: cpu={.resources.limits.cpu}, memory={.resources.limits.memory}
{end}
'
echo ""

echo ""
echo "=== 5. 节点选择约束 ==="
echo "NodeSelector:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeSelector}' | jq . 2>/dev/null || echo "  (none)"
echo ""
echo "NodeAffinity:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.affinity.nodeAffinity}' | jq . 2>/dev/null || echo "  (none)"
echo ""
echo "Tolerations:"
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.tolerations}' | jq . 2>/dev/null || echo "  (none)"

echo ""
echo "=== 6. PVC 状态 ==="
PVCS=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.volumes[*].persistentVolumeClaim.claimName}')
if [ -n "$PVCS" ]; then
    for pvc in $PVCS; do
        echo "PVC: $pvc"
        kubectl get pvc $pvc -n $NAMESPACE
    done
else
    echo "  (no PVCs)"
fi

echo ""
echo "=== 7. 节点资源概况 ==="
echo "可调度节点:"
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,STATUS:.status.conditions[?(@.type=="Ready")].status,CPU:.status.allocatable.cpu,MEMORY:.status.allocatable.memory,PODS:.status.allocatable.pods,TAINTS:.spec.taints[*].key'

echo ""
echo "=== 8. 配额检查 ==="
echo "ResourceQuota:"
kubectl get resourcequota -n $NAMESPACE 2>/dev/null || echo "  (no quota)"
echo ""
echo "LimitRange:"
kubectl get limitrange -n $NAMESPACE 2>/dev/null || echo "  (no limitrange)"

echo ""
echo "=== 9. 建议 ==="
echo "根据上述信息，请检查:"
echo "  1. Pod 事件中的具体调度失败原因"
echo "  2. 节点资源是否足够"
echo "  3. 节点选择器和亲和性配置"
echo "  4. PVC 是否已绑定"
echo "  5. 配额是否超限"
```

## 详细诊断命令

### 资源不足诊断

```bash
# ==================== 集群资源概况 ====================

# 查看所有节点资源使用
kubectl top nodes

# 查看节点资源分配详情
kubectl describe nodes | grep -A 10 "Allocated resources"

# 计算集群可分配资源
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,CPU:.status.allocatable.cpu,MEMORY:.status.allocatable.memory,PODS:.status.allocatable.pods'

# 查看节点资源压力条件
kubectl describe nodes | grep -E "MemoryPressure|DiskPressure|PIDPressure"

# ==================== Pod 资源请求分析 ====================

# 查看 Pending Pod 的资源请求
kubectl get pod <pod-name> -n <namespace> -o jsonpath='
requests.cpu: {.spec.containers[*].resources.requests.cpu}
requests.memory: {.spec.containers[*].resources.requests.memory}
'

# 查看命名空间资源使用汇总
kubectl top pods -n <namespace>

# 分析节点上的资源分配
kubectl describe node <node-name> | grep -A 20 "Non-terminated Pods"

# ==================== 大资源请求 Pod 排查 ====================

# 查找请求 CPU 最多的 Pod
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.status.phase=="Running") |
  "\(.metadata.namespace)/\(.metadata.name): \(.spec.containers[].resources.requests.cpu // "not set")"
' | sort -t: -k2 -rn | head -10

# 查找请求内存最多的 Pod
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.status.phase=="Running") |
  "\(.metadata.namespace)/\(.metadata.name): \(.spec.containers[].resources.requests.memory // "not set")"
' | sort -t: -k2 -rn | head -10
```

### 节点选择问题诊断

```bash
# ==================== 标签和选择器 ====================

# 查看 Pod 的 nodeSelector
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeSelector}'

# 查看所有节点标签
kubectl get nodes --show-labels

# 查看特定标签的节点
kubectl get nodes -l <key>=<value>

# 查看节点详细标签
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, labels: .metadata.labels}'

# ==================== 亲和性规则 ====================

# 查看 Pod 的所有亲和性规则
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity}' | jq .

# 查看 nodeAffinity
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity.nodeAffinity}' | jq .

# 查看 podAffinity
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity.podAffinity}' | jq .

# 查看 podAntiAffinity
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.affinity.podAntiAffinity}' | jq .

# ==================== 污点和容忍 ====================

# 查看所有节点的污点
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints[*].key'

# 查看特定节点的完整污点信息
kubectl describe node <node-name> | grep -A 5 "Taints:"

# 查看 Pod 的容忍配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.tolerations}' | jq .

# 查找没有污点的节点
kubectl get nodes -o json | jq '.items[] | select(.spec.taints == null) | .metadata.name'

# ==================== 拓扑分布约束 ====================

# 查看 Pod 的拓扑分布约束
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.topologySpreadConstraints}' | jq .

# 查看节点的拓扑标签
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, zone: .metadata.labels["topology.kubernetes.io/zone"], region: .metadata.labels["topology.kubernetes.io/region"]}'
```

### 存储问题诊断

```bash
# ==================== PVC 状态检查 ====================

# 查看命名空间中的 PVC
kubectl get pvc -n <namespace>

# 查看 PVC 详情和事件
kubectl describe pvc <pvc-name> -n <namespace>

# 查看 Pending 的 PVC
kubectl get pvc -A --field-selector=status.phase=Pending

# 查看 PVC 绑定的 PV
kubectl get pvc <pvc-name> -n <namespace> -o jsonpath='{.spec.volumeName}'

# ==================== StorageClass 检查 ====================

# 查看所有 StorageClass
kubectl get sc

# 查看默认 StorageClass
kubectl get sc -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# 查看 StorageClass 详情
kubectl describe sc <storageclass-name>

# ==================== PV 状态检查 ====================

# 查看所有 PV
kubectl get pv

# 查看可用的 PV
kubectl get pv --field-selector=status.phase=Available

# 查看 PV 详情
kubectl describe pv <pv-name>

# ==================== CSI 驱动检查 ====================

# 查看 CSI 驱动
kubectl get csidrivers

# 查看 CSI 节点信息
kubectl get csinodes

# 查看 CSI 组件 Pod
kubectl get pods -n kube-system | grep -E "csi|provisioner"

# 查看 CSI 驱动日志
kubectl logs -n kube-system -l app=csi-provisioner --tail=50
```

### 配额问题诊断

```bash
# ==================== ResourceQuota 检查 ====================

# 查看命名空间配额
kubectl get resourcequota -n <namespace>

# 查看配额详情
kubectl describe resourcequota -n <namespace>

# 查看配额使用情况
kubectl get resourcequota -n <namespace> -o yaml

# ==================== LimitRange 检查 ====================

# 查看 LimitRange
kubectl get limitrange -n <namespace>

# 查看 LimitRange 详情
kubectl describe limitrange -n <namespace>

# ==================== 命名空间资源汇总 ====================

# 查看命名空间资源使用
kubectl describe ns <namespace> | grep -A 20 "Resource Quotas"

# 计算命名空间内 Pod 资源请求总和
kubectl get pods -n <namespace> -o json | jq '
  [.items[].spec.containers[].resources.requests.cpu // "0" | 
   if . == "0" then 0 elif test("m$") then (.[:-1] | tonumber) / 1000 else tonumber end] | add
'
```

### 调度器问题诊断

```bash
# ==================== 调度器状态 ====================

# 查看调度器 Pod
kubectl get pods -n kube-system -l component=kube-scheduler

# 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100

# 查看 Pod 使用的调度器
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.schedulerName}'

# 查看调度器 leader
kubectl get endpoints kube-scheduler -n kube-system -o yaml

# ==================== 调度器事件 ====================

# 查看调度相关事件
kubectl get events -n <namespace> --field-selector reason=FailedScheduling

# 查看所有调度事件
kubectl get events -A --field-selector reason=FailedScheduling --sort-by='.lastTimestamp'

# 实时监控调度事件
kubectl get events -A --field-selector reason=FailedScheduling -w
```

## 常见解决方案

### 资源不足解决

```yaml
# 方案 1: 调整 Pod 资源请求
apiVersion: v1
kind: Pod
metadata:
  name: optimized-pod
spec:
  containers:
    - name: app
      resources:
        requests:
          cpu: 100m      # 降低请求
          memory: 128Mi
        limits:
          cpu: 500m
          memory: 512Mi
---
# 方案 2: 使用 VPA 自动调整
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: my-app-vpa
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  updatePolicy:
    updateMode: Auto
  resourcePolicy:
    containerPolicies:
      - containerName: '*'
        minAllowed:
          cpu: 50m
          memory: 64Mi
        maxAllowed:
          cpu: 2
          memory: 4Gi
```

### 节点选择问题解决

```bash
# 方案 1: 为节点添加标签
kubectl label node <node-name> <key>=<value>

# 方案 2: 移除不匹配的 nodeSelector
kubectl patch deployment <name> -p '{"spec":{"template":{"spec":{"nodeSelector":null}}}}'

# 方案 3: 添加节点容忍
kubectl patch deployment <name> --type=json -p='[
  {"op":"add","path":"/spec/template/spec/tolerations","value":[
    {"key":"node-role.kubernetes.io/master","effect":"NoSchedule","operator":"Exists"}
  ]}
]'

# 方案 4: 移除节点污点
kubectl taint node <node-name> <key>:<effect>-
```

### 存储问题解决

```yaml
# 方案 1: 创建缺失的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-disk-ssd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_ssd
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
---
# 方案 2: 手动创建 PV
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: manual
  hostPath:
    path: /data/pv
```

### 配额问题解决

```yaml
# 方案: 调整 ResourceQuota
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    requests.cpu: "100"       # 增加配额
    requests.memory: 200Gi
    limits.cpu: "200"
    limits.memory: 400Gi
    pods: "500"
```

## 监控告警配置

```yaml
# pod-pending-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-pending-alerts
  namespace: monitoring
spec:
  groups:
    - name: pod.pending
      interval: 30s
      rules:
        # Pod Pending 超过 5 分钟
        - alert: PodPendingTooLong
          expr: |
            sum by (namespace, pod) (
              kube_pod_status_phase{phase="Pending"} == 1
            ) * on (namespace, pod) group_left()
            (time() - kube_pod_created) > 300
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 5 分钟"
            description: "请检查调度器事件和节点资源"
            runbook_url: "https://wiki.example.com/runbooks/pod-pending"
        
        # Pod Pending 超过 15 分钟 (严重)
        - alert: PodPendingCritical
          expr: |
            sum by (namespace, pod) (
              kube_pod_status_phase{phase="Pending"} == 1
            ) * on (namespace, pod) group_left()
            (time() - kube_pod_created) > 900
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 15 分钟"
            description: "需要立即处理，检查集群资源和调度配置"
        
        # 大量 Pod Pending
        - alert: ManyPodsPending
          expr: |
            count(kube_pod_status_phase{phase="Pending"}) > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "集群有超过 10 个 Pod 处于 Pending 状态"
            description: "当前 Pending Pod 数: {{ $value }}"
        
        # 调度失败事件增加
        - alert: HighSchedulingFailures
          expr: |
            increase(scheduler_schedule_attempts_total{result="error"}[5m]) > 10
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "调度器失败次数增加"
            description: "过去 5 分钟内调度失败 {{ $value }} 次"
        
        # 集群资源不足预警
        - alert: ClusterResourcesLow
          expr: |
            sum(kube_node_status_allocatable{resource="cpu"}) - 
            sum(kube_pod_container_resource_requests{resource="cpu"}) < 4
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "集群可用 CPU 资源不足 4 核"
            description: "考虑扩容节点或优化资源请求"
```

## 紧急处理流程

```bash
# ==================== 紧急处理命令 ====================

# 1. 快速查看所有 Pending Pod
kubectl get pods -A --field-selector=status.phase=Pending

# 2. 批量查看 Pending Pod 原因
kubectl get events -A --field-selector reason=FailedScheduling --sort-by='.lastTimestamp' | tail -20

# 3. 临时跳过调度约束 (仅紧急情况)
kubectl patch deployment <name> -n <namespace> -p '{"spec":{"template":{"spec":{"nodeSelector":null,"affinity":null}}}}'

# 4. 强制删除无法调度的 Pod (让控制器重新创建)
kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0

# 5. 紧急扩容节点 (ACK)
aliyun cs POST /clusters/{ClusterId}/nodepools/{NodepoolId}/nodes --body '{"count": 2}'

# 6. 临时移除节点污点
kubectl taint node <node-name> <taint-key>-

# 7. 取消节点不可调度状态
kubectl uncordon <node-name>

# 8. 驱逐低优先级 Pod 释放资源
kubectl get pods -A -o json | jq -r '.items[] | select(.spec.priority < 0) | "\(.metadata.namespace) \(.metadata.name)"' | \
  while read ns pod; do kubectl delete pod $pod -n $ns; done
```

## 最佳实践

### 预防措施

- [ ] **合理设置资源请求**: requests 应反映实际使用量
- [ ] **配置 PodDisruptionBudget**: 防止意外驱逐
- [ ] **使用节点亲和性软约束**: preferredDuringScheduling
- [ ] **配置集群自动扩缩**: Cluster Autoscaler
- [ ] **监控 Pending Pod 数量**: 设置告警
- [ ] **定期审查资源配额**: 避免配额阻塞
- [ ] **预留缓冲资源**: 保证突发需求

### 诊断原则

1. **先看 Events**: `kubectl describe pod` 确定根本原因
2. **分类排查**: 资源 → 节点 → 存储 → 配额
3. **针对性解决**: 根据具体原因采取对应措施
4. **验证修复**: 确认 Pod 成功调度

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
