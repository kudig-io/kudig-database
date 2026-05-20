---
title: Kmesh
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- istio
- envoy
- cilium
- helm
- ebpf
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kmesh 是什么
- 如何 Kmesh
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kmesh
- cncf
- landscape
---

# Kmesh

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kmesh.net/ |
| **GitHub** | https://github.com/kmesh-net/kmesh |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, C |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kmesh 是一个基于 eBPF 和可编程内核的无 Sidecar 服务网格，在内核空间实现流量治理能力。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。

### 核心特性

- **无 Sidecar**: 内核级流量治理，无需部署 Sidecar 代理容器
- **eBPF 驱动**: 使用 eBPF 在内核态实现流量拦截和路由
- **低延迟**: 消除 Sidecar 引入的网络跳转，降低 P99 延迟
- **低资源占用**: 无需为每个 Pod 分配 Sidecar 的 CPU 和内存
- **Istio 兼容**: 兼容 Istio 控制平面，支持 xDS 协议
- **L4/L7 治理**: 支持负载均衡、灰度发布、流量镜像、熔断等

---

## 架构设计

```
┌─────────────────────────────────────────────────┐
│              Control Plane (Istio)                │
│  ┌──────────┐  ┌──────────┐                     │
│  │  istiod   │  │  Kmesh   │                     │
│  │ (xDS 下发)│  │ daemon   │                     │
│  └─────┬────┘  └────┬─────┘                     │
└────────┼─────────────┼──────────────────────────┘
         │ xDS         │ eBPF Program
         │             │ Load/Update
┌────────▼─────────────▼──────────────────────────┐
│              Data Plane (内核空间)                │
│                                                   │
│  ┌──────────────────────────────────────────┐    │
│  │              eBPF Programs                │    │
│  │  ┌──────────┐  ┌───────────┐            │    │
│  │  │ sockops  │  │ Traffic   │            │    │
│  │  │ (L4 路由)│  │ Mgmt (L7) │            │    │
│  │  └──────────┘  └───────────┘            │    │
│  │  ┌──────────┐  ┌───────────┐            │    │
│  │  │ Load     │  │ Circuit   │            │    │
│  │  │ Balance  │  │ Breaker   │            │    │
│  │  └──────────┘  └───────────┘            │    │
│  └──────────────────────────────────────────┘    │
│                                                   │
│  ┌───────────┐  ┌───────────┐  ┌───────────┐    │
│  │  Pod A    │  │  Pod B    │  │  Pod C    │    │
│  │ (无Sidecar)│ │ (无Sidecar)│ │ (无Sidecar)│    │
│  └───────────┘  └───────────┘  └───────────┘    │
└───────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 前提：已安装 Kubernetes 集群和 Istio 控制平面

# 安装 Kmesh
helm repo add kmesh https://kmesh-net.github.io/kmesh/
helm install kmesh kmesh/kmesh \
  --namespace kmesh-system \
  --create-namespace

# 验证安装
kubectl get pods -n kmesh-system
```

### 启用 Kmesh 治理

```bash
# 为命名空间启用 Kmesh (替代 Istio Sidecar 注入)
kubectl label namespace default istio.io/dataplane-mode=Kmesh

# 部署应用 (无需 Sidecar 注入)
kubectl apply -f my-app.yaml
```

### 流量治理配置

```yaml
# 使用标准 Istio VirtualService 配置灰度发布
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: my-service
spec:
  hosts:
    - my-service
  http:
    - match:
        - headers:
            x-version:
              exact: "v2"
      route:
        - destination:
            host: my-service
            subset: v2
    - route:
        - destination:
            host: my-service
            subset: v1
          weight: 90
        - destination:
            host: my-service
            subset: v2
          weight: 10
```

---

## 与其他方案对比

| 特性 | Kmesh | Istio (Sidecar) | Istio Ambient | Cilium Mesh |
|:---|:---|:---|:---|:---|
| 架构 | 内核 eBPF | Sidecar 代理 | ztunnel+waypoint | eBPF |
| 额外延迟 | ~0.1ms | ~1-3ms | ~0.5ms | ~0.2ms |
| 内存开销/Pod | 0 | 50-100MB | 共享节点代理 | 0 |
| L7 治理 | 支持 | 完整 | 支持 | 部分 |
| 控制平面 | Istio | Istio | Istio | Cilium |
| 内核版本要求 | 5.10+ | 无 | 无 | 5.4+ |

---

## 最佳实践

1. **内核版本**: 确保节点内核版本 >= 5.10，推荐 5.15+ 获得最佳 eBPF 支持
2. **渐进迁移**: 从非关键服务开始启用 Kmesh，验证功能后逐步扩大范围
3. **监控**: 利用 Kmesh 导出的 eBPF 指标监控流量治理效果
4. **混合模式**: 可与 Istio Sidecar 在同一集群中共存，按命名空间选择模式
5. **安全策略**: 配合 Istio 的 mTLS 和授权策略使用

---

## 参考资源

- [Kmesh 官方文档](https://kmesh.net/docs/)
- [Kmesh GitHub](https://github.com/kmesh-net/kmesh)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
