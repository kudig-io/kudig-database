# 03 - Kubernetes 性能调优专家指南

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler | **质量等级**: ⭐⭐⭐⭐⭐ 专家级

> **性能优化实战宝典**: 基于万级节点集群性能优化经验，涵盖从系统调优到应用优化的全方位性能提升方案

---

## 目录

- [1. 系统性能瓶颈识别](#1-系统性能瓶颈识别)
- [2. 资源优化策略](#2-资源优化策略)
- [3. 调度器调优参数](#3-调度器调优参数)
- [4. 网络性能优化](#4-网络性能优化)
- [5. 存储IO调优](#5-存储io调优)
- [6. 应用层性能优化](#6-应用层性能优化)
- [7. 监控与基准测试](#7-监控与基准测试)

---

## 1. 系统性能瓶颈识别

### 1.1 性能瓶颈分类矩阵

| 瓶颈类型 | 典型症状 | 检测指标 | 影响程度 | 优化优先级 |
|---------|---------|---------|---------|-----------|
| **CPU瓶颈** | 应用响应慢、调度延迟 | CPU使用率>80%、Load Average高 | 高 | P0 |
| **内存瓶颈** | OOMKilled、频繁GC | 内存使用率>85%、Page Fault多 | 高 | P0 |
| **磁盘IO瓶颈** | 读写延迟高、吞吐量低 | IOPS饱和、Await时间长 | 中 | P1 |
| **网络瓶颈** | 通信延迟、丢包 | 带宽利用率>70%、RTT高 | 中 | P1 |
| **API Server瓶颈** | 请求超时、限流 | QPS过高、延迟增加 | 高 | P0 |
| **etcd瓶颈** | 数据读写慢、leader切换 | WAL延迟、fsync时间长 | 高 | P0 |

### 1.2 性能诊断工具链

```bash
#!/bin/bash
# ========== 性能综合诊断脚本 ==========
set -euo pipefail

NODE_NAME=${1:-"all-nodes"}
OUTPUT_DIR="/tmp/performance-analysis-$(date +%Y%m%d-%H%M%S)"

mkdir -p ${OUTPUT_DIR}
echo "性能分析报告生成中: ${OUTPUT_DIR}"

# 1. 系统级别性能数据收集
collect_system_metrics() {
    echo "=== 系统性能指标收集 ==="
    
    # CPU使用情况
    echo "CPU使用率统计:"
    kubectl top nodes | tee ${OUTPUT_DIR}/cpu-usage.txt
    
    # 内存使用情况
    echo -e "\n内存使用统计:"
    kubectl top pods -A --sort-by=memory | head -20 | tee ${OUTPUT_DIR}/memory-usage.txt
    
    # 节点资源压力
    echo -e "\n节点资源压力:"
    kubectl describe nodes | grep -E "(memory|cpu).*pressure" | tee ${OUTPUT_DIR}/resource-pressure.txt
}

# 2. 网络性能检测
check_network_performance() {
    echo -e "\n=== 网络性能检测 ==="
    
    # Pod间网络延迟测试
    kubectl run netperf-test --image=networkstatic/netperf --restart=Never \
      --overrides='{"spec":{"hostNetwork":true}}' -- \
      netperf -H 8.8.8.8 -t TCP_RR -- -r 64
    
    # DNS解析性能
    kubectl run dns-test --image=busybox --restart=Never -- \
      sh -c "for i in \$(seq 1 10); do time nslookup kubernetes.default; done" \
      2>&1 | tee ${OUTPUT_DIR}/dns-performance.txt
}

# 3. 存储性能测试
test_storage_performance() {
    echo -e "\n=== 存储性能测试 ==="
    
    # 创建存储性能测试Pod
    cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: storage-perf-test
spec:
  containers:
  - name: fio-test
    image: ljishen/fio
    command: ["fio"]
    args:
    - "--name=test"
    - "--rw=randrw"
    - "--bs=4k"
    - "--iodepth=16"
    - "--size=1g"
    - "--direct=1"
    - "--runtime=60"
    - "--time_based"
    volumeMounts:
    - name: test-volume
      mountPath: /data
  volumes:
  - name: test-volume
    persistentVolumeClaim:
      claimName: perf-test-pvc
EOF
    
    # 等待测试完成
    kubectl wait --for=condition=Ready pod/storage-perf-test --timeout=90s
    kubectl logs storage-perf-test > ${OUTPUT_DIR}/storage-performance.txt
    kubectl delete pod/storage-perf-test
}

# 4. API Server性能分析
analyze_api_server() {
    echo -e "\n=== API Server性能分析 ==="
    
    # API Server指标收集
    kubectl get --raw /metrics | grep -E "(apiserver_request_|etcd_|rest_client_)" \
      > ${OUTPUT_DIR}/api-server-metrics.txt
    
    # 请求延迟分析
    echo "API Server延迟分布:"
    kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket \
      | awk '{print $1}' | sort -n | tail -10 >> ${OUTPUT_DIR}/api-latency.txt
}

# 5. 应用性能剖析
profile_application() {
    echo -e "\n=== 应用性能剖析 ==="
    
    # Java应用堆栈分析
    kubectl get pods -n production -l app=java-app -o name | head -1 | \
      xargs -I {} kubectl exec {} -n production -- jstack 1 > ${OUTPUT_DIR}/java-thread-dump.txt
    
    # Go应用pprof分析
    kubectl port-forward svc/go-app-service 6060:6060 -n production &
    sleep 5
    curl -s http://localhost:6060/debug/pprof/profile?seconds=30 > ${OUTPUT_DIR}/go-profile.pb.gz
    kill %1
}

# 执行所有检查
collect_system_metrics
check_network_performance
test_storage_performance
analyze_api_server
profile_application

echo -e "\n性能分析完成，报告位置: ${OUTPUT_DIR}"
ls -la ${OUTPUT_DIR}
```

### 1.3 性能瓶颈识别流程

```mermaid
graph TD
    A[性能问题发现] --> B{确定瓶颈类型}
    B --> C[CPU瓶颈?]
    B --> D[内存瓶颈?]
    B --> E[IO瓶颈?]
    B --> F[网络瓶颈?]
    
    C --> C1[检查CPU使用率]
    C --> C2[分析进程CPU消耗]
    C --> C3[检查调度延迟]
    
    D --> D1[检查内存使用率]
    D --> D2[分析内存分配模式]
    D --> D3[检查页面交换]
    
    E --> E1[检查磁盘IO等待]
    E --> E2[分析存储类型性能]
    E --> E3[检查文件系统缓存]
    
    F --> F1[检查网络带宽使用]
    F --> F2[分析网络延迟]
    F --> F3[检查连接数限制]
    
    C1 --> G[定位热点函数]
    D2 --> H[优化内存分配]
    E2 --> I[调整存储配置]
    F2 --> J[优化网络策略]
    
    G --> K[应用层优化]
    H --> K
    I --> K
    J --> K
    
    K --> L[验证优化效果]
    L --> M{性能达标?}
    M -->|是| N[优化完成]
    M -->|否| A
```

---

## 2. 资源优化策略

### 2.1 CPU优化配置

```yaml
# ========== CPU优化配置模板 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cpu-optimized-app
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: app:v1.0
        resources:
          requests:
            # 基于实际使用量的95百分位
            cpu: "300m"
          limits:
            # 合理的上限，避免过度限制
            cpu: "1500m"
            
        # CPU亲和性设置
        env:
        - name: GOMAXPROCS
          value: "2"  # 限制Go运行时使用的CPU核心数
        - name: JAVA_TOOL_OPTIONS
          value: >
            -XX:ActiveProcessorCount=2
            -XX:+UseContainerSupport
            -XX:ParallelGCThreads=2
            -XX:ConcGCThreads=1
            
        # CPU调度优先级
        securityContext:
          # 设置CPU调度策略
          sysctls:
          - name: kernel.sched_min_granularity_ns
            value: "10000000"  # 10ms
          - name: kernel.sched_latency_ns
            value: "24000000"  # 24ms

---
# ========== CPU绑核配置 ==========
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: cpu-manager
  namespace: kube-system
spec:
  selector:
    matchLabels:
      name: cpu-manager
  template:
    metadata:
      labels:
        name: cpu-manager
    spec:
      # 启用静态CPU管理策略
      kubeletConfig:
        cpuManagerPolicy: static
        reservedSystemCPUs: "0,1"  # 为系统保留CPU核心
        
      containers:
      - name: cpu-manager
        image: k8s.gcr.io/cpu-manager:v1.0
        command:
        - /cpu-manager
        - --policy=static
        - --reserved-cpus=0,1
        volumeMounts:
        - name: sysfs
          mountPath: /sys
        securityContext:
          privileged: true
          
      volumes:
      - name: sysfs
        hostPath:
          path: /sys
```

### 2.2 内存优化配置

```yaml
# ========== 内存优化配置模板 ==========
apiVersion: v1
kind: Pod
metadata:
  name: memory-optimized-app
  namespace: production
spec:
  containers:
  - name: app
    image: app:v1.0
    resources:
      requests:
        # 基于稳态使用量的1.2倍
        memory: "512Mi"
      limits:
        # requests的1.5-2倍，允许合理突发
        memory: "1Gi"
        
    # 内存优化环境变量
    env:
    # Java应用内存优化
    - name: JAVA_OPTS
      value: >
        -Xmx768m
        -Xms512m
        -XX:+UseG1GC
        -XX:MaxGCPauseMillis=200
        -XX:+UnlockExperimentalVMOptions
        -XX:+UseCGroupMemoryLimitForHeap
        -XX:MaxRAMPercentage=75.0
        
    # Go应用内存优化
    - name: GOMEMLIMIT
      value: "800MiB"  # Go 1.19+ 内存软限制
    - name: GOGC
      value: "20"      # 垃圾回收触发比例
      
    # 内存安全设置
    securityContext:
      # 启用内存保护
      sysctls:
      - name: vm.overcommit_memory
        value: "1"  # 启用内存超额分配
      - name: vm.swappiness
        value: "1"  # 降低交换倾向

---
# ========== HugePages配置 ==========
apiVersion: v1
kind: Pod
metadata:
  name: hugepages-app
  namespace: production
spec:
  containers:
  - name: app
    image: database:v1.0
    resources:
      requests:
        memory: "2Gi"
        hugepages-2Mi: "1Gi"
      limits:
        memory: "2Gi"
        hugepages-2Mi: "1Gi"
        
    volumeMounts:
    - name: hugepage-2mi
      mountPath: /hugepages-2Mi
      
  volumes:
  - name: hugepage-2mi
    emptyDir:
      medium: HugePages-2Mi
```

### 2.3 资源配额优化

```yaml
# ========== 命名空间资源配额 ==========
apiVersion: v1
kind: ResourceQuota
metadata:
  name: production-quota
  namespace: production
spec:
  hard:
    # CPU配额
    requests.cpu: "20"
    limits.cpu: "40"
    # 内存配额
    requests.memory: "40Gi"
    limits.memory: "80Gi"
    # 存储配额
    requests.storage: "2Ti"
    persistentvolumeclaims: "100"
    # 对象数量限制
    pods: "1000"
    services: "50"
    secrets: "100"
    
  # 作用域选择器
  scopeSelector:
    matchExpressions:
    - scopeName: PriorityClass
      operator: In
      values: ["high-priority", "system-node-critical"]

---
# ========== LimitRange配置 ==========
apiVersion: v1
kind: LimitRange
metadata:
  name: production-limits
  namespace: production
spec:
  limits:
  # 容器默认限制
  - type: Container
    default:
      cpu: "500m"
      memory: "1Gi"
    defaultRequest:
      cpu: "100m"
      memory: "256Mi"
    max:
      cpu: "4"
      memory: "16Gi"
    min:
      cpu: "10m"
      memory: "32Mi"
      
  # Pod级别限制
  - type: Pod
    max:
      cpu: "8"
      memory: "32Gi"
```

---

## 3. 调度器调优参数

### 3.1 调度器性能调优

```yaml
# ========== 调度器高级配置 ==========
apiVersion: kubescheduler.config.k8s.io/v1beta3
kind: KubeSchedulerConfiguration
metadata:
  name: scheduler-config
profiles:
- schedulerName: default-scheduler
  plugins:
    # 预选阶段优化
    filter:
      disabled:
      - name: "NodeResourcesFit"  # 如果不需要严格的资源检查
      enabled:
      - name: "NodeResourcesBalancedAllocation"
        weight: 2
        
    # 优选阶段优化
    score:
      enabled:
      - name: "NodeResourcesLeastAllocated"
        weight: 1
      - name: "InterPodAffinity"
        weight: 2
      - name: "NodeAffinity"
        weight: 1
        
  pluginConfig:
  # 调度器性能参数
  - name: "NodeResourcesFit"
    args:
      scoringStrategy:
        type: LeastAllocated
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
          
  # 批量调度优化
  - name: "VolumeBinding"
    args:
      bindTimeoutSeconds: 30
      
# 调度器全局配置
extenders:
- urlPrefix: "http://scheduler-extender.example.com"
  filterVerb: "filter"
  prioritizeVerb: "prioritize"
  weight: 1
  enableHttps: false
  nodeCacheCapable: true

---
# ========== 调度器资源限制 ==========
apiVersion: v1
kind: Pod
metadata:
  name: kube-scheduler
  namespace: kube-system
spec:
  containers:
  - name: kube-scheduler
    image: k8s.gcr.io/kube-scheduler:v1.32.0
    resources:
      requests:
        cpu: "200m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "1Gi"
        
    # 调度器性能参数
    command:
    - kube-scheduler
    - --address=0.0.0.0
    - --leader-elect=true
    - --kubeconfig=/etc/kubernetes/scheduler.conf
    - --authentication-kubeconfig=/etc/kubernetes/scheduler.conf
    - --authorization-kubeconfig=/etc/kubernetes/scheduler.conf
    - --bind-address=0.0.0.0
    - --secure-port=10259
    - --profiling=false  # 生产环境禁用性能分析
    
    # 性能优化参数
    - --percentage-of-nodes-to-score=50  # 评分节点比例
    - --pod-max-in-unschedulable-pods-duration=60s  # 无法调度Pod的最大等待时间
    - --scheduler-name=default-scheduler
```

### 3.2 调度策略优化

```yaml
# ========== 自定义调度策略 ==========
apiVersion: kubescheduler.config.k8s.io/v1beta3
kind: KubeSchedulerConfiguration
profiles:
- schedulerName: high-performance-scheduler
  plugins:
    preFilter:
      enabled:
      - name: "NodeResourcesFit"
    filter:
      enabled:
      - name: "NodeUnschedulable"
      - name: "NodeAffinity"
      - name: "NodeResourcesFit"
      - name: "VolumeRestrictions"
      - name: "TaintToleration"
    postFilter:
      enabled:
      - name: "DefaultPreemption"
    preScore:
      enabled:
      - name: "InterPodAffinity"
    score:
      enabled:
      - name: "NodeResourcesBalancedAllocation"
        weight: 2
      - name: "ImageLocality"
        weight: 1
      - name: "InterPodAffinity"
        weight: 1
      - name: "NodeAffinity"
        weight: 1
      - name: "NodePreferAvoidPods"
        weight: 10000
      - name: "NodeResourcesLeastAllocated"
        weight: 1
      - name: "TaintToleration"
        weight: 1

---
# ========== 拓扑感知调度 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: topology-aware-app
  namespace: production
spec:
  replicas: 6
  template:
    spec:
      # 拓扑分布约束
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: topology-aware-app
            
      # 节点亲和性
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: topology.kubernetes.io/region
                operator: In
                values:
                - us-west-1
                
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: topology-aware-app
              topologyKey: kubernetes.io/hostname
```

---

## 4. 网络性能优化

### 4.1 CNI插件优化

```yaml
# ========== Calico网络优化配置 ==========
apiVersion: crd.projectcalico.org/v1
kind: FelixConfiguration
metadata:
  name: default
spec:
  # 性能优化参数
  bpfLogLevel: ""
  bpfEnabled: true  # 启用eBPF数据平面
  floatingIPs: Disabled
  healthPort: 9099
  logSeverityScreen: Info
  
  # 连接跟踪优化
  netlinkTimeoutSecs: 10
  reportingIntervalSecs: 0
  
  # 路由优化
  routeRefreshIntervalSecs: 90
  vxlanVNI: 4096

---
# ========== Cilium高性能配置 ==========
apiVersion: cilium.io/v2
kind: CiliumConfig
metadata:
  name: cilium-config
  namespace: kube-system
spec:
  # 启用高性能特性
  enable-bpf-clock-probe: true
  enable-bpf-tproxy: true
  enable-host-firewall: false  # 如不需要可关闭提升性能
  enable-ipv4-masquerade: true
  enable-ipv6-masquerade: false
  
  # 负载均衡优化
  kube-proxy-replacement: strict
  enable-health-check-nodeport: true
  node-port-bind-addr: "0.0.0.0"
  
  # 监控和调试
  monitor-aggregation: medium
  monitor-aggregation-flags: all
  monitor-aggregation-interval: 5s
```

### 4.2 Service性能优化

```yaml
# ========== Headless Service优化 ==========
apiVersion: v1
kind: Service
metadata:
  name: high-performance-service
  namespace: production
spec:
  clusterIP: None  # Headless Service减少DNS查询
  selector:
    app: backend
  ports:
  - name: http
    port: 8080
    targetPort: 8080
    protocol: TCP

---
# ========== ExternalTrafficPolicy优化 ==========
apiVersion: v1
kind: Service
metadata:
  name: external-service
  namespace: production
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保持客户端源IP，减少SNAT
  selector:
    app: frontend
  ports:
  - port: 80
    targetPort: 8080

---
# ========== Session Affinity配置 ==========
apiVersion: v1
kind: Service
metadata:
  name: session-affinity-service
  namespace: production
spec:
  selector:
    app: app-with-session
  ports:
  - port: 80
    targetPort: 8080
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时会话保持
```

### 4.3 网络策略优化

```yaml
# ========== 高性能网络策略 ==========
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: optimized-network-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: high-performance-app
  policyTypes:
  - Ingress
  - Egress
  
  # 优化的入口规则
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
      
  # 优化的出口规则
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 5432
    - protocol: TCP
      port: 3306
      
  # 允许必要的基础设施通信
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
    ports:
    - protocol: UDP
      port: 53  # DNS
    - protocol: TCP
      port: 53  # DNS
```

---

## 5. 存储IO调优

### 5.1 存储性能配置

```yaml
# ========== 高性能StorageClass配置 ==========
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp3
  fsType: ext4
  iops: "3000"      # IOPS性能
  throughput: "125" # 吞吐量(MB/s)
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer

---
# ========== 本地存储优化 ==========
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-fast
provisioner: kubernetes.io/no-provisioner
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: false

---
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-fast
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-fast
  local:
    path: /mnt/fast-disks/ssd1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - worker-node-1
```

### 5.2 应用层存储优化

```yaml
# ========== 存储优化的Pod配置 ==========
apiVersion: v1
kind: Pod
metadata:
  name: io-optimized-app
  namespace: production
spec:
  containers:
  - name: app
    image: database:v1.0
    volumeMounts:
    - name: data-volume
      mountPath: /var/lib/mysql
      # IO优化挂载选项
      mountPropagation: None
      
    # IO调度优化
    env:
    - name: MYSQLD_OPTS
      value: >
        --innodb-flush-method=O_DIRECT
        --innodb-io-capacity=2000
        --innodb-read-io-threads=8
        --innodb-write-io-threads=8
        
  volumes:
  - name: data-volume
    persistentVolumeClaim:
      claimName: mysql-pvc
      
---
# ========== 缓存优化配置 ==========
apiVersion: v1
kind: ConfigMap
metadata:
  name: cache-config
  namespace: production
data:
  redis.conf: |
    # 内存优化
    maxmemory 2gb
    maxmemory-policy allkeys-lru
    
    # 网络优化
    tcp-keepalive 300
    timeout 0
    
    # 持久化优化
    save 900 1
    save 300 10
    save 60 10000
    
    # 性能优化
    lazyfree-lazy-eviction yes
    lazyfree-lazy-expire yes
    lazyfree-lazy-server-del yes
```

### 5.3 存储监控和基准测试

```bash
#!/bin/bash
# ========== 存储性能基准测试 ==========
set -euo pipefail

TEST_NAMESPACE=${1:-"storage-test"}
STORAGE_CLASS=${2:-"fast-ssd"}

echo "开始存储性能测试..."

# 1. 创建测试环境
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Namespace
metadata:
  name: ${TEST_NAMESPACE}
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: perf-test-pvc
  namespace: ${TEST_NAMESPACE}
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: ${STORAGE_CLASS}
  resources:
    requests:
      storage: 10Gi
EOF

# 2. 部署FIO测试
kubectl run fio-test --image=ljishen/fio -n ${TEST_NAMESPACE} \
  --overrides='{
    "spec": {
      "containers": [{
        "name": "fio-test",
        "command": ["fio"],
        "args": [
          "--name=test",
          "--rw=randrw",
          "--bs=4k",
          "--iodepth=16",
          "--size=2g",
          "--direct=1",
          "--runtime=120",
          "--time_based",
          "--group_reporting",
          "--output-format=json"
        ],
        "volumeMounts": [{
          "name": "test-volume",
          "mountPath": "/data"
        }]
      }],
      "volumes": [{
        "name": "test-volume",
        "persistentVolumeClaim": {
          "claimName": "perf-test-pvc"
        }
      }]
    }
  }'

# 3. 等待测试完成并收集结果
kubectl wait --for=condition=Ready pod/fio-test -n ${TEST_NAMESPACE} --timeout=150s
kubectl logs pod/fio-test -n ${TEST_NAMESPACE} > /tmp/storage-benchmark-results.json

# 4. 解析测试结果
echo "=== 存储性能测试结果 ==="
jq '.jobs[].read' /tmp/storage-benchmark-results.json
jq '.jobs[].write' /tmp/storage-benchmark-results.json

# 5. 清理测试资源
kubectl delete namespace ${TEST_NAMESPACE}

echo "存储性能测试完成"
```

---

## 6. 应用层性能优化

### 6.1 JVM应用优化

```yaml
# ========== JVM性能优化配置 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: java-app-optimized
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: java-app:v1.0
        resources:
          requests:
            memory: "1Gi"
            cpu: "500m"
          limits:
            memory: "2Gi"
            cpu: "2000m"
            
        # JVM性能优化参数
        env:
        - name: JAVA_OPTS
          value: >
            -server
            -Xmx1536m
            -Xms1024m
            -XX:+UseG1GC
            -XX:MaxGCPauseMillis=200
            -XX:+UnlockExperimentalVMOptions
            -XX:+UseCGroupMemoryLimitForHeap
            -XX:MaxRAMPercentage=75.0
            -XX:+UseContainerSupport
            -XX:ActiveProcessorCount=2
            -XX:ParallelGCThreads=2
            -XX:ConcGCThreads=1
            -XX:+PrintGC
            -XX:+PrintGCDetails
            -XX:+PrintGCTimeStamps
            -Xloggc:/var/log/gc.log
            -XX:+UseGCLogFileRotation
            -XX:NumberOfGCLogFiles=5
            -XX:GCLogFileSize=100M
            
        # JVM启动优化
        startupProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - |
              curl -f http://localhost:8080/actuator/health || exit 1
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 30
```

### 6.2 Go应用优化

```yaml
# ========== Go应用性能优化 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: go-app-optimized
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: go-app:v1.0
        resources:
          requests:
            memory: "256Mi"
            cpu: "250m"
          limits:
            memory: "512Mi"
            cpu: "1000m"
            
        # Go运行时优化
        env:
        - name: GOMEMLIMIT
          value: "460MiB"  # 90% of limit
        - name: GOGC
          value: "20"      # 更频繁的GC
        - name: GOMAXPROCS
          value: "2"       # 限制CPU核心数
        - name: GOTRACEBACK
          value: "crash"   # 崩溃时打印堆栈
          
        # 性能监控
        ports:
        - name: pprof
          containerPort: 6060
          protocol: TCP
          
        # 启动优化
        startupProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 20
```

### 6.3 Python应用优化

```yaml
# ========== Python应用性能优化 ==========
apiVersion: apps/v1
kind: Deployment
metadata:
  name: python-app-optimized
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: python-app:v1.0
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "2000m"
            
        # Python性能优化
        env:
        - name: PYTHONUNBUFFERED
          value: "1"
        - name: PYTHONDONTWRITEBYTECODE
          value: "1"
        - name: PYTHONHASHSEED
          value: "random"
        - name: UVICORN_WORKERS
          value: "4"  # 根据CPU核心调整
        - name: UVICORN_THREADS
          value: "1"
          
        # 启动命令优化
        command:
        - uvicorn
        - main:app
        - --host
        - "0.0.0.0"
        - --port
        - "8080"
        - --workers
        - "4"
        - --http
        - "h11"
        - --loop
        - "uvloop"
        - --interface
        - "asgi3"
```

---

## 7. 监控与基准测试

### 7.1 性能监控仪表板

```yaml
# ========== Grafana性能监控面板 ==========
apiVersion: integreatly.org/v1alpha1
kind: GrafanaDashboard
metadata:
  name: k8s-performance-dashboard
  namespace: monitoring
spec:
  json: |
    {
      "dashboard": {
        "title": "Kubernetes Performance Dashboard",
        "panels": [
          {
            "title": "集群CPU使用率",
            "type": "graph",
            "targets": [
              {
                "expr": "sum(rate(container_cpu_usage_seconds_total[5m])) by (node)",
                "legendFormat": "{{node}}"
              }
            ]
          },
          {
            "title": "内存使用趋势",
            "type": "graph",
            "targets": [
              {
                "expr": "sum(container_memory_working_set_bytes) by (namespace)",
                "legendFormat": "{{namespace}}"
              }
            ]
          },
          {
            "title": "API Server延迟",
            "type": "graph",
            "targets": [
              {
                "expr": "histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))",
                "legendFormat": "99th percentile"
              }
            ]
          },
          {
            "title": "etcd性能",
            "type": "graph",
            "targets": [
              {
                "expr": "histogram_quantile(0.99, etcd_disk_backend_commit_duration_seconds_bucket)",
                "legendFormat": "fsync 99th"
              }
            ]
          }
        ]
      }
    }
```

### 7.2 自动化性能测试

```bash
#!/bin/bash
# ========== 自动化性能基准测试套件 ==========
set -euo pipefail

TEST_SUITE=${1:-"full"}
RESULTS_DIR="/tmp/performance-benchmarks-$(date +%Y%m%d-%H%M%S)"

mkdir -p ${RESULTS_DIR}
echo "开始性能基准测试: ${TEST_SUITE}"

# 基准测试配置
declare -A TEST_CONFIGS=(
    ["cpu"]="stress-ng --cpu 4 --timeout 60s"
    ["memory"]="stress-ng --vm 2 --vm-bytes 1G --timeout 60s"
    ["disk"]="fio --name=test --rw=randrw --bs=4k --iodepth=16 --size=1g --runtime=60"
    ["network"]="iperf3 -c benchmark-server -t 60"
)

# 执行基准测试
run_benchmark() {
    local test_type=$1
    local test_cmd=${TEST_CONFIGS[$test_type]}
    
    echo "执行${test_type}基准测试..."
    
    case $test_type in
        "cpu")
            kubectl run cpu-bench --image=alexeiled/stress-ng --restart=Never \
              -- ${test_cmd}
            ;;
        "memory")
            kubectl run mem-bench --image=alexeiled/stress-ng --restart=Never \
              -- ${test_cmd}
            ;;
        "disk")
            # 创建存储测试环境
            cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: disk-bench
spec:
  containers:
  - name: fio-test
    image: ljishen/fio
    command: ["sh", "-c"]
    args:
    - "${test_cmd} --output-format=json > /results/fio-results.json"
    volumeMounts:
    - name: results
      mountPath: /results
    - name: test-volume
      mountPath: /data
  volumes:
  - name: results
    emptyDir: {}
  - name: test-volume
    persistentVolumeClaim:
      claimName: bench-pvc
EOF
            ;;
    esac
    
    # 等待测试完成
    kubectl wait --for=condition=Ready pod/${test_type}-bench --timeout=90s 2>/dev/null || true
    
    # 收集结果
    if kubectl get pod/${test_type}-bench >/dev/null 2>&1; then
        kubectl logs pod/${test_type}-bench > ${RESULTS_DIR}/${test_type}-results.txt
        kubectl delete pod/${test_type}-bench
    fi
}

# 根据测试套件执行相应测试
case ${TEST_SUITE} in
    "quick")
        run_benchmark "cpu"
        run_benchmark "memory"
        ;;
    "full")
        run_benchmark "cpu"
        run_benchmark "memory"
        run_benchmark "disk"
        ;;
    "network")
        run_benchmark "network"
        ;;
esac

# 生成测试报告
cat > ${RESULTS_DIR}/benchmark-report.md <<EOF
# Kubernetes性能基准测试报告

## 测试信息
- 测试时间: $(date)
- 测试套件: ${TEST_SUITE}
- Kubernetes版本: $(kubectl version --short | grep Server | awk '{print $3}')

## 测试结果摘要

### CPU性能
$(cat ${RESULTS_DIR}/cpu-results.txt 2>/dev/null || echo "无数据")

### 内存性能
$(cat ${RESULTS_DIR}/memory-results.txt 2>/dev/null || echo "无数据")

### 磁盘IO性能
$(cat ${RESULTS_DIR}/disk-results.txt 2>/dev/null || echo "无数据")

### 网络性能
$(cat ${RESULTS_DIR}/network-results.txt 2>/dev/null || echo "无数据")

## 建议优化措施
- 根据测试结果调整资源配置
- 优化应用性能参数
- 考虑硬件升级需求
EOF

echo "基准测试完成，结果保存在: ${RESULTS_DIR}"
ls -la ${RESULTS_DIR}
```

### 7.3 持续性能监控

```yaml
# ========== 持续性能监控配置 ==========
apiVersion: batch/v1
kind: CronJob
metadata:
  name: performance-monitoring
  namespace: monitoring
spec:
  schedule: "*/30 * * * *"  # 每30分钟执行一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: perf-collector
            image: perf-tools:latest
            command:
            - /scripts/collect-performance-metrics.sh
            env:
            - name: SLACK_WEBHOOK_URL
              valueFrom:
                secretKeyRef:
                  name: monitoring-secrets
                  key: slack-webhook-url
            volumeMounts:
            - name: scripts
              mountPath: /scripts
          volumes:
          - name: scripts
            configMap:
              name: perf-scripts
          restartPolicy: OnFailure

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: perf-scripts
  namespace: monitoring
data:
  collect-performance-metrics.sh: |
    #!/bin/bash
    set -euo pipefail
    
    # 收集性能指标
    COLLECT_TIME=$(date -Iseconds)
    
    # CPU使用率
    CPU_USAGE=$(kubectl top nodes | awk 'NR>1 {sum+=$3} END {print sum/NR}')
    
    # 内存使用率
    MEM_USAGE=$(kubectl top nodes | awk 'NR>1 {sum+=$5} END {print sum/NR}')
    
    # API Server延迟
    API_LATENCY=$(kubectl get --raw /metrics | grep apiserver_request_duration_seconds | \
      awk '/quantile="0.99"/ {print $2}' | head -1)
    
    # 生成报告
    cat <<REPORT
    {
      "timestamp": "${COLLECT_TIME}",
      "cpu_usage_percent": ${CPU_USAGE},
      "memory_usage_percent": ${MEM_USAGE},
      "api_server_latency_99th_ms": ${API_LATENCY},
      "cluster_health": "$(if (( $(echo "${CPU_USAGE} < 80" | bc -l) )) && (( $(echo "${MEM_USAGE} < 85" | bc -l) )); then echo "healthy"; else echo "warning"; fi)"
    }
    REPORT
```

---
## 8. 性能优化实战案例库

### 8.1 大规模集群性能优化案例

#### 案例1: API Server性能瓶颈突破
```markdown
**优化背景**: 
万级节点集群API Server QPS达到2000+时出现明显延迟，影响集群管理效率

**问题诊断**:
```bash
# 监控API Server性能指标
kubectl get --raw /metrics | grep apiserver_request_duration_seconds | \
  awk '/quantile="0.99"/ {print $2}'
# 发现99th percentile延迟达到2.5秒

# 分析请求类型分布
kubectl get --raw /metrics | grep apiserver_request_total | \
  awk '{print $1}' | cut -d'_' -f4- | sort | uniq -c | sort -nr
# 发现list/watch请求占比较高

# 检查etcd性能
kubectl exec -n kube-system etcd-master1 -- etcdctl check perf
# 发现etcd写入延迟较高
```

**优化方案**:

1. **API Server调优**:
```yaml
# kube-apiserver优化配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - name: kube-apiserver
    command:
    - kube-apiserver
    - --max-requests-inflight=1200
    - --max-mutating-requests-inflight=600
    - --request-timeout=2m
    - --min-request-timeout=1800
    - --enable-aggregator-routing=true
    - --storage-backend=etcd3
    - --etcd-servers=https://etcd1:2379,https://etcd2:2379,https://etcd3:2379
    - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
    - --etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
    - --etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
    - --profiling=false  # 生产环境关闭性能分析
```

2. **etcd优化**:
```bash
# etcd性能调优参数
ETCD_HEARTBEAT_INTERVAL=100
ETCD_ELECTION_TIMEOUT=1000
ETCD_QUOTA_BACKEND_BYTES=8589934592  # 8GB
ETCD_AUTO_COMPACTION_RETENTION=1
ETCD_AUTO_COMPACTION_MODE=periodic
ETCD_SNAPSHOT_COUNT=10000
```

3. **客户端优化**:
```go
// Go客户端连接池优化
config := &rest.Config{
    Host: "https://kubernetes:6443",
    QPS:   100,     // 每秒查询速率
    Burst: 200,     // 突发查询数
}
clientset, err := kubernetes.NewForConfig(config)
```

**优化效果**:
- API Server 99th延迟从2.5秒降至0.8秒
- QPS承载能力提升至5000+
- etcd写入延迟降低60%
```

#### 案例2: 节点资源利用率优化
```markdown
**优化背景**: 
集群节点CPU平均使用率仅35%，内存使用率45%，资源浪费严重

**问题分析**:
```bash
# 分析节点资源分布
kubectl top nodes | awk 'NR>1 {print $1,$3,$5}' | \
  awk '{cpu[$1]=$2; mem[$1]=$3} END {
    for(node in cpu) {
      print node, cpu[node], mem[node]
    }
  }' | sort -k2 -n

# 检查Pod资源请求设置
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].resources.requests.cpu}{"\t"}{.spec.containers[*].resources.limits.cpu}{"\n"}{end}' | \
  head -20

# 分析资源碎片化程度
kubectl describe nodes | grep -E "(Allocated|Requests)" | \
  awk '{print $2,$4}' | sort | uniq -c
```

**优化策略**:

1. **垂直Pod自动扩缩容(VPA)**:
```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: app-vpa-optimizer
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: high-traffic-app
  updatePolicy:
    updateMode: "Initial"  # 首次优化后手动调整
  resourcePolicy:
    containerPolicies:
    - containerName: app
      minAllowed:
        cpu: 100m
        memory: 128Mi
      maxAllowed:
        cpu: 2000m
        memory: 4Gi
```

2. **水平Pod自动扩缩容优化**:
```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: smart-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: traffic-sensitive-app
  minReplicas: 3
  maxReplicas: 50
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60  # 降低触发阈值
  - type: External
    external:
      metric:
        name: custom.business.metric
      target:
        type: Value
        value: "1000"
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
      - type: Pods
        value: 4
        periodSeconds: 60
```

3. **节点资源优化脚本**:
```bash
#!/bin/bash
# 节点资源优化自动化脚本

optimize_node_resources() {
    local node_name=$1
    
    # 获取节点详细信息
    node_info=$(kubectl describe node ${node_name})
    
    # 计算资源使用率
    allocatable_cpu=$(echo "${node_info}" | grep "cpu " | awk '{print $2}')
    allocatable_memory=$(echo "${node_info}" | grep "memory " | awk '{print $2}')
    
    # 分析Pod资源请求
    pod_requests=$(kubectl get pods -o jsonpath='{range .items[?(@.spec.nodeName=="'${node_name}'")]}{.spec.containers[*].resources.requests.cpu}{"\n"}{end}' | \
                   awk '{sum+=$1} END {print sum}')
    
    # 计算优化建议
    cpu_utilization=$(echo "scale=2; ${pod_requests}/${allocatable_cpu}*100" | bc)
    
    if (( $(echo "${cpu_utilization} < 40" | bc -l) )); then
        echo "节点${node_name} CPU利用率偏低(${cpu_utilization}%)，建议优化"
        # 实施优化措施
        optimize_workload_distribution ${node_name}
    fi
}

optimize_workload_distribution() {
    local node_name=$1
    
    # 重新平衡Pod分布
    kubectl patch deployment app-deployment -p '{
        "spec": {
            "template": {
                "spec": {
                    "affinity": {
                        "podAntiAffinity": {
                            "preferredDuringSchedulingIgnoredDuringExecution": [
                                {
                                    "weight": 100,
                                    "podAffinityTerm": {
                                        "labelSelector": {
                                            "matchExpressions": [
                                                {
                                                    "key": "app",
                                                    "operator": "In",
                                                    "values": ["high-cpu-app"]
                                                }
                                            ]
                                        },
                                        "topologyKey": "kubernetes.io/hostname"
                                    }
                                }
                            ]
                        }
                    }
                }
            }
        }
    }'
}
```

**优化成果**:
- 节点CPU平均利用率提升至65%
- 内存利用率提升至70%
- 集群整体资源成本降低30%
- 应用性能稳定性显著改善

### 8.2 性能监控最佳实践

#### 核心性能指标监控体系

| 监控维度 | 关键指标 | 告警阈值 | 检测频率 | 响应策略 |
|---------|---------|---------|---------|---------|
| **API Server** | QPS、99th延迟、错误率 | QPS>3000、延迟>1s、错误率>1% | 30秒 | 自动扩容、限流降级 |
| **etcd** | WAL延迟、fsync时间、存储使用率 | WAL>100ms、fsync>50ms、使用率>80% | 15秒 | 存储扩容、性能调优 |
| **节点资源** | CPU使用率、内存使用率、磁盘IO | CPU>85%、内存>90%、IO等待>30% | 60秒 | 资源调度、节点扩容 |
| **网络性能** | 带宽利用率、丢包率、延迟 | 利用率>70%、丢包>0.1%、延迟>100ms | 30秒 | 网络优化、负载均衡 |
| **应用性能** | 响应时间、吞吐量、错误率 | RT>500ms、QPS下降30%、错误率>0.5% | 实时 | 自动扩缩容、故障转移 |

#### 性能基准测试框架

```yaml
# 性能基准测试配置
apiVersion: batch/v1
kind: Job
metadata:
  name: performance-benchmark
spec:
  template:
    spec:
      containers:
      - name: benchmark
        image: k8s.gcr.io/benchmark-runner:latest
        command:
        - /benchmark
        - --duration=300s
        - --concurrency=100
        - --target=qps-test
        - --metrics-output=prometheus
        env:
        - name: TARGET_SERVICE
          value: "http://test-app.production.svc.cluster.local"
        - name: PROMETHEUS_ENDPOINT
          value: "http://prometheus.monitoring.svc:9090"
      restartPolicy: Never
  backoffLimit: 3
```

#### 性能优化检查清单

- [ ] 定期审查资源请求和限制设置
- [ ] 监控并优化Pod分布策略
- [ ] 实施合适的自动扩缩容策略
- [ ] 优化存储IO性能配置
- [ ] 调整网络插件参数
- [ ] 定期清理无用资源
- [ ] 优化镜像拉取策略
- [ ] 实施有效的缓存策略

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-02 | 版本: v1.25-v1.32 | 质量等级: ⭐⭐⭐⭐⭐ 专家级