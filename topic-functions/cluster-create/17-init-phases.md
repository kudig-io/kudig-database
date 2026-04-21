# init 阶段详解: mark-control-plane 与 upload-config

## 源码路径

`cmd/kubeadm/app/phases/controlplane/` (mark-control-plane)
`cmd/kubeadm/app/phases/uploadconfig/` (upload-config)

---

## init 完整阶段列表

kubeadm init 实际包含 17 个阶段:

```
1.  certs              生成 PKI 证书
2.  kubeconfig         生成 kubeconfig
3.  kubeconfig/admin   生成 admin kubeconfig
4.  etcd                生成 etcd manifest
5.  control-plane       生成 control plane manifests
6.  kubelet-start       写入 kubelet 配置 + 启动 kubelet
7.  wait-control-plane  等待 API Server 就绪
8.  upload-config       上传 kubeadm-config ConfigMap
9.  bootstrap-token     创建 bootstrap token
10. mark-control-plane  标记节点为 control-plane
11. etcd (wait)        等待 etcd 就绪
12. (post-stage)       后处理
```

---

## mark-control-plane 阶段

**源码**: `cmd/kubeadm/app/phases/controlplane/markcontrolplane.go`

```go
func MarkControlPlane(cfg *InitConfiguration) error {
    // 1. 给节点打标签
    kubectl label node <node-name> node-role.kubernetes.io/control-plane-

    // 2. 添加污点 (防止 Pod 调度到 control-plane)
    kubectl taint node <node-name> node-role.kubernetes.io/control-plane:NoSchedule
}
```

**效果**:

```bash
# 节点标签
kubectl get nodes --show-labels | grep node-role
# 输出:
# NAME     LABELS
# master   node-role.kubernetes.io/control-plane=

# 节点污点
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'
# 输出:
# [{"effect":"NoSchedule","key":"node-role.kubernetes.io/control-plane"}]
```

**注意**: 污点 `NoSchedule` 意味着除非 Pod 容忍该污点，否则不会被调度到 control-plane。

---

## kubeadm init --skip-phases

```bash
# 跳过特定阶段
kubeadm init --skip-phases=mark-control-plane

# 从某个阶段开始恢复
kubeadm init --skip-phases=preflight,certs

# 查看可跳过的阶段
kubeadm init --help | grep -A 20 "skip-phases"
```

---

## upload-config 阶段

**源码**: `cmd/kubeadm/app/phases/uploadconfig/uploadconfig.go`

```go
func UploadConfiguration(cfg *InitConfiguration) error {
    // 上传 InitConfiguration 到 ConfigMap: kube-system/kubeadm-config
    // 内容包括:
    // - ClusterConfiguration (版本、网络、组件镜像等)
    // - InitConfiguration (API Server 端点、证书密钥等，加密)
    // - BootstrapTokens 信息
}
```

**ConfigMap 内容**:

```bash
kubectl get configmap kubeadm-config -n kube-system -o yaml

# 关键字段:
# kind: InitConfiguration
# spec:
#   localAPIEndpoint:
#     advertiseAddress: "192.168.1.1"
#     bindPort: 6443
#   clusterConfiguration:
#     kubernetesVersion: v1.28.0
#     networking:
#       podSubnet: 10.244.0.0/16
#       serviceSubnet: 10.96.0.0/12
```

---

## ClusterConfiguration vs InitConfiguration

| 字段 | ClusterConfiguration | InitConfiguration |
|------|---------------------|-------------------|
| 存储位置 | ConfigMap | ConfigMap (加密) |
| 内容 | 集群级别配置 | 节点级别配置 |
| 用途 | 集群升级时读取 | 新节点 join 时使用 |
| 包含敏感信息 | 否 | 是 (证书密钥) |

```yaml
# ClusterConfiguration (非敏感)
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.28.0
networking:
  podSubnet: 10.244.0.0/16
  serviceSubnet: 10.96.0.0/12
  dnsDomain: cluster.local
apiServer:
  extraArgs:
    service-node-port-range: "30000-32767"
controllerManager:
  extraArgs:
    node-cidr-mask-size: "24"

---

# InitConfiguration (敏感)
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: "192.168.1.1"
  bindPort: 6443
nodeRegistration:
  criSocket: /var/run/containerd/containerd.sock
  name: master-1
  taints: null
```

---

## kubeadm config 家族

```bash
# 查看当前集群配置
kubectl get configmap kubeadm-config -n kube-system -o yaml

# 从集群拉取 InitConfiguration (不含密钥)
kubeadm config view > init-config.yaml

# 生成默认组件镜像列表
kubeadm config images list --kubernetes-version v1.28.0

# 拉取所有镜像
kubeadm config images pull

# 配合配置文件
kubeadm config images pull --image-repository=registry.cn-hangzhou.aliyuncs.com/google_containers
```

---

## kubeadm join --config 详解

```yaml
# join-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: JoinConfiguration
discovery:
  bootstrapToken:
    apiServerEndpoint: lb:6443
    token: abc123.def456
    caCertHashes:
    - sha256:xxxx
  timeout: 5m
nodeRegistration:
  criSocket: /var/run/containerd/containerd.sock
  name: node-1
  taints:
  - effect: NoSchedule
    key: node-role.kubernetes.io/worker
```

---

## phase 子命令

```bash
# 分阶段执行 init (不推荐生产使用，仅调试)
kubeadm init phase certs all --cert-dir=/tmp/pki
kubeadm init phase kubeconfig all --cert-dir=/tmp/pki
kubeadm init phase control-plane all
kubeadm init phase etcd local
kubeadm init phase kubelet-start

# 查看所有 phase
kubeadm init phase --help

# 输出:
# cgroup              验证 cgroup 环境
# certs               证书阶段
# control-plane       生成控制面 manifests
# etcd                生成 etcd manifest
# kubeconfig          生成 kubeconfig
# kubelet-start       写入 kubelet 配置
# mark-control-plane  标记 control-plane
# preflight           预检
# upload-config       上传配置到 ConfigMap
# wait-control-plane  等待控制面就绪
```

---

## kubeadm.alpha.certs (已废弃)

```bash
# 旧命令 (1.24+ 已废弃):
kubeadm alpha certs renew all

# 新命令:
kubeadm certs renew all

# 查看证书列表
kubeadm certs list

# 输出:
# CERTIFICATE                EXPIRES                RESIDUAL TIME
# ca                        2033-12-20 00:00:00     9y
# apiserver                 2024-12-20 00:00:00     1y
# apiserver-kubelet-client  2024-12-20 00:00:00     1y
# front-proxy-ca            2033-12-20 00:00:00     9y
# front-proxy-client        2024-12-20 00:00:00     1y
# etcd-ca                   2033-12-20 00:00:00     9y
# etcd-server               2024-12-20 00:00:00     1y
```
