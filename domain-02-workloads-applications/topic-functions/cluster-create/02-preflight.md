---
title: 预检流程 (kubeadm preflight) [cluster-create]
description: 'description: ''| `cmd/kubeadm/app/preflight/checks.go` | L501-L800 | 网络和端口检查 |'''
category: general
tags:
- reference
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 预检流程 (kubeadm preflight) 是什么
- 如何 预检流程 (kubeadm preflight)
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 预检流程
- kubeadm
- preflight
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

title: 预检流程 (kubeadm preflight)
description: '| `cmd/kubeadm/app/preflight/checks.go` | L501-L800 | 网络和端口检查 |'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- containerd
- docker
last_updated: '2026-05-18'
difficulty: beginner
reading_level: beginner
audience:
- DevOps工程师
- Kubernetes初学者
estimated_read_time: 5min
intent_queries:
- kubeadm preflight checks system verification
- kubeadm init preflight CRI container runtime check
- kubeadm preflight port check swap memory CPU
- kubeadm ignore-preflight-errors NumCPU Mem
- kubeadm preflight system requirements
trigger_keywords:
- preflight
- checks
- system verification
- CRI
- container runtime
- port check
- swap
- CPU
- memory
- ignore-preflight-errors
- firewalld
- kernel
related_domains:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
related_topics:
- kubeadm init
- kubeadm join
- system requirements
- CRI
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

# 预检流程 (kubeadm preflight)

## 函数/流程签名

```go
func RunInitMasterChecks(cfg *kubeadmapi.InitConfiguration, ignorePreflightErrors []string) error
func RunJoinNodeChecks(cfg *kubeadmapi.JoinConfiguration, ignorePreflightErrors []string) error
func checkPortOpen(port int) error
func IsContainerRuntimePresent() error
func IsKubernetesVersionSupported(version string) error
func checkSystemVerification() error
func RunChecks(checks []Checker, ignorePreflightErrors sets.Set[string]) error
type Checker interface { Name() string; Check() error }
```

## 源码位置

| 文件路径 | 行号范围 | 说明 |
|---------|---------|------|
| `cmd/kubeadm/app/preflight/checks.go` | L40-L200 | Checker 接口和核心检查 |
| `cmd/kubeadm/app/preflight/checks.go` | L201-L500 | 系统级检查函数 |
| `cmd/kubeadm/app/preflight/checks.go` | L501-L800 | 网络和端口检查 |
| `cmd/kubeadm/app/preflight/checks.go` | L801-L1100 | 证书和配置检查 |
| `cmd/kubeadm/app/cmd/init.go` | L200-L300 | init 预检入口 |
| `cmd/kubeadm/app/cmd/join.go` | L150-L250 | join 预检入口 |

## 参数说明

### 预检项目列表

| 检查名称 | 类型 | 说明 | 失败条件 |
|---------|------|------|---------|
| `NumCPU` | 系统 | CPU 核心数 | < 2 核 (init), < 1 核 (join) |
| `Mem` | 系统 | 内存大小 | < 1700MB |
| `Swap` | 系统 | swap 状态 | swap 开启 |
| `FileContent--proc-sys-net-ipv4-ip_forward` | 内核 | IP 转发 | ip_forward=0 |
| `FileContent--proc-sys-net-bridge-bridge-nf-call-iptables` | 内核 | bridge iptables | 未设置 |
| `Port-6443` | 网络 | API Server 端口 | 端口被占用 |
| `Port-2379` | 网络 | etcd 客户端端口 | 端口被占用 |
| `Port-2380` | 网络 | etcd 对等端口 | 端口被占用 |
| `Port-10250` | 网络 | kubelet 端口 | 端口被占用 |
| `Port-10259` | 网络 | scheduler 端口 | 端口被占用 |
| `Port-10257` | 网络 | controller-manager 端口 | 端口被占用 |
| `CRI` | 运行时 | CRI 运行时 | socket 不存在或不可连接 |
| `Service-Docker` | 服务 | Docker 服务 | Docker 运行但非推荐 |
| `IsPrivilegedUser` | 权限 | root 权限 | 非 root 用户 |
| `KubernetesVersion` | 版本 | K8s 版本兼容性 | kubeadm 和 kubelet 版本不匹配 |
| `Firewalld` | 网络 | 防火墙状态 | firewalld 运行中 |
| `DirAvailable--etc-kubernetes-manifests` | 文件 | manifest 目录 | 目录已存在且非空 |
| `DirAvailable--var-lib-etcd` | 文件 | etcd 数据目录 | 目录已存在且非空 |

