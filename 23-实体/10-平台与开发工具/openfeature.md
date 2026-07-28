---
title: OpenFeature [entities]
description: '## 概述'
summary: 'OpenFeature 是特性标志（Feature Flag）的开放标准，提供供应商无关的统一 API 和多语言 SDK。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- openfeature
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenFeature 是什么
- 如何 OpenFeature
trigger_keywords:
- OpenFeature
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenFeature

> **CNCF 状态**: Incubating | **类别**: Application Platform | **主要语言**: TypeScript, Go, Java, Python

## 概述

OpenFeature 是特性标志（Feature Flag）的开放标准，2023 年加入 CNCF 孵化。它提供供应商无关的统一 API 和多语言 SDK，允许开发者在不更换代码的情况下切换不同的特性标志提供商（如 LaunchDarkly、Unleash、Flagsmith、ConfigCat 等），实现渐进式发布、A/B 测试和功能开关。OpenFeature 的核心价值在于消除供应商锁定——通过标准化 API 层，应用代码只需依赖 OpenFeature SDK，底层的 Flag 评估引擎可以无缝切换。OpenFeature 还提供了 Flagd（轻量级特性标志评估守护进程），可以在 Kubernetes 中以 sidecar 或 DaemonSet 方式运行，提供无需外部 SaaS 依赖的本地评估能力。

## 核心能力

- **供应商无关**: 统一 API 支持多种后端提供商（LaunchDarkly、Unleash、Flagsmith 等）
- **多语言 SDK**: JavaScript、Go、Java、Python、.NET、PHP、Swift、C++ 等
- **Hooks 机制**: 在标志评估前后执行自定义逻辑（日志、追踪、指标）
- **上下文支持**: 基于用户、环境等上下文进行定向评估
- **类型安全**: 支持布尔、字符串、数字、对象类型的特性标志
- **Flagd**: 轻量级本地评估守护进程，支持 Kubernetes 原生部署

## 架构

OpenFeature 采用分层解耦架构：

- **OpenFeature SDK**: 应用层 API，提供 `Client.getBooleanValue()` 等统一接口
- **Provider Interface**: SDK 与后端 Flag 引擎之间的适配层
- **Flagd**: 轻量级守护进程，支持文件/HTTP/gRPC 配置源，本地评估
- **Flag Configuration**: YAML/JSON 格式的 Flag 定义（名称、类型、默认值、规则）
- **Hooks**: 评估前后的回调（Before/After/Error），集成可观测性和策略
- **In-process/Out-of-process**: 支持进程内评估（SDK 直连）和进程外评估（Flagd）

评估流程：`应用调用 SDK → Provider/Flagd 评估 → 返回 Flag 值 → 应用决策`

## K8s 集成

OpenFeature 通过 Flagd 组件实现 Kubernetes 原生集成。Flagd 可以作为 Sidecar 注入到应用 Pod 中（通过 Mutating Webhook 自动注入），应用通过本地 gRPC/HTTP 访问 Flagd 进行评估，避免对外部 SaaS 的网络依赖。Flagd 的 Flag 配置可以通过 ConfigMap/CRD（`FeatureFlagConfiguration`）管理，支持 GitOps 部署。OpenFeature Operator 管理注入和 CRD 生命周期。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准资源管理机制完全兼容。

## 生产场景

1. **渐进式发布**: 新功能通过 Feature Flag 控制，先开放给少量用户验证
2. **A/B 测试**: 基于 Flag 规则对不同用户群体展示不同功能版本
3. **紧急降级**: 发现问题时通过 Flag 快速关闭问题功能，无需重新部署
4. **多环境配置**: 不同环境（dev/staging/prod）使用不同 Flag 配置

## 安装与配置

```bash
# 安装 OpenFeature Operator
kubectl apply -f https://github.com/open-feature/open-feature-operator/releases/download/v0.2.45/install.yaml
kubectl get pods -n open-feature-operator-system
```

### FeatureFlag CRD 配置

```yaml
apiVersion: core.openfeature.dev/v1alpha2
kind: FeatureFlagConfiguration
metadata:
  name: my-flags
spec:
  featureFlagSpec: |
    {
      "flags": {
        "new-ui-enabled": {
          "state": "ENABLED",
          "variants": {
            "on": true,
            "off": false
          },
          "defaultVariant": "off"
        },
        "checkout-flow": {
          "state": "ENABLED",
          "variants": {
            "v1": "classic",
            "v2": "express"
          },
          "defaultVariant": "v1",
          "targeting": {
            "if": [
              {"in": ["beta-user", {"var": "userGroup"}]},
              "v2", "v1"
            ]
          }
        }
      }
    }
```

