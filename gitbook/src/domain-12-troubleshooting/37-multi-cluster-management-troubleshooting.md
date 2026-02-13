# 37 - 多集群管理故障排查 (Multi-Cluster Management Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Multi-Cluster](https://kubernetes.io/docs/concepts/cluster-administration/federation/)

---

## 1. 多集群管理故障诊断总览 (Multi-Cluster Management Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **集群连接异常** | 无法访问远程集群 | 跨集群操作失败 | P0 - 紧急 |
| **资源同步失败** | Federation控制器异常 | 多集群资源不一致 | P1 - 高 |
| **身份认证问题** | RBAC权限不足 | 跨集群访问受限 | P1 - 高 |
| **网络连通性** | 集群间网络不通 | 服务发现失效 | P0 - 紧急 |
| **配置管理混乱** | 配置漂移/冲突 | 运维复杂度增加 | P2 - 中 |

### 1.2 多集群架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    多集群管理故障诊断架构                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      管理平面层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   控制台    │    │   CLI工具   │    │   API网关   │              │  │
│  │  │ (Console)   │    │  (kubectl)  │    │  (Gateway)  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   配置管理   │   │   身份认证   │   │   网络管理   │                   │
│  │ (Config)    │   │ (Identity)  │   │  (Network)  │                   │
│  │   同步      │   │   联邦      │   │   互联      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      联邦控制层                                     │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                    Federation Controller                      │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   资源同步   │  │   状态监控   │  │   故障恢复   │           │  │  │
│  │  │  │ (Sync)      │  │  (Monitor)  │  │  (Recovery) │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   集群A     │    │   集群B     │    │   集群C     │                   │
│  │ (Cluster)   │    │ (Cluster)   │    │ (Cluster)   │                   │
│  │   成员      │    │   成员      │    │   成员      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   工作负载   │    │   工作负载   │    │   工作负载   │                   │
│  │  (Workload) │    │  (Workload) │    │  (Workload) │                   │
│  │   运行      │    │   运行      │    │   运行      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      服务发现层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   DNS服务   │    │   负载均衡   │   │   服务网格   │              │  │
│  │  │  (DNS)      │    │  (LB)       │   │  (Mesh)     │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 集群连接和认证故障排查 (Cluster Connection and Authentication Issues)

### 2.1 集群连接状态检查

