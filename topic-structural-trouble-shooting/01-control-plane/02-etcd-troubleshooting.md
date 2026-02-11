# etcd 故障排查指南

> **适用版本**: etcd v3.4 - v3.5 (Kubernetes v1.25 - v1.32) | **最后更新**: 2026-01 | **难度**: 高级

## 🎯 本文档价值

etcd 是 Kubernetes 的“灵魂”，它的稳定性直接决定了集群的存亡。本文档旨在帮助不同层级的运维人员掌握 etcd 的运维精髓。

### 🎓 初学者视角
- **核心概念**：etcd 是一个高可用的键值存储系统，专门用于保存 Kubernetes 的所有集群数据（如 Pod, Service, ConfigMap 等）。
- **简单类比**：如果 Kubernetes 是一个大型超市，API Server 是前台，那么 etcd 就是后台的库存数据库。前台下班了超市还能开（现有 Pod 运行），但库存数据库坏了就没法进货或调货了（无法创建新资源）。

### 👨‍💻 资深专家视角
- **共识机制**：深入理解 Raft 算法中的选举限制和日志复制瓶颈。
- **存储引擎**：解析 bbolt 存储引擎的 mmap 机制如何影响磁盘 IO 性能。
- **灾难恢复**：掌握在多数节点丢失场景下的强行重组（Force New Cluster）与冷备恢复策略。

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 15 分钟快速诊断与止血

1. **健康检查一键看**：`ETCDCTL_API=3 etcdctl --write-out=table endpoint status --cluster`，关注 `Leader`、`DbSize`、`Raft Term`、`Recv/Send` 延迟；若无输出，立即检查监听端口/证书。
2. **磁盘 IO/空间快照**：`iostat -x 1 5`、`df -h /var/lib/etcd`，`DbSize` 接近配额时先 `compaction`+`defrag`，必要时临时扩容磁盘。
3. **网络与延迟**：`ping`/`mtr`/`tcptraceroute` 节点间 2379/2380，确认无丢包和 RTT 异常；云环境检查安全组/SLB 健康检查。
4. **证书有效性**：`openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -enddate`，证书将到期先做轮转，避免同时过期。
5. **热点与大 key**：`etcdctl --endpoints=... check perf` + `etcdctl --endpoints=... alarm list`，若有 `NOSPACE` 报警，执行 `compaction + defrag`；若观测到异常大 key，用 `etcdctl get <prefix> --prefix --keys-only | head` 定位来源。
6. **快速缓解策略**：
   - 读写受阻：先将高频 LIST/Watch 的租户/系统（如监控/同步器）降速或暂时停用。
   - Leader 抖动：优先锁定低延迟、性能好的节点作为首选 Leader（优化节点亲和或隔离噪声业务）。
   - 资源瓶颈：临时调高 etcd Pod request/limit，或在独立裸机/更快磁盘上运行。
7. **证据留存**：保存 `endpoint status`、`alarm list`、Prometheus etcd 指标（如 `etcd_server_leader_changes_seen_total`、`etcd_debugging_mvcc_db_total_size_in_bytes`）及系统 dmesg，便于复盘。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 etcd 服务不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 服务未启动 | `connection refused` | etcdctl/客户端 | etcdctl 命令输出 |
| 进程崩溃 | `etcd process exited` | systemd/容器运行时 | `journalctl -u etcd` |
| 端口未监听 | `dial tcp 127.0.0.1:2379: connect: connection refused` | etcdctl | 命令行输出 |
| 数据目录损坏 | `member has already been bootstrapped` | etcd 日志 | `journalctl -u etcd` |
| WAL 文件损坏 | `wal: crc mismatch` | etcd 日志 | etcd 启动日志 |

#### 1.1.2 etcd 集群故障

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 集群无 Leader | `no leader` | etcdctl | `etcdctl endpoint status` |
| 成员不健康 | `member <id> is unhealthy` | etcdctl | `etcdctl endpoint health` |
| 脑裂 | `request sent was ignored` | etcd 日志 | etcd 日志 |
| 选举超时 | `election timeout elapsed` | etcd 日志 | etcd 日志 |
| 成员无法加入 | `cluster ID mismatch` | etcd 日志 | etcd 启动日志 |

#### 1.1.3 etcd 性能问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 请求超时 | `context deadline exceeded` | API Server/etcdctl | 客户端日志 |
| 延迟过高 | `took too long to execute` | etcd 日志 | etcd 日志 |
| 磁盘 IO 慢 | `disk operations took too long` | etcd 日志 | etcd 日志 |
| 网络延迟高 | `rafthttp: request cluster ID mismatch` | etcd 日志 | etcd 日志 |
| 快照过大 | `database space exceeded` | etcd/API Server | etcd 日志 |

#### 1.1.4 etcd 数据问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 数据空间满 | `mvcc: database space exceeded` | etcd | etcd 日志 |
| 数据不一致 | `inconsistent snapshot` | etcd | etcd 日志 |
| 历史版本过多 | `etcdserver: too many requests` | etcd | etcd 日志 |
| 压缩失败 | `compaction failed` | etcd | etcd 日志 |
| 碎片化严重 | 存储空间异常增长 | 监控 | 磁盘使用监控 |

### 1.2 报错查看方式汇总

