---
title: Gateway API vs Ingress：下一代 Kubernetes 网络入口
summary: 深入对比 Gateway API 与传统 Ingress 的架构差异、能力边界和迁移路径，评估生产环境采用 Gateway API 的时机和策略。
category: research
tags:
- research
- gateway-api
- ingress
- networking
- service-mesh
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# Gateway API vs Ingress：下一代 Kubernetes 网络入口

## 研究背景

Kubernetes Ingress 自 v1.1 以来一直是集群外流量入口的标准抽象。但 Ingress 的设计局限性在生产实践中日益凸显：

- **注释地狱（Annotation Hell）**：高级路由（金丝雀、流量切分、Header 匹配）依赖厂商特定注释，不可移植
- **单一路由模型**：无法区分基础设施提供者（Infra Provider）和应用开发者（Route Owner）的关注点
- **协议受限**：原生仅支持 HTTP/HTTPS，TCP/UDP/gRPC 需要厂商扩展
- **跨命名空间限制**：Ingress 和 Backend 通常必须在同一命名空间

Gateway API（由 SIG-Network 主导）于 2023 年达到 GA，旨在彻底解决这些问题。

## 核心问题

1. Gateway API 的三角色模型（GatewayClass → Gateway → xRoute）相比 Ingress 带来了哪些本质改变？
2. 各大 Ingress Controller（NGINX、Envoy Gateway、Traefik、Cilium）对 Gateway API 的实现成熟度如何？
3. 从 Ingress 迁移到 Gateway API 的路径、工具和风险是什么？
4. Gateway API 与 Service Mesh 的关系：是融合还是并行？

## 调研发现

### 发现一：角色分离是 Gateway API 的核心价值

```
传统 Ingress 模型:
  集群管理员 ←→ Ingress Controller ←→ Ingress（应用开发者创建）

  问题：Ingress 既是基础设施配置又是应用路由规则，职责混淆

Gateway API 模型:
  ┌─────────────────────────────────────────────────────────┐
  │  GatewayClass（基础设施提供者: 如 cilium.io、nginx.org）   │
  │    → 定义能力边界和实现参数                                │
  │                                                         │
  │  Gateway（平台工程团队创建）                                │
  │    → 定义监听器、证书、基础设施级配置                        │
  │    → 可跨命名空间引用                                       │
  │                                                         │
  │  HTTPRoute/TCPRoute/GRPCRoute（应用开发者创建）            │
  │    → 定义路由规则（路径、Header、权重）                      │
  │    → 通过 Gateway Listener 引用连接到 Gateway               │
  │    → 可跨命名空间（通过 ReferenceGrant 授权）                │
  └─────────────────────────────────────────────────────────┘
```

这种分离使得：
- 平台团队控制 Gateway（端口、TLS、安全基线）
- 应用团队控制 Route（路由规则、流量切分）
- 权限通过 RBAC 自然隔离

### 发现二：功能能力对比

| 能力 | Ingress | Gateway API | 说明 |
|------|---------|-------------|------|
| HTTP 路径匹配 | ✅ | ✅ | 基础能力 |
| HTTP Header 匹配 | ⚠️ 注释 | ✅ 原生 | Ingress 依赖厂商注释 |
| 金丝雀/流量切分 | ⚠️ 注释 | ✅ 原生 | `weight: 10` 直接配置 |
| 请求/响应变换 | ❌ | ✅ 原生 | 添加/删除 Header |
| 跨命名空间后端 | ❌ | ✅ ReferenceGrant | 安全的跨命名空间引用 |
| TCP/UDP 路由 | ⚠️ 注释 | ✅ TCPRoute/UDPRoute | 原生多协议支持 |
| gRPC 路由 | ⚠️ 注释 | ✅ GRPCRoute | 原生 gRPC 方法匹配 |
| 多证书/TLS | ⚠️ SNI 注释 | ✅ Listener TLS | 每个 Listener 独立证书 |
| 健康检查 | ⚠️ 注释 | ✅ 原生 | BackendTLSPolicy 等 |
| 请求超时 | ⚠️ 注释 | ✅ 原生 | BackendTrafficPolicy |

### 发现三：主流实现成熟度（2026 年中）

