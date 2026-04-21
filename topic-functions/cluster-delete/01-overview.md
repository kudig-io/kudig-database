# Kubernetes 集群删除逻辑 — 基于官方代码分析

## 概述

Kubernetes 集群删除涉及多个层面：**节点级重置**（`kubeadm reset`）、**API 对象删除**（`kubectl delete node`）、**etcd 成员移除** 以及 **系统级清理**（容器、网络、证书、数据目录）。本文档基于 `kubernetes/kubernetes` 源码，系统分析集群删除的完整逻辑。

---

## 源码路径

- reset 入口: `cmd/kubeadm/app/cmd/reset.go`
- reset phases: `cmd/kubeadm/app/cmd/phases/reset/`
- etcd 操作: `cmd/kubeadm/app/phases/etcd/local.go`
- workflow 引擎: `cmd/kubeadm/app/cmd/phases/workflow/runner.go`

---

## 删除方式对比

```
┌─────────────────────────────────────────────────────────────────────┐
│                     集群删除方式                                      │
├──────────────────────┬──────────────────────────────────────────────┤
│ kubeadm reset        │ 单节点重置：停止 kubelet、删除容器/配置/证书   │
│ kubectl delete node  │ API 层删除：从 etcd 移除 Node 对象            │
│ kubeadm reset --force│ 强制重置：跳过确认提示                        │
│ 手动清理             │ iptables、CNI、/var/lib/kubelet 等             │
└──────────────────────┴──────────────────────────────────────────────┘
```

---

## kubeadm reset 流程总览

```
┌─────────────────────────────────────────────────────────────┐
│                     kubeadm reset                             │
├─────────────────────────────────────────────────────────────┤
│  1. preflight            预检检查（root 权限确认）            │
│  2. remove-etcd-member   从 etcd 集群移除本地成员             │
│  3. cleanup-node         节点清理（核心阶段）                  │
│     ├─ 停止 kubelet 服务                                     │
│     ├─ 卸载 /var/lib/kubelet 下的挂载点                       │
│     ├─ 移除 Kubernetes 管理的容器                             │
│     ├─ 清理 /etc/kubernetes/manifests/                       │
│     ├─ 清理 /etc/kubernetes/pki/                             │
│     ├─ 删除 kubeconfig 文件                                   │
│     └─ 清理 tmp 目录（可选）                                  │
└─────────────────────────────────────────────────────────────┘
```

---

## 核心代码分析

### 1. 入口: cmd/kubeadm/app/cmd/reset.go

```go
func newCmdReset(in io.Reader, out io.Writer, resetOptions *resetOptions) *cobra.Command {
    resetRunner := workflow.NewRunner()

    cmd := &cobra.Command{
        Use:   "reset",
        Short: "Performs a best effort revert of changes made to this host by 'kubeadm init' or 'kubeadm join'",
        RunE: func(cmd *cobra.Command, args []string) error {
            data, err := resetRunner.InitData(args)
            if err != nil {
                return err
            }
            if err := resetRunner.Run(args); err != nil {
                return err
            }
            fmt.Print(manualCleanupInstructions)
            return nil
        },
    }

    resetRunner.AppendPhase(phases.NewPreflightPhase())
    resetRunner.AppendPhase(phases.NewRemoveETCDMemberPhase())
    resetRunner.AppendPhase(phases.NewCleanupNodePhase())

    resetRunner.SetDataInitializer(func(cmd *cobra.Command, args []string) (workflow.RunData, error) {
        return newResetData(cmd, resetOptions, in, out, true)
    })

    resetRunner.BindToCommand(cmd)
    return cmd
}
```

**关键设计**:
- 使用 `workflow.Runner` 管理 Phase 执行，与 `kubeadm init` 共享同一 workflow 引擎
- 三个 Phase 按固定顺序执行：`preflight` → `remove-etcd-member` → `cleanup-node`
- 支持 `--skip-phases` 跳过特定阶段
- 执行完成后打印手动清理提示（CNI 配置、iptables 规则等需要手动清理）

