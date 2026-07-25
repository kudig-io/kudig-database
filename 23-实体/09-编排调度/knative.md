---
title: Knative (entities)
description: '## 概述'
summary: 'Knative 是由 Google 发起、捐赠至 CNCF 的 Graduated 项目，为 Kubernetes 提供无服务器（Serverless）工作负载管理能力。它包含 Serving（请求驱动的自动扩缩容，支持 Scale-to-Zero）和 Eventing（事件驱动架构）两大核心组件，使开发者无需关注底层基础设施即可运行和连接服务。'
category: entities
tags:
- k8s
- cncf
- orchestration
- knative
- prometheus
- grafana
- istio
- crd
- operator
- kserve
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Knative 是什么
- 如何 Knative
trigger_keywords:
- Knative
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Knative

> **CNCF 状态**: Graduated | **类别**: Orchestration | **主要语言**: Go

## 概述

Knative（发音为 "kay-nay-tiv"）是 Google 于 2018 年开源的 Kubernetes 原生 Serverless 框架，2022 年成为 CNCF Graduated 项目。它在 Kubernetes 之上增加了两组关键能力：**Serving**（基于请求的自动扩缩容路由系统）和 **Eventing**（声明式的事件路由系统）。Serving 使服务能够根据流量自动从零扩展（Scale-to-Zero），并在流量减少时自动缩容至零，极大降低空闲资源的成本。Eventing 提供标准化的 CloudEvents 消息总线，将事件源（Kafka、GitHub、Kubernetes 事件等）与消费者（Knative Service）解耦连接。

Knative 通过 CRD（Configuration、Revision、Route、Service）扩展 Kubernetes API，对开发者隐藏了 Deployment、Service、Ingress 等底层资源的复杂性。支持 Istio、Kourier、Contour、Envoy Gateway 等多种网络层，也支持通过 KEDA 进行事件驱动的自动扩缩。

## Key Features

- **请求驱动自动扩缩（Autoscaling）**：基于并发请求数或 RPS 自动扩缩 Pod，支持 Scale-to-Zero 和突发流量快速响应
- **Revision 管理**：每次配置变更生成不可变 Revision，支持金丝雀发布和流量分割
- **CloudEvents 事件路由**：通过 Trigger、Broker、Source 抽象实现声明式的事件驱动架构
- **多网络层支持**：支持 Istio、Kourier（轻量级）、Contour、Envoy Gateway，无需强制依赖完整 Service Mesh
- **渐进式交付**：通过 `traffic` 字段实现金丝雀、蓝绿和多版本路由
- **KEDA 集成**：基于 Kafka、RabbitMQ 等消息队列深度自动扩缩

## Architecture

Knative Serving 由三个核心控制器组成：**KnS Controller**（管理 Knative Service 生命周期）、**KnR Controller**（管理 Route 和流量路由）、**Autoscaler / KPA**（Pod Autoscaler，根据队列深度计算所需副本数）。**Activator** 组件负责在 Scale-to-Zero 期间接收请求并触发冷启动，同时暂存请求避免丢失。

Eventing 由 **Broker**（事件总线）、**Trigger**（订阅过滤规则）和 **Source**（事件源）构成，使用消息总线（如 Kafka Channel、InMemory Channel）进行事件投递。

## K8s 集成

Knative 完全基于 Kubernetes CRD 构建。Service 资源自动生成 Deployment、Service、VirtualService/HTTPRoute 等底层资源。Autoscaler 通过 Kubernetes Metrics API 和自定义指标（Prometheus）驱动 HPA 或 Knative 自有的 KPA。安装通过 Knative Operator 或 YAML 清单完成，与标准 Kubernetes RBAC 和 NetworkPolicy 兼容。

## 生产部署要点

- **minReplicas**：延迟敏感服务设置 `minReplicas: 1` 避免 Scale-to-Zero 的冷启动延迟
- **HPA vs KPA**：CPU 密集型用 HPA，请求驱动用 KPA
- **Activator 副本数**：高流量场景增加 Activator 副本，避免成为瓶颈
- **网络层选择**：生产环境推荐 Kourier（轻量）或 Istio（完整功能），避免使用 InMemory Channel
- **ConfigMap 调优**：调整 `config-autoscaler` 中的 `target`、`maxScale`、`scaleDownDelay`

## 生产场景

1. **API 后端 Serverless 化**：突发流量 API（如报告生成、图像处理）按需扩缩，空闲时零成本
2. **异步事件处理**：Kafka 事件触发 Knative Service，实现 EDA 架构
3. **CI/CD 触发**：Git Push 事件通过 Eventing 触发构建流水线
4. **ML 推理端点**：结合 KServe，提供按需扩展的模型推理服务

## 安装与配置

```bash
# 使用 Knative Operator 安装
kubectl apply -f https://github.com/knative/operator/releases/download/knative-v1.15.0/operator.yaml
kubectl create namespace knative-serving
# 创建 KnativeServing CR 触发安装
kubectl apply -f - <<EOF
apiVersion: operator.knative.dev/v1beta1
kind: KnativeServing
metadata:
  name: knative-serving
  namespace: knative-serving
spec:
  ingress:
    kourier:
      enabled: true
EOF
# 验证安装
kubectl get pods -n knative-serving
kubectl get ksvc -A
```

