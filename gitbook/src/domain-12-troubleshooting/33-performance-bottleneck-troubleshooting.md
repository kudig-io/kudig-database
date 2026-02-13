# 33 - 性能瓶颈故障排查 (Performance Bottleneck Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Performance](https://kubernetes.io/docs/setup/best-practices/cluster-large/)

---

## 1. 性能瓶颈故障诊断总览 (Performance Bottleneck Diagnosis Overview)

### 1.1 常见性能瓶颈分类

| 瓶颈类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **CPU资源不足** | 调度延迟/应用响应慢 | 计算密集型应用 | P1 - 高 |
| **内存压力** | OOMKilled/频繁GC | 内存敏感应用 | P0 - 紧急 |
| **磁盘I/O瓶颈** | 存储延迟/写入缓慢 | 数据库/存储应用 | P1 - 高 |
| **网络带宽限制** | 通信延迟/丢包 | 微服务通信 | P1 - 高 |
| **API Server压力** | 请求超时/限流 | 集群管理操作 | P0 - 紧急 |

### 1.2 性能监控架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  性能瓶颈故障诊断架构                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       应用层监控                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   应用指标   │   │   业务指标   │   │   用户体验   │              │  │
│  │  │ (App Metrics)│   │ (Biz Metrics)│   │ (UX Metrics)│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   容器层     │   │   Pod层     │   │   节点层     │                   │
│  │ (Container) │   │  (Pod)      │   │  (Node)     │                   │
│  │   性能      │   │   性能      │   │   性能      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      基础设施层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   CPU调度   │   │   内存管理   │   │   存储I/O    │              │  │
│  │  │ (Scheduler) │   │ (Memory)    │   │  (Storage)  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   网络层     │   │   集群层     │   │   云服务层   │                   │
│  │ (Network)   │   │ (Cluster)   │   │ (Cloud)     │                   │
│  │   性能      │   │   性能      │   │   性能      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      监控告警层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   指标收集   │   │   异常检测   │   │   根因分析   │              │  │
│  │  │ (Metrics)   │   │ (Anomaly)   │   │  (Root Cause)│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. CPU性能瓶颈排查 (CPU Performance Bottleneck Troubleshooting)

### 2.1 CPU使用率分析

```bash
# ========== 1. 集群CPU使用概况 ==========
# 查看节点CPU使用情况
kubectl top nodes

# 分析CPU使用率最高的节点
kubectl top nodes --sort-by=cpu | tail -5

# 查看Pod CPU使用情况
kubectl top pods --all-namespaces --sort-by=cpu | head -10

# ========== 2. CPU资源分配检查 ==========
# 检查CPU请求和限制配置
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
        .spec.containers[*].resources.requests.cpu
}{
        "\t"
}{
        .spec.containers[*].resources.limits.cpu
}{
        "\n"
}{
    end
}' | sort -k3 -n | tail -10

# 分析CPU资源碎片化
kubectl describe nodes | grep -E "(CPU Requests|CPU Limits)" -A3

# 检查CPU压力事件
kubectl get events --field-selector reason=SystemOOM,type=Warning --sort-by='.lastTimestamp'

# ========== 3. 进程级别CPU分析 ==========
# 在节点上分析CPU使用
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "
        echo 'Top CPU consuming processes:'
        ps aux --sort=-%cpu | head -10
        echo 'CPU info:'
        cat /proc/cpuinfo | grep processor | wc -l
        echo 'Load average:'
        uptime
    "
done
```

### 2.2 CPU调度性能优化

```bash
# ========== 调度器性能检查 ==========
# 检查调度器延迟指标
kubectl port-forward -n kube-system svc/prometheus-k8s 9090:9090 &
sleep 3

# 查询调度延迟
curl -s "http://localhost:9090/api/v1/query?query=scheduler_e2e_scheduling_duration_seconds" | jq

# 检查调度队列长度
curl -s "http://localhost:9090/api/v1/query?query=scheduler_pending_pods" | jq

# 分析调度失败原因
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedScheduling --sort-by='.lastTimestamp'

# 清理端口转发
kill %1 2>/dev/null

# ========== CPU亲和性优化 ==========
# 为CPU密集型应用配置亲和性
cat <<EOF > cpu-intensive-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cpu-intensive-app
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
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
        # CPU亲和性配置
        env:
        - name: GOMAXPROCS
          value: "4"
      # 节点亲和性确保调度到合适的节点
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node.kubernetes.io/instance-type
                operator: In
                values:
                - c5.xlarge
                - c5.2xlarge
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - cpu-intensive-app
              topologyKey: kubernetes.io/hostname
EOF

# ========== CPU管理策略 ==========
# 启用静态CPU管理策略
cat <<EOF > cpu-manager-config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cpuManagerPolicy: static
systemReserved:
  cpu: "500m"
  memory: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
EOF
```

---

## 3. 内存性能瓶颈排查 (Memory Performance Bottleneck Troubleshooting)

### 3.1 内存使用分析

```bash
# ========== 1. 内存使用概况 ==========
# 查看节点内存使用情况
kubectl top nodes --sort-by=memory

# 分析内存使用最高的Pod
kubectl top pods --all-namespaces --sort-by=memory | head -10

# 检查内存压力事件
kubectl get events --field-selector reason=SystemOOM --sort-by='.lastTimestamp'

# ========== 2. 内存泄漏检测 ==========
# 监控内存增长趋势
cat <<'EOF' > memory-leak-detector.sh
#!/bin/bash

NAMESPACE=${1:-default}
INTERVAL=${2:-60}

echo "Monitoring memory usage for namespace: $NAMESPACE"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    kubectl top pods -n $NAMESPACE --no-headers | while read pod cpu mem; do
        # 记录内存使用
        echo "$TIMESTAMP,$pod,$mem" >> /tmp/memory-usage.csv
        
        # 检测快速增长
        if [ -f "/tmp/prev-$pod.mem" ]; then
            PREV_MEM=$(cat /tmp/prev-$pod.mem)
            CURR_MEM_NUM=$(echo $mem | sed 's/Mi//')
            PREV_MEM_NUM=$(echo $PREV_MEM | sed 's/Mi//')
            
            if (( $(echo "$CURR_MEM_NUM > $PREV_MEM_NUM * 1.1" | bc -l) )); then
                echo "$TIMESTAMP - ⚠️  Rapid memory growth detected for $pod: $PREV_MEM -> $mem"
            fi
        fi
        
        echo $mem > /tmp/prev-$pod.mem
    done
    
    sleep $INTERVAL
done
EOF

chmod +x memory-leak-detector.sh

# ========== 3. 内存压力分析 ==========
# 检查节点内存压力
kubectl describe nodes | grep -A10 "MemoryPressure"

# 分析内存分配情况
kubectl describe nodes | grep -E "(Memory Requests|Memory Limits)" -A3

# 检查OOMKilled事件
kubectl get events --all-namespaces --field-selector reason=OOMKilling --sort-by='.lastTimestamp'
```

### 3.2 内存优化策略