## 调用链

```mermaid
flowchart TB
    subgraph Init["init 预检"]
        A[RunInitMasterChecks] --> B[系统检查]
        B --> C[NumCPU >= 2]
        B --> D[Mem >= 1700MB]
        B --> E[Swap disabled]
        B --> F[IsPrivilegedUser]
        A --> G[内核检查]
        G --> H[ip_forward enabled]
        G --> I[bridge-nf-call-iptables]
        G --> J[overcommit_memory]
        A --> K[网络检查]
        K --> L[Port 6443 free]
        K --> M[Port 2379 free]
        K --> N[Port 2380 free]
        K --> O[Port 10250 free]
        A --> P[运行时检查]
        P --> Q[CRI socket exists]
        P --> R[CRI Version API]
        A --> S[版本检查]
        S --> T[kubeadm == kubelet version]
    end

    subgraph Join["join 预检"]
        U[RunJoinNodeChecks] --> V[系统检查 (同 init)]
        U --> W[CRI 检查]
        U --> X[端口检查 (部分)]
        U --> Y[Discovery 检查]
        Y --> Z[API Server reachable]
        Y --> AA[Token valid]
        Y --> AB[CA hash matches]
    end
```

## 源码分析

### 预检执行引擎

```go
// cmd/kubeadm/app/preflight/checks.go
// RunChecks 执行一组预检
func RunChecks(checks []Checker, ignorePreflightErrors sets.Set[string]) error {
    foundErrors := []error{}

    for _, check := range checks {
        // 1. 检查是否在忽略列表中
        name := check.Name()
        if ignorePreflightErrors.Has(name) {
            fmt.Printf("[preflight] WARNING: Skipping check: %s\n", name)
            continue
        }

        // 2. 执行检查
        if err := check.Check(); err != nil {
            // 3. 区分 WARNING 和 ERROR
            if IsWarning(err) {
                fmt.Printf("[preflight] WARNING: %s: %v\n", name, err)
            } else {
                fmt.Printf("[preflight] ERROR: %s: %v\n", name, err)
                foundErrors = append(foundErrors, err)
            }
        } else {
            fmt.Printf("[preflight] PASS: %s\n", name)
        }
    }

    // 4. 如果有 ERROR，返回汇总错误
    if len(foundErrors) > 0 {
        return fmt.Errorf("preflight checks failed: %v", foundErrors)
    }
    return nil
}
```

### 端口检查

```go
// cmd/kubeadm/app/preflight/checks.go
// PortCheck 检查端口是否可用
type PortCheck struct {
    port  int
    proto string
}

func (c PortCheck) Name() string {
    return fmt.Sprintf("Port-%d", c.port)
}

func (c PortCheck) Check() error {
    // 1. 尝试监听端口
    addr := fmt.Sprintf(":%d", c.port)
    listener, err := net.Listen(c.proto, addr)
    if err != nil {
        // 端口被占用
        return fmt.Errorf("Port %d is in use", c.port)
    }
    listener.Close()
    return nil
}
```

### CRI 检查

