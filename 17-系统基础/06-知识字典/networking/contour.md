---
title: Contour Ingress 控制器
description: Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress
  和 Gatew...
summary: Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress
  和 Gatew...
category: dictionary
tags:
- k8s
- glossary
- networking
- ingress
- envoy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Contour Ingress 控制器 是什么
- Contour 详解
trigger_keywords:
- Contour Ingress 控制器
- Contour
- dictionary
prerequisites:
- kubernetes
---



# Contour Ingress 控制器（Contour）

## 概述

Contour 是 VMware 开源的 Kubernetes Ingress 控制器，基于 Envoy Proxy 构建，支持 Ingress 和 Gateway API，提供高性能的 L7 负载均衡和流量管理能力。

## 核心概念/原理

- **Envoy 驱动**：使用 Envoy 作为数据面，控制面用 Go 编写
- **双 API 支持**：同时支持 Kubernetes Ingress 和 Gateway API
- **HTTProxy CRD**：Contour 自定义的路由配置资源，支持丰富的流量策略
- **CNCF Sandbox**：CNCF 沙箱项目

## 关键机制或特性

- 动态 Envoy 配置（通过 xDS API）
- TLS 终止与 SNI 路由
- 流量分割（权重路由）用于金丝雀发布
- WebSocket / gRPC 代理
- 速率限制（集成 ratelimit 服务）
- Contour 支持多 Gateway 部署

## 使用场景与最佳实践

- 替代 nginx-ingress 的高性能 Ingress 方案
- 需要 Envoy 级别流量控制的场景
- Gateway API 的早期采纳
- 金丝雀发布和流量镜像需求

## 参考链接

- https://projectcontour.io/
- https://github.com/projectcontour/contour

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              Contour Control Plane                  │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Ingress/    │  │ xDS Server   │  │ Leader    │  │
│  │ HTTPProxy   │  │ (Envoy API)  │  │ Election  │  │
│  │ Controller  │  │              │  │           │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │        Envoy Data Plane (DaemonSet)         │  │
│  │  (LDS/RDS/CDS/EDS 动态配置)                │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（projectcontour/contour）

| 模块 | 路径 | 职责 |
|------|------|------|
| Controller | `internal/controller/` | Ingress/HTTPProxy CRD 监听 |
| DAG Builder | `internal/dag/` | 路由规则转有向无环图 |
| xDS Server | `internal/xds/` | Envoy 配置生成与推送 |
| Gateway API | `internal/gatewayapi/` | Gateway API 资源处理 |
| Status | `internal/status/` | CRD status 更新 |

### 配置下发流程

1. Contour 监听 Ingress/HTTPProxy/Gateway CRD 变更
2. DAG Builder 构建路由有向无环图
3. xDS Server 生成 Envoy 配置（LDS/RDS/CDS/EDS）
4. 通过 gRPC 流式推送到 Envoy Sidecar
5. Envoy 热加载配置，零停机更新

## 生产案例

### 案例 1：HTTPProxy 路由冲突导致流量异常

| 时间 | 事件 |
|------|------|
| 15:00 | 新增 HTTPProxy 资源后，部分 API 返回 404 |
| 15:10 | 检查 Contour 日志：路由 DAG 构建警告 |
| 15:20 | 根因：两个 HTTPProxy 声明了相同的 `/api` 前缀，优先级未明确 |
| 15:30 | 修复：使用 `spec.includes` 委托或调整 `spec.routes.conditions` 精确匹配 |

**修复命令**：
```bash
# 查看 HTTPProxy 状态 🟢 只读
kubectl get httpproxy -A -o wide
# 检查 Contour 日志 🟢 只读
kubectl logs -n projectcontour deploy/contour --tail=50
# 查看 Envoy 配置 🟢 只读
kubectl exec -n projectcontour ds/envoy -- curl -s localhost:9001/config_dump | jq '.configs[]'
```

### 案例 2：xDS 推送延迟导致配置不一致

**现象**：更新 Ingress 后，部分 Envoy 实例仍使用旧配置。

**诊断**：Contour xDS Server 队列积压，gRPC 连接数超过限制。

**修复**：增加 Contour 副本数，调整 `--xds-address` 和 `--xds-port` 参数，启用 xDS 增量推送。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有 Envoy 实例断开 xDS 连接 | 重启 Contour，检查证书有效性 |
| P1 | 单路由规则不生效 | 检查 HTTPProxy status，确认 DAG 构建成功 |
| P2 | 配置推送延迟 > 5s | 扩容 Contour，检查 CRD 数量 |

## 面试要点

1. **Q：Contour 与 Nginx Ingress Controller 的核心差异？**
   A：Contour 基于 Envoy 数据面，通过 xDS 动态配置实现零停机更新；Nginx IC 基于 Nginx，配置变更需要 reload（短暂连接中断）。Contour 支持 HTTPProxy CRD（比 Ingress 更强大）和 Gateway API；Nginx IC 生态更成熟、社区更大。Contour 适合需要高级流量管理的场景。

2. **Q：Contour 的 DAG Builder 如何工作？**
   A：DAG Builder 将所有 Ingress/HTTPProxy/Gateway 资源解析为有向无环图：节点代表路由条件（host/path/header），边代表转发目标。构建过程：① 解析所有资源；② 检测冲突和循环引用；③ 生成 Envoy 可理解的虚拟主机和路由配置。实现路径：`internal/dag/`。

3. **Q：如何从 Nginx Ingress 迁移到 Contour？**
   A：① 并行部署 Contour + Envoy；② 使用 HTTPProxy 的 `spec.includes` 逐步迁移路由；③ 通过 DNS 权重或 LB 分流验证；④ 确认无问题后下线 Nginx IC；⑤ 注意：Nginx 特有注解需转换为 HTTPProxy 字段。

## Related

- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[17-系统基础/06-知识字典/networking/traefik.md|Traefik]]
