---
title: Zero Trust Architecture
description: 零信任架构知识域 — 零信任原则、服务网格 mTLS、微分段、身份驱动安全、BeyondCorp
summary: 零信任架构子目录索引，涵盖零信任核心原则、Istio/Linkerd mTLS、NetworkPolicy 微分段、SPIFFE 身份、BeyondCorp 远程访问
category: subdomain
tags:
- zero-trust
- mtls
- microsegmentation
- spiffe
- beyondcorp
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# 零信任架构 Zero Trust Architecture

> "永不信任，始终验证" — 构建身份驱动、微分段、持续验证的安全架构。

## 核心原则

| 原则 | 说明 | K8s 实现 |
|------|------|----------|
| 显式验证 | 基于所有可用数据点认证和授权 | mTLS + RBAC + NetworkPolicy |
| 最小权限 | 仅授予完成任务所需的权限 | RBAC + OPA/Gatekeeper |
| 假设已被攻破 | 假设攻击者已在网络内部 | 微分段 + 加密 + 审计 |

## 文件索引

| 文件 | 内容 | 难度 |
|------|------|------|
| [[安全/零信任架构/01-zero-trust-kubernetes.md\|01-zero-trust-kubernetes]] | K8s 零信任架构全景 | advanced |

## 技术栈

```
┌─────────────────────────────────────────────────────────┐
│                    零信任架构层次                        │
├─────────────────────────────────────────────────────────┤
│  L7: 应用层    │ API Gateway + OAuth2/OIDC + WAF        │
│  L4: 传输层    │ Service Mesh mTLS (Istio/Linkerd)      │
│  L3: 网络层    │ NetworkPolicy 微分段 + Calico/Cilium   │
│  L2: 身份层    │ SPIFFE/SPIRE + ServiceAccount          │
│  L1: 设备层    │ 节点认证 + 安全启动 + 可信执行环境      │
└─────────────────────────────────────────────────────────┘
```

## Related

- [[安全/身份与访问/index.md|身份与访问]]
- [[安全/网络安全/index.md|网络安全]]
- [[网络/服务网格/index.md|服务网格]]
- [[安全/运行时安全/index.md|运行时安全]]