```go
// cmd/kubeadm/app/preflight/checks.go
// IsContainerRuntimePresent 检查 CRI 运行时
func IsContainerRuntimePresent() error {
    // 1. 检测 CRI socket
    sockets := []string{
        "/var/run/containerd/containerd.sock",
        "/var/run/crio/crio.sock",
    }

    for _, socket := range sockets {
        if _, err := os.Stat(socket); err == nil {
            // 2. 尝试连接 CRI
            runtimeSvc, err := remote.NewRemoteRuntimeService(socket, 5*time.Second)
            if err != nil {
                continue
            }
            defer runtimeSvc.Close()

            // 3. 调用 Version API
            _, err = runtimeSvc.Version(context.TODO(), &runtimeapi.VersionRequest{})
            if err != nil {
                return fmt.Errorf("CRI runtime version check failed: %w", err)
            }
            return nil
        }
    }

    return errors.New("[ERROR CRI]: container runtime is not ready")
}
```

## 执行流程

```
步骤 1:  系统基础检查
    → CPU >= 2, Memory >= 1700MB
    → swap 已关闭
    → 以 root 用户运行
    ↓
步骤 2:  内核参数检查
    → net.ipv4.ip_forward = 1
    → net.bridge.bridge-nf-call-iptables = 1
    → vm.overcommit_memory = 1 (可选)
    ↓
步骤 3:  端口检查
    → 6443 (API Server) 未被占用
    → 2379, 2380 (etcd) 未被占用
    → 10250, 10257, 10259 (kubelet/CM/scheduler) 未被占用
    ↓
步骤 4:  CRI 运行时检查
    → containerd/crio socket 存在
    → gRPC Version API 响应正常
    → CRI API 版本兼容
    ↓
步骤 5:  版本兼容性检查
    → kubeadm 版本 == kubelet 版本
    → 不支持跨大版本
    ↓
步骤 6:  文件系统检查
    → /etc/kubernetes/manifests/ 不存在或为空
    → /var/lib/etcd/ 不存在或为空
    → /etc/kubernetes/pki/ 状态正确
```

## 使用场景

### 场景 1: 跳过特定预检

```bash
# 跳过 CPU 检查 (单核测试环境)
kubeadm init --ignore-preflight-errors=NumCPU

# 跳过多个检查
kubeadm init --ignore-preflight-errors=NumCPU,Mem,Swap

# 跳过所有警告 (不推荐)
kubeadm init --ignore-preflight-errors=all
```

### 场景 2: 预检前的系统准备

```bash
# 关闭 swap
swapoff -a
sed -i '/swap/d' /etc/fstab

# 加载内核模块
cat > /etc/modules-load.d/k8s.conf <<EOF
overlay
br_netfilter
EOF
modprobe overlay
modprobe br_netfilter

# 设置内核参数
cat > /etc/sysctl.d/k8s.conf <<EOF
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF
sysctl --system

# 安装 containerd
apt-get install -y containerd
mkdir -p /etc/containerd
containerd config default > /etc/containerd/config.toml
systemctl restart containerd

# 安装 kubeadm, kubelet, kubectl
apt-get install -y kubeadm=1.28.0-1.1 kubelet=1.28.0-1.1 kubectl=1.28.0-1.1
systemctl enable kubelet
```

### 场景 3: 检查预检结果

```bash
# 手动运行预检 (不执行 init)
kubeadm init --dry-run

# 查看 kubelet 版本
kubelet --version
# Kubernetes v1.28.0

# 检查端口占用
ss -tlnp | grep -E '6443|2379|2380|10250|10257|10259'

# 检查内核参数
sysctl net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables
```

## 配置示例

### 预检忽略配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
nodeRegistration:
  ignorePreflightErrors:
  - NumCPU      # 跳过 CPU 核心数检查
  - Mem         # 跳过内存检查
  criSocket: unix:///var/run/containerd/containerd.sock
```

## 实战示例

### 常见预检错误及修复

```bash
# [ERROR Swap]: running with swap on is not supported
swapoff -a
sed -i '/swap/d' /etc/fstab

# [ERROR Port-6443]: Port 6443 is in use
lsof -i :6443
# 杀掉占用进程或更换端口

# [ERROR CRI]: container runtime is not ready
systemctl restart containerd
crictl info  # 验证 CRI

# [ERROR FileContent--proc-sys-net-ipv4-ip_forward]
sysctl -w net.ipv4.ip_forward=1

