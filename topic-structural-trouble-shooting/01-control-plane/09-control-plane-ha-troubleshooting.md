# 控制平面高可用故障处理指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 生产环境高可用保障

## 🚨 高可用故障现象与影响分析

### 常见高可用故障现象

| 故障现象 | 典型表现 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| 控制平面节点宕机 | `control plane node NotReady` | ⭐⭐⭐ 高 | P0 |
| etcd 集群脑裂 | `etcd cluster is unhealthy` | ⭐⭐⭐ 高 | P0 |
| API Server 负载不均衡 | `some apiservers not responding` | ⭐⭐ 中 | P1 |
| Leader 选举频繁切换 | `leader election churning` | ⭐⭐ 中 | P1 |
| 控制平面组件启动失败 | `control plane components crashlooping` | ⭐⭐⭐ 高 | P0 |
| 证书不一致导致认证失败 | `certificate signed by unknown authority` | ⭐⭐⭐ 高 | P0 |

### 故障影响范围评估

```bash
# 快速评估控制平面健康状态
kubectl get nodes -l node-role.kubernetes.io/control-plane
kubectl get pods -n kube-system -l tier=control-plane
kubectl get componentstatuses

# 检查 etcd 集群状态
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health

# 检查控制平面组件日志
for component in kube-apiserver kube-controller-manager kube-scheduler; do
  echo "=== $component logs ==="
  kubectl logs -n kube-system -l component=$component --tail=50
done
```

## 🔍 高可用故障诊断方法

### 诊断原理说明

控制平面高可用故障通常涉及以下关键组件的协同工作：

1. **etcd 集群**：分布式键值存储，需要多数派节点存活
2. **API Server**：无状态组件，可通过负载均衡器分发请求
3. **Controller Manager**：通过 Leader 选举确保单一实例运行
4. **Scheduler**：通过 Leader 选举确保调度一致性
5. **网络连通性**：控制平面节点间的网络通信

### 故障诊断决策树

```
控制平面故障
    ├── etcd 集群状态检查
    │   ├── 集群健康状态
    │   ├── 节点成员关系
    │   ├── 数据一致性
    │   └── 网络分区检测
    ├── API Server 可用性检查
    │   ├── 负载均衡器状态
    │   ├── 证书有效性
    │   ├── 后端节点连通性
    │   └── 请求处理能力
    ├── Leader 选举状态检查
    │   ├── Controller Manager
    │   ├── Scheduler
    │   ├── Lease 对象状态
    │   └── 选举频率分析
    └── 组件启动状态检查
        ├── Pod 状态分析
        ├── 启动日志检查
        ├── 资源依赖验证
        └── 配置文件校验
```

### 详细诊断命令

#### 1. etcd 集群故障诊断

```bash
#!/bin/bash
# etcd 集群高可用诊断脚本

echo "=== etcd 集群高可用诊断 ==="

# 1. 集群成员检查
echo "1. etcd 集群成员状态:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member list -w table

# 2. 集群健康检查
echo "2. etcd 集群健康检查:"
for endpoint in $(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[*].status.podIP}'); do
  echo "Checking endpoint: https://$endpoint:2379"
  kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://$endpoint:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health
done

# 3. 集群状态详细信息
echo "3. etcd 集群详细状态:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint status -w table

# 4. 检查是否有告警
echo "4. etcd 告警检查:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt alarm list

# 5. 网络连通性检查
echo "5. etcd 节点网络连通性:"
for pod in $(kubectl get pods -n kube-system -l component=etcd -o name); do
  echo "Testing connectivity from $pod:"
  kubectl exec -n kube-system $pod -- ping -c 3 $(kubectl get pod -n kube-system -l component=etcd -o jsonpath='{.items[1].status.podIP}')
done
```

#### 2. API Server 高可用诊断

