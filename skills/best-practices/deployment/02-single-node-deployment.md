---
title: 02 - 单节点部署 (Single Node All-in-One) [deployment]
description: 'title: 02 - 单节点部署 (Single Node All-in-One)'
category: general
tags:
- deployment
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- flannel
- calico
- coredns
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 单节点部署 (Single Node All-in-One) 是什么
- 如何 单节点部署 (Single Node All-in-One)
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 单节点部署
- Single
- Node
- All-in-One
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- ebpf-basics
- cni-basics
- etcd-basics
- mysql-basics
created: "2026-05-23"
---

title: 02 - 单节点部署 (Single Node All-in-One)
description: '# 02 - 单节点部署 (Single Node All-in-One)'
category: deployment
tags:
- k8s
- deployment
- rolling-update
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
- flannel
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 单节点部署 (Single Node All-in-One) 是什么
- 如何 单节点部署 (Single Node All-in-One)
trigger_keywords:
- 单节点部署
- Single
- Node
- All-in-One
- deployment
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

# 02 - 单节点部署 (Single Node All-in-One)

> **适用场景**: 个人开发者、小团队、边缘节点 | **预计时间**: 1-2 小时 | **复杂度**: ⭐⭐  
> **目标**: 在一台真实 Linux 服务器上部署完整 K8s，体验真实组件协作方式

---

<!-- chunk: 概述 -->## 概述

单节点部署将 Kubernetes 控制平面和工作节点合并在一台物理机或虚拟机上运行。与 kind/minikube 不同，本方案**直接在操作系统上部署真实的 K8s 组件**，更接近生产环境的体验。

**本文你将学会**:
- 完整的 Linux 系统准备（内核参数、防火墙、SELinux 等）
- k3s 一键安装和自定义配置
- kubeadm 标准安装的完整流程
- containerd 运行时配置
- CNI 网络插件安装
- 基础设施组件（metrics-server、存储、Ingress）

**适用场景**:
- 个人开发和学习（需要接近真实环境的体验）
- 小团队内部测试环境
- 边缘计算节点 / IoT 网关
- 资源受限的小规模部署
- CI/CD Runner 节点

---

<!-- chunk: 前置条件 -->## 前置条件

## 操作系统支持

| 发行版 | 版本 | 说明 |
|--------|------|------|
| **Ubuntu** | 20.04 / 22.04 / 24.04 LTS | 推荐首选，社区支持最好 |
| **CentOS** | 7.9 / Stream 8 / Stream 9 | CentOS 7 将 EOL，建议 Stream 8+ |
| **RHEL** | 8.x / 9.x | 企业级支持 |
| **Debian** | 11 / 12 | 稳定可靠 |
| **Rocky Linux** | 8.x / 9.x | CentOS 替代品 |

## 硬件要求

| 方案 | CPU (最低/推荐) | 内存 (最低/推荐) | 磁盘 (最低/推荐) | 网络 |
|------|----------------|-----------------|-----------------|------|
| **k3s** | 1核 / 2核+ | 512MB / 2GB+ | 5GB / 20GB+ | 可选互联网 |
| **kubeadm** | 2核 / 4核+ | 2GB / 4GB+ | 20GB / 50GB+ | 需要互联网 (或离线包) |
| **MicroK8s** | 1核 / 2核+ | 1GB / 4GB+ | 10GB / 30GB+ | 需要互联网 |

---

<!-- chunk: 通用系统准备 (所有方案必做) -->## 通用系统准备 (所有方案必做)

> **重要**: 以下步骤适用于所有方案 (k3s / kubeadm / MicroK8s)，请务必完成。

## 1. 设置主机名

```bash
# 设置有意义的主机名
sudo hostnamectl set-hostname k8s-single-node

# 写入 hosts (确保主机名可解析)
echo "$(hostname -I | awk '{print $1}') $(hostname)" | sudo tee -a /etc/hosts

# 验证
hostname
# 预期输出: k8s-single-node
ping -c 1 $(hostname)
# 预期: 能 ping 通
```

## 2. 关闭 Swap

> **为什么要关闭 Swap？** kubelet 默认要求关闭 swap，因为 swap 会导致 Pod 的内存限制失效，影响调度器的决策准确性。

```bash
# 立即关闭 swap
sudo swapoff -a

# 永久关闭 (注释掉 /etc/fstab 中的 swap 行)
sudo sed -i '/swap/s/^/#/' /etc/fstab
# 备注: 此命令将 fstab 中包含 "swap" 的行前面加上 # 号

# 验证 swap 已关闭
free -h | grep Swap
# 预期输出: Swap:          0B          0B          0B
# 如果 Swap total 不是 0，说明没关成功
```

