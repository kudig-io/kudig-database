---
title: GitOps 漂移检测与自愈
description: 配置漂移检测、自愈机制与人工变更管理
summary: ArgoCD/Flux 漂移检测配置、自愈策略、告警通知及紧急手动变更处理流程
category: manifests-patterns
tags:
- k8s
- manifests
- gitops
- drift-detection
- self-heal
- argocd
- flux
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
estimated_read_time: 10min
intent_queries:
- GitOps 漂移检测
- ArgoCD selfHeal 配置
- Flux 配置漂移
trigger_keywords:
- drift
- self-heal
- diff
- configuration-drift
prerequisites:
- argocd-basics
- flux-basics
authors:
- name: KUDIG Team
  role: contributor
---

# GitOps 漂移检测与自愈

## 1. 什么是配置漂移

配置漂移指集群中实际运行的状态与 Git 仓库中声明的期望状态不一致。常见原因：

- 紧急 hotfix 直接 `kubectl edit`
- HPA 或 VPA 自动修改副本数/资源
- Mutating Webhook 注入字段
- 人为误操作

## 2. ArgoCD 漂移检测

### 2.1 自动同步与自愈

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  syncPolicy:
    automated:
      prune: true               # 自动删除多余资源
      selfHeal: true            # 自动纠正漂移
    syncOptions:
      - RespectIgnoreDifferences=true
```

### 2.2 忽略特定字段的漂移

某些字段由其他控制器管理，不应视为漂移：

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  ignoreDifferences:
    - group: apps
      kind: Deployment
      jsonPointers:
        - /spec/replicas       # HPA 管理的副本数
    - group: ""
      kind: Service
      jsonPointers:
        - /spec/clusterIP      # 自动分配的 ClusterIP
    - group: ""
      kind: ConfigMap
      jsonPointers:
        - /data/last-run       # 运行时动态更新
    - group: apps
      kind: Deployment
      jqPathExpressions:
        - '.spec.template.spec.containers[].resources'  # VPA 管理的资源
```

### 2.3 集群级忽略差异

```yaml
# argocd-cm ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cm
  namespace: argocd
data:
  resource.customizations.ignoreDifferences.apps_Deployment: |
    jsonPointers:
      - /spec/replicas
```

## 3. Flux 漂移管理

Flux 默认在每次 Reconciliation 时应用 Git 状态，但使用 `prune: true` 确保删除已移除的资源：

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: my-app
  namespace: flux-system
spec:
  interval: 10m
  path: ./apps/production
  prune: true                   # 自动清理多余资源
  wait: true
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      namespace: my-app
```

## 4. 漂移告警

### 4.1 ArgoCD Notifications

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-notifications-cm
  namespace: argocd
data:
  trigger.on-deployed: |
    - when: app.status.operationState.phase in ['Succeeded']
      send: [slack-deployed]
  trigger.on-health-degraded: |
    - when: app.status.health.status == 'Degraded'
      send: [slack-health-degraded]
  service.slack: |
    token: $slack-token
    username: ArgoCD
  template.slack-deployed: |
    message: |
      ✅ {{.app.metadata.name}} 已成功同步
      状态: {{.app.status.sync.status}}
```

### 4.2 Flux Alert Provider

```yaml
apiVersion: notification.toolkit.fluxcd.io/v1beta3
kind: Provider
metadata:
  name: slack
  namespace: flux-system
spec:
  type: slack
  channel: flux-alerts
  address: https://hooks.slack.com/services/xxx
---
apiVersion: notification.toolkit.fluxcd.io/v1beta3
kind: Alert
metadata:
  name: flux-alerts
  namespace: flux-system
spec:
  providerRef:
    name: slack
  eventSources:
    - kind: Kustomization
      name: "*"
    - kind: HelmRelease
      name: "*"
  eventSeverity: error         # 只告警错误级别
```

## 5. kube-diff 持续监控

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: drift-detector
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: drift-detector
  template:
    spec:
      containers:
        - name: detector
          image: registry.example.com/drift-detector:v1.0
          args:
            - --interval=5m
            - --alert-webhook=https://hooks.slack.com/xxx
            - --ignore-fields=spec.replicas,metadata.annotations.checksum
```

## 6. 紧急手动变更处理

> ⚠️ **紧急变更流程**：当必须手动修改时，遵循以下步骤避免漂移冲突

```bash
# 🟡 中风险：紧急手动变更
# 1. 先在 Git 中记录变更（创建 PR）
# 2. 手动 kubectl apply 临时修改
kubectl scale deployment my-app --replicas=5 -n production

# 3. 尽快合并 Git PR
# 4. ArgoCD selfHeal 会同步到最终状态

# 如果需要暂停自愈
kubectl patch application my-app -n argocd \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/syncPolicy/automated/selfHeal","value":false}]'
```

## 7. 生产实践

| 实践 | 说明 |
|------|------|
| 生产环境启用 selfHeal | 自动纠正漂移 |
| 合理使用 ignoreDifferences | 忽略 HPA/VPA 管理的字段 |
| 告警优先 | 漂移发生时立即通知 |
| 文档化紧急流程 | 团队知道如何处理漂移告警 |
| 定期审计 | 检查是否有未纳入 GitOps 的资源 |

## Related

- [[清单模式/04-gitops-patterns/01-argocd-app-of-apps|App-of-Apps 模式]]
- [[清单模式/07-resilience-patterns/02-hpa-advanced-patterns|HPA 高级模式]]

## See Also

- [ArgoCD 漂移检测](https://argo-cd.readthedocs.io/en/stable/user-guide/diffing/)
- [Flux Notifications](https://fluxcd.io/flux/components/notification/)

<!-- risk-assessed -->
