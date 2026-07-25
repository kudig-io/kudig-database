---
title: "Helm 生产模式"
description: "Helm Chart 生产级实践：Chart 开发规范、Values 管理策略、helm-secrets 加密、Chart 测试、Library Charts 与 OCI Registry 分发"
summary: "全面覆盖 Helm 在生产环境中的最佳实践，包括 Chart 开发规范与模板设计、多环境 Values 管理策略、helm-secrets/SOPS 敏感信息加密、ct/chart-testing 自动化测试、Library Charts 复用模式以及 OCI Registry 分发与版本管理"
category: 发布变更
tags:
- helm
- chart-development
- values-management
- helm-secrets
- chart-testing
- library-charts
- oci-registry
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Helm Chart 生产环境开发规范是什么"
- "Helm 多环境 Values 如何管理"
- "helm-secrets 如何加密敏感配置"
trigger_keywords:
- helm
- chart
- values
- helm-secrets
- library-chart
- oci-registry
- chart-testing
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

# Helm 生产模式

## 概述

Helm 是 Kubernetes 生态中最主流的包管理和应用部署工具。然而，从"能用"到"生产级"之间存在巨大鸿沟——模板设计不当导致升级失败、Values 管理混乱导致环境漂移、敏感信息明文存储导致安全风险、缺乏测试导致 Chart 变更引入回归。

本文提供经过大规模生产验证的 Helm 实践模式，覆盖 Chart 开发规范、Values 管理策略、敏感信息加密、自动化测试、模板复用和 OCI 分发。与 [[11-发布变更/01-GitOps/99-helm-production-guide.md|Helm 生产指南]] 侧重基础操作不同，本文聚焦于企业级 Chart 工程化实践。

## 核心概念

### Chart 工程化架构

```
┌─────────────────────────────────────────────────────────────────┐
│                  Helm Chart 工程化架构                            │
│                                                                   │
│  开发层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Library Charts (公共模板库)                               │    │
│  │  • _helpers.tpl (命名规范、标签生成)                       │    │
│  │  • _resources.tpl (Deployment/Service/Ingress 模板)       │    │
│  │  • _security.tpl (SecurityContext/NetworkPolicy)          │    │
│  └──────────────────────────────────────────────────────────┘    │
│                          │ 依赖引用                               │
│                          ▼                                        │
│  应用层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Application Charts                                        │    │
│  │  • Chart.yaml (元数据 + 依赖声明)                          │    │
│  │  • values.yaml (默认值)                                    │    │
│  │  • templates/ (应用特定模板)                               │    │
│  │  • values-staging.yaml / values-production.yaml            │    │
│  └──────────────────────────────────────────────────────────┘    │
│                          │                                        │
│                          ▼                                        │
│  分发层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  OCI Registry (harbor.internal/charts)                     │    │
│  │  • 语义化版本 (SemVer)                                     │    │
│  │  • 不可变 Tag (生产版本)                                   │    │
│  │  • 签名验证 (cosign/notation)                              │    │
│  └──────────────────────────────────────────────────────────┘    │
│                          │                                        │
│                          ▼                                        │
│  部署层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  GitOps (Argo CD / Flux)                                   │    │
│  │  • helm-secrets 解密                                       │    │
│  │  • 多环境 Values 覆盖                                      │    │
│  │  • 自动化 Rollback                                         │    │
│  └──────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

### Values 管理策略对比

| 策略 | 适用场景 | 优点 | 缺点 |
|------|---------|------|------|
| 单 values.yaml + --set | 开发/测试 | 简单直接 | 环境差异难管理 |
| 多 values 文件 (values-{env}.yaml) | 多环境部署 | 环境隔离清晰 | 文件数量多 |
| Kustomize + Helm | 复杂覆盖需求 | 声明式、可审计 | 学习曲线陡 |
| Helmfile | 多 Chart 编排 | 统一入口、依赖管理 | 额外工具依赖 |
| Argo CD Application + Helm | GitOps 流程 | 与 GitOps 深度集成 | 需要 Argo CD |

### Chart 版本管理策略

| 版本类型 | 格式 | 含义 | 示例 |
|---------|------|------|------|
| Chart Version | SemVer | Chart 模板/结构变更 | 1.2.0 → 1.3.0 |
| App Version | 自定义 | 应用镜像版本 | v2.5.1 |
| 生产 Tag | 不可变 | 经过验证的发布版本 | 1.2.0-prod-20260719 |
| 开发 Tag | 可变 | 开发中的快照 | 1.3.0-rc.1 |

## 生产部署/实现

### Chart 开发规范模板

生产级 Chart 的标准结构和模板设计：

```yaml
# 🟢 低风险：Chart 模板文件，不直接影响集群
# Chart.yaml - 规范的元数据声明
apiVersion: v2
name: payment-service
description: Payment processing microservice with multi-provider support
type: application
version: 2.1.0
appVersion: "4.2.0"
kubeVersion: ">=1.28.0-0"
home: https://github.com/internal/payment-service
sources:
- https://github.com/internal/payment-service
maintainers:
- name: Platform Team
  email: platform@company.com
