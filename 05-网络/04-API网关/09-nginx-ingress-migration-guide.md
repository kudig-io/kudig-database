---
title: 09 - 传统 Ingress 控制器向云原生 API 网关迁移
description: '**文档版本**: v1.0 | **适用版本**: nginx-ingress 1.x → Higress/APISIX/Kong/Envoy
  Gateway | **更新日期**: 2026-03-04 | **关键词**: 迁移, nginx-ingress, 注解映射, 零停机, 并行部署'
summary: '**文档版本**: v1.0 | **适用版本**: nginx-ingress 1.x → Higress/APISIX/Kong/Envoy
  Gateway | **更新日期**: 2026-03-04 | **关键词**: 迁移, nginx-ingress, 注解映射, 零停机, 并行部署'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- prometheus
- istio
- helm
- redis
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- 传统 Ingress 控制器向云原生 API 网关迁移 是什么
- 如何 传统 Ingress 控制器向云原生 API 网关迁移
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- 传统
- Ingress
- 控制器向云原生
- API
- 网关迁移
- cloud
- native
- api
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- redis-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/ingress-fta.md
  label: '故障树: ingress'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 09 - 传统 [[Ingress|Ingress]] 控制器向云原生 API 网关迁移

> **文档版本**: v1.0 | **适用版本**: nginx-ingress 1.x → Higress/APISIX/Kong/Envoy Gateway | **更新日期**: 2026-03-04 | **关键词**: 迁移, nginx-ingress, 注解映射, 零停机, 并行部署

<!-- chunk: 目录 -->## 目录

