# 25 - 网络连通性故障排查 (Network Connectivity Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Networking](https://kubernetes.io/docs/concepts/cluster-administration/networking/)

---

## 1. 网络连通性故障诊断总览 (Network Connectivity Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod间通信失败** | 同集群Pod无法互访 | 微服务调用失败 | P0 - 紧急 |
| **Service访问异常** | ClusterIP服务不可达 | 服务发现失效 | P0 - 紧急 |
| **外部网络阻断** | 无法访问互联网 | 依赖服务中断 | P1 - 高 |
| **DNS解析失败** | 域名无法解析 | 服务调用失败 | P0 - 紧急 |
| **网络策略阻断** | 合法流量被拦截 | 业务功能异常 | P1 - 高 |

### 1.2 网络连通性架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   网络连通性故障诊断架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      应用层通信                                        │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │  (Client)   │    │  (Server)   │    │  (Database) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Service   │    │   DNS解析    │   │   网络策略   │                   │
│  │  (ClusterIP)│    │ (CoreDNS)   │   │ (Network    │                   │
│  │   负载均衡   │    │   解析      │   │ Policy)     │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      CNI网络层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Calico    │    │   Cilium    │    │   Flannel   │              │  │
│  │  │  (网络插件)  │    │  (eBPF)     │    │  (VXLAN)    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   iptables  │    │    IPVS     │    │    路由     │                   │
│  │   规则链     │    │   负载均衡   │    │   表       │                   │
│  │  (NAT/DNAT) │    │   转发      │    │  (Kernel)   │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      节点网络接口                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   eth0      │    │   tunl0     │    │   vethXXX   │              │  │
│  │  │ (物理网卡)   │    │ (隧道接口)   │    │ (虚拟网卡)   │              │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 基础网络连通性检查 (Basic Network Connectivity Check)

### 2.1 Pod网络状态验证

```bash
# ========== 1. Pod网络基本信息检查 ==========
# 检查Pod IP分配
kubectl get pods -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.podIP
}{
        "\t"
}{
        .status.phase
}{
        "\n"
}{
    end
}'

# 验证Pod网络就绪状态
kubectl get pods -n <namespace> -o jsonpath='{
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

# 检查节点网络插件状态
kubectl get pods -n kube-system | grep -E "(calico|cilium|flannel|weave)"

# ========== 2. 网络接口检查 ==========
# 进入Pod检查网络接口
kubectl exec -n <namespace> <pod-name> -- ip addr show

# 查看路由表
kubectl exec -n <namespace> <pod-name> -- ip route show

# 检查DNS配置
kubectl exec -n <namespace> <pod-name> -- cat /etc/resolv.conf
```

### 2.2 CNI插件状态检查

```bash
# ========== CNI配置检查 ==========
# 查看CNI配置文件
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read node; do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "ls /etc/cni/net.d/"
done

# 检查CNI插件Pod状态
# Calico
kubectl get pods -n calico-system
# Cilium  
kubectl get pods -n kube-system | grep cilium
# Flannel
kubectl get pods -n kube-system | grep flannel

# ========== 网络插件日志检查 ==========
# Calico日志
kubectl logs -n calico-system -l k8s-app=calico-node --tail=100

# Cilium日志
kubectl logs -n kube-system -l k8s-app=cilium --tail=100

# Flannel日志
kubectl logs -n kube-system -l app=flannel --tail=100
```

---

## 3. Pod间通信问题排查 (Inter-Pod Communication Issues)

### 3.1 同Namespace通信测试

```bash
# ========== 1. 基础连通性测试 ==========
# 创建测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: network-test-client
  namespace: <namespace>
spec:
  containers:
  - name: client
    image: busybox
    command: ["sleep", "3600"]
---
apiVersion: v1
kind: Pod
metadata:
  name: network-test-server
  namespace: <namespace>
  labels:
    app: test-server
spec:
  containers:
  - name: server
    image: nginx
    ports:
    - containerPort: 80
EOF

# 测试Pod间Ping
kubectl exec -n <namespace> network-test-client -- ping -c 3 <server-pod-ip>

# 测试HTTP访问
kubectl exec -n <namespace> network-test-client -- wget -qO- http://<server-pod-ip>

# ========== 2. Service访问测试 ==========
# 创建测试Service
kubectl expose pod network-test-server --port=80 --target-port=80 --name=test-service -n <namespace>

# 通过Service访问
kubectl exec -n <namespace> network-test-client -- wget -qO- http://test-service.<namespace>.svc.cluster.local

# ========== 3. DNS解析测试 ==========
# 测试DNS解析
kubectl exec -n <namespace> network-test-client -- nslookup test-service.<namespace>.svc.cluster.local

# 检查DNS配置
kubectl exec -n <namespace> network-test-client -- cat /etc/resolv.conf
```

