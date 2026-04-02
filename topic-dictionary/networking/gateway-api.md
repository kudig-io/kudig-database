# Gateway API

## 概述

Gateway API 是 Kubernetes 中用于暴露网络服务的一组扩展 API（以 CustomResourceDefinition 实现），旨在提供比 Ingress 更动态、更灵活、更面向角色的流量路由能力。它是 Ingress 的继任者，支持基础设施自动配置和高级路由策略，已被 Kubernetes 项目推荐为新项目的首选方案。

## 核心概念/原理

- **角色导向设计（Role-Oriented）**：API 资源按照组织中不同角色的职责分层：
  - **基础设施提供商**：管理 GatewayClass，定义底层控制器和公共配置。
  - **集群运维**：管理 Gateway，定义流量入口实例（如云负载均衡器或集群内代理）。
  - **应用开发者**：管理 HTTPRoute / GRPCRoute，定义应用层路由规则。
- **资源模型**：
  - **GatewayClass**：声明一组由同一控制器管理的 Gateway，类似 Ingress 中的 IngressClass。
  - **Gateway**：描述一个具体的流量处理基础设施实例，定义监听器（listener）、协议、端口及允许附加路由的命名空间策略。
  - **HTTPRoute**：定义从 Gateway 监听器到后端 Service 的 HTTP 请求路由规则，支持基于路径、主机名、Header 等的匹配。
  - **GRPCRoute**：定义 gRPC 流量的路由规则，要求底层 Gateway 支持 HTTP/2（无需 HTTP/1 升级）。
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

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/gateway/
