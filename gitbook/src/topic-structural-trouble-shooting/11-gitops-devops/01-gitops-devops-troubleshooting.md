# GitOps/DevOps 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: GitOps 运维保障

## 0. 10 分钟快速诊断

1. **控制器存活**：检查 ArgoCD/Flux 控制器 Pod 状态与日志。
2. **同步状态**：`kubectl get applications/helmreleases/kustomizations -A`，定位 OutOfSync/Failed。
3. **仓库连接**：验证 repo secret/SSH key/Token，确认仓库可访问。
4. **渲染检查**：确认 Helm/Kustomize 渲染是否失败或资源冲突。
5. **漂移检测**：对关键应用执行 diff，判断实际与期望偏差。
6. **快速缓解**：
   - 临时暂停自动同步，手动回滚稳定版本。
   - 修复仓库凭证或降低同步频率。
7. **证据留存**：保存控制器日志、同步状态与失败事件。

## 🔧 GitOps/DevOps 常见问题与影响分析

### GitOps 核心组件故障现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| ArgoCD 同步失败 | `ApplicationOutOfSync` 持续存在 | ⭐⭐⭐ 高 | P0 |
| FluxCD reconciliation 失败 | `ReconciliationFailed` 事件频繁 | ⭐⭐⭐ 高 | P0 |
| Git 仓库连接问题 | `failed to clone repository` | ⭐⭐⭐ 高 | P0 |
| Helm Chart 部署失败 | `Helm release failed` | ⭐⭐ 中 | P1 |
| CI/CD 流水线阻塞 | `pipeline stuck in pending` | ⭐⭐⭐ 高 | P0 |
| 配置漂移检测失效 | `drift detected but not corrected` | ⭐⭐ 中 | P1 |
| 多环境同步异常 | `environments out of sync` | ⭐⭐ 中 | P1 |
| Secret 管理失败 | `failed to decrypt secrets` | ⭐⭐⭐ 高 | P0 |

### GitOps 状态检查命令

```bash
# ArgoCD 状态检查
echo "=== ArgoCD 状态检查 ==="
kubectl get pods -n argocd -l app.kubernetes.io/name=argocd-application-controller
kubectl get applications -A
argocd app list 2>/dev/null || echo "ArgoCD CLI 未配置"

# FluxCD 状态检查
echo "=== FluxCD 状态检查 ==="
kubectl get pods -n flux-system
flux check 2>/dev/null || echo "Flux CLI 未配置"
kubectl get kustomizations,helmreleases -A

# Git 仓库连接检查
echo "=== Git 仓库连接检查 ==="
kubectl get gitrepositories -A
kubectl get secrets -n argocd -l argocd.argoproj.io/secret-type=repository 2>/dev/null

# CI/CD 流水线状态
echo "=== CI/CD 流水线状态 ==="
kubectl get pipelineruns,taskruns -A 2>/dev/null || echo "Tekton 未部署"
# 或者检查 Jenkins/GitLab CI 等其他 CI 系统状态
```

## 🔍 GitOps/DevOps 问题诊断方法

### 诊断原理说明

GitOps 故障诊断需要从以下几个维度进行分析：

1. **Git 仓库层面**：SSH 密钥、访问权限、网络连接
2. **GitOps 控制器层面**：ArgoCD/FluxCD 状态、配置同步
3. **应用部署层面**：Helm Chart、Kustomize、Manifest 状态
4. **CI/CD 流水线层面**：触发器、任务执行、资源依赖
5. **安全合规层面**：Secret 管理、RBAC 配置、审计日志

### GitOps 问题诊断决策树

```
GitOps 故障
    ├── Git 仓库连接问题
    │   ├── SSH 密钥配置
    │   ├── 访问令牌权限
    │   ├── 网络策略限制
    │   └── 仓库 URL 配置
    ├── 同步状态问题
    │   ├── ArgoCD Application 状态
    │   ├── Flux Kustomization 状态
    │   ├── Helm Release 状态
    │   └── 资源健康检查
    ├── 部署配置问题
    │   ├── Helm Chart 版本兼容性
    │   ├── Kustomize overlay 配置
    │   ├── Manifest 语法错误
    │   └── 依赖关系处理
    └── 流水线执行问题
        ├── 触发器配置
        ├── 任务依赖关系
        ├── 资源配额限制
        └── 执行环境状态
```