## 3. 关闭防火墙 (开发/测试环境)

> **备注**: 生产环境不建议关闭防火墙，而是配置允许规则。这里为了简化操作先关闭。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# ===== Ubuntu/Debian =====
sudo ufw disable
sudo ufw status
# 预期输出: Status: inactive

# ===== CentOS/RHEL/Rocky =====
sudo systemctl stop firewalld
sudo systemctl disable firewalld
sudo systemctl status firewalld
# 预期输出: Active: inactive (dead)

# ===== 生产环境替代方案: 只开放必要端口 =====
# K8s 必需端口:
# 6443    - kube-apiserver (HTTPS API)
# 2379-2380 - etcd (客户端+对等通信)
# 10250   - kubelet API
# 10259   - kube-scheduler
# 10257   - kube-controller-manager
# 30000-32767 - NodePort 范围
#
# 示例 (firewalld):
# sudo firewall-cmd --permanent --add-port=6443/tcp
# sudo firewall-cmd --permanent --add-port=10250/tcp
# sudo firewall-cmd --permanent --add-port=30000-32767/tcp
# sudo firewall-cmd --reload
```

## 4. 关闭 SELinux (CentOS/RHEL)

> **为什么？** SELinux 的严格模式会阻止 kubelet 和容器的某些操作。可以设为 permissive (只记录不阻止)。

```bash
# 查看当前状态
getenforce
# 如果输出 Enforcing，需要关闭

# 临时设置为 permissive
sudo setenforce 0

# 永久设置
sudo sed -i 's/^SELINUX=enforcing/SELINUX=permissive/' /etc/selinux/config

# 验证
getenforce
# 预期输出: Permissive
```

## 5. 加载内核模块

> **为什么需要这些模块？**  
> - `overlay`: 容器文件系统 (OverlayFS) 需要  
> - `br_netfilter`: 允许 iptables 处理桥接网络流量 (CNI 网络依赖)

```bash
# 配置开机自动加载
cat <<EOF | sudo tee /etc/modules-load.d/k8s.conf
overlay
br_netfilter
EOF

# 立即加载
sudo modprobe overlay
sudo modprobe br_netfilter

# 验证模块已加载
lsmod | grep -E "overlay|br_netfilter"
# 预期输出:
# br_netfilter           xxxxx  0
# overlay                xxxxx  0
```

## 6. 配置内核网络参数

> **为什么需要这些参数？**  
> - `bridge-nf-call-iptables`: 确保 Pod 间的桥接流量能被 iptables/netfilter 处理  
> - `ip_forward`: 允许节点进行 IP 转发（Pod 网络必需）

```bash
cat <<EOF | sudo tee /etc/sysctl.d/k8s.conf
# 允许 iptables 查看桥接流量
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
# 允许 IP 转发 (Pod 网络必需)
net.ipv4.ip_forward                 = 1
EOF

# 立即应用
sudo sysctl --system

# 验证参数生效
sysctl net.bridge.bridge-nf-call-iptables net.bridge.bridge-nf-call-ip6tables net.ipv4.ip_forward
# 预期输出: 三个值都是 1
```

## 7. 时间同步

> **为什么重要？** 证书验证、日志时间戳、etcd 一致性都依赖准确的时间。

```bash
# Ubuntu/Debian
sudo apt-get install -y chrony
sudo systemctl enable chrony && sudo systemctl start chrony

# CentOS/RHEL
sudo yum install -y chrony
sudo systemctl enable chronyd && sudo systemctl start chronyd

# 验证时间同步
chronyc tracking | grep "System time"
# 预期: System time 偏差应小于 1 秒
timedatectl
# 预期: NTP synchronized: yes (或 System clock synchronized: yes)
```

---

<!-- chunk: 方案 A: k3s 单节点部署 (推荐) -->## 方案 A: k3s 单节点部署 (推荐)

> **k3s** 是 Rancher (SUSE) 推出的轻量级 Kubernetes 发行版，CNCF 认证。  
> **特点**: 单个二进制文件 (~60MB)，内置 Traefik Ingress、CoreDNS、本地存储、ServiceLB。  
> **最适合**: 个人开发、边缘计算、IoT、资源受限场景。

## A1. 在线安装

```bash
# ===== 最简安装 (一条命令) =====
curl -sfL https://get.k3s.io | sh -

