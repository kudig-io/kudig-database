# 17 - 分布式共识与etcd原理 (Distributed Consensus & etcd)

## 生产环境etcd运维最佳实践

### 企业级etcd架构设计

#### 高可用etcd集群架构
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    企业级etcd高可用架构                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  客户端层 (Client Layer)                                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • API Server (多个实例)                                              │   │
│  │ • Controller Manager                                                 │   │
│  │ • Scheduler                                                          │   │
│  │ • 自定义控制器                                                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│           │                                                                  │
│           ▼                                                                  │
│  负载均衡层 (Load Balancer)                                                 │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • DNS SRV记录                                                        │   │
│  │ • Keepalived VIP                                                     │   │
│  │ • 外部负载均衡器                                                     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│           │                                                                  │
│           ▼                                                                  │
│  etcd集群层 (etcd Cluster)                                                  │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  etcd-1 (Leader)        etcd-2 (Follower)      etcd-3 (Follower)    │   │
│  │  ┌─────────────┐       ┌─────────────┐        ┌─────────────┐      │   │
│  │  │   Member    │       │   Member    │        │   Member    │      │   │
│  │  │  • WAL日志   │       │  • WAL日志   │        │  • WAL日志   │      │   │
│  │  │  • 数据快照  │       │  • 数据快照  │        │  • 数据快照  │      │   │
│  │  │  • 网络通信  │       │  • 网络通信  │        │  • 网络通信  │      │   │
│  │  └─────────────┘       └─────────────┘        └─────────────┘      │   │
│  │         │                       │                      │             │   │
│  │         └───────────────────────┼──────────────────────┘             │   │
│  │                                 │                                    │   │
│  │                    ┌─────────────────────┐                           │   │
│  │                    │   Raft共识协议       │                           │   │
│  │                    │  • Leader选举        │                           │   │
│  │                    │  • 日志复制          │                           │   │
│  │                    │  • 安全性保证        │                           │   │
│  │                    └─────────────────────┘                           │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  存储层 (Storage Layer)                                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • 本地SSD存储                                                        │   │
│  │ • RAID 10配置                                                        │   │
│  │ • 定期备份策略                                                       │   │
│  │ • 灾难恢复方案                                                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### etcd性能优化配置

#### 生产环境配置模板
```yaml
# etcd生产环境配置
apiVersion: v1
kind: Pod
metadata:
  name: etcd-member
  namespace: kube-system
spec:
  containers:
    - name: etcd
      image: k8s.gcr.io/etcd:3.5.6-0
      command:
        - etcd
        - --name=$(ETCD_NAME)
        - --data-dir=/var/lib/etcd
        - --listen-client-urls=https://0.0.0.0:2379
        - --advertise-client-urls=https://$(ETCD_NAME):2379
        - --listen-peer-urls=https://0.0.0.0:2380
        - --initial-advertise-peer-urls=https://$(ETCD_NAME):2380
        - --initial-cluster-token=etcd-cluster-1
        - --initial-cluster=$(ETCD_INITIAL_CLUSTER)
        - --initial-cluster-state=new
        - --client-cert-auth=true
        - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
        - --cert-file=/etc/kubernetes/pki/etcd/server.crt
        - --key-file=/etc/kubernetes/pki/etcd/server.key
        - --peer-client-cert-auth=true
        - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
        - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
        - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
        - --auto-compaction-mode=revision
        - --auto-compaction-retention=1000
        - --quota-backend-bytes=8589934592  # 8GB
        - --heartbeat-interval=100          # 100ms
        - --election-timeout=1000           # 1000ms
        - --snapshot-count=10000            # 10000条日志后快照
        - --max-request-bytes=1572864       # 1.5MB
        - --grpc-keepalive-min-time=5s
        - --grpc-keepalive-interval=2h
        - --grpc-keepalive-timeout=20s

      env:
        - name: ETCD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: ETCD_INITIAL_CLUSTER
          value: "etcd-0=https://etcd-0:2380,etcd-1=https://etcd-1:2380,etcd-2=https://etcd-2:2380"

      volumeMounts:
        - name: etcd-data
          mountPath: /var/lib/etcd
        - name: etcd-certs
          mountPath: /etc/kubernetes/pki/etcd
          readOnly: true

      resources:
        requests:
          memory: "512Mi"
          cpu: "500m"
        limits:
          memory: "2Gi"
          cpu: "1000m"

      livenessProbe:
        exec:
          command:
            - /bin/sh
            - -ec
            - ETCDCTL_API=3 etcdctl --endpoints=https://[127.0.0.1]:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key get foo
        initialDelaySeconds: 15
        timeoutSeconds: 15

      readinessProbe:
        exec:
          command:
            - /bin/sh
            - -ec
            - ETCDCTL_API=3 etcdctl --endpoints=https://[127.0.0.1]:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt --key=/etc/kubernetes/pki/etcd/healthcheck-client.key member list
        initialDelaySeconds: 15
        timeoutSeconds: 15

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

### etcd监控与告警

#### 关键监控指标
```yaml
# etcd监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: etcd-monitoring-rules
  namespace: monitoring
