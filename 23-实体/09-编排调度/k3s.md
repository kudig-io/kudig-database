---
title: k3s 轻量级 Kubernetes
description: '## 概述'
summary: 'k3s 是经过 CNCF 认证的轻量级 Kubernetes 发行版，专为资源受限环境设计。它将 Kubernetes 所需的所有组件打包到单个小于 100MB 的二进制文件中，非常适合 IoT、边缘计算、CI/CD 和开发环境。k3s 移除了遗留和可选组件，'
category: entities
tags:
- k8s
- cncf
- runtime
- k3s
- etcd
- prometheus
- grafana
- cilium
- flannel
- coredns
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- k3s 轻量级 Kubernetes 是什么
- 如何 k3s 轻量级 Kubernetes
trigger_keywords:
- k3s
- 轻量级
- Kubernetes
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[k3s|k3s]] 轻量级 Kubernetes

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

k3s 是经过 CNCF 认证的轻量级 Kubernetes 发行版，专为资源受限环境设计。它将 Kubernetes 所需的所有组件打包到单个小于 100MB 的二进制文件中，非常适合 IoT、边缘计算、CI/CD 和开发环境。k3s 移除了遗留和可选组件，同时保持完全兼容标准 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]]。

## 核心能力

- **轻量级部署**: 单二进制文件，内存占用约 512MB
- **快速安装**: 30 秒内完成安装，开箱即用
- **内置组件**: 包含 containerd、Flannel、CoreDNS、Traefik
- **SQLite/etcd**: 默认 SQLite，支持 etcd、MySQL、PostgreSQL
- **ARM 支持**: 原生支持 ARM64 和 ARMv7
- **自动证书**: TLS 证书自动生成和轮换

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **生产环境**: 使用外部数据库 (MySQL/PostgreSQL/etcd) 替代 SQLite
- **高可用**: 部署至少 3 个 Server 节点
- **网络**: 根据场景选择 Flannel 后端 (vxlan/wireguard/host-gw)
- **安全**: 轮换 Node Token，限制 API Server 访问
- **备份**: 定期备份数据存储和证书
- **升级**: 使用自动升级控制器管理版本

## 架构定位

在 CNCF 生态中，k3s 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]

## 安装与配置

### 单节点快速安装

```bash
# 🟢 安装 k3s server（单节点）
curl -sfL https://get.k3s.io | sh -s - \
  --write-kubeconfig-mode 644 \
  --disable traefik \
  --flannel-backend=wireguard-native

# 🟢 验证安装
kubectl get nodes
kubectl get pods -A

# 🟢 获取 kubeconfig
export KUBECONFIG=/etc/rancher/k3s/k3s.yaml
```

### 高可用集群部署

```bash
# 🟢 安装第一个 Server 节点（嵌入式 etcd）
curl -sfL https://get.k3s.io | sh -s - server \
  --cluster-init \
  --tls-san k3s-api.example.com \
  --disable traefik \
  --flannel-backend=wireguard-native

# 获取 join token
cat /var/lib/rancher/k3s/server/node-token

# 🟢 加入额外 Server 节点（至少 3 个）
curl -sfL https://get.k3s.io | sh -s - server \
  --server https://<first-server>:6443 \
  --token <node-token> \
  --tls-san k3s-api.example.com

# 🟢 加入 Agent 节点
curl -sfL https://get.k3s.io | sh -s - agent \
  --server https://k3s-api.example.com:6443 \
  --token <node-token>
```

### 使用外部数据库 (PostgreSQL)

```bash
# 🟢 使用 PostgreSQL 作为数据存储
curl -sfL https://get.k3s.io | sh -s - server \
  --datastore-endpoint="postgres://user:pass@db-host:5432/k3s" \
  --tls-san k3s-api.example.com
```

### 配置文件 (/etc/rancher/k3s/config.yaml)

```yaml
# Server 配置
write-kubeconfig-mode: "0644"
tls-san:
  - k3s-api.example.com
  - 10.0.1.100
# 禁用不需要的组件
disable:
  - traefik
  - servicelb
  - local-storage
# 网络
flannel-backend: wireguard-native
cluster-cidr: 10.42.0.0/16
service-cidr: 10.43.0.0/16
# 节点标签
node-label:
  - "node-type=server"
# 污点
node-taint:
  - "CriticalAddonsOnly=true:NoExecute"
```

### 自动升级控制器

