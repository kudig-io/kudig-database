---
title: 命名空间规划策略
description: 'Kubernetes 多团队命名空间隔离模型、Namespace-as-a-Service 与自动化生命周期管理'
summary: 'Kubernetes 多团队命名空间隔离模型、Namespace-as-a-Service 与自动化生命周期管理'
category: production-operations
tags:
- governance
- namespace
- multi-tenancy
- resource-quota
- lifecycle
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 命名空间规划策略 是什么
- 如何设计 Kubernetes 命名空间
trigger_keywords:
- namespace
- multi-tenancy
- resource-quota
- limit-range
- naas
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 命名空间规划策略

## 1. 概述

命名空间（Namespace）是 Kubernetes 资源隔离的基本单元。合理的命名空间策略直接影响多团队协作效率、资源利用率和安全边界。本文定义命名空间的规划模型、生命周期管理和自动化策略。

核心原则：
- **团队自治**：每个团队拥有独立的命名空间边界
- **环境隔离**：dev/staging/production 严格分离
- **资源可控**：通过 Quota/LimitRange 精确控制资源分配
- **生命周期自动化**：命名空间的创建、变更、回收全流程自动化

## 2. 多团队隔离模型

### 2.1 模型一：Namespace-per-Team

每个团队拥有固定的命名空间集合：

```
platform/
├── team-auth/          # 认证团队
├── team-payment/       # 支付团队
├── team-order/         # 订单团队
└── team-infra/         # 基础设施团队
```

适用场景：团队规模 < 20 人，服务数量 < 50。

```yaml
# 命名空间定义
apiVersion: v1
kind: Namespace
metadata:
  name: team-auth
  labels:
    kubernetes.io/metadata.name: team-auth
    platform.kubernetes.io/team: auth
    platform.kubernetes.io/tier: critical
  annotations:
    platform.kubernetes.io/team-lead: "zhangsan@example.com"
    platform.kubernetes.io/cost-center: "CC-AUTH-001"
```

### 2.2 模型二：Namespace-per-Team-per-Environment

每个团队在每个环境拥有独立命名空间：

```
dev/
├── auth-dev/
├── payment-dev/
└── order-dev/
staging/
├── auth-staging/
├── payment-staging/
└── order-staging/
production/
├── auth-prod/
├── payment-prod/
└── order-prod/
```

适用场景：团队需要独立的环境生命周期，CI/CD 流水线需要隔离环境。

### 2.3 模型三：Namespace-per-Service

每个微服务拥有独立命名空间：

```
svc-auth/
svc-auth-dev/
svc-auth-staging/
svc-payment/
svc-payment-dev/
svc-payment-staging/
```

适用场景：微服务数量 > 100，服务间需要强隔离（独立 RBAC、NetworkPolicy）。

### 2.4 模型选型

| 维度 | Team-per-NS | Team-env-per-NS | Service-per-NS |
|------|-------------|-----------------|----------------|
| 隔离强度 | 中 | 高 | 最高 |
| 运维复杂度 | 低 | 中 | 高 |
| RBAC 粒度 | 团队级 | 团队+环境级 | 服务级 |
| 资源利用率 | 高 | 中 | 低 |
| 适用规模 | < 50 服务 | 50-200 服务 | > 200 服务 |

## 3. Namespace-as-a-Service 实现

### 3.1 NaaS 控制器

Namespace-as-a-Service（NaaS）通过 CRD + Controller 实现自助式命名空间申请：

```yaml
# CRD 定义
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: namespacerequests.platform.io
spec:
  group: platform.io
  versions:
    - name: v1alpha1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                team:
                  type: string
                environment:
                  type: string
                  enum: [dev, staging, production]
                purpose:
                  type: string
                resourceTier:
                  type: string
                  enum: [small, medium, large, xlarge]
                ttlDays:
                  type: integer
                  default: 90
              required: [team, environment, resourceTier]
  scope: Cluster
  names:
    plural: namespacerequests
    singular: namespacerequest
    kind: NamespaceRequest
    shortNames: [nsr]
```

### 3.2 资源配额模板

