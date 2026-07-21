---
title: Network Security
description: 网络安全知识域 — NetworkPolicy 微分段、零信任网络架构、多集群安全、网络纵深防御
category: subdomain
tags:
- network-policy
- zero-trust
- microsegmentation
- mTLS
- defense-in-depth
tier: core
created: '2026-07-02'
last_updated: '2026-07-21'
---
# 网络安全 Network Security

> 构建 Kubernetes 网络纵深防御体系，实现微分段与零信任网络。

## 网络安全层次

| 层次 | 机制 | 工具 |
|------|------|------|
| L3/L4 网络策略 | NetworkPolicy | Calico/Cilium |
| L7 应用层 | Service Mesh mTLS | Istio/Linkerd |
| 入口控制 | Ingress/WAF | NGINX/Envoy |
| DNS 安全 | DNS 策略 | CoreDNS/RPZ |
| 东西向流量 | 微分段 | Cilium ClusterMesh |

## 文档索引

| 文档 | 主题 | 难度 |
|------|------|------|
| [[安全/网络安全/02-network-security-policies.md\|NetworkPolicy 实践]] | 网络策略编写与测试 | intermediate |
| [[安全/网络安全/07-zero-trust-security-architecture.md\|零信任安全架构]] | 零信任原则与落地 | advanced |
| [[安全/网络安全/18-network-defense-depth.md\|网络纵深防御]] | 多层防御体系设计 | advanced |
| [[安全/网络安全/19-zero-trust-architecture.md\|零信任架构实现]] | SPIFFE/mTLS/微分段 | advanced |
| [[安全/网络安全/21-multicluster-security.md\|多集群安全]] | 跨集群网络策略与信任 | advanced |

## NetworkPolicy 最佳实践

- 默认拒绝所有（Default Deny All）
- 按命名空间粒度策略管理
- 使用 `ipBlock` 限制外部访问
- 结合 DNS 策略控制域名解析
- 定期审计策略有效性（Cilium Hubble 可视化）

## Related

- [[安全/零信任架构/index.md|零信任架构]]
- [[安全/运行时安全/index.md|运行时安全]]
- [[网络/index.md|网络 Network]]