# 预期输出:
# [INFO]  Finding release for channel stable
# [INFO]  Using v1.28.x+k3s1 as release
# [INFO]  Downloading hash https://github.com/k3s-io/k3s/releases/download/...
# [INFO]  Installing k3s to /usr/local/bin/k3s
# [INFO]  systemd: Creating service file
# [INFO]  systemd: Enabling k3s unit
# [INFO]  systemd: Starting k3s

# 验证服务状态
sudo systemctl status k3s
# 预期: Active: active (running)

# 查看节点 (k3s 自带 kubectl)
sudo k3s kubectl get nodes
# 预期输出:
# NAME              STATUS   ROLES                  AGE   VERSION
# k8s-single-node   Ready    control-plane,master   1m    v1.28.x+k3s1
```

## A2. 配置 kubectl (免 sudo)

```bash
# 复制 kubeconfig 到用户目录
mkdir -p ~/.kube
sudo cp /etc/rancher/k3s/k3s.yaml ~/.kube/config
sudo chown $(id -u):$(id -g) ~/.kube/config

# 设置环境变量 (写入 shell 配置文件)
echo 'export KUBECONFIG=~/.kube/config' >> ~/.bashrc
source ~/.bashrc

# 验证 (不需要 sudo 了)
kubectl get nodes
# 预期: 显示节点信息

kubectl get pods -A
# 预期输出 (k3s 内置组件):
# NAMESPACE     NAME                                      READY   STATUS    RESTARTS   AGE
# kube-system   coredns-xxx                               1/1     Running   0          2m  ← DNS 服务
# kube-system   helm-install-traefik-xxx                  0/1     Completed 0          2m  ← Traefik 安装 Job
# kube-system   helm-install-traefik-crd-xxx              0/1     Completed 0          2m  ← Traefik CRD
# kube-system   local-path-provisioner-xxx                1/1     Running   0          2m  ← 本地存储
# kube-system   metrics-server-xxx                        1/1     Running   0          2m  ← 指标服务
# kube-system   svclb-traefik-xxx                         2/2     Running   0          2m  ← ServiceLB
# kube-system   traefik-xxx                               1/1     Running   0          2m  ← Ingress Controller
```

## A3. 自定义安装选项

```bash
# ===== 常用安装参数 =====

# 禁用 Traefik (想用 Nginx Ingress 或其他 Ingress Controller)
curl -sfL https://get.k3s.io | INSTALL_K3S_EXEC="--disable traefik" sh -

# 禁用多个内置组件
curl -sfL https://get.k3s.io | INSTALL_K3S_EXEC="--disable traefik --disable servicelb" sh -

# 指定版本安装
curl -sfL https://get.k3s.io | INSTALL_K3S_VERSION="v1.28.4+k3s1" sh -

# 指定数据目录 (默认 /var/lib/rancher/k3s)
curl -sfL https://get.k3s.io | sh -s - \
  --data-dir /data/k3s \
  --write-kubeconfig-mode 644 \
  --node-name my-node

# 使用外部数据库 (适合后续扩展为多 Master)
curl -sfL https://get.k3s.io | sh -s - \
  --datastore-endpoint="mysql://user:pass@tcp(db-host:3306)/k3s"
```

## A4. k3s 配置文件方式 (推荐)

> **备注**: 对于复杂配置，使用配置文件比命令行参数更清晰、易维护。

```yaml
# 创建配置文件: /etc/rancher/k3s/config.yaml
# k3s 启动时会自动读取此文件
sudo mkdir -p /etc/rancher/k3s
sudo tee /etc/rancher/k3s/config.yaml << 'EOF'
# k3s 服务端配置文件
# 文档: https://docs.k3s.io/installation/configuration

# kubeconfig 文件权限 (644 = 所有用户可读)
write-kubeconfig-mode: "0644"

# 节点名称
node-name: "single-node"

# 网络 CIDR 配置
cluster-cidr: "10.42.0.0/16"     # Pod 网络地址段 (默认值)
service-cidr: "10.43.0.0/16"     # Service 网络地址段 (默认值)

# 数据目录
data-dir: "/data/k3s"

# 禁用不需要的内置组件
disable:
  - traefik      # 禁用 Traefik (如果要用 Nginx Ingress)
  - servicelb    # 禁用 ServiceLB (如果不需要 LoadBalancer)

# 本地存储路径
default-local-storage-path: "/data/k3s/storage"

# TLS SAN (如果需要通过域名或其他 IP 访问 API Server)
# tls-san:
#   - "k8s.example.com"
#   - "192.168.1.100"
EOF

# 重启 k3s 使配置生效
sudo systemctl restart k3s

