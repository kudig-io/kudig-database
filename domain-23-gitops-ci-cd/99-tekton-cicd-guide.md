# Tekton 云原生 CI/CD 实践指南

> **适用版本**: Tekton Pipelines v0.68 / Tekton Triggers v0.30  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、Tekton 架构](#一tekton-架构)
- [二、安装部署](#二安装部署)
- [三、Task 与 TaskRun](#三task-与-taskrun)
- [四、Pipeline 与 PipelineRun](#四pipeline-与-pipelinerun)
- [五、Workspace 数据传递](#五workspace-数据传递)
- [六、Triggers 事件触发](#六triggers-事件触发)
- [七、Catalog 与复用](#七catalog-与复用)
- [八、与 Argo CD 集成](#八与-argo-cd-集成)
- [九、Tekton vs Jenkins/GitHub Actions 对比](#九tekton-vs-jenkinsgithub-actions-对比)

---

## 一、Tekton 架构

```
Tekton 组件
├── Pipelines (核心)
│   ├── Task         ← 最小执行单元 (Pod 中的容器序列)
│   ├── TaskRun      ← Task 的实例化执行
│   ├── Pipeline     ← Task 的有向无环图 (DAG)
│   └── PipelineRun  ← Pipeline 的实例化执行
│
├── Triggers (事件驱动)
│   ├── EventListener   ← 接收 Webhook
│   ├── TriggerBinding  ← 提取事件参数
│   ├── TriggerTemplate ← 生成 PipelineRun
│   └── ClusterTriggerBinding (全局)
│
├── Catalog (任务库)
│   └── Tekton Hub (社区共享 Task)
│
└── Chains (供应链安全)
    └── 自动签名 Artifact

设计理念
├── 每个 Step = 一个容器
├── 不可变镜像 (无 shell 脚本)
├── 声明式配置 (GitOps 友好)
└── 可组合、可复用
```

---

## 二、安装部署

```bash
# 安装 Tekton Pipelines
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/latest/release.yaml

# 安装 Tekton Triggers
kubectl apply -f https://storage.googleapis.com/tekton-releases/triggers/latest/release.yaml

# 安装 Tekton Dashboard (可选)
kubectl apply -f https://storage.googleapis.com/tekton-releases/dashboard/latest/release.yaml

# 验证
kubectl get pods -n tekton-pipelines
```

### 生产级配置

```yaml
# 配置默认 ServiceAccount
apiVersion: v1
kind: ConfigMap
metadata:
  name: config-defaults
  namespace: tekton-pipelines
data:
  default-timeout-minutes: "60"
  default-service-account: "tekton-pipeline"
  default-managed-by-label-value: "tekton-pipelines"
  default-pod-template: |
    securityContext:
      runAsNonRoot: true
      seccompProfile:
        type: RuntimeDefault
```

---

## 三、Task 与 TaskRun

### 3.1 基础 Task

```yaml
apiVersion: tekton.dev/v1
kind: Task
metadata:
  name: build-go-app
  namespace: cicd
spec:
  description: Build a Go application
  
  params:
    - name: git-url
      type: string
      description: Git repository URL
    - name: git-revision
      type: string
      default: main
    - name: image-tag
      type: string
      default: latest
  
  workspaces:
    - name: source
      description: Source code workspace
    - name: dockerconfig
      description: Docker config for push
      optional: true
  
  steps:
    - name: clone
      image: gcr.io/tekton-releases/github.com/tektoncd/pipeline/cmd/git-init:latest
      script: |
        git clone $(params.git-url) $(workspaces.source.path)/repo
        cd $(workspaces.source.path)/repo
        git checkout $(params.git-revision)
    
    - name: build
      image: golang:1.23-alpine
      workingDir: $(workspaces.source.path)/repo
      script: |
        go build -o app ./cmd/main.go
    
    - name: test
      image: golang:1.23-alpine
      workingDir: $(workspaces.source.path)/repo
      script: |
        go test -v ./...
    
    - name: build-image
      image: gcr.io/kaniko-project/executor:latest
      env:
        - name: DOCKER_CONFIG
          value: $(workspaces.dockerconfig.path)
      command:
        - /kaniko/executor
      args:
        - --context=$(workspaces.source.path)/repo
        - --dockerfile=Dockerfile
        - --destination=myregistry/app:$(params.image-tag)
        - --cache=true
```

### 3.2 直接运行 Task

```yaml
apiVersion: tekton.dev/v1
kind: TaskRun
metadata:
  generateName: build-go-app-run-
  namespace: cicd
spec:
  taskRef:
    name: build-go-app
  params:
    - name: git-url
      value: https://github.com/org/myapp.git
    - name: git-revision
      value: v1.2.0
    - name: image-tag
      value: v1.2.0
  workspaces:
    - name: source
      emptyDir: {}
    - name: dockerconfig
      secret:
        secretName: docker-registry-config
```

---

## 四、Pipeline 与 PipelineRun

### 4.1 完整 CI/CD Pipeline

```yaml
apiVersion: tekton.dev/v1
kind: Pipeline
metadata:
  name: ci-cd-pipeline
  namespace: cicd
spec:
  description: Full CI/CD pipeline for Go application
  
  params:
    - name: git-url
      type: string
    - name: git-revision
      type: string
    - name: image-tag
      type: string
    - name: deploy-env
      type: string
      default: staging
  
  workspaces:
    - name: shared-source
    - name: dockerconfig
    - name: argocd-config
  
  tasks:
    # 并行: 代码检出 + 安全扫描
    - name: clone
      taskRef:
        name: git-clone
        kind: ClusterTask
      workspaces:
        - name: output
          workspace: shared-source
      params:
        - name: url
          value: $(params.git-url)
        - name: revision
          value: $(params.git-revision)
    
    - name: lint
      runAfter: [clone]
      taskRef:
        name: golangci-lint
      workspaces:
        - name: source
          workspace: shared-source
    
    - name: unit-test
      runAfter: [clone]
      taskRef:
        name: go-test
      workspaces:
        - name: source
          workspace: shared-source
    
    - name: build-image
      runAfter: [lint, unit-test]
      taskRef:
        name: kaniko-build
      workspaces:
        - name: source
          workspace: shared-source
        - name: dockerconfig
          workspace: dockerconfig
      params:
        - name: image-tag
          value: $(params.image-tag)
    
    - name: security-scan
      runAfter: [build-image]
      taskRef:
        name: trivy-scan
      params:
        - name: image
          value: myregistry/app:$(params.image-tag)
    
    - name: deploy
      runAfter: [security-scan]
      when:
        - input: $(params.deploy-env)
          operator: in
          values: ["staging", "production"]
      taskRef:
        name: argocd-sync
      workspaces:
        - name: config
          workspace: argocd-config
      params:
        - name: app-name
          value: myapp-$(params.deploy-env)
        - name: revision
          value: $(params.image-tag)
  
  finally:
    # 总是执行的通知
    - name: notify
      taskRef:
        name: slack-notify
      params:
        - name: message
          value: "Pipeline completed for $(params.git-revision)"
```

### 4.2 PipelineRun

```yaml
apiVersion: tekton.dev/v1
kind: PipelineRun
metadata:
  generateName: ci-cd-pipeline-run-
  namespace: cicd
spec:
  pipelineRef:
    name: ci-cd-pipeline
  params:
    - name: git-url
      value: https://github.com/org/myapp.git
    - name: git-revision
      value: main
    - name: image-tag
      value: 1.2.3
    - name: deploy-env
      value: staging
  workspaces:
    - name: shared-source
      volumeClaimTemplate:
        spec:
          accessModes: ["ReadWriteOnce"]
          resources:
            requests:
              storage: 1Gi
    - name: dockerconfig
      secret:
        secretName: docker-registry-config
    - name: argocd-config
      secret:
        secretName: argocd-token
  timeouts:
    pipeline: "1h"
    tasks: "30m"
```

---

## 五、Workspace 数据传递

| 类型 | 用途 | 示例 |
|:---|:---|:---|
| EmptyDir | 临时共享 | build 产物传递 |
| PVC | 持久化 | 缓存依赖 |
| ConfigMap | 配置文件 | 构建配置 |
| Secret | 敏感数据 | registry 凭证 |
| VolumeClaimTemplate | 动态 PVC | PipelineRun 隔离 |

```yaml
# 缓存示例
workspaces:
  - name: go-cache
    persistentVolumeClaim:
      claimName: go-mod-cache
```

---

## 六、Triggers 事件触发

### 6.1 GitHub Webhook 触发

```yaml
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerTemplate
metadata:
  name: github-trigger-template
  namespace: cicd
spec:
  params:
    - name: git-url
    - name: git-revision
    - name: image-tag
  resourcetemplates:
    - apiVersion: tekton.dev/v1
      kind: PipelineRun
      metadata:
        generateName: github-ci-cd-
      spec:
        pipelineRef:
          name: ci-cd-pipeline
        params:
          - name: git-url
            value: $(tt.params.git-url)
          - name: git-revision
            value: $(tt.params.git-revision)
          - name: image-tag
            value: $(tt.params.image-tag)
        workspaces:
          - name: shared-source
            volumeClaimTemplate:
              spec:
                accessModes: ["ReadWriteOnce"]
                resources:
                  requests:
                    storage: 1Gi
---
apiVersion: triggers.tekton.dev/v1beta1
kind: TriggerBinding
metadata:
  name: github-binding
  namespace: cicd
spec:
  params:
    - name: git-url
      value: $(body.repository.clone_url)
    - name: git-revision
      value: $(body.after)
    - name: image-tag
      value: $(body.ref)
---
apiVersion: triggers.tekton.dev/v1beta1
kind: EventListener
metadata:
  name: github-listener
  namespace: cicd
spec:
  serviceAccountName: tekton-triggers
  triggers:
    - name: github-push
      interceptors:
        - ref:
            name: github
          params:
            - name: secretRef
              value:
                secretName: github-webhook-secret
                secretKey: secretToken
        - ref:
            name: cel
          params:
            - name: filter
              value: "body.ref == 'refs/heads/main'"
      bindings:
        - ref: github-binding
      template:
        ref: github-trigger-template
```

### 6.2 暴露 EventListener

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tekton-webhook
  namespace: cicd
spec:
  rules:
  - host: tekton-webhook.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: el-github-listener
            port:
              number: 8080
```

---

## 七、Catalog 与复用

```bash
# 从 Tekton Hub 安装 Task
tkn hub get task git-clone

# 安装到集群
tkn hub install task git-clone

# 查看可用 Task
tkn hub search build
```

### 常用社区 Task

| Task | 用途 |
|:---|:---|
| git-clone | 代码检出 |
| kaniko | 无特权构建镜像 |
| buildah | 容器镜像构建 |
| trivy-scanner | 漏洞扫描 |
| argocd-task | Argo CD 同步 |
| slack-send | Slack 通知 |
| helm-upgrade-from-source | Helm 部署 |

---

## 八、与 Argo CD 集成

```
Git Push
    |
    ▼
GitHub Webhook ──► Tekton EventListener
    |
    ▼
Tekton Pipeline
    ├── Build Image
    ├── Push to Registry
    ├── Update GitOps Repo (kustomize edit set image)
    └── Commit & Push
         |
         ▼
    Argo CD (轮询 Git)
         |
         ▼
    自动同步到 K8s 集群
```

```yaml
# Pipeline 中的 GitOps 步骤
- name: update-gitops
  runAfter: [build-image]
  workspaces:
    - name: gitops-repo
      workspace: gitops-repo
  taskSpec:
    workspaces:
      - name: gitops-repo
    steps:
      - name: update-image
        image: bitnami/kubectl:latest
        script: |
          cd $(workspaces.gitops-repo.path)
          kustomize edit set image app=myregistry/app:$(params.image-tag)
          git config --global user.email "tekton@example.com"
          git config --global user.name "Tekton"
          git add .
          git commit -m "Update image to $(params.image-tag)"
          git push origin main
```

---

## 九、Tekton vs Jenkins/GitHub Actions 对比

| 维度 | Tekton | Jenkins | GitHub Actions |
|:---|:---|:---|:---|
| **架构** | K8s Native | 独立服务 | SaaS / Self-hosted |
| **执行环境** | K8s Pod | Agent/Slave | Runner (VM/Container) |
| **可移植性** | 任何 K8s 集群 | 需迁移基础设施 | 绑定 GitHub |
| **配置方式** | YAML (GitOps) | Groovy / UI | YAML |
| **扩展性** | Task Catalog | Plugin 生态 | Marketplace |
| **安全性** | 容器隔离 | Agent 共享 | Runner 隔离 |
| **成本** | 基础设施成本 | 基础设施 + 维护 | 免费额度 + 付费 |
| **学习曲线** | 高 | 中 | 低 |
| **调试** | kubectl logs | Blue Ocean UI | Web UI |
| **生态集成** | K8s 原生 | 通用 | GitHub 生态 |

### 选型决策

```
选择 Tekton 如果:
  ✅ 已在 K8s 上运行
  ✅ 需要 GitOps 原生 CI/CD
  ✅ 需要跨云可移植性
  ✅ 需要供应链安全 (Tekton Chains)
  ✅ 团队接受 K8s 运维复杂度

选择 Jenkins 如果:
  ✅ 已有 Jenkins 基础设施
  ✅ 需要丰富的插件生态
  ✅ 团队熟悉 Jenkins

选择 GitHub Actions 如果:
  ✅ 代码托管在 GitHub
  ✅ 快速启动，无需基础设施
  ✅ 简单的 CI/CD 需求
```

---

## 参考链接

- [Tekton 官方文档](https://tekton.dev/docs/)
- [Tekton Hub](https://hub.tekton.dev/)
- [Tekton GitHub](https://github.com/tektoncd/pipeline)
- [Tekton Triggers](https://tekton.dev/docs/triggers/)
- [Tekton Chains](https://tekton.dev/docs/chains/)
- [CLI 工具 tkn](https://tekton.dev/docs/cli/)