### Pod 注入 Flagd Sidecar

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-app
  annotations:
    openfeature.dev/enabled: "true"
    openfeature.dev/featureflagconfiguration: "my-flags"
spec:
  containers:
  - name: app
    image: myapp:latest
    env:
    - name: FLAGD_HOST
      value: "localhost"
    - name: FLAGD_PORT
      value: "8013"
```

## 运维操作

```bash
# 🟢 查看 FeatureFlag 配置
kubectl get featureflagconfiguration -A
kubectl describe featureflagconfiguration my-flags

# 🟢 检查 Flagd sidecar 状态
kubectl get pods -l openfeature.dev/enabled=true
kubectl logs <pod> -c flagd

# 🟡 更新 Flag 配置（实时生效）
kubectl apply -f updated-flags.yaml

# 🟡 启用/禁用特定 Flag
kubectl patch featureflagconfiguration my-flags --type=merge -p '{"spec":{"featureFlagSpec":"{...updated...}"}}'

# 🔴 删除 Flag 配置（影响所有引用 Pod）
kubectl delete featureflagconfiguration my-flags
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Flag 未注入 | annotation 缺失 | `kubectl get pod -o yaml` | 添加 openfeature.dev annotations |
| Flagd 连接失败 | sidecar 未启动 | `kubectl logs pod -c flagd` | 检查 Operator 状态 |
| Flag 值不更新 | 配置未同步 | `kubectl describe featureflagconfiguration` | 检查 CRD 状态 |
| 应用获取默认值 | targeting 规则错误 | 检查 JSON targeting 语法 | 修复 targeting 表达式 |
| Operator CrashLoop | CRD 版本不兼容 | `kubectl logs -n open-feature-operator-system` | 升级 Operator 版本 |

```
排查流程:
├── Flag 未生效
│   ├── kubectl get pod -o yaml → 确认 annotations
│   ├── kubectl logs pod -c flagd → sidecar 日志
│   └── curl localhost:8013/flagd.evaluation.v1.Service/ResolveBoolean → 测试解析
├── Operator 异常
│   ├── kubectl get pods -n open-feature-operator-system
│   └── kubectl logs operator → 查看错误
└── 配置同步延迟
    ├── kubectl describe featureflagconfiguration → 检查 status
    └── 确认 flagd 轮询间隔配置
```

## 生产案例

### 案例 1: 渐进式功能发布

- **场景**: 新结账流程需要按用户组渐进开放
- **方案**: 使用 targeting 规则按 userGroup 变量分流；先开放 5% beta 用户，观察指标后逐步扩大到 100%
- **效果**: 功能发布风险可控，回滚时间从部署回滚 10min 缩短到 Flag 切换 <1s

### 案例 2: 多团队 Flag 管理冲突

- **场景**: 多个团队共用一个 FeatureFlagConfiguration，修改互相影响
- **方案**: 按团队拆分 FeatureFlagConfiguration CRD；Pod annotation 引用多个配置；建立 Flag 命名规范
- **效果**: 消除团队间冲突，Flag 变更影响范围可控

## 对比

| 特性 | OpenFeature | LaunchDarkly | Unleash | Flagsmith | 适用场景 |
|------|-------------|-------------|---------|-----------|----------|
| 开放标准 | ✅ CNCF | ❌ | ❌ | ❌ | 供应商无关 |
| 自托管 | ✅ Flagd | ❌ | ✅ | ✅ | 数据主权 |
| 多语言 SDK | ✅ | ✅ | ✅ | ✅ | 多语言项目 |
| K8s 原生 | ✅ CRD | ❌ | ❌ | ❌ | 云原生 |
| CNCF 状态 | Incubating | 非 CNCF | 非 CNCF | 非 CNCF | 生态成熟度 |

## 架构定位

在 CNCF 生态中，OpenFeature 属于 **Application Platform** 类别，为云原生应用提供标准化特性标志能力。

## 参考链接

- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]

## Related

- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kagent]] — Kagent
- [[devspace]] — DevSpace
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openfeature
- [[23-实体/cncf-infrastructure.md|[[23-实体/15-参考与索引/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