### 3.2 跨Namespace通信测试

```bash
# ========== 跨Namespace通信测试 ==========
# 在不同Namespace创建测试Pod
kubectl create namespace test-namespace

cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: cross-namespace-client
  namespace: test-namespace
spec:
  containers:
  - name: client
    image: busybox
    command: ["sleep", "3600"]
EOF

# 测试跨Namespace Service访问
kubectl exec -n test-namespace cross-namespace-client -- wget -qO- http://test-service.<original-namespace>.svc.cluster.local

# 验证网络策略影响
kubectl get networkpolicy --all-namespaces
```

---

## 4. Service网络问题排查 (Service Network Issues)

### 4.1 ClusterIP服务问题

```bash
# ========== 1. Service配置检查 ==========
# 查看Service详细信息
kubectl describe service <service-name> -n <namespace>

# 检查Endpoints
kubectl get endpoints <service-name> -n <namespace>

# 验证Selector匹配
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}'

# ========== 2. iptables/IPVS规则检查 ==========
# 在节点上检查iptables规则
NODE_NAME=$(kubectl get pods -n <namespace> -o jsonpath='{.items[0].spec.nodeName}')
kubectl debug node/$NODE_NAME --image=nicolaka/netshoot -it -- sh

# 在debug容器中执行:
# 检查NAT规则
iptables -t nat -L -n -v | grep <service-cluster-ip>

# 检查Endpoint规则
iptables -t nat -L KUBE-SERVICES -n -v | grep <service-name>

# 对于IPVS模式:
# ipvsadm -Ln | grep <service-cluster-ip>

# ========== 3. kube-proxy状态检查 ==========
# 检查kube-proxy Pod
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# 查看kube-proxy模式
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50 | grep -i "proxy mode"

# 检查kube-proxy配置
kubectl get configmap -n kube-system kube-proxy -o yaml
```

### 4.2 Headless Service问题

```bash
# ========== Headless Service测试 ==========
# 创建Headless Service
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Service
metadata:
  name: headless-service
  namespace: <namespace>
spec:
  clusterIP: None
  ports:
  - port: 80
    targetPort: 80
  selector:
    app: test-server
EOF

# 测试DNS SRV记录
kubectl exec -n <namespace> network-test-client -- nslookup headless-service.<namespace>.svc.cluster.local

# 验证Pod DNS记录
kubectl exec -n <namespace> network-test-client -- nslookup network-test-server.headless-service.<namespace>.svc.cluster.local
```

---

## 5. DNS解析问题排查 (DNS Resolution Issues)

### 5.1 CoreDNS状态检查

```bash
# ========== 1. CoreDNS基础检查 ==========
# 检查CoreDNS Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看CoreDNS配置
kubectl get configmap -n kube-system coredns -o yaml

# 检查CoreDNS日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100

# ========== 2. DNS解析测试 ==========
# 测试集群内部域名解析
kubectl exec -n <namespace> <pod-name> -- nslookup kubernetes.default.svc.cluster.local

# 测试外部域名解析
kubectl exec -n <namespace> <pod-name> -- nslookup google.com

# 测试反向解析
kubectl exec -n <namespace> <pod-name> -- nslookup <pod-ip>.in-addr.arpa

# ========== 3. DNS配置问题排查 ==========
# 检查Pod DNS策略
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsPolicy}'

# 验证DNS配置
kubectl exec -n <namespace> <pod-name> -- cat /etc/resolv.conf

# 检查自定义DNS配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig}'
```

### 5.2 DNS性能问题

