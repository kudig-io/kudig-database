# 控制面阶段 (Control Plane & Static Pods)

## 源码路径

`cmd/kubeadm/app/phases/controlplane/manifests.go`
`cmd/kubeadm/app/phases/etcd/local.go`

---

## 静态 Pod manifests 生成

kubeadm 在以下路径写入静态 Pod manifests:

```
/etc/kubernetes/manifests/
├── etcd.yaml           # etcd 静态 Pod
├── kube-apiserver.yaml # API Server
├── kube-controller-manager.yaml
└── kube-scheduler.yaml
```

---

## kube-apiserver.yaml 关键参数

```yaml
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: kube-apiserver
    image: k8s.gcr.io/kube-apiserver:v1.x.x
    command:
    - kube-apiserver
    - --advertise-address=<node-ip>
    - --service-cluster-ip-range=<service-cidr>
    - --service-account-issuer=api
    - --service-account-signing-key-file=/etc/kubernetes/pki/sa.key
    - --service-account-api-audiences=api
    - --etcd-servers=https://127.0.0.1:2379
    - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
    - --etcd-certfile=/etc/kubernetes/pki/etcd/server.crt
    - --etcd-keyfile=/etc/kubernetes/pki/etcd/server.key
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt
    - --kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key
    - --enable-admission-plugins=NodeRestriction
    - --runtime-config=api/all=true
    volumeMounts:
    - name: etcd-certs
      mountPath: /etc/kubernetes/pki/etcd
    - name: kube-certs
      mountPath: /etc/kubernetes/pki
  hostNetwork: true
  priority: 2000000000
  priorityClassName: system-node-critical
```

---

## kube-controller-manager.yaml 关键参数

```yaml
command:
- kube-controller-manager
- --leader-elect=true
- --controllers=*,bootstrapsigner,tokencleaner
- --service-cluster-ip-range=<service-cidr>
- --cluster-cidr=<pod-cidr>
- --root-ca-file=/etc/kubernetes/pki/ca.crt
- --service-account-private-key-file=/etc/kubernetes/pki/sa.key
- --horizontal-pod-autoscaler-sync-period=15s
- --flex-volume-plugin-dir=/etc/kubernetes/volumeplugins
```

---

## kube-scheduler.yaml 关键参数

```yaml
command:
- kube-scheduler
- --leader-elect=true
- --scheduler-name=default-scheduler
- --profiling=true
```

---

## kube-proxy 部署

kube-proxy 不是静态 Pod，而是 DaemonSet (除非 CNI 自行实现):

```go
// kubeadm init 完成后:
// 1. 创建 kube-proxy ServiceAccount
// 2. 创建 kube-proxy ConfigMap (iptables/ipvs 配置)
// 3. 创建 kube-proxy DaemonSet
// 4. kubelet 在每个节点启动 kube-proxy 容器
```

```bash
# 验证 kube-proxy 部署
kubectl get ds -n kube-system -l k8s-app=kube-proxy
# 输出:
# NAME         DESIRED   CURRENT   READY   AGE
# kube-proxy   3         3         3       10m
```

---

## kubelet config.yaml 完整参数

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
address: 0.0.0.0                          # 监听地址
port: 10250                               # kubelet API 端口
readOnlyPort: 10255                       # 只读端口 (已废弃)
cgroupDriver: systemd                      # cgroup driver (systemd/cgroupfs)
cgroupVersion: 2                           # cgroup v1/v2
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
tlsCertFile: /var/lib/kubelet/pki/kubelet.crt
tlsPrivateKeyFile: /var/lib/kubelet/pki/kubelet.key
authentication:
  anonymous:
    enabled: false                         # 禁止匿名访问
  webhook:
    cacheTTL: 2h0m0s
    enabled: true                          # 使用 API Server 授权
  bootstrap:
    enabled: true
    token: <bootstrap-token>               # 首次启动用 Bootstrap Token
authorization:
  mode: Webhook                           # 委托给 API Server 授权
serverTLSBootstrap: true                   # 向 API Server 申请正式证书
rotateCertificates: true                   # 自动轮换证书
runtimeRequestTimeout: 2m0s
evictionHard:
  memory.available: 100Mi
  nodefs.available: 10%
  imagefs.available: 15%
```

---

## cgroup Driver 检测

kubelet 会自动检测宿主机的 cgroup driver:

```go
// 检测方式:
func detectCgroupDriver() string {
    // 检查 /sys/fs/cgroup/cgroup.controllers
    // 如果存在，优先使用 cgroup v2
    // 否则使用 cgroup v1 (systemd 或 cgroupfs)
}
```

```bash
# 查看当前 cgroup 版本
mount | grep cgroup

# 查看 cgroup driver
cat /sys/fs/cgroup/cgroup.controllers  # v2 有此文件
cat /sys/fs/cgroup/unified/cgroup.controllers  # v1
```

---

## kubelet 启动流程

```go
// cmd/kubeadm/app/phases/kubelet/config.go
func writeKubeletConfig() error {
    // 1. 生成 /var/lib/kubelet/config.yaml
    // 2. 生成 /etc/kubernetes/bootstrap-kubelet.conf
    // 3. 启动 kubelet 服务
}
```

kubelet 读取 `config.yaml` 中配置启动，连接 API Server:

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
serverTLSBootstrap: true  # 向 API Server 申请证书
authentication:
  bootstrap:
    token: <bootstrap-token>
  webhook:
    enabled: true
    cacheTTL: 2h0m0s
authorization:
  mode: Webhook
```

---

## 等待控制面就绪

```go
// cmd/kubeadm/app/phases/controlplane/wait.go
func WaitForControlPlane() error {
    // 轮询检查:
    // 1. /healthz 的 API Server 健康检查端点
    // 2. etcd 健康检查
    // 3. 等待所有静态 Pod 状态变为 Running
}
```

---

## 关键: 静态 Pod 由 kubelet 直接管理

```
kubelet 启动 → 读取 /etc/kubernetes/manifests/*.yaml → 创建 Pod → 容器运行时拉取镜像并启动
```

不使用 Kubernetes 调度，不经过 API Server，不需要节点上的 kubelet 先有证书。
