---
title: Meshery
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- istio
- cilium
- helm
- docker
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Meshery 是什么
- 如何 Meshery
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Meshery
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- cilium-basics
---

title: Meshery
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- istio
- cilium
- helm
- docker
- wasm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Meshery 是什么
- 如何 Meshery
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Meshery
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

# Meshery

> **成熟度**: Sandbox | **加入时间**: 2021-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://meshery.io |
| **GitHub** | https://github.com/meshery/meshery |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, JavaScript |
| **CNCF 分类** | Service Mesh Management |
| **维护组织** | Layer5 |

---

## 项目概述

Meshery 是云原生管理平面，提供服务网格和云原生基础设施的生命周期管理。它支持多种服务网格 (Istio, Linkerd, Consul, Kuma, NSM 等) 的安装、配置、性能测试和运维管理，并提供统一的 Web 界面和 CLI。Meshery 还定义了 MeshModel 标准，用于描述云原生基础设施。

---

## 核心特性

- **多网格支持**: 管理 10+ 种服务网格
- **生命周期管理**: 安装、升级、卸载服务网格
- **性能测试**: 内置负载测试和性能比较
- **配置管理**: 统一界面管理网格配置
- **MeshModel**: 云原生基础设施建模标准
- **设计模式**: 预定义的云原生部署模式
- **适配器架构**: 可扩展的适配器支持新网格

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Meshery Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    User Interface                         │   │
│  │  ┌─────────────────┐  ┌──────────────┐  ┌────────────┐  │   │
│  │  │   Meshery UI    │  │  mesheryctl  │  │  REST API  │  │   │
│  │  │  (React)        │  │  (CLI)       │  │  /GraphQL  │  │   │
│  │  └────────┬────────┘  └──────┬───────┘  └─────┬──────┘  │   │
│  └───────────┼─────────────────┼──────────────────┼─────────┘  │
│              │                 │                   │             │
│  ┌───────────▼─────────────────▼───────────────────▼─────────┐  │
│  │                    Meshery Server                          │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │                Core Components                       │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │  │  │
│  │  │  │  Lifecycle  │  │ Performance │  │   Config   │  │  │  │
│  │  │  │  Manager    │  │   Manager   │  │   Manager  │  │  │  │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │  │  │
│  │  │  │  MeshModel  │  │   Pattern   │  │   Events   │  │  │  │
│  │  │  │   Engine    │  │   Engine    │  │   System   │  │  │  │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                           │                                │  │
│  │  ┌────────────────────────▼────────────────────────────┐  │  │
│  │  │                  Mesh Adapters                       │  │  │
│  │  │  ┌──────┐ ┌──────┐ ┌───────┐ ┌──────┐ ┌─────────┐  │  │  │
│  │  │  │Istio │ │Linkerd│ │Consul │ │ Kuma │ │  NGINX  │  │  │  │
│  │  │  └──────┘ └──────┘ └───────┘ └──────┘ └─────────┘  │  │  │
│  │  │  ┌──────┐ ┌──────┐ ┌───────┐ ┌──────────────────┐  │  │  │
│  │  │  │ NSM  │ │Traefik│ │Cilium │ │   App Mesh      │  │  │  │
│  │  │  └──────┘ └──────┘ └───────┘ └──────────────────┘  │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│  ┌───────────────────────────▼───────────────────────────────┐  │
│  │                 Kubernetes Clusters                         │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │  │
│  │  │  Cluster A  │  │  Cluster B  │  │   Cluster C     │   │  │
│  │  │  (Istio)    │  │  (Linkerd)  │  │   (Consul)      │   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Meshery Server** | 核心服务器，管理适配器和集群 |
| **Meshery UI** | Web 管理界面，基于 React |
| **mesheryctl** | CLI 工具，本地管理和操作 |
| **Adapters** | 服务网格适配器，支持各种网格 |
| **MeshModel** | 云原生基础设施建模引擎 |

---

## 快速开始

### 安装 mesheryctl

```bash
# macOS/Linux (Homebrew)
brew install mesheryctl

# Linux (curl)
curl -L https://meshery.io/install | bash -

# Windows (Scoop)
scoop install mesheryctl
```

### 启动 Meshery

```bash
# 使用 Docker 启动
mesheryctl system start

# 使用 Kubernetes 部署
mesheryctl system start --platform kubernetes

# 指定提供商
mesheryctl system start --provider meshery

# 访问 UI
# 打开 http://localhost:9081
```

### Helm 安装

```bash
helm repo add meshery https://meshery.io/charts/
helm repo update

helm install meshery meshery/meshery \
  --namespace meshery \
  --create-namespace
```

---

## 服务网格管理

### 安装服务网格

```bash
# 列出可用适配器
mesheryctl mesh list

# 安装 Istio
mesheryctl mesh deploy --adapter istio

# 安装 Linkerd
mesheryctl mesh deploy --adapter linkerd

# 安装到指定命名空间
mesheryctl mesh deploy --adapter istio --namespace istio-system
```

### 通过 UI 管理

