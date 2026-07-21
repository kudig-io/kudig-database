---
title: Kubernetes Multi-Tenancy Architecture
description: K8s 多租户架构 — 软/硬隔离模型、Namespace 策略、vCluster 虚拟集群、多租户网络与安全
summary: 企业级 Kubernetes 多租户设计，涵盖隔离模型、资源治理、网络策略、成本分摊
category: practice
tags:
- multi-tenancy
- namespace
- vcluster
- isolation
- resource-governance
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: cluster
---
# Kubernetes 多租户架构

> 在共享集群中安全、高效地服务多个团队/业务单元。

## 多租户模型对比

| 模型 | 隔离级别 | 成本 | 复杂度 | 适用 |
|------|----------|------|--------|------|
| 命名空间隔离 | 软隔离 | 低 | 低 | 内部团队 |
| 节点池隔离 | 中隔离 | 中 | 中 | 安全要求较高 |
| vCluster 虚拟集群 | 硬隔离 | 中 | 中 | 多团队独立控制平面 |
| 独立集群 | 完全隔离 | 高 | 高 | 合规/强隔离 |

## 命名空间多租户

### 租户命名空间模板

```yaml
# 租户命名空间 + 资源配额 + 网络策略 + RBAC
apiVersion: v1
kind: Namespace
metadata:
  name: team-commerce
  labels:
    tenant: commerce
    environment: production
    cost-center: CC-1001
  annotations:
    scheduler.alpha.kubernetes.io/node-selector: "tenant=commerce"
---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-commerce-quota
  namespace: team-commerce
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
    secrets: "50"
    configmaps: "50"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: team-commerce
spec:
  limits:
    - default:
        cpu: "1"
        memory: 1Gi
      defaultRequest:
        cpu: 100m
        memory: 128Mi
      type: Container
---
# 默认拒绝跨租户流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-cross-tenant
  namespace: team-commerce
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              tenant: commerce
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: ingress-nginx
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              tenant: commerce
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

### 租户 RBAC

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-commerce-admin
  namespace: team-commerce
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: admin
subjects:
  - kind: Group
    name: oidc:team-commerce
    apiGroup: rbac.authorization.k8s.io
---
# 平台团队只读
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: platform-readonly
  namespace: team-commerce
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: view
subjects:
  - kind: Group
    name: oidc:platform-team
    apiGroup: rbac.authorization.k8s.io
```

## vCluster 虚拟集群

### 架构

```
┌─────────────────────────────────────────────┐
│           Host Cluster (物理集群)             │
│  ┌─────────────┐  ┌─────────────┐           │
│  │ vCluster A  │  │ vCluster B  │           │
│  │ (Team A)    │  │ (Team B)    │           │
│  │ ┌─────────┐ │  │ ┌─────────┐ │           │
│  │ │API Srvr │ │  │ │API Srvr │ │           │
│  │ │etcd     │ │  │ │etcd     │ │           │
│  │ │Scheduler│ │  │ │Scheduler│ │           │
│  │ └─────────┘ │  │ └─────────┘ │           │
│  └─────────────┘  └─────────────┘           │
│         │                  │                  │
│  ┌──────▼──────────────────▼──────────────┐  │
│  │        Shared Node Pool                 │  │
│  │  (Pods 实际运行在宿主集群节点上)         │  │
│  └─────────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

### 部署 vCluster

```bash
# 安装 vCluster CLI
curl -L -o vcluster "https://github.com/loft-sh/vcluster/releases/latest/download/vcluster-linux-amd64"

# 创建虚拟集群
vcluster create team-a \
  --namespace vcluster-team-a \
  --distro k8s \
  --set sync.nodes.enabled=true \
  --set sync.persistentvolumes.enabled=true \
  --set policies.resourceQuota.enabled=true \
  --set policies.limitRange.enabled=true

# 连接到虚拟集群
vcluster connect team-a --namespace vcluster-team-a
kubectl get nodes  # 看到虚拟节点
```

### vCluster 配置

```yaml
# vcluster.yaml
sync:
  nodes:
    enabled: true
  persistentvolumes:
    enabled: true
  ingresses:
    enabled: true
policies:
  resourceQuota:
    enabled: true
    quota:
      requests.cpu: "10"
      requests.memory: 20Gi
      limits.cpu: "20"
      limits.memory: 40Gi
      pods: "30"
  limitRange:
    enabled: true
    default:
      cpu: "1"
      memory: 1Gi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
  networkPolicy:
    enabled: true
