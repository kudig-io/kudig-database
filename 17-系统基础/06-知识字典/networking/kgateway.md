---
title: KGateway API 网关
description: KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于
  Envoy Proxy...
summary: KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于 Envoy
  Proxy...
category: dictionary
tags:
- k8s
- glossary
- networking
- gateway
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
- KGateway API 网关 是什么
- KGateway 详解
trigger_keywords:
- KGateway API 网关
- KGateway
- dictionary
prerequisites:
- kubernetes
---



# KGateway API 网关（KGateway）

## 概述

KGateway（原 Gloo Edge/Gloo Gateway）是 Solo.io 开源的 Kubernetes API 网关，基于 Envoy Proxy，完整支持 Gateway API，提供丰富的流量管理和安全功能。

## 核心概念/原理

- **Envoy 驱动**：基于 Envoy 的高性能网关
- **Gateway API**：完整支持 Kubernetes Gateway API
- **多协议**：HTTP/gRPC/WebSocket/TCP
- **Solo.io**：企业级网关方案

## 关键机制或特性

- Gateway API 完整实现
- 路由规则和流量分割
- 速率限制和熔断
- TLS 终止和 mTLS
- WAF（Web Application Firewall）集成
- AI Gateway 功能（LLM 路由/Token 管理）
- 与 Grafana/Prometheus 可观测性集成

## 使用场景与最佳实践

- Kubernetes 入口流量管理
- API 网关和反向代理
- 微服务的统一入口
- Gateway API 的生产部署
- AI 应用的 API 网关

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  kgateway 控制面（kgateway + Gloo 扩展）           │   │
│  │  - Gateway API 控制器（Gateway/HTTPRoute 等）      │   │
│  │  - 策略控制器（RoutePolicy/UpstreamPolicy）        │   │
│  │  - AI Gateway 控制器（LLM 路由/Token 配额）        │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  kgateway Proxy（数据面，Envoy）                   │   │
│  │  - xDS 配置推送（ADS）                             │   │
│  │  - L7 路由 / 限流 / 认证 / 可观测性                 │   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  客户端 ──▶ Gateway (LB/NodePort) ──▶ Envoy ──▶ 后端服务  │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kgateway-dev/kgateway）

| 模块 | 路径 | 职责 |
|------|------|------|
| Gateway 控制器 | `internal/gateway/` | GatewayClass/Gateway/HTTPRoute 翻译 |
| 策略系统 | `internal/policies/` | RoutePolicy、UpstreamPolicy 等策略处理 |
| xDS 翻译 | `internal/xds/` | 将 K8s 资源编译为 Envoy xDS 配置 |
| AI Gateway | `internal/ai/` | LLM 路由、Token 限流、模型回退 |
| 可观测性 | `internal/observability/` | 指标/追踪/访问日志集成 |

### 请求处理流程

1. 客户端请求到达 Gateway 监听地址（LB/NodePort/HostPort）
2. Envoy 按 VirtualHost/Route 匹配 HTTPRoute 规则
3. 策略链执行：认证（JWT/OIDC）→ 限流（RLS）→ 路由（权重/镜像）
4. 转发至 Upstream（Endpoints 由 EndpointSlice 动态下发）
5. 访问日志与指标输出至 Prometheus/Grafana/OTel

## 生产案例

### 案例 1：HTTPRoute 变更后 503 风暴

| 时间 | 事件 |
|------|------|
| 11:00 | 上线新 HTTPRoute 规则（将 20% 流量切到新版本），随即 503 爆发 |
| 11:05 | Envoy 日志显示 upstream 连接建立失败 |
| 11:10 | 定位为新版本服务尚无 Ready Endpoints，权重路由已下发 |
| 11:15 | 回滚 HTTPRoute，503 消失 |
| 11:30 | 先扩容新版本并确认 Ready，再灰度切流 |

**根因**：HTTPRoute 权重变更即时生效于数据面，而新后端未就绪；缺少"目标服务 Ready 才允许路由"的保护机制。

**修复命令**：
```bash
# 查看 HTTPRoute 状态 🟢 只读
kubectl get httproute -n app -o wide
# 查看 Envoy upstream 状态 🟢 只读
kubectl exec deploy/kgateway-proxy -n kgateway-system -- curl localhost:19000/config_dump | grep -A5 "hosts"
# 回滚路由变更 🟡 中风险
kubectl rollout undo httproute/orders -n app 2>/dev/null || kubectl apply -f httproute-backup.yaml
```

