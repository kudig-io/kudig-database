---
title: kubeadm 集群创建生命周期
description: '## 概述'
summary: '`kubeadm init` 是 Kubernetes 官方推荐的集群引导工具，采用"最小化集群引导"设计理念——只安装核心控制面组件，不安装 CNI 等附加组件。整个初始化过程被分解为 12 个有序的阶段（Phase），每个阶段完成特定任务，支持独立执行和跳过。'
category: skills
tags:
- k8s
- kubeadm
- cluster-lifecycle
- init
- preflight
- certs
- kubeconfig
- control-plane
- etcd
- apiserver
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 集群创建生命周期 是什么
- 如何 kubeadm 集群创建生命周期
trigger_keywords:
- kubeadm
- 集群创建生命周期
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---



# kubeadm 集群创建生命周期

## 概述

`kubeadm init` 是 Kubernetes 官方推荐的集群引导工具，采用"最小化集群引导"设计理念——只安装核心控制面组件，不安装 CNI 等附加组件。整个初始化过程被分解为 12 个有序的阶段（Phase），每个阶段完成特定任务，支持独立执行和跳过。

## kubeadm init 12 个阶段

| # | Phase | 说明 | 关键输出 |
|---|-------|------|---------|
| 1 | `preflight` | 系统预检：端口、内核参数、cgroup、容器运行时 | 预检通过/失败报告 |
| 2 | `certs` | PKI 证书生成：3 组 CA + 14 对证书/密钥 | `/etc/kubernetes/pki/` |
| 3 | `kubeconfig` | kubeconfig 文件生成：admin/controller-manager/scheduler | 4 个 kubeconfig 文件 |
| 4 | `kubelet-start` | 写入 [[kubelet|kubelet]] 配置并启动服务 | `/var/lib/kubelet/config.yaml` |
| 5 | `control-plane` | 生成控制面静态 Pod manifest | `/etc/kubernetes/manifests/` |
| 6 | `etcd` | 生成 etcd 静态 Pod manifest | etcd 静态 Pod |
| 7 | `wait-control-plane` | 等待 API Server 就绪（轮询 /healthz） | API Server 200 OK |
| 8 | `upload-config` | 上传 InitConfiguration 和 ClusterConfiguration 到 ConfigMap | `kubeadm-config` ConfigMap |
| 9 | `mark-control-plane` | 添加 control-plane 标签和 NoSchedule 污点 | 节点标签和污点 |
| 10 | `bootstrap-token` | 创建 Bootstrap Token Secret | `bootstrap-token-xxx` Secret |
| 11 | `kubelet-finalize` | 终止 kubelet 证书引导，切换到正式证书 | 证书轮换就绪 |
| 12 | `addon` | 部署 [[CoreDNS|CoreDNS]] + kube-proxy [[DaemonSet|DaemonSet]] | 核心附加组件 |

## 核心参数说明

### kubeadm init 关键参数

| 标志 | 默认值 | 说明 |
|------|--------|------|
| `--config` | 无 | 配置文件路径（推荐） |
| `--cri-socket` | 自动检测 | CRI socket 路径 |
| `--kubernetes-version` | `stable-1` | Kubernetes 版本 |
| `--pod-network-cidr` | 无 | Pod CIDR（必须与 CNI 匹配） |
| `--apiserver-advertise-address` | 自动检测 | API Server 公告地址 |
| `--control-plane-endpoint` | 无 | HA 负载均衡地址 |
| `--upload-certs` | `false` | 上传证书到 Secret（HA 场景） |
| `--skip-phases` | 无 | 跳过指定阶段 |
| `--dry-run` | `false` | 干跑模式 |

## 配置文件示例

```yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 192.168.1.10
  bindPort: 6443
nodeRegistration:
  name: master-1
  criSocket: unix:///var/run/containerd/containerd.sock
  taints:
  - key: node-role.kubernetes.io/control-plane
    effect: NoSchedule
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
clusterName: production-cluster
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "lb.example.com:6443"
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
apiServer:
  certSANs:
    - "lb.example.com"
  extraArgs:
    audit-log-path: "/var/log/kubernetes/audit.log"
etcd:
  local:
    dataDir: "/var/lib/etcd"
---
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
rotateCertificates: true
serverTLSBootstrap: true
cgroupDriver: systemd

```

## 预检阶段（preflight）详细检查项

preflight 阶段执行以下检查，任何检查失败都会阻止 init 继续进行：

- **端口检查**：6443（API Server）、10250（kubelet）、2379-2380（etcd）等是否被占用
- **内核参数**：`net.bridge.bridge-nf-call-iptables` 是否设为 1
- **cgroup 驱动**：kubelet 和容器运行时的 cgroup driver 是否一致（推荐 systemd）
- **Swap 状态**：默认要求关闭 swap（可通过 `--ignore-preflight-errors=Swap` 忽略）
- **容器运行时**：[[containerd|containerd]]/cri-o/docker 是否已安装并运行
- **权限检查**：是否以 root 用户运行
- **主机名**：是否符合 DNS 子域规范

## kubeadm 不安装的组件

kubeadm 采用最小化设计，以下组件需要手动安装：

| 组件 | 说明 | 安装方式 |
|------|------|---------|
| CNI 网络插件 | Pod 跨节点通信 | Calico/Cilium/Flannel |
| 存储插件 | 持久化存储 | CSI 驱动（Rook/Longhorn 等） |
| Ingress Controller | 外部流量入口 | NGINX/Traefik/Contour |
| Metrics Server | 资源指标 | `kubectl apply -f metrics-server.yaml` |
| Dashboard | Web UI | `kubectl apply -f dashboard.yaml` |

## 云厂商对比

| 方案 | 控制面管理 | etcd 管理 | CNI | 升级方式 |
|------|-----------|----------|-----|---------|
| kubeadm | 手动 | 手动（stacked/external） | 自选 | `kubeadm upgrade` |
| EKS | AWS 托管 | AWS 托管 | VPC CNI | AWS 控制升级 |
| GKE | Google 托管 | Google 托管 | GKE CNI | 自动/手动 |
| AKS | Microsoft 托管 | Microsoft 托管 | Azure CNI | 自动/手动 |

## 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| 端口被占用 | `[preflight] Port 6443 is in use` | 停止占用进程或指定 `--apiserver-bind-port` |
| 证书已存在 | 重复 init | 删除 `/etc/kubernetes/pki` 后重试 |
| kubelet 启动失败 | cgroup driver 不匹配 | 统一使用 `systemd` |
| CoreDNS Pending | 缺少 CNI 插件 | 安装 Calico/Cilium |
| 镜像拉取失败 | 网络问题 | `kubeadm config images pull` 预拉取 |

## 相关技能

- [[skills/kubeadm-ha-cluster-setup.md|kubeadm 高可用集群搭建]]
- [[skills/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]]
- [[concepts/kubernetes-pki-certificate-system.md|Kubernetes PKI 证书体系]]
- [[skills/configure-health-probes.md|配置健康探针]]
- [[deployment|Deployment]]

## Related

- [[cri-o]] — CRI-O
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-pki-certificate-system.md|kubernetes-pki-certificate-system]] — Kubernetes PKI 证书体系

- 15-kubelet-deep-dive
- 17-apiserver-tuning
- 32-kubeadm-cluster-lifecycle
```