keywords:
- payment
- microservice
dependencies:
- name: common
  version: "1.x.x"
  repository: "oci://harbor.internal/charts/library"
  tags:
  - backend-service
---
# values.yaml - 结构化的默认值（带注释说明）
# 生产环境通过 values-production.yaml 覆盖
replicaCount: 3

image:
  repository: registry.internal/payment-service
  tag: ""  # 默认使用 appVersion
  pullPolicy: IfNotPresent
  pullSecrets:
  - name: registry-credentials

service:
  type: ClusterIP
  port: 8080
  metricsPort: 9090

resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: "2"
    memory: 2Gi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 20
  targetCPUUtilizationPercentage: 70
  targetMemoryUtilizationPercentage: 80

podDisruptionBudget:
  enabled: true
  minAvailable: 2

securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 1000
  seccompProfile:
    type: RuntimeDefault

containerSecurityContext:
  allowPrivilegeEscalation: false
  readOnlyRootFilesystem: true
  capabilities:
    drop:
    - ALL

probes:
  liveness:
    path: /healthz
    initialDelaySeconds: 15
    periodSeconds: 10
    failureThreshold: 3
  readiness:
    path: /readyz
    initialDelaySeconds: 5
    periodSeconds: 5
    failureThreshold: 3
  startup:
    path: /healthz
    failureThreshold: 30
    periodSeconds: 2

serviceMonitor:
  enabled: true
  interval: 30s
  path: /metrics

networkPolicy:
  enabled: true
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
    ports:
    - port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: production
    ports:
    - port: 5432
    - port: 6379
```

### helm-secrets 敏感信息管理

```yaml
# 🔴 高风险：涉及敏感信息加密，密钥管理不当会导致数据泄露或不可用
# .sops.yaml - SOPS 加密规则配置
creation_rules:
- path_regex: secrets/.*\.yaml$
  encrypted_regex: ^(data|stringData)$
  age: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p
- path_regex: values-production\.yaml$
  encrypted_regex: ^(password|token|secret|key)$
  age: age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p
---
# secrets/database.yaml (加密前)
apiVersion: v1
kind: Secret
metadata:
  name: payment-db-credentials
  namespace: production
type: Opaque
stringData:
  username: payment_svc
  password: "ENC[AES256_GCM,data:xxx,type:str]"  # SOPS 加密后的值
  host: "pg-primary.database.svc"
---
# 加密操作
# helm secrets encrypt secrets/database.yaml > secrets/database.enc.yaml
#
# 在 Argo CD 中使用（通过 helm-secrets 插件）
# helm secrets -f secrets/database.enc.yaml upgrade payment-service ./charts/payment-service
```

### 多环境 Values 管理

```yaml
# 🟡 中风险：生产环境 Values 变更会触发重新部署
# values-production.yaml - 生产环境覆盖值
replicaCount: 10

image:
  tag: "4.2.0"
  pullPolicy: Always

resources:
  requests:
    cpu: "1"
    memory: 1Gi
  limits:
    cpu: "4"
    memory: 4Gi

autoscaling:
  enabled: true
  minReplicas: 10
  maxReplicas: 50
  targetCPUUtilizationPercentage: 65

podDisruptionBudget:
  minAvailable: 8

# 生产环境特有配置
topologySpreadConstraints:
- maxSkew: 1
  topologyKey: topology.kubernetes.io/zone
  whenUnsatisfiable: DoNotSchedule
  labelSelector:
    matchLabels:
      app.kubernetes.io/name: payment-service

affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app.kubernetes.io/name: payment-service
      topologyKey: kubernetes.io/hostname

# 生产环境 tolerations（专用节点池）
tolerations:
- key: dedicated
  operator: Equal
  value: payment
  effect: NoSchedule

nodeSelector:
  workload-type: payment
---
# values-staging.yaml - 预发布环境覆盖值
replicaCount: 3

image:
  tag: "4.2.0-rc.1"

resources:
  requests:
    cpu: 250m
    memory: 512Mi
  limits:
    cpu: "1"
    memory: 1Gi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 10
