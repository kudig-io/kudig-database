---
title: Tokenetes (entities)
description: '## 概述'
summary: 'Tokenetes（也称为 Vault CRD Operator）是一个 Kubernetes Operator，用于将 HashiCorp Vault 中的密钥自动同步到 Kubernetes [[Secrets|Secrets]]。它通过自定义资源 (CRD) 简化了 Vault 与 Kubernetes 的集成，支持多种认证方式和密钥类型，'
category: entities
tags:
- k8s
- cncf
- security
- tokenetes
- prometheus
- grafana
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
- Tokenetes 是什么
- 如何 Tokenetes
trigger_keywords:
- Tokenetes
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Tokenetes

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Java / Go

## 概述

Tokenetes 是一个 CNCF 沙箱项目，旨在为微服务架构提供自动化的事务令牌（Transaction Token）管理。它通过短期、范围受限的令牌替代长期 API Key，实现服务间调用的最小权限和可审计访问。Tokenetes 自动拦截和注入服务间通信的认证令牌，无需修改应用代码，实现透明的新一代服务间认证。

## Key Features（核心能力）

- **自动令牌注入**：通过 Sidecar/Init Container 自动拦截和注入认证令牌
- **短期令牌**：令牌有效期极短（分钟级），降低泄露风险
- **范围限制**：令牌绑定到特定操作和资源路径
- **Sidecar 拦截**：通过 iptables/网络代理透明拦截 HTTP 请求注入令牌
- **审计追踪**：记录每个令牌的颁发和使用，支持全链路审计
- **策略引擎**：基于属性和上下文的动态令牌策略

## 架构与工作原理

Tokenetes 由 Token Controller 和 Sidecar Proxy 组成。Token Controller 管理令牌策略和密钥。Sidecar Proxy 部署在每个服务 Pod 中，拦截出站 HTTP 请求，自动从 Token Controller 获取短期令牌并注入到请求头；拦截入站请求，验证令牌有效性并执行授权策略。令牌通过非对称加密签名，Proxy 本地缓存公钥进行快速验证。

## K8s 集成

Tokenetes 通过 Mutating Webhook 自动将 Sidecar Proxy 注入到标记的 Pod 中。Sidecar 以 init-container + sidecar 模式运行，通过 iptables 规则拦截 Pod 的所有网络流量。TokenController 通过 Deployment 部署，以 CRD（TokenPolicy）管理令牌策略。与 K8s ServiceAccount 集成，利用 Workload Identity 简化 Pod 身份认证。

## 生产用例

- **微服务间零信任**：替代长期 API Key 的服务间认证
- **合规审计**：记录每次服务间调用的认证和授权
- **最小权限实施**：为每个服务调用动态授予最小必要权限
- **API 安全加固**：为遗留 API 增加透明的事务级认证

## 安装与配置

```bash
# 🟢 安装 Tokenetes
kubectl apply -f https://github.com/tokenetes/tokenetes/releases/latest/download/tokenetes.yaml

# 🟢 验证安装
kubectl get pods -n tokenetes-system
kubectl get crd | grep tokenetes

# 🟢 查看 Token Controller 状态
kubectl get pods -n tokenetes-system -l app=token-controller

# 🟢 查看 Sidecar 注入状态
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .spec.containers[*]}{.name}{" "}{end}{"\n"}{end}' | grep tokenetes
```

### TokenPolicy CRD 示例

```yaml
apiVersion: tokenetes.io/v1alpha1
kind: TokenPolicy
metadata:
  name: payment-service-policy
  namespace: production
spec:
  # 目标服务
  selector:
    matchLabels:
      app: payment-service
  # 令牌策略
  tokenConfig:
    ttl: 5m  # 令牌有效期
    algorithm: ES256  # 签名算法
    scope:
    - "payment:read"
    - "payment:write"
  # 授权规则
  rules:
  - principal:
      serviceAccount: order-service
      namespace: production
    actions:
    - "payment:create"
    - "payment:read"
    conditions:
    - "request.amount < 10000"
  - principal:
      serviceAccount: admin-service
    actions:
    - "*"
```

