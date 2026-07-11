---
title: OpenFunction (entities)
description: '## 概述'
summary: 'OpenFunction 是一个云原生 FaaS (Function as a [[Service|Service]]) 平台，使开发者能够专注于业务逻辑。它集成了 Knative、KEDA、Dapr、Shipwright 等云原生项目，提供从源码构建到函数运行的完整生命周期管理，支持同步和异步函数运行时。'
category: entities
tags:
- k8s
- cncf
- serverless
- openfunction
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
- OpenFunction 是什么
- 如何 OpenFunction
trigger_keywords:
- OpenFunction
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenFunction

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: Go

## 概述

OpenFunction 是由 KubeSphere 团队（青云科技）开发的云原生 FaaS 平台，2021 年进入 CNCF Sandbox。它为开发者提供从源码到函数运行的完整 Serverless 体验，开发者只需提交源码和简单配置，OpenFunction 自动完成构建（通过 Shipwright + Cloud Native Buildpacks）、部署和事件绑定。

OpenFunction 的独特之处在于同时支持**同步函数**（基于 Knative Serving，HTTP 请求驱动）和**异步函数**（基于 KEDA + Dapr，事件驱动）。异步函数可以从 Kafka、RabbitMQ、MQTT 等消息源消费事件，并利用 Dapr 的 Building Blocks 简化对外部资源（数据库、状态存储、Pub/Sub）的访问。

## Key Features

- **源码到函数**：支持 Go、Node.js、Python、Java 等，通过 BuildPacks 自动构建
- **同步函数**：基于 Knative Serving 的 HTTP 请求驱动函数，支持 Scale-to-Zero
- **异步函数**：基于 KEDA + Dapr 的事件驱动函数，从消息队列消费事件
- **Dapr 集成**：通过 Dapr Building Blocks 访问数据库、Pub/Sub、状态存储等
- **Shipwright 构建**：统一管理多策略镜像构建（BuildPacks、Ko、BuildKit）
- **事件源管理**：通过 EventSource 和 Trigger CRD 定义事件路由

## Architecture

OpenFunction 由 **Function Controller**（管理函数生命周期 CRD）、**Build Controller**（通过 Shipwright 管理构建任务）、**Serving Controller**（管理函数运行时，分发到 Knative 或 KEDA）和 **EventSource/Trigger Controller**（管理异步事件路由）组成。同步函数最终渲染为 Knative Service，异步函数渲染为 KEDA ScaledObject + Deployment + Dapr Sidecar。

## K8s 集成

OpenFunction 完全基于 Kubernetes CRD 构建。核心 CRD 包括 `Function`（函数定义）、`FunctionSample`（函数模板）、`EventSource`（事件源）和 `Trigger`（事件触发规则）。安装通过 Helm Chart 完成，依赖 Knative Serving、KEDA、Dapr、Shipwright 和 Ingress Controller 作为运行时组件。

## 生产部署要点

- **运行时选择**：HTTP API 使用同步函数 (Knative)，消息处理使用异步函数 (KEDA+Dapr)
- **构建缓存**：配置 BuildPacks 缓存加速重复构建
- **资源限制**：为函数设置合理的 CPU/内存限制，避免资源争抢
- **冷启动优化**：对延迟敏感的函数设置 `minReplicas: 1` 避免冷启动
- **事件去重**：异步函数实现幂等处理，应对消息重复投递
- **监控告警**：监控函数错误率和延迟，配置 KEDA 的 fallback 策略

## 生产场景

1. **HTTP API Serverless**：按需扩缩的 RESTful API，空闲时零资源
2. **IoT 事件处理**：MQTT 事件触发异步函数处理传感器数据
3. **文件上传后处理**：S3 事件触发图片转换/视频转码函数
4. **定时数据同步**：Cron 触发的数据 ETL 函数

## 安装

```bash
# 使用 Helm 安装 OpenFunction（含依赖）
helm repo add openfunction https://openfunction.github.io/charts/
helm repo update
helm install openfunction openfunction/openfunction -n openfunction --create-namespace

# 部署一个同步函数
kubectl apply -f - <<EOF
apiVersion: core.openfunction.io/v1beta2
kind: Function
metadata:
  name: function-sample
spec:
  version: "v1.0.0"
  image: "registry.example.com/function-sample:v1"
  imageCredentials:
    name: push-credentials
  build:
    builder: openfunction/builder-go:latest
    env:
      FUNC_NAME: "HelloWorld"
    src: ./sample/go/http
    imagePush: true
  serving:
    template:
      containers:
        - name: function
    triggers:
      http:
        port: 8080
EOF
```

## 对比

| 特性 | OpenFunction | Knative | OpenFaaS | Fission |
|------|-------------|---------|----------|---------|
| 同步+异步 | ✅ | 同步 | ✅ 基础 | ✅ |
| 源码构建 | ✅ BuildPacks | ❌ | ✅ | ✅ |
| Dapr 集成 | ✅ | ❌ | ⚠️ | ❌ |
| 事件源 | ✅ KEDA | ⚠️ | ❌ | ❌ |

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/autoscaling-strategies.md|autoscaling-strategies]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[knative]] — Knative
- [[keda]] — KEDA
- [[shipwright]] — Shipwright
- [[dapr]] — Dapr
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfunction
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
