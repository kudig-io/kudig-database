---
title: 入口控制器
description: Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller
  实现，需要...
summary: Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller
  实现，需要...
category: dictionary
tags:
- k8s
- glossary
- ingress
- networking
- nginx
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 入口控制器 是什么
- Ingress Controller 详解
trigger_keywords:
- 入口控制器
- Ingress Controller
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 入口控制器

> **英文名**: Ingress Controller

## 概述

Ingress Controller 是实现 Ingress 规则的实际组件。Kubernetes 本身不包含 Ingress Controller 实现，需要用户部署第三方控制器来处理 HTTP/HTTPS 路由。

## 核心概念/原理

### 主流 Ingress Controller

- **Nginx Ingress Controller**：最流行，基于 Nginx，功能丰富。
- **Traefik**：自动服务发现，支持多种协议。
- **Kong Ingress Controller**：基于 Kong API 网关。
- **HAProxy Ingress**：基于 HAProxy 的高性能方案。

### 工作原理

Ingress Controller 监听 Ingress 资源的变化，动态更新自身的反向代理配置，将外部流量路由到对应的后端 Service。

## 关键机制或特性

- Ingress Controller 通常以 Deployment + Service（LoadBalancer/NodePort）方式部署。
- 支持通过注解（annotations）扩展路由能力（限流、CORS、重写等）。
- 一个集群可以部署多个 Ingress Controller，通过 `ingressClassName` 区分。

## 使用场景与最佳实践

- 生产环境部署高可用的 Ingress Controller（多副本 + PDB）。
- 使用 HPA 根据流量自动扩缩 Ingress Controller。
- 配置请求限流和 WAF 规则保护后端服务。
- 考虑迁移到 Gateway API 获得更强大的路由能力。

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Ingress Controller（Deployment 多副本 + PDB）     │   │
│  │  - 监听 Ingress / Service / EndpointSlice         │   │
│  │  - 将 Ingress 规则翻译为数据面配置                │   │
│  │  （Nginx/Envoy/Traefik/Haproxy 等）               │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  暴露方式：LoadBalancer / NodePort / HostNetwork  │   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  客户端 ──▶ LB/NodePort ──▶ Ingress 数据面 ──▶ Service ──▶ Pod│
│                                     │                  │
│                          （限流/WAF/重写/认证中间件）     │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/ingress-nginx 为例）

| 模块 | 路径 | 职责 |
|------|------|------|
| 控制器 | `internal/ingress/controller/` | Ingress 监听与事件处理 |
| 翻译器 | `internal/ingress/nginx/` | Ingress → nginx.conf 模板渲染 |
| 状态同步 | `internal/ingress/status/` | LB 地址回写 Ingress Status |
| 中间件 | `internal/ingress/annotations/` | 注解解析（限流/重写/CORS） |

### 流量路径与关键点

1. Ingress 资源创建后，Controller 通过 informer 感知并加入队列
2. 校验 Ingress（ingressClassName 匹配、规则合法性），失败写 Event
3. 翻译器生成数据面配置（nginx.conf/envoy route），热加载或 ADS 推送
4. 数据面根据 host/path 规则路由到后端 Service，执行中间件链
5. LB 地址回写 Ingress `.status.loadBalancer`，供 DNS 绑定

## 生产案例

### 案例 1：Ingress Controller 热加载导致连接尖峰

| 时间 | 事件 |
|------|------|
| 10:00 | 频繁发布 Ingress 变更，nginx 每次 reload 产生 502 尖峰 |
| 10:15 | 定位为默认 `nginx.ingress.kubernetes.io/reload` 模式全量 reload |
| 10:30 | 切换为 lua-nginx-module 动态配置（lua reload 无连接中断） |

**根因**：nginx reload 会重开 worker 进程，存量长连接短暂中断；高频变更场景放大影响。

