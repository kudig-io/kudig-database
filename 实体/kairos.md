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

## 安装

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
p2p:
  enabled: true
  token: "<your-p2p-token>"
EOF

# 写入 USB 启动盘
dd if=kairos-ubuntu.iso of=/dev/sdb bs=4M
# 将 cloud-config.yaml 放在 USB 的 OEM 分区
# 设备通电启动后自动安装并加入集群
```

## 对比

| 特性 | Kairos | Talos Linux | Flatcar | bootc |
|------|--------|-------------|---------|-------|
| 不可变 OS | ✅ | ✅ | ✅ | ✅ |
| P2P 组网 | ✅ | ❌ | ❌ | ❌ |
| Zero-Touch | ✅ | ⚠️ | ❌ | ⚠️ |
| 多基础发行版 | ✅ | ❌ 自研 | ❌ | ✅ |

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
