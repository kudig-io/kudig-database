---
title: K0s (entities)
description: '## 概述'
summary: 'K0s 是一个轻量级、全功能的 Kubernetes 发行版，打包为单一二进制文件，零依赖、零摩擦地安装和运行。k0s 的设计目标是简化 Kubernetes 的安装、运维和升级过程，适用于从边缘设备到大规模数据中心的各种场景。'
category: entities
tags:
- k8s
- cncf
- runtime
- k0s
- etcd
- prometheus
- grafana
- cilium
- calico
- containerd
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K0s 是什么
- 如何 K0s
trigger_keywords:
- K0s
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K0s

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

K0s 是由 Mirantis（原 Docker Enterprise 团队）开源的轻量级、全功能 Kubernetes 发行版，2021 年加入 CNCF Sandbox。它打包为单一二进制文件，零依赖、零摩擦地安装和运行。k0s 的设计目标是简化 Kubernetes 的安装、运维和升级过程，适用于从边缘设备到大规模数据中心的各种场景。与 k3s 类似，k0s 致力于降低 Kubernetes 的使用门槛，但提供了更完整的上游 Kubernetes 兼容性。

## 核心特性

- **单一二进制**: 所有组件（API Server、Controller Manager、Scheduler、kubelet）打包在一个二进制文件中
- **零依赖**: 无需预装容器运行时、etcd 或其他组件，二进制自包含一切
- **全功能**: 包含 CoreDNS、CNI（Calico/kube-router）、metrics-server 等核心组件
- **k0sctl**: 基础设施即代码工具，通过 YAML 配置实现多节点自动化部署
- **Autopilot**: 内置滚动升级和自动恢复能力
- **灵活架构**: 支持单节点、多 Controller HA 和 Worker 分离部署

## 架构

k0s 将 Kubernetes 所有控制平面组件（API Server、Controller Manager、Scheduler、etcd）编译为单一 Go 二进制文件。通过子命令（`k0s controller`、`k0s worker`、`k0s etcd`）在同一二进制中启动不同角色。Controller 节点运行内嵌的 etcd 作为存储后端，Worker 节点仅运行 kubelet 和容器运行时（containerd）。k0sctl 通过 SSH 连接目标节点，自动分发二进制、配置服务和加入集群。默认 CNI 使用 kube-router，可切换为 Calico。

## Kubernetes 集成

k0s 是 100% 上游 Kubernetes 兼容发行版，通过 CNCF 一致性认证。所有 Kubernetes API、kubectl 命令和标准 CRD/Operator 在 k0s 上完全兼容。控制平面以 systemd 服务运行，kubelet 通过本地 socket 连接 API Server。支持标准的 kubeconfig 认证、RBAC 和 NetworkPolicy。通过 Containerd Socket Interface 兼容标准 CRI 插件。

## 生产使用场景

1. **边缘 IoT**: 在资源受限的边缘设备上运行轻量级 Kubernetes
2. **裸金属自建**: 替代 kubeadm 简化裸金属集群的部署和运维
3. **开发测试**: 快速创建本地开发集群，零配置启动
4. **Air-gap 环境**: 单二进制 + 离线镜像包适配隔离网络环境

## 安装与配置

```bash
# 单节点安装
curl -sSLf https://get.k0s.sh | sudo sh
sudo k0s install controller --single
sudo k0s start
# 多节点部署（使用 k0sctl）
curl -sSLf https://github.com/k0sproject/k0sctl/releases/latest/download/k0sctl-linux-x64 -o k0sctl
k0sctl apply --config cluster.yaml
```

### k0sctl 集群配置示例

```yaml
# cluster.yaml
apiVersion: k0sctl.k0sproject.io/v1beta1
kind: Cluster
metadata:
  name: prod-k0s-cluster
spec:
  k0s:
    version: "1.30.2+k0s.0"
    config:
      apiVersion: k0s.k0sproject.io/v1beta1
      kind: ClusterConfig
      spec:
        network:
          provider: calico
          calico:
            mode: vxlan
        storage:
          type: etcd
        telemetry:
          enabled: false
  hosts:
    - role: controller+etcd
      count: 3
      ssh:
        address: 10.0.1.1
        user: root
        keyPath: ~/.ssh/id_rsa
    - role: worker
      count: 5
      ssh:
        address: 10.0.2.1
        user: root
        keyPath: ~/.ssh/id_rsa
```

### 获取 kubeconfig

```bash
# 从 k0sctl 获取
k0sctl kubeconfig > kubeconfig
export KUBECONFIG=./kubeconfig
kubectl get nodes
```

## 运维操作

