---
title: Kubernetes 网络安全最佳实践
description: '# Kubernetes 网络安全最佳实践'
summary: '本指南提供生产环境 Kubernetes 网络安全配置的最佳实践，涵盖从网络策略到服务网格的全方位内容 ^[inferred]。'
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
tier: core
created: '2026-05-23'
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

每个生产命名空间必须配置默认拒绝 [[ingress|Ingress]] 和 Egress 的网络策略 ^[inferred]：

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

配置默认拒绝策略时，必须允许 Pod 向 kube-system 命名空间的 DNS 查询（UDP/TCP 53），否则 [[service|Service]] 发现会失败 ^[inferred]。

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

使用 Istio AuthorizationPolicy 基于 [[spiffe|SPIFFE]] 身份（principals）进行细粒度访问控制 ^[inferred]。

## 实施步骤

1. **启用网络策略支持**：确认 CNI 插件（Calico/Cilium）支持 [[networkpolicy|NetworkPolicy]]
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

- [[22-概念/10-最佳实践/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|NetworkPolicy]]
- [[istio|Istio]]
- [[26-技能/05-网络/cni/最佳实践/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]]
- [[22-概念/03-网络/service-mesh-architecture.md|Service Mesh Architecture]]

## 生产案例

### 案例 1: 未加密的 Service 流量被截获

| 时间 | 事件 |
|------|------|
| - | 安全审计发现内部服务间 HTTP 明文传输敏感数据 |
| - | 攻击者可通过 ARP 欺骗或节点入侵截获流量 |
| - | 🟡 启用 Istio mTLS 或应用层 TLS |

**根因**: 内部服务默认信任网络，未加密通信。

### 案例 2: NodePort 暴露导致未授权访问

**现象**: 内部服务通过 NodePort 被外部直接访问。

**诊断**: `kubectl get svc -A -o wide` 发现多个 NodePort Service

**修复**: 🟡 改用 ClusterIP + Ingress，或配置安全组限制 NodePort 访问

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 发现未授权访问 | 立即限制网络策略 |
| P1 | 安全配置缺失 | 补充 NetworkPolicy |
| P2 | 安全加固 | 启用 mTLS + 审计 |

## 面试要点

1. **Q: Kubernetes 网络安全的层次？**
   A: ① 网络隔离(NetworkPolicy) ② 传输加密(mTLS/TLS) ③ 身份认证(ServiceAccount/OAuth) ④ 授权(RBAC) ⑤ 审计(Audit Log)。零信任架构要求每层都验证。

2. **Q: 如何实现 Pod 间通信加密？**
   A: ① Istio/Linkerd mTLS(自动透明加密) ② 应用层 TLS(手动管理证书) ③ cert-manager 自动签发证书 ④ SPIFFE/SPIRE 工作负载身份。

3. **Q: 生产环境网络安全检查清单？**
   A: ① default-deny NetworkPolicy ② 禁用不必要的 NodePort/hostNetwork ③ 启用 mTLS ④ 限制 Pod 出口流量 ⑤ 定期安全扫描 ⑥ 网络流量审计日志。

## Related

- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[22-概念/03-网络/service-mesh-architecture.md|service-mesh-architecture]] — Service Mesh Architecture
- [[26-技能/05-网络/cni/最佳实践/k8s-network-configuration-guide.md|k8s-network-configuration-guide]] — Kubernetes 网络配置最佳实践


<!-- risk-assessed -->
