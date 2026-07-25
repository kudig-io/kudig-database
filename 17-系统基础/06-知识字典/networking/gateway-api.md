---
title: Gateway API
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- istio
- envoy
- ingress
- gateway
- rbac
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Gateway API 是什么
- 如何 Gateway API
trigger_keywords:
- Gateway
- API
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- service-mesh-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Gateway API

## 概述

Gateway API 是 [[Kubernetes|Kubernetes]] 中用于暴露网络服务的一组扩展 API（以 CustomResourceDefinition 实现），旨在提供比 [[Ingress|Ingress]] 更动态、更灵活、更面向角色的流量路由能力。它是 Ingress 的继任者，支持基础设施自动配置和高级路由策略，已被 Kubernetes 项目推荐为新项目的首选方案。

## 核心概念/原理

- **角色导向设计（Role-Oriented）**：API 资源按照组织中不同角色的职责分层：
  - **基础设施提供商**：管理 GatewayClass，定义底层控制器和公共配置。
  - **集群运维**：管理 Gateway，定义流量入口实例（如云负载均衡器或集群内代理）。
  - **应用开发者**：管理 HTTPRoute / GRPCRoute，定义应用层路由规则。
- **资源模型**：
  - **GatewayClass**：声明一组由同一控制器管理的 Gateway，类似 Ingress 中的 IngressClass。
  - **Gateway**：描述一个具体的流量处理基础设施实例，定义监听器（listener）、协议、端口及允许附加路由的命名空间策略。
  - **HTTPRoute**：定义从 Gateway 监听器到后端 [[Service|Service]] 的 HTTP 请求路由规则，支持基于路径、主机名、Header 等的匹配。
  - **GRPCRoute**：定义 [[gRPC|gRPC]] 流量的路由规则，要求底层 Gateway 支持 HTTP/2（无需 HTTP/1 升级）。
- **双向信任模型**：Gateway 通过 `allowedRoutes` 控制哪些命名空间的路由可以附加到自身；路由则通过 `parentRefs` 声明要附加的 Gateway，实现双向授权。

## 关键机制或特性

- **可移植性（Portable）**：Gateway API 规范以自定义资源形式定义，已被众多实现（云厂商、开源代理）广泛支持。
- **表达能力（Expressive）**：原生支持 Header 匹配、流量加权、路径重写、重定向、跨命名空间路由等在 Ingress 中只能通过注解实现的功能。
  - `type: PathPrefix`
  - `type: Exact`
  - Header 匹配与修改
  - 请求/响应过滤器
- **可扩展性（Extensible）**：允许在 API 的不同层级链接自定义资源（如参数配置、后端引用），实现细粒度定制而不破坏核心规范。
- **一致性（Conformance）**：项目提供明确的兼容性定义和测试套件，确保不同实现之间提供一致的 API 行为。

## 使用场景

- **多租户/多团队协作**：基础设施团队管理 GatewayClass 和 Gateway，应用团队独立管理各自命名空间下的 HTTPRoute，实现职责分离。
- **高级流量管理**：需要基于请求头、权重分割、蓝绿发布、金丝雀发布等复杂路由策略。
- **跨命名空间服务暴露**：通过 Gateway 的 `allowedRoutes` 策略，安全地将一个入口共享给多个命名空间的应用。
- **从 Ingress 迁移**：已有 Ingress 资源可通过一次性转换迁移到 Gateway API，获得更强的功能和更清晰的资源分层。

## 最佳实践/注意事项

- **安装实现后再使用**：Gateway API 本身只是规范 CRD，集群中必须安装并部署具体的实现（控制器）才能生效。
  - 安装 CRD：`kubectl apply -k "github.com/kubernetes-sigs/gateway-api/config/crd?ref=v1.0.0"`
  - 部署所选实现（如 Envoy Gateway、NGINX Gateway Fabric、Istio 等）
- **阅读实现文档**：不同实现对特定功能的支持程度和 caveat 不同，生产使用前需仔细评估。
- **按角色分配 RBAC**：建议为基础设施团队授予 GatewayClass/Gateway 的管理权限，为开发团队授予 Route 资源的管理权限。
- **规划迁移路径**：Ingress API 已冻结，建议新系统直接采用 Gateway API；存量系统可逐步将路由规则从 Ingress 迁移到 HTTPRoute。

## 生产 YAML 示例

### 完整的 GatewayClass + Gateway + HTTPRoute