### 案例 2：AI Gateway Token 配额误判导致 LLM 调用被限

**现象**：AI 应用调用 LLM 频繁 429，业务方反馈配额充足。

**诊断**：AI Gateway 按请求头中的 model 字段路由并统计 token；部分请求未携带模型头被归入默认池，挤占公共配额。

**修复**：为各业务方配置独立 `UpstreamPolicy` 与 token 配额；校验模型头必填并配置默认回退模型；监控 token 使用报表核对与实际账单一致性。

## 对比评测

| 维度 | kgateway | Envoy Gateway | Traefik |
|------|----------|---------------|---------|
| API | Gateway API 原生 | Gateway API | 自定义 CRD |
| AI Gateway | ✅ 内置 | ❌ | ❌ |
| 策略模型 | RoutePolicy 声明式 | 有限 | 中间件 |
| 生态（Solo 系） | Gloo 兼容 | 独立 | 独立 |
| 适用场景 | 企业级 + AI 入口 | 通用网关 | 轻量入口 |

**选型建议**：需要 AI Gateway 能力（LLM 路由/Token 管理）选 kgateway；标准 Gateway API 场景 Envoy Gateway 更聚焦；边缘/轻量场景选 Traefik。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 503 | `kubectl get endpointslices -n app` | 后端未 Ready |
| 404 | `kubectl get httproute -o yaml` | 路由匹配条件错误 |
| 429 | 检查 RateLimitPolicy | Token/请求配额耗尽 |
| 路由不生效 | 查看 Proxy 状态与 Accepted 条件 | 控制器未同步或翻译失败 |

## 生产部署清单

- [ ] 切流前确认目标后端 Ready，配置就绪门禁
- [ ] AI Gateway 场景统一模型头规范与配额模型
- [ ] 数据面资源规划（Envoy 内存与连接数）
- [ ] 策略变更走 GitOps 并保留快速回滚通道
- [ ] 访问日志全量采集，审计关键路由变更

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 路由变更引发 503/5xx 风暴 | 立即回滚最近变更并检查后端就绪 |
| P1 | Gateway API 版本升级（v1 → v1.1/v1.2） | 预发验证 CRD 兼容性与翻译差异 |
| P2 | 从 Ingress/自定义 API 迁移 Gateway API | 制定双轨运行与迁移清单 |

## 运维要点

- 基于 Gateway API 管理入口流量，升级前先核对 GatewayClass 控制器版本兼容性。
- 使用 `kubectl get gateway -A` 与 `kubectl describe httproute` 快速定位路由状态问题。
- 灰度发布优先用 HTTPRoute 权重路由，配合 Argo Rollouts 实现自动化金丝雀。
- 生产环境启用 xDS 指标采集与访问日志（默认已接入 Prometheus）。

## 面试要点

> 以下 Q&A 覆盖 kgateway 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：kgateway 与 Envoy Gateway 的定位差异？**
   A：两者都基于 Gateway API + Envoy 数据面，但 kgateway 源自 Gloo 生态（Solo.io），核心差异在于策略扩展体系（RoutePolicy/UpstreamPolicy）和内置 AI Gateway 能力（LLM 路由、Token 配额、模型回退），更侧重企业级 API 治理与 AI 流量管理场景。

2. **Q：Gateway API 的 HTTPRoute 权重路由与 Ingress 的流量切分有何优势？**
   A：HTTPRoute 是声明式 API 对象，权重切分、Header 匹配、镜像流量等作为一等公民表达，且支持"部分变更即时生效 + 状态反馈（Accepted/ResolvedRefs）"；Ingress 依赖注解、无状态反馈，运维可观测性差。Gateway API 的准入校验也避免了非法配置直接下发。

3. **Q：AI Gateway 中 Token 配额是如何实现的？**
   A：kgateway 在 Envoy 过滤器链中注入 AI 专用过滤器：按 model 字段识别请求，从配额池（Redis 后端）预扣/实扣 token（按 max_tokens 估算），超出返回 429 并触发模型回退或排队；配额模型支持按租户/模型/维度组合，实现 AI 成本治理。

## 参考链接

- https://kgateway.dev/
- https://github.com/kgateway-dev/kgateway

## Related

- [[17-系统基础/06-知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[17-系统基础/06-知识字典/networking/contour.md|Contour]]
- [[17-系统基础/06-知识字典/networking/traefik.md|Traefik]]