```bash
# ========== 内存限制配置 ==========
# 为内存敏感应用配置合理的限制
cat <<EOF > memory-optimized-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: memory-sensitive-app
  namespace: production
spec:
  replicas: 2
  template:
    spec:
      containers:
      - name: app
        image: myapp:latest
        resources:
          requests:
            memory: "1Gi"
            cpu: "500m"
          limits:
            memory: "2Gi"  # 设置上限防止OOM
            cpu: "1000m"
        # 内存优化环境变量
        env:
        - name: JAVA_OPTS
          value: "-Xmx1536m -Xms1024m -XX:+UseG1GC"
        - name: GOGC
          value: "20"  # Go垃圾回收触发比例
      # QoS等级设置
      priorityClassName: high-priority
EOF

# ========== 内存压力测试 ==========
# 创建内存压力测试工具
cat <<'EOF' > memory-stress-test.sh
#!/bin/bash

TARGET_NAMESPACE=${1:-test}
MEMORY_SIZE=${2:-1Gi}

echo "Creating memory stress test in namespace: $TARGET_NAMESPACE"

cat <<DEPLOYMENT | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: memory-stress-test
  namespace: $TARGET_NAMESPACE
spec:
  replicas: 1
  selector:
    matchLabels:
      app: memory-stress
  template:
    metadata:
      labels:
        app: memory-stress
    spec:
      containers:
      - name: stress-ng
        image: alexeiled/stress-ng
        args: ["--vm", "1", "--vm-bytes", "$MEMORY_SIZE", "--timeout", "300s"]
        resources:
          requests:
            memory: "$MEMORY_SIZE"
            cpu: "100m"
          limits:
            memory: "$MEMORY_SIZE"
            cpu: "500m"
DEPLOYMENT

# 监控测试期间的内存使用
echo "Monitoring memory usage during stress test..."
for i in {1..30}; do
    kubectl top pods -n $TARGET_NAMESPACE | grep memory-stress
    sleep 10
done

# 清理测试资源
kubectl delete deployment memory-stress-test -n $TARGET_NAMESPACE
echo "Stress test completed"
EOF

chmod +x memory-stress-test.sh
```

---

## 4. 存储I/O性能瓶颈排查 (Storage I/O Performance Bottleneck Troubleshooting)

### 4.1 存储性能分析

```bash
# ========== 1. 存储I/O监控 ==========
# 检查PV/PVC性能
kubectl get pvc --all-namespaces -o jsonpath='{
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
        .spec.storageClassName
}{
        "\t"
}{
        .status.capacity.storage
}{
        "\n"
}{
    end
}'

# 分析存储类性能特征
kubectl get storageclass -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .provisioner
}{
        "\t"
}{
        .parameters.type
}{
        "\n"
}{
    end
}'

# ========== 2. 存储延迟测试 ==========
# 创建存储性能测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: storage-benchmark
  namespace: default
spec:
  containers:
  - name: fio
    image: ljishen/fio
    command: ["fio"]
    args:
    - "--name=test"
    - "--rw=randrw"
    - "--bs=4k"
    - "--iodepth=16"
    - "--size=1g"
    - "--runtime=60"
    - "--direct=1"
    - "--filename=/data/testfile"
    volumeMounts:
    - name: test-volume
      mountPath: /data
  volumes:
  - name: test-volume
    persistentVolumeClaim:
      claimName: test-pvc  # 需要预先创建PVC
EOF

# ========== 3. 存储瓶颈诊断 ==========
# 分析存储I/O模式
cat <<'EOF' > storage-io-analyzer.sh
#!/bin/bash

NAMESPACE=${1:-default}

echo "Analyzing storage I/O patterns for namespace: $NAMESPACE"

# 收集存储相关指标
kubectl get pods -n $NAMESPACE -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.volumes[*].persistentVolumeClaim.claimName
}{
        "\n"
}{
    end
}' | while IFS=$'\t' read pod pvc; do
    if [ -n "$pvc" ]; then
        echo "Pod: $pod, PVC: $pvc"
        
        # 检查PVC状态
        kubectl describe pvc $pvc -n $NAMESPACE | grep -E "(Status|Access Modes|StorageClass)"
        
        # 分析存储使用情况
        kubectl exec -n $NAMESPACE $pod -- df -h 2>/dev/null | grep -v tmpfs || echo "Cannot access filesystem"
    fi
done

# 检查存储类性能
kubectl get storageclass -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .parameters.iops
}{
        "\t"
}{
        .parameters.throughput
}{
        "\n"
}{
    end
}'
EOF

chmod +x storage-io-analyzer.sh
```