```bash
# 查看 etcd 进程状态（systemd 管理）
systemctl status etcd

# 查看 etcd 日志（systemd 管理）
journalctl -u etcd -f --no-pager -l

# 查看 etcd 日志（静态 Pod 方式）
crictl logs $(crictl ps -q --name etcd)

# 查看 etcd 容器日志
kubectl logs -n kube-system etcd-<node-name> --tail=500

# 查看 etcd 集群健康状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health --cluster

# 查看 etcd 集群状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status --cluster --write-out=table

# 查看 etcd 成员列表
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  member list --write-out=table

# 查看 etcd 数据库大小
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status --write-out=json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize}'
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **API Server** | 完全不可用 | API Server 无法读写数据，所有 API 请求失败 |
| **集群状态存储** | 数据不可访问 | 所有 Kubernetes 资源状态无法读取/更新 |
| **配置存储** | 不可用 | ConfigMap、Secret 等配置无法读取 |
| **Watch 机制** | 中断 | 所有 Watch 连接断开，控制器无法接收事件 |
| **选举** | 失效 | 依赖 etcd 的 Leader 选举机制失效 |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **现有工作负载** | 短期无影响 | 已运行的 Pod 继续运行，但无法管理 |
| **Scheduler** | 完全失效 | 新 Pod 无法调度 |
| **Controller Manager** | 完全失效 | 所有控制器停止工作 |
| **kubelet** | 部分影响 | 已有 Pod 继续运行，无法同步新配置 |
| **集群运维** | 完全不可用 | kubectl 所有操作失败 |
| **服务发现** | 部分影响 | Service 的 Endpoints 无法更新 |
| **自动化流程** | 中断 | CI/CD、自动扩缩容等流程中断 |

#### 1.3.3 故障传播链

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         etcd 故障影响传播链                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   etcd 故障                                                                  │
│       │                                                                      │
│       ├──► API Server 无法工作                                               │
│       │         │                                                            │
│       │         ├──► kubectl 所有命令失败                                    │
│       │         │                                                            │
│       │         ├──► Scheduler 无法调度新 Pod                                │
│       │         │                                                            │
│       │         ├──► Controller Manager 控制循环中断                         │
│       │         │         │                                                  │
│       │         │         ├──► Deployment 无法管理副本                       │
│       │         │         ├──► 自动扩缩容失效                                │
│       │         │         └──► 节点异常检测失效                              │
│       │         │                                                            │
│       │         └──► 所有 Watch 连接断开                                     │
│       │                                                                      │
│       ├──► 数据一致性风险                                                    │
│       │         │                                                            │
│       │         ├──► 集群脑裂可能导致数据分叉                                │
│       │         └──► 恢复时可能丢失数据                                      │
│       │                                                                      │
│       └──► 证书/Secret 不可读                                                │
│                 │                                                            │
│                 └──► 依赖证书的服务可能受影响                                │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 1.3.4 影响严重程度

| 故障类型 | 严重程度 | RTO 要求 | 说明 |
|----------|----------|----------|------|
| 单节点故障（3节点集群） | 中 | 30分钟 | 集群仍可用，但需要尽快恢复 |
| 多数节点故障 | 极高 | 立即 | 集群不可用，需紧急处理 |
| 数据损坏 | 极高 | 立即 | 可能需要从备份恢复 |
| 性能下降 | 中 | 2小时 | 影响用户体验，需要优化 |
| 空间不足 | 高 | 1小时 | 可能导致写入失败 |

---

## 2. 排查方法与步骤

### 2.1 排查原理

etcd 是 Kubernetes 集群的核心数据存储，采用 Raft 共识算法保证数据一致性。排查 etcd 问题需要从以下层面入手：

1. **进程层面**：etcd 进程是否正常运行
2. **集群层面**：集群成员状态、Leader 选举、数据同步
3. **存储层面**：磁盘空间、IO 性能、数据完整性
4. **网络层面**：成员间网络连通性、延迟
5. **配置层面**：启动参数、证书配置

### 2.2 排查逻辑决策树

```
开始排查
    │
    ├─► 检查进程状态
    │       │
    │       ├─► 进程不存在 ──► 检查启动失败原因
    │       │       │
    │       │       ├─► 数据目录问题 ──► 检查数据目录权限和完整性
    │       │       ├─► 证书问题 ──► 检查证书配置
    │       │       └─► 配置错误 ──► 检查启动参数
    │       │
    │       └─► 进程存在 ──► 继续下一步
    │
    ├─► 检查集群健康
    │       │
    │       ├─► 无 Leader ──► 检查成员状态和网络
    │       │
    │       ├─► 部分成员不健康 ──► 排查异常成员
    │       │
    │       └─► 集群健康 ──► 继续下一步
    │
    ├─► 检查存储状态
    │       │
    │       ├─► 空间不足 ──► 执行压缩和碎片整理
    │       │
    │       ├─► IO 延迟高 ──► 检查磁盘性能
    │       │
    │       └─► 存储正常 ──► 继续下一步
    │
    ├─► 检查网络连通性
    │       │
    │       ├─► 成员间不通 ──► 排查网络问题
    │       │
    │       ├─► 延迟高 ──► 优化网络或调整超时参数
    │       │
    │       └─► 网络正常 ──► 继续下一步
    │
    └─► 检查性能指标
            │
            ├─► 请求延迟高 ──► 分析负载来源
            │
            └─► 性能正常 ──► 完成排查
```

### 2.3 排查步骤和具体命令

#### 2.3.1 第一步：检查进程状态

```bash
# 检查 etcd 进程是否存在
ps aux | grep etcd | grep -v grep

# 检查进程详细信息
pgrep -a etcd

# systemd 管理的服务状态
systemctl status etcd

# 静态 Pod 方式检查
crictl ps -a | grep etcd

# 查看 etcd 启动参数
cat /proc/$(pgrep -x etcd)/cmdline | tr '\0' '\n'

# 检查 etcd 数据目录
ls -la /var/lib/etcd/

# 检查数据目录大小
du -sh /var/lib/etcd/
```

#### 2.3.2 第二步：检查集群健康

```bash
# 设置环境变量简化命令
export ETCDCTL_API=3
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/healthcheck-client.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 检查端点健康状态
etcdctl endpoint health --cluster