### 详细诊断命令

#### 1. ArgoCD 故障诊断

```bash
#!/bin/bash
# ArgoCD 故障诊断脚本

echo "=== ArgoCD 故障诊断 ==="

# 1. 检查 ArgoCD 组件状态
echo "1. ArgoCD 组件状态检查:"
kubectl get pods -n argocd -l app.kubernetes.io/part-of=argocd

# 检查控制器日志
echo "ArgoCD Application Controller 日志摘要:"
kubectl logs -n argocd -l app.kubernetes.io/name=argocd-application-controller --tail=100 | grep -i error | tail -10

# 2. 检查应用程序同步状态
echo "2. 应用程序同步状态:"
kubectl get applications -A -o json | jq -r '
  .items[] | 
  select(.status.sync.status != "Synced" or .status.health.status != "Healthy") |
  "\(.metadata.namespace)/\(.metadata.name): sync=\(.status.sync.status), health=\(.status.health.status)"
'

# 3. 检查 Git 仓库配置
echo "3. Git 仓库配置检查:"
kubectl get secrets -n argocd -l argocd.argoproj.io/secret-type=repository -o json | jq -r '
  .items[] |
  "Repository: \(.data.url | @base64d)",
  "  Type: \(.type)",
  "  Status: \(.data.lastConnectionState | @base64d // "unknown")"
'

# 4. 检查应用详细状态
echo "4. 应用详细状态检查:"
for app in $(kubectl get applications -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
  namespace=$(echo $app | cut -d/ -f1)
  name=$(echo $app | cut -d/ -f2)
  
  echo "检查应用 $namespace/$name:"
  kubectl get application $name -n $namespace -o json | jq -r '
    "\(.metadata.name):",
    "  Sync Status: \(.status.sync.status)",
    "  Health Status: \(.status.health.status)",
    "  Reconciled At: \(.status.reconciledAt)",
    "  Operation State: \(.status.operationState.phase // "none")"
  '
  
  # 检查最近的同步操作
  if [ "$(kubectl get application $name -n $namespace -o jsonpath='{.status.operationState.phase}')" = "Failed" ]; then
    echo "  最近同步失败原因:"
    kubectl get application $name -n $namespace -o jsonpath='{.status.operationState.message}' | head -3
  fi
  echo ""
done

# 5. 检查 RBAC 配置
echo "5. ArgoCD RBAC 配置检查:"
kubectl get configmap argocd-rbac-cm -n argocd -o yaml 2>/dev/null || echo "RBAC 配置不存在"

# 6. 检查证书和 TLS 配置
echo "6. 证书和 TLS 配置检查:"
kubectl get secrets -n argocd -l argocd.argoproj.io/secret-type=repo-creds 2>/dev/null
```

#### 2. FluxCD 故障诊断

```bash
#!/bin/bash
# FluxCD 故障诊断脚本

echo "=== FluxCD 故障诊断 ==="

# 1. 检查 Flux 组件状态
echo "1. Flux 组件状态检查:"
kubectl get pods -n flux-system

# 检查 Flux 控制器日志
echo "Flux Controllers 日志摘要:"
for controller in source-controller kustomize-controller helm-controller notification-controller; do
  echo "=== $controller 日志 ==="
  kubectl logs -n flux-system -l app=$controller --tail=50 | grep -i error | tail -5
done

# 2. 检查 GitRepository 状态
echo "2. GitRepository 状态检查:"
kubectl get gitrepositories -A -o json | jq -r '
  .items[] |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].type)=\(.status.conditions[-1].status) (\(.status.conditions[-1].reason // "unknown"))"
'

# 3. 检查 Kustomization 状态
echo "3. Kustomization 状态检查:"
kubectl get kustomizations -A -o json | jq -r '
  .items[] |
  select(.status.conditions[-1].status != "True") |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].type)=\(.status.conditions[-1].status) (\(.status.conditions[-1].reason))"
'

# 4. 检查 HelmRelease 状态
echo "4. HelmRelease 状态检查:"
kubectl get helmreleases -A -o json | jq -r '
  .items[] |
  select(.status.conditions[-1].status != "True") |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[-1].type)=\(.status.conditions[-1].status) (\(.status.conditions[-1].reason))"
'

# 5. 检查源码变更检测
echo "5. 源码变更检测检查:"
for repo in $(kubectl get gitrepositories -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
  namespace=$(echo $repo | cut -d/ -f1)
  name=$(echo $repo | cut -d/ -f2)
  
  echo "检查仓库 $namespace/$name:"
  kubectl get gitrepository $name -n $namespace -o json | jq -r '
    "  URL: \(.spec.url)",
    "  Revision: \(.status.artifact.revision // "unknown")",
    "  Last Updated: \(.status.lastHandledReconcileAt // "never")"
  '
done

# 6. 检查依赖关系
echo "6. 依赖关系检查:"
kubectl get kustomizations -A -o json | jq -r '
  .items[] |
  select(.spec.dependsOn) |
  "\(.metadata.namespace)/\(.metadata.name) 依赖于:",
  (.spec.dependsOn[] | "  - \(.namespace // "default")/\(.name)")
'
```

