---
title: k3s
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- flannel
- coredns
- helm
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- k3s 是什么
- 如何 k3s
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- k3s
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- etcd-basics
- mysql-basics
---

title: k3s
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- flannel
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- k3s 是什么
- 如何 k3s
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- k3s
- cncf
- landscape
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

# k3s

> **成熟度**: Sandbox | **加入时间**: 2020-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://k3s.io |
| **GitHub** | https://github.com/k3s-io/k3s |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Kubernetes Distribution |
| **维护组织** | Rancher (SUSE) |

---

## 项目概述

k3s 是经过 CNCF 认证的轻量级 Kubernetes 发行版，专为资源受限环境设计。它将 Kubernetes 所需的所有组件打包到单个小于 100MB 的二进制文件中，非常适合 IoT、边缘计算、CI/CD 和开发环境。k3s 移除了遗留和可选组件，同时保持完全兼容标准 Kubernetes API。

---

## 核心特性

- **轻量级部署**: 单二进制文件，内存占用约 512MB
- **快速安装**: 30 秒内完成安装，开箱即用
- **内置组件**: 包含 containerd、Flannel、CoreDNS、Traefik
- **SQLite/etcd**: 默认 SQLite，支持 etcd、MySQL、PostgreSQL
- **ARM 支持**: 原生支持 ARM64 和 ARMv7
- **自动证书**: TLS 证书自动生成和轮换
- **Helm Controller**: 内置 Helm Chart 部署支持
- **高可用**: 支持多 Server 节点 HA 部署

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                        k3s Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    k3s Server Node                        │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              k3s Binary (~100MB)                     │ │   │
│  │  │  ┌─────────┬──────────┬───────────┬──────────────┐  │ │   │
│  │  │  │ API     │ Control  │ Scheduler │ Controller   │  │ │   │
│  │  │  │ Server  │ Manager  │           │ Manager      │  │ │   │
│  │  │  └─────────┴──────────┴───────────┴──────────────┘  │ │   │
│  │  │  ┌─────────┬──────────┬───────────┬──────────────┐  │ │   │
│  │  │  │ kubelet │ kube-    │ containerd│ Flannel      │  │ │   │
│  │  │  │         │ proxy    │           │ CNI          │  │ │   │
│  │  │  └─────────┴──────────┴───────────┴──────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │            Data Store (SQLite/etcd/MySQL)            │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                         Node Token                               │
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                     k3s Agent Nodes                       │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │
│  │  │   Agent 1   │  │   Agent 2   │  │   Agent N   │      │   │
│  │  │ ┌─────────┐ │  │ ┌─────────┐ │  │ ┌─────────┐ │      │   │
│  │  │ │ kubelet │ │  │ │ kubelet │ │  │ │ kubelet │ │      │   │
│  │  │ │containerd│ │  │ │containerd│ │  │ │containerd│ │     │   │
│  │  │ │ Flannel │ │  │ │ Flannel │ │  │ │ Flannel │ │      │   │
│  │  │ └─────────┘ │  │ └─────────┘ │  │ └─────────┘ │      │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Built-in Add-ons                        │   │
│  │  ┌─────────┐ ┌─────────┐ ┌───────────┐ ┌─────────────┐  │   │
│  │  │ Traefik │ │ CoreDNS │ │ Local Path│ │ Metrics     │  │   │
│  │  │ Ingress │ │         │ │ Provisioner│ │ Server      │  │   │
│  │  └─────────┘ └─────────┘ └───────────┘ └─────────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **k3s Server** | 控制平面，包含 API Server、Scheduler、Controller |
| **k3s Agent** | 工作节点，运行 kubelet 和容器运行时 |
| **containerd** | 默认容器运行时，替代 Docker |
| **Flannel** | 默认 CNI 插件，提供 Pod 网络 |
| **SQLite** | 默认数据存储，单节点部署 |
| **Traefik** | 内置 Ingress Controller |

---

## 快速开始

### 安装 Server 节点

```bash
# 一键安装 k3s Server
curl -sfL https://get.k3s.io | sh -

# 检查服务状态
sudo systemctl status k3s

# 获取 kubeconfig
sudo cat /etc/rancher/k3s/k3s.yaml

# 查看节点
sudo k3s kubectl get nodes
```

### 加入 Agent 节点

```bash
# 在 Server 节点获取 Token
sudo cat /var/lib/rancher/k3s/server/node-token

# 在 Agent 节点执行
curl -sfL https://get.k3s.io | K3S_URL=https://server-ip:6443 \
  K3S_TOKEN=<node-token> sh -
```

---

## 配置示例

### Server 配置文件

```yaml
# /etc/rancher/k3s/config.yaml
write-kubeconfig-mode: "0644"
tls-san:
  - "k3s.example.com"
  - "192.168.1.100"

# 禁用内置组件
disable:
  - traefik
  - servicelb

# 使用外部数据库
datastore-endpoint: "mysql://user:pass@tcp(db.example.com:3306)/k3s"

# 集群 CIDR 配置
cluster-cidr: "10.42.0.0/16"
service-cidr: "10.43.0.0/16"

# Flannel 配置
flannel-backend: "vxlan"

# Kubelet 参数
kubelet-arg:
  - "max-pods=250"
  - "eviction-hard=memory.available<500Mi"
```