### 4.2 存储优化策略

```bash
# ========== 高性能存储配置 ==========
# 配置高性能存储类
cat <<EOF > high-performance-storage.yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  fsType: ext4
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
mountOptions:
  - discard  # 启用TRIM
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: high-performance-pvc
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 100Gi
EOF

# ========== 存储缓存优化 ==========
# 配置本地存储缓存
cat <<EOF > local-storage-cache.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: local-cache-agent
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: local-cache
  template:
    metadata:
      labels:
        app: local-cache
    spec:
      containers:
      - name: cache-agent
        image: redis:alpine
        ports:
        - containerPort: 6379
        volumeMounts:
        - name: cache-storage
          mountPath: /data
        resources:
          requests:
            memory: "256Mi"
            cpu: "100m"
          limits:
            memory: "512Mi"
            cpu: "200m"
      volumes:
      - name: cache-storage
        hostPath:
          path: /var/lib/local-cache
          type: DirectoryOrCreate
---
apiVersion: v1
kind: Service
metadata:
  name: local-cache-service
  namespace: kube-system
spec:
  selector:
    app: local-cache
  ports:
  - port: 6379
    targetPort: 6379
EOF
```

---

## 5. 网络性能瓶颈排查 (Network Performance Bottleneck Troubleshooting)

### 5.1 网络延迟和吞吐量分析

```bash
# ========== 1. 网络连通性测试 ==========
# Pod间网络延迟测试
kubectl run network-test --image=nicolaka/netshoot -n <namespace> -it --rm -- sh -c "
echo 'Testing pod-to-pod latency:'
for pod_ip in \$(kubectl get pods -n <namespace> -o jsonpath='{.items[*].status.podIP}'); do
    echo \"Testing \$pod_ip\"
    ping -c 3 \$pod_ip
done
"

# Service网络测试
kubectl run network-test --image=nicolaka/netshoot -n <namespace> -it --rm -- sh -c "
echo 'Testing service connectivity:'
nslookup <service-name>.<namespace>.svc.cluster.local
curl -w \"@curl-format.txt\" -o /dev/null -s http://<service-name>.<namespace>.svc.cluster.local
"

# ========== 2. 网络带宽测试 ==========
# 创建网络性能测试工具
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: iperf-server
  namespace: default
spec:
  containers:
  - name: iperf
    image: networkstatic/iperf3
    command: ["iperf3", "-s"]
    ports:
    - containerPort: 5201
---
apiVersion: v1
kind: Pod
metadata:
  name: iperf-client
  namespace: default
spec:
  containers:
  - name: iperf
    image: networkstatic/iperf3
    command: ["sleep", "3600"]
EOF

# 执行带宽测试
kubectl exec iperf-client -- iperf3 -c iperf-server.default.pod.cluster.local -t 30 -P 4

# ========== 3. 网络策略影响分析 ==========
# 检查网络策略配置
kubectl get networkpolicy --all-namespaces -o jsonpath='{
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
        .spec.podSelector
}{
        "\n"
}{
    end
}'

# 分析网络策略复杂度
kubectl get networkpolicy --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.ingress
}{
        "\t"
}{
        .spec.egress
}{
        "\n"
}{
    end
}' | wc -l
```

### 5.2 网络优化策略