```bash
#!/bin/bash
# API Server 高可用诊断脚本

echo "=== API Server 高可用诊断 ==="

# 1. 负载均衡器检查
echo "1. 负载均衡器后端状态:"
LB_ENDPOINT=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}' | sed 's/https:\/\///' | cut -d':' -f1)
if command -v curl &> /dev/null; then
  for i in {1..10}; do
    curl -k https://$LB_ENDPOINT:6443/healthz -w "%{http_code} %{time_total}s\n" -o /dev/null -s
  done
fi

# 2. 各 API Server 实例状态
echo "2. 各 API Server 实例状态:"
kubectl get pods -n kube-system -l component=kube-apiserver -o wide

# 3. API Server 证书检查
echo "3. API Server 证书状态:"
for pod in $(kubectl get pods -n kube-system -l component=kube-apiserver -o name); do
  echo "Checking $pod:"
  kubectl exec -n kube-system $pod -- openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -subject
done

# 4. 请求分发均匀性检查
echo "4. API Server 请求分发检查:"
kubectl get --raw /metrics | grep apiserver_request_total | head -10

# 5. 连接数分布检查
echo "5. API Server 连接数分布:"
for pod in $(kubectl get pods -n kube-system -l component=kube-apiserver -o name); do
  echo "$pod connections:"
  kubectl exec -n kube-system $pod -- netstat -an | grep :6443 | grep ESTABLISHED | wc -l
done
```

#### 3. Leader 选举状态诊断

```bash
#!/bin/bash
# Leader 选举状态诊断脚本

echo "=== Leader 选举状态诊断 ==="

# 1. Controller Manager Leader 检查
echo "1. Controller Manager Leader 状态:"
kubectl get leases -n kube-system | grep kube-controller-manager

# 2. Scheduler Leader 检查
echo "2. Scheduler Leader 状态:"
kubectl get leases -n kube-system | grep kube-scheduler

# 3. Lease 对象详细信息
echo "3. Lease 对象详情:"
kubectl get leases -n kube-system -o yaml

# 4. Leader 选举日志分析
echo "4. Controller Manager Leader 选举日志:"
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100 | grep -i "leader\|election"

# 5. 频繁切换检测
echo "5. 近期 Leader 切换统计:"
kubectl get events -n kube-system --field-selector reason=LeaderElection | tail -20
```

## 🔧 高可用故障解决方案

### etcd 集群故障恢复

#### 方案一：单节点故障恢复

```bash
#!/bin/bash
# etcd 单节点故障恢复脚本

FAILED_NODE="control-plane-03"  # 故障节点名称
CLUSTER_MEMBERS=("control-plane-01" "control-plane-02" "control-plane-03")

echo "=== etcd 单节点故障恢复 ==="

# 1. 移除故障节点
echo "1. 移除故障节点 $FAILED_NODE:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member remove $(kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member list | grep $FAILED_NODE | cut -d',' -f1)

# 2. 清理故障节点数据
echo "2. 清理故障节点数据:"
ssh $FAILED_NODE "sudo rm -rf /var/lib/etcd/member"

# 3. 重新加入集群
echo "3. 重新加入集群:"
ssh $FAILED_NODE "sudo kubeadm join-phase control-plane-join etcd --config /etc/kubernetes/kubeadm-config.yaml"

# 4. 验证集群状态
echo "4. 验证集群恢复状态:"
kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member list
```

#### 方案二：集群脑裂恢复