#### 3. CI/CD 流水线诊断

```bash
#!/bin/bash
# CI/CD 流水线诊断脚本

echo "=== CI/CD 流水线诊断 ==="

# 1. Tekton 流水线诊断（如果使用 Tekton）
if kubectl get crd pipelineruns.tekton.dev &>/dev/null; then
  echo "1. Tekton 流水线状态检查:"
  kubectl get pipelineruns -A --sort-by=.status.startTime | tail -10
  
  echo "2. PipelineRun 失败分析:"
  kubectl get pipelineruns -A -o json | jq -r '
    .items[] |
    select(.status.conditions[] | select(.type=="Succeeded" and .status=="False")) |
    "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[].message)"
  ' | head -10
  
  echo "3. TaskRun 状态检查:"
  kubectl get taskruns -A --sort-by=.status.startTime | tail -10
fi

# 2. Jenkins 流水线检查（如果使用 Jenkins）
if kubectl get deploy -n jenkins jenkins &>/dev/null; then
  echo "Jenkins 状态检查:"
  kubectl get pods -n jenkins
  echo "Jenkins 队列状态:"
  kubectl exec -n jenkins deploy/jenkins -- curl -s http://localhost:8080/queue/api/json | jq '.items | length' 2>/dev/null || echo "无法获取队列状态"
fi

# 3. GitLab CI 检查（如果使用 GitLab）
echo "3. GitLab Runner 状态检查:"
kubectl get pods -n gitlab-runner 2>/dev/null || echo "GitLab Runner 未部署"

# 4. 通用流水线资源检查
echo "4. 通用流水线资源检查:"
# 检查 PVC 状态（流水线工作空间）
kubectl get pvc -A | grep -E "(pipeline|workspace|build)" || echo "未找到流水线相关 PVC"

# 检查资源配额
kubectl get resourcequotas -A 2>/dev/null

# 检查 LimitRange
kubectl get limitranges -A 2>/dev/null

# 5. 流水线触发器检查
echo "5. 流水线触发器检查:"
# 检查 EventListener (Tekton Triggers)
kubectl get eventlisteners -A 2>/dev/null

# 检查 Ingress/Webhook 配置
kubectl get ingresses -A | grep -i webhook 2>/dev/null
```

#### 4. Secret 管理诊断

