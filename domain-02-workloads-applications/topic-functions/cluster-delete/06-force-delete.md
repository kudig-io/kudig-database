---
title: 强制删除与异常场景处理 (topic-code-analysis)
description: reset 的容错机制（best-effort 策略）以及手动处理方案，涵盖 --force 标志、错误处理、跳过阶段、异常场景与处理方案。
category: general
tags:
- reference
- etcd
- kubelet
- flannel
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 强制删除与异常场景处理 是什么
- 如何 强制删除与异常场景处理
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 强制删除与异常场景处理
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
created: "2026-05-23"
---

title: 强制删除与异常场景处理
category: cluster-delete
tags:
- force
- reset
- error
- exception
- node
- unreachable
- etcd
- container
- unmount
last_updated: 2026-05-18
description: 生产环境中集群删除常遇到各种异常场景：节点不可达、etcd 仲裁丢失、kubelet 无法停止、容器运行时异常等。本文档分析 kubeadm
  reset 的容错机制（best-effort 策略）以及手动处理方案，涵盖 --force 标志、错误处理、跳过阶段、异常场景与处理方案。
difficulty: advanced
intent_queries:
- kubeadm reset force flag error handling
- kubernetes cluster deletion exception handling
- etcd quorum lost cluster deletion
- node unreachable cluster deletion
- container runtime failure cluster reset
trigger_keywords:
- force reset
- --force
- best effort
- skip-phases
- unreachable node
- API Server unavailable
- etcd quorum lost
- container runtime error
- device busy
- unmount failed
- NFS mount stuck
reading_level: advanced
audience:
- platform-engineer
- kubernetes-administrator
- sre
estimated_read_time: 5min
related_domains:
- domain-01-cluster-fundamentals
- domain-01-cluster-fundamentals
related_topics:
- cluster-delete
- reset
- cleanup
- etcd-cleanup
- ha-delete
- troubleshooting
domain_link: '[Installation](../domain-01-cluster-fundamentals/README.md)'
topic_link: '[Cluster Delete Overview](./01-overview.md)'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 强制删除与异常场景处理

## 概述

生产环境中集群删除常遇到各种异常场景：节点不可达、etcd 仲裁丢失、kubelet 无法停止、容器运行时异常等。本文档分析 `kubeadm reset` 的容错机制以及手动处理方案。

---

## 1. --force 标志

### 1.1 源码分析

**源码**: `cmd/kubeadm/app/cmd/phases/reset/preflight.go`

```go
func runPreflight(c workflow.RunData) error {
    r := c.(resetData)

    if !r.ForceReset() && !r.DryRun() {
        klog.Warning("[reset] WARNING: Changes made to this host by 'kubeadm init' or 'kubeadm join' will be reverted.")
        if err := util.InteractivelyConfirmAction("reset",
            "Are you sure you want to proceed?", r.InputReader()); err != nil {
            return err
        }
    }

    fmt.Println("[preflight] Running pre-flight checks")
    return preflight.RunRootCheckOnly(r.IgnorePreflightErrors())
}
```

**`-f` / `--force` 作用**:
- 跳过交互式确认提示
- **不会** 跳过任何清理操作
- **不会** 忽略错误

### 1.2 使用场景

```bash
# 自动化脚本中避免交互
kubeadm reset -f

# 等价于
echo "y" | kubeadm reset
```

---

## 2. 错误处理策略

### 2.1 reset 的容错设计

`kubeadm reset` 采用 **"best effort"** 策略：每个步骤失败后仅 **warning**，不中断后续步骤。