```bash
# ========== DNS性能测试 ==========
# 测试DNS解析延迟
kubectl exec -n <namespace> <pod-name> -- sh -c "
for i in \$(seq 1 10); do
    start=\$(date +%s.%N)
    nslookup kubernetes.default.svc.cluster.local >/dev/null 2>&1
    end=\$(date +%s.%N)
    echo \"Query \$i: \$(echo \"\$end - \$start\" | bc) seconds\"
done
"

# ========== DNS缓存问题排查 ==========
# 检查CoreDNS缓存配置
kubectl get configmap -n kube-system coredns -o yaml | grep -A10 cache

# 测试缓存效果
kubectl exec -n <namespace> <pod-name> -- sh -c "
nslookup google.com
sleep 1
time nslookup google.com  # 第二次应该更快
"

# ========== DNS转发问题 ==========
# 检查上游DNS配置
kubectl get configmap -n kube-system coredns -o yaml | grep -A5 forward

# 测试上游DNS连通性
kubectl exec -n kube-system -l k8s-app=kube-dns -- nslookup google.com 8.8.8.8
```

---

## 6. 网络策略影响排查 (Network Policy Impact)

### 6.1 网络策略验证

```bash
# ========== 1. 策略影响分析 ==========
# 查看所有网络策略
kubectl get networkpolicy --all-namespaces

# 检查特定Pod受影响的策略
kubectl get networkpolicy -n <namespace> -o jsonpath='{
    range .items[*]
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

# ========== 2. 策略测试 ==========
# 创建测试策略
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: test-deny-all
  namespace: <namespace>
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF

# 测试策略效果
kubectl exec -n <namespace> network-test-client -- wget -qO- --timeout=5 http://test-service.<namespace>.svc.cluster.local

# 删除测试策略
kubectl delete networkpolicy test-deny-all -n <namespace>
```

### 6.2 策略调试工具

```bash
# ========== 网络策略调试 ==========
# 使用netshoot工具箱
kubectl run netshoot --image=nicolaka/netshoot -n <namespace> -it --rm -- sh

# 在netshoot容器中执行:
# 检查iptables规则
iptables -L -n -v | grep -i networkpolicy

# 测试特定端口连通性
nc -zv <target-pod-ip> <port>

# 检查连接跟踪
conntrack -L | grep <pod-ip>

# ========== CNI特定调试 ==========
# Calico调试
calicoctl get networkpolicy --all-namespaces
calicoctl get workloadendpoint -n <namespace>

# Cilium调试
cilium connectivity test
cilium monitor --type drop  # 监控丢包
```

---

## 7. 监控和告警配置 (Monitoring and Alerting)

### 7.1 网络连通性监控

```bash
# ========== 网络监控指标 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: network-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-state-metrics
  endpoints:
  - port: http-metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'kube_pod_status_ready|kube_service_info'
      action: keep
EOF

# ========== 网络连通性告警 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: network-connectivity-alerts
  namespace: monitoring
spec:
  groups:
  - name: network.rules
    rules:
    - alert: PodNetworkUnreachable
      expr: kube_pod_status_ready{condition="false"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Pod network is unreachable (namespace {{ \$labels.namespace }} pod {{ \$labels.pod }})"
        
    - alert: ServiceEndpointsMissing
      expr: kube_service_spec_type == 1 and kube_endpoint_address_not_ready == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Service has missing endpoints (namespace {{ \$labels.namespace }} service {{ \$labels.service }})"
        
    - alert: CoreDNSDown
      expr: up{job="coredns"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "CoreDNS is down"
        
    - alert: HighDNSErrorRate
      expr: increase(coredns_dns_responses_total{rcode="SERVFAIL"}[5m]) > 10
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "High DNS error rate detected"
EOF
```

### 7.2 网络健康检查脚本

