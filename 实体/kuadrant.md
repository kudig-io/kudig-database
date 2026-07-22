---
title: Kuadrant (entities)
description: '## 概述'
summary: 'Kuadrant 是一个 Kubernetes Gateway API 的策略引擎，为 Gateway API 添加 API 管理能力，包括认证、授权、限流和 DNS 管理。'
category: entities
tags:
- k8s
- cncf
- networking
- kuadrant
- gateway
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuadrant 是什么
- 如何 Kuadrant
trigger_keywords:
- Kuadrant
prerequisites:
- kubectl-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kuadrant

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, Rust

## 概述

Kuadrant 是一个 Kubernetes Gateway API 的策略引擎，由 Red Hat 推动开发，2023 年加入 CNCF 沙箱。它为 Gateway API 添加 API 管理能力，包括认证（AuthN）、授权（AuthZ）、限流（Rate Limiting）和 DNS 管理。Kuadrant 通过 Policy Attachment 模式将策略附加到 Gateway API 资源（Gateway、HTTPRoute）上，无需修改路由配置即可添加安全和流量管理策略，实现了 Gateway API 原生的 API 管理体验。Kuadrant 的组件包括 Authorino（认证授权）、Limitador（限流引擎）和 Kuadrant Operator（策略编排），底层基于 Envoy 网关实现数据面策略执行。

## 核心能力

- **认证（AuthN）: 支持 API Key、JWT、OAuth2、mTLS、OpenFGA 等多种认证方式
- **授权（AuthZ）: 基于 OpenFGA 的细粒度关系型授权
- **限流（Rate Limiting）: 多维度（IP、用户、Header、路径）灵活限流
- **DNS 管理**: DNSPolicy 实现跨区域 DNS 地理路由
- **TLS 自动化**: TLSPolicy 配合 cert-manager 实现证书自动化
- **Gateway API 原生**: 基于 Policy Attachment 模式，无需修改路由配置

## 架构

Kuadrant 采用 Policy Attachment + 多组件协作架构：

- **Kuadrant Operator**: 集群级控制器，编排所有 Kuadrant 组件
- **Authorino**: 认证授权引擎，集成多种 AuthN/AuthZ 后端
- **Limitador**: 基于 Redis 的高性能限流引擎（Rust 实现）
- **AuthPolicy CRD**: 声明式认证和授权策略，附加到 Gateway/HTTPRoute
- **RateLimitPolicy CRD**: 声明式限流策略，定义限流维度和阈值
- **DNSPolicy CRD**: DNS 地理路由和健康检查策略

策略执行流程：`请求 → Gateway (Envoy) → Authorino (Auth) → Limitador (Rate Limit) → 后端服务`

## K8s 集成

Kuadrant 通过 Gateway API 的 Policy Attachment 模式与 Kubernetes 集成。AuthPolicy 和 RateLimitPolicy 通过 targetRef 附加到 Gateway 或 HTTPRoute 资源。Kuadrant Operator 监听这些策略 CRD，将配置翻译为 Envoy 的 ext-auth 和 rate-limit filter 配置。Envoy Gateway / Istio 等数据面在请求处理时调用 Authorino（通过 gRPC ext-auth）和 Limitador（通过 gRPC rate-limit）执行策略。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Gateway API 完全兼容，是 Gateway API 生态的核心策略组件。

## 生产场景

1. **API 网关安全**: 为所有 API 添加 JWT 认证和细粒度授权
2. **多租户限流**: 为不同租户/用户/API 设置差异化的速率限制
3. **多区域流量调度**: 通过 DNSPolicy 实现多区域部署的就近访问和故障转移
4. **零信任 API**: 对内部微服务间的 API 调用进行 mTLS 认证和授权

## 安装与配置

### Operator 部署

```bash
# 安装 Kuadrant Operator
kubectl apply -f https://github.com/Kuadrant/kuadrant-operator/releases/latest/download/install.yaml

# 等待 Operator 就绪
kubectl wait --for=condition=Available deployment/kuadrant-operator-controller-manager -n kuadrant-system

# 验证部署
kubectl get pods -n kuadrant-system
kubectl get crd | grep kuadrant
```

