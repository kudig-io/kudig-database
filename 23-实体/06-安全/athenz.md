---
title: Athenz (entities)
description: '## 概述'
summary: 'Athenz 是由 Yahoo (Verizon Media) 开发的开源平台，提供基于 X.509 证书的服务身份认证和细粒度的基于角色的访问控制 (RBAC)。它为微服务架构提供零信任安全模型，每个服务都获得唯一的 X.509 身份证书，所有服务间通信通过 mTLS 加密和验证。Athenz 同时支持集中式和去中心化的授权模式。'
category: entities
tags:
- k8s
- cncf
- security
- athenz
- containerd
- rbac
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
- Athenz 是什么
- 如何 Athenz
trigger_keywords:
- Athenz
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Athenz

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Java, Go

## 概述

Athenz 是由 Yahoo（现 Verizon Media）于 2016 年开源的身份认证与授权平台，2017 年进入 CNCF Sandbox。它为微服务架构提供**零信任（Zero Trust）安全模型**的核心能力：每个服务获得唯一的 X.509 SVID（Secure Production Identity Verification）证书作为身份标识，所有服务间通信通过 mTLS 双向加密认证。Athenz 的授权模型支持集中式（ZMS）和去中心化（ZPE）两种模式，ZPE 将策略缓存在本地执行，避免每次授权都依赖中心服务。

Athenz 在 Yahoo 内部管理着超过 10 万个服务的身份和授权，是经过超大规模生产验证的成熟方案。它是 SPIFFE/SPIRE 的早期灵感来源之一，两者在 SVID 概念上高度相似，但 Athenz 更侧重于 RBAC/ABAC 策略管理。

## Key Features

- **X.509 SVID 身份**：每个服务获得基于域（Domain）和实体（Entity）的唯一身份证书
- **ZMS（Identity Store）**：集中式策略管理服务，管理域、角色、策略和实体
- **ZTS（Token Service）**：签发 OAuth2 Access Token 和 TLS 证书
- **ZPE（Policy Engine）**：本地化授权决策引擎，缓存策略，毫秒级响应
- **细粒度 RBAC/ABAC**：支持基于角色和属性的访问控制，策略支持通配符和条件
- **多语言 SDK**：Java、Go、Python、Node.js 客户端库

## Architecture

Athenz 包含三个核心组件：**ZMS（Zentinel Management System）** 是权威的策略管理服务，存储所有域的实体、角色和策略定义；**ZTS（Zentinel Token Service）** 负责签发证书和 OAuth2 Token；**ZPE（Zentinel Policy Engine）** 作为嵌入式库在每个服务中运行，本地缓存策略文件（`.pol`）进行高速授权决策。服务通过 SIA（Service Identity Agent）或 SDK 从 ZTS 获取证书并自动轮换。所有组件通过 mTLS 互相认证。

## K8s 集成

Athenz 提供 **Athenz SIA for Kubernetes**，通过 Admission Webhook 或 Init Container 自动为 Pod 注入 SVID 证书。可配置基于 Namespace/ServiceAccount 到 Athenz 域的映射，实现 Pod 身份的自动注册。ZPE 以 Sidecar 或 Init Container 形式运行，将策略文件注入到 Pod 中。也支持通过 Service Mesh（如 Envoy）的 SDS API 下发证书。

## 生产部署要点

- **域规划**：按组织/产品线划分域，保持域的边界清晰
- **最小权限**：策略遵循最小权限原则，避免使用通配符
- **证书轮换**：配置自动证书轮换，通常 24 小时更新一次
- **本地授权**：使用 ZPE 进行本地授权决策，减少对中心服务的依赖
- **审计日志**：启用 ZMS 审计日志，记录所有策略变更操作

## 生产场景

1. **微服务 mTLS 身份**：数百个服务使用 Athenz 签发的 X.509 证书互相认证
2. **API 授权网关**：API Gateway 调用 ZPE 验证调用方是否有权访问目标资源
3. **跨团队资源访问**：通过域间信任关系实现跨团队资源的细粒度授权
4. **合规审计**：所有授权决策可审计，满足金融/医疗行业的合规要求

## 安装与配置

```bash
# 使用 Helm 在 K8s 中部署 Athenz
helm repo add athenz https://athenz.github.io/athenz
helm install athenz-zms athenz/zms -n athenz --create-namespace \
  --set replicaCount=3 \
  --set persistence.enabled=true
helm install athenz-zts athenz/zts -n athenz \
  --set replicaCount=3

# 安装 SIA Admission Controller
helm install athenz-sia athenz/sia -n athenz

# 验证部署
kubectl get pods -n athenz
kubectl get svc -n athenz
```

```yaml
# Pod 自动注入身份证书 (annotation)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: payment-service
  namespace: production
spec:
  template:
    metadata:
      annotations:
        athenz.io/domain: "myteam.payment"
        athenz.io/service: "payment-api"
        athenz.io/cert-refresh-interval: "24h"
    spec:
      serviceAccountName: payment-sa
      containers:
        - name: payment
          image: myorg/payment:v2.0
          volumeMounts:
            - name: athenz-certs
              mountPath: /var/run/athenz
              readOnly: true
      volumes:
        - name: athenz-certs
          emptyDir:
            medium: Memory
---
# Athenz 域和策略定义
apiVersion: athenz.io/v1
kind: Domain
metadata:
  name: myteam.payment
spec:
  roles:
    - name: admin
      members:
        - user.john
        - user.jane
    - name: readers
      members:
        - myteam.billing.billing-svc
  policies:
    - name: access
      assertions:
        - role: myteam.payment:role.readers
          resource: myteam.payment:data.transactions
          action: read
        - role: myteam.payment:role.admin
          resource: myteam.payment:*
          action: *
```

