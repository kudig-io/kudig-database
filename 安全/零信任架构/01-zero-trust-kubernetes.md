---
title: Zero Trust Kubernetes
description: Kubernetes 零信任架构完整指南 — mTLS、微分段、SPIFFE 身份、策略引擎、生产实践
summary: K8s 零信任架构生产指南，涵盖 Istio mTLS 配置、NetworkPolicy 微分段、SPIFFE/SPIRE 身份、OPA/Gatekeeper 策略、BeyondCorp 远程访问
tags:
- zero-trust
- kubernetes
- mtls
- networkpolicy
- spiffe
- opa
difficulty: advanced
domain: 安全
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# Kubernetes 零信任架构完整指南

## 1. 零信任架构概述

### 1.1 为什么 K8s 需要零信任

传统边界安全模型在 K8s 环境中失效：
- **动态工作负载**：Pod IP 动态变化，无法基于 IP 做访问控制
- **东西向流量**：微服务间大量内部通信，传统防火墙无法覆盖
- **多租户**：共享集群中不同团队的工作负载需要隔离
- **供应链风险**：镜像、Helm Chart、Operator 都可能被篡改

### 1.2 零信任架构框架

```
┌─────────────────────────────────────────────────────────────┐
│                     零信任控制平面                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ 身份认证  │  │ 授权策略  │  │ 策略引擎  │  │ 审计日志  │   │
│  │ (SPIRE)  │  │ (RBAC)   │  │ (OPA)    │  │ (Audit)  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                     零信任数据平面                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ mTLS     │  │ 微分段    │  │ 加密存储  │  │ 运行时防护│   │
│  │ (Istio)  │  │ (NetPol) │  │ (Sealed) │  │ (Falco)  │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## 2. 服务网格 mTLS

### 2.1 Istio mTLS 配置

**PeerAuthentication（全局 mTLS）**：
```yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT  # 强制 mTLS
```

**按命名空间配置**：
```yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: production
spec:
  mtls:
    mode: STRICT
  portLevelMtls:
    8080:
      mode: PERMISSIVE  # 特定端口允许明文（过渡期）
```

### 2.2 AuthorizationPolicy（L7 授权）

```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: backend
spec:
  selector:
    matchLabels:
      app: backend-api
  action: ALLOW
  rules:
    - from:
        - source:
            principals:
              - "cluster.local/ns/frontend/sa/frontend-service"
      to:
        - operation:
            methods: ["GET", "POST"]
            paths: ["/api/v1/*"]
```

**拒绝所有默认策略**：
```yaml
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  {}  # 空规则 = 拒绝所有
```

### 2.3 Linkerd mTLS

```yaml
# 自动 mTLS（默认启用）
apiVersion: policy.linkerd.io/v1beta1
kind: Server
metadata:
  name: backend-api
  namespace: backend
spec:
  podSelector:
    matchLabels:
      app: backend-api
  port: http
  proxyProtocol: HTTP/2
---
apiVersion: policy.linkerd.io/v1alpha1
kind: AuthorizationPolicy
metadata:
  name: frontend-to-backend
  namespace: backend
spec:
  targetRef:
    group: policy.linkerd.io
    kind: Server
    name: backend-api
  requiredAuthenticationRefs:
    - group: policy.linkerd.io
      kind: ServiceAccount
      name: frontend-service
      namespace: frontend
```

## 3. NetworkPolicy 微分段

### 3.1 默认拒绝所有

```yaml
# 拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
# 拒绝所有出站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Egress
```

### 3.2 应用级微分段

```yaml
# 允许 frontend → backend
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: backend
spec:
  podSelector:
    matchLabels:
      app: backend-api
  policyTypes:
    - Ingress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: frontend
          podSelector:
            matchLabels:
              app: frontend-web
      ports:
        - protocol: TCP
          port: 8080
