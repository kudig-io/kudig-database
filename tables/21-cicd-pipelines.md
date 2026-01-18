# 21 - CI/CD管道表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [argoproj.github.io/argo-cd](https://argoproj.github.io/argo-cd/)

## CI/CD工具对比(生产环境决策矩阵)

| 工具 | 类型 | 特点 | K8S集成 | 学习曲线 | ACK集成 | 社区活跃度 | 企业支持 | 典型用户规模 |
|-----|------|------|--------|---------|---------|-----------|---------|------------|
| **ArgoCD** | GitOps CD | 声明式,可视化强,多集群管理 | 原生 | 中 | 支持 | ⭐⭐⭐⭐⭐ | 商业版 | 1000+ Apps |
| **Flux** | GitOps CD | 轻量,CNCF项目,Helm友好 | 原生 | 低 | 支持 | ⭐⭐⭐⭐⭐ | 社区 | 500+ Apps |
| **Tekton** | K8S原生CI/CD | 灵活,云原生,CRD定义 | 原生 | 高 | 支持 | ⭐⭐⭐⭐ | IBM/Red Hat | 复杂流水线 |
| **Jenkins** | CI/CD | 成熟生态,插件丰富(2000+) | 插件 | 中 | 支持 | ⭐⭐⭐⭐⭐ | CloudBees | 企业级CI |
| **Jenkins X** | K8S原生CI/CD | GitOps+预览环境,自动化强 | 原生 | 高 | 部分 | ⭐⭐⭐ | CloudBees | 100+ Apps |
| **GitLab CI** | 一体化CI/CD | 代码+CI一体,UI友好 | Runner | 中 | 支持 | ⭐⭐⭐⭐⭐ | GitLab Inc | 全能平台 |
| **GitHub Actions** | CI/CD | GitHub原生,Marketplace丰富 | Action | 低 | 支持 | ⭐⭐⭐⭐⭐ | GitHub | 开源项目 |
| **云效** | 阿里云CI/CD | 云原生集成,低门槛,中文文档 | 原生 | 低 | 原生 | ⭐⭐⭐⭐ | 阿里云 | 中小企业 |
| **Spinnaker** | 多云CD | 多云部署,Netflix开源 | 原生 | 高 | 支持 | ⭐⭐⭐ | Netflix | 大规模部署 |

## GitOps决策树

```
是否需要GitOps? 
├─ 是 → 团队规模?
│   ├─ <50人 → Flux(轻量简单)
│   ├─ 50-200人 → ArgoCD(功能完整,UI强大)
│   └─ >200人 → ArgoCD + ApplicationSet(多租户)
├─ 否 → 已有CI工具?
    ├─ GitHub → GitHub Actions + kubectl
    ├─ GitLab → GitLab CI + kubectl
    ├─ Jenkins → Jenkins + Kubernetes Plugin
    └─ 无 → 云效(ACK原生,快速上手)
```

## ArgoCD生产级配置

### ArgoCD高可用部署

