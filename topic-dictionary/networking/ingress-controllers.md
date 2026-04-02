# Ingress Controllers

## 概述

Ingress 资源本身只是声明式的路由配置，**必须有 Ingress Controller 在集群中运行**才能将其转化为实际的流量转发规则。Ingress Controller 通常以负载均衡器或反向代理的形式实现，负责监听 Ingress 和 EndpointSlice 的变化，并动态配置底层数据面（如 NGINX、Envoy、云厂商 LB 等）。

## 核心概念/原理

- **控制器与 IngressClass**：每个 Ingress 通过 `ingressClassName` 字段关联一个 IngressClass，IngressClass 则声明了负责实现该类的控制器名称（`spec.controller`）。控制器仅处理匹配其 IngressClass 的 Ingress 资源。
- **多控制器共存**：一个集群中可以同时部署多个 Ingress Controller，只要它们使用不同的 IngressClass 即可。例如，一个用于内部流量（内部 NGINX），一个用于公网流量（云厂商 LB）。
- **默认控制器**：若创建 Ingress 时未指定 `ingressClassName`，且集群中恰好只有一个 IngressClass 被标记为默认，则 Kubernetes 会自动将其分配给该 Ingress。

## 关键机制或特性

- **官方维护控制器**：Kubernetes 项目官方维护 AWS 和 GCE 的 Ingress Controller。
- **社区与第三方控制器**：社区提供了大量实现，涵盖不同数据面和云环境，常见的包括：
  - **NGINX Ingress Controller**（基于 NGINX）
  - **Traefik**（Go 编写的反向代理）
  - **HAProxy Ingress**（基于 HAProxy）
  - **Contour / Emissary-Ingress**（基于 Envoy）
  - **Istio Ingress**（基于 Istio）
  - **Kong Ingress Controller**
  - **Cilium Ingress Controller**
  - **云厂商方案**：Azure Application Gateway、Alibaba Cloud API Gateway、OCI Native Ingress Controller 等
- **功能差异**：不同控制器对路径类型、注解、TLS、速率限制、高级路由等功能的支持程度不同。

## 使用场景

- **根据现有技术栈选型**：若团队熟悉 NGINX，可选择 NGINX Ingress Controller；若需要 Service Mesh 能力，可选 Istio 或 Cilium。
- **多云/混合云部署**：在不同云厂商的集群中部署对应控制器，以利用云原生负载均衡能力。
- **安全与 WAF 需求**：选择集成 Web 应用防火墙（WAF）的控制器，如 BunkerWeb、Wallarm。

## 最佳实践/注意事项

- **仔细阅读控制器文档**：不同控制器的行为、注解和限制差异较大，部署前务必查看官方文档中的 caveats。
- **推荐使用 Gateway API**：Ingress API 已冻结，官方建议新项目或重构时评估并迁移到 Gateway API。
- **避免多个默认 IngressClass**：集群中最多只能有一个默认 IngressClass，否则会导致未指定类的 Ingress 创建被阻止。
- **区分 IngressClass 的适用范围**：通过 IngressClass 的 `parameters.scope` 字段，可以将配置参数限定在集群级或命名空间级，便于多团队协作。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/
