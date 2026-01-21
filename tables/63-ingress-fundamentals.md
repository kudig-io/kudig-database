# Kubernetes Ingress 基础概念与核心原理 (Ingress Fundamentals)

> **适用版本**: Kubernetes v1.25 - v1.32  
> **文档版本**: v3.0 | 生产级 Ingress 配置参考  
> **最后更新**: 2026-01  
> **参考**: [kubernetes.io/docs/concepts/services-networking/ingress](https://kubernetes.io/docs/concepts/services-networking/ingress/)

---

## Ingress 核心架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────────────┐
│                              Kubernetes Ingress Architecture                                     │
├─────────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐    │
│  │                                   External Layer                                         │    │
│  │                                                                                          │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐    │    │
│  │  │   Browser       │  │   Mobile App    │  │   API Client    │  │   IoT Device    │    │    │
│  │  │                 │  │                 │  │                 │  │                 │    │    │
│  │  │ HTTPS Request   │  │ HTTPS Request   │  │ REST/gRPC       │  │ MQTT/HTTP       │    │    │
│  │  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘  └────────┬────────┘    │    │
│  │           │                    │                    │                    │             │    │
│  │           └──────────────────┬─┴────────────────────┴─┬──────────────────┘             │    │
│  │                              │                        │                                │    │
│  └──────────────────────────────┼────────────────────────┼────────────────────────────────┘    │
│                                 │                        │                                     │
│                                 ▼                        ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐   │
│  │                           Cloud Load Balancer / Edge Proxy                               │   │
│  │                                                                                          │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────────────┐   │   │
│  │  │  AWS ALB/NLB │ Azure App GW │ GCP Cloud LB │ Alibaba ALB │ On-Prem F5/HAProxy  │   │   │
│  │  │                                                                                  │   │   │
│  │  │  • SSL Termination (可选)    • DDoS Protection    • WAF (可选)                  │   │   │
│  │  │  • Health Check              • Geographic Routing  • Rate Limiting              │   │   │
│  │  └─────────────────────────────────────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────┬──────────────────────────────────────────────────┘   │
│                                         │                                                      │
│                                         │ NodePort / LoadBalancer Service                     │
│                                         ▼                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              Ingress Controller Layer                                    │   │
│  │                                                                                          │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                        Ingress Controller (NGINX/Traefik/Envoy)                     │ │   │
│  │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │ │   │
│  │  │  │  Watcher     │  │  Config Gen  │  │  Proxy Engine│  │  Metrics     │           │ │   │
│  │  │  │              │  │              │  │              │  │  Exporter    │           │ │   │
│  │  │  │ Watch:       │  │ Generate:    │  │ • TLS Term   │  │              │           │ │   │
│  │  │  │ • Ingress    │─>│ • nginx.conf │─>│ • L7 Route   │  │ Prometheus   │           │ │   │
│  │  │  │ • Service    │  │ • envoy.yaml │  │ • Load Bal   │  │ metrics      │           │ │   │
│  │  │  │ • Endpoints  │  │ • traefik    │  │ • Rate Limit │  │ /metrics     │           │ │   │
│  │  │  │ • Secret     │  │   config     │  │ • Auth       │  │              │           │ │   │
│  │  │  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘           │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────┬──────────────────────────────────────────────────┘   │
│                                         │                                                      │
│                    ┌────────────────────┼────────────────────┐                                │
│                    │                    │                    │                                │
│                    ▼                    ▼                    ▼                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐   │
│  │                               Kubernetes Service Layer                                   │   │
│  │                                                                                          │   │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                       │   │
│  │  │  Service A       │  │  Service B       │  │  Service C       │                       │   │
│  │  │  (ClusterIP)     │  │  (ClusterIP)     │  │  (ClusterIP)     │                       │   │
│  │  │                  │  │                  │  │                  │                       │   │
│  │  │  api.example.com │  │  www.example.com │  │  admin.example.  │                       │   │
│  │  │  /api/*          │  │  /*              │  │  com/*           │                       │   │
│  │  └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘                       │   │
│  │           │                     │                     │                                 │   │
│  └───────────┼─────────────────────┼─────────────────────┼─────────────────────────────────┘   │
│              │                     │                     │                                     │
│              ▼                     ▼                     ▼                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              EndpointSlice / Pod Layer                                   │   │
│  │                                                                                          │   │
│  │  ┌────────────────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  Node 1                    Node 2                    Node 3                         │ │   │
│  │  │  ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐             │ │   │
│  │  │  │ Pod A-1         │      │ Pod A-2         │      │ Pod B-1         │             │ │   │
│  │  │  │ 10.244.1.10:8080│      │ 10.244.2.11:8080│      │ 10.244.3.12:3000│             │ │   │
│  │  │  └─────────────────┘      └─────────────────┘      └─────────────────┘             │ │   │
│  │  │  ┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐             │ │   │
│  │  │  │ Pod B-2         │      │ Pod C-1         │      │ Pod C-2         │             │ │   │
│  │  │  │ 10.244.1.13:3000│      │ 10.244.2.14:9000│      │ 10.244.3.15:9000│             │ │   │
│  │  │  └─────────────────┘      └─────────────────┘      └─────────────────┘             │ │   │
│  │  └────────────────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              Control Plane Components                                    │   │
│  │                                                                                          │   │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐                       │   │
│  │  │  API Server      │  │  etcd            │  │  Controller Mgr  │                       │   │
│  │  │                  │  │                  │  │                  │                       │   │
│  │  │  Store Ingress   │  │  Persist         │  │  EndpointSlice   │                       │   │
│  │  │  Resources       │  │  All Resources   │  │  Controller      │                       │   │
│  │  └──────────────────┘  └──────────────────┘  └──────────────────┘                       │   │
│  └─────────────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 一、Ingress 概念总览

### 1.1 Ingress 核心定义

| 概念 | 英文术语 | 定义 | 作用 | 版本要求 |
|-----|---------|------|------|---------|
| **Ingress** | Ingress | Kubernetes API 对象，管理集群外部访问集群内服务的 HTTP/HTTPS 路由规则 | 定义 L7 路由规则、TLS 终止、虚拟主机 | v1.19+ GA |
| **Ingress Controller** | Ingress Controller | 负责实现 Ingress 规则的控制器组件，通常是反向代理服务器 | 监听 Ingress 资源变化，配置代理规则，实际处理流量转发 | - |
| **IngressClass** | IngressClass | 定义 Ingress 控制器类型的 API 对象 | 支持多控制器共存，指定 Ingress 由哪个控制器处理 | v1.19+ GA |
| **Backend** | Backend | Ingress 路由到的后端 Service | 接收转发的流量，负载均衡到 Pod | v1.19+ |
| **Default Backend** | Default Backend | 未匹配任何规则时的默认后端 | 处理未知请求、健康检查、404 页面 | v1.19+ |
| **TLS Secret** | TLS Secret | 存储 TLS 证书的 Kubernetes Secret | 为 Ingress 提供 HTTPS 加密能力 | v1.19+ |
| **Path Type** | Path Type | 路径匹配类型 | 控制 URL 路径如何匹配 | v1.18+ |

### 1.2 Ingress vs 其他流量入口方案对比

| 方案 | 协议层 | 协议支持 | 外部访问方式 | 负载均衡 | TLS 终止 | 路由能力 | 适用场景 | 复杂度 | 成本 |
|-----|-------|---------|------------|---------|---------|---------|---------|-------|-----|
| **ClusterIP** | L4 | TCP/UDP | 仅集群内部 | kube-proxy | 无 | 无 | 内部服务通信 | 低 | 无 |
| **NodePort** | L4 | TCP/UDP | Node IP:30000-32767 | kube-proxy | 无 | 无 | 开发测试、简单对外 | 低 | 无 |
| **LoadBalancer** | L4 | TCP/UDP | 云 LB 外部 IP | 云 LB + kube-proxy | 云 LB (可选) | 无 | 单服务对外暴露 | 中 | 每服务一个 LB |
| **Ingress** | L7 | HTTP/HTTPS | 统一入口 IP | Ingress Controller | 支持 | Host/Path 路由 | 多服务 HTTP 路由 | 中 | 共享控制器 |
| **Gateway API** | L4/L7 | HTTP/HTTPS/TCP/UDP/gRPC/TLS | 统一入口 | Gateway 实现 | 支持 | 丰富 (Header/Query 等) | 复杂多协议场景 | 高 | 共享网关 |
| **Service Mesh** | L4/L7 | 全协议 | Sidecar 代理 | Envoy/Linkerd | mTLS | 全功能 | 微服务治理、零信任 | 高 | Sidecar 开销 |

### 1.3 Ingress 核心能力矩阵

| 能力 | 英文 | 说明 | 依赖 | 生产建议 |
|-----|------|------|------|---------|
| **基于主机名路由** | Host-based Routing | 根据 HTTP Host 头转发请求到不同后端 | DNS 配置 | 必备 |
| **基于路径路由** | Path-based Routing | 根据 URL 路径转发请求到不同后端 | 无 | 必备 |
| **TLS 终止** | TLS Termination | 在 Ingress Controller 解密 HTTPS 流量 | TLS Secret | 必备 |
| **TLS 透传** | TLS Passthrough | 不解密，直接透传到后端 (需控制器支持) | 后端 TLS 配置 | 特殊场景 |
| **名称虚拟主机** | Name-based Virtual Hosting | 同一 IP 托管多个域名 | DNS 配置 | 必备 |
| **负载均衡** | Load Balancing | 在后端 Pod 间分配流量 | 多副本 Pod | 必备 |
| **会话保持** | Session Affinity | 将同一客户端请求路由到同一 Pod | 注解配置 | 有状态应用 |
| **重写规则** | URL Rewrite | 重写请求 URL 路径 | 注解配置 | 常用 |
| **限流** | Rate Limiting | 限制请求速率 | 注解配置 | 必备 |
| **认证** | Authentication | Basic Auth/OAuth/JWT | 注解/插件 | 推荐 |
| **金丝雀发布** | Canary Release | 按权重/Header/Cookie 分流 | 注解配置 | 推荐 |
| **跨域 (CORS)** | CORS | 跨域资源共享配置 | 注解配置 | Web 应用 |

### 1.4 为什么需要 Ingress

| 没有 Ingress | 使用 Ingress |
|-------------|-------------|
| 每个服务需要独立 LoadBalancer，成本高 | 多服务共享一个入口，降低成本 |
| 无法基于域名/路径路由 | 支持灵活的 L7 路由规则 |
| TLS 需要在每个服务单独配置 | 集中管理 TLS 证书 |
| 无法实现金丝雀发布等高级流量管理 | 支持金丝雀、蓝绿、A/B 测试 |
| 难以统一实施安全策略 | 集中配置认证、限流、WAF |
| 缺乏统一的可观测性 | 统一的访问日志和指标 |

---

## 二、Ingress 资源模型

### 2.1 Ingress API 结构 (networking.k8s.io/v1)

```
Ingress (networking.k8s.io/v1)
├── apiVersion: networking.k8s.io/v1
├── kind: Ingress
├── metadata
│   ├── name                          # Ingress 名称 (必填)
│   ├── namespace                     # 命名空间 (默认 default)
│   ├── labels                        # 标签 (推荐)
│   │   ├── app: <app-name>
│   │   ├── environment: <env>
│   │   └── team: <team-name>
│   └── annotations                   # 控制器特定配置 (重要)
│       ├── nginx.ingress.kubernetes.io/*
│       ├── traefik.ingress.kubernetes.io/*
│       └── alb.ingress.kubernetes.io/*
├── spec
│   ├── ingressClassName              # 指定 IngressClass (强烈推荐)
│   ├── defaultBackend                # 默认后端 (可选)
│   │   ├── service
│   │   │   ├── name                  # Service 名称
│   │   │   └── port
│   │   │       ├── number            # 端口号
│   │   │       └── name              # 或端口名称
│   │   └── resource                  # 或自定义资源 (少用)
│   │       ├── apiGroup
│   │       ├── kind
│   │       └── name
│   ├── tls[]                         # TLS 配置数组
│   │   ├── hosts[]                   # 证书适用的主机名列表
│   │   └── secretName                # TLS 证书 Secret 名称
│   └── rules[]                       # 路由规则数组 (核心)
│       ├── host                      # 主机名 (可选，空则匹配所有)
│       └── http
│           └── paths[]               # 路径规则数组
│               ├── path              # URL 路径
│               ├── pathType          # 路径匹配类型 (必填)
│               │   # Exact | Prefix | ImplementationSpecific
│               └── backend           # 后端服务
│                   └── service
│                       ├── name      # Service 名称
│                       └── port
│                           ├── number
│                           └── name
└── status                            # 状态 (只读)
    └── loadBalancer
        └── ingress[]
            ├── ip                    # 分配的 IP 地址
            ├── hostname              # 分配的主机名 (云环境)
            └── ports[]               # 端口信息 (v1.30+)
```

### 2.2 Ingress 规范字段详解

| 字段路径 | 类型 | 必填 | 默认值 | 说明 | 示例 |
|---------|------|------|-------|------|------|
| `metadata.name` | string | 是 | - | Ingress 资源名称，需符合 DNS 子域名规范 | `api-ingress` |
| `metadata.namespace` | string | 否 | default | 命名空间 | `production` |
| `metadata.annotations` | map[string]string | 否 | - | 控制器特定配置，是扩展 Ingress 能力的主要方式 | 见注解章节 |
| `spec.ingressClassName` | string | 推荐 | 默认 IngressClass | 指定处理此 Ingress 的 IngressClass | `nginx` |
| `spec.defaultBackend.service.name` | string | 条件 | - | 默认后端 Service 名称 | `default-backend` |
| `spec.defaultBackend.service.port.number` | int32 | 条件 | - | 默认后端端口号 | `80` |
| `spec.defaultBackend.service.port.name` | string | 条件 | - | 默认后端端口名称 (与 number 二选一) | `http` |
| `spec.tls[].hosts` | []string | 否 | - | TLS 证书适用的主机名列表 | `["api.example.com"]` |
| `spec.tls[].secretName` | string | 否 | - | TLS 证书 Secret 名称 | `api-tls-secret` |
| `spec.rules[].host` | string | 否 | - | 主机名，空则匹配所有主机 | `api.example.com` |
| `spec.rules[].http.paths[].path` | string | 是 | - | URL 路径 | `/api` |
| `spec.rules[].http.paths[].pathType` | string | 是 | - | 路径类型: Exact/Prefix/ImplementationSpecific | `Prefix` |
| `spec.rules[].http.paths[].backend.service.name` | string | 是 | - | 后端 Service 名称 | `api-service` |
| `spec.rules[].http.paths[].backend.service.port.number` | int32 | 条件 | - | 后端端口号 | `8080` |

### 2.3 路径类型详解 (pathType)

| 路径类型 | 英文 | 匹配规则 | 正则等价 | 性能 | 推荐场景 |
|---------|------|---------|---------|------|---------|
| **Exact** | 精确匹配 | 完全匹配 URL 路径，大小写敏感 | `^/foo$` | 最高 | 特定端点，如 `/health`, `/metrics` |
| **Prefix** | 前缀匹配 | 基于 `/` 分隔的路径元素前缀匹配 | `^/foo(/.*)?$` | 高 | 服务路由，如 `/api/*` |
| **ImplementationSpecific** | 实现特定 | 由 IngressClass/控制器决定 | 取决于控制器 | 取决于实现 | 需要正则匹配等高级场景 |

**Prefix 路径匹配详细规则:**

| Ingress Path | 请求 Path | 匹配结果 | 原因 |
|-------------|-----------|---------|------|
| `/foo` | `/foo` | 匹配 | 精确前缀 |
| `/foo` | `/foo/` | 匹配 | 尾部 `/` 被忽略 |
| `/foo` | `/foo/bar` | 匹配 | `/foo` 是 `/foo/bar` 的前缀元素 |
| `/foo` | `/foobar` | **不匹配** | `/foo` 不是 `/foobar` 的前缀元素 (无 `/` 分隔) |
| `/foo/` | `/foo` | 匹配 | 尾部 `/` 被忽略 |
| `/foo/` | `/foo/bar` | 匹配 | 前缀元素匹配 |
| `/` | `/foo` | 匹配 | `/` 匹配所有路径 |
| `/` | `/` | 匹配 | 精确匹配根路径 |

**路径优先级规则:**
1. **Exact 优先于 Prefix**: 相同路径时，Exact 匹配优先
2. **长路径优先**: 多个 Prefix 规则时，最长匹配路径优先
3. **同长度按定义顺序**: 相同长度路径按 YAML 定义顺序

```yaml
# 路径优先级示例
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      # 优先级 1: Exact 匹配
      - path: /api/v1/health
        pathType: Exact
        backend: {service: {name: health-check, port: {number: 80}}}
      # 优先级 2: 最长 Prefix
      - path: /api/v1/users
        pathType: Prefix
        backend: {service: {name: user-service, port: {number: 8080}}}
      # 优先级 3: 较短 Prefix
      - path: /api/v1
        pathType: Prefix
        backend: {service: {name: api-v1, port: {number: 8080}}}
      # 优先级 4: 根路径
      - path: /
        pathType: Prefix
        backend: {service: {name: default-backend, port: {number: 80}}}
```

---

## 三、IngressClass 详解

### 3.1 IngressClass 概念与作用

| 属性 | 说明 |
|-----|------|
| **目的** | 支持集群中运行多个不同的 Ingress Controller |
| **作用** | 明确指定 Ingress 资源由哪个控制器处理 |
| **默认 IngressClass** | 可设置一个为集群默认，未指定 ingressClassName 的 Ingress 自动使用 |
| **参数传递** | 支持通过 `parameters` 字段传递控制器特定的全局配置 |
| **命名空间作用域** | parameters 可以是集群级别或命名空间级别 |

### 3.2 IngressClass API 结构

```yaml
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx                                    # IngressClass 名称
  labels:
    app.kubernetes.io/name: ingress-nginx
    app.kubernetes.io/component: controller
  annotations:
    # 设置为默认 IngressClass (集群中只能有一个)
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  # 控制器标识符 - 必须与控制器 --controller-class 参数匹配
  controller: k8s.io/ingress-nginx
  
  # 可选: 控制器参数配置
  parameters:
    # API Group (可选，默认 core)
    apiGroup: k8s.nginx.org
    # 资源类型
    kind: IngressNginxConfiguration
    # 资源名称
    name: nginx-config
    # 命名空间 (Namespace 作用域时需要)
    namespace: ingress-nginx
    # 作用域: Namespace 或 Cluster
    scope: Namespace
```

### 3.3 常见 Ingress Controller 标识符

| 控制器 | Controller 标识符 | IngressClass 名称建议 | 维护方 |
|-------|-----------------|---------------------|-------|
| **NGINX Ingress (K8s 社区版)** | `k8s.io/ingress-nginx` | `nginx` | Kubernetes 社区 |
| **NGINX Ingress (F5 版)** | `nginx.org/ingress-controller` | `nginx-plus` | F5/NGINX |
| **Traefik** | `traefik.io/ingress-controller` | `traefik` | Traefik Labs |
| **HAProxy** | `haproxy.org/ingress-controller` | `haproxy` | HAProxy Tech |
| **Contour** | `projectcontour.io/ingress-controller` | `contour` | VMware/CNCF |
| **Kong** | `ingress-controllers.konghq.com/kong` | `kong` | Kong Inc |
| **Ambassador/Emissary** | `getambassador.io/ingress-controller` | `ambassador` | Ambassador Labs |
| **ALB (阿里云)** | `ingress.k8s.alibabacloud/alb` | `alb` | 阿里云 |
| **AWS ALB** | `ingress.k8s.aws/alb` | `alb` | AWS |
| **GCE** | `ingress.gce.io/ingress` | `gce` | Google |
| **Azure Application Gateway** | `azure/application-gateway` | `azure-agic` | Microsoft |
| **Istio** | `istio.io/ingress-controller` | `istio` | Istio 社区 |

### 3.4 多 IngressClass 配置示例

```yaml
# 生产环境 NGINX IngressClass (默认)
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
  parameters:
    apiGroup: k8s.nginx.org
    kind: IngressNginxConfiguration
    name: nginx-production-config
    namespace: ingress-nginx
    scope: Namespace
---
# 内部流量 Traefik IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: traefik-internal
spec:
  controller: traefik.io/ingress-controller
---
# 阿里云 ALB IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: alb
spec:
  controller: ingress.k8s.alibabacloud/alb
  parameters:
    apiGroup: alibabacloud.com
    kind: AlbConfig
    name: production-alb
---
# 使用特定 IngressClass 的 Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: internal-api
  namespace: internal
spec:
  ingressClassName: traefik-internal  # 明确指定
  rules:
  - host: internal-api.corp.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: internal-api
            port:
              number: 8080
```

---

## 四、Ingress 工作原理

### 4.1 Ingress Controller 工作流程

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Ingress Controller 工作流程                                   │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           1. Watch 阶段                                       │   │
│  │                                                                               │   │
│  │  Ingress Controller 通过 client-go Informer 机制监听以下资源:                  │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │   │
│  │  │   Ingress   │  │   Service   │  │ Endpoints/  │  │   Secret    │         │   │
│  │  │  资源变化    │  │  资源变化    │  │EndpointSlice│  │  (TLS证书)   │         │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │   │
│  │         │                │                │                │                 │   │
│  │         └────────────────┴────────────────┴────────────────┘                 │   │
│  │                                    │                                          │   │
│  │                                    ▼                                          │   │
│  │                          ┌─────────────────┐                                  │   │
│  │                          │   Event Queue   │                                  │   │
│  │                          │   (WorkQueue)   │                                  │   │
│  │                          └────────┬────────┘                                  │   │
│  └───────────────────────────────────┼──────────────────────────────────────────┘   │
│                                      │                                               │
│                                      ▼                                               │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           2. Sync 阶段                                        │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  Sync Handler 处理流程:                                                  │ │   │
│  │  │                                                                          │ │   │
│  │  │  1. 从 Queue 获取事件                                                    │ │   │
│  │  │  2. 获取所有相关的 Ingress 资源                                          │ │   │
│  │  │  3. 过滤: 只处理匹配当前 IngressClass 的 Ingress                         │ │   │
│  │  │  4. 解析 Ingress rules 和 annotations                                    │ │   │
│  │  │  5. 查询关联的 Service 和 Endpoints                                      │ │   │
│  │  │  6. 获取 TLS Secret 中的证书                                             │ │   │
│  │  │  7. 构建内部路由模型                                                      │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                               │
│                                      ▼                                               │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                        3. Config Generation 阶段                              │   │
│  │                                                                               │   │
│  │  根据控制器类型生成对应的代理配置:                                             │   │
│  │                                                                               │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │   │
│  │  │     NGINX       │  │     Envoy       │  │    Traefik      │               │   │
│  │  │                 │  │                 │  │                 │               │   │
│  │  │  nginx.conf     │  │  xDS API        │  │  Dynamic Config │               │   │
│  │  │  ┌───────────┐ │  │  ┌───────────┐ │  │  ┌───────────┐ │               │   │
│  │  │  │ server {} │ │  │  │ Listener  │ │  │  │ Router    │ │               │   │
│  │  │  │ location  │ │  │  │ Route     │ │  │  │ Service   │ │               │   │
│  │  │  │ upstream  │ │  │  │ Cluster   │ │  │  │ Middleware│ │               │   │
│  │  │  └───────────┘ │  │  └───────────┘ │  │  └───────────┘ │               │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │   │
│  └───────────────────────────────────────────┬──────────────────────────────────┘   │
│                                              │                                       │
│                                              ▼                                       │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           4. Reload 阶段                                      │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  不同控制器的配置更新方式:                                                │ │   │
│  │  │                                                                          │ │   │
│  │  │  NGINX:                                                                  │ │   │
│  │  │    - 配置文件变更 → nginx -t 验证 → nginx -s reload                      │ │   │
│  │  │    - Endpoints 变更 → Lua 动态更新 (无需 reload)                         │ │   │
│  │  │                                                                          │ │   │
│  │  │  Envoy (Contour/Istio):                                                  │ │   │
│  │  │    - xDS API 推送 → 热更新，无需重启                                      │ │   │
│  │  │                                                                          │ │   │
│  │  │  Traefik:                                                                │ │   │
│  │  │    - 内置动态配置 → 实时更新，无需重启                                     │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────┬──────────────────────────────────┘   │
│                                              │                                       │
│                                              ▼                                       │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           5. Status Update 阶段                               │   │
│  │                                                                               │   │
│  │  更新 Ingress 的 status.loadBalancer 字段:                                    │   │
│  │                                                                               │   │
│  │  status:                                                                      │   │
│  │    loadBalancer:                                                              │   │
│  │      ingress:                                                                 │   │
│  │      - ip: "203.0.113.10"           # 分配的外部 IP                          │   │
│  │        hostname: "xxx.elb.amazonaws.com"  # 或云 LB 的域名                   │   │
│  │                                                                               │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 流量转发路径对比

| 转发模式 | 路径 | 数据流 | 优点 | 缺点 | 控制器支持 |
|---------|-----|-------|------|------|-----------|
| **Service 转发** | Ingress → Service (ClusterIP) → Pod | `Client → LB → IC → Service VIP → kube-proxy → Pod` | 简单，利用 K8s 原生负载均衡 | 额外跳转，延迟稍高 | 所有控制器默认 |
| **Endpoints 直连** | Ingress → Endpoints → Pod | `Client → LB → IC → Pod IP` | 绕过 kube-proxy，延迟更低 | 需要控制器支持 | NGINX, Traefik, Kong |
| **EndpointSlice 直连** | Ingress → EndpointSlice → Pod | `Client → LB → IC → Pod IP` | 大规模集群优化，支持 Zone 感知 | 需要 v1.21+ | 现代控制器 |

### 4.3 请求处理流程详解

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              HTTP 请求处理详细流程                                       │
├────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                         │
│  Client Request: GET https://api.example.com/v1/users HTTP/1.1                         │
│                                                                                         │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 1: DNS 解析                                                                 │   │
│  │                                                                                  │   │
│  │ api.example.com → 203.0.113.10 (Ingress Controller External IP)                 │   │
│  │                                                                                  │   │
│  │ 来源: external-dns 自动创建 或 手动 DNS 记录                                     │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 2: TLS 握手 (HTTPS)                                                         │   │
│  │                                                                                  │   │
│  │ 1. Client Hello (支持的 TLS 版本、密码套件)                                      │   │
│  │ 2. Server Hello (选择 TLS 1.3, AES-256-GCM)                                     │   │
│  │ 3. Certificate (api.example.com 证书，来自 TLS Secret)                          │   │
│  │ 4. Key Exchange (ECDHE)                                                          │   │
│  │ 5. Finished (加密通道建立)                                                       │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 3: Host 匹配                                                                │   │
│  │                                                                                  │   │
│  │ HTTP Host Header: api.example.com                                               │   │
│  │                                                                                  │   │
│  │ 匹配 Ingress Rule:                                                              │   │
│  │   rules:                                                                         │   │
│  │   - host: api.example.com  ← 匹配                                               │   │
│  │   - host: www.example.com  ← 不匹配                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 4: Path 匹配                                                                │   │
│  │                                                                                  │   │
│  │ Request Path: /v1/users                                                          │   │
│  │                                                                                  │   │
│  │ 匹配规则 (按优先级):                                                             │   │
│  │   - path: /v1/users  pathType: Exact   → 优先匹配 (如存在)                      │   │
│  │   - path: /v1        pathType: Prefix  → 次优先匹配 ← 选中                       │   │
│  │   - path: /          pathType: Prefix  → 最低优先                               │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 5: 后端选择与负载均衡                                                        │   │
│  │                                                                                  │   │
│  │ Backend Service: api-v1-service:8080                                             │   │
│  │                                                                                  │   │
│  │ EndpointSlice 后端 Pod:                                                          │   │
│  │   - 10.244.1.10:8080 (Ready)                                                    │   │
│  │   - 10.244.2.11:8080 (Ready)                                                    │   │
│  │   - 10.244.3.12:8080 (Ready)                                                    │   │
│  │                                                                                  │   │
│  │ 负载均衡算法: Round Robin (默认)                                                  │   │
│  │ 选中: 10.244.2.11:8080                                                           │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 6: 请求转发                                                                 │   │
│  │                                                                                  │   │
│  │ 原始请求:                                                                        │   │
│  │   GET /v1/users HTTP/1.1                                                        │   │
│  │   Host: api.example.com                                                         │   │
│  │   X-Real-IP: (无)                                                               │   │
│  │                                                                                  │   │
│  │ 转发请求 (添加代理头):                                                           │   │
│  │   GET /v1/users HTTP/1.1                                                        │   │
│  │   Host: api.example.com                                                         │   │
│  │   X-Real-IP: 1.2.3.4                                                            │   │
│  │   X-Forwarded-For: 1.2.3.4                                                      │   │
│  │   X-Forwarded-Proto: https                                                      │   │
│  │   X-Forwarded-Host: api.example.com                                             │   │
│  │   X-Request-ID: abc-123-def-456                                                 │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                        │                                                │
│                                        ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │ Step 7: 响应处理                                                                 │   │
│  │                                                                                  │   │
│  │ Pod 响应 → Ingress Controller → 返回 Client                                     │   │
│  │                                                                                  │   │
│  │ 可选处理:                                                                        │   │
│  │   - 响应压缩 (gzip/brotli)                                                      │   │
│  │   - 响应头修改                                                                   │   │
│  │   - 缓存 (如配置)                                                               │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                         │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 五、Ingress 配置示例

### 5.1 基础配置示例

#### 5.1.1 最简单的 Ingress (单服务)

```yaml
# minimal-ingress.yaml
# 将所有流量转发到一个服务
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: minimal-ingress
  namespace: default
spec:
  ingressClassName: nginx
  defaultBackend:
    service:
      name: web-service
      port:
        number: 80
```

#### 5.1.2 基于主机名的路由

```yaml
# host-based-routing.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: host-based-routing
  namespace: production
  labels:
    app: multi-domain
    environment: production
spec:
  ingressClassName: nginx
  rules:
  # 主站
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: www-frontend
            port:
              number: 80
  # API 服务
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-gateway
            port:
              number: 8080
  # 管理后台
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-dashboard
            port:
              number: 3000
  # 文档站点
  - host: docs.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: docs-service
            port:
              number: 80
```

#### 5.1.3 基于路径的路由

```yaml
# path-based-routing.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: path-based-routing
  namespace: production
  annotations:
    # NGINX 特定: 启用 URL 重写
    nginx.ingress.kubernetes.io/rewrite-target: /$2
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      # API v1 - 精确匹配健康检查端点
      - path: /api/v1/health
        pathType: Exact
        backend:
          service:
            name: api-v1-health
            port:
              number: 80
      # API v1 服务
      - path: /api/v1(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-v1-service
            port:
              number: 8080
      # API v2 服务
      - path: /api/v2(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-v2-service
            port:
              number: 8080
      # 用户服务
      - path: /users
        pathType: Prefix
        backend:
          service:
            name: user-service
            port:
              number: 8081
      # 订单服务
      - path: /orders
        pathType: Prefix
        backend:
          service:
            name: order-service
            port:
              number: 8082
      # 静态资源
      - path: /static
        pathType: Prefix
        backend:
          service:
            name: static-service
            port:
              number: 80
      # 默认 - 前端应用 (最低优先级)
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend-service
            port:
              number: 80
```

#### 5.1.4 混合路由 (主机名 + 路径)

```yaml
# mixed-routing.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mixed-routing
  namespace: production
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - "*.example.com"
    secretName: wildcard-tls-secret
  rules:
  # 生产环境 API - 多版本
  - host: api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1
            port:
              number: 8080
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2
            port:
              number: 8080
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-latest
            port:
              number: 8080
  # 移动端专用 API
  - host: mobile-api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: mobile-bff
            port:
              number: 8080
  # Web 应用
  - host: www.example.com
    http:
      paths:
      - path: /assets
        pathType: Prefix
        backend:
          service:
            name: cdn-origin
            port:
              number: 80
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-frontend
            port:
              number: 3000
```

### 5.2 TLS 配置示例

#### 5.2.1 基础 TLS 配置

```yaml
# tls-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tls-ingress
  namespace: production
  annotations:
    # 强制 HTTPS 重定向
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    # HSTS 配置
    nginx.ingress.kubernetes.io/hsts: "true"
    nginx.ingress.kubernetes.io/hsts-max-age: "31536000"
    nginx.ingress.kubernetes.io/hsts-include-subdomains: "true"
    nginx.ingress.kubernetes.io/hsts-preload: "true"
spec:
  ingressClassName: nginx
  tls:
  # 单域名证书
  - hosts:
    - api.example.com
    secretName: api-tls-secret
  # 通配符证书
  - hosts:
    - "*.example.com"
    secretName: wildcard-tls-secret
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
  - host: www.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-service
            port:
              number: 80
---
# TLS Secret
apiVersion: v1
kind: Secret
metadata:
  name: api-tls-secret
  namespace: production
type: kubernetes.io/tls
data:
  # base64 编码的证书和私钥
  tls.crt: LS0tLS1CRUdJTi...
  tls.key: LS0tLS1CRUdJTi...
```

#### 5.2.2 cert-manager 自动证书管理

```yaml
# cert-manager-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auto-tls-ingress
  namespace: production
  annotations:
    # cert-manager 自动签发证书
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    # 可选: 使用 DNS01 challenge
    # cert-manager.io/acme-challenge-type: dns01
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    - www.example.com
    secretName: example-com-tls  # cert-manager 自动创建
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
# ClusterIssuer 配置
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    email: admin@example.com
    server: https://acme-v02.api.letsencrypt.org/directory
    privateKeySecretRef:
      name: letsencrypt-prod-account-key
    solvers:
    # HTTP01 Challenge (需要 Ingress 可从公网访问)
    - http01:
        ingress:
          class: nginx
    # 或 DNS01 Challenge (适用于内网 Ingress)
    # - dns01:
    #     cloudflare:
    #       email: admin@example.com
    #       apiTokenSecretRef:
    #         name: cloudflare-api-token
    #         key: api-token
```

### 5.3 高级配置示例

#### 5.3.1 金丝雀发布配置 (NGINX Ingress)

```yaml
# canary-ingress.yaml
# 主 Ingress - 生产流量
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: production-ingress
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-stable
            port:
              number: 8080
---
# 金丝雀 Ingress - 10% 流量
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: canary-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    # 方式1: 按权重分流 (10%)
    nginx.ingress.kubernetes.io/canary-weight: "10"
    # 方式2: 按 Header 分流 (可与权重组合)
    # nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
    # nginx.ingress.kubernetes.io/canary-by-header-value: "true"
    # 方式3: 按 Cookie 分流
    # nginx.ingress.kubernetes.io/canary-by-cookie: "canary"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-canary
            port:
              number: 8080
```

#### 5.3.2 限流配置

```yaml
# rate-limit-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: rate-limited-ingress
  namespace: production
  annotations:
    # 全局限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-rpm: "1000"
    # 并发连接限制
    nginx.ingress.kubernetes.io/limit-connections: "10"
    # 限流白名单
    nginx.ingress.kubernetes.io/limit-whitelist: "10.0.0.0/8,192.168.0.0/16"
    # 超出限流返回 429
    nginx.ingress.kubernetes.io/limit-rate-after: "10m"
    nginx.ingress.kubernetes.io/limit-rate: "100k"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

#### 5.3.3 认证配置

```yaml
# auth-ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: auth-ingress
  namespace: production
  annotations:
    # Basic Auth
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: basic-auth-secret
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required"
    # 或 External Auth (OAuth2 Proxy)
    # nginx.ingress.kubernetes.io/auth-url: "https://oauth2-proxy.example.com/oauth2/auth"
    # nginx.ingress.kubernetes.io/auth-signin: "https://oauth2-proxy.example.com/oauth2/start?rd=$scheme://$host$request_uri"
spec:
  ingressClassName: nginx
  rules:
  - host: admin.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: admin-service
            port:
              number: 8080
---
# Basic Auth Secret (htpasswd 格式)
apiVersion: v1
kind: Secret
metadata:
  name: basic-auth-secret
  namespace: production
type: Opaque
data:
  # 生成方式: htpasswd -c auth admin
  auth: YWRtaW46JGFwcjEkc29tZXNhbHQkdGhlcGFzc3dvcmRoYXNo
```

### 5.4 生产环境完整配置示例

```yaml
# production-ingress-complete.yaml
---
# IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx-production
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
---
# TLS Secret (由 cert-manager 管理)
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-example-com
  namespace: production
spec:
  secretName: api-example-com-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - api.example.com
  - "*.api.example.com"
---
# Production Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-gateway-ingress
  namespace: production
  labels:
    app: api-gateway
    environment: production
    team: platform
  annotations:
    # 基础配置
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    
    # 安全加固
    nginx.ingress.kubernetes.io/hsts: "true"
    nginx.ingress.kubernetes.io/hsts-max-age: "31536000"
    nginx.ingress.kubernetes.io/hsts-include-subdomains: "true"
    nginx.ingress.kubernetes.io/hsts-preload: "true"
    
    # 代理配置
    nginx.ingress.kubernetes.io/proxy-body-size: "100m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "16k"
    nginx.ingress.kubernetes.io/proxy-buffers-number: "4"
    
    # 负载均衡
    nginx.ingress.kubernetes.io/upstream-hash-by: "$request_uri"
    nginx.ingress.kubernetes.io/load-balance: "ewma"
    
    # 限流
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/limit-connections: "50"
    
    # 后端协议
    nginx.ingress.kubernetes.io/backend-protocol: "HTTP"
    
    # 跨域配置
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://www.example.com,https://app.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/cors-allow-headers: "DNT,X-CustomHeader,Keep-Alive,User-Agent,X-Requested-With,If-Modified-Since,Cache-Control,Content-Type,Authorization"
    nginx.ingress.kubernetes.io/cors-allow-credentials: "true"
    nginx.ingress.kubernetes.io/cors-max-age: "86400"
    
    # 自定义响应头
    nginx.ingress.kubernetes.io/configuration-snippet: |
      more_set_headers "X-Content-Type-Options: nosniff";
      more_set_headers "X-Frame-Options: DENY";
      more_set_headers "X-XSS-Protection: 1; mode=block";
      more_set_headers "Referrer-Policy: strict-origin-when-cross-origin";
    
    # 访问日志
    nginx.ingress.kubernetes.io/enable-access-log: "true"
    
    # 监控集成
    prometheus.io/scrape: "true"
    prometheus.io/port: "10254"
spec:
  ingressClassName: nginx-production
  tls:
  - hosts:
    - api.example.com
    - "*.api.example.com"
    secretName: api-example-com-tls
  rules:
  # API v2 (当前稳定版)
  - host: api.example.com
    http:
      paths:
      # 健康检查 - 精确匹配
      - path: /health
        pathType: Exact
        backend:
          service:
            name: health-check-service
            port:
              number: 80
      # API v2 路由
      - path: /v2
        pathType: Prefix
        backend:
          service:
            name: api-v2-service
            port:
              number: 8080
      # API v1 路由 (旧版本，逐步下线)
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: api-v1-service
            port:
              number: 8080
      # 默认路由到最新版本
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-v2-service
            port:
              number: 8080
  # 内部 API (子域名)
  - host: internal.api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: internal-api-service
            port:
              number: 8080
---
# 对应的后端 Service
apiVersion: v1
kind: Service
metadata:
  name: api-v2-service
  namespace: production
  labels:
    app: api
    version: v2
spec:
  type: ClusterIP
  selector:
    app: api
    version: v2
  ports:
  - name: http
    port: 8080
    targetPort: 8080
    protocol: TCP
---
# Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-v2
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api
      version: v2
  template:
    metadata:
      labels:
        app: api
        version: v2
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: api
      containers:
      - name: api
        image: api-server:v2.0.0
        ports:
        - name: http
          containerPort: 8080
        readinessProbe:
          httpGet:
            path: /health/ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /health/live
            port: 8080
          initialDelaySeconds: 15
          periodSeconds: 20
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 2
            memory: 2Gi
---
# PodDisruptionBudget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-v2-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: api
      version: v2
```

---

## 六、NGINX Ingress 常用注解参考

### 6.1 基础配置注解

| 注解 | 默认值 | 说明 | 示例 |
|-----|-------|------|------|
| `nginx.ingress.kubernetes.io/ssl-redirect` | true | HTTPS 重定向 | `"true"` |
| `nginx.ingress.kubernetes.io/force-ssl-redirect` | false | 强制 HTTPS 重定向 | `"true"` |
| `nginx.ingress.kubernetes.io/backend-protocol` | HTTP | 后端协议 | `"HTTP"`, `"HTTPS"`, `"GRPC"`, `"GRPCS"` |
| `nginx.ingress.kubernetes.io/rewrite-target` | - | URL 重写目标 | `"/$2"` |
| `nginx.ingress.kubernetes.io/use-regex` | false | 启用正则路径 | `"true"` |
| `nginx.ingress.kubernetes.io/app-root` | - | 应用根路径重定向 | `"/app"` |
| `nginx.ingress.kubernetes.io/default-backend` | - | 自定义默认后端 | `"namespace/service"` |

### 6.2 代理配置注解

| 注解 | 默认值 | 说明 | 生产建议 |
|-----|-------|------|---------|
| `nginx.ingress.kubernetes.io/proxy-body-size` | 1m | 请求体大小限制 | 根据业务，如 `"100m"` |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | 5 | 连接超时 (秒) | `"10"` |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | 60 | 读取超时 (秒) | `"60"` |
| `nginx.ingress.kubernetes.io/proxy-send-timeout` | 60 | 发送超时 (秒) | `"60"` |
| `nginx.ingress.kubernetes.io/proxy-buffer-size` | 4k | 代理缓冲区大小 | `"16k"` |
| `nginx.ingress.kubernetes.io/proxy-buffers-number` | 4 | 缓冲区数量 | `"8"` |
| `nginx.ingress.kubernetes.io/proxy-next-upstream` | error timeout | 故障转移条件 | `"error timeout http_502 http_503"` |
| `nginx.ingress.kubernetes.io/proxy-next-upstream-tries` | 3 | 故障转移次数 | `"3"` |

### 6.3 负载均衡注解

| 注解 | 默认值 | 说明 | 可选值 |
|-----|-------|------|-------|
| `nginx.ingress.kubernetes.io/load-balance` | round_robin | 负载均衡算法 | `round_robin`, `least_conn`, `ip_hash`, `ewma` |
| `nginx.ingress.kubernetes.io/upstream-hash-by` | - | 一致性哈希键 | `"$request_uri"`, `"$remote_addr"` |
| `nginx.ingress.kubernetes.io/affinity` | - | 会话亲和性 | `"cookie"` |
| `nginx.ingress.kubernetes.io/session-cookie-name` | INGRESSCOOKIE | 会话 Cookie 名称 | 自定义名称 |
| `nginx.ingress.kubernetes.io/session-cookie-max-age` | - | Cookie 过期时间 | `"172800"` |
| `nginx.ingress.kubernetes.io/session-cookie-path` | / | Cookie 路径 | `"/"` |

### 6.4 安全相关注解

| 注解 | 默认值 | 说明 | 生产建议 |
|-----|-------|------|---------|
| `nginx.ingress.kubernetes.io/whitelist-source-range` | - | IP 白名单 | CIDR 格式 |
| `nginx.ingress.kubernetes.io/limit-rps` | - | 每秒请求限制 | `"100"` |
| `nginx.ingress.kubernetes.io/limit-rpm` | - | 每分钟请求限制 | `"1000"` |
| `nginx.ingress.kubernetes.io/limit-connections` | - | 并发连接限制 | `"50"` |
| `nginx.ingress.kubernetes.io/auth-type` | - | 认证类型 | `basic`, `digest` |
| `nginx.ingress.kubernetes.io/auth-secret` | - | 认证 Secret | Secret 名称 |
| `nginx.ingress.kubernetes.io/auth-url` | - | 外部认证 URL | OAuth2 Proxy URL |
| `nginx.ingress.kubernetes.io/enable-modsecurity` | false | 启用 ModSecurity WAF | `"true"` |
| `nginx.ingress.kubernetes.io/modsecurity-snippet` | - | ModSecurity 规则 | 自定义规则 |

### 6.5 TLS/SSL 注解

| 注解 | 默认值 | 说明 | 推荐值 |
|-----|-------|------|-------|
| `nginx.ingress.kubernetes.io/ssl-protocols` | TLSv1.2 TLSv1.3 | TLS 协议版本 | `"TLSv1.2 TLSv1.3"` |
| `nginx.ingress.kubernetes.io/ssl-ciphers` | - | 密码套件 | 现代密码套件 |
| `nginx.ingress.kubernetes.io/ssl-prefer-server-ciphers` | true | 优先服务端密码 | `"true"` |
| `nginx.ingress.kubernetes.io/hsts` | false | 启用 HSTS | `"true"` |
| `nginx.ingress.kubernetes.io/hsts-max-age` | 15724800 | HSTS 有效期 | `"31536000"` |
| `nginx.ingress.kubernetes.io/hsts-include-subdomains` | false | HSTS 包含子域 | `"true"` |
| `nginx.ingress.kubernetes.io/hsts-preload` | false | HSTS 预加载 | `"true"` |
| `nginx.ingress.kubernetes.io/ssl-passthrough` | false | TLS 透传 | `"true"` (特殊场景) |

### 6.6 金丝雀发布注解

| 注解 | 说明 | 示例 |
|-----|------|------|
| `nginx.ingress.kubernetes.io/canary` | 启用金丝雀 | `"true"` |
| `nginx.ingress.kubernetes.io/canary-weight` | 权重分流 (0-100) | `"10"` |
| `nginx.ingress.kubernetes.io/canary-by-header` | Header 分流键 | `"X-Canary"` |
| `nginx.ingress.kubernetes.io/canary-by-header-value` | Header 分流值 | `"true"` |
| `nginx.ingress.kubernetes.io/canary-by-header-pattern` | Header 正则匹配 | `"^(true\|yes)$"` |
| `nginx.ingress.kubernetes.io/canary-by-cookie` | Cookie 分流 | `"canary"` |

---

## 七、Ingress 版本演进

### 7.1 API 版本演进历史

| 版本 | API Version | 状态 | K8s 版本 | 说明 |
|-----|-------------|------|---------|------|
| **Alpha** | `extensions/v1beta1` | 已移除 | v1.1 - v1.13 | 最初版本 |
| **Beta** | `networking.k8s.io/v1beta1` | 已移除 | v1.14 - v1.21 | 迁移到 networking API Group |
| **GA** | `networking.k8s.io/v1` | 稳定 | v1.19+ | 当前生产版本 |

### 7.2 各 K8s 版本 Ingress 相关变更

| K8s 版本 | Ingress 相关变更 | 影响 |
|---------|-----------------|------|
| **v1.18** | IngressClass 引入 (Beta)，pathType 字段引入 | 需要指定 pathType |
| **v1.19** | Ingress 和 IngressClass GA，networking.k8s.io/v1 稳定 | 推荐使用 GA 版本 |
| **v1.20** | `kubernetes.io/ingress.class` 注解弃用警告 | 建议迁移到 ingressClassName |
| **v1.22** | `extensions/v1beta1` 和 `networking.k8s.io/v1beta1` 移除 | 必须使用 v1 |
| **v1.25** | 默认 IngressClass 行为改进，多个默认 IngressClass 处理优化 | 更一致的行为 |
| **v1.28** | Ingress 路径匹配增强，更严格的验证 | 验证更严格 |
| **v1.29** | PortStatus 字段添加到 IngressLoadBalancerStatus | 更多状态信息 |
| **v1.30** | Gateway API v1.0 GA | Ingress 与 Gateway API 长期共存 |
| **v1.31** | 持续稳定性改进 | - |
| **v1.32** | 当前最新稳定版 | - |

### 7.3 API 迁移指南

```yaml
# 旧版本 (v1.21 前) - 已不支持
apiVersion: extensions/v1beta1  # 或 networking.k8s.io/v1beta1
kind: Ingress
metadata:
  name: old-ingress
  annotations:
    kubernetes.io/ingress.class: nginx  # 旧方式指定控制器
spec:
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        backend:
          serviceName: web-service  # 旧字段名
          servicePort: 80           # 旧字段名
---
# 新版本 (v1.22+) - 当前标准
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: new-ingress
  # annotations 中的 ingress.class 不再推荐
spec:
  ingressClassName: nginx  # 新方式指定控制器
  rules:
  - host: example.com
    http:
      paths:
      - path: /
        pathType: Prefix  # 必填字段
        backend:
          service:        # 结构化配置
            name: web-service
            port:
              number: 80  # 或 name: http
```

**迁移步骤:**
1. 将 `apiVersion` 改为 `networking.k8s.io/v1`
2. 添加 `spec.ingressClassName` 替代 `kubernetes.io/ingress.class` 注解
3. 为每个 path 添加 `pathType` 字段
4. 将 `backend.serviceName/servicePort` 改为 `backend.service.name/port.number`

---

## 八、kubectl Ingress 操作命令

### 8.1 基础命令

| 命令 | 说明 | 示例 |
|-----|------|------|
| `kubectl get ingress` | 列出当前命名空间的 Ingress | `kubectl get ing` |
| `kubectl get ingress -A` | 列出所有命名空间的 Ingress | `kubectl get ing -A` |
| `kubectl get ingress -o wide` | 显示更多信息 (ADDRESS, PORTS) | `kubectl get ing -o wide` |
| `kubectl get ingress <name> -o yaml` | 查看 Ingress YAML | `kubectl get ing my-ingress -o yaml` |
| `kubectl describe ingress <name>` | 查看 Ingress 详情和事件 | `kubectl describe ing my-ingress` |
| `kubectl delete ingress <name>` | 删除 Ingress | `kubectl delete ing my-ingress` |
| `kubectl edit ingress <name>` | 编辑 Ingress | `kubectl edit ing my-ingress` |

### 8.2 创建 Ingress 命令

```bash
# 创建简单 Ingress
kubectl create ingress simple-ingress \
  --class=nginx \
  --rule="demo.example.com/*=demo-service:80"

# 创建带 TLS 的 Ingress
kubectl create ingress tls-ingress \
  --class=nginx \
  --rule="secure.example.com/*=web-service:80,tls=my-tls-secret"

# 创建多规则 Ingress
kubectl create ingress multi-rule-ingress \
  --class=nginx \
  --rule="api.example.com/v1*=api-v1:8080" \
  --rule="api.example.com/v2*=api-v2:8080" \
  --rule="www.example.com/*=frontend:80"

# 创建带默认后端的 Ingress
kubectl create ingress default-backend-ingress \
  --class=nginx \
  --default-backend=default-service:80 \
  --rule="app.example.com/*=app-service:8080"
```

### 8.3 调试和排障命令

```bash
# 查看 IngressClass
kubectl get ingressclass
kubectl describe ingressclass nginx

# 查看默认 IngressClass
kubectl get ingressclass -o jsonpath='{.items[?(@.metadata.annotations.ingressclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# 检查 Ingress Controller Pod 状态
kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --tail=100

# 查看 Ingress Controller 配置 (NGINX)
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- cat /etc/nginx/nginx.conf
kubectl exec -n ingress-nginx deploy/ingress-nginx-controller -- nginx -T

# 测试 Ingress 配置
# 方式1: 通过 curl
curl -H "Host: api.example.com" http://<INGRESS-IP>/
curl -k -H "Host: api.example.com" https://<INGRESS-IP>/

# 方式2: 通过临时 Pod
kubectl run -it --rm debug --image=curlimages/curl --restart=Never -- \
  curl -H "Host: api.example.com" http://ingress-nginx-controller.ingress-nginx.svc/

# 查看 Ingress 状态
kubectl get ingress -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.loadBalancer.ingress[*].ip}{"\n"}{end}'

# 检查后端 Service 和 Endpoints
kubectl get svc <service-name>
kubectl get endpoints <service-name>
kubectl get endpointslices -l kubernetes.io/service-name=<service-name>

# 查看 Ingress Events
kubectl get events --field-selector involvedObject.kind=Ingress
```

### 8.4 常用排障 Shell 脚本

```bash
#!/bin/bash
# ingress-debug.sh - Ingress 排障脚本

NAMESPACE=${1:-default}
INGRESS_NAME=${2:-}

echo "=== Ingress 列表 ==="
kubectl get ingress -n $NAMESPACE -o wide

if [ -n "$INGRESS_NAME" ]; then
  echo ""
  echo "=== Ingress 详情: $INGRESS_NAME ==="
  kubectl describe ingress $INGRESS_NAME -n $NAMESPACE
  
  echo ""
  echo "=== 关联的 Service ==="
  kubectl get ingress $INGRESS_NAME -n $NAMESPACE -o jsonpath='{.spec.rules[*].http.paths[*].backend.service.name}' | tr ' ' '\n' | sort -u | while read svc; do
    echo "--- Service: $svc ---"
    kubectl get svc $svc -n $NAMESPACE -o wide
    kubectl get endpoints $svc -n $NAMESPACE
  done
  
  echo ""
  echo "=== TLS 证书 ==="
  kubectl get ingress $INGRESS_NAME -n $NAMESPACE -o jsonpath='{.spec.tls[*].secretName}' | tr ' ' '\n' | while read secret; do
    echo "--- Secret: $secret ---"
    kubectl get secret $secret -n $NAMESPACE -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates -subject
  done
fi

echo ""
echo "=== IngressClass ==="
kubectl get ingressclass

echo ""
echo "=== Ingress Controller Pods ==="
kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller

echo ""
echo "=== 最近 Events ==="
kubectl get events -n $NAMESPACE --sort-by='.lastTimestamp' | grep -i ingress | tail -20
```

---

## 九、Ingress 设计原则与最佳实践

### 9.1 设计原则

| 原则 | 说明 | 实践建议 |
|-----|------|---------|
| **单一职责** | 每个 Ingress 负责一个域名或一组紧密相关的服务 | 避免单个 Ingress 过于复杂 |
| **命名规范** | 使用有意义、一致的命名 | `<app>-<env>-ingress` 或 `<domain>-ingress` |
| **版本控制** | Ingress 配置纳入 GitOps | 使用 Git 管理，配合 ArgoCD/Flux |
| **安全优先** | 默认启用 TLS，配置安全头 | 强制 HTTPS、HSTS、安全响应头 |
| **可观测性** | 配置监控和日志 | Prometheus 指标、访问日志、告警 |
| **渐进发布** | 使用金丝雀/蓝绿部署 | 降低发布风险 |
| **资源隔离** | 按环境/团队使用不同 IngressClass 或命名空间 | 避免配置冲突 |

### 9.2 反模式与解决方案

| 反模式 | 问题 | 正确做法 |
|-------|-----|---------|
| **无 IngressClass** | 依赖隐式默认，升级可能中断 | 始终指定 `ingressClassName` |
| **单点故障** | 控制器无高可用 | 多副本 + PodAntiAffinity + PDB |
| **资源无限制** | 控制器 OOM/CPU 饱和 | 配置合理的 requests/limits |
| **无 TLS** | 明文传输敏感数据 | 启用 TLS，强制 HTTPS 重定向 |
| **过长超时** | 连接资源耗尽 | 合理配置超时 (10s 连接，60s 读取) |
| **无速率限制** | 易受 DDoS 攻击 | 配置限流策略 |
| **过大 Ingress** | 单个 Ingress 规则过多，难以维护 | 拆分为多个 Ingress |
| **硬编码 IP** | IP 变更导致配置失效 | 使用 DNS 和 Service 名称 |
| **忽略健康检查** | 流量发送到不健康 Pod | 配置 readiness/liveness probe |

### 9.3 生产环境检查清单

| 检查项 | 状态 | 说明 |
|-------|-----|------|
| 高可用部署 | [ ] | 控制器至少 3 副本，跨可用区分布 |
| TLS 配置 | [ ] | 所有 Ingress 启用 TLS，证书自动续期 |
| HTTPS 重定向 | [ ] | HTTP 自动重定向到 HTTPS |
| HSTS 配置 | [ ] | 启用 HSTS，建议预加载 |
| 限流策略 | [ ] | 配置 RPS/连接数限制 |
| 超时配置 | [ ] | 合理的连接/读取/发送超时 |
| 请求体限制 | [ ] | 根据业务配置 proxy-body-size |
| 监控告警 | [ ] | Prometheus 指标、Grafana 仪表板、告警规则 |
| 访问日志 | [ ] | 启用访问日志，配置日志轮转 |
| 安全头 | [ ] | X-Content-Type-Options, X-Frame-Options 等 |
| 后端健康检查 | [ ] | Service 后端 Pod 有 readinessProbe |
| PDB 配置 | [ ] | 控制器和后端服务配置 PodDisruptionBudget |
| 资源限制 | [ ] | 控制器配置 CPU/内存 requests/limits |
| 网络策略 | [ ] | NetworkPolicy 限制 Ingress Controller 访问范围 |
| 备份恢复 | [ ] | Ingress 配置纳入备份策略 |

---

## 十、故障排除

### 10.1 常见问题诊断

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|-----|---------|---------|---------|
| **Ingress 无 ADDRESS** | 控制器未运行或未分配 IP | `kubectl get pods -n ingress-nginx` | 检查控制器状态，查看 Service 类型 |
| **404 Not Found** | 规则不匹配或后端不存在 | `kubectl describe ingress <name>` | 检查 host/path 配置，确认 Service 存在 |
| **502 Bad Gateway** | 后端 Pod 不健康 | `kubectl get endpoints <service>` | 检查 Pod 状态和 readinessProbe |
| **503 Service Unavailable** | 无可用后端 | `kubectl get endpoints <service>` | 确认 Pod 运行且通过健康检查 |
| **SSL 证书错误** | 证书不匹配或过期 | `openssl s_client -connect <host>:443` | 检查 Secret 证书有效性 |
| **路径匹配问题** | pathType 配置错误 | 对比请求路径和 Ingress 规则 | 调整 pathType 和 path |
| **超时错误** | 后端响应慢或超时配置过短 | `kubectl logs <ingress-pod>` | 调整超时参数或优化后端 |
| **限流生效** | 触发限流规则 | 查看 429 状态码 | 调整限流参数或优化客户端 |

### 10.2 诊断流程图

```
请求失败
    │
    ├─→ 检查 DNS 解析
    │      │
    │      └─ nslookup <host>
    │              │
    │              ├─ 解析失败 → 检查 DNS 记录
    │              │
    │              └─ 解析成功 ↓
    │
    ├─→ 检查网络连通性
    │      │
    │      └─ curl -v http://<ip>
    │              │
    │              ├─ 连接失败 → 检查防火墙、LoadBalancer
    │              │
    │              └─ 连接成功 ↓
    │
    ├─→ 检查 TLS
    │      │
    │      └─ curl -kv https://<host>
    │              │
    │              ├─ 证书错误 → 检查 TLS Secret
    │              │
    │              └─ TLS 正常 ↓
    │
    ├─→ 检查 Ingress 规则
    │      │
    │      └─ kubectl describe ingress <name>
    │              │
    │              ├─ 规则不匹配 → 调整 host/path/pathType
    │              │
    │              └─ 规则匹配 ↓
    │
    ├─→ 检查后端 Service
    │      │
    │      └─ kubectl get endpoints <svc>
    │              │
    │              ├─ Endpoints 为空 → 检查 Pod 和 Selector
    │              │
    │              └─ Endpoints 存在 ↓
    │
    └─→ 检查 Pod 健康状态
           │
           └─ kubectl describe pod <pod>
                   │
                   ├─ Pod 不健康 → 修复 Pod 问题
                   │
                   └─ Pod 健康 → 检查应用日志
```

### 10.3 常用诊断命令集合

```bash
#!/bin/bash
# ingress-troubleshoot.sh

# 1. 检查 Ingress Controller 状态
echo "=== Ingress Controller Status ==="
kubectl get pods -n ingress-nginx -l app.kubernetes.io/component=controller -o wide
kubectl get svc -n ingress-nginx

# 2. 检查 Ingress 资源
echo ""
echo "=== Ingress Resources ==="
kubectl get ingress -A -o wide

# 3. 检查特定 Ingress
INGRESS_NAME=${1:-""}
NAMESPACE=${2:-"default"}
if [ -n "$INGRESS_NAME" ]; then
  echo ""
  echo "=== Ingress Details: $INGRESS_NAME ==="
  kubectl describe ingress $INGRESS_NAME -n $NAMESPACE
  
  # 4. 检查关联的 Service 和 Endpoints
  echo ""
  echo "=== Backend Services ==="
  SERVICES=$(kubectl get ingress $INGRESS_NAME -n $NAMESPACE -o jsonpath='{.spec.rules[*].http.paths[*].backend.service.name}')
  for svc in $SERVICES; do
    echo "--- Service: $svc ---"
    kubectl get svc $svc -n $NAMESPACE -o wide
    kubectl get endpoints $svc -n $NAMESPACE
    kubectl get endpointslices -l kubernetes.io/service-name=$svc -n $NAMESPACE
  done
fi

# 5. 检查控制器日志
echo ""
echo "=== Controller Logs (last 50 lines) ==="
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller --tail=50

# 6. 检查 Events
echo ""
echo "=== Recent Events ==="
kubectl get events -A --sort-by='.lastTimestamp' | grep -i ingress | tail -20

# 7. 测试连通性
INGRESS_IP=$(kubectl get svc -n ingress-nginx ingress-nginx-controller -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
if [ -n "$INGRESS_IP" ]; then
  echo ""
  echo "=== Connectivity Test ==="
  echo "Ingress IP: $INGRESS_IP"
  curl -s -o /dev/null -w "HTTP Status: %{http_code}\n" http://$INGRESS_IP/
fi
```

---

## 十一、Ingress 与 Gateway API 对比

### 11.1 功能对比

| 功能 | Ingress | Gateway API | 说明 |
|-----|---------|-------------|------|
| **协议支持** | HTTP/HTTPS | HTTP/HTTPS/TCP/UDP/gRPC/TLS | Gateway API 更全面 |
| **路由能力** | Host/Path | Host/Path/Header/Query/Method | Gateway API 更丰富 |
| **流量分割** | 注解 (控制器特定) | 原生支持 (BackendRef weight) | Gateway API 标准化 |
| **跨命名空间** | 受限 | ReferenceGrant 支持 | Gateway API 更灵活 |
| **角色分离** | 无 | Gateway/HTTPRoute 分离 | Gateway API 更清晰 |
| **扩展方式** | 注解 | Policy Attachment | Gateway API 更标准 |
| **成熟度** | GA (稳定) | GA (v1.0) | 两者都稳定 |
| **控制器支持** | 广泛 | 逐渐增加 | Ingress 更成熟 |

### 11.2 选型建议

| 场景 | 推荐方案 | 理由 |
|-----|---------|------|
| **简单 HTTP 路由** | Ingress | 成熟稳定，配置简单 |
| **现有 Ingress 基础设施** | Ingress | 避免迁移成本 |
| **多协议需求 (TCP/UDP/gRPC)** | Gateway API | 原生支持非 HTTP 协议 |
| **复杂流量管理** | Gateway API | 更丰富的路由和流量分割 |
| **多团队协作** | Gateway API | 清晰的角色分离 |
| **跨命名空间路由** | Gateway API | ReferenceGrant 机制 |
| **服务网格集成** | Gateway API | 与 Istio 等更好集成 |
| **新项目** | Gateway API | 面向未来的选择 |

### 11.3 迁移路径

```
当前状态                                     目标状态
┌─────────────────────┐                    ┌─────────────────────┐
│     Ingress         │                    │    Gateway API      │
│                     │                    │                     │
│  ┌───────────────┐ │                    │  ┌───────────────┐ │
│  │ Ingress       │ │   渐进式迁移        │  │ Gateway       │ │
│  │ (多规则)      │ │ ──────────────────> │  │ (基础设施层)  │ │
│  └───────────────┘ │                    │  └───────────────┘ │
│                     │                    │         │          │
│                     │                    │         ▼          │
│                     │                    │  ┌───────────────┐ │
│                     │                    │  │ HTTPRoute     │ │
│                     │                    │  │ (应用层)      │ │
│                     │                    │  └───────────────┘ │
└─────────────────────┘                    └─────────────────────┘

迁移策略:
1. 评估当前 Ingress 使用情况
2. 确认 Ingress Controller 支持 Gateway API
3. 创建 Gateway 资源 (平台团队)
4. 逐步将 Ingress 转换为 HTTPRoute
5. 验证功能和性能
6. 完成迁移，下线旧 Ingress
```

---

## 十二、资源依赖关系

### 12.1 Ingress 相关资源关系图

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           Ingress 资源依赖关系                                       │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│   ┌────────────────────────────────────────────────────────────────────────────┐    │
│   │                            IngressClass                                     │    │
│   │  spec.controller: k8s.io/ingress-nginx                                     │    │
│   │  spec.parameters: → IngressNginxConfiguration (可选)                        │    │
│   └────────────────────────────────────────────────────────────────────────────┘    │
│                                        ↑                                             │
│                            spec.ingressClassName                                    │
│                                        │                                             │
│   ┌────────────────────────────────────┴───────────────────────────────────────┐    │
│   │                              Ingress                                        │    │
│   │                                                                             │    │
│   │   metadata:                                                                 │    │
│   │     name: my-ingress                                                        │    │
│   │     namespace: production                                                   │    │
│   │     annotations: {...}  ←─── 控制器特定配置                                 │    │
│   │                                                                             │    │
│   │   spec:                                                                     │    │
│   │     ingressClassName: nginx  ───→ IngressClass                             │    │
│   │     tls:                                                                    │    │
│   │       - secretName: tls-secret ───→ Secret (TLS)                           │    │
│   │     rules:                                                                  │    │
│   │       - backend.service.name ───→ Service                                  │    │
│   └─────────────────────────────────────────────────────────────────────────────┘    │
│            │                              │                              │            │
│            │                              │                              │            │
│            ▼                              ▼                              ▼            │
│   ┌─────────────────┐          ┌─────────────────┐          ┌─────────────────┐     │
│   │  Secret (TLS)   │          │    Service      │          │ Ingress Ctrl    │     │
│   │                 │          │                 │          │   Deployment    │     │
│   │  type: tls      │          │  type:ClusterIP │          │                 │     │
│   │  tls.crt        │          │  selector: ...  │          │  Watch:         │     │
│   │  tls.key        │          │  ports: ...     │          │  - Ingress      │     │
│   └─────────────────┘          └────────┬────────┘          │  - Service      │     │
│                                         │                    │  - Endpoints    │     │
│                                         │                    │  - Secret       │     │
│                                         ▼                    └─────────────────┘     │
│                                ┌─────────────────┐                                   │
│                                │  EndpointSlice  │                                   │
│                                │                 │                                   │
│                                │  endpoints:     │                                   │
│                                │  - 10.244.1.10  │                                   │
│                                │  - 10.244.2.11  │                                   │
│                                └────────┬────────┘                                   │
│                                         │                                            │
│                           ┌─────────────┴─────────────┐                             │
│                           │                           │                             │
│                           ▼                           ▼                             │
│                  ┌─────────────────┐        ┌─────────────────┐                    │
│                  │      Pod 1      │        │      Pod 2      │                    │
│                  │  10.244.1.10    │        │  10.244.2.11    │                    │
│                  │  :8080          │        │  :8080          │                    │
│                  └─────────────────┘        └─────────────────┘                    │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 12.2 完整资源配置示例

```yaml
# 1. IngressClass (通常由平台团队创建)
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
---
# 2. Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    environment: production
---
# 3. TLS Secret
apiVersion: v1
kind: Secret
metadata:
  name: app-tls-secret
  namespace: production
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-certificate>
  tls.key: <base64-encoded-private-key>
---
# 4. ConfigMap (应用配置)
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
data:
  APP_ENV: "production"
  LOG_LEVEL: "info"
---
# 5. Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: production
  labels:
    app: web-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
      - name: web
        image: web-app:v1.0.0
        ports:
        - name: http
          containerPort: 8080
        envFrom:
        - configMapRef:
            name: app-config
        readinessProbe:
          httpGet:
            path: /health/ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /health/live
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 15
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
---
# 6. Service
apiVersion: v1
kind: Service
metadata:
  name: web-app-service
  namespace: production
  labels:
    app: web-app
spec:
  type: ClusterIP
  selector:
    app: web-app
  ports:
  - name: http
    port: 80
    targetPort: http
    protocol: TCP
---
# 7. Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-app-ingress
  namespace: production
  labels:
    app: web-app
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.example.com
    secretName: app-tls-secret
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-app-service
            port:
              number: 80
---
# 8. PodDisruptionBudget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-app-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: web-app
---
# 9. HorizontalPodAutoscaler
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

---

**Ingress 核心原则**: 选择合适的控制器 → 明确指定 IngressClass → 合理规划路由结构 → 启用 TLS 加密 → 配置高可用与监控 → 实施安全策略

---

> **参考文档**:  
> - [Kubernetes Ingress 官方文档](https://kubernetes.io/docs/concepts/services-networking/ingress/)
> - [Kubernetes IngressClass 文档](https://kubernetes.io/docs/concepts/services-networking/ingress/#ingress-class)
> - [NGINX Ingress Controller 文档](https://kubernetes.github.io/ingress-nginx/)
> - [Gateway API 官方文档](https://gateway-api.sigs.k8s.io/)

---

*Kusheet - Kubernetes 知识速查表项目*

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
