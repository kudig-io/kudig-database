---
title: 节点清理机制 — cleanup-node 源码分析 (topic-code-analysis)
description: 'description: 深入分析 kubeadm reset cleanup-node 阶段的源码实现，涵盖停止 kubelet 服务、卸载挂载点、移除容器、清理配置目录（pki/manifests/kubelet）、删除'
summary: 'description: 深入分析 kubeadm reset cleanup-node 阶段的源码实现，涵盖停止 kubelet 服务、卸载挂载点、移除容器、清理配置目录（pki/manifests/kubelet）、删除'
category: general
tags:
- reference
- etcd
- kubelet
- scheduler
- controller-manager
- containerd
- cri-o
- docker
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点清理机制 — cleanup-node 源码分析 是什么
- 如何 节点清理机制 — cleanup-node 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点清理机制
- cleanup-node
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 节点清理机制 — cleanup-node 源码分析
category: cluster-delete
tags:
- cleanup-node
- cleanup
- kubelet
- unmount
- container
- cri
- pki
- kubeconfig
last_updated: 2026-05-18
description: 深入分析 kubeadm reset cleanup-node 阶段的源码实现，涵盖停止 kubelet 服务、卸载挂载点、移除容器、清理配置目录（pki/manifests/kubelet）、删除
  kubeconfig 文件以及 Rootless 模式清理等完整流程。
difficulty: advanced
intent_queries:
- kubeadm cleanup-node phase source code
- cleanup-node cleanup kubernetes kubelet
- unmountKubeletDirectory kubernetes
- removeContainers kubernetes CRI
- CleanDir kubernetes kubeadm
trigger_keywords:
- cleanup-node
- stopKubelet
- unmountKubeletDirectory
- removeContainers
- CleanDir
- MNT_FORCE
- MNT_DETACH
- ListKubeContainers
- RemoveContainers
- RemoveUsersAndGroups
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
- etcd-cleanup
- force-delete
- security-delete
- network-cleanup
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

# 节点清理机制 — cleanup-node 源码分析

## 概述

`cleanup-node` 是 `kubeadm reset` 的核心阶段，负责停止服务、删除容器、卸载挂载点、清理配置目录和证书文件。本文档基于源码深入分析每一步清理操作。

---

## 源码路径

- 清理入口: `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go`
- 卸载逻辑: `cmd/kubeadm/app/cmd/phases/reset/unmount.go` / `unmount_linux.go`
- 容器运行时: `cmd/kubeadm/app/util/runtime/`

---

## 清理流程总览

```
┌──────────────────────────────────────────────────────────────┐
│  cleanup-node 阶段                                            │
├──────────────────────────────────────────────────────────────┤
│  1. 停止 kubelet 服务                                         │
│  2. 卸载 /var/lib/kubelet 下的挂载点                          │
│  3. 移除 Kubernetes 管理的所有容器                             │
│  4. 清理配置和证书目录                                         │
│  5. 删除 kubeconfig 文件                                      │
│  6. (可选) 清理 tmp 目录                                      │
│  7. (可选) 移除 Rootless 模式的用户和组                        │
└──────────────────────────────────────────────────────────────┘
```

---

## 1. 停止 kubelet 服务

**源码**: `cleanupnode.go` → `runCleanupNode()`

```go
initSystem, err := initsystem.GetInitSystem()
if err != nil {
    klog.Warningln("[reset] The kubelet service could not be stopped by kubeadm.")
    klog.Warningln("[reset] Please ensure kubelet is stopped manually")
} else {
    if !r.DryRun() {
        fmt.Println("[reset] Stopping the kubelet service")
        if err := initSystem.ServiceStop("kubelet"); err != nil {
            klog.Warningf("[reset] The kubelet service could not be stopped: [%v]\n", err)
        }
    }
}
```

**关键**:
- 使用 `initsystem.GetInitSystem()` 自动检测 init 系统（systemd / initd / openrc）
- 停止失败仅 **warning**，不中断 reset 流程
- kubelet 停止是后续容器删除的前提

