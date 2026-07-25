---
title: GitLab CI/CD 企业级流水线自动化平台
description: 'description: ''- [一、概述](#一概述)'''
summary: 'description: ''- [一、概述](#一概述)'''
category: general
tags:
- gitops
- cicd
- devops
- scheduler
- prometheus
- grafana
- helm
- argocd
- flux
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- gitlab-enterprise-cicd是什么？
- gitlab-enterprise-cicd的使用方法
- gitlab-enterprise-cicd的最佳实践
trigger_keywords:
- GitLab
- CI
- CD
- 企业级流水线自动化平台
- release
- change
- management
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: GitLab CI/CD 企业级流水线自动化平台
description: '- [一、概述](#一概述)'
category: gitops-ci-cd
tags:
- k8s
- gitops
- ci-cd
- [[ArgoCD|argocd]]
- [[Flux|flux]]
- scheduler
- [[Prometheus|prometheus]]
- grafana
- helm
- docker
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- DevOps 工程师
- SRE
- 开发工程师
estimated_read_time: 5min
intent_queries:
- GitLab CI/CD 企业级流水线自动化平台 是什么
- 如何 GitLab CI/CD 企业级流水线自动化平台
- Kubernetes 23 gitops ci cd 最佳实践
trigger_keywords:
- GitLab
- CI
- CD
- 企业级流水线自动化平台
- gitops
- ci
- cd
cross_refs:
- type: domain
  path: ../平台工程/
  label: '相关知识域: 平台工程'
- type: domain
  path: ../发布变更/
  label: '相关知识域: domain-24-infrastructure-as-code'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/git.md
  label: '速查卡: git'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# GitLab CI/CD 企业级流水线自动化平台

> **适用版本**: GitLab CE/EE v17.10+ / Runner v17.x
> **最后更新**: 2026-04-24
> **难度**: 中级 → 高级

---

<!-- chunk: 📋 目录 -->## 📋 目录