# 输出示例：
# https://10.0.0.1:2379 is healthy: successfully committed proposal: took = 15.345ms
# https://10.0.0.2:2379 is healthy: successfully committed proposal: took = 12.234ms
# https://10.0.0.3:2379 is healthy: successfully committed proposal: took = 18.456ms

# 检查端点状态（包含 Leader 信息）
etcdctl endpoint status --cluster --write-out=table

# 输出示例：
# +------------------------+------------------+---------+---------+-----------+...
# |        ENDPOINT        |        ID        | VERSION | DB SIZE | IS LEADER |...
# +------------------------+------------------+---------+---------+-----------+...
# | https://10.0.0.1:2379  | 8e9e05c52164694d | 3.5.9   | 25 MB   | false     |...
# | https://10.0.0.2:2379  | 91bc3c398fb3c146 | 3.5.9   | 25 MB   | true      |...
# | https://10.0.0.3:2379  | fd422379fda50e48 | 3.5.9   | 25 MB   | false     |...
# +------------------------+------------------+---------+---------+-----------+...

# 检查成员列表
etcdctl member list --write-out=table

# 检查告警
etcdctl alarm list
```

#### 2.3.3 第三步：检查存储状态

```bash
# 检查数据库大小
etcdctl endpoint status --write-out=json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize, dbSizeInUse: .Status.dbSizeInUse}'

# 检查磁盘空间
df -h /var/lib/etcd/

# 检查 inode 使用
df -i /var/lib/etcd/

# 检查磁盘 IO 性能
# 使用 fio 测试（如果可用）
fio --name=etcd-test --directory=/var/lib/etcd --size=100M --bs=2300 --fdatasync=1 --ioengine=sync --rw=write

# 使用 dd 简单测试
dd if=/dev/zero of=/var/lib/etcd/test.img bs=512 count=1000 oflag=dsync 2>&1 | tail -1

# 清理测试文件
rm -f /var/lib/etcd/test.img

# 检查 WAL 文件
ls -la /var/lib/etcd/member/wal/

# 检查快照文件
ls -la /var/lib/etcd/member/snap/
```

#### 2.3.4 第四步：检查网络连通性

```bash
# 检查 etcd 端口监听
netstat -tlnp | grep -E "2379|2380"
ss -tlnp | grep -E "2379|2380"

# 测试成员间连通性（从当前节点）
# 假设其他节点 IP 为 10.0.0.2 和 10.0.0.3
curl -k https://10.0.0.2:2379/health
curl -k https://10.0.0.3:2379/health

# 检查网络延迟
ping -c 10 10.0.0.2
ping -c 10 10.0.0.3

# 检查防火墙规则
iptables -L -n | grep -E "2379|2380"

# 测试 TCP 连通性
nc -zv 10.0.0.2 2379
nc -zv 10.0.0.2 2380
```

#### 2.3.5 第五步：检查证书状态

```bash
# 检查 etcd CA 证书
openssl x509 -in /etc/kubernetes/pki/etcd/ca.crt -noout -dates -subject

# 检查 etcd 服务端证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates -subject

# 检查 etcd 对等证书
openssl x509 -in /etc/kubernetes/pki/etcd/peer.crt -noout -dates -subject

# 检查健康检查客户端证书
openssl x509 -in /etc/kubernetes/pki/etcd/healthcheck-client.crt -noout -dates -subject

