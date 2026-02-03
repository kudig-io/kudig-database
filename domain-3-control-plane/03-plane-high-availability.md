# 控制平面高可用部署模式 (Control Plane High Availability Deployment Patterns)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 部署架构指南

---

## 目录

1. [高可用设计原则](#1-高可用设计原则)
2. [部署模式详解](#2-部署模式详解)
3. [etcd高可用配置](#3-etcd高可用配置)
4. [负载均衡策略](#4-负载均衡策略)
5. [故障切换机制](#5-故障切换机制)
6. [监控告警配置](#6-监控告警配置)
7. [性能调优建议](#7-性能调优建议)
8. [生产环境最佳实践](#8-生产环境最佳实践)

---

## 1. 高可用设计原则

### 1.1 高可用核心要素

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        High Availability Core Elements                          │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  1. 冗余设计 (Redundancy)                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │  组件冗余:                                                               │    │
│  │  ├── 控制平面组件: 3-5个实例                                             │    │
│  │  ├── etcd集群: 3-5个节点                                                 │    │
│  │  └── 负载均衡器: 2个以上                                                 │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  2. 故障隔离 (Fault Isolation)                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │  故障域划分:                                                             │    │
│  │  ├── 物理机架隔离                                                         │    │
│  │  ├── 电源回路隔离                                                         │    │
│  │  ├── 网络交换机隔离                                                       │    │
│  │  └── 可用区隔离                                                           │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  3. 自动故障转移 (Automatic Failover)                                           │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │  自动化机制:                                                             │    │
│  │  ├── Leader选举 (etcd Raft)                                              │    │
│  │  ├── 健康检查 (Load Balancer)                                            │    │
│  │  ├── 节点驱逐 (Node Controller)                                          │    │
│  │  └── Pod重新调度 (Scheduler)                                             │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  4. 数据一致性 (Data Consistency)                                               │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │  一致性保证:                                                             │    │
│  │  ├── etcd Raft协议 (强一致性)                                            │    │
│  │  ├── 多数派写入原则                                                      │    │
│  │  └── 数据备份与恢复                                                      │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 高可用等级定义

| 等级 | 可用性 | 年度停机时间 | 部署复杂度 | 成本 |
|------|--------|-------------|------------|------|
| **Level 1** | 99% | 87.6小时 | 简单 | 低 |
| **Level 2** | 99.5% | 43.8小时 | 中等 | 中 |
| **Level 3** | 99.9% | 8.76小时 | 复杂 | 高 |
| **Level 4** | 99.95% | 4.38小时 | 很复杂 | 很高 |
| **Level 5** | 99.99% | 52.6分钟 | 极复杂 | 极高 |

### 1.3 故障域规划

```
故障域隔离策略:

┌─────────────────────────────────────────────────────────────────────────┐
│                           Fault Domain Planning                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Rack Level Isolation (机架级隔离):                                      │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │
│  │ Control-1   │ │ Control-2   │ │ Control-3   │                        │
│  │ (Rack A)    │ │ (Rack B)    │ │ (Rack C)    │                        │
│  └─────────────┘ └─────────────┘ └─────────────┘                        │
│                                                                          │
│  Power Domain Isolation (电源域隔离):                                    │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │
│  │ Control-1   │ │ Control-2   │ │ Control-3   │                        │
│  │ (PDU-A)     │ │ (PDU-B)     │ │ (PDU-C)     │                        │
│  └─────────────┘ └─────────────┘ └─────────────┘                        │
│                                                                          │
│  Network Domain Isolation (网络域隔离):                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │
│  │ Control-1   │ │ Control-2   │ │ Control-3   │                        │
│  │ (Switch-A)  │ │ (Switch-B)  │ │ (Switch-C)  │                        │
│  └─────────────┘ └─────────────┘ └─────────────┘                        │
│                                                                          │
│  Availability Zone Isolation (可用区隔离):                               │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                        │
│  │ Control-1   │ │ Control-2   │ │ Control-3   │                        │
│  │ (AZ-A)      │ │ (AZ-B)      │ │ (AZ-C)      │                        │
│  └─────────────┘ └─────────────┘ └─────────────┘                        │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 部署模式详解

### 2.1 模式1: 标准3节点HA

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           Standard 3-Node HA Setup                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  架构特点:                                                                      │
│  • 成本效益最优                                                                │
│  • 部署简单                                                                     │
│  • 维护方便                                                                     │
│  • 适用于中小型生产环境                                                         │
│                                                                                  │
│  拓扑结构:                                                                      │
│                                                                                  │
│  Load Balancer (VIP: apiserver.cluster.local)                                  │
│          │                                                                      │
│          ▼                                                                      │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                              │
│  │ Control-1   │ │ Control-2   │ │ Control-3   │                              │
│  │             │ │             │ │             │                              │
│  │ etcd-1      │ │ etcd-2      │ │ etcd-3      │                              │
│  │ apiserver-1 │ │ apiserver-2 │ │ apiserver-3 │                              │
│  │ kcm-1       │ │ kcm-2       │ │ kcm-3       │                              │
│  │ scheduler-1 │ │ scheduler-2 │ │ scheduler-3 │                              │
│  │ ccm-1       │ │ ccm-2       │ │ ccm-3       │                              │
│  └─────────────┘ └─────────────┘ └─────────────┘                              │
│                                                                                  │
│  配置示例:                                                                      │
│                                                                                  │
│  etcd集群配置:                                                                  │
│  ETCD_INITIAL_CLUSTER=                                                          │
│    etcd-1=https://control-1:2380,                                              │
│    etcd-2=https://control-2:2380,                                              │
│    etcd-3=https://control-3:2380                                               │
│                                                                                  │
│  API Server配置:                                                                │
│  --etcd-servers=https://control-1:2379,https://control-2:2379,https://control-3:2379│
│  --bind-address=0.0.0.0                                                        │
│  --secure-port=6443                                                            │
│                                                                                  │
│  负载均衡器配置:                                                                │
│  upstream kubernetes_apiserver {                                               │
│    server control-1:6443 max_fails=3 fail_timeout=30s;                         │
│    server control-2:6443 max_fails=3 fail_timeout=30s;                         │
│    server control-3:6443 max_fails=3 fail_timeout=30s;                         │
│  }                                                                             │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 模式2: 分离式架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Separated Architecture Pattern                           │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  架构特点:                                                                      │
│  • 控制平面与etcd分离                                                          │
│  • 独立扩展能力                                                                │
│  • 更好的资源隔离                                                              │
│  • 适用于大型生产环境                                                          │
│                                                                                  │
│  拓扑结构:                                                                      │
│                                                                                  │
│  Load Balancer (VIP: apiserver.cluster.local)                                  │
│          │                                                                      │
│          ▼                                                                      │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                              │
│  │ API-1       │ │ API-2       │ │ API-3       │  ← API Server Tier           │
│  │             │ │             │ │             │                              │
│  └─────────────┘ └─────────────┘ └─────────────┘                              │
│          │            │             │                                          │
│          ▼            ▼             ▼                                          │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                              etcd Cluster                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │ etcd-1      │ │ etcd-2      │ │ etcd-3      │ │ etcd-4/5    │       │    │
│  │  │ (Leader)    │ │ (Follower)  │ │ (Follower)  │ │ (Optional)  │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                              │
│  │ KCM-1       │ │ KCM-2       │ │ KCM-3       │  ← Controller Manager Tier   │
│  │             │ │             │ │             │                              │
│  └─────────────┘ └─────────────┘ └─────────────┘                              │
│                                                                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                              │
│  │ SCHED-1     │ │ SCHED-2     │ │ SCHED-3     │  ← Scheduler Tier            │
│  │             │ │             │ │             │                              │
│  └─────────────┘ └─────────────┘ └─────────────┘                              │
│                                                                                  │
│  优势分析:                                                                      │
│  ├── API Server可独立水平扩展                                                  │
│  ├── etcd集群可专门优化存储性能                                                │
│  ├── 各组件故障域完全隔离                                                      │
│  └── 资源分配更加精细                                                          │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 模式3: 混合云部署

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         Hybrid Cloud HA Pattern                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  架构特点:                                                                      │
│  • 跨云厂商部署                                                                │
│  • 异地多活能力                                                                │
│  • RTO < 5分钟                                                                  │
│  • 成本较高但可靠性极佳                                                        │
│                                                                                  │
│  拓扑结构:                                                                      │
│                                                                                  │
│                    Global Load Balancer                                         │
│                             │                                                    │
│          ┌──────────────────┼──────────────────┐                                │
│          │                  │                  │                                 │
│          ▼                  ▼                  ▼                                 │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                         │
│  │ Region-A    │    │ Region-B    │    │ Region-C    │                         │
│  │ (Primary)   │    │ (Standby)   │    │ (DR Site)   │                         │
│  │             │    │             │    │             │                         │
│  │ Control-A   │◄──►│ Control-B   │◄──►│ Control-C   │                         │
│  │ etcd-A      │    │ etcd-B      │    │ etcd-C      │                         │
│  └─────────────┘    └─────────────┘    └─────────────┘                         │
│          │                  │                  │                                 │
│          └──────────────────┼──────────────────┘                                 │
│                             ▼                                                    │
│                    Cross-Region Replication                                      │
│                                                                                  │
│  配置要点:                                                                      │
│                                                                                  │
│  1. DNS配置:                                                                    │
│     kubernetes.cluster.local IN CNAME lb-global.example.com                     │
│                                                                                  │
│  2. 跨区域网络:                                                                 │
│     ├── VPN隧道连接各区域                                                      │
│     ├── 专线互联保证低延迟                                                     │
│     └── 网络质量监控                                                           │
│                                                                                  │
│  3. 数据同步:                                                                   │
│     ├── etcd跨区域复制                                                         │
│     ├── 镜像仓库同步                                                           │
│     └── 配置管理同步                                                           │
│                                                                                  │
│  4. 故障切换:                                                                   │
│     ├── 健康检查机制                                                           │
│     ├── 自动DNS切换                                                            │
│     └── 应用状态同步                                                           │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.4 模式4: 边缘计算部署

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Edge Computing HA Pattern                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  架构特点:                                                                      │
│  • 中心-边缘分层架构                                                           │
│  • 边缘节点自治能力                                                             │
│  • 断网情况下继续运行                                                           │
│  • 适用于IoT/边缘计算场景                                                      │
│                                                                                  │
│  拓扑结构:                                                                      │
│                                                                                  │
│  Central Control Plane (中心控制平面)                                          │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                              Master Cluster                             │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                       │    │
│  │  │ Control-1   │ │ Control-2   │ │ Control-3   │                       │    │
│  │  │             │ │             │ │             │                       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘                       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │                                                                      │
│          ▼                                                                      │
│  Multi-Region Edge Clusters (多区域边缘集群)                                    │
│                                                                                  │
│  Region A Edge Cluster                        Region B Edge Cluster            │
│  ┌─────────────────────────┐                 ┌─────────────────────────┐       │
│  │    Edge Control Plane   │                 │    Edge Control Plane   │       │
│  │  ┌───────────────────┐  │                 │  ┌───────────────────┐  │       │
│  │  │ Edge-API Server   │  │                 │  │ Edge-API Server   │  │       │
│  │  │ (Local Cache)     │  │                 │  │ (Local Cache)     │  │       │
│  │  └───────────────────┘  │                 │  └───────────────────┘  │       │
│  │                          │                 │                          │       │
│  │  ┌───────────────────┐  │                 │  ┌───────────────────┐  │       │
│  │  │ Local Controllers │  │                 │  │ Local Controllers │  │       │
│  │  │ (Autonomous Mode) │  │                 │  │ (Autonomous Mode) │  │       │
│  │  └───────────────────┘  │                 │  └───────────────────┘  │       │
│  │                          │                 │                          │       │
│  │  Worker Nodes (10-100)  │                 │  Worker Nodes (10-100)  │       │
│  └─────────────────────────┘                 └─────────────────────────┘       │
│          │                                              │                        │
│          └──────────────────────────────────────────────┘                        │
│                                    │                                             │
│                                    ▼                                             │
│                         Inter-Cluster Synchronization                           │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. etcd高可用配置

### 3.1 etcd集群配置最佳实践

```yaml
# etcd静态Pod配置示例
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
  labels:
    component: etcd
    tier: control-plane
spec:
  hostNetwork: true
  priorityClassName: system-node-critical
  containers:
  - name: etcd
    image: quay.io/coreos/etcd:v3.5.12
    command:
    - etcd
    - --name=etcd-1
    - --data-dir=/var/lib/etcd
    - --wal-dir=/var/lib/etcd/wal
    
    # 网络配置
    - --listen-peer-urls=https://0.0.0.0:2380
    - --listen-client-urls=https://0.0.0.0:2379
    - --advertise-client-urls=https://control-1:2379
    - --initial-advertise-peer-urls=https://control-1:2380
    
    # 集群配置
    - --initial-cluster=etcd-1=https://control-1:2380,etcd-2=https://control-2:2380,etcd-3=https://control-3:2380
    - --initial-cluster-state=new
    - --initial-cluster-token=etcd-cluster-1
    
    # 安全配置
    - --client-cert-auth=true
    - --peer-client-cert-auth=true
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    
    # 性能调优
    - --quota-backend-bytes=8589934592        # 8GB存储配额
    - --max-request-bytes=10485760            # 10MB最大请求
    - --heartbeat-interval=100                # 100ms心跳间隔
    - --election-timeout=1000                 # 1s选举超时
    - --snapshot-count=10000                  # 1万事务触发快照
    - --auto-compaction-retention=1h          # 1小时自动压缩
    - --max-txn-ops=10000                     # 最大事务操作数
    
    # 监控配置
    - --listen-metrics-urls=http://0.0.0.0:2381
    
    volumeMounts:
    - name: etcd-data
      mountPath: /var/lib/etcd
    - name: etcd-certs
      mountPath: /etc/kubernetes/pki/etcd
      readOnly: true
      
  volumes:
  - name: etcd-data
    hostPath:
      path: /var/lib/etcd
      type: DirectoryOrCreate
  - name: etcd-certs
    hostPath:
      path: /etc/kubernetes/pki/etcd
      type: DirectoryOrCreate
```

### 3.2 etcd性能调优参数

| 参数 | 默认值 | 推荐值(小集群) | 推荐值(大集群) | 说明 |
|------|--------|---------------|---------------|------|
| `--quota-backend-bytes` | 2GB | 4GB | 8GB | 存储配额 |
| `--max-request-bytes` | 1.5MB | 5MB | 10MB | 最大请求大小 |
| `--heartbeat-interval` | 100ms | 100ms | 200ms | 心跳间隔 |
| `--election-timeout` | 1000ms | 1000ms | 2000ms | 选举超时 |
| `--snapshot-count` | 100000 | 10000 | 5000 | 快照触发计数 |
| `--auto-compaction-retention` | 0 | 1h | 30m | 自动压缩保留 |
| `--max-txn-ops` | 128 | 512 | 1024 | 最大事务操作数 |

### 3.3 etcd监控指标

```prometheus
# etcd关键监控指标
etcd_server_has_leader{job="etcd"} == 1                    # Leader存在
etcd_server_leader_changes_seen_total{job="etcd"} < 3      # Leader切换次数
etcd_disk_backend_commit_duration_seconds{job="etcd", quantile="0.99"} < 0.25  # 磁盘提交延迟
etcd_disk_wal_fsync_duration_seconds{job="etcd", quantile="0.99"} < 0.1        # WAL同步延迟
etcd_network_peer_round_trip_time_seconds{job="etcd", quantile="0.99"} < 0.1   # 节点间RTT
etcd_mvcc_db_total_size_in_bytes{job="etcd"} < 8589934592  # 数据库大小
etcd_server_proposals_committed_total{job="etcd"}          # 提交的提案数
etcd_server_proposals_failed_total{job="etcd"} == 0        # 失败的提案数
```

---

## 4. 负载均衡策略

### 4.1 负载均衡器配置

#### HAProxy配置示例

```haproxy
# /etc/haproxy/haproxy.cfg
global
    log /dev/log local0
    log /dev/log local1 notice
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin expose-fd listeners
    stats timeout 30s
    user haproxy
    group haproxy
    daemon
    
    # SSL配置
    ca-base /etc/ssl/certs
    crt-base /etc/ssl/private
    
    ssl-default-bind-ciphers ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS
    ssl-default-bind-options no-sslv3
    
defaults
    log global
    mode tcp
    option tcplog
    option dontlognull
    timeout connect 5000
    timeout client 50000
    timeout server 50000
    errorfile 400 /etc/haproxy/errors/400.http
    errorfile 403 /etc/haproxy/errors/403.http
    errorfile 408 /etc/haproxy/errors/408.http
    errorfile 500 /etc/haproxy/errors/500.http
    errorfile 502 /etc/haproxy/errors/502.http
    errorfile 503 /etc/haproxy/errors/503.http
    errorfile 504 /etc/haproxy/errors/504.http

frontend kubernetes-frontend
    bind *:6443 ssl crt /etc/kubernetes/pki/apiserver.crt
    mode tcp
    option tcplog
    default_backend kubernetes-backend

backend kubernetes-backend
    mode tcp
    balance roundrobin
    option tcp-check
    tcp-check connect port 6443 ssl
    tcp-check send-binary 160301006f0100006b0303 # TLS Client Hello
    tcp-check expect binary 160303004a020000460303 # TLS Server Hello
    
    server control-1 control-1:6443 check fall 3 rise 2 inter 2000
    server control-2 control-2:6443 check fall 3 rise 2 inter 2000
    server control-3 control-3:6443 check fall 3 rise 2 inter 2000
    
    # 备用后端
    server backup-1 backup-1:6443 check backup
    server backup-2 backup-2:6443 check backup
```

#### Nginx配置示例

```nginx
# /etc/nginx/nginx.conf
stream {
    upstream kubernetes_apiserver {
        least_conn;
        server control-1:6443 max_fails=3 fail_timeout=30s;
        server control-2:6443 max_fails=3 fail_timeout=30s;
        server control-3:6443 max_fails=3 fail_timeout=30s;
        
        # 健康检查
        zone tcp_servers 64k;
        check interval=2000 rise=2 fall=3 timeout=1000 type=tcp;
    }
    
    server {
        listen 6443 ssl;
        proxy_pass kubernetes_apiserver;
        proxy_timeout 10m;
        proxy_responses 1;
        error_log /var/log/nginx/kubernetes_apiserver_error.log;
        
        # SSL配置
        ssl_certificate /etc/kubernetes/pki/apiserver.crt;
        ssl_certificate_key /etc/kubernetes/pki/apiserver.key;
        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_ciphers ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384;
        ssl_prefer_server_ciphers off;
    }
}

http {
    # 健康检查端点
    server {
        listen 8080;
        
        location /healthz {
            access_log off;
            return 200 "healthy\n";
            add_header Content-Type text/plain;
        }
        
        location /stats {
            stub_status on;
            access_log off;
            allow 127.0.0.1;
            deny all;
        }
    }
}
```

### 4.2 健康检查配置

```yaml
# Kubernetes ServiceMonitor配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kubernetes-apiserver
  namespace: monitoring
spec:
  endpoints:
  - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    interval: 30s
    metricRelabelings:
    - action: keep
      regex: apiserver_request_(latency|count|duration)|etcd_(server|disk|network)
      sourceLabels:
      - __name__
    port: https
    scheme: https
    tlsConfig:
      caFile: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      serverName: kubernetes
  jobLabel: component
  namespaceSelector:
    matchNames:
    - default
  selector:
    matchLabels:
      component: apiserver
      provider: kubernetes
```

---

## 5. 故障切换机制

### 5.1 etcd故障切换

```
etcd故障检测与切换流程:

┌─────────────────────────────────────────────────────────────────────────┐
│                        etcd Failure Detection                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. 健康检查 (每1秒)                                                    │
│     ├── TCP连接检查                                                      │
│     ├── HTTP /health 端点检查                                            │
│     └── Raft状态检查                                                     │
│                                                                          │
│  2. 故障检测 (连续3次失败)                                              │
│     ├── 标记节点为unhealthy                                              │
│     ├── 从负载均衡器移除                                                 │
│     └── 触发告警                                                         │
│                                                                          │
│  3. 自动恢复机制                                                        │
│     ├── Raft协议自动重新选举Leader                                       │
│     ├── 数据从healthy节点同步                                            │
│     └── 故障节点自动重新加入集群                                         │
│                                                                          │
│  4. 人工干预场景                                                        │
│     ├── 磁盘故障                                                         │
│     ├── 网络分区                                                         │
│     └── 硬件故障                                                         │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 API Server故障切换

```go
// API Server健康检查端点
func healthzHandler(w http.ResponseWriter, r *http.Request) {
    // 1. 检查etcd连接
    if err := checkEtcdHealth(); err != nil {
        http.Error(w, fmt.Sprintf("etcd unhealthy: %v", err), http.StatusServiceUnavailable)
        return
    }
    
    // 2. 检查关键组件
    if err := checkComponentHealth(); err != nil {
        http.Error(w, fmt.Sprintf("component unhealthy: %v", err), http.StatusServiceUnavailable)
        return
    }
    
    // 3. 返回健康状态
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("ok"))
}

// 负载均衡器健康检查脚本
#!/bin/bash
# /usr/local/bin/check-apiserver.sh

APISERVER_ENDPOINT="https://localhost:6443/healthz"
TIMEOUT=5

# 执行健康检查
response=$(curl -sk --max-time $TIMEOUT $APISERVER_ENDPOINT 2>/dev/null)

if [[ "$response" == "ok" ]]; then
    exit 0  # Healthy
else
    exit 1  # Unhealthy
fi
```

### 5.3 控制器故障切换

```
控制器Leader选举机制:

┌─────────────────────────────────────────────────────────────────────────┐
│                     Controller Leader Election                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. Lease API实现                                                       │
│     apiVersion: coordination.k8s.io/v1                                  │
│     kind: Lease                                                         │
│     metadata:                                                           │
│       name: kube-controller-manager                                     │
│       namespace: kube-system                                            │
│     spec:                                                               │
│       holderIdentity: controller-1_1234567890abcdef                     │
│       leaseDurationSeconds: 15                                          │
│       renewDeadlineSeconds: 10                                          │
│       leaseTransitions: 3                                               │
│                                                                          │
│  2. 选举流程                                                            │
│     ├── 启动时尝试获取Lease                                              │
│     ├── 定期续约(每10秒)                                                 │
│     ├── 监控Lease变化                                                    │
│     └── 失去Lease时主动释放资源                                          │
│                                                                          │
│  3. 故障切换时间                                                        │
│     ├── 检测到故障: < 15秒                                               │
│     ├── 新Leader选举: < 30秒                                             │
│     └── 服务恢复: < 1分钟                                                │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 6. 监控告警配置

### 6.1 关键监控指标

```yaml
# Prometheus告警规则
groups:
- name: kubernetes.controlplane
  rules:
  # API Server告警
  - alert: APIServerDown
    expr: up{job="apiserver"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "API Server is down"
      description: "Kubernetes API Server has been down for more than 5 minutes"

  - alert: APIServerLatencyHigh
    expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "API Server latency is high"
      description: "API Server 99th percentile latency is above 1 second"

  # etcd告警
  - alert: EtcdInsufficientMembers
    expr: count(etcd_server_has_leader) < 3
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd cluster has insufficient members"
      description: "etcd cluster has {{ $value }} members, less than required 3"

  - alert: EtcdHighCommitDurations
    expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.25
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "etcd high commit durations"
      description: "etcd backend commit duration is high"

  # 控制器告警
  - alert: ControllerManagerDown
    expr: up{job="kube-controller-manager"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Controller Manager is down"
      description: "Kubernetes Controller Manager has been down for more than 5 minutes"

  - alert: SchedulerDown
    expr: up{job="kube-scheduler"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Scheduler is down"
      description: "Kubernetes Scheduler has been down for more than 5 minutes"

  # 节点告警
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="false"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Node is not ready"
      description: "Node {{ $labels.node }} has been not ready for more than 10 minutes"
```

### 6.2 Grafana仪表板配置

```json
{
  "dashboard": {
    "title": "Kubernetes Control Plane Overview",
    "panels": [
      {
        "title": "API Server Availability",
        "type": "stat",
        "targets": [
          {
            "expr": "up{job=\"apiserver\"}",
            "legendFormat": "API Server Up"
          }
        ]
      },
      {
        "title": "etcd Cluster Health",
        "type": "graph",
        "targets": [
          {
            "expr": "etcd_server_has_leader",
            "legendFormat": "Has Leader"
          },
          {
            "expr": "etcd_server_leader_changes_seen_total",
            "legendFormat": "Leader Changes"
          }
        ]
      },
      {
        "title": "Control Plane Latency",
        "type": "heatmap",
        "targets": [
          {
            "expr": "rate(apiserver_request_duration_seconds_sum[5m]) / rate(apiserver_request_duration_seconds_count[5m])",
            "legendFormat": "Avg Latency"
          }
        ]
      }
    ]
  }
}
```

---

## 7. 性能调优建议

### 7.1 资源分配建议

| 组件 | CPU请求 | CPU限制 | 内存请求 | 内存限制 | 存储类型 |
|------|---------|---------|----------|----------|----------|
| **etcd** | 2核 | 4核 | 8GB | 16GB | NVMe SSD |
| **API Server** | 2核 | 4核 | 8GB | 16GB | SSD |
| **Controller Manager** | 1核 | 2核 | 2GB | 4GB | SSD |
| **Scheduler** | 1核 | 2核 | 2GB | 4GB | SSD |
| **Load Balancer** | 1核 | 2核 | 1GB | 2GB | - |

### 7.2 网络优化

```bash
# 系统网络参数调优
cat >> /etc/sysctl.conf << EOF
# TCP优化
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 134217728
net.ipv4.tcp_wmem = 4096 65536 134217728
net.ipv4.tcp_congestion_control = bbr
net.ipv4.tcp_slow_start_after_idle = 0

# 网络连接优化
net.core.netdev_max_backlog = 5000
net.core.somaxconn = 65535
net.ipv4.ip_local_port_range = 1024 65535

# 文件描述符
fs.file-max = 2097152
EOF

# 应用配置
sysctl -p

# ulimit设置
echo "* soft nofile 1048576" >> /etc/security/limits.conf
echo "* hard nofile 1048576" >> /etc/security/limits.conf
```

---

## 8. 生产环境最佳实践

### 8.1 部署清单

```yaml
# 生产环境部署检查清单
production_checklist:
  infrastructure:
    - [ ] 3+控制平面节点，跨故障域部署
    - [ ] 专用网络，与工作节点网络隔离
    - [ ] 高性能存储(NVMe SSD)
    - [ ] 冗余网络连接(双网卡bonding)
    - [ ] UPS电源保护
  
  security:
    - [ ] 启用TLS双向认证
    - [ ] 定期证书轮换(90天)
    - [ ] 网络策略限制访问
    - [ ] 审计日志启用并集中存储
    - [ ] RBAC权限最小化原则
  
  monitoring:
    - [ ] Prometheus监控部署
    - [ ] Grafana仪表板配置
    - [ ] 告警规则完善
    - [ ] 日志收集系统
    - [ ] 性能基线建立
  
  backup:
    - [ ] etcd定期备份(每日)
    - [ ] 备份验证机制
    - [ ] 灾难恢复演练
    - [ ] 多地域备份存储
    - [ ] 备份保留策略
  
  maintenance:
    - [ ] 定期健康检查
    - [ ] 性能基准测试
    - [ ] 安全补丁及时更新
    - [ ] 文档完善更新
    - [ ] 团队培训计划
```

### 8.2 故障演练方案

```bash
#!/bin/bash
# control-plane-failure-test.sh

set -euo pipefail

echo "=== Kubernetes Control Plane Failure Test ==="

# 测试1: 单个API Server故障
echo "Test 1: Simulating API Server failure..."
kubectl drain control-1 --ignore-daemonsets --delete-emptydir-data
sleep 60
kubectl uncordon control-1
echo "✓ API Server failover test completed"

# 测试2: etcd节点故障
echo "Test 2: Simulating etcd node failure..."
docker stop etcd-control-2
sleep 120
docker start etcd-control-2
echo "✓ etcd failover test completed"

# 测试3: 网络分区模拟
echo "Test 3: Simulating network partition..."
iptables -A INPUT -s control-3 -j DROP
sleep 120
iptables -D INPUT -s control-3 -j DROP
echo "✓ Network partition test completed"

# 验证集群状态
echo "Verifying cluster health..."
kubectl get nodes
kubectl get pods -A
echo "✓ All tests completed successfully"
```

通过实施这些高可用部署模式和最佳实践，可以显著提升Kubernetes控制平面的可靠性和稳定性，确保生产环境的持续可用性。