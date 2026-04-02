# Ingress

## 概述

Ingress 是 Kubernetes 中用于管理集群外部 HTTP/HTTPS 访问到内部 Service 的 API 对象。它支持基于主机名（Host）和路径（Path）的路由规则，可提供负载均衡、SSL/TLS 终止以及基于名称的虚拟主机等能力。需要注意的是，**Ingress API 已被冻结**，Kubernetes 官方不再对其新增功能，推荐使用 **Gateway API** 作为继任方案。

## 核心概念/原理

- **Ingress 规则（Rules）**：每条 HTTP 规则包含可选的 `host`、路径列表 `paths` 以及对应的后端 `backend`。只有当请求的 Host 和 Path 同时匹配时，流量才会被转发到指定的 Service 端口。
- **路径类型（PathType）**：
  - `Exact`：精确匹配 URL 路径（区分大小写）。
  - `Prefix`：按 `/` 分隔的路径元素逐段前缀匹配。
  - `ImplementationSpecific`：匹配逻辑由具体的 IngressClass 实现决定。
- **默认后端（DefaultBackend）**：当没有任何规则匹配请求时，流量会转发到默认后端。若 Ingress 未定义规则，则必须显式指定 `defaultBackend`。
- **资源后端（Resource Backend）**：除 Service 外，Ingress 后端也可以是同一命名空间内的自定义资源（如对象存储），用于提供静态资源。
- **TLS 终止**：通过在 Ingress 中引用包含 `tls.crt` 和 `tls.key` 的 Secret，实现 HTTPS 接入。Ingress 只支持 443 端口，并假设 TLS 在 Ingress 点终止，到 Pod 的流量为明文。

## 关键机制或特性

- **IngressClass**：Ingress 需关联一个 IngressClass，以指明由哪个控制器实现。IngressClass 可携带控制器特定的参数（`parameters`），支持集群级（Cluster）或命名空间级（Namespace）作用域。
- **默认 IngressClass**：通过注解 `ingressclass.kubernetes.io/is-default-class: "true"` 标记默认 IngressClass。若集群中存在多个默认类，则不允许创建未指定 `ingressClassName` 的 Ingress。
- **主机名通配符**：支持 `*.foo.com` 形式的通配符主机名，但仅覆盖单个 DNS 标签。
- **多路径匹配优先级**：当多个路径匹配请求时，优先选择最长匹配路径；若长度相同，则 `Exact` 优先于 `Prefix`。
- **控制器负载均衡**：Ingress 控制器自带负载均衡策略设置（如算法、权重），高级功能（如持久会话、动态权重）通常需通过底层负载均衡器或 Service 实现。

## 使用场景

- **简单扇出（Fanout）**：使用单一路由 IP，根据路径将流量分发到多个 Service（如 `/api` -> service-a，`/web` -> service-b）。
- **基于名称的虚拟主机**：在同一 IP 上根据 `Host` 头将不同域名路由到不同后端（如 `foo.bar.com` -> service1，`bar.foo.com` -> service2）。
- **HTTPS 安全接入**：为多个域名配置 TLS 证书，实现统一的 SSL 终止入口。
- **统一外部入口管理**：将多个子系统的路由规则集中到一个 Ingress 资源中管理。

## 最佳实践/注意事项

- **必须部署 Ingress Controller**：仅创建 Ingress 资源不会生效，集群中必须至少运行一个 Ingress Controller。
- **建议定义默认 IngressClass**：即使某些控制器可在无 IngressClass 时工作，官方仍建议显式配置默认类，以提高可移植性和可维护性。
- **注意控制器差异**：不同 Ingress Controller 对注解、路径匹配、TLS 实现等行为存在差异，选型前需仔细阅读对应文档。
- **TLS 默认规则限制**：TLS 证书中的 `hosts` 必须与 `rules` 中的 `host` 显式匹配，否则默认规则（无 host）下 TLS 可能无法正常工作。
- **API 冻结与迁移**：Ingress API 已冻结，建议新架构优先考虑 Gateway API，旧系统可规划逐步迁移。

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/ingress/