### Pod 标注启用 Tokenetes

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: payment-service
  labels:
    app: payment-service
  annotations:
    tokenetes.io/inject: "true"  # 启用 Sidecar 注入
    tokenetes.io/mode: "enforce"  # enforce 或 audit
spec:
  serviceAccountName: payment-service
  containers:
  - name: payment
    image: payment-service:latest
    ports:
    - containerPort: 8080
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 TokenPolicy
kubectl get tokenpolicy -A
kubectl describe tokenpolicy payment-service-policy -n production

# 🟢 查看 Token Controller 日志
kubectl logs -n tokenetes-system -l app=token-controller --tail=50

# 🟢 查看 Sidecar 日志
kubectl logs <pod-name> -c tokenetes-sidecar -n production

# 🟢 查看令牌颁发统计
kubectl exec -n tokenetes-system deploy/token-controller -- /tokenetes stats

# 🟡 更新令牌策略
kubectl apply -f token-policy.yaml

# 🟡 禁用 Sidecar 注入
kubectl annotate pod <pod-name> tokenetes.io/inject- -n production

# 🟢 查看审计日志
kubectl logs -n tokenetes-system -l app=token-controller | grep "audit"
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Sidecar 未注入 | 注解缺失/Webhook 失败 | `kubectl describe pod <name>` | 添加 tokenetes.io/inject 注解 |
| 令牌验证失败 | 时钟不同步/密钥不匹配 | 查看 Sidecar 日志 | 同步 NTP/检查密钥配置 |
| 服务调用被拒绝 | 策略未授权 | `kubectl describe tokenpolicy` | 更新 TokenPolicy 规则 |
| 高延迟 | Sidecar 拦截开销 | 查看 Sidecar 指标 | 调整缓存策略/评估性能 |
| Controller 不可用 | Pod 崩溃/资源不足 | `kubectl logs -l app=token-controller` | 检查资源和配置 |

### 排查流程

```
1. kubectl get pods -n tokenetes-system → 确认组件状态
2. kubectl describe tokenpolicy → 检查策略配置
3. kubectl logs <pod> -c tokenetes-sidecar → 查看 Sidecar 日志
4. kubectl logs -l app=token-controller → 查看 Controller 日志
5. 检查服务间网络连通性
```

## 生产案例

### 案例1: 微服务零信任迁移
- **场景**: 50+ 微服务使用硬编码 API Key，安全风险高
- **方案**: Tokenetes 透明注入短期令牌，替代长期 API Key
- **效果**: 消除硬编码密钥，实现服务间零信任认证

### 案例2: 金融合规审计
- **场景**: 金融系统需要记录每次服务间调用的授权信息
- **方案**: Tokenetes 审计模式记录所有令牌颁发和验证
- **效果**: 满足金融合规审计要求，实现全链路可追溯

## 对比替代方案

| 维度 | Tokenetes | SPIFFE/SPIRE | Service Mesh mTLS | API Key |
|------|-----------|-------------|------------------|--------|
| 认证层级 | 应用层 | 传输层 | 传输层 | 应用层 |
| 令牌有效期 | 分钟级 | 小时级 | 连接级 | 长期 |
| 粒度 | 操作级 | 服务级 | 服务级 | 服务级 |
| 代码修改 | 无需 | 无需 | 无需 | 需要 |
| 审计 | 完整 | 有限 | 有限 | 无 |

## 检查清单

- [ ] Token Controller 副本数 >= 2 (HA)
- [ ] TokenPolicy 已配置并测试
- [ ] Sidecar 注入已启用 (目标 Pod)
- [ ] 令牌 TTL 合理 (建议 5-15m)
- [ ] 审计日志已启用并保留
- [ ] 监控令牌颁发和验证指标
- [ ] 制定密钥轮换策略

## Related

- [[kuma]] — Kuma
- [[kuberhealthy]] — Kuberhealthy
- [[23-实体/trivy.md|[[Trivy|trivy]]]] — Trivy
- [[23-实体/06-安全/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tokenetes
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
