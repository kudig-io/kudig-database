# 26 - DNS 故障排查 (DNS Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes DNS](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

---

## 1. DNS 故障诊断总览 (DNS Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **内部域名解析失败** | Service/Pod域名无法解析 | 服务间调用失败 | P0 - 紧急 |
| **外部域名解析失败** | 无法访问互联网域名 | 依赖服务中断 | P1 - 高 |
| **DNS响应超时** | 查询响应缓慢 | 应用性能下降 | P1 - 高 |
| **DNS缓存问题** | 解析结果陈旧 | 配置变更延迟 | P2 - 中 |
| **CoreDNS异常** | DNS服务不可用 | 全局解析失效 | P0 - 紧急 |

### 1.2 DNS架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        DNS 故障诊断架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       应用层DNS查询                                  │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │ (Client)    │    │ (Client)    │    │ (Client)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CoreDNS服务层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                     CoreDNS Pods                              │  │  │
│  │  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐       │  │  │
│  │  │  │  CoreDNS-1  │    │  CoreDNS-2  │    │  CoreDNS-3  │       │  │  │
│  │  │  │   (Pod)     │    │   (Pod)     │    │   (Pod)     │       │  │  │
│  │  │  └─────────────┘    └─────────────┘    └─────────────┘       │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   集群DNS    │   │   外部DNS    │   │   反向解析   │                   │
│  │ (Cluster)   │   │ (External)  │   │ (Reverse)   │                   │
│  │   解析      │   │   解析      │   │   解析      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      DNS数据源层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Service   │    │   Endpoint  │   │   External  │              │  │
│  │  │  发现       │    │   发现      │   │   DNS服务器  │              │  │
│  │  │ (API)      │    │ (API)      │   │  (Upstream) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      缓存和转发层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   内存缓存   │   │   负载均衡   │   │   超时重试   │              │  │
│  │  │ (Cache)     │   │ (LB)        │   │ (Retry)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. CoreDNS 基础状态检查 (CoreDNS Basic Status Check)

### 2.1 CoreDNS 服务状态验证

```bash
# ========== 1. CoreDNS部署检查 ==========
# 检查CoreDNS Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看CoreDNS详细信息
kubectl describe pods -n kube-system -l k8s-app=kube-dns

# 检查CoreDNS Service
kubectl get service -n kube-system kube-dns

# 验证Service endpoints
kubectl get endpoints -n kube-system kube-dns

# ========== 2. CoreDNS配置检查 ==========
# 查看CoreDNS ConfigMap
kubectl get configmap -n kube-system coredns -o yaml

# 检查CoreDNS配置文件
kubectl get configmap -n kube-system coredns -o jsonpath='{.data.Corefile}'

# 验证DNS服务IP
kubectl get service -n kube-system kube-dns -o jsonpath='{.spec.clusterIP}'

# ========== 3. DNS服务发现检查 ==========
# 检查kubelet DNS配置
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.nodeInfo.osImage
}{
        "\n"
}{
    end
}'

# 验证节点DNS配置
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "cat /etc/resolv.conf"
done
```

### 2.2 DNS解析基础测试

```bash
# ========== 基础解析测试 ==========
# 创建测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: dns-test
  namespace: default
spec:
  containers:
  - name: test
    image: busybox
    command: ["sleep", "3600"]
EOF

# 等待Pod就绪
sleep 30

# 测试集群内部域名解析
kubectl exec dns-test -- nslookup kubernetes.default.svc.cluster.local

# 测试Service DNS解析
kubectl exec dns-test -- nslookup kube-dns.kube-system.svc.cluster.local

# 测试外部域名解析
kubectl exec dns-test -- nslookup google.com

# ========== DNS配置验证 ==========
# 检查Pod的DNS配置
kubectl exec dns-test -- cat /etc/resolv.conf

# 验证搜索域配置
kubectl exec dns-test -- cat /etc/resolv.conf | grep search

# 检查DNS服务器配置
kubectl exec dns-test -- cat /etc/resolv.conf | grep nameserver
```

---

## 3. 内部DNS解析问题排查 (Internal DNS Resolution Issues)

### 3.1 Service DNS解析问题

