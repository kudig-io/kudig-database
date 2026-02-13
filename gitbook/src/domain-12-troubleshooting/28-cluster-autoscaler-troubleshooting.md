# 28 - 集群自动扩缩容故障排查 (Cluster Autoscaler Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Cluster Autoscaler](https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler)

---

## 1. 集群自动扩缩容故障诊断总览 (Cluster Autoscaler Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **扩缩容不触发** | 节点数量不变 | 资源利用率不合理 | P1 - 高 |
| **扩缩容震荡** | 频繁增删节点 | 成本浪费/不稳定 | P1 - 高 |
| **节点驱逐失败** | Pod无法迁移 | 缩容卡住 | P0 - 紧急 |
| **资源评估错误** | 扩容不足/过度 | 性能问题/成本浪费 | P1 - 高 |
| **云提供商API失败** | 无法创建/删除节点 | 扩缩容完全失效 | P0 - 紧急 |

### 1.2 集群自动扩缩容架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                集群自动扩缩容故障诊断架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       工作负载层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │ (Pending)   │    │ (Running)   │    │ (Pending)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                   调度器评估层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     kube-scheduler                            │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   调度评估   │  │   资源需求   │  │   节点选择   │           │  │  │
│  │  │  │ (Predicates)│  │  (Requests) │  │  (Priorities)│           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                  集群自动扩缩容控制器                                │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                cluster-autoscaler                             │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   扩容决策   │  │   缩容决策   │  │   节点管理   │           │  │  │
│  │  │  │ (Scale Up)  │  │ (Scale Down)│  │  (Drain)    │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   云API调用  │   │   节点池管理  │   │   驱逐管理   │                   │
│  │ (Cloud API) │   │ (Node Groups)│   │  (Eviction) │                   │
│  │   创建/删除  │   │   扩缩容     │   │   Pod迁移    │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      云基础设施层                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   EC2实例   │    │   虚拟机     │   │   物理机     │              │  │
│  │  │ (AWS)       │    │ (VMware)    │   │ (Bare Metal)│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Cluster Autoscaler 基础状态检查 (Basic Status Check)

### 2.1 组件状态验证

```bash
# ========== 1. Autoscaler部署检查 ==========
# 检查Cluster Autoscaler Pod状态
kubectl get pods -n kube-system | grep cluster-autoscaler

# 查看Autoscaler详细信息
kubectl describe pods -n kube-system -l app=cluster-autoscaler

# 检查Autoscaler日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100

# ========== 2. 配置参数检查 ==========
# 查看Autoscaler配置
kubectl get deployment cluster-autoscaler -n kube-system -o yaml

# 检查关键启动参数
kubectl get deployment cluster-autoscaler -n kube-system -o jsonpath='{
    .spec.template.spec.containers[0].command
}' | jq

# 验证节点组配置
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml

# ========== 3. 节点组状态检查 ==========
# 查看受管节点组
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.labels."cluster-autoscaler\.kubernetes\.io/safe-to-evict"
}{
        "\n"
}{
    end
}'

# 检查节点标签
kubectl get nodes --show-labels | grep -E "(node-group|min|max)"
```

### 2.2 扩缩容状态监控

```bash
# ========== 当前扩缩容状态 ==========
# 查看Autoscaler状态
kubectl get configmap cluster-autoscaler-status -n kube-system -o jsonpath='{.data.status}'

# 检查节点组健康状态
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.unschedulable
}{
        "\t"
}{
        .status.conditions[?(@.type=="Ready")].status
}{
        "\n"
}{
    end
}'

# 查看Pending状态的Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending

# ========== 资源使用情况 ==========
# 检查节点资源分配
kubectl describe nodes | grep -A5 "Allocated resources"

# 查看节点可分配资源
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.allocatable.cpu
}{
        "\t"
}{
        .status.allocatable.memory
}{
        "\n"
}{
    end
}'

# 分析Pod资源请求
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .spec.containers[*].resources.requests.cpu
}{
        " "
}{
    end
}' | tr ' ' '\n' | grep -v '^$' | awk '{sum+=$1} END {print "Total CPU requests:", sum}'
```

