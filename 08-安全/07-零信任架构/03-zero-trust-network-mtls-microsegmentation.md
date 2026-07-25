---
title: Zero Trust Network Architecture — Service Mesh mTLS and Microsegmentation
description: K8s 零信任网络 — Service Mesh mTLS、微分段、身份驱动访问、SPIFFE/SPIRE、BeyondCorp 模式
summary: 在 Kubernetes 上实现零信任网络架构，通过 mTLS、微分段和身份驱动策略消除隐式信任
category: practice
tags:
- zero-trust
- mtls
- service-mesh
- microsegmentation
- spiffe
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: security
---
# 零信任网络架构 — mTLS 与微分段

> 消除隐式信任，构建身份驱动的 Kubernetes 网络安全体系。

## 零信任原则

```
传统模型:  边界防火墙 → 内部全信任（扁平网络）
零信任:    每次请求都验证身份 + 加密 + 授权（永不信任，始终验证）

┌─────────────────────────────────────────────────┐
│  零信任三支柱                                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐     │
│  │ 身份验证  │  │ 传输加密  │  │ 最小权限  │     │
│  │ (Who)    │  │ (mTLS)   │  │ (Policy) │     │
│  └──────────┘  └──────────┘  └──────────┘     │
│       │              │              │           │
│  SPIFFE/SPIRE   Istio/Linkerd   OPA/Kyverno   │
│  ServiceAccount  自动证书轮换    L7 授权策略    │
└─────────────────────────────────────────────────┘
```

## Service Mesh mTLS（Istio）

### 全局严格 mTLS

```yaml
# 全网格强制 mTLS
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT  # 拒绝明文流量
---
# 命名空间级覆盖（迁移期间允许 PERMISSIVE）
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-ns
  namespace: legacy
spec:
  mtls:
    mode: PERMISSIVE  # 接受 mTLS 和明文（过渡期）
```

### 授权策略（L7 微分段）

```yaml
# 仅允许 frontend 访问 api-server 的 /api/v1/* 路径
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: api-server-access
  namespace: production
spec:
  selector:
    matchLabels:
      app: api-server
  action: ALLOW
  rules:
    - from:
        - source:
            principals:
              - "cluster.local/ns/production/sa/frontend"
              - "cluster.local/ns/production/sa/mobile-bff"
      to:
        - operation:
            methods: ["GET", "POST", "PUT", "DELETE"]
            paths: ["/api/v1/*"]
    - from:
        - source:
            principals:
              - "cluster.local/ns/monitoring/sa/prometheus"
      to:
        - operation:
            methods: ["GET"]
            paths: ["/metrics", "/healthz"]
---
# 默认拒绝所有（命名空间级）
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  {}  # 空规则 = 拒绝所有
```

### 出口流量控制

```yaml
# 限制 Pod 只能访问特定外部服务
apiVersion: networking.istio.io/v1alpha3
kind: ServiceEntry
metadata:
  name: external-https
  namespace: production
spec:
  hosts:
    - "*.amazonaws.com"
    - "api.stripe.com"
  location: MESH_EXTERNAL
  ports:
    - number: 443
      name: https
      protocol: TLS
  resolution: DNS
---
# Sidecar 限制（减少配置推送范围）
apiVersion: networking.istio.io/v1alpha3
kind: Sidecar
metadata:
  name: default
  namespace: production
spec:
  outboundTrafficPolicy:
    mode: REGISTRY_ONLY  # 仅允许注册的服务
  egress:
    - hosts:
        - "./*"
        - "istio-system/*"
        - "production/external-https.mesh-external"
```

## SPIFFE/SPIRE 身份框架

### SPIRE 部署

```yaml
# SPIRE Server（StatefulSet）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: spire-server
  namespace: spire
spec:
  replicas: 1
  serviceName: spire-server
  template:
    spec:
      containers:
        - name: spire-server
          image: ghcr.io/spiffe/spire-server:1.9.0
          args:
            - -config
            - /run/spire/config/server.conf
          volumeMounts:
            - name: config
              mountPath: /run/spire/config
            - name: data
              mountPath: /run/spire/data
---
# SPIRE Agent（DaemonSet）
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: spire-agent
  namespace: spire
spec:
  template:
    spec:
      hostPID: true
      hostNetwork: true
      containers:
        - name: spire-agent
          image: ghcr.io/spiffe/spire-agent:1.9.0
          args:
            - -config
            - /run/spire/config/agent.conf
          volumeMounts:
            - name: spire-workload-api
              mountPath: /run/spire/sockets
      volumes:
        - name: spire-workload-api
          hostPath:
            path: /run/spire/sockets
            type: DirectoryOrCreate
```