```bash
# ========== 1. Service发现检查 ==========
# 查看所有Service
kubectl get services --all-namespaces

# 检查特定Service的DNS记录
SERVICE_NAME="kubernetes"
SERVICE_NAMESPACE="default"

kubectl exec dns-test -- nslookup $SERVICE_NAME.$SERVICE_NAMESPACE.svc.cluster.local

# 验证Service endpoints
kubectl get endpoints $SERVICE_NAME -n $SERVICE_NAMESPACE

# 检查Service选择器
kubectl get service $SERVICE_NAME -n $SERVICE_NAMESPACE -o jsonpath='{.spec.selector}'

# ========== 2. Headless Service测试 ==========
# 创建Headless Service测试
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: test-headless
  namespace: default
spec:
  clusterIP: None
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: test-app
---
apiVersion: v1
kind: Pod
metadata:
  name: test-app-pod
  namespace: default
  labels:
    app: test-app
spec:
  containers:
  - name: nginx
    image: nginx
EOF

# 测试Headless Service DNS
kubectl exec dns-test -- nslookup test-headless.default.svc.cluster.local

# 测试Pod DNS记录
kubectl exec dns-test -- nslookup test-app-pod.test-headless.default.svc.cluster.local

# ========== 3. DNS解析延迟分析 ==========
# 测量DNS解析时间
kubectl exec dns-test -- sh -c "
for i in \$(seq 1 10); do
    start=\$(date +%s.%N)
    nslookup kubernetes.default.svc.cluster.local >/dev/null 2>&1
    end=\$(date +%s.%N)
    echo \"Query \$i: \$(echo \"\$end - \$start\" | bc) seconds\"
done
"
```

### 3.2 Pod DNS解析问题

```bash
# ========== Pod DNS记录检查 ==========
# 查看Pod IP和DNS记录
kubectl get pods -n default -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.podIP
}{
        "\n"
}{
    end
}'

# 测试Pod DNS解析
POD_NAME=$(kubectl get pods -n default -o jsonpath='{.items[0].metadata.name}')
kubectl exec dns-test -- nslookup $POD_NAME.default.pod.cluster.local

# ========== DNS策略检查 ==========
# 检查Pod DNS策略
kubectl get pod dns-test -o jsonpath='{.spec.dnsPolicy}'

# 验证自定义DNS配置
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: dns-custom-test
  namespace: default
spec:
  dnsPolicy: "None"
  dnsConfig:
    nameservers:
      - 8.8.8.8
      - 8.8.4.4
    searches:
      - default.svc.cluster.local
      - svc.cluster.local
      - cluster.local
    options:
      - name: ndots
        value: "5"
  containers:
  - name: test
    image: busybox
    command: ["sleep", "3600"]
EOF

# 测试自定义DNS配置
kubectl exec dns-custom-test -- nslookup kubernetes.default.svc.cluster.local
```

---

## 4. 外部DNS解析问题排查 (External DNS Resolution Issues)

### 4.1 上游DNS配置检查

```bash
# ========== 1. CoreDNS上游配置检查 ==========
# 查看CoreDNS转发配置
kubectl get configmap -n kube-system coredns -o jsonpath='{.data.Corefile}' | grep forward

# 检查上游DNS服务器可达性
kubectl exec -n kube-system -l k8s-app=kube-dns -- nslookup google.com 8.8.8.8

# 验证多个上游DNS服务器
UPSTREAM_DNS_SERVERS=("8.8.8.8" "1.1.1.1" "208.67.222.222")
for dns in "${UPSTREAM_DNS_SERVERS[@]}"; do
    echo "Testing upstream DNS: $dns"
    kubectl exec -n kube-system -l k8s-app=kube-dns -- nslookup google.com $dns
done

# ========== 2. 网络策略影响检查 ==========
# 检查网络策略是否阻止外部DNS访问
kubectl get networkpolicy --all-namespaces

# 测试DNS端口连通性
kubectl exec dns-test -- nc -zv 8.8.8.8 53
kubectl exec dns-test -- nc -zv 1.1.1.1 53

# ========== 3. 防火墙和NAT检查 ==========
# 检查节点网络配置
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=nicolaka/netshoot -it -- sh -c "
        echo 'Checking DNS connectivity:'
        nc -zv 8.8.8.8 53
        echo 'Checking default route:'
        ip route show default
    "
done
```

### 4.2 DNS转发和缓存问题