```yaml
# ArgoCD生产级安装(HA模式)
apiVersion: v1
kind: Namespace
metadata:
  name: argocd
---
# 使用HA安装清单
# kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/ha/install.yaml

# 自定义配置 - argocd-cm ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cm
  namespace: argocd
data:
  # 启用匿名访问(只读)
  users.anonymous.enabled: "false"
  # Git仓库超时
  timeout.reconciliation: "180s"
  # 应用健康评估
  resource.customizations: |
    argoproj.io/Rollout:
      health.lua: |
        hs = {}
        if obj.status ~= nil then
          if obj.status.phase == "Healthy" then
            hs.status = "Healthy"
            hs.message = obj.status.message
            return hs
          end
        end
        hs.status = "Progressing"
        hs.message = "Waiting for rollout to finish"
        return hs
  # Webhook配置
  webhook.github.secret: "github-webhook-secret"
  # 单点登录(SSO) - OIDC示例
  dex.config: |
    connectors:
      - type: oidc
        id: oidc
        name: OIDC
        config:
          issuer: https://your-oidc-provider.com
          clientID: $oidc.clientId
          clientSecret: $oidc.clientSecret
          requestedScopes: ["openid", "profile", "email", "groups"]
---
# RBAC配置 - argocd-rbac-cm
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-rbac-cm
  namespace: argocd
data:
  # 默认策略: 只读
  policy.default: role:readonly
  # 自定义角色和权限
  policy.csv: |
    # 平台管理员 - 全部权限
    p, role:platform-admin, applications, *, */*, allow
    p, role:platform-admin, clusters, *, *, allow
    p, role:platform-admin, repositories, *, *, allow
    g, platform-team, role:platform-admin
    
    # 项目管理员 - 项目内全部权限
    p, role:project-admin, applications, *, {project}}/*, allow
    p, role:project-admin, repositories, get, *, allow
    g, project-a-admin, role:project-admin
    
    # 开发者 - 同步和查看
    p, role:developer, applications, sync, {project}}/*, allow
    p, role:developer, applications, get, {project}}/*, allow
    g, dev-team, role:developer
    
    # 只读用户
    p, role:readonly, applications, get, */*, allow
    g, viewer-team, role:readonly
---
# Application定义 - 单应用示例
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: myapp-production
  namespace: argocd
  # Finalizer确保删除时同步删除K8s资源
  finalizers:
    - resources-finalizer.argocd.argoproj.io
  labels:
    environment: production
    team: backend
spec:
  project: production-apps
  
  # Git源配置
  source:
    repoURL: https://github.com/org/repo.git
    targetRevision: release-v1.2.3  # 使用Tag/Branch/Commit
    path: manifests/production
    
    # Helm Chart支持
    helm:
      releaseName: myapp
      valueFiles:
        - values-prod.yaml
        - values-prod-secrets.yaml  # 敏感配置分离
      # 参数覆盖
      parameters:
        - name: image.tag
          value: "v1.2.3"
        - name: replicaCount
          value: "5"
      # values.yaml覆盖
      values: |
        ingress:
          enabled: true
          annotations:
            cert-manager.io/cluster-issuer: letsencrypt-prod
    
    # Kustomize支持
    # kustomize:
    #   namePrefix: prod-
    #   commonLabels:
    #     environment: production
    #   images:
    #     - name: myapp
    #       newTag: v1.2.3
  
  # 目标集群和命名空间
  destination:
    server: https://kubernetes.default.svc  # 本集群
    # server: https://remote-cluster-api:6443  # 远程集群
    namespace: production
  
  # 同步策略
  syncPolicy:
    automated:
      prune: true      # 删除Git中不存在的资源
      selfHeal: true   # 自动修复配置漂移
      allowEmpty: false  # 不允许空目录
    syncOptions:
      - CreateNamespace=true  # 自动创建namespace
      - PruneLast=true        # 最后删除资源(避免依赖问题)
      - ApplyOutOfSyncOnly=true  # 仅同步不一致资源
      - ServerSideApply=true  # 使用Server-Side Apply(v1.25+)
      - RespectIgnoreDifferences=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  
  # 忽略差异(避免频繁同步)
  ignoreDifferences:
    - group: apps
      kind: Deployment
      jsonPointers:
        - /spec/replicas  # 忽略副本数差异(HPA控制)
    - group: apps
      kind: StatefulSet
      jsonPointers:
        - /spec/volumeClaimTemplates  # 忽略PVC差异
  
  # 健康评估自定义
  revisionHistoryLimit: 10
  
  # 通知配置
  info:
    - name: "Slack Channel"
      value: "#team-backend-alerts"
    - name: "Oncall"
      value: "https://oncall.example.com/backend"
---
# ApplicationSet - 多环境多集群管理
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: microservices-suite
  namespace: argocd
spec:
  # List生成器 - 显式定义
  generators:
    # 环境 x 集群矩阵
    - matrix:
        generators:
          # 环境列表
          - list:
              elements:
                - env: dev
                  replicas: "1"
                  resources: small
                - env: staging
                  replicas: "2"
                  resources: medium
                - env: production
                  replicas: "5"
                  resources: large
          # 集群列表
          - list:
              elements:
                - cluster: us-east-1
                  url: https://k8s-us-east-1.example.com
                - cluster: us-west-2
                  url: https://k8s-us-west-2.example.com
                - cluster: eu-central-1
                  url: https://k8s-eu-central-1.example.com
  
  # Git目录生成器(适合多服务)
  # generators:
  #   - git:
  #       repoURL: https://github.com/org/repo.git
  #       revision: HEAD
  #       directories:
  #         - path: services/*
  
  template:
    metadata:
      name: 'microservice-{{env}}-{{cluster}}'
      labels:
        environment: '{{env}}'
        region: '{{cluster}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/org/repo.git
        targetRevision: HEAD
        path: 'manifests/{{env}}'
        helm:
          releaseName: 'app-{{env}}'
          parameters:
            - name: replicaCount
              value: '{{replicas}}'
            - name: resources.preset
              value: '{{resources}}'
      destination:
        server: '{{url}}'
        namespace: '{{env}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
---
# AppProject - 项目隔离和权限控制
apiVersion: argoproj.io/v1alpha1
kind: AppProject
metadata:
  name: production-apps
  namespace: argocd
spec:
  description: "生产环境应用项目"
  
  # 源仓库白名单
  sourceRepos:
    - 'https://github.com/org/repo.git'
    - 'https://github.com/org/helm-charts.git'
  
  # 目标集群和命名空间白名单
  destinations:
    - namespace: 'production'
      server: https://kubernetes.default.svc
    - namespace: 'staging'
      server: https://kubernetes.default.svc
  
  # 允许部署的K8s资源类型
  clusterResourceWhitelist:
    - group: ''
      kind: Namespace
    - group: 'rbac.authorization.k8s.io'
      kind: ClusterRole
  
  # 命名空间资源白名单
  namespaceResourceWhitelist:
    - group: '*'
      kind: '*'
  
  # 拒绝的资源类型(安全加固)
  namespaceResourceBlacklist:
    - group: ''
      kind: ResourceQuota
    - group: ''
      kind: LimitRange
  
  # 同步窗口(维护窗口)
  syncWindows:
    - kind: allow
      schedule: '0 8-20 * * 1-5'  # 工作日8:00-20:00
      duration: 12h
      applications:
        - '*'
      manualSync: true  # 允许手动同步
    - kind: deny
      schedule: '0 20-8 * * *'  # 夜间禁止同步
      duration: 12h
      applications:
        - 'critical-*'
  
  # 孤立资源警告(资源不在Git中)
  orphanedResources:
    warn: true
```

### ArgoCD多集群管理

