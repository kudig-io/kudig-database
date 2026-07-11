---
title: Aeraki Mesh [entities]
description: '## 概述'
summary: 'Aeraki Mesh 是 Istio 服务网格的扩展框架，专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，'
category: entities
tags:
- k8s
- cncf
- networking
- aeraki-mesh
- prometheus
- grafana
- istio
- envoy
- redis
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Aeraki Mesh 是什么
- 如何 Aeraki Mesh
trigger_keywords:
- Aeraki
- Mesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Aeraki Mesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Aeraki Mesh 是由美团开源的 Istio 服务网格扩展框架，2021 年加入 CNCF Sandbox。它专注于为非 HTTP 协议提供流量管理能力。在微服务架构中，除了 HTTP/gRPC 之外，还广泛使用 Dubbo、Thrift、Redis、Kafka 等协议。Aeraki Mesh 通过扩展 Istio 的数据面（Envoy）和控制面，使这些非 HTTP 协议也能享受服务网格的流量路由、负载均衡、熔断限流和可观测性能力。

## 核心特性

- **多协议管理**: Dubbo、Thrift、Redis、Kafka、RocketMQ、Zookeeper 等协议
- **MetaProtocol**: 通用协议扩展框架，支持自定义协议
- **MetaRouter CRD**: 类似 VirtualService 的协议级路由规则
- **Istio 集成**: 与 Istio 控制平面无缝集成，共享 mTLS 和可观测性
- **Redis 读写分离**: 自动解析 Redis 协议实现读写路由
- **Dubbo 灰度**: 基于 Dubbo 服务名的版本路由和流量比例控制

## 架构

Aeraki Mesh 在 Istio 架构上增加了两个组件。Aeraki（控制面扩展）作为 Istio 的翻译器，监听 MetaRouter CRD 和 Istio Service Entry，将非 HTTP 协议的治理规则翻译为 Envoy 过滤器链配置，通过 xDS 下发。数据面上，Aeraki 为 Envoy 注入 MetaProtocol Proxy 或专用协议 Filter（如 Dubbo Proxy、Redis Proxy），在 L7 解析协议元数据（方法名、服务名、参数）进行路由决策。Aeraki 也支持 RDS（Route Discovery Service）动态下发路由规则。

## Kubernetes 集成

Aeraki Mesh 作为 Istio 的扩展部署。它监听 Kubernetes API 中的 MetaRouter CRD 和 Service Entry，通过 Istio 的 Sidecar 注入机制安装 Envoy 扩展 Filter。`Service` 端口命名（如 `tcp-dubbo`、`tcp-redis`）触发 Aeraki 应用对应协议的 Filter。MetaRouter CRD 与 VirtualService 并行工作，VirtualService 管 HTTP，MetaRouter 管非 HTTP。与 Istio 的 mTLS、AuthorizationPolicy 等安全机制完全兼容。

## 生产使用场景

1. **Dubbo 微服务网格**: 将 Java Dubbo 服务纳入网格管理，实现灰度发布和流量控制
2. **Redis 读写分离**: 自动将读请求路由到 Replica，写请求路由到 Master
3. **Kafka 流量管理**: 对 Kafka 消息流量进行限流和监控
4. **Thrift 服务治理**: 为 PHP/Thrift 服务提供熔断和超时能力

## 安装

```bash
# 前置: Istio 已安装
istioctl install --set profile=default
# 安装 Aeraki
kubectl apply -f https://raw.githubusercontent.com/aeraki-mesh/aeraki/main/deploy/aeraki.yaml
# 配置 Dubbo 路由
kubectl apply -f - <<EOF
apiVersion: metaprotocol.aeraki.io/v1beta1
kind: MetaRouter
metadata: { name: dubbo-router }
spec:
  hosts: ["org.apache.dubbo.demo.DemoService..*..*"]
  routes:
  - match: { metadata: { method: { exact: "sayHello" } } }
    route: { cluster: outbound|20880||dubbo-demo.v1 }
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Aeraki Mesh** | 多协议管理、Istio 兼容 | 社区较小 |
| Istio 原生 | HTTP/gRPC 全面支持 | 非 HTTP 协议支持有限 |
| Spring Cloud | Java 原生治理 | 需引入 Spring Cloud SDK |
| Envoy Filter 手动配置 | 完全自定义 | 维护成本极高 |

## 架构定位

在 CNCF 生态中，Aeraki Mesh 属于 **Networking / Service Mesh** 类别，是 Istio 在非 HTTP 协议治理方面的重要补充。它解决了传统服务网格仅覆盖 HTTP 的局限性。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[grpc]] — gRPC
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- aeraki-mesh
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