```bash
# ========== 缓存问题诊断 ==========
# 测试DNS缓存效果
kubectl exec dns-test -- sh -c "
nslookup google.com
sleep 1
time nslookup google.com  # 第二次应该更快
"

# 检查CoreDNS缓存配置
kubectl get configmap -n kube-system coredns -o jsonpath='{.data.Corefile}' | grep -A5 cache

# 清除DNS缓存测试
kubectl delete pod -n kube-system -l k8s-app=kube-dns
sleep 30
kubectl exec dns-test -- time nslookup google.com

# ========== 转发配置优化 ==========
# 更新CoreDNS配置以优化转发
cat <<EOF > coredns-optimized-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
          lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
          ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
          max_concurrent 1000
          expire 30s
        }
        cache 30 {
          denial 5
          prefetch 1 10m 10%
        }
        loop
        reload
        loadbalance
    }
EOF

# 应用优化配置
kubectl apply -f coredns-optimized-config.yaml

# 重启CoreDNS以应用更改
kubectl rollout restart deployment coredns -n kube-system
```

---

## 5. CoreDNS性能和稳定性问题 (CoreDNS Performance and Stability)

### 5.1 性能监控和指标

```bash
# ========== CoreDNS指标检查 ==========
# 检查CoreDNS指标端点
kubectl port-forward -n kube-system svc/kube-dns 9153:9153 &
PORT_FORWARD_PID=$!

# 等待端口转发建立
sleep 5

# 获取CoreDNS指标
curl -s http://localhost:9153/metrics | grep -E "(coredns_dns_requests_total|coredns_dns_responses_total|coredns_dns_request_duration_seconds)"

# 检查错误率指标
curl -s http://localhost:9153/metrics | grep coredns_dns_responses_total | grep "rcode=\"SERVFAIL\""

# 停止端口转发
kill $PORT_FORWARD_PID

# ========== 资源使用监控 ==========
# 监控CoreDNS资源使用
kubectl top pods -n kube-system -l k8s-app=kube-dns

# 检查CoreDNS日志中的性能警告
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100 | grep -i "slow|timeout|error"

# 分析CoreDNS副本数量是否足够
COREDNS_REPLICAS=$(kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.replicas}')
NODE_COUNT=$(kubectl get nodes --no-headers | wc -l)
echo "CoreDNS replicas: $COREDNS_REPLICAS, Node count: $NODE_COUNT"
```

### 5.2 高可用性配置

```bash
# ========== 副本数量优化 ==========
# 根据节点数量调整CoreDNS副本数
RECOMMENDED_REPLICAS=$((NODE_COUNT > 3 ? NODE_COUNT/2 : NODE_COUNT))
echo "Recommended CoreDNS replicas: $RECOMMENDED_REPLICAS"

# 调整CoreDNS副本数量
kubectl scale deployment coredns -n kube-system --replicas=$RECOMMENDED_REPLICAS

# ========== 反亲和性配置 ==========
# 为CoreDNS添加反亲和性配置
cat <<EOF > coredns-anti-affinity.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
spec:
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: k8s-app
                  operator: In
                  values: ["kube-dns"]
              topologyKey: kubernetes.io/hostname
EOF

# 应用反亲和性配置
kubectl patch deployment coredns -n kube-system --patch "$(cat coredns-anti-affinity.yaml)"

# ========== 资源限制配置 ==========
# 为CoreDNS设置合理的资源限制
cat <<EOF > coredns-resources.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: coredns
        resources:
          requests:
            cpu: "100m"
            memory: "70Mi"
          limits:
            cpu: "1000m"
            memory: "170Mi"
EOF

kubectl patch deployment coredns -n kube-system --patch "$(cat coredns-resources.yaml)"
```

---

## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 DNS监控指标配置

```bash
# ========== ServiceMonitor配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: coredns-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-dns
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
EOF

# ========== Prometheus告警规则 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dns-alerts
  namespace: monitoring
spec:
  groups:
  - name: dns.rules
    rules:
    - alert: CoreDNSDown
      expr: up{job="coredns"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "CoreDNS is down"
        
    - alert: HighDNSErrorRate
      expr: rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m]) > 0.01
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "High DNS error rate ({{ \$value }} errors/sec)"
        
    - alert: SlowDNSQueries
      expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Slow DNS queries (99th percentile > 2s)"
        
    - alert: CoreDNSHighMemoryUsage
      expr: container_memory_usage_bytes{container="coredns"} > 150 * 1024 * 1024
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS high memory usage ({{ \$value }} bytes)"
        
    - alert: DNSCacheHitRateLow
      expr: rate(coredns_cache_hits_total[5m]) / (rate(coredns_cache_hits_total[5m]) + rate(coredns_cache_misses_total[5m])) < 0.8
      for: 10m
      labels:
        severity: info
      annotations:
        summary: "DNS cache hit rate low (< 80%)"
EOF
```

### 6.2 DNS健康检查工具