```bash
# ========== 网络插件优化 ==========
# Calico网络策略优化
cat <<EOF > calico-optimization.yaml
apiVersion: crd.projectcalico.org/v1
kind: FelixConfiguration
metadata:
  name: default
spec:
  # 性能优化参数
  reportingInterval: 0s  # 禁用报告以提高性能
  prometheusMetricsEnabled: true
  prometheusGoMetricsEnabled: false
  prometheusProcessMetricsEnabled: false
  
  # 连接跟踪优化
  conntrackMax: 1048576  # 增加连接跟踪表大小
  conntrackTCPLiberal: true
  
  # 日志级别优化
  logSeverityScreen: Warning  # 减少日志输出
---
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  ipipMode: Never  # 在支持的环境中禁用IPIP封装
  vxlanMode: CrossSubnet  # 仅跨子网使用VXLAN
  natOutgoing: true
  blockSize: 24
EOF

# ========== 服务网格优化 ==========
# Istio性能优化配置
cat <<EOF > istio-performance-optimization.yaml
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
metadata:
  name: istio-performance
spec:
  components:
    pilot:
      k8s:
        resources:
          requests:
            cpu: 1000m
            memory: 2Gi
          limits:
            cpu: 2000m
            memory: 4Gi
    ingressGateways:
    - name: istio-ingressgateway
      enabled: true
      k8s:
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 1000m
            memory: 2Gi
            
  values:
    global:
      proxy:
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 2000m
            memory: 1Gi
            
    pilot:
      env:
        PILOT_PUSH_THROTTLE: "100"  # 增加推送节流限制
        PILOT_TRACE_SAMPLING: "1"   # 降低追踪采样率
        
    telemetry:
      v2:
        prometheus:
          enabled: false  # 禁用高开销的遥测功能
EOF
```

---

## 6. API Server性能瓶颈排查 (API Server Performance Bottleneck Troubleshooting)

### 6.1 API Server性能监控

```bash
# ========== 1. API Server指标收集 ==========
# 检查API Server性能指标
kubectl port-forward -n kube-system svc/prometheus-k8s 9090:9090 &
sleep 3

# 查询关键性能指标
API_METRICS=(
    "apiserver_request_total"
    "apiserver_request_latency_seconds"
    "apiserver_current_inflight_requests"
    "etcd_request_duration_seconds"
    "apiserver_admission_controller_admission_duration_seconds"
)

for metric in "${API_METRICS[@]}"; do
    echo "=== $metric ==="
    curl -s "http://localhost:9090/api/v1/query?query=$metric" | jq '.data.result[:3]'
    echo ""
done

# 检查API Server错误率
curl -s "http://localhost:9090/api/v1/query?query=rate(apiserver_request_total{code=~\"5..\"}[5m])" | jq

# 清理端口转发
kill %1 2>/dev/null

# ========== 2. API Server压力测试 ==========
# 创建API压力测试工具
cat <<'EOF' > api-server-stress-test.sh
#!/bin/bash

CONCURRENT_REQUESTS=${1:-10}
DURATION=${2:-60}

echo "Starting API Server stress test"
echo "Concurrent requests: $CONCURRENT_REQUESTS"
echo "Duration: $DURATION seconds"

# 并发API请求测试
for i in $(seq 1 $CONCURRENT_REQUESTS); do
    (
        for j in $(seq 1 $((DURATION / 5))); do
            kubectl get pods --all-namespaces >/dev/null 2>&1
            kubectl get nodes >/dev/null 2>&1
            kubectl get services --all-namespaces >/dev/null 2>&1
            sleep 5
        done
    ) &
done

# 等待所有后台任务完成
wait

echo "Stress test completed"
EOF

chmod +x api-server-stress-test.sh

# ========== 3. API Server配置优化 ==========
# 优化API Server配置
cat <<EOF > apiserver-optimization.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    - --max-requests-inflight=3000        # 增加并发请求数
    - --max-mutating-requests-inflight=1000
    - --request-timeout=2m                # 增加请求超时时间
    - --min-request-timeout=300           # 最小请求超时
    - --enable-priority-and-fairness=true # 启用优先级和公平性
    - --api-priority-and-fairness-config-producer-rate=100
    - --http2-max-streams-per-connection=1000
    - --etcd-compaction-interval=15m      # 优化etcd压缩间隔
    - --etcd-count-metric-poll-period=1m  # 增加etcd指标轮询频率
    - --watch-cache-sizes="*#1000"        # 增加watch缓存大小
EOF
```