```

### Library Chart 复用模式

```yaml
# 🟢 低风险：Library Chart 模板定义
# charts/common/templates/_deployment.tpl
{{- define "common.deployment" -}}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "common.fullname" . }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
spec:
  {{- if not .Values.autoscaling.enabled }}
  replicas: {{ .Values.replicaCount }}
  {{- end }}
  selector:
    matchLabels:
      {{- include "common.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "common.selectorLabels" . | nindent 8 }}
      annotations:
        checksum/config: {{ include (print $.Template.BasePath "/configmap.yaml") . | sha256sum }}
        checksum/secret: {{ include (print $.Template.BasePath "/secret.yaml") . | sha256sum }}
    spec:
      {{- with .Values.image.pullSecrets }}
      imagePullSecrets:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      serviceAccountName: {{ include "common.serviceAccountName" . }}
      securityContext:
        {{- toYaml .Values.securityContext | nindent 8 }}
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag | default .Chart.AppVersion }}"
        imagePullPolicy: {{ .Values.image.pullPolicy }}
        securityContext:
          {{- toYaml .Values.containerSecurityContext | nindent 10 }}
        ports:
        - name: http
          containerPort: {{ .Values.service.port }}
          protocol: TCP
        - name: metrics
          containerPort: {{ .Values.service.metricsPort }}
          protocol: TCP
        livenessProbe:
          httpGet:
            path: {{ .Values.probes.liveness.path }}
            port: http
          initialDelaySeconds: {{ .Values.probes.liveness.initialDelaySeconds }}
          periodSeconds: {{ .Values.probes.liveness.periodSeconds }}
          failureThreshold: {{ .Values.probes.liveness.failureThreshold }}
        readinessProbe:
          httpGet:
            path: {{ .Values.probes.readiness.path }}
            port: http
          initialDelaySeconds: {{ .Values.probes.readiness.initialDelaySeconds }}
          periodSeconds: {{ .Values.probes.readiness.periodSeconds }}
          failureThreshold: {{ .Values.probes.readiness.failureThreshold }}
        resources:
          {{- toYaml .Values.resources | nindent 10 }}
        volumeMounts:
        - name: tmp
          mountPath: /tmp
      volumes:
      - name: tmp
        emptyDir: {}
      {{- with .Values.nodeSelector }}
      nodeSelector:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      {{- with .Values.tolerations }}
      tolerations:
        {{- toYaml . | nindent 8 }}
      {{- end }}
{{- end -}}
```

### OCI Registry 分发

```bash
# 🟡 中风险：推送 Chart 到 Registry 影响部署源
# 登录 OCI Registry
helm registry login harbor.internal -u $HARBOR_USER -p $HARBOR_TOKEN

# 打包 Chart
helm package ./charts/payment-service --destination ./dist/

# 推送到 OCI Registry
helm push ./dist/payment-service-2.1.0.tgz oci://harbor.internal/charts

# 从 OCI Registry 安装
helm install payment-service oci://harbor.internal/charts/payment-service \
  --version 2.1.0 \
  -f values-production.yaml \
  -n production

# 使用 cosign 签名 Chart（供应链安全）
cosign sign harbor.internal/charts/payment-service:2.1.0

# 验证签名
cosign verify harbor.internal/charts/payment-service:2.1.0
```

## 运维操作

### Chart 测试

```bash
# 🟢 低风险：只读测试操作
# 模板渲染验证（不实际部署）
helm template payment-service ./charts/payment-service \
  -f values-production.yaml \
  --namespace production \
  --debug

# Lint 检查
helm lint ./charts/payment-service -f values-production.yaml

# 使用 chart-testing 进行完整测试
ct lint --charts ./charts/payment-service --target-branch main
ct install --charts ./charts/payment-service --namespace test-helm

# 单元测试（helm-unittest 插件）
helm unittest ./charts/payment-service

# 验证 Chart 依赖
helm dependency update ./charts/payment-service
helm dependency list ./charts/payment-service
```

### 生产升级操作

```bash
# 🟡 中风险：生产环境 Helm 升级
# 升级前：查看当前部署状态
helm list -n production
helm get values payment-service -n production
helm history payment-service -n production

# 升级前：dry-run 验证
helm upgrade payment-service ./charts/payment-service \
  -f values-production.yaml \
  --namespace production \
  --dry-run --debug

# 执行升级（带原子回滚）
helm upgrade payment-service ./charts/payment-service \
  -f values-production.yaml \
  --namespace production \
  --atomic \
  --timeout 10m \
  --wait

# 升级后验证
helm status payment-service -n production
kubectl get pods -n production -l app.kubernetes.io/name=payment-service
```

### 回滚操作

```bash
# 🔴 高风险：回滚会恢复到旧版本
# 查看历史版本
helm history payment-service -n production