1. [为什么要迁移](#1-为什么要迁移)
2. [迁移策略概述](#2-迁移策略概述)
3. [Nginx 注解到插件映射表](#3-nginx-注解到插件映射表)
4. [迁移实战：nginx-ingress → Higress](#4-迁移实战nginx-ingress--higress)
5. [迁移实战：nginx-ingress → APISIX](#5-迁移实战nginx-ingress--apisix)
6. [迁移实战：nginx-ingress → Kong](#6-迁移实战nginx-ingress--kong)
7. [零停机迁移清单](#7-零停机迁移清单)
8. [常见问题与陷阱](#8-常见问题与陷阱)

---

<!-- chunk: 1. 为什么要迁移 -->## 1. 为什么要迁移

## nginx-ingress 的能力边界

nginx-ingress（ingress-nginx）是 [[Kubernetes|Kubernetes]] 社区使用最广泛的 Ingress 控制器，但随着业务复杂度提升，其局限性日益明显。

## 功能对比：nginx-ingress vs 现代 API 网关

| 能力维度 | nginx-ingress | Higress | APISIX | Kong | [[Envoy|Envoy]] Gateway |
|---------|--------------|---------|--------|------|--------------|
| **配置方式** | Ingress + 注解 | CRD + Gateway API | CRD + Admin API | CRD + Admin API | Gateway API |
| **动态配置** | ❌ 需重载 Nginx | ✅ 无重载热更新 | ✅ 无重载热更新 | ✅ 无重载 | ✅ xDS 动态 |
| **限流（全局）** | ❌ 仅单实例限流 | ✅ 全局共享限流 | ✅ 全局共享限流 | ✅ 全局共享限流 | ✅ 全局共享限流 |
| **认证插件** | 基础（BasicAuth） | JWT/OIDC/ApiKey | JWT/OIDC/多种 | JWT/OIDC/多种 | JWT/OIDC |
| **流量染色/金丝雀** | 注解实现，有限 | ✅ 精细灰度 | ✅ 精细灰度 | ✅ 精细灰度 | ✅ 权重路由 |
| **可观测性** | 基础 Prometheus | ✅ 完整三件套 | ✅ 完整三件套 | ✅ 完整三件套 | ✅ 完整三件套 |
| **插件/中间件** | 少量注解 | Wasm/Lua 插件 | Lua/Go 插件 | Lua/Go 插件 | Wasm/xDS |
| **多租户** | 命名空间隔离 | ✅ 企业级多租户 | ✅ 多租户 | ✅ 多租户 RBAC | ✅ 策略隔离 |
| **AI 网关** | ❌ | ✅ 完整 AI 能力 | ⚠️ 基础 | ✅ AI 插件 | ⚠️ 基础 |
| **Gateway API** | ⚠️ 实验性 | ✅ 支持 | ⚠️ 部分 | ⚠️ 部分 | ✅ 原生 |
| **WebSocket/gRPC** | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 支持 | ✅ 原生 |
| **配置热重载代价** | 高（Nginx reload）| 零 | 零 | 零 | 零 |

## 迁移的核心驱动力

```
nginx-ingress 痛点分析
─────────────────────────────────────────────────────────────────

① 高频更新场景卡顿
   nginx-ingress 每次路由变更需要重载 Nginx 配置
   大型集群（1000+ 路由）reload 时间可达数秒
   导致短暂的连接中断和延迟毛刺

② 功能以注解实现，难以维护
   复杂业务逻辑通过堆叠注解实现
   注解之间存在隐式依赖，难以调试
   团队协作困难，文档化成本高

③ 全局限流无法实现
   nginx-ingress 的限流是单副本级别
   多副本部署下无法实现精确的全局 QPS 限制
   无法满足 SLA 和计费场景需求

④ 插件扩展能力弱
   自定义逻辑需要修改 nginx.conf 模板
   升级 ingress-nginx 可能覆盖自定义配置
   Lua 脚本扩展门槛高，社区生态有限

⑤ 可观测性不足
   缺少请求级别的详细追踪
   难以与现代 APM 系统深度集成
   调试复杂路由问题效率低
```

---

<!-- chunk: 2. 迁移策略概述 -->## 2. 迁移策略概述

## 三种迁移模式对比

```
┌─────────────────────────────────────────────────────────────────────┐
│                      迁移策略选择矩阵                                │
├──────────────────┬──────────────────┬──────────────────────────────-┤
│  策略            │  适用场景         │  风险等级                      │
├──────────────────┼──────────────────┼──────────────────────────────-┤
│  并行部署        │ 服务数量多、业务   │  低风险                        │
│  (Parallel)      │ 复杂、零容忍停机  │  可随时回滚                    │
├──────────────────┼──────────────────┼──────────────────────────────-┤
│  增量切流        │ 中等规模、希望渐   │  中风险                        │
│  (Incremental)   │ 进验证           │  按服务逐步验证                 │
├──────────────────┼──────────────────┼──────────────────────────────-┤
│  一次性切换      │ 小规模、服务简    │  高风险                        │
│  (Big Bang)      │ 单、有维护窗口   │  建议仅用于非核心服务            │
└──────────────────┴──────────────────┴──────────────────────────────-┘
```

## 并行部署架构（推荐）

```
流量入口（DNS/LB）
        │
        │ 按域名/路径分流
        ├─────────────────────────────────────┐
        │                                     │
        ▼                                     ▼
┌───────────────┐                   ┌──────────────────┐
│  nginx-ingress │                   │  新 API 网关      │
│  (现有流量)    │                   │  (新迁移服务)     │
└───────────────┘                   └──────────────────┘
        │                                     │
   现有后端服务                           后端服务（相同）
        
阶段 1: 新服务直接使用新网关
阶段 2: 低流量服务迁移，验证正确性
阶段 3: 核心服务迁移，DNS 切流
阶段 4: 下线 nginx-ingress
```

## DNS 切流方案

```bash
# 方案 A：DNS 权重（Route53/阿里云 DNS）
# 将域名指向两个 LB IP，调整权重完成灰度

# 方案 B：同 LB 双 IngressClass（推荐）
# nginx-ingress 使用 ingressClassName: nginx
# 新网关使用 ingressClassName: higress（或其他）
# 两者共享同一 LB，通过 ingressClass 区分

# 方案 C：内部服务迁移（无 DNS 变更）
# 直接修改 ingressClassName 字段，即时生效
```

---

<!-- chunk: 3. Nginx 注解到插件映射表 -->## 3. Nginx 注解到插件映射表

## 流量控制类注解映射

| nginx-ingress 注解 | Higress 等效配置 | APISIX 等效配置 | Kong 等效配置 |
|-------------------|----------------|----------------|--------------|
| `nginx.ingress.kubernetes.io/limit-rps` | `higress.io/request-limiting` | `limit-req` 插件 | `rate-limiting` 插件 |
| `nginx.ingress.kubernetes.io/limit-connections` | `ClientTrafficPolicy` 连接限制 | `limit-conn` 插件 | `connection-limiting` 插件 |
| `nginx.ingress.kubernetes.io/proxy-body-size` | `higress.io/proxy-body-size` | `request-size-limiting` 插件 | `request-size-limiting` 插件 |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout` | `BackendTrafficPolicy` timeout | 上游超时配置 | `proxy` 插件超时 |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | `BackendTrafficPolicy` timeout | 上游超时配置 | `proxy` 插件超时 |
| `nginx.ingress.kubernetes.io/proxy-send-timeout` | `BackendTrafficPolicy` timeout | 上游超时配置 | `proxy` 插件超时 |

## 路由改写类注解映射

| nginx-ingress 注解 | Higress 等效配置 | APISIX 等效配置 | Kong 等效配置 |
|-------------------|----------------|----------------|--------------|
| `nginx.ingress.kubernetes.io/rewrite-target` | HTTPRoute URLRewrite 过滤器 | `proxy-rewrite` 插件 | `request-transformer` 插件 |
| `nginx.ingress.kubernetes.io/use-regex` | HTTPRoute PathRegexp | 正则路由 | 正则路由 |
| `nginx.ingress.kubernetes.io/app-root` | HTTPRoute 重定向 | `redirect` 插件 | `redirect` 插件 |
| `nginx.ingress.kubernetes.io/backend-protocol` | BackendTLSPolicy | 上游 scheme 配置 | `service.protocol` |

## 认证安全类注解映射

| nginx-ingress 注解 | Higress 等效配置 | APISIX 等效配置 | Kong 等效配置 |
|-------------------|----------------|----------------|--------------|
| `nginx.ingress.kubernetes.io/auth-type: basic` | `BasicAuth` 插件 | `basic-auth` 插件 | `basic-auth` 插件 |
| `nginx.ingress.kubernetes.io/auth-url` | `ExtAuth` 插件 | `forward-auth` 插件 | `ldap-auth` / 自定义 |
| `nginx.ingress.kubernetes.io/auth-secret` | JWT 插件 Secret 引用 | `jwt-auth` 插件 | `jwt` 插件 |
| `nginx.ingress.kubernetes.io/whitelist-source-range` | `SecurityPolicy` ipAllowList | `ip-restriction` 插件 | `ip-restriction` 插件 |
| `nginx.ingress.kubernetes.io/ssl-redirect` | HTTPRoute 重定向过滤器 | `redirect` 插件 | `redirect` 插件 |

## TLS/SSL 类注解映射

| nginx-ingress 注解 | Higress 等效配置 | APISIX 等效配置 | Kong 等效配置 |
|-------------------|----------------|----------------|--------------|
| `nginx.ingress.kubernetes.io/force-ssl-redirect` | 入口点重定向配置 | `redirect` 插件 HTTP→HTTPS | `redirect` 插件 |
| `nginx.ingress.kubernetes.io/ssl-passthrough` | Gateway TLS passthrough | SNI 路由 + TLS 直通 | `stream-listen` 四层 |
| `nginx.ingress.kubernetes.io/backend-ssl` | `BackendTLSPolicy` | 上游 HTTPS 配置 | `service.protocol: https` |
| `cert-manager.io/cluster-issuer` | 直接引用 Secret / cert-manager | cert-manager Secret | cert-manager Secret |

## 高级功能类注解映射

| nginx-ingress 注解 | Higress 等效配置 | APISIX 等效配置 | Kong 等效配置 |
|-------------------|----------------|----------------|--------------|
| `nginx.ingress.kubernetes.io/canary: "true"` | HTTPRoute 权重路由 | `traffic-split` 插件 | `canary-release` 插件 |
| `nginx.ingress.kubernetes.io/canary-weight` | backendRefs weight | `traffic-split` weight | `canary-release` weight |
| `nginx.ingress.kubernetes.io/canary-by-header` | HTTPRoute header 匹配 | `traffic-split` header | 路由 header 条件 |
| `nginx.ingress.kubernetes.io/configuration-snippet` | EnvoyPatchPolicy | `serverless-pre-function` | `pre-function` 插件 |
| `nginx.ingress.kubernetes.io/server-snippet` | EnvoyPatchPolicy | `serverless-pre-function` | `pre-function` 插件 |
| `nginx.ingress.kubernetes.io/enable-cors` | `SecurityPolicy` CORS | `cors` 插件 | `cors` 插件 |
| `nginx.ingress.kubernetes.io/cors-allow-origin` | `SecurityPolicy` CORS allowOrigins | `cors` 插件 allow_origins | `cors` 插件 origins |

---

<!-- chunk: 4. 迁移实战：nginx-ingress → Higress -->## 4. 迁移实战：nginx-ingress → Higress

## 迁移前：原 nginx-ingress 配置

```yaml
# 原有 Ingress 配置（nginx-ingress）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    kubernetes.io/ingress.class: "nginx"
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/use-regex: "true"
    nginx.ingress.kubernetes.io/limit-rps: "100"
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/enable-cors: "true"
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com"
    nginx.ingress.kubernetes.io/cors-allow-methods: "GET, POST, PUT, DELETE, OPTIONS"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"
    nginx.ingress.kubernetes.io/canary: "false"
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - api.example.com
    secretName: api-tls-secret
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

## 迁移后：Higress 配置

**步骤 1：安装 Higress**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Higress（使用 Helm）
helm repo add higress.io https://higress.io/helm-charts
helm install higress -n higress-system higress.io/higress \
  --create-namespace \
  --render-subchart-notes \
  --set global.local=false \
  --set global.enableIstioAPI=false
```
**步骤 2：转换路由配置（保留原 Ingress 兼容模式）**

```yaml
# 方式 A：直接修改 ingressClassName（最简单）
# 将原 Ingress 的 ingressClassName 改为 higress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    # 替换 ingress class
    kubernetes.io/ingress.class: "higress"   # ← 改为 higress
    # Higress 兼容大多数 nginx 注解，但部分需调整：
    nginx.ingress.kubernetes.io/rewrite-target: /$2   # ✅ 兼容
    nginx.ingress.kubernetes.io/use-regex: "true"     # ✅ 兼容
    nginx.ingress.kubernetes.io/proxy-body-size: "10m" # ✅ 兼容
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10" # ✅ 兼容
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"    # ✅ 兼容
    nginx.ingress.kubernetes.io/enable-cors: "true"         # ✅ 兼容
    nginx.ingress.kubernetes.io/cors-allow-origin: "https://app.example.com"
    nginx.ingress.kubernetes.io/force-ssl-redirect: "true"  # ✅ 兼容
spec:
  ingressClassName: higress      # ← 改为 higress
  # ... 其余配置不变
```

**步骤 3：升级为原生 Higress CRD（可选，充分利用高级特性）**

```yaml
# 方式 B：迁移到 Higress 原生 CRD

# 路由配置
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: default
  namespace: higress-system
spec:
  registries: []

---
# 使用 HTTPRoute（Gateway API 标准）
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-route
  namespace: production
spec:
  parentRefs:
  - name: higress-gateway
    namespace: higress-system
  hostnames:
  - "api.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /
    backendRefs:
    - name: api-service
      port: 8080

---
# 全局限流插件
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: request-limiting
  namespace: higress-system
spec:
  selector:
    matchLabels:
      higress: higress-system-higress-gateway
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/request-limiting:v1
  phase: AUTHZ
  priority: 20
  pluginConfig:
    limit_by_param: X-User-Id
    limit_by_header: ""
    limit_by_per_ip: true
    rules:
    - limit_count: 100
      limit_window: 60
      rejected_code: 429

---
# CORS 策略
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: cors-plugin
  namespace: higress-system
spec:
  selector:
    matchLabels:
      higress: higress-system-higress-gateway
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/cors:v1
  pluginConfig:
    allow_origins:
    - https://app.example.com
    allow_methods:
    - GET
    - POST
    - PUT
    - DELETE
    - OPTIONS
    allow_headers:
    - Authorization
    - Content-Type
    max_age: 3600
```

**步骤 4：验证迁移**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 并行运行验证（两个 IngressClass 同时工作）
kubectl get ingress -n production

# 测试 Higress 路由
curl -H "Host: api.example.com" \
     -H "X-Test-Gateway: higress" \
     https://<higress-lb-ip>/api/v1/users

# 对比 nginx-ingress 返回
curl -H "Host: api.example.com" \
     https://<nginx-lb-ip>/api/v1/users

# 确认结果一致后，切换 DNS/LB 到 Higress
```
---

<!-- chunk: 5. 迁移实战：nginx-ingress → APISIX -->## 5. 迁移实战：nginx-ingress → APISIX

## 迁移前原配置

```yaml
# 原 nginx-ingress 配置（含认证和限流）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production
  annotations:
    kubernetes.io/ingress.class: "nginx"
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: basic-auth-secret
    nginx.ingress.kubernetes.io/auth-realm: "Authentication Required"
    nginx.ingress.kubernetes.io/limit-rps: "50"
    nginx.ingress.kubernetes.io/canary: "false"
    nginx.ingress.kubernetes.io/configuration-snippet: |
      more_set_headers "X-Content-Type-Options: nosniff";
      more_set_headers "X-Frame-Options: DENY";
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 8080
```

## 迁移到 APISIX

**步骤 1：安装 APISIX Ingress Controller**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 APISIX（含 Ingress Controller）
helm repo add apisix https://charts.apiseven.com
helm repo update

helm install apisix apisix/apisix \
  --namespace apisix \
  --create-namespace \
  --set ingress-controller.enabled=true \
  --set ingress-controller.config.apisix.serviceNamespace=apisix
```
**步骤 2：创建 APISIX 路由和插件**

```yaml
# apisix-route-with-plugins.yaml
apiVersion: apisix.apache.org/v2
kind: ApisixRoute
metadata:
  name: app-route
  namespace: production
spec:
  http:
  - name: app-route
    match:
      hosts:
      - app.example.com
      paths:
      - /*
    backends:
    - serviceName: app-service
      servicePort: 8080
      weight: 100
    plugins:
    # 替换 basic auth
    - name: basic-auth
      enable: true
    # 替换 limit-rps
    - name: limit-req
      enable: true
      config:
        rate: 50
        burst: 10
        rejected_code: 429
    # 替换 configuration-snippet 中的 Header 设置
    - name: response-rewrite
      enable: true
      config:
        headers:
          set:
            X-Content-Type-Options: "nosniff"
            X-Frame-Options: "DENY"
```

```yaml
# apisix-consumer-basic-auth.yaml
# 创建 Consumer（BasicAuth 用户）
apiVersion: apisix.apache.org/v2
kind: ApisixConsumer
metadata:
  name: admin-user
  namespace: production
spec:
  authParameter:
    basicAuth:
      value:
        username: admin
        password: "changeme123"
```

**步骤 3：金丝雀路由迁移**

```yaml
# apisix-canary.yaml
# 原来 nginx canary 注解迁移为 traffic-split 插件
apiVersion: apisix.apache.org/v2
kind: ApisixRoute
metadata:
  name: app-canary-route
  namespace: production
spec:
  http:
  - name: app-traffic-split
    match:
      hosts:
      - app.example.com
      paths:
      - /*
    backends:
    - serviceName: app-service-v1
      servicePort: 8080
      weight: 90
    - serviceName: app-service-v2
      servicePort: 8080
      weight: 10
    plugins:
    # 基于 Header 的金丝雀
    - name: traffic-split
      enable: true
      config:
        rules:
        - matchers:
          - - vars=""
            - ["http_x_canary", "==", "true"]
          weighted_upstreams:
          - upstream:
              name: app-v2
              type: roundrobin
              nodes:
                "app-service-v2.production.svc.cluster.local:8080": 1
            weight: 100
```

**步骤 4：验证与切流**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 ApisixRoute 状态
kubectl get apisixroute -n production

# 通过 APISIX Admin API 查看路由
kubectl port-forward -n apisix svc/apisix-admin 9180:9180
curl http://localhost:9180/apisix/admin/routes \
  -H "X-API-KEY: $(kubectl get secret apisix-admin -n apisix -o jsonpath='{.data.key}' | base64 -d)"

# 功能验证
curl -H "Host: app.example.com" \
     -u admin:changeme123 \
     http://<apisix-lb-ip>/api/test

# 验证限流
for i in $(seq 1 60); do
  curl -s -o /dev/null -w "%{http_code}\n" \
    -H "Host: app.example.com" \
    -u admin:changeme123 \
    http://<apisix-lb-ip>/api/test
done
```
---

<!-- chunk: 6. 迁移实战：nginx-ingress → Kong -->## 6. 迁移实战：nginx-ingress → Kong

## 迁移前原配置

```yaml
# 原 nginx-ingress 配置（含 JWT 认证）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    kubernetes.io/ingress.class: "nginx"
    nginx.ingress.kubernetes.io/auth-url: "http://auth-service.default.svc.cluster.local/auth/verify"
    nginx.ingress.kubernetes.io/auth-response-headers: "X-User-Id, X-User-Role"
    nginx.ingress.kubernetes.io/proxy-buffer-size: "128k"
    nginx.ingress.kubernetes.io/proxy-buffers-number: "4"
    nginx.ingress.kubernetes.io/limit-rps: "200"
    nginx.ingress.kubernetes.io/whitelist-source-range: "10.0.0.0/8,192.168.0.0/16"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

## 迁移到 Kong Ingress Controller（KIC）

**步骤 1：安装 KIC**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Kong Ingress Controller（DB-less 模式）
helm repo add kong https://charts.konghq.com
helm repo update

helm install kong kong/ingress \
  --namespace kong \
  --create-namespace
```
**步骤 2：创建 KongPlugin 资源**

```yaml
# kong-plugins.yaml

# 替换 auth-url（外部认证）→ Kong forward-auth 插件
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: forward-auth
  namespace: production
plugin: forward-auth
config:
  uri: "http://auth-service.default.svc.cluster.local/auth/verify"
  response_headers:
  - X-User-Id
  - X-User-Role

---
# 替换 limit-rps → Kong rate-limiting 插件
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: rate-limiting
  namespace: production
plugin: rate-limiting
config:
  minute: 200           # 每分钟 200 次（对应 rps=200/60s*60s）
  policy: redis         # 使用 Redis 实现全局限流
  redis_host: redis.redis.svc.cluster.local
  redis_port: 6379

---
# 替换 whitelist-source-range → Kong ip-restriction 插件
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: ip-restriction
  namespace: production
plugin: ip-restriction
config:
  allow:
  - "10.0.0.0/8"
  - "192.168.0.0/16"

---
# 响应头缓冲（无直接等价插件，使用 proxy 插件参数）
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: proxy-config
  namespace: production
plugin: request-size-limiting
config:
  allowed_payload_size: 128
```

**步骤 3：将插件附着到 Ingress**

```yaml
# ingress-with-kong.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-ingress
  namespace: production
  annotations:
    # 改为 KIC
    kubernetes.io/ingress.class: "kong"
    # 附着插件（多个插件用逗号分隔）
    konghq.com/plugins: "forward-auth,rate-limiting,ip-restriction"
    # 路由优先级
    konghq.com/strip-path: "false"
spec:
  ingressClassName: kong
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

**步骤 4：声明式配置验证**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 deck（Kong 声明式管理工具）
brew install kong/tap/deck

# 导出当前 Kong 配置
deck dump --kong-addr http://localhost:8001 > kong-config.yaml

# 验证配置
deck validate -s kong-config.yaml

# Diff 对比（迁移前后）
deck diff --kong-addr http://localhost:8001 -s new-config.yaml

# 检查 KongPlugin 状态
kubectl get kongplugin -n production

# 功能测试
curl -H "Host: api.example.com" \
     -H "Authorization: Bearer eyJhbGci..." \
     http://<kong-lb-ip>/api/v1/test

# 验证 IP 限制
curl -H "Host: api.example.com" \
     -H "X-Forwarded-For: 1.2.3.4" \
     http://<kong-lb-ip>/api/v1/test
# 期望返回 403
```
---

<!-- chunk: 7. 零停机迁移清单 -->## 7. 零停机迁移清单

## 迁移前准备（Pre-Migration）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ─────────────────────────────────────────────────────────────────
# 阶段 0：盘点与基线建立
# ─────────────────────────────────────────────────────────────────

# ✅ 盘点所有 Ingress 资源
kubectl get ingress -A -o wide > ingress-inventory.txt
kubectl get ingress -A -o json | jq '[.items[] | {
  name: .metadata.name,
  namespace: .metadata.namespace,
  class: .metadata.annotations["kubernetes.io/ingress.class"],
  annotations: .metadata.annotations,
  hosts: [.spec.rules[].host],
  tls: .spec.tls
}]' > ingress-details.json

# ✅ 提取所有使用的注解
kubectl get ingress -A -o json | jq '[.items[].metadata.annotations | keys[]] | unique | sort'

# ✅ 建立流量基线（迁移前记录）
# 导出 Prometheus 指标快照
kubectl port-forward -n monitoring svc/prometheus 9090:9090 &
curl 'http://localhost:9090/api/v1/query?query=sum(rate(nginx_ingress_controller_requests[5m]))by(ingress,namespace)' \
  > baseline-traffic.json

# ✅ 记录现有证书状态
kubectl get cert -A -o wide

# ✅ 验证目标网关安装正常
kubectl get pods -n <target-gateway-namespace>
kubectl get svc -n <target-gateway-namespace>
```
## 并行验证阶段

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ─────────────────────────────────────────────────────────────────
# 阶段 1：并行部署验证（使用独立 LB 地址测试）
# ─────────────────────────────────────────────────────────────────

# 获取新网关的 LB IP
export NEW_GW_IP=$(kubectl get svc -n <namespace> <gateway-svc> \
  -o jsonpath='{.status.loadBalancer.ingress[0].ip}')

# 逐接口验证功能一致性
declare -a endpoints=(
  "/api/v1/health"
  "/api/v1/users"
  "/api/v1/orders"
)

for ep in "${endpoints[@]}"; do
  echo "=== Testing $ep ==="
  
  # nginx-ingress 结果
  NGINX_RESP=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Host: api.example.com" \
    http://$NGINX_IP$ep)
  
  # 新网关结果
  NEW_RESP=$(curl -s -o /dev/null -w "%{http_code}" \
    -H "Host: api.example.com" \
    http://$NEW_GW_IP$ep)
  
  if [ "$NGINX_RESP" = "$NEW_RESP" ]; then
    echo "✅ $ep: 状态码一致 ($NGINX_RESP)"
  else
    echo "❌ $ep: 不一致 nginx=$NGINX_RESP new=$NEW_RESP"
  fi
done
```
## 流量切换阶段

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ─────────────────────────────────────────────────────────────────
# 阶段 2：流量切换（按比例灰度）
# ─────────────────────────────────────────────────────────────────

# 方案：修改 LB 后端权重（以 AWS NLB Target Group 为例）
# nginx-ingress: 100% → 90% → 50% → 0%
# 新网关: 0% → 10% → 50% → 100%

# 监控切换过程中的关键指标
# 1. 错误率（目标 < 0.1%）
# 2. P99 延迟（目标不超过基线 20%）
# 3. 连接成功率

# 实时监控脚本
watch -n 5 'kubectl top pods -n <new-gateway-ns>; \
  curl -s "http://localhost:9090/api/v1/query?query=sum(rate(http_requests_total{status=~\"5..\"}[1m]))/sum(rate(http_requests_total[1m]))" | jq ".data.result"'
```
## 回滚预案

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ─────────────────────────────────────────────────────────────────
# 紧急回滚步骤（< 5 分钟完成）
# ─────────────────────────────────────────────────────────────────

# 方案 A：DNS 回滚（TTL < 60s 时有效）
# 将域名 DNS 记录重新指向 nginx-ingress LB IP

# 方案 B：LB 权重回滚
# 立即将新网关权重设为 0，nginx-ingress 设为 100

# 方案 C：ingressClass 回滚
kubectl patch ingress api-ingress -n production \
  --type='json' \
  -p='[{"op": "replace", "path": "/spec/ingressClassName", "value": "nginx"}]'

# 方案 D：注解回滚（如使用注解方式）
kubectl annotate ingress api-ingress -n production \
  kubernetes.io/ingress.class=nginx --overwrite
```
## 迁移完成验证

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# ─────────────────────────────────────────────────────────────────
# 阶段 3：完成验证与清理
# ─────────────────────────────────────────────────────────────────

# ✅ 确认所有流量已切换
kubectl get ingress -A -o json | \
  jq '[.items[] | select(.metadata.annotations["kubernetes.io/ingress.class"] == "nginx")]'
# 期望输出为空数组 []

# ✅ 验证 nginx-ingress 无请求（持续 30 分钟）
# 检查 nginx-ingress Pod 日志是否无新请求

# ✅ 验证证书有效性
for host in api.example.com app.example.com; do
  echo | openssl s_client -connect $host:443 2>/dev/null | \
    openssl x509 -noout -dates
done

# ✅ 清理 nginx-ingress（确认后执行）
helm uninstall ingress-nginx -n ingress-nginx  # ⚠️ 删除 release 及关联资源
kubectl delete ns ingress-nginx  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

<!-- chunk: 8. 常见问题与陷阱 -->## 8. 常见问题与陷阱

## 问题 1：路径匹配语义差异

```
症状：迁移后部分路径无法路由，返回 404
原因：nginx-ingress 和 Gateway API 的路径匹配语义不同

nginx-ingress 行为：
  path: /api  → 匹配 /api、/api/、/api/v1、/apitest（前缀是子字符串）
  
Gateway API 行为：
  PathPrefix: /api → 匹配 /api、/api/、/api/v1
                     不匹配 /apitest（要求路径分隔符边界）

解决方案：
```

```yaml
# ❌ 可能遗漏 /apitest 路径（如原 nginx 依赖该行为）
- matches:
  - path:
      type: PathPrefix
      value: /api

# ✅ 需要同时覆盖 /apitest 的场景
- matches:
  - path:
      type: PathPrefix
      value: /api
- matches:
  - path:
      type: PathPrefix
      value: /apitest
```

## 问题 2：rewrite-target 正则组引用

```bash
# nginx-ingress 写法
# path: /api(/|$)(.*) + rewrite-target: /$2

# Gateway API 等效写法（无法直接用 capture group）
# 需改为 ReplacePrefixMatch
```

```yaml
# ✅ 使用 ReplacePrefixMatch 替代正则 rewrite
filters:
- type: URLRewrite
  urlRewrite:
    path:
      type: ReplacePrefixMatch
      replacePrefixMatch: /     # 去掉 /api 前缀
```

## 问题 3：SSL 重定向行为差异

| 行为 | nginx-ingress | 新网关处理方式 |
|------|--------------|--------------|
| HTTP → HTTPS 自动跳转 | `force-ssl-redirect: "true"` | 入口点配置全局重定向 或 HTTPRoute 过滤器 |
| 跳转状态码 | 默认 308 | 可配置 301/302/307/308 |
| 跳转到其他域名 | `ssl-redirect` 注解 | HTTPRoute `RequestRedirect` 过滤器 |

```yaml
# Gateway API 中配置 HTTP → HTTPS 跳转
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: http-redirect
spec:
  parentRefs:
  - name: gateway
    sectionName: http
  hostnames:
  - api.example.com
  rules:
  - filters:
    - type: RequestRedirect
      requestRedirect:
        scheme: https
        statusCode: 301
```

## 问题 4：注解 configuration-snippet 无等价物

```
症状：迁移后无法使用 configuration-snippet 注入原始 Nginx 配置
原因：现代 API 网关不暴露底层代理配置

解决方案（按功能分类）：
```

| snippet 用途 | 替代方案 |
|-------------|---------|
| 添加响应头 | SecurityPolicy / Middleware headers |
| 修改缓冲区大小 | ClientTrafficPolicy / EnvoyPatchPolicy |
| 自定义日志格式 | 访问日志策略（telemetry） |
| Lua 脚本逻辑 | Wasm 插件 / serverless 插件 |
| 限速算法调整 | RateLimitPolicy 精细配置 |

## 问题 5：websocket 路由注解消失

```yaml
# nginx-ingress websocket 配置
annotations:
  nginx.ingress.kubernetes.io/proxy-read-timeout: "3600"
  nginx.ingress.kubernetes.io/proxy-send-timeout: "3600"
  nginx.ingress.kubernetes.io/websocket-services: "websocket-service"

# Gateway API 等效（现代网关默认支持 WebSocket 升级）
# 只需正常配置 HTTPRoute，确保超时设置合理
```

```yaml
# 设置长连接超时
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: websocket-timeout
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: websocket-route
  timeout:
    http:
      connectionIdleTimeout: "3600s"
```

## 问题 6：IngressClass 迁移期间双路由冲突

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 现象：同一 Host 在两个 IngressController 中各有一条路由
# 导致流量不确定性路由到其中一个

# 排查
kubectl get ingress -A | grep "api.example.com"

# 解决：迁移期间确保同一 Host 只在一个 Controller 中配置
# 方案：按服务逐步迁移（不同服务不同 Host），而非单个 Host 灰度

# 如必须单 Host 灰度：
# 使用 DNS 加权（不同 IP，不同 IngressClass）
# 不要在同一 K8s 集群内靠 IngressClass 区分同一 Host
```
## 迁移问题快速参考

| 症状 | 可能原因 | 排查命令 |
|------|---------|---------|
| 404 Not Found | 路径匹配语义不同 | 检查 HTTPRoute matches 配置 |
| 502 Bad Gateway | 后端服务端口/TLS 配置错误 | 检查 BackendRef 和 BackendTLSPolicy |
| 403 Forbidden | 认证插件配置不当 | 检查 SecurityPolicy / KongPlugin |
| 429 Too Many Requests（意外） | 限流配置单位不同（rps vs rpm） | 核对限流窗口和阈值 |
| 证书错误 | Secret 名称或命名空间不对 | `kubectl describe gateway` 查看 TLS 状态 |
| WebSocket 断连 | 超时配置过短 | 增大 connectionIdleTimeout |
| CORS 错误 | 原 CORS 注解未完整迁移 | 检查 SecurityPolicy CORS 配置 |

---

<!-- chunk: 参考资料 -->## 参考资料

- [ingress-nginx 官方文档](https://kubernetes.github.io/ingress-nginx/)
- [Kubernetes Gateway API 迁移指南](https://gateway-api.sigs.k8s.io/guides/migrating-from-ingress/)
- [Higress 注解兼容说明](https://higress.io/docs/user/annotation-use-case)
- [APISIX Ingress Controller 文档](https://apisix.apache.org/docs/ingress-controller/next/getting-started/)
- [Kong Ingress Controller 迁移](https://docs.konghq.com/kubernetes-ingress-controller/latest/guides/migrate-from-nginx-ingress/)
- 本域相关文档：
  - [01 - API 网关架构总览](./01-api-gateway-architecture-overview.md)
  - [02 - Kubernetes Gateway API 深度解析](./02-kubernetes-gateway-api-deep-dive.md)
  - [03 - API 网关选型指南](./03-api-gateway-selection-guide.md)
  - [04 - Higress 企业级实践](./04-higress-enterprise-gateway.md)
  - [05 - APISIX 企业级实践](./05-apisix-enterprise-gateway.md)
  - [06 - Kong 企业级实践](./06-kong-enterprise-gateway.md)
  - [07 - Envoy Gateway 企业级实践](./07-envoy-gateway-enterprise.md)
  - [08 - Traefik 企业级实践](./08-traefik-enterprise-gateway.md)
  - [domain-5: Nginx Ingress 完整指南](../网络/21-nginx-ingress-complete-guide.html)
  - [domain-5: Ingress 高级特性](../网络/23-ingress-advanced-routing.html)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[05-网络/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 10 - Wasm 插件生态与开发实践

## See Also

- 07-envoy-gateway-enterprise
- 08-traefik-enterprise-gateway
- 10-wasm-plugin-ecosystem
- 11-api-gateway-security-practices

## Related

- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
