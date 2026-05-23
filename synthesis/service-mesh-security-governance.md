---
title: 服务网格与安全治理的融合
description: '# 服务网格与安全治理的融合'
category: synthesis
tags:
- service-mesh
- security
- istio
- policy
- zero-trust
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 服务网格与安全治理的融合 是什么
- 如何 服务网格与安全治理的融合
trigger_keywords:
- 服务网格与安全治理的融合
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
created: "2026-05-23"
relationships:
  - target: "[[entities/opa]]"
    type: related_to
---

# 服务网格与安全治理的融合

## 网格安全能力

```
Istio 安全特性:
├── mTLS: 自动证书管理
├── AuthorizationPolicy: L7 细粒度授权
├── RequestAuthentication: JWT 验证
├── PeerAuthentication: mTLS 强制
└── Telemetry: 安全审计日志
```

## 与 [[entities/opa|OPA]] 集成

```yaml
# OPA + Istio 实现动态策略
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: opa-policy
spec:
  action: CUSTOM
  provider:
    name: opa
  rules:
  - to:
    - operation:
        paths: ["/api/*"]
```

## 合规价值

```
审计需求: "谁访问了什么数据？"
网格答案:
  - Source: service-a/ns-a
  - Destination: database/ns-b
  - Action: SELECT
  - Time: 2026-05-21T14:32:00Z
  - Result: ALLOW
```

## 相关 Domain

- domain-03-networking-traffic/03-service-mesh/01-istio-security-configuration
- domain-05-security-compliance/02-policy-engineering/01-opa-gatekeeper
## Related

- [[entities/istio|Istio (entities)]]
- [[domain-17-system-foundation/topic-dictionary/networking/service|Service]]
- [[domain-03-networking-traffic/02-service-mesh/01-istio-enterprise-service-mesh|Istio 企业级服务网格架构与实践]]
- [[entities/02-istio-advanced-traffic-management|Istio 高级流量管理 (entities)]]
