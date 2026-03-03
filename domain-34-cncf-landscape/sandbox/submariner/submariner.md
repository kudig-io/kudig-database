# Submariner

> **成熟度**: Sandbox | **加入时间**: 2021-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://submariner.io |
| **GitHub** | https://github.com/submariner-io/submariner |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Networking |
| **适用场景** | 多集群网络互通 |

---

## 项目概述

Submariner 实现 Kubernetes 多集群之间的 Pod 和 Service 网络直连。它在集群之间建立加密隧道 (IPsec/WireGuard)，允许跨集群的 Pod 直接通信和 Service 发现，解决了多集群环境下的网络连通性问题。

---

## 核心特性

- **跨集群 Pod 网络**: Pod 到 Pod 直接通信
- **跨集群 Service 发现**: 使用 ServiceImport/ServiceExport
- **加密隧道**: IPsec 或 WireGuard 加密
- **Globalnet**: 处理重叠 CIDR 的情况
- **Gateway 选举**: 自动选举网关节点
- **Lighthouse DNS**: 跨集群 DNS 解析

---

## 架构设计

```
┌────────────────────────────────────────────────────────────┐
│                  Submariner Architecture                    │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  ┌──────────────────┐         ┌──────────────────┐        │
│  │   Cluster A      │  IPsec/ │   Cluster B      │        │
│  │                  │ WireGuard│                  │        │
│  │  ┌────────────┐  │◄───────►│  ┌────────────┐  │        │
│  │  │  Gateway   │  │  Tunnel │  │  Gateway   │  │        │
│  │  │  Node      │  │         │  │  Node      │  │        │
│  │  └────────────┘  │         │  └────────────┘  │        │
│  │                  │         │                  │        │
│  │  ┌────────────┐  │         │  ┌────────────┐  │        │
│  │  │ Lighthouse │  │◄───────►│  │ Lighthouse │  │        │
│  │  │ (DNS)      │  │  Service│  │ (DNS)      │  │        │
│  │  └────────────┘  │ Discovery │ └────────────┘  │        │
│  │                  │         │                  │        │
│  │  ┌────────────┐  │         │  ┌────────────┐  │        │
│  │  │ Route Agent│  │         │  │ Route Agent│  │        │
│  │  │ (DaemonSet)│  │         │  │ (DaemonSet)│  │        │
│  │  └────────────┘  │         │  └────────────┘  │        │
│  │                  │         │                  │        │
│  │  Pod: 10.42.0.x │         │  Pod: 10.43.0.x │        │
│  │  Svc: 10.96.x.x │         │  Svc: 10.97.x.x │        │
│  └──────────────────┘         └──────────────────┘        │
│                                                            │
│  ┌──────────────────────────────────────────────────────┐  │
│  │                   Broker Cluster                      │  │
│  │  ┌────────────────────────────────────────────────┐  │  │
│  │  │  CRD Store: Clusters, Endpoints, ServiceImports│  │  │
│  │  └────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 subctl

```bash
curl -Ls https://get.submariner.io | bash
export PATH=$PATH:~/.local/bin
```

### 部署 Broker

```bash
# 在 Broker 集群
subctl deploy-broker --kubeconfig broker-kubeconfig
```

### 加入集群

```bash
# 集群 A 加入
subctl join broker-info.subm --kubeconfig cluster-a-kubeconfig \
  --clusterid cluster-a

# 集群 B 加入
subctl join broker-info.subm --kubeconfig cluster-b-kubeconfig \
  --clusterid cluster-b
```

### 验证连接

```bash
# 检查连接状态
subctl show all

# 验证跨集群连通性
subctl verify --kubecontexts cluster-a,cluster-b --only connectivity
```

---

## 跨集群 Service 发现

### 导出 Service

```yaml
# 在 Cluster A
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: my-service
  namespace: default
```

### 访问跨集群 Service

```bash
# 在 Cluster B 中的 Pod 可以直接访问
curl http://my-service.default.svc.clusterset.local
```

---

## Globalnet (重叠 CIDR)

```bash
# 加入时启用 Globalnet
subctl join broker-info.subm \
  --clusterid cluster-a \
  --globalnet \
  --globalnet-cidr 242.0.0.0/16
```

---

## 最佳实践

1. **网关节点**: 为网关节点分配足够带宽
2. **CIDR 规划**: 提前规划避免 CIDR 重叠
3. **WireGuard**: 推荐使用 WireGuard 替代 IPsec
4. **监控**: 监控隧道状态和延迟
5. **高可用**: 配置多个网关节点

---

## 参考资源

- [官方文档](https://submariner.io/operations/)
- [GitHub Repo](https://github.com/submariner-io/submariner)
- [快速入门](https://submariner.io/getting-started/)
- [架构设计](https://submariner.io/getting-started/architecture/)

---

**维护者**: Kudig Team | **许可证**: MIT