# 验证配置
sudo k3s check-config
```

## A5. k3s 离线安装

> **场景**: 服务器没有互联网访问，需要提前下载安装包。

```bash
# ===== 在有网络的机器上准备 =====

# 1. 下载 k3s 二进制文件
wget https://github.com/k3s-io/k3s/releases/download/v1.28.4+k3s1/k3s

# 2. 下载离线镜像包
wget https://github.com/k3s-io/k3s/releases/download/v1.28.4+k3s1/k3s-airgap-images-amd64.tar.gz

# 3. 下载安装脚本
wget https://get.k3s.io -O install.sh

# ===== 在目标服务器上安装 =====

# 1. 放置二进制文件
sudo cp k3s /usr/local/bin/k3s
sudo chmod +x /usr/local/bin/k3s

# 2. 放置离线镜像
sudo mkdir -p /var/lib/rancher/k3s/agent/images/
sudo cp k3s-airgap-images-amd64.tar.gz /var/lib/rancher/k3s/agent/images/

# 3. 执行离线安装
chmod +x install.sh
INSTALL_K3S_SKIP_DOWNLOAD=true ./install.sh

# 验证
sudo k3s kubectl get nodes
```

## A6. k3s 管理命令

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 查看服务状态
sudo systemctl status k3s

# 查看实时日志
sudo journalctl -u k3s -f
# 备注: -f 表示实时跟踪，按 Ctrl+C 退出

# 查看最近 50 行日志
sudo journalctl -u k3s --no-pager -n 50

# 重启服务
sudo systemctl restart k3s

# 停止/启动
sudo systemctl stop k3s
sudo systemctl start k3s

# 查看 k3s 版本和运行参数
k3s --version
ps aux | grep k3s  # 查看运行时参数

# 卸载 k3s (会清除所有 K8s 数据)
/usr/local/bin/k3s-uninstall.sh
# 备注: 此脚本会: 停止服务 → 删除二进制 → 清理数据目录 → 删除网络规则
```

---

<!-- chunk: 方案 B: kubeadm 单节点部署 -->## 方案 B: kubeadm 单节点部署

> **kubeadm** 是 Kubernetes 官方提供的集群初始化工具。  
> **特点**: 完全标准的 K8s，与生产环境一致的组件架构，但需要手动安装更多组件。  
> **最适合**: 想深入理解 K8s 架构、准备向多节点/生产环境过渡的场景。

## B1. 安装容器运行时 (containerd)

> **为什么选 containerd？** 从 K8s 1.24 开始，Docker 不再直接支持作为运行时。containerd 是目前最主流的选择，轻量且稳定。

**Ubuntu/Debian**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 安装 containerd
sudo apt-get update
sudo apt-get install -y containerd

# 生成默认配置
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml

# 关键配置: 启用 SystemdCgroup
# 备注: kubelet 默认使用 systemd 作为 cgroup driver，containerd 必须匹配
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml

# 如果需要配置镜像加速 (国内用户)
# 编辑 /etc/containerd/config.toml，找到 [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
# 添加:
# [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
#   endpoint = ["https://docker.mirrors.ustc.edu.cn"]

# 重启 containerd
sudo systemctl restart containerd
sudo systemctl enable containerd

# 验证
sudo systemctl status containerd
# 预期: Active: active (running)

# 验证 cgroup driver
containerd config dump | grep SystemdCgroup
# 预期输出: SystemdCgroup = true
```

**CentOS/RHEL/Rocky**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 添加 Docker 仓库 (containerd 在 Docker 仓库中)
sudo yum install -y yum-utils
sudo yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo

# 安装 containerd
sudo yum install -y containerd.io

# 后续配置步骤与 Ubuntu 相同
sudo mkdir -p /etc/containerd
containerd config default | sudo tee /etc/containerd/config.toml
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
sudo systemctl restart containerd
sudo systemctl enable containerd
```

## B2. 安装 kubeadm、kubelet、kubectl

**Ubuntu/Debian**:
```bash
# 安装依赖
sudo apt-get install -y apt-transport-https ca-certificates curl gpg

# 添加 Kubernetes 官方 APT 仓库签名密钥
sudo mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.28/deb/Release.key | \
  sudo gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg

# 添加仓库
echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] https://pkgs.k8s.io/core:/stable:/v1.28/deb/ /' | \
  sudo tee /etc/apt/sources.list.d/kubernetes.list

# 安装指定版本
sudo apt-get update
sudo apt-get install -y kubelet kubeadm kubectl

# 锁定版本 (防止 apt upgrade 时意外升级)
sudo apt-mark hold kubelet kubeadm kubectl

# 验证
kubeadm version
# 预期: kubeadm version: &version.Info{Major:"1", Minor:"28", ...}
kubelet --version
kubectl version --client
```

