---
title: 渐进式交付策略
summary: 渐进式交付策略：渐进式交付（Progressive Delivery）是持续交付的演进，通过逐步将流量导向新版本并自动验证指标来降低发布风险。核心思想：小步快跑、自动回滚、指标驱动。
category: concepts
tags:
- progressive-delivery
- argo-rollouts
- canary
- gitops
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 渐进式交付策略

## 渐进式交付概述

渐进式交付（Progressive Delivery）是持续交付的演进，通过**逐步将流量导向新版本**并**自动验证指标**来降低发布风险。核心思想：小步快跑、自动回滚、指标驱动。

与传统部署的区别：
| 方式 | 风险控制 | 回滚速度 | 用户影响 |
|------|----------|----------|----------|
| 全量部署 | 无 | 慢（分钟级） | 全部用户 |
| 渐进式交付 | 强 | 快（秒级） | 少量用户 |

## 部署策略

### Canary 部署
将新版本逐步暴露给一小部分流量，监控关键指标后决定是否继续扩大：

```
阶段1: 5% 流量 → 观察 5 分钟
阶段2: 25% 流量 → 观察 5 分钟
阶段3: 75% 流量 → 观察 5 分钟
阶段4: 100% 流量 → 部署完成
```

任意阶段指标异常（错误率 > 1%、P99 延迟上升 20%）自动回滚。

### Blue-Green 部署
维护两个完整环境，通过流量切换实现零停机部署：

- **Blue**：当前生产版本
- **Green**：新版本，经过完整验证后切换流量
- 回滚：将流量切回 Blue 环境

优势：回滚极快（秒级）、无部分流量风险。劣势：需要双倍资源。

### Feature Flags
通过配置开关控制功能可见性，与部署解耦：

- 代码部署 ≠ 功能上线
- 支持按用户/租户/百分比灰度
- A/B 测试能力

相关：OpenFeature 下文详述

## Argo Rollouts v1.9

[Argo Rollouts](https://argoproj.github.io/rollouts/) 是 Kubernetes 原生的渐进式交付控制器，CNCF Argo 项目的一部分。

核心能力：
- **Canary + Blue-Green 原生支持**：声明式定义部署策略
- **指标分析（Analysis）**：集成 Prometheus、Datadog、New Relic 等自动验证
- **自动提升/回滚**：基于 AnalysisRun 结果自动决策
- **流量管理集成**：支持 Istio、Linkerd、Nginx Ingress、AWS ALB、Ambassador
- **v1.9 新特性**：改进的多模板 Analysis、增强的 Experiment 能力、更好的 GitOps 集成

Rollout CRD 示例：
```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: my-app
spec:
  replicas: 5
  strategy:
    canary:
      steps:
        - setWeight: 10
        - pause: { duration: 5m }
        - setWeight: 40
        - pause: { duration: 5m }
        - setWeight: 80
        - pause: { duration: 5m }
      analysis:
        templates:
          - templateName: success-rate
        startingStep: 1
```

## Flagger

[Flagger](https://flagger.app/) 是另一个渐进式交付工具，由 Flux 团队维护：

- 与 **Flux GitOps** 深度集成
- 支持 Canary、Blue-Green、A/B Testing
- 内置指标分析（Prometheus、Datadog、CloudWatch）
- 支持 Istio、Linkerd、Contour、Nginx、Gloo

Argo Rollouts vs Flagger 选择：
| 维度 | Argo Rollouts | Flagger |
|------|---------------|---------|
| GitOps 生态 | Argo CD | Flux |
| 学习曲线 | 中等 | 较低 |
| 社区活跃度 | 高 | 中 |
| 流量管理 | 更丰富 | 够用 |

## Kubernetes 版本演进与渐进式交付

K8s 版本演进对部署策略有直接影响：

### v1.33 → v1.35：In-Place Pod Resize GA
- **v1.33 Beta**：允许在不重启 Pod 的情况下调整 CPU/内存
- **v1.35 GA**：生产就绪
- 影响：渐进式交付中可动态调整资源，避免因资源不足导致的回滚

### v1.34：DRA（Dynamic Resource Allocation）GA
- 声明式分配 GPU、FPGA、网络设备等特殊资源
- 影响：AI/ML 工作负载的渐进式交付更加灵活

### v1.36：User Namespaces GA
- 容器内 UID 映射到宿主机不同 UID，增强安全性
- 影响：多租户渐进式交付的安全隔离更完善

## GitOps 部署流水线

完整的渐进式交付流水线：

```
Git (代码 + 配置变更)
  │
  ▼
GitOps Agent (ArgoCD / Flux)
  │  检测变更，同步到集群
  ▼
Rollouts Controller / Flagger
  │  执行 Canary/Blue-Green 策略
  ▼
指标分析 (AnalysisRun)
  │  查询 Prometheus/Datadog
  ▼
自动决策
  ├── 通过 → 自动提升到下一阶段
  └── 失败 → 自动回滚 + 告警
```

关键原则：
- **声明式**：所有策略定义在 Git 中
- **可观测**：每一步都有指标和日志
- **自动化**：人工干预仅在策略设计阶段
- **可审计**：Git 历史即发布记录

相关：GitOps

## OpenFeature CNCF 标准 + flagd

[OpenFeature](https://openfeature.dev/) 是 CNCF Incubating 项目，定义了**功能标志的统一 SDK 接口**：

- **厂商无关**：同一 SDK 可对接不同 Feature Flag 后端
- **Provider 模式**：flagd、LaunchDarkly、Flagsmith 等都实现 Provider 接口
- **Evaluation Context**：传入用户属性进行上下文感知的标志评估

[flagd](https://flagd.dev/) 是 OpenFeature 的参考实现，CNCF 沙箱项目：
- 轻量级、声明式配置
- 支持 JSON/YAML 标志定义
- gRPC + REST API
- 与 Kubernetes ConfigMap 原生集成

标志定义示例：
```json
{
  "flags": {
    "new-checkout": {
      "state": "ENABLED",
      "variants": { "on": true, "off": false },
      "defaultVariant": "off",
      "targeting": {
        "if": [
          { "in": ["beta-user", { "var": "groups" }] },
          "on",
          "off"
        ]
      }
    }
  }
}
```

## Helm 4 与 OCI Registry

### Helm 4（开发中）
- 改进的依赖管理和 Chart 验证
- 更好的安全特性（签名验证）
- 性能优化
- 向后兼容 Helm 3

### OCI Registry 支持成熟
- Helm Charts 可直接存储在 OCI Registry（GHCR、ECR、Harbor）
- `helm push/pull` 原生 OCI 操作
- 与 GitOps 流水线集成：Chart 版本可追溯

### Kustomize 内部覆盖模式
Kustomize 适合在渐进式交付中管理环境差异：

```
base/
  ├── deployment.yaml
  ├── service.yaml
  └── kustomization.yaml
overlays/
  ├── dev/
  │   └── kustomization.yaml    # 副本数 1，资源限制低
  ├── staging/
  │   └── kustomization.yaml    # 副本数 3，中等资源
  └── prod/
      └── kustomization.yaml    # 副本数 5，高资源，Canary 策略
```

相关：container orchestration

## 相关概念

- GitOps：GitOps 基础设施管理
- container orchestration：Kubernetes 编排
- [[platform-engineering-idp]]：平台工程与 IDP
- cloud native security：零信任安全

## Related

- [[概念/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[概念/platform-engineering-idp.md|platform engineering idp]] — 平台工程与 IDP
- [[概念/slo-error-budget-framework.md|slo error budget framework]] — SLO 与 Error Budget 框架


<!-- risk-assessed -->
