---
title: Kubernetes 集群删除操作指南
description: '# Kubernetes 集群删除操作指南'
summary: 'func newCmdReset(in io.Reader, out io.Writer, resetOptions *resetOptions) *cobra.Command'
category: references
tags:
- k8s
- operations
- cluster-delete
- etcd
- kubelet
- scheduler
- cilium
- flannel
- calico
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 集群删除操作指南 是什么
- 如何 Kubernetes 集群删除操作指南
trigger_keywords:
- Kubernetes
- 集群删除操作指南
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 集群删除操作指南

### 01 Overview

#### 函数签名

```go
func newCmdReset(in io.Reader, out io.Writer, resetOptions *resetOptions) *cobra.Command

func (r *resetData) ForceReset() bool
func (r *resetData) DryRun() bool
func (r *resetData) Client() clientset.Interface
func (r *resetData) CertificatesDir() string
func (r *resetData) CRISocketPath() string
func (r *resetData) CleanupTmpDir() bool
func (r *resetData) Cfg() *kubeadmapi.InitConfiguration
func (r *resetData) ResetCfg() *kubeadmapi.ResetConfiguration
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| reset 入口 | `cmd/kubeadm/app/cmd/reset.go` | 命令注册、resetData 构建 |
| reset phases | `cmd/kubeadm/app/cmd/phases/reset/` | preflight/remove-etcd/cleanup-node |
| etcd 操作 | `cmd/kubeadm/app/phases/etcd/local.go` | RemoveStackedEtcdMember |
| workflow 引擎 | `cmd/kubeadm/app/cmd/phases/workflow/runner.go` | Phase 执行框架 |
| 配置加载 | `cmd/kubeadm/app/util/config/initconfiguration.go` | 配置解析与默认值 |
| 清理工具 | `cmd/kubeadm/app/util/users.go` | 用户和目录清理 |

#### resetOptions 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `kubeconfigPath` | `string` | admin kubeconfig 路径，默认 `/etc/kubernetes/admin.conf` |
| `cfgPath` | `string` | `--config` 指定的配置文件路径 |
| `ignorePreflightErrors` | `[]string` | 忽略的预检错误列表 |
| `externalcfg` | `*kubeadmapiv1.ResetConfiguration` | 命令行标志构建的默认配置 |
| `skipCRIDetect` | `bool` | 是否跳过容器运行时检测 |

---

### 02 Reset

#### 函数签名

```go
func newCmdReset(in io.Reader, out io.Writer, resetOptions *resetOptions) *cobra.Command

func newResetData(cmd *cobra.Command, opts *resetOptions, in io.Reader, out io.Writer, allowExperimental bool) (*resetData, error)

func AddResetFlags(flagSet *flag.FlagSet, resetOptions *resetOptions)

func runPreflight(c workflow.RunData) error
func runRemoveETCDMember(c workflow.RunData) error
func runCleanupNode(c workflow.RunData) error

func LoadOrDefaultResetConfiguration(configPath string, defaultCfg *kubeadmapiv1.ResetConfiguration, opts LoadOrDefaultConfigurationOptions) (*kubeadmapi.ResetConfiguration, error)

```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 命令入口 | `cmd/kubeadm/app/cmd/reset.go` | 命令注册、resetData 构建 |
| 预检阶段 | `cmd/kubeadm/app/cmd/phases/reset/preflight.go` | root 权限检查、用户确认 |
| etcd 移除 | `cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go` | RemoveStackedEtcdMember |
| 节点清理 | `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go` | 停止服务、清理目录 |
| 卸载逻辑 | `cmd/kubeadm/app/cmd/phases/reset/unmount.go` | 挂载点卸载 |
| Linux 卸载 | `cmd/kubeadm/app/cmd/phases/reset/unmount_linux.go` | unmount2() 系统调用 |
| 配置加载 | `cmd/kubeadm/app/util/config/initconfiguration.go` | ResetConfiguration 加载 |

#### resetOptions 字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `kubeconfigPath` | `string` | kubeconfig 路径，默认 `/etc/kubernetes/admin.conf` |
| `cfgPath` | `string` | `--config` 指定的配置文件路径 |
| `ignorePreflightErrors` | `[]string` | 忽略的预检错误 |
| `externalcfg` | `*kubeadmapiv1.ResetConfiguration` | 命令行标志构建的默认配置 |
| `skipCRIDetect` | `bool` | 是否跳过容器运行时检测 |

---

### 03 Delete Node

#### 概述

Kubernetes 节点删除分为两个层面：**API 层删除**（`kubectl delete node`，从 etcd 移除 Node 对象）和**节点级重置**（`kubeadm reset`，清理本地数据）。两者通常配合使用。本文档从源码层面分析完整的节点删除流程，涵盖 drain 驱逐、Node 对象删除、Node Lifecycle Controller 响应、kubeadm reset 重置等关键环节。

---

#### 函数签名

```go
func (nc *Controller) reconcileNodeDelete(node *v1.Node) error

