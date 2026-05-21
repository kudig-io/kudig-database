---
title: Cilium
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- envoy
- cilium
- helm
- ingress
- networkpolicy
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- 如何 Cilium
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Cilium
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
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
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/cilium-fta.md
  label: '故障树: cilium'
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

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-networking-ecosystem|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[synthesis/Cilium eBPF × 可观测性|Cilium eBPF × 可观测性]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/service-mesh-evolution|服务网格演进]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/Kubernetes Core Concepts|Kubernetes Core Concepts]] — Cross-reference
- [[concepts/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/networkpolicy-fta|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[entities/ecosystem-changelog|生态组件变更日志索引]] — Cross-reference
- [[entities/tetragon|Tetragon]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.9|cilium v1.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.8|cilium v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.18|cilium v1.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.19|cilium v1.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.8|cilium v1.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.9|cilium v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.16|cilium v1.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.3|cilium v1.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.7|cilium v1.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.12|cilium v1.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.6|cilium v1.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.13|cilium v1.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.17|cilium v1.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.2|cilium v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.5|cilium v1.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.10|cilium v1.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.14|cilium v1.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.15|cilium v1.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.0|cilium v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.4|cilium v1.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.11|cilium v1.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.10|cilium v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.11|cilium v0.11 Release Notes]]