```bash
#!/bin/bash
# etcd 集群脑裂恢复脚本

echo "=== etcd 集群脑裂恢复 ==="

# 1. 识别多数派集群
echo "1. 识别多数派集群成员:"
MAJORITY_MEMBERS=()
for node in control-plane-01 control-plane-02 control-plane-03; do
  if ssh $node "ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health" &>/dev/null; then
    MAJORITY_MEMBERS+=($node)
  fi
done

echo "多数派成员: ${MAJORITY_MEMBERS[@]}"

# 2. 从多数派恢复集群
echo "2. 从多数派恢复集群:"
PRIMARY_NODE=${MAJORITY_MEMBERS[0]}
ssh $PRIMARY_NODE "sudo systemctl stop etcd"

# 备份当前数据
ssh $PRIMARY_NODE "sudo cp -r /var/lib/etcd /var/lib/etcd.backup.$(date +%Y%m%d_%H%M%S)"

# 重建集群
ssh $PRIMARY_NODE "sudo rm -rf /var/lib/etcd/member"
ssh $PRIMARY_NODE "sudo kubeadm init phase etcd local --config /etc/kubernetes/kubeadm-config.yaml"

# 3. 逐步添加其他节点
for node in "${MAJORITY_MEMBERS[@]:1}"; do
  echo "添加节点 $node 到集群:"
  ssh $node "sudo systemctl stop etcd"
  ssh $node "sudo rm -rf /var/lib/etcd/member"
  ssh $node "sudo kubeadm join-phase control-plane-join etcd --config /etc/kubernetes/kubeadm-config.yaml"
done

# 4. 验证集群状态
echo "4. 验证集群恢复:"
ssh $PRIMARY_NODE "ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member list"
```

### API Server 高可用优化

#### 方案一：负载均衡器配置优化

```yaml
# HAProxy 高可用配置示例
global
    log         127.0.0.1 local2
    chroot      /var/lib/haproxy
    pidfile     /var/run/haproxy.pid
    maxconn     4000
    user        haproxy
    group       haproxy
    daemon

defaults
    mode                    tcp
    log                     global
    option                  tcplog
    option                  dontlognull
    option                  redispatch
    retries                 3
    timeout queue           1m
    timeout connect         10s
    timeout client          1m
    timeout server          1m
    timeout check           10s
    maxconn                 3000

frontend kubernetes-frontend
    bind *:6443
    mode tcp
    option tcplog
    default_backend kubernetes-backend

backend kubernetes-backend
    mode tcp
    balance roundrobin
    option httpchk GET /healthz
    http-check expect status 200
    server control-plane-01 192.168.1.11:6443 check fall 3 rise 2
    server control-plane-02 192.168.1.12:6443 check fall 3 rise 2
    server control-plane-03 192.168.1.13:6443 check fall 3 rise 2
```

#### 方案二：API Server 健康检查优化

```yaml
# 优化的 API Server 健康检查配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.32.0
    livenessProbe:
      httpGet:
        host: 127.0.0.1
        path: /livez
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 15
      periodSeconds: 10
      timeoutSeconds: 15
      failureThreshold: 8
    readinessProbe:
      httpGet:
        host: 127.0.0.1
        path: /readyz
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 15
      failureThreshold: 3
```

### Leader 选举优化

#### 方案一：调整选举参数

```yaml
# Controller Manager 选举参数优化
apiVersion: v1
kind: Pod
metadata:
  name: kube-controller-manager
spec:
  containers:
  - name: kube-controller-manager
    image: registry.k8s.io/kube-controller-manager:v1.32.0
    command:
    - kube-controller-manager
    # 选举参数优化
    - --leader-elect=true
    - --leader-elect-lease-duration=15s
    - --leader-elect-renew-deadline=10s
    - --leader-elect-retry-period=2s
    - --leader-elect-resource-lock=leases
    - --leader-elect-resource-name=kube-controller-manager
    - --leader-elect-resource-namespace=kube-system

---
# Scheduler 选举参数优化
apiVersion: v1
kind: Pod
metadata:
  name: kube-scheduler
spec:
  containers:
  - name: kube-scheduler
    image: registry.k8s.io/kube-scheduler:v1.32.0
    command:
    - kube-scheduler
    # 选举参数优化
    - --leader-elect=true
    - --leader-elect-lease-duration=15s
    - --leader-elect-renew-deadline=10s
    - --leader-elect-retry-period=2s
    - --leader-elect-resource-lock=leases
```

#### 方案二：选举稳定性监控

