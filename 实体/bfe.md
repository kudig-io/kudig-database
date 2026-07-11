---
title: BFE
description: '## 概述'
summary: 'BFE 是百度开源的现代化七层负载均衡器和反向代理，处理百度内部每天数万亿级别的请求。它提供高级流量路由、安全防护、可观测性等能力，支持 HTTP/HTTPS/HTTP2/QUIC 等协议，适合作为 Kubernetes [[Ingress|Ingress]] Controller 或独立的流量网关。'
category: entities
tags:
- k8s
- cncf
- networking
- bfe
- prometheus
- grafana
- ingress
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
- BFE 是什么
- 如何 BFE
trigger_keywords:
- BFE
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# BFE

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

BFE（Baidu Front End）是百度开源的 CNCF 沙箱项目，是一个企业级的前端代理和流量接入平台。它在百度内部承担着每秒数百万请求的流量接入任务。BFE 设计目标是提供比 Nginx/HAProxy 更好的运维体验和更丰富的流量管理功能，特别在多租户隔离、流量路由和协议转换方面有独到设计。BFE 支持 HTTP/HTTPS、HTTPS、WebSocket、gRPC、SPDY 等多种协议。

## Key Features（核心能力）

- **多租户架构**：内置租户隔离机制，支持多业务线共享同一实例
- **高级流量路由**：基于请求内容的细粒度路由（Header、Cookie、Query、Path）
- **协议转换**：支持 HTTP 到 gRPC、HTTP/2 到 HTTP/1.1 的协议转换
- **多协议支持**：HTTP/HTTPS/HTTP2/WebSocket/gRPC/TCP/UDP
- **WAF 集成**：内置 Web 应用防火墙功能
- **细粒度限流**：支持基于租户、路由、请求特征的细粒度速率限制

## 架构与工作原理

BFE 采用多租户核心设计：流量首先按租户（Tenant）分类，每个租户内通过路由规则（Route）分发到不同的子集群（Subcluster）。BFE 的路由模型支持基于请求 Header、Path、Cookie、Query 参数的多条件匹配。数据平面使用 Go 语言编写，利用 goroutine 实现高并发。BFE 支持热加载配置，无需重启即可更新路由规则。

## K8s 集成

BFE 可作为 Kubernetes 的 Ingress Controller 使用。通过 CRD 定义 BFE 特有的路由和租户配置，或通过标准 Ingress 资源集成。BFE 以 Deployment 部署，前端通过 LoadBalancer Service 或 NodePort 暴露。支持自动发现 K8s Service Endpoints 作为后端，通过健康检查自动剔除不健康节点。

## 生产用例

- **企业级流量接入**：大规模互联网应用的前端接入网关
- **多租户网关**：多业务线共享同一网关实例，逻辑隔离
- **协议适配**：前端 HTTPS 到后端 gRPC 的协议转换
- **灰度发布**：基于请求特征的细粒度流量分配

## 安装与快速开始

```bash
helm repo add bfe https://bfenetworks.github.io/bfe-helm-charts/
helm install bfe bfe/bfe -n bfe-system --create-namespace
```

## 对比替代方案

相比 Nginx，BFE 提供更强的多租户隔离和 Go 语言的可扩展性。相比 Envoy，BFE 更专注于流量接入场景且运维体验更好，但生态和社区不如 Envoy 活跃。

## Related

- [[meshery]] — Meshery
- [[knative]] — Knative
- [[konveyor]] — Konveyor
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- bfe
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