### 6.2 API Server扩展策略

```bash
# ========== API Server水平扩展 ==========
# 配置API Server高可用
cat <<EOF > apiserver-ha-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  extraArgs:
    # 性能优化参数
    max-requests-inflight: "3000"
    max-mutating-requests-inflight: "1000"
    request-timeout: "2m"
    
    # 安全和认证优化
    anonymous-auth: "false"
    profiling: "false"
    
    # 缓存优化
    watch-cache: "true"
    default-watch-cache-size: "1000"
    
    # 速率限制
    enable-priority-and-fairness: "true"
  
  # 多副本配置
  replicas: 3
  certSANs:
  - "kubernetes.default.svc"
  - "kubernetes.default.svc.cluster.local"
  - "10.96.0.1"  # Service IP
  
controllerManager:
  extraArgs:
    concurrent-deployment-syncs: "10"
    concurrent-endpoint-syncs: "10"
    concurrent-gc-syncs: "30"
    
scheduler:
  extraArgs:
    profiling: "false"
EOF

# ========== API Server负载均衡 ==========
# 配置外部负载均衡器
cat <<EOF > apiserver-lb-config.yaml
apiVersion: v1
kind: Service
metadata:
  name: kubernetes-api-lb
  namespace: kube-system
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: nlb
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: arn:aws:acm:region:account:certificate/cert-id
spec:
  type: LoadBalancer
  ports:
  - name: https
    port: 6443
    targetPort: 6443
    protocol: TCP
  selector:
    component: kube-apiserver
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: kube-apiserver-pdb
  namespace: kube-system
spec:
  minAvailable: 2
  selector:
    matchLabels:
      component: kube-apiserver
EOF
```

---

## 7. 性能监控和告警 (Performance Monitoring and Alerting)

### 7.1 性能指标监控配置

```bash
# ========== 核心性能指标配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: performance-alerts
  namespace: monitoring
spec:
  groups:
  - name: performance.rules
    rules:
    # CPU性能告警
    - alert: HighCPUUsage
      expr: rate(container_cpu_usage_seconds_total[5m]) > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High CPU usage detected ({{ \$value }}%)"
        
    - alert: CPUSaturation
      expr: rate(container_cpu_usage_seconds_total[5m]) > 0.95
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "CPU saturation detected ({{ \$value }}%)"
        
    # 内存性能告警
    - alert: HighMemoryUsage
      expr: (container_memory_working_set_bytes / container_spec_memory_limit_bytes) > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High memory usage detected ({{ \$value }}%)"
        
    - alert: MemoryPressure
      expr: (container_memory_working_set_bytes / container_spec_memory_limit_bytes) > 0.95
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Memory pressure detected ({{ \$value }}%)"
        
    # 存储性能告警
    - alert: HighDiskIO
      expr: rate(container_fs_writes_bytes_total[5m]) > 100*1024*1024  # 100MB/s
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High disk I/O detected ({{ \$value }} bytes/sec)"
        
    # 网络性能告警
    - alert: HighNetworkLatency
      expr: histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High network latency detected ({{ \$value }}s)"
        
    # API Server性能告警
    - alert: APIServerLatencyHigh
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "API Server latency high ({{ \$value }}s)"
        
    - alert: APIServerErrors
      expr: rate(apiserver_request_total{code=~"5.."}[5m]) > 0.01
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "API Server error rate high ({{ \$value }}/sec)"
EOF
```

### 7.2 性能基准测试工具

