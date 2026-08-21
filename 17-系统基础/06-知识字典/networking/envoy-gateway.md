---
title: Envoy Gateway
description: Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy
  官方的网关方案，...
summary: Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy 官方的网关方案，...
category: dictionary
tags:
- k8s
- glossary
- envoy-gateway
- gateway-api
- ingress
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Envoy Gateway 是什么
- Envoy Gateway 详解
trigger_keywords:
- Envoy Gateway
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Envoy Gateway

> **英文名**: Envoy Gateway

## 概述

Envoy Gateway 是 CNCF 项目，提供基于 Envoy 的 Kubernetes Gateway API 实现。它是 Envoy 官方的网关方案，将 Envoy 作为独立的数据平面，通过 Gateway API 标准化管理入站流量。

## 核心概念/原理

### 核心架构

- **Envoy Gateway Controller**：监听 Gateway API 资源并配置 Envoy。
- **Envoy Proxy**：数据平面，处理实际流量。
- **EnvoyProxy CRD**：自定义 Envoy 部署和配置。

### Gateway API 概念

| 资源 | 功能 |
|------|------|
| GatewayClass | 网关实现类型 |
| Gateway | 入口点和监听器定义 |
| HTTPRoute | HTTP 路由规则 |
| TLSRoute | TLS 路由规则 |
| GRPCRoute | gRPC 路由规则 |

## 关键机制或特性

- **Gateway API 原生**：完全遵循 Kubernetes Gateway API 标准。
- **Envoy Extension**：支持 Envoy 的 Wasm/Lua 扩展。
- **Rate Limiting**：内置限流功能。
- **Security Policy**：JWT 验证、CORS、ExtAuth 等。
- **Traffic Splitting**：基于权重的流量分割（金丝雀）。

## 使用场景与最佳实践

- 新集群使用 Envoy Gateway 替代传统 Ingress Controller。
- 使用 Gateway API 标准化入站流量管理。
- 配合 cert-manager 自动管理 TLS 证书。
- 使用 EnvoyProxy CRD 自定义 Envoy 部署参数。
- 关注 Gateway API 的 GAMMA 倡议（服务间流量管理）。

## 架构深度解析

### Envoy Gateway 控制面架构

