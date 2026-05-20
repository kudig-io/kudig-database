---
title: Cilium
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- envoy
- cilium
- helm
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- 如何 Cilium
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cilium
- cncf
- landscape
cross_refs:
- type: fta
  path: ../topic-fta/list/cilium-fta.md
  label: '故障树: cilium'
---

# Cilium

> **成熟度**: Graduated | **加入时间**: 2021-10 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cilium.io |
| **GitHub** | https://github.com/cilium/cilium |
| **文档** | https://docs.cilium.io |
| **许可证** | Apache-2.0 |
| **主要语言** | Go, C |
| **CNCF 分类** | Networking |

---

## 项目概述

### 简介
Cilium 是一个基于 eBPF 的开源网络、安全和可观测性解决方案，为 Kubernetes 和其他容器编排平台提供高性能网络能力。

### 核心定位
Cilium 利用 Linux 内核的 eBPF 技术，在内核级别提供网络连接、负载均衡、网络策略和可观测性，实现了传统网络方案难以达到的性能和灵活性。

### 发展历程
- **2016**: Isovalent 开始开发 Cilium
- **2017**: 开源 Cilium 项目
- **2021-10**: 加入 CNCF 作为孵化项目
- **2023-10**: 成为 CNCF 毕业项目
- **2024**: Cilium v1.15+ 持续演进

---

## 核心功能

### 主要特性
- **eBPF 网络**: 高性能内核级网络数据路径
- **网络策略**: L3/L4/L7 网络策略支持
- **负载均衡**: 替代 kube-proxy 的高性能 LB
- **服务网格**: 基于 eBPF 的 Sidecar-less 服务网格
- **可观测性**: Hubble 网络流量可视化
- **多集群**: ClusterMesh 跨集群网络

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                         User Space                          │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │   Cilium Agent  │ │    Cilium CLI   │ │    Hubble     │ │
│  │   (Daemon)      │ │                 │ │  (Observ.)    │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                       Kernel Space                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    eBPF Programs                        ││
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐   ││
│  │  │  XDP    │ │   TC    │ │  Socket │ │   Cgroup    │   ││
│  │  │(Ingress)│ │ (L3/L4) │ │   LB    │ │  (Policy)   │   ││
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘   ││
│  └─────────────────────────────────────────────────────────┘│
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    eBPF Maps                            ││
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────┐   ││
│  │  │Endpoints│ │Services │ │ Policy  │ │  Tunnel     │   ││
│  │  │   Map   │ │   Map   │ │   Map   │ │    Map      │   ││
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────┘   ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Cilium Agent | 节点守护进程 | 管理 eBPF 程序和配置 |
| Cilium Operator | 集群控制器 | 管理集群级资源 |
| Hubble | 可观测性 | 网络流量监控和可视化 |
| Cilium CLI | 命令行工具 | 管理和调试 Cilium |

### 工作原理
1. Cilium Agent 在每个节点上运行
2. 监听 Kubernetes API 获取 Pod 和 Service 信息
3. 编译并加载 eBPF 程序到内核
4. eBPF 程序在数据路径处理网络流量
5. Hubble 收集和展示网络流量数据

---

## 使用场景

### 典型应用
- **Kubernetes CNI**: 高性能 Pod 网络
- **网络策略**: 细粒度的网络访问控制
- **负载均衡**: 替代 kube-proxy
- **服务网格**: Sidecar-less 服务网格
- **多集群网络**: 跨集群 Pod 通信

### 适用条件
- 需要高性能网络（eBPF 加速）
- 需要 L7 网络策略
- 需要网络可观测性
- 需要 Sidecar-less 服务网格

### 不适用场景
- 旧版本 Linux 内核（< 4.9）
- 不支持 eBPF 的环境
- 简单的网络需求

---

## 快速开始

### 安装部署
```bash
# 使用 Cilium CLI 安装
cilium install

# 使用 Helm 安装
helm repo add cilium https://helm.cilium.io/
helm install cilium cilium/cilium --namespace kube-system

# 启用 Hubble
cilium hubble enable --ui
```

### 基础配置
```yaml
# CiliumNetworkPolicy 示例
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: allow-http-ingress
spec:
  endpointSelector:
    matchLabels:
      app: backend
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "80"
        protocol: TCP
      rules:
        http:
        - method: GET
          path: "/api/.*"
```

### 验证测试
```bash
# 检查 Cilium 状态
cilium status

# 连接测试
cilium connectivity test

# 查看 Hubble 流量
hubble observe --follow
```

---

## 最佳实践

### 生产环境建议
- 使用直接路由模式（避免隧道开销）
- 启用 kube-proxy 替换
- 配置适当的资源限制
- 启用 Hubble 监控

### 性能优化
- 启用 XDP 加速
- 使用 eBPF host routing
- 配置 MTU 优化
- 启用 BPF masquerade

### 安全加固
- 启用 mTLS（通过 Cilium Service Mesh）
- 配置网络策略默认拒绝
- 启用身份认证
- 审计网络流量

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: CNI 网络插件
- **Prometheus**: 指标导出
- **Grafana**: 可视化仪表盘
- **Envoy**: L7 代理支持

### 常见集成方案
- Cilium + Hubble 网络监控
- Cilium + Prometheus + Grafana
- Cilium Service Mesh
- Cilium ClusterMesh 多集群

---

## 参考资源

- [官方文档](https://docs.cilium.io)
- [GitHub Repo](https://github.com/cilium/cilium)
- [CNCF 项目页面](https://www.cncf.io/projects/cilium/)
- [Cilium 博客](https://cilium.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT
