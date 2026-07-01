---
title: 集群删除故障排查手册 (topic-code-analysis)
description: 'title: 集群删除故障排查手册'
category: general
tags:
- reference
- troubleshooting
- etcd
- apiserver
- kubelet
- containerd
- docker
- pdb
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 集群删除故障排查手册 是什么
- 如何 集群删除故障排查手册
- Kubernetes 07 platform engineering 最佳实践
- 集群删除故障排查手册 故障排查
- 集群删除故障排查手册 排障步骤
trigger_keywords:
- 集群删除故障排查手册
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

title: 集群删除故障排查手册
category: cluster-delete
tags:
- troubleshooting
- reset
- error
- kubectl
- drain
- etcd
- container
- unmount
- kubernetes
last_updated: 2026-05-18
description: 集群删除过程中常遇到各种异常：reset 卡住、etcd 移除失败、容器无法删除、网络规则残留等。本文档汇总常见问题场景，提供系统化的排查方法和解决方案，涵盖
  kubeadm reset、etcd、容器删除、卸载、kubectl delete node 以及重新初始化失败等场景。
difficulty: intermediate
intent_queries:
- kubeadm reset troubleshooting common errors
- kubernetes cluster deletion failure recovery
- etcd member removal failure kubernetes
- kubectl drain failure troubleshooting
- container deletion failure kubernetes cluster
trigger_keywords:
- reset卡住
- etcd移除失败
- failed to unmount
- device busy
- could not obtain a client
- unhealthy cluster
- port 10250 is in use
- ProgressDeadlineExceeded equivalent
- container runtime error
- NFS mount stuck
reading_level: intermediate
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
- force-delete
- ha-delete
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

# 集群删除故障排查手册

## 概述

集群删除过程中常遇到各种异常：reset 卡住、etcd 移除失败、容器无法删除、网络规则残留等。本文档汇总常见问题场景，提供系统化的排查方法。

---

## 1. kubeadm reset 常见错误

### 1.1 reset 卡住不动

**症状**: `kubeadm reset` 执行后长时间无输出

**排查**:

```bash
# 查看 kubeadm 进程状态
ps aux | grep kubeadm

# 查看是否卡在容器删除
crictl ps
ctr -n k8s.io tasks ls

# 查看是否卡在 umount
cat /proc/$(pgrep kubeadm)/wchan

# 查看系统日志
journalctl -u kubelet -f
dmesg -w
```

**常见原因与解决**:

| 原因 | 解决 |
|------|------|
| CRI 连接超时 | 重启 containerd: `systemctl restart containerd` |
| umount 卡在 NFS | 强制卸载: `umount -l /var/lib/kubelet/pods/.../volumes/...` |
| 大量容器删除慢 | 先手动删除: `crictl rmp -a` |

### 1.2 "[preflight] Some fatal errors occurred"

**症状**:
```
[preflight] Some fatal errors occurred:
[ERROR IsPrivilegedUser]: user is not running as root
```

**解决**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
sudo kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置
```

reset 只检查 root 权限（`RunRootCheckOnly`），不检查其他系统条件。

### 1.3 "Could not obtain a client set"

**症状**:
```
[reset] Could not obtain a client set from the kubeconfig file: /etc/kubernetes/admin.conf
```

**分析**: 这是 **Warning**，不是 Error。API Server 已不可用（控制面组件已停止），但 reset 仍可继续。

**处理**: 无需处理，reset 会以降级模式执行（跳过 etcd 成员移除）。

---

## 2. etcd 相关问题

### 2.1 "Failed to remove etcd member"

**症状**:
```
[reset] Failed to remove etcd member: context deadline exceeded
```

**排查**:

```bash
# 检查 etcd Pod 是否还在运行
crictl ps | grep etcd

# 检查 etcd 健康状态
ETCDCTL_API=3 etcdctl endpoint health \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key \
  --endpoints=https://127.0.0.1:2379

# 检查 etcd 成员列表
ETCDCTL_API=3 etcdctl member list --write-out=table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/peer.crt \
  --key=/etc/kubernetes/pki/etcd/peer.key
