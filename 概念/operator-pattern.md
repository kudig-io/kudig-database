---
title: Operator Pattern (CRD + Controller)
description: Operator Pattern (CRD + Controller) — Kubernetes 生产运维知识库
summary: Operator Pattern (CRD + Controller) — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- operator
- crd
- webhook
- extension
- controller
- etcd
- apiserver
- prometheus
- istio
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Operator Pattern (CRD + Controller) 是什么
- 如何 Operator Pattern (CRD + Controller)
trigger_keywords:
- Operator
- Pattern
- CRD
- Controller
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- etcd-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Operator Pattern (CRD + Controller)

## Custom Resource Definition (CRD)

CRDs extend [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|the Kubernetes API]] with custom resource types without modifying API Server code:

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
spec:
  group: example.com
  names:
    kind: Database
    plural: databases
  scope: Namespaced
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:  # Validation schema
```

CRD features:
- **Schema validation**: OpenAPI v3 JSON schema validation
- **Subresources**: `/status` and `/scale` endpoints
- **Additional printer columns**: Custom `kubectl get` columns
- **Multiple versions**: With conversion webhooks for cross-version migration

## Operator Controller

An Operator is a custom controller that manages CRD instances:

1. **Watch** CRD changes via Informer
2. **Reconcile**: Compare desired spec vs actual cluster state
3. **Create/Update** dependent Kubernetes resources ([[Deployments|Deployments]], Services, PVCs, etc.)
4. **Update Status** on the CRD instance

Popular operators: [[Prometheus|Prometheus]] Operator, Elasticsearch Operator, MySQL Operator, [[ArgoCD|ArgoCD]].

## Admission Webhooks

Webhooks intercept API requests in two phases:

| Type | Phase | Purpose | Example |
|------|-------|---------|---------|
| **Mutating** | Before validation | Modify requests | Istio sidecar injection, default values |
| **Validating** | After validation | Reject non-compliant requests | OPA/Gatekeeper policies, Kyverno |

Webhooks run as external HTTPS services registered with API Server. They must respond within the configured timeout or requests are rejected (or ignored for `failurePolicy: Ignore`).

## API Aggregation

The API aggregation layer allows running independent API Servers alongside the main kube-apiserver. Examples include metrics-server and custom metrics adapter. Requests are proxied through the main API Server.

## 源码实现分析

### Kubebuilder Reconcile 核心流程

```go
// controllers/database_controller.go (Kubebuilder 生成)
func (r *DatabaseReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    // 1. 获取 CR 实例
    var db examplev1.Database
    if err := r.Get(ctx, req.NamespacedName, &db); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err) // 已删除，忽略
    }
    // 2. 处理 Finalizer（删除前清理外部资源）
    if !db.DeletionTimestamp.IsZero() {
        return r.cleanupExternalResources(ctx, &db)
    }
    // 3. 对账逻辑：期望状态 vs 实际状态
    statefulSet := r.buildStatefulSet(&db) // 构建期望的 StatefulSet
    existing := &appsv1.StatefulSet{}
    err := r.Get(ctx, types.NamespacedName{Name: db.Name, Namespace: db.Namespace}, existing)
    if errors.IsNotFound(err) {
        ctrl.SetControllerReference(&db, statefulSet, r.Scheme) // OwnerReference
        r.Create(ctx, statefulSet)
    } else if !equality.Semantic.DeepEqual(existing.Spec, statefulSet.Spec) {
        existing.Spec = statefulSet.Spec
        r.Update(ctx, existing) // 滚动更新
    }
    // 4. 更新 Status 子资源
    db.Status.Phase = "Running"
    db.Status.ReadyReplicas = existing.Status.ReadyReplicas
    r.Status().Update(ctx, &db)
    // 5. 设置重新对账间隔（用于检查外部状态）
    return ctrl.Result{RequeueAfter: 30 * time.Second}, nil
}
```

### Operator 架构全景

```
┌──────────────────────────────────────────────────────────┐
│                  Operator 架构全景                        │
├──────────────────────────────────────────────────────────┤
│  User: kubectl apply database.yaml                       │
│         │                                                │
│         ▼                                                │
│  ┌─────────────┐    Watch/Informer    ┌─────────────┐  │
│  │  API Server  │ ──────────────────▶ │  Operator    │  │
│  │  (etcd)      │ ◀────────────────── │  Controller  │  │
│  └─────────────┘    Create/Update     └──────┬──────┘  │
│         │                                   │          │
│         ▼                                   ▼          │
│  ┌─────────────┐                    ┌─────────────┐  │
│  │  Webhook     │                    │  Managed     │  │
│  │  (Admission) │                    │  Resources   │  │
│  │  验证/变更    │                    │  STS/SVC/PVC │  │
│  └─────────────┘                    └─────────────┘  │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：Kubebuilder 初始化 Operator 项目

```bash
# 🟢 低风险：本地开发操作
mkdir database-operator && cd database-operator
kubebuilder init --domain example.com --repo github.com/org/database-operator
kubebuilder create api --group example --version v1 --kind Database
# 生成文件结构：
# api/v1/database_types.go    — CRD Spec/Status 定义
# controllers/database_controller.go — Reconcile 逻辑
# config/crd/                 — CRD YAML
# config/rbac/                — RBAC 权限
make manifests  # 生成 CRD YAML
make install    # 🟡 安装 CRD 到集群
make deploy     # 🟡 部署 Operator 到集群
```

