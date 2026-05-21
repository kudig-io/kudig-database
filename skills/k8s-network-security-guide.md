---
title: Kubernetes 网络安全最佳实践
description: '# Kubernetes 网络安全最佳实践'
category: skills
tags:
- k8s
- security
- network-policy
- mtls
- service-mesh
- istio
- cilium
- calico
- ingress
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 网络安全最佳实践 是什么
- 如何 Kubernetes 网络安全最佳实践
trigger_keywords:
- Kubernetes
- 网络安全最佳实践
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
- cni-basics
---

# Kubernetes 网络安全最佳实践

## 概述

本指南提供生产环境 Kubernetes 网络安全配置的最佳实践，涵盖从网络策略到服务网格的全方位内容 ^[inferred]。

## 网络策略设计原则

- **默认拒绝**：所有流量默认拒绝，仅允许明确授权的通信 ^[inferred]
- **最小权限**：仅允许必要的通信路径 ^[inferred]
- **分层防护**：入口层 -> 命名空间层 -> Pod 层 -> 出口层 ^[inferred]
- **可观测性**：网络流量监控和审计 ^[inferred]

## 关键配置

### 默认拒绝策略

每个生产命名空间必须配置默认拒绝 Ingress 和 Egress 的网络策略 ^[inferred]：

```yaml
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
```

### 命名空间隔离

限制跨命名空间通信，仅允许来自同一命名空间的 Pod 的入站流量 ^[inferred]。

### DNS 出口策略

配置默认拒绝策略时，必须允许 Pod 向 kube-system 命名空间的 DNS 查询（UDP/TCP 53），否则 Service 发现会失败 ^[inferred]。

### 应用层策略

按应用标签配置 Pod 间通信策略。例如允许 frontend 访问 backend 的 8080 端口，允许 backend 访问 database 的 5432 端口 ^[inferred]。

## 服务网格安全

### Istio mTLS

```yaml
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
spec:
  mtls:
    mode: STRICT
```

启用 STRICT 模式强制服务间 mTLS 加密 ^[inferred]。

### 授权策略

使用 Istio AuthorizationPolicy 基于 SPIFFE 身份（principals）进行细粒度访问控制 ^[inferred]。

## 实施步骤

1. **启用网络策略支持**：确认 CNI 插件（Calico/Cilium）支持 NetworkPolicy
2. **配置默认拒绝策略**：为生产命名空间配置 Ingress + Egress 默认拒绝
3. **配置应用网络策略**：按应用间依赖关系配置允许规则
4. **安装服务网格**：安装 Istio，启用自动注入和 mTLS

## 常见陷阱

### 策略冲突

多个网络策略同时生效可能导致预期外的流量被阻断。应定期检查所有网络策略并测试连通性 ^[inferred]。

### DNS 策略缺失

配置默认拒绝策略但未允许 DNS 查询会导致 Service 发现失败，应用无法找到后端服务 ^[inferred]。

### 服务网格注入失败

Pod 未自动注入 sidecar 会导致 mTLS 和授权策略不生效。应检查命名空间的 `istio-injection=enabled` 标签 ^[inferred]。

## 验证方法

- 检查网络策略：`kubectl get networkpolicy --all-namespaces`
- 检查默认拒绝策略是否生效
- 检查服务网格状态和 mTLS 配置
- 测试网络连通性 ^[inferred]

## 相关资源

- [[concepts/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[entities/networkpolicy.md|NetworkPolicy]]
- [[istio|Istio]]
- [[skills/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]

## Related

- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[skills/k8s-network-configuration-guide.md|k8s-network-configuration-guide]] — Kubernetes 网络配置最佳实践