---

## 3. 扩缩容不触发问题排查 (Scale Not Triggering Troubleshooting)

### 3.1 扩容不触发问题

```bash
# ========== 1. Pending Pod分析 ==========
# 查看Pending Pod的详细信息
kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .status.conditions[?(@.type=="PodScheduled")].reason
}{
        "\t"
}{
        .status.conditions[?(@.type=="PodScheduled")].message
}{
        "\n"
}{
    end
}'

# 分析调度失败原因
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedScheduling --sort-by='.lastTimestamp'

# 检查资源请求是否合理
kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.containers[*].resources.requests
}{
        "\n"
}{
    end
}'

# ========== 2. 节点组配置检查 ==========
# 检查节点组大小限制
NODE_GROUPS=$(kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.labels."cluster-autoscaler\.kubernetes\.io/node-group"
}{
        "\n"
}{
    end
}' | sort | uniq)

for ng in $NODE_GROUPS; do
    echo "Checking node group: $ng"
    kubectl get nodes -l "cluster-autoscaler.kubernetes.io/node-group=$ng" --no-headers | wc -l
    # 检查对应的云提供商节点组配置
done

# 验证Autoscaler节点组配置
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "nodegroup\|max\|min"

# ========== 3. 资源评估问题 ==========
# 检查Autoscaler资源评估
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "calculating.*resources\|insufficient.*resources"

# 分析节点资源碎片化
kubectl describe nodes | grep -E "(CPU Requests|Memory Requests)" -A3

# 检查Pod反亲和性影响
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .spec.affinity
}{
        "\n"
}{
    end
}' | grep -E "(podAntiAffinity|nodeAffinity)"
```

### 3.2 缩容不触发问题

```bash
# ========== 1. 节点利用情况检查 ==========
# 查找低利用率节点
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.allocatable.cpu
}{
        "\t"
}{
        .status.allocatable.memory
}{
        "\n"
}{
    end
}' | while read node cpu mem; do
    echo "Checking node: $node"
    # 计算节点实际使用率
    POD_CPU=$(kubectl top nodes $node --no-headers | awk '{print $2}' | sed 's/m//')
    POD_MEM=$(kubectl top nodes $node --no-headers | awk '{print $4}' | sed 's/Mi//')
    
    CPU_UTIL=$(echo "scale=2; $POD_CPU / (${cpu}000) * 100" | bc)
    MEM_UTIL=$(echo "scale=2; $POD_MEM / ${mem%Gi} * 100" | bc)
    
    echo "  CPU utilization: ${CPU_UTIL}%"
    echo "  Memory utilization: ${MEM_UTIL}%"
    
    if (( $(echo "$CPU_UTIL < 50" | bc -l) )) && (( $(echo "$MEM_UTIL < 50" | bc -l) )); then
        echo "  ⚠️  Low utilization node candidate"
    fi
done

# ========== 2. 缩容障碍检查 ==========
# 检查不可驱逐的Pod
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.annotations."cluster-autoscaler\.kubernetes\.io/safe-to-evict"
}{
        "\n"
}{
    end
}' | grep -E "(false|$)"

# 查看系统关键Pod
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[?(@.metadata.namespace=="kube-system")]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.priorityClassName
}{
        "\n"
}{
    end
}' | grep -E "(system-cluster-critical|system-node-critical)"

# 检查PodDisruptionBudget影响
kubectl get pdb --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .spec.minAvailable
}{
        "\t"
}{
        .status.disruptionsAllowed
}{
        "\n"
}{
    end
}'
```

---

## 4. 扩缩容震荡问题排查 (Scale Oscillation Troubleshooting)

### 4.1 震荡检测和分析

