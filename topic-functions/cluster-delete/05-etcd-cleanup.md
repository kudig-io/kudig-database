---
title: etcd 数据清理与成员移除 — 源码分析
category: cluster-delete
tags:
- etcd
- member
- remove
- cleanup
- data-dir
- stacked
- external
- quorum
- raft
last_updated: 2026-05-18
description: 深入分析 control-plane 节点删除时 etcd 集群成员移除和数据清理的源码实现，涵盖 remove-etcd-member 阶段的 etcd 类型检测、数据目录获取、RemoveStackedEtcdMemberFromCluster
  成员移除逻辑、唯一成员特殊处理以及外部 etcd 处理策略。
difficulty: advanced
intent_queries:
- kubernetes etcd member removal source code
- RemoveStackedEtcdMemberFromCluster kubernetes
- etcd data directory cleanup kubernetes
- etcd quorum maintenance cluster deletion
- external etcd vs stacked etcd kubernetes
trigger_keywords:
- etcd member removal
- RemoveStackedEtcdMember
- getEtcdDataDir
- etcd.yaml
- etcd manifest
- ListMembers
- RemoveMember
- etcd quorum
- raft consensus
- external etcd
reading_level: advanced
audience:
- platform-engineer
- etcd-operator
- kubernetes-administrator
estimated_read_time: 5min
related_domains:
- domain-3-control-plane
- domain-2-installation
related_topics:
- cluster-delete
- reset
- cleanup
- force-delete
- ha-delete
- troubleshooting
domain_link: '[Control Plane](../domain-3-control-plane/README.md)'
topic_link: '[Cluster Delete Overview](./01-overview.md)'
---


# etcd 数据清理与成员移除 — 源码分析

## 概述

控制面节点的删除需要额外处理 etcd 集群：从集群中移除成员、清理本地数据。如果处理不当，会导致 etcd 仲裁丢失或数据不一致。本文档基于源码分析 `remove-etcd-member` 阶段的完整逻辑。

---

## 源码路径

- 成员移除阶段: `cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go`
- etcd 操作实现: `cmd/kubeadm/app/phases/etcd/local.go`
- etcd 工具库: `cmd/kubeadm/app/util/etcd/`

---

## 流程总览

```
┌──────────────────────────────────────────────────────────────┐
│  remove-etcd-member 阶段                                      │
├──────────────────────────────────────────────────────────────┤
│  1. 检测 etcd 配置（是否使用本地 etcd）                        │
│  2. 获取 etcd 数据目录                                        │
│  3. 从 etcd 集群移除本节点成员                                 │
│  4. 清理本地 etcd 数据目录                                     │
│  5. 兜底：如果成员移除失败，仍然清理数据目录                    │
└──────────────────────────────────────────────────────────────┘
```

---

## 1. etcd 类型检测

**源码**: `removeetcdmember.go` → `runRemoveETCDMemberPhase()`

```go
etcdManifestPath := filepath.Join(kubeadmconstants.KubernetesDir,
    kubeadmconstants.ManifestsSubDirName, "etcd.yaml")
etcdDataDir, err := getEtcdDataDir(etcdManifestPath, cfg)
if err == nil {
    // 本地 etcd（stacked etcd）
} else {
    fmt.Println("[reset] No etcd config found. Assuming external etcd")
    fmt.Println("[reset] Please, manually reset etcd to prevent further issues")
}
```

```
┌─────────────────────────────────────────────────────────────┐
│  etcd 类型判断                                                │
├─────────────────────────────────────────────────────────────┤
│  检测 /etc/kubernetes/manifests/etcd.yaml 是否存在            │
│  ├─ 存在 → Stacked/Local etcd → 执行成员移除 + 数据清理      │
│  └─ 不存在 → External etcd → 仅提示手动清理                   │
└─────────────────────────────────────────────────────────────┘
```

**工作节点**: 无 etcd.yaml，此阶段直接跳过（仅打印 "No etcd config found"）。

---

## 2. etcd 数据目录获取

### 2.1 getEtcdDataDir 实现

