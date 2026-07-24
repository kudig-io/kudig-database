---
title: Argo Workflows
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- argo
- prometheus
- grafana
- helm
- argocd
- containerd
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Argo Workflows 是什么
- 如何 Argo Workflows
trigger_keywords:
- Argo
- Workflows
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Argo|Argo]] Workflows

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **Argo CD**: GitOps 持续交付，声明式应用部署
- **Argo Workflows**: Kubernetes 原生工作流引擎
- **Argo Events**: 事件驱动自动化
- **Argo Rollouts**: 渐进式发布策略（金丝雀、蓝绿）

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用 SSO 集成身份认证
- 配置 RBAC 细粒度权限
- 启用 Git webhook 触发同步
- 配置应用健康检查
- 配置 repo server 缓存
- 合理设置同步间隔

## 架构定位

在 CNCF 生态中，Argo 属于 **Continuous Integration & Delivery** 类别，为云原生应用提供完整的 GitOps 和 CI/CD 能力。

## 安装与配置

```bash
# 安装 Argo Workflows
kubectl create namespace argo
kubectl apply -n argo -f https://github.com/argoproj/argo-workflows/releases/latest/download/install.yaml

# 安装 Argo Rollouts
kubectl create namespace argo-rollouts
kubectl apply -n argo-rollouts -f https://github.com/argoproj/argo-rollouts/releases/latest/download/install.yaml

# 安装 Argo Events
kubectl create namespace argo-events
kubectl apply -n argo-events -f https://github.com/argoproj/argo-events/releases/latest/download/install.yaml
```

### Workflow 示例

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  generateName: ci-pipeline-
  namespace: argo
spec:
  entrypoint: build-test-deploy
  templates:
  - name: build-test-deploy
    steps:
    - - name: build
        template: build-image
    - - name: test
        template: run-tests
    - - name: deploy
        template: deploy-app
  - name: build-image
    container:
      image: docker:24
      command: [docker, build, -t, myapp:latest, .]
  - name: run-tests
    container:
      image: myapp:latest
      command: [make, test]
  - name: deploy-app
    container:
      image: bitnami/kubectl
      command: [kubectl, apply, -f, k8s/]
```

## 运维操作

```bash
# 🟢 查看 Workflow 状态
argo list -n argo
argo get <workflow-name> -n argo
argo logs <workflow-name> -n argo

# 🟢 查看 Rollout 状态
kubectl argo rollouts get rollout my-app -n production
kubectl argo rollouts status my-app -n production

# 🟡 手动触发 Workflow
argo submit workflow.yaml -n argo
argo resume <workflow-name> -n argo

# 🟡 暂停/继续 Rollout
kubectl argo rollouts pause my-app -n production
kubectl argo rollouts resume my-app -n production

# 🔴 终止 Workflow
argo delete <workflow-name> -n argo
argo delete --all -n argo

# 🔴 中止 Rollout（回滚）
kubectl argo rollouts abort my-app -n production
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Workflow 卡住 Pending | 资源不足/节点调度失败 | `kubectl describe pod -n argo` | 检查资源配额和节点状态 |
| Step 失败 | 容器执行错误 | `argo logs <wf> --node-field-selector=displayName=step` | 检查日志修复脚本 |
| Rollout 卡住 | 健康检查未通过 | `kubectl argo rollouts get rollout my-app` | 检查 Pod readiness |
| Controller 异常 | CRD 版本不匹配 | `kubectl logs -n argo-rollouts deployment/argo-rollouts` | 升级 CRD |
| 事件未触发 | EventSource 配置错误 | `kubectl logs -n argo-events -l eventsource-name=my-es` | 检查事件源连接 |

```
排查流程:
├── Workflow 异常
│   ├── argo get → 节点状态树
│   ├── argo logs → 失败 Step 日志
│   ├── kubectl get pods -n argo → Pod 状态
│   └── kubectl get events -n argo → 调度事件
├── Rollout 异常
│   ├── kubectl argo rollouts get → 发布状态
│   ├── kubectl get rs → ReplicaSet 状态
│   └── 检查 AnalysisRun → 指标分析结果
└── 控制平面
    ├── kubectl get pods -n argo → 组件健康
    ├── kubectl logs controller → 控制器日志
    └── 检查 CRD 版本 → API 兼容性
```

## 生产案例

