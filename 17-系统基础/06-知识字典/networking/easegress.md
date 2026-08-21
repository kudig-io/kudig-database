---
title: Easegress 流量编排
description: Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Serv...
summary: Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Serv...
category: dictionary
tags:
- k8s
- glossary
- networking
- gateway
- service-mesh
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Easegress 流量编排 是什么
- Easegress 详解
trigger_keywords:
- Easegress 流量编排
- Easegress
- dictionary
prerequisites:
- kubernetes
---



# Easegress 流量编排（Easegress）

## 概述

Easegress 是 MegaEase 开源的 CNCF Sandbox 项目，提供全场景的流量编排能力，集 API 网关、服务网格 Sidecar、Service Mesh Controller 于一体，支持 HTTP/TCP/MQTT 等多协议。

## 核心概念/原理

- **全场景**：API Gateway + Service Mesh + Serverless Runtime
- **多协议**：HTTP/2、gRPC、WebSocket、MQTT、TCP
- **CNCF Sandbox**：MegaEase 主导
- **Go 编写**：高性能低资源占用

## 关键机制或特性

- Pipeline 流量处理管道
- Filter 链式过滤器（限流/认证/重试/路由等）
- 服务注册与发现（K8s/Consul/Eureka/Nacos）
- 分布式一致性（基于 Raft）
- Serverless Runtime（Wasm + 函数运行时）
- Prometheus 指标导出

## 使用场景与最佳实践

- API 网关和反向代理
- 微服务的流量治理
- MQTT IoT 设备流量管理
- Serverless 函数的网关层
- 传统系统现代化改造的流量层

## 参考链接

- https://megaease.com/easegress/
- https://github.com/megaease/easegress

## 架构深度解析

### 核心架构

