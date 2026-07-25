---
title: KubeClipper [entities]
description: '## 概述'
summary: 'KubeClipper 是一个轻量级的 Kubernetes 集群全生命周期管理平台，提供 Web UI 和 CLI 工具，支持在物理机、虚拟机和云主机上快速部署和管理 Kubernetes 集群。'
category: entities
tags:
- k8s
- cncf
- platform
- kubeclipper
- etcd
- prometheus
- grafana
- cilium
- containerd
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeClipper 是什么
- 如何 KubeClipper
trigger_keywords:
- KubeClipper
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeClipper

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

KubeClipper 是一个轻量级的 Kubernetes 集群全生命周期管理平台，由九州云（99Cloud）开源开发，2022 年加入 CNCF 沙箱。它提供 Web UI 和 CLI 工具，支持在物理机、虚拟机和云主机上快速部署和管理 Kubernetes 集群。KubeClipper 采用 Agent 架构，无需依赖 Ansible 或 SSH 密钥分发，通过自研的 kc-agent 管理节点。它支持离线部署（air-gapped）、集群扩缩容、版本升级、备份恢复、组件管理（CNI、CSI、监控）等完整的集群运维能力。KubeClipper 特别适合私有云、国产化和离线场景下的 Kubernetes 集群管理。

## 核心能力

- **全生命周期管理**: 创建、扩缩容、升级、备份恢复、卸载 Kubernetes 集群
- **离线部署**: 预打包离线镜像，支持完全断网环境下的集群部署
- **多组件管理**: 集成 Cilium、Calico、Containerd、Harbor、Prometheus 等常用组件
- **Web UI + CLI**: 图形化界面降低运维门槛，同时提供 kcctl CLI 工具
- **Agent 架构**: 无需 Ansible/SSH，kc-Agent 通过 gRPC 与控制中心通信
- **多架构支持**: 支持 x86_64 和 ARM64 架构

## 架构

KubeClipper 采用中心化控制 + Agent 模式：

- **kc-server**: 控制中心，提供 API Server、Web UI 和调度逻辑，数据存储在内置 etcd
- **kc-agent**: 部署在每个被管理节点上的守护进程，执行具体的安装和运维操作
- **gRPC 通信**: kc-server 与 kc-agent 之间通过 gRPC 双向通信，无需 SSH
- **Cluster CRD**: 以 Kubernetes CRD 方式声明集群期望状态
- **Step Pipeline**: 运维操作被分解为有序的 Step（安装依赖、配置 etcd、部署控制面等）
- **离线包管理**: 统一的离线包仓库，支持按需下载和缓存

管理流程：`用户 (UI/CLI) → kc-server → CRD 期望状态 → kc-agent (执行) → 节点配置`

## K8s 集成

KubeClipper 的管理面本身就是一个 Kubernetes API 服务器（内置 etcd），通过 CRD（`Cluster`、`Node`）声明式管理目标集群。kc-agent 部署在被管理节点上，接收并执行安装、升级等操作指令。每个运维操作被分解为原子 Step（如安装 containerd、初始化 etcd、配置 CNI），按 Pipeline 有序执行。支持与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中各种 CNI（Cilium、Calico）、CSI 和 Ingress Controller 集成。

## 生产场景

1. **私有云 K8s 部署**: 在裸金属服务器上批量部署和管理多个生产级 Kubernetes 集群
2. **国产化信创环境**: 支持麒麟 OS、鲲鹏 ARM 等国产化基础设施
3. **离线环境部署**: 在完全断网的机房环境中部署 Kubernetes 集群
4. **多集群运维**: 统一管理开发、测试、生产多套 Kubernetes 集群

## 安装与配置

```bash
# 下载 kcctl CLI
curl -sfL https://oss.kubeclipper.io/kcctl-install.sh | KC_VERSION=v1.4.0 bash -

# 初始化 KubeClipper 管理节点（离线模式）
kcctl deploy --user root --passwd $SERVER_PASSWORD \
  --pkg kc.tar.gz --ip 10.0.0.1

# 访问 Web UI
echo "https://10.0.0.1"

# 添加节点（安装 kc-agent）
kcctl join --agent 10.0.0.2,10.0.0.3,10.0.0.4,10.0.0.5,10.0.0.6 \
  --user root --passwd $NODE_PASSWORD

# 创建集群
kcctl create cluster --name prod-cluster \
  --master 10.0.0.2,10.0.0.3,10.0.0.4 \
  --worker 10.0.0.5,10.0.0.6 \
  --cni calico --cri containerd \
  --k8s-version v1.29.2
```

