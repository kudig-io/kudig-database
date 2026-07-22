---
title: Kairos (entities)
description: '## 概述'
summary: 'Kairos 是一个不可变 Linux 元发行版框架，专注于将任何 Linux 发行版转化为不可变的、基于容器镜像的操作系统，特别适用于边缘计算和 Kubernetes 节点的自动化部署。它支持通过 cloud-init 风格的 YAML 配置实现零接触安装（Zero-Touch Provisioning），'
category: entities
tags:
- k8s
- cncf
- edge
- kairos
- prometheus
- grafana
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kairos 是什么
- 如何 Kairos
trigger_keywords:
- Kairos
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kairos

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Go

## 概述

Kairos 是由 SUSE 工程师发起的开源不可变 Linux 元发行版框架，2022 年进入 CNCF Sandbox。它将**任意 Linux 发行版**（Ubuntu、Fedora、openSUSE、Alpine 等）转化为不可变的、基于容器镜像的操作系统。Kairos 特别适用于**边缘计算和 Kubernetes 节点**的自动化部署——通过 cloud-init 风格的 YAML 配置实现零接触安装（Zero-Touch Provisioning，ZTP），设备通电后自动从网络拉取配置并加入集群。

Kairos 的核心特性是 **P2P 网格组网**能力。多个 Kairos 节点通过 P2P 协议（基于 libp2p）自动发现彼此并组建 Kubernetes 集群（内置 K3s），无需中心化的控制节点。这使得在边缘场景部署分布式 K8s 集群变得极其简单——设备通电 + 网络连接 → 自动组建集群。

## Key Features

- **不可变 OS**：基于容器镜像的原子系统更新和回滚（A/B 分区）
- **多基础发行版**：支持 Ubuntu、Fedora、openSUSE、Alpine、Rocky 等基础系统
- **Zero-Touch Provisioning**：cloud-init 风格的 YAML 配置实现通电即部署
- **P2P 集群组网**：基于 libp2p 的自动节点发现和 K3s 集群组建
- **OCI 镜像分发**：OS 镜像通过标准 OCI Registry 分发和版本管理
- **边缘优化**：低资源占用，支持 ARM/x86 架构

## Architecture

Kairos 由 **Kairos OS 镜像**（不可变的基础系统镜像，包含内核和容器运行时）、**Kairos Agent**（运行在每个节点上，负责配置、升级和集群管理）、**P2P 网格层**（基于 libp2p 的节点发现和通信）和 **cloud-init 配置**（`cloud-config.yaml` 定义节点角色和集群配置）组成。系统采用 A/B 分区方案——每次升级写入备用分区，重启时切换，失败可回滚。

## K8s 集成

Kairos 内置 K3s 轻量级 Kubernetes。通过 `cloud-config.yaml` 配置 `k3s.enabled: true` 并指定角色（server/agent），节点通电后自动加入或组建 K3s 集群。P2P 网格自动处理节点发现和 TLS 证书分发。也支持部署完整的 Kubernetes（通过自定义 cloud-init 安装 kubeadm）。

## 生产部署要点

- **镜像精简**：自定义 Kairos 镜像时只安装必要的包，减小攻击面
- **P2P 令牌安全**：P2P 网络令牌需要安全存储和分发
- **升级策略**：使用蓝绿升级策略，先升级部分节点验证后再全量升级
- **配置管理**：将 cloud-config 纳入版本控制，确保配置可追溯
- **离线部署**：边缘场景预先下载 K3s 二进制和镜像到 OCI 镜像中

## 生产场景

1. **边缘 IoT 集群**：数百个边缘设备通电后自动组建分布式 K8s 集群
2. **零售门店部署**：各门店服务器预装 Kairos，远程管理 OS 升级
3. **离线 Kubernetes**：工业环境中无外网的 K8s 集群自动化部署
4. **不可变安全基线**：关键节点的不可变 OS，防止配置漂移

## 安装与配置

```bash
# 下载 Kairos ISO
wget https://github.com/kairos-io/kairos/releases/latest/download/kairos-ubuntu-v1.x.iso

# 创建 cloud-config.yaml
cat > cloud-config.yaml <<EOF
#cloud-config
hostname: edge-node-01
users:
  - name: kairos
    ssh_authorized_keys:
      - ssh-rsa AAAA...
k3s:
  enabled: true
  args:
    - --cluster-cidr=10.244.0.0/16
    - --disable=traefik
p2p:
  enabled: true
  token: "<your-p2p-token>"
  dns: false
EOF

# 写入 USB 启动盘
dd if=kairos-ubuntu.iso of=/dev/sdb bs=4M status=progress
# 将 cloud-config.yaml 放在 USB 的 OEM 分区
# 设备通电启动后自动安装并加入集群

# 使用 kairos-cli 管理节点
kairos-cli --url https://node-ip:9090 info
kairos-cli --url https://node-ip:9090 upgrade --image quay.io/kairos/kairos-ubuntu:v1.6.0
```