```bash
# ========== 1. 多集群连接验证 ==========
# 检查当前上下文
kubectl config current-context

# 列出所有集群上下文
kubectl config get-contexts

# 验证集群可达性
for context in $(kubectl config get-contexts -o name); do
    echo "Testing connection to: $context"
    kubectl cluster-info --context=$context
done

# ========== 2. 集群健康状态检查 ==========
# 并行检查所有集群状态
cat <<'EOF' > cluster-health-check.sh
#!/bin/bash

echo "=== Multi-Cluster Health Check ==="

# 获取所有上下文
CONTEXTS=$(kubectl config get-contexts -o name)

# 并行检查每个集群
for context in $CONTEXTS; do
    (
        echo "Checking cluster: $context"
        
        # 检查API服务器连通性
        if kubectl cluster-info --context=$context &>/dev/null; then
            echo "  ✓ API Server: Connected"
        else
            echo "  ✗ API Server: Unreachable"
        fi
        
        # 检查节点状态
        NODE_COUNT=$(kubectl get nodes --context=$context 2>/dev/null | grep -v NAME | wc -l)
        if [ $NODE_COUNT -gt 0 ]; then
            READY_NODES=$(kubectl get nodes --context=$context 2>/dev/null | grep Ready | wc -l)
            echo "  ✓ Nodes: $READY_NODES/$NODE_COUNT ready"
        else
            echo "  ✗ Nodes: Unable to retrieve node information"
        fi
        
        # 检查核心组件
        CORE_COMPONENTS=$(kubectl get pods -n kube-system --context=$context 2>/dev/null | grep -E "(kube-apiserver|etcd|kube-controller|kube-scheduler)" | wc -l)
        RUNNING_COMPONENTS=$(kubectl get pods -n kube-system --context=$context 2>/dev/null | grep -E "(kube-apiserver|etcd|kube-controller|kube-scheduler)" | grep Running | wc -l)
        echo "  ✓ Core Components: $RUNNING_COMPONENTS/$CORE_COMPONENTS running"
        
    ) &
done

# 等待所有检查完成
wait

echo "Health check completed"
EOF

chmod +x cluster-health-check.sh

# ========== 3. 网络连通性测试 ==========
# 跨集群网络测试
cat <<'EOF' > cross-cluster-network-test.sh
#!/bin/bash

SOURCE_CONTEXT=$1
TARGET_CONTEXT=$2

if [ -z "$SOURCE_CONTEXT" ] || [ -z "$TARGET_CONTEXT" ]; then
    echo "Usage: $0 <source-context> <target-context>"
    exit 1
fi

echo "Testing network connectivity from $SOURCE_CONTEXT to $TARGET_CONTEXT"

# 获取目标集群API服务器地址
API_SERVER=$(kubectl config view -o jsonpath="{.clusters[?(@.name == \"$TARGET_CONTEXT\")].cluster.server}" --context=$TARGET_CONTEXT)

if [ -z "$API_SERVER" ]; then
    echo "Failed to get API server address for $TARGET_CONTEXT"
    exit 1
fi

echo "Target API Server: $API_SERVER"

# 在源集群中测试连通性
kubectl run network-test --image=busybox --context=$SOURCE_CONTEXT --rm -it -- sh -c "
    echo 'Testing DNS resolution...'
    nslookup $(echo $API_SERVER | sed 's|https://||' | sed 's|:.*||')
    
    echo 'Testing TCP connectivity...'
    telnet $(echo $API_SERVER | sed 's|https://||' | sed 's|:.*||') $(echo $API_SERVER | sed 's|.*:||')
    
    echo 'Testing HTTPS connectivity...'
    wget --spider --timeout=10 $API_SERVER/healthz
"
EOF

chmod +x cross-cluster-network-test.sh
```

### 2.2 身份认证和授权问题

```bash
# ========== 认证配置检查 ==========
# 检查kubeconfig配置
kubectl config view --raw

# 验证认证令牌有效性
for context in $(kubectl config get-contexts -o name); do
    echo "Checking auth for context: $context"
    kubectl auth can-i get pods --context=$context
done

# ========== RBAC权限验证 ==========
# 创建权限测试脚本
cat <<'EOF' > rbac-permission-test.sh
#!/bin/bash

CONTEXT=$1
NAMESPACE=${2:-default}

if [ -z "$CONTEXT" ]; then
    echo "Usage: $0 <context> [namespace]"
    exit 1
fi

echo "Testing RBAC permissions for context: $CONTEXT"

PERMISSIONS=(
    "get pods"
    "create deployments"
    "delete services"
    "list namespaces"
    "get nodes"
    "create persistentvolumeclaims"
)

for permission in "${PERMISSIONS[@]}"; do
    if kubectl auth can-i $permission --context=$CONTEXT -n $NAMESPACE; then
        echo "  ✓ $permission"
    else
        echo "  ✗ $permission"
    fi
done

# 测试跨命名空间权限
echo "Testing cross-namespace permissions:"
for ns in $(kubectl get namespaces --context=$CONTEXT -o jsonpath='{.items[*].metadata.name}'); do
    if kubectl auth can-i get pods --context=$CONTEXT -n $ns; then
        echo "  ✓ Access to namespace: $ns"
    else
        echo "  ✗ No access to namespace: $ns"
    fi
done
EOF

chmod +x rbac-permission-test.sh

# ========== 服务账户令牌问题 ==========
# 检查服务账户令牌
kubectl get secrets -n <namespace> | grep service-account

# 验证令牌有效性
TOKEN_SECRET=$(kubectl get serviceaccount default -n <namespace> -o jsonpath='{.secrets[0].name}')
kubectl get secret $TOKEN_SECRET -n <namespace> -o jsonpath='{.data.token}' | base64 -d

# 重建无效令牌
kubectl delete secret $TOKEN_SECRET -n <namespace>
kubectl create serviceaccount default -n <namespace> --save-config=false
```