func (nc *Controller) markPodsNotReady(node *v1.Node) error

func (nc *noExecuteTaintManager) taintEviction(node *v1.Node) error

func draincmd.RunDrain(ctx context.Context, drainer *Drainer, nodes []string) error

func (d *Drainer) deleteOrEvictPodsSimple(ctx context.Context, pods []*corev1.Pod) error

func RunCleanup(cmd *cobra.Command, args []string) error

func removeETCDMember(cfg *kubeadmapi.InitConfiguration) error

func cleanupNode(dirsToClean []string) error

```

---

#### 源码位置

| 功能 | 文件路径 |
|------|---------|
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/node_lifecycle_controller.go` |
| Pod 驱逐逻辑 | `pkg/controller/nodelifecycle/taint_controller.go` |
| kubectl drain | `staging/src/k8s.io/kubectl/pkg/cmd/drain/drain.go` |
| Pod 优雅终止 | `pkg/kubelet/kubelet_pods.go` |
| kubeadm reset | `cmd/kubeadm/app/cmd/reset.go` |
| etcd 成员移除 | `cmd/kubeadm/app/phases/removeetcdmember/` |
| 节点清理 | `cmd/kubeadm/app/phases/reset/cleanup.go` |

---

---

### 04 Cleanup

#### 概述

`cleanup-node` 是 `kubeadm reset` 的核心阶段，负责停止服务、删除容器、卸载挂载点、清理配置目录和证书文件。本文档基于源码深入分析每一步清理操作。

---

#### 源码路径

- 清理入口: `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go`
- 卸载逻辑: `cmd/kubeadm/app/cmd/phases/reset/unmount.go` / `unmount_linux.go`
- 容器运行时: `cmd/kubeadm/app/util/runtime/`

---

#### 清理流程总览

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

---

### 05 Etcd Cleanup

#### 概述

控制面节点的删除需要额外处理 etcd 集群：从集群中移除成员、清理本地数据。如果处理不当，会导致 etcd 仲裁丢失或数据不一致。本文档基于源码分析 `remove-etcd-member` 阶段的完整逻辑。

---

#### 源码路径

- 成员移除阶段: `cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go`
- etcd 操作实现: `cmd/kubeadm/app/phases/etcd/local.go`
- etcd 工具库: `cmd/kubeadm/app/util/etcd/`

---

#### 流程总览

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

---

### 06 Force Delete

#### 概述

生产环境中集群删除常遇到各种异常场景：节点不可达、etcd 仲裁丢失、kubelet 无法停止、容器运行时异常等。本文档分析 `kubeadm reset` 的容错机制以及手动处理方案。

---

#### 1.1 源码分析

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

#### 1.2 使用场景

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 自动化脚本中避免交互
kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置

