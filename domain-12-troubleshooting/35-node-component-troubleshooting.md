# 35 - 节点组件故障排查 (Node Component Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Node Components](https://kubernetes.io/docs/concepts/architecture/nodes/)

---

## 目录

1. [概述与诊断框架](#1-概述与诊断框架)
2. [kubelet 故障排查](#2-kubelet-故障排查)
3. [容器运行时故障排查](#3-容器运行时故障排查)
4. [kube-proxy 故障排查](#4-kube-proxy-故障排查)
5. [节点资源压力诊断](#5-节点资源压力诊断)
6. [证书和认证问题](#6-证书和认证问题)
7. [自动化诊断工具](#7-自动化诊断工具)
8. [监控告警配置](#8-监控告警配置)
9. [最佳实践](#9-最佳实践)

---

## 1. 概述与诊断框架

### 1.1 节点组件概述

Kubernetes 节点组件是集群工作节点上的核心守护进程，负责 Pod 的运行、网络管理和资源调度。主要包括：

- **kubelet**: 节点代理，负责 Pod 生命周期管理
- **kube-proxy**: 网络代理，维护 Service 网络规则
- **容器运行时**: Docker/containerd/cri-o，负责容器执行
- **cAdvisor**: 资源监控代理（集成在 kubelet 中）

### 1.2 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **kubelet异常** | Node NotReady/Unknown | 节点不可调度 | P0 - 紧急 |
| **容器运行时故障** | Pod无法创建/启动 | 应用部署失败 | P0 - 紧急 |
| **kube-proxy异常** | Service访问失败 | 网络连通性中断 | P1 - 高 |
| **节点资源压力** | Eviction/Pod驱逐 | 应用服务中断 | P0 - 紧急 |
| **证书过期** | 节点认证失败 | 节点失联 | P0 - 紧急 |

### 1.3 节点组件架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    节点组件故障诊断架构                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       节点管理层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   kubelet   │    │ kube-proxy  │    │  容器运行时  │              │  │
│  │  │  (Node Agent)│   │ (Network)   │    │ (Container) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   状态上报   │   │   网络代理   │   │   镜像管理   │                   │
│  │ (Status)    │   │ (Proxy)     │   │ (Image)     │                   │
│  │   心跳      │   │   iptables  │   │   拉取      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      节点资源层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   CPU调度   │    │   内存管理   │    │   存储管理   │              │  │
│  │  │ (CPU)       │    │ (Memory)    │    │ (Storage)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      系统底层层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   内核      │    │   文件系统   │   │   网络栈     │              │  │
│  │  │ (Kernel)    │    │ (Filesystem)│   │ (Network)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. kubelet故障排查 (kubelet Troubleshooting)

### 2.1 kubelet状态检查

```bash
# ========== 1. 基础状态验证 ==========
# 检查kubelet服务状态
systemctl status kubelet

# 查看kubelet进程
ps aux | grep kubelet

# 检查kubelet版本
kubelet --version

# ========== 2. 配置文件检查 ==========
# 查看kubelet配置
cat /var/lib/kubelet/config.yaml

# 检查kubelet启动参数
ps aux | grep kubelet | grep -v grep

# 验证证书文件
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -text -noout

# ========== 3. 节点状态分析 ==========
# 查看节点详细状态
kubectl describe node <node-name>

# 检查节点条件
kubectl get node <node-name> -o jsonpath='{.status.conditions}'

# 分析节点事件
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp'
```

### 2.2 kubelet日志分析

```bash
# ========== 日志收集和分析 ==========
# 查看kubelet系统日志
journalctl -u kubelet --since "1 hour ago"

# 实时监控kubelet日志
journalctl -u kubelet -f

# 过滤错误日志
journalctl -u kubelet --since "1 hour ago" | grep -i "error\|failed\|warning"

# 分析特定时间段日志
journalctl -u kubelet --since "2026-02-04 10:00:00" --until "2026-02-04 11:00:00"

# ========== 常见错误模式识别 ==========
# 证书相关错误
journalctl -u kubelet | grep -i "certificate\|tls\|x509"

# 资源相关错误
journalctl -u kubelet | grep -i "eviction\|pressure\|resource"

# 网络相关错误
journalctl -u kubelet | grep -i "network\|connection\|timeout"

# 容器运行时错误
journalctl -u kubelet | grep -i "cri\|container\|docker\|containerd"
```

### 2.3 kubelet性能监控

```bash
# ========== 性能指标收集 ==========
# 监控kubelet资源使用
top -p $(pgrep kubelet)

# 查看kubelet内存使用
cat /proc/$(pgrep kubelet)/status | grep -E "VmRSS|VmSize"

# 监控kubelet文件描述符
ls -l /proc/$(pgrep kubelet)/fd | wc -l

# ========== 性能瓶颈诊断 ==========
# 检查kubelet goroutine数量
curl -s http://localhost:10248/debug/pprof/goroutine?debug=2 | head -20

# 分析kubelet堆栈信息
curl -s http://localhost:10248/debug/pprof/heap > /tmp/kubelet.heap
go tool pprof /tmp/kubelet.heap

# 监控kubelet API响应时间
while true; do
    time curl -k https://localhost:10250/healthz
    sleep 5
done
```

---

## 3. 容器运行时故障排查 (Container Runtime Troubleshooting)

### 3.1 容器运行时状态检查

```bash
# ========== 1. 运行时基础检查 ==========
# 检查containerd状态
systemctl status containerd
crictl info

# 检查Docker状态
systemctl status docker
docker info

# 查看运行时版本
containerd --version
docker --version

# ========== 2. 镜像管理检查 ==========
# 查看本地镜像
crictl images
docker images

# 检查镜像拉取历史
crictl inspecti <image-name>
docker inspect <image-name>

# 验证镜像完整性
docker image ls --digests

# ========== 3. 容器状态分析 ==========
# 查看运行中的容器
crictl ps
docker ps

# 检查容器详细信息
crictl inspect <container-id>
docker inspect <container-id>

# 分析容器日志
crictl logs <container-id>
docker logs <container-id>
```

### 3.2 容器运行时性能问题

```bash
# ========== 性能监控 ==========
# 监控容器运行时资源使用
systemctl status containerd --no-pager
systemctl status docker --no-pager

# 检查运行时进程资源
ps aux | grep -E "containerd|dockerd" | grep -v grep

# 监控容器创建延迟
time crictl run --rm -it alpine:latest echo "test"

# ========== 存储和文件系统检查 ==========
# 检查容器存储使用
df -h /var/lib/containerd
df -h /var/lib/docker

# 分析容器日志文件大小
du -sh /var/lib/containerd/io.containerd.grpc.v1.cri/containers/*/
du -sh /var/lib/docker/containers/*/

# 检查inode使用情况
df -i /var/lib/containerd
df -i /var/lib/docker

# ========== 网络性能测试 ==========
# 测试容器网络连通性
docker run --rm -it alpine:latest ping -c 3 8.8.8.8
crictl exec <pod-id> ping -c 3 8.8.8.8

# 验证DNS解析
docker run --rm -it busybox nslookup kubernetes.default
crictl exec <pod-id> nslookup kubernetes.default
```

---

## 4. kube-proxy故障排查 (kube-proxy Troubleshooting)

### 4.1 kube-proxy状态验证

```bash
# ========== 1. 基础组件检查 ==========
# 检查kube-proxy Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# 查看kube-proxy详细信息
kubectl describe pods -n kube-system -l k8s-app=kube-proxy

# 检查kube-proxy配置
kubectl get configmap -n kube-system kube-proxy -o yaml

# 验证kube-proxy版本
kubectl exec -n kube-system -l k8s-app=kube-proxy -- kube-proxy --version

# ========== 2. 网络规则检查 ==========
# 检查iptables规则
iptables-save | grep -E "KUBE-SERVICES|KUBE-NODEPORTS"

# 查看ipvs规则
ipvsadm -Ln

# 验证服务规则同步
kubectl get services --all-namespaces | wc -l
iptables -t nat -L KUBE-SERVICES -n | wc -l

# ========== 3. 连接性测试 ==========
# 测试Service访问
kubectl run debug-pod --image=busybox --rm -it -- sh
# 在Pod内执行:
# wget -qO- http://<service-ip>:<port>

# 验证NodePort访问
curl http://<node-ip>:<node-port>
```

### 4.2 kube-proxy配置问题

```bash
# ========== 配置验证 ==========
# 检查kube-proxy模式
kubectl get configmap -n kube-system kube-proxy -o jsonpath='{.data.config\.conf}' | grep mode

# 验证集群CIDR配置
kubectl get configmap -n kube-system kube-proxy -o jsonpath='{.data.config\.conf}' | grep clusterCIDR

# 检查健康检查配置
kubectl get configmap -n kube-system kube-proxy -o jsonpath='{.data.config\.conf}' | grep healthz

# ========== 规则同步问题 ==========
# 手动刷新iptables规则
kubectl delete pod -n kube-system -l k8s-app=kube-proxy

# 验证规则重建
sleep 30
iptables -t nat -L KUBE-SERVICES -n | head -10

# 检查规则同步日志
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100 | grep -i "sync\|update"
```

---

## 5. 节点资源压力故障排查 (Node Resource Pressure Troubleshooting)

### 5.1 资源使用监控

```bash
# ========== 实时资源监控 ==========
# 监控节点资源使用
watch -n 5 'kubectl top nodes'

# 查看节点详细资源
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 检查系统资源使用
top -b -n 1 | head -20
free -h
df -h

# ========== 资源压力分析 ==========
# 检查节点压力条件
kubectl get node <node-name> -o jsonpath='{
    .status.conditions[?(@.type=="MemoryPressure")].status
}{
        "\t"
}{
        .status.conditions[?(@.type=="DiskPressure")].status
}{
        "\t"
}{
        .status.conditions[?(@.type=="PIDPressure")].status
}'

# 分析驱逐事件
kubectl get events --field-selector involvedObject.kind=Node --sort-by='.lastTimestamp' | grep -i evict

# 查看Pod驱逐历史
kubectl get events --all-namespaces --field-selector reason=Evicted --sort-by='.lastTimestamp'
```

### 5.2 资源优化策略

```bash
# ========== 资源预留配置 ==========
# 检查系统预留配置
cat /var/lib/kubelet/config.yaml | grep -A 5 "systemReserved\|kubeReserved"

# 验证资源预留效果
kubectl describe node <node-name> | grep -A 5 "Allocated resources"

# ========== 驱逐阈值调整 ==========
# 查看当前驱逐配置
cat /var/lib/kubelet/config.yaml | grep -A 10 "eviction"

# 调整内存驱逐阈值
cat <<EOF > kubelet-eviction-config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
  imagefs.inodesFree: "10%"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "20%"
  imagefs.inodesFree: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
  nodefs.inodesFree: "1m30s"
  imagefs.available: "1m30s"
  imagefs.inodesFree: "1m30s"
EOF

# ========== 资源清理脚本 ==========
cat <<'EOF' > node-resource-cleaner.sh
#!/bin/bash

NODE_NAME=$1

if [ -z "$NODE_NAME" ]; then
    echo "Usage: $0 <node-name>"
    exit 1
fi

echo "Cleaning resources on node: $NODE_NAME"

# 清理已完成的Pod
kubectl delete pods --field-selector=status.phase==Succeeded -A

# 清理Evicted状态的Pod
kubectl delete pods --field-selector=status.phase==Failed -A

# 清理未使用的镜像
kubectl debug node/$NODE_NAME --image=busybox -it -- sh -c "
    crictl rmi --prune
    docker image prune -af
"

# 清理临时文件
kubectl debug node/$NODE_NAME --image=busybox -it -- sh -c "
    find /tmp -type f -mtime +7 -delete
    journalctl --vacuum-time=7d
"

echo "Resource cleanup completed on node: $NODE_NAME"
EOF

chmod +x node-resource-cleaner.sh
```

---

## 6. 证书和认证故障排查 (Certificate and Authentication Troubleshooting)

### 6.1 证书状态检查

```bash
# ========== 证书有效性验证 ==========
# 检查kubelet证书
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -text -noout | grep -E "Subject:|Not After|X509v3 Subject Alternative Name"

# 验证证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -enddate

# 检查CA证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -text -noout | grep -E "Subject:|Issuer:"

# ========== 证书轮换状态 ==========
# 检查证书轮换配置
cat /var/lib/kubelet/config.yaml | grep -A 5 "rotateCertificates"

# 验证证书轮换功能
kubectl get csr -A | grep -E "Approved|Pending"

# 检查证书签名请求
kubectl get csr | grep kubelet
```

### 6.2 认证和授权问题

```bash
# ========== 节点认证检查 ==========
# 验证节点认证状态
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
}'

# 检查节点证书签名
kubectl certificate approve <csr-name>

# 验证RBAC权限
kubectl auth can-i get nodes --as system:node:<node-name>

# ========== 网络策略影响 ==========
# 检查网络策略对节点的影响
kubectl get networkpolicy -A

# 验证节点网络连通性
kubectl run network-test --image=busybox --rm -it -- sh -c "
    ping -c 3 <api-server-ip>
    telnet <api-server-ip> 6443
"
```

---

## 7. 监控和告警配置 (Monitoring and Alerting)

### 7.1 节点组件监控指标

```bash
# ========== 监控配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: node-components-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kubelet
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'kubelet_.*'
      action: keep
---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: container-runtime-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: container-runtime
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
  name: node-components-alerts
  namespace: monitoring
spec:
  groups:
  - name: node.rules
    rules:
    - alert: KubeletDown
      expr: up{job="kubelet"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Kubelet is down on node {{ \$labels.node }}"
        
    - alert: ContainerRuntimeDown
      expr: up{job="container-runtime"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Container runtime is down on node {{ \$labels.node }}"
        
    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ \$labels.node }} is under memory pressure"
        
    - alert: NodeDiskPressure
      expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ \$labels.node }} is under disk pressure"
        
    - alert: KubeletClientCertificateExpiration
      expr: kubelet_certificate_expiration_seconds < 86400 * 7
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Kubelet client certificate expires in {{ \$value | humanizeDuration }} on node {{ \$labels.node }}"
        
    - alert: HighContainerRestarts
      expr: rate(kube_pod_container_status_restarts_total[5m]) > 5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "High container restart rate ({{ \$value }}/sec) on node {{ \$labels.node }}"
EOF
```

### 7.2 节点健康检查工具

```bash
# ========== 自动化健康检查 ==========
cat <<'EOF' > node-health-check.sh
#!/bin/bash

NODE_NAME=${1:-all}
CHECK_INTERVAL=${2:-300}  # 5分钟检查间隔

echo "Starting node health check for: $NODE_NAME"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    if [ "$NODE_NAME" = "all" ]; then
        NODES=$(kubectl get nodes -o jsonpath='{.items[*].metadata.name}')
    else
        NODES=$NODE_NAME
    fi
    
    for node in $NODES; do
        echo "$TIMESTAMP - Checking node: $node"
        
        # 检查节点状态
        NODE_STATUS=$(kubectl get node $node -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
        if [ "$NODE_STATUS" != "True" ]; then
            echo "$TIMESTAMP - ⚠️  Node $node is not ready (Status: $NODE_STATUS)"
        else
            echo "$TIMESTAMP - ✓ Node $node is ready"
        fi
        
        # 检查资源压力
        MEMORY_PRESSURE=$(kubectl get node $node -o jsonpath='{.status.conditions[?(@.type=="MemoryPressure")].status}')
        DISK_PRESSURE=$(kubectl get node $node -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}')
        
        if [ "$MEMORY_PRESSURE" = "True" ]; then
            echo "$TIMESTAMP - ⚠️  Node $node has memory pressure"
        fi
        
        if [ "$DISK_PRESSURE" = "True" ]; then
            echo "$TIMESTAMP - ⚠️  Node $node has disk pressure"
        fi
        
        # 检查Pod驱逐
        EVICTED_PODS=$(kubectl get pods --field-selector spec.nodeName=$node,status.phase=Failed -o jsonpath='{.items[*].metadata.name}' | wc -w)
        if [ $EVICTED_PODS -gt 0 ]; then
            echo "$TIMESTAMP - ⚠️  Node $node has $EVICTED_PODS evicted pods"
        fi
    done
    
    echo "$TIMESTAMP - Health check cycle completed"
    echo "---"
    
    sleep $CHECK_INTERVAL
done
EOF

chmod +x node-health-check.sh

# ========== 节点性能基准测试 ==========
cat <<'EOF' > node-performance-benchmark.sh
#!/bin/bash

NODE_NAME=$1

if [ -z "$NODE_NAME" ]; then
    echo "Usage: $0 <node-name>"
    exit 1
fi

echo "Running performance benchmark on node: $NODE_NAME"

# 创建基准测试Pod
cat <<TESTPOD | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: perf-benchmark-$NODE_NAME
  namespace: default
spec:
  nodeName: $NODE_NAME
  containers:
  - name: benchmark
    image: busybox
    command: ["sleep", "3600"]
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "500m"
        memory: "512Mi"
TESTPOD

# 等待Pod就绪
sleep 30

# 执行性能测试
echo "=== CPU Performance Test ==="
kubectl exec perf-benchmark-$NODE_NAME -- time dd if=/dev/zero of=/dev/null bs=1M count=1000

echo "=== Memory Performance Test ==="
kubectl exec perf-benchmark-$NODE_NAME -- time dd if=/dev/zero of=/tmp/testfile bs=1M count=500
kubectl exec perf-benchmark-$NODE_NAME -- time dd if=/tmp/testfile of=/dev/null bs=1M

echo "=== Network Performance Test ==="
kubectl exec perf-benchmark-$NODE_NAME -- ping -c 10 8.8.8.8

echo "=== Disk I/O Test ==="
kubectl exec perf-benchmark-$NODE_NAME -- time dd if=/dev/zero of=/tmp/disktest bs=1M count=100 conv=fdatasync

# 清理测试资源
kubectl delete pod perf-benchmark-$NODE_NAME

echo "Performance benchmark completed for node: $NODE_NAME"
EOF

chmod +x node-performance-benchmark.sh
```

---