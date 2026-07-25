---
title: Kubernetes API Versioning, Deprecation, and Migration Strategy
description: K8s API 版本管理 — API 生命周期、废弃策略、迁移工具、CRD 版本管理、兼容性保证、升级路径
summary: Kubernetes API 版本管理机制与生产环境 API 迁移的完整实践
category: reference
tags:
- api-version
- deprecation
- migration
- compatibility
- upgrade
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: cluster
---
# Kubernetes API 版本管理与迁移策略

> 理解 K8s API 版本生命周期，安全完成 API 迁移与集群升级。

## API 版本生命周期

```
Alpha (v1alpha1) → Beta (v1beta1) → GA/Stable (v1)
     │                  │                  │
  默认关闭          默认开启           永久支持
  可能破坏性变更    可能有变更         向后兼容
  单版本可用        多版本可用         长期维护
```

## 版本命名规则

| 版本 | 格式 | 稳定性 | 示例 |
|------|------|--------|------|
| Alpha | v{N}alpha{M} | 不稳定，随时移除 | `batch/v1alpha1` |
| Beta | v{N}beta{M} | 较稳定，可能有变更 | `networking.k8s.io/v1beta1` |
| GA | v{N} | 稳定，向后兼容 | `apps/v1`, `v1` |

## 关键 API 废弃时间线

| API | 废弃版本 | 移除版本 | 替代 |
|-----|----------|----------|------|
| `extensions/v1beta1` Ingress | 1.14 | 1.22 | `networking.k8s.io/v1` |
| `apps/v1beta1` Deployment | 1.9 | 1.16 | `apps/v1` |
| `rbac.authorization.k8s.io/v1beta1` | 1.17 | 1.22 | `rbac.authorization.k8s.io/v1` |
| `policy/v1beta1` PodSecurityPolicy | 1.21 | 1.25 | Pod Security Admission |
| `policy/v1beta1` PodDisruptionBudget | 1.21 | 1.25 | `policy/v1` |
| `batch/v1beta1` CronJob | 1.21 | 1.25 | `batch/v1` |
| `discovery.k8s.io/v1beta1` EndpointSlice | 1.21 | 1.25 | `discovery.k8s.io/v1` |
| `flowcontrol.apiserver.k8s.io/v1beta2` | 1.26 | 1.29 | `flowcontrol.apiserver.k8s.io/v1` |
| `flowcontrol.apiserver.k8s.io/v1beta3` | 1.29 | 1.32 | `flowcontrol.apiserver.k8s.io/v1` |
| `admissionregistration.k8s.io/v1beta1` | 1.16 | 1.22 | `admissionregistration.k8s.io/v1` |

## 检测废弃 API 使用

### kubectl 检测

```bash
# 查看集群中使用的 API 版本
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 使用 kubent（kube-no-trouble）扫描
kubent  # 扫描集群中所有废弃 API 资源
kubent --target-versions 1.30  # 针对目标版本检查

# 使用 pluto 扫描
pluto detect-all-in-cluster  # 扫描集群
pluto detect-files -d ./manifests/  # 扫描本地文件
pluto detect-helm  # 扫描 Helm releases
```

### 审计日志检测

```yaml
# 审计策略：记录废弃 API 调用
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  - level: Metadata
    resources:
      - group: "extensions"
        resources: ["*"]
      - group: "apps"
        version: "v1beta1"
        resources: ["*"]
    omitStages: ["RequestReceived"]
```

```promql
# Prometheus 告警：废弃 API 被调用
apiserver_requested_deprecated_apis{group!="", version!=""} > 0
```

## 迁移工具

### kubectl convert（插件）

```bash
# 安装 convert 插件
kubectl krew install convert

# 转换单个文件
kubectl convert -f old-ingress.yaml --output-version networking.k8s.io/v1

# 批量转换目录
kubectl convert -f ./manifests/ --output-version apps/v1 -o yaml > converted/
```

### kubent（检测）

```bash
# 安装
brew install kubent  # macOS
# 或从 GitHub Releases 下载

# 扫描集群
kubent
# 输出示例:
# KIND         NAMESPACE  NAME         API_VERSION
# Ingress      default    my-ingress   extensions/v1beta1 (removed in 1.22)
# CronJob      prod       backup-job   batch/v1beta1 (removed in 1.25)
```

