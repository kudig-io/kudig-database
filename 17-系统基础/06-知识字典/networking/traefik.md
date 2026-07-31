---
title: Traefik
description: Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为
  Kubernetes...
summary: Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为 Kubernetes...
category: dictionary
tags:
- k8s
- glossary
- traefik
- ingress
- reverse-proxy
- gateway-api
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Traefik 是什么
- Traefik 详解
trigger_keywords:
- Traefik
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Traefik

> **英文名**: Traefik

## 概述

Traefik 是现代化的 HTTP 反向代理和负载均衡器，原生支持 Docker、Kubernetes、Consul 等多种后端。它作为 Kubernetes Ingress Controller 和 Gateway API 实现，以自动服务发现和实时配置更新著称。

## 核心概念/原理

### 核心架构

- **EntryPoints**：入口点（HTTP/HTTPS/TCP 端口）。
- **Routers**：路由规则（匹配 Host/Path/Header）。
- **Services**：后端服务（负载均衡组）。
- **Middlewares**：请求处理链（认证、限流、重定向等）。
- **Providers**：配置源（K8s Ingress/Gateway API/Docker 等）。

### 与 Nginx Ingress 对比

| 特性 | Traefik | Nginx Ingress |
|------|---------|---------------|
| 配置更新 | 热更新（无 reload） | reload |
| Dashboard | 内置 | 无 |
| Gateway API | 原生支持 | 支持 |
| 中间件 | 丰富的 Middleware | Annotation |

## 关键机制或特性

- **自动服务发现**：监听 K8s API 自动发现 Ingress/Gateway 资源。
- **Middleware 链**：RateLimit、CircuitBreaker、Auth、Compress 等。
- **Let's Encrypt**：自动签发和续期 TLS 证书。
- **Dashboard**：内置 Web UI 查看路由和中间件状态。
- **TCP/UDP**：支持非 HTTP 协议的流量代理。

## 使用场景与最佳实践

- 中小集群可选择 Traefik 替代 Nginx Ingress Controller。
- 使用 Middleware 实现限流、认证、压缩等功能。
- 启用自动 TLS（Let's Encrypt）简化证书管理。
- 配合 Gateway API 实现更精细的流量管理。
- 使用 Traefik Pilot 或 Prometheus 监控代理指标。

## 参考链接

- [Traefik Official](https://doc.traefik.io/traefik/)

## 架构深度解析

### 核心架构

```
┌─────────────────────────────────────────────────────┐
│              Traefik (Go, Cloud Native)             │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Entrypoints │  │ Routers      │  │ Services  │  │
│  │ (监听端口)  │  │ (路由规则)   │  │ (后端池)  │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │     Middlewares (Filter Chain)              │  │
│  │  RateLimit / Auth / Headers / Retry         │  │
│  └──────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────┤
│  Providers: K8s CRD / Ingress / File / Docker     │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（traefik/traefik）

| 模块 | 路径 | 职责 |
|------|------|------|
| Entrypoints | `pkg/server/router/` | 监听器与 TLS 配置 |
| Routers | `pkg/router/` | 路由规则匹配与分发 |
| Middlewares | `pkg/middlewares/` | 中间件链（限流/鉴权/头） |
| K8s Provider | `pkg/provider/kubernetes/` | CRD/Ingress 监听与转换 |
| ACME | `pkg/provider/acme/` | Let's Encrypt 自动证书 |

### 请求处理流程

1. 客户端请求 → Entrypoint（如 :80, :443）
2. TLS 终止（若配置）→ 明文 HTTP
3. Router 匹配（Host/Path/Method/Header）
4. Middleware 链执行（限流→鉴权→头修改）
5. Service 负载均衡选择后端
6. 转发请求 → 响应回传

## 生产案例

### 案例 1：ACME 证书申请失败

| 时间 | 事件 |
|------|------|
| 10:00 | 新域名 HTTPS 访问失败，证书未签发 |
| 10:10 | 检查 Traefik 日志：ACME challenge 超时 |
| 10:20 | 根因：HTTP-01 challenge 路径被其他 Ingress 拦截 |
| 10:30 | 修复：确保 `/.well-known/acme-challenge/` 路径优先路由到 Traefik |

**修复命令**：
```bash
# 检查证书状态 🟢 只读
kubectl exec -n traefik deploy/traefik -- traefik version
# 查看 ACME 日志 🟢 只读
kubectl logs -n traefik deploy/traefik | grep -i acme
# 检查证书存储 🟢 只读
kubectl get secret -n traefik acme-cert -o yaml
```

### 案例 2：Middleware 顺序错误导致 CORS 失败

**现象**：前端跨域请求被拒绝，预检请求返回 404。

**诊断**：CORS Middleware 在 Auth Middleware 之后执行，预检请求（无 Auth 头）被拒绝。

**修复**：调整 Middleware 顺序：`cors` → `auth` → `ratelimit`。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 所有 Entrypoint 不可用 | 回滚 Traefik 版本，检查配置语法 |
| P1 | 单路由规则不生效 | 检查 Router 优先级和匹配条件 |
| P2 | ACME 证书即将过期 | 检查 ACME 配置，手动触发续签 |

## 面试要点

1. **Q：Traefik 与 Nginx Ingress Controller 的核心差异？**
   A：Traefik 原生支持自动服务发现（K8s/Docker/Consul），配置动态生效无需 reload；Nginx IC 需要 reload 配置（短暂中断）。Traefik 内置 Let's Encrypt 自动证书；Nginx 需要 cert-manager 配合。Traefik 适合云原生动态环境；Nginx 适合需要极致性能和成熟生态的场景。

2. **Q：Traefik 的 Middleware 链如何工作？**
   A：Middleware 是有序的 HTTP 处理器链：每个 Middleware 实现 `ServeHTTP` 接口，可修改请求/响应或短路返回。常见 Middleware：RateLimit（令牌桶限流）、BasicAuth/DigestAuth（鉴权）、Headers（安全头）、Retry（重试）。通过 CRD 的 `spec.middlewares` 字段引用。

3. **Q：如何在 Traefik 中实现金丝雀发布？**
   A：使用 Traefik 的加权路由：① 创建两个 Service（stable/canary）；② 配置 Router 的 `services` 字段，设置 weight（如 90/10）；③ 或使用 Traefik 的 Canary Deployment 功能（基于 Header 的流量分割）；④ 配合 Flagger/Argo Rollouts 实现自动化渐进式发布。

## Related

- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|LoadBalancer]]


<!-- risk-assessed -->