1. 打开 Meshery UI (http://localhost:9081)
2. 导航到 Lifecycle > Mesh Deployment
3. 选择服务网格和配置
4. 点击 Deploy

---

## 性能测试

### CLI 性能测试

```bash
# 基本负载测试
mesheryctl perf apply \
  --name "my-load-test" \
  --url "http://my-service:8080" \
  --qps 100 \
  --concurrent-requests 10 \
  --duration 60s \
  --mesh istio

# 使用 SMP 配置文件
mesheryctl perf apply --file perf-config.yaml
```

### 性能测试配置

```yaml
# perf-config.yaml
apiVersion: v1alpha1
kind: PerformanceProfile
metadata:
  name: baseline-test
spec:
  name: "Istio Baseline"
  loadGenerators:
    - type: fortio
      config:
        url: "http://productpage:9080/productpage"
        qps: 200
        connections: 5
        duration: "120s"
        headers:
          Content-Type: "application/json"
  scheduledAt: ""
  meshName: istio
```

### 比较测试

```bash
# 对比不同网格的性能
mesheryctl perf apply \
  --name "istio-test" \
  --mesh istio \
  --url http://svc:8080 \
  --qps 500 --duration 120s

mesheryctl perf apply \
  --name "linkerd-test" \
  --mesh linkerd \
  --url http://svc:8080 \
  --qps 500 --duration 120s

# 在 UI 中查看对比结果
```

---

## 设计模式 (Patterns)

### 使用预定义模式

```bash
# 列出可用模式
mesheryctl pattern list

# 应用模式
mesheryctl pattern apply --file canary-pattern.yaml

# 删除模式
mesheryctl pattern delete --file canary-pattern.yaml
```

### 模式定义

```yaml
# canary-deployment.yaml
name: Canary Deployment
services:
  frontend:
    type: Deployment
    namespace: default
    settings:
      replicas: 3
    traits:
      meshmap:
        meshName: istio
        trafficPolicy:
          canary:
            weight: 90
          canaryWeight: 10
```

---

## MeshModel

### 组件模型

```bash
# 列出 MeshModel 组件
mesheryctl model list

# 查看特定模型
mesheryctl model view kubernetes

# 导出模型
mesheryctl model export kubernetes --output model.json
```

### 关系定义

```yaml
# MeshModel 关系示例
apiVersion: model.meshery.io/v1alpha1
kind: Relationship
metadata:
  name: deployment-service
spec:
  type: binding
  subType: mount
  from:
    kind: Deployment
    model: kubernetes
  to:
    kind: Service
    model: kubernetes
```

---

## 多集群管理

### 连接集群

```bash
# 添加 Kubernetes 集群
mesheryctl system config kubernetes

# 使用特定 kubeconfig
mesheryctl system config kubernetes --kubeconfig /path/to/config

# 列出已连接集群
mesheryctl system context list
```

### 在 UI 中管理

- 导航到 Settings > Kubernetes
- 上传 kubeconfig 或输入 API Server URL
- 选择目标集群进行操作

---

## 常用命令

```bash
# 系统管理
mesheryctl system start              # 启动 Meshery
mesheryctl system stop               # 停止 Meshery
mesheryctl system restart            # 重启
mesheryctl system status             # 查看状态
mesheryctl system update             # 更新
mesheryctl system reset              # 重置

# 网格管理
mesheryctl mesh list                 # 列出网格
mesheryctl mesh deploy --adapter istio  # 部署网格
mesheryctl mesh remove --adapter istio  # 移除网格

# 性能测试
mesheryctl perf apply --name test    # 运行测试
mesheryctl perf list                 # 列出测试
mesheryctl perf view <id>            # 查看结果

# 模式管理
mesheryctl pattern list              # 列出模式
mesheryctl pattern apply -f p.yaml   # 应用模式

# 过滤器
mesheryctl filter list               # 列出 WASM 过滤器
mesheryctl filter apply -f f.wasm    # 应用过滤器
```

---

## 扩展和适配器

### 适配器开发

```go
// adapter.go - 适配器接口
type Adapter interface {
    ApplyOperation(ctx context.Context, op OperationRequest) error
    ListOperations() ([]Operation, error)
    StreamEvents(ctx context.Context) (<-chan Event, error)
}
```

### 支持的适配器

| 适配器 | 服务网格 | 状态 |
|:---|:---|:---|
| meshery-istio | Istio | 稳定 |
| meshery-linkerd | Linkerd | 稳定 |
| meshery-consul | Consul Connect | 稳定 |
| meshery-kuma | Kuma | 稳定 |
| meshery-nsm | Network Service Mesh | Beta |
| meshery-nginx-sm | NGINX Service Mesh | Beta |
| meshery-traefik-mesh | Traefik Mesh | Beta |
| meshery-cilium | Cilium | Beta |

---

## 最佳实践

1. **渐进评估**: 使用 Meshery 对比测试不同网格
2. **性能基线**: 在部署网格前建立性能基线
3. **模式复用**: 使用设计模式标准化部署
4. **多集群视图**: 统一管理多集群网格部署
5. **持续测试**: 定期运行性能测试检测退化
6. **社区模式**: 贡献和复用社区设计模式

---

## 参考资源

- [官方文档](https://docs.meshery.io)
- [GitHub Repo](https://github.com/meshery/meshery)
- [MeshModel](https://docs.meshery.io/concepts/logical/models)
- [设计模式目录](https://meshery.io/catalog)
- [性能管理](https://docs.meshery.io/tasks/performance/managing-performance)
- [Layer5 社区](https://layer5.io/community)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
