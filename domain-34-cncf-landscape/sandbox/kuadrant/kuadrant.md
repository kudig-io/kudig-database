# Kuadrant

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kuadrant.io/ |
| **GitHub** | https://github.com/Kuadrant/kuadrant-operator |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kuadrant 是一个 Kubernetes Gateway API 的策略引擎，为 Gateway API 添加 API 管理能力，包括认证、授权、限流和 DNS 管理。它通过 Policy Attachment 模式将策略附加到 Gateway API 资源（Gateway、HTTPRoute）上，无需修改路由配置即可添加安全和流量管理策略，实现了 Gateway API 原生的 API 管理。

### 核心特性

- **Gateway API 原生**: 通过 Policy Attachment 与标准 Gateway API 集成
- **认证策略 (AuthPolicy)**: JWT 验证、API Key、OAuth2、mTLS
- **限流策略 (RateLimitPolicy)**: 灵活的多维度限流规则
- **DNS 策略 (DNSPolicy)**: 自动化 DNS 记录管理和地理负载均衡
- **TLS 策略 (TLSPolicy)**: 自动化 TLS 证书管理（集成 cert-manager）
- **多网关**: 跨多个 Gateway 一致地应用策略

---

## 快速开始

### 安装

```bash
# 安装 Kuadrant Operator
kubectl apply -f https://github.com/Kuadrant/kuadrant-operator/releases/latest/download/kuadrant-operator.yaml

# 创建 Kuadrant 实例
kubectl apply -f - <<EOF
apiVersion: kuadrant.io/v1beta1
kind: Kuadrant
metadata:
  name: kuadrant
  namespace: kuadrant-system
spec: {}
EOF
```

### 认证策略

```yaml
apiVersion: kuadrant.io/v1
kind: AuthPolicy
metadata:
  name: api-auth
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-routes
  rules:
    authentication:
      jwt:
        jwt-auth:
          issuerUrl: https://auth.example.com
          audiences:
            - "my-api"
    authorization:
      opa-policy:
        rego: |
          allow {
            input.auth.identity.role == "admin"
          }
          allow {
            input.auth.identity.role == "user"
            input.request.method == "GET"
          }
    response:
      success:
        headers:
          x-user-id:
            plain:
              value: auth.identity.sub
```

### 限流策略

```yaml
apiVersion: kuadrant.io/v1
kind: RateLimitPolicy
metadata:
  name: api-rate-limit
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-routes
  limits:
    per-user:
      rates:
        - limit: 100
          window: 60s
      counters:
        - auth.identity.sub
      when:
        - predicate: request.headers.exists("authorization")
    global:
      rates:
        - limit: 1000
          window: 60s
```

### DNS 策略

```yaml
apiVersion: kuadrant.io/v1alpha1
kind: DNSPolicy
metadata:
  name: geo-dns
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: Gateway
    name: main-gateway
  loadBalancing:
    geo:
      defaultGeo: US
    weighted:
      defaultWeight: 100
  providerRefs:
    - name: aws-route53-credentials
```

---

## 与其他方案对比

| 特性 | Kuadrant | Kong (GW API) | Gloo Gateway | Ambassador |
|:---|:---|:---|:---|:---|
| Gateway API | Policy Attachment | 插件 | 扩展 CRD | 映射 |
| 认证 | AuthPolicy | 插件 | 扩展 | 过滤器 |
| 限流 | RateLimitPolicy | 插件 | 扩展 | 过滤器 |
| DNS 管理 | DNSPolicy | 不内置 | 不内置 | 不内置 |
| TLS 自动化 | TLSPolicy | cert-manager | cert-manager | cert-manager |
| 多网关一致性 | 原生支持 | 单网关 | 单网关 | 单网关 |

---

## 最佳实践

1. **策略层级**: Gateway 级设置默认策略，HTTPRoute 级覆盖特定路由的策略
2. **限流维度**: 组合用户、IP、路径等多维度实现精细化限流
3. **认证分层**: 公开 API 用 API Key，内部 API 用 JWT/mTLS
4. **DNS 地理路由**: 多区域部署时配合 DNSPolicy 实现就近访问
5. **TLS 自动化**: 配合 cert-manager 实现证书的自动签发和续期

---

## 参考资源

- [Kuadrant 官方文档](https://docs.kuadrant.io/)
- [Kuadrant GitHub](https://github.com/Kuadrant)
- [Gateway API Policy Attachment](https://gateway-api.sigs.k8s.io/geps/gep-713/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
