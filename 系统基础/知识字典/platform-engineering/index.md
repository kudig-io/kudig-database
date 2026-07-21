---
title: 平台工程知识词典
description: 涵盖 Kubernetes 平台工程全领域的完整术语体系，包括 Operator、CRD、GitOps、IDP、多集群管理、API 扩展等
summary: 平台工程领域词典，覆盖 Operator、CRD、Crossplane、Backstage、Karmada、GitOps、Dapr 等核心概念
category: dictionary
tags:
- dictionary
- platform-engineering
- operator
- gitops
- idp
- multi-cluster
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
audience:
- 平台工程师
- SRE
- 架构师
---

# 平台工程知识词典（Platform Engineering）

> 本词典覆盖 Kubernetes 平台工程领域的核心术语、技术组件及工程实践，是平台工程师构建内部开发者平台（IDP）和云原生基础设施的权威参考。

## 领域概述

平台工程是构建内部开发者平台（IDP）的学科，目标是：

- **降低认知负载**：开发者无需了解底层基础设施细节
- **自助服务**：开发者自主获取资源、部署应用
- **标准化**：统一的技术栈、最佳实践、安全基线
- **可扩展性**：通过 Operator/CRD 扩展 K8s API

## 核心术语定义

### K8s API 扩展

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Custom Resource (CR) | 自定义 K8s API 对象 | 声明式、版本化 |
| Custom Resource Definition (CRD) | 定义 CR 的 Schema | OpenAPI v3 验证 |
| Operator Pattern | CR + Controller 的自动化运维模式 | 领域知识编码 |
| Operator Framework | 构建 Operator 的 SDK/工具链 | Operator SDK |
| API Aggregation | 扩展 K8s API Server | APIService |
| Admission Webhook | 准入控制扩展 | Mutating/Validating |
| API Group/Version | API 分组与版本管理 | core/apps/v1 |
| API Priority and Fairness | API 请求优先级与公平性 | 防止 API Server 过载 |

### 内部开发者平台 (IDP)

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Backstage | Spotify 开源开发者门户 | 服务目录、模板、文档 |
| Developer Portal | 开发者自助服务门户 | Backstage/Port |
| Platform Metrics | 平台效能度量 | DORA/SPACE 指标 |
| KubeVela | 应用交付平台，OAM 实现 | 应用抽象、工作流 |
| KusionStack | 蚂蚁开源应用配置管理 | 代码化配置 |
| Score | 工作负载规范标准 | 平台无关的工作负载定义 |

### GitOps 与持续交付

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| GitOps | Git 作为单一事实来源的运维模式 | ArgoCD/Flux |
| OpenGitOps | GitOps 原则标准 | CNCF 工作组 |
| Infrastructure as Code | 基础设施代码化管理 | Terraform/Pulumi |
| CloudEvents | 事件数据格式标准 | CNCF 标准 |

### 多集群与联邦

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Karmada | CNCF 多集群编排引擎 | 策略分发、流量调度 |
| Open Cluster Management | 轻量级多集群管理 | Red Hat 主导 |
| Cluster API | 声明式集群生命周期管理 | CAPI |
| KubeStellar | IBM 多集群配置管理 | 边缘场景 |
| KCP | K8s 控制平面即服务 | 多租户控制平面 |
| Rancher | 企业级 K8s 管理平台 | SUSE |
| Cozystack | 托管 K8s 平台 | 开箱即用 |

### 应用运行时与编排

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Dapr | 分布式应用运行时 | Sidecar 模式、构建块 |
| Crossplane | 云基础设施控制平面 | 云资源 CRD 化 |
| Armada | 多集群批处理调度 | 大规模 Job 调度 |
| Cadence | 工作流编排引擎 | Uber 开源 |
| NATS | 云原生消息系统 | 轻量、高性能 |
| gRPC | 高性能 RPC 框架 | HTTP/2、Protobuf |

### 边缘与虚拟化

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| KubeEdge | 云边协同框架 | 华为开源 |
| KubeVirt | K8s 上运行 VM | 虚拟化工作负载 |
| WebAssembly Workloads | Wasm 工作负载支持 | SpinKube/wasmCloud |