```bash
#!/bin/bash
# Secret 管理诊断脚本

echo "=== Secret 管理诊断 ==="

# 1. External Secrets Operator 检查
echo "1. External Secrets Operator 状态:"
kubectl get pods -n external-secrets 2>/dev/null || echo "External Secrets Operator 未部署"

if kubectl get crd externalsecrets.external-secrets.io &>/dev/null; then
  echo "ExternalSecret 资源状态:"
  kubectl get externalsecrets -A -o json | jq -r '
    .items[] |
    select(.status.conditions[] | select(.type=="Ready" and .status=="False")) |
    "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[].message)"
  '
fi

# 2. Sealed Secrets 检查
echo "2. Sealed Secrets 状态:"
kubectl get pods -n kube-system -l app.kubernetes.io/name=sealed-secrets-controller 2>/dev/null

if kubectl get crd sealedsecrets.bitnami.com &>/dev/null; then
  echo "SealedSecret 资源状态:"
  kubectl get sealedsecrets -A -o json | jq -r '
    .items[] |
    select(.status.conditions[] | select(.type=="Synced" and .status=="False")) |
    "\(.metadata.namespace)/\(.metadata.name): \(.status.conditions[].message)"
  '
fi

# 3. Vault Integration 检查
echo "3. Vault Integration 状态:"
kubectl get pods -n vault 2>/dev/null || echo "Vault 未部署"

# 检查 Vault Agent Injector
kubectl get mutatingwebhookconfigurations | grep vault 2>/dev/null

# 4. Secret 一般状态检查
echo "4. Secret 一般状态检查:"
# 检查未使用的 Secret
echo "可能未使用的 Secret (30天内未更新):"
kubectl get secrets --all-namespaces -o json | jq -r '
  .items[] |
  select((.metadata.creationTimestamp | fromdateiso8601) < (now - 30*24*3600)) |
  "\(.metadata.namespace)/\(.metadata.name) (创建时间: \(.metadata.creationTimestamp))"
' | head -10

# 检查 Secret 大小
echo "大尺寸 Secret 检查 (>1MB):"
kubectl get secrets --all-namespaces -o json | jq -r '
  .items[] |
  select(.data != null) |
  .data | to_entries[] | .value | @base64d | length as $size |
  select($size > 1048576) |
  "\(.metadata.namespace)/\(.metadata.name): \($size) bytes"
'
```

## 🔧 GitOps/DevOps 问题解决方案

### ArgoCD 问题解决

#### 方案一：ArgoCD 应用同步失败修复

```yaml
# ArgoCD 应用配置优化
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-app
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/myorg/my-app.git
    targetRevision: HEAD
    path: k8s/overlays/prod
    helm:
      valueFiles:
      - values-prod.yaml
      parameters:
      - name: replicaCount
        value: "3"
      releaseName: my-app-prod
  destination:
    server: https://kubernetes.default.svc
    namespace: prod
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
    - ApplyOutOfSyncOnly=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
    - /spec/replicas
  info:
  - name: url
    value: https://my-app.example.com
```

#### 方案二：ArgoCD RBAC 配置优化

```yaml
# ArgoCD RBAC 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-rbac-cm
  namespace: argocd
data:
  policy.default: role:readonly
  policy.csv: |
    p, role:org-admin, applications, *, */*, allow
    p, role:org-admin, clusters, get, *, allow
    p, role:org-admin, repositories, get, *, allow
    p, role:org-admin, repositories, create, *, allow
    p, role:org-admin, repositories, update, *, allow
    p, role:org-admin, repositories, delete, *, allow
    
    p, role:team-admin, applications, *, my-team/*, allow
    p, role:team-admin, projects, get, my-team-project, allow
    
    g, org:admin-group, role:org-admin
    g, team:dev-group, role:team-admin
    
    # 禁止危险操作
    p, role:*, applications, delete, *, deny
    p, role:*, clusters, delete, *, deny
```

### FluxCD 问题解决

#### 方案一：FluxCD 依赖关系配置

```yaml
# FluxCD 依赖关系配置示例
---
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: infrastructure
  namespace: flux-system
spec:
  interval: 5m
  url: https://github.com/myorg/infrastructure.git
  ref:
    branch: main

---
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: infrastructure-base
  namespace: flux-system
spec:
  interval: 10m
  path: ./base
  prune: true
  sourceRef:
    kind: GitRepository
    name: infrastructure
  timeout: 5m
  validation: client

---
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: infrastructure-prod
  namespace: flux-system
spec:
  interval: 10m
  path: ./overlays/prod
  prune: true
  sourceRef:
    kind: GitRepository
    name: infrastructure
  dependsOn:
  - name: infrastructure-base
  timeout: 5m
  validation: client
```

#### 方案二：FluxCD Helm Release 配置优化