```bash
# ========== 1. 历史扩缩容事件分析 ==========
# 查看Autoscaler历史决策
kubectl logs -n kube-system -l app=cluster-autoscaler --since=24h | grep -E "(scale up|scale down|removing node|adding node)"

# 统计扩缩容频率
kubectl logs -n kube-system -l app=cluster-autoscaler --since=1h | grep -c "scale up\|scale down"

# 分析节点生命周期
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.creationTimestamp
}{
        "\n"
}{
    end
}' | sort -k2

# ========== 2. 配置参数检查 ==========
# 检查扩缩容延迟配置
kubectl get deployment cluster-autoscaler -n kube-system -o jsonpath='{
    .spec.template.spec.containers[0].command
}' | grep -E "(scale-down-delay|scale-up-delay|expander)"

# 验证稳定窗口设置
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "stabilization\|delay"

# 检查扩缩容阈值配置
kubectl get deployment cluster-autoscaler -n kube-system -o jsonpath='{
    .spec.template.spec.containers[0].command
}' | grep -E "(threshold|utilization)"
```

### 4.2 震荡缓解策略

```bash
# ========== 参数优化配置 ==========
# 调整扩缩容延迟参数
cat <<EOF > autoscaler-optimized-config.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        command:
        - ./cluster-autoscaler
        - --v=4
        - --stderrthreshold=info
        - --cloud-provider=aws  # 根据实际云提供商调整
        - --skip-nodes-with-local-storage=false
        - --skip-nodes-with-system-pods=false
        - --scale-down-delay-after-add=10m      # 增加扩容后的缩容延迟
        - --scale-down-delay-after-delete=0s
        - --scale-down-delay-after-failure=3m
        - --scale-down-unneeded-time=10m        # 增加不必要的节点判定时间
        - --scale-down-unready-time=20m
        - --scale-down-utilization-threshold=0.5 # 调整利用率阈值
        - --max-node-provision-time=15m
        - --balance-similar-node-groups=true
        - --expander=least-waste
EOF

# ========== 节点组优化 ==========
# 配置多样化的节点组
cat <<EOF > diversified-nodegroups.yaml
# 多种实例类型的节点组配置示例
NodeGroups:
- Name: general-purpose-small
  InstanceType: t3.medium
  MinSize: 2
  MaxSize: 10
  Labels:
    node.kubernetes.io/instance-type: t3.medium
    node.kubernetes.io/lifecycle: spot

- Name: general-purpose-large
  InstanceType: m5.large
  MinSize: 1
  MaxSize: 5
  Labels:
    node.kubernetes.io/instance-type: m5.large
    node.kubernetes.io/lifecycle: on-demand

- Name: compute-intensive
  InstanceType: c5.xlarge
  MinSize: 0
  MaxSize: 3
  Labels:
    node.kubernetes.io/instance-type: c5.xlarge
    node.kubernetes.io/lifecycle: spot
EOF

# ========== 工作负载优化 ==========
# 为应用设置合适的资源请求
cat <<EOF > workload-optimization.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: optimized-app
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: myapp:latest
        resources:
          requests:
            cpu: "500m"      # 合理的请求值
            memory: "1Gi"
          limits:
            cpu: "1000m"     # 适当的限制值
            memory: "2Gi"
        # 添加反亲和性减少在同一节点调度
        affinity:
          podAntiAffinity:
            preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchExpressions:
                  - key: app
                    operator: In
                    values:
                    - optimized-app
                topologyKey: kubernetes.io/hostname
EOF
```

---

## 5. 节点驱逐和迁移问题 (Node Draining and Migration Issues)

### 5.1 驱逐失败问题