---

## 2. 卸载 kubelet 挂载点

### 2.1 符号链接解析

```go
kubeletRunDirectory, err := filepath.EvalSymlinks(kubeadmconstants.KubeletRunDirectory)
if err != nil {
    klog.Warningf("[reset] Skipping unmount of directories in %q: %v\n",
        kubeadmconstants.KubeletRunDirectory, err)
}
```

`KubeletRunDirectory` 默认为 `/var/lib/kubelet`，先解析可能的符号链接。

### 2.2 Linux 卸载实现

**源码**: `unmount_linux.go`

```go
func unmountKubeletDirectory(kubeletRunDirectory string, flags []string) error {
    raw, err := os.ReadFile("/proc/mounts")
    if err != nil {
        return err
    }

    if !strings.HasSuffix(kubeletRunDirectory, "/") {
        kubeletRunDirectory += "/"
    }

    var errList []error
    mounts := strings.Split(string(raw), "\n")
    flagsInt := flagsToInt(flags)

    for _, mount := range mounts {
        m := strings.Split(mount, " ")
        if len(m) < 2 || !strings.HasPrefix(m[1], kubeletRunDirectory) {
            continue
        }
        if err := syscall.Unmount(m[1], flagsInt); err != nil {
            if err == syscall.EINVAL {
                klog.Warningf("[reset] Ignoring EINVAL error while unmounting %q", m[1])
                continue
            }
            errList = append(errList, errors.WithMessagef(err, "failed to unmount %q", m[1]))
        }
    }
    return errors.Wrapf(utilerrors.NewAggregate(errList), ...)
}
```

### 2.3 卸载标志

```go
var flagMap = map[string]int{
    kubeadmapi.UnmountFlagMNTForce:       unix.MNT_FORCE,
    kubeadmapi.UnmountFlagMNTDetach:      unix.MNT_DETACH,
    kubeadmapi.UnmountFlagMNTExpire:      unix.MNT_EXPIRE,
    kubeadmapi.UnmountFlagUmountNoFollow: unix.UMOUNT_NOFOLLOW,
}
```

| 标志 | 值 | 说明 |
|------|-----|------|
| `MNT_FORCE` | 1 | 强制卸载，即使有进程在使用 |
| `MNT_DETACH` | 2 | 懒卸载，标记为不可用，等引用消失后卸载 |
| `MNT_EXPIRE` | 4 | 标记为过期，下次非过期卸载时真正卸载 |
| `UMOUNT_NOFOLLOW` | 8 | 不跟随符号链接 |

**使用方法**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
kubeadm reset --config=reset.yaml  # ⚠️ 清理节点所有 K8s 配置
# reset.yaml:
# unmountFlags:
#   - MNT_DETACH
```

### 2.4 非 Linux 平台

**源码**: `unmount.go` (非 Linux 构建标签)

```go
func unmountKubeletDirectory(kubeletRunDirectory string, flags []string) error {
    klog.Warning("Cannot unmount filesystems on current OS, all mounted file systems will need to be manually unmounted")
    return nil
}
```

非 Linux 平台（如 macOS、Windows）为 **NOOP**，仅打印警告。

---

## 3. 移除容器

### 3.1 容器移除流程

```go
func removeContainers(criSocketPath string) error {
    containerRuntime := utilruntime.NewContainerRuntime(criSocketPath)
    if err := containerRuntime.Connect(); err != nil {
        return err
    }
    defer containerRuntime.Close(context.Background())

    containers, err := containerRuntime.ListKubeContainers()
    if err != nil {
        return err
    }
    return containerRuntime.RemoveContainers(containers)
}
```

**流程**:
1. 通过 CRI Socket 连接容器运行时（containerd / dockerd / cri-o）
2. 列出所有 Kubernetes 管理的容器
3. 批量删除这些容器

### 3.2 CRI Socket 自动检测

```
┌────────────────────────────────────────────────────────┐
│  CRI Socket 检测优先级                                   │
├────────────────────────────────────────────────────────┤
│  1. --cri-socket 命令行参数                              │
│  2. ResetConfiguration.criSocket                        │
│  3. InitConfiguration.nodeRegistration.criSocket        │
│  4. 自动检测:                                            │
│     ├─ /run/containerd/containerd.sock                  │
│     ├─ /run/crio/crio.sock                              │
│     └─ /var/run/dockershim.sock (已弃用)                │
└────────────────────────────────────────────────────────┘
```

### 3.3 ListKubeContainers 的筛选逻辑

容器运行时接口通过 CRI（Container Runtime Interface）gRPC 调用：
- `RuntimeService.ListContainers()` — 列出所有容器
- 通过 Pod metadata 中的 Kubernetes namespace/label 筛选 Kube 管理的容器
- `RuntimeService.RemoveContainer()` — 逐个删除

---

## 4. 目录清理

### 4.1 需要清理的目录

```go
dirsToClean := []string{
    filepath.Join(kubeadmconstants.KubernetesDir, kubeadmconstants.ManifestsSubDirName),
}
// /etc/kubernetes/manifests

