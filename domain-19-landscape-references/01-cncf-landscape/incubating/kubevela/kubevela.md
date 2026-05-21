---
title: KubeVela
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- argocd
- flux
- redis
- hpa
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- KubeVela 是什么
- 如何 KubeVela
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- KubeVela
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- redis-basics
---

title: KubeVela
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- argocd
- flux
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- KubeVela 是什么
- 如何 KubeVela
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- KubeVela
- cncf
- landscape
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

# KubeVela

> **成熟度**: Incubating | **加入时间**: 2021-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubevela.io |
| **GitHub** | https://github.com/kubevela/kubevela |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Delivery |

---

## 项目概述

KubeVela 是现代应用交付平台，实现了开放应用模型（OAM）规范。它为开发者提供以应用为中心的抽象，简化 Kubernetes 上的应用部署、运维和多集群管理。

## 核心特性

- **应用抽象**: 以应用为中心，屏蔽底层 Kubernetes 复杂性
- **OAM 模型**: 组件、特征、策略的标准化定义
- **多集群交付**: 统一管理多个 Kubernetes 集群的应用
- **GitOps**: 与 Flux/ArgoCD 集成实现 GitOps 工作流
- **可扩展**: CUE 语言定义自定义组件和特征
- **工作流**: 内置应用交付工作流引擎

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     KubeVela Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   User Interfaces                          │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐ │ │
│  │  │  VelaUX  │  │   CLI    │  │  GitOps  │  │    API    │ │ │
│  │  │  (Web)   │  │  (vela)  │  │ (Flux)   │  │           │ │ │
│  │  └──────────┘  └──────────┘  └──────────┘  └───────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Application CRD                          │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  Component     │  Trait        │  Policy    │ Workflow│ │ │
│  │  │  (webservice)  │  (scaler)     │  (topology)│ (deploy)│ │ │
│  │  │  (task)        │  (gateway)    │  (override)│ (notify)│ │ │
│  │  │  (daemon)      │  (sidecar)    │            │         │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   KubeVela Core                            │ │
│  │  ┌───────────────┐  ┌───────────────┐  ┌────────────────┐│ │
│  │  │  Application  │  │   Workflow    │  │  Multi-Cluster ││ │
│  │  │  Controller   │  │   Engine      │  │   Dispatcher   ││ │
│  │  └───────────────┘  └───────────────┘  └────────────────┘│ │
│  │                                                            │ │
│  │  ┌───────────────┐  ┌───────────────┐                    │ │
│  │  │    CUE        │  │  Definition   │                    │ │
│  │  │    Engine     │  │   Registry    │                    │ │
│  │  └───────────────┘  └───────────────┘                    │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│      ┌───────────────────────┼───────────────────────┐         │
│      ▼                       ▼                       ▼         │
│  ┌─────────┐          ┌─────────┐           ┌─────────┐       │
│  │Cluster 1│          │Cluster 2│           │Cluster N│       │
│  │ (Prod)  │          │ (Stage) │           │ (Dev)   │       │
│  └─────────┘          └─────────┘           └─────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### OAM 核心概念

| 概念 | 说明 |
|------|------|
| Component | 应用组件（如 Web 服务、任务、守护进程） |
| Trait | 运维能力（如扩缩容、网关、日志） |
| Policy | 部署策略（如拓扑、覆盖配置） |
| Workflow | 交付工作流步骤 |

---

## 快速开始

### 安装 KubeVela

```bash
# 安装 CLI
curl -fsSl https://kubevela.io/script/install.sh | bash

# 安装 KubeVela 控制器
vela install

# 安装 VelaUX（Web UI）
vela addon enable velaux
```

### 部署第一个应用

```yaml
# first-app.yaml
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: first-app
spec:
  components:
    - name: express-server
      type: webservice
      properties:
        image: oamdev/hello-world
        ports:
          - port: 8000
            expose: true
      traits:
        - type: scaler
          properties:
            replicas: 2
        - type: gateway
          properties:
            domain: hello.example.com
            http:
              "/": 8000
```

```bash
# 部署应用
vela up -f first-app.yaml

# 查看状态
vela status first-app

# 端口转发访问
vela port-forward first-app 8000:8000
```