### 案例1: ML 训练流水线 DAG 编排

- **场景**: AI 团队需要编排数据预处理→训练→评估→部署的多步骤流水线
- **排查**: 初始用 Jenkins 管理，依赖关系复杂，失败重试困难
- **方案**:
  1. 使用 Argo Workflows DAG 模板定义训练流水线
  2. 配置 `retryStrategy` 自动重试失败步骤
  3. 使用 Artifact 传递模型文件（S3 后端）
- **效果**: 流水线执行时间缩短 40%，失败自动恢复率达 95%

### 案例2: 金丝雀发布自动化回滚

- **场景**: 新版本上线后错误率飙升，需要自动回滚
- **排查**: Argo Rollouts AnalysisRun 检测到 5xx 率超过 5% 阈值
- **方案**:
  1. 配置 AnalysisTemplate 关联 Prometheus 查询
  2. 设置 `failureLimit: 2` 自动 abort
  3. 添加 Slack 通知 Webhook
- **效果**: 故障发现到回滚完成仅需 90s，无需人工介入

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[crossplane]]
- [[实体/vault.md|vault]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/gitops-principles.md|gitops-principles]]

## Related

- [[sops]] — SOPS (Secrets OPerationS)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 01-argo-cd-enterprise-gitops
- 99-argo-cd-gitops-guide
- 36-ecosystem-kustomize-helm-argocd
- 09-gitops-workflow-argocd
- [[故障诊断/高级排障/38-gitops-argocd-troubleshooting.md|38-gitops-argocd-troubleshooting]]
- [[工作负载/06-java-cicd-tekton-argocd.md|06-java-cicd-tekton-argocd]]
- [[故障诊断/FTA故障树/list/gitops-argocd-fta.md|GitOps(ArgoCD) 异常故障树分析]]
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.8
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.12.md|RELEASE-NOTES-2.12]]
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.13.md|RELEASE-NOTES-2.13]]
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-2.4
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- RELEASE-NOTES-2.0
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.1.md|RELEASE-NOTES-3.1]]
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-2.1
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.0.md|RELEASE-NOTES-3.0]]
- RELEASE-NOTES-0.7
- RELEASE-NOTES-2.5
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-2.2
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.3.md|RELEASE-NOTES-3.3]]
- RELEASE-NOTES-0.4
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.6.md|RELEASE-NOTES-2.6]]
- RELEASE-NOTES-1.1
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.7.md|RELEASE-NOTES-2.7]]
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-2.3
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.2.md|RELEASE-NOTES-3.2]]
- RELEASE-NOTES-0.5
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.8.md|RELEASE-NOTES-2.8]]
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.10.md|RELEASE-NOTES-2.10]]
- RELEASE-NOTES-0.10
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.14.md|RELEASE-NOTES-2.14]]
- RELEASE-NOTES-0.11
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.9.md|RELEASE-NOTES-2.9]]
- [[归档/release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.11.md|RELEASE-NOTES-2.11]]
- [[实体/pixie.md|Pixie]]
- [[实体/kuberhealthy.md|Kuberhealthy]]
- [[实体/kubescape.md|Kubescape]]
- [[实体/perses.md|Perses]]
- [[实体/03-prometheus-ha-deployment.md|Prometheus 高可用部署]]
- [[实体/trickster.md|Trickster]]
- [[实体/distribution.md|Distribution]]
- [[实体/hami.md|HAMI]]
- [[实体/06-containerd-observability.md|containerd 可观测性]]
- [[实体/kubeelasti.md|KubeElastic]]
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- [[实体/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[实体/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[实体/storage-terms.md|K8s 存储术语参考]] — Cross-reference
- [[实体/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]] — Cross-reference
- [[实体/k8s-deployment-create.md|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[实体/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[实体/k8s-production-operations.md|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[实体/k8s-ai-infra-domain-guide.md|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[实体/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[实体/platform-engineering-terms.md|K8s 平台工程术语参考]] — Cross-reference
- [[实体/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[概念/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[技能/节点/gpu/诊断排障/ts-ai-ml-workloads.md|AI/ML 工作负载排查]] — Cross-reference
- [[技能/工作负载/deployment/deployment-canary-and-bluegreen.md|金丝雀与蓝绿发布]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]
- [[生态参考/领域索引/helm-index.md|Helm 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
