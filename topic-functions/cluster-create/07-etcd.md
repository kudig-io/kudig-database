# etcd 集群初始化细节

## 源码路径

`cmd/kubeadm/app/phases/etcd/local.go`
`cmd/kubeadm/app/phases/etcd/util.go`

---

## etcd 在 kubeadm 中的角色

kubeadm 默认启动**本地单节点 etcd**，作为 API Server 的存储后端。

---

## etcd Quorum 机制

etcd 使用 Raft 共识算法，quorum (多数) 决定集群可写:

```
节点数    Quorum (多数)   容灾能力
1         1               0 节点故障
2         2               0 节点故障 (2节点无法容灾)
3         2               1 节点故障
4         3               1 节点故障
5         3               2 节点故障
```

**3 节点是最小高可用配置**。2 节点集群容灾能力为零，任何一个节点故障都会导致写失败。

---

## etcd 端点: API vs Metrics

etcd 暴露两个不同的端口:

| 端口 | 名称 | 用途 |
|------|------|------|
| 2379 | Client API | API Server 连接 etcd |
| 2380 | Peer API | etcd 节点间通信 (raft) |

```
                    ┌─────────────────┐
                    │  etcd server    │
                    │                 │
client (API Server) │                 │ peer (其他 etcd 节点)
        ↓           │                 │
    :2379           │                 │
                    └─────────────────┘
                           ↕
                       :2380
```

---

## 启动流程

```
kubeadm init
    ↓
etcd phase
    ↓
1. 生成 etcd CA (如果 /etc/kubernetes/pki/etcd/ 不存在)
    ↓
2. 生成 etcd Server/Peer/Client 证书
    ↓
3. 生成 /etc/kubernetes/manifests/etcd.yaml
    ↓
kubelet 检测到 etcd.yaml 变化 → 启动 etcd 容器
    ↓
等待 etcd 健康检查通过
```

---

## etcd.yaml manifest 关键参数

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    image: k8s.gcr.io/etcd:3.x.x
    command:
    - etcd
    - --data-dir=/var/lib/etcd
    - --listen-client-urls=https://127.0.0.1:2379
    - --advertise-client-urls=https://127.0.0.1:2379
    - --listen-peer-urls=https://127.0.0.1:2380
    - --initial-cluster=master=https://127.0.0.1:2380
    - --name=master
    - --client-cert-auth=true
    - --peer-client-cert-auth=true
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    volumeMounts:
    - name: etcd-data
      mountPath: /var/lib/etcd
  hostNetwork: true
  priority: 2000000000
  priorityClassName: system-node-critical
```

---

## 证书详解

| 证书 | 用途 |
|------|------|
| `server.crt/key` | 客户端连接 etcd server (API Server 使用) |
| `peer.crt/key` | etcd 节点间通信 (raft) |
| `healthcheck-client.crt/key` | kubeadm 健康检查使用 |

---

## 健康检查

```go
// cmd/kubeadm/app/phases/etcd/local.go
func WaitForEtcd() error {
    client, err := etcd.NewClient([]string{"https://127.0.0.1:2379"}, ca, cert, key)
    if err != nil {
        return err
    }
    // 轮询 /v2/members 和 /health 端点
    for i := 0; i < 30; i++ {
        if err := client.CheckHealth(); err == nil {
            return nil
        }
        time.Sleep(1 * time.Second)
    }
    return errors.New("etcd failed to start")
}
```

---

## 多节点 etcd 集群 (高可用)

当使用 `kubeadm init --control-plane` 添加新的 master 节点时:

```go
// cmd/kubeadm/app/phases/etcd/util.go
func JoinEtcdMember(cfg *InitConfiguration) error {
    // 1. 从 API Server 获取现有 etcd 成员列表
    // 2. 生成新的 etcd 证书 (含新节点 IP)
    // 3. 调用 etcd API: etcdctl member add <name> --peer-urls=<url>
    // 4. 生成新节点的 etcd.yaml
    // 5. 等待新成员加入集群
}
```

---

## 数据目录

```
/var/lib/etcd/
├── member/
│   ├── wal/          # Write-Ahead Log
│   └── snap/         # Snapshot
└── kv.db             # LevelDB/RocksDB 数据文件
```

---

## 备份与恢复

```bash
# 备份
ETCDCTL_API=3 etcdctl snapshot save backup.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 恢复
ETCDCTL_API=3 etcdctl snapshot restore backup.db \
  --data-dir=/var/lib/etcd/restored
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| etcd 启动失败 | 端口 2380/2379 被占用 | 检查端口占用 |
| `certificate is not valid` | 证书 SAN 不含新节点 IP | 重新生成含正确 IP 的证书 |
| 数据不一致 | 多节点网络分区 | 检查网络、减小选举超时 |
| `database space exceeded` | 磁盘空间不足 | 清理磁盘或压缩 etcd |
