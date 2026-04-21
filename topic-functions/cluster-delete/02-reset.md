# kubeadm reset 源码分析

## 概述

`kubeadm reset` 是 Kubernetes 官方提供的节点级重置命令，用于 "best effort" 地回滚 `kubeadm init` 或 `kubeadm join` 对本机所做的修改。本文档基于源码深入分析其执行逻辑。

---

## 源码路径

- 命令入口: `cmd/kubeadm/app/cmd/reset.go`
- 预检阶段: `cmd/kubeadm/app/cmd/phases/reset/preflight.go`
- etcd 移除: `cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go`
- 节点清理: `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go`
- 卸载逻辑: `cmd/kubeadm/app/cmd/phases/reset/unmount.go` / `unmount_linux.go`

---

## 1. 命令注册与初始化

### 1.1 resetOptions

```go
type resetOptions struct {
    kubeconfigPath        string
    cfgPath               string
    ignorePreflightErrors []string
    externalcfg           *kubeadmapiv1.ResetConfiguration
    skipCRIDetect         bool
}
```

**源码**: `cmd/kubeadm/app/cmd/reset.go`

命令行标志通过 `AddResetFlags` 注册：

```go
func AddResetFlags(flagSet *flag.FlagSet, resetOptions *resetOptions) {
    flagSet.StringVar(&resetOptions.externalcfg.CertificatesDir, ...)
    flagSet.BoolVarP(&resetOptions.externalcfg.Force, "force", "f", ...)
    flagSet.BoolVar(&resetOptions.externalcfg.DryRun, ...)
    flagSet.BoolVar(&resetOptions.externalcfg.CleanupTmpDir, ...)
    options.AddKubeConfigFlag(flagSet, &resetOptions.kubeconfigPath)
    options.AddConfigFlag(flagSet, &resetOptions.cfgPath)
    options.AddIgnorePreflightErrorsFlag(flagSet, &resetOptions.ignorePreflightErrors)
    cmdutil.AddCRISocketFlag(flagSet, &resetOptions.externalcfg.CRISocket)
}
```

### 1.2 resetData 构建

```go
type resetData struct {
    certificatesDir       string
    client                clientset.Interface
    criSocketPath         string
    forceReset            bool
    ignorePreflightErrors sets.Set[string]
    inputReader           io.Reader
    outputWriter          io.Writer
    cfg                   *kubeadmapi.InitConfiguration
    resetCfg              *kubeadmapi.ResetConfiguration
    dryRun                bool
    cleanupTmpDir         bool
}
```

**构建流程** (`newResetData`):

```
┌──────────────────────────────────────────────────────────────┐
│  newResetData()                                               │
├──────────────────────────────────────────────────────────────┤
│  1. 加载或默认 ResetConfiguration                             │
│     configutil.LoadOrDefaultResetConfiguration()               │
│                                                                │
│  2. 构建 API Client                                            │
│     ├─ DryRun: 使用 FakeClient                                 │
│     └─ 正常: 从 admin.conf 构建 ClientSet                      │
│                                                                │
│  3. 从集群获取 InitConfiguration                               │
│     configutil.FetchInitConfigurationFromCluster()             │
│     (失败不中断，仅 warning)                                    │
│                                                                │
│  4. 检测 CRI Socket                                            │
│     ├─ 命令行指定 → 使用指定的                                  │
│     ├─ ResetConfiguration 中指定 → 使用配置的                   │
│     ├─ InitConfiguration 中有 → 使用集群配置的                  │
│     └─ 否则 → 自动检测容器运行时                                │
│                                                                │
│  5. 确定 CertificatesDir                                       │
│     优先级: 命令行 > ResetConfiguration > InitConfiguration > 默认 │
│                                                                │
│  6. 返回 resetData 结构体                                       │
└──────────────────────────────────────────────────────────────┘
```

**关键**: 如果无法连接 API Server（集群已不可用），`reset` 仍然可以执行。`cfg` 为 nil 时，etcd 移除阶段会跳过，但节点清理阶段仍可正常工作。

---

## 2. Phase 注册与执行

### 2.1 Phase 列表

```go
resetRunner.AppendPhase(phases.NewPreflightPhase())
resetRunner.AppendPhase(phases.NewRemoveETCDMemberPhase())
resetRunner.AppendPhase(phases.NewCleanupNodePhase())
```

**执行顺序**（固定，不可通过 `--skip-phases` 改变顺序）:

| # | Phase 名称 | 别名 | 说明 |
|---|-----------|------|------|
| 1 | `preflight` | `pre-flight` | 预检：root 权限检查、用户确认 |
| 2 | `remove-etcd-member` | — | 从 etcd 集群移除本地成员（仅控制面节点） |
| 3 | `cleanup-node` | `cleanupnode` | 节点清理：停止服务、删除容器、清理目录 |