```yaml
# 集群配置示例（YAML 模式）
apiVersion: core.kubeclipper.io/v1
kind: Cluster
metadata:
  name: prod-cluster
spec:
  kubernetesVersion: v1.29.2
  containerRuntime:
    type: containerd
    version: 1.7.13
  networking:
    cni:
      type: calico
      version: v3.27.0
    podSubnet: 10.244.0.0/16
    serviceSubnet: 10.96.0.0/12
  etcd:
    dataDir: /var/lib/etcd
  masters:
  - id: node-01
  - id: node-02
  - id: node-03
  workers:
  - id: node-04
  - id: node-05
  addons:
  - name: metrics-server
  - name: ingress-nginx
  - name: prometheus-stack
```

## 运维操作

```bash
# 🟢 低风险：查看集群和节点状态
kcctl get cluster
kcctl get node
kcctl describe cluster prod-cluster

# 🟡 中风险：扩容 Worker 节点
kcctl join --cluster prod-cluster --worker 10.0.0.7,10.0.0.8

# 🟡 中风险：升级集群版本
kcctl upgrade cluster prod-cluster --k8s-version v1.30.0

# 🟡 中风险：备份 etcd
kcctl backup cluster prod-cluster --backup-name pre-upgrade

# 🔴 高风险：删除集群
kcctl delete cluster prod-cluster --force

# 🟢 低风险：查看操作日志
kcctl get operation --cluster prod-cluster
kcctl describe operation <operation-id>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 节点添加失败 | Agent 安装失败 | `kcctl describe node <id>` | 检查节点网络和 SSH 凭据 |
| 集群创建失败 | 组件安装错误 | `kcctl describe operation <id>` | 查看 Step 日志，修复具体步骤 |
| 升级失败 | 版本不兼容 | `kcctl get operation --cluster <name>` | 回滚到上一版本 |
| 节点 NotReady | kc-agent 断开 | `systemctl status kc-agent` | 重启 agent，检查 gRPC 连接 |
| 离线部署失败 | 镜像包不完整 | 检查离线包 checksum | 重新下载完整离线包 |

```
排查流程：
├── 集群操作失败？
│   ├── kcctl describe operation → 查看失败 Step
│   ├── 登录目标节点查看日志
│   └── 检查节点资源（磁盘/内存）
├── 节点异常？
│   ├── systemctl status kc-agent → 检查 Agent
│   ├── 检查节点与 kc-server 的 gRPC 连接
│   └── 查看 kubelet 日志
└── 离线环境问题？
    ├── 确认离线包完整性
    ├── 检查本地 Registry 可用性
    └── 验证镜像列表完整
```

## 生产案例

### 案例 1：离线机房批量部署 K8s 集群

- **场景**：金融行业 5 个离线机房，每个需要部署 3 套 K8s 集群（dev/staging/prod）
- **排查**：传统 Kubespray 需要配置 Ansible + SSH，离线环境配置复杂
- **方案**：使用 KubeClipper 离线包 + Agent 架构，无需 SSH，通过 Web UI 批量创建集群
- **效果**：15 套集群部署从 2 周缩短至 2 天，全程 Web UI 操作

### 案例 2：国产化信创环境集群管理

- **场景**：政府项目要求在麒麟 OS + 鲲鹏 ARM 上运行 K8s
- **排查**：主流工具对 ARM + 麒麟 OS 支持不完善
- **方案**：KubeClipper 原生支持 ARM64 + 麒麟 OS，提供完整的离线部署和生命周期管理
- **效果**：顺利通过信创验收，集群稳定运行 12 个月零故障

## 对比

| 特性 | KubeClipper | Kubespray | KubeKey | Rancher |
|------|-------------|-----------|---------|---------|
| 离线部署 | ✅ 原生 | ⚠️ 需配置 | ✅ | ⚠️ 有限 |
| Agent 架构 | ✅ 自研 | ❌ Ansible | ❌ Ansible | ✅ |
| Web UI | ✅ | ❌ | ❌ | ✅ |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，KubeClipper 属于 **Platform** 类别，为云原生应用提供轻量级集群生命周期管理能力。

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[containerd]]
- networking.md|cilium-ebpf-networking]]

## Related

- [[kmesh]] — Kmesh
- [[kpt]] — kpt
- [[logging-operator]] — Loggingng Operator|Logging Operator]]
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubeclipper
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
