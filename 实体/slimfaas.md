---
title: SlimFaas (entities)
description: '## 概述'
summary: 'SlimFaas 是一个轻量级的 Kubernetes 原生 Function-as-a-[[Service|Service]] (FaaS) 平台，专注于简单性和低资源占用。它可以将普通的 Kubernetes Deployment 作为函数运行，支持 Scale-to-Zero（缩容到零）和按需自动扩容，无需复杂的 FaaS 框架。'
category: entities
tags:
- k8s
- cncf
- serverless
- slimfaas
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
- SlimFaas 是什么
- 如何 SlimFaas
trigger_keywords:
- SlimFaas
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# SlimFaas

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: C#

## 概述

SlimFaas 是一个极轻量级的 Kubernetes 原生 FaaS（Function-as-a-Service）平台，用 C# 编写。它的设计理念是"**极简**"——将普通的 Kubernetes Deployment 包装为可 Scale-to-Zero 的"函数"，通过简单的 HTTP 代理转发请求并自动管理副本扩缩。与 Knative、OpenFaaS 等完整 FaaS 框架相比，SlimFaas 不需要额外的 CRD、Operator 或消息总线，只需一个 Deployment 和 ConfigMap 即可运行。

SlimFaas 通过监听 HTTP 请求触发目标 Deployment 的扩容（从 0 到 N），在空闲超时后自动缩容到零。它特别适合低流量、间歇性的后台任务 API 场景，如定时报告生成、Webhook 处理器等。

## Key Features

- **Scale-to-Zero**：无请求时自动将目标 Deployment 副本数缩为 0，节省资源
- **HTTP 代理转发**：通过内置反向代理将请求转发到目标函数 Pod
- **按需扩容**：请求到达时自动扩容到指定副本数，支持配置最大副本数
- **无需 CRD**：通过 Kubernetes 原生 Deployment + ConfigMap 配置，无额外依赖
- **异步函数**：支持 async-function 模式，立即返回 202 Accepted 后台处理
- **预热 API**：通过 `wake-function` API 提前预热函数，减少冷启动延迟

## Architecture

SlimFaas 本身作为一个 Deployment 运行在 Kubernetes 中。它包含：**HTTP 代理**（接收外部请求，转发到目标函数）、**Scaler 控制器**（监控请求队列，通过 Kubernetes API 调整目标 Deployment 副本数）、**History 管理**（记录每个函数的最后请求时间，判断是否需要缩容到零）。配置通过 ConfigMap 或环境变量定义函数名、目标 Deployment、缩容超时等参数。

## K8s 集成

SlimFaas 直接使用 Kubernetes API 管理目标 Deployment 的 `spec.replicas`。它不需要自定义 CRD，只需将 SlimFaas 部署为集群中的代理服务，通过 ConfigMap 配置要管理的函数列表。请求到达 SlimFaas 时，它先检查目标 Deployment 副本数，如果为 0 则先扩容再转发请求。

## 生产部署要点

- **健康检查**：为函数配置 readinessProbe，确保冷启动后流量正确路由
- **超时配置**：根据函数的冷启动时间合理设置 scale-down-timeout
- **预热机制**：对延迟敏感的函数使用 wake-function API 提前预热
- **异步模式**：耗时操作使用 async-function 模式避免请求超时
- **资源限制**：为函数设置合理的 CPU/Memory limits 保护集群

## 生产场景

1. **Webhook 处理器**：接收第三方 Webhook（如 GitHub、Stripe），低频但需要即时响应
2. **定时报告生成**：API 触发报告生成，空闲时零资源消耗
3. **后台数据处理**：异步处理上传文件、图片转换等间歇性任务
4. **开发/测试环境**：非关键服务自动缩容到零，降低开发集群成本

## 安装

```bash
# Helm 安装 SlimFaas
helm repo add slimfaas https://slimfaas.github.io/slimfaas/
helm install slimfaas slimfaas/slimfaas -n slimfaas --create-namespace

# 配置一个函数（通过 ConfigMap）
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: slimfaas-config
data:
  functions.yaml: |
    functions:
      - name: report-generator
        deployment: report-gen
        namespace: default
        replicas: 1
        maxReplicas: 3
        scaleDownTimeout: 300
        port: 8080
EOF
```

## 对比

| 特性 | SlimFaas | Knative | OpenFaaS | Fission |
|------|----------|---------|----------|---------|
| 复杂度 | ⭐ 极简 | ⭐⭐⭐ | ⭐⭐ | ⭐⭐ |
| Scale-to-Zero | ✅ | ✅ | ✅ | ✅ |
| CRD 依赖 | ❌ 无 | ✅ 多个 | ✅ | ✅ |
| 资源占用 | < 50MB | ~500MB+ | ~200MB | ~300MB |
| 适用规模 | 小/中 | 大 | 中 | 中 |

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]

## Related

- [[k8up]] — K8up
- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- slimfaas
- [[实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