```
┌─────────────────────────────────────────────────────┐
│              Easegress (Go, 分布式)                │
├─────────────────────────────────────────────────────┤
│  ┌───────────┐  ┌────────────┐  ┌─────────────┐  │
│  │ Supervisor │  │ Pipeline   │  │ Cluster     │  │
│  │ (对象管理) │  │ (Filter链) │  │ (Raft/etcd) │  │
│  └─────┬─────┘  └──────┬─────┘  └──────┬──────┘  │
│        │               │               │         │
│  ┌─────▼───────────────▼───────────────▼─────┐  │
│  │         Traffic Objects (CRD-like)         │  │
│  │  HTTPServer / HTTPRoute / Service / Filter │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（megaease/easegress）

| 模块 | 路径 | 职责 |
|------|------|------|
| Supervisor | `pkg/supervisor/` | 对象生命周期管理、热更新 |
| Pipeline | `pkg/object/pipeline/` | Filter 链编排与执行 |
| Cluster | `pkg/cluster/` | Raft 共识、分布式配置同步 |
| HTTP Server | `pkg/object/httpserver/` | HTTP/HTTPS 服务与路由 |
| Filter | `pkg/filter/` | 内置 Filter（限流/熔断/JWT） |

### 请求处理流程

1. 客户端请求 → HTTPServer 监听端口
2. 路由匹配（Host/Path/Method）→ 目标 Pipeline
3. Pipeline 执行 Filter 链（鉴权→限流→熔断→转发）
4. Service 负载均衡选择后端
5. 响应回传 + 指标采集

## 生产案例

### 案例 1：Raft 集群脑裂导致配置不一致

| 时间 | 事件 |
|------|------|
| 03:00 | 网络分区导致 3 节点 Easegress 集群分裂为 2+1 |
| 03:05 | 少数派节点拒绝服务（无法选举 Leader） |
| 03:10 | 网络恢复后集群自动收敛，但部分配置丢失 |
| 03:20 | 修复：启用 `--cluster-join-urls` 静态发现，避免动态发现失败 |

**修复命令**：
```bash
# 检查集群状态 🟢 只读
curl http://localhost:2380/members | jq .
# 查看当前 Leader 🟢 只读
curl http://localhost:2380/status | jq .leader
```

### 案例 2：Pipeline Filter 顺序错误导致 JWT 验证绕过

**现象**：未认证请求可以访问受保护 API。

**诊断**：Pipeline 中 `proxy` Filter 在 `jwtVerifier` 之前执行，请求未经鉴权即被转发。

**修复**：调整 Pipeline filters 顺序：`jwtVerifier` → `rateLimiter` → `proxy`。

## 对比评测

| 维度 | Easegress | NGINX | Envoy |
|------|----------|-------|-------|
| 架构模式 | Filter 管道（Go） | 静态配置模块 | xDS 动态配置 |
| 流量编排 | 支持（编排工作流） | 有限 | 支持（Wasm/Lua） |
| 多协议 | HTTP/2、WebSocket、MQTT、TCP/UDP | HTTP/WebSocket | HTTP/2、gRPC、TCP |
| 配置热更新 | 支持（无中断） | 需 reload | 支持（xDS） |
| 运维复杂度 | 低（单二进制） | 低 | 中 |

**选型建议**：需要流量编排与多协议接入（非 HTTP 场景）时选 Easegress；标准七层负载均衡优先 Envoy；存量 NGINX 环境无需迁移。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 请求 502 | `egctl get traffic`；查看 Filter 日志 | 后端无健康节点、Filter 顺序错误 |
| 灰度流量异常 | `egctl get ingress`；检查匹配规则 | 路由规则优先级冲突 |
| 配置不生效 | `egctl get config`；检查版本 | 配置未提交、集群未同步 |
| 限流误伤 | 查看限流 Filter 配置 | 令牌桶参数过小 |

## 生产部署清单

- [ ] 控制面高可用（≥3 副本，etcd 存储）与 PDB
- [ ] Filter 管道已做性能压测（限流/熔断/灰度链路）
- [ ] 证书管理（自动轮转）与审计日志已配置
- [ ] 监控接入（Prometheus metrics：`eg_requests_total` 等）
- [ ] 灰度发布演练完成（权重路由 + 回滚）

## 常见误区与设计要点

- **误区 1**：把 Easegress 当纯反向代理用，忽略其编排能力——复杂流量场景应先设计 Filter 管道。
- **误区 2**：Filter 顺序随意排列——限流/熔断应前置，安全认证在路由之前。
- **误区 3**：单实例部署无 HA——生产必须 ≥3 副本并配 PDB，配置存储用 etcd。
- **设计要点**：灰度发布优先用权重路由 + 健康检查联动；多协议接入按 Filter 分管道隔离，避免互相影响。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 集群全部节点不可用 | 从备份恢复 etcd 数据，重建集群 |
| P1 | 单节点 Raft 同步失败 | 移除并重新加入节点 |
| P2 | Filter 延迟增加 | 禁用非关键 Filter，性能分析 |

## 性能参考

- 单实例吞吐：HTTP 转发 5-10w QPS（4C8G），TLS 终结约 3-5w。
- 延迟：P99 < 5ms（纯转发），多 Filter 管道每级增加 0.5-1ms。
- 扩展：无状态水平扩容，前置 L4 LB 分发；etcd 集群独立部署保证配置一致性。
- 容量规划：按峰值 QPS 预留 30% 冗余，连接数受 fd 限制（`ulimit -n`）。
- 性能瓶颈定位：用 `egctl profile` 与 pprof 分析 Filter 热点，优先优化耗时 Top3 管道。

## 面试要点

1. **Q：Easegress 与 APISIX/Kong 的核心差异是什么？**
   A：Easegress 基于 Go 实现，内置 Raft 分布式共识，无需外部 etcd；APISIX 基于 OpenResty（Nginx+Lua），依赖 etcd 存储配置；Kong 基于 OpenResty，支持插件生态。Easegress 优势在于原生分布式、Pipeline 编排灵活；APISIX 优势在于极致性能和丰富插件。

2. **Q：Easegress 的 Pipeline 机制如何工作？**
   A：Pipeline 是有序的 Filter 链：每个 Filter 实现 `Handle(ctx) (code, resp)` 接口。请求按顺序经过各 Filter，任一 Filter 返回非 Continue 状态即短路。支持条件跳转（JumpIf）和子 Pipeline 引用，实现复杂流量编排。

3. **Q：如何在 K8s 中使用 Easegress？**
   A：① Helm 部署 Easegress 集群（3 节点）；② 通过 Easegress Portal 或 API 创建 HTTPServer/HTTPRoute；③ 使用 Service type=LoadBalancer 暴露；④ 可选：配合 Ingress Controller 模式自动转换 Ingress 资源。

## Related

- [[17-系统基础/06-知识字典/networking/traefik.md|Traefik]]
- [[17-系统基础/06-知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[17-系统基础/06-知识字典/networking/contour.md|Contour]]