```bash
# ========== 网络连通性检查脚本 ==========
cat <<'EOF' > network-connectivity-check.sh
#!/bin/bash

NAMESPACE=${1:-default}
TEST_DURATION=${2:-300}  # 5分钟测试

echo "Starting network connectivity test for namespace: $NAMESPACE"
echo "Test duration: $TEST_DURATION seconds"

# 创建测试Pod
cat <<TESTPOD | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: connectivity-test-client
  namespace: $NAMESPACE
spec:
  containers:
  - name: client
    image: busybox
    command: ["sleep", "3600"]
---
apiVersion: v1
kind: Pod
metadata:
  name: connectivity-test-server
  namespace: $NAMESPACE
  labels:
    app: test-server
spec:
  containers:
  - name: server
    image: nginx
    ports:
    - containerPort: 80
TESTPOD

# 等待Pod就绪
sleep 30

SERVER_IP=$(kubectl get pod connectivity-test-server -n $NAMESPACE -o jsonpath='{.status.podIP}')
SERVICE_NAME="test-connectivity-service"

# 创建测试Service
kubectl expose pod connectivity-test-server --port=80 --target-port=80 --name=$SERVICE_NAME -n $NAMESPACE

# 执行连通性测试
END_TIME=$(($(date +%s) + TEST_DURATION))
while [ $(date +%s) -lt $END_TIME ]; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 测试Pod间通信
    if kubectl exec -n $NAMESPACE connectivity-test-client -- ping -c 1 -W 3 $SERVER_IP >/dev/null 2>&1; then
        echo "$TIMESTAMP - Pod-to-Pod: ✓"
    else
        echo "$TIMESTAMP - Pod-to-Pod: ✗"
    fi
    
    # 测试Service访问
    if kubectl exec -n $NAMESPACE connectivity-test-client -- wget -qO- --timeout=5 http://$SERVICE_NAME.$NAMESPACE.svc.cluster.local >/dev/null 2>&1; then
        echo "$TIMESTAMP - Service Access: ✓"
    else
        echo "$TIMESTAMP - Service Access: ✗"
    fi
    
    # 测试DNS解析
    if kubectl exec -n $NAMESPACE connectivity-test-client -- nslookup $SERVICE_NAME.$NAMESPACE.svc.cluster.local >/dev/null 2>&1; then
        echo "$TIMESTAMP - DNS Resolution: ✓"
    else
        echo "$TIMESTAMP - DNS Resolution: ✗"
    fi
    
    sleep 30
done

# 清理测试资源
kubectl delete pod connectivity-test-client connectivity-test-server -n $NAMESPACE
kubectl delete service $SERVICE_NAME -n $NAMESPACE

echo "Network connectivity test completed"
EOF

chmod +x network-connectivity-check.sh

# ========== 网络性能基准测试 ==========
cat <<'EOF' > network-benchmark.sh
#!/bin/bash

SOURCE_POD=$1
TARGET_POD=$2
NAMESPACE=${3:-default}
ITERATIONS=${4:-10}

echo "Running network benchmark: $SOURCE_POD -> $TARGET_POD"

TARGET_IP=$(kubectl get pod $TARGET_POD -n $NAMESPACE -o jsonpath='{.status.podIP}')

echo "Target IP: $TARGET_IP"
echo "Iterations: $ITERATIONS"
echo ""

# ICMP测试
echo "=== ICMP Ping Test ==="
kubectl exec -n $NAMESPACE $SOURCE_POD -- ping -c $ITERATIONS $TARGET_IP

# HTTP测试
echo -e "\n=== HTTP Latency Test ==="
kubectl exec -n $NAMESPACE $SOURCE_POD -- sh -c "
for i in \$(seq 1 $ITERATIONS); do
    start=\$(date +%s.%N)
    wget -qO- --timeout=5 http://$TARGET_IP >/dev/null 2>&1
    end=\$(date +%s.%N)
    echo \"Request \$i: \$(echo \"\$end - \$start\" | bc) seconds\"
done
"

# DNS测试
echo -e "\n=== DNS Resolution Test ==="
kubectl exec -n $NAMESPACE $SOURCE_POD -- sh -c "
for i in \$(seq 1 $ITERATIONS); do
    start=\$(date +%s.%N)
    nslookup kubernetes.default.svc.cluster.local >/dev/null 2>&1
    end=\$(date +%s.%N)
    echo \"DNS Query \$i: \$(echo \"\$end - \$start\" | bc) seconds\"
done
"
EOF

chmod +x network-benchmark.sh
```

---