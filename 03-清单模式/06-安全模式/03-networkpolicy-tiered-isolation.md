---
title: NetworkPolicy 分层隔离模式
description: 基于网络层级的纵深隔离——DMZ/应用层/数据层/系统层
summary: 使用 NetworkPolicy 实现 DMZ、应用层、数据层、系统层之间的纵深网络隔离
category: manifests-patterns
tags:
- k8s
- manifests
- security
- networkpolicy
- tiered-isolation
- zero-trust
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- NetworkPolicy 分层隔离
- Kubernetes 网络分层
- 纵深防御网络策略
trigger_keywords:
- networkpolicy
- tiered
- dmz
- isolation
- defense-in-depth
prerequisites:
- networkpolicy-basics
- security-basics
authors:
- name: KUDIG Team
  role: contributor
---

# NetworkPolicy 分层隔离模式

## 1. 网络分层架构

```
┌─────────────────────────────────────────────┐
│              DMZ 层 (ingress-nginx)          │
│         仅暴露 80/443 → 互联网               │
├─────────────────────────────────────────────┤
│           应用层 (frontend/backend)           │
│      frontend ← DMZ, backend ← frontend      │
├─────────────────────────────────────────────┤
│            数据层 (database/cache)            │
│        database ← backend, cache ← backend   │
├─────────────────────────────────────────────┤
│         系统层 (kube-system/monitoring)       │
│          仅允许控制平面访问                   │
└─────────────────────────────────────────────┘
```

## 2. DMZ 层（Ingress Controller）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: dmz-policy
  namespace: ingress-nginx
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 允许互联网访问 80/443
    - from:
        - ipBlock:
            cidr: 0.0.0.0/0
      ports:
        - protocol: TCP
          port: 80
        - protocol: TCP
          port: 443
  egress:
    # 仅允许访问应用层命名空间
    - to:
        - namespaceSelector:
            matchLabels:
              tier: application
      ports:
        - protocol: TCP
          port: 8080
        - protocol: TCP
          port: 8443
    # DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

## 3. 应用层策略

### 3.1 Frontend（仅接收 DMZ 流量）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: frontend-policy
  namespace: app-frontend
spec:
  podSelector:
    matchLabels:
      app: frontend
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
  egress:
    # 仅访问 backend
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: app-backend
      ports:
        - protocol: TCP
          port: 8080
    # DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

### 3.2 Backend（接收 Frontend，访问数据层）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: app-backend
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: app-frontend
  egress:
    # 访问数据层
    - to:
        - namespaceSelector:
            matchLabels:
              tier: data
      ports:
        - protocol: TCP
          port: 5432    # PostgreSQL
        - protocol: TCP
          port: 6379    # Redis
    # 访问外部 API（白名单）
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
            except:
              - 10.0.0.0/8        # 禁止访问内网
              - 172.16.0.0/12
              - 192.168.0.0/16
      ports:
        - protocol: TCP
          port: 443
    # DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

## 4. 数据层策略（最严格）

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: database-policy
  namespace: data-tier
spec:
  podSelector:
    matchLabels:
      app: postgresql
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 仅允许应用层 backend 访问
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: app-backend
      ports:
        - protocol: TCP
          port: 5432
  egress:
    # 数据库不需要主动发起出站
    # 仅允许 DNS
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

## 5. Namespace 标签规划

```yaml
# DMZ 层
apiVersion: v1
kind: Namespace
metadata:
  name: ingress-nginx
  labels:
    tier: dmz
---
# 应用层
apiVersion: v1
kind: Namespace
metadata:
  name: app-frontend
  labels:
    tier: application
---
apiVersion: v1
kind: Namespace
metadata:
  name: app-backend
  labels:
    tier: application
---
# 数据层
apiVersion: v1
kind: Namespace
metadata:
  name: data-tier
  labels:
    tier: data
---
# 系统层
apiVersion: v1
kind: Namespace
metadata:
  name: kube-system
  labels:
    tier: system
```

## 6. 监控层跨层访问

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: monitoring-egress
  namespace: monitoring
spec:
  podSelector:
    matchLabels:
      app: prometheus
  policyTypes:
    - Egress
  egress:
    # 允许 scrape 所有 Namespace 的 metrics
    - to:
        - namespaceSelector: {}   # 所有命名空间
      ports:
        - protocol: TCP
          port: 9090
        - protocol: TCP
          port: 9100
```

## 7. 验证连通性

```bash
# 🟢 低风险：网络测试
# 使用 network-multitool 测试连通性
kubectl run netshoot --rm -it --image=nicolaka/netshoot -n app-frontend -- bash

# 在 frontend Pod 中测试能否访问 database（应被拒绝）
nc -zv postgresql.data-tier 5432  # 预期：超时/拒绝

# 测试到 backend 的连通性（应成功）
nc -zv backend.app-backend 8080   # 预期：成功
```

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 Namespace 标签分层 | `tier: dmz/application/data/system` |
| 数据层不允许出站 | 防止数据泄露 |
| 监控层单独策略 | 允许跨层 scrape |
| 使用 Cilium L7 策略 | 基于 HTTP path/method 更细粒度 |
| 定期审计策略 | 检查是否有过度放行的规则 |

## Related

- [[03-清单模式/06-安全模式/02-networkpolicy-default-deny|Default Deny 模式]]
- [[03-清单模式/06-安全模式/07-opa-kyverno-policy-examples|OPA/Kyverno 策略]]

## See Also

- [NetworkPolicy 设计模式](https://kubernetes.io/docs/concepts/services-networking/network-policies/)
- [Calico 网络分层最佳实践](https://docs.tigera.io/calibetatest/security/network-isolation)

<!-- risk-assessed -->
