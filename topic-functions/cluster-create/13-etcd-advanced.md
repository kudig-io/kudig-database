# etcd 进阶: 数据存储与维护

## 源码路径

`cmd/kubeadm/app/phases/etcd/`
`pkg/storageos/` (etcd 存储层)

---

## etcd 数据目录结构

```
/var/lib/etcd/
└── member/
    ├── wal/           # Write-Ahead Log
    │   ├── 0.db
    │   └── 0.db.wal/
    ├── snap/          # Snapshot
    │   ├── 0000000000000001-0000000000000001.snap
    │   └── 0000000000000002-0000000000000002.snap
    └── kv.db          # B-tree 数据库文件 (LevelDB/RocksDB)
```

---

## WAL (Write-Ahead Log)

```go
// WAL 记录所有写入操作
// 用于:
type WAL struct {
    // 1. 崩溃恢复: 重放未写入快照的日志
    // 2. 日志条目大小: 每个条目 ~500 bytes
    // 3. 默认每个 10000 条创建 snapshot
}
```

**性能影响**: WAL 写入是同步的，SSD 是必须的。

---

## Snapshot (快照)

```go
// Snapshot 定期将内存状态写入磁盘
// 用于:
// 1. 减少 WAL 文件大小
// 2. 加速启动恢复
// 3. 节点间数据同步 (snapshot 传输替代逐条重放)

// 默认配置:
// - snapcount: 10000 (每 10000 条日志创建 snapshot)
// - max-snapshots: 5 (保留最近 5 个快照)
```

---

## 数据压缩 (Defragmentation)

etcd 使用 LSM-tree 或 B-tree 存储，删除数据后空间不立即释放:

```bash
# 查看 etcd 数据大小
du -sh /var/lib/etcd/member/

# 手动压缩 (清理历史版本)
ETCDCTL_API=3 etcdctl compact 12345

# 手动 defrag (释放磁盘空间)
ETCDCTL_API=3 etcdctl defrag

# 自动 defrag (etcd 3.4+ 默认开启)
# 后台自动检测并 defrag 碎片
```

---

## etcd 健康检查详解

```go
// 健康检查端点:
// 1. /health (HTTP)
// 2. /v2/members (API)
// 3. /v2/stats/self (统计信息)
```

```bash
# 检查 etcd 健康状态
curl -k https://127.0.0.1:2379/health

# 查看成员列表
ETCDCTL_API=3 etcdctl member list -w table

# 输出:
# +------------------+---------+--------+----------------------------+----------------+
# |        ID        | STATUS  |  NAME  |         PEER ADDRS         |   CLIENT ADDRS |
# +------------------+---------+--------+----------------------------+----------------+
# | 8e9e05c52164694d | started | master | https://192.168.1.1:2380   | https://192.168.1.1:2379 |
# +------------------+---------+--------+----------------------------+----------------+
```

---

## etcd 读写流程

```
写操作 (PUT):
Client → API Server → etcd Client → etcd Server → WAL → Apply → DB

读操作 (GET):
Client → API Server → etcd Client → etcd Server → DB (内存缓存)
```

---

## etcd 选举机制

```go
// raft 协议选举:
// 1. 初始状态: 所有节点都是 Follower
// 2. 选举超时: 随机 150-300ms
// 3. 超时后成为 Candidate，发起选举
// 4. 获得多数票 (>50%) 成为 Leader
// 5. Leader 向 Follower 发送心跳 (心跳间隔: 100ms)

// 关键超时参数:
// - election-timeout: 1000ms (默认)
// - heartbeat-interval: 100ms
```

---

## etcd 成员变更

```bash
# 添加新成员
ETCDCTL_API=3 etcdctl member add new-node --peer-urls=https://192.168.1.4:2380

# 移除成员
ETCDCTL_API=3 etcdctl member remove <member-id>

# 更新成员地址
ETCDCTL_API=3 etcdctl member update <member-id> --peer-urls=https://192.168.1.5:2380
```

---

## 备份与恢复

```bash
# 备份 (快照)
ETCDCTL_API=3 etcdctl snapshot save backup.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 从备份恢复
ETCDCTL_API=3 etcdctl snapshot restore backup.db \
  --data-dir=/var/lib/etcd/restored \
  --name=master \
  --initial-cluster=master=https://127.0.0.1:2380 \
  --initial-cluster-token=etcd-cluster-1 \
  --initial-advertise-peer-urls=https://127.0.0.1:2380

# 恢复后用新数据目录启动 etcd
```

---

## 单节点 etcd 升级为多节点

```bash
# 1. 在新节点上执行 join
kubeadm join control-plane-endpoint:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <key>

# 2. kubeadm 会:
#    - 将 etcd CA 证书上传到 ConfigMap
#    - 解密 certificate-key 获取 etcd 证书
#    - 调用 etcd API 添加新成员
#    - 生成新节点的 etcd manifest
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `database space exceeded` | 磁盘空间不足 | `etcdctl defrag` + 清理磁盘 |
| `request timed out` | 网络延迟高 | 检查网络、调整 election-timeout |
| 启动慢 | 大数据量恢复 | 使用 snapshot 恢复而非重放 WAL |
| 脑裂 | 网络分区 | 检查网络配置、增加选举超时 |
