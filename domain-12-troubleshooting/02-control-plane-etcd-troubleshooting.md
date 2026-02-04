# 02 - etcd 故障排查 (etcd Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [etcd.io/docs](https://etcd.io/docs/)

---

## 1. etcd 故障诊断总览 (etcd Diagnosis Overview)

### 1.1 etcd 常见故障类型

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **集群不可用** | etcdctl无法连接、leader选举失败 | 数据存储层瘫痪 | P0 - 紧急 |
| **数据不一致** | 读写异常、key-value不一致 | 数据可靠性受损 | P0 - 紧急 |
| **性能下降** | 读写延迟高、吞吐量下降 | API响应变慢 | P1 - 高 |
| **磁盘空间不足** | no space left on device | 写入操作失败 | P1 - 高 |
| **成员故障** | member unhealthy、follower lag | 集群稳定性下降 | P2 - 中 |
| **网络分区** | 网络隔离、脑裂现象 | 集群分裂风险 | P2 - 中 |

### 1.2 etcd 架构关键组件

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         etcd 故障诊断架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       Kubernetes 组件                                   │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │ API Server  │  │ Scheduler   │  │ Controller  │  │ kubelet     │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    etcd Client API (gRPC)                            │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Raft协议    │    │   存储引擎    │    │   网络层     │                   │
│  │ (共识算法)   │    │ (boltdb)    │    │ (gRPC)     │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    etcd 集群成员 (通常3/5/7个)                        │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │  │
│  │  │   etcd-1    │  │   etcd-2    │  │   etcd-3    │                  │  │
│  │  │ (Leader)    │  │ (Follower)  │  │ (Follower)  │                  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      持久化存储 (磁盘)                                │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │  │
│  │  │ WAL日志文件  │  │ Snapshot快照 │  │ 数据库文件   │                  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. etcd 集群不可用故障排查 (Cluster Unavailability)

### 2.1 故障诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      etcd 集群不可用诊断流程                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Kubernetes API Server 无法访问etcd                                        │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查本地etcd进程状态                          │                 │
│   │ systemctl status etcd                                │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 进程未运行 ──▶ 启动etcd服务                                 │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查etcd端口监听                              │                 │
│   │ ss -tlnp | grep 2379                                 │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 端口未监听 ──▶ 检查配置/防火墙                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查集群成员状态                              │                 │
│   │ ETCDCTL_API=3 etcdctl member list                    │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 成员列表异常 ──▶ 检查集群配置                                │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查集群健康状态                              │                 │
│   │ ETCDCTL_API=3 etcdctl endpoint health --cluster      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 健康检查失败 ──▶ 转具体成员故障排查                          │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 5: 检查磁盘空间和IO                              │                 │
│   │ df -h /var/lib/etcd                                  │                 │
│   │ iotop -ao                                            │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 详细诊断命令

```bash
# ========== 1. 基础状态检查 ==========

# 检查etcd服务状态
systemctl status etcd
systemctl is-active etcd

# 检查etcd进程
ps aux | grep etcd
netstat -tlnp | grep etcd

# 检查端口监听
ss -tlnp | grep -E "2379|2380"

# ========== 2. 集群状态检查 ==========

# 设置etcdctl环境变量
export ETCDCTL_API=3
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379

# 检查集群成员
etcdctl member list -w table

# 检查集群健康状态
etcdctl endpoint health --cluster

# 检查集群状态详情
etcdctl endpoint status --cluster -w table

# ========== 3. 领导者选举状态 ==========

# 查看当前领导者
etcdctl endpoint status --cluster -w json | jq '.[] | select(.leaderInfo.leader != "0")'

# 检查Raft状态
etcdctl endpoint status --cluster -w json | jq '.[].raftTerm'
etcdctl endpoint status --cluster -w json | jq '.[].raftIndex'
etcdctl endpoint status --cluster -w json | jq '.[].raftAppliedIndex'

# ========== 4. 磁盘和性能检查 ==========

# 检查磁盘空间
df -h /var/lib/etcd
du -sh /var/lib/etcd/*

# 检查WAL日志大小
ls -lh /var/lib/etcd/member/wal/

# 检查数据库大小
etcdctl endpoint status --cluster -w json | jq '.[].dbSize'

# 检查磁盘IO性能
iostat -x 1 5
iotop -ao

# ========== 5. 日志分析 ==========

# 查看etcd日志
journalctl -u etcd -f --no-pager

# 查看最近错误日志
journalctl -u etcd --since "1 hour ago" | grep -i "error\|warn\|panic"

# 检查wal日志损坏
etcdctl check perf --load="s"
```

### 2.3 常见错误及解决方案

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `etcdserver: no leader` | 集群分裂/多数成员故障 | 恢复多数成员或重新初始化 |
| `etcdserver: mvcc: database space exceeded` | 数据库空间满 | 清理旧版本(compact)+碎片整理 |
| `etcdserver: walpb: crc mismatch` | WAL日志损坏 | 从快照恢复或重新初始化 |
| `etcdserver: request timed out` | 网络延迟高/磁盘慢 | 检查网络和磁盘性能 |
| `etcdserver: failed to purge snap file` | 磁盘空间不足 | 清理磁盘空间 |
| `unhealthy cluster` | 成员间通信失败 | 检查网络连通性和防火墙 |