```yaml
# FluxCD Helm Release 配置优化
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: my-app
  namespace: flux-system
spec:
  chart:
    spec:
      chart: my-app
      version: "1.2.x"
      sourceRef:
        kind: HelmRepository
        name: my-repo
        namespace: flux-system
  interval: 5m
  releaseName: my-app-prod
  targetNamespace: prod
  storageNamespace: flux-system
  install:
    remediation:
      retries: 3
  upgrade:
    remediation:
      retries: 3
      remediateLastFailure: true
  rollback:
    timeout: 10m
    disableHooks: false
    recreate: false
    force: false
    cleanupOnFail: true
  values:
    replicaCount: 3
    image:
      repository: nginx
      tag: stable
      pullPolicy: IfNotPresent
  valuesFrom:
  - kind: ConfigMap
    name: my-app-values
    valuesKey: values.yaml
  postRenderers:
  - kustomize:
      patchesStrategicMerge:
      - apiVersion: apps/v1
        kind: Deployment
        metadata:
          name: my-app
        spec:
          template:
            spec:
              containers:
              - name: my-app
                resources:
                  limits:
                    memory: "128Mi"
                    cpu: "500m"
```

### CI/CD 流水线问题解决

#### 方案一：Tekton 流水线优化配置

```yaml
# Tekton 流水线优化配置
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: ci-pipeline
spec:
  workspaces:
  - name: shared-workspace
  params:
  - name: repo-url
    type: string
  - name: revision
    type: string
    default: main
  tasks:
  - name: fetch-repository
    taskRef:
      name: git-clone
    workspaces:
    - name: output
      workspace: shared-workspace
    params:
    - name: url
      value: $(params.repo-url)
    - name: revision
      value: $(params.revision)
    timeout: 5m
    
  - name: run-tests
    taskRef:
      name: run-unit-tests
    runAfter:
    - fetch-repository
    workspaces:
    - name: source
      workspace: shared-workspace
    timeout: 15m
    
  - name: build-image
    taskRef:
      name: kaniko
    runAfter:
    - run-tests
    workspaces:
    - name: source
      workspace: shared-workspace
    params:
    - name: IMAGE
      value: $(params.image-url)
    timeout: 20m
    
  - name: deploy-to-staging
    taskRef:
      name: kubectl-deploy
    runAfter:
    - build-image
    workspaces:
    - name: manifest-dir
      workspace: shared-workspace
    params:
    - name: KUBECONFIG
      value: $(params.staging-kubeconfig)
    - name: MANIFEST_DIR
      value: k8s/staging
    timeout: 10m

---
apiVersion: tekton.dev/v1beta1
kind: PipelineRun
metadata:
  name: ci-pipeline-run-$(uid)
spec:
  pipelineRef:
    name: ci-pipeline
  workspaces:
  - name: shared-workspace
    volumeClaimTemplate:
      spec:
        accessModes:
        - ReadWriteOnce
        resources:
          requests:
            storage: 1Gi
        storageClassName: fast-ssd
  params:
  - name: repo-url
    value: https://github.com/myorg/my-app.git
  - name: revision
    value: $(tt.params.git-revision)
  - name: image-url
    value: registry.example.com/my-app:$(tt.params.git-revision)
  timeouts:
    pipeline: 1h
    tasks: 30m
```

#### 方案二：流水线资源优化

```yaml
# 流水线资源配额和限制
apiVersion: v1
kind: ResourceQuota
metadata:
  name: pipeline-quota
  namespace: cicd
spec:
  hard:
    requests.cpu: "10"
    requests.memory: 20Gi
    limits.cpu: "20"
    limits.memory: 40Gi
    persistentvolumeclaims: "20"
    requests.storage: 100Gi

---
apiVersion: v1
kind: LimitRange
metadata:
  name: pipeline-limit-range
  namespace: cicd
spec:
  limits:
  - default:
      cpu: "1"
      memory: 2Gi
    defaultRequest:
      cpu: "500m"
      memory: 1Gi
    type: Container
  - max:
      storage: 10Gi
    min:
      storage: 1Gi
    type: PersistentVolumeClaim
```

### Secret 管理问题解决

#### 方案一：External Secrets 配置

```yaml
# External Secrets 配置示例
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: my-app-secrets
  namespace: prod
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secrets-manager
    kind: ClusterSecretStore
  target:
    name: my-app-secrets
    creationPolicy: Owner
    template:
      engineVersion: v2
      data:
        database-password: "{{ .password }}"
        api-key: "{{ .apiKey }}"
        tls.crt: "{{ .tlsCert }}"
        tls.key: "{{ .tlsKey }}"
  data:
  - secretKey: password
    remoteRef:
      key: my-app/database
      property: password
  - secretKey: apiKey
    remoteRef:
      key: my-app/api
      property: key
  - secretKey: tlsCert
    remoteRef:
      key: my-app/tls
      property: certificate
  - secretKey: tlsKey
    remoteRef:
      key: my-app/tls
      property: privateKey

---
apiVersion: external-secrets.io/v1beta1
kind: ClusterSecretStore
metadata:
  name: aws-secrets-manager
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
```