dirsToClean = append(dirsToClean, certsDir)
// /etc/kubernetes/pki

if r.CleanupTmpDir() {
    tempDir := path.Join(kubeadmconstants.KubernetesDir, kubeadmconstants.TempDir)
    dirsToClean = append(dirsToClean, tempDir)
}
// /etc/kubernetes/tmp (如果指定 --cleanup-tmp-dir)

// 卸载后追加
dirsToClean = append(dirsToClean, kubeletRunDirectory)
// /var/lib/kubelet
```

**完整目录列表**:

| 目录 | 说明 |
|------|------|
| `/etc/kubernetes/manifests/` | 静态 Pod manifests |
| `/etc/kubernetes/pki/` | 证书和密钥 |
| `/etc/kubernetes/tmp/` | 临时文件（需 `--cleanup-tmp-dir`） |
| `/var/lib/kubelet/` | kubelet 数据（卸载后清理） |

### 4.2 CleanDir 实现

```go
func CleanDir(filePath string) error {
    if _, err := os.Stat(filePath); os.IsNotExist(err) {
        return nil
    }
    d, err := os.Open(filePath)
    if err != nil {
        return err
    }
    defer d.Close()
    names, err := d.Readdirnames(-1)
    if err != nil {
        return err
    }
    for _, name := range names {
        if err = os.RemoveAll(filepath.Join(filePath, name)); err != nil {
            return err
        }
    }
    return nil
}
```

**关键**: `CleanDir` 只清理目录**内容**，不删除目录本身。这保留了目录的权限和属主设置。

---

## 5. kubeconfig 文件删除

### 5.1 删除的文件列表

```go
filesToClean := []string{
    filepath.Join(configPathDir, kubeadmconstants.AdminKubeConfigFileName),
    // /etc/kubernetes/admin.conf

    filepath.Join(configPathDir, kubeadmconstants.SuperAdminKubeConfigFileName),
    // /etc/kubernetes/super-admin.conf

    filepath.Join(configPathDir, kubeadmconstants.KubeletKubeConfigFileName),
    // /etc/kubernetes/kubelet.conf

    filepath.Join(configPathDir, kubeadmconstants.KubeletBootstrapKubeConfigFileName),
    // /etc/kubernetes/bootstrap-kubelet.conf

    filepath.Join(configPathDir, kubeadmconstants.ControllerManagerKubeConfigFileName),
    // /etc/kubernetes/controller-manager.conf

    filepath.Join(configPathDir, kubeadmconstants.SchedulerKubeConfigFileName),
    // /etc/kubernetes/scheduler.conf
}
```

### 5.2 kubeconfig 删除策略

```
┌────────────────────────────────────────────────────────────┐
│  kubeconfig 文件                                             │
├────────────────────────────────────────────────────────────┤
│  admin.conf              → 管理员 kubeconfig                │
│  super-admin.conf        → 超级管理员 kubeconfig (v1.29+)   │
│  kubelet.conf            → kubelet 连接 API Server          │
│  bootstrap-kubelet.conf  → TLS Bootstrap 阶段的临时配置      │
│  controller-manager.conf → kube-controller-manager 使用     │
│  scheduler.conf          → kube-scheduler 使用              │
└────────────────────────────────────────────────────────────┘
```

**注意**: 这些文件使用 `os.RemoveAll` 删除（文件被删除）。

---

## 6. Rootless 模式清理

```go
if r.Cfg() != nil && features.Enabled(r.Cfg().FeatureGates, features.RootlessControlPlane) {
    if !r.DryRun() {
        klog.V(1).Infoln("[reset] Removing users and groups created for rootless control-plane")
        if err := users.RemoveUsersAndGroups(); err != nil {
            klog.Warningf("[reset] Failed to remove users and groups: %v\n", err)
        }
    }
}
```

当启用了 `RootlessControlPlane` Feature Gate 时，reset 还会移除为非 root 控制面创建的系统用户和组。

---

## 7. 清理流程图

```
┌─────────────────────────────────────────────────────────────────┐
│  cleanup-node 执行流程                                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐                                                │
│  │ 停止 kubelet  │ ─── 失败 → warning，继续                      │
│  └──────┬───────┘                                                │
│         ▼                                                         │
│  ┌──────────────────────┐                                        │
│  │ 卸载 /var/lib/kubelet │ ─── 解析符号链接 → 读取 /proc/mounts  │
│  │ 下的所有挂载点         │    → syscall.Unmount 每个挂载点        │
│  └──────┬───────────────┘                                        │
│         ▼                                                         │
│  ┌────────────────┐                                              │
│  │ 移除所有容器    │ ─── CRI 连接 → ListKubeContainers → Remove   │
│  └──────┬─────────┘                                              │
│         ▼                                                         │
│  ┌──────────────────────────────────────┐                        │
│  │ resetConfigDir()                      │                        │
│  │  ├─ CleanDir: manifests/ pki/ kubelet │                        │
│  │  ├─ CleanDir: tmp/ (可选)             │                        │
│  │  └─ RemoveAll: kubeconfig 文件 × 6    │                        │
│  └──────┬───────────────────────────────┘                        │
│         ▼                                                         │
│  ┌────────────────────────────────┐                              │
│  │ RemoveUsersAndGroups (可选)     │                              │
│  └────────────────────────────────┘                              │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘

```

---

## 8. 不被自动清理的内容

`kubeadm reset` **不会**自动清理以下内容：

| 项目 | 路径/命令 | 原因 |
|------|----------|------|
| CNI 配置 | `/etc/cni/net.d/` | 不同 CNI 插件有不同清理需求 |
| iptables 规则 | `iptables -F` | 可能影响非 Kubernetes 规则 |
| ipvs 规则 | `ipvsadm -C` | 同上 |
| 用户 kubeconfig | `$HOME/.kube/config` | 用户私有文件 |
| etcd 数据 | `/var/lib/etcd/` | 由 remove-etcd-member 阶段处理 |
| 容器运行时数据 | `/var/lib/containerd/` 等 | 运行时自身数据 |

**手动清理命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X
ipvsadm -C
rm -rf /etc/cni/net.d  # ⚠️ 删除系统/数据文件
rm -rf $HOME/.kube/config  # ⚠️ 删除系统/数据文件
```

---

## 参考

- [cleanupnode.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go)
- [unmount_linux.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/reset/unmount_linux.go)
- [container runtime util](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm/app/util/runtime/)

## Related

- [[reference|#reference Hub]] — tag hub

- [[README|README]]
- [[scripts/man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]

```

<!-- risk-assessed -->