```

## 节点池隔离

```yaml
# 专用节点池（通过污点/标签）
apiVersion: v1
kind: Node
metadata:
  labels:
    tenant: finance
    security-level: high
spec:
  taints:
    - key: tenant
      value: finance
      effect: NoSchedule
---
# 租户 Pod 容忍污点
apiVersion: apps/v1
kind: Deployment
metadata:
  name: finance-app
  namespace: team-finance
spec:
  template:
    spec:
      tolerations:
        - key: tenant
          value: finance
          effect: NoSchedule
      nodeSelector:
        tenant: finance
```

## 多租户成本分摊

```yaml
# Kubecost 按命名空间分摊
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubecost-allocation
data:
  allocation-config.yaml: |
    aggregation:
      - namespace
      - label:team
      - label:cost-center
    idle: true
    sharedCosts:
      shared-namespace:
        - kube-system
        - monitoring
      splitBy: namespace
```

## 多租户安全检查清单

- [ ] 每个租户命名空间有 ResourceQuota
- [ ] 每个租户命名空间有 LimitRange
- [ ] 默认拒绝跨租户 NetworkPolicy
- [ ] RBAC 按团队/角色绑定
- [ ] Pod Security Admission 设为 restricted
- [ ] 禁止租户创建 ClusterRole/ClusterRoleBinding
- [ ] 镜像拉取限制（允许列表）
- [ ] 审计日志按租户过滤
- [ ] 成本标签强制（cost-center/team）
- [ ] 定期权限审计（access review）

## 选型决策

| 需求 | 推荐方案 |
|------|----------|
| 内部 3-10 团队 | 命名空间 + RBAC + NetworkPolicy |
| 需要独立控制平面 | vCluster |
| 强合规/金融 | 独立集群或节点池隔离 |
| SaaS 多租户 | vCluster + 独立 etcd |
| 成本敏感 | 命名空间共享 + Kubecost 分摊 |

## 层级命名空间 (Hierarchical Namespaces)

### HNC 架构

```
┌────────────────────────────────────────────┐
│  Root Namespace: company                     │
│  ├── org: engineering                       │
│  │   ├── team: backend                      │
│  │   │   ├── env: backend-prod             │
│  │   │   └── env: backend-staging          │
│  │   └── team: frontend                     │
│  │       ├── env: frontend-prod            │
│  │       └── env: frontend-staging         │
│  └── org: data-science                      │
│      ├── team: ml-platform                  │
│      └── team: analytics                    │
└────────────────────────────────────────────┘
策略继承: company → engineering → backend → backend-prod
```

### HNC 部署与配置

```bash
# 🟢 只读：安装 Hierarchical Namespace Controller
kubectl apply -f https://github.com/kubernetes-sigs/hierarchical-namespaces/releases/latest/download/default.yaml

# 🟡 中风险：创建层级结构
kubectl hns create backend-prod --parent engineering
kubectl hns create backend-staging --parent engineering

# 🟢 只读：查看层级树
kubectl hns tree company
```

```yaml
# HierarchyConfiguration — 定义父子关系
apiVersion: hnc.x-k8s.io/v1alpha2
kind: HierarchyConfiguration
metadata:
  name: hierarchy
  namespace: backend-prod
spec:
  parent: engineering
---
# HNC 策略继承 — 父命名空间的 NetworkPolicy 自动传播到子命名空间
apiVersion: hnc.x-k8s.io/v1alpha2
kind: HNCConfiguration
metadata:
  name: config
spec:
  resources:
    - resource: networkpolicies.networking.k8s.io
      mode: Propagate
    - resource: resourcequotas
      mode: Propagate
    - resource: limitranges
      mode: Propagate
    - resource: roles.rbac.authorization.k8s.io
      mode: Propagate
    - resource: rolebindings.rbac.authorization.k8s.io
      mode: Propagate
```

## 租户自助服务与 Onboarding

### 自动化租户初始化 Pipeline

```yaml
# ArgoCD Application — 租户 Onboarding
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: tenant-onboarding-commerce
  namespace: argocd
spec:
  project: platform
  source:
    repoURL: https://github.com/org/platform-templates
    path: tenants/commerce
    targetRevision: main
  destination:
    server: https://kubernetes.default.svc
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
```

### 租户模板目录结构

```
tenants/
└── commerce/
    ├── namespace.yaml          # Namespace + labels
    ├── resource-quota.yaml     # ResourceQuota
    ├── limit-range.yaml        # LimitRange
    ├── network-policy.yaml     # 默认拒绝 + 允许规则
    ├── rbac.yaml               # RoleBinding (OIDC Group)
    ├── pod-security.yaml       # PSA restricted
    ├── storage-class.yaml      # 允许的 StorageClass
    └── monitoring.yaml         # ServiceMonitor + 告警规则
