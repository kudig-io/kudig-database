# 控制平面扩缩容指南 (Control Plane Scalability Guide)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 扩展性规划指南

---

## 目录

1. [扩缩容设计原则](#1-扩缩容设计原则)
2. [水平扩展策略](#2-水平扩展策略)
3. [垂直扩展优化](#3-垂直扩展优化)
4. [etcd扩缩容管理](#4-etcd扩缩容管理)
5. [API Server扩缩容](#5-api-server扩缩容)
6. [控制器管理器扩展](#6-控制器管理器扩展)
7. [调度器扩展机制](#7-调度器扩展机制)
8. [自动扩缩容配置](#8-自动扩缩容配置)
9. [性能容量规划](#9-性能容量规划)
10. [扩缩容最佳实践](#10-扩缩容最佳实践)

---

## 1. 扩缩容设计原则

### 1.1 扩展性三角权衡

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Scalability Trade-offs Triangle                         │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  扩展性维度分析:                                                                 │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           扩展性三角 (Scalability Triangle)                │    │
│  │  ┌─────────────────────────────────────────────────────────────────────┐  │    │
│  │  │                                                                     │  │    │
│  │  │        Consistency (一致性)                                         │  │    │
│  │  │              /\                                                    │  │    │
│  │  │             /  \                                                   │  │    │
│  │  │            /    \                                                  │  │    │
│  │  │           /      \                                                 │  │    │
│  │  │          /        \                                                │  │    │
│  │  │         /          \                                               │  │    │
│  │  │        /            \                                              │  │    │
│  │  │       /              \                                             │  │    │
│  │  │      /                \                                            │  │    │
│  │  │     /                  \                                           │  │    │
│  │  │    /                    \                                          │  │    │
│  │  │   /                      \                                         │  │    │
│  │  │  /                        \                                        │  │    │
│  │  │ /                          \                                       │  │    │
│  │  │/____________________________\                                      │  │    │
│  │  │ Availability (可用性)         Performance (性能)                   │  │    │
│  │  └─────────────────────────────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  权衡策略:                                                                       │
│  ├── 高一致性 + 高可用性 → 性能降低 (强一致性)                                   │
│  ├── 高性能 + 高可用性 → 一致性降低 (最终一致性)                                 │
│  └── 高性能 + 高一致性 → 可用性降低 (分区容忍)                                   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 扩展性指标体系

| 指标类别 | 具体指标 | 基准值 | 扩展目标 |
|----------|----------|--------|----------|
| **节点规模** | 支持节点数 | 1000节点 | 5000+节点 |
| **Pod规模** | 支持Pod数 | 15万Pod | 150万Pod |
| **API吞吐** | QPS | 1000 QPS | 10000+ QPS |
| **响应延迟** | P99延迟 | <100ms | <50ms |
| **资源利用率** | CPU使用率 | <70% | <80% |
| **存储性能** | etcd延迟 | <10ms | <5ms |

### 1.3 扩展性设计模式

```yaml
# 扩展性架构模式对比
scalability_patterns:
  sharding:
    description: "分片模式 - 按资源类型或命名空间分片"
    适用场景: "超大规模集群 (>5000节点)"
    优势: "线性扩展，隔离故障域"
    劣势: "复杂度高，跨分片协调困难"
    
  federation:
    description: "联邦模式 - 多集群联邦管理"
    适用场景: "地理分布式部署"
    优势: "地理位置隔离，独立扩展"
    劣势: "跨集群同步复杂，延迟较高"
    
  hierarchical:
    description: "分层模式 - 控制平面分层部署"
    适用场景: "混合云环境"
    优势: "灵活部署，成本优化"
    劣势: "管理复杂，一致性挑战"
```

---

## 2. 水平扩展策略

### 2.1 组件水平扩展架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Horizontal Scaling Architecture                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  控制平面水平扩展拓扑:                                                           │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Load Balancer                                 │    │
│  │                    (VIP: apiserver.cluster.local)                       │    │
│  └─────────────────────────────────┬───────────────────────────────────────┘    │
│                                    │                                             │
│          ┌─────────────────────────┼─────────────────────────┐                   │
│          │                         │                         │                    │
│          ▼                         ▼                         ▼                    │
│  ┌─────────────┐           ┌─────────────┐           ┌─────────────┐             │
│  │ API Server  │           │ API Server  │           │ API Server  │             │
│  │    (N1)     │           │    (N2)     │           │    (N3)     │             │
│  └─────────────┘           └─────────────┘           └─────────────┘             │
│          │                         │                         │                    │
│          ▼                         ▼                         ▼                    │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           etcd Cluster                                  │    │
│  │  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐               │    │
│  │  │   etcd-1    │     │   etcd-2    │     │   etcd-3    │               │    │
│  │  │  (Leader)   │◄───►│ (Follower)  │◄───►│ (Follower)  │               │    │
│  │  └─────────────┘     └─────────────┘     └─────────────┘               │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                    │                                             │
│          ┌─────────────────────────┼─────────────────────────┐                   │
│          │                         │                         │                    │
│          ▼                         ▼                         ▼                    │
│  ┌─────────────┐           ┌─────────────┐           ┌─────────────┐             │
│  │     KCM     │           │     KCM     │           │   Scheduler │             │
│  │  (Leader)   │           │ (Follower)  │           │  (Leader)   │             │
│  └─────────────┘           └─────────────┘           └─────────────┘             │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 水平扩展配置示例

```yaml
# API Server水平扩展配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver-horizontal-scale
  namespace: kube-system
spec:
  replicas: 3  # 根据负载调整副本数
  template:
    spec:
      containers:
      - name: kube-apiserver
        image: registry.k8s.io/kube-apiserver:v1.30.0
        args:
        # 基础配置
        - --advertise-address=$(POD_IP)
        - --allow-privileged=true
        - --authorization-mode=Node,RBAC
        
        # 水平扩展优化参数
        - --max-requests-inflight=2000          # 增加并发处理能力
        - --max-mutating-requests-inflight=1000 # 增加写操作并发
        - --default-watch-cache-size=2000       # 扩大Watch缓存
        - --watch-cache-sizes=                  # 按资源类型优化缓存
            pods#10000,
            nodes#2000,
            services#2000,
            endpoints#5000,
            configmaps#2000,
            secrets#2000
        
        # etcd优化配置
        - --etcd-servers=                       # 多etcd节点
            https://etcd-1:2379,
            https://etcd-2:2379,
            https://etcd-3:2379
        - --etcd-qps=100                        # etcd查询速率限制
        - --etcd-burst=200                      # etcd突发查询限制
        
        # 性能优化
        - --enable-priority-and-fairness=true   # 启用API优先级和公平性
        - --goaway-chance=0.001                 # 减少连接断开概率
        - --delete-collection-workers=8         # 增加集合删除工作线程
        
        ports:
        - containerPort: 6443
          name: https
          
        resources:
          requests:
            cpu: 2000m
            memory: 4Gi
          limits:
            cpu: 4000m
            memory: 8Gi
```

---

## 3. 垂直扩展优化

### 3.1 资源规格建议

| 集群规模 | API Server | KCM/Scheduler | etcd |
|----------|------------|---------------|------|
| 小型 (<100节点) | 2核4GB | 1核2GB | 2核8GB |
| 中型 (100-500节点) | 4核8GB | 2核4GB | 4核16GB |
| 大型 (500-1000节点) | 8核16GB | 4核8GB | 8核32GB |
| 超大型 (>1000节点) | 16核32GB | 8核16GB | 16核64GB |

### 3.2 垂直扩展配置优化

```bash
#!/bin/bash
# 垂直扩展优化脚本

# 1. 系统级优化
optimize_system() {
    # 内核参数调优
    cat >> /etc/sysctl.conf << EOF
# 网络优化
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 1200

# 文件句柄限制
fs.file-max = 2097152
fs.nr_open = 2097152

# 内存优化
vm.swappiness = 1
vm.dirty_ratio = 10
vm.dirty_background_ratio = 5
EOF

    sysctl -p
    
    # 文件句柄限制
    echo "* soft nofile 1048576" >> /etc/security/limits.conf
    echo "* hard nofile 1048576" >> /etc/security/limits.conf
}

# 2. etcd垂直扩展优化
optimize_etcd() {
    # etcd资源配置
    cat > /etc/systemd/system/etcd.service << EOF
[Unit]
Description=etcd
Documentation=https://github.com/coreos/etcd

[Service]
Type=notify
WorkingDirectory=/var/lib/etcd
Environment=ETCD_NAME=$(hostname)
Environment=ETCD_DATA_DIR=/var/lib/etcd
Environment=ETCD_WAL_DIR=/var/lib/etcd/wal

ExecStart=/usr/local/bin/etcd \\
  --name $(hostname) \\
  --data-dir /var/lib/etcd \\
  --wal-dir /var/lib/etcd/wal \\
  --listen-client-urls https://0.0.0.0:2379 \\
  --advertise-client-urls https://$(hostname -I | cut -d' ' -f1):2379 \\
  --listen-peer-urls https://0.0.0.0:2380 \\
  --initial-advertise-peer-urls https://$(hostname -I | cut -d' ' -f1):2380 \\
  --initial-cluster-state new \\
  --initial-cluster-token etcd-cluster-1 \\
  
  # 性能优化参数
  --quota-backend-bytes=8589934592 \\        # 8GB存储配额
  --max-request-bytes=10485760 \\            # 10MB最大请求
  --grpc-keepalive-min-time=5s \\            # gRPC保活
  --grpc-keepalive-interval=2h \\
  --grpc-keepalive-timeout=20s \\
  --snapshot-count=10000 \\                  # 快照频率
  --heartbeat-interval=100 \\                # 心跳间隔(ms)
  --election-timeout=1000 \\                 # 选举超时(ms)
  
  # 安全配置
  --client-cert-auth=true \\
  --peer-client-cert-auth=true \\
  --cert-file=/etc/etcd/ssl/etcd.crt \\
  --key-file=/etc/etcd/ssl/etcd.key \\
  --trusted-ca-file=/etc/etcd/ssl/ca.crt \\
  --peer-cert-file=/etc/etcd/ssl/etcd-peer.crt \\
  --peer-key-file=/etc/etcd/ssl/etcd-peer.key \\
  --peer-trusted-ca-file=/etc/etcd/ssl/ca.crt

Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl restart etcd
}

# 执行优化
optimize_system
optimize_etcd
```

---

## 4. etcd扩缩容管理

### 4.1 etcd集群扩缩容策略

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          etcd Scaling Strategies                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  扩容场景:                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Scale Up Process                              │    │
│  │                                                                          │    │
│  │  1. 集群评估                                                             │    │
│  │     ├── 当前负载分析                                                      │    │
│  │     ├── 性能瓶颈识别                                                      │    │
│  │     └── 扩容必要性判断                                                    │    │
│  │                                                                          │    │
│  │  2. 节点准备                                                             │    │
│  │     ├── 硬件资源配置 (推荐SSD/NVMe)                                       │    │
│  │     ├── 网络连通性测试                                                    │    │
│  │     └── 证书和配置准备                                                    │    │
│  │                                                                          │    │
│  │  3. 集群扩容                                                             │    │
│  │     ├── 添加新成员到集群                                                  │    │
│  │     ├── 等待数据同步完成                                                  │    │
│  │     └── 验证集群健康状态                                                  │    │
│  │                                                                          │    │
│  │  4. 负载验证                                                             │    │
│  │     ├── 性能基准测试                                                      │    │
│  │     ├── 监控指标验证                                                      │    │
│  │     └── 故障恢复测试                                                      │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  缩容场景:                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                          Scale Down Process                             │    │
│  │                                                                          │    │
│  │  1. 风险评估                                                             │    │
│  │     ├── 数据一致性检查                                                    │    │
│  │     ├── 集群健康状态确认                                                  │    │
│  │     └── 降级影响分析                                                      │    │
│  │                                                                          │    │
│  │  2. 节点迁移                                                             │    │
│  │     ├── 停止目标节点服务                                                  │    │
│  │     ├── 等待领导权转移                                                    │    │
│  │     └── 移除集群成员                                                      │    │
│  │                                                                          │    │
│  │  3. 验证确认                                                             │    │
│  │     ├── 集群状态检查                                                      │    │
│  │     ├── 性能影响评估                                                      │    │
│  │     └── 监控告警确认                                                      │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 etcd扩缩容操作手册

```bash
#!/bin/bash
# etcd扩缩容管理脚本

# etcd扩容函数
scale_up_etcd() {
    local new_member_ip=$1
    local new_member_name="etcd-$(date +%s)"
    
    echo "开始扩容etcd集群，新增节点: $new_member_ip"
    
    # 1. 在现有集群中添加新成员
    echo "步骤1: 添加新成员到集群..."
    etcdctl member add $new_member_name \
        --peer-urls=https://$new_member_ip:2380
    
    # 2. 在新节点上启动etcd
    echo "步骤2: 在新节点启动etcd..."
    ssh $new_member_ip "
        mkdir -p /var/lib/etcd
        cat > /etc/etcd/etcd.conf << EOF
ETCD_NAME=$new_member_name
ETCD_DATA_DIR=/var/lib/etcd
ETCD_WAL_DIR=/var/lib/etcd/wal
ETCD_LISTEN_PEER_URLS=https://0.0.0.0:2380
ETCD_LISTEN_CLIENT_URLS=https://0.0.0.0:2379
ETCD_INITIAL_ADVERTISE_PEER_URLS=https://$new_member_ip:2380
ETCD_ADVERTISE_CLIENT_URLS=https://$new_member_ip:2379
ETCD_INITIAL_CLUSTER_STATE=existing
EOF
        systemctl start etcd
    "
    
    # 3. 验证集群状态
    echo "步骤3: 验证集群状态..."
    sleep 30
    etcdctl endpoint health --cluster
    etcdctl endpoint status --cluster -w table
}

# etcd缩容函数
scale_down_etcd() {
    local remove_member_ip=$1
    
    echo "开始缩容etcd集群，移除节点: $remove_member_ip"
    
    # 1. 获取要移除的成员ID
    local member_id=$(etcdctl member list -w json | \
        jq -r ".members[] | select(.peerURLs[] | contains(\"$remove_member_ip\")) | .ID")
    
    if [ -z "$member_id" ]; then
        echo "错误: 未找到IP为 $remove_member_ip 的成员"
        return 1
    fi
    
    # 2. 停止目标节点的etcd服务
    echo "步骤1: 停止目标节点etcd服务..."
    ssh $remove_member_ip "systemctl stop etcd"
    
    # 3. 从集群中移除成员
    echo "步骤2: 从集群移除成员..."
    etcdctl member remove $member_id
    
    # 4. 验证集群状态
    echo "步骤3: 验证集群状态..."
    etcdctl endpoint health --cluster
    etcdctl member list -w table
}

# 使用示例
# scale_up_etcd "10.0.1.100"
# scale_down_etcd "10.0.1.50"
```

---

## 5. API Server扩缩容

### 5.1 API Server水平扩展

```yaml
# API Server水平扩展Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kube-apiserver-hpa
  namespace: kube-system
spec:
  replicas: 3
  selector:
    matchLabels:
      component: kube-apiserver
  template:
    metadata:
      labels:
        component: kube-apiserver
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: component
                  operator: In
                  values:
                  - kube-apiserver
              topologyKey: kubernetes.io/hostname
      containers:
      - name: kube-apiserver
        image: registry.k8s.io/kube-apiserver:v1.30.0
        command:
        - kube-apiserver
        args:
        # 基础配置
        - --advertise-address=$(POD_IP)
        - --allow-privileged=true
        - --authorization-mode=Node,RBAC
        - --client-ca-file=/etc/kubernetes/pki/ca.crt
        - --enable-admission-plugins=NodeRestriction
        - --enable-bootstrap-token-auth=true
        - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
        - --etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
        - --etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
        - --etcd-servers=https://etcd-0:2379,https://etcd-1:2379,https://etcd-2:2379
        - --kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt
        - --kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key
        - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
        - --proxy-client-cert-file=/etc/kubernetes/pki/front-proxy-client.crt
        - --proxy-client-key-file=/etc/kubernetes/pki/front-proxy-client.key
        - --requestheader-allowed-names=front-proxy-client
        - --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
        - --requestheader-extra-headers-prefix=X-Remote-Extra-
        - --requestheader-group-headers=X-Remote-Group
        - --requestheader-username-headers=X-Remote-User
        - --secure-port=6443
        - --service-account-issuer=https://kubernetes.default.svc.cluster.local
        - --service-account-key-file=/etc/kubernetes/pki/sa.pub
        - --service-account-signing-key-file=/etc/kubernetes/pki/sa.key
        - --service-cluster-ip-range=10.96.0.0/12
        - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
        - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
        
        # 扩展性优化参数
        - --max-requests-inflight=4000
        - --max-mutating-requests-inflight=2000
        - --default-watch-cache-size=4000
        - --watch-cache-sizes=pods#20000,nodes#5000,services#5000,endpoints#10000
        - --enable-priority-and-fairness=true
        - --goaway-chance=0.001
        - --delete-collection-workers=16
        - --etcd-qps=200
        - --etcd-burst=400
        
        ports:
        - containerPort: 6443
          name: https
        readinessProbe:
          httpGet:
            path: /readyz
            port: 6443
            scheme: HTTPS
          initialDelaySeconds: 5
          periodSeconds: 5
        livenessProbe:
          httpGet:
            path: /livez
            port: 6443
            scheme: HTTPS
          initialDelaySeconds: 10
          periodSeconds: 10
        resources:
          requests:
            cpu: 2000m
            memory: 8Gi
          limits:
            cpu: 4000m
            memory: 16Gi
        volumeMounts:
        - mountPath: /etc/kubernetes/pki
          name: k8s-certs
          readOnly: true
        - mountPath: /etc/ssl/certs
          name: ca-certs
          readOnly: true
        - mountPath: /etc/kubernetes/pki/etcd
          name: etcd-certs
          readOnly: true
      hostNetwork: true
      priorityClassName: system-node-critical
      volumes:
      - hostPath:
          path: /etc/kubernetes/pki
          type: DirectoryOrCreate
        name: k8s-certs
      - hostPath:
          path: /etc/ssl/certs
          type: DirectoryOrCreate
        name: ca-certs
      - hostPath:
          path: /etc/kubernetes/pki/etcd
          type: DirectoryOrCreate
        name: etcd-certs
```

### 5.2 自动扩缩容配置

```yaml
# API Server HPA配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: kube-apiserver-hpa
  namespace: kube-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: kube-apiserver-hpa
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: apiserver_request_duration_seconds_count
      target:
        type: AverageValue
        averageValue: "1000"  # 每秒请求数
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
      - type: Pods
        value: 4
        periodSeconds: 15
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
      selectPolicy: Min
```

---

## 6. 控制器管理器扩展

### 6.1 多实例部署配置

```yaml
# Controller Manager多实例部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kube-controller-manager-multi
  namespace: kube-system
spec:
  replicas: 3
  selector:
    matchLabels:
      component: kube-controller-manager
  template:
    metadata:
      labels:
        component: kube-controller-manager
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: component
                operator: In
                values:
                - kube-controller-manager
            topologyKey: kubernetes.io/hostname
      containers:
      - name: kube-controller-manager
        image: registry.k8s.io/kube-controller-manager:v1.30.0
        command:
        - kube-controller-manager
        args:
        # 基础配置
        - --allocate-node-cidrs=true
        - --authentication-kubeconfig=/etc/kubernetes/controller-manager.conf
        - --authorization-kubeconfig=/etc/kubernetes/controller-manager.conf
        - --bind-address=0.0.0.0
        - --client-ca-file=/etc/kubernetes/pki/ca.crt
        - --cluster-cidr=10.244.0.0/16
        - --cluster-name=kubernetes
        - --cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt
        - --cluster-signing-key-file=/etc/kubernetes/pki/ca.key
        - --controllers=*,bootstrapsigner,tokencleaner
        - --kubeconfig=/etc/kubernetes/controller-manager.conf
        - --leader-elect=true
        - --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
        - --root-ca-file=/etc/kubernetes/pki/ca.crt
        - --service-account-private-key-file=/etc/kubernetes/pki/sa.key
        - --service-cluster-ip-range=10.96.0.0/12
        - --use-service-account-credentials=true
        
        # 扩展性优化
        - --concurrent-deployment-syncs=10
        - --concurrent-endpoint-syncs=10
        - --concurrent-gc-syncs=30
        - --concurrent-namespace-syncs=20
        - --concurrent-replicaset-syncs=10
        - --concurrent-service-syncs=5
        - --large-cluster-size-threshold=500
        - --node-eviction-rate=0.1
        - --secondary-node-eviction-rate=0.01
        
        livenessProbe:
          failureThreshold: 8
          httpGet:
            host: 127.0.0.1
            path: /healthz
            port: 10257
            scheme: HTTPS
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 15
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 2Gi
        volumeMounts:
        - mountPath: /etc/ssl/certs
          name: ca-certs
          readOnly: true
        - mountPath: /etc/kubernetes/pki
          name: k8s-certs
          readOnly: true
        - mountPath: /etc/kubernetes/controller-manager.conf
          name: kubeconfig
          readOnly: true
      hostNetwork: true
      priorityClassName: system-node-critical
      volumes:
      - hostPath:
          path: /etc/ssl/certs
          type: DirectoryOrCreate
        name: ca-certs
      - hostPath:
          path: /etc/kubernetes/pki
          type: DirectoryOrCreate
        name: k8s-certs
      - hostPath:
          path: /etc/kubernetes/controller-manager.conf
          type: FileOrCreate
        name: kubeconfig
```

---

## 7. 调度器扩展机制

### 7.1 多调度器部署

```yaml
# 多调度器配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kube-scheduler-multi
  namespace: kube-system
spec:
  replicas: 3
  selector:
    matchLabels:
      component: kube-scheduler
  template:
    metadata:
      labels:
        component: kube-scheduler
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: component
                operator: In
                values:
                - kube-scheduler
            topologyKey: kubernetes.io/hostname
      containers:
      - name: kube-scheduler
        image: registry.k8s.io/kube-scheduler:v1.30.0
        command:
        - kube-scheduler
        args:
        # 基础配置
        - --authentication-kubeconfig=/etc/kubernetes/scheduler.conf
        - --authorization-kubeconfig=/etc/kubernetes/scheduler.conf
        - --bind-address=0.0.0.0
        - --kubeconfig=/etc/kubernetes/scheduler.conf
        - --leader-elect=true
        - --lock-object-name=kube-scheduler
        
        # 扩展性优化
        - --parallelism=16
        - --percentage-of-nodes-to-score=50
        - --pod-max-in-unschedulable-pods-duration=30s
        - --scheduler-name=default-scheduler
        
        livenessProbe:
          failureThreshold: 8
          httpGet:
            host: 127.0.0.1
            path: /healthz
            port: 10259
            scheme: HTTPS
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 15
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 1Gi
        volumeMounts:
        - mountPath: /etc/kubernetes/scheduler.conf
          name: kubeconfig
          readOnly: true
      hostNetwork: true
      priorityClassName: system-node-critical
      volumes:
      - hostPath:
          path: /etc/kubernetes/scheduler.conf
          type: FileOrCreate
        name: kubeconfig
```

---

## 8. 自动扩缩容配置

### 8.1 基于指标的自动扩缩容

```yaml
# 基于自定义指标的扩缩容
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: control-plane-hpa
  namespace: kube-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: kube-apiserver
  minReplicas: 3
  maxReplicas: 15
  metrics:
  # CPU使用率
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 75
  
  # 内存使用率
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  
  # API Server请求速率
  - type: Pods
    pods:
      metric:
        name: apiserver_request_total
      target:
        type: AverageValue
        averageValue: "2000"
  
  # etcd延迟
  - type: External
    external:
      metric:
        name: etcd_disk_backend_commit_duration_seconds
      target:
        type: Value
        value: "0.05"  # 50ms
  
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      - type: Pods
        value: 2
        periodSeconds: 60
      selectPolicy: Max
    
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 120
      selectPolicy: Min
```

### 8.2 基于Prometheus的自定义指标

```yaml
# Prometheus Adapter配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: adapter-config
  namespace: custom-metrics
data:
  config.yaml: |
    rules:
    - seriesQuery: 'apiserver_request_total'
      resources:
        overrides:
          namespace:
            resource: namespace
          pod:
            resource: pod
      name:
        matches: "apiserver_request_total"
        as: "apiserver_request_rate"
      metricsQuery: 'sum(rate(apiserver_request_total[2m])) by (<<.GroupBy>>)'
      
    - seriesQuery: 'etcd_disk_backend_commit_duration_seconds'
      resources:
        overrides:
          namespace:
            resource: namespace
          pod:
            resource: pod
      name:
        matches: "etcd_disk_backend_commit_duration_seconds"
        as: "etcd_commit_latency"
      metricsQuery: 'avg(etcd_disk_backend_commit_duration_seconds) by (<<.GroupBy>>)'

---
# 自定义指标HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: custom-metrics-hpa
  namespace: kube-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: kube-apiserver
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Pods
    pods:
      metric:
        name: apiserver_request_rate
      target:
        type: AverageValue
        averageValue: "1500"
  - type: External
    external:
      metric:
        name: etcd_commit_latency
      target:
        type: Value
        value: "0.03"
```

---

## 9. 性能容量规划

### 9.1 容量规划矩阵

| 集群规模 | 节点数 | Pod数 | API QPS | etcd节点 | 建议配置 |
|----------|--------|-------|---------|----------|----------|
| 小型 | <100 | <1000 | <1000 | 3 | 2核8GB |
| 中型 | 100-500 | 1000-5000 | 1000-5000 | 3 | 4核16GB |
| 大型 | 500-1000 | 5000-15000 | 5000-15000 | 5 | 8核32GB |
| 超大型 | 1000-3000 | 15000-50000 | 15000-50000 | 5 | 16核64GB |
| 极大型 | >3000 | >50000 | >50000 | 7 | 32核128GB |

### 9.2 性能基准测试

```bash
#!/bin/bash
# 控制平面性能基准测试脚本

# 测试配置
TEST_DURATION=300  # 5分钟
CONCURRENT_USERS=100
API_SERVER_ENDPOINT="https://kubernetes.default.svc:443"

# 1. 基准测试准备
setup_benchmark() {
    echo "=== 控制平面性能基准测试 ==="
    echo "测试时长: ${TEST_DURATION}秒"
    echo "并发用户: ${CONCURRENT_USERS}"
    echo "API Server: ${API_SERVER_ENDPOINT}"
    echo
    
    # 创建测试命名空间
    kubectl create namespace scalability-test --dry-run=client -o yaml | kubectl apply -f -
    
    # 部署测试应用
    cat > /tmp/scalability-test.yaml << EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: scalability-test-app
  namespace: scalability-test
spec:
  replicas: 100
  selector:
    matchLabels:
      app: scalability-test
  template:
    metadata:
      labels:
        app: scalability-test
    spec:
      containers:
      - name: test-app
        image: nginx:latest
        resources:
          requests:
            memory: "64Mi"
            cpu: "250m"
EOF
    
    kubectl apply -f /tmp/scalability-test.yaml
    kubectl wait --for=condition=available --timeout=300s deployment/scalability-test-app -n scalability-test
}

# 2. API延迟测试
test_api_latency() {
    echo "测试API延迟..."
    
    # GET请求延迟测试
    echo "测试GET请求延迟..."
    for i in {1..1000}; do
        start_time=$(date +%s%3N)
        kubectl get pods -n scalability-test >/dev/null 2>&1
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/get-latency.txt
    done
    
    # POST请求延迟测试
    echo "测试POST请求延迟..."
    for i in {1..500}; do
        start_time=$(date +%s%3N)
        cat > /tmp/temp-pod.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: temp-pod-$i
  namespace: scalability-test
spec:
  containers:
  - name: temp
    image: busybox
    command: ["sleep", "3600"]
EOF
        kubectl apply -f /tmp/temp-pod.yaml >/dev/null 2>&1
        end_time=$(date +%s%3N)
        echo "$((end_time - start_time))" >> /tmp/post-latency.txt
    done
}

# 3. 并发性能测试
test_concurrent_performance() {
    echo "测试并发性能..."
    
    # 使用hey工具进行并发测试
    hey -z ${TEST_DURATION}s -c ${CONCURRENT_USERS} \
        -H "Authorization: Bearer $(kubectl create token default -n scalability-test)" \
        "${API_SERVER_ENDPOINT}/api/v1/namespaces/scalability-test/pods" \
        > /tmp/concurrent-results.txt 2>&1
}

# 4. 结果分析
analyze_results() {
    echo "=== 测试结果分析 ==="
    
    # GET延迟统计
    echo "GET请求延迟统计:"
    if [ -f /tmp/get-latency.txt ]; then
        awk '{sum+=$1; sumsq+=$1*$1; if(NR==1)min=max=$1; if($1<min)min=$1; if($1>max)max=$1} 
             END {printf "平均: %.2fms, 标准差: %.2fms, 最小: %.2fms, 最大: %.2fms\n", 
                  sum/NR, sqrt(sumsq/NR - (sum/NR)^2), min, max}' /tmp/get-latency.txt
    fi
    
    # POST延迟统计
    echo "POST请求延迟统计:"
    if [ -f /tmp/post-latency.txt ]; then
        awk '{sum+=$1; sumsq+=$1*$1; if(NR==1)min=max=$1; if($1<min)min=$1; if($1>max)max=$1} 
             END {printf "平均: %.2fms, 标准差: %.2fms, 最小: %.2fms, 最大: %.2fms\n", 
                  sum/NR, sqrt(sumsq/NR - (sum/NR)^2), min, max}' /tmp/post-latency.txt
    fi
    
    # 并发测试结果
    echo "并发性能结果:"
    if [ -f /tmp/concurrent-results.txt ]; then
        grep "Requests/sec" /tmp/concurrent-results.txt || echo "未找到请求速率数据"
        grep "Latency" /tmp/concurrent-results.txt || echo "未找到延迟数据"
    fi
}

# 5. 清理测试资源
cleanup_test() {
    echo "清理测试资源..."
    kubectl delete namespace scalability-test --ignore-not-found=true
    rm -f /tmp/scalability-test.yaml /tmp/temp-pod.yaml /tmp/get-latency.txt /tmp/post-latency.txt /tmp/concurrent-results.txt
}

# 执行测试
setup_benchmark
test_api_latency
test_concurrent_performance
analyze_results
cleanup_test

echo "=== 基准测试完成 ==="
```

---

## 10. 扩缩容最佳实践

### 10.1 扩缩容检查清单

```markdown
## 扩缩容前检查清单

### 基础环境检查
- [ ] 集群版本兼容性确认
- [ ] 硬件资源充足性评估
- [ ] 网络连通性测试
- [ ] 存储性能基准验证
- [ ] 证书有效期检查

### 配置检查
- [ ] 负载均衡器配置验证
- [ ] DNS解析正确性确认
- [ ] 安全组规则检查
- [ ] 时间同步状态验证
- [ ] 日志收集系统就绪

### 监控告警
- [ ] 核心指标监控启用
- [ ] 告警规则配置完成
- [ ] 通知渠道测试通过
- [ ] 性能基线建立
- [ ] 容量水位线设定

### 备份恢复
- [ ] etcd数据备份完成
- [ ] 配置文件备份
- [ ] 恢复流程文档化
- [ ] 灾备演练完成
- [ ] 回滚方案准备

### 团队准备
- [ ] 操作人员培训完成
- [ ] 变更窗口确定
- [ ] 回滚负责人指定
- [ ] 沟通机制建立
- [ ] 应急联系人确认
```

### 10.2 扩缩容操作流程

```bash
#!/bin/bash
# 标准化扩缩容操作流程

# 日志记录函数
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a /var/log/scaling-operation.log
}

# 预检查阶段
pre_check() {
    log "开始预检查阶段..."
    
    # 1. 集群健康检查
    log "检查集群健康状态..."
    if ! kubectl get nodes | grep -q " Ready "; then
        log "错误: 集群节点状态异常"
        return 1
    fi
    
    # 2. 控制平面组件检查
    log "检查控制平面组件..."
    kubectl get pods -n kube-system -l tier=control-plane
    if [ $? -ne 0 ]; then
        log "错误: 控制平面组件状态异常"
        return 1
    fi
    
    # 3. etcd集群健康检查
    log "检查etcd集群健康..."
    if ! etcdctl endpoint health --cluster; then
        log "错误: etcd集群不健康"
        return 1
    fi
    
    log "预检查完成，集群状态正常"
    return 0
}

# 扩容执行阶段
scale_up() {
    local target_replicas=$1
    log "开始扩容操作，目标副本数: $target_replicas"
    
    # 1. 备份当前配置
    log "备份当前配置..."
    kubectl get deployment kube-apiserver -n kube-system -o yaml > /backup/apiserver-before-scaleup-$(date +%Y%m%d-%H%M%S).yaml
    
    # 2. 更新副本数
    log "更新API Server副本数..."
    kubectl scale deployment kube-apiserver -n kube-system --replicas=$target_replicas
    
    # 3. 等待Pod就绪
    log "等待新Pod就绪..."
    kubectl wait --for=condition=available --timeout=300s deployment/kube-apiserver -n kube-system
    
    # 4. 验证负载均衡
    log "验证负载均衡..."
    for i in {1..10}; do
        curl -sk https://kubernetes.default.svc:443/healthz
        sleep 5
    done
    
    log "扩容操作完成"
}

# 缩容执行阶段
scale_down() {
    local target_replicas=$1
    log "开始缩容操作，目标副本数: $target_replicas"
    
    # 1. 检查当前负载
    log "检查当前负载情况..."
    current_load=$(kubectl top pods -n kube-system | grep apiserver | awk '{print $2}' | sed 's/m//' | awk '{sum+=$1} END {print sum}')
    log "当前CPU使用总量: ${current_load}m"
    
    # 2. 备份配置
    kubectl get deployment kube-apiserver -n kube-system -o yaml > /backup/apiserver-before-scaledown-$(date +%Y%m%d-%H%M%S).yaml
    
    # 3. 执行缩容
    log "执行缩容操作..."
    kubectl scale deployment kube-apiserver -n kube-system --replicas=$target_replicas
    
    # 4. 等待稳定
    log "等待集群稳定..."
    sleep 60
    
    # 5. 验证功能
    log "验证核心功能..."
    kubectl get nodes
    kubectl get pods --all-namespaces
    
    log "缩容操作完成"
}

# 后验证阶段
post_verification() {
    log "开始后验证阶段..."
    
    # 1. 功能验证
    log "验证集群功能..."
    kubectl get nodes -o wide
    kubectl get componentstatuses
    
    # 2. 性能验证
    log "验证性能指标..."
    kubectl top nodes
    kubectl top pods -n kube-system
    
    # 3. 监控告警验证
    log "验证监控告警..."
    # 这里可以集成具体的监控系统API调用
    
    log "后验证完成"
}

# 主执行流程
main() {
    local operation=$1
    local target_replicas=$2
    
    case $operation in
        "scale-up")
            if pre_check && scale_up $target_replicas; then
                post_verification
                log "扩容操作成功完成"
            else
                log "扩容操作失败"
                exit 1
            fi
            ;;
        "scale-down")
            if pre_check && scale_down $target_replicas; then
                post_verification
                log "缩容操作成功完成"
            else
                log "缩容操作失败"
                exit 1
            fi
            ;;
        *)
            echo "用法: $0 {scale-up|scale-down} <replicas>"
            exit 1
            ;;
    esac
}

# 执行主函数
main "$@"
```

### 10.3 监控告警配置

```yaml
# 控制平面扩缩容监控告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: control-plane-scaling-alerts
  namespace: monitoring
spec:
  groups:
  - name: control-plane.scaling.rules
    rules:
    # API Server扩缩容相关告警
    - alert: APIServerHighLatency
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 0.5
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "API Server高延迟"
        description: "API Server 99th percentile latency is above 500ms"
        
    - alert: APIServerHighErrorRate
      expr: rate(apiserver_request_total{code=~"5.."}[5m]) / rate(apiserver_request_total[5m]) > 0.05
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "API Server高错误率"
        description: "API Server error rate is above 5%"
        
    - alert: ControlPlaneUnderProvisioned
      expr: kube_deployment_spec_replicas{deployment="kube-apiserver"} < 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "控制平面实例不足"
        description: "Control plane instances are below minimum threshold"
        
    # etcd扩缩容相关告警
    - alert: EtcdHighCommitLatency
      expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "etcd提交延迟高"
        description: "etcd 99th percentile commit latency is above 100ms"
        
    - alert: EtcdHighFsyncLatency
      expr: histogram_quantile(0.99, rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) > 0.05
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "etcd fsync延迟高"
        description: "etcd 99th percentile fsync latency is above 50ms"
        
    # 自动扩缩容相关告警
    - alert: HPAStuckScaling
      expr: kube_horizontalpodautoscaler_status_condition{condition="ScalingActive",status="false"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "HPA扩缩容卡住"
        description: "Horizontal Pod Autoscaler is stuck and not scaling"
        
    - alert: HPAScaleUpFailed
      expr: increase(kube_horizontalpodautoscaler_status_desired_replicas[10m]) == 0 and
            kube_horizontalpodautoscaler_status_current_replicas < kube_horizontalpodautoscaler_spec_min_replicas
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "HPA扩容失败"
        description: "HPA failed to scale up despite being below minimum replicas"
```

这份扩缩容指南涵盖了从设计原则到具体实施的完整内容，包括水平扩展、垂直扩展、etcd管理、自动扩缩容等多个方面，为Kubernetes控制平面的扩展性提供了全面的技术指导。