```bash
# ========== 1. 驱逐障碍分析 ==========
# 查看无法驱逐的Pod
kubectl get pods --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.annotations."cluster-autoscaler\.kubernetes\.io/safe-to-evict"
}{
        "\n"
}{
    end
}' | grep "false"

# 检查PDB限制
kubectl get pdb --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .spec.minAvailable
}{
        "\t"
}{
        .status.currentHealthy
}{
        "\t"
}{
        .status.desiredHealthy
}{
        "\n"
}{
    end
}'

# 分析驱逐失败原因
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "evict\|drain\|failed" --color=never

# ========== 2. 手动驱逐测试 ==========
# 测试节点驱逐
NODE_TO_DRAIN=$(kubectl get nodes -o jsonpath='{
    .items[0].metadata.name
}')
echo "Testing drain on node: $NODE_TO_DRAIN"

# 模拟Autoscaler驱逐
kubectl drain $NODE_TO_DRAIN --ignore-daemonsets --delete-emptydir-data --timeout=300s

# 检查驱逐结果
kubectl get pods --field-selector=spec.nodeName=$NODE_TO_DRAIN

# 恢复节点
kubectl uncordon $NODE_TO_DRAIN
```

### 5.2 Pod迁移性能优化

```bash
# ========== 迁移时间监控 ==========
# 监控Pod驱逐和重新调度时间
cat <<'EOF' > pod-migration-monitor.sh
#!/bin/bash

NODE_NAME=$1
INTERVAL=${2:-60}

if [ -z "$NODE_NAME" ]; then
    echo "Usage: $0 <node-name> [interval]"
    exit 1
fi

echo "Monitoring pod migration for node: $NODE_NAME"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 获取节点上的Pod列表
    PODS=$(kubectl get pods --all-namespaces --field-selector=spec.nodeName=$NODE_NAME -o jsonpath='{.items[*].metadata.name}')
    
    if [ -n "$PODS" ]; then
        echo "$TIMESTAMP - Pods on node: $PODS"
        
        # 检查Pod状态变化
        for pod in $PODS; do
            NS=$(kubectl get pod $pod --all-namespaces --field-selector=spec.nodeName=$NODE_NAME -o jsonpath='{.items[0].metadata.namespace}')
            STATUS=$(kubectl get pod $pod -n $NS -o jsonpath='{.status.phase}')
            NODE=$(kubectl get pod $pod -n $NS -o jsonpath='{.spec.nodeName}')
            
            echo "  $pod: $STATUS on $NODE"
        done
    else
        echo "$TIMESTAMP - No pods found on node $NODE_NAME"
    fi
    
    echo "---"
    sleep $INTERVAL
done
EOF

chmod +x pod-migration-monitor.sh

# ========== 驱逐策略优化 ==========
# 配置优雅驱逐参数
cat <<EOF > eviction-optimization.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: eviction-optimized-app
  namespace: production
spec:
  template:
    metadata:
      annotations:
        # 设置驱逐容忍时间
        cluster-autoscaler.kubernetes.io/safe-to-evict: "true"
    spec:
      terminationGracePeriodSeconds: 30  # 缩短优雅终止时间
      containers:
      - name: app
        image: myapp:latest
        # 添加preStop钩子实现优雅关闭
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]
EOF

# ========== PDB配置最佳实践 ==========
cat <<EOF > pdb-best-practices.yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
  namespace: production
spec:
  minAvailable: 2  # 确保至少2个Pod可用
  selector:
    matchLabels:
      app: my-app
---
# 为关键系统组件设置PDB
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-addons-pdb
  namespace: kube-system
spec:
  minAvailable: 1
  selector:
    matchLabels:
      k8s-app: kube-dns
EOF
```

---

## 6. 云提供商集成问题 (Cloud Provider Integration Issues)

### 6.1 云API调用问题