```bash
# 添加外部集群
argocd cluster add <context-name> --name prod-cluster-us

# 列出集群
argocd cluster list

# 配置集群凭证
kubectl create secret generic cluster-credentials \
  -n argocd \
  --from-literal=server=https://remote-cluster:6443 \
  --from-literal=config='{"bearerToken":"xxx","tlsClientConfig":{"caData":"xxx"}}'
```

## Flux CD生产级配置

```bash
# Flux安装(HA模式)
flux bootstrap github \
  --owner=my-org \
  --repository=fleet-infra \
  --branch=main \
  --path=./clusters/production \
  --personal \
  --components-extra=image-reflector-controller,image-automation-controller
```

```yaml
# GitRepository源 - 带认证
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: myapp-repo
  namespace: flux-system
spec:
  interval: 1m
  url: https://github.com/org/repo
  ref:
    branch: main
    # tag: v1.0.0  # 或使用Tag
    # semver: ">=1.0.0 <2.0.0"  # 语义化版本
  secretRef:
    name: git-credentials
  timeout: 60s
  ignore: |
    # 忽略特定文件
    /**/README.md
    /**/.gitignore
---
# OCIRepository - OCI镜像仓库(Helm Chart)
apiVersion: source.toolkit.fluxcd.io/v1beta2
kind: OCIRepository
metadata:
  name: myapp-chart
  namespace: flux-system
spec:
  interval: 5m
  url: oci://registry.example.com/charts/myapp
  ref:
    semver: "1.x"
  secretRef:
    name: oci-credentials
---
# Kustomization - 应用部署
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: myapp-production
  namespace: flux-system
spec:
  interval: 10m
  retryInterval: 1m
  timeout: 5m
  
  sourceRef:
    kind: GitRepository
    name: myapp-repo
  
  path: ./manifests/production
  
  # Kustomize配置
  prune: true
  wait: true  # 等待所有资源就绪
  force: false  # 强制替换资源(谨慎使用)
  
  # 健康检查
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: myapp
      namespace: production
    - apiVersion: apps/v1
      kind: StatefulSet
      name: database
      namespace: production
  
  # 依赖关系(先部署基础设施)
  dependsOn:
    - name: infra-controllers
    - name: cert-manager
  
  # 变量替换(后处理)
  postBuild:
    substitute:
      CLUSTER_NAME: "prod-us-east-1"
      ENVIRONMENT: "production"
    substituteFrom:
      - kind: ConfigMap
        name: cluster-settings
      - kind: Secret
        name: cluster-secrets
  
  # 通知(集成Notification Controller)
  # healthChecks, 失败时触发告警
---
# HelmRelease - Helm Chart部署
apiVersion: helm.toolkit.fluxcd.io/v2beta1
kind: HelmRelease
metadata:
  name: myapp
  namespace: production
spec:
  interval: 5m
  timeout: 10m
  
  chart:
    spec:
      chart: myapp
      version: '1.x'  # 语义化版本
      sourceRef:
        kind: HelmRepository
        name: myrepo
        namespace: flux-system
      interval: 1m
  
  # Helm Values
  values:
    replicaCount: 5
    image:
      repository: registry.example.com/myapp
      tag: v1.2.3
    ingress:
      enabled: true
      className: nginx
      hosts:
        - host: app.example.com
          paths:
            - path: /
              pathType: Prefix
  
  # Values来源(可多个)
  valuesFrom:
    - kind: ConfigMap
      name: myapp-config
      valuesKey: values.yaml
    - kind: Secret
      name: myapp-secrets
      valuesKey: secrets.yaml
  
  # 安装/升级配置
  install:
    crds: CreateReplace  # CRD处理策略
    remediation:
      retries: 3
  upgrade:
    crds: CreateReplace
    remediation:
      retries: 3
      remediateLastFailure: true
    cleanupOnFail: true
  
  # 测试(Helm Test)
  test:
    enable: true
    ignoreFailures: false
  
  # 回滚配置
  rollback:
    recreate: true
    force: false
    cleanupOnFail: true
  
  # 卸载配置
  uninstall:
    keepHistory: false
---
# ImageRepository + ImagePolicy - 自动更新镜像
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: myapp-image
  namespace: flux-system
spec:
  image: registry.example.com/myapp
  interval: 1m
  secretRef:
    name: registry-credentials
---
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: myapp-policy
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: myapp-image
  policy:
    semver:
      range: '>=1.0.0 <2.0.0'  # 只更新1.x版本
    # numerical:  # 数字排序
    #   order: asc
    # alphabetical:  # 字母排序
    #   order: asc
  filterTags:
    pattern: '^v[0-9]+\.[0-9]+\.[0-9]+$'  # 只匹配v1.2.3格式
    extract: '$version'
---
# ImageUpdateAutomation - 自动提交更新
apiVersion: image.toolkit.fluxcd.io/v1beta1
kind: ImageUpdateAutomation
metadata:
  name: myapp-auto-update
  namespace: flux-system
spec:
  interval: 30m
  sourceRef:
    kind: GitRepository
    name: myapp-repo
  git:
    checkout:
      ref:
        branch: main
    commit:
      author:
        email: fluxcd@example.com
        name: Flux CD
      messageTemplate: |
        Automated image update
        
        Files:
        {{ range $filename, $_ := .Updated.Files -}}
        - {{ $filename }}
        {{ end -}}
        
        Objects:
        {{ range $resource, $_ := .Updated.Objects -}}
        - {{ $resource.Kind }} {{ $resource.Name }}
        {{ end -}}
    push:
      branch: main
  update:
    path: ./manifests/production
    strategy: Setters  # 使用Marker更新
```