```
┌──────────────────────────────────────────────────────────────┐
│  reset 容错设计                                                │
├──────────────────────────────────────────────────────────────┤
│  停止 kubelet 失败    → Warning: "Please ensure kubelet      │
│                        is stopped manually"                  │
│                                                                │
│  卸载挂载点失败      → Warning: 继续清理目录                   │
│                                                                │
│  移除容器失败        → Warning: 继续清理配置                   │
│                                                                │
│  清理目录失败        → Warning: 继续清理下一个目录              │
│                                                                │
│  删除文件失败        → Warning: 继续删除下一个文件              │
│                                                                │
│  etcd 成员移除失败   → Warning: "please manually remove       │
│                        this etcd member using etcdctl"        │
│                                                                │
│  移除用户/组失败     → Warning: 继续执行                       │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 各阶段错误处理源码

**kubelet 停止**:
```go
if err := initSystem.ServiceStop("kubelet"); err != nil {
    klog.Warningf("[reset] The kubelet service could not be stopped: [%v]\n", err)
    klog.Warningln("[reset] Please ensure kubelet is stopped manually")
}
```

**容器移除**:
```go
if err := removeContainers(r.CRISocketPath()); err != nil {
    klog.Warningf("[reset] Failed to remove containers: %v\n", err)
}
```

**目录清理**:
```go
for _, dir := range dirsToClean {
    if err := CleanDir(dir); err != nil {
        klog.Warningf("[reset] Failed to delete contents of %q directory: %v", dir, err)
    }
}
```

**文件删除**:
```go
for _, path := range filesToClean {
    if err := os.RemoveAll(path); err != nil {
        klog.Warningf("[reset] Failed to remove file: %q [%v]\n", path, err)
    }
}
```

**etcd 成员移除**:
```go
err := etcdphase.RemoveStackedEtcdMemberFromCluster(r.Client(), cfg)
if err != nil {
    klog.Warningf("[reset] Failed to remove etcd member: %v, please manually remove this etcd member using etcdctl", err)
}
```

---

## 3. 跳过阶段

### 3.1 --skip-phases

```bash
kubeadm reset --skip-phases=remove-etcd-member
```

**常见使用场景**:

| 跳过的阶段 | 场景 |
|-----------|------|
| `remove-etcd-member` | etcd 集群已不可用，或已手动移除成员 |
| `preflight` | 自动化脚本中不需要确认 |
| `cleanup-node` | 只想移除 etcd 成员而不清理节点 |

### 3.2 源码实现

```go
if len(resetRunner.Options.SkipPhases) == 0 {
    resetRunner.Options.SkipPhases = data.resetCfg.SkipPhases
}
```

也可通过配置文件指定：

```yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: ResetConfiguration
skipPhases:
  - remove-etcd-member
cleanupTmpDir: true
force: true
```

---

## 4. 异常场景与处理方案

### 4.1 节点不可达

**场景**: 节点网络不通、SSH 无法连接、kubelet 无法启动

**处理**:

```bash
# 在可达的控制面节点上
kubectl delete node <unreachable-node>

# 手动移除 etcd 成员
etcdctl member list
etcdctl member remove <member-id>

# 如果节点恢复可达后，在节点上执行
kubeadm reset -f
```

### 4.2 API Server 不可用

**场景**: 所有控制面节点宕机，API Server 无法启动

**分析**: `newResetData` 中 API Client 构建失败时不中断：

```go
client, err = kubeconfigutil.ClientSetFromFile(opts.kubeconfigPath)
if err == nil {
    // 从集群获取配置
    initCfg, err = configutil.FetchInitConfigurationFromCluster(...)
} else {
    klog.V(1).Infof("[reset] Could not obtain a client set from the kubeconfig file")
}
```

**结果**:
- `cfg` 为 nil → etcd 成员移除阶段无法自动执行
- `client` 为 nil → 无法通过 API 与集群通信
- 节点清理仍然可以执行（不需要 API Client）

**手动处理**:

```bash
# 直接在节点上执行
kubeadm reset -f --skip-phases=remove-etcd-member

# 手动清理 etcd
rm -rf /var/lib/etcd
```

### 4.3 etcd 仲裁丢失

**场景**: 3 节点 etcd 集群中 2 个节点已丢失，剩 1 个节点无法达成仲裁

**症状**:
```
[reset] Failed to remove etcd member: etcdserver: unhealthy cluster, please manually remove this etcd member using etcdctl
```

**处理**:

```bash
# 在剩余节点上强制恢复（会丢失数据共识）
# 停止 etcd
crictl stop $(crictl ps --name etcd -q)

