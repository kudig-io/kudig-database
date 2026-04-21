# Kubernetes 集群新建逻辑 — 基于官方代码分析

## 概述

Kubernetes 官方推荐使用 `kubeadm` 作为集群新建的标准工具。本文档基于 `kubernetes/kubernetes` 和 `kubernetes/kubernetes/kubeadm` 源码，分析集群新建的核心逻辑与阶段划分。

---

## 源码路径

- 主源码: `kubernetes/cmd/kubeadm`
- 核心包: `kubernetes/cmd/kubeadm/app/cmd/phases`

---

## 创建流程总览

```
┌─────────────────────────────────────────────────────────────┐
│                     kubeadm init                             │
├─────────────────────────────────────────────────────────────┤
│  1. preflight          预检检查                              │
│  2. certs              生成 PKI 证书与密钥                   │
│  3. kubeconfig         生成各组件 kubeconfig                 │
│  4. kubeconfig         生成 admin kubeconfig                 │
│  5. etcd               本地 etcd 集群初始化                   │
│  6) kubelet-start      kubelet 配置文件写入                   │
│  7) control-plane      生成静态 Pod manifests                │
│  8) etcd               生成 etcd Pod manifest                │
│  9) wait-control-plane 等待控制面组件就绪                    │
│  10. upload-config     上传 kubeadm ConfigMap                 │
│  11. bootstrap-token  创建 Bootstrap Token                  │
│  12. mark-control-plane 设置 control-plane 节点标签         │
└─────────────────────────────────────────────────────────────┘
```

---

## 核心代码分析

### 1. 入口: cmd/kubeadm/app/cmd/init.go

```go
// Run executes init command
func Run(cmd *cobra.Command, args []string) error {
    // 1. 加载配置
    initCfg, err := configutil.LoadInitConfiguration(cfgPath)
    // 2. 创建高可用控制面 (如果指定)
    if initCfg.ControlPlane != nil {
        return nil // 详见 ha.go
    }
    // 3. 执行各个阶段
    return initRunner.Run(initCfg)
}
```

**关键**: `initRunner` 是一个 PhaseList，按顺序执行各个阶段。

---

### 2. 证书阶段: certs.go

**源码路径**: `cmd/kubeadm/app/phases/certs/certs.go`

```go
func RunCerts(initConfig *kubeadmapi.InitConfiguration) error {
    // 生成以下证书:
    // - CA (kubernetes-ca)
    // - API Server cert
    // - API Server Kubelet Serving cert
    // - Front Proxy CA + Client cert
    // - etcd CA + Server/Client cert
    // - Service AccountSigningKey (用于 ServiceAccount token 签发)
}
```

**证书存储路径**: `/etc/kubernetes/pki/`

---

### 3. kubeconfig 阶段: kubeconfig.go

**源码路径**: `cmd/kubeadm/app/phases/kubeconfig/kubeconfig.go`

```go
// 生成以下 kubeconfig:
// - kubelet.conf      (Node 节点连接 API Server)
// - admin.conf        (管理员连接 API Server)
// - controller-manager.conf
// - scheduler.conf
```

---

### 4. etcd 阶段: etcdadm.go

**源码路径**: `cmd/kubeadm/app/phases/etcd/local.go`

```go
func RunEtcdLocal() error {
    // 1. 生成 etcd CA (如果不存在)
    // 2. 生成 etcd Server/Peer/Client 证书
    // 3. 创建 /etc/kubernetes/manifests/etcd.yaml
}
```

---

### 5. ControlPlane 阶段: controlplane.go

**源码路径**: `cmd/kubeadm/app/phases/controlplane/manifests.go`

```go
// 生成以下静态 Pod manifests:
// - /etc/kubernetes/manifests/kube-apiserver.yaml
// - /etc/kubernetes/manifests/kube-controller-manager.yaml
// - /etc/kubernetes/manifests/kube-scheduler.yaml
```

**注意**: 这些是静态 Pod，由 kubelet 直接管理，不经过 Kubernetes 调度。

---

### 6. kubelet-start 阶段

**源码路径**: `cmd/kubeadm/app/phases/kubelet`

写入以下配置:
- `/var/lib/kubelet/config.yaml` — kubelet 配置文件
- `/etc/kubernetes/bootstrap-kubelet.conf` — bootstrap 配置文件

---

## 节点加入流程: kubeadm join

```
┌────────────────────────────────────────┐
│          kubeadm join                  │
├────────────────────────────────────────┤
│  1. preflight       预检               │
│  2. tls-bootstrap  验证服务器身份      │
│  3. kubelet-start   写入 kubelet 配置  │
│  4. control-plane   (可选)标记为 master│
└────────────────────────────────────────┘
```

---

## 关键数据结构

### InitConfiguration

```go
type InitConfiguration struct {
    APIEndpoint       APIEndpoint
    NodeRegistration  NodeRegistrationOptions
    CertificatesDir   string
    DryRun            bool
    FeatureGates      map[string]bool
    KubernetesVersion string
    ControlPlan       *ControlPlane
}
```

---

## 常见问题

### 1. 证书过期

默认证书有效期 1 年，可通过 `kubeadm alpha certs renew` 更新。

### 2. etcd 数据目录

默认 `/var/lib/etcd`，建议使用独立磁盘防止数据丢失。

### 3. 静态 Pod 失败排查

```bash
# 查看 kubelet 日志
journalctl -u kubelet -f
# 查看静态 Pod 状态
crictl pods
crictl logs <pod-id>
```

---

## 参考

- [kubernetes/kubeadm 源码](https://github.com/kubernetes/kubernetes/tree/master/cmd/kubeadm)
- [官方文档: Creating a cluster with kubeadm](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/create-cluster-kubeadm/)