### Flux通知配置

```yaml
# Provider - 告警提供商
apiVersion: notification.toolkit.fluxcd.io/v1beta2
kind: Provider
metadata:
  name: slack
  namespace: flux-system
spec:
  type: slack
  channel: team-backend
  secretRef:
    name: slack-webhook
---
# Alert - 告警规则
apiVersion: notification.toolkit.fluxcd.io/v1beta2
kind: Alert
metadata:
  name: production-alerts
  namespace: flux-system
spec:
  providerRef:
    name: slack
  eventSeverity: info  # info, error
  eventSources:
    - kind: Kustomization
      name: '*'
    - kind: HelmRelease
      name: '*'
  inclusionList:
    - ".*succeeded.*"
    - ".*failed.*"
  summary: "生产环境同步状态"
```

## Tekton Pipeline生产级配置

```yaml
# Pipeline定义 - 完整CI/CD流程
apiVersion: tekton.dev/v1beta1
kind: Pipeline
metadata:
  name: build-test-deploy-pipeline
spec:
  params:
    - name: git-url
      type: string
      description: Git仓库URL
    - name: git-revision
      type: string
      default: main
    - name: image
      type: string
      description: 镜像完整路径
    - name: environment
      type: string
      default: dev
      description: 部署环境
  
  workspaces:
    - name: source
      description: 源代码工作区
    - name: docker-credentials
      description: Docker凭证
    - name: kubeconfig
      description: Kubernetes配置
  
  tasks:
    # 任务1: Git Clone
    - name: git-clone
      taskRef:
        name: git-clone
        kind: ClusterTask
      workspaces:
        - name: output
          workspace: source
      params:
        - name: url
          value: $(params.git-url)
        - name: revision
          value: $(params.git-revision)
        - name: deleteExisting
          value: "true"
    
    # 任务2: 单元测试
    - name: unit-test
      runAfter: [git-clone]
      taskRef:
        name: golang-test
      workspaces:
        - name: source
          workspace: source
      params:
        - name: package
          value: ./...
        - name: flags
          value: "-v -cover"
    
    # 任务3: 代码扫描(SonarQube)
    - name: code-scan
      runAfter: [unit-test]
      taskRef:
        name: sonarqube-scanner
      workspaces:
        - name: source
          workspace: source
      params:
        - name: SONAR_HOST_URL
          value: "https://sonarqube.example.com"
        - name: SONAR_PROJECT_KEY
          value: "myapp"
    
    # 任务4: 构建镜像(Kaniko)
    - name: build-push-image
      runAfter: [code-scan]
      taskRef:
        name: kaniko
        kind: ClusterTask
      workspaces:
        - name: source
          workspace: source
        - name: dockerconfig
          workspace: docker-credentials
      params:
        - name: IMAGE
          value: $(params.image)
        - name: DOCKERFILE
          value: ./Dockerfile
        - name: EXTRA_ARGS
          value:
            - "--cache=true"
            - "--cache-ttl=24h"
            - "--build-arg=VERSION=$(params.git-revision)"
    
    # 任务5: 镜像扫描(Trivy)
    - name: image-scan
      runAfter: [build-push-image]
      taskRef:
        name: trivy-scanner
      params:
        - name: IMAGE
          value: $(params.image)
        - name: SEVERITY
          value: "HIGH,CRITICAL"
    
    # 任务6: 部署到K8s
    - name: deploy-to-k8s
      runAfter: [image-scan]
      taskRef:
        name: kubernetes-actions
        kind: ClusterTask
      workspaces:
        - name: kubeconfig-dir
          workspace: kubeconfig
        - name: manifest-dir
          workspace: source
      params:
        - name: script
          value: |
            kubectl set image deployment/myapp \
              app=$(params.image) \
              -n $(params.environment)
            kubectl rollout status deployment/myapp \
              -n $(params.environment) \
              --timeout=5m
    
    # 任务7: 集成测试
    - name: integration-test
      runAfter: [deploy-to-k8s]
      taskRef:
        name: run-tests
      params:
        - name: TEST_COMMAND
          value: |
            kubectl run test-pod --rm -i --restart=Never \
              --image=curlimages/curl:latest \
              -- curl http://myapp.$(params.environment).svc.cluster.local/health
    
    # 任务8: 通知(Slack)
    - name: send-notification
      runAfter: [integration-test]
      taskRef:
        name: send-to-webhook-slack
        kind: ClusterTask
      params:
        - name: webhook-secret
          value: slack-webhook-secret
        - name: message
          value: "✅ Pipeline成功: $(params.image) 已部署到 $(params.environment)"
  
  # 失败时的清理任务
  finally:
    - name: cleanup-on-failure
      when:
        - input: $(tasks.status)
          operator: in
          values: ["Failed"]
      taskRef:
        name: cleanup-task
      params:
        - name: environment
          value: $(params.environment)
---
# PipelineRun - 触发执行
apiVersion: tekton.dev/v1beta1
kind: PipelineRun
metadata:
  generateName: build-deploy-run-
  labels:
    app: myapp
    environment: production
spec:
  pipelineRef:
    name: build-test-deploy-pipeline
  
  params:
    - name: git-url
      value: https://github.com/org/repo.git
    - name: git-revision
      value: v1.2.3
    - name: image
      value: registry.cn-hangzhou.aliyuncs.com/myorg/myapp:v1.2.3
    - name: environment
      value: production
  
  workspaces:
    - name: source
      volumeClaimTemplate:
        spec:
          accessModes:
            - ReadWriteOnce
          resources:
            requests:
              storage: 5Gi
          storageClassName: fast-ssd
    - name: docker-credentials
      secret:
        secretName: docker-credentials
    - name: kubeconfig
      secret:
        secretName: kubeconfig-prod
  
  # 超时配置
  timeouts:
    pipeline: "1h"
    tasks: "30m"
    finally: "10m"
  
  # Pod模板(资源限制)
  podTemplate:
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
    nodeSelector:
      workload-type: ci-cd
    tolerations:
      - key: "ci-workload"
        operator: "Equal"
        value: "true"
        effect: "NoSchedule"
---
# EventListener + Trigger - Webhook触发
apiVersion: triggers.tekton.dev/v1beta1
kind: EventListener
metadata:
  name: github-listener
spec:
  serviceAccountName: tekton-triggers-sa
  triggers:
    - name: github-push-trigger
      interceptors:
        - ref:
            name: "github"
          params:
            - name: "secretRef"
              value:
                secretName: github-webhook-secret
                secretKey: secret
            - name: "eventTypes"
              value: ["push", "pull_request"]
        - ref:
            name: "cel"
          params:
            - name: "filter"
              value: "body.ref == 'refs/heads/main'"
      bindings:
        - ref: github-push-binding
      template:
        ref: build-deploy-template
---
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerBinding
metadata:
  name: github-push-binding
spec:
  params:
    - name: git-url
      value: $(body.repository.clone_url)
    - name: git-revision
      value: $(body.after)
    - name: image
      value: "registry.example.com/$(body.repository.name):$(body.after[0:7])"
---
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerTemplate
metadata:
  name: build-deploy-template
spec:
  params:
    - name: git-url
    - name: git-revision
    - name: image
  resourcetemplates:
    - apiVersion: tekton.dev/v1beta1
      kind: PipelineRun
      metadata:
        generateName: github-run-
      spec:
        pipelineRef:
          name: build-test-deploy-pipeline
        params:
          - name: git-url
            value: $(tt.params.git-url)
          - name: git-revision
            value: $(tt.params.git-revision)
          - name: image
            value: $(tt.params.image)
```