---

## 3. 数据一致性问题排查 (Data Consistency Issues)

### 3.1 数据一致性检查

```bash
# ========== 1. 数据一致性验证 ==========

# 检查集群数据一致性
ETCDCTL_API=3 etcdctl --write-out=table endpoint hashkv --cluster

# 比较各成员的数据哈希值
for endpoint in 127.0.0.1:2379 127.0.0.2:2379 127.0.0.3:2379; do
  echo "=== $endpoint ==="
  ETCDCTL_API=3 etcdctl --endpoints=https://$endpoint \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key \
    endpoint hashkv
done

# ========== 2. 版本检查 ==========

# 检查etcd版本
etcdctl version
etcd --version

# 检查集群版本兼容性
etcdctl endpoint status --cluster -w json | jq '.[].version'

# ========== 3. 数据完整性检查 ==========

# 检查数据库完整性
ETCDCTL_API=3 etcdctl --write-out=table endpoint status --cluster

# 检查告警状态
ETCDCTL_API=3 etcdctl alarm list

# 清除告警(如有必要)
ETCDCTL_API=3 etcdctl alarm disarm
```

### 3.2 数据恢复操作

```bash
# ========== 1. 数据备份 ==========

# 创建快照备份
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db

# 验证快照完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-snapshot-*.db -w table

# ========== 2. 数据恢复 ==========

# 停止etcd服务
systemctl stop etcd

# 清理旧数据(谨慎操作!)
mv /var/lib/etcd /var/lib/etcd.backup.$(date +%Y%m%d-%H%M%S)

# 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.1.10:2380 \
  --initial-cluster-token=etcd-cluster-1 \
  --initial-advertise-peer-urls=https://10.0.1.10:2380

# 启动etcd服务
systemctl start etcd

# ========== 3. 成员替换 ==========

# 移除故障成员
ETCDCTL_API=3 etcdctl member remove <member-id>

# 添加新成员
ETCDCTL_API=3 etcdctl member add etcd-new --peer-urls=https://10.0.1.11:2380

# 在新节点上启动etcd(使用返回的配置)
```

---

## 4. 性能问题排查 (Performance Issues)

### 4.1 性能监控指标

```bash
# ========== 1. 关键性能指标 ==========

# 获取etcd性能指标
curl -s https://localhost:2379/metrics --cert /etc/kubernetes/pki/etcd/server.crt --key /etc/kubernetes/pki/etcd/server.key --cacert /etc/kubernetes/pki/etcd/ca.crt | grep -E "(etcd_mvcc_db_total_size_in_bytes|etcd_disk_wal_fsync_duration_seconds|etcd_network_peer_round_trip_time_seconds)"

# 关键指标说明:
# etcd_mvcc_db_total_size_in_bytes: 数据库大小
# etcd_disk_wal_fsync_duration_seconds: WAL刷盘延迟
# etcd_network_peer_round_trip_time_seconds: 网络往返时间
# etcd_server_proposals_pending: 待处理提案数
# etcd_server_proposals_committed_total: 已提交提案数

# ========== 2. 延迟分析 ==========

# 检查WAL刷盘延迟
ETCDCTL_API=3 etcdctl check perf --load="s"

# 查看慢操作日志
journalctl -u etcd | grep -i "slow"

# ========== 3. 资源使用监控 ==========

# CPU和内存使用
top -p $(pgrep etcd)

# 磁盘IO统计
iostat -x 1 10

# 网络流量
iftop -i eth0
```

### 4.2 性能优化建议

```bash
# ========== 1. 数据库维护 ==========

# 压缩历史版本(定期执行)
ETCDCTL_API=3 etcdctl compact $(ETCDCTL_API=3 etcdctl get / --prefix --keys-only --limit=1 -w json | jq -r .header.revision)

# 碎片整理
ETCDCTL_API=3 etcdctl defrag --cluster

# ========== 2. 配置优化 ==========

# 调整etcd配置参数(/etc/etcd/etcd.conf)
ETCD_HEARTBEAT_INTERVAL=100      # 心跳间隔(ms)
ETCD_ELECTION_TIMEOUT=1000       # 选举超时(ms)
ETCD_QUOTA_BACKEND_BYTES=8589934592  # 数据库配额(8GB)
ETCD_AUTO_COMPACTION_RETENTION=1 # 自动压缩保留小时数

# ========== 3. 硬件优化 ==========

# 使用SSD存储
lsblk -d -o NAME,ROTA  # ROTA=0表示SSD

# 调整文件系统
tune2fs -l /dev/sdX | grep "Filesystem features"

# 调整系统参数
echo 'vm.swappiness=1' >> /etc/sysctl.conf
echo 'fs.file-max=1000000' >> /etc/sysctl.conf
```

