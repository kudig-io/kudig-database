# 预检阶段 (Preflight Checks)

## 源码路径

`cmd/kubeadm/app/cmd/options/cmd.go` / `cmd/kubeadm/app/preflight`

---

## 检查项分类

### 系统级检查

| 检查项 | 源码文件 | 说明 |
|--------|---------|------|
| 操作系统 | `linux_os.go` | 仅支持 Linux |
| kernel 版本 | `kernel.go` | >= 3.10 |
| Cgroups | `cgroups.go` | 检查 cgroup v1/v2 支持 |
| 端口可用性 | `ports.go` | 检查必需端口是否被占用 |
| 内存/CPU | `resource.go` | master 建议 2C2G+ |

### Kubernetes 组件检查

| 检查项 | 说明 |
|--------|------|
| kubelet 版本 | 与 API Server 版本兼容性 |
| kubeadm 版本 | 确保版本一致性 |
| container runtime | Docker/containerd/cri-o |
| conntrack | iptables/nftables 依赖 |
| ipvs | kube-proxy ipvs 模式依赖 |

### 证书检查

```go
// cmd/kubeadm/app/preflight/certs.go
func CheckCerts() error {
    // 检查 /etc/kubernetes/pki 是否存在
    // 如果已存在，检查证书是否即将过期
}
```

---

## 关键代码: preflight.go

```go
func RunPreflight(initCfg *kubeadmapi.InitConfiguration) error {
    // 1. 系统检查
    checks := []Checker{
        NewLinuxOSChecker(),
        NewKernelChecker(),
        NewPortChecker(),
        NewMemoryChecker(),
        NewSwapChecker(),
    }
    // 2. 执行检查
    for _, check := range checks {
        if err := check.Check(); err != nil {
            return err
        }
    }
    return nil
}
```

---

## 跳过检查

```bash
# 跳过所有预检
kubeadm init --skip-preflight-checks

# 仅跳过特定检查
kubeadm init --ignore-preflight-errors=Port-6443

# 常见忽略场景:
# - 内存不足测试 (测试环境)
# - 特定端口被占用
```

---

## --image-repository 镜像配置

离线环境或私有仓库场景:

```bash
# 使用私有镜像仓库
kubeadm init --image-repository registry.example.com/k8s

# 默认镜像 (k8s.gcr.io)
# 中国大陆建议使用代理/镜像:
# --image-repository=registry.cn-hangzhou.aliyuncs.com/google_containers
```

```bash
# 查看需要拉取的镜像
kubeadm config images list --kubernetes-version v1.28.0

# 输出:
# k8s.gcr.io/kube-apiserver:v1.28.0
# k8s.gcr.io/kube-controller-manager:v1.28.0
# k8s.gcr.io/kube-scheduler:v1.28.0
# k8s.gcr.io/kube-proxy:v1.28.0
# k8s.gcr.io/pause:3.9
# k8s.gcr.io/etcd:3.5.x
# k8s.gcr.io/coredns:1.10.x
```

---

## kubeadm init --dry-run

```bash
# 演练模式 (不实际执行，只输出结果)
kubeadm init --dry-run

# 配合配置文件演练
kubeadm init --config=kubeadm-config.yaml --dry-run

# 输出:
# [2024-01-01 00:00:00] Validated configuration.
# [2024-01-01 00:00:00] Confirmed all phases would be executed.
```

---

## kubeadm reset 集群清理

重置节点 (用于清理、重装):

```bash
# 在所有节点执行 (从节点开始)
kubeadm reset

# 清理内容包括:
# - 删除 /etc/kubernetes 目录 (manifests, pki, kubeconfig)
# - 删除 /var/lib/kubelet 目录
# - 删除 /var/lib/etcd 目录
# - 删除 $HOME/.kube/config
# - 停止 kubelet 服务
# - 清理 iptables 规则 (--cleanup-iptables)
```

```bash
# 带完整清理
kubeadm reset --cleanup-iptables

# 只清理 iptables，不删除文件
kubeadm reset --cleanup-iptables=false

# 使用 kubeconfig 文件
kubeadm reset --kubeconfig=/path/to/kubeconfig
```

---

## kubeadm reset 完整流程

```go
// cmd/kubeadm/app/cmd/reset.go
func RunReset(cmd *cobra.Command, args []string) error {
    // 1. 读取 kubeconfig
    // 2. 从集群移除本节点 (如果节点还在集群中)
    // 3. 停止 kubelet
    // 4. 清理 /etc/kubernetes
    // 5. 清理 /var/lib/kubelet
    // 6. 清理 /var/lib/etcd (如果 --remove-etcd-member)
    // 7. 清理 iptables 规则
    // 8. 清理 CNI 配置文件
}
```

---

## 从集群移除节点

```bash
# 1. 先驱逐节点上的 Pod (在 control-plane 执行)
kubectl drain <node-name> --delete-emptydir-data --ignore-daemonsets

# 2. 删除节点
kubectl delete node <node-name>

# 3. 在被移除节点上执行 reset
kubeadm reset --cleanup-iptables

# 4. 如果是 control-plane 节点，还需要:
#    - 重置 etcd 成员: etcdctl member remove <member-id>
#    - 删除 etcd 数据目录
```

---

## 常见失败场景

| 错误 | 原因 | 解决 |
|------|------|------|
| [ERROR Port-6443]: Port 6443 is in use | API Server 端口被占用 | 释放端口或更换监听端口 |
| [ERROR Swap]: Running with swap on | 未关闭 swap | `swapoff -a` 并注释 fstab |
| [ERROR FileContent-proc-sys-net-ipv4-ip-forward]: /proc/sys/net/ipv4/ip_forward | ip_forward 未开启 | `sysctl -w net.ipv4.ip_forward=1` |
| [ERROR CRI]: container runtime not ready | containerd/Docker 未启动 | `systemctl start containerd` |
| [ERROR FileContent-proc-sys-net-bridge-bridge-nf-call-iptables] | bridge-nf-call-iptables 未设置 | `sysctl -w net.bridge.bridge-nf-call-iptables=1` |