# 回滚到上一个版本
helm rollback payment-service -n production

# 回滚到指定版本
helm rollback payment-service 15 -n production --wait --timeout 5m

# 回滚后验证
helm status payment-service -n production
kubectl rollout status deployment/payment-service -n production
```

## 故障排查

### Chart 渲染错误

```bash
# 🟢 低风险：只读诊断
# 查看详细的模板渲染错误
helm template payment-service ./charts/payment-service \
  -f values-production.yaml \
  --debug 2>&1 | tail -50

# 检查 values 类型错误（常见：字符串 vs 数字）
helm template payment-service ./charts/payment-service \
  -f values-production.yaml \
  --show-only templates/deployment.yaml

# 验证 Chart 依赖是否完整
helm dependency list ./charts/payment-service
ls ./charts/payment-service/charts/
```

### 升级失败排查

```bash
# 🟢 低风险：只读诊断
# 查看 Helm Release 状态
helm status payment-service -n production
helm get manifest payment-service -n production | head -50

# 查看升级事件
kubectl get events -n production --sort-by='.lastTimestamp' | grep -i "payment\|helm"

# 检查 Pod 启动失败原因
kubectl describe pod -n production -l app.kubernetes.io/name=payment-service | grep -A10 "Events:"
kubectl logs -n production -l app.kubernetes.io/name=payment-service --previous --tail=50

# 对比当前部署与期望状态
helm get manifest payment-service -n production > /tmp/expected.yaml
kubectl get all -n production -l app.kubernetes.io/name=payment-service -o yaml > /tmp/actual.yaml
diff /tmp/expected.yaml /tmp/actual.yaml
```

### Secrets 解密失败

```bash
# 🟢 低风险：只读诊断
# 检查 SOPS 密钥是否可用
sops --decrypt secrets/database.enc.yaml > /dev/null 2>&1 && echo "OK" || echo "FAILED"

# 检查 age 密钥文件
ls -la ~/.config/sops/age/keys.txt

# 验证加密文件的元数据
sops filestatus secrets/database.enc.yaml
```

## 最佳实践

### Chart 开发规范

1. **模板必须幂等**：多次 `helm upgrade` 结果一致，避免使用 `randAlphaNum` 等非确定性函数。

2. **所有资源必须有标签**：使用 `helm.sh/chart`、`app.kubernetes.io/version`、`app.kubernetes.io/managed-by` 标准标签。

3. **ConfigMap/Secret 变更触发重启**：通过 `checksum/config` annotation 确保配置变更时 Pod 自动重启。

4. **资源限制必须设置**：所有容器必须声明 requests 和 limits，避免资源争抢。

5. **安全默认值**：默认启用 `runAsNonRoot`、`readOnlyRootFilesystem`、`drop ALL capabilities`。

### CI/CD 集成

Chart 变更应集成到 [[11-发布变更/01-GitOps/08-cicd-pipeline-patterns.md|CI/CD 流水线]]：
- PR 阶段：`helm lint` + `ct lint` + `helm unittest`
- Merge 阶段：`helm template` 渲染验证 + `ct install` 到测试集群
- Release 阶段：`helm package` + `helm push` 到 OCI Registry + `cosign sign`
- Deploy 阶段：Argo CD 同步或 `helm upgrade --atomic`

### 与 GitOps 集成

Helm Chart 与 [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops.md|Argo CD]] 集成时：
- Chart 源码存储在应用仓库
- 打包后的 Chart 存储在 OCI Registry
- Argo CD Application 引用 OCI Registry 中的 Chart
- 环境差异通过 Argo CD 的 `helm.values` 或独立 values 文件管理
- 敏感信息通过 helm-secrets 插件在 Argo CD 侧解密

## Related

- [[11-发布变更/01-GitOps/99-helm-production-guide.md|Helm 生产指南]]
- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops.md|Argo CD 企业级 GitOps]]
- [[11-发布变更/01-GitOps/08-cicd-pipeline-patterns.md|CI/CD 流水线模式]]
- [[11-发布变更/01-GitOps/07-gitops-security-compliance.md|GitOps 安全合规]]
- [[11-发布变更/01-GitOps/09-argo-rollouts-progressive-delivery.md|Argo Rollouts 渐进式交付]]
- [[11-发布变更/04-变更管理/07-rollback-automation-patterns.md|回滚自动化模式]]
- [[11-发布变更/01-GitOps/06-flux-gitops-continuous-delivery.md|Flux GitOps 持续交付]]
