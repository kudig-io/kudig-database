---
title: KubeClipper
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- grafana
- cilium
- flannel
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KubeClipper 是什么
- 如何 KubeClipper
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeClipper
- cncf
- landscape
---

# KubeClipper

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubeclipper.io/ |
| **GitHub** | https://github.com/kubeclipper/kubeclipper |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KubeClipper 是一个轻量级的 Kubernetes 集群全生命周期管理平台，提供 Web UI 和 CLI 工具，支持在物理机、虚拟机和云主机上快速部署和管理 Kubernetes 集群。它采用 Agent 架构，无需依赖 Ansible 或 SSH，支持离线部署、集群扩缩容、版本升级、备份恢复等完整的集群运维能力。

### 核心特性

- **快速部署**: 通过 Web UI 或 CLI 快速部署 Kubernetes 集群，支持高可用配置
- **离线安装**: 支持离线镜像和软件包，适合无公网环境部署
- **多集群管理**: 统一管理多个 Kubernetes 集群的生命周期
- **插件管理**: 内置常用组件（CNI、CSI、Ingress、监控等）的一键安装
- **版本升级**: 支持 Kubernetes 版本的滚动升级
- **备份恢复**: 支持 etcd 数据的备份和恢复

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                    KubeClipper                        │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │              kc-server (控制面)               │    │
│  │  ┌───────────┐ ┌───────────┐ ┌────────────┐  │    │
│  │  │ API Server│ │ Controller│ │ etcd       │  │    │
│  │  │           │ │ Manager   │ │ (内置)     │  │    │
│  │  └───────────┘ └───────────┘ └────────────┘  │    │
│  │  ┌───────────────────────────────────────┐   │    │
│  │  │           Web Console (UI)             │   │    │
│  │  └───────────────────────────────────────┘   │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│           ┌────────────┼────────────┐                │
│           │            │            │                │
│   ┌───────▼──────┐ ┌───▼──────┐ ┌──▼───────────┐   │
│   │  kc-agent    │ │ kc-agent │ │  kc-agent    │   │
│   │  (Node 1)    │ │ (Node 2) │ │  (Node 3)    │   │
│   └───────┬──────┘ └───┬──────┘ └───┬──────────┘   │
└───────────┼────────────┼────────────┼───────────────┘
            │            │            │
   ┌────────▼────┐ ┌─────▼─────┐ ┌───▼────────┐
   │  K8s Cluster │ │ K8s Cluster│ │ K8s Cluster │
   │  (Master+    │ │ (Worker)   │ │ (Worker)    │
   │   Worker)    │ │            │ │             │
   └─────────────┘ └────────────┘ └─────────────┘
```

---

## 快速开始

### 安装 KubeClipper

```bash
# 下载 kcctl 命令行工具
curl -sfL https://oss.kubeclipper.io/get | bash -

# 初始化 KubeClipper 服务端
kcctl deploy --server <server-ip>

# 访问 Web Console
# 默认地址: http://<server-ip>:80
# 默认账号: admin / Thinkbig1
```

### 添加节点

```bash
# 在目标节点上安装 Agent
kcctl join --server <server-ip> --agent <node-ip>

# 或通过 Web UI 添加节点
```

### 创建 Kubernetes 集群

```bash
# 通过 CLI 创建集群
kcctl create cluster \
  --name my-cluster \
  --master node1,node2,node3 \
  --worker node4,node5 \
  --version v1.28.0 \
  --cni calico \
  --cri containerd
```

### 通过 Web UI 创建集群

```yaml
# 访问 Web Console > 集群管理 > 创建集群
# 1. 选择节点角色 (Master/Worker)
# 2. 配置 Kubernetes 版本
# 3. 选择容器运行时 (containerd/docker)
# 4. 选择网络插件 (Calico/Cilium/Flannel)
# 5. 确认创建
```

---

## 高级功能

### 高可用集群

```bash
# 创建 3 Master + 3 Worker 的高可用集群
kcctl create cluster \
  --name ha-cluster \
  --master node1,node2,node3 \
  --worker node4,node5,node6 \
  --external-etcd etcd1,etcd2,etcd3 \
  --lb-ip 192.168.1.100 \
  --lb-port 6443
```

### 离线部署

```bash
# 下载离线资源包
kcctl resource download \
  --version v1.28.0 \
  --cni calico \
  --cri containerd \
  --output /path/to/offline-pkg

# 导入离线资源
kcctl resource import --path /path/to/offline-pkg

# 创建集群 (使用本地资源)
kcctl create cluster \
  --name offline-cluster \
  --offline \
  --version v1.28.0
```

### 集群扩缩容

```bash
# 添加 Worker 节点
kcctl scale cluster my-cluster \
  --add-worker node6,node7

# 移除 Worker 节点
kcctl scale cluster my-cluster \
  --remove-worker node7

# 添加 Master 节点 (扩展控制面)
kcctl scale cluster my-cluster \
  --add-master node8
```

### 版本升级

```bash
# 升级集群版本
kcctl upgrade cluster my-cluster \
  --version v1.29.0 \
  --strategy rolling
```

### 插件管理

```bash
# 安装 Helm
kcctl addon install helm --cluster my-cluster

# 安装监控 (Prometheus + Grafana)
kcctl addon install monitoring --cluster my-cluster

# 安装 Ingress Controller
kcctl addon install ingress-nginx --cluster my-cluster

# 查看已安装插件
kcctl addon list --cluster my-cluster
```

### 备份与恢复

```bash
# 创建 etcd 备份
kcctl backup create --cluster my-cluster --name backup-20240301

# 查看备份列表
kcctl backup list --cluster my-cluster

# 恢复集群
kcctl backup restore --cluster my-cluster --name backup-20240301
```

---

## 与其他方案对比

| 特性 | KubeClipper | Kubekey | RKE2 | Kubespray |
|:---|:---|:---|:---|:---|
| 架构 | Agent 无 SSH | SSH | Agent | Ansible |
| Web UI | 内置 | 无 | 无 | 无 |
| 离线部署 | 支持 | 支持 | 支持 | 支持 |
| 多集群管理 | 原生支持 | 不支持 | 不支持 | 不支持 |
| 插件管理 | 内置 | 有限 | 有限 | Addon |
| 升级 | 滚动升级 | 支持 | 支持 | 支持 |
| 依赖 | 无 | SSH | 无 | Ansible/Python |

---

## 最佳实践

1. **高可用部署**: 生产环境至少部署 3 个 Master 节点，使用外部负载均衡
2. **离线准备**: 在有网环境提前下载离线包，便于在隔离环境中快速部署
3. **备份策略**: 定期备份 etcd 数据，配置自动备份任务
4. **版本规划**: 在测试环境先验证版本升级，再应用到生产环境
5. **监控告警**: 安装监控插件，配置集群和节点级别的告警

---

## 参考资源

- [KubeClipper 官方文档](https://kubeclipper.io/docs/)
- [KubeClipper GitHub](https://github.com/kubeclipper/kubeclipper)
- [KubeClipper 快速入门](https://kubeclipper.io/docs/getting-started/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