### 设备与资源扩展

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Device Plugin | 设备资源扩展机制 | GPU/FPGA 等硬件 |
| DRA (Dynamic Resource Allocation) | 动态资源分配 | K8s 1.30+ Beta |
| Network Plugin | 网络插件扩展 | CNI |

## 技术组件索引

### API 扩展类

- [[系统基础/知识字典/platform-engineering/custom-resource.md|Custom Resource]]
- [[系统基础/知识字典/platform-engineering/custom-resources.md|Custom Resources（综合）]]
- [[系统基础/知识字典/platform-engineering/extending-the-kubernetes-api.md|扩展 K8s API]]
- [[系统基础/知识字典/platform-engineering/operator-pattern.md|Operator 模式]]
- [[系统基础/知识字典/platform-engineering/operator-framework.md|Operator Framework]]
- [[系统基础/知识字典/platform-engineering/api-group.md|API Group]]
- [[系统基础/知识字典/platform-engineering/api-version.md|API Version]]
- [[系统基础/知识字典/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性]]
- [[系统基础/知识字典/platform-engineering/kubernetes-api-aggregation-layer.md|API 聚合层]]
- [[系统基础/知识字典/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[系统基础/知识字典/platform-engineering/server-side-apply.md|Server-Side Apply]]
- [[系统基础/知识字典/platform-engineering/compatibility-version-for-control-plane.md|控制平面版本兼容]]
- [[系统基础/知识字典/platform-engineering/coordinated-leader-election.md|协调领导者选举]]

### IDP 与开发者体验类

- [[系统基础/知识字典/platform-engineering/backstage.md|Backstage（开发者门户）]]
- [[系统基础/知识字典/platform-engineering/developer-portal-and-platform-metrics.md|开发者门户与平台指标]]
- [[系统基础/知识字典/platform-engineering/kubevela.md|KubeVela（应用交付）]]
- [[系统基础/知识字典/platform-engineering/kusionstack.md|KusionStack（配置管理）]]
- [[系统基础/知识字典/platform-engineering/score.md|Score（工作负载规范）]]
- [[系统基础/知识字典/platform-engineering/openchoreo.md|OpenChoreo]]
- [[系统基础/知识字典/platform-engineering/openfeature.md|OpenFeature（特性开关）]]

### GitOps 与 IaC 类

- [[系统基础/知识字典/platform-engineering/gitops-and-continuous-delivery.md|GitOps 与持续交付]]
- [[系统基础/知识字典/platform-engineering/opengitops.md|OpenGitOps]]
- [[系统基础/知识字典/platform-engineering/infrastructure-as-code-for-kubernetes.md|K8s IaC]]
- [[系统基础/知识字典/platform-engineering/crossplane.md|Crossplane]]
- [[系统基础/知识字典/platform-engineering/cloudevents.md|CloudEvents]]

### 多集群管理类

- [[系统基础/知识字典/platform-engineering/karmada.md|Karmada]]
- [[系统基础/知识字典/platform-engineering/open-cluster-management.md|OCM]]
- [[系统基础/知识字典/platform-engineering/cluster-api-and-fleet-management.md|Cluster API 与集群编队]]
- [[系统基础/知识字典/platform-engineering/kubestellar.md|KubeStellar]]
- [[系统基础/知识字典/platform-engineering/kcp.md|KCP]]
- [[系统基础/知识字典/platform-engineering/rancher.md|Rancher]]
- [[系统基础/知识字典/platform-engineering/cozystack.md|Cozystack]]

### 应用运行时类

- [[系统基础/知识字典/platform-engineering/dapr.md|Dapr（分布式应用运行时）]]
- [[系统基础/知识字典/platform-engineering/armada.md|Armada（批处理调度）]]
- [[系统基础/知识字典/platform-engineering/cadence.md|Cadence（工作流）]]
- [[系统基础/知识字典/platform-engineering/nats.md|NATS（消息系统）]]
- [[系统基础/知识字典/platform-engineering/grpc.md|gRPC]]

### 边缘与虚拟化类

- [[系统基础/知识字典/platform-engineering/kubeedge.md|KubeEdge]]
- [[系统基础/知识字典/platform-engineering/kubevirt-virtual-machines.md|KubeVirt VM]]
- [[系统基础/知识字典/platform-engineering/webassembly-wasm-workloads.md|Wasm 工作负载]]

### 设备与资源扩展类

- [[系统基础/知识字典/platform-engineering/device-plugins.md|Device Plugins]]
- [[系统基础/知识字典/platform-engineering/dynamic-resource-allocation-good-practices.md|DRA 最佳实践]]
- [[系统基础/知识字典/platform-engineering/network-plugins.md|Network Plugins]]
- [[系统基础/知识字典/platform-engineering/compute-storage-and-networking-extensions.md|计算存储网络扩展]]
- [[系统基础/知识字典/platform-engineering/proxies-in-kubernetes.md|K8s 代理]]
- [[系统基础/知识字典/platform-engineering/manifest.md|Manifest]]
- [[系统基础/知识字典/platform-engineering/kind.md|Kind（本地集群）]]

## 平台工程成熟度模型

```
Level 0: 临时脚本
  └─ 手动 kubectl、Shell 脚本

Level 1: 基础设施即代码
  └─ Terraform/Helm 管理基础设施

Level 2: GitOps
  └─ ArgoCD/Flux 声明式交付

Level 3: 自助服务平台
  └─ Backstage + 服务目录 + 模板

Level 4: 金色路径 (Golden Path)
  └─ 标准化模板 + 自动化 + 策略强制

Level 5: 智能平台
  └─ AI 驱动优化 + 自愈 + 成本优化
```

## 生产最佳实践

### Operator 开发

1. **CRD 设计**：遵循 K8s API 规范，使用 kubebuilder 生成
2. **Reconcile 幂等**：多次执行结果一致
3. **状态管理**：Status 子资源记录实际状态
4. **Finalizer**：清理外部资源后再删除 CR
5. **Webhook**：Validating 防止非法配置，Mutating 注入默认值

### GitOps 实践

1. **单一事实来源**：所有配置存 Git，禁止手动 kubectl apply
2. **环境分离**：base + overlays (dev/staging/prod)
3. **自动同步**：ArgoCD auto-sync + self-heal
4. **渐进式交付**：Argo Rollouts 金丝雀/蓝绿

### 多集群管理

1. **Hub-Spoke 架构**：中心集群管理，成员集群独立运行
2. **策略分发**：Karmada/OCM 统一策略下发
3. **版本一致性**：成员集群版本差异 ≤ 2 个小版本
4. **灾难恢复**：成员集群可脱离联邦独立运行

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| CRD 创建失败 | Schema 验证错误/名称冲突 | 检查 CRD YAML、`kubectl explain` |
| Operator 未 Reconcile | Controller 崩溃/Watch 配置错误 | 检查 Operator Pod 日志 |
| Webhook 拒绝请求 | 策略检查不通过 | 检查 Webhook 日志、调整策略 |
| ArgoCD 同步失败 | 清单错误/权限不足 | 检查 ArgoCD UI、RBAC |
| 多集群同步延迟 | 网络问题/控制器过载 | 检查集群连接、控制器资源 |

## 学习路径

```
基础: CRD/CR → Operator 模式 → Kubebuilder
进阶: GitOps (ArgoCD) → Crossplane → Backstage
高级: 多集群 (Karmada) → 自定义 API Server → Dapr
专家: 平台架构设计 → 金色路径 → 平台效能度量
```

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/
- https://operatorframework.io/
- https://backstage.io/
- https://karmada.io/
- https://www.crossplane.io/
- https://dapr.io/
- https://kubevela.io/

## Related

- [[系统基础/知识字典/multi-cloud/federation.md|集群联邦]]
- [[系统基础/知识字典/operations/argo.md|ArgoCD]]
- [[系统基础/知识字典/configuration/server-side-apply.md|Server-Side Apply]]
- [[系统基础/知识字典/specialized-workloads/kubevirt.md|KubeVirt]]

## 深度技术解析

### Operator 开发示例

```go
// Reconcile 函数核心逻辑
func (r *MyAppReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 1. 获取 CR
    var myapp myappv1alpha1.MyApp
    if err := r.Get(ctx, req.NamespacedName, &myapp); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // 2. 处理 Finalizer
    if myapp.DeletionTimestamp.IsZero() {
        // 添加 Finalizer
        if !controllerutil.ContainsFinalizer(&myapp, finalizerName) {
            controllerutil.AddFinalizer(&myapp, finalizerName)
            return ctrl.Result{}, r.Update(ctx, &myapp)
        }
    } else {
        // 清理外部资源
        if controllerutil.ContainsFinalizer(&myapp, finalizerName) {
            if err := r.cleanupExternalResources(ctx, &myapp); err != nil {
                return ctrl.Result{}, err
            }
            controllerutil.RemoveFinalizer(&myapp, finalizerName)
            return ctrl.Result{}, r.Update(ctx, &myapp)
        }
        return ctrl.Result{}, nil
    }

    // 3. 确保 Deployment 存在
    desired := r.buildDeployment(&myapp)
    if err := r.createOrUpdate(ctx, &myapp, desired); err != nil {
        return ctrl.Result{}, err
    }

    // 4. 更新 Status
    myapp.Status.ReadyReplicas = desired.Status.ReadyReplicas
    myapp.Status.Phase = "Running"
    return ctrl.Result{}, r.Status().Update(ctx, &myapp)
}
```

### GitOps 工作流

```
GitOps 完整流程:

开发者 Push 代码
    │
    ▼
Git Repo (单一事实来源)
├── base/           # 基础配置
├── overlays/
│   ├── dev/        # 开发环境
│   ├── staging/    # 预发环境
│   └── prod/       # 生产环境
    │
    ▼
ArgoCD (持续监控 Git)
    │
    ├── 检测差异 (Git vs Cluster)
    ├── 自动/手动同步
    └── Self-Heal (漂移修复)
    │
    ▼
K8s Cluster (实际状态)
    │
    ▼
监控/告警 (Prometheus/Grafana)
```

### Crossplane 云资源管理

```yaml
# Crossplane Composition: 定义云数据库抽象
apiVersion: apiextensions.crossplane.io/v1
kind: Composition
metadata:
  name: compositepostgresqlinstance
spec:
  compositeTypeRef:
    apiVersion: database.example.org/v1alpha1
    kind: CompositePostgreSQLInstance
  resources:
  - name: rdsinstance
    base:
      apiVersion: rds.aws.crossplane.io/v1beta2
      kind: Instance
      spec:
        forProvider:
          region: us-east-1
          dbInstanceClass: db.t3.medium
          engine: postgres
    patches:
    - fromFieldPath: spec.parameters.storageGB
      toFieldPath: spec.forProvider.allocatedStorage
---
# 开发者只需创建这个简单资源
apiVersion: database.example.org/v1alpha1
kind: PostgreSQLInstance
metadata:
  name: my-db
spec:
  parameters:
    storageGB: 100
    version: "15"
```

## 生产案例研究

### 案例：从手动运维到 GitOps 平台

**背景：** 某公司 50+ 微服务，手动 kubectl 部署，配置漂移严重。

**转型方案：**
1. 基础设施: Terraform 管理云资源
2. 应用部署: ArgoCD + Helm/Kustomize
3. 开发者门户: Backstage 服务目录 + 模板
4. 策略强制: OPA/Gatekeeper 准入控制
5. 可观测性: OTel + Prometheus + Grafana

**关键成果：**
- 部署时间: 从 30min 降至 5min
- 配置漂移: 从每周 10+ 次降至 0（自动修复）
- 开发者自助: 80% 操作无需平台团队介入

## 常用运维命令速查

```bash
# === CRD/Operator ===
# 查看 CRD
kubectl get crd | grep myapp
# 查看 CR 实例
kubectl get myapps -A
# 查看 Operator 日志
kubectl logs -n operator-system -l app=myapp-operator
# 查看 CR 事件
kubectl describe myapp my-instance

# === ArgoCD ===
# 查看应用状态
argocd app list
argocd app get my-app
# 手动同步
argocd app sync my-app
# 查看同步差异
argocd app diff my-app

# === Crossplane ===
# 查看托管资源
kubectl get managed
# 查看 Provider 状态
kubectl get providerrevisions
# 查看 Composition
kubectl get compositions

# === Backstage ===
# 查看服务目录 (API)
curl -s http://backstage:7007/api/catalog/entities | jq

# === 多集群 ===
# Karmada 集群状态
kubectl --kubeconfig $KARMADA_CONFIG get clusters
# OCM 集群状态
kubectl get managedclusters
```

## 常见问题 FAQ

**Q1: Operator 和 Helm 怎么选？**

A: 
- Helm: 简单应用部署，参数化模板，无运行时管理
- Operator: 复杂有状态应用，需要持续调谐、自动修复、领域知识
判断标准：应用是否需要“运维专家”持续管理？是 → Operator，否 → Helm

**Q2: GitOps 如何处理 Secret？**

A: Secret 不能明文存 Git。方案：
1. Sealed Secrets: 加密后可安全存 Git
2. External Secrets Operator: 从 Vault/KMS 同步
3. SOPS: 文件级加密
4. Helm Secrets: 加密 values 文件

**Q3: 多集群管理选 Karmada 还是 OCM？**

A: 
- Karmada: 功能全面（调度、流量、策略），华为主导，CNCF 孵化
- OCM: 轻量级、插件化，Red Hat 主导，适合简单场景
新项目建议 Karmada（社区活跃、功能完整）。

**Q4: Backstage 值得投入吗？**

A: 取决于团队规模：
- <10 个服务: 不需要，文档即可
- 10-50 个服务: 可选，服务目录有价值
- >50 个服务: 强烈推荐，自助服务 + 模板 + 文档
注意：Backstage 本身需要维护成本，建议专人负责。

**Q5: Crossplane 和 Terraform 怎么选？**

A: 
- Terraform: 基础设施一次性创建，命令式，状态文件管理
- Crossplane: 持续调谐，声明式，K8s 原生，自动修复漂移
建议：基础设施用 Terraform，应用相关云资源用 Crossplane。

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| IDP | Internal Developer Platform | 内部开发者平台 |
| CRD | Custom Resource Definition | 自定义资源定义 |
| CR | Custom Resource | 自定义资源 |
| OAM | Open Application Model | 开放应用模型 |
| DORA | DevOps Research and Assessment | DevOps 效能指标 |
| APF | API Priority and Fairness | API 优先级与公平性 |
| CAPI | Cluster API | 集群 API |
| OCM | Open Cluster Management | 开放集群管理 |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| Operator SDK | v1.33+ | v1.34+ | v1.35+ | v1.36+ |
| ArgoCD | v2.9+ | v2.10+ | v2.11+ | v2.12+ |
| Crossplane | v1.14+ | v1.15+ | v1.16+ | v1.17+ |
| Karmada | v1.8+ | v1.9+ | v1.10+ | v1.11+ |
| Backstage | v1.20+ | v1.22+ | v1.24+ | v1.26+ |
| Dapr | v1.12+ | v1.13+ | v1.14+ | v1.15+ |

## 平台工程检查清单

| 检查项 | 说明 | 状态 |
|--------|------|------|
| GitOps 流程 | 所有配置存 Git，自动同步 | ☐ |
| 服务目录 | 所有服务在 Backstage 注册 | ☐ |
| 金色路径模板 | 标准化应用模板 | ☐ |
| 策略强制 | OPA/Gatekeeper 准入控制 | ☐ |
| 自助服务 | 开发者可自主创建资源 | ☐ |
| 可观测性集成 | 新服务自动接入监控 | ☐ |
| 成本可视化 | 按团队/服务成本分配 | ☐ |