```yaml
# 1. GatewayClass — 基础设施提供商定义
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy-gateway
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
---
# 2. Gateway — 集群运维配置入口
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
  namespace: infra
spec:
  gatewayClassName: envoy-gateway
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      mode: Terminate
      certificateRefs:
      - kind: Secret
        name: wildcard-tls
        namespace: infra
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: "true"     # 仅允许标记的命名空间附加路由
  - name: http
    protocol: HTTP
    port: 80
    allowedRoutes:
      namespaces:
        from: Same                     # 仅同命名空间
---
# 3. HTTPRoute — 应用开发者定义路由
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: web-routes
  namespace: production                # 需要有 gateway-access: true 标签
spec:
  parentRefs:
  - name: production-gateway
    namespace: infra
    sectionName: https
  hostnames:
  - "app.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
      headers:
      - name: X-API-Version
        value: "v2"
    backendRefs:
    - name: api-v2
      port: 8080
      weight: 90
    - name: api-v3-canary
      port: 8080
      weight: 10                       # 10% 金丝雀流量
  - matches:
    - path:
        type: PathPrefix
        value: /
    backendRefs:
    - name: web-frontend
      port: 80
```

### GRPCRoute 示例

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GRPCRoute
metadata:
  name: grpc-routes
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: infra
  hostnames:
  - "grpc.example.com"
  rules:
  - matches:
    - method:
        service: myapp.UserService
        method: GetUser
    backendRefs:
    - name: user-service
      port: 9090

```

## Gateway API vs Ingress 对比

| 维度 | Ingress | Gateway API |
|------|---------|-------------|
| API 状态 | 冻结（不再添加新功能） | 活跃开发 |
| 角色分离 | 无 | GatewayClass/Gateway/Route 分层 |
| Header 匹配 | 需注解（控制器特定） | 原生支持 |
| 流量加权 | 不支持 | 原生支持（backendRef weight） |
| 跨命名空间路由 | 不支持 | 原生支持（双向信任模型） |
| gRPC 路由 | 通过注解 | GRPCRoute 原生 |
| 可移植性 | 低（依赖注解） | 高（一致性测试套件） |
| 实现数量 | 非常多 | 快速增长 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Gateway 无 Address | 控制器未安装或 GatewayClass 不匹配 | `kubectl get gatewayclass`；检查控制器 Pod |
| HTTPRoute 未生效 | parentRef 不匹配或命名空间不在 allowedRoutes 中 | `kubectl describe httproute` 查看 status.parents |
| 路由冲突 | 多个 HTTPRoute 匹配同一路径 | 检查 hostnames 和 matches 的优先级 |
| TLS 证书错误 | certificateRefs 指向的 Secret 不存在 | `kubectl get secret -n infra wildcard-tls` |

## 生产检查清单

- [ ] 安装 Gateway API CRD（`kubectl apply -k "github.com/kubernetes-sigs/gateway-api/config/crd?ref=v1.2.0"`）
- [ ] 部署选定的实现控制器（Envoy Gateway、NGINX Gateway Fabric 等）
- [ ] 按角色分配 RBAC（infra 团队管 Gateway，dev 团队管 Route）
- [ ] 命名空间标记 `gateway-access` 标签用于 allowedRoutes
- [ ] TLS Secret 使用 [[cert-manager|cert-manager]] 自动管理
- [ ] 新项目直接使用 Gateway API，存量 Ingress 规划迁移

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Gateway API CRD
kubectl apply -k "github.com/kubernetes-sigs/gateway-api/config/crd?ref=v1.2.0"

# 查看 GatewayClass
kubectl get gatewayclasses

# 查看 Gateway 状态和 Address
kubectl get gateways -A
kubectl describe gateway production-gateway -n infra

# 查看 HTTPRoute 状态
kubectl get httproutes -n production
kubectl describe httproute web-routes -n production

# 检查 Route 是否被 Gateway 接受
kubectl get httproute web-routes -n production -o jsonpath='{.status.parents}'
```
## 交叉引用

- [Ingress](ingress.md) — 被 Gateway API 取代的旧方案
- [Ingress Controllers](ingress-controllers.md) — 多数控制器同时支持 Ingress 和 Gateway API
- [Service](service.md) — Gateway API 后端指向的 Service
- [Service Mesh](service-mesh.md) — Istio 通过 Gateway API 管理服务网格流量

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/gateway/

## Related

- [[21-生态参考/03-领域索引/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]

```

<!-- risk-assessed -->