```bash
# ========== DNS健康检查脚本 ==========
cat <<'EOF' > dns-health-check.sh
#!/bin/bash

TEST_NAMESPACE=${1:-default}
INTERVAL=${2:-60}

echo "Starting DNS health check for namespace: $TEST_NAMESPACE"
echo "Check interval: $INTERVAL seconds"

# 创建测试Pod
cat <<TESTPOD | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: dns-health-check-pod
  namespace: $TEST_NAMESPACE
spec:
  containers:
  - name: tester
    image: busybox
    command: ["sleep", "3600"]
TESTPOD

# 等待Pod就绪
sleep 30

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 测试内部DNS解析
    INTERNAL_RESULT=$(kubectl exec -n $TEST_NAMESPACE dns-health-check-pod -- nslookup kubernetes.default.svc.cluster.local 2>&1)
    if echo "$INTERNAL_RESULT" | grep -q "Address"; then
        echo "$TIMESTAMP - Internal DNS: ✓"
    else
        echo "$TIMESTAMP - Internal DNS: ✗"
        echo "  Error: $INTERNAL_RESULT"
    fi
    
    # 测试外部DNS解析
    EXTERNAL_RESULT=$(kubectl exec -n $TEST_NAMESPACE dns-health-check-pod -- nslookup google.com 2>&1)
    if echo "$EXTERNAL_RESULT" | grep -q "Address"; then
        echo "$TIMESTAMP - External DNS: ✓"
    else
        echo "$TIMESTAMP - External DNS: ✗"
        echo "  Error: $EXTERNAL_RESULT"
    fi
    
    # 测试DNS延迟
    START_TIME=$(date +%s.%N)
    kubectl exec -n $TEST_NAMESPACE dns-health-check-pod -- nslookup kubernetes.default.svc.cluster.local >/dev/null 2>&1
    END_TIME=$(date +%s.%N)
    DELAY=$(echo "$END_TIME - $START_TIME" | bc)
    
    if (( $(echo "$DELAY > 1" | bc -l) )); then
        echo "$TIMESTAMP - DNS Delay: ${DELAY}s (WARNING)"
    else
        echo "$TIMESTAMP - DNS Delay: ${DELAY}s (OK)"
    fi
    
    echo "---"
    sleep $INTERVAL
done
EOF

chmod +x dns-health-check.sh

# ========== DNS性能基准测试 ==========
cat <<'EOF' > dns-benchmark.sh
#!/bin/bash

TEST_NAMESPACE=${1:-default}
ITERATIONS=${2:-100}

echo "Running DNS benchmark with $ITERATIONS iterations"

# 测试多种DNS查询类型
QUERY_TYPES=(
    "kubernetes.default.svc.cluster.local"
    "google.com"
    "github.com"
    "kubernetes.io"
)

for query in "${QUERY_TYPES[@]}"; do
    echo "Benchmarking: $query"
    
    TOTAL_TIME=0
    SUCCESS_COUNT=0
    
    for i in $(seq 1 $ITERATIONS); do
        START_TIME=$(date +%s.%N)
        RESULT=$(kubectl exec -n $TEST_NAMESPACE dns-health-check-pod -- nslookup $query 2>&1)
        END_TIME=$(date +%s.%N)
        
        if echo "$RESULT" | grep -q "Address"; then
            DELAY=$(echo "$END_TIME - $START_TIME" | bc)
            TOTAL_TIME=$(echo "$TOTAL_TIME + $DELAY" | bc)
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        fi
    done
    
    if [ $SUCCESS_COUNT -gt 0 ]; then
        AVG_TIME=$(echo "scale=4; $TOTAL_TIME / $SUCCESS_COUNT" | bc)
        SUCCESS_RATE=$(echo "scale=2; $SUCCESS_COUNT * 100 / $ITERATIONS" | bc)
        echo "  Average response time: ${AVG_TIME}s"
        echo "  Success rate: ${SUCCESS_RATE}%"
        echo "  Successful queries: $SUCCESS_COUNT/$ITERATIONS"
    else
        echo "  All queries failed"
    fi
    echo ""
done
EOF

chmod +x dns-benchmark.sh

# ========== DNS故障模拟和恢复测试 ==========
cat <<'EOF' > dns-failure-test.sh
#!/bin/bash

TEST_NAMESPACE=${1:-default}

echo "Testing DNS failure scenarios"

# 1. 模拟CoreDNS Pod故障
echo "=== Testing CoreDNS Pod Restart ==="
COREDNS_POD=$(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}')
echo "Restarting CoreDNS pod: $COREDNS_POD"

kubectl delete pod -n kube-system $COREDNS_POD
sleep 10

# 等待CoreDNS恢复
echo "Waiting for CoreDNS to recover..."
while ! kubectl exec -n $TEST_NAMESPACE dns-health-check-pod -- nslookup kubernetes.default.svc.cluster.local >/dev/null 2>&1; do
    echo "CoreDNS still recovering..."
    sleep 5
done
echo "CoreDNS recovered ✓"

# 2. 测试网络分区场景
echo "=== Testing Network Partition Scenario ==="
# 这里可以添加网络策略来模拟网络分区

# 3. 测试上游DNS故障
echo "=== Testing Upstream DNS Failure ==="
# 临时修改CoreDNS配置指向不可达的DNS服务器
# 然后测试解析行为

echo "DNS failure testing completed"
EOF

chmod +x dns-failure-test.sh
```