---

## 组件类型

### Webservice（Web 服务）

```yaml
components:
  - name: api-server
    type: webservice
    properties:
      image: nginx:latest
      ports:
        - port: 80
          expose: true
      env:
        - name: NODE_ENV
          value: production
      cpu: "0.5"
      memory: "512Mi"
```

### Worker（后台任务）

```yaml
components:
  - name: worker
    type: worker
    properties:
      image: my-worker:latest
      cmd: ["python", "worker.py"]
```

### Task（一次性任务）

```yaml
components:
  - name: db-migration
    type: task
    properties:
      image: my-app:latest
      cmd: ["python", "migrate.py"]
      count: 1
      restart: Never
```

### Helm Chart

```yaml
components:
  - name: redis
    type: helm
    properties:
      repoType: helm
      url: https://charts.bitnami.com/bitnami
      chart: redis
      version: "17.0.0"
      values:
        auth:
          enabled: false
```

---

## 运维特征 (Traits)

```yaml
traits:
  # 自动扩缩容
  - type: scaler
    properties:
      replicas: 3
      
  # HPA
  - type: hpa
    properties:
      min: 1
      max: 10
      cpu: 80
      
  # 网关/路由
  - type: gateway
    properties:
      domain: api.example.com
      http:
        "/api": 8080
        
  # Sidecar
  - type: sidecar
    properties:
      name: logging-agent
      image: fluent/fluent-bit
      
  # 存储
  - type: storage
    properties:
      pvc:
        - name: data
          mountPath: /data
          storageClassName: standard
          size: 10Gi
```

---

## 多集群部署

### 添加集群

```bash
# 添加子集群
vela cluster join <kubeconfig-path> --name prod-cluster

# 查看集群
vela cluster list
```

### 多集群策略

```yaml
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: multi-cluster-app
spec:
  components:
    - name: nginx
      type: webservice
      properties:
        image: nginx:latest
        
  policies:
    - name: topology-policy
      type: topology
      properties:
        clusters: ["prod-cluster", "staging-cluster"]
        namespace: production
        
    - name: override-policy
      type: override
      properties:
        components:
          - name: nginx
            traits:
              - type: scaler
                properties:
                  replicas: 5  # 生产环境更多副本
```

---

## 工作流

```yaml
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: workflow-app
spec:
  components:
    - name: nginx
      type: webservice
      properties:
        image: nginx:latest
        
  workflow:
    steps:
      - name: deploy-staging
        type: deploy
        properties:
          policies: ["staging-policy"]
          
      - name: manual-approval
        type: suspend
        
      - name: deploy-production
        type: deploy
        properties:
          policies: ["production-policy"]
          
      - name: notify
        type: notification
        properties:
          slack:
            url: https://hooks.slack.com/xxx
            message: "App deployed to production"
```

---

## 自定义定义 (CUE)

```cue
// my-webservice.cue
"my-webservice": {
    type: "component"
    attributes: workload: definition: {
        apiVersion: "apps/v1"
        kind:       "Deployment"
    }
    template: {
        output: {
            apiVersion: "apps/v1"
            kind:       "Deployment"
            spec: {
                selector: matchLabels: app: context.name
                template: {
                    metadata: labels: app: context.name
                    spec: containers: [{
                        name:  context.name
                        image: parameter.image
                        ports: [{containerPort: parameter.port}]
                    }]
                }
            }
        }
        parameter: {
            image: string
            port:  *80 | int
        }
    }
}
```

---

## 最佳实践

1. **模块化组件**: 使用 Helm/Kustomize 复用现有配置
2. **环境隔离**: 通过 Policy 实现多环境差异化配置
3. **渐进交付**: 使用 Workflow 实现分阶段发布
4. **GitOps**: 将 Application YAML 存储在 Git 仓库
5. **可观测性**: 集成 Prometheus/Grafana 监控应用状态

---

## 参考资源

- [官方文档](https://kubevela.io/docs)
- [GitHub Repo](https://github.com/kubevela/kubevela)
- [OAM 规范](https://oam.dev/)
- [Addon 目录](https://kubevela.io/docs/reference/addons/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