## 部署策略对比

| 策略 | 描述 | 风险 | 回滚速度 | 适用场景 | 资源成本 | 实现复杂度 | 用户影响 |
|-----|------|------|---------|---------|---------|----------|---------|
| **滚动更新** | 逐步替换Pod | 低 | 快(秒级) | 默认策略,无状态应用 | 低(1.25x) | 低 | 短暂影响 |
| **蓝绿部署** | 两套环境瞬间切换 | 低 | 最快(瞬时) | 零停机要求,关键服务 | 高(2x) | 中 | 无影响 |
| **金丝雀发布** | 小流量逐步验证 | 最低 | 快(分钟级) | 重要变更,新功能试验 | 中(1.1-1.5x) | 高 | 最小影响 |
| **A/B测试** | 按用户特征路由 | 低 | 快 | 功能对比验证 | 中(1.5x) | 高 | 无感知 |
| **影子部署** | 复制流量测试不影响 | 无 | N/A | 性能测试,负载测试 | 高(2x) | 高 | 无影响 |
| **重建** | 先删除再创建 | 高 | 慢 | 开发环境,有状态应用 | 低(1x) | 低 | 停机 |

### 滚动更新详细配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-rolling-update
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 最多额外创建2-3个Pod (10*0.25=2.5)
      maxUnavailable: 25%  # 最多2-3个Pod不可用
  # 快速回滚策略
  revisionHistoryLimit: 10
  progressDeadlineSeconds: 600  # 10分钟内必须完成
  
  template:
    spec:
      containers:
      - name: app
        image: myapp:v2
        readinessProbe:  # 关键: 确保就绪才接收流量
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          failureThreshold: 3
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
---
# PodDisruptionBudget - 保证可用性
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  minAvailable: 80%  # 或 maxUnavailable: 2
  selector:
    matchLabels:
      app: myapp
```

### 蓝绿部署(通过Service切换)

```yaml
# Blue Deployment (当前生产)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-blue
  labels:
    app: myapp
    version: blue
spec:
  replicas: 5
  selector:
    matchLabels:
      app: myapp
      version: blue
  template:
    metadata:
      labels:
        app: myapp
        version: blue
    spec:
      containers:
      - name: app
        image: myapp:v1.0.0
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 1000m
            memory: 2Gi
---
# Green Deployment (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp-green
  labels:
    app: myapp
    version: green
spec:
  replicas: 5
  selector:
    matchLabels:
      app: myapp
      version: green
  template:
    metadata:
      labels:
        app: myapp
        version: green
    spec:
      containers:
      - name: app
        image: myapp:v2.0.0  # 新版本
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 1000m
            memory: 2Gi
---
# Service (切换selector实现蓝绿)
apiVersion: v1
kind: Service
metadata:
  name: myapp