### 高可用部署

```bash
# 第一个 Server 节点
curl -sfL https://get.k3s.io | sh -s - server \
  --cluster-init \
  --tls-san=k3s-lb.example.com

# 获取 Token
sudo cat /var/lib/rancher/k3s/server/token

# 加入其他 Server 节点
curl -sfL https://get.k3s.io | sh -s - server \
  --server https://first-server:6443 \
  --token <token>
```

### 使用外部 etcd

```yaml
# /etc/rancher/k3s/config.yaml
datastore-endpoint: "https://etcd1:2379,https://etcd2:2379,https://etcd3:2379"
datastore-cafile: "/etc/rancher/k3s/etcd-ca.crt"
datastore-certfile: "/etc/rancher/k3s/etcd-client.crt"
datastore-keyfile: "/etc/rancher/k3s/etcd-client.key"
```

---

## 边缘计算部署

### IoT 设备配置

```bash
# 资源受限环境安装
curl -sfL https://get.k3s.io | INSTALL_K3S_EXEC="--disable traefik \
  --disable servicelb \
  --disable metrics-server \
  --kubelet-arg=max-pods=20" sh -

# ARM 设备自动检测架构安装
curl -sfL https://get.k3s.io | sh -
```

### 离线安装

```bash
# 下载安装包
wget https://github.com/k3s-io/k3s/releases/download/v1.28.4+k3s1/k3s
wget https://github.com/k3s-io/k3s/releases/download/v1.28.4+k3s1/k3s-airgap-images-amd64.tar

# 准备离线镜像
sudo mkdir -p /var/lib/rancher/k3s/agent/images/
sudo cp k3s-airgap-images-amd64.tar /var/lib/rancher/k3s/agent/images/

# 安装二进制
sudo cp k3s /usr/local/bin/
sudo chmod +x /usr/local/bin/k3s

# 离线安装
INSTALL_K3S_SKIP_DOWNLOAD=true ./install.sh
```

---

## Helm Chart 部署

### HelmChart CRD

```yaml
# /var/lib/rancher/k3s/server/manifests/nginx.yaml
apiVersion: helm.cattle.io/v1
kind: HelmChart
metadata:
  name: nginx-ingress
  namespace: kube-system
spec:
  repo: https://kubernetes.github.io/ingress-nginx
  chart: ingress-nginx
  targetNamespace: ingress-nginx
  createNamespace: true
  valuesContent: |-
    controller:
      replicaCount: 2
      service:
        type: LoadBalancer
```

### HelmChartConfig 覆盖

```yaml
# 覆盖内置 Traefik 配置
apiVersion: helm.cattle.io/v1
kind: HelmChartConfig
metadata:
  name: traefik
  namespace: kube-system
spec:
  valuesContent: |-
    dashboard:
      enabled: true
    ports:
      websecure:
        tls:
          enabled: true
```

---

## 监控与日志

### 查看日志

```bash
# Server 日志
sudo journalctl -u k3s -f

# Agent 日志
sudo journalctl -u k3s-agent -f

# 容器日志
sudo k3s crictl logs <container-id>
```

### Prometheus 监控

```yaml
apiVersion: helm.cattle.io/v1
kind: HelmChart
metadata:
  name: prometheus
  namespace: kube-system
spec:
  repo: https://prometheus-community.github.io/helm-charts
  chart: kube-prometheus-stack
  targetNamespace: monitoring
  createNamespace: true
```

---

## 与标准 K8s 差异

| 特性 | k3s | 标准 K8s |
|:---|:---|:---|
| **二进制大小** | ~100MB | ~1GB |
| **内存占用** | ~512MB | ~2GB+ |
| **默认存储** | SQLite | etcd |
| **容器运行时** | containerd | 可选 |
| **Ingress** | Traefik | 无 |
| **存储类** | Local Path | 无 |

---

## 最佳实践

1. **生产环境**: 使用外部数据库 (MySQL/PostgreSQL/etcd) 替代 SQLite
2. **高可用**: 部署至少 3 个 Server 节点
3. **网络**: 根据场景选择 Flannel 后端 (vxlan/wireguard/host-gw)
4. **安全**: 轮换 Node Token，限制 API Server 访问
5. **备份**: 定期备份数据存储和证书
6. **升级**: 使用自动升级控制器管理版本

---

## 参考资源

- [官方文档](https://docs.k3s.io)
- [GitHub Repo](https://github.com/k3s-io/k3s)
- [Rancher k3s 文档](https://rancher.com/docs/k3s/latest/en/)
- [k3s 升级指南](https://docs.k3s.io/upgrades)
- [k3s HA 部署](https://docs.k3s.io/datastore/ha)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
