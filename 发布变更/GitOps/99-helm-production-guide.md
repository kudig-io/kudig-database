---
title: Helm 生产实践指南（阿里云专有云版）
description: Helm chart 开发规范、values 分层管理、helm-secrets/SOPS 加密、chart 测试、依赖管理、与 ArgoCD/Flux
  集成、回滚策略，面向阿里云与专有云 K8s 发布变更场景
summary: Helm chart 开发规范、values 分层管理、helm-secrets/SOPS 加密、chart 测试、依赖管理、与 ArgoCD/Flux
  集成、回滚策略，面向阿里云与专有云 K8s 发布变更场景
category: gitops
tags:
- k8s
- helm
- chart
- gitops
- argocd
- flux
- sops
- secrets
- values
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-26'
updated: '2026-06-26'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DevOps 工程师
- 平台工程师
estimated_read_time: 35min
intent_queries:
- Helm 生产实践规范
- Helm values 分层管理
- Helm ArgoCD Flux 集成
trigger_keywords:
- Helm
- chart
- values
- helm-secrets
- ArgoCD
- Flux
- 回滚
prerequisites:
- helm-basics
- kubectl-basics
- gitops-basics
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




# Helm 生产实践指南（阿里云专有云版）

> **适用版本**: Helm v3.13+ | **Kubernetes v1.28 - v1.32** | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境的 Helm 生产实践，覆盖 chart 开发、values 分层、secret 加密、测试、依赖、GitOps 集成与回滚。云厂商命令以阿里云/专有云为主。

<!-- chunk: 目录 -->
## 目录