spec:
  selector:
    app: myapp
    version: blue  # 切换到green实现蓝绿部署
  ports:
    - port: 80
      targetPort: 8080
  type: ClusterIP

# 切换命令:
# kubectl patch service myapp -p '{"spec":{"selector":{"version":"green"}}}'
# 回滚命令:
# kubectl patch service myapp -p '{"spec":{"selector":{"version":"blue"}}}'
```

### 金丝雀发布(Argo Rollouts)

```yaml
# Argo Rollouts安装
# kubectl create namespace argo-rollouts
# kubectl apply -n argo-rollouts -f https://github.com/argoproj/argo-rollouts/releases/latest/download/install.yaml

# 金丝雀Rollout
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: myapp
spec:
  replicas: 10
  
  revisionHistoryLimit: 5
  
  selector:
    matchLabels:
      app: myapp
  
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
      - name: app
        image: myapp:v2
        ports:
        - containerPort: 8080
          name: http
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
  
  strategy:
    canary:
      # 金丝雀Service(用于测试流量)
      canaryService: myapp-canary
      # 稳定Service(生产流量)
      stableService: myapp-stable
      
      # 流量管理(Istio/ALB/Nginx)
      trafficRouting:
        istio:
          virtualService:
            name: myapp-vsvc
            routes:
              - primary
        # ALB示例
        # alb:
        #   ingress: myapp-ingress
        #   servicePort: 80
        # Nginx示例
        # nginx:
        #   stableIngress: myapp-stable
        #   additionalIngressAnnotations:
        #     canary-by-header: X-Canary
      
      # 金丝雀步骤
      steps:
        # 第1步: 10%流量
        - setWeight: 10
        - pause: {duration: 5m}
        
        # 第2步: 30%流量
        - setWeight: 30
        - pause: {duration: 5m}
        
        # 第3步: 50%流量
        - setWeight: 50
        - pause: {duration: 10m}
        
        # 第4步: 80%流量
        - setWeight: 80
        - pause: {duration: 10m}
        
        # 第5步: 100%流量(全量)
        - setWeight: 100
      
      # 分析(自动判断是否继续)
      analysis:
        templates:
          - templateName: success-rate
          - templateName: latency-p99
        startingStep: 1  # 从第1步开始分析
        args:
          - name: service-name
            value: myapp-canary
      
      # 自动提升或回滚
      # autoPromotionEnabled: false  # 手动提升
      # autoPromotionSeconds: 600  # 10分钟后自动提升
---
# AnalysisTemplate - 成功率分析
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  args:
    - name: service-name
  metrics:
    - name: success-rate
      interval: 1m
      count: 5  # 连续5次成功才算通过
      successCondition: result[0] >= 0.95  # 成功率>=95%
      failureLimit: 3  # 失败3次则回滚
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(
              istio_requests_total{
                destination_service_name="{{args.service-name}}",
                response_code!~"5.*"
              }[5m]
            ))
            /
            sum(rate(
              istio_requests_total{
                destination_service_name="{{args.service-name}}"
              }[5m]
            ))
---
# AnalysisTemplate - P99延迟分析
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: latency-p99
spec:
  args:
    - name: service-name
  metrics:
    - name: latency-p99
      interval: 1m
      successCondition: result[0] < 1000  # P99 < 1秒
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            histogram_quantile(0.99, 
              sum(rate(
                istio_request_duration_milliseconds_bucket{
                  destination_service_name="{{args.service-name}}"
                }[5m]
              )) by (le)
            )
---
# Service配置
apiVersion: v1
kind: Service
metadata:
  name: myapp-stable
spec:
  selector:
    app: myapp
  ports:
    - port: 80
      targetPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: myapp-canary
spec:
  selector:
    app: myapp
  ports:
    - port: 80
      targetPort: 8080
---
# Istio VirtualService
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: myapp-vsvc
spec:
  hosts:
    - myapp.example.com
  http:
    - name: primary
      match:
        - uri:
            prefix: /
      route:
        - destination:
            host: myapp-stable
          weight: 100
        - destination:
            host: myapp-canary
          weight: 0
```

## CI/CD最佳实践矩阵

| 实践 | 说明 | 工具支持 | 实施优先级 | 成本 | 收益 |
|-----|------|---------|----------|-----|------|
| **GitOps** | Git作为唯一真相源,声明式部署 | ArgoCD/Flux | P0 | 低 | 高 |
| **基础设施即代码** | 所有配置版本控制,可追溯 | Terraform/Pulumi | P0 | 中 | 高 |
| **不可变部署** | 新版本新Pod,不修改现有资源 | K8S默认 | P0 | 无 | 高 |
| **环境一致性** | 开发=预发=生产环境一致 | Kustomize/Helm | P0 | 低 | 高 |
| **自动化测试** | 部署前单元测试+集成测试 | Tekton/GitHub Actions | P1 | 中 | 高 |
| **渐进式交付** | 金丝雀/蓝绿降低风险 | Argo Rollouts/Flagger | P1 | 中 | 高 |
| **自动回滚** | 基于指标自动回滚 | Argo Rollouts/Flagger | P1 | 低 | 高 |
| **多环境管理** | 统一管理dev/staging/prod | ApplicationSet/Kustomize | P1 | 低 | 中 |
| **Secret管理** | 敏感信息加密和轮换 | Sealed Secrets/External Secrets | P0 | 中 | 高 |
| **镜像扫描** | 构建时扫描漏洞 | Trivy/Clair | P1 | 低 | 高 |
| **镜像签名** | 保证镜像供应链安全 | Cosign/Notary | P2 | 中 | 中 |
| **RBAC控制** | 最小权限原则 | K8S RBAC | P0 | 低 | 高 |
| **审计日志** | 所有操作可追溯 | K8S Audit/日志系统 | P1 | 低 | 中 |
| **监控告警** | 部署状态实时监控 | Prometheus/Grafana | P0 | 中 | 高 |
| **通知集成** | 部署事件通知团队 | Slack/钉钉/企业微信 | P2 | 低 | 中 |

### GitOps安全加固

```yaml
# Sealed Secrets - 加密存储Secret到Git
# 安装: kubectl apply -f https://github.com/bitnami-labs/sealed-secrets/releases/download/v0.24.0/controller.yaml