### 场景二：观察 Operator 对账行为

```bash
# 🟢 低风险：只读观察
kubectl get database my-db -o yaml          # 查看 CR spec + status
kubectl get events --field-selector involvedObject.name=my-db  # 对账事件
kubectl logs -l app.kubernetes.io/name=database-operator -f    # Operator 日志
# 观察级联删除（OwnerReference）
kubectl delete database my-db  # 🟡 触发 Finalizer 清理 + 级联删除子资源
kubectl get statefulset,svc,pvc -l app=my-db  # 确认子资源已清理
```

### 场景三：Webhook 验证与变更

```yaml
# 🟡 中风险：注册 Webhook 影响所有匹配资源的创建/更新
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: database-validating-webhook
webhooks:
- name: vdatabase.example.com
  rules:
  - apiGroups: ["example.com"]
    apiVersions: ["v1"]
    operations: ["CREATE", "UPDATE"]
    resources: ["databases"]
  clientConfig:
    service:
      name: database-operator-webhook
      namespace: operator-system
      path: /validate-example-com-v1-database
  failurePolicy: Fail  # Webhook 不可用时拒绝请求
  timeoutSeconds: 10
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | Reconcile 只会被触发一次 | Level-triggered：任何相关资源变化都会重新入队，必须幂等 |
| 2 | Operator 可以管理任何资源 | RBAC 限制 Operator 只能操作其 ServiceAccount 授权的资源 |
| 3 | Status 和 Spec 可以一起更新 | Status 是子资源，必须用 `r.Status().Update()` 单独更新 |
| 4 | Webhook 失败可以忽略 | `failurePolicy: Fail` 时 Webhook 不可用会阻断所有匹配请求 |
| 5 | 删除 CR 就会删除子资源 | 必须设置 OwnerReference 或 Finalizer，否则子资源成为孤儿 |
| 6 | 一个 Operator 只能管理一种 CR | 可以管理多种 CR，但建议单一职责，复杂场景拆分多个 Operator |

## 面试要点

1. **Q: Operator 模式的核心思想是什么？与普通 Controller 有何区别？**
   A: 核心思想是将人类运维经验编码为软件。普通 Controller（如 Deployment Controller）管理通用资源；Operator 管理有状态应用的领域逻辑（如 MySQL 主从切换、备份恢复、版本升级）。Operator = CRD（领域模型）+ Controller（对账逻辑）+ Webhook（准入控制）。

2. **Q: Reconcile 为什么必须是幂等的？如何实现？**
   A: 因为 Informer 可能重复投递事件（网络抨动、Leader 切换、Requeue）。幂等实现：① 先 Get 再判断是否存在，而非直接 Create；② 用 DeepEqual 比较 Spec 再决定是否 Update；③ 所有操作基于“当前状态 → 期望状态”的转换，而非“执行某个动作”。

3. **Q: CRD 的 Finalizer 机制如何工作？**
   A: Finalizer 是 CR 上的字符串标记。删除 CR 时：① API Server 设置 deletionTimestamp 但不真正删除；② Operator 检测到 deletionTimestamp，执行清理（删除外部数据库、释放云资源）；③ 清理完成后移除 Finalizer；④ API Server 真正删除对象。若 Operator 崩溃，CR 会卡在 Terminating 状态。

4. **Q: 生产环境 Operator 有哪些关键设计考量？**
   A: ① Leader Election（多副本部署时只有一个活跃 Reconcile）；② 指数退避重试（避免 API Server 过载）；③ Status Conditions（记录各阶段状态，方便 kubectl wait）；④ 资源限制（Operator Pod 设置 requests/limits）；⑤ 可观测性（导出 reconcile 延迟/错误率指标）；⑥ 版本升级（CRD 多版本 + Conversion Webhook）。

## Related
- [[概念/etcd × Operator 模式.md|etcd × Operator 模式]] — 综合
- [[概念/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合
- [[概念/CRD × 可观测性.md|CRD × 可观测性]] — 综合

- [[概念/Operator 模式 × 可观测性.md|Operator 模式 × 可观测性]]

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[技能/develop-crd-operator.md|develop-crd-operator]] — Develop CRD Operator
- [[实体/crd-custom-resources.md|crd-custom-resources]] — CRD (Custom Resource Definition)
- [[概念/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[概念/controller-pattern.md|Controller Pattern]]
- [[概念/declarative-api.md|Declarative API]]
- [[实体/crd-custom-resources.md|CRD Custom Resources]]
- Admission Webhooks
- [[技能/develop-crd-operator.md|Develop CRD Operator]]
- Wiki Digest — Daily (2026-05-21) — Cross-reference
- [[实体/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]] — Cross-reference
- [[实体/platform-engineering-terms.md|K8s 平台工程术语参考]] — Cross-reference
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[概念/声明式 API × 控制器模式.md|声明式 API × 控制器模式]] — Cross-reference
- [[概念/deployment-controller-architecture.md|Deployment 控制器架构]] — Cross-reference
- [[实体/kube-apiserver.md|kube-apiserver]] — Cross-reference
- [[实体/metal3-io.md|Metal3]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/helm-index.md|Helm 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