- [一、概述](#一概述)
- [二、架构设计](#二架构设计)
- [三、核心配置](#三核心配置)
- [四、安全与合规](#四安全与合规)
- [五、多环境管理策略](#五多环境管理策略)
- [六、监控与回滚](#六监控与回滚)
- [七、最佳实践](#七最佳实践)
- [八、故障排查](#八故障排查)

---

<!-- chunk: 一、概述 -->## 一、概述

GitLab CI/CD 是一体化 DevOps 平台 GitLab 的核心功能模块，提供从代码管理到持续交付的完整解决方案。与 Jenkins、GitHub Actions 等独立 CI/CD 工具不同，GitLab CI/CD 与 GitLab 的代码管理、容器注册表、安全扫描、制品管理等模块深度集成，形成了一个"开箱即用"的 DevOps 工作流。

GitLab CI/CD 的核心设计理念是"配置即代码"——通过项目根目录下的 `.gitlab-ci.yml` 文件定义完整的构建、测试和部署流程。这种设计使得 CI/CD 配置与代码一同版本管理，任何配置变更都可以通过 Merge Request 进行审查和讨论。

在企业级场景中，GitLab CI/CD 的优势在于其内置的安全扫描能力（SAST、DAST、依赖扫描、容器扫描、许可证合规）、Environment 保护机制（手动审批、访问限制）、以及 Kubernetes Executor（直接在 K8s 集群中调度构建任务）。本文档将深入探讨这些企业级特性的配置和最佳实践。

GitLab 提供 CE（Community Edition）和 EE（Enterprise Edition）两个版本。企业级功能（如高级安全扫描、合规框架、价值流分析）仅在 EE 版本中可用。GitLab 还提供了 SaaS 托管服务（gitlab.com）和自托管部署（Omnibus/Helm Chart）两种部署模式。

---

<!-- chunk: 二、架构设计 -->## 二、架构设计

## 2.1 GitLab 企业级架构

```mermaid
graph TB
    subgraph "用户接入层"
        LB[Load Balancer<br/>NGINX/HAProxy]
    end

    subgraph "GitLab 核心服务"
        WE[Workhorse<br/>请求路由]
        RA[Rails App<br/>核心应用]
        GI[Gitaly<br/>Git 存储]
        SI[Sidekiq<br/>后台任务]
    end

    subgraph "CI/CD 服务"
        RU[GitLab Runner<br/>构建执行器]
        KA[Kubernetes Agent<br/>集群连接]
        REG[Container Registry<br/>镜像仓库]
    end

    subgraph "存储层"
        DB[(PostgreSQL<br/>数据库)]
        RD[(Redis<br/>缓存/队列)]
        OS[Object Storage<br/>S3/MinIO]
    end

    subgraph "监控层"
        PROM[Prometheus]
        GRAF[Grafana]
        ALERT[Alertmanager]
    end

    LB --> WE
    WE --> RA
    RA --> GI
    RA --> SI
    RA --> DB
    RA --> RD
    SI --> RD
    RU --> RA
    RU --> DOCKER[Docker Engine]
    RU --> K8S[Kubernetes Cluster]
    KA --> K8S
    RA --> OS
    RA --> REG
    PROM --> RA
    PROM --> RU
    GRAF --> PROM
    ALERT --> PROM
```

## 2.2 Runner 架构

```mermaid
graph LR
    subgraph "GitLab Server"
        API[GitLab API]
        QUEUE[Job Queue]
    end

    subgraph "Runner Manager"
        CONFIG[config.toml]
        SCHEDULER[Job Scheduler]
    end

    subgraph "Executor 类型"
        DOCKER_EXEC[Docker Executor<br/>容器隔离]
        K8S_EXEC[Kubernetes Executor<br/>Pod 调度]
        SHELL_EXEC[Shell Executor<br/>直接执行]
        SSH_EXEC[SSH Executor<br/>远程执行]
    end

    API --> SCHEDULER
    QUEUE --> SCHEDULER
    SCHEDULER --> DOCKER_EXEC
    SCHEDULER --> K8S_EXEC
    SCHEDULER --> SHELL_EXEC
    SCHEDULER --> SSH_EXEC
```

## 2.3 Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: gitlab
  namespace: gitlab-system
spec:
  serviceName: gitlab
  replicas: 1
  selector:
    matchLabels:
      app: gitlab
  template:
    metadata:
      labels:
        app: gitlab
    spec:
      initContainers:
      - name: configure
        image: busybox:1.35
        command: ['sh', '-c', 'chown -R 998:998 /var/opt/gitlab']
        volumeMounts:
        - name: gitlab-data
          mountPath: /var/opt/gitlab
      containers:
      - name: gitlab
        image: gitlab/gitlab-ce:17.10.0-ce.0
        env:
        - name: GITLAB_OMNIBUS_CONFIG
          valueFrom:
            configMapKeyRef:
              name: gitlab-config
              key: gitlab.rb
        - name: POSTGRES_USER
          value: "gitlab"
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secrets
              key: password
        ports:
        - containerPort: 80
          name: http
        - containerPort: 22
          name: ssh
        readinessProbe:
          httpGet:
            path: /-/readiness
            port: 80
          initialDelaySeconds: 60
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /-/health
            port: 80
          initialDelaySeconds: 120
          periodSeconds: 30
        volumeMounts:
        - name: gitlab-data
          mountPath: /var/opt/gitlab
        - name: gitlab-config
          mountPath: /etc/gitlab
        - name: gitlab-logs
          mountPath: /var/log/gitlab
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
      volumes:
      - name: gitlab-config
        configMap:
          name: gitlab-config
      - name: gitlab-logs
        emptyDir: {}
  volumeClaimTemplates:
  - metadata:
      name: gitlab-data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 200Gi
```

---

<!-- chunk: 三、核心配置 -->## 三、核心配置

## 3.1 Runner 配置

```toml
# config.toml - GitLab Runner 配置
concurrent = 20
check_interval = 0
log_level = "info"

[session_server]
  session_timeout = 1800

runners
  name = "kubernetes-runner"
  url = "https://gitlab.example.com/"
  token = "${RUNNER_TOKEN}"
  executor = "kubernetes"
  [runners.kubernetes]
    host = "https://kubernetes.default"
    image = "ubuntu:22.04"
    namespace = "gitlab-runner"
    privileged = true
    poll_timeout = 180
    service_account = "gitlab-runner"
    [runners.kubernetes.node_selector]
      gitlab-runner = "true"
    runners.kubernetes.volumes.empty_dir
      name = "docker-certs"
      mount_path = "/certs/client"
      medium = "Memory"
    [runners.kubernetes.resources]
      [runners.kubernetes.resources.requests]
        cpu = "500m"
        memory = "1Gi"
      [runners.kubernetes.resources.limits]
        cpu = "2"
        memory = "4Gi"

runners
  name = "docker-runner"
  url = "https://gitlab.example.com/"
  token = "${RUNNER_TOKEN}"
  executor = "docker"
  [runners.docker]
    tls_verify = false
    image = "docker:24-dind"
    privileged = true
    disable_cache = false
    volumes = [
      "/cache",
      "/var/run/docker.sock:/var/run/docker.sock",
      "/certs/client"
    ]
    pull_policy = "if-not-present"
```

## 3.2 企业级 CI/CD 流水线

```yaml
# .gitlab-ci.yml
stages:
  - security-scan
  - build
  - test
  - deploy-staging
  - e2e-test
  - deploy-production
  - cleanup

variables:
  DOCKER_DRIVER: overlay2
  DOCKER_TLS_CERTDIR: "/certs"
  KUBE_NAMESPACE: ${CI_PROJECT_PATH_SLUG}-${CI_COMMIT_REF_SLUG}
  HELM_RELEASE: ${CI_PROJECT_NAME}-${CI_COMMIT_REF_SLUG}

.docker-login: &docker-login
  before_script:
    - docker login -u $CI_REGISTRY_USER -p $CI_REGISTRY_PASSWORD $CI_REGISTRY

# 安全扫描阶段
security-scan:
  stage: security-scan
  image: aquasec/trivy:latest
  script:
    - trivy fs --exit-code 1 --severity HIGH,CRITICAL .
    - trivy fs --format sarif -o trivy-results.sarif .
  artifacts:
    reports:
      sast: trivy-results.sarif
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

# 构建阶段
build-app:
  stage: build
  image: docker:24-dind
  services:
    - docker:24-dind
  <<: *docker-login
  script:
    - docker build --pull -t $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA .
    - docker push $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
    - |
      if "$CI_COMMIT_BRANCH" == "$CI_DEFAULT_BRANCH"; then
        docker tag $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA $CI_REGISTRY_IMAGE:latest
        docker push $CI_REGISTRY_IMAGE:latest
      fi
  rules:
    - if: $CI_PIPELINE_SOURCE == "push"

# 单元测试
unit-test:
  stage: test
  image: node:20-alpine
  script:
    - npm ci
    - npm run test:coverage
  coverage: '/Statements\s*:\s*(\d+\.\d+)%/'
  artifacts:
    reports:
      coverage_report:
        coverage_format: cobertura
        path: coverage/cobertura-coverage.xml
    paths:
      - coverage/
    expire_in: 1 week

# 集成测试
integration-test:
  stage: test
  image: python:3.12-slim
  services:
    - postgres:16-alpine
    - redis:7-alpine
  variables:
    POSTGRES_DB: test_db
    POSTGRES_USER: test_user
    POSTGRES_PASSWORD: test_password
  script:
    - pip install -r requirements.txt
    - pytest -v --tb=short tests/integration/

# 部署到预发布环境
deploy-staging:
  stage: deploy-staging
  image: bitnami/kubectl:latest
  environment:
    name: staging
    url: https://staging.${CI_PROJECT_NAME}.example.com
  script:
    - kubectl config use-context staging
    - |
      helm upgrade --install ${HELM_RELEASE} ./helm/chart \
        --namespace ${KUBE_NAMESPACE} \
        --create-namespace \
        --set image.tag=${CI_COMMIT_SHA} \
        --set ingress.host=staging.${CI_PROJECT_NAME}.example.com \
        --atomic \
        --timeout 10m
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

# 端到端测试
e2e-test:
  stage: e2e-test
  image: cypress/included:13.0.0
  variables:
    CYPRESS_baseUrl: https://staging.${CI_PROJECT_NAME}.example.com
  script:
    - npm ci
    - npx cypress run --record --key $CYPRESS_RECORD_KEY
  artifacts:
    when: always
    paths:
      - cypress/screenshots/
      - cypress/videos/
    expire_in: 1 week
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

# 生产部署
deploy-production:
  stage: deploy-production
  image: bitnami/kubectl:latest
  environment:
    name: production
    url: https://${CI_PROJECT_NAME}.example.com
  script:
    - kubectl config use-context production
    - |
      helm upgrade --install ${HELM_RELEASE} ./helm/chart \
        --namespace ${KUBE_NAMESPACE} \
        --set image.tag=${CI_COMMIT_SHA} \
        --set ingress.host=${CI_PROJECT_NAME}.example.com \
        --atomic \
        --timeout 15m
  rules:
    - if: $CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+$/
  when: manual
  allow_failure: false

# 缓存配置
cache:
  key: ${CI_COMMIT_REF_SLUG}
  paths:
    - node_modules/
    - .m2/repository/
    - .gradle/
  policy: pull-push
```

## 3.3 流水线模板与复用

```yaml
# 流水线复用 - include 模板
include:
  - template: Jobs/Dependency-Scanning.gitlab-ci.yml
  - template: Jobs/Secret-Detection.gitlab-ci.yml
  - template: Jobs/SAST.gitlab-ci.yml
  - template: Jobs/Container-Scanning.gitlab-ci.yml

# 矩阵构建
parallel-build:
  stage: build
  parallel:
    matrix:
      - NODE_VERSION: ["18", "20", "22"]
        OS: ["alpine", "slim"]
  script:
    - docker build
        --build-arg NODE_VERSION=$NODE_VERSION
        -t $CI_REGISTRY_IMAGE:node${NODE_VERSION}-${OS} .

# extends 继承
.deploy-template:
  stage: deploy
  image: bitnami/kubectl:latest
  script:
    - helm upgrade --install ${HELM_RELEASE} ./helm/chart
        --namespace ${KUBE_NAMESPACE}
        --set image.tag=${CI_COMMIT_SHA}
        --atomic
        --timeout 10m
  when: manual

deploy-staging:
  extends: .deploy-template
  environment:
    name: staging
  variables:
    KUBE_NAMESPACE: staging
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

deploy-production:
  extends: .deploy-template
  environment:
    name: production
  variables:
    KUBE_NAMESPACE: production
  rules:
    - if: $CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+$/
```

---

<!-- chunk: 四、安全与合规 -->## 四、安全与合规

## 4.1 安全扫描集成

```yaml
# 安全扫描完整配置
security-sast:
  stage: security-scan
  image:
    name: registry.gitlab.com/security-products/sast:latest
    entrypoint: [""]
  variables:
    SAST_EXCLUDED_PATHS: "spec, test, tests, tmp"
    SAST_ANALYZER_IMAGE_PREFIX: "registry.gitlab.com/security-products"
  script:
    - /analyze -t sast
  artifacts:
    reports:
      sast: gl-sast-report.json
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

security-dependency-scanning:
  stage: security-scan
  image:
    name: registry.gitlab.com/security-products/dependency-scanning:latest
    entrypoint: [""]
  variables:
    DS_EXCLUDED_PATHS: "vendor, node_modules"
  script:
    - /analyze -t dependency_scanning
  artifacts:
    reports:
      dependency_scanning: gl-dependency-scanning-report.json
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

security-container-scanning:
  stage: security-scan
  image:
    name: registry.gitlab.com/security-products/container-scanning:latest
    entrypoint: [""]
  variables:
    CS_IMAGE: $CI_REGISTRY_IMAGE:$CI_COMMIT_SHA
  script:
    - /analyze -t container_scanning
  artifacts:
    reports:
      container_scanning: gl-container-scanning-report.json
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

security-license-scanning:
  stage: security-scan
  image:
    name: registry.gitlab.com/security-products/license-scanning:latest
    entrypoint: [""]
  script:
    - /analyze -t license_scanning
  artifacts:
    reports:
      license_scanning: gl-license-scanning-report.json
```

## 4.2 访问控制与环境保护

```yaml
# 分支保护规则
protected_branches:
  - name: main
    push_access_levels:
      - user: maintainers
    merge_access_levels:
      - user: developers
    unprotect_access_levels:
      - user: owners
    code_owner_approval_required: true

  - name: release/*
    push_access_levels:
      - user: maintainers
    merge_access_levels:
      - user: maintainers

# 环境保护规则
environments:
  production:
    deployment_tier: production
    state: available
    protected: true
    approval_rules:
      - name: "Production Approval"
        required_approvals: 2
        user_ids: [production-approvers-group]

  staging:
    deployment_tier: staging
    state: available
    approval_rules:
      - name: "Staging Approval"
        required_approvals: 1

# 合规流水线
compliance-pipeline:
  description: "企业合规流水线框架"
  pipeline:
    source: "project/compliance/ci-config@main"
    include:
      - template: Jobs/SAST.gitlab-ci.yml
      - template: Jobs/Secret-Detection.gitlab-ci.yml
      - template: Jobs/Dependency-Scanning.gitlab-ci.yml
```

---

<!-- chunk: 五、多环境管理策略 -->## 五、多环境管理策略

## 5.1 环境晋升流程

```yaml
# 多环境晋升策略
stages:
  - build
  - test
  - deploy-dev
  - deploy-staging
  - deploy-production

deploy-dev:
  stage: deploy-dev
  environment:
    name: dev/${CI_COMMIT_REF_NAME}
    url: https://dev-${CI_COMMIT_REF_SLUG}.example.com
    on_stop: stop-dev
  script:
    - helm upgrade --install ${CI_PROJECT_NAME}-${CI_COMMIT_REF_SLUG} ./helm
        --set image.tag=${CI_COMMIT_SHA}
        --set environment=dev
  rules:
    - if: $CI_PIPELINE_SOURCE == "push"

deploy-staging:
  stage: deploy-staging
  environment:
    name: staging
    url: https://staging.example.com
  script:
    - helm upgrade --install ${CI_PROJECT_NAME} ./helm
        --set image.tag=${CI_COMMIT_SHA}
        --set environment=staging
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH

deploy-production:
  stage: deploy-production
  environment:
    name: production
    url: https://app.example.com
  script:
    - helm upgrade --install ${CI_PROJECT_NAME} ./helm
        --set image.tag=${CI_COMMIT_SHA}
        --set environment=production
        --atomic
        --timeout 15m
  rules:
    - if: $CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+$/
  when: manual

stop-dev:
  stage: deploy-dev
  environment:
    name: dev/${CI_COMMIT_REF_NAME}
    action: stop
  script:
    - helm uninstall ${CI_PROJECT_NAME}-${CI_COMMIT_REF_SLUG}
  rules:
    - if: $CI_PIPELINE_SOURCE == "push"
      when: manual
```

## 5.2 Review Apps

```yaml
# 动态环境 - Review Apps
deploy-review:
  stage: deploy
  environment:
    name: review/${CI_MERGE_REQUEST_IID}
    url: https://review-${CI_MERGE_REQUEST_IID}.example.com
    on_stop: stop-review
  script:
    - helm upgrade --install review-${CI_MERGE_REQUEST_IID} ./helm
        --set image.tag=${CI_COMMIT_SHA}
        --set ingress.host=review-${CI_MERGE_REQUEST_IID}.example.com
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"

stop-review:
  stage: deploy
  environment:
    name: review/${CI_MERGE_REQUEST_IID}
    action: stop
  script:
    - helm uninstall review-${CI_MERGE_REQUEST_IID}
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
      when: manual
```

---

<!-- chunk: 六、监控与回滚 -->## 六、监控与回滚

## 6.1 关键指标监控

```yaml
groups:
- name: gitlab-ci.rules
  rules:
  - alert: GitLabPipelineFailureRateHigh
    expr: |
      rate(gitlab_ci_pipeline_duration_seconds_count{status="failed"}[5m]) /
      rate(gitlab_ci_pipeline_duration_seconds_count[5m]) * 100 > 10
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "流水线失败率过高"
      description: "最近5分钟内流水线失败率超过10%"

  - alert: GitLabRunnerOffline
    expr: gitlab_runner_up == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "GitLab Runner 离线"
      description: "Runner {{ $labels.instance }} 已离线超过5分钟"

  - alert: GitLabBuildQueueBacklog
    expr: gitlab_ci_pending_jobs > 50
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "构建队列积压"

  - alert: GitLabStorageUsageHigh
    expr: |
      (node_filesystem_size_bytes{mountpoint="/var/opt/gitlab"} -
       node_filesystem_free_bytes{mountpoint="/var/opt/gitlab"}) /
      node_filesystem_size_bytes{mountpoint="/var/opt/gitlab"} * 100 > 85
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "GitLab 存储空间不足"
```

## 6.2 回滚策略

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# GitLab 环境回滚
# 方式一: 重新部署旧版本
helm rollback ${HELM_RELEASE} <revision> -n production

# 方式二: 重新运行旧的 Pipeline Job
# GitLab UI → CI/CD → Pipelines → 选择成功的 Pipeline → Retry

# 方式三: 通过 API 触发回滚
curl --request POST \
  --form token=${TRIGGER_TOKEN} \
  --form ref=main \
  --form variables[DEPLOY_ACTION]=rollback \
  --form variables[DEPLOY_VERSION]=v1.2.3 \
  "https://gitlab.example.com/api/v4/projects/${PROJECT_ID}/trigger/pipeline"
```
---

<!-- chunk: 七、最佳实践 -->## 七、最佳实践

## 7.1 流水线优化

```yaml
1. 缓存策略:
   - 使用 cache 指令缓存依赖
   - 配置 pull-push 策略
   - 按 branch 设置不同的 cache key

2. 并行化:
   - 独立测试阶段使用 parallel
   - 使用 matrix 构建多版本
   - 避免不必要的串行依赖

3. 工件管理:
   - 设置合理的 expire_in
   - 使用 when: on_success 减少存储
   - 定期清理旧工件

4. Runner 资源:
   - 按阶段配置不同资源
   - 使用 node_selector 调度到专用节点
   - 监控 Runner 利用率

5. 安全扫描:
   - 所有 MR 触发安全扫描
   - 配置扫描失败阻断合并
   - 定期审查扫描报告
```

## 7.2 GitOps 集成

```yaml
# GitLab CI + Argo CD 集成
update-gitops:
  stage: deploy
  image: alpine/git:latest
  script:
    - git clone https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/org/gitops-manifests.git
    - cd gitops-manifests
    - kustomize edit set image app=${CI_REGISTRY_IMAGE}:${CI_COMMIT_SHA}
    - git config user.name "GitLab CI"
    - git config user.email "ci@example.com"
    - git add .
    - git commit -m "Update ${CI_PROJECT_NAME} to ${CI_COMMIT_SHA}"
    - git push origin main
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH
```

---

<!-- chunk: 八、故障排查 -->## 八、故障排查

## 8.1 常见问题诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# GitLab CI/CD 故障排查工具

check_pipeline_status() {
    echo "=== Pipeline Status ==="
    curl -s -H "PRIVATE-TOKEN: $GITLAB_TOKEN" \
      "https://gitlab.example.com/api/v4/projects/$PROJECT_ID/pipelines?per_page=10" | \
      jq '.[] | {id, status, ref, created_at}'
}

check_runner_health() {
    echo "=== Runner Health ==="
    curl -s -H "PRIVATE-TOKEN: $GITLAB_TOKEN" \
      "https://gitlab.example.com/api/v4/runners?scope=active" | \
      jq '.[] | {id, name, status, active}'
    kubectl top pods -n gitlab-runner
}

check_service_health() {
    echo "=== Service Health ==="
    kubectl exec -n gitlab-system sts/gitlab -- gitlab-ctl status
    kubectl exec -n gitlab-system sts/postgresql -- pg_isready
    kubectl exec -n gitlab-system sts/redis -- redis-cli ping
}

# Job 日志查看
# GitLab UI → CI/CD → Jobs → 选择失败的 Job → 查看日志

# Runner 调试
# 在 Job 中添加 before_script: - env (查看环境变量)
# 在 Job 中添加 before_script: - kubectl get pods (查看 Agent Pod)
```
```yaml
常见问题及解决方案:
  Job 超时:
    原因: 资源不足、网络问题、测试卡住
    解决: 增加 timeout、检查资源限制、添加测试超时

  Runner 不可用:
    原因: Runner 注册失败、K8s 资源不足
    解决: 检查 Runner 注册状态、检查 K8s 节点资源

  镜像拉取失败:
    原因: Registry 认证问题、网络不通
    解决: 检查 DOCKER_AUTH_CONFIG、验证 Registry 连通性

  缓存不生效:
    原因: cache key 不匹配、Runner 不支持分布式缓存
    解决: 检查 cache key 配置、配置 S3 缓存后端
```

---

<!-- chunk: 九、GitLab CI/CD 高级特性 -->## 九、GitLab CI/CD 高级特性

## 9.1 并行矩阵构建

GitLab CI/CD 的 `parallel: matrix` 功能允许在单个 Job 定义中生成多个并行构建实例。这对于需要在不同运行时版本、不同操作系统或不同配置组合下测试的项目尤其有用。矩阵构建的每个实例都会获得独立的运行环境和变量，构建结果在 Merge Request 中以可折叠的形式展示。

```yaml
# 多版本、多平台并行构建
build-matrix:
  stage: build
  parallel:
    matrix:
      - NODE_VERSION: ["18", "20", "22"]
        OS: ["alpine", "slim"]
  image: node:${NODE_VERSION}-${OS}
  script:
    - npm ci
    - npm run build
    - npm run test
  artifacts:
    paths:
      - dist/
    expire_in: 1 week
```

## 9.2 流水线缓存策略

缓存是加速 GitLab CI/CD 流水线的关键手段。GitLab 提供了分支级别的缓存隔离机制——默认情况下，每个分支只能访问自己的缓存和 main 分支的缓存。通过 `policy: pull-push` 配置，可以在首次运行时创建缓存，后续运行时优先拉取缓存。

```yaml
# 分级缓存策略
.cache-template: &cache-template
  cache:
    key:
      files:
        - package-lock.json
      prefix: "${CI_COMMIT_REF_SLUG}"
    paths:
      - node_modules/
      - .cache/
      - dist/
    policy: pull-push
    fallback_keys:
      - npm-default
      - npm-main

# 仅拉取缓存（用于MR流水线，避免写入）
.cache-pull: &cache-pull
  cache:
    key:
      files:
        - package-lock.json
      prefix: "${CI_COMMIT_REF_SLUG}"
    paths:
      - node_modules/
    policy: pull

build-with-cache:
  stage: build
  <<: [*cache-template]
  script:
    - npm ci
    - npm run build

test-mr:
  stage: test
  <<: [*cache-pull]
  script:
    - npm run test
  rules:
    - if: $CI_PIPELINE_SOURCE == "merge_request_event"
```

## 9.3 环境保护与审批流程

GitLab 的 Environment Protection 机制是生产部署安全的核心保障。通过配置环境保护规则，可以强制要求特定环境的部署必须经过指定审批人的确认，同时可以设置等待计时器防止误操作。结合 GitLab 的合规框架（Compliance Framework），可以实现跨项目的统一审批策略。

```yaml
# 多层级审批配置
deploy-production:
  stage: deploy-production
  environment:
    name: production/${CI_PROJECT_NAME}
    url: https://${CI_PROJECT_NAME}.example.com
    deployment_tier: production
    action: start
  before_script:
    - echo "Deploying to production requires approval from ${APPROVER_TEAM}"
  script:
    - helm upgrade --install ${CI_PROJECT_NAME} ./helm
        --set image.tag=${CI_COMMIT_SHA}
        --atomic
        --timeout 15m
  rules:
    - if: $CI_COMMIT_TAG =~ /^v\d+\.\d+\.\d+$/
      when: manual
      allow_failure: false
  needs:
    - job: e2e-test
      artifacts: false
  resource_group: production-deploy

# 资源组 - 确保同一环境同时只有一个部署
resource_groups:
  - key: production-deploy
    process_mode: unordered
```

## 9.4 GitLab 与 Argo CD 深度集成

GitLab CI/CD 与 Argo CD 的集成是企业级 GitOps 的标准模式。在这种模式下，GitLab CI 负责构建镜像和运行测试，Argo CD 负责将镜像部署到 Kubernetes 集群。集成点在于 GitLab CI 在流水线成功后将镜像标签更新到 GitOps 清单仓库，Argo CD 自动检测到变更并同步部署。

```yaml
# GitLab CI → GitOps 集成
update-gitops:
  stage: deploy
  image: alpine/git:latest
  variables:
    GITOPS_REPO: "https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/org/gitops-manifests.git"
  script:
    - git clone --depth 1 ${GITOPS_REPO} gitops
    - cd gitops/apps/${CI_PROJECT_NAME}/overlays/production
    - kustomize edit set image app=${CI_REGISTRY_IMAGE}:${CI_COMMIT_SHA}
    - cd -
    - git config user.name "GitLab CI"
    - git config user.email "ci@gitlab.example.com"
    - git add .
    - git commit -m "Update ${CI_PROJECT_NAME} to ${CI_COMMIT_SHA}"
    - git push origin main
  rules:
    - if: $CI_COMMIT_BRANCH == $CI_DEFAULT_BRANCH
```

## 9.5 GitLab Runner 运维管理

GitLab Runner 的运维管理包括注册注销、版本升级、资源监控和弹性伸缩。在 Kubernetes 环境中，推荐使用 Helm Chart 部署 Runner，通过 `values.yaml` 管理配置，实现声明式的 Runner 管理。

```yaml
# GitLab Runner Helm Values
replicas: 3
runnerRegistrationToken: "${RUNNER_REGISTRATION_TOKEN}"
rbac:
  create: true
  resources:
    - pods
    - pods/exec
    - pods/attach
    - secrets
    - configmaps
runners:
  config: |
    runners
      executor = "kubernetes"
      [runners.kubernetes]
        namespace = "gitlab-runner"
        image = "ubuntu:22.04"
        privileged = true
        [runners.kubernetes.node_selector]
          gitlab-runner = "true"
resources:
  requests:
    memory: 256Mi
    cpu: 100m
  limits:
    memory: 512Mi
    cpu: 500m
```

---

**文档版本**: v2.0
**最后更新**: 2026-04-24
**适用版本**: GitLab 17.10+

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 发布变更 MOC
- [[11-发布变更/README.md|Domain 08: GitOps与CI/CD (GitOps & CI/CD)]]
- Domain-23 GitOps & CI/CD — 开源项目索引
- Argo CD企业级GitOps实践指南
- Jenkins企业级CI/CD流水线深度实践
- GitHub Actions Enterprise CI/CD Platform 深度实践
- Tekton 云原生 CI/CD 深度实践
- Flux v2 GitOps 持续交付深度实践
- GitOps 安全与合规深度实践
- CI/CD 流水线模式与渐进式交付深度实践
- Argo CD 企业级 GitOps 实践指南
- Flux GitOps 实践指南

## See Also

- 01-argo-cd-enterprise-gitops
- 02-jenkins-enterprise-cicd
- 04-github-actions-enterprise
- 05-tekton-cloud-native-cicd

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