# 使用 --force-unhealthy 恢复
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  member list

# 如果无法操作，直接清理
kubeadm reset -f
rm -rf /var/lib/etcd
```

### 4.4 容器运行时异常

**场景**: containerd/docker 崩溃，无法通过 CRI 删除容器

**源码处理**:
```go
if err := removeContainers(r.CRISocketPath()); err != nil {
    klog.Warningf("[reset] Failed to remove containers: %v\n", err)
}
```

**手动处理**:

```bash
# containerd
ctr -n k8s.io containers rm $(ctr -n k8s.io containers -q)
ctr -n k8s.io tasks kill $(ctr -n k8s.io tasks -q)

# 或直接重启 containerd 服务
systemctl restart containerd

# 然后重新执行 reset
kubeadm reset -f
```

### 4.5 卸载挂载点失败

**场景**: `/var/lib/kubelet/pods/...` 下的挂载点 busy 无法卸载

**源码处理**:
```go
if err := syscall.Unmount(m[1], flagsInt); err != nil {
    if err == syscall.EINVAL {
        klog.Warningf("[reset] Ignoring EINVAL error while unmounting %q", m[1])
        continue
    }
    errList = append(errList, ...)
}
```

**手动处理**:

```bash
# 查看挂载点
mount | grep kubelet

# 强制卸载
umount -f $(mount | grep kubelet | awk '{print $3}')

# 懒卸载
umount -l $(mount | grep kubelet | awk '{print $3}')

# 使用 ResetConfiguration
kubeadm reset --config=reset.yaml
# reset.yaml:
# unmountFlags: ["MNT_DETACH"]
```

### 4.6 证书目录非默认

**源码处理**:
```go
if certsDir != kubeadmapiv1.DefaultCertificatesDir {
    klog.Warningf("[reset] WARNING: Cleaning a non-default certificates directory: %q\n", certsDir)
}
```

如果使用了自定义证书目录（非 `/etc/kubernetes/pki`），reset 会打印额外警告。

---

## 5. 完整的灾难恢复删除脚本

```bash
#!/bin/bash
set -e

NODE_NAME=$(hostname)

echo "=== Force Reset Node: ${NODE_NAME} ==="

# 1. 尝试正常 reset
kubeadm reset -f || true

# 2. 停止所有服务
systemctl stop kubelet 2>/dev/null || true
systemctl stop containerd 2>/dev/null || true

# 3. 强制卸载 kubelet 挂载点
mount | grep '/var/lib/kubelet' | awk '{print $3}' | while read mp; do
    umount -l "$mp" 2>/dev/null || true
done

# 4. 清理容器运行时
rm -rf /var/lib/containerd/*
rm -rf /var/lib/docker/*
systemctl start containerd

# 5. 清理 Kubernetes 数据
rm -rf /etc/kubernetes/
rm -rf /var/lib/kubelet/
rm -rf /var/lib/etcd/
rm -rf /etc/cni/net.d/
rm -rf /opt/cni/
rm -rf $HOME/.kube/

# 6. 清理网络规则
iptables -F 2>/dev/null || true
iptables -t nat -F 2>/dev/null || true
iptables -t mangle -F 2>/dev/null || true
iptables -X 2>/dev/null || true
ipvsadm -C 2>/dev/null || true
ip link delete cni0 2>/dev/null || true
ip link delete flannel.1 2>/dev/null || true

# 7. 清理残留接口
ip link show type dummy | grep -o '^[0-9]*: [^@]*' | cut -d: -f2 | tr -d ' ' | while read iface; do
    ip link delete "$iface" 2>/dev/null || true
done

echo "=== Node ${NODE_NAME} has been fully reset ==="
```

---

## 参考

- [reset.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/reset.go)
- [cleanupnode.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go)
- [官方文档: kubeadm reset](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/)

## Related

- [[README|README]]
- [[man/INSTALL|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git|git]]