```
┌──────────────────────────────────────────────────────────────┐
│  Kubernetes API                                               │
│  GatewayClass → Gateway → HTTPRoute（声明式期望状态）          │
│       │                                                       │
│       ▼                                                       │
│  Envoy Gateway 控制面（Deployment，CNCF 项目）                │
│  ├─ Gateway API 控制器：watch CRD 并校验资源                 │
│  ├─ IR 翻译器：期望状态 → Envoy xDS 配置（IR 中间表示）       │
│  ├─ Infra 管理器：创建/管理 EnvoyProxy 数据面资源             │
│  └─ xDS Server：通过 gRPC 推送配置到数据面                    │
│       │                                                       │
│       ▼                                                       │
│  Envoy Proxy 数据面（每个 Gateway 一个 Deployment/DS）        │
│  ├─ LDS/RDS/CDS/EDS：监听器、路由、集群、端点                 │
│  ├─ 高性能 C++ 代理：HTTP/2、gRPC、TLS 终结、限流            │
│  └─ 可编程扩展：Wasm、Lua、External Processing               │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（envoyproxy/gateway）

| 模块 | 路径 | 职责 |
|------|------|------|
| Gateway API 控制器 | `internal/gatewayapi/` | 监听 Gateway/HTTPRoute/GRPCRoute 等 CRD，生成 IR |
| 翻译器 | `internal/ir/` | 将 API 资源翻译为 xDS 配置（Listener/Route/Cluster） |
| xDS 服务器 | `internal/xds/` | 基于 envoy-go-control-plane 推送增量配置 |
| Infra 管理 | `internal/infrastructure/` | 创建数据面 Deployment/Service/ConfigMap |
| 策略扩展 | `internal/policy/` | BackendTrafficPolicy、SecurityPolicy、EnvoyPatchPolicy |

### 流程步骤

1. 管理员创建 GatewayClass，指定 controllerName 为 `gateway.envoyproxy.io/gatewayclass-controller`。
2. Envoy Gateway 监听 Gateway 资源，为每个 Gateway 生成 EnvoyProxy 数据面（Deployment + Service）。
3. 用户创建 HTTPRoute 绑定 Gateway，声明域名/路径/后端路由规则。
4. 控制器将期望状态翻译为 IR，再转换为 xDS 配置通过 gRPC 推送到数据面。
5. 数据面 Envoy 生效新配置（热更新，不断连）；任何 CRD 变更实时同步。

## 生产案例

### 案例 1：Gateway 数据面升级引发的 502 风暴

| 时间 | 事件 |
|------|------|
| 10:00 | 升级 Envoy Gateway 到 v1.0.3，控制面滚动完成 |
| 10:05 | 部分服务开始出现 502 Bad Gateway |
| 10:10 | 检查发现数据面 Pod 仍为旧版本镜像（镜像拉取被限流，滚动超时） |
| 10:20 | `kubectl get gateway` 显示 Accepted=False，条件 `Ready` 缺失 |
| 10:30 | 手动删除旧数据面 Pod 触发重建，恢复 |
| 11:00 | 复盘：升级未检查数据面镜像版本一致性，且未配置 PDB |

**根因**：控制面与新数据面版本不匹配（xDS 协议差异），旧数据面 Pod 一直未重建。
**修复命令**：
```bash
# 查看 Gateway 状态与条件 🟢 只读
kubectl get gateway -A -o wide
kubectl get gatewayclass -o yaml
# 检查数据面版本 🟢 只读
kubectl get pods -l gateway.envoyproxy.io/owning-gateway-name=<gw> -o jsonpath='{.items[*].spec.containers[*].image}'
# 强制滚动重启数据面 🟡 中风险
kubectl rollout restart deployment/<envoy-gw-name>
```

### 案例 2：HTTPRoute 权重路由灰度事故

**现象**：金丝雀发布设置 v2 权重 10%，但实际 v2 流量占比 50%+。
**诊断**：`kubectl get httproute -o yaml` 发现权重配置被误写为 `weight: 100`（YAML 缩进错误导致 backendRefs 内覆盖）；xDS 配置确认 RDS 中集群权重不对。
**修复**：修正权重为 10/90 后应用，观察 `kubectl get httproute` 状态；建议用 GitOps 流水线做权重变更审核，避免手改。

## 对比评测

| 维度 | Envoy Gateway | NGINX Ingress | Istio Ingress Gateway |
|------|--------------|---------------|----------------------|
| API 标准 | Gateway API（标准） | Ingress（旧）+ 注解 | Gateway API / VirtualService |
| 数据面 | Envoy（xDS） | NGINX（静态配置） | Envoy（xDS） |
| 扩展性 | Wasm/EnvoyPatchPolicy | Lua/第三方模块 | Wasm/EnvoyFilter |
| 生态集成 | CNCF 孵化、GAMMA | 成熟但创新慢 | 服务网格同源 |
| 适用场景 | 新集群标准入口 | 存量 Ingress 迁移 | 网格 + 网关统一 |

**选型建议**：新项目首选 Envoy Gateway（标准 API + 可扩展数据面）；存量 NGINX 环境平滑迁移；已用 Istio 的团队可复用其入口。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| Gateway 不 Accepted | `kubectl get gateway -o yaml`（conditions） | GatewayClass 未匹配、监听器冲突 |
| 502/504 | `kubectl logs <envoy-pod>`；`kubectl get endpoints` | 后端无端点、TLS 配置错误 |
| 证书不生效 | `kubectl get secret -n <ns>`；检查 referenceGrant | Secret 跨命名空间引用未授权 |
| 路由不匹配 | `kubectl get httproute -o yaml`（resolvedRefs） | Hostname 冲突、Backend 类型不支持 |
| xDS 推送失败 | `kubectl logs <eg-controller>` | 控制面与数据面版本不匹配 |

## 生产部署清单

- [ ] GatewayClass 与 Gateway 资源已定义，状态 Accepted=True
- [ ] 数据面资源（Deployment/Service）配置了 PDB 与资源限额
- [ ] TLS 证书经 cert-manager 自动轮转，并配置 referenceGrant 授权
- [ ] 监控数据面（Envoy metrics: `envoy_http_downstream_rq_5xx`）与控制器日志
- [ ] 金丝雀/权重路由变更走 GitOps 审核流程

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 已知 CVE 或数据面崩溃 | 立即升级，先验证 GatewayClass 兼容性 |
| P0 | xDS 推送持续失败 | 检查控制面/数据面版本矩阵，回滚任一 |
| P1 | 需要 gRPC/HTTP3 等新协议 | 升级到支持版本，灰度 Gateway 验证 |
| P1 | Gateway API v1 正式版特性需求 | 评估升级（v1.0+ 稳定 API） |
| P2 | 稳定运行且无新需求 | 跟随 CNCF 版本节奏半年一次 |

## 面试要点

> 以下 Q&A 覆盖 Envoy Gateway 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Envoy Gateway 与 NGINX Ingress Controller 的架构差异是什么？**
   A：NGINX Ingress 是"控制器 + 静态配置"：控制器 watch Ingress 对象后渲染 NGINX 配置文件并 reload（reload 期间有连接闪断风险），扩展靠注解与 Lua 模块；Envoy Gateway 采用"声明式期望状态 + 动态 xDS 推送"：控制器将 Gateway API 资源翻译为 IR，再通过 gRPC xDS 增量下发到 Envoy 数据面（热更新零中断），扩展靠 Wasm 与 EnvoyPatchPolicy。核心差异是配置交付方式（静态 reload vs 动态 xDS）与 API 标准（Ingress 注解 vs Gateway API）。

2. **Q：Gateway API 中 GatewayClass、Gateway、HTTPRoute 三者的关系？**
   A：GatewayClass 是集群级资源，声明使用哪个控制器实现（如 `gateway.envoyproxy.io/gatewayclass-controller`）与参数模板；Gateway 是命名空间级资源，实例化一个监听器集合（端口、协议、TLS、地址），必须引用一个 GatewayClass；HTTPRoute 是路由资源，通过 `parentRefs` 绑定 Gateway，声明域名/路径匹配与后端转发、权重、重写等策略。三者形成"实现 → 入口 → 路由"的三层抽象，允许多团队解耦管理。

3. **Q：如何实现金丝雀发布与流量灰度？**
   A：用 HTTPRoute 的权重路由：同一 hostname 配置两个 backendRefs（v1/v2 两个 Service），分别设置 weight（如 90/10），Envoy 按权重随机选择集群。进阶：结合 header/method 匹配做定向灰度（`matches` 中的 headers 条件），或 BackendTrafficPolicy 配置限流保护新版本。发布后通过监控 v2 的错误率与延迟决定放量或回滚（把 weight 改回 0/100 即可，xDS 秒级生效）。

## 参考链接

- [Envoy Gateway Official](https://gateway.envoyproxy.io/)

## Related

- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/traefik.md|Traefik]]
- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]


<!-- risk-assessed -->