```bash
#!/bin/bash
# Leader 选举稳定性监控脚本

MONITOR_LOG="/var/log/kubernetes/leader-election-monitor.log"

{
  echo "=== Leader 选举监控报告 $(date) ==="
  
  # Controller Manager Leader 状态
  CM_LEADER=$(kubectl get leases -n kube-system kube-controller-manager -o jsonpath='{.spec.holderIdentity}')
  echo "Controller Manager Leader: $CM_LEADER"
  
  # Scheduler Leader 状态
  SCHEDULER_LEADER=$(kubectl get leases -n kube-system kube-scheduler -o jsonpath='{.spec.holderIdentity}')
  echo "Scheduler Leader: $SCHEDULER_LEADER"
  
  # 检查近期选举事件
  RECENT_ELECTIONS=$(kubectl get events -n kube-system --field-selector reason=LeaderElection --sort-by=.lastTimestamp | tail -10)
  if [ -n "$RECENT_ELECTIONS" ]; then
    echo "近期选举事件:"
    echo "$RECENT_ELECTIONS"
  fi
  
  # 检查 Lease 更新时间
  CM_LAST_RENEW=$(kubectl get leases -n kube-system kube-controller-manager -o jsonpath='{.spec.renewTime}')
  SCHEDULER_LAST_RENEW=$(kubectl get leases -n kube-system kube-scheduler -o jsonpath='{.spec.renewTime}')
  echo "Controller Manager 最后续约时间: $CM_LAST_RENEW"
  echo "Scheduler 最后续约时间: $SCHEDULER_LAST_RENEW"
  
} >> "$MONITOR_LOG"
```

## ⚠️ 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| etcd 节点移除 | ⭐⭐⭐ 高 | 可能导致数据不一致 | 从备份恢复集群 |
| 集群脑裂恢复 | ⭐⭐⭐ 高 | 数据丢失风险 | 使用最近备份恢复 |
| 负载均衡器配置变更 | ⭐⭐ 中 | 可能影响 API 访问 | 恢复原配置文件 |
| 选举参数调整 | ⭐⭐ 中 | 可能影响选举频率 | 恢复默认参数值 |

## 📊 高可用验证与监控

### 高可用验证脚本

```bash
#!/bin/bash
# 高可用配置验证脚本

echo "=== 控制平面高可用验证 ==="

# 1. etcd 集群验证
echo "1. etcd 集群验证:"
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
ETCD_HEALTH=$(kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt endpoint health)
if echo "$ETCD_HEALTH" | grep -q "is healthy"; then
  echo "✓ etcd 集群健康"
else
  echo "✗ etcd 集群存在问题"
fi

# 2. API Server 高可用验证
echo "2. API Server 高可用验证:"
API_SERVER_COUNT=$(kubectl get pods -n kube-system -l component=kube-apiserver | grep -c Running)
if [ $API_SERVER_COUNT -ge 2 ]; then
  echo "✓ API Server 高可用 (运行实例: $API_SERVER_COUNT)"
else
  echo "✗ API Server 实例不足"
fi

# 3. Leader 选举验证
echo "3. Leader 选举验证:"
CM_LEADER=$(kubectl get leases -n kube-system kube-controller-manager -o jsonpath='{.spec.holderIdentity}' 2>/dev/null)
SCHEDULER_LEADER=$(kubectl get leases -n kube-system kube-scheduler -o jsonpath='{.spec.holderIdentity}' 2>/dev/null)

if [ -n "$CM_LEADER" ] && [ -n "$SCHEDULER_LEADER" ]; then
  echo "✓ Controller Manager Leader: $CM_LEADER"
  echo "✓ Scheduler Leader: $SCHEDULER_LEADER"
else
  echo "✗ Leader 选举存在问题"
fi

# 4. 组件健康状态验证
echo "4. 组件健康状态验证:"
kubectl get componentstatuses
```

### 高可用监控告警配置

