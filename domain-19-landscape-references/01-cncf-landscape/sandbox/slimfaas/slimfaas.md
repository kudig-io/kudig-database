---
title: SlimFaas
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SlimFaas 是什么
- 如何 SlimFaas
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- SlimFaas
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

title: SlimFaas
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- serverless
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- SlimFaas 是什么
- 如何 SlimFaas
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- SlimFaas
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# SlimFaas

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/AxaFrance/slimfaas |
| **许可证** | Apache-2.0 |
| **开发语言** | C# |
| **CNCF 状态** | Sandbox |

---

## 项目概述

SlimFaas 是一个轻量级的 Kubernetes 原生 Function-as-a-Service (FaaS) 平台，专注于简单性和低资源占用。它可以将普通的 Kubernetes Deployment 作为函数运行，支持 Scale-to-Zero（缩容到零）和按需自动扩容，无需复杂的 FaaS 框架。SlimFaas 通过简单的 HTTP 代理机制转发请求到目标函数，并管理函数的冷启动和生命周期。

### 核心特性

- **Scale-to-Zero**: 无请求时自动将函数缩容到 0 副本
- **按需扩容**: 收到请求时自动唤醒函数并扩容
- **零改造**: 任何 HTTP 服务都可以作为函数运行，无需 SDK
- **同步/异步**: 支持同步调用和异步（队列）调用模式
- **极简架构**: 单个代理 Pod 即可运行整个平台
- **低资源**: 代理本身仅需 ~30MB 内存

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│            SlimFaas Proxy                          │
│                                                    │
│  ┌──────────────────────────────────────────┐     │
│  │  HTTP Router                              │     │
│  │  /function/{name}/... → 同步调用          │     │
│  │  /async-function/{name}/... → 异步队列    │     │
│  │  /wake-function/{name} → 预热唤醒         │     │
│  └──────────────────┬───────────────────────┘     │
│                     │                              │
│  ┌──────────────────▼───────────────────────┐     │
│  │  Scale Manager                            │     │
│  │  - 监控请求活动                            │     │
│  │  - 无活动 → Scale to 0                    │     │
│  │  - 收到请求 → Scale to N                  │     │
│  │  - 冷启动等待                              │     │
│  └──────────────────────────────────────────┘     │
└────────────────────────────────────────────────────┘
         │              │              │
    ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
    │Func A   │   │Func B   │   │Func C   │
    │(0-N Pod)│   │(0-N Pod)│   │(0-N Pod)│
    │ Deploy  │   │ Deploy  │   │ Deploy  │
    └─────────┘   └─────────┘   └─────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装
helm repo add slimfaas https://axafrance.github.io/slimfaas/
helm install slimfaas slimfaas/slimfaas \
  --namespace slimfaas \
  --create-namespace
```

### 部署函数

```yaml
# 任何 Deployment 都可以作为函数
apiVersion: apps/v1
kind: Deployment
metadata:
  name: hello-function
  annotations:
    slimfaas/function: "true"
    slimfaas/scale-down-timeout: "300"    # 5分钟无请求后缩容到 0
    slimfaas/default-visibility: "true"
  labels:
    slimfaas/min-scale: "0"               # 最小副本数
    slimfaas/max-scale: "10"              # 最大副本数
spec:
  replicas: 0
  selector:
    matchLabels:
      app: hello-function
  template:
    metadata:
      labels:
        app: hello-function
    spec:
      containers:
        - name: hello
          image: myorg/hello-function:latest
          ports:
            - containerPort: 8080
          readinessProbe:
            httpGet:
              path: /health
              port: 8080
```

### 调用函数

```bash
# 同步调用 (等待结果)
curl http://slimfaas.slimfaas.svc/function/hello-function/api/greet \
  -H "Content-Type: application/json" \
  -d '{"name": "World"}'

# 异步调用 (立即返回，后台处理)
curl http://slimfaas.slimfaas.svc/async-function/hello-function/api/process \
  -H "Content-Type: application/json" \
  -d '{"data": "process this"}'

# 预热函数 (不发送实际请求，只唤醒)
curl -X POST http://slimfaas.slimfaas.svc/wake-function/hello-function
```

---

## 与其他方案对比

| 特性 | SlimFaas | OpenFaaS | Knative | KEDA |
|:---|:---|:---|:---|:---|
| 架构复杂度 | 极简 (1 Pod) | 中 | 高 | 中 |
| 资源占用 | ~30MB | ~100MB+ | ~500MB+ | ~100MB |
| Scale-to-Zero | 内置 | 需配置 | 内置 | 需配置 |
| 函数改造 | 无需 | 需要 | 无需 | 无需 |
| 异步队列 | 内置 | 内置 | 需额外组件 | 外部队列 |
| 适用场景 | 轻量级 FaaS | 通用 FaaS | 企业级 Serverless | 事件驱动 |

---

## 最佳实践

1. **健康检查**: 为函数配置 readinessProbe，确保冷启动后流量正确路由
2. **超时配置**: 根据函数的冷启动时间合理设置 scale-down-timeout
3. **预热机制**: 对延迟敏感的函数使用 wake-function API 提前预热
4. **异步模式**: 耗时操作使用 async-function 模式避免请求超时
5. **资源限制**: 为函数设置合理的 CPU/Memory limits 保护集群

---

## 参考资源

- [SlimFaas GitHub](https://github.com/AxaFrance/slimfaas)
- [SlimFaas Helm Chart](https://axafrance.github.io/slimfaas/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