### Helm 迁移

```bash
# 检查 Helm release 使用的 API 版本
helm list -A -o json | jq '.[].name' | while read release; do
  helm get manifest $release | grep "apiVersion:" | sort -u
done

# 升级 Chart（确保使用新 API）
helm upgrade my-release ./chart --set apiVersion=networking.k8s.io/v1
```

## CRD 版本管理

### 多版本 CRD

```yaml
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: widgets.example.com
spec:
  group: example.com
  names:
    kind: Widget
    plural: widgets
  scope: Namespaced
  versions:
    - name: v1
      served: true
      storage: true  # 存储版本
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                size:
                  type: string
                color:
                  type: string
    - name: v1alpha1
      served: true
      storage: false  # 非存储版本
      deprecated: true
      deprecationWarning: "v1alpha1 已废弃，请使用 v1"
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                size:
                  type: integer  # 旧版本用整数
  conversion:
    strategy: Webhook
    webhook:
      conversionReviewVersions: ["v1"]
      clientConfig:
        service:
          namespace: default
          name: widget-conversion-webhook
          path: /convert
```

### Conversion Webhook 实现

```go
// 版本转换逻辑
func convertV1Alpha1ToV1(src *v1alpha1.Widget) *v1.Widget {
    return &v1.Widget{
        Spec: v1.WidgetSpec{
            Size:  fmt.Sprintf("%dpx", src.Spec.Size), // int → string
            Color: src.Spec.Color,
        },
    }
}

func convertV1ToV1Alpha1(src *v1.Widget) *v1alpha1.Widget {
    size, _ := strconv.Atoi(strings.TrimSuffix(src.Spec.Size, "px"))
    return &v1alpha1.Widget{
        Spec: v1alpha1.WidgetSpec{
            Size:  size,
            Color: src.Spec.Color,
        },
    }
}
```

## 集群升级中的 API 迁移

### 升级前检查清单

```bash
#!/bin/bash
# pre-upgrade-api-check.sh
TARGET_VERSION="1.30"

echo "=== 检查废弃 API 使用 ==="
kubent --target-versions $TARGET_VERSION

echo "=== 检查 Helm releases ==="
pluto detect-helm --target-versions $TARGET_VERSION

echo "=== 检查 GitOps 仓库 ==="
pluto detect-files -d ~/gitops-repo/ --target-versions $TARGET_VERSION

echo "=== 检查 CRD 版本 ==="
kubectl get crd -o json | jq -r '.items[] | 
  select(.spec.versions[] | select(.deprecated == true)) | 
  .metadata.name'

echo "=== 检查 Webhook 兼容性 ==="
kubectl get validatingwebhookconfigurations,mutatingwebhookconfigurations \
  -o json | jq -r '.items[].webhooks[].admissionReviewVersions'
```

### 迁移顺序

```
1. 检测所有废弃 API 使用（kubent/pluto）
2. 更新 GitOps 仓库中的 manifests
3. 升级 Helm Charts 到兼容版本
4. 更新 CRD 定义（先添加新版本，后移除旧版本）
5. 更新 CI/CD Pipeline 中的 kubectl 命令
6. 验证所有 Operator 兼容目标版本
7. 执行集群升级
8. 移除旧版本 CRD（确认无流量后）
```

## 兼容性保证

| 保证 | 说明 |
|------|------|
| GA API 不删除 | 一旦 GA，永不从 API Server 移除 |
| 向后兼容 | 新版本必须能读取旧版本数据 |
| 字段不删除 | GA 后字段只增不减（可标记废弃） |
| 语义不变 | 字段含义不会改变 |
| Beta 可能变更 | Beta API 可能有破坏性变更 |
| Alpha 无保证 | 随时可能移除 |

## 最佳实践

1. **CI 中集成 pluto/kubent** — 每次 PR 自动检测废弃 API
2. **GitOps 仓库定期扫描** — CronJob 每周检测
3. **监控 `apiserver_requested_deprecated_apis`** — 实时告警
4. **CRD 使用 Conversion Webhook** — 支持多版本平滑迁移
5. **升级前至少 1 个版本检测** — 不要等到移除版本才迁移
6. **Operator 兼容性矩阵** — 升级前确认所有 Operator 支持目标版本

---

## API 优先级与公平性 (APF)

### 概念与架构