# 1. 创建普通Secret
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
  namespace: production
stringData:
  username: admin
  password: super-secret-password

# 2. 使用kubeseal加密
# kubeseal --format=yaml < secret.yaml > sealed-secret.yaml

# 3. 加密后的SealedSecret(可安全提交Git)
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: db-credentials
  namespace: production
spec:
  encryptedData:
    username: AgBh...加密内容...
    password: AgCQ...加密内容...
  template:
    metadata:
      name: db-credentials
      namespace: production
    type: Opaque

# SealedSecret部署后自动解密为普通Secret
```

### External Secrets Operator(企业级)

```yaml
# External Secrets Operator
# helm install external-secrets external-secrets/external-secrets -n external-secrets-system

# SecretStore - 连接外部密钥管理系统
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: vault-backend
  namespace: production
spec:
  provider:
    vault:
      server: "https://vault.example.com"
      path: "secret"
      version: "v2"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "production-role"
          serviceAccountRef:
            name: external-secrets-sa
---
# ExternalSecret - 从外部同步到K8s Secret
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: db-credentials
  namespace: production
spec:
  refreshInterval: 1h  # 每小时同步一次
  secretStoreRef:
    name: vault-backend
    kind: SecretStore
  target:
    name: db-credentials
    creationPolicy: Owner
    template:
      engineVersion: v2
      data:
        # 模板化输出
        connection-string: |
          postgres://{{ .username }}:{{ .password }}@postgres:5432/mydb
  data:
    - secretKey: username
      remoteRef:
        key: database/production
        property: username
    - secretKey: password
      remoteRef:
        key: database/production
        property: password
```

## ACK DevOps集成架构

```
┌─────────────────────────────────────────────────────────────┐
│                       云效 Flow (CI)                        │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐ │
│  │代码拉取   │ → │单元测试   │ → │镜像构建   │ → │镜像扫描   │ │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘ │
│         │                                           │        │
│         ▼                                           ▼        │
│   ┌──────────┐                              ┌──────────┐    │
│   │  Codeup  │                              │   ACR    │    │
│   └──────────┘                              └──────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼ Webhook/手动触发
┌─────────────────────────────────────────────────────────────┐
│                    ArgoCD / Flux (GitOps CD)                │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐ │
│  │同步Git   │ → │对比差异   │ → │应用变更   │ → │健康检查   │ │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘ │
│         │                                           │        │
│         ▼                                           ▼        │
│   ┌──────────┐                              ┌──────────┐    │
│   │ GitRepo  │                              │   ACK    │    │
│   └──────────┘                              └──────────┘    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     监控告警 (ARMS)                         │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐ │
│  │应用监控   │   │日志服务   │   │链路追踪   │   │告警通知   │ │
│  └──────────┘   └──────────┘   └──────────┘   └──────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### ACK + 云效完整流程

```yaml
# 云效Flow配置(.flow.yml)
version: "1.0"
name: "ACK部署流水线"
stages:
  - stage:
      name: "构建阶段"
      jobs:
        - job:
            name: "单元测试"
            steps:
              - step: golang@1.18
                  run: |
                    go mod download
                    go test -v -cover ./...
        - job:
            name: "构建镜像"
            steps:
              - step: docker-build@1
                  registry: registry.cn-hangzhou.aliyuncs.com
                  namespace: myorg
                  image: myapp
                  tag: ${PIPELINE_ID}
                  dockerfile: Dockerfile
                  scan: true  # 启用镜像安全扫描
  
  - stage:
      name: "部署阶段"
      jobs:
        - job:
            name: "更新GitOps仓库"
            steps:
              - step: bash@latest
                  run: |
                    # 克隆GitOps仓库
                    git clone https://${GIT_TOKEN}@github.com/org/gitops-repo.git
                    cd gitops-repo
                    
                    # 更新镜像Tag
                    sed -i "s|image: .*|image: registry.cn-hangzhou.aliyuncs.com/myorg/myapp:${PIPELINE_ID}|" \
                      manifests/production/deployment.yaml
                    
                    # 提交和推送
                    git config user.email "ci@example.com"
                    git config user.name "CI Bot"
                    git add .
                    git commit -m "Update image to ${PIPELINE_ID}"
                    git push origin main
        - job:
            name: "等待ArgoCD同步"
            steps:
              - step: bash@latest
                  run: |
                    # 等待ArgoCD完成同步
                    argocd app wait myapp-production --timeout 600
                    argocd app get myapp-production
```

