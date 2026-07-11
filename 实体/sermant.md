---
title: Sermant (entities)
description: '## 概述'
summary: 'Sermant 是华为开源的基于 Java Agent 的无代理服务网格方案，通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。它支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。'
category: entities
tags:
- k8s
- cncf
- service-mesh
- sermant
- prometheus
- grafana
- istio
- cilium
- opa
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Sermant 是什么
- 如何 Sermant
trigger_keywords:
- Sermant
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Sermant

> **CNCF 状态**: Sandbox | **类别**: [[Service|Service]]Service Mesh）|Service Mesh]] | **主要语言**: Java

## 概述

Sermant 是华为开源的基于 Java Agent 的无代理（Agentless）服务网格方案，2022 年加入 CNCF Sandbox。它通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。Sermant 支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。它是从 ServiceComb 生态演进而来的轻量级治理方案。

## 核心特性

- **字节码增强**: 无侵入式增强 Java 应用，无需修改业务代码
- **无 Sidecar**: 直接在 JVM 内运行，消除 Sidecar 代理开销
- **插件化架构**: 按需加载治理插件（路由、限流、监控等）
- **配置热更新**: 通过 Sermant Backend 动态调整治理策略，无需重启
- **多框架支持**: Spring Cloud、Dubbo、gRPC 等 Java 微服务框架
- **监控集成**: 上报治理指标到 Prometheus，追踪数据到 Zipkin

## 架构

Sermant 由三部分组成。Agent（sermant-agent）通过 `-javaagent` 参数挂载到 Java 应用 JVM 中，利用 Java Instrumentation API 在类加载时增强字节码。框架核心（sermant-framework）提供插件加载、字节码增强、配置管理和服务治理框架。Backend（sermant-backend）是控制面，提供配置管理、心跳监控和治理策略下发。插件（sermant-plugins）按功能组织，如路由插件增强 HTTP 客户端实现流量路由，限流插件增强服务入口实现 QPS 控制。

## Kubernetes 集成

在 Kubernetes 中，Sermant Agent 通过 Init Container 或镜像构建阶段注入到 Java 应用 Pod。无需 Sidecar 代理容器，降低资源消耗约 30-50%。服务注册发现通过 Sermant 插件连接注册中心（如 ZooKeeper、Nacos）。Backend 以 Deployment 形式部署在集群中，通过 ConfigMap 或后端服务下发治理策略。与 Kubernetes Service 和 Endpoint 配合实现负载均衡。

## 生产使用场景

1. **Java 微服务迁移**: 从传统微服务框架迁移到云原生架构，保持治理能力
2. **Sidecar 替代**: 在 Sidecar 代理性能开销不可接受时，使用 Sermant 降低开销
3. **灰度发布**: 通过路由插件实现 Java 服务的标签路由和金丝雀发布
4. **限流降级**: 在服务入口通过限流插件实现 QPS 控制和熔断降级

## 安装

```bash
# 下载 Sermant Agent
wget https://github.com/huaweicloud/Sermant/releases/latest/sermant-agent.zip
unzip sermant-agent.zip
# 挂载到 Java 应用
java -javaagent:/path/to/sermant-agent/agent/sermant-agent.jar \
  -Dsermant.plugins=flowcontrol,router,service-registry \
  -jar application.jar
# 部署 Backend
kubectl apply -f https://raw.githubusercontent.com/huaweicloud/Sermant/main/sermant-backend/deploy/kubernetes.yaml
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Sermant** | 无 Sidecar、低开销 | 仅支持 Java |
| Istio Sidecar | 语言无关、功能全面 | 资源开销大、延迟增加 |
| Spring Cloud | Java 原生、成熟 | 需修改代码、耦合 SDK |
| Dubbo | 高性能 RPC | 需引入 Dubbo 框架 |

## 架构定位

在 CNCF 生态中，Sermant 属于 **Service Mesh** 类别，代表了 Sidecar-less 服务网格的发展方向。它与 Istio、Kmesh 等项目互补，专注于 Java 场景。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[deployment]]
- networking.md|cilium-ebpf-networking]]

## Related

- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[serverless-devs]] — Serverless Devs
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- sermant
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