spec:
  groups:
    - name: etcd.rules
      rules:
        # 集群健康状态
        - alert: EtcdClusterUnhealthy
          expr: up{job="etcd"} < 3
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "etcd集群不健康"
            description: "只有 {{ $value }} 个etcd成员在线，少于所需的3个"

        # 领导者变更
        - alert: EtcdLeaderChange
          expr: increase(etcd_server_leader_changes_seen_total[1h]) > 2
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "etcd领导者频繁变更"
            description: "过去1小时领导者变更了 {{ $value }} 次"

        # 磁盘空间
        - alert: EtcdDiskSpaceLow
          expr: etcd_disk_backend_commit_duration_seconds > 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "etcd磁盘IO延迟高"
            description: "etcd提交延迟为 {{ $value }} 秒"

        # 数据库大小
        - alert: EtcdDatabaseSizeExceeded
          expr: etcd_debugging_mvcc_db_total_size_in_bytes > 2147483648  # 2GB
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "etcd数据库过大"
            description: "etcd数据库大小为 {{ $value }} 字节"

        # GRPC请求延迟
        - alert: EtcdGrpcSlow
          expr: histogram_quantile(0.99, rate(etcd_grpc_unary_requests_duration_seconds_bucket[5m])) > 0.15
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "etcd GRPC请求缓慢"
            description: "99%的GRPC请求延迟超过 {{ $value }} 秒"
```

#### etcd诊断脚本
```bash
#!/bin/bash
# etcd生产环境诊断脚本

echo "=== etcd集群诊断报告 ==="

# 1. 集群成员状态
echo "1. 集群成员状态:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  member list -w table
echo

# 2. 集群健康检查
echo "2. 集群健康检查:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health -w table
echo

# 3. 性能基准测试
echo "3. 性能基准测试:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  check perf
echo

# 4. 数据库大小和状态
echo "4. 数据库状态:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status -w table
echo

# 5. 压缩状态检查
echo "5. 压缩状态:"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  get / --prefix --keys-only --limit=10
echo

# 6. 生成维护建议
echo "6. 维护建议:"
cat << EOF
etcd维护建议:
1. 定期备份: 使用etcdctl snapshot save
2. 监控磁盘空间: 确保至少20%空闲空间
3. 定期压缩: 清理历史版本数据
4. 监控网络延迟: 成员间延迟应<10ms
5. 定期更新: 跟进安全补丁和版本升级
EOF
```

### etcd备份与恢复策略

#### 自动备份配置
```yaml
# etcd备份CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: etcd-backup
  namespace: kube-system
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: etcd-backup
              image: k8s.gcr.io/etcd:3.5.6-0
              command:
                - /bin/sh
                - -c
                - |
                  ETCDCTL_API=3 etcdctl \
                    --endpoints=https://etcd-client:2379 \
                    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
                    --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
                    --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
                    snapshot save /backup/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db
                  
                  # 清理7天前的备份
                  find /backup -name "etcd-snapshot-*" -mtime +7 -delete
              
              volumeMounts:
                - name: backup-storage
                  mountPath: /backup
                - name: etcd-certs
                  mountPath: /etc/kubernetes/pki/etcd
                  readOnly: true
          
          restartPolicy: OnFailure
          
          volumes:
            - name: backup-storage
              persistentVolumeClaim:
                claimName: etcd-backup-pvc
            - name: etcd-certs
              hostPath:
                path: /etc/kubernetes/pki/etcd
                type: Directory