```go
func getEtcdDataDir(manifestPath string, cfg *kubeadmapi.InitConfiguration) (string, error) {
    const etcdVolumeName = "etcd-data"
    var dataDir string

    // 优先级 1: 从集群配置获取
    if cfg != nil && cfg.Etcd.Local != nil {
        return cfg.Etcd.Local.DataDir, nil
    }

    // 优先级 2: 从默认配置获取
    if _, err := os.Stat(manifestPath); os.IsNotExist(err) {
        cfg := &kubeadmapiv1.ClusterConfiguration{}
        scheme.Scheme.Default(cfg)
        return cfg.Etcd.Local.DataDir, nil
    }

    // 优先级 3: 从 etcd.yaml manifest 解析
    etcdPod, err := utilstaticpod.ReadStaticPodFromDisk(manifestPath)
    if err != nil {
        return "", err
    }
    for _, volumeMount := range etcdPod.Spec.Volumes {
        if volumeMount.Name == etcdVolumeName {
            dataDir = volumeMount.HostPath.Path
            break
        }
    }
    if dataDir == "" {
        return dataDir, errors.New("invalid etcd pod manifest")
    }
    return dataDir, nil
}
```

**获取优先级**:

```
┌──────────────────────────────────────────────────┐
│  etcd 数据目录获取优先级                           │
├──────────────────────────────────────────────────┤
│  1. InitConfiguration.Etcd.Local.DataDir          │
│     (从 kubeadm-config ConfigMap 获取)             │
│                                                    │
│  2. 默认 ClusterConfiguration 的 etcd 数据目录     │
│     (/var/lib/etcd)                                │
│                                                    │
│  3. 从 etcd.yaml 静态 Pod manifest 解析            │
│     查找 volume name = "etcd-data" 的 HostPath     │
└──────────────────────────────────────────────────┘
```

默认数据目录: `/var/lib/etcd`

---

## 3. 成员移除: RemoveStackedEtcdMemberFromCluster

**源码**: `cmd/kubeadm/app/phases/etcd/local.go`

```go
func RemoveStackedEtcdMemberFromCluster(
    client clientset.Interface,
    cfg *kubeadmapi.InitConfiguration,
) error {
    // 1. 创建 etcd 客户端，连接所有 stacked etcd 成员
    etcdClient, err := etcdutil.NewFromCluster(client, cfg.CertificatesDir)
    if err != nil {
        return err
    }

    // 2. 列出所有成员
    members, err := etcdClient.ListMembers()
    if err != nil {
        return err
    }

    // 3. 如果只剩一个成员，跳过移除
    if len(members) == 1 {
        etcdClientAddress := etcdutil.GetClientURL(&cfg.LocalAPIEndpoint)
        if slices.Contains(etcdClient.Endpoints, etcdClientAddress) {
            return nil
        }
    }

    // 4. 获取本节点的 member ID
    etcdPeerAddress := etcdutil.GetPeerURL(&cfg.LocalAPIEndpoint)
    id, err := etcdClient.GetMemberID(etcdPeerAddress)
    if err != nil {
        if errors.Is(err, etcdutil.ErrNoMemberIDForPeerURL) {
            return nil
        }
        return err
    }

    // 5. 移除成员
    members, err = etcdClient.RemoveMember(id)
    if err != nil {
        return err
    }
    return nil
}
```

### 3.1 执行流程图

```
┌────────────────────────────────────────────────────────────────┐
│  RemoveStackedEtcdMemberFromCluster                             │
├────────────────────────────────────────────────────────────────┤
│                                                                  │
│  NewFromCluster()                                                │
│    → 通过 API Server 获取所有 etcd Pod 端点                      │
│    → 使用 etcd 证书创建 gRPC 客户端                              │
│         │                                                        │
│         ▼                                                        │
│  ListMembers()                                                   │
│    → etcdctl member list                                         │
│         │                                                        │
│         ▼                                                        │
│  只剩 1 个成员？ ─── 是 → 跳过移除（最后一个成员）               │
│         │                                                        │
│         否                                                       │
│         ▼                                                        │
│  GetMemberID(peerURL)                                            │
│    → 根据 peer URL 查找 member ID                                │
│         │                                                        │
│         ▼                                                        │
│  成员已不存在？ ─── 是 → 跳过（已移除）                          │
│         │                                                        │
│         否                                                       │
│         ▼                                                        │
│  RemoveMember(id)                                                │
│    → etcdctl member remove <id>                                  │
│                                                                  │
└────────────────────────────────────────────────────────────────┘
```

### 3.2 etcd 客户端创建

```go
etcdClient, err := etcdutil.NewFromCluster(client, cfg.CertificatesDir)
```

**过程**:
1. 通过 Kubernetes API 获取 `kube-system` namespace 下的 etcd Pod
2. 从 Pod 注解中提取 `etcd.advertise.client.urls`
3. 使用 `/etc/kubernetes/pki/etcd/` 下的证书创建 etcd gRPC 客户端
   - `ca.crt` — etcd CA 证书
   - `server.crt` / `server.key` — etcd 客户端证书

