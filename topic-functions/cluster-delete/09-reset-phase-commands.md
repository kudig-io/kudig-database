# reset 子命令与 Phase 操作速查

## 概述

与 `kubeadm init phase` 类似，`kubeadm reset` 也支持通过子命令单独执行各个阶段。本文档提供完整的子命令速查和操作示例。

---

## 1. reset phase 子命令

### 1.1 查看所有 phase

```bash
kubeadm reset phase --help
```

输出:
```
Use "kubeadm reset phase <command> --help" for more information about a given command.

Available Commands:
  cleanup-node        Run cleanup node
  preflight           Run reset pre-flight checks
  remove-etcd-member  Remove a local etcd member
```

### 1.2 子命令详细参数

#### preflight

```bash
kubeadm reset phase preflight --help
```

| 标志 | 继承自 | 说明 |
|------|--------|------|
| `--dry-run` | `options.DryRun` | 干跑模式 |
| `-f, --force` | `options.Force` | 跳过确认 |
| `--ignore-preflight-errors` | `options.IgnorePreflightErrors` | 忽略预检错误 |

```go
InheritFlags: []string{
    options.IgnorePreflightErrors,
    options.Force,
    options.DryRun,
}
```

#### remove-etcd-member

```bash
kubeadm reset phase remove-etcd-member --help
```

| 标志 | 继承自 | 说明 |
|------|--------|------|
| `--dry-run` | `options.DryRun` | 干跑模式 |
| `--kubeconfig` | `options.KubeconfigPath` | kubeconfig 路径 |

```go
InheritFlags: []string{
    options.KubeconfigPath,
    options.DryRun,
}
```

#### cleanup-node

```bash
kubeadm reset phase cleanup-node --help
```

| 标志 | 继承自 | 说明 |
|------|--------|------|
| `--certificates-dir` | `options.CertificatesDir` | 证书目录 |
| `--cri-socket` | `options.NodeCRISocket` | CRI socket |
| `--cleanup-tmp-dir` | `options.CleanupTmpDir` | 清理 tmp 目录 |
| `--dry-run` | `options.DryRun` | 干跑模式 |

```go
InheritFlags: []string{
    options.CertificatesDir,
    options.NodeCRISocket,
    options.CleanupTmpDir,
    options.DryRun,
}
```

---

## 2. 常用操作场景

### 2.1 仅移除 etcd 成员（不清理节点）

```bash
kubeadm reset phase remove-etcd-member --kubeconfig=/etc/kubernetes/admin.conf
```

**场景**: 需要从 etcd 集群中移除一个已宕机的控制面成员，但暂时保留节点上的其他数据。

### 2.2 仅清理节点（不处理 etcd）

```bash
kubeadm reset phase cleanup-node --certificates-dir=/etc/kubernetes/pki
```

**场景**: etcd 成员已手动移除，只需要清理节点上的配置和容器。

### 2.3 跳过确认直接清理

```bash
kubeadm reset phase preflight --force
kubeadm reset phase remove-etcd-member
kubeadm reset phase cleanup-node --cleanup-tmp-dir
```

### 2.4 Dry-run 各阶段

```bash
kubeadm reset phase preflight --dry-run
kubeadm reset phase remove-etcd-member --dry-run
kubeadm reset phase cleanup-node --dry-run
```

---

## 3. Phase 与 InheritFlags 对照

```
┌──────────────────────────────────────────────────────────────────┐
│  Phase                │ 继承的 Flags                              │
├───────────────────────┼──────────────────────────────────────────┤
│  preflight            │ IgnorePreflightErrors, Force, DryRun      │
│  remove-etcd-member   │ KubeconfigPath, DryRun                    │
│  cleanup-node         │ CertificatesDir, NodeCRISocket,           │
│                       │ CleanupTmpDir, DryRun                     │
└───────────────────────┴──────────────────────────────────────────┘
```

**设计**: 每个 Phase 通过 `InheritFlags` 声明它需要的命令行标志。`workflow.Runner.BindToCommand()` 会自动将继承的标志绑定到对应的 phase 子命令上。

---

## 4. 使用 --config 文件

### 4.1 ResetConfiguration 完整示例

```yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: ResetConfiguration
certificatesDir: /etc/kubernetes/pki
cleanupTmpDir: true
criSocket: unix:///run/containerd/containerd.sock
dryRun: false
force: true
ignorePreflightErrors:
  - IsPrivilegedUser
skipPhases:
  - preflight
unmountFlags:
  - MNT_DETACH
```

### 4.2 使用配置文件执行

```bash
kubeadm reset --config=reset-config.yaml
```

**优先级**: 命令行标志 > 配置文件 > 默认值

---

## 5. init vs reset Phase 对比

```
┌──────────────────────────────────────────────────────────────────┐
│  kubeadm init phases              │ kubeadm reset phases          │
├───────────────────────────────────┼──────────────────────────────┤
│  preflight                        │ preflight                     │
│  certs                            │ ─                            │
│  kubeconfig                       │ ─                            │
│  kubelet-start                    │ ─                            │
│  control-plane                    │ ─                            │
│  etcd                             │ remove-etcd-member           │
│  wait-control-plane               │ ─                            │
│  upload-config                    │ ─                            │
│  bootstrap-token                  │ ─                            │
│  mark-control-plane               │ ─                            │
│  addon                            │ ─                            │
│  ─                                │ cleanup-node                 │
├───────────────────────────────────┼──────────────────────────────┤
│  共 12 个 phase                   │ 共 3 个 phase                 │
└───────────────────────────────────┴──────────────────────────────┘
```

**关键差异**:
- init 是**构建**过程（12 个阶段），逐步创建组件
- reset 是**销毁**过程（3 个阶段），快速回滚
- reset 的 `cleanup-node` 是一个**聚合**阶段，一次性完成所有清理

---

## 6. 完整操作速查表

| 场景 | 命令 |
|------|------|
| 标准重置 | `kubeadm reset` |
| 强制重置（脚本用） | `kubeadm reset -f` |
| 仅移除 etcd | `kubeadm reset phase remove-etcd-member` |
| 仅清理节点 | `kubeadm reset phase cleanup-node` |
| 跳过 etcd 移除 | `kubeadm reset --skip-phases=remove-etcd-member` |
| 清理 tmp 目录 | `kubeadm reset --cleanup-tmp-dir` |
| 干跑模式 | `kubeadm reset --dry-run` |
| 使用配置文件 | `kubeadm reset --config=reset.yaml` |
| 指定 CRI socket | `kubeadm reset --cri-socket=unix:///run/containerd/containerd.sock` |
| 指定证书目录 | `kubeadm reset --certificates-dir=/custom/pki` |
| 忽略权限检查 | `kubeadm reset --ignore-preflight-errors=IsPrivilegedUser` |
| 懒卸载模式 | `kubeadm reset --config=reset.yaml` (配置 unmountFlags) |

---

## 7. 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 成功完成所有阶段 |
| 1 | 阶段执行失败（如 preflight 失败） |
| 2 | 命令行参数错误 |

**注意**: 由于 reset 采用 "best effort" 策略，大部分 warning 不会导致非零退出码。只有 preflight 阶段的确认拒绝或致命错误才会导致退出码非零。

---

## 参考

- [reset phase 源码](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm/app/cmd/phases/reset/)
- [workflow runner BindToCommand](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/workflow/runner.go)
- [官方文档: kubeadm reset phase](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/#phase)
