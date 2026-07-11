---
title: NetworkPolicy Default Deny 模式
description: 零信任网络基线——默认拒绝所有流量，按需放行
summary: 使用 NetworkPolicy 实现默认拒绝所有入站/出站流量，逐步放行必要通信的零信任模式
category: manifests-patterns
tags:
- k8s
- manifests
- security
- networkpolicy
- zero-trust
- networking
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 8min
intent_queries:
- NetworkPolicy Default Deny
- 零信任网络配置
- Kubernetes 网络隔离基线
trigger_keywords:
- networkpolicy
- default-deny
- zero-trust
- network-isolation
prerequisites:
- k8s-networking-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# NetworkPolicy Default Deny 模式

## 1. 零信任网络原则

默认拒绝所有流量，仅显式放行已知通信路径：

```
传统模式: 允许所有 → 按需拒绝
零信任模式: 拒绝所有 → 按需允许 ✅
```

## 2. Namespace 级别 Default Deny

```yaml
# 拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production-apps
spec:
  podSelector: {}                  # 匹配命名空间下所有 Pod
  policyTypes:
    - Ingress
  ingress: []                      # 空列表 = 拒绝所有
---
# 拒绝所有出站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production-apps
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress: []
```

## 3. 放行必要流量

### 3.1 放行同 Namespace 内通信

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-same-namespace
  namespace: production-apps
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - podSelector: {}          # 同 Namespace 内所有 Pod
  egress:
    - to:
        - podSelector: {}
```

### 3.2 放行 Ingress Controller

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-ingress-controller
  namespace: production-apps
spec:
  podSelector: {}
  policyTypes:
    - Ingress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
          podSelector:
            matchLabels:
              app.kubernetes.io/name: ingress-nginx
```

### 3.3 放行 DNS（kube-system）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
  namespace: production-apps
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
```

### 3.4 放行外部数据库

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-external-db
  namespace: production-apps
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Egress
  egress:
    - to:
        - ipBlock:
            cidr: 10.0.5.0/24      # 数据库子网
      ports:
        - protocol: TCP
          port: 5432
```

## 4. 组合策略示例

```yaml
# 应用完整策略：frontend 只允许 backend，backend 允许 DB
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: production-apps
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 仅允许 Ingress Controller
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
      ports:
        - protocol: TCP
          port: 8080
  egress:
    # 仅允许访问 backend
    - to:
        - podSelector:
            matchLabels:
              app: backend
      ports:
        - protocol: TCP
          port: 8080
    # 允许 DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

## 5. Cilium NetworkPolicy（增强版）

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: backend-policy
  namespace: production-apps
spec:
  endpointSelector:
    matchLabels:
      app: backend
  egress:
    # 基于 FQDN 放行（DNS 自动解析）
    - toFQDNs:
        - matchPattern: "*.example.com"
        - matchName: "api.stripe.com"
      toPorts:
        - ports:
            - port: "443"
              protocol: TCP
    # 基于 Service 放行（K8s 原生）
    - toEndpoints:
        - matchLabels:
            "k8s:io.kubernetes.pod.namespace": kube-system
            "k8s:k8s-app": kube-dns
```

## 6. 迁移到 Default Deny

```bash
# 🟢 低风险：只读审计模式
# 1. 先用 audit 模式观察（不实际阻断）
kubectl apply -f networkpolicy-audit.yaml

# 2. 分析日志确认无误
kubectl logs -n kube-system calico-node | grep "Deny"

# 3. 逐步切换到 enforce 模式
# 4. 先对非关键应用启用，再推广
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 先审计后执行 | audit 模式观察后再 enforce |
| 为每个 Namespace 加 default deny | 基线安全策略 |
| 不要忘记 DNS 放行 | 否则域名解析失败 |
| 使用 namespaceSelector 标签 | 比名称更可靠 |
| 考虑 Cilium/Calico 增强策略 | FQDN、L7 级别策略 |

## Related

- [[清单模式/05-security-patterns/03-networkpolicy-tiered-isolation|分层网络隔离]]
- [[清单模式/YAML参考/22-networkpolicy-reference|NetworkPolicy 参考]]

## See Also

- [NetworkPolicy 文档](https://kubernetes.io/docs/concepts/services-networking/network-policies/)
- [Cilium NetworkPolicy](https://docs.cilium.io/en/stable/network/kubernetes/policy/)

<!-- risk-assessed -->
