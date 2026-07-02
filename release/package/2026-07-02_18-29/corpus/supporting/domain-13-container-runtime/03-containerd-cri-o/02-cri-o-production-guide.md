---
title: CRI-O 生产指南
description: 面向阿里云专有云及 RHEL/CentOS 节点的 CRI-O 安装、配置、Pod 生命周期管理、镜像签名验证及与 containerd
  的选型对比。
summary: 面向阿里云专有云及 RHEL/CentOS 节点的 CRI-O 安装、配置、Pod 生命周期管理、镜像签名验证及与 containerd 的选型对比。
category: container-runtime
tags:
- cri-o
- cri
- podman
- redhat
- container-runtime
- kubernetes
- dedicated-cloud
- runtime-handler
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
audience:
- SRE
- 容器平台工程师
- Red Hat 系专有云运维
estimated_read_time: 19min
intent_queries:
- CRI-O 生产环境安装配置
- CRI-O 与 containerd 区别
- CRI-O 镜像签名验证怎么配
trigger_keywords:
- cri-o
- crio
- podman
- registries.conf
- runtime handler
- skopeo
prerequisites:
- domain-13-container-runtime/03-containerd-cri-o/01-containerd-production-operations.md
- domain-13-container-runtime/01-containerd-deep-guide.md
- domain-02-workloads-applications/00-core-workloads/15-container-runtime-interfaces.md
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRI-O 生产指南

> 适用场景：基于 RHEL/CentOS/Alibaba Cloud Linux 的阿里云专有云节点、OpenShift 兼容环境，以及对 OCI 合规、镜像签名有强需求的 Kubernetes 集群。CRI-O 是一个专门为 Kubernetes 设计的轻量级 CRI 运行时。

## 目录

- [1. 背景与定位](#1-背景与定位)
- [2. 安装与版本选择](#2-安装与版本选择)
- [3. 核心配置文件](#3-核心配置文件)
- [4. Pod 生命周期管理](#4-pod-生命周期管理)
- [5. 镜像管理与签名验证](#5-镜像管理与签名验证)
- [6. Runtime Handler 配置](#6-runtime-handler-配置)
- [7. 监控与日志](#7-监控与日志)
- [8. 常见故障排查](#8-常见故障排查)
- [9. CRI-O 与 containerd 对比](#9-cri-o-与-containerd-对比)
- [10. 阿里云专有云 RHEL 节点部署注意事项](#10-阿里云专有云-rhel-节点部署注意事项)
- [11. 与 Podman 协同管理](#11-与-podman-协同管理)
- [12. 生产检查清单](#12-生产检查清单)
- [13. 相关文档](#13-相关文档)
## 1. 背景与定位

CRI-O 由 Red Hat 主导开发，目标是提供一个 "just enough" 的 CRI 实现：只包含 Kubernetes 运行容器所需的最小功能集合，不包含镜像构建、本地 CLI 容器运行等附加能力。它的镜像存储、网络配置与 Podman/Skopeo 共享 libpod 生态，因而在 Red Hat Enterprise Linux 与 Alibaba Cloud Linux 3 上有较好的兼容性。

在阿里云专有云场景中，CRI-O 常用于以下情况：

- 客户要求使用 Red Hat 认证的运行时栈；
- 需要容器镜像签名验证（sigstore/simple signing）以满足合规；
- 节点操作系统为 RHEL 8/9，希望与 Podman 工具链保持一致。

## 2. 安装与版本选择

CRI-O 版本与 Kubernetes 版本存在强绑定关系。CRI-O 1.28 对应 Kubernetes 1.28，依此类推。安装前请确认目标仓库提供的小版本与 ACK/ASO 节点镜像一致。

### 2.1 在 RHEL/Alibaba Cloud Linux 上安装

以下命令演示在 Alibaba Cloud Linux 3/RHEL 8 上安装 CRI-O 1.30，并锁定版本防止运行时漂移。

```bash
# 启用 CRI-O 1.30 仓库并安装，版本号需与 Kubernetes 大版本一致
sudo curl -L -o /etc/yum.repos.d/devel:kubic:libcontainers:stable.repo \
  https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/CentOS_8/devel:kubic:libcontainers:stable.repo
sudo curl -L -o /etc/yum.repos.d/devel:kubic:libcontainers:stable:cri-o:1.30.repo \
  https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable:/cri-o:/1.30/CentOS_8/devel:kubic:libcontainers:stable:cri-o:1.30.repo
sudo yum install -y cri-o-1.30.*
```

安装完成后，启用 systemd 服务并设置为开机自启。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启动并启用 crio 服务，检查其是否能正常监听 CRI socket
sudo systemctl enable --now crio
sudo systemctl status crio --no-pager
ls -l /var/run/crio/crio.sock
```
### 2.2 配置 kubelet 使用 CRI-O

在 ACK/ASO 节点池中切换运行时后，kubelet 需要指向 CRI-O 的 socket。若手工配置单节点，需要修改 kubelet 启动参数。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 将 kubelet 的 CRI endpoint 指向 CRI-O socket
KUBELET_EXTRA_ARGS="--container-runtime=remote --container-runtime-endpoint=unix:///var/run/crio/crio.sock"
echo "KUBELET_EXTRA_ARGS=${KUBELET_EXTRA_ARGS}" | sudo tee /etc/sysconfig/kubelet
sudo systemctl restart kubelet
```
## 3. 核心配置文件

CRI-O 的配置分散在多个文件中，理解每个文件的作用有助于快速定位问题：

| 配置文件 | 作用 |
| --- | --- |
| `/etc/crio/crio.conf` | CRI-O 主配置：runtime、pause image、log、metrics 等 |
| `/etc/crio/crio.conf.d/*.conf` | 按场景覆盖主配置，生产推荐优先使用 drop-in 文件 |
| `/etc/containers/registries.conf` | 镜像仓库、mirror、insecure registry 配置 |
| `/etc/containers/policy.json` | 镜像签名/加密策略 |
| `/etc/containers/storage.conf` | 镜像与容器存储路径、驱动、配额 |

### 3.1 主配置示例

使用 drop-in 文件覆盖 pause 镜像、cgroup manager 与 metrics 端口，避免升级主配置文件时丢失自定义配置。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建 drop-in 配置，覆盖 pause 镜像与 cgroup 管理器
sudo tee /etc/crio/crio.conf.d/99-ack-custom.conf <<'EOF'
[crio.image]
pause_image = "registry-vpc.cn-hangzhou.aliyuncs.com/acs/pause:3.9"

[crio.runtime]
cgroup_manager = "systemd"
default_runtime = "runc"
log_size_max = 134217728

[crio.metrics]
enable_metrics = true
metrics_port = 9537
EOF
sudo systemctl restart crio
```
### 3.2 配置镜像仓库与 mirror

CRI-O 使用 `/etc/containers/registries.conf` 管理仓库。下面的配置为 Docker Hub 配置阿里云加速器，并将 `registry.example.com` 标记为 insecure（仅用于测试环境，生产应使用 TLS）。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 配置 Docker Hub 镜像加速与私有 insecure 仓库
sudo tee /etc/containers/registries.conf <<'EOF'
unqualified-search-registries = ["docker.io"]

[[registry]]
prefix = "docker.io"
location = "<your-id>.mirror.aliyuncs.com"
insecure = false

[[registry]]
prefix = "registry.example.com"
location = "registry.example.com"
insecure = true
EOF
sudo systemctl reload crio
```
## 4. Pod 生命周期管理

CRI-O 完全通过 CRI 与 kubelet 交互，日常运维主要使用 `crictl` 或 `oc`。下面的命令展示如何在节点上查看 Pod、容器、日志与事件。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CRI-O 节点上的 Pod、容器与日志，定位 Pod 状态异常
sudo crictl pods
sudo crictl ps -a
sudo crictl logs <container-id>
sudo crictl inspectp <pod-id>
```
若需要手动创建沙箱或容器（例如做网络连通性测试），可以使用 CRI 原语。注意：这些操作不会受 Kubernetes 管理，仅用于排障。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 手动创建 Pod 沙箱，生成 sandbox-id 供后续创建容器使用
POD_ID=$(sudo crictl runp /tmp/pod-config.json)
echo "Pod sandbox id: ${POD_ID}"
```
沙箱配置 `/tmp/pod-config.json` 示例：

```json
{
  "metadata": {
    "name": "debug-sandbox",
    "namespace": "default",
    "uid": "debug-001"
  },
  "linux": {}
}
```

## 5. 镜像管理与签名验证

### 5.1 本地镜像存储

CRI-O 的镜像默认存储在 `/var/lib/containers`，与 Podman 共享 storage。可以使用 `crictl images` 或 `podman images` 查看。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CRI-O/Podman 本地镜像列表与占用
sudo crictl images
sudo podman images --storage-driver overlay
```
### 5.2 镜像签名策略

生产环境中，为防止供应链攻击，可对关键业务镜像开启签名验证。CRI-O 通过 `/etc/containers/policy.json` 配置策略。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 配置仅允许指定仓库的签名镜像，其余仓库默认拒绝
sudo tee /etc/containers/policy.json <<'EOF'
{
  "default": [{"type": "reject"}],
  "transports": {
    "docker": {
      "registry.cn-hangzhou.aliyuncs.com/demo": [
        {"type": "signedBy", "keyType": "GPGKeys", "keyPath": "/etc/pki/demo-pubkey.gpg"}
      ]
    }
  }
}
EOF
sudo systemctl reload crio
```
开启签名验证后，未签名的镜像会触发 `ImagePullBackOff`，值班人员可通过 `crictl pull` 或 `journalctl -u crio` 查看签名校验失败的具体原因。

## 6. Runtime Handler 配置

CRI-O 支持通过 `runtime_handler` 字段为不同 Pod 选择不同 OCI runtime。例如默认使用 runc，安全敏感业务使用 Kata Containers。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建 runtime handler drop-in，注册 kata 作为可选运行时
sudo tee /etc/crio/crio.conf.d/50-runtime-handler.conf <<'EOF'
[crio.runtime.runtimes.runc]
allowed_annotations = []

[crio.runtime.runtimes.kata]
allowed_annotations = []
runtime_path = "/usr/bin/containerd-shim-kata-v2"
runtime_type = "vm"
EOF
sudo systemctl restart crio
```
在 Pod 中通过 `runtimeClassName: kata` 即可使用 Kata。详见 [[domain-13-container-runtime/03-containerd-cri-o/03-oci-runtimes-comparison.md|OCI 运行时对比]]。

## 7. 监控与日志

CRI-O 内置 Prometheus 风格 metrics，默认监听 `localhost:9537`。生产建议通过 Node Exporter 或 Prometheus Operator 的 `ServiceMonitor` 采集。

```bash
# 查看 CRI-O metrics 是否可访问
curl -s http://localhost:9537/metrics | grep crio_operations
```

关键指标包括：

| 指标 | 含义 | 告警建议 |
| --- | --- | --- |
| `crio_operations` | 各 CRI 接口调用次数与延迟 | P99 延迟突增时触发 |
| `crio_image_pulls` | 镜像拉取成功/失败计数 | 失败率 > 5% 触发 |
| `crio_containers` | 当前容器数量 | 接近节点上限时触发 |
| `crio_runtime_operations` | OCI runtime 操作计数 | 频繁失败说明 runtime 异常 |

日志统一由 systemd 管理，排查时先定位时间窗口。

```bash
# 查看 CRI-O 最近 200 条日志，过滤错误关键字
sudo journalctl -u crio -n 200 --no-pager | grep -iE "error|fail|warn"
```

## 8. 常见故障排查

### 8.1 Pod 卡在 ContainerCreating

步骤一：查看 kubelet 与 CRI-O 日志，确认是否卡在 sandbox 创建。

```bash
# 查看 kubelet 与 CRI-O 日志，定位 sandbox/container 创建阶段
sudo journalctl -u kubelet -n 200 --no-pager | grep -i "ContainerCreating"
sudo journalctl -u crio -n 200 --no-pager | grep -i "sandbox"
```

步骤二：检查 pause 镜像是否可拉取、CNI 插件是否就绪、runtime handler 是否存在。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 pause 镜像与 CNI 插件状态
sudo crictl pull registry-vpc.cn-hangzhou.aliyuncs.com/acs/pause:3.9
ls -l /opt/cni/bin/ | head
```
### 8.2 镜像拉取失败且提示 unauthorized

可能是 `/etc/containers/registries.conf` 中的认证未配置，或 imagePullSecret 未下发到节点。可使用 `crictl pull` 复现。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 手动拉取镜像并查看 CRI-O 返回的认证错误详情
sudo crictl pull registry.cn-hangzhou.aliyuncs.com/demo/app:v1.0
sudo journalctl -u crio -n 50 --no-pager | grep -i "unauthorized"
```
### 8.3 存储损坏导致无法创建容器

`/var/lib/containers` 异常卸载或磁盘损坏时，CRI-O 可能无法读取 storage metadata。此时应先停止 CRI-O，备份并重建存储目录（注意会丢失本地镜像）。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 备份并重建容器存储元数据，仅在存储损坏且无法修复时执行
sudo systemctl stop crio
sudo mv /var/lib/containers /var/lib/containers.bak.$(date +%Y%m%d%H%M)
sudo mkdir -p /var/lib/containers
sudo systemctl start crio
```
## 9. CRI-O 与 containerd 对比

| 维度 | CRI-O | containerd |
| --- | --- | --- |
| 设计目标 | 仅服务 Kubernetes CRI | 通用容器运行时，支持 CRI 与直接 API |
| 社区主导 | Red Hat / OpenShift | CNCF / Docker 生态 |
| 镜像签名 | 原生支持 policy.json | 需借助 Nerdctl/外部工具 |
| 工具链 | crictl、podman、skopeo | crictl、ctr、nerdctl |
| 配置文件 | crio.conf + registries.conf | config.toml |
| 资源占用 | 极简，启动快 | 略重，但插件生态更丰富 |
| 阿里云 ACK | 非主流默认选项 | 默认运行时，文档与工具更成熟 |

在阿里云专有云环境中，若客户未指定运行时，优先推荐 containerd；若存在 Red Hat 生态或镜像签名合规需求，可评估 CRI-O。

## 10. 阿里云专有云 RHEL 节点部署注意事项

在阿里云专有云使用 RHEL 8/9 作为节点操作系统时，CRI-O 部署需要注意订阅、SELinux、防火墙与内核模块等问题。

1. **订阅管理**：RHEL 节点必须注册到 Red Hat Subscription Manager 或本地 Satellite，才能安装 container-selinux、cri-o 等依赖。使用 `subscription-manager register --username <user> --password <pass>` 完成注册后，启用对应的软件仓库。

2. **SELinux**：CRI-O 默认启用 SELinux 标签。若集群策略要求 Permissive 模式，需要在 `/etc/selinux/config` 中调整并重启节点。生产环境建议保持 Enforcing，并通过 `container-selinux` 包提供的策略规则运行容器。

3. **防火墙**：确保节点间 10250（kubelet）、6443（apiserver）、2379/2380（etcd）以及 CRI-O 运行时所需的端口已放行。专有云 ASO 通常通过安全组统一控制。

4. **内核模块**：使用 Kata Containers 时，需要加载 `kvm`、`kvm_intel` 或 `kvm_amd` 模块，并确认 `/dev/kvm` 存在。

```bash
# 检查 RHEL 节点是否已注册并启用 container-tools 模块
subscription-manager status
yum module list | grep container-tools
sudo yum module enable -y container-tools:rhel8
```

## 11. 与 Podman 协同管理

CRI-O 与 Podman 共享 `/var/lib/containers` storage 与 `/etc/containers` 配置。这意味着运维人员可以在同一节点上使用 Podman 做镜像预览、签名验证或本地调试，但需要注意权限差异：CRI-O 以 root 运行，Podman 默认使用当前用户。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 Podman 查看 CRI-O 已拉取的镜像（需要 root 权限以访问同一 storage）
sudo podman images --storage-driver overlay

# 使用 skopeo 检查远程镜像的 layer 与签名信息
skopeo inspect --tls-verify=false docker://registry.cn-hangzhou.aliyuncs.com/demo/app:v1.0
```
在排障时，若 CRI-O 拉取镜像失败，可先用 Podman 或 skopeo 在同一节点上验证仓库连通性与镜像完整性，缩小问题范围。

## 12. 生产检查清单

- [ ] CRI-O 版本与 Kubernetes 大版本一致；
- [ ] `/etc/crio/crio.conf.d/` 使用 drop-in 覆盖，避免主配置被升级覆盖；
- [ ] `cgroup_manager = "systemd"` 与 kubelet 配置一致；
- [ ] pause 镜像指向内网可访问地址；
- [ ] `/etc/containers/registries.conf` 已配置 mirror 与私有仓库；
- [ ] `/etc/containers/policy.json` 签名策略已按业务需求启用；
- [ ] metrics 端口 9537 可被节点本地 Prometheus 抓取；
- [ ] 节点 drain/uncordon 流程已验证；
- [ ] 关键镜像已做预拉取验证。

## 13. 相关文档

- [[domain-13-container-runtime/03-containerd-cri-o/01-containerd-production-operations.md|containerd 生产运维指南]]
- [[domain-13-container-runtime/03-containerd-cri-o/03-oci-runtimes-comparison.md|OCI 运行时对比]]
- [[domain-13-container-runtime/01-containerd-deep-guide.md|containerd 深度指南]]
- [[domain-02-workloads-applications/00-core-workloads/15-container-runtime-interfaces.md|容器运行时接口]]

```

<!-- risk-assessed -->
