---
title: Kubespray 生产部署指南
summary: 解析基于 Ansible 的 Kubernetes 集群部署工具 Kubespray 的架构、部署流程、离线场景与生产实践。
category: 集群基础
tags:
- kubespray
- ansible
- cluster-lifecycle
- deployment
- air-gapped
- ha
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 平台架构师
- SRE
estimated_read_time: 22min
intent_queries:
- Kubespray 是什么
- Kubespray 与 kubeadm 区别
- 如何用 Ansible 部署 K8s
- 离线部署 Kubernetes
trigger_keywords:
- Kubespray
- Ansible
- 集群部署
- 离线
- inventory
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Kubespray 生产部署指南

> **文档类型**: 工具深度专题 | **适用版本**: Kubernetes v1.28–v1.33 / Kubespray release-2.24+ | **最后更新**: 2026-07
> **使用场景**: 在裸机、虚机、私有云上批量部署生产级 Kubernetes 集群，离线/气隙环境供给，多 master 高可用集群生命周期管理

---

## 目录

1. [概述](#1-概述)
2. [集群供给工具对比](#2-集群供给工具对比)
3. [Kubespray 架构](#3-kubespray-架构)
4. [部署模型与组件](#4-部署模型与组件)
5. [部署流程实战](#5-部署流程实战)
6. [离线与气隙部署](#6-离线与气隙部署)
7. [升级与扩缩容](#7-升级与扩缩容)
8. [与 GitOps / Cluster API 的关系](#8-与-gitops--cluster-api-的关系)
9. [生产实践](#9-生产实践)
10. [排障](#10-排障)
11. [相关文档](#11-相关文档)

---

## 1. 概述

### 1.1 Kubespray 是什么

**Kubespray** 是 Kubernetes SIG Cluster Lifecycle 维护的开源项目，它使用 **Ansible playbook** 在已有的裸机 (Bare Metal) 或虚机 (VM) 上部署**生产级**的 Kubernetes 集群。其底层调用 [[集群基础/控制平面/32-kubeadm-cluster-lifecycle.md|kubeadm]] 完成引导原语，但在 kubeadm 之上封装了一整套集群生命周期能力：inventory 管理、CNI 选择、高可用 (HA)、容器运行时、离线镜像、滚动升级、节点扩缩容与重置。

一句话定位：**声明式 Ansible 驱动的集群供给 (Declarative Ansible-driven cluster provisioning)**。

与单纯的手工 kubeadm 不同，Kubespray 解决的是"我有一批已装好 OS 的机器，如何可重复、可审计、可离线地把一个多 master 的生产集群拉起来"这个问题。它不负责创建机器本身（这是 Terraform/Cluster API 的职责），而是假定机器已就绪，专注于**机器之上到 Kubernetes 可用**这一段。

### 1.2 核心能力速览

| 能力域 | 说明 |
|--------|------|
| **批量部署** | 通过 Ansible 并发 SSH 到数十上百节点，统一拉起集群 |
| **高可用** | 多 master + etcd 集群 + 内/外部 LB（kube-vip / haproxy / nginx / MetalLB）|
| **CNI 可选** | Calico（默认）/ Flannel / Cilium / Canal / Kube-ovn / Macvlan / Weave |
| **容器运行时** | containerd（默认）/ CRI-O |
| **离线/气隙** | `download.yml` 预下载镜像与二进制到私有 registry，支持完全无外网部署 |
| **生命周期** | 部署 (`cluster.yml`)、扩容 (`scale.yml`)、升级 (`upgrade-cluster.yml`)、重置 (`reset.yml`)、删除节点 (`remove-node.yml`) |
| **可重复** | inventory + group_vars 用 git 管理，集群配置即代码 (CaC) |
| **跨发行版** | 支持 Ubuntu / Debian / RHEL / Rocky / Alma / CentOS / Fedora / openSUSE / Flatcar / Oracle Linux |

### 1.3 适用与不适用

**适用场景**：
- 私有云、本地数据中心、传统 IT 环境（VMware / OpenStack / 物理机）；
- 金融、政企等**气隙 (air-gapped)** 环境的合规部署；
- 需要稳定生命周期、低弹性需求的中大型集群；
- 希望"inventory 即真相"、用 git 审计变更的团队。

**不适用场景**：
- 云上托管 K8s（EKS/GKE/AKS/ACK）已有控制平面，无需 Kubespray；
- 需要频繁弹性扩缩容、按需起停——Ansible 是 push 模型，吞吐有限，更适合 Cluster API；
- 只想要最小学习成本玩单机——`minikube`/`kind`/`k3s` 更轻。

---

## 2. 集群供给工具对比

理解 Kubespray 的定位，必须把它放回"集群供给工具谱系"中比较。下面这张表是选型决策的核心依据。

### 2.1 核心对比表

| 工具 | 驱动机制 | 定位 | 典型适用 | 节点创建 |
|------|----------|------|----------|----------|
| **kubeadm** | 命令行 (CLI) | 最小集群引导（底层原语） | 学习、自定义脚本基础 | ❌ 不创建机器 |
| **Kubespray** | Ansible playbook | 裸金属/虚机批量部署，HA + CNI + 离线 + 升级 | 私有云、离线、传统 IT | ❌ 不创建机器 |
| **Cluster API (CAPI)** | K8s CRD + controller | 云原生声明式集群生命周期 | 多云、弹性、GitOps | ✅ 通过 provider 创建 |
| **RKE / RKE2** | Rancher CLI/Agent | Rancher 生态一键部署 | Rancher 用户 | ⚠️ 需配合节点供给 |
| **Terraform + kubeadm** | IaC + CLI | 基础设施编排 + kubeadm 引导 | 高度自定义 | ✅ Terraform 创建 |
| **kind / k3s / minikube** | 单机二进制 | 本地开发/边缘轻量 | 开发、测试、IoT | ✅ 本地容器/进程 |

### 2.2 关键差异维度

```
                    机器创建          OS 之上到 K8s         弹性/自愈           声明式 reconcile
                    ─────────        ─────────────         ─────────           ──────────────
Terraform           ✅ 强                                  ❌                  ✅ (IaC)
Cluster API         ✅ (provider)    ✅                    ✅ 持续 reconcile    ✅ (CRD)
Kubespray           ❌               ✅ 强                 ⚠️ 手动触发          ⚠️ (push)
kubeadm             ❌               ⚠️ 最小原语           ❌                  ❌
RKE2                ⚠️               ✅                    ⚠️                  ⚠️
```

**Kubespray 的位置**：它恰好处在 kubeadm（太底层，只管引导一个集群）和 Cluster API（需要管理集群、需要 infra provider、偏云原生）之间。当你的约束是"机器已经是裸机/虚机、不能或不想用 CAPI 的 controller 模型、需要一个能离线、能 HA、能用 Ansible 可审计地把集群拉起来"时，Kubespray 几乎是唯一成熟的开源选择。

### 2.3 选型决策树

```
要在哪里跑 K8s？
├── 公有云托管 (EKS/GKE/AKS) → 直接用托管服务，Kubespray 不适用
├── 公有云自建 (EC2 自管)     → Cluster API 或 Terraform+kubeadm 更优
└── 私有云 / 裸机 / 气隙
    ├── 需要频繁弹性、多集群、GitOps → Cluster API（私有 infra provider）
    ├── 一键部署、Rancher 生态       → RKE2
    └── 稳定生命周期、可离线、Ansible 团队熟悉 → ✅ Kubespray
```

> **提示**：这三类工具并非互斥。常见组合见 [第 8 节](#8-与-gitops--cluster-api-的关系)：Terraform 起机器 → Kubespray 部署 → 后续用 Kubespray/CAPI 管生命周期。

---

## 3. Kubespray 架构

### 3.1 总体架构

Kubespray 的核心是 **Ansible**：一台控制机 (Ansible controller) 通过 SSH 连接到所有目标节点，按 playbook 定义的角色 (roles) 顺序执行任务，最终在各节点上安装容器运行时、etcd、控制平面组件与工作节点组件。

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        Kubespray 总体架构                                       │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│   ┌──────────────────┐     SSH (22)     ┌─────────────────────────────────┐  │
│   │  Ansible 控制机   │ ───────────────▶│  目标节点集群 (inventory 定义)    │  │
│   │  (你的笔记本/跳板) │                 │                                  │  │
│   │                  │                  │  ┌─────────┐  ┌─────────┐        │  │
│   │  • kubespray repo│                  │  │ master1 │  │ master2 │  ...   │  │
│   │  • inventory/    │                  │  │ (etcd + │  │ (etcd + │        │  │
│   │    group_vars    │                  │  │  cp)    │  │  cp)    │        │  │
│   │  • Python3 venv  │                  │  └─────────┘  └─────────┘        │  │
│   │                  │                  │  ┌─────────┐  ┌─────────┐        │  │
│   │  Playbooks:      │                  │  │ worker1 │  │ worker2 │  ...   │  │
│   │  • cluster.yml   │                  │  └─────────┘  └─────────┘        │  │
│   │  • scale.yml     │                  │                                  │  │
│   │  • upgrade-      │                  │  组件 (由 roles 安装):            │  │
│   │    cluster.yml   │                  │  • containerd / CRI-O            │  │
│   │  • reset.yml     │                  │  • etcd (3/5/7 节点)             │  │
│   └──────────────────┘                  │  • kube-apiserver/controller/    │  │
│                                         │    scheduler (static pod)        │  │
│                                         │  • kubelet + kube-proxy          │  │
│                                         │  • CNI (Calico/Cilium/...)       │  │
│                                         │  • 核心插件 (CoreDNS等)          │  │
│                                         └─────────────────────────────────┘  │
│                                                                               │
│   高可用入口:                                                                  │
│   • 控制平面 VIP: kube-vip / haproxy / nginx / keepalived                      │
│   • LoadBalancer 服务: MetalLB / kube-vip (BGP/ARP)                           │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 三大组成：Playbook + Inventory + Roles

#### 3.2.1 Playbook（入口）

Kubespray 顶层提供几个核心 playbook，对应集群生命周期的不同阶段：

| Playbook | 用途 | 风险 |
|----------|------|------|
| `cluster.yml` | **初始部署**整个集群（最常用） | 🔴 实际变更节点 |
| `scale.yml` | 扩容 worker 节点 | 🟡 修改集群成员 |
| `upgrade-cluster.yml` | 滚动升级 K8s 版本 | 🔴 影响控制平面 |
| `reset.yml` | 重置集群（清理所有 K8s 组件） | 🔴 不可逆，清空节点 |
| `remove-node.yml` | 移除指定节点 | 🟡 缩容 |
| `recover-control-plane.yml` | 恢复失效控制平面 | 🔴 高危恢复 |
| `facts.yml` | 收集节点 facts（缓存加速） | 🟢 只读 |
| `download.yml` | 预下载镜像/二进制（离线场景） | 🟢 下载，不改集群 |

#### 3.2.2 Inventory（声明式配置）

inventory 描述"有哪些节点、它们扮演什么角色"。Kubespray 强制使用**分组 (groups)** 来映射 K8s 拓扑：

```
k8s_cluster          # 父组，含下面两个
├── kube_control_plane   # 控制平面节点（运行 apiserver/controller-manager/scheduler）
├── kube_node            # 工作节点（运行 kubelet 接受 Pod）
├── etcd                 # etcd 集群成员（可与 control_plane 共生或独立）
├── calico_rr / kube_ingress / ...   # 可选功能组
```

> **关键设计**：`etcd`、`kube_control_plane`、`kube_node` 三个组决定了节点的角色。一个节点可以同时属于 `etcd` + `kube_control_plane`（stacked etcd 模式），也可只属于 `etcd`（外部 etcd 模式）。

#### 3.2.3 Roles（执行单元）

Kubespray 把部署逻辑拆成大量 Ansible role，每个 role 负责一类组件。核心 roles 包括：

| Role | 职责 |
|------|------|
| `kubernetes/preinstall` | OS 预处理：内核模块、sysctl、swap、依赖包、用户 |
| `container-engine/containerd` | 安装配置 containerd（SystemdCgroup、sandbox image） |
| `download` | 下载 K8s 二进制、镜像、etcd、CNI 二进制 |
| `etcd` | 部署 etcd 集群（证书、member join） |
| `kubernetes/control-plane` | 首个 master `kubeadm init`、其余 master `kubeadm join --control-plane` |
| `kubernetes/node` | worker `kubeadm join`、kubelet 配置 |
| `kubernetes/client` | 生成 kubeconfig |
| `network_plugin/{calico,cilium,flannel,...}` | 部署 CNI |
| `kubespray-defaults` | 加载所有默认变量（fallback 值） |
| `kubernetes-apps/` | 部署 CoreDNS、kube-proxy、metrics-server 等集群内附加组件 |

### 3.3 执行流程（cluster.yml 内部）

```
cluster.yml 执行顺序（简化）:

1. kubespray-defaults        加载默认变量
2. facts                     收集所有节点 facts
3. bootstrap-os              安装 Python、设置主机名、时间同步
4. preinstall                内核模块、sysctl、关闭 swap、创建目录
5. container-engine          安装 containerd，配置 SystemdCgroup
6. download                  拉取二进制 + 容器镜像（离线场景跳过/用本地）
7. etcd                      生成 etcd 证书，逐台 join 成 etcd 集群
8. kubernetes/control-plane  ──┐
   ├── 第一个 master:          │  kubeadm init
   │   kubeadm init            │  + 上传证书 + 生成 join token
   ├── 其余 master:            │
   │   kubeadm join            │
   │   --control-plane         │
9. kubernetes/node          ──┘  worker: kubeadm join
10. network_plugin            部署 CNI（Calico/Cilium/...）
11. kubernetes-apps           CoreDNS、kube-proxy、metrics-server
12. postinstall               打印集群访问信息、生成 kubeconfig
```

### 3.4 支持矩阵（速查）

| 维度 | 支持项 |
|------|--------|
| **CNI** | Calico（默认）、Cilium、Flannel、Canal、Kube-ovn、Macvlan、Weave、Multus（meta-plugin） |
| **容器运行时** | containerd（默认）、CRI-O（docker 已弃用） |
| **etcd 模式** | Stacked（与 master 共生）、External（独立节点） |
| **LB** | 内部 haproxy/nginx（Kubespray 自管）、kube-vip、外部 LB（F5/硬件） |
| **网络插件模式** | BGP、IPIP、VXLAN、eBPF（Cilium） |
| **Service Mesh** | 可叠加 Istio/Linkerd（Kubespray 不强制） |

---

## 4. 部署模型与组件

### 4.1 Inventory 分组与拓扑

一个生产级 HA 集群的最小推荐拓扑是 **3 master（stacked etcd）+ N worker**。下面给出对应的 inventory 结构。

#### 4.1.1 三种 etcd 拓扑

```
模型 A: Stacked etcd（默认，推荐 ≤ 一定规模）
   master1[etcd+cp]   master2[etcd+cp]   master3[etcd+cp]   worker1..N
   优点：节点少、运维简单；缺点：etcd 与控制平面争抢资源

模型 B: External etcd（大规模、强隔离）
   etcd1   etcd2   etcd3          ← 独立机器，仅跑 etcd
   master1[cp]  master2[cp]  master3[cp]
   worker1..N
   优点：etcd 性能/故障隔离；缺点：多 3 台机器

模型 C: 单 master（仅测试，不要用于生产）
   master1[etcd+cp]   worker1..N
   ⚠️ 无 HA，apiserver/etcd 单点
```

#### 4.1.2 节点分组到 K8s 角色的映射

| Inventory 组 | 节点上的角色 | 安装的组件 |
|--------------|-------------|-----------|
| `etcd` | etcd 集群成员 | etcd 二进制 + 证书 |
| `kube_control_plane` | 控制平面 | apiserver/controller-manager/scheduler（static pod）|
| `kube_node` | 工作节点 | kubelet + kube-proxy |
| `k8s_cluster` | 父组 | （逻辑分组，无额外组件） |
| `calico_rr` | Calico 路由反射器（可选） | bird |

### 4.2 关键 group_vars

Kubespray 把配置项分层放在 `inventory/mycluster/group_vars/` 下，最常用的是 `all.yml`、`k8s_cluster.yml`、`etcd.yml`。下表列出最关键的几个：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `kube_version` | 随 release 固定 | 目标 K8s 版本，如 `v1.30.0` |
| `cluster_name` | `cluster.local` | 集群域名后缀 |
| `container_manager` | `containerd` | 容器运行时 |
| `kube_network_plugin` | `calico` | CNI 选择 |
| `kube_service_addresses` | `10.233.0.0/18` | Service CIDR |
| `kube_pods_subnet` | `10.233.64.0/18` | Pod CIDR |
| `kube_proxy_mode` | `iptables` | kube-proxy 模式（可改 `ipvs`） |
| `loadbalancer_apiserver` | （外部 LB 地址） | 控制 plane LB 的 VIP/地址 |
| `kube_image_repo` | `registry.k8s.io` | 镜像仓库（离线时改私有） |
| `pod_infra_image_repo` | `registry.k8s.io/pause` | pause 镜像仓库 |
| `download_run_once` | `false` | 是否在控制机集中下载后分发（离线/加速） |
| `etcd_deployment_type` | `host`（二进制） | etcd 部署形态（`host`/`docker`） |
| `kube_vip_address` | （可选） | kube-vip 控制平面 VIP |

### 4.3 组件对应 role 速查

```
kube_apiserver        ← kubernetes/control-plane (kubeadm 生成 static pod)
kube_controller_mgr   ← kubernetes/control-plane
kube_scheduler        ← kubernetes/control-plane
kubelet               ← kubernetes/node + kubernetes/control-plane
kube_proxy            ← kubernetes-apps/ansible (DaemonSet)
etcd                  ← etcd role
containerd            ← container-engine/containerd
CoreDNS               ← kubernetes-apps/ansible
metrics-server        ← kubernetes-apps/metrics_server
Calico/Cilium/...     ← network_plugin/<name>
```

---

## 5. 部署流程实战

本节给出从零到可用的完整 runbook。以 **3 master（stacked etcd）+ 2 worker** 为例。

### 5.1 步骤 1：节点 OS 准备（每台目标机器）

在每台目标节点上完成以下准备（可脚本化批量执行）。这些是 Kubespray 的前置假设。

**要求**：
- OS：Ubuntu 20.04/22.04、RHEL/Rocky/Alma 8/9、CentOS 7 等；
- Python 3.8+（Kubespray 在节点上执行 module 需要）；
- 控制机到所有节点的 SSH 免密；
- 节点上的用户具备免密 sudo（或在 inventory 配 `ansible_become_password`）；
- 节点间网络互通，防火墙放行 K8s 端口（6443/2379-2380/10250 等）。

节点预配置脚本（Ubuntu 示例）：

> **🟡 中风险** — 修改系统参数（swap、内核模块、sysctl），执行前确认节点不在承载其他业务。

```bash
# 🟡 中风险：关闭 swap、加载内核模块、设置 sysctl（影响节点运行环境）
#!/bin/bash
set -euo pipefail

# 1. 关闭 swap（kubelet 要求）
swapoff -a
sed -i '/swap/d' /etc/fstab

# 2. 加载内核模块
cat > /etc/modules-load.d/k8s.conf <<'EOF'
overlay
br_netfilter
EOF
modprobe overlay && modprobe br_netfilter

# 3. 内核参数
cat > /etc/sysctl.d/k8s.conf <<'EOF'
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF
sysctl --system >/dev/null

# 4. 确保 Python3（Ansible 在节点上需要）
apt-get update -qq && apt-get install -y -qq python3

echo "Node ready for Kubespray."
```

> **🟢 低风险：建立控制机到节点的 SSH 互信**（在控制机执行）

```bash
# 🟢 低风险：仅配置 SSH key，不改变节点服务
ssh-keygen -t ed25519 -N "" -f ~/.ssh/kubespray_id
for h in master1 master2 master3 worker1 worker2; do
  ssh-copy-id -i ~/.ssh/kubespray_id.pub deploy@$h
done
```

### 5.2 步骤 2：获取 Kubespray（控制机）

> **🟢 低风险：仅在控制机克隆代码、安装 Python 依赖，不触碰目标节点**

```bash
# 🟢 低风险：克隆仓库
git clone https://github.com/kubernetes-sigs/kubespray.git
cd kubespray

# 🟢 低风险：在 venv 中安装 Ansible 与依赖（避免污染系统 Python）
python3 -m venv venv
source venv/bin/activate
pip install -U pip
pip install -r requirements.txt
```

`requirements.txt` 锁定了兼容的 Ansible 版本（通常为 Ansible-Core + 一组 collection）。**不要随意升级 Ansible**，Kubespray 对 Ansible 版本有严格要求。

### 5.3 步骤 3：配置 inventory（控制机）

> **🟡 中风险：inventory 决定集群拓扑与配置，错误配置会导致部署失败或拓扑错误**

```bash
# 🟡 中风险：创建自定义 inventory（基于官方 sample）
cp -rfp inventory/sample inventory/mycluster
```

#### 5.3.1 编辑 `inventory/mycluster/hosts.yaml`

以 3 master（stacked etcd）+ 2 worker 为例：

```yaml
# inventory/mycluster/hosts.yaml
all:
  hosts:
    master1:
      ansible_host: 10.0.0.11
      ip: 10.0.0.11
      access_ip: 10.0.0.11
    master2:
      ansible_host: 10.0.0.12
      ip: 10.0.0.12
      access_ip: 10.0.0.12
    master3:
      ansible_host: 10.0.0.13
      ip: 10.0.0.13
      access_ip: 10.0.0.13
    worker1:
      ansible_host: 10.0.0.21
      ip: 10.0.0.21
      access_ip: 10.0.0.21
    worker2:
      ansible_host: 10.0.0.22
      ip: 10.0.0.22
      access_ip: 10.0.0.22
  children:
    kube_control_plane:
      hosts:
        master1:
        master2:
        master3:
    kube_node:
      hosts:
        worker1:
        worker2:
    etcd:
      hosts:
        master1:      # stacked etcd：与 control_plane 共生
        master2:
        master3:
    k8s_cluster:
      children:
        kube_control_plane:
        kube_node:
  vars:
    ansible_user: deploy
    ansible_ssh_private_key_file: ~/.ssh/kubespray_id
    ansible_become: true          # 用 sudo 执行特权任务
```

#### 5.3.2 编辑 `group_vars/all/all.yml`（节选关键项）

```yaml
# inventory/mycluster/group_vars/all/all.yml
# --- 集群基本配置 ---
cluster_name: prod.k8s.local

# --- 网络 CIDR ---
kube_service_addresses: 10.233.0.0/18   # Service CIDR
kube_pods_subnet: 10.233.64.0/18        # Pod CIDR
kube_network_node_prefix: 24

# --- kube-proxy ---
kube_proxy_mode: ipvs                    # 生产推荐 ipvs

# --- 下载策略（非离线场景）---
download_run_once: true                  # 控制机下载一次后分发，加速
download_force_cache: true
```

#### 5.3.3 编辑 `group_vars/k8s_cluster/k8s-cluster.yml`（节选关键项）

```yaml
# inventory/mycluster/group_vars/k8s_cluster/k8s-cluster.yml
kube_version: v1.30.0
container_manager: containerd
kube_network_plugin: calico              # CNI

# --- 控制平面高可用（kube-vip 提供 VIP）---
kube_vip_address: 10.0.0.10
loadbalancer_apiserver:
  address: 10.0.0.10
  port: 6443

# --- 关闭不需要的特性，强化默认安全 ---
kube_encrypt_secret_data: true           # etcd 静态加密 Secret
```

### 5.4 步骤 4：校验连通性

> **🟢 低风险：仅 ping，不改任何状态**

```bash
# 🟢 低风险：测试 Ansible 到所有节点的连通性
ansible all -i inventory/mycluster/hosts.yaml -m ping
```

期望所有节点返回 `SUCCESS`。若有 `UNREACHABLE`，检查 SSH key、`ansible_user`、防火墙。

### 5.5 步骤 5：执行部署

> **🔴 高风险：实际变更所有目标节点，部署完整集群。执行前确认：已校验 inventory；已在非生产环境演练；已规划变更窗口与回滚（reset.yml）。**

```bash
# 🔴 高风险：部署整个集群（耗时 15-45 分钟，取决于节点数与网络）
ansible-playbook -i inventory/mycluster/hosts.yaml cluster.yml -b
```

常用执行参数：
- `-b` / `--become`：用 sudo 执行特权任务；
- `-e "@offline.yaml"`：覆盖变量（离线场景见第 6 节）；
- `--limit master1,worker1`：只跑部分节点（调试）；
- `--tags download` / `--skip-tags download`：按 tag 缩小范围；
- `-v` / `-vvv`：增加日志详细度（排障见第 10 节）。

执行过程中 Ansible 会逐 role 推进。若中途某 task 失败，修复后**重跑同一个 `cluster.yml`** 即可——Kubespray 大部分任务是幂等的。

### 5.6 步骤 6：验证集群

> **🟢 低风险：只读命令**

部署完成后，kubeconfig 会被生成在控制机与首个 master。

```bash
# 🟢 低风险：从首个 master 拷贝 kubeconfig 到控制机
ssh deploy@master1 "sudo cat /etc/kubernetes/admin.conf" > ~/.kube/prod.k8s.local.conf
export KUBECONFIG=~/.kube/prod.k8s.local.conf

# 🟢 低风险：检查节点状态
kubectl get nodes -o wide

# 🟢 低风险：检查核心组件 Pod
kubectl get pods -n kube-system

# 🟢 低风险：验证 etcd 健康度（任一 master）
ssh deploy@master1 "sudo ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/ssl/etcd/ssl/ca.pem \
  --cert=/etc/ssl/etcd/ssl/node-master1.pem \
  --key=/etc/ssl/etcd/ssl/node-master1-key.pem \
  endpoint health --cluster"
```

期望输出：5 个节点均为 `Ready`，`kube-system` 下 CoreDNS、kube-proxy、Calico/Cilium、metrics-server 全部 `Running`，etcd 集群所有 endpoint `healthy`。

---

## 6. 离线与气隙部署

离线 (air-gapped) 部署是 Kubespray 相对其他工具的**显著强项**，也是金融、政企场景的刚需。

### 6.1 离线部署原理

Kubespray 在联网环境下默认从 `registry.k8s.io`、`quay.io`、`docker.io` 等公网仓库拉取镜像与二进制。离线场景下，思路是：**把所有依赖预先下载到一个内部可达的私有 registry / 本地文件服务器，再让 Kubespray 指向它。**

```
联网区                                       气隙区
─────────                                   ─────────
download.yml ──▶ 私有 registry          Kubespray ──▶ 从私有 registry 拉取
(在临时联网节点)   (harbor / nexus)         (控制机)     镜像 + 二进制
                  持有全部镜像
```

### 6.2 离线部署步骤

#### 6.2.1 步骤 1：在联网区预下载

> **🟢 低风险：只下载，不部署**

```bash
# 🟢 低风险：在一台联网机器上执行 download.yml，把镜像/二进制拉到本地缓存或推到私有 registry
# 方式 A：download_run_once + 本地缓存目录，再 rsync 进气隙区
ansible-playbook -i inventory/mycluster/hosts.yaml download.yml \
  -e download_run_once=true \
  -e download_localhost=true

# 然后把 roles/download/files/ 缓存目录 rsync 进气隙区
```

#### 6.2.2 步骤 2：把镜像推入气隙区私有 registry

把缓存的镜像 tar 包导入 harbor/nexus 等私有 registry，确保气隙区节点可访问（DNS、证书、防火墙）。

#### 6.2.3 步骤 3：配置 `group_vars` 指向私有 registry

```yaml
# inventory/mycluster/group_vars/all/all.yml（离线关键项）
# 所有镜像仓库指向私有 registry
kube_image_repo: "harbor.internal/library"
pod_infra_image_repo: "harbor.internal/library/pause"
registry.k8s.io: "harbor.internal/library"
quay.io: "harbor.internal/quay"
docker.io: "harbor.internal/docker.io"

# 不再从公网下载二进制，用本地文件
download_run_once: true
download_force_cache: true
download_keep_remote_cache: true
```

或用更简洁的 `offline.yaml` 变量文件统一覆盖：

```bash
# 🔴 高风险：在气隙区部署（变更节点）
ansible-playbook -i inventory/mycluster/hosts.yaml cluster.yml -b \
  --extra-vars "@offline.yaml"
```

### 6.3 离线场景的注意事项

| 事项 | 说明 |
|------|------|
| **私有 registry 证书** | 节点需信任 registry CA；自签证书要分发到 `/etc/docker/certs.d/` 与 containerd 配置 |
| **认证** | 私有 registry 需登录时，配置 `registry_auths` |
| **二进制来源** | K8s、etcd、CNI、crictl 等二进制也要离线，用缓存目录分发 |
| **OS 包** | containerd/python 等系统包离线时需自建本地 apt/yum 镜像 |
| **验证** | 部署前在气隙区用 `ansible all -m ping` + `crictl pull <私有镜像>` 验证可达性 |

---

## 7. 升级与扩缩容

### 7.1 集群升级

Kubespray 的 `upgrade-cluster.yml` 会按"先 etcd → 再逐个 master → 再逐个 worker"的顺序滚动升级，并自动 `drain`/`uncordon` 节点。升级路径建议遵循 **一次升一个小版本 (skew ≤ 1)** 的原则，详见 [[集群基础/控制平面/35-cluster-upgrade-runbook.md|集群升级 Runbook]]。

> **🔴 高风险：升级影响控制平面可用性，必须先备份 etcd、确认版本兼容性、规划变更窗口。**

```bash
# 🟢 低风险：升级前备份 etcd（任一 master，务必先做）
ssh deploy@master1 "sudo ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%F).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/ssl/etcd/ssl/ca.pem \
  --cert=/etc/ssl/etcd/ssl/node-master1.pem \
  --key=/etc/ssl/etcd/ssl/node-master1-key.pem"

# 🟡 中风险：dry-run 预览（部分模块支持 --check）
ansible-playbook -i inventory/mycluster/hosts.yaml upgrade-cluster.yml -b \
  -e kube_version=v1.31.0 --check

# 🔴 高风险：实际滚动升级到 v1.31.0
ansible-playbook -i inventory/mycluster/hosts.yaml upgrade-cluster.yml -b \
  -e kube_version=v1.31.0
```

升级完成后验证：
- `kubectl get nodes` 所有节点版本一致；
- `kubectl get pods -n kube-system` 组件正常；
- CNI 版本与 K8s 版本兼容。

### 7.2 扩容 worker 节点

> **🟡 中风险：新增节点到集群，需确保新节点 OS/网络已就绪**

```bash
# 🟡 中风险：编辑 inventory 加入新节点（hosts.yaml 的 kube_node 组追加 newworker1）
# 然后：
ansible-playbook -i inventory/mycluster/hosts.yaml scale.yml -b \
  --limit newworker1
```

`scale.yml` 只针对新节点执行 join 流程，不影响现有节点。

### 7.3 缩容 / 移除节点

> **🟡 中风险：移除节点会驱逐其上 Pod，需确认有足够容量承接**

```bash
# 🟡 中风险：从集群移除节点（自动 drain + delete node + 清理组件）
ansible-playbook -i inventory/mycluster/hosts.yaml remove-node.yml -b \
  -e node=worker2
```

移除后记得同步更新 inventory 的 `hosts.yaml`，保持声明式一致。

### 7.4 完全重置（慎用）

> **🔴 高风险：reset.yml 会卸载所有 K8s 组件、清理 etcd 数据，不可逆。仅用于彻底重建。**

```bash
# 🔴 高风险：重置整个集群（销毁性）
ansible-playbook -i inventory/mycluster/hosts.yaml reset.yml -b
```

---

## 8. 与 GitOps / Cluster API 的关系

### 8.1 Push vs Pull/Reconcile

Kubespray 是典型的 **push 模型**：Ansible 控制机主动 SSH 推送到节点，跑完 playbook 即结束，没有持续调和 (reconcile)。这与 Cluster API 的 **pull/reconcile 模型**形成对比。

| 维度 | Kubespray (push) | Cluster API (reconcile) |
|------|------------------|-------------------------|
| 触发 | 人工/CI 跑 playbook | controller 持续 watch CR |
| 状态维持 | 跑完即止，不自动纠偏 | 自动向期望状态收敛 |
| 速度 | 受 SSH 并发限制 | 控制循环，弹性更快 |
| 管理集群 | 不需要 | 需要一个 management cluster |
| 适合 | 稳定生命周期、低弹性 | 频繁弹性、多集群、GitOps |

### 8.2 常见组合架构

这三类工具并非互斥，实际生产中常组合使用：

**组合 1：Terraform → Kubespray（经典私有云）**
```
Terraform 创建 VM/裸机 → 输出 inventory → Kubespray 部署 K8s → 后续用 Kubespray 管生命周期
```

**组合 2：Kubespray 建 management cluster → CAPI 管下游（混合）**
```
Kubespray 部署一个稳定的 management cluster
  → 在其上安装 CAPI + infra/cluster providers
  → CAPI 管理下游多个工作负载集群（弹性、GitOps 友好）
```
这种模式让"管理面"稳（Kubespray 一次性建好、低频维护），"工作面"弹（CAPI 持续 reconcile）。详见 [[平台工程/运维/14-cluster-api-deep-dive.md|Cluster API 深度解析]]。

**组合 3：Kubespray + ArgoCD（应用层 GitOps）**
```
Kubespray 负责集群供给（底层）
ArgoCD/Flux 负责应用交付（上层 GitOps）
两者分层，职责清晰
```

### 8.3 什么时候不该用 Kubespray

- 需要"节点宕机自动重建"——这是 CAPI 的 controller 职责，Kubespray 不会自愈节点；
- 需要秒级弹性扩缩——Ansible SSH 速度是瓶颈；
- 全云原生栈、希望一切都用 CRD 表达——CAPI 更契合。

---

## 9. 生产实践

### 9.1 拓扑与规模建议

| 集群规模 | 推荐拓扑 | etcd 模式 | 备注 |
|----------|----------|-----------|------|
| 小（≤ 20 节点） | 3 master stacked | Stacked | 资源够用，运维简单 |
| 中（20–100） | 3 master + N worker | Stacked | master 给足 CPU/内存 |
| 大（100–500） | 3–5 master + 外部 etcd | External | etcd 独立 3–5 节点 |
| 超大（> 500） | 多区域多集群 | External + 联邦 | 考虑拆分集群而非单点扩 |

etcd 节点数必须是**奇数**（3/5/7），保证多数派。

### 9.2 负载均衡选型

控制平面 LB 是 HA 的关键。推荐：

- **kube-vip**（首选）：以 static pod / DaemonSet 提供 control plane VIP，轻量、无外部依赖，ARP 或 BGP 模式。生产中常与 MetalLB 配合（kube-vip 管 apiserver VIP，MetalLB 管 Service LoadBalancer）；
- **MetalLB**：为 Service 类型 LoadBalancer 提供虚 IP（BGP/ARP），适合裸金属环境暴露 Ingress；
- **外部硬件 LB**（F5/A10）：大型企业已有投资时，Kubespray 通过 `loadbalancer_apiserver` 指向外部 VIP；
- **Kubespray 内置 haproxy/nginx**：在指定节点跑 haproxy 做 4 层代理，适合无外部 LB 的小规模场景。

### 9.3 inventory 即代码（CaC）

把整个 `inventory/mycluster/` 目录纳入 git 管理，每一次拓扑/配置变更都走 PR 评审。这样：
- 集群配置**可审计**（谁改了什么、为什么改）；
- **可回滚**（出问题 revert PR 后重跑 playbook）；
- **可复制**（建新集群直接基于 git 中的 inventory fork）。

配合 CI（GitLab CI/GitHub Actions）可在 PR 上自动跑 `ansible-playbook --check` 与 lint。

### 9.4 变更安全清单

每次部署/升级前过一遍：

- [ ] etcd 已做 snapshot 备份；
- [ ] inventory 已 review，拓扑正确（etcd 奇数节点）；
- [ ] kube_version 与 CNI 版本兼容性已核对；
- [ ] 先在 staging 集群演练；
- [ ] 选择变更窗口，提前 drain 影响评估；
- [ ] playbook 输出全程记录（`tee deploy.log`），便于事后排障；
- [ ] 准备好 `reset.yml` / etcd 恢复作为兜底回滚。

### 9.5 性能与执行优化

> **🟢 低风险：dry-run 预演（部分模块支持）**

```bash
# 🟢 低风险：dry-run，部分 Ansible 模块支持 --check（不实际变更）
ansible-playbook -i inventory/mycluster/hosts.yaml cluster.yml -b --check
```

生产执行优化技巧：
- `forks=50`（`ansible.cfg` 或 `-f 50`）：提高 SSH 并发，大集群明显加速；
- `--flush-cache` 之间合理用 `facts.yml` 缓存：避免每次重复采集；
- `download_run_once: true`：控制机下载一次镜像后用 `kubeadm config images push` 或 `ctr` 分发，避免每节点都从公网拉；
- 分批部署：先 `--limit` 部署 master，再扩 worker，便于分阶段验证。

---

## 10. 排障

### 10.1 通用排障手段

> **🟢 低风险：详细日志与连通性诊断**

```bash
# 🟢 低风险：最详细输出（-vvv），定位失败 task
ansible-playbook -i inventory/mycluster/hosts.yaml cluster.yml -b -vvv

# 🟢 低风险：指定从某个 task 重跑（调试用，配合 --start-at-task）
ansible-playbook -i inventory/mycluster/hosts.yaml cluster.yml -b \
  --start-at-task="Install kubelet"

# 🟢 低风险：检查 SSH 与 sudo 是否正常（最常见的前置故障）
ansible all -i inventory/mycluster/hosts.yaml -m ping -vvv
ansible all -i inventory/mycluster/hosts.yaml -m command -a "sudo -n true"
```

### 10.2 常见问题与对策

| 现象 | 可能原因 | 对策 |
|------|----------|------|
| `UNREACHABLE` | SSH key/用户/防火墙 | 核对 `ansible_user`、key、节点 22 端口 |
| `Permission denied (sudo)` | 免密 sudo 未配 | 配 `/etc/sudoers.d/deploy` 免密或设 `ansible_become_password` |
| 下载镜像超时 | 公网慢/被墙 | 配代理 `http_proxy/https_proxy` 或改 `kube_image_repo` 指向国内镜像 |
| `br_netfilter` not loaded | 内核模块未加载 | 跑 preinstall 或手动 `modprobe br_netfilter overlay` |
| CNI Pod `CrashLoopBackOff` | CIDR 冲突 / IPAM 耗尽 | 核对 `kube_pods_subnet` 与节点 `pod_subnet`，检查 IPAM 池 |
| etcd member join 失败 | 证书/时钟不同步 | 确保所有节点 NTP 同步、etcd 证书 SAN 正确 |
| `kubeadm init` 卡住 | 端口被占 / 容器运行时未就绪 | `crictl ps`、检查 6443/10250 占用 |
| 升级后 node `NotReady` | kubelet 版本不匹配 | 确认该节点 kubelet 已被 role 更新，`systemctl restart kubelet` |
| 静态 Pod 不更新 | manifest 缓存 | 检查 `/etc/kubernetes/manifests/`，必要时等 kubelet 重新拉起 |

### 10.3 节点级诊断

> **🟢 低风险：在故障节点上做只读诊断**

```bash
# 🟢 低风险：内核模块
lsmod | grep -E "br_netfilter|overlay"

# 🟢 低风险：sysctl
sysctl net.bridge.bridge-nf-call-iptables net.ipv4.ip_forward

# 🟢 低风险：容器运行时
sudo crictl version
sudo crictl ps -a | head

# 🟢 低风险：kubelet 状态与日志
sudo systemctl status kubelet
sudo journalctl -u kubelet --since "30 min ago" --no-pager | tail -50

# 🟢 低风险：节点端口占用
sudo ss -tlnp | grep -E "6443|2379|2380|10250"
```

### 10.4 失败重跑策略

Kubespray 大部分 task 幂等，失败修复后直接重跑同一 playbook 即可。少数需注意：
- etcd 部分失败：若已生成部分 member，重跑前可能需手动 `etcdctl member remove` 残留 member；
- `kubeadm init` 部分成功后失败：必要时 `kubeadm reset` 后重跑（🔴 高风险）；
- CNI 部署失败：可单独 `--tags network` 重跑 CNI role。

---

## 11. 相关文档

- [[集群基础/控制平面/32-kubeadm-cluster-lifecycle.md|kubeadm 集群生命周期]] —— Kubespray 底层调用的引导原语，理解 kubeadm 有助于理解 Kubespray 各阶段。
- [[平台工程/运维/14-cluster-api-deep-dive.md|Cluster API 深度解析]] —— 对照理解声明式 reconcile 模型，以及与 Kubespray 的组合架构。
- [[集群基础/99-kubernetes-production-architecture-blueprint.md|K8s 生产架构蓝图]] —— 生产级集群的整体架构参考，Kubespray 是其落地工具之一。
- [[集群基础/控制平面/35-cluster-upgrade-runbook.md|集群升级 Runbook]] —— 升级流程的完整手册，Kubespray `upgrade-cluster.yml` 是其自动化实现。
- [[集群基础/控制平面/03-plane-high-availability.md|控制平面高可用]] —— HA 原理与 LB 选型，Kubespray 的多 master + kube-vip 即据此落地。

<!-- risk-assessed -->