```bash
# ========== 1. 云提供商配置检查 ==========
# 检查云提供商凭据
kubectl get secrets -n kube-system | grep -E "(aws|azure|gcp)"

# 验证云API访问权限
# AWS示例
kubectl run aws-cli-test --image=amazon/aws-cli -n kube-system -it --rm -- sh -c "
aws sts get-caller-identity
aws ec2 describe-instances --filters Name=tag-key,Values=k8s.io/cluster-autoscaler/enabled
"

# GCP示例
kubectl run gcp-cli-test --image=gcr.io/google.com/cloudsdktool/cloud-sdk -n kube-system -it --rm -- sh -c "
gcloud auth list
gcloud compute instances list --filter='labels.cluster-name:*'
"

# Azure示例
kubectl run azure-cli-test --image=mcr.microsoft.com/azure-cli -n kube-system -it --rm -- sh -c "
az account show
az vm list --query '[].{Name:name, ResourceGroup:resourceGroup}' -o table
"

# ========== 2. 节点组配置验证 ==========
# AWS ASG配置检查
aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names <asg-name> | jq '{
    AutoScalingGroupName,
    MinSize,
    MaxSize,
    DesiredCapacity,
    AvailabilityZones
}'

# GCP实例组检查
gcloud compute instance-groups managed describe <instance-group-name> --zone=<zone>

# Azure虚拟机规模集检查
az vmss show --name <vmss-name> --resource-group <rg-name>

# ========== 3. API调用监控 ==========
# 监控云API调用成功率
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "cloud.*api\|failed.*request" --color=never

# 检查API速率限制
kubectl logs -n kube-system -l app=cluster-autoscaler | grep -i "rate.*limit\|throttle" --color=never
```

### 6.2 成本优化配置

```bash
# ========== 混合实例策略 ==========
# Spot实例配置示例
cat <<EOF > spot-instance-config.yaml
# AWS Spot实例配置
NodeGroup:
  Name: spot-worker-pool
  InstanceTypes: 
    - t3.large
    - t3.xlarge
    - t3.2xlarge
  SpotInstanceDraining: true
  Lifecycle: spot
  MaxPrice: "0.05"  # 最高价格
  OnDemandBaseCapacity: 1  # 基础按需实例数
  OnDemandPercentageAboveBaseCapacity: 0  # 其余100%使用Spot
EOF

# ========== 成本监控集成 ==========
# 配置成本感知扩缩容
cat <<EOF > cost-aware-autoscaling.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: cluster-autoscaler
        command:
        - ./cluster-autoscaler
        - --expander=price  # 基于价格的扩缩容器
        - --price-configmap=pricing-model  # 价格模型配置
        - --balancing-ignore-label=instance-type  # 忽略实例类型平衡
        env:
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: access-key-id
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: secret-access-key
EOF

# ========== 资源优化建议 ==========
# 基于历史使用情况的节点组优化
cat <<'EOF' > resource-optimization-analyzer.sh
#!/bin/bash

echo "Analyzing resource usage for autoscaling optimization"

# 收集一周的资源使用数据
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\n"
}{
    end
}' | while read node; do
    echo "Analyzing node: $node"
    
    # 获取CPU使用历史
    CPU_USAGE=$(kubectl top node $node --no-headers | awk '{print $2}' | sed 's/m//')
    echo "  Current CPU usage: ${CPU_USAGE}m"
    
    # 获取内存使用历史
    MEM_USAGE=$(kubectl top node $node --no-headers | awk '{print $4}' | sed 's/Mi//')
    echo "  Current Memory usage: ${MEM_USAGE}Mi"
    
    # 分析Pod分布
    POD_COUNT=$(kubectl get pods --field-selector=spec.nodeName=$node --no-headers | wc -l)
    echo "  Pod count: $POD_COUNT"
done

# 建议节点组大小调整
echo "Recommended optimizations:"
echo "1. Adjust min/max sizes based on usage patterns"
echo "2. Consider mixed instance types for cost optimization"
echo "3. Review spot instance usage during peak hours"
EOF

chmod +x resource-optimization-analyzer.sh
```

---

## 7. 监控和告警配置 (Monitoring and Alerting)

### 7.1 Autoscaler监控指标