```

#### 灾难恢复流程
```bash
#!/bin/bash
# etcd灾难恢复脚本

set -e

BACKUP_FILE="$1"
NEW_CLUSTER="${2:-false}"

if [ -z "$BACKUP_FILE" ]; then
    echo "用法: $0 <备份文件> [new-cluster]"
    echo "示例: $0 /backup/etcd-snapshot-20240115-020000.db true"
    exit 1
fi

echo "开始etcd恢复流程..."
echo "备份文件: $BACKUP_FILE"
echo "新建集群: $NEW_CLUSTER"

# 1. 停止所有etcd实例
echo "1. 停止etcd服务..."
systemctl stop etcd || true

# 2. 清理现有数据
echo "2. 清理现有数据..."
rm -rf /var/lib/etcd/member

# 3. 恢复备份
echo "3. 恢复备份数据..."
ETCDCTL_API=3 etcdctl snapshot restore "$BACKUP_FILE" \
  --data-dir=/var/lib/etcd \
  --name=$(hostname) \
  --initial-cluster=$(hostname)=https://$(hostname):2380 \
  --initial-cluster-token=etcd-cluster-1 \
  --initial-advertise-peer-urls=https://$(hostname):2380

# 4. 如果是新建集群，更新配置
if [ "$NEW_CLUSTER" = "true" ]; then
    echo "4. 配置新集群..."
    # 更新initial-cluster-state为new
    sed -i 's/--initial-cluster-state=existing/--initial-cluster-state=new/' /etc/kubernetes/manifests/etcd.yaml
fi

# 5. 启动etcd
echo "5. 启动etcd服务..."
systemctl start etcd

# 6. 验证恢复
echo "6. 验证恢复状态..."
sleep 10
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

echo "etcd恢复完成!"
```

### etcd安全加固

#### 网络安全配置
```yaml
# etcd网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: etcd-network-policy
  namespace: kube-system
spec:
  podSelector:
    matchLabels:
      component: etcd
  policyTypes:
    - Ingress
    - Egress
  
  ingress:
    # 允许API Server访问
    - from:
        - podSelector:
            matchLabels:
              component: kube-apiserver
      ports:
        - protocol: TCP
          port: 2379
    
    # 允许etcd成员间通信
    - from:
        - podSelector:
            matchLabels:
              component: etcd
      ports:
        - protocol: TCP
          port: 2379
        - protocol: TCP
          port: 2380
  
  egress:
    # 允许etcd成员间通信
    - to:
        - podSelector:
            matchLabels:
              component: etcd
      ports:
        - protocol: TCP
          port: 2379
        - protocol: TCP
          port: 2380
```

#### 证书管理最佳实践
```bash
#!/bin/bash
# etcd证书轮换脚本

# 备份当前证书
cp -r /etc/kubernetes/pki/etcd /etc/kubernetes/pki/etcd.backup.$(date +%Y%m%d)

# 生成新证书
openssl genrsa -out /etc/kubernetes/pki/etcd/server.key 2048
openssl req -new -key /etc/kubernetes/pki/etcd/server.key -out /etc/kubernetes/pki/etcd/server.csr -subj "/CN=etcd-server"
openssl x509 -req -in /etc/kubernetes/pki/etcd/server.csr -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key -CAcreateserial -out /etc/kubernetes/pki/etcd/server.crt -days 365

# 重启etcd
systemctl restart etcd

# 验证证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -text -noout
```

### 最佳实践总结

#### 生产环境部署要点
1. **硬件要求**：专用SSD存储，至少4核CPU，8GB内存
2. **网络要求**：低延迟网络(<10ms)，带宽充足
3. **集群规模**：奇数个节点(3,5,7)，避免偶数
4. **备份策略**：每日全量备份，保留30天
5. **监控告警**：关键指标全覆盖，设置合理阈值
6. **安全配置**：启用TLS，网络策略限制，定期证书轮换

#### 性能调优建议
1. **磁盘优化**：使用SSD，RAID 10配置
2. **参数调优**：根据负载调整心跳和选举超时
3. **压缩策略**：定期自动压缩，控制数据库大小
4. **资源限制**：合理设置CPU和内存限制
5. **网络优化**：就近部署，减少网络延迟
| 3. 请求投票 | 向其他节点发送RequestVote |
| 4. 收集选票 | 等待多数派响应 |
| 5a. 当选 | 获得多数票,成为Leader |
| 5b. 落选 | 收到更高term,退回Follower |
| 5c. 平局 | 超时重新选举 |

## Raft日志复制

| 阶段 | 操作 |
|-----|------|
| 1 | Client发送写请求到Leader |
| 2 | Leader追加日志条目(uncommitted) |
| 3 | Leader并行发送AppendEntries给Followers |
| 4 | Followers追加日志,返回成功 |
| 5 | Leader收到多数确认,提交(commit)日志 |
| 6 | Leader响应Client成功 |
| 7 | 后续心跳通知Followers提交 |

## etcd核心特性

| 特性 | 说明 |
|-----|------|
| 强一致性 | 基于Raft保证 |
| Watch机制 | 监听key变化 |
| MVCC | 多版本并发控制 |
| 事务 | 支持原子事务操作 |
| TTL | 键值过期时间 |
| Lease | 租约机制 |

## etcd数据模型

| 概念 | 说明 |
|-----|------|
| Key-Value | 扁平化键值存储 |
| Revision | 全局递增版本号 |
| ModRevision | 键的最后修改版本 |
| CreateRevision | 键的创建版本 |
| Version | 键的修改次数 |

### K8s在etcd中的存储结构

```
/registry/
├── pods/
│   ├── default/
│   │   ├── nginx-abc123
│   │   └── nginx-def456
│   └── kube-system/
│       └── coredns-xyz789
├── deployments/
│   └── default/
│       └── nginx
├── services/
│   └── default/
│       └── kubernetes
├── secrets/
│   └── default/
│       └── default-token-xxxxx
└── ...
```

## etcd集群架构

| 配置 | 节点数 | 容错能力 | 说明 |
|-----|-------|---------|------|
| 单节点 | 1 | 0 | 开发测试 |
| 最小HA | 3 | 1 | 生产最小配置 |
| 推荐HA | 5 | 2 | 生产推荐 |
| 大规模 | 7 | 3 | 大型集群 |

> 公式: 容忍故障节点数 = (N-1)/2

## etcd性能指标

| 指标 | 建议值 | 说明 |
|-----|-------|------|
| 磁盘延迟 | <10ms | WAL写入延迟 |
| 网络延迟 | <10ms | 节点间RTT |
| IOPS | >3000 | 磁盘IO能力 |
| 磁盘类型 | SSD | 必须使用SSD |

## etcd运维操作

| 操作 | 命令 |
|-----|------|
| 查看成员 | `etcdctl member list` |
| 查看状态 | `etcdctl endpoint status` |
| 健康检查 | `etcdctl endpoint health` |
| 添加成员 | `etcdctl member add <name> --peer-urls=<url>` |
| 移除成员 | `etcdctl member remove <id>` |
| 备份 | `etcdctl snapshot save backup.db` |
| 恢复 | `etcdctl snapshot restore backup.db` |
| 碎片整理 | `etcdctl defrag` |

### etcd备份脚本

```bash
#!/bin/bash
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d%H%M).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

## etcd与API Server交互

| 操作 | API Server | etcd |
|-----|-----------|------|
| 创建资源 | POST /api/v1/pods | Put key |
| 读取资源 | GET /api/v1/pods/name | Get key |
| 更新资源 | PUT /api/v1/pods/name | Txn (compare+put) |
| 删除资源 | DELETE /api/v1/pods/name | Delete key |
| 监听变化 | Watch /api/v1/pods | Watch prefix |

## etcd调优参数

| 参数 | 默认值 | 建议值 | 说明 |
|-----|-------|-------|------|
| quota-backend-bytes | 2GB | 8GB | 存储配额 |
| snapshot-count | 100000 | 10000 | 快照触发条数 |
| heartbeat-interval | 100ms | 100ms | 心跳间隔 |
| election-timeout | 1000ms | 1000ms | 选举超时 |
| auto-compaction-retention | 0 | 1h | 自动压缩保留 |

## 常见问题

| 问题 | 原因 | 解决 |
|-----|------|------|
| 空间不足 | 超过quota | 压缩+碎片整理 |
| 选举频繁 | 网络/磁盘慢 | 优化基础设施 |
| 延迟高 | 磁盘IOPS不足 | 使用SSD |
| 脑裂 | 网络分区 | 检查网络 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
