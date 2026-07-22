---
title: Contour (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- networking
- contour
- coredns
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Contour 是什么
- 如何 Contour
trigger_keywords:
- Contour
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Contour

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: Go

## 概述

Contour 是一个 CNCF 孵化项目，由 VMware（现 Broadcom）开发维护，是一个高性能的 Kubernetes Ingress 控制器。它基于 Envoy Proxy 数据平面，支持高级流量管理功能。Contour 于 2017 年开源，支持 HTTP/2、gRPC、TLS 透传、加权路由等高级特性。作为最早采用 Envoy 作为数据平面的 Ingress 控制器之一，Contour 在性能和功能丰富度方面表现突出。

## Key Features（核心能力）

- **Envoy 数据平面**：基于 Envoy Proxy 提供高性能 L7 代理和路由
- **HTTPProxy CRD**：使用声明式 HTTPProxy 自定义资源替代 Ingress，支持更丰富的路由配置
- **多协议支持**：支持 HTTP/1.1、HTTP/2、HTTP/3 (QUIC)、gRPC 和 WebSocket
- **TLS 自动化**：集成 cert-manager 实现证书自动化管理，支持 Let's Encrypt
- **加权路由**：支持蓝绿部署和金丝雀发布的加权流量分配
- **Global Rate Limiting**：支持全局速率限制，保护后端服务

## 架构与工作原理

Contour 采用数据平面/控制平面分离架构。控制平面组件 Contour 作为 Deployment 部署，通过 Kubernetes API 监听 Ingress、HTTPProxy、Service 等资源变化，将配置转换为 Envoy 的 xDS 配置，通过 gRPC 流式推送到 Envoy。数据平面 Envoy 作为 DaemonSet 或 Deployment 部署，负责实际的流量代理。Contour 支持两种部署模式：leader-elected Contour（高可用）和 per-pod Envoy（独立配置）。

## K8s 集成

Contour 通过 CRD（HTTPProxy）和标准 Ingress 资源与 Kubernetes 集成。HTTPProxy CRD 提供比 Ingress 更强大的功能，包括路由嵌套、跨命名空间委托、流量权重分配等。Contour Controller 监听 K8s API 变更并实时更新 Envoy 配置，实现声明式流量管理。Service 的 readiness probe 变更会自动反映到 Envoy 的端点配置中。

## 生产用例

- **生产级 Ingress 网关**：为微服务提供统一的入口流量管理
- **金丝雀发布**：通过 HTTPProxy 的加权路由实现渐进式流量切换
- **gRPC 负载均衡**：利用 Envoy 对 gRPC 的原生 L7 负载均衡能力
- **多租户 Ingress**：通过 HTTPProxy 的跨命名空间委托支持多团队流量管理

## 安装与快速开始

```bash
# 🟢 Helm 安装
helm install contour oci://ghcr.io/projectcontour/contour/contour \
  -n projectcontour --create-namespace

# 🟢 验证安装
kubectl get pods -n projectcontour
kubectl get httpproxy -A

# 🟢 查看 Envoy Service
kubectl get svc -n projectcontour envoy
```

### HTTPProxy 示例

```yaml
apiVersion: projectcontour.io/v1
kind: HTTPProxy
metadata:
  name: app-proxy
  namespace: default
spec:
  virtualhost:
    fqdn: app.example.com
    tls:
      secretName: app-tls
  routes:
  - conditions:
    - prefix: /api
    services:
    - name: api-service
      port: 8080
      weight: 90
    - name: api-service-canary
      port: 8080
      weight: 10  # 10% 金丝雀
  - conditions:
    - prefix: /
    services:
    - name: frontend-service
      port: 80
    timeoutPolicy:
      response: 30s
      idle: 5m
    retryPolicy:
      count: 3
      retryOn:
      - 5xx
      - connect-failure
```

### 跨命名空间委托

```yaml
# 根 HTTPProxy (平台团队管理)
apiVersion: projectcontour.io/v1
kind: HTTPProxy
metadata:
  name: root-proxy
  namespace: ingress
spec:
  virtualhost:
    fqdn: "*.example.com"
  includes:
  - name: team-a-routes
    namespace: team-a
    conditions:
    - prefix: /team-a
---
# 团队 HTTPProxy (团队自己管理)
apiVersion: projectcontour.io/v1
kind: HTTPProxy
metadata:
  name: team-a-routes
  namespace: team-a
spec:
  routes:
  - services:
    - name: team-a-service
      port: 8080
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 HTTPProxy 状态
kubectl get httpproxy -A
kubectl describe httpproxy <name> -n <ns>

# 🟢 查看 Contour 日志
kubectl logs -n projectcontour -l app=contour --tail=50

# 🟢 查看 Envoy 日志
kubectl logs -n projectcontour -l app=envoy --tail=50

# 🟢 查看 Envoy 配置 (xDS)
kubectl exec -n projectcontour <envoy-pod> -- curl -s localhost:9001/config_dump | jq

# 🟢 查看 Envoy 统计
kubectl exec -n projectcontour <envoy-pod> -- curl -s localhost:9001/stats

# 🟢 查看 Envoy 集群
kubectl exec -n projectcontour <envoy-pod> -- curl -s localhost:9001/clusters
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| HTTPProxy 无效 | 配置错误/冲突 | `kubectl describe httpproxy` |
| 503 错误 | 后端无健康端点 | 检查 Service/Endpoints |
| 路由不生效 | 条件冲突/优先级 | 检查 conditions 和 includes |
| TLS 失败 | Secret 不存在/过期 | 检查 TLS Secret |
| Envoy 配置未更新 | Contour 异常 | 检查 Contour 日志 |

## 对比替代方案

| 特性 | Contour | Nginx Ingress | Istio Gateway | Traefik |
|------|---------|---------------|---------------|--------|
| 数据平面 | Envoy | Nginx | Envoy | Traefik |
| 配置方式 | HTTPProxy CRD | Ingress + annotations | VirtualService | IngressRoute CRD |
| 动态配置 | xDS (实时) | reload | xDS | 实时 |
| 复杂度 | 中 | 低 | 高 | 低 |
| 金丝雀 | 原生权重 | 注解 | 原生 | 原生 |
| gRPC | 原生 | 需配置 | 原生 | 原生 |

## 检查清单

- [ ] 理解 Contour 架构 (Control Plane + Envoy)
- [ ] 掌握 HTTPProxy CRD 配置
- [ ] 能配置加权路由 (金丝雀)
- [ ] 理解跨命名空间委托
- [ ] 掌握故障排查流程
- [ ] 了解 Contour vs Nginx vs Istio 选型

## Related

- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]

<!-- risk-assessed -->