```yaml
# 完整 cloud-config 示例（生产级）
#cloud-config
hostname: edge-node-{{.NodeID}}
users:
  - name: kairos
    passwd: "$6$hash..."
    ssh_authorized_keys:
      - ssh-ed25519 AAAA... admin@company.com
stages:
  network:
    - name: "Configure network"
      commands:
        - nmcli con mod "Wired connection 1" ipv4.addresses 10.0.1.{{.NodeNum}}/24
        - nmcli con mod "Wired connection 1" ipv4.gateway 10.0.1.1
k3s:
  enabled: true
  version: v1.29.2+k3s1
  args:
    - --cluster-cidr=10.244.0.0/16
    - --service-cidr=10.96.0.0/12
    - --disable=traefik
    - --flannel-backend=host-gw
p2p:
  enabled: true
  token: "${P2P_TOKEN}"
  network_token: "${NETWORK_TOKEN}"
```

## 运维操作

```bash
# 🟢 查看节点状态和版本
kairos-cli info
kubectl get nodes -o wide

# 🟡 升级节点 OS（A/B 分区，可回滚）
kairos-cli upgrade --image quay.io/kairos/kairos-ubuntu:v1.7.0
# 升级后重启切换分区
systemctl reboot

# 🟡 回滚到上一个版本（升级失败时）
# 在 GRUB 菜单选择上一个分区启动
grub-editenv list  # 查看当前启动分区

# 🟢 查看 P2P 网格状态
kairos-cli p2p status

# 🟡 重置节点（清除所有数据，重新安装）
kairos-cli reset

# 🔴 强制重置并清除所有数据（不可恢复）
kairos-cli reset --force
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 节点未加入集群 | P2P token 不匹配或网络不通 | `kairos-cli p2p status` | 检查 token 一致性和防火墙规则 |
| 升级后无法启动 | 新镜像损坏或不兼容 | GRUB 菜单选择旧分区 | 回滚到上一版本 |
| K3s 未启动 | cloud-config 配置错误 | `journalctl -u k3s` | 检查 /oem/cloud-config.yaml |
| 节点无法拉取镜像 | 离线环境未配置本地 Registry | `crictl images` | 配置私有 Registry 或预加载镜像 |
| 磁盘空间不足 | 容器镜像和日志累积 | `df -h /var/lib/rancher` | 清理无用镜像和日志 |

```
排查流程：
├── 节点未加入集群
│   ├── 检查 P2P token 是否与其他节点一致
│   ├── 检查防火墙是否允许 P2P 端口 (8000-8010)
│   ├── journalctl -u kairos-agent 查看 agent 日志
│   └── 确认 cloud-config.yaml 格式正确
├── 升级失败
│   ├── 检查目标镜像是否可拉取
│   ├── 确认磁盘剩余空间足够（A/B 分区需要双倍空间）
│   └── GRUB 回滚到上一版本
└── K3s 异常
    ├── systemctl status k3s 查看服务状态
    ├── kubectl get nodes 确认节点状态
    └── 检查 /var/lib/rancher/k3s 日志
```

## 生产案例

### 案例 1：零售门店边缘集群自动部署

- **场景**：全国 200+ 零售门店，每店 2 台服务器，无 IT 人员现场，需要零接触部署 K8s 集群
- **排查**：之前使用 Ansible + kubeadm 部署，需要现场配置网络、SSH 密钥，每店部署耗时 4 小时
- **方案**：预装 Kairos ISO，cloud-config 通过 DHCP + DNS 自动获取，P2P 自动组建 K3s 集群，远程管理升级
- **效果**：部署时间从 4 小时降至 15 分钟（通电即用），远程 OS 升级成功率 99.5%，运维人员减少 80%

### 案例 2：工业环境离线 K8s 集群

- **场景**：工厂产线无外网连接，需要部署不可变 K8s 集群运行 MES 系统，安全合规要求禁止配置漂移
- **排查**：传统 OS 经常被现场工程师修改配置，导致环境不一致和故障
- **方案**：Kairos 不可变 OS + 所有应用容器化，OS 镜像通过内网 OCI Registry 分发，A/B 分区升级
- **效果**：配置漂移事件归零，OS 升级回滚时间 < 2 分钟，年度非计划停机减少 90%

## 对比

| 特性 | Kairos | Talos Linux | Flatcar | bootc | 适用场景 |
|------|--------|-------------|---------|-------|----------|
| 不可变 OS | ✅ | ✅ | ✅ | ✅ | 安全合规 |
| P2P 组网 | ✅ | ❌ | ❌ | ❌ | 边缘无中心场景 |
| Zero-Touch | ✅ | ⚠️ | ❌ | ⚠️ | 无人值守部署 |
| 多基础发行版 | ✅ | ❌ 自研 | ❌ | ✅ | 企业 OS 偏好 |
| 运维复杂度 | 低 | 中 | 中 | 中 | 小团队边缘运维 |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]

## Related

- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kitops]] — KitOps
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[k3s]] — k3s 轻量级 Kubernetes

- kairos
- [[实体/interlink.md|InterLink]]
- [[实体/akri.md|Akri]]
- [[实体/openyurt.md|OpenYurt]]
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference


<!-- risk-assessed -->