---

## 3. 资源同步和联邦控制故障排查 (Resource Sync and Federation Control Issues)

### 3.1 联邦控制器状态检查

```bash
# ========== 1. 联邦控制器验证 ==========
# 检查联邦控制器部署
kubectl get deployments -n federation-system

# 查看控制器Pod状态
kubectl get pods -n federation-system -l app=federation-controller-manager

# 检查控制器日志
kubectl logs -n federation-system -l app=federation-controller-manager --tail=100

# 验证控制器版本
kubectl get deployments -n federation-system federation-controller-manager -o jsonpath='{.spec.template.spec.containers[0].image}'

# ========== 2. 资源同步状态分析 ==========
# 检查联邦资源状态
kubectl get federatedresources -A

# 分析同步失败的资源
kubectl get federatedresources -A | grep -v "true"

# 查看详细同步状态
for fr in $(kubectl get federatedresources -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
    echo "=== $fr ==="
    kubectl describe federatedresource $fr
done

# ========== 3. 同步延迟监控 ==========
# 创建同步延迟监控脚本
cat <<'EOF' > sync-latency-monitor.sh
#!/bin/bash

INTERVAL=${1:-60}  # 检查间隔秒数

echo "Starting sync latency monitoring (interval: ${INTERVAL}s)"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    echo "$TIMESTAMP - Checking sync latencies:"
    
    # 检查各集群的资源同步时间
    for context in $(kubectl config get-contexts -o name); do
        # 获取该集群中最近更新的资源
        LATEST_RESOURCE=$(kubectl get all -A --context=$context --sort-by=.metadata.creationTimestamp 2>/dev/null | tail -1)
        if [ -n "$LATEST_RESOURCE" ]; then
            RESOURCE_AGE=$(kubectl get all -A --context=$context --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1:].metadata.creationTimestamp}' 2>/dev/null)
            if [ -n "$RESOURCE_AGE" ]; then
                AGE_SECONDS=$(($(date +%s) - $(date -d "$RESOURCE_AGE" +%s)))
                echo "  $context: Latest resource age ${AGE_SECONDS}s"
                
                # 如果超过阈值则告警
                if [ $AGE_SECONDS -gt 300 ]; then  # 5分钟阈值
                    echo "  ⚠️  $context: High sync latency detected (${AGE_SECONDS}s)"
                fi
            fi
        fi
    done
    
    echo "---"
    sleep $INTERVAL
done
EOF

chmod +x sync-latency-monitor.sh
```

### 3.2 联邦资源配置问题

```bash
# ========== 配置模板验证 ==========
# 检查联邦资源配置
kubectl get federatedtypesconfigs -n federation-system

# 验证支持的资源类型
kubectl get federatedtypesconfigs -n federation-system -o jsonpath='{.items[*].metadata.name}'

# 分析配置冲突
kubectl describe federatedtypesconfig <resource-type> -n federation-system

# ========== 资源分布策略检查 ==========
# 检查放置策略
kubectl get propagationpolicies -A

# 验证集群选择器
kubectl get clusterselectors -A

# 分析资源分布
for pp in $(kubectl get propagationpolicies -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
    echo "=== Policy: $pp ==="
    kubectl describe propagationpolicy $pp
done

# ========== 自动故障转移测试 ==========
# 创建故障转移测试脚本
cat <<'EOF' > failover-test.sh
#!/bin/bash

FEDERATED_RESOURCE=$1
FAULTY_CLUSTER=$2

if [ -z "$FEDERATED_RESOURCE" ] || [ -z "$FAULTY_CLUSTER" ]; then
    echo "Usage: $0 <federated-resource> <faulty-cluster>"
    exit 1
fi

echo "Testing failover for $FEDERATED_RESOURCE from $FAULTY_CLUSTER"

# 1. 记录当前资源分布
echo "Current resource distribution:"
kubectl get $FEDERATED_RESOURCE -o jsonpath='{.spec.placement.clusters[*].name}'

# 2. 模拟集群故障
echo "Simulating cluster failure for $FAULTY_CLUSTER"
kubectl cordon $FAULTY_CLUSTER

# 3. 等待重新调度
echo "Waiting for failover..."
sleep 120

# 4. 验证新的资源分布
echo "New resource distribution:"
kubectl get $FEDERATED_RESOURCE -o jsonpath='{.spec.placement.clusters[*].name}'

# 5. 恢复集群
echo "Restoring cluster $FAULTY_CLUSTER"
kubectl uncordon $FAULTY_CLUSTER

echo "Failover test completed"
EOF

chmod +x failover-test.sh
```