**CentOS/RHEL/Rocky**:
```bash
# 添加 Kubernetes YUM 仓库
cat <<EOF | sudo tee /etc/yum.repos.d/kubernetes.repo
[kubernetes]
name=Kubernetes
baseurl=https://pkgs.k8s.io/core:/stable:/v1.28/rpm/
enabled=1
gpgcheck=1
gpgkey=https://pkgs.k8s.io/core:/stable:/v1.28/rpm/repodata/repomd.xml.key
exclude=kubelet kubeadm kubectl cri-tools kubernetes-cni
EOF

# 安装
sudo yum install -y kubelet kubeadm kubectl --disableexcludes=kubernetes

# 启用 kubelet (kubeadm init 后会自动启动)
sudo systemctl enable kubelet
```

## B3. 初始化集群

```bash
# ===== 预检查 (可选但推荐) =====
sudo kubeadm init --dry-run 2>&1 | head -30
# 备注: --dry-run 模拟初始化，检查环境是否满足要求，不实际执行

# ===== 正式初始化 =====
sudo kubeadm init \
  --pod-network-cidr=10.244.0.0/16 \
  --apiserver-advertise-address=0.0.0.0 \
  --kubernetes-version=v1.28.0
# 参数说明:
# --pod-network-cidr    Pod 网络地址段 (Flannel 默认使用 10.244.0.0/16，Calico 使用 192.168.0.0/16)
# --apiserver-advertise-address  API Server 监听地址 (0.0.0.0 表示监听所有网卡)
# --kubernetes-version  指定 K8s 版本 (避免使用默认的不稳定版本)

# 预期输出 (关键部分):
# [init] Using Kubernetes version: v1.28.0
# [preflight] Running pre-flight checks
# ...
# [addons] Applied essential addon: CoreDNS
# [addons] Applied essential addon: kube-proxy
#
# Your Kubernetes control-plane has initialized successfully!
#
# To start using your cluster, you need to run the following as a regular user:
#   mkdir -p $HOME/.kube
#   sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
#   sudo chown $(id -u):$(id -g) $HOME/.kube/config
#
# You can now join any number of machines by running the following on each node:
#   kubeadm join xxx:6443 --token xxx --discovery-token-ca-cert-hash sha256:xxx

# ===== 配置 kubectl =====
mkdir -p $HOME/.kube
sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
sudo chown $(id -u):$(id -g) $HOME/.kube/config

# 验证连接 (此时节点是 NotReady，因为还没装 CNI)
kubectl get nodes
# 预期输出:
# NAME              STATUS     ROLES           AGE   VERSION
# k8s-single-node   NotReady   control-plane   1m    v1.28.0
# 备注: NotReady 是正常的! 安装 CNI 后会变为 Ready
```

## B4. 允许 Master 节点调度 Pod

> **为什么？** 默认情况下，K8s 不允许在控制平面节点上运行用户 Pod (有 taint)。单节点模式必须移除这个限制。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
# 移除 control-plane 的 taint
kubectl taint nodes --all node-role.kubernetes.io/control-plane-
# 预期输出:
# node/k8s-single-node untainted

# 验证 taint 已移除
kubectl describe node | grep -A 3 Taints
# 预期输出: Taints: <none>
```

## B5. 安装 CNI 网络插件

> **CNI (Container Network Interface)** 负责为 Pod 分配 IP、实现 Pod 间通信。没有 CNI，Pod 无法联网。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# ===== 方案 1: Flannel (简单、适合学习) =====
# 备注: Flannel 只提供基本的网络互通，不支持 NetworkPolicy
kubectl apply -f https://github.com/flannel-io/flannel/releases/latest/download/kube-flannel.yml

# 预期输出:
# namespace/kube-flannel created
# serviceaccount/flannel created
# ...
# daemonset.apps/kube-flannel-ds created

# ===== 方案 2: Calico (功能更丰富，推荐) =====
# 备注: Calico 支持 NetworkPolicy、BGP、eBPF，适合进阶学习和生产
# 注意: 如果使用 Calico，kubeadm init 时 --pod-network-cidr 应设为 192.168.0.0/16
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.26.0/manifests/calico.yaml

# ===== 等待 CNI 就绪 =====
# 查看 CNI Pod 状态 (等待变为 Running)
kubectl get pods -n kube-system -w
# 或 (Flannel)
kubectl get pods -n kube-flannel -w

# 验证节点变为 Ready (安装 CNI 后约 30-60 秒)
kubectl get nodes
# 预期输出:
# NAME              STATUS   ROLES           AGE   VERSION
# k8s-single-node   Ready    control-plane   5m    v1.28.0
#                    ↑ 从 NotReady 变为 Ready
```