1. [Chart 开发规范](#chart-开发规范)
2. [Values 分层管理](#values-分层管理)
3. [Secret 加密：helm-secrets / SOPS](#secret-加密helm-secrets--sops)
4. [Chart 测试](#chart-测试)
5. [依赖管理](#依赖管理)
6. [与 ArgoCD/Flux 集成](#与-argocdflux-集成)
7. [回滚策略](#回滚策略)
8. [阿里云/专有云场景](#阿里云专有云场景)
9. [发布审计与可追溯性](#发布审计与可追溯性)
10. [多集群与专有云发布流水线](#多集群与专有云发布流水线)
11. [发布变更检查清单](#发布变更检查清单)

---

<!-- chunk: 1. Chart 开发规范 -->
## 1. Chart 开发规范

### 1.1 目录结构

```
myapp/
├── Chart.yaml          # chart 元数据
├── values.yaml         # 默认 values
├── values-dev.yaml     # 开发环境覆盖
├── values-staging.yaml # 预发环境覆盖
├── values-prod.yaml    # 生产环境覆盖
├── charts/             # 依赖 chart
├── templates/
│   ├── _helpers.tpl
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── ingress.yaml
│   ├── configmap.yaml
│   ├── secret.yaml
│   ├── hpa.yaml
│   ├── pdb.yaml
│   ├── serviceaccount.yaml
│   └── NOTES.txt
├── tests/
│   └── test-connection.yaml
└── README.md
```

### 1.2 Chart.yaml 规范

```yaml
apiVersion: v2
name: myapp
description: A Helm chart for myapp on ACK/Apsara Stack
type: application
version: 1.2.3
appVersion: "2.1.0"
kubeVersion: ">=1.28.0-0"
keywords:
  - myapp
  - alicloud
  - apsara-stack
home: https://example.com/myapp
sources:
  - https://github.com/example/myapp
maintainers:
  - name: SRE Team
    email: sre@example.com
dependencies:
  - name: postgresql
    version: "13.2.0"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
```

### 1.3 模板开发规范

| 规范项 | 要求 |
|:---|:---|
| 标签 | 必须包含 `app.kubernetes.io/name`、`app.kubernetes.io/instance`、`app.kubernetes.io/version`、`app.kubernetes.io/managed-by` |
| 资源名称 | 使用 `include "myapp.fullname" .` 生成，避免冲突 |
| 镜像仓库 | 优先使用阿里云/专有云镜像仓库地址 |
| 资源限制 | 必须设置 `resources.requests` 和 `resources.limits` |
| 健康检查 | 必须配置 `livenessProbe`、`readinessProbe`、`startupProbe` |
| PodDisruptionBudget | 生产环境必须配置 |
| 安全上下文 | 推荐配置 `securityContext.runAsNonRoot: true` |

---

<!-- chunk: 2. Values 分层管理 -->
## 2. Values 分层管理

### 2.1 分层策略

| 层级 | 文件 | 作用 |
|:---|:---|:---|
| 默认值 | `values.yaml` | 通用默认，可被覆盖 |
| 环境层 | `values-dev.yaml` | 开发环境覆盖 |
| 环境层 | `values-staging.yaml` | 预发环境覆盖 |
| 环境层 | `values-prod.yaml` | 生产环境覆盖 |
| 集群层 | `values-prod-ack-hz.yaml` | 特定集群覆盖 |
| 敏感值 | `secrets.yaml` (加密) | 密码、Token、证书 |

### 2.2 渲染命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 本地渲染检查
helm template myapp ./myapp \
  -f ./myapp/values.yaml \
  -f ./myapp/values-prod.yaml \
  --namespace production

# 安装/升级
helm upgrade --install myapp ./myapp \
  -f ./myapp/values.yaml \
  -f ./myapp/values-prod.yaml \
  -f ./myapp/secrets.yaml \
  --namespace production \
  --create-namespace \
  --atomic \
  --timeout 10m
```
### 2.3 Values 示例

```yaml
# values-prod.yaml
replicaCount: 3

image:
  repository: registry.cn-hangzhou.aliyuncs.com/myorg/myapp
  tag: "v2.1.0"
  pullPolicy: IfNotPresent

resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: 2000m
    memory: 2Gi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 20
  targetCPUUtilizationPercentage: 70

podDisruptionBudget:
  enabled: true
  minAvailable: 2
```

---

<!-- chunk: 3. Secret 加密 -->
## 3. Secret 加密：helm-secrets / SOPS

### 3.1 安装 helm-secrets

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm plugin install https://github.com/jkroepke/helm-secrets --version v4.5.1
```
### 3.2 SOPS 配置

```bash
mkdir -p .sops
cat > .sops.yaml <<EOF
creation_rules:
  - path_regex: secrets/.*\.yaml$
    kms: aliyun-kms://cn-hangzhou/<key-id>
EOF
```

### 3.3 加密 secrets 文件

```bash
vim secrets-prod.yaml
sops --encrypt --in-place secrets-prod.yaml
git add secrets-prod.yaml
```

### 3.4 Helm 使用加密 values

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm secrets upgrade --install myapp ./myapp \
  -f ./myapp/values-prod.yaml \
  -f ./myapp/secrets/secrets-prod.yaml \
  --namespace production
```
---

<!-- chunk: 4. Chart 测试 -->
## 4. Chart 测试

### 4.1 模板渲染测试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm lint ./myapp
helm template myapp ./myapp -f ./myapp/values-prod.yaml > /tmp/rendered.yaml
```
### 4.2 单元测试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm plugin install https://github.com/helm-unittest/helm-unittest.git
helm unittest ./myapp
```
### 4.3 测试 Pod 模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: "{{ include "myapp.fullname" . }}-test-connection"
  labels:
    {{- include "myapp.labels" . | nindent 4 }}
  annotations:
    "helm.sh/hook": test
spec:
  containers:
    - name: wget
      image: busybox:1.36
      command: ['wget']
      args: ['{{ include "myapp.fullname" . }}:{{ .Values.service.port }}']
  restartPolicy: Never
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm test myapp --namespace production
```
---

<!-- chunk: 5. 依赖管理 -->
## 5. 依赖管理

### 5.1 声明依赖

```yaml
dependencies:
  - name: postgresql
    version: "13.2.0"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
    alias: db
  - name: redis
    version: "18.0.0"
    repository: "https://charts.bitnami.com/bitnami"
    condition: redis.enabled
```

### 5.2 更新依赖

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm dependency update ./myapp
helm dependency build ./myapp
```
### 5.3 私有 Chart 仓库（阿里云/专有云 Harbor）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm repo add mycharts https://harbor.example.com/chartrepo/myrepo \
  --username <user> --password <pass>

helm package ./myapp
helm cm-push myapp-1.2.3.tgz mycharts
```
---

<!-- chunk: 6. 与 ArgoCD/Flux 集成 -->
## 6. 与 ArgoCD/Flux 集成

### 6.1 ArgoCD Application

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: myapp-prod
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://git.example.com/helm-charts.git
    targetRevision: main
    path: myapp
    helm:
      valueFiles:
        - values-prod.yaml
        - secrets://secrets/secrets-prod.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
      - CreateNamespace=true
```

### 6.2 Flux HelmRelease

```yaml
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: myapp
  namespace: production
spec:
  interval: 5m
  chart:
    spec:
      chart: myapp
      version: "1.2.3"
      sourceRef:
        kind: HelmRepository
        name: mycharts
        namespace: flux-system
  values:
    replicaCount: 3
    image:
      repository: registry.cn-hangzhou.aliyuncs.com/myorg/myapp
      tag: "v2.1.0"
```

---

<!-- chunk: 7. 回滚策略 -->
## 7. 回滚策略

### 7.1 Helm 原生回滚

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm history myapp -n production
helm rollback myapp 2 -n production --wait --timeout 10m
helm status myapp -n production
kubectl get pods -n production
```
### 7.2 升级时自动回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm upgrade --install myapp ./myapp \
  -f ./myapp/values-prod.yaml \
  --namespace production \
  --atomic \
  --cleanup-on-fail \
  --timeout 10m
```
### 7.3 GitOps 回滚

| 场景 | 操作 |
|:---|:---|
| 配置错误 | Git revert values 变更，ArgoCD 自动同步 |
| 镜像问题 | 回退 image.tag，重新触发同步 |
| chart 问题 | 回退 chart version，Flux/ArgoCD 自动降级 |
| 灾难性故障 | 直接 `helm rollback` 后锁定 Git 分支 |

---

<!-- chunk: 8. 阿里云/专有云场景 -->
## 8. 阿里云/专有云场景

### 8.1 镜像仓库选择

| 环境 | 推荐镜像仓库 |
|:---|:---|
| 阿里云 ACK | 阿里云容器镜像服务 ACR（个人版/企业版） |
| 专有云 Apsara Stack | 专有云 Harbor / ACR 企业版同步 |
| 跨地域部署 | ACR 企业版多地域同步实例 |

### 8.2 专有云平台适配

- 所有 Helm chart 镜像地址需替换为专有云内部 Harbor 地址
- `imagePullSecrets` 必须配置（专有云通常需要认证）
- Ingress 需适配专有云 SLB 与 DNS 体系
- StorageClass 使用专有云 CSI 名称，如 `alicloud-disk-essd-apsara`
- 监控对接专有云 ARMS/SLS/天基监控

### 8.3 阿里云 CLI 验证发布

```bash
aliyun cs GET /k8s/clusters/<cluster-id>/nodes
aliyun slb DescribeLoadBalancers --RegionId cn-hangzhou
```

---

<!-- chunk: 9. 发布审计与可追溯性 -->
## 9. 发布审计与可追溯性

### 9.1 Helm Release 标签规范

```yaml
{{- define "myapp.releaseLabels" -}}
app.kubernetes.io/managed-by: {{ .Release.Service }}
helm.sh/chart: {{ include "myapp.chart" . }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
release.revision: {{ .Release.Revision | quote }}
git.commit: {{ .Values.global.gitCommit | default "unknown" | quote }}
git.branch: {{ .Values.global.gitBranch | default "unknown" | quote }}
ci.pipeline: {{ .Values.global.ciPipelineId | default "unknown" | quote }}
{{- end }}
```

### 9.2 发布历史保留

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
helm history myapp -n production --max 20
```
---

<!-- chunk: 10. 多集群与专有云发布流水线 -->
## 10. 多集群与专有云发布流水线

### 10.1 多集群 values 管理

```
myapp/
└── values/
    ├── base.yaml
    ├── clusters/
    │   ├── ack-hangzhou-prod.yaml
    │   └── apsara-beijing-prod.yaml
    └── regions/
        ├── hangzhou.yaml
        └── beijing.yaml
```

### 10.2 CI/CD 发布脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
set -e
ENV=$1
CLUSTER=$2
CHART_VERSION=$3

aliyun cs GET /k8s/${CLUSTER}/user_config > /tmp/kubeconfig
export KUBECONFIG=/tmp/kubeconfig

helm upgrade --install myapp ./myapp \
  --version ${CHART_VERSION} \
  -f values/base.yaml \
  -f values/clusters/${CLUSTER}.yaml \
  -f secrets/${ENV}.yaml \
  --namespace production \
  --atomic \
  --timeout 15m

helm test myapp -n production
```
### 10.3 发布门禁

| 阶段 | 检查项 | 失败处理 |
|:---|:---|:---|
| 代码提交 | pre-commit helm lint | 阻断提交 |
| CI 构建 | helm unittest + template diff | 阻断合并 |
| 预发部署 | helm test + 冒烟测试 | 阻断上线 |
| 生产部署 | 双人复核 + 变更窗口 | 回滚 |

---

<!-- chunk: 11. 检查清单 -->
## 11. 发布变更检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| Chart 通过 lint | 无错误无警告 | `helm lint` |
| 模板渲染正确 | 与预期一致 | `helm template` |
| Values 分层清晰 | dev/staging/prod 分离 | 文件结构 |
| Secrets 已加密 | Git 中无明文 | `sops -d` 可解密 |
| 资源限制配置 | requests/limits 完整 | 渲染结果 |
| HPA/PDB 配置 | 生产环境启用 | 渲染结果 |
| 镜像仓库可访问 | 专有云 Harbor 可达 | `crictl pull` |
| 测试 Pod 通过 | `helm test` 成功 | 执行测试 |
| 回滚计划明确 | 已知上一个可用版本 | `helm history` |
| 灰度策略 | 金丝雀/蓝绿已配置 | Argo Rollouts/Flagger |

---

## Related

- [[发布变更/README.md|Release & Change Management Domain]]
- GitOps 目录

## See Also

- [[云厂商/01-alibaba-cloud/apsara-stack-components.md|专有云组件索引]]
- [[生产运维/ticket-routing-rules.md|工单分类与路由规则]]

---

## Helm 与阿里云 ACR 仓库完整示例

以下示例演示从 Chart 开发到推送到阿里云 ACR、再到 ACK 集群安装的完整流程。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 登录阿里云 ACR（杭州 VPC 域名）
helm registry login registry-vpc.cn-hangzhou.aliyuncs.com \
  --username ${ALIBABA_CLOUD_ACCESS_KEY_ID} \
  --password ${ALIBABA_CLOUD_ACCESS_KEY_SECRET}

# 2. 创建 ACR Chart 仓库命名空间
aliyun cr CreateNamespace --RegionId cn-hangzhou --NamespaceName platform-charts

# 3. 打包 Chart
helm package ./my-chart

# 4. 推送到 ACR OCI 仓库
helm push my-chart-1.2.3.tgz oci://registry-vpc.cn-hangzhou.aliyuncs.com/platform-charts

# 5. 在 ACK 集群中安装
helm upgrade --install my-app \
  oci://registry-vpc.cn-hangzhou.aliyuncs.com/platform-charts/my-chart \
  --version 1.2.3 \
  -f values.yaml \
  -f values-prod.yaml \
  -n production \
  --create-namespace

# 6. 验证
helm list -n production
kubectl get pods -n production
```
### 专有云 Harbor 仓库示例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加 Harbor chart 仓库
helm repo add apsara-platform https://harbor.apsara.example.com/chartrepo/platform
helm repo update

# 推送 Chart
curl -u ${HARBOR_USER}:${HARBOR_PASS} \
  -X POST https://harbor.apsara.example.com/api/chartrepo/platform/charts \
  -F "chart=@my-chart-1.2.3.tgz"

# 安装
helm upgrade --install my-app apsara-platform/my-chart \
  -f values-prod.yaml -n production

```
---

## Helm 故障排查速查

| 现象 | 可能原因 | 排查命令 |
|:---|:---|:---|
| `helm install` 超时 | 镜像拉取慢或 Pod 未就绪 | `helm status <release>`、`kubectl get pods` |
| `template` 报错 | values 类型不匹配或缺少必填项 | `helm lint`、`helm template --debug` |
| `rollback` 失败 | 历史版本被清理或 CRD 不兼容 | `helm history <release>` |
| ArgoCD OutOfSync | Helm values 与 Git 不一致 | 检查 valueFiles 路径与内容 |

---

## Helm 生产发布完整流程示例

以下是在阿里云 ACK 环境中，从 Chart 打包到生产发布的完整命令流程。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 本地验证
helm lint ./my-chart
helm template my-app ./my-chart -f ./my-chart/values-prod.yaml > /tmp/rendered.yaml
kubectl apply --dry-run=client -f /tmp/rendered.yaml

# 2. 运行 chart test
helm test my-app -n staging

# 3. 打包并签名
helm package --sign ./my-chart --key platform@example.com

# 4. 推送到 ACR
helm push my-chart-1.2.3.tgz oci://registry-vpc.cn-hangzhou.aliyuncs.com/platform-charts

# 5. 生产安装/升级
helm upgrade --install my-app \
  oci://registry-vpc.cn-hangzhou.aliyuncs.com/platform-charts/my-chart \
  --version 1.2.3 \
  -f values-prod.yaml \
  -n production \
  --create-namespace \
  --atomic \
  --timeout 10m

# 6. 验证
helm status my-app -n production
kubectl rollout status deployment/my-app -n production
```
### 专有云发布注意事项

- 镜像与 Chart 必须提前同步到专有云 Harbor/ACR。
- 生产发布需经过变更审批窗口，禁止高峰期变更。
- 发布前确认备份与回滚方案可用。
- 发布后立即检查业务关键指标与告警。

---

## Helm 与 OPA/Kyverno 策略集成

可在 Chart 中配置 Pod Security Context 与 NetworkPolicy，并通过 OPA/Kyverno 强制校验。

```yaml
# templates/networkpolicy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: {{ include "my-chart.fullname" . }}
spec:
  podSelector:
    matchLabels:
      {{- include "my-chart.selectorLabels" . | nindent 6 }}
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: {{ .Release.Namespace }}
      ports:
        - protocol: TCP
          port: {{ .Values.service.port }}
```

```yaml
# Kyverno ClusterPolicy 示例
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-non-root
spec:
  validationFailureAction: Enforce
  rules:
    - name: check-run-as-non-root
      match:
        resources:
          kinds:
            - Pod
      validate:
        message: "Pod 必须设置 runAsNonRoot: true"
        pattern:
          spec:
            securityContext:
              runAsNonRoot: "true"
```

---

## Helm 与 Flux Kustomize 集成

Flux 除了 HelmRelease，也支持通过 Kustomize 引用 Helm Chart，适合需要额外补丁或多仓库场景。

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
  - helmrelease.yaml
  - helmrepository.yaml
configMapGenerator:
  - name: my-chart-values
    behavior: merge
    files:
      - values-prod.yaml
```

### 阿里云 CodeUp 作为 Git 源

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: helm-gitops
  namespace: flux-system
spec:
  interval: 1m
  url: https://codeup.aliyun.com/platform/helm-gitops.git
  ref:
    branch: main
  secretRef:
    name: codeup-creds
```

通过 Flux 的 GitRepository + HelmRelease 组合，可实现 Chart 与 values 分离、环境隔离、自动同步。

---

## Helm 历史版本管理与 ConfigMap 清理

Helm 默认保留所有 Release 历史，历史过多会导致 ConfigMap 超限。生产环境建议限制历史版本数量。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装时限制历史版本
helm upgrade --install my-app ./my-chart --history-max 10

# 清理旧历史
helm history my-app -n production
helm rollback my-app 5 -n production

# 自动清理 Job 与测试 Pod
kubectl delete pod -n production -l helm.sh/hook=test-success
```
建议将 `--history-max` 写入 CI/CD 模板，避免默认无限制增长。

---

## 总结

本文档覆盖了 Helm Chart 开发、values 分层、Secret 加密、测试、依赖、GitOps 集成、回滚及阿里云/专有云发布实践。遵循这些规范可显著提升发布稳定性与可维护性。

```

<!-- risk-assessed -->