### Kuadrant 实例与策略配置

```yaml
# 创建 Kuadrant 实例
apiVersion: kuadrant.io/v1beta1
kind: Kuadrant
metadata:
  name: kuadrant
  namespace: kuadrant-system
spec:
  gatewaySelector: {}
---
# AuthPolicy - JWT 认证
apiVersion: kuadrant.io/v1beta2
kind: AuthPolicy
metadata:
  name: my-auth
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: my-route
  rules:
    authentication:
      jwt:
        issuer: "https://my-idp.com"
        audiences: ["my-api"]
---
# RateLimitPolicy - 限流
apiVersion: kuadrant.io/v1beta2
kind: RateLimitPolicy
metadata:
  name: my-ratelimit
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: my-route
  limits:
    - name: per-user
      rates:
        - limit: 100
          duration: 60
          unit: seconds
```

## 运维操作

```bash
# 🟢 查看策略状态
kubectl get authpolicy -A
kubectl get ratelimitpolicy -A
kubectl describe authpolicy my-auth

# 🟡 应用新策略
kubectl apply -f auth-policy.yaml

# 🟡 更新限流配置
kubectl patch ratelimitpolicy my-ratelimit --type merge -p '{"spec":{"limits":[{"name":"per-user","rates":[{"limit":200,"duration":60,"unit":"seconds"}]}]}}'

# 🔴 删除策略
kubectl delete authpolicy my-auth
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略未生效 | targetRef 不匹配 | `kubectl describe authpolicy <name>` | 检查 targetRef 配置 |
| 认证失败 | JWT issuer 错误 | 检查 AuthPolicy rules | 确认 issuer 和 audiences |
| 限流未触发 | 配置未同步 | `kubectl get ratelimitpolicy -o yaml` | 检查 limits 配置 |
| Operator CrashLoop | CRD 版本不匹配 | `kubectl logs -n kuadrant-system` | 重新应用 CRD |

**排查流程：**
```
策略未生效
├── 检查 Operator 状态 → kubectl get pods -n kuadrant-system
├── 检查策略状态 → kubectl describe authpolicy <name>
├── 检查 targetRef → 确认 HTTPRoute 存在
├── 检查 Gateway → kubectl get gateway
└── 查看日志 → kubectl logs -n kuadrant-system -l app=kuadrant
```

## 生产案例

### 案例一：API 网关统一认证

- **场景**: 多个微服务通过 Gateway API 暴露，需要统一 JWT 认证
- **排查**: 使用 Kuadrant AuthPolicy 在网关层统一认证，无需每个服务单独实现
- **方案**: 为每个 HTTPRoute 配置 AuthPolicy，JWT 验证在网关层完成
- **效果**: 服务无需关心认证逻辑，安全策略集中管理

### 案例二：API 限流保护

- **场景**: 公开 API 需要限流保护，防止滥用
- **排查**: 使用 Kuadrant RateLimitPolicy 在网关层限流
- **方案**: 按用户/API Key 限流，每分钟 100 次请求
- **效果**: 后端服务免受流量冲击，无需修改应用代码

## 对比

| 特性 | Kuadrant | Kong | Tyk | APISIX | 适用场景 |
|------|----------|------|-----|--------|----------|
| Gateway API | ✅ 原生 | ⚠️ | ⚠️ | ⚠️ | Kuadrant 首选 |
| Policy Attachment | ✅ | ❌ | ❌ | ❌ | - |
| OpenFGA AuthZ | ✅ | ❌ | ❌ | ❌ | 细粒度授权 |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF | - |

## 架构定位

在 CNCF 生态中，Kuadrant 属于 **Networking** 类别，为云原生应用提供 Gateway API 策略管理能力。

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[open-cluster-management]] — [[实体/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[cloud-custodian]] — Cloud Custodian
- [[cert-manager]] — cert-manager
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kuadrant
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