#### 方案二：Sealed Secrets 配置

```bash
#!/bin/bash
# Sealed Secrets 配置脚本

echo "=== Sealed Secrets 配置 ==="

# 1. 安装 kubeseal CLI
echo "1. 安装 kubeseal CLI:"
if ! command -v kubeseal &> /dev/null; then
  wget https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.2/kubeseal-0.24.2-linux-amd64.tar.gz
  tar xzf kubeseal-0.24.2-linux-amd64.tar.gz
  sudo install kubeseal /usr/local/bin/
  rm kubeseal-0.24.2-linux-amd64.tar.gz kubeseal
fi

# 2. 获取 Sealed Secrets 公钥
echo "2. 获取 Sealed Secrets 公钥:"
kubeseal --fetch-cert > sealed-secrets-public-cert.pem

# 3. 创建加密的 Secret
echo "3. 创建加密的 Secret 示例:"
cat > secret.yaml << EOF
apiVersion: v1
kind: Secret
metadata:
  name: my-sensitive-data
  namespace: prod
type: Opaque
data:
  username: $(echo -n "myuser" | base64)
  password: $(echo -n "mypassword123" | base64)
EOF

# 加密 Secret
kubeseal --format yaml --cert sealed-secrets-public-cert.pem < secret.yaml > sealed-secret.yaml

echo "加密后的 SealedSecret 已生成: sealed-secret.yaml"

# 4. 验证加密结果
echo "4. 验证加密结果:"
kubectl apply -f sealed-secret.yaml
kubectl get sealedsecret my-sensitive-data -n prod
```

## ⚠️ 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| ArgoCD 应用配置更新 | ⭐⭐ 中 | 可能导致应用同步失败 | 使用 Git revert 回滚配置 |
| FluxCD Kustomization 调整 | ⭐⭐ 中 | 可能影响资源配置同步 | 恢复原 Kustomization 配置 |
| CI/CD 流水线变更 | ⭐⭐⭐ 高 | 可能影响部署流程 | 回滚到上一个稳定版本 |
| Secret 配置修改 | ⭐⭐⭐ 高 | 可能导致应用无法启动 | 恢复原 Secret 配置 |

## 📊 GitOps/DevOps 验证与监控

### GitOps 验证脚本

```bash
#!/bin/bash
# GitOps 验证脚本

echo "=== GitOps 验证 ==="

# 1. ArgoCD 验证
echo "1. ArgoCD 验证:"
if kubectl get crd applications.argoproj.io &>/dev/null; then
  SYNCED_APPS=$(kubectl get applications -A --no-headers | grep -c "Synced")
  TOTAL_APPS=$(kubectl get applications -A --no-headers | wc -l)
  echo "ArgoCD 应用同步状态: $SYNCED_APPS/$TOTAL_APPS 已同步"
  
  if [ $SYNCED_APPS -ne $TOTAL_APPS ]; then
    echo "未同步的应用:"
    kubectl get applications -A | grep -v "Synced"
  fi
else
  echo "ArgoCD 未部署"
fi

# 2. FluxCD 验证
echo "2. FluxCD 验证:"
if kubectl get crd kustomizations.kustomize.toolkit.fluxcd.io &>/dev/null; then
  HEALTHY_KUSTOMIZATIONS=$(kubectl get kustomizations -A --no-headers | grep -c "True")
  TOTAL_KUSTOMIZATIONS=$(kubectl get kustomizations -A --no-headers | wc -l)
  echo "Flux Kustomization 健康状态: $HEALTHY_KUSTOMIZATIONS/$TOTAL_KUSTOMIZATIONS 健康"
  
  if [ $HEALTHY_KUSTOMIZATIONS -ne $TOTAL_KUSTOMIZATIONS ]; then
    echo "不健康的 Kustomization:"
    kubectl get kustomizations -A | grep -v "True"
  fi
else
  echo "FluxCD 未部署"
fi

# 3. Git 仓库连接验证
echo "3. Git 仓库连接验证:"
for repo in $(kubectl get gitrepositories -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' 2>/dev/null); do
  namespace=$(echo $repo | cut -d/ -f1)
  name=$(echo $repo | cut -d/ -f2)
  
  READY=$(kubectl get gitrepository $name -n $namespace -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
  if [ "$READY" = "True" ]; then
    echo "✓ $namespace/$name: 连接正常"
  else
    echo "✗ $namespace/$name: 连接失败"
    REASON=$(kubectl get gitrepository $name -n $namespace -o jsonpath='{.status.conditions[?(@.type=="Ready")].reason}')
    MESSAGE=$(kubectl get gitrepository $name -n $namespace -o jsonpath='{.status.conditions[?(@.type=="Ready")].message}')
    echo "  原因: $REASON"
    echo "  消息: $MESSAGE"
  fi
done

# 4. 流水线验证
echo "4. 流水线验证:"
if kubectl get crd pipelineruns.tekton.dev &>/dev/null; then
  RECENT_PIPELINES=$(kubectl get pipelineruns -A --sort-by=.status.completionTime | tail -5)
  echo "最近的流水线运行:"
  echo "$RECENT_PIPELINES"
else
  echo "未检测到 Tekton 流水线"
fi

echo "GitOps 验证完成！"
```