---

## 7. 故障恢复和预防措施 (Incident Recovery and Prevention)

### 7.1 应急恢复流程

```bash
# ========== 紧急恢复步骤 ==========
# 1. 立即诊断DNS问题
echo "=== Emergency DNS Diagnosis ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 2. 临时恢复外部DNS访问
cat <<EOF > emergency-dns-fix.yaml
apiVersion: v1
kind: Pod
metadata:
  name: emergency-dns-test
  namespace: default
spec:
  dnsPolicy: "None"
  dnsConfig:
    nameservers:
      - 8.8.8.8
      - 1.1.1.1
    searches:
      - default.svc.cluster.local
      - svc.cluster.local
      - cluster.local
  containers:
  - name: test
    image: busybox
    command: ["sleep", "3600"]
EOF

kubectl apply -f emergency-dns-fix.yaml

# 3. 重启CoreDNS
echo "Restarting CoreDNS..."
kubectl rollout restart deployment coredns -n kube-system

# 4. 验证恢复
sleep 30
kubectl exec emergency-dns-test -- nslookup kubernetes.default.svc.cluster.local
kubectl exec emergency-dns-test -- nslookup google.com
```

### 7.2 预防性配置

```bash
# ========== DNS冗余配置 ==========
# 配置多个上游DNS服务器
cat <<EOF > redundant-dns-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health
        kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
        }
        prometheus :9153
        forward . 8.8.8.8 1.1.1.1 208.67.222.222 {
          health_check 5s
          max_fails 3
          expire 30s
          policy sequential
        }
        cache 30
        loop
        reload
        loadbalance
    }
EOF

# ========== 健康检查增强 ==========
# 添加更完善的健康检查
cat <<EOF > enhanced-health-check.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
          lameduck 10s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
          ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
          health_check 5s
          max_fails 3
          expire 30s
          policy sequential
        }
        cache 30 {
          success 1000
          denial 100
          prefetch 2 10m 20%
        }
        loop
        reload
        loadbalance
        hosts {
          127.0.0.1 localhost
          fallthrough
        }
    }
EOF

# ========== 监控告警完善 ==========
# 添加更详细的监控和告警配置
cat <<'EOF' > comprehensive-dns-monitoring.yaml
# PrometheusRule for comprehensive DNS monitoring
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: comprehensive-dns-alerts
  namespace: monitoring
spec:
  groups:
  - name: dns-comprehensive.rules
    rules:
    # 基础可用性监控
    - alert: CoreDNSPodDown
      expr: kube_pod_status_ready{condition="true",pod=~"coredns-.+"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "CoreDNS pod is not ready"
        
    # 性能监控
    - alert: DNSHighLatency
      expr: histogram_quantile(0.95, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "DNS query latency high (95th percentile > 1s)"
        
    # 错误率监控
    - alert: DNSNXDOMAINRateHigh
      expr: rate(coredns_dns_responses_total{rcode="NXDOMAIN"}[5m]) > 10
      for: 2m
      labels:
        severity: info
      annotations:
        summary: "High rate of NXDOMAIN responses ({{ \$value }}/sec)"
        
    # 缓存效率监控
    - alert: DNSCacheEfficiencyLow
      expr: rate(coredns_cache_hits_total[10m]) / (rate(coredns_cache_hits_total[10m]) + rate(coredns_cache_misses_total[10m])) < 0.7
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "DNS cache efficiency low (< 70%)"
        
    # 资源使用监控
    - alert: CoreDNSMemoryUsageHigh
      expr: container_memory_working_set_bytes{container="coredns"} > 100 * 1024 * 1024
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS memory usage high ({{ \$value }} bytes)"
EOF
```

---