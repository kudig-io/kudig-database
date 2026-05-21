---
title: Easegress
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- envoy
- docker
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Easegress 是什么
- 如何 Easegress
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Easegress
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

title: Easegress
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- envoy
- docker
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Easegress 是什么
- 如何 Easegress
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Easegress
- cncf
- landscape
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
# Easegress

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://megaease.com/easegress/ |
| **GitHub** | https://github.com/easegress-io/easegress |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Easegress 是一个云原生的全生命周期 API 编排和流量网关，提供高可用、高性能的流量调度能力。它支持丰富的流量治理功能，包括 API 编排、金丝雀发布、限流熔断、服务发现、WebSocket、MQTT 代理等。Easegress 采用过滤器链（Filter Pipeline）架构，用户可以灵活组合过滤器实现复杂的流量处理逻辑。

### 核心特性

- **流量网关**: HTTP/HTTPS 反向代理、负载均衡、TLS 终止
- **API 编排**: 将多个后端 API 聚合为一个接口返回
- **过滤器链**: 可组合的请求/响应处理管道
- **高可用**: 内置 Raft 共识，多节点集群部署
- **多协议**: HTTP、gRPC、WebSocket、MQTT
- **Wasm 扩展**: 通过 WebAssembly 扩展自定义过滤器
- **FaaS 集成**: 内置 FaaS Controller，支持 Knative

---

## 快速开始

### 安装

```bash
# 下载
curl -LO https://github.com/easegress-io/easegress/releases/latest/download/easegress-server
chmod +x easegress-server

# 启动
./easegress-server --name node-1

# 或使用 Docker
docker run -d --name easegress \
  -p 2379:2379 -p 2380:2380 -p 2381:2381 \
  megaease/easegress:latest
```

### 创建 HTTP 管道

```yaml
name: demo-pipeline
kind: Pipeline
flow:
  - filter: rate-limiter
  - filter: proxy

filters:
  - name: rate-limiter
    kind: RateLimiter
    policies:
      - name: default
        timeoutDuration: 100ms
        limitRefreshPeriod: 1s
        limitForPeriod: 100

  - name: proxy
    kind: Proxy
    pools:
      - servers:
          - url: http://backend-1:8080
            weight: 80
          - url: http://backend-2:8080
            weight: 20
        loadBalance:
          policy: roundRobin

---
name: demo-server
kind: HTTPServer
port: 8080
rules:
  - paths:
      - path: /api/v1
        backend: demo-pipeline
```

### API 编排

```yaml
# 将多个 API 聚合为一个响应
name: aggregator-pipeline
kind: Pipeline
flow:
  - filter: api-aggregator

filters:
  - name: api-aggregator
    kind: APIAggregator
    pipelines:
      - name: user-api
        url: http://user-service/api/user/{id}
        method: GET
      - name: order-api
        url: http://order-service/api/orders?userId={id}
        method: GET
    # 返回合并后的 JSON:
    # { "user": {...}, "orders": [...] }
```

---

## 与其他方案对比

| 特性 | Easegress | NGINX | Envoy | Kong |
|:---|:---|:---|:---|:---|
| API 编排 | 内置聚合 | 不支持 | 不支持 | 插件 |
| 高可用 | Raft 共识 | 外部 | 需外部 | 外部 DB |
| Wasm 扩展 | 支持 | 不支持 | 支持 | 不支持 |
| 配置方式 | YAML/API | 配置文件 | xDS/API | Admin API |
| MQTT | 支持 | 不支持 | 不支持 | 插件 |

---

## 最佳实践

1. **过滤器组合**: 按职责拆分过滤器（限流→认证→路由→代理），保持管道清晰
2. **集群部署**: 生产环境部署 3+ 节点的 Raft 集群确保高可用
3. **Wasm 扩展**: 复杂的自定义逻辑使用 Wasm 过滤器实现，避免修改核心代码
4. **健康检查**: 为上游服务器配置主动健康检查
5. **监控**: 利用内置的 Prometheus metrics 监控流量和延迟

---

## 参考资源

- [Easegress 官方文档](https://megaease.com/easegress/)
- [Easegress GitHub](https://github.com/easegress-io/easegress)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