**修复命令**：
```bash
# 查看 reload 频率与错误 🟢 只读
kubectl logs deploy/ingress-nginx-controller | grep -E "reload|502" | tail -20
# 开启动态 reload（ConfigMap）🟡 中风险
kubectl -n ingress-nginx edit cm ingress-nginx-controller
# allow-snippet-annotations: "true"  （谨慎，安全风险）
# 或升级支持 lua 动态配置的版本
```

### 案例 2：多 Ingress Controller 资源争抢

**现象**：集群部署两个 Ingress Controller（Nginx + Traefik），Ingress 资源被错误控制器接管。

**诊断**：IngressClass 未指定或两个控制器都未配置 `ingress-class` 过滤，导致双写冲突与行为不一致。

**修复**：统一使用 `ingressClassName` 显式指定控制器归属；Controller 配置 `--ingress-class` 只处理匹配资源；存量 Ingress 迁移时逐个更新并验证。

## 对比评测

| 维度 | Ingress-nginx | Envoy Gateway | Traefik | HAProxy Ingress |
|------|--------------|---------------|---------|-----------------|
| 性能 | 中（reload 模型） | 高（xDS 动态） | 中 | 高 |
| 配置动态性 | 低（reload） | 高（ADS） | 高（热更新） | 高（reload 快） |
| 生态 | 最大 | 增长中 | 大 | 中 |
| 适用场景 | 通用 | 企业级/多协议 | 边缘/轻量 | 高性能 |

**选型建议**：通用默认选 Ingress-nginx；需要动态配置与多协议选 Envoy Gateway；边缘轻量选 Traefik；极致性能选 HAProxy。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 502/504 | `kubectl logs ds/ingress-nginx | grep -E "502|504"` | 后端 Pod 异常或超时配置 |
| 规则不生效 | `kubectl get ingress -o yaml` 看 Event | ingressClassName 不匹配 |
| reload 尖峰 | 监控 reload 次数 | 变更过于频繁 |
| 证书错误 | 检查 Secret 与 TLS 配置 | 证书过期或 SNI 不匹配 |

## 生产部署清单

- [ ] 多副本 + PDB + HPA 保障高可用
- [ ] 变更频率控制与 reload 策略评估
- [ ] 统一 ingressClassName 管理多控制器共存
- [ ] 配置访问日志采集与 WAF/限流加固
- [ ] 规划 Gateway API 迁移路径

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 控制器故障导致入口全挂 | 立即回滚最近变更并检查数据面健康 |
| P1 | Ingress API 版本演进 | 验证 v1 语义兼容性与注解迁移 |
| P2 | 从 Ingress 迁移 Gateway API | 制定双轨运行与灰度迁移清单 |

## 面试要点

> 以下 Q&A 覆盖 Ingress Controller 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Ingress 资源与 Ingress Controller 的关系？**
   A：Ingress 只是声明式配置（规则），本身不处理流量；Controller 是实际的数据面管理者——监听 Ingress/Service/EndpointSlice 变化，将规则翻译为自身数据面（nginx/envoy/haproxy）配置并执行路由。没有 Controller，Ingress 资源不产生任何效果。

2. **Q：nginx reload 模型为什么会产生连接中断？**
   A：nginx reload 会优雅关闭旧 worker 并启动新 worker 加载新配置，存量长连接需在旧 worker 生命周期内完成迁移或被动中断；高频变更时中断被放大。解决思路：减少变更频率、使用支持动态配置的实现（lua/Envoy xDS）、或采用连接迁移机制。

3. **Q：一个集群部署多个 Ingress Controller 如何避免冲突？**
   A：通过 IngressClass 解耦：每个 Ingress 用 `ingressClassName` 声明归属，Controller 启动参数 `--ingress-class` 只接管匹配的资源；IngressClass 还可绑定 `parameters` 引用自定义配置（如限流模板），实现多控制器隔离共存。

## 参考链接

- [Ingress Controller - Official Documentation](https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/)

## Related

[[17-系统基础/06-知识字典/networking/ingress-controllers.md|Ingress Controllers]]


<!-- risk-assessed -->