---

### 2. 数据结构: ResetConfiguration

```go
type ResetConfiguration struct {
    metav1.TypeMeta

    CertificatesDir       string   // 证书目录，默认 /etc/kubernetes/pki
    CleanupTmpDir         bool     // 是否清理 /etc/kubernetes/tmp
    CRISocket             string   // 容器运行时 socket
    DryRun                bool     // 干跑模式
    Force                 bool     // 跳过确认提示
    IgnorePreflightErrors []string // 忽略的预检错误
    SkipPhases            []string // 跳过的阶段
    UnmountFlags          []string // Linux unmount2() 系统调用标志
    Timeouts              *Timeouts
}
```

---

### 3. resetData 接口

```go
type resetData interface {
    ForceReset() bool
    InputReader() io.Reader
    IgnorePreflightErrors() sets.Set[string]
    Cfg() *kubeadmapi.InitConfiguration
    ResetCfg() *kubeadmapi.ResetConfiguration
    DryRun() bool
    Client() clientset.Interface
    CertificatesDir() string
    CRISocketPath() string
    CleanupTmpDir() bool
}
```

**关键**: `resetData` 通过接口抽象，支持测试注入和 dry-run 模式。`Cfg()` 返回从集群 `kubeadm-config` ConfigMap 获取的 `InitConfiguration`，如果无法连接 API Server 则为 nil。

---

## 完整删除流程（生产环境推荐）

```
┌──────────────────────────────────────────────────────────────┐
│                  生产环境集群删除流程                           │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  1. 驱逐节点上的 Pod                                          │
│     kubectl drain <node> --ignore-daemonsets --delete-emptydir-data │
│                                                              │
│  2. 从 API 层删除 Node 对象                                   │
│     kubectl delete node <node>                                │
│                                                              │
│  3. 在目标节点执行 reset                                      │
│     kubeadm reset                                             │
│                                                              │
│  4. 手动清理残留                                              │
│     - iptables -F                                             │
│     - ipvsadm --clear                                         │
│     - rm -rf /etc/cni/net.d                                   │
│     - rm -rf /var/lib/kubelet                                 │
│     - rm -rf $HOME/.kube                                      │
│                                                              │
│  5. (控制面节点) 确认 etcd 成员已移除                          │
│     etcdctl member list                                       │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## 命令行标志

| 标志 | 说明 | 默认值 |
|------|------|--------|
| `--certificates-dir` | 证书目录 | `/etc/kubernetes/pki` |
| `--cleanup-tmp-dir` | 清理 tmp 目录 | `false` |
| `--cri-socket` | 容器运行时 socket | 自动检测 |
| `--dry-run` | 干跑模式 | `false` |
| `-f, --force` | 跳过确认 | `false` |
| `--ignore-preflight-errors` | 忽略预检错误 | |
| `--skip-phases` | 跳过阶段 | |
| `--kubeconfig` | kubeconfig 路径 | `/etc/kubernetes/admin.conf` |

---

## 手动清理提示

`kubeadm reset` 执行完成后会输出以下提示：

```
The reset process does not perform cleanup of CNI plugin configuration,
network filtering rules and kubeconfig files.

For information on how to perform this cleanup manually, please see:
    https://k8s.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/
```

**需要手动清理的内容**:
- CNI 配置: `/etc/cni/net.d/`
- iptables 规则: `iptables -F && iptables -t nat -F && iptables -t mangle -F`
- IPVS 规则: `ipvsadm -C`
- kubeconfig: `$HOME/.kube/config`
- kubelet 数据: `/var/lib/kubelet/`（如果未被 reset 清理）

---

## 参考

- [kubeadm reset 源码](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm/app/cmd/reset.go)
- [官方文档: kubeadm reset](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/)
- [reset phases 源码](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm/app/cmd/phases/reset/)
