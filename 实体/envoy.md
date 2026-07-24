---
title: Envoy (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- envoy
- prometheus
- grafana
- istio
- gateway
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Envoy 是什么
- 如何 Envoy
trigger_keywords:
- Envoy
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Envoy

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: C++

## 概述

description: '## 项目概述'

## 核心能力

- **L3/L4 代理**: TCP/UDP 代理，支持 TLS 终止
- **L7 代理**: HTTP/2、gRPC、WebSocket 支持
- **服务发现**: 支持多种服务发现机制
- **负载均衡**: 多种负载均衡算法
- **健康检查**: 主动和被动健康检查
- **可观测性**: 丰富的统计、日志、追踪支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 启用 TLS 终止和 mTLS
- 配置合理的超时和重试策略
- 使用动态配置（xDS）而非静态配置
- 启用访问日志和追踪
- 合理配置连接池大小
- 启用 HTTP/2 和连接复用

## 架构定位

Envoy 是云原生生态中最核心的代理组件，广泛用于：
- **Ingress Controller**: Contour, Gloo, Emissary
- **Service Mesh**: Istio, Linkerd (数据平面)
- **API Gateway**: Envoy Gateway, Kong
- **内部代理**: 微服务间通信

## xDS 动态配置

| xDS 类型 | 用途 | 说明 |
|----------|------|------|
| LDS | Listener Discovery | 动态添加/修改监听器 |
| RDS | Route Discovery | 动态路由规则 |
| CDS | Cluster Discovery | 动态上游集群 |
| EDS | Endpoint Discovery | 动态端点列表 |
| SDS | Secret Discovery | 动态证书 |

## 运维操作

### Admin API

```bash
# 🟢 查看配置
curl localhost:9901/config_dump

# 🟢 查看集群状态
curl localhost:9901/clusters

# 🟢 查看统计
curl localhost:9901/stats
curl localhost:9901/stats | grep upstream_rq

# 🟢 查看服务器信息
curl localhost:9901/server_info

# 🟢 查看监听器
curl localhost:9901/listeners

# 🟢 查看路由表
curl localhost:9901/config_dump | jq '.configs[] | select(."@type"=="type.googleapis.com/envoy.admin.v3.RoutesConfigDump")'

# 🟡 开启/关闭日志
curl -X POST localhost:9901/logging?level=debug
curl -X POST localhost:9901/logging?level=info

# 🟡 健康检查
curl localhost:9901/ready
```

### K8s 中的 Envoy 操作

```bash
# 🟢 查看 Envoy Pod
kubectl get pods -l app=envoy

# 🟢 查看 Envoy 日志
kubectl logs <envoy-pod> --tail=50

# 🟢 进入 Envoy Pod
kubectl exec -it <envoy-pod> -- sh

# 🟢 查看访问日志
kubectl logs <envoy-pod> -c istio-proxy --tail=100

# 🟢 查看 Envoy 统计 (Istio)
istioctl proxy-config stats <pod>
istioctl proxy-config clusters <pod>
istioctl proxy-config routes <pod>
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 503 UC | 上游无健康主机 | 检查后端 Pod 状态 |
| 503 UH | 无健康上游 | 检查健康检查配置 |
| 504 UT | 上游超时 | 调整 timeout 配置 |
| 连接拒绝 | 端口未监听 | 检查后端服务 |
| TLS 失败 | 证书不匹配 | 检查 SDS/证书 |
| 高延迟 | 连接池耗尽 | 调整连接池大小 |

### 关键统计指标

| 指标 | 含义 |
|------|------|
| upstream_rq_total | 上游请求总数 |
| upstream_rq_5xx | 上游 5xx 响应 |
| upstream_cx_active | 活跃上游连接 |
| downstream_rq_active | 活跃下游请求 |
| cluster_manager.cluster_added | 集群添加 |
| http.ingress_http.downstream_rq_time | 请求延迟 |

## 检查清单

- [ ] 理解 Envoy xDS 动态配置
- [ ] 掌握 Admin API 使用
- [ ] 能排查 503/504 错误
- [ ] 理解连接池配置
- [ ] 掌握访问日志分析
- [ ] 了解 Envoy 在 Istio/Contour 中的角色

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[dapr]] — Dapr
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- 04-envoy-proxy-enterprise
- 99-envoy-gateway-enterprise-guide
- 07-envoy-gateway-enterprise
- envoy
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.32
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.36
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.37
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.33
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.34
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.30
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-1.31
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- RELEASE-NOTES-1.35
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- [[实体/networking-terms.md|[[K8s 网络术语参考|K8s 网络术语参考]]]] — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[实体/tooling-terms.md|K8s 工具链术语参考]] — Cross-reference
- [[实体/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[概念/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[概念/microservice-resilience-patterns.md|Microservice Resilience Patterns]] — Cross-reference
- [[技能/网络/service-mesh/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