---

## 5. 成员故障排查 (Member Failure)

### 5.1 单成员故障处理

```bash
# ========== 1. 识别故障成员 ==========

# 检查成员健康状态
ETCDCTL_API=3 etcdctl endpoint health --cluster

# 查看详细状态
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table

# ========== 2. 故障成员移除 ==========

# 获取故障成员ID
FAULTY_MEMBER_ID=$(ETCDCTL_API=3 etcdctl member list -w json | jq -r '.members[] | select(.name=="etcd-2") | .ID')

# 移除故障成员
ETCDCTL_API=3 etcdctl member remove $FAULTY_MEMBER_ID

# ========== 3. 添加新成员 ==========

# 在新节点上准备数据目录
mkdir -p /var/lib/etcd

# 添加成员到集群
NEW_MEMBER_ID=$(ETCDCTL_API=3 etcdctl member add etcd-new --peer-urls=https://10.0.1.12:2380 | grep "ETCD_INITIAL_CLUSTER=" | cut -d'"' -f2)

# 在新节点启动etcd(使用返回的配置)
```

### 5.2 网络分区处理

```bash
# ========== 1. 检测网络分区 ==========

# 检查成员间连通性
for member in etcd-1 etcd-2 etcd-3; do
  echo "=== Checking $member ==="
  ping -c 3 $member
  telnet $member 2379
  telnet $member 2380
done

# ========== 2. 脑裂恢复 ==========

# 检查各分区的成员数量
ETCDCTL_API=3 etcdctl member list --write-out=table

# 确保多数派存活(至少(N/2)+1个成员)
# 例如: 3节点集群需要至少2个节点在线

# 如果无法恢复多数派，需要强制重新初始化
# ⚠️ 此操作会导致数据丢失!
```

---

## 6. 生产环境应急处理 (Production Emergency Response)

### 6.1 紧急诊断脚本

```bash
#!/bin/bash
# etcd-emergency-check.sh

echo "=== etcd 紧急诊断报告 ==="
echo "时间: $(date)"
echo ""

# 1. 基础状态检查
echo "1. etcd服务状态:"
systemctl is-active etcd && echo "✅ 服务运行中" || echo "❌ 服务异常"

echo -e "\n2. 端口监听状态:"
ss -tlnp | grep -E "2379|2380" || echo "❌ 端口未监听"

# 2. 集群健康检查
echo -e "\n3. 集群健康状态:"
export ETCDCTL_API=3
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379

etcdctl endpoint health --cluster 2>/dev/null || echo "❌ 集群不健康"

# 3. 磁盘空间检查
echo -e "\n4. 磁盘空间使用:"
df -h /var/lib/etcd | tail -1

# 4. 数据库大小
echo -e "\n5. 数据库大小:"
etcdctl endpoint status --cluster -w json 2>/dev/null | jq -r '.[].dbSize' | numfmt --to=iec

# 5. 最近错误日志
echo -e "\n6. 最近错误日志:"
journalctl -u etcd --since "10 minutes ago" | grep -i "error\|warn" | tail -5

echo -e "\n=== 诊断完成 ==="
```

### 6.2 故障恢复优先级

| 故障类型 | 恢复优先级 | 时间要求 | 操作步骤 |
|---------|-----------|---------|---------|
| **完全不可用** | 最高 | 30分钟内 | 1. 检查多数派 2. 恢复服务 3. 数据验证 |
| **性能严重下降** | 高 | 2小时内 | 1. 性能分析 2. 资源扩容 3. 参数调优 |
| **单成员故障** | 中 | 4小时内 | 1. 移除故障成员 2. 添加新成员 3. 数据同步 |
| **磁盘空间不足** | 高 | 1小时内 | 1. 紧急compact 2. 碎片整理 3. 扩容存储 |

---

## 7. 预防措施与最佳实践 (Prevention & Best Practices)

### 7.1 监控告警配置

```yaml
# Prometheus告警规则
groups:
- name: etcd.rules
  rules:
  # etcd不可用
  - alert: EtcdDown
    expr: up{job="etcd"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "etcd实例 {{ $labels.instance }} 不可用"
  
  # etcd数据库空间不足
  - alert: EtcdDatabaseSpaceExceeded
    expr: etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.8
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd数据库空间使用率超过80%"
  
  # etcd成员不健康
  - alert: EtcdMemberUnhealthy
    expr: etcd_server_has_leader == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "etcd成员 {{ $labels.instance }} 无领导者"
```

### 7.2 运维检查清单

- [ ] 定期备份etcd数据（每日）
- [ ] 监控磁盘空间使用率（阈值80%）
- [ ] 验证集群成员健康状态
- [ ] 检查WAL日志增长趋势
- [ ] 定期执行数据库压缩和碎片整理
- [ ] 测试灾难恢复流程
- [ ] 保持etcd版本更新

---