```

**常见原因**:

| 原因 | 解决 |
|------|------|
| etcd 不健康 | 修复 etcd 后再 reset，或 `--skip-phases=remove-etcd-member` |
| 仲裁丢失 | 手动移除成员或直接清理数据 |
| 证书过期 | 使用 `--skip-phases` 跳过 |
| 网络不通 | 直接 `rm -rf /var/lib/etcd` |

### 2.2 "etcdserver: unhealthy cluster"

**症状**:
```
[reset] Failed to remove etcd member: etcdserver: unhealthy cluster
```

**处理**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
# 方案 1: 跳过 etcd 移除，直接清理
kubeadm reset -f --skip-phases=remove-etcd-member  # ⚠️ 清理节点所有 K8s 配置
rm -rf /var/lib/etcd  # ⚠️ 删除系统/数据文件

# 方案 2: 手动恢复 etcd 仲裁后再移除
# 在健康的 etcd 节点上
etcdctl endpoint health --cluster
etcdctl member list
# 移除不健康的成员
etcdctl member remove <unhealthy-member-id>  # ⚠️ 移除 etcd 成员，可能丢数据
```

### 2.3 "No etcd config found"

**症状**:
```
[reset] No etcd config found. Assuming external etcd
[reset] Please, manually reset etcd to prevent further issues
```

**分析**: 这是**信息提示**，不是错误。出现在：
- 工作节点（无 etcd）
- `/etc/kubernetes/manifests/etcd.yaml` 已被删除
- 使用外部 etcd

**处理**: 工作节点无需处理。控制面节点如果使用 stacked etcd，需要手动清理 `/var/lib/etcd`。

---

## 3. 容器删除问题

### 3.1 "Failed to remove containers"

**症状**:
```
[reset] Failed to remove containers: failed to connect to CRI
```

**排查**:

```bash
# 检查容器运行时状态
systemctl status containerd
systemctl status docker

# 检查 CRI socket 是否存在
ls -la /run/containerd/containerd.sock
ls -la /var/run/crio/crio.sock

# 测试 CRI 连接
crictl info
```

**解决**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 重启容器运行时
systemctl restart containerd

# 或指定 CRI socket
kubeadm reset -f --cri-socket=unix:///run/containerd/containerd.sock  # ⚠️ 清理节点所有 K8s 配置