| 实现 | Gateway API 版本 | 成熟度 | 生产就绪 | 推荐场景 |
|------|-----------------|--------|---------|---------|
| **Envoy Gateway** | v1.2 (GA) | ⬤⬤⬤⬤⬤ | ✅ | 新集群首选、Envoy 原生 |
| **Cilium Gateway** | v1.3 (GA) | ⬤⬤⬤⬤⬤ | ✅ | 已用 Cilium CNI 的集群 |
| **NGINX Gateway** | v1.4 (GA) | ⬤⬤⬤⬤ | ✅ | 传统 NGINX 用户迁移 |
| **Traefik v3** | v1.2 (GA) | ⬤⬤⬤⬤ | ✅ | 中小规模、已用 Traefik |
| **Istio Gateway** | v1.3 (GA) | ⬤⬤⬤⬤⬤ | ✅ | 已用 Istio Service Mesh |
| **Kong Gateway** | v1.0 (Beta) | ⬤⬤⬤ | ⚠️ | API 网关场景 |

### 发现四：迁移路径与工具

Gateway API 提供了官方的 `ingress2gateway` 工具，可自动转换现有 Ingress 资源：

```bash
# 🟢 安装 ingress2gateway
kubectl krew install ingress2gateway

# 🟢 转换现有 Ingress 资源（干运行）
ingress2gateway print --providers ingress-nginx --namespace production

# 🟢 转换并直接应用
ingress2gateway convert --providers ingress-nginx | kubectl apply -f -

# 🟢 同时保留 Ingress 和 Gateway（双运行模式，验证后删除 Ingress）
```

**推荐迁移策略（双运行模式）**：

```
阶段 1: 双运行
  → Gateway Controller 与 Ingress Controller 并行部署
  → 新应用使用 HTTPRoute，旧应用保留 Ingress
  → 逐步将 Ingress 转换为 HTTPRoute

阶段 2: 验证
  → 对比两套路由的流量行为
  → 验证金丝雀、TLS、Header 匹配等功能
  → 确认可观测性数据完整

阶段 3: 切换
  → DNS/LB 流量切换到 Gateway 入口
  → 观察 1-2 周
  → 下线 Ingress Controller
```

### 发现五：Gateway API 与 Service Mesh 的关系

Gateway API 正在成为统一南北向（入口）和东西向（Mesh）路由的标准接口：

- **Istio**：Gateway API 是推荐入口方式，同时支持 Mesh 内路由（通过 Service 导出）
- **Cilium Service Mesh**：基于 Gateway API 提供无 sidecar Mesh + 统一入口
- **Linkerd**：正在增加 Gateway API 支持
- **Kubernetes GAMMA initiative**：Gateway API for Mesh Management Architecture，将 Gateway API 扩展到 Service Mesh 内部路由

**趋势判断**：Gateway API 将成为 Kubernetes 网络路由的统一抽象层，无论南北向还是东西向。

## 结论与建议

1. **Gateway API 已生产就绪**：Envoy Gateway、Cilium、NGINX 三大实现的 GA 版本可以支撑生产流量。
2. **新集群应直接采用 Gateway API**：避免 Ingress 注释地狱的技术债。
3. **存量集群应在 6-12 个月内规划迁移**：使用 ingress2gateway 工具，采用双运行模式降低风险。
4. **Gateway API + Service Mesh 是未来**：GAMMA initiative 将统一入口和 Mesh 路由，投资 Gateway API 即投资未来。
5. **选型建议**：已用 Cilium → Cilium Gateway；需要 Envoy 全能力 → Envoy Gateway；传统 NGINX → NGINX Gateway。

## 参考资料

- Gateway API 官方规范: https://gateway-api.sigs.k8s.io/
- ingress2gateway 工具: https://github.com/kubernetes-sigs/ingress2gateway
- Envoy Gateway 文档: https://gateway.envoyproxy.io/
- [[22-概念/03-网络/service-networking.md|Service Networking]]
- [[05-网络/04-API网关/01-api-gateway-architecture-overview.md|Gateway API 目录]]
- [[25-研究/02-网络与安全/ebpf-networking-revolution.md|eBPF 网络革命]]

## Related

- [[24-综合/03-网络与服务网格/service-ingress.md|Service × Ingress]]
- [[05-网络/index.md|网络目录索引]]