---

## 4. 跨集群网络和服务发现问题排查 (Cross-Cluster Networking and Service Discovery Issues)

### 4.1 服务发现配置检查

```bash
# ========== 1. DNS配置验证 ==========
# 检查集群DNS配置
for context in $(kubectl config get-contexts -o name); do
    echo "=== DNS Config for $context ==="
    kubectl get configmap -n kube-system coredns --context=$context -o yaml | grep -A 10 "kubernetes"
done

# 验证跨集群DNS解析
cat <<'EOF' > cross-cluster-dns-test.sh
#!/bin/bash

SERVICE_NAME=$1
SOURCE_CONTEXT=$2

if [ -z "$SERVICE_NAME" ] || [ -z "$SOURCE_CONTEXT" ]; then
    echo "Usage: $0 <service-name> <source-context>"
    exit 1
fi

echo "Testing cross-cluster DNS for service: $SERVICE_NAME"

# 在每个集群中测试DNS解析
for context in $(kubectl config get-contexts -o name); do
    echo "Testing from context: $context"
    
    # 获取服务的集群IP
    SERVICE_IP=$(kubectl get service $SERVICE_NAME --context=$context -o jsonpath='{.spec.clusterIP}' 2>/dev/null)
    
    if [ -n "$SERVICE_IP" ]; then
        echo "  Service IP in $context: $SERVICE_IP"
        
        # 从源集群测试解析
        kubectl run dns-test-$context --image=busybox --context=$SOURCE_CONTEXT --rm -it -- sh -c "
            echo 'Resolving $SERVICE_NAME.$context.svc.cluster.local'
            nslookup $SERVICE_NAME.$context.svc.cluster.local
        "
    else
        echo "  Service not found in $context"
    fi
done
EOF

chmod +x cross-cluster-dns-test.sh

# ========== 2. 网络策略检查 ==========
# 检查跨集群网络策略
kubectl get networkpolicies -A --context=<context>

# 验证网络连通性
cat <<'EOF' > network-connectivity-test.sh
#!/bin/bash

SOURCE_CONTEXT=$1
TARGET_SERVICE=$2
TARGET_PORT=${3:-80}

if [ -z "$SOURCE_CONTEXT" ] || [ -z "$TARGET_SERVICE" ]; then
    echo "Usage: $0 <source-context> <target-service> [port]"
    exit 1
fi

echo "Testing network connectivity from $SOURCE_CONTEXT to $TARGET_SERVICE:$TARGET_PORT"

# 在源集群中部署测试Pod
TEST_POD="connectivity-test-$(date +%s)"

kubectl run $TEST_POD \
    --image=busybox \
    --context=$SOURCE_CONTEXT \
    --restart=Never \
    --command -- sleep 3600

# 等待Pod就绪
sleep 30

# 测试到各个集群的连接
for context in $(kubectl config get-contexts -o name); do
    echo "Testing connectivity to $context cluster..."
    
    # 获取目标服务在该集群的访问地址
    SERVICE_CLUSTER_IP=$(kubectl get service $TARGET_SERVICE --context=$context -o jsonpath='{.spec.clusterIP}' 2>/dev/null)
    
    if [ -n "$SERVICE_CLUSTER_IP" ]; then
        kubectl exec $TEST_POD --context=$SOURCE_CONTEXT -- \
            nc -zvw3 $SERVICE_CLUSTER_IP $TARGET_PORT && \
            echo "  ✓ Connected to $SERVICE_CLUSTER_IP:$TARGET_PORT" || \
            echo "  ✗ Failed to connect to $SERVICE_CLUSTER_IP:$TARGET_PORT"
    fi
done

# 清理测试Pod
kubectl delete pod $TEST_POD --context=$SOURCE_CONTEXT
EOF

chmod +x network-connectivity-test.sh
```

