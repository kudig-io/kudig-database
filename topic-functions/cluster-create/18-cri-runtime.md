# 容器运行时 (CRI) 详解

## 源码路径

`cmd/kubeadm/app/phases/kubelet/`
`cmd/kubeadm/app/preflight/crictl.go`
`pkg/kubelet/cri/`

---

## CRI 架构

Kubernetes 通过 CRI (Container Runtime Interface) 与容器运行时交互:

```
                    ┌─────────────────────────┐
                    │      kubelet             │
                    │                          │
                    │   CRI API (gRPC)        │
                    └───────────┬─────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        ↓                       ↓                       ↓
┌──────────────┐      ┌──────────────┐       ┌──────────────┐
│  containerd   │      │    docker    │       │    cri-o     │
│   (原生)      │      │  (cri-dockerd)│       │   (原生)     │
└──────────────┘      └──────────────┘       └──────────────┘
```

---

## 容器运行时对比

| 运行时 | socket | 特点 | kubeadm 支持 |
|--------|--------|------|-------------|
| containerd | `/var/run/containerd/containerd.sock` | 轻量、K8s 默认 | ✅ 原生 |
| docker | `/var/run/docker.sock` | 广泛使用、成熟 | ⚠️ via cri-dockerd |
| cri-o | `/var/run/cri-dockerd.sock` | 纯 OCI、标准 | ✅ 原生 (1.24+) |

---

## Pause 容器

每个 Pod 都有一个基础设施容器 (pause container):

```bash
# pause 容器作用:
# 1. 作为 Pod 内所有容器的父容器
# 2. 持有 Pod 的网络栈 (netns)
# 3. 接收 SIGTERM 时优雅终止所有业务容器

# pause 镜像:
# k8s.gcr.io/pause:3.9

# 容器启动顺序:
# 1. pause 容器启动 (创建 netns)
# 2. 业务容器 join pause 的 netns (--network=container:pause)
# 3. pause 容器等待所有业务容器退出
```

---

## containerd 配置

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri"]
  # 容器镜像配置
  sandbox_image = "k8s.gcr.io/pause:3.9"

  # 容器日志配置
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"

  # 日志驱动
  [plugins."io.containerd.grpc.v1.cri".logging]
    maxSize = "10Mi"
    maxFiles = 3

# CNI 配置目录
[plugins."io.containerd.cri.v1.cri".cni]
  binDir = "/opt/cni/bin"
  confDir = "/etc/cni/net.d"
```

---

## kubeadm 中的 CRI 配置

```bash
# 指定 CRI socket
kubeadm init --cri-socket=/var/run/containerd/containerd.sock

# kubelet 配置文件中的 containerRuntimeEndpoint
# /var/lib/kubelet/config.yaml
containerRuntimeEndpoint: unix:///var/run/containerd/containerd.sock
```

---

## 镜像预拉取

kubeadm init 不会自动拉取所有镜像，需要手动:

```bash
# 列出需要拉取的镜像
kubeadm config images list --kubernetes-version v1.28.0
# 输出:
# k8s.gcr.io/kube-apiserver:v1.28.0
# k8s.gcr.io/kube-controller-manager:v1.28.0
# k8s.gcr.io/kube-scheduler:v1.28.0
# k8s.gcr.io/kube-proxy:v1.28.0
# k8s.gcr.io/pause:3.9
# k8s.gcr.io/etcd:3.5.x
# k8s.gcr.io/coredns:1.10.x

# 预拉取所有镜像
kubeadm config images pull

# 指定镜像仓库
kubeadm config images pull \
  --image-repository=registry.cn-hangzhou.aliyuncs.com/google_containers
```

---

## crictl 工具

crictl 是 CRI 兼容容器的调试工具:

```bash
# 查看所有容器 (包括已停止)
crictl ps -a

# 查看运行中的容器
crictl ps

# 查看镜像
crictl images

# 查看 pod
crictl pods

# 查看容器日志
crictl logs <container-id>

# 停止/删除容器
crictl stop <container-id>
crictl rm <container-id>

# 手动拉取镜像
crictl pull k8s.gcr.io/pause:3.9

# 查看运行时信息
crictl info

# 连接到容器
crictl exec -it <container-id> sh
```

---

## kubelet 与 CRI 交互

```
kubelet 启动:
    ↓
读取 --container-runtime-endpoint (默认: /var/run/containerd/containerd.sock)
    ↓
通过 CRI gRPC API 与 containerd 通信
    ↓
创建 pause 容器 (PodSandbox)
    ↓
创建业务容器 (Containers)
    ↓
管理生命周期 (健康检查、日志、回收)
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `failed to create pod sandbox` | 网络插件未安装 | 先安装 CNI |
| `image pull failed` | 镜像仓库访问不了 | 配置 mirror 或 pre-pull |
| `container runtime not ready` | containerd 未启动 | `systemctl start containerd` |
| `cni config not found` | CNI 配置文件缺失 | 检查 /etc/cni/net.d |
| `too many open files` | 文件描述符限制 | 修改 /etc/security/limits.conf |