### 2.2 Workflow Runner 执行机制

```
┌────────────────────────────────────────────────┐
│  workflow.Runner.Run()                          │
├────────────────────────────────────────────────┤
│  for each phase in phases:                      │
│    if phase in skipPhases:                      │
│      continue                                   │
│    phase.Run(data)                              │
└────────────────────────────────────────────────┘
```

每个 Phase 通过 `workflow.Phase` 结构体定义：

```go
type Phase struct {
    Name         string
    Aliases      []string
    Short        string
    Long         string
    Run          func(RunData) error
    InheritFlags []string
}
```

---

## 3. 预检阶段: preflight

**源码**: `cmd/kubeadm/app/cmd/phases/reset/preflight.go`

```go
func runPreflight(c workflow.RunData) error {
    r := c.(resetData)

    // 1. 非 force 模式下需要用户确认
    if !r.ForceReset() && !r.DryRun() {
        if err := util.InteractivelyConfirmAction("reset",
            "Are you sure you want to proceed?", r.InputReader()); err != nil {
            return err
        }
    }

    // 2. 执行 root 权限检查
    fmt.Println("[preflight] Running pre-flight checks")
    return preflight.RunRootCheckOnly(r.IgnorePreflightErrors())
}
```

**注意**: reset 预检只执行 **root 权限检查**，不执行 init/join 那样的完整预检（端口、系统参数等）。

---

## 4. 配置加载策略

### 4.1 ResetConfiguration 加载

```go
resetCfg, err := configutil.LoadOrDefaultResetConfiguration(
    opts.cfgPath,           // --config 指定的文件路径
    opts.externalcfg,       // 命令行标志构建的默认配置
    configutil.LoadOrDefaultConfigurationOptions{
        AllowExperimental: allowExperimental,
        SkipCRIDetect:     opts.skipCRIDetect,
    },
)
```

**优先级**: `--config` 文件 > 命令行标志 > 默认值

### 4.2 InitConfiguration 获取

```go
initCfg, err = configutil.FetchInitConfigurationFromCluster(
    client, nil, "reset",
    getNodeRegistration,    // true
    getAPIEndpoint,         // 仅控制面节点
    getComponentConfigs,    // true
)
```

从 `kube-system/kubeadm-config` ConfigMap 中读取集群配置。用于：
- 获取 etcd 数据目录路径
- 获取 CRI Socket 信息
- 判断节点角色（控制面/工作节点）

---

## 5. DryRun 模式

`kubeadm reset --dry-run` 使用 FakeClient 替代真实 API 调用：

```go
if dryRunFlag {
    dryRun := apiclient.NewDryRun().
        WithDefaultMarshalFunction().
        WithWriter(os.Stdout)
    dryRun.AppendReactor(dryRun.GetKubeadmConfigReactor()).
        AppendReactor(dryRun.GetKubeletConfigReactor()).
        AppendReactor(dryRun.GetKubeProxyConfigReactor())
    client = dryRun.FakeClient()
}
```

在 dry-run 模式下，所有实际操作被替换为打印 `[dryrun] Would ...` 消息：

```
[dryrun] Would stop the kubelet service
[dryrun] Would unmount mounted directories in "/var/lib/kubelet"
[dryrun] Would remove Kubernetes-managed containers
[dryrun] Would delete contents of directories: [...]
[dryrun] Would delete files: [...]
```

---

## 6. 跳过阶段

通过 `--skip-phases` 可以跳过任意阶段：

```bash
kubeadm reset --skip-phases=remove-etcd-member
```

**源码**:
```go
if len(resetRunner.Options.SkipPhases) == 0 {
    resetRunner.Options.SkipPhases = data.resetCfg.SkipPhases
}
```

命令行 `--skip-phases` 优先于配置文件中的 `skipPhases`。

---

## 7. 手动清理提示

reset 执行成功后，会打印手动清理提示：

```go
var manualCleanupInstructions = dedent.Dedent(`
    The reset process does not perform cleanup of CNI plugin configuration,
    network filtering rules and kubeconfig files.

    For information on how to perform this cleanup manually, please see:
        https://k8s.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/
`)
```

这是 reset 后**必须手动处理**的清理项：
- CNI 插件配置（`/etc/cni/net.d/`）
- 网络过滤规则（iptables/ipvs）
- 用户目录下的 kubeconfig 文件（`$HOME/.kube/config`）

---

## 参考

- [reset.go 源码](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/reset.go)
- [workflow runner](https://github.com/kubernetes/kubernetes/blob/master/cmd/kubeadm/app/cmd/phases/workflow/runner.go)