### 4.2 负载均衡和服务网格问题

```bash
# ========== 负载均衡器状态 ==========
# 检查跨集群负载均衡
kubectl get services -A --context=<context> | grep LoadBalancer

# 验证外部IP分配
for context in $(kubectl config get-contexts -o name); do
    echo "=== LoadBalancer IPs in $context ==="
    kubectl get services -A --context=$context | grep LoadBalancer | awk '{print $4}'
done

# ========== 服务网格集成检查 ==========
# Istio多集群配置检查
istioctl remote-clusters

# 验证服务网格连通性
cat <<'EOF' > mesh-connectivity-test.sh
#!/bin/bash

SOURCE_CONTEXT=$1
TARGET_CONTEXT=$2

if [ -z "$SOURCE_CONTEXT" ] || [ -z "$TARGET_CONTEXT" ]; then
    echo "Usage: $0 <source-context> <target-context>"
    exit 1
fi

echo "Testing service mesh connectivity between $SOURCE_CONTEXT and $TARGET_CONTEXT"

# 检查Istio安装状态
for context in $SOURCE_CONTEXT $TARGET_CONTEXT; do
    echo "Checking Istio in $context:"
    kubectl get pods -n istio-system --context=$context | grep Running
done

# 部署测试应用
kubectl apply -f - <<TESTAPP --context=$SOURCE_CONTEXT
apiVersion: v1
kind: Pod
metadata:
  name: mesh-test-source
  labels:
    app: mesh-test
spec:
  containers:
  - name: tester
    image: curlimages/curl
    command: ["sleep", "3600"]
TESTAPP

kubectl apply -f - <<TESTAPP --context=$TARGET_CONTEXT
apiVersion: v1
kind: Service
metadata:
  name: mesh-test-target
spec:
  selector:
    app: mesh-test-target
  ports:
  - port: 80
    targetPort: 8080
---
apiVersion: v1
kind: Pod
metadata:
  name: mesh-test-target
  labels:
    app: mesh-test-target
spec:
  containers:
  - name: server
    image: nginx
    ports:
    - containerPort: 8080
TESTAPP

# 等待应用就绪
sleep 60

# 测试跨集群服务调用
kubectl exec mesh-test-source --context=$SOURCE_CONTEXT -- \
    curl -s http://mesh-test-target.$TARGET_CONTEXT.svc.cluster.local

# 清理测试资源
kubectl delete pod mesh-test-source --context=$SOURCE_CONTEXT
kubectl delete service mesh-test-target pod mesh-test-target --context=$TARGET_CONTEXT
EOF

chmod +x mesh-connectivity-test.sh
```

---

## 5. 监控和告警配置 (Monitoring and Alerting Configuration)

### 5.1 多集群监控集成

```bash
# ========== 监控配置 ==========
# 创建多集群Prometheus配置
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-multi-cluster-config
  namespace: monitoring
data:
  prometheus.yml: |
    global:
      scrape_interval: 15s
      
    scrape_configs:
    # 本地集群监控
    - job_name: 'kubernetes-nodes-local'
      kubernetes_sd_configs:
      - role: node
      relabel_configs:
      - source_labels: [__address__]
        target_label: __param_target
      - source_labels: [__param_target]
        target_label: instance
      - target_label: __address__
        replacement: kubernetes.default.svc:443
      - source_labels: [__meta_kubernetes_node_name]
        target_label: node
    
    # 远程集群监控
    - job_name: 'kubernetes-nodes-remote'
      kubernetes_sd_configs:
      - api_server: 'https://remote-cluster-api:6443'
        role: node
        authorization:
          credentials_file: /var/run/secrets/kubernetes.io/serviceaccount/token
        tls_config:
          ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      relabel_configs:
      - source_labels: [__address__]
        target_label: __param_target
      - source_labels: [__param_target]
        target_label: instance
      - target_label: __address__
        replacement: kubernetes.default.svc:443
      - source_labels: [__meta_kubernetes_node_name]
        target_label: node
        replacement: "\${1}-remote"
EOF

# ========== 联邦监控配置 ==========
# 创建联邦Prometheus配置
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: federation-prometheus
  namespace: monitoring
spec:
  serviceAccountName: prometheus
  serviceMonitorSelector:
    matchLabels:
      team: frontend
  ruleSelector:
    matchLabels:
      team: frontend
  resources:
    requests:
      memory: 400Mi
  enableAdminAPI: false
  externalLabels:
    cluster: federation-control-plane
  remoteRead:
  - url: http://prometheus-remote1.monitoring.svc:9090/api/v1/read
    readRecent: true
  - url: http://prometheus-remote2.monitoring.svc:9090/api/v1/read
    readRecent: true
EOF
```