```

### 租户 Onboarding 检查单

```bash
#!/bin/bash
# 🟡 中风险：租户初始化验证脚本
TENANT=$1
NS="team-${TENANT}"

echo "=== 租户 Onboarding 验证: $TENANT ==="

# 1. 命名空间存在性
echo -n "[1/8] Namespace: "
kubectl get ns $NS &>/dev/null && echo "✅" || echo "❌ 缺失"

# 2. ResourceQuota
echo -n "[2/8] ResourceQuota: "
kubectl get resourcequota -n $NS --no-headers | grep -q . && echo "✅" || echo "❌"

# 3. LimitRange
echo -n "[3/8] LimitRange: "
kubectl get limitrange -n $NS --no-headers | grep -q . && echo "✅" || echo "❌"

# 4. NetworkPolicy
echo -n "[4/8] NetworkPolicy (default-deny): "
kubectl get networkpolicy default-deny-cross-tenant -n $NS &>/dev/null && echo "✅" || echo "❌"

# 5. RBAC
echo -n "[5/8] RoleBinding: "
kubectl get rolebinding -n $NS -o name | grep -q $TENANT && echo "✅" || echo "❌"

# 6. Pod Security Admission
echo -n "[6/8] PSA restricted: "
kubectl get ns $NS -o jsonpath='{.metadata.labels.pod-security\.kubernetes\.io/enforce}' | grep -q restricted && echo "✅" || echo "❌"

# 7. 成本标签
echo -n "[7/8] Cost Labels: "
kubectl get ns $NS -o jsonpath='{.metadata.labels.cost-center}' | grep -q . && echo "✅" || echo "❌"

# 8. 监控
echo -n "[8/8] ServiceMonitor: "
kubectl get servicemonitor -n $NS --no-headers 2>/dev/null | grep -q . && echo "✅" || echo "❌ (可选)"

echo ""
echo "=== 完成 ==="
```

## 多租户监控与审计

### 租户级指标隔离

```yaml
# Prometheus 多租户采集 — 按命名空间分片
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: tenant-metrics
  namespace: monitoring
spec:
  replicas: 2
  ruleSelector:
    matchLabels:
      role: tenant-alerts
  podMonitorNamespaceSelector:
    matchLabels:
      tenant-monitoring: enabled
  serviceMonitorNamespaceSelector:
    matchLabels:
      tenant-monitoring: enabled
  externalLabels:
    cluster: prod-cn-1
  remoteWrite:
    - url: http://thanos-receive.monitoring:19291/api/v1/receive
      writeRelabelConfigs:
        - sourceLabels: [namespace]
          regex: "team-(.*)"
          targetLabel: tenant
          replacement: "$1"
```

### 租户审计日志过滤

```yaml
# 审计策略 — 按租户记录关键操作
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 租户命名空间内的所有写操作
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: ""
        resources: ["pods", "services", "secrets", "configmaps"]
      - group: "apps"
        resources: ["deployments", "statefulsets"]
    namespaces: ["team-*"]
  # RBAC 变更 — 全集群
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: "rbac.authorization.k8s.io"
        resources: ["*"]
  # 只读操作 — 仅元数据
  - level: Metadata
    verbs: ["get", "list", "watch"]
    resources:
      - group: ""
        resources: ["*"]
  # 默认
  - level: None
    nonResourceURLs: ["/healthz*", "/version", "/openapi*"]
```

### 租户异常行为告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: tenant-anomaly-alerts
  namespace: monitoring
spec:
  groups:
    - name: tenant-anomalies
      rules:
        - alert: TenantQuotaNearLimit
          expr: |
            kube_resourcequota{type="used"} / kube_resourcequota{type="hard"} > 0.9
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "租户 {{ $labels.namespace }} 配额 {{ $labels.resource }} 已用 90%+"

        - alert: TenantPodExplosion
          expr: |
            increase(kube_pod_created[10m]) > 20
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "命名空间 {{ $labels.namespace }} 10分钟内创建 >20 Pod，可能异常"

        - alert: TenantExcessiveRBAC
          expr: |
            count by (namespace) (kube_rolebinding_info) > 15
          labels:
            severity: info
          annotations:
            summary: "命名空间 {{ $labels.namespace }} RoleBinding 过多，建议审计"
```