# [ERROR FileContent--proc-sys-net-bridge-bridge-nf-call-iptables]
modprobe br_netfilter
sysctl -w net.bridge.bridge-nf-call-iptables=1

# [ERROR NumCPU]: the number of available CPUs 1 is less than the required 2
kubeadm init --ignore-preflight-errors=NumCPU
```

### 完整系统准备脚本

```bash
#!/bin/bash
# 完整的 Kubernetes 节点准备脚本 (Ubuntu/Debian)
# 解决所有 kubeadm preflight 检查

set -euo pipefail

echo "=== 1. 关闭 swap ==="
swapoff -a
sed -i '/swap/d' /etc/fstab

echo "=== 2. 加载内核模块 ==="
cat > /etc/modules-load.d/k8s.conf <<EOF
overlay
br_netfilter
EOF
modprobe overlay
modprobe br_netfilter

echo "=== 3. 设置内核参数 ==="
cat > /etc/sysctl.d/k8s.conf <<EOF
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
vm.overcommit_memory                = 1
vm.panic_on_oom                     = 0
fs.inotify.max_user_watches         = 1048576
fs.file-max                         = 15728640
fs.nr_open                          = 1048576
net.netfilter.nf_conntrack_max      = 1048576
EOF
sysctl --system

echo "=== 4. 安装 containerd ==="
apt-get update
apt-get install -y containerd
mkdir -p /etc/containerd
containerd config default > /etc/containerd/config.toml
# 设置 systemd cgroup driver
sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
systemctl restart containerd
systemctl enable containerd

echo "=== 5. 安装 kubeadm, kubelet, kubectl ==="
apt-get install -y apt-transport-https ca-certificates curl gpg
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.28/deb/Release.key | \
  gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.28/deb/ /' | \
  tee /etc/apt/sources.list.d/kubernetes.list
apt-get update
apt-get install -y kubelet kubeadm kubectl
apt-mark hold kubelet kubeadm kubectl
systemctl enable kubelet

echo "=== 6. 验证 ==="
echo "containerd: $(containerd --version)"
echo "kubeadm: $(kubeadm version --output=short)"
echo "kubelet: $(kubelet --version)"
echo "sysctl ip_forward: $(sysctl -n net.ipv4.ip_forward)"
echo "swap: $(free -h | grep Swap | awk '{print $2}')"
echo "=== 节点准备完成 ==="
```

### 预检输出详解

```bash
# 正常的 preflight 输出
kubeadm init --dry-run
# [preflight] Running pre-flight checks
# [preflight] The number of available CPUs 4 is above the required 2
# [preflight] The available memory 8192 MB is above the required 1700 MB
# [preflight] Checking swap status: swap is disabled
# [preflict] Checking kernel parameters: net.ipv4.ip_forward=1
# [preflight] Checking kernel parameters: net.bridge.bridge-nf-call-iptables=1
# [preflight] Checking port 6443: available
# [preflight] Checking port 2379: available
# [preflight] Checking port 2380: available
# [preflight] Checking port 10250: available
# [preflight] Checking port 10257: available
# [preflight] Checking port 10259: available
# [preflight] Checking CRI runtime: containerd is available
# [preflight] Checking kubeadm version: v1.28.0
# [preflight] Checking kubelet version: v1.28.0
# [preflight] All pre-flight checks passed
```

### 预检端口说明

```yaml
# 各组件使用的端口
ports:
  kube_apiserver:
    port: 6443
    protocol: TCP
    description: "API Server HTTPS 端口"
  etcd_client:
    port: 2379
    protocol: TCP
    description: "etcd 客户端通信"
  etcd_peer:
    port: 2380
    protocol: TCP
    description: "etcd 对等通信"
  kubelet:
    port: 10250
    protocol: TCP
    description: "kubelet API (主端口)"
  kube_controller_manager:
    port: 10257
    protocol: TCP
    description: "Controller Manager 安全端口"
  kube_scheduler:
    port: 10259
    protocol: TCP
    description: "Scheduler 安全端口"
  kube_proxy:
    port: 10256
    protocol: TCP
    description: "kube-proxy 健康检查"
  nodeport_range:
    port: 30000-32767
    protocol: TCP/UDP
    description: "NodePort Service 端口范围"