```yaml
# Prometheus 高可用告警规则
groups:
- name: kubernetes.high-availability
  rules:
  - alert: EtcdClusterDegraded
    expr: count(etcd_server_has_leader == 0) > 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "etcd 集群降级"
      description: "{{ $value }} 个 etcd 节点失去 leader"

  - alert: ControlPlaneNodeDown
    expr: kube_node_status_condition{condition="Ready",status="false"} == 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "控制平面节点宕机"
      description: "控制平面节点 {{ $labels.node }} 不可用"

  - alert: APIServerUnavailable
    expr: up{job="kubernetes-apiservers"} == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "API Server 不可用"
      description: "API Server 实例不可访问"

  - alert: FrequentLeaderElection
    expr: rate(kube_leader_election_status[5m]) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "频繁的 Leader 选举"
      description: "Leader 选举过于频繁，可能影响集群稳定性"

  - alert: InsufficientControlPlaneReplicas
    expr: count(kube_pod_status_ready{namespace="kube-system",pod=~"kube-apiserver-.+"} == 1) < 2
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "控制平面副本不足"
      description: "可用的控制平面实例少于2个"
```

## 📚 高可用最佳实践

### 高可用架构设计

```yaml
# 推荐的高可用架构配置
highAvailability:
  etcd:
    replicas: 3  # 奇数个节点
    storage: local-ssd
    backup: automated-hourly
    
  apiServer:
    replicas: 3
    loadBalancer: external-hardware
    healthCheck: /readyz
    
  controllerManager:
    replicas: 3
    leaderElection: enabled
    leaseDuration: "15s"
    
  scheduler:
    replicas: 3
    leaderElection: enabled
    leaseDuration: "15s"
    
  networking:
    controlPlaneIsolation: true
    dedicatedNetworkSegment: "192.168.10.0/24"
```

### 定期高可用检查

```bash
#!/bin/bash
# 定期高可用健康检查脚本

HEALTH_CHECK_LOG="/var/log/kubernetes/ha-health-check-$(date +%Y%m%d).log"

{
  echo "=== 高可用健康检查报告 $(date) ==="
  
  # 1. 基础组件检查
  echo "1. 基础组件状态:"
  kubectl get componentstatuses -o wide
  
  # 2. 控制平面节点检查
  echo "2. 控制平面节点状态:"
  kubectl get nodes -l node-role.kubernetes.io/control-plane -o wide
  
  # 3. etcd 集群检查
  echo "3. etcd 集群状态:"
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o name | head -1)
  kubectl exec -n kube-system $ETCD_POD -- ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key --cacert=/etc/kubernetes/pki/etcd/ca.crt member list
  
  # 4. Leader 状态检查
  echo "4. Leader 选举状态:"
  kubectl get leases -n kube-system
  
  # 5. 负载均衡检查
  echo "5. 负载均衡器状态:"
  LB_ENDPOINT=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}' | sed 's/https:\/\///' | cut -d':' -f1)
  for i in {1..5}; do
    curl -k https://$LB_ENDPOINT:6443/healthz -w "%{http_code}\n" -o /dev/null -s
  done
  
} >> "$HEALTH_CHECK_LOG"
```

## 🔄 典型高可用故障案例

### 案例一：网络分区导致 etcd 脑裂

**问题描述**：由于网络故障，etcd 集群被分割成两个独立的分区，各自选出不同的 leader。

**根本原因**：网络 ACL 配置错误，控制平面节点间网络通信中断。

**解决方案**：
1. 立即隔离故障分区，确保只有一个合法集群运行
2. 修复网络 ACL 配置，恢复节点间通信
3. 从多数派集群恢复数据一致性
4. 重新加入分离的节点

### 案例二：证书不一致导致 API Server 故障

**问题描述**：控制平面节点上的 API Server 证书不一致，导致部分节点无法正常服务。

**根本原因**：手动更新证书时操作不一致，或者自动轮换机制配置错误。

**解决方案**：
1. 统一重新生成所有控制平面证书
2. 确保所有节点使用相同的 CA 证书
3. 重启所有 API Server 实例
4. 验证证书一致性和服务可用性

## 📞 高可用支持

**紧急故障响应**：
- 立即启动备用控制平面
- 联系云服务商技术支持
- 执行预先准备的灾难恢复预案

**专业服务**：
- CNCF 认证的 Kubernetes 托管服务
- 企业级高可用架构咨询服务
- 7×24 小时技术支持热线