```bash
# 🟢 查看集群状态
k0s status

# 🟢 查看节点信息
kubectl get nodes -o wide

# 🟢 查看控制平面组件状态
kubectl get componentstatuses
kubectl get pods -n kube-system

# 🟡 添加 Worker 节点
k0s token create --role=worker > worker-token
# 在新节点执行:
curl -sSLf https://get.k0s.sh | sudo sh
sudo k0s install worker --token-file worker-token
sudo k0s start

# 🟡 滚动升级集群
k0sctl apply --config cluster-upgraded.yaml
# 或使用 Autopilot
k0s autopilot

# 🔴 移除节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data
kubectl delete node <node>

# 🟢 查看 etcd 健康状态
k0s etcd leave --help  # 仅查看
etcdctl --endpoints=https://127.0.0.1:2379 endpoint health

# 🟡 备份 etcd
k0s etcd snapshot save /backup/etcd-$(date +%Y%m%d).db
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点 NotReady | kubelet 未启动 | `systemctl status k0scontroller` | `sudo k0s start` |
| API Server 无响应 | 证书过期 | `k0s status` | 重新生成证书 |
| etcd 集群不健康 | 多数节点失联 | `etcdctl endpoint health` | 恢复失联节点或移除成员 |
| Pod Pending | CNI 未就绪 | `kubectl get pods -n kube-system -l k8s-app=calico-node` | 检查 Calico DaemonSet |
| 升级失败 | 版本跳跃过大 | `k0s version` | 逐版本升级 |
| Worker 加入失败 | Token 过期 | `k0s token create --role=worker` | 重新生成 Token |

### 排查流程

```
k0s 集群异常
├─ 控制平面不可用？
│  ├─ k0s status 报错 → 检查 systemd 服务状态
│  ├─ API Server 无响应 → 检查证书有效期、端口 6443
│  └─ etcd 异常 → etcdctl endpoint status
├─ Worker 节点 NotReady？
│  ├─ kubelet 未运行 → systemctl status k0sworker
│  ├─ 容器运行时异常 → crictl ps / journalctl -u k0sworker
│  └─ 网络不通 → 检查 CNI Pod 和路由表
└─ 升级失败？
   ├─ Autopilot 报错 → kubectl get updateconfigs -n kube-system
   └─ 版本不兼容 → 确认升级路径（最多跳 2 个 minor）
```

## 生产案例

### 案例 1: 边缘工厂 Air-gap 环境部署

**场景**: 某制造企业在无外网的工厂环境部署 K8s 管理产线 IoT 设备。

**方案**:
1. 在有网环境下载 k0s 二进制 + 离线镜像包
2. 使用 k0s airgap install 安装
3. 配置私有 Registry 镜像源
```bash
# 离线安装
sudo k0s install controller --single --disable-components=konnectivity-server
sudo k0s start
# 导入离线镜像
k0s image import airgap-images.tar
```

**效果**: 30 分钟内完成单节点部署，稳定运行 18 个月无故障。

### 案例 2: 从 kubeadm 迁移到 k0s

**场景**: 运维团队希望简化 50 节点裸金属集群的运维复杂度。

**方案**:
1. 新建 k0s 集群（3 Controller + 5 Worker 先行）
2. 逐步将工作负载迁移到新集群
3. 旧节点重新以 Worker 角色加入 k0s 集群
4. 使用 k0sctl 管理全集群生命周期

**效果**: 升级时间从 4 小时缩短到 30 分钟，运维人力减少 60%。

## 对比与替代方案

| 维度 | k0s | k3s | Talos Linux | kubeadm |
|------|-----|-----|-------------|----------|
| 部署方式 | 单二进制 | 单二进制 | 不可变 OS | 多组件 |
| 上游兼容性 | 100% | 替换部分组件 | 100% | 100% |
| 存储后端 | etcd | SQLite/Dqlite/etcd | etcd | etcd |
| 管理工具 | k0sctl | Rancher | talosctl | kubeadm |
| 自动升级 | Autopilot | System Upgrade Controller | 内置 | 手动 |
| 最小资源 | 512MB RAM | 512MB RAM | 1GB RAM | 2GB RAM |
| 社区规模 | 中 | 大 | 中 | 官方 |

## 检查清单

- [ ] 控制平面节点数为奇数（3/5）确保 etcd 多数派
- [ ] k0sctl 配置文件已纳入版本控制
- [ ] etcd 定期备份已配置（cron + 远程存储）
- [ ] 升级路径已验证（不超过 2 个 minor 版本跳跃）
- [ ] Air-gap 环境离线镜像包已准备
- [ ] Worker Token 安全存储，定期轮换
- [ ] 监控告警：API Server/etcd/kubelet 健康检查
- [ ] 节点自动恢复策略已配置

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **k0s** | 单二进制、全功能、k0sctl 优秀 | 社区较小 |
| k3s | CNCF 生态最大、Rancher 支持 | 替换组件（etcd→SQLite/Dqlite） |
| Talos Linux | 不可变 OS、API 驱动 | 需替换整个操作系统 |
| kubeadm | 官方标准 | 配置复杂、步骤多 |

## 架构定位

在 CNCF 生态中，k0s 属于 **Runtime / Kubernetes Distribution** 类别，是轻量级 Kubernetes 发行版的重要选择，适合需要上游兼容性但希望简化运维的场景。

## 参考链接

- [[etcd]]
- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]

## Related

- [[bank-vaults]] — Bank-Vaults
- [[thanos]] — Thanos
- [[02-containerd-security-hardening]] — containerd 安全加固
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- k0s
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/node-index.md|Node 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