### 3.3 唯一成员的特殊处理

```go
if len(members) == 1 {
    etcdClientAddress := etcdutil.GetClientURL(&cfg.LocalAPIEndpoint)
    if slices.Contains(etcdClient.Endpoints, etcdClientAddress) {
        klog.V(1).Info("[etcd] This is the only remaining etcd member, skip removing it")
        return nil
    }
}
```

**原因**: etcd 集群中最后一个成员不能通过 `member remove` API 移除自身。此时只需清理本地数据即可。

---

## 4. 数据目录清理

### 4.1 正常清理

```go
if !r.DryRun() {
    err := etcdphase.RemoveStackedEtcdMemberFromCluster(r.Client(), cfg)
    if err != nil {
        klog.Warningf("[reset] Failed to remove etcd member: %v, please manually remove this etcd member using etcdctl", err)
    } else {
        if err := CleanDir(etcdDataDir); err != nil {
            klog.Warningf("[reset] Failed to delete contents of the etcd directory: %q", etcdDataDir, err)
        } else {
            fmt.Printf("[reset] Deleted contents of the etcd data directory: %v\n", etcdDataDir)
        }
    }
}
```

### 4.2 兜底清理

即使 `remove-etcd-member` 阶段在 `cleanup-node` 之前执行，如果 `cleanup-node` 已经清除了 etcd.yaml，成员移除会失败。源码包含兜底逻辑：

```go
empty, _ := IsDirEmpty(etcdDataDir)
if !empty && !r.DryRun() {
    if err := CleanDir(etcdDataDir); err != nil {
        klog.Warningf("[reset] Failed to delete contents of the etcd directory: %q", etcdDataDir, err)
    } else {
        fmt.Printf("[reset] Deleted contents of the etcd data directory: %v\n", etcdDataDir)
    }
}
```

**场景**: 如果 `cleanup-node` 在 `remove-etcd-member` 之前被单独执行（通过 `kubeadm reset phase cleanup-node`），etcd.yaml 已被删除，但数据目录可能仍有残留文件。

---

## 5. 外部 etcd 处理

```go
} else {
    fmt.Println("[reset] No etcd config found. Assuming external etcd")
    fmt.Println("[reset] Please, manually reset etcd to prevent further issues")
}
```

使用外部 etcd 时：
- `kubeadm reset` **不执行** 任何 etcd 操作
- 需要手动使用 `etcdctl` 移除成员
- etcd 数据由外部 etcd 集群管理

---

## 6. etcd 仲裁与删除安全

### 6.1 仲裁要求

etcd 使用 Raft 协议，写操作需要 **多数派（quorum）** 确认：

```
┌────────────────────────────────────────────────────────┐
│  etcd 成员数与仲裁                                      │
├────────────────────────────────────────────────────────┤
│  成员数: 1  │  仲裁: 1  │  可容忍故障: 0               │
│  成员数: 3  │  仲裁: 2  │  可容忍故障: 1               │
│  成员数: 5  │  仲裁: 3  │  可容忍故障: 2               │
│  成员数: 7  │  仲裁: 4  │  可容忍故障: 3               │
└────────────────────────────────────────────────────────┘
```

### 6.2 删除安全操作

```
┌───────────────────────────────────────────────────────────┐
│  安全删除控制面节点顺序                                     │
├───────────────────────────────────────────────────────────┤
│  3 节点 HA 集群:                                           │
│  ├─ 删除第 1 个: 3→2 成员, 仲裁仍为 2, 集群可用 ✅         │
│  ├─ 删除第 2 个: 2→1 成员, 仲裁为 1, 集群勉强可用 ⚠️      │
│  └─ 删除第 3 个: 集群完全销毁                               │
│                                                             │
│  ⚠️ 关键: 每次删除一个，确认集群健康后再删下一个            │
│  ⚠️ 不要同时 reset 多个控制面节点                           │
└───────────────────────────────────────────────────────────┘
```

### 6.3 手动 etcd 成员移除

当 `kubeadm reset` 的自动移除失败时：

```bash
# 查看成员列表
etcdctl member list \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  --endpoints=https://127.0.0.1:2379

# 移除成员
etcdctl member remove <member-id> \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  --endpoints=https://127.0.0.1:2379

# 清理数据目录
rm -rf /var/lib/etcd
```

---

## 参考

- [removeetcdmember.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go)
- [etcd local.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/phases/etcd/local.go)
- [etcd util](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm/app/util/etcd/)