```

## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `[ERROR Swap]` | swap 未关闭 | `swapoff -a && sed -i '/swap/d' /etc/fstab` |
| `[ERROR Port-6443]` | 端口被占用 | `lsof -i :6443` 找到并停止占用进程 |
| `[ERROR CRI]` | CRI 运行时不可用 | 安装并启动 containerd |
| `[ERROR NumCPU]` | CPU 核心不足 | `--ignore-preflight-errors=NumCPU` |
| `[ERROR Firewalld]` | 防火墙运行中 | `systemctl stop firewalld && systemctl disable firewalld` |
| `[ERROR DirAvailable]` | 目录已存在 | `kubeadm reset` 或手动清理 |
| `[ERROR KubeletVersion]` | 版本不匹配 | 统一 kubeadm 和 kubelet 版本 |
| `[WARNING IsDocker]` | 使用 Docker | 改用 containerd |

## 相关函数

- [集群概览](01-overview.md) — init 流程的第一个 phase
- [CRI 运行时](18-cri-runtime.md) — CRI 预检详细分析
- [节点加入](06-join.md) — join 预检
- [证书管理](03-certs.md) — 证书相关预检
- [集群升级](09-upgrade.md) — 升级预检

### 内核参数检查详解

```go
// cmd/kubeadm/app/preflight/checks.go
// FileContentCheck 检查内核参数文件内容
type FileContentCheck struct {
    path     string
    expected []byte
}

func (c FileContentCheck) Name() string {
    return fmt.Sprintf("FileContent-%s", strings.ReplaceAll(c.path, "/", "-"))
}

func (c FileContentCheck) Check() error {
    // 读取 /proc/sys/net/ipv4/ip_forward 等文件
    content, err := os.ReadFile(c.path)
    if err != nil {
        return fmt.Errorf("cannot read %s: %w", c.path, err)
    }

    // 检查值是否为 "1"
    trimmed := strings.TrimSpace(string(content))
    if trimmed != "1" {
        return fmt.Errorf("%s is not set to 1 (current: %s)", c.path, trimmed)
    }
    return nil
}
```

### 版本兼容性检查

```go
// cmd/kubeadm/app/preflight/checks.go
// KubernetesVersionCheck 检查 kubeadm 和 kubelet 版本一致性
type KubernetesVersionCheck struct {
    kubeadmVersion string
    kubeletVersion string
}

func (c KubernetesVersionCheck) Check() error {
    // kubeadm 和 kubelet 版本必须一致
    kubeadmVer, _ := version.ParseSemantic(c.kubeadmVersion)
    kubeletVer, _ := version.ParseSemantic(c.kubeletVersion)

    if kubeadmVer.Major() != kubeletVer.Major() ||
       kubeadmVer.Minor() != kubeletVer.Minor() {
        return fmt.Errorf("kubeadm version %s does not match kubelet version %s",
            c.kubeadmVersion, c.kubeletVersion)
    }
    return nil
}
```

### 目录检查

```go
// cmd/kubeadm/app/preflight/checks.go
// DirAvailableCheck 检查目录是否可用 (空或不存在)
type DirAvailableCheck struct {
    path string
}

func (c DirAvailableCheck) Check() error {
    // 检查 /etc/kubernetes/manifests 目录
    // 如果目录存在且非空 → 之前 init 未清理
    entries, err := os.ReadDir(c.path)
    if err != nil {
        if os.IsNotExist(err) {
            return nil // 目录不存在 → OK
        }
        return err
    }

    if len(entries) > 0 {
        return fmt.Errorf("%s is not empty (run kubeadm reset to clean up)", c.path)
    }
    return nil
}
```

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/containerd.md|containerd]]
