---
title: Dapr (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- dapr
- istio
- redis
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dapr 是什么
- 如何 Dapr
trigger_keywords:
- Dapr
prerequisites:
- kubectl-basics
- service-mesh-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dapr

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

Dapr（Distributed Application Runtime）是一个 CNCF 毕业项目，由 Microsoft 主导开发。它是一个可移植的、事件驱动的分布式应用运行时，为云原生和边缘应用提供构建块（Building Block）抽象。开发者通过标准 API 调用即可获得服务调用、状态管理、发布订阅、密钥管理、可观测性等分布式系统能力，无需关注底层实现。Dapr 采用 Sidecar 模式，与业务代码解耦，支持 Go, Java, Python, .NET, Node.js 等多语言。项目于 2021 年加入 CNCF，2024 年正式毕业。

## Key Features（核心能力）

- **Building Blocks API**：提供服务调用、State、Pub/Sub、Bindings、Secrets、Actors、Workflow 等标准 API
- **Sidecar 模式**：以 Sidecar 容器运行，与业务代码通过 HTTP/gRPC 通信
- **多语言支持**：通过标准 API 支持 Go, Java, Python, .NET, JS, Rust 等
- **Component 模型**：通过 Component CRD 插入不同的后端（Redis、Kafka、AWS、Azure）
- **可插拔架构**：状态存储、消息总线、密钥存储等组件均可替换
- **Actor 模型**：内置虚拟 Actor 模式实现有状态的并发单元

## 架构与工作原理

Dapr 采用 Sidecar 架构：Dapr Sidecar（daprd）与业务容器运行在同一个 Pod 中，通过 localhost HTTP/gRPC 通信。Sidecar 从 Dapr Operator 获取配置，从 Component CRD 加载各 Building Block 的后端连接信息。Dapr Operator 管理 Sidecar 注入和配置分发；Dapr Sentry 提供 mTLS 证书管理；Dapr Placement Service 管理 Actor 分布。

## K8s 集成

Dapr 在 Kubernetes 中通过 Mutating Webhook 自动将 Sidecar（daprd）注入到标注了 dapr.io/enabled: "true" 的 Pod 中。Component CRD 定义状态存储、消息总线等后端配置。Configuration CRD 控制 Dapr 运行时行为（如 tracing、mTLS）。通过 K8s Service 暴露 Dapr API，应用通过 Dapr App Protocol 与 Sidecar 通信。

## 生产用例

- **微服务通信**：通过 Dapr Service Invocation API 实现服务间 mTLS 调用
- **事件驱动架构**：使用 Dapr Pub/Sub API 连接 Kafka/RabbitMQ 而不绑定具体中间件
- **有状态应用**：通过 State API 使用 Redis/CosmosDB 管理应用状态
- **多语言团队**：不同语言团队共享同一分布式系统能力抽象

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add dapr https://dapr.github.io/helm-charts/
helm repo update

# 🟢 安装 Dapr 控制平面
helm install dapr dapr/dapr \
  -n dapr-system --create-namespace \
  --set global.mtls.enabled=true \
  --set global.ha.enabled=true \
  --set dapr_operator.replicaCount=3

# 🟢 验证安装
kubectl get pods -n dapr-system
kubectl get crd | grep dapr.io

# 🟢 启用应用 Pod 的 Dapr Sidecar
kubectl annotate deploy myapp dapr.io/enabled=true \
  dapr.io/app-id=myapp \
  dapr.io/app-port=8080

# 🟢 安装 Dapr CLI
curl -fsSL https://raw.githubusercontent.com/dapr/cli/master/install/install.sh | bash
dapr init -k  # K8s 模式初始化
```

### Component CRD 示例

```yaml
# State Store (Redis)
apiVersion: dapr.io/v1alpha1
kind: Component
metadata:
  name: statestore
  namespace: production
spec:
  type: state.redis
  version: v1
  metadata:
    - name: redisHost
      value: redis-master:6379
    - name: redisPassword
      secretKeyRef:
        name: redis-secret
        key: password
    - name: actorStateStore
      value: "true"
---
# Pub/Sub (Kafka)
apiVersion: dapr.io/v1alpha1
kind: Component
metadata:
  name: pubsub
  namespace: production
spec:
  type: pubsub.kafka
  version: v1
  metadata:
    - name: brokers
      value: kafka-0.kafka:9092,kafka-1.kafka:9092
    - name: consumerGroup
      value: myapp-group
    - name: authType
      value: password
    - name: saslUsername
      secretKeyRef:
        name: kafka-secret
        key: username
    - name: saslPassword
      secretKeyRef:
        name: kafka-secret
        key: password
---
# Secrets Store
apiVersion: dapr.io/v1alpha1
kind: Component
metadata:
  name: vault
  namespace: production
spec:
  type: secretstores.hashicorp.vault
  version: v1
  metadata:
    - name: vaultAddr
      value: http://vault:8200
    - name: vaultKVPrefix
      value: secret/data/myapp
---
# Dapr Configuration
apiVersion: dapr.io/v1alpha1
kind: Configuration
metadata:
  name: appconfig
  namespace: production
spec:
  tracing:
    samplingRate: "1"
    zipkin:
      endpointAddress: http://zipkin:9411/api/v2/spans
  mtls:
    enabled: true
    workloadCertTTL: 24h
    allowedClockSkew: 15m
  accessControl:
    defaultAction: deny
    trustDomain: production
    policies:
      - appId: frontend
        defaultAction: allow
      - appId: backend
        defaultAction: allow
        operations:
          - name: /api/*
            httpVerb: ['GET', 'POST']
            action: allow
```

## 运维操作

```bash
# 🟢 查看 Dapr 控制平面状态
kubectl get pods -n dapr-system
dapr status -k

# 🟢 查看已注册的组件
kubectl get components -A

# 🟢 查看应用 Dapr 配置
kubectl get deploy myapp -o jsonpath='{.spec.template.metadata.annotations}' | jq .

# 🟢 查看 Sidecar 日志
kubectl logs <pod-name> -c daprd --tail=100

# 🟡 更新组件配置
kubectl apply -f updated-component.yaml
# Sidecar 会自动热加载组件变更

# 🟡 升级 Dapr 版本
helm upgrade dapr dapr/dapr -n dapr-system --set global.tag=1.14.0

# 🔴 禁用应用的 Dapr Sidecar
kubectl annotate deploy myapp dapr.io/enabled-
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| Sidecar 未注入 | 缺少注解 | `kubectl get pod -o yaml \| grep dapr` | 添加 dapr.io/enabled 注解 |
| 组件初始化失败 | 后端不可达 | `kubectl logs <pod> -c daprd` | 检查 Component 配置和网络 |
| 服务调用失败 | mTLS 证书问题 | `dapr status -k` | 检查 Sentry 和 trustDomain |
| State 操作超时 | Redis 连接池耗尽 | 查看 Sidecar 日志 | 调整连接池参数 |

```bash
# 排查流程
# 1. 检查 Dapr 控制平面
kubectl get pods -n dapr-system
dapr status -k

# 2. 检查 Sidecar 状态
kubectl get pod <pod> -o jsonpath='{.spec.containers[*].name}'
kubectl logs <pod> -c daprd --tail=50

# 3. 检查组件状态
kubectl get components -n production -o yaml | grep -A5 status

# 4. 测试 Dapr API
curl http://localhost:3500/v1.0/healthz
curl http://localhost:3500/v1.0/metadata
```

## 生产案例

### 案例1：多语言微服务架构
- **场景**：企业有 Go/Java/Python/.NET 团队，需要统一的分布式系统能力
- **方案**：Dapr Sidecar 提供统一的 State/PubSub/Secrets API；各语言团队通过 HTTP/gRPC 调用 Dapr API；Component 抽象屏蔽后端差异
- **效果**：新服务接入分布式能力从 2周 缩短到 1天，后端切换无需修改业务代码

### 案例2：事件驱动订单系统
- **场景**：电商订单系统需要解耦订单、库存、支付、通知服务
- **方案**：Dapr Pub/Sub + Kafka；订单服务发布事件，其他服务订阅；Dapr Binding 集成外部支付网关
- **效果**：服务间耦合度降低 90%，新订阅者接入无需修改发布者代码

## 对比替代方案

| 维度 | Dapr | Istio | Spring Cloud | Knative |
|------|------|-------|-------------|--------|
| 抽象层级 | 应用层 | 网络层 | 应用层 | 平台层 |
| 语言支持 | 多语言 | 多语言 | Java | 多语言 |
| State管理 | 内置 | 无 | 有 | 无 |
| Pub/Sub | 内置 | 无 | 有 | 有 |
| Actor | 内置 | 无 | 无 | 无 |
| 学习曲线 | 中 | 高 | 中 | 中 |

## 检查清单

- [ ] Dapr 控制平面已部署且所有组件 Running
- [ ] mTLS 已启用（生产环境必须）
- [ ] Component 已配置且后端可达
- [ ] 应用 Pod 已正确标注 Dapr 注解
- [ ] Access Control 策略已配置
- [ ] Tracing 已启用（Zipkin/Jaeger）
- [ ] Sidecar 资源限制已配置
- [ ] 组件 Secret 已使用 K8s Secret 存储

## Related

- [[02-istio-advanced-traffic-management]] — [[istio|Istio]]io 高级流量管理|Istio 高级流量管理]]
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[pixie]] — Pixie
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-dapr-enterprise-distributed-runtime
- dapr
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