```
客户端请求 → API Server
    │
    ├── FlowSchema (分类请求)
    │   ├── system: 最高优先级 (kubelet, controller-manager)
    │   ├── leader-election: 高优先级
    │   ├── workload-high: 生产工作负载
    │   ├── workload-low: 普通请求
    │   └── catch-all: 最低优先级
    │
    └── PriorityLevelConfiguration (分配并发)
        ├── exempt: 无限制
        ├── system: 30% 并发
        ├── leader-election: 20%
        ├── workload-high: 25%
        ├── workload-low: 20%
        └── catch-all: 5%
```

### 生产配置

```yaml
# 自定义 PriorityLevel
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: gitops-controllers
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 15
    limitResponse:
      type: Queue
      queuing:
        queues: 64
        handSize: 8
        queueLengthLimit: 50
---
# 为 ArgoCD/Flux 创建专用 FlowSchema
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: gitops-controllers
spec:
  priorityLevelConfiguration:
    name: gitops-controllers
  matchingPrecedence: 800
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: argocd-application-controller
            namespace: argocd
        - kind: ServiceAccount
          serviceAccount:
            name: flux-controller
            namespace: flux-system
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          verbs: ["*"]
          namespaces: ["*"]
```

### APF 监控

```promql
# 请求被拒绝（并发耗尽）
rate(apiserver_flowcontrol_rejected_requests_total[5m]) > 0

# 队列等待时间
histogram_quantile(0.99,
  rate(apiserver_flowcontrol_request_wait_duration_seconds_bucket[5m])
) > 1

# 各优先级并发使用率
apiserver_flowcontrol_current_executing_requests
/
apiserver_flowcontrol_nominal_concurrency_limit
```

---

## 自动化迁移 Pipeline

### GitOps 仓库 API 迁移自动化

```yaml
# .github/workflows/api-migration.yaml
name: API Version Migration
on:
  schedule:
    - cron: '0 2 * * 1'  # 每周一凌晨 2 点
  workflow_dispatch:
    inputs:
      target_version:
        description: '目标 K8s 版本'
        required: true
        default: '1.33'

jobs:
  detect-deprecated:
    runs-on: ubuntu-latest
    outputs:
      has_deprecated: ${{ steps.check.outputs.found }}
    steps:
      - uses: actions/checkout@v4
      - name: Install pluto
        run: |
          curl -sL https://github.com/FairwindsOps/pluto/releases/latest/download/pluto_linux_amd64.tar.gz | tar xz
          sudo mv pluto /usr/local/bin/
      - name: Detect deprecated APIs
        id: check
        run: |
          pluto detect-files -d ./manifests/ \
            --target-versions ${{ github.event.inputs.target_version || '1.33' }} \
            --output json > deprecated.json
          COUNT=$(cat deprecated.json | jq '.items | length')
          echo "found=$COUNT" >> $GITHUB_OUTPUT
          if [ "$COUNT" -gt 0 ]; then
            echo "::warning::发现 $COUNT 个废弃 API 使用"
            cat deprecated.json | jq '.items[] | "\(.name): \(.version) -> \(.targetVersion)"'
          fi

  auto-migrate:
    needs: detect-deprecated
    if: needs.detect-deprecated.outputs.has_deprecated != '0'
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install kubectl-convert
        run: |
          curl -LO "https://dl.k8s.io/release/v1.33.0/bin/linux/amd64/kubectl-convert"
          chmod +x kubectl-convert && sudo mv kubectl-convert /usr/local/bin/
      - name: Auto-convert manifests
        run: |
          find ./manifests/ -name '*.yaml' -exec grep -l 'apiVersion:' {} \; | while read f; do
            kubectl convert -f "$f" --output-version apps/v1 -o yaml > "${f}.new" 2>/dev/null && \
              mv "${f}.new" "$f" || rm -f "${f}.new"
          done
      - name: Create PR
        uses: peter-evans/create-pull-request@v6
        with:
          title: "chore: migrate deprecated APIs for K8s ${{ github.event.inputs.target_version || '1.33' }}"
          body: "自动检测并迁移废弃 API。请人工审查后合并。"
          branch: api-migration/auto
```

### 集群内废弃 API 监控 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: deprecated-api-scanner
  namespace: monitoring