```yaml
# System Upgrade Controller
apiVersion: upgrade.cattle.io/v1
kind: Plan
metadata:
  name: k3s-server-upgrade
  namespace: system-upgrade
spec:
  concurrency: 1
  cordon: true
  nodeSelector:
    matchExpressions:
    - key: node-role.kubernetes.io/control-plane
      operator: Exists
  serviceAccountName: system-upgrade
  upgrade:
    image: rancher/k3s-upgrade
  version: v1.30.2+k3s1
---
apiVersion: upgrade.cattle.io/v1
kind: Plan
metadata:
  name: k3s-agent-upgrade
  namespace: system-upgrade
spec:
  concurrency: 2
  cordon: true
  nodeSelector:
    matchExpressions:
    - key: node-role.kubernetes.io/control-plane
      operator: DoesNotExist
  prepare:
    image: rancher/k3s-upgrade
    args: ["prepare", "k3s-server-upgrade"]
  serviceAccountName: system-upgrade
  upgrade:
    image: rancher/k3s-upgrade
  version: v1.30.2+k3s1
```

## 运维操作

```bash
# 🟢 检查 k3s 服务状态
systemctl status k3s  # server
systemctl status k3s-agent  # agent

# 🟢 查看 k3s 日志
journalctl -u k3s -f --since "10 min ago"

# 🟢 检查集群状态
kubectl get nodes -o wide
kubectl get pods -A

# 🟢 查看嵌入式 etcd 状态
k3s etcd-snapshot list
k3s etcd-snapshot create --name manual-backup

# 🟡 重启 k3s
systemctl restart k3s

# 🟢 获取节点 token
cat /var/lib/rancher/k3s/server/node-token

# 🟢 检查证书
k3s certificate rotate
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点无法加入 | token 错误/网络不通 | `journalctl -u k3s-agent` | 检查 token/防火墙 6443 |
| etcd 不健康 | 节点数为偶数/磁盘慢 | `k3s etcd-snapshot list` | 确保奇数节点/SSD |
| Pod 网络不通 | Flannel 配置错误 | `kubectl logs -n kube-system -l app=flannel` | 检查 cluster-cidr/MTU |
| 升级失败 | 版本跳跃太大 | `kubectl get plan -n system-upgrade` | 逐版本升级 |
| 磁盘空间不足 | 镜像/日志累积 | `df -h`; `crictl images` | 配置镜像 GC/清理 |

### 排查流程

```
k3s 集群异常
├── 节点无法加入？
│   ├── token 正确？→ cat /var/lib/rancher/k3s/server/node-token
│   ├── 网络可达？→ curl -k https://<server>:6443/cacerts
│   └── 防火墙开放？→ 6443, 8472(vxlan), 51820(wireguard)
├── 控制平面异常？
│   ├── etcd 健康？→ k3s etcd-snapshot list
│   ├── 证书有效？→ k3s certificate rotate
│   └── 资源充足？→ free -h; df -h
└── 工作负载异常？
    ├── DNS 解析？→ kubectl exec pod -- nslookup kubernetes
    ├── 网络连通？→ kubectl exec pod -- ping <other-pod>
    └── 存储挂载？→ kubectl get pvc -A
```

## 生产案例

### 案例1：边缘计算 50 节点 k3s 集群

- **场景**：零售门店 50 个边缘节点，每节点 4GB RAM，运行 POS 系统
- **方案**：k3s + 嵌入式 etcd（3 server）+ WireGuard 网络；禁用 Traefik/service-lb 节省资源；使用 System Upgrade Controller 自动更新
- **效果**：每节点仅占用 512MB 内存，升级零人工干预

### 案例2：CI/CD 临时集群

- **场景**：每次 PR 需要独立 K8s 环境运行集成测试
- **方案**：GitHub Actions 中安装 k3s（30s）→ 部署应用 → 运行测试 → 销毁
- **效果**：每个 PR 获得完整 K8s 环境，测试时间从 15min 降至 5min

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| k3s | 轻量、快速、内置全部 | 功能精简、社区较小 | 边缘/IoT/CI |
| k0s | 更轻量、无依赖 | 生态较小 | 极简环境 |
| MicroK8s | Ubuntu原生、snap安装 | 仅Ubuntu最佳 | Ubuntu 开发环境 |
| kubeadm | 标准、灵活、全功能 | 复杂、需手动配置 | 生产数据中心 |
| EKS/AKS/GKE | 全托管、零运维 | 成本高、厂商锁定 | 公有云生产 |

## 检查清单

- [ ] 生产环境使用外部数据库或嵌入式 etcd（非 SQLite）
- [ ] Server 节点数 >= 3（高可用）
- [ ] TLS SAN 包含所有访问地址
- [ ] 网络后端已选择（wireguard/vxlan/host-gw）
- [ ] 自动升级控制器已部署
- [ ] etcd 快照备份已配置
- [ ] 节点 token 已安全存储
- [ ] 资源监控已配置

## Related

- [[podman-container-tools]] — Podman Desktop
- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[19-故障诊断/04-高级排障/44-kind-k3s-single-node-troubleshooting.md|44-kind-k3s-single-node-troubleshooting]]
- k3s
- [[23-实体/15-参考与索引/multi-cloud-terms.md|K8s 多云架构术语参考]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/node-index.md|Node 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