```yaml
# Knative Service 示例
apiVersion: serving.knative.dev/v1
kind: Service
metadata:
  name: hello-app
  namespace: default
spec:
  template:
    metadata:
      annotations:
        autoscaling.knative.dev/minScale: "0"
        autoscaling.knative.dev/maxScale: "10"
        autoscaling.knative.dev/target: "100"  # 每 Pod 100 并发
    spec:
      containers:
      - image: gcr.io/knative-samples/helloworld-go
        ports:
        - containerPort: 8080
        env:
        - name: TARGET
          value: "Knative"
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 256Mi
  traffic:
  - percent: 90
    latestRevision: true
  - percent: 10
    revisionName: hello-app-00001  # 金丝雀 10%
```

```bash
# 部署并测试
kubectl apply -f hello-app.yaml
kubectl get ksvc hello-app
# 获取 URL 并测试
curl $(kubectl get ksvc hello-app -o jsonpath='{.status.url}')
# 观察 Scale-to-Zero
kubectl get pods -w  # 等待无流量后 Pod 缩为 0
```

## 运维操作

```bash
# 🟢 查看 Knative Service 状态
kubectl get ksvc -A
kubectl get revisions -A
kubectl get routes -A

# 🟢 查看自动扩缩状态
kubectl get pods -l serving.knative.dev/service=hello-app
kubectl get podautoscaler -A
kubectl logs -n knative-serving -l app=autoscaler --tail=50

# 🟢 查看 Eventing 状态
kubectl get brokers -A
kubectl get triggers -A
kubectl get sources -A

# 🟡 调整自动扩缩参数
kubectl patch configmap config-autoscaler -n knative-serving \
  --type merge -p '{"data":{"max-scale-up-rate":"1000","scale-down-delay":"5m"}}'

# 🟡 流量分割调整
kubectl patch ksvc hello-app --type merge -p '{"spec":{"traffic":[{"percent":100,"latestRevision":true}]}}'

# 🔴 删除 Knative Service
kubectl delete ksvc hello-app
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Service 无法访问 | Ingress/Activator 异常 | `kubectl get pods -n knative-serving` | 检查 Kourier/Istio 状态 |
| Scale-to-Zero 后无法唤醒 | Activator 异常/冷启动超时 | `kubectl logs -n knative-serving -l app=activator` | 检查 Activator 资源和超时配置 |
| 扩容不及时 | Autoscaler 配置保守 | `kubectl logs -n knative-serving -l app=autoscaler` | 调整 target 和 max-scale-up-rate |
| Revision 创建失败 | 镜像拉取失败/资源不足 | `kubectl describe revision <name>` | 检查镜像和资源配额 |
| Eventing 事件丢失 | Broker/Trigger 配置错误 | `kubectl describe trigger <name>` | 检查 filter 和 subscriber 配置 |

```
排查流程：
├─ Service 不可用
│  ├─ kubectl get ksvc 检查 Ready 状态
│  ├─ kubectl get revision 检查最新 Revision
│  └─ 检查 Ingress 和 Activator 状态
├─ 自动扩缩问题
│  ├─ 检查 Autoscaler 日志
│  ├─ 检查 config-autoscaler 配置
│  └─ 确认 Metrics 采集正常
└─ Eventing 问题
   ├─ 检查 Broker 是否 Ready
   ├─ 检查 Trigger filter 规则
   └─ 检查 Channel 消息投递状态
```

## 生产案例

### 案例 1：突发流量 API Serverless 化

- **场景**: 报告生成 API 每天仅高峰期 2 小时有流量，其余时间空闲
- **排查**: 传统 Deployment 持续占用 4 Pod 资源，利用率 <10%
- **方案**: 迁移至 Knative Service，Scale-to-Zero + 基于并发自动扩容
- **效果**: 空闲时零资源消耗，高峰期自动扩至 20 Pod，年节省成本 75%

### 案例 2：事件驱动数据处理管道

- **场景**: Kafka 消息需要触发处理函数，传统方案需要常驻消费者
- **排查**: 消费者空闲时浪费资源，高峰时处理能力不足
- **方案**: Knative Eventing + Kafka Source，消息触发 Knative Service 处理
- **效果**: 按需消费，无消息时零 Pod，高峰时自动扩容处理

## 对比

| 维度 | Knative | OpenFaaS | KEDA | Fission |
|------|---------|----------|------|---------|
| Scale-to-Zero | ✅ 原生 | ✅ | ✅(配合 Deploy) | ✅ |
| 事件驱动 | ✅ Eventing | ⚠️ 有限 | ✅ 核心 | ⚠️ |
| 流量管理 | ✅ Revision | ❌ | ❌ | ❌ |
| CNCF 状态 | Graduated | 活跃 | Graduated | 非 CNCF |
| 适用场景 | 全功能 Serverless | 轻量 FaaS | 事件扩缩 | 轻量 FaaS |

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[chaosblade]] — ChaosBlade
- [[network-service-mesh]] — [[23-实体/04-网络/network-service-mesh.md|Network Service Mesh (NSM)]]]Service Mesh）|Service Mesh]] (NSM)
- [[kserve]] — KServe
- [[meshery]] — Meshery
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- knative
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