---

<!-- chunk: 方案 C: MicroK8s 单节点部署 -->## 方案 C: MicroK8s 单节点部署

> **MicroK8s** 是 Canonical (Ubuntu 母公司) 推出的轻量 K8s 发行版，通过 snap 包管理。  
> **最适合**: Ubuntu 系统用户、想要插件化管理的场景。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `chmod/chown -R`：递归改权限，误操作破坏系统文件访问

```bash
# 安装 (仅限支持 snap 的 Linux 发行版)
sudo snap install microk8s --classic --channel=1.28/stable

# 预期输出:
# microk8s (1.28/stable) v1.28.x from Canonical✓ installed

# 加入用户组 (免 sudo)
sudo usermod -a -G microk8s $USER
sudo chown -R $USER ~/.kube
newgrp microk8s  # 立即生效 (或重新登录)

# 等待 MicroK8s 就绪
microk8s status --wait-ready
# 预期: microk8s is running

# 启用常用插件
microk8s enable dns          # CoreDNS (必须)
microk8s enable storage      # 本地存储
microk8s enable ingress      # Ingress Controller
microk8s enable dashboard    # Web Dashboard
microk8s enable metrics-server  # 指标服务

# 查看启用的插件
microk8s status

# 配置 kubectl
# 方式 1: 使用 microk8s 自带的 kubectl
microk8s kubectl get nodes

# 方式 2: 导出 kubeconfig 给标准 kubectl 使用
microk8s config > ~/.kube/config
kubectl get nodes

# 访问 Dashboard
microk8s dashboard-proxy
# 预期: 输出 Dashboard URL 和 Token

# 卸载
# sudo snap remove microk8s
```

---

<!-- chunk: 部署后基础配置 (kubeadm 方案适用) -->## 部署后基础配置 (kubeadm 方案适用)

> **备注**: k3s 已内置以下大部分组件，MicroK8s 通过插件管理。kubeadm 需要手动安装。

## 安装 metrics-server

> **作用**: 提供节点和 Pod 的 CPU/内存指标，是 `kubectl top` 和 HPA 的数据来源。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# 单节点/自签名证书需要添加 --kubelet-insecure-tls 参数
# 备注: 因为 kubelet 使用的是自签名证书，metrics-server 默认不信任
kubectl patch deployment metrics-server -n kube-system --type='json' \
  -p='[{"op":"add","path":"/spec/template/spec/containers/0/args/-","value":"--kubelet-insecure-tls"}]'

# 等待就绪 (约 30 秒)
kubectl wait --for=condition=available deployment/metrics-server -n kube-system --timeout=60s

# 验证
kubectl top nodes
# 预期输出:
# NAME              CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# k8s-single-node   150m         3%     1200Mi          30%

kubectl top pods -A
# 预期: 显示所有 Pod 的 CPU/内存使用
```

## 安装本地存储 (StorageClass)

> **作用**: 提供 PVC 动态供给能力，应用可以声明存储需求并自动创建 PV。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 安装 local-path-provisioner (Rancher 出品，与 k3s 内置的相同)
kubectl apply -f https://raw.githubusercontent.com/rancher/local-path-provisioner/v0.0.26/deploy/local-path-storage.yaml

# 设置为默认 StorageClass
kubectl patch storageclass local-path -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 验证
kubectl get storageclass
# 预期输出:
# NAME                   PROVISIONER                    RECLAIMPOLICY   VOLUMEBINDINGMODE      ALLOWVOLUMEEXPANSION   AGE
# local-path (default)   rancher.io/local-path          Delete          WaitForFirstConsumer   false                  10s

# 测试 PVC 创建
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: test-pvc
spec:
  accessModes: ["ReadWriteOnce"]
  resources:
    requests:
      storage: 1Gi
EOF

kubectl get pvc test-pvc
# 预期: STATUS 为 Pending (WaitForFirstConsumer 模式，等有 Pod 使用时才绑定)

# 清理测试
kubectl delete pvc test-pvc
```

## 安装 Ingress Controller

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 安装 Nginx Ingress Controller (裸金属版本)
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-v1.8.2/deploy/static/provider/baremetal/deploy.yaml

# 等待就绪
kubectl wait --for=condition=available deployment/ingress-nginx-controller -n ingress-nginx --timeout=120s