### GitOps 监控告警配置

```yaml
# Prometheus GitOps 监控告警
groups:
- name: gitops
  rules:
  - alert: ArgocdAppOutOfSync
    expr: argocd_app_info{sync_status!="Synced"} > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "ArgoCD 应用不同步"
      description: "应用 {{ $labels.name }} 在命名空间 {{ $labels.namespace }} 中不同步"

  - alert: FluxKustomizationNotReady
    expr: gotk_reconcile_condition{type="Ready",status!="True"} > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Flux Kustomization 未就绪"
      description: "Kustomization {{ $labels.exported_name }} 在命名空间 {{ $labels.namespace }} 中未就绪"

  - alert: GitRepositoryNotReady
    expr: gotk_reconcile_condition{type="Ready",status!="True",kind="GitRepository"} > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Git 仓库未就绪"
      description: "Git 仓库 {{ $labels.name }} 连接失败"

  - alert: PipelineRunFailed
    expr: tekton_pipelinerun_duration_seconds_count{status="Failed"} > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "流水线运行失败"
      description: "流水线 {{ $labels.pipeline }} 运行失败"

  - alert: ExternalSecretSyncFailed
    expr: externalsecret_status_condition{type="Ready",status!="True"} > 0
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "外部 Secret 同步失败"
      description: "外部 Secret {{ $labels.name }} 同步失败"

  - alert: SealedSecretSyncFailed
    expr: sealed_secrets_controller_sync_errors > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Sealed Secret 同步失败"
      description: "Sealed Secret 解密同步出现错误"
```

## 📚 GitOps/DevOps 最佳实践

### GitOps 配置管理最佳实践

```yaml
# GitOps 配置管理最佳实践
gitopsBestPractices:
  repositoryStructure:
    base:
      path: k8s/base
      contains: 
        - common resources
        - namespace definitions
        - cluster-wide configurations
    
    overlays:
      dev:
        path: k8s/overlays/dev
        extends: ../base
        environmentSpecific: true
        
      staging:
        path: k8s/overlays/staging
        extends: ../base
        environmentSpecific: true
        
      prod:
        path: k8s/overlays/prod
        extends: ../base
        environmentSpecific: true
        securityEnhanced: true
  
  automationPolicies:
    autoSync:
      enabled: true
      prune: true
      selfHeal: true
      
    syncWindows:
      - kind: allow
        schedule: "0 0 * * *"  # 每天午夜允许同步
        duration: 1h
        applications:
        - "*"
        
      - kind: deny
        schedule: "0 8-18 * * 1-5"  # 工作时间禁止同步
        duration: 10h
        applications:
        - "prod/*"
  
  securityControls:
    rbac:
      defaultPolicy: readonly
      adminGroups:
        - "platform-team"
        - "sre-team"
      
    auditLogging:
      enabled: true
      retention: 90d
      
    driftDetection:
      enabled: true
      alertThreshold: 5m
```