### 工作负载身份注册

```bash
# 注册工作负载（基于 ServiceAccount）
spire-server entry create \
  -spiffeID spiffe://example.org/ns/production/sa/api-server \
  -parentID spiffe://example.org/agent/k8s \
  -selector k8s:ns:production \
  -selector k8s:sa:api-server \
  -ttl 3600

# 验证工作负载身份
spire-agent api fetch -socketPath /run/spire/sockets/agent.sock
```

## 网络微分段（NetworkPolicy + Mesh）

### 分层防御

```yaml
# L3/L4: NetworkPolicy（CNI 层）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-server-netpol
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes: ["Ingress", "Egress"]
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
        - podSelector:
            matchLabels:
              app: mobile-bff
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - podSelector:
            matchLabels:
              app: postgres
      ports:
        - protocol: TCP
          port: 5432
    - to:
        - podSelector:
            matchLabels:
              app: redis
      ports:
        - protocol: TCP
          port: 6379
    # 允许 DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
---
# L7: Istio AuthorizationPolicy（应用层）
# 即使 L3/L4 允许，L7 仍可限制路径/方法
```

### 微分段策略矩阵

| 源 → 目标 | frontend | api-server | payment | database |
|-----------|----------|------------|---------|----------|
| frontend | - | ✅ /api/* | ❌ | ❌ |
| api-server | ❌ | - | ✅ /charge | ✅ SELECT/INSERT |
| payment | ❌ | ✅ /callback | - | ✅ SELECT/UPDATE |
| monitoring | ✅ /metrics | ✅ /metrics | ✅ /metrics | ❌ |

## 证书管理自动化

### cert-manager + Istio

```yaml
# 自签 CA（生产建议用 Vault/ACME）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: internal-ca
spec:
  ca:
    secretName: internal-ca-key-pair
---
# Istio 网关证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: gateway-tls
  namespace: istio-system
spec:
  secretName: gateway-tls-cert
  issuerRef:
    name: internal-ca
    kind: ClusterIssuer
  dnsNames:
    - "*.example.com"
    - "*.internal.example.com"
  duration: 2160h  # 90 天
  renewBefore: 720h  # 提前 30 天续期
  privateKey:
    algorithm: ECDSA
    size: 256
```

## 零信任成熟度模型

| 阶段 | 特征 | 关键动作 |
|------|------|----------|
| L1: 边界安全 | 仅依赖 NetworkPolicy | 默认拒绝 + 白名单 |
| L2: 传输加密 | 全网格 mTLS | Istio STRICT mode |
| L3: 身份驱动 | 基于 SA 的 L7 授权 | AuthorizationPolicy |
| L4: 持续验证 | 动态策略 + 异常检测 | OPA + 行为分析 |
| L5: 自适应 | 风险评分 + 自动响应 | AI 驱动策略调整 |

## 实施路线图

```
Phase 1 (1-2月): 部署 Istio + PERMISSIVE mTLS + 可观测性
Phase 2 (2-3月): 逐命名空间切换 STRICT mTLS
Phase 3 (3-4月): 部署 AuthorizationPolicy（先 AUDIT 后 ENFORCE）
Phase 4 (4-6月): 引入 SPIFFE/SPIRE + 出口控制
Phase 5 (持续):  异常检测 + 自适应策略 + GameDay 验证
```

## 故障排查

```bash
# 检查 mTLS 状态
istioctl authn tls-check
istioctl x authz check <pod-name> -n production

# 查看 Sidecar 证书
istioctl proxy-config secret <pod-name> -n production

# 测试连通性
kubectl exec frontend-xxx -n production -- curl -v http://api-server:8080/health
# 如果 mTLS 问题：
kubectl exec frontend-xxx -n production -- curl -v https://api-server:8080/health --cacert /var/run/secrets/istio/root-cert.pem

# 查看被拒绝的请求
kubectl logs -n istio-system -l app=istiod --tail=50 | grep "RBAC: access denied"
```

## Related

- [[08-安全/07-零信任架构/index.md|零信任架构]]
- [[08-安全/07-零信任架构/01-zero-trust-kubernetes.md|零信任基础]]
- [[05-网络/03-服务网格/index.md|服务网格]]