# 验证
kubectl get pods -n ingress-nginx
# 预期: ingress-nginx-controller-xxx 状态为 Running

kubectl get svc -n ingress-nginx
# 预期: 看到 NodePort 类型的 Service，记下端口号
```

---

<!-- chunk: 方案对比 -->## 方案对比

| 特性 | k3s | kubeadm | MicroK8s |
|------|-----|---------|----------|
| **安装复杂度** | 极简 (一条命令) | 中等 (需多个步骤) | 简单 (snap) |
| **安装耗时** | ~2 分钟 | ~10 分钟 | ~5 分钟 |
| **资源占用** | 极低 (~512MB) | 较高 (~2GB) | 中等 (~1GB) |
| **K8s 兼容性** | 高 (CNCF 认证) | 完全标准 | 高 (CNCF 认证) |
| **内置组件** | Traefik, CoreDNS, 本地存储, metrics-server | 仅 CoreDNS + kube-proxy | 通过插件管理 |
| **CNI 网络** | 内置 Flannel | 需手动安装 | 内置 Calico |
| **升级方式** | 重装二进制 / 系统包升级 | `kubeadm upgrade` | `snap refresh` |
| **离线安装** | 支持 (air-gap 镜像包) | 较复杂 | 不支持 |
| **适用系统** | Linux (arm64 也支持) | Linux | Linux (Ubuntu 优先) |
| **多节点扩展** | 简单 (agent 加入) | 标准 (kubeadm join) | 简单 (microk8s add-node) |
| **生产就绪** | 边缘/小规模生产 | 所有规模生产 | 小规模/开发 |

**选择建议**:
- **快速上手、边缘/IoT** → k3s
- **标准化、准备过渡到生产** → kubeadm
- **Ubuntu 用户、喜欢插件化** → MicroK8s

---

<!-- chunk: 单节点性能调优 -->## 单节点性能调优

```bash
# ===== 1. 系统级优化 =====

# 增加文件描述符限制
cat <<EOF | sudo tee -a /etc/security/limits.conf
* soft nofile 65535
* hard nofile 65535
* soft nproc 65535
* hard nproc 65535
EOF

# 优化网络参数
cat <<EOF | sudo tee -a /etc/sysctl.d/k8s-performance.conf
# 连接跟踪表大小 (默认 65536 可能不够)
net.netfilter.nf_conntrack_max = 131072
# 允许更多 TIME_WAIT 连接复用
net.ipv4.tcp_tw_reuse = 1
# 增加本地端口范围
net.ipv4.ip_local_port_range = 1024 65535
# 增加 TCP 连接积压
net.core.somaxconn = 32768
EOF
sudo sysctl --system

# ===== 2. kubelet 优化 (kubeadm 方案) =====
# 编辑 kubelet 配置
# /var/lib/kubelet/config.yaml 或 /etc/kubernetes/kubelet.conf
# 增加:
#   maxPods: 110              # 单节点最大 Pod 数 (默认 110)
#   containerLogMaxSize: 50Mi # 容器日志单文件最大
#   containerLogMaxFiles: 5   # 容器日志最多保留文件数
#   imageGCHighThresholdPercent: 85  # 磁盘使用率超 85% 开始清理镜像
#   imageGCLowThresholdPercent: 80   # 清理到 80% 以下
```

---

<!-- chunk: 单节点备份策略 -->## 单节点备份策略

```bash
# ===== k3s 备份 =====
# k3s 使用内置 SQLite (单节点默认) 或 etcd

# SQLite 备份 (简单)
sudo cp /var/lib/rancher/k3s/server/db/state.db /backup/k3s-state-$(date +%Y%m%d).db

# 使用 k3s 内置命令创建 etcd 快照 (如果使用 etcd)
sudo k3s etcd-snapshot save --name manual-backup

# ===== kubeadm 备份 (etcd 快照) =====
ETCDCTL_API=3 sudo etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证快照
ETCDCTL_API=3 sudo etcdctl snapshot status /backup/etcd-$(date +%Y%m%d).db --write-out=table
# 预期: 显示快照的 hash、revision、total keys 等信息