```yaml
# 资源配额按 tier 分配
quota-templates:
  small:
    requests.cpu: "4"
    requests.memory: "8Gi"
    limits.cpu: "8"
    limits.memory: "16Gi"
    pods: "20"
    services: "10"
    persistentvolumeclaims: "5"
    
  medium:
    requests.cpu: "16"
    requests.memory: "32Gi"
    limits.cpu: "32"
    limits.memory: "64Gi"
    pods: "100"
    services: "30"
    persistentvolumeclaims: "20"
    
  large:
    requests.cpu: "64"
    requests.memory: "128Gi"
    limits.cpu: "128"
    limits.memory: "256Gi"
    pods: "500"
    services: "100"
    persistentvolumeclaims: "50"
    
  xlarge:
    requests.cpu: "256"
    requests.memory: "512Gi"
    limits.cpu: "512"
    limits.memory: "1Ti"
    pods: "2000"
    services: "300"
    persistentvolumeclaims: "100"
```

### 3.3 Controller 逻辑

```go
func (r *NamespaceRequestReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    nsr := &platformv1alpha1.NamespaceRequest{}
    if err := r.Get(ctx, req.NamespacedName, nsr); err != nil {
        return ctrl.Result{}, client.IgnoreNotFound(err)
    }

    // 1. 创建 Namespace
    ns := &corev1.Namespace{
        ObjectMeta: metav1.ObjectMeta{
            Name: fmt.Sprintf("%s-%s", nsr.Spec.Team, nsr.Spec.Environment),
            Labels: map[string]string{
                "platform.io/team":        nsr.Spec.Team,
                "platform.io/environment": nsr.Spec.Environment,
                "platform.io/managed-by":  "naas-controller",
            },
            Annotations: map[string]string{
                "platform.io/created-at": time.Now().Format(time.RFC3339),
                "platform.io/ttl-days":   strconv.Itoa(nsr.Spec.TTLDays),
            },
        },
    }
    if err := r.Create(ctx, ns); err != nil && !apierrors.IsAlreadyExists(err) {
        return ctrl.Result{}, err
    }

    // 2. 创建 ResourceQuota
    quota := r.buildQuota(nsr)
    if err := r.Create(ctx, quota); err != nil && !apierrors.IsAlreadyExists(err) {
        return ctrl.Result{}, err
    }

    // 3. 创建 LimitRange
    limitRange := r.buildLimitRange(nsr)
    if err := r.Create(ctx, limitRange); err != nil && !apierrors.IsAlreadyExists(err) {
        return ctrl.Result{}, err
    }

    // 4. 创建 RBAC
    if err := r.ensureRBAC(ctx, nsr); err != nil {
        return ctrl.Result{}, err
    }

    // 5. 更新状态
    nsr.Status.Phase = "Active"
    nsr.Status.NamespaceName = ns.Name
    return ctrl.Result{}, r.Status().Update(ctx, nsr)
}
```

## 4. ResourceQuota/LimitRange 配套策略

### 4.1 ResourceQuota

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-auth-quota
  namespace: team-auth
spec:
  hard:
    requests.cpu: "16"
    requests.memory: "32Gi"
    limits.cpu: "32"
    limits.memory: "64Gi"
    pods: "100"
    services: "30"
    services.nodeports: "2"
    services.loadbalancers: "1"
    persistentvolumeclaims: "20"
    requests.storage: "200Gi"
    count/deployments.apps: "30"
    count/statefulsets.apps: "5"
    count/jobs.batch: "10"
  scopeSelector:
    matchExpressions:
      - operator: In
        scopeName: PriorityClass
        values: ["low", "normal"]
```

### 4.2 LimitRange

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: team-auth-limits
  namespace: team-auth
spec:
  limits:
    - type: Container
      default:
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:
        cpu: "100m"
        memory: "128Mi"
      max:
        cpu: "4"
        memory: "8Gi"
      min:
        cpu: "50m"
        memory: "64Mi"
      maxLimitRequestRatio:
        cpu: "10"
        memory: "4"
    - type: Pod
      max:
        cpu: "8"
        memory: "16Gi"
    - type: PersistentVolumeClaim
      max:
        storage: "100Gi"
      min:
        storage: "1Gi"
```

