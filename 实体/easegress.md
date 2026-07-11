---
title: Easegress (entities)
description: '## 概述'
summary: 'Easegress 是一个云原生的全生命周期 API 编排和流量网关，提供高可用、高性能的流量调度能力。它支持丰富的流量治理功能，包括 API 编排、金丝雀发布、限流熔断、服务发现、WebSocket、MQTT 代理等。Easegress 采用过滤器链（Filter Pipeline）架构，用户可以灵活组合过滤器实现复杂的流量处理逻辑。'
category: entities
tags:
- k8s
- cncf
- networking
- easegress
- prometheus
- grafana
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Easegress 是什么
- 如何 Easegress
trigger_keywords:
- Easegress
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Easegress

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Easegress 是由 Megaease 开发的开源云原生流量编排和 API 网关，2021 年进入 CNCF Sandbox。它将传统的 API Gateway、Service Mesh 数据面、边缘网关和 Serverless 路由能力整合到单一二进制中。Easegress 采用**过滤器链（Filter Pipeline）**架构，每个请求依次通过认证、限流、改写、代理等过滤器，开发者可以像搭积木一样灵活组合过滤器链实现复杂的流量治理逻辑。

Easegress 支持 Raft 集群模式实现高可用，内置 Wasm 过滤器扩展能力，也提供 MQTT 代理、WebSocket 代理等物联网场景支持。它原生支持 Kubernetes Service Discovery 和 Consul/Eureka/Nacos 等外部注册中心。

## Key Features

- **过滤器链管道**：请求通过可组合的过滤器序列处理（Adapter、Validator、Proxy、Builder 等）
- **多协议支持**：HTTP/HTTPS、HTTP/2、gRPC、WebSocket、MQTT
- **Wasm 扩展**：通过 Wasm 过滤器实现自定义逻辑，不影响核心代码
- **流量治理**：金丝雀发布、A/B 测试、限流、熔断、重试、超时
- **服务网格数据面**：可作为 Service Mesh 的数据面代理
- **Serverless 编排**：将 FaaS 函数编排为复合 API

## Architecture

Easegress 的核心是 **Pipeline**（管道）和 **Filter**（过滤器）。每个 HTTPServer 监听端口，将请求路由到 Pipeline。Pipeline 内部由有序的 Filter 列表组成，每个 Filter 处理请求并决定是否传递给下一个 Filter 或直接返回。后端池（BackendPool）管理上游服务实例列表，支持轮询、加权、一致性哈希等负载均衡策略。集群模式下使用 Raft 协议同步配置和数据。

## K8s 集成

Easegress 通过 Kubernetes Service Discovery 自动发现 Pod IP 作为上游实例。可通过 Easegress Operator 或 Helm Chart 在 K8s 中部署。也支持通过 Ingress Controller 模式作为集群入口网关，使用标准 Ingress 资源或自定义 CRD 配置路由规则。

## 生产部署要点

- **过滤器组合**：按职责拆分过滤器（限流→认证→路由→代理），保持管道清晰
- **集群部署**：生产环境部署 3+ 节点的 Raft 集群确保高可用
- **Wasm 扩展**：复杂的自定义逻辑使用 Wasm 过滤器实现，避免修改核心代码
- **健康检查**：为上游服务器配置主动健康检查
- **监控**：利用内置的 Prometheus metrics 监控流量和延迟

## 生产场景

1. **统一 API 网关**：聚合后端多个微服务为统一 API，处理认证、限流和路由
2. **灰度发布**：通过流量分割实现金丝雀发布，逐步将流量迁移到新版本
3. **IoT 消息代理**：MQTT 代理场景下处理海量设备连接和消息路由
4. **Service Mesh 数据面**：作为轻量级 Sidecar 或边缘代理处理东西向/南北向流量

## 安装

```bash
# 下载 Easegress 单二进制
wget https://github.com/megaease/easegress/releases/latest/download/easegress-server-linux-amd64.tar.gz
tar xzf easegress-server-linux-amd64.tar.gz
./bin/easegress-server -f ./config/easegress-server.yaml

# Kubernetes Helm 部署
helm repo add easegress https://megaease.github.io/easegress
helm install easegress easegress/easegress -n easegress --create-namespace
```

## 对比

| 特性 | Easegress | Envoy | Kong | APISIX |
|------|-----------|-------|------|--------|
| 过滤器链 | ✅ 灵活 | Filter Chain | Plugin | Plugin |
| Wasm 扩展 | ✅ | ✅ | ⚠️ | ✅ |
| MQTT 代理 | ✅ | ❌ | ❌ | ⚠️ |
| Service Mesh | ✅ | ✅ 核心 | ❌ | ⚠️ |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[kairos]] — Kairos
- [[kaito]] — KAITO
- [[youki]] — youki
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- easegress
- [[实体/cncf-infrastructure.md|[[CNCF 基础设施与混沌工程项目全景|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