# 建议: 设置 cron 定时备份
# sudo crontab -e
# 0 2 * * * /usr/local/bin/k3s etcd-snapshot save --name auto-backup-$(date +\%Y\%m\%d)
```

---

<!-- chunk: 验收清单 -->## 验收清单

- [ ] 系统准备完成 (swap关闭、内核模块加载、网络参数设置、时间同步)
- [ ] 集群成功部署，节点为 Ready 状态
- [ ] kubectl 可以正常连接和操作集群 (无需 sudo)
- [ ] kube-system 下所有 Pod 正常运行
- [ ] CNI 网络插件工作正常 (Pod 可跨节点通信)
- [ ] 可以成功部署和访问应用
- [ ] 存储类配置正常（PVC 可正常创建）
- [ ] metrics-server 工作正常 (`kubectl top nodes`)

---

<!-- chunk: 常见问题 (FAQ) -->## 常见问题 (FAQ)

## Q1: kubeadm init 报错 "container runtime is not running"

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# containerd 配置问题，检查并修复
sudo rm -f /etc/containerd/config.toml  # 删除可能有问题的配置
sudo systemctl restart containerd
# 或重新生成配置
containerd config default | sudo tee /etc/containerd/config.toml
sudo sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
sudo systemctl restart containerd
```

## Q2: 节点一直 NotReady

```bash
# 通常是 CNI 未安装或安装失败
kubectl get pods -A | grep -v Running  # 检查有没有非 Running 的 Pod
kubectl describe node | grep -A 5 Conditions  # 查看 NotReady 原因
sudo journalctl -u kubelet -n 50 --no-pager  # 查看 kubelet 日志
# 常见原因: CNI 未安装 → 安装 Flannel 或 Calico
```

## Q3: k3s 安装后 kubectl 报权限错误

```bash
# k3s 的 kubeconfig 默认只有 root 可读
sudo chmod 644 /etc/rancher/k3s/k3s.yaml
# 或复制到用户目录 (参考 A2 步骤)
```

## Q4: Pod 一直 Pending，提示 "1 node(s) had untolerated taint"

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
# kubeadm 单节点忘记移除 taint
kubectl taint nodes --all node-role.kubernetes.io/control-plane-
```

## Q5: 镜像拉取失败 (国内网络)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# k3s: 使用 /etc/rancher/k3s/registries.yaml 配置镜像代理
sudo tee /etc/rancher/k3s/registries.yaml << 'EOF'
mirrors:
  docker.io:
    endpoint:
      - "https://docker.mirrors.ustc.edu.cn"
  gcr.io:
    endpoint:
      - "https://gcr.mirrors.ustc.edu.cn"
EOF
sudo systemctl restart k3s

# kubeadm/containerd: 编辑 /etc/containerd/config.toml 添加 mirror
```

---

<!-- chunk: 清理/卸载 -->## 清理/卸载

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)

```bash
# ===== k3s 卸载 =====
/usr/local/bin/k3s-uninstall.sh
# 清理残留数据 (可选)
sudo rm -rf /var/lib/rancher /etc/rancher  # ⚠️ 删除系统/数据文件

# ===== kubeadm 卸载 =====
sudo kubeadm reset -f  # ⚠️ 清理节点所有 K8s 配置
sudo rm -rf /etc/kubernetes/ /var/lib/kubelet/ /var/lib/etcd/ ~/.kube/  # ⚠️ 删除系统/数据文件
# 清理 iptables 规则
sudo iptables -F && sudo iptables -t nat -F && sudo iptables -t mangle -F && sudo iptables -X
# 卸载软件包
# Ubuntu: sudo apt-get purge -y kubeadm kubelet kubectl containerd
# CentOS: sudo yum remove -y kubeadm kubelet kubectl containerd.io

# ===== MicroK8s 卸载 =====
sudo snap remove microk8s

# ===== 恢复系统设置 =====
# 重新启用 swap (如果需要)
# sudo swapon -a
# 重新启用防火墙 (如果需要)
# sudo systemctl start firewalld
```

---

**下一步**: 掌握单节点部署后，前往 → [03-development-environment-deployment.md](./03-development-environment-deployment.md) 学习多节点研发环境搭建。

---

**来源文档**: `domain-01-cluster-fundamentals/12-cluster-deployment-patterns.md`

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-deployment MOC
- [[domain-08-release-change-management/topic-deployment/README.md|Kubernetes 部署方案指南 (Deployment Guide)]]
- [[domain-08-release-change-management/topic-deployment/01-local-demo-deployment.md|01 - 本机单机 Demo 部署]]
- [[domain-08-release-change-management/topic-deployment/03-development-environment-deployment.md|03 - 研发环境部署 (Development Environment Deployment)]]
- [[domain-08-release-change-management/topic-deployment/04-production-environment-deployment.md|04 - 生产环境部署 (Production Environment Deployment)]]

## Related

- [[README|README]]
- [[MOC|MOC]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]

```