### CI/CD 流水线最佳实践

```bash
#!/bin/bash
# CI/CD 流水线最佳实践检查脚本

BEST_PRACTICES_REPORT="/var/log/kubernetes/cicd-best-practices-$(date +%Y%m%d).log"

{
  echo "=== CI/CD 最佳实践检查报告 $(date) ==="
  
  # 1. 流水线安全检查
  echo "1. 流水线安全检查:"
  
  # 检查是否使用固定标签的镜像
  FIXED_TAGS=$(kubectl get pipelineruns -A -o json | jq -r '.items[].spec | select(.tasks) | .tasks[].taskSpec.steps[].image' | grep -E ":(latest|dev|staging)$" | wc -l)
  if [ $FIXED_TAGS -gt 0 ]; then
    echo "⚠ 发现 $FIXED_TAGS 个使用非固定标签的镜像"
  else
    echo "✓ 所有镜像都使用固定标签"
  fi
  
  # 2. 资源限制检查
  echo "2. 资源限制检查:"
  UNLIMITED_TASKS=$(kubectl get taskruns -A -o json | jq -r '.items[].spec | select(.steps) | .steps[] | select(.resources == null)' | wc -l)
  if [ $UNLIMITED_TASKS -gt 0 ]; then
    echo "⚠ 发现 $UNLIMITED_TASKS 个未设置资源限制的任务"
  else
    echo "✓ 所有任务都设置了资源限制"
  fi
  
  # 3. 流水线缓存检查
  echo "3. 流水线缓存检查:"
  CACHE_VOLUMES=$(kubectl get pv -o json | jq -r '.items[] | select(.spec.claimRef.name | contains("cache")) | .metadata.name' | wc -l)
  echo "发现 $CACHE_VOLUMES 个缓存卷"
  
  # 4. 并行执行检查
  echo "4. 并行执行检查:"
  PARALLEL_PIPELINES=$(kubectl get pipelineruns -A --field-selector=status.phase=Running | wc -l)
  echo "当前并行运行的流水线: $PARALLEL_PIPELINES"
  
  # 5. 流水线成功率统计
  echo "5. 流水线成功率统计:"
  SUCCESS_RATE=$(kubectl get pipelineruns -A --sort-by=.status.completionTime | tail -20 | grep -c "Succeeded" || echo "0")
  TOTAL_RECENT=$(kubectl get pipelineruns -A --sort-by=.status.completionTime | tail -20 | wc -l)
  if [ $TOTAL_RECENT -gt 0 ]; then
    RATE_PERCENT=$((SUCCESS_RATE * 100 / TOTAL_RECENT))
    echo "最近20个流水线成功率: ${RATE_PERCENT}%"
  fi
  
} >> "$BEST_PRACTICES_REPORT"

echo "最佳实践检查报告已生成: $BEST_PRACTICES_REPORT"
```

## 🔄 典型 GitOps 故障案例

### 案例一：ArgoCD 应用持续 OutOfSync

**问题描述**：ArgoCD 应用状态持续显示 OutOfSync，即使手动同步也无法解决。

**根本原因**：应用配置中包含了自动生成的字段（如时间戳、随机值），导致 Git 状态与集群状态永远不一致。

**解决方案**：
1. 使用 ignoreDifferences 配置忽略自动生成的字段
2. 移除配置中的动态值
3. 设置合理的同步选项

### 案例二：FluxCD 依赖关系死锁

**问题描述**：多个 Kustomization 资源相互依赖，导致 reconciliation 过程死锁。

**根本原因**：循环依赖或依赖配置错误，Flux 无法确定正确的应用顺序。

**解决方案**：
1. 分析并消除循环依赖
2. 重新设计依赖层次结构
3. 使用 dependsOn 字段明确指定依赖关系

## 📞 GitOps 支持资源

**官方文档**：
- ArgoCD: https://argo-cd.readthedocs.io/
- FluxCD: https://fluxcd.io/docs/
- Tekton: https://tekton.dev/docs/

**社区支持**：
- GitOps Working Group: https://github.com/gitops-working-group/gitops-working-group
- CNCF GitOps TAG: https://github.com/cncf/tag-app-delivery
- Kubernetes Slack #gitops 频道