spec:
  schedule: "0 8 * * *"  # 每天 8:00
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: api-scanner
          containers:
            - name: scanner
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== 废弃 API 扫描报告 $(date) ==="
                  # 检查 Prometheus 指标
                  DEPRECATED=$(curl -s http://prometheus:9090/api/v1/query?query=apiserver_requested_deprecated_apis | \
                    jq '.data.result | length')
                  if [ "$DEPRECATED" -gt 0 ]; then
                    echo "🔴 发现 $DEPRECATED 个废弃 API 被调用"
                    curl -s http://prometheus:9090/api/v1/query?query=apiserver_requested_deprecated_apis | \
                      jq '.data.result[] | .metric'
                    # 发送告警通知
                    # curl -X POST $WEBHOOK_URL -d '{"text": "..."}'
                  else
                    echo "✅ 无废弃 API 使用"
                  fi
          restartPolicy: OnFailure
```

---

## 回滚与应急策略

### API 迁移回滚

```bash
# 🟡 如果迁移后出现问题，快速回滚

# 1. GitOps 回滚（推荐）
# ArgoCD:
argocd app rollback my-app
# Flux:
flux suspend kustomization my-app
git revert HEAD
git push
flux resume kustomization my-app

# 2. 手动回滚单个资源
kubectl apply -f backup/old-manifest.yaml

# 3. 如果是 CRD 版本问题
# 重新启用旧版本
kubectl patch crd widgets.example.com --type='json' \
  -p='[{"op":"replace","path":"/spec/versions/1/served","value":true}]'
```

### 升级失败应急

```bash
# 🔴 集群升级失败应急流程

# 1. 评估影响
kubectl get nodes
kubectl get pods -A --field-selector=status.phase!=Running

# 2. 如果 API Server 无法启动
# 回滚静态 Pod 镜像:
# /etc/kubernetes/manifests/kube-apiserver.yaml
#   image: registry.k8s.io/kube-apiserver:v1.32.x

# 3. 如果 etcd 数据损坏
# 从升级前快照恢复:
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restore

# 4. 通知相关团队
# 5. 记录事故时间线
# 6. 复盘并更新升级流程
```

---

## 多集群 API 兼容性管理

### 版本矩阵管理

| 集群 | 当前版本 | 目标版本 | 计划升级时间 | 阻塞因素 |
|------|----------|----------|------------|----------|
| prod-cn | 1.31 | 1.33 | 2026-Q3 | Operator 兼容性 |
| prod-us | 1.32 | 1.33 | 2026-Q3 | 无 |
| staging | 1.33 | 1.33 | - | 已是最新 |
| dev | 1.33 | 1.33 | - | 已是最新 |

### 跨版本兼容策略

```yaml
# 多版本 Manifest 管理 (Kustomize)
# base/deployment.yaml 使用最低公共 API 版本
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
  template:
    spec:
      containers:
        - name: app
          image: myapp:v1.0
---
# overlays/prod-cn/ 可添加版本特定配置
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - ../../base
patches:
  - target:
      kind: Deployment
      name: my-app
    patch: |
      - op: replace
        path: /spec/replicas
        value: 5
```

### Operator 兼容性检查

```bash
#!/bin/bash
# 🟢 检查所有 Operator 的 K8s 版本兼容性
TARGET_VERSION="1.33"

echo "=== Operator 兼容性检查 (目标: v$TARGET_VERSION) ==="

# 检查 CRD 版本
kubectl get crd -o json | jq -r '.items[] |
  select(.spec.versions[] | select(.deprecated == true)) |
  "CRD: \(.metadata.name) - 有废弃版本"'

# 检查 Operator 镜像版本
for ns in argocd cert-manager ingress-nginx monitoring operators; do
  echo "\n[$ns]"
  kubectl -n $ns get deploy -o custom-columns=\
  'NAME:.metadata.name,IMAGE:.spec.template.spec.containers[0].image' \
  --no-headers 2>/dev/null
done

# 检查 Webhook 兼容性
kubectl get validatingwebhookconfigurations -o json | \
  jq -r '.items[] | "Webhook: \(.metadata.name), APIVersions: \(.webhooks[0].admissionReviewVersions)"'
```

## Related

- [[01-集群基础/04-API版本/index.md|API 版本]]
- [[01-集群基础/03-控制平面/index.md|控制平面]]
- [[11-发布变更/07-迁移方案/index.md|迁移方案]]