### 5.2 多集群告警规则

```bash
# ========== 跨集群告警规则 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: multi-cluster-alerts
  namespace: monitoring
spec:
  groups:
  - name: multi-cluster.rules
    rules:
    # 集群连通性告警
    - alert: ClusterUnreachable
      expr: up{job=~"kubernetes-nodes.*"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Cluster {{ \$labels.cluster }} is unreachable"
        
    # 资源同步延迟告警
    - alert: ResourceSyncDelay
      expr: federation_resource_sync_duration_seconds > 300
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Resource sync delay detected in cluster {{ \$labels.cluster }}"
        
    # 跨集群网络延迟
    - alert: CrossClusterNetworkLatency
      expr: probe_duration_seconds{job="cross-cluster-probe"} > 1
      for: 3m
      labels:
        severity: warning
      annotations:
        summary: "High cross-cluster network latency ({{ \$value }}s)"
        
    # 联邦控制器异常
    - alert: FederationControllerDown
      expr: up{job="federation-controller"} == 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Federation controller is down"
        
    # 配置漂移检测
    - alert: ConfigurationDriftDetected
      expr: count(count by (cluster, config_hash) (federation_config_version)) > 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Configuration drift detected across clusters"
EOF

# ========== 健康检查探针配置 ==========
# 创建跨集群健康检查
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cross-cluster-health-check
  namespace: monitoring
spec:
  schedule: "*/5 * * * *"
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: health-check
            image: curlimages/curl
            command:
            - /bin/sh
            - -c
            - |
              # 检查各个集群的健康状态
              CLUSTERS="cluster1 cluster2 cluster3"
              
              for cluster in \$CLUSTERS; do
                echo "Checking health of \$cluster..."
                
                # 检查API服务器
                curl -sk https://\$cluster-api:6443/healthz | grep -q "ok" && \
                  echo "✓ \$cluster API is healthy" || \
                  echo "✗ \$cluster API is unhealthy"
                  
                # 检查核心组件
                kubectl get pods -n kube-system --context=\$cluster 2>/dev/null | grep -E "(kube-apiserver|etcd)" | grep -q Running && \
                  echo "✓ \$cluster core components are running" || \
                  echo "✗ \$cluster core components have issues"
              done
              
              # 发送告警到监控系统
              if [ \$? -ne 0 ]; then
                curl -X POST http://alertmanager:9093/api/v1/alerts \
                  -H "Content-Type: application/json" \
                  -d '[{
                    "status": "firing",
                    "labels": {
                      "alertname": "CrossClusterHealthCheckFailed",
                      "severity": "critical"
                    },
                    "annotations": {
                      "summary": "Cross-cluster health check failed"
                    }
                  }]'
              fi
            env:
            - name: CLUSTER1_API
              value: "https://cluster1-api:6443"
            - name: CLUSTER2_API
              value: "https://cluster2-api:6443"
            - name: CLUSTER3_API
              value: "https://cluster3-api:6443"
          restartPolicy: OnFailure
EOF
```

---

## 6. 灾难恢复和故障转移 (Disaster Recovery and Failover)

### 6.1 备份和恢复策略