```

### 3.3 DNS 出站策略

```yaml
# 允许 DNS 查询
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
```

## 4. SPIFFE/SPIRE 身份

### 4.1 SPIFFE ID 格式

```
spiffe://trust-domain/path
spiffe://cluster.local/ns/production/sa/backend-service
```

### 4.2 SPIRE 部署

```yaml
# SPIRE Server StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: spire-server
  namespace: spire
spec:
  replicas: 1
  selector:
    matchLabels:
      app: spire-server
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
```

### 4.3 工作负载注册

```yaml
# 注册 Entry
apiVersion: spire.spiffe.io/v1alpha1
kind: ClusterSPIFFEID
metadata:
  name: backend-service
spec:
  spiffeIDTemplate: "spiffe://cluster.local/ns/{{ .PodMeta.Namespace }}/sa/{{ .PodSpec.ServiceAccountName }}"
  podSelector:
    matchLabels:
      app: backend-api
  dnsNameTemplates:
    - "{{ .PodMeta.Name }}.backend.svc.cluster.local"
```

## 5. OPA/Gatekeeper 策略引擎

### 5.1 强制 mTLS

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequireMTLS
metadata:
  name: require-mtls
spec:
  match:
    kinds:
      - apiGroups: ["security.istio.io"]
        kinds: ["PeerAuthentication"]
  parameters:
    mode: STRICT
```

### 5.2 禁止特权容器

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: deny-privileged
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces:
      - kube-system
      - istio-system
```

### 5.3 镜像来源限制

```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedRepos
metadata:
  name: restrict-repos
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
  parameters:
    repos:
      - "gcr.io/my-project/"
      - "registry.internal.company.com/"
```

## 6. 生产最佳实践

### 6.1 零信任成熟度模型

| 阶段 | 特征 | 关键动作 |
|------|------|----------|
| L1: 初始 | 无网络策略，明文通信 | 部署 NetworkPolicy 默认拒绝 |
| L2: 基础 | 基本微分段，部分 mTLS | 启用 Istio PERMISSIVE 模式 |
| L3: 进阶 | 全面 mTLS，策略引擎 | STRICT mTLS + OPA 策略 |
| L4: 优化 | 自动化策略，持续验证 | SPIFFE 身份 + 自动轮换 |
| L5: 领先 | AI 驱动威胁检测 | 行为分析 + 自适应策略 |

### 6.2 迁移策略

```
阶段 1: PERMISSIVE 模式（观察期）
  └── 收集遥测数据，识别明文流量

阶段 2: 按命名空间启用 STRICT
  └── 从低风险命名空间开始

阶段 3: 全面 STRICT
  └── 所有命名空间强制 mTLS

阶段 4: 持续优化
  └── 自动化策略更新 + 威胁检测
```

### 6.3 监控指标

```yaml
# Istio mTLS 指标
- istio_requests_total{connection_security_policy="mutual_tls"}
- istio_tcp_connections_closed_total{connection_security_policy="mutual_tls"}

# NetworkPolicy 指标（Cilium）
- cilium_policy_l3_denied_total
- cilium_policy_l7_denied_total

# SPIRE 指标
- spire_server_rpc_count
- spire_agent_svid_count
```

## 7. 故障排查

### 7.1 mTLS 握手失败

```bash
# 检查 PeerAuthentication
kubectl get peerauthentication -A

# 检查 DestinationRule
kubectl get destinationrule -A

# 查看 Istio 日志
kubectl logs -n istio-system -l app=istiod

# 测试连接
istioctl x authz check <pod-name>
```

### 7.2 NetworkPolicy 调试

```bash
# Cilium 策略追踪
cilium policy trace --src-namespace frontend --src-pod frontend-xxx \
  --dst-namespace backend --dst-pod backend-xxx --dport 8080

# Calico 策略日志
kubectl logs -n kube-system -l k8s-app=calico-node
```

## Related

- [[安全/零信任架构/index.md|零信任架构索引]]
- [[安全/身份与访问/index.md|身份与访问]]
- [[网络/服务网格/index.md|服务网格]]
- [[安全/网络安全/index.md|网络安全]]