```bash
# ========== 监控配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: cluster-autoscaler-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: cluster-autoscaler
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: autoscaler-alerts
  namespace: monitoring
spec:
  groups:
  - name: autoscaler.rules
    rules:
    - alert: ClusterAutoscalerDown
      expr: up{job="cluster-autoscaler"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Cluster Autoscaler is down"
        
    - alert: ScalingNotTriggered
      expr: cluster_autoscaler_unschedulable_pods_count > 5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "High number of unschedulable pods ({{ \$value }}) but no scaling triggered"
        
    - alert: ScaleOscillationDetected
      expr: increase(cluster_autoscaler_scaled_up_nodes_total[10m]) > 5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Frequent scale up events detected ({{ \$value }} in 10 minutes)"
        
    - alert: NodeDrainFailure
      expr: cluster_autoscaler_evictions_skipped_total > 10
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node drain failures detected ({{ \$value }} pods skipped)"
        
    - alert: CloudAPIErrors
      expr: rate(cluster_autoscaler_cloud_provider_api_errors_total[5m]) > 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Cloud provider API errors ({{ \$value }}/sec)"
        
    - alert: CostAnomaly
      expr: cluster_autoscaler_cost_per_hour > 100
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "High autoscaling cost detected (${{ \$value }}/hour)"
EOF
```

### 7.2 性能分析工具

```bash
# ========== Autoscaler性能分析 ==========
cat <<'EOF' > autoscaler-performance-analyzer.sh
#!/bin/bash

NAMESPACE=${1:-kube-system}
DURATION=${2:-3600}  # 1小时分析

echo "Analyzing Cluster Autoscaler performance for $DURATION seconds"

# 收集性能指标
kubectl logs -n $NAMESPACE -l app=cluster-autoscaler --since=${DURATION}s | \
grep -E "(scaled up|scaled down|evicted|added|removed)" | \
awk '{print $1" "$2" "$NF}' > /tmp/autoscaler-events.txt

echo "=== Scaling Events Summary ==="
echo "Scale up events: $(grep "scaled up" /tmp/autoscaler-events.txt | wc -l)"
echo "Scale down events: $(grep "scaled down" /tmp/autoscaler-events.txt | wc -l)"
echo "Pod evictions: $(grep "evicted" /tmp/autoscaler-events.txt | wc -l)"

# 分析时间间隔
echo "=== Timing Analysis ==="
if [ -s /tmp/autoscaler-events.txt ]; then
    echo "First event: $(head -1 /tmp/autoscaler-events.txt)"
    echo "Last event: $(tail -1 /tmp/autoscaler-events.txt)"
    
    # 计算事件频率
    EVENT_COUNT=$(wc -l < /tmp/autoscaler-events.txt)
    echo "Total events: $EVENT_COUNT"
    echo "Events per minute: $(echo "scale=2; $EVENT_COUNT * 60 / $DURATION" | bc)"
fi

# 清理临时文件
rm /tmp/autoscaler-events.txt

echo "Performance analysis completed"
EOF

chmod +x autoscaler-performance-analyzer.sh

# ========== 成本效益分析 ==========
cat <<'EOF' > cost-benefit-analyzer.sh
#!/bin/bash

echo "Performing autoscaling cost-benefit analysis"

# 计算当前集群成本
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.capacity.cpu
}{
        "\t"
}{
        .status.capacity.memory
}{
        "\n"
}{
    end
}' | while read node cpu mem; do
    # 基于实例类型估算成本 (示例价格)
    if [[ $node == *"spot"* ]]; then
        HOURLY_COST=0.02  # Spot实例假设价格
    else
        HOURLY_COST=0.10  # 按需实例假设价格
    fi
    
    DAILY_COST=$(echo "$HOURLY_COST * 24" | bc)
    MONTHLY_COST=$(echo "$DAILY_COST * 30" | bc)
    
    echo "Node $node: \$${MONTHLY_COST}/month"
done

# 分析资源利用率
echo "=== Resource Utilization Analysis ==="
kubectl top nodes | tail -n +2 | while read node cpu cpu_percent mem mem_percent; do
    echo "$node: CPU ${cpu_percent}, Memory ${mem_percent}"
done

echo "Analysis complete. Consider adjusting node groups based on utilization patterns."
EOF

chmod +x cost-benefit-analyzer.sh
```

---