# 如果 CRI 完全不可用，手动清理
systemctl stop containerd
rm -rf /var/lib/containerd/*  # ⚠️ 删除系统/数据文件
systemctl start containerd
kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置
```

### 3.2 容器无法停止（任务忙碌）

**症状**: `crictl stop` 超时

```bash
# 强制停止所有容器
crictl stop $(crictl ps -q) 2>/dev/null || true
crictl rmp $(crictl ps -a -q) 2>/dev/null || true

# 使用 ctr 直接操作 containerd
ctr -n k8s.io tasks kill $(ctr -n k8s.io tasks -q) -s SIGKILL
ctr -n k8s.io containers rm $(ctr -n k8s.io containers -q)
ctr -n k8s.io snapshots rm $(ctr -n k8s.io snapshots -q)
```

---

## 4. 卸载问题

### 4.1 "failed to unmount" / device busy

**症状**:
```
[reset] encountered the following errors while unmounting directories in "/var/lib/kubelet":
failed to unmount "/var/lib/kubelet/pods/xxx/volumes/xxx": device or resource busy
```

**排查**:

```bash
# 查看哪些进程在使用挂载点
fuser -vm /var/lib/kubelet/pods/xxx/volumes/xxx

# 或使用 lsof
lsof +D /var/lib/kubelet/pods/xxx/volumes/xxx

# 查看挂载详情
mount | grep kubelet
cat /proc/mounts | grep kubelet
```

**解决**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 方案 1: 杀死占用进程
fuser -km /var/lib/kubelet/pods/xxx/volumes/xxx

# 方案 2: 懒卸载
umount -l /var/lib/kubelet/pods/xxx/volumes/xxx

# 方案 3: 强制卸载
umount -f /var/lib/kubelet/pods/xxx/volumes/xxx

# 方案 4: 使用 MNT_DETACH 标志
kubeadm reset -f --config=reset.yaml  # ⚠️ 清理节点所有 K8s 配置
# reset.yaml:
# unmountFlags: ["MNT_DETACH"]
```

### 4.2 NFS 挂载点卡死

**症状**: `umount` 命令完全卡死，无法中断

**解决**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 使用懒卸载（不等待远程响应）
umount -l /var/lib/kubelet/pods/xxx/volumes/kubernetes.io~nfs/xxx

# 如果连 umount -l 也卡死
# 1. 在另一个终端执行
umount -f /var/lib/kubelet/pods/xxx/volumes/kubernetes.io~nfs/xxx

# 2. 最后手段：重启
reboot
# 重启后执行 kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置
```

---

## 5. kubectl delete node 问题

### 5.1 Node 对象删除卡住

**症状**: `kubectl delete node` 一直不返回

**原因**: kubelet 正在运行，会持续更新 Node 状态，Controller 也在处理关联 Pod。

**解决**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 方案 1: 先停止目标节点的 kubelet
ssh <node> "systemctl stop kubelet"
kubectl delete node <node>

# 方案 2: 强制删除（不等优雅终止）
kubectl delete node <node> --force --grace-period=0

# 方案 3: 最终化器卡住
kubectl patch node <node> -p '{"metadata":{"finalizers":null}}' --type=merge
kubectl delete node <node>
```

### 5.2 drain 失败

**症状**:
```
error: unable to drain node "<node>", aborting command...
there are pending pods when an error occurred...
```

**常见原因**:

| 原因 | 解决 |
|------|------|
| Pod 使用了 local PV | `--delete-emptydir-data` 或手动处理 |
| Pod 有 PodDisruptionBudget | 降低 PDB 或等待 |
| DaemonSet Pod | `--ignore-daemonsets` |
| 无法驱逐的 Pod（unmanaged） | `--force` |

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 强制 drain
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force --timeout=60s

# 如果还是失败，跳过 drain 直接删除
kubectl delete node <node> --force --grace-period=0
```

---

## 6. 重新初始化失败

### 6.1 "Port 10250 is in use"

**原因**: 旧容器或进程仍在使用端口。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 查看端口占用
ss -tlnp | grep 10250

# 杀死占用进程
fuser -k 10250/tcp

# 重启容器运行时
systemctl restart containerd
```

### 6.2 "[ERROR DirAvailable--etc-kubernetes-manifests]"

**原因**: `/etc/kubernetes/manifests/` 目录中有残留文件。

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
rm -rf /etc/kubernetes/manifests/*  # ⚠️ 删除系统/数据文件
kubeadm init ...
```

### 6.3 "[ERROR CRI]: unable to check image"

**原因**: 容器运行时配置残留。

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 重置容器运行时
systemctl stop containerd
rm -rf /var/lib/containerd/*  # ⚠️ 删除系统/数据文件
systemctl start containerd
```

### 6.4 etcd 数据残留导致初始化失败

**症状**:
```
[etcd] Failed to start etcd: member count: 0
```

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

```bash
# 确保 etcd 数据已完全清除
rm -rf /var/lib/etcd  # ⚠️ 删除系统/数据文件
# 确保目录存在
mkdir -p /var/lib/etcd
```

---

## 7. 日志排查速查

| 组件 | 日志命令 |
|------|---------|
| kubeadm | `kubeadm reset -v=5` (增加日志级别) |
| kubelet | `journalctl -u kubelet -f` |
| containerd | `journalctl -u containerd -f` |
| etcd | `crictl logs $(crictl ps --name etcd -q)` |
| API Server | `crictl logs $(crictl ps --name kube-apiserver -q)` |
| 内核 | `dmesg -w` |
| 系统消息 | `journalctl -f` |

**高级调试**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 使用最高日志级别
kubeadm reset -v=10  # ⚠️ 清理节点所有 K8s 配置

# strace 跟踪系统调用
strace -f -e trace=umount kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置
```

---

## 参考

- [kubeadm reset 故障排查](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/)
- [kubectl drain 问题](https://kubernetes.io/docs/tasks/administer-cluster/safely-drain-node/)
- [etcd 故障恢复](https://etcd.io/docs/latest/op-guide/recovery/)

## Related

- [[README|README]]
- [[scripts/man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/kubernetes.md|kubernetes]]