### 4.3 配额使用率监控

```yaml
# Prometheus 告警规则
groups:
  - name: namespace-quota
    rules:
      - alert: NamespaceQuotaHigh
        expr: |
          (
            kube_resourcequota{type="used"} 
            / kube_resourcequota{type="hard"}
          ) > 0.85
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Namespace {{ $labels.namespace }} 配额使用率 > 85%"
          
      - alert: NamespaceQuotaExhausted
        expr: |
          kube_resourcequota{type="used"} 
          >= kube_resourcequota{type="hard"}
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Namespace {{ $labels.namespace }} 配额已耗尽"
```

## 5. 自动化清理策略

### 5.1 TTL 自动清理

```yaml
# CronJob 定期清理过期 Namespace
apiVersion: batch/v1
kind: CronJob
metadata:
  name: namespace-cleanup
  namespace: platform-system
spec:
  schedule: "0 2 * * *"    # 每天凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: namespace-cleanup-sa
          containers:
            - name: cleanup
              image: namespace-cleanup:latest
              command:
                - /bin/sh
                - -c
                - |
                  # 查找超过 TTL 的 Namespace
                  for ns in $(kubectl get ns -l platform.io/managed-by=naas-controller -o json | \
                    jq -r '.items[] | select(
                      (.metadata.annotations["platform.io/ttl-days"] // "90") | tonumber
                    ) as $ttl |
                    select(
                      (now - (.metadata.annotations["platform.io/created-at"] | fromdate)) > ($ttl * 86400)
                    ) | .metadata.name'); do
                    
                    echo "清理过期 Namespace: ${ns}"
                    
                    # 先标记为 Terminating，给 24h 缓冲
                    kubectl annotate ns "${ns}" platform.io/scheduled-deletion="$(date -u -d '+24 hours' +%Y-%m-%dT%H:%M:%SZ)"
                    
                    # 发送通知
                    curl -X POST "${WEBHOOK_URL}" -d "{\"text\": \"Namespace ${ns} 将在 24h 后被自动删除\"}"
                  done
          restartPolicy: OnFailure
```

### 5.2 空闲 Namespace 检测

```yaml
# 检测 30 天无活跃 Pod 的 Namespace
- alert: NamespaceIdle
  expr: |
    count by (namespace) (
      kube_pod_info{namespace=~"team-.*"}
    ) == 0
  for: 720h    # 30 天
  labels:
    severity: info
  annotations:
    summary: "Namespace {{ $labels.namespace }} 已 30 天无活跃 Pod"
```

### 5.3 清理审批流程

```yaml
# 清理策略分级
cleanup-policies:
  dev:
    auto-delete: true
    ttl-days: 30
    notify-before-days: 7
    
  staging:
    auto-delete: true
    ttl-days: 90
    notify-before-days: 14
    
  production:
    auto-delete: false
    require-approval: true
    approval-teams: ["platform-leads", "security"]
```

## 6. 最佳实践

### 6.1 命名规范

```
格式: {team}-{environment}
示例:
  - auth-dev
  - payment-staging
  - order-prod
  - infra-shared
```

### 6.2 标签必备项

```yaml
labels:
  platform.io/team: <team-name>
  platform.io/environment: <env>
  platform.io/tier: <critical|high|medium|low>
  platform.io/managed-by: naas-controller
```

### 6.3 系统命名空间保护

```yaml
# 不允许修改的系统命名空间
protected-namespaces:
  - kube-system
  - kube-public
  - kube-node-lease
  - platform-system
  - cert-manager
  - ingress-nginx
```

## Related

- [[02-label-convention-governance|标签/注解规范治理]]
- [[03-admission-policy-governance|准入策略治理]]

## See Also

- [Kubernetes Namespace 文档](https://kubernetes.io/docs/concepts/overview/working-with-objects/namespaces/)
- [Hierarchical Namespace Controller](https://github.com/kubernetes-sigs/hierarchical-namespaces)


<!-- risk-assessed -->