```bash
# ========== 多集群备份配置 ==========
# 创建备份脚本
cat <<'EOF' > multi-cluster-backup.sh
#!/bin/bash

BACKUP_DIR=${1:-"/backup/multi-cluster"}
TIMESTAMP=$(date +%Y%m%d-%H%M%S)

echo "Starting multi-cluster backup to $BACKUP_DIR/$TIMESTAMP"

mkdir -p $BACKUP_DIR/$TIMESTAMP

# 备份每个集群的配置
for context in $(kubectl config get-contexts -o name); do
    echo "Backing up cluster: $context"
    
    CLUSTER_BACKUP_DIR="$BACKUP_DIR/$TIMESTAMP/$context"
    mkdir -p $CLUSTER_BACKUP_DIR
    
    # 备份集群配置
    kubectl config view --raw --context=$context > $CLUSTER_BACKUP_DIR/kubeconfig
    
    # 备份重要资源
    RESOURCES=("namespaces" "deployments" "services" "configmaps" "secrets" "persistentvolumeclaims")
    
    for resource in "${RESOURCES[@]}"; do
        echo "  Backing up $resource..."
        kubectl get $resource -A --context=$context -o yaml > $CLUSTER_BACKUP_DIR/${resource}.yaml 2>/dev/null
    done
    
    # 备份联邦资源配置
    if kubectl get federatedresources --context=$context &>/dev/null; then
        kubectl get federatedresources -A --context=$context -o yaml > $CLUSTER_BACKUP_DIR/federatedresources.yaml
    fi
done

# 创建备份元数据
cat > $BACKUP_DIR/$TIMESTAMP/metadata.json <<METADATA
{
  "timestamp": "$TIMESTAMP",
  "clusters": $(kubectl config get-contexts -o name | jq -R -s 'split("\n")[:-1]'),
  "backup_version": "1.0",
  "created_by": "$(whoami)"
}
METADATA

# 压缩备份
tar -czf $BACKUP_DIR/backup-$TIMESTAMP.tar.gz -C $BACKUP_DIR $TIMESTAMP
rm -rf $BACKUP_DIR/$TIMESTAMP

echo "Backup completed: $BACKUP_DIR/backup-$TIMESTAMP.tar.gz"
EOF

chmod +x multi-cluster-backup.sh

# ========== 恢复测试脚本 ==========
# 创建恢复验证脚本
cat <<'EOF' > restore-validation.sh
#!/bin/bash

BACKUP_FILE=$1

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <backup-file.tar.gz>"
    exit 1
fi

echo "Validating backup: $BACKUP_FILE"

# 解压备份
TEMP_DIR="/tmp/restore-validation-$(date +%s)"
mkdir -p $TEMP_DIR
tar -xzf $BACKUP_FILE -C $TEMP_DIR

# 验证备份完整性
BACKUP_CONTENTS=$(ls $TEMP_DIR)
echo "Backup contains: $BACKUP_CONTENTS"

# 验证每个集群的配置
for cluster_dir in $TEMP_DIR/*/; do
    cluster_name=$(basename $cluster_dir)
    echo "Validating cluster: $cluster_name"
    
    # 检查必要文件
    REQUIRED_FILES=("kubeconfig" "namespaces.yaml" "deployments.yaml")
    for file in "${REQUIRED_FILES[@]}"; do
        if [ -f "$cluster_dir/$file" ]; then
            echo "  ✓ Found $file"
        else
            echo "  ✗ Missing $file"
        fi
    done
    
    # 验证YAML语法
    for yaml_file in $cluster_dir/*.yaml; do
        if [ -f "$yaml_file" ]; then
            yamllint $yaml_file >/dev/null 2>&1 && \
                echo "  ✓ Valid YAML: $(basename $yaml_file)" || \
                echo "  ✗ Invalid YAML: $(basename $yaml_file)"
        fi
    done
done

# 清理临时目录
rm -rf $TEMP_DIR

echo "Validation completed"
EOF

chmod +x restore-validation.sh
```

### 6.2 自动故障转移配置