## 多租户安全加固

### Pod Security Admission 分层

```yaml
# 平台命名空间 — privileged（允许 DaemonSet、监控 Agent）
apiVersion: v1
kind: Namespace
metadata:
  name: monitoring
  labels:
    pod-security.kubernetes.io/enforce: privileged
    pod-security.kubernetes.io/audit: restricted
---
# 租户命名空间 — restricted（最严格）
apiVersion: v1
kind: Namespace
metadata:
  name: team-commerce
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### 镜像拉取限制（准入策略）

```yaml
# Kyverno — 限制租户镜像源
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-tenant-registries
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: validate-registry
      match:
        any:
          - resources:
              kinds: ["Pod"]
              namespaceSelector:
                matchLabels:
                  tenant: commerce
      validate:
        message: "只允许使用公司镜像仓库 registry.internal 或 Docker Hub 官方镜像"
        pattern:
          spec:
            containers:
              - image: "registry.internal/* | docker.io/library/*"
```

### 租户网络隔离进阶

```yaml
# DNS 策略 — 租户只能访问内部 DNS
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-dns-egress
  namespace: team-commerce
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
---
# 禁止租户访问云元数据服务 (SSRF 防护)
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-metadata-access
  namespace: team-commerce
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
            except:
              - 169.254.169.254/32  # AWS/阿里云元数据
              - 100.100.100.200/32  # 阿里云元数据
```

## 多租户故障排查

### 诊断命令集

```bash
# 🟢 只读：租户资源使用概览
kubectl top pods -n team-commerce --sort-by=cpu | head -20

# 🟢 只读：检查配额使用情况
kubectl describe resourcequota -n team-commerce

# 🟢 只读：网络策略验证
kubectl get networkpolicy -n team-commerce -o wide

# 🟢 只读：检查跨租户流量（需要 Cilium）
cilium policy trace --src-namespace team-commerce --dst-namespace team-finance

# 🟢 只读：审计日志查询（租户操作历史）
kubectl logs -n kube-system -l component=kube-apiserver --tail=1000 | \
  grep "team-commerce" | grep -E "(create|delete|update)"

# 🟢 只读：vCluster 状态检查
kubectl get pods -n vcluster-team-a
vcluster connect team-a -- kubectl get nodes

# 🟢 只读：检查租户 Pod 安全违规
kubectl get events -n team-commerce --field-selector reason=FailedCreatePodSandBox

# 🟡 中风险：强制清理租户异常 Pod
kubectl delete pods -n team-commerce --field-selector status.phase=Failed
```

### 常见问题排查表

| 问题 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| 租户 Pod 无法创建 | ResourceQuota 已满 | `kubectl describe resourcequota -n NS` | 调整配额或清理资源 |
| 跨租户访问失败 | NetworkPolicy 拒绝 | `kubectl get netpol -n NS -o yaml` | 添加允许规则 |
| 镜像拉取失败 | 准入策略拦截 | `kubectl get events -n NS` | 使用允许的镜像仓库 |
| RBAC 权限不足 | RoleBinding 缺失 | `kubectl auth can-i --as=system:serviceaccount:NS:sa ...` | 补充 RoleBinding |
| vCluster 无法连接 | Syncer Pod 异常 | `kubectl logs -n vcluster-NS -l app=vcluster` | 重启 Syncer |
| 存储卷挂载失败 | StorageClass 限制 | `kubectl get sc; kubectl describe pvc -n NS` | 使用允许的 SC |

## 多租户成熟度模型

| 等级 | 名称 | 特征 | 关键能力 |
|------|------|------|----------|
| L1 | 共享集群 | 无隔离，所有团队共享 | 基本 Namespace |
| L2 | 软隔离 | RBAC + Quota + NetworkPolicy | 命名空间多租户 |
| L3 | 策略驱动 | PSA + 准入策略 + 审计 | Kyverno/Gatekeeper |
| L4 | 自助服务 | 自动化 Onboarding + Portal | GitOps 模板 + HNC |
| L5 | 硬隔离 | vCluster/节点池 + 独立监控 | 控制平面隔离 |
| L6 | 平台即产品 | 内部开发者平台 + 计量计费 | Backstage + Kubecost |

## Related

- [[集群基础/控制平面/index.md|控制平面]]
- [[生产运维/集群治理/index.md|集群治理]]
- [[安全/身份与访问/index.md|身份与访问]]