# 批量检查所有 etcd 相关证书
for cert in /etc/kubernetes/pki/etcd/*.crt; do
  echo "=== $cert ==="
  openssl x509 -in $cert -noout -dates 2>/dev/null
done

# 验证证书链
openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/server.crt
```

#### 2.3.6 第六步：检查性能指标

```bash
# 获取 etcd 指标
curl -k https://127.0.0.1:2379/metrics | grep -E "etcd_server|etcd_disk|etcd_network"

# 检查请求延迟
curl -k https://127.0.0.1:2379/metrics | grep etcd_disk_backend_commit_duration_seconds

# 检查 Leader 变更次数
curl -k https://127.0.0.1:2379/metrics | grep etcd_server_leader_changes_seen_total

# 检查提案失败次数
curl -k https://127.0.0.1:2379/metrics | grep etcd_server_proposals_failed_total

# 检查数据库大小
curl -k https://127.0.0.1:2379/metrics | grep etcd_mvcc_db_total_size_in_bytes

# 检查正在进行的快照数
curl -k https://127.0.0.1:2379/metrics | grep etcd_debugging_snap_save_total_duration_seconds
```

#### 2.3.7 第七步：检查日志

```bash
# 实时查看日志（systemd）
journalctl -u etcd -f --no-pager

# 查看最近的错误日志
journalctl -u etcd -p err --since "1 hour ago"

# 静态 Pod 方式查看日志
crictl logs $(crictl ps -q --name etcd) 2>&1 | tail -500

# 查找常见错误模式
journalctl -u etcd | grep -iE "(error|failed|warning|timeout)" | tail -50

# 查找 Raft 相关日志
journalctl -u etcd | grep -i raft | tail -50

# 查找选举相关日志
journalctl -u etcd | grep -iE "(election|leader|campaign)" | tail -50

# 查找磁盘相关警告
journalctl -u etcd | grep -iE "(disk|slow|took too long)" | tail -50
```

### 2.4 排查注意事项

#### 2.4.1 安全注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **数据备份** | etcd 是核心数据存储 | 任何操作前必须有完整备份 |
| **证书安全** | etcd 证书用于加密通信 | 不要泄露证书文件 |
| **访问控制** | etcd 包含敏感数据 | 限制 etcd 端口访问 |
| **日志敏感性** | 日志可能包含敏感信息 | 注意日志的分享范围 |

#### 2.4.2 操作注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **集群多数原则** | etcd 需要多数节点存活 | 3 节点至少需要 2 节点正常 |
| **顺序操作** | 成员变更需要顺序执行 | 一次只操作一个成员 |
| **数据一致性** | 避免强制操作导致数据不一致 | 尽量等待集群自愈 |
| **性能影响** | 诊断命令可能影响性能 | 避免在高负载时执行 |
| **版本兼容** | etcdctl 版本需与服务端匹配 | 确保版本一致 |

#### 2.4.3 关键原则

1. **备份优先**：任何修复操作前先备份
2. **保守原则**：能不重启就不重启，能不删数据就不删数据
3. **逐步操作**：多节点环境逐个处理
4. **验证恢复**：每步操作后验证集群状态

### 🚀 2.5 深度解析（专家专区）

#### 2.5.1 理解 WAL 与 Snapshot
- **WAL (Write Ahead Log)**：记录每一次写操作，保证崩溃恢复。如果 WAL 损坏，etcd 可能无法启动。
- **Snapshot**：etcd 状态机的快照，用于加速新成员加入和压缩 WAL。
- **专家提示**：如果 etcd 频繁触发 Snapshot，说明写负载非常高。通过调整 `--snapshot-count`（默认 100,000）可以平衡 IO 压力与故障恢复速度。

#### 2.5.2 磁盘 IOPS 与 Fsync 延迟
etcd 对磁盘延迟极其敏感，因为它需要等待 Fsync 确认日志已持久化。
- **故障特征**：日志中出现 `disk operations took too long` 或 `wal: sync duration of 500ms, expected less than 10ms`。
- **专家建议**：即使是云环境，也应使用预留 IOPS 的极速云盘或本地 SSD。避免将 etcd 与其他高写负载服务（如数据库、日志收集）部署在同一物理卷。

#### 2.5.3 成员变更的正确姿势
在 3 节点集群中，如果一个节点永久损坏：
1. **错误做法**：直接在损坏节点上尝试重启。
2. **正确做法**：在健康节点执行 `etcdctl member remove <id>`，然后在新节点上执行 `etcdctl member add <name>` 并作为新成员启动（设置 `--initial-cluster-state=existing`）。
- **避坑指南**：严禁在多数节点离线时直接通过 YAML 删除再创建 Pod，这可能导致集群元数据混乱。

---

## 3. 解决方案与风险控制

### 3.1 etcd 进程无法启动

#### 3.1.1 解决步骤

```bash
# 场景 1：数据目录权限问题
# 步骤 1：检查数据目录权限
ls -la /var/lib/etcd/

# 步骤 2：修复权限（根据部署方式）
chown -R etcd:etcd /var/lib/etcd/  # systemd 方式
# 或者
chown -R root:root /var/lib/etcd/  # 容器化方式

# 步骤 3：重启服务
systemctl restart etcd

# 场景 2：数据目录损坏
# 步骤 1：先备份现有数据
cp -r /var/lib/etcd /var/lib/etcd.bak.$(date +%Y%m%d)

# 步骤 2：检查 WAL 文件完整性
ls -la /var/lib/etcd/member/wal/

# 步骤 3：如果有快照，尝试从快照恢复
# 详见 3.5 节数据恢复流程

# 场景 3：配置错误
# 步骤 1：检查配置文件或启动参数
cat /etc/kubernetes/manifests/etcd.yaml  # 静态 Pod
# 或者
cat /etc/etcd/etcd.conf  # systemd 方式

# 步骤 2：常见配置检查项
# - --data-dir 路径是否正确
# - --initial-cluster 成员列表是否正确
# - --cert-file 和 --key-file 路径是否正确
# - --peer-cert-file 和 --peer-key-file 路径是否正确

# 步骤 3：修复配置后重启
systemctl restart etcd
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 权限修改可能影响其他服务 | 只修改 etcd 数据目录 |
| **极高** | 数据目录操作可能导致数据丢失 | 必须先备份 |
| **中** | 配置修改需要重启 | 在维护窗口操作 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 数据目录操作前必须备份
2. 不要随意删除 WAL 或快照文件
3. 配置修改前保存原始配置
4. 如果是多节点集群，确保其他节点正常
5. 考虑联系专业支持
```

### 3.2 etcd 集群无 Leader

#### 3.2.1 解决步骤

```bash
# 步骤 1：检查所有成员状态
etcdctl endpoint status --cluster --write-out=table

# 步骤 2：检查各成员网络连通性
for node in 10.0.0.1 10.0.0.2 10.0.0.3; do
  echo "=== $node ==="
  curl -k https://$node:2379/health
  ping -c 3 $node
done

# 步骤 3：检查各成员日志
# 在每个节点执行
journalctl -u etcd --since "10 minutes ago" | grep -iE "(election|leader|campaign)"

# 步骤 4：如果是网络分区，修复网络后等待自动恢复
# 检查防火墙
iptables -L -n | grep -E "2379|2380"

# 步骤 5：如果网络正常但仍无 Leader，尝试重启健康节点
# 注意：一次只重启一个节点
systemctl restart etcd  # 在一个节点执行

# 步骤 6：等待选举完成（通常 10-30 秒）
sleep 30
etcdctl endpoint status --cluster --write-out=table

# 步骤 7：如果仍无法选举 Leader，检查集群是否丢失多数成员
etcdctl member list
# 如果多数成员离线，需要进行灾难恢复（见 3.5 节）
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 无 Leader 期间集群不可写 | 优先恢复服务 |
| **中** | 重启可能延长恢复时间 | 先等待自动恢复 |
| **高** | 强制操作可能导致数据丢失 | 不要使用 --force 参数 |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 无 Leader 状态下集群只读，现有工作负载不受影响
2. 优先检查网络问题，大多数情况是网络导致
3. 不要同时重启多个成员
4. 避免使用 --force-new-cluster 除非确定数据丢失可接受
5. 记录所有操作步骤用于后续分析
```

### 3.3 etcd 数据空间满

#### 3.3.1 解决步骤

```bash
# 步骤 1：确认空间问题
etcdctl endpoint status --write-out=json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize}'
etcdctl alarm list

# 步骤 2：清除告警（如果有 NOSPACE 告警）
etcdctl alarm disarm

# 步骤 3：获取当前 revision
CURRENT_REV=$(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')
echo "Current revision: $CURRENT_REV"

# 步骤 4：执行压缩
# 保留最近 1000 个 revision
COMPACT_REV=$((CURRENT_REV - 1000))
etcdctl compact $COMPACT_REV

# 步骤 5：执行碎片整理（在每个成员上执行）
# 注意：碎片整理会阻塞写入，建议在低峰期执行
etcdctl defrag --cluster

# 步骤 6：验证空间释放
etcdctl endpoint status --write-out=json | jq '.[] | {endpoint: .Endpoint, dbSize: .Status.dbSize}'

# 步骤 7：配置自动压缩（推荐）
# 在 etcd 启动参数中添加：
# --auto-compaction-mode=periodic
# --auto-compaction-retention=1h
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 压缩会删除历史版本 | 确保不需要回滚到旧版本 |
| **高** | 碎片整理会短暂阻塞写入 | 在低峰期执行 |
| **低** | 配置修改需重启 | 逐个节点重启 |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 碎片整理期间服务短暂不可用，建议在维护窗口执行
2. 压缩操作不可逆，执行前确认不需要历史数据
3. 建议配置自动压缩避免空间问题
4. 监控 etcd 空间使用，设置告警阈值（建议 80%）
5. 考虑增加磁盘空间或调整 quota-backend-bytes
```

### 3.4 etcd 性能问题

#### 3.4.1 解决步骤

```bash
# 步骤 1：确认性能问题类型
# 检查磁盘延迟
curl -k https://127.0.0.1:2379/metrics | grep etcd_disk_backend_commit_duration_seconds

# 检查 Raft 提议延迟
curl -k https://127.0.0.1:2379/metrics | grep etcd_server_proposals_pending

# 步骤 2：检查磁盘 IO
iostat -x 1 10
# 关注 await 和 %util 指标

# 步骤 3：检查网络延迟
for node in 10.0.0.1 10.0.0.2 10.0.0.3; do
  echo "=== Latency to $node ==="
  ping -c 10 $node | tail -1
done

# 步骤 4：优化磁盘性能
# 使用 SSD（强烈推荐）
# 或者调整磁盘调度器
echo deadline > /sys/block/sda/queue/scheduler

# 步骤 5：优化 etcd 参数
# 调整心跳间隔和选举超时（在高延迟网络中）
# --heartbeat-interval=500    # 默认 100ms
# --election-timeout=5000     # 默认 1000ms

# 步骤 6：限制 API Server 请求
# 在 API Server 配置中添加：
# --etcd-count-metric-poll-period=0
# --etcd-compaction-interval=0  # 禁用 API Server 发起的压缩

# 步骤 7：监控和告警
# 添加 Prometheus 告警规则
cat << 'EOF'
groups:
- name: etcd
  rules:
  - alert: EtcdHighCommitLatency
    expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.25
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "etcd 磁盘提交延迟过高"
EOF
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 参数调整需要重启 | 逐个节点重启 |
| **低** | 磁盘调度器调整一般无风险 | 先在测试环境验证 |
| **中** | API Server 参数变更影响监控 | 确保有替代监控方案 |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 性能优化需要根据实际场景调整
2. 心跳和选举超时调整需要所有成员一致
3. 使用 SSD 是解决 etcd 性能问题的最有效方法
4. 避免与其他高 IO 服务共用磁盘
5. 定期监控 etcd 性能指标
```

### 3.5 etcd 数据恢复

#### 3.5.1 从快照恢复（推荐）

```bash
# 步骤 1：获取最新的 etcd 快照
# 如果有定期备份
ls -la /backup/etcd/

# 如果没有备份，但 etcd 还能访问，先创建快照
etcdctl snapshot save /backup/etcd/snapshot-$(date +%Y%m%d%H%M%S).db

# 步骤 2：停止所有 etcd 成员
# 在所有节点执行
systemctl stop etcd
# 或者移除静态 Pod manifest
mv /etc/kubernetes/manifests/etcd.yaml /tmp/

# 步骤 3：备份现有数据目录（所有节点）
mv /var/lib/etcd /var/lib/etcd.bak.$(date +%Y%m%d)

# 步骤 4：从快照恢复（在每个节点执行，参数不同）
# 节点 1
etcdctl snapshot restore /backup/etcd/snapshot.db \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-cluster-token=etcd-cluster \
  --initial-advertise-peer-urls=https://10.0.0.1:2380 \
  --data-dir=/var/lib/etcd

# 节点 2
etcdctl snapshot restore /backup/etcd/snapshot.db \
  --name=etcd-2 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-cluster-token=etcd-cluster \
  --initial-advertise-peer-urls=https://10.0.0.2:2380 \
  --data-dir=/var/lib/etcd

# 节点 3
etcdctl snapshot restore /backup/etcd/snapshot.db \
  --name=etcd-3 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380,etcd-2=https://10.0.0.2:2380,etcd-3=https://10.0.0.3:2380 \
  --initial-cluster-token=etcd-cluster \
  --initial-advertise-peer-urls=https://10.0.0.3:2380 \
  --data-dir=/var/lib/etcd

# 步骤 5：启动 etcd（所有节点）
systemctl start etcd
# 或者恢复静态 Pod manifest
mv /tmp/etcd.yaml /etc/kubernetes/manifests/

# 步骤 6：验证恢复
etcdctl endpoint status --cluster --write-out=table
etcdctl member list
```

#### 3.5.2 单节点强制恢复（最后手段）

```bash
# ⚠️ 警告：此操作会丢失数据，仅在无其他选择时使用

# 步骤 1：停止所有控制平面组件
# 移除静态 Pod manifests
mv /etc/kubernetes/manifests/*.yaml /tmp/manifests/

# 步骤 2：备份当前数据
cp -r /var/lib/etcd /var/lib/etcd.emergency.bak

# 步骤 3：删除原数据目录
rm -rf /var/lib/etcd

# 步骤 4：使用 --force-new-cluster 参数创建新集群
# 修改 etcd 启动参数，添加 --force-new-cluster

# 步骤 5：启动 etcd
# 等待 etcd 启动成功

# 步骤 6：移除 --force-new-cluster 参数并重启

# 步骤 7：恢复其他控制平面组件
mv /tmp/manifests/*.yaml /etc/kubernetes/manifests/

# 步骤 8：验证集群状态
kubectl get nodes
kubectl get pods -A
```

#### 3.5.3 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **极高** | 恢复操作可能覆盖数据 | 确保有多份备份 |
| **极高** | 快照可能不是最新的 | 接受数据丢失风险 |
| **极高** | 强制恢复可能导致数据不一致 | 仅作最后手段 |

#### 3.5.4 安全生产风险提示

```
⚠️  紧急恢复安全生产风险提示：
1. 【备份】确保有多个时间点的备份
2. 【评估】评估可接受的数据丢失范围
3. 【通知】通知相关团队和利益相关方
4. 【记录】详细记录所有恢复步骤
5. 【验证】恢复后全面验证数据完整性
6. 【复盘】事后进行根因分析和改进
7. 【演练】建立定期备份恢复演练机制
```

### 3.6 etcd 备份最佳实践

```bash
# 创建 etcd 备份脚本
cat > /usr/local/bin/etcd-backup.sh << 'EOF'
#!/bin/bash
set -e

BACKUP_DIR=/backup/etcd
DATE=$(date +%Y%m%d%H%M%S)
BACKUP_FILE=${BACKUP_DIR}/snapshot-${DATE}.db

# 创建备份目录
mkdir -p ${BACKUP_DIR}

# 执行备份
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  snapshot save ${BACKUP_FILE}

# 验证备份
etcdctl snapshot status ${BACKUP_FILE}

# 保留最近 7 天的备份
find ${BACKUP_DIR} -name "snapshot-*.db" -mtime +7 -delete

echo "Backup completed: ${BACKUP_FILE}"
EOF

chmod +x /usr/local/bin/etcd-backup.sh

# 配置定时任务
cat > /etc/cron.d/etcd-backup << 'EOF'
# 每小时备份一次
0 * * * * root /usr/local/bin/etcd-backup.sh >> /var/log/etcd-backup.log 2>&1
EOF
```

---

## 附录

### A. etcd 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `etcd_server_has_leader` | 是否有 Leader | = 0 |
| `etcd_server_leader_changes_seen_total` | Leader 变更次数 | > 3/hour |
| `etcd_disk_backend_commit_duration_seconds` | 磁盘提交延迟 | P99 > 250ms |
| `etcd_mvcc_db_total_size_in_bytes` | 数据库大小 | > quota * 0.8 |
| `etcd_server_proposals_failed_total` | 提案失败数 | > 0 |

### B. 常见启动参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--data-dir` | - | 数据存储目录 |
| `--initial-cluster` | - | 初始集群成员列表 |
| `--listen-peer-urls` | - | 对等节点通信地址 |
| `--listen-client-urls` | - | 客户端访问地址 |
| `--quota-backend-bytes` | 2GB | 数据库大小限制 |
| `--auto-compaction-mode` | - | 自动压缩模式 |
| `--auto-compaction-retention` | - | 自动压缩保留时间 |
| `--heartbeat-interval` | 100ms | 心跳间隔 |
| `--election-timeout` | 1000ms | 选举超时 |

### C. 相关文档链接

- [etcd 官方文档](https://etcd.io/docs/)
- [Kubernetes etcd 操作指南](https://kubernetes.io/docs/tasks/administer-cluster/configure-upgrade-etcd/)
- [etcd 灾难恢复](https://etcd.io/docs/v3.5/op-guide/recovery/)

---

## 📚 D. 生产环境实战案例精选

### 案例 1:etcd 数据库膨胀至 8GB 导致写入失败

#### 🎯 故障场景
某大型互联网公司,集群 800 节点、8000 Pod,运行 3 个月后突然所有 `kubectl apply` 失败,业务无法发布,影响重大。

#### 🔍 排查过程
1. **现象确认**:
   ```bash
   kubectl apply -f deployment.yaml
   # Error from server: etcdserver: mvcc: database space exceeded
   ```

2. **数据库检查**:
   ```bash
   etcdctl endpoint status --write-out=table
   # +------------------+------------------+---------+---------+-----------+
   # |     ENDPOINT     |        ID        | VERSION | DB SIZE | IS LEADER |
   # +------------------+------------------+---------+---------+-----------+
   # | 127.0.0.1:2379   | 8e9e05c52164694d | 3.5.9   | 8.4 GB  | true      |  # ❌ 超出默认 2GB 配额 4 倍！
   # +------------------+------------------+---------+---------+-----------+
   
   # 检查告警
   etcdctl alarm list
   # memberID:14276254820001 alarm:NOSPACE  # ❌ 空间告警触发
   ```

3. **数据分析**:
   ```bash
   # 统计各类资源数量
   kubectl get all -A | wc -l  # 12000 个资源
   kubectl get events -A | wc -l  # 150000 个事件 ❌ 异常多！
   kubectl get leases -A | wc -l  # 8500 个租约
   
   # 检查历史版本
   etcdctl endpoint status --write-out=json | jq '.[] | .Status.header.revision'
   # 25678901  # 2500 万个版本累积
   ```

4. **根因分析**:
   - 大量 Event 对象未清理(API Server 默认保留 1 小时,但累积速度快)
   - 未配置自动压缩,历史版本无限累积
   - 配额设置为默认 2GB,实际使用超出
   - 未监控数据库大小,直到写入失败才发现

#### ⚡ 应急措施
1. **立即解除空间告警**:
   ```bash
   # 解除 NOSPACE 告警(允许临时写入)
   etcdctl alarm disarm
   
   # 验证
   etcdctl alarm list
   # (empty)  ✅ 告警已解除
   ```

2. **压缩历史版本**:
   ```bash
   # 获取当前版本号
   rev=$(etcdctl endpoint status --write-out=json | jq -r '.[] | .Status.header.revision')
   echo "Current revision: $rev"  # 25678901
   
   # 压缩到当前版本(删除所有历史)
   etcdctl compact $rev
   # compacted revision 25678901 ✅
   
   # 验证数据库大小(压缩后空间未释放)
   etcdctl endpoint status --write-out=table
   # DB SIZE: 8.4 GB  # 仍未变化,需要 defrag
   ```

3. **碎片整理回收空间**:
   ```bash
   # ⚠️ Defrag 会短暂阻塞读写,生产环境需谨慎
   # 逐个节点执行,避免全部阻塞
   
   # 节点 1
   etcdctl defrag --endpoints=https://10.0.0.1:2379
   # Finished defragmenting etcd member[https://10.0.0.1:2379]
   
   # 等待 30 秒,观察集群状态
   sleep 30
   etcdctl endpoint health --cluster
   
   # 节点 2
   etcdctl defrag --endpoints=https://10.0.0.2:2379
   sleep 30
   
   # 节点 3
   etcdctl defrag --endpoints=https://10.0.0.3:2379
   
   # 验证空间回收
   etcdctl endpoint status --write-out=table
   # DB SIZE: 1.2 GB  ✅ 大幅减少！
   ```

4. **清理过期 Event**:
   ```bash
   # Event 会自动过期,但可以主动清理旧的
   # 删除 1 小时前的 Event
   kubectl get events -A --sort-by='.lastTimestamp' | tail -100000 | awk '{print $1" "$2}' | xargs -n 2 kubectl delete event -n
   ```

5. **5 分钟后恢复正常**:
   ```bash
   kubectl apply -f deployment.yaml
   # deployment.apps/myapp created  ✅ 恢复成功
   ```

#### 🛡️ 长期优化
1. **配置自动压缩**:
   ```yaml
   # etcd 静态 Pod 配置
   apiVersion: v1
   kind: Pod
   metadata:
     name: etcd
     namespace: kube-system
   spec:
     containers:
     - name: etcd
       command:
       - etcd
       - --auto-compaction-mode=periodic  # ✅ 启用自动压缩
       - --auto-compaction-retention=5m   # ✅ 每 5 分钟压缩一次
       - --quota-backend-bytes=8589934592 # ✅ 提高配额至 8GB
   ```

2. **定时碎片整理**:
   ```yaml
   # CronJob 每天凌晨 3 点整理
   apiVersion: batch/v1
   kind: CronJob
   metadata:
     name: etcd-defrag
     namespace: kube-system
   spec:
     schedule: "0 3 * * *"
     jobTemplate:
       spec:
         template:
           spec:
             hostNetwork: true
             containers:
             - name: defrag
               image: quay.io/coreos/etcd:v3.5.9
               command:
               - /bin/sh
               - -c
               - |
                 for ep in https://10.0.0.1:2379 https://10.0.0.2:2379 https://10.0.0.3:2379; do
                   echo "Defragmenting $ep"
                   etcdctl defrag --endpoints=$ep
                   sleep 30  # 每个节点间隔 30 秒
                 done
               env:
               - name: ETCDCTL_API
                 value: "3"
             restartPolicy: OnFailure
   ```

3. **降低 Event 产生速率**:
   ```yaml
   # API Server 配置
   - --event-ttl=30m  # 降低 Event TTL 至 30 分钟(默认 1 小时)
   ```

4. **监控告警**:
   ```yaml
   # Prometheus 告警规则
   groups:
   - name: etcd-storage
     rules:
     - alert: EtcdDatabaseSizeTooLarge
       expr: etcd_mvcc_db_total_size_in_bytes > 6 * 1024 * 1024 * 1024  # > 6GB
       for: 10m
       labels:
         severity: warning
       annotations:
         summary: "etcd 数据库过大"
         description: "etcd 数据库大小 {{ $value | humanize1024 }}，接近配额"
     
     - alert: EtcdNoSpaceAlarm
       expr: etcd_server_has_space == 0
       for: 1m
       labels:
         severity: critical
       annotations:
         summary: "etcd 空间不足告警"
         description: "etcd 触发 NOSPACE 告警,写入将失败"
   ```

#### 💡 经验总结
- **容量规划失误**:未预估数据增长速率,配额设置过小
- **维护缺失**:未配置自动压缩和定期碎片整理
- **监控盲区**:未监控数据库大小趋势
- **改进方向**:自动化维护、容量监控、定期演练、提前扩容

---

### 案例 2:网络分区导致 etcd 脑裂

#### 🎯 故障场景
某金融公司多机房部署,3 节点 etcd 集群分布在 3 个机房,某天机房间专线抖动,导致集群出现"双 Leader"现象,数据出现分叉,恢复后部分数据丢失。

#### 🔍 排查过程
1. **现象确认**:
   ```bash
   # 机房 A(节点 1)
   etcdctl --endpoints=https://10.0.1.1:2379 endpoint status
   # ENDPOINT         | ID       | IS LEADER
   # 10.0.1.1:2379    | node-1   | true       # ❌ Leader 1
   
   # 机房 B(节点 2)
   etcdctl --endpoints=https://10.0.2.1:2379 endpoint status
   # ENDPOINT         | ID       | IS LEADER
   # 10.0.2.1:2379    | node-2   | true       # ❌ Leader 2 (脑裂！)
   
   # 机房 C(节点 3)
   etcdctl --endpoints=https://10.0.3.1:2379 endpoint status
   # context deadline exceeded  # 无法访问
   ```

2. **网络诊断**:
   ```bash
   # 从节点 1 测试到节点 2/3
   ping -c 5 10.0.2.1
   # 5 packets transmitted, 2 received, 60% packet loss  # ❌ 丢包严重
   
   tcpping -c 10 10.0.2.1 2380
   # avg: 2500ms  # ❌ 延迟极高(正常 < 10ms)
   
   # 检查路由
   mtr 10.0.2.1
   # 发现机房间专线抖动
   ```

3. **日志分析**:
   ```bash
   # 节点 1 日志
   journalctl -u etcd | grep -i "lost"
   # failed to send out heartbeat on time (deadline exceeded)
   # lost leader 8e9e05c52164694d, became candidate
   # elected leader 8e9e05c52164694d at term 158  # 新 Leader 产生
   
   # 节点 2 日志
   # lost leader 8e9e05c52164694d, became candidate
   # elected leader f70e05c52164694e at term 159  # 另一个 Leader 产生 ❌
   ```

4. **根因分析**:
   - 机房间专线抖动,丢包 60%,延迟 > 2s
   - election-timeout 默认 1s,网络延迟导致选举超时
   - 节点 1 和节点 2 各自与节点 3 失联,分别形成多数派(1+3 和 2+3)
   - 两个分区各自选举 Leader,产生"脑裂"
   - 恢复后数据合并出现冲突,部分写入丢失

#### ⚡ 应急措施
1. **立即隔离故障节点**:
   ```bash
   # 强制停止节点 2(少数派)
   ssh 10.0.2.1 "systemctl stop etcd"
   
   # 保留节点 1(多数派)继续服务
   etcdctl --endpoints=https://10.0.1.1:2379 member list
   # 确认只有节点 1 为 Leader
   ```

2. **等待网络恢复**:
   ```bash
   # 持续监控网络状态
   while true; do
     ping -c 1 10.0.2.1 && echo "Network OK" || echo "Network Down"
     sleep 5
   done
   ```

3. **修复网络后恢复节点**:
   ```bash
   # 删除节点 2 的数据目录(避免数据冲突)
   ssh 10.0.2.1 "rm -rf /var/lib/etcd/*"
   
   # 从节点 1 创建快照
   etcdctl --endpoints=https://10.0.1.1:2379 snapshot save /backup/snapshot-recovery.db
   
   # 在节点 2 恢复快照
   scp /backup/snapshot-recovery.db 10.0.2.1:/tmp/
   ssh 10.0.2.1 "etcdctl snapshot restore /tmp/snapshot-recovery.db --data-dir=/var/lib/etcd"
   
   # 启动节点 2
   ssh 10.0.2.1 "systemctl start etcd"
   
   # 验证恢复
   etcdctl --endpoints=https://10.0.1.1:2379,https://10.0.2.1:2379 endpoint status --cluster
   # 确认只有一个 Leader
   ```

#### 🛡️ 长期优化
1. **增大选举超时(适应高延迟网络)**:
   ```yaml
   # etcd 配置
   - --heartbeat-interval=500      # 从 100ms 增至 500ms
   - --election-timeout=5000       # 从 1000ms 增至 5000ms
   # 注意:所有节点必须配置一致
   ```

2. **优化机房部署策略**:
   ```
   原方案(不推荐):
   机房 A: etcd-1
   机房 B: etcd-2
   机房 C: etcd-3
   
   优化方案(推荐):
   机房 A: etcd-1, etcd-2 (同机房多数派)
   机房 B: etcd-3         (避免单机房全部不可用)
   ```

3. **网络监控与熔断**:
   ```yaml
   # 监控机房间延迟
   - alert: EtcdHighPeerLatency
     expr: histogram_quantile(0.99, rate(etcd_network_peer_round_trip_time_seconds_bucket[5m])) > 0.5
     for: 5m
     labels:
       severity: warning
     annotations:
       summary: "etcd 节点间延迟过高"
   ```

4. **数据一致性检查**:
   ```bash
   # 定期比对各节点数据一致性
   for ep in https://10.0.1.1:2379 https://10.0.2.1:2379 https://10.0.3.1:2379; do
     echo "=== $ep ==="
     etcdctl --endpoints=$ep get / --prefix --keys-only | md5sum
   done
   # 3 个节点的 MD5 应该一致
   ```

#### 💡 经验总结
- **架构设计缺陷**:跨机房部署未考虑网络抖动风险
- **参数配置不当**:election-timeout 过小,不适应高延迟场景
- **监控缺失**:未监控节点间延迟和 Leader 变更
- **改进方向**:优化部署架构、调整超时参数、加强网络监控、定期一致性检查