```bash
# ========== 故障检测和转移控制器 ==========
# 创建自动故障转移控制器
cat <<'EOF' > failover-controller.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: failover-controller
  namespace: federation-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: failover-controller
  template:
    metadata:
      labels:
        app: failover-controller
    spec:
      containers:
      - name: controller
        image: k8s-failover-controller:latest
        env:
        - name: HEALTH_CHECK_INTERVAL
          value: "30s"
        - name: FAILOVER_THRESHOLD
          value: "3"
        - name: PRIMARY_CLUSTER
          value: "cluster1"
        - name: STANDBY_CLUSTERS
          value: "cluster2,cluster3"
        volumeMounts:
        - name: kubeconfig
          mountPath: /root/.kube
          readOnly: true
      volumes:
      - name: kubeconfig
        secret:
          secretName: multi-cluster-kubeconfig
---
apiVersion: v1
kind: Secret
metadata:
  name: multi-cluster-kubeconfig
  namespace: federation-system
type: Opaque
data:
  config: <base64-encoded-kubeconfig>
EOF

# ========== 故障转移策略配置 ==========
# 创建故障转移策略
cat <<'EOF' > failover-policy.yaml
apiVersion: federation.k8s.io/v1alpha1
kind: FailoverPolicy
metadata:
  name: default-failover-policy
spec:
  # 主集群配置
  primary:
    clusterName: cluster1
    priority: 1
    
  # 备用集群配置
  standbys:
  - clusterName: cluster2
    priority: 2
    weight: 50
  - clusterName: cluster3
    priority: 3
    weight: 30
    
  # 故障检测配置
  healthCheck:
    interval: 30s
    timeout: 10s
    failureThreshold: 3
    successThreshold: 1
    
  # 自动转移配置
  autoFailover:
    enabled: true
    cooldownPeriod: 300s  # 5分钟冷却期
    maxFailovers: 3       # 最大转移次数
    
  # 数据同步配置
  dataSync:
    enabled: true
    syncInterval: 60s
    consistencyLevel: eventual
    
  # 通知配置
  notifications:
    email:
      enabled: true
      recipients:
      - sre-team@example.com
    webhook:
      enabled: true
      url: https://webhook.example.com/failover
EOF

# ========== 手动故障转移工具 ==========
# 创建故障转移CLI工具
cat <<'EOF' > cluster-failover.sh
#!/bin/bash

ACTION=$1
TARGET_CLUSTER=$2

case $ACTION in
    "status")
        echo "=== Current Cluster Status ==="
        kubectl get federatedclusters -o wide
        ;;
        
    "switch")
        if [ -z "$TARGET_CLUSTER" ]; then
            echo "Usage: $0 switch <target-cluster>"
            exit 1
        fi
        
        echo "Switching primary cluster to: $TARGET_CLUSTER"
        
        # 验证目标集群健康
        if ! kubectl get federatedcluster $TARGET_CLUSTER -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}' | grep -q True; then
            echo "Error: Target cluster $TARGET_CLUSTER is not ready"
            exit 1
        fi
        
        # 执行故障转移
        kubectl patch federatedcluster $TARGET_CLUSTER -p '{"spec":{"primary":true}}' --type=merge
        
        # 验证转移结果
        sleep 30
        kubectl get federatedclusters -o jsonpath='{.items[*].metadata.name}:{.spec.primary}' | tr ' ' '\n'
        ;;
        
    "drain")
        if [ -z "$TARGET_CLUSTER" ]; then
            echo "Usage: $0 drain <cluster-name>"
            exit 1
        fi
        
        echo "Draining cluster: $TARGET_CLUSTER"
        kubectl cordon $TARGET_CLUSTER
        kubectl drain $TARGET_CLUSTER --ignore-daemonsets --delete-emptydir-data
        ;;
        
    *)
        echo "Usage: $0 {status|switch|drain} [cluster-name]"
        echo "  status: Show current cluster status"
        echo "  switch: Switch primary cluster"
        echo "  drain: Drain a cluster for maintenance"
        exit 1
        ;;
esac
EOF

chmod +x cluster-failover.sh
```

---