### ACK专属配置

| 功能 | 产品 | 集成方式 | 使用场景 |
|-----|------|---------|---------|
| **代码仓库** | 云效Codeup | 原生 | 企业代码托管 |
| **CI构建** | 云效Flow | 原生 | 持续集成流水线 |
| **镜像仓库** | ACR企业版 | 原生 | 镜像存储+扫描 |
| **CD部署** | ArgoCD/云效AppStack | 组件安装/原生 | GitOps部署 |
| **Secret管理** | KMS + External Secrets | Helm | 密钥轮换 |
| **监控** | ARMS应用监控 | 默认 | APM+指标 |
| **日志** | SLS | 默认 | 日志聚合 |
| **链路追踪** | ARMS链路追踪 | 默认 | 分布式追踪 |
| **告警** | 云监控 | 默认 | 统一告警 |

## 故障排查和运维

### CI/CD常见问题

| 问题 | 症状 | 排查命令 | 解决方案 |
|-----|------|---------|---------|
| **ArgoCD同步失败** | OutOfSync | `argocd app get <app>` | 检查Git仓库连接,验证YAML语法 |
| **Helm部署失败** | Failed | `helm list -n <ns>` | 检查values.yaml,查看Helm错误 |
| **镜像拉取失败** | ImagePullBackOff | `kubectl describe pod` | 检查镜像仓库凭证,验证镜像存在 |
| **Pipeline超时** | Timeout | 查看Pipeline日志 | 增加超时时间,优化构建步骤 |
| **Webhook未触发** | 无PipelineRun | 检查Webhook配置 | 验证Secret,检查防火墙 |
| **部署到错误集群** | 应用在错误位置 | `kubectl config current-context` | 检查kubeconfig,验证集群选择 |

```bash
# ArgoCD调试命令
argocd app get myapp --refresh  # 强制刷新
argocd app diff myapp           # 查看差异
argocd app sync myapp --dry-run # 模拟同步
argocd app manifests myapp      # 查看渲染后的清单
argocd app history myapp        # 查看历史版本
argocd app rollback myapp 5     # 回滚到版本5

# Flux调试命令
flux get all                     # 查看所有Flux资源
flux reconcile source git myapp-repo  # 强制同步Git
flux reconcile kustomization myapp    # 强制应用Kustomization
flux logs --all-namespaces       # 查看Flux日志
flux suspend kustomization myapp # 暂停同步
flux resume kustomization myapp  # 恢复同步

# Tekton调试命令
tkn pipeline list                # 列出Pipeline
tkn pipelinerun logs <run> -f    # 查看运行日志
tkn pipelinerun describe <run>   # 查看详情
tkn pipelinerun cancel <run>     # 取消运行
```

### 回滚策略

```bash
# Deployment回滚
kubectl rollout undo deployment/myapp
kubectl rollout undo deployment/myapp --to-revision=3
kubectl rollout status deployment/myapp

# Helm回滚
helm rollback myapp 3  # 回滚到版本3
helm history myapp     # 查看历史版本

# ArgoCD回滚
argocd app rollback myapp 5  # 回滚到历史版本5
argocd app rollback myapp --prune  # 回滚并清理

# Argo Rollouts回滚
kubectl argo rollouts abort myapp  # 中止金丝雀
kubectl argo rollouts undo myapp   # 回滚到上一版本
```

## 性能优化建议

| 优化项 | 说明 | 预期效果 | 实施难度 |
|-------|------|---------|---------|
| **镜像缓存** | 使用镜像加速器,启用构建缓存 | 构建时间减少50-70% | 低 |
| **并行构建** | Tekton并行任务,多stage构建 | 流水线时间减少30-50% | 中 |
| **增量部署** | 只部署变化的服务 | 部署时间减少60-80% | 中 |
| **预拉取镜像** | 节点预拉热门镜像 | 启动时间减少70% | 低 |
| **资源限制** | 合理设置CI Pod资源 | 提高并发能力 | 低 |
| **分片同步** | ArgoCD Application分组 | 同步时间减少40% | 中 |
| **本地缓存** | 使用本地Chart仓库 | Helm部署加速50% | 中 |

### 高可用配置

```yaml
# ArgoCD高可用(3副本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: argocd-server
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app.kubernetes.io/name: argocd-server
              topologyKey: kubernetes.io/hostname
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app.kubernetes.io/name: argocd-server
---
# Redis HA(哨兵模式)
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: argocd-redis-ha-server
spec:
  replicas: 3
  serviceName: argocd-redis-ha
  template:
    spec:
      containers:
        - name: redis
          image: redis:7.0-alpine
          args:
            - "--save"
            - ""
            - "--appendonly"
            - "no"
```

---

**CI/CD原则**: GitOps优先 + 自动化测试 + 渐进式交付 + 快速回滚 + 全程可观测

**架构建议**: 
- **小团队(<50人)**: GitHub Actions/GitLab CI + Flux + Helm
- **中型团队(50-200人)**: ArgoCD + Tekton/云效 + Argo Rollouts
- **大型团队(>200人)**: ArgoCD多租户 + Spinnaker多云 + 自建平台

**运维建议**:
- 开发环境: 自动同步+自动清理
- 预发环境: 自动同步+手动提升
- 生产环境: 手动同步+金丝雀+自动回滚

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