# 等价于
echo "y" | kubeadm reset  # ⚠️ 清理节点所有 K8s 配置
```

---

---

### 07 Ha Delete

#### 概述

高可用（HA）Kubernetes 集群的删除比单节点集群复杂得多：需要维护 etcd 仲裁、处理负载均衡器、确保控制面组件正常退出。本文档分析 HA 集群删除的关键注意事项。

---

#### HA 架构回顾

```
┌──────────────────────────────────────────────────────────────────┐
│  HA 集群架构（Stacked etcd）                                      │
├──────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  │
│  │  CP Node 1       │  │  CP Node 2       │  │  CP Node 3       │  │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────┐  │  │
│  │  │ API Server │  │  │  │ API Server │  │  │  │ API Server │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │  Scheduler │  │  │  │  Scheduler │  │  │  │  Scheduler │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │    CCM     │  │  │  │    CCM     │  │  │  │    CCM     │  │  │
│  │  ├───────────┤  │  │  ├───────────┤  │  │  ├───────────┤  │  │
│  │  │   etcd-1   │  │  │  │   etcd-2   │  │  │  │   etcd-3   │  │  │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────┘  │  │
│  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘  │
│           │                    │                    │              │
│           └────────────────────┼────────────────────┘              │
│                                │                                    │
│                     ┌──────────▼──────────┐                        │
│                     │   Load Balancer      │                        │
│                     │   (kube-vip
...(截断)

#### 1.1 删除顺序要求

```
┌──────────────────────────────────────────────────────────────┐
│  3 节点 HA 集群的安全删除顺序                                  │
├──────────────────────────────────────────────────────────────┤
│                                                                │
│  Step 1: 删除第 1 个控制面节点                                  │
│    成员数: 3 → 2  (仲裁: 2, 仍可用 ✅)                        │
│    ├─ drain + delete node                                      │
│    ├─ kubeadm reset (自动移除 etcd 成员)                       │
│    └─ 确认 etcd 健康后继续                                     │
│                                                                │
│  Step 2: 删除第 2 个控制面节点                                  │
│    成员数: 2 → 1  (仲裁: 1, 勉强可用 ⚠️)                      │
│    ├─ drain + delete node                                      │
│    ├─ kubeadm reset                                            │
│    └─ 此时集群仍有 1 个 etcd 成员                              │
│                                                                │
│  Step 3: 删除第 3 个控制面节点                                  │
│    成员数: 1 → 0  (集群销毁)                                   │
│    └─ kubeadm reset -f                                         │
│                                                                │
└──────────────────────────────────────────────────────────────┘

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
---

### 08 Cloud Delete

#### 概述

`cluster-create/10-cloud-comparison.md` 分析了各云厂商的集群创建方案。本文档补充其**删除/销毁**侧的对比分析，涵盖 EKS、AKS、GKE、ACK、TKE 以及 kubeadm 自建集群的删除差异。

---

#### 删除方式对比

| 方案 | 删除命令 | 控制面清理 | etcd 处理 | Worker 清理 |
|------|---------|-----------|----------|-------------|
| kubeadm | `kubeadm reset` + 手动清理 | 需手动逐节点 | 需手动移除成员 | 需手动 |  # ⚠️ 清理节点所有 K8s 配置
| EKS | `eksctl delete cluster` | AWS 自动 | 托管，无需处理 | ASG 自动回收 |
| AKS | `az aks delete` | Azure 自动 | 托管，无需处理 | VMSS 自动回收 |
| GKE | `gcloud container clusters delete` | Google 自动 | 托管，无需处理 | MIG 自动回收 |
| ACK | `aliyun cs DELETE /clusters/<id>` | 阿里云自动 | 托管，无需处理 | ECS 自动释放 |
| TKE | `tencentcloud cli delete-cluster` | 腾讯云自动 | 托管，无需处理 | CVM 自动回收 |

---

#### kubeadm 删除流程（对比基准）

```
┌──────────────────────────────────────────────────────────────────┐
│  kubeadm 集群删除（全手动）                                       │
├──────────────────────────────────────────────────────────────────┤
│  1. kubectl drain <node>         ← 手动驱逐                      │
│  2. kubectl delete node <node>   ← 手动删除 Node 对象             │
│  3. kubeadm reset -f             ← 手动在每台节点执行             │
│  4. etcdctl member remove        ← 手动移除 etcd 成员             │
│  5. iptables/ipvs 清理           ← 手动清理网络规则               │
│  6. CNI/数据目录清理             ← 手动清理                       │
│  7. LB/DNS 清理                  ← 手动清理                       │
│                                                                   │
│  ⚠️ 每一步都需要人工介入，容易遗漏                               │
└──────────────────────────────────────────────────────────────────┘
```

---

### 09 Reset Phase Commands

#### 函数签名

```go
func NewPreflightPhase() workflow.Phase
func NewRemoveETCDMemberPhase() workflow.Phase
func NewCleanupNodePhase() workflow.Phase

func runPreflight(c workflow.RunData) error
func runRemoveETCDMember(c workflow.RunData) error
func runCleanupNode(c workflow.RunData) error
```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Phase 注册 | `cmd/kubeadm/app/cmd/phases/reset/reset.go` | Phase 列表定义 |
| preflight | `cmd/kubeadm/app/cmd/phases/reset/preflight.go` | 预检阶段实现 |
| remove-etcd-member | `cmd/kubeadm/app/cmd/phases/reset/removeetcdmember.go` | etcd 成员移除 |
| cleanup-node | `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go` | 节点清理 |
| unmount | `cmd/kubeadm/app/cmd/phases/reset/unmount.go` | 挂载点卸载 |
| workflow runner | `cmd/kubeadm/app/cmd/phases/workflow/runner.go` | Phase 执行框架 |

#### preflight Phase 继承的标志

| 标志 | 来源常量 | 说明 |
|------|---------|------|
| `--dry-run` | `options.DryRun` | 干跑模式，不执行实际操作 |
| `-f, --force` | `options.Force` | 跳过用户确认提示 |
| `--ignore-preflight-errors` | `options.IgnorePreflightErrors` | 忽略指定预检错误 |

---

### 10 Security Delete

#### 函数签名

```go
func runCleanupNode(c workflow.RunData) error
func CleanDir(targetPath string) error
func RemoveStackedEtcdMember(client clientset.Interface, cfg *kubeadmapi.InitConfiguration, timeout time.Duration) error
func CleanupTmpDir(tmpDir string) error

// 安全擦除（外部工具）
// shred -vfz -n 3 <file>
// dd if=/dev/urandom of=/dev/sdX bs=1M

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```

#### 源码位置

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| 节点清理 | `cmd/kubeadm/app/cmd/phases/reset/cleanupnode.go` | 清理证书/容器/目录 |
| etcd 移除 | `cmd/kubeadm/app/phases/etcd/local.go` | RemoveStackedEtcdMember |
| 垃圾回收 | `pkg/controller/garbagecollector/` | 级联删除 |
| Secret 控制器 | `pkg/controller/secret/` | SA Token 管理 |
| RBAC 注册 | `cmd/kubeadm/app/phases/markcontrolplane/` | kubeadm RBAC 资源 |

#### 自动清理的证书文件

| 路径 | 清理方式 | 说明 |
|------|---------|------|
| `/etc/kubernetes/pki/*.crt` | CleanDir 内容 | 所有证书 |
| `/etc/kubernetes/pki/*.key` | CleanDir 内容 | 所有私钥 |
| `/etc/kubernetes/pki/etcd/` | CleanDir 内容 | etcd 证书子目录 |
| `/var/lib/kubelet/pki/` | CleanDir | kubelet 证书 |
| `/var/lib/etcd/member/` | CleanDir | etcd 数据（含 WAL） |

---

### 11 Network Cleanup

#### 概述

`kubeadm reset` **不会**自动清理网络配置。这是设计决策——不同 CNI 插件有不同的清理需求，且 iptables/ipvs 规则可能包含非 Kubernetes 规则，盲目清理会影响主机网络。本文档详细分析各类网络配置的残留位置和清理方法。  # ⚠️ 清理节点所有 K8s 配置

---

#### 源码中的设计决策

```go
var manualCleanupInstructions = dedent.Dedent(`
    The reset process does not perform cleanup of CNI plugin configuration,
    network filtering rules and kubeconfig files.
`)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```

**原因**:
1. CNI 插件种类繁多（Calico/Cilium/Flannel/Weave/Terway...），清理逻辑各不相同
2. iptables 规则可能包含非 Kubernetes 规则，盲目 `iptables -F` 会破坏主机网络
3. 路由和虚拟接口可能与宿主网络共享命名空间

---

#### 1.1 CNI 配置目录

```bash
ls /etc/cni/net.d/

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```

不同 CNI 插件的配置文件：

| CNI 插件 | 配置文件 | 说明 |
|----------|---------|------|
| Flannel | `10-flannel.conflist` | Flannel CNI 配置 |
| Calico | `10-calico.conflist` / `calico-kubeconfig` | Calico CNI + kubeconfig |
| Cilium | `05-cilium.conflist` | Cilium CNI 配置 |
| Weave | `10-weave.conflist` | Weave CNI 配置 |
| Terway (ACK) | `10-terway.conflist` | 阿里云 Terway |
| Amazon VPC | `10-aws.conflist` | AWS VPC CNI |

---

### 12 Troubleshooting

#### 概述

集群删除过程中常遇到各种异常：reset 卡住、etcd 移除失败、容器无法删除、网络规则残留等。本文档汇总常见问题场景，提供系统化的排查方法。

---

#### 1.1 reset 卡住不动

**症状**: `kubeadm reset`  # ⚠️ 清理节点所有 K8s 配置

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[26-技能/03-节点/node/诊断排障/troubleshoot-node-issues.md|节点故障排查]]
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|K8s 知识图谱]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[kube-vip]] — kube-vip
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

```

<!-- risk-assessed -->