## 运维操作

```bash
# 🟢 检查 Athenz 组件状态
kubectl get pods -n athenz
kubectl get svc -n athenz

# 🟢 检查 ZMS 健康状态
curl -sk https://athenz-zms.athenz.svc:4443/v1/status

# 🟢 检查 ZTS 健康状态
curl -sk https://athenz-zts.athenz.svc:4443/v1/status

# 🟢 查看域列表
curl -sk -H "Authorization: Bearer <token>" \
  https://athenz-zms.athenz.svc:4443/v1/domain

# 🟢 查看特定域的策略
curl -sk -H "Authorization: Bearer <token>" \
  https://athenz-zms.athenz.svc:4443/v1/domain/myteam.payment/policy

# 🟢 检查 Pod 证书状态
kubectl exec <pod> -- cat /var/run/athenz/ntoken
kubectl exec <pod> -- openssl x509 -in /var/run/athenz/cert.pem -noout -dates

# 🟡 手动触发证书轮换
kubectl exec <pod> -- /usr/bin/sia -force-refresh

# 🟢 检查 SIA Admission Webhook
kubectl get validatingwebhookconfigurations | grep athenz
kubectl get mutatingwebhookconfigurations | grep athenz
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 无法获取证书 | SIA Webhook 未运行 | `kubectl get pods -n athenz` | 检查 SIA Pod 状态 |
| 证书过期 | 自动轮换失败 | `openssl x509 -dates` | 手动触发刷新/检查 ZTS |
| 授权拒绝 | 策略未配置/缓存过期 | 检查 ZPE 日志 | 更新策略/强制刷新缓存 |
| mTLS 握手失败 | 证书链不完整 | `openssl s_client -connect` | 检查 CA 证书配置 |
| ZMS 不可用 | 数据库连接失败 | 检查 ZMS 日志 | 检查 MySQL/PostgreSQL 连接 |
| 跨域访问失败 | 域间信任未配置 | 检查域策略 | 配置跨域角色委托 |

### 排查流程

```
Athenz 身份/授权异常
├── 证书获取失败
│   ├── 检查 SIA Pod 状态和日志
│   ├── 检查 Pod annotation 配置
│   ├── 检查 ZTS 可达性
│   └── 检查 ServiceAccount 到域的映射
├── 授权决策失败
│   ├── 检查 ZPE 策略缓存是否最新
│   ├── 检查域/角色/策略定义
│   ├── 检查调用方身份 (SVID)
│   └── 强制刷新 ZPE 缓存
└── mTLS 通信失败
    ├── 检查证书有效期
    ├── 检查 CA 证书链完整性
    ├── 检查 SAN (Subject Alternative Name)
    └── 检查证书轮换配置
```

## 生产案例

### 案例 1: 微服务零信任身份体系

- **场景**: 200+ 微服务需要互相认证，传统 API Key 管理混乱
- **排查**: API Key 泄露事件频发；无法追踪哪个服务调用了哪个 API
- **方案**: 部署 Athenz；每个服务通过 SIA 自动获取 X.509 SVID；所有服务间通信使用 mTLS；ZPE 本地授权
- **效果**: 消除 API Key 泄露风险；所有调用可审计；授权延迟 <1ms (ZPE 本地)

### 案例 2: 跨团队资源访问控制

- **场景**: 数据团队需要访问支付团队的交易数据，但需要细粒度控制
- **排查**: 传统 RBAC 无法表达“数据团队只能读取脱敏后的交易数据”
- **方案**: 创建跨域角色委托；支付域创建 `data-readers` 角色，委托给数据域；策略限制只能访问 `data.transactions.masked` 资源
- **效果**: 细粒度跨团队授权；所有访问可审计；无需共享凭证

## 对比与替代方案

| 维度 | Athenz | SPIRE | Keycloak | OPA |
|------|--------|-------|----------|-----|
| 身份类型 | X.509 SVID | X.509/JWT SVID | OAuth2/OIDC | ❌ |
| 授权模型 | RBAC/ABAC + ZPE | 身份签发 | RBAC/UMA | Rego 策略 |
| 去中心化 | ✅ ZPE 本地 | ❌ | ❌ | ✅ 本地 |
| 证书管理 | ✅ 自动轮换 | ✅ 自动轮换 | ❌ | ❌ |
| 适用场景 | 服务间零信任 | 服务身份 | 用户身份 | 策略引擎 |
| 规模验证 | 10万+ 服务 | 大规模 | 中规模 | 大规模 |

## 检查清单

- [ ] ZMS/ZTS 多副本部署且健康
- [ ] SIA Admission Webhook 正常工作
- [ ] Pod 自动获取证书且自动轮换
- [ ] 域规划清晰，策略最小权限
- [ ] ZPE 本地授权延迟 <5ms
- [ ] 审计日志已启用并接入 SIEM
- [ ] 证书有效期和轮换策略已配置
- [ ] 跨域信任关系已验证
- [ ] 监控覆盖 ZMS/ZTS/SIA 健康状态

## 参考链接

- [[pod-lifecycle]]
- [[23-实体/02-K8s核心组件/csi-drivers.md|csi-drivers]]

## Related

- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference

<!-- risk-assessed -->