```bash
# ========== 综合性能基准测试 ==========
cat <<'EOF' > performance-benchmark.sh
#!/bin/bash

TEST_NAMESPACE="perf-benchmark"
RESULTS_DIR="/tmp/performance-results-$(date +%Y%m%d-%H%M%S)"

mkdir -p $RESULTS_DIR

echo "=== Kubernetes Performance Benchmark ==="
echo "Results directory: $RESULTS_DIR"

# 1. CPU性能测试
echo "1. Running CPU performance test..."
kubectl run cpu-test --image=busybox -n $TEST_NAMESPACE -it --rm -- sh -c "
    time dd if=/dev/zero of=/dev/null bs=1M count=1000
" 2>&1 | tee $RESULTS_DIR/cpu-test.txt

# 2. 内存性能测试
echo "2. Running memory performance test..."
kubectl run mem-test --image=busybox -n $TEST_NAMESPACE -it --rm -- sh -c "
    time dd if=/dev/zero of=/tmp/testfile bs=1M count=500
    sync
    time dd if=/tmp/testfile of=/dev/null bs=1M
" 2>&1 | tee $RESULTS_DIR/mem-test.txt

# 3. 网络性能测试
echo "3. Running network performance test..."
kubectl run net-test --image=nicolaka/netshoot -n $TEST_NAMESPACE -it --rm -- sh -c "
    echo 'Testing DNS resolution:'
    time nslookup kubernetes.default.svc.cluster.local
    
    echo 'Testing internal connectivity:'
    time ping -c 10 kubernetes.default.svc.cluster.local
" 2>&1 | tee $RESULTS_DIR/net-test.txt

# 4. 存储性能测试
echo "4. Running storage performance test..."
kubectl apply -f - <<STORAGE_TEST
apiVersion: v1
kind: Pod
metadata:
  name: storage-test
  namespace: $TEST_NAMESPACE
spec:
  containers:
  - name: fio
    image: ljishen/fio
    command: ["fio"]
    args:
    - "--name=test"
    - "--rw=randrw"
    - "--bs=4k"
    - "--iodepth=16"
    - "--size=100m"
    - "--runtime=30"
    - "--direct=1"
    - "--group_reporting"
    volumeMounts:
    - name: test-volume
      mountPath: /data
  volumes:
  - name: test-volume
    emptyDir: {}
STORAGE_TEST

sleep 40  # 等待测试完成

# 5. API Server性能测试
echo "5. Running API Server performance test..."
for i in {1..100}; do
    kubectl get pods -n $TEST_NAMESPACE --no-headers >/dev/null 2>&1
done
echo "API calls completed: 100" | tee $RESULTS_DIR/api-test.txt

# 6. 生成综合报告
echo "6. Generating performance report..."
cat > $RESULTS_DIR/performance-report.txt <<REPORT
Kubernetes Performance Benchmark Report
======================================
Test Time: $(date)
Test Namespace: $TEST_NAMESPACE

Summary Results:
$(cat $RESULTS_DIR/*.txt | grep -E "(real|user|sys|completed|average)")

Detailed logs available in: $RESULTS_DIR
REPORT

cat $RESULTS_DIR/performance-report.txt

# 清理测试资源
kubectl delete namespace $TEST_NAMESPACE

echo "Performance benchmark completed"
echo "Results saved to: $RESULTS_DIR"
EOF

chmod +x performance-benchmark.sh

# ========== 性能趋势分析 ==========
cat <<'EOF' > performance-trend-analyzer.sh
#!/bin/bash

DAYS=${1:-7}
METRICS_SERVER="http://prometheus-k8s.monitoring.svc:9090"

echo "=== Performance Trend Analysis (Last $DAYS days) ==="

# 收集关键性能指标趋势
METRICS=(
    "rate(container_cpu_usage_seconds_total[5m])"
    "container_memory_working_set_bytes"
    "rate(container_network_receive_bytes_total[5m])"
    "histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))"
)

for metric in "${METRICS[@]}"; do
    echo "Analyzing: $metric"
    
    # 这里可以添加与Prometheus的集成来获取历史数据
    echo "  [Historical data collection logic would go here]"
    echo ""
done

echo "Trend analysis completed"
EOF

chmod +x performance-trend-analyzer.sh
```

---