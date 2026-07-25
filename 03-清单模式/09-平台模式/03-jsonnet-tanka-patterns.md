---
title: "Jsonnet/Tanka 模式：大型 K8s 项目的配置管理"
description: "Jsonnet 语言特性与 Tanka 环境管理，涵盖与 Kustomize 对比、大型项目组织及生产实践"
summary: "系统讲解 Jsonnet 语言在 Kubernetes 配置管理中的应用：Jsonnet 函数式特性、Tanka 环境管理与部署、与 Kustomize/Helm 的对比、大型多环境项目的组织结构及生产实践"
category: 清单模式
tags:
- jsonnet
- tanka
- configuration
- functional
- multi-environment
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
- "Jsonnet 怎么管理 K8s 配置"
- "Tanka 和 Kustomize 怎么选"
- "大型 K8s 项目配置怎么组织"
trigger_keywords:
- jsonnet
- tanka
- jsonnet-lib
- multi-environment
- configuration
prerequisites:
- kubectl-basics
- yaml-basics
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

# Jsonnet/Tanka 模式

## 概述

Jsonnet 是 Google 开源的数据模板语言（Jsonnet = JSON + Network），设计目标是替代 JSON/YAML 作为配置语言。它提供变量、函数、继承、条件表达式等编程能力，同时保持纯函数式（无副作用）和确定性输出。Tanka 是 Grafana Labs 开发的 Jsonnet 配置管理工具，提供环境管理、差异对比和部署编排。

Jsonnet/Tanka 在大型 Kubernetes 项目（如 Grafana 自身、Bitnami 内部平台）中广泛使用，特别适合需要**大量复用、多环境差异化、复杂条件逻辑**的场景。相比 Helm 的模板方式和 Kustomize 的补丁叠加，Jsonnet 的函数式组合更灵活、更可测试。

## 核心概念

### Jsonnet 语言特性

| 特性 | 说明 | 示例 |
|------|------|------|
| 变量 | `local` 绑定 | `local replicas = 3;` |
| 函数 | 参数化模板 | `local deploy(name, replicas) = {...}` |
| 继承 | `+` 合并对象 | `base + {spec+: {replicas: 5}}` |
| 条件 | `if/then/else` | `if env == "prod" then 5 else 2` |
| 推导 | 列表/对象推导 | `{["key"+i]: val for i in std.range(1,3)}` |
| 标准库 | `std.*` 函数 | `std.map()`, `std.filter()` |
| 隐藏字段 | `::` 不输出 | `local config:: {...}` |
| 断言 | `assert` 验证 | `assert replicas > 0 : "must be positive"` |

### Jsonnet vs Helm vs Kustomize vs CUE

| 维度 | Jsonnet/Tanka | Helm | Kustomize | CUE |
|------|--------------|------|-----------|-----|
| 范式 | 函数式 | 模板 | 补丁叠加 | 约束统一 |
| 类型安全 | 弱（运行时） | 无 | 无 | 强（编译时） |
| 复用 | 函数 + 继承 | Chart 依赖 | Base + Overlay | 定义 + 统一 |
| 条件逻辑 | 完整（if/for） | Go template | 有限 | 有限 |
| 多环境 | Tanka environments | values 文件 | Overlay 目录 | 统一覆盖 |
| 测试 | jsonnet unit test | helm unittest | 无 | cue vet |
| 学习曲线 | 中-高 | 中 | 低 | 高 |
| 生态 | 中（Grafana 生态） | 高 | 高 | 中 |
| 适用规模 | 大型（100+ 资源） | 中型 | 小型-中型 | 中型-大型 |

### Tanka 架构

```
tanka-project/
├── jsonnetfile.json        # 依赖管理
├── environments/           # 环境定义
│   ├── production/
│   │   ├── main.jsonnet    # 环境入口
│   │   └── spec.json       # 集群连接信息
│   ├── staging/
│   │   ├── main.jsonnet
│   │   └── spec.json
│   └── development/
│       ├── main.jsonnet
│       └── spec.json
├── components/             # 可复用组件
│   ├── deployment.libsonnet
│   ├── service.libsonnet
│   └── ingress.libsonnet
├── services/               # 服务定义
│   ├── api-server/
│   │   └── main.jsonnet
│   └── web-frontend/
│       └── main.jsonnet
└── vendor/                 # 依赖（jsonnet-bundler）
```

## 生产部署

### Tanka 项目初始化

```bash
# 🟢 低风险：初始化 Tanka 项目
# 安装工具
go install github.com/grafana/tanka/cmd/tk@latest
go install github.com/jsonnet-bundler/jsonnet-bundler/cmd/jb@latest

# 初始化项目
mkdir tanka-platform && cd tanka-platform
tk init

# 添加环境
tk env add environments/production --server=https://k8s-prod.example.com:6443 --namespace=production
tk env add environments/staging --server=https://k8s-staging.example.com:6443 --namespace=staging

# 添加依赖（jsonnet-bundler）
jb install github.com/grafana/jsonnet-libs/k8s-libsonnet/1.30@main
jb install github.com/grafana/jsonnet-libs/nginx@master
```

### 可复用组件

```jsonnet
// components/deployment.libsonnet
// 🟢 低风险：Deployment 组件库
local k = import 'k8s-libsonnet/1.30/main.libsonnet';

{
  // 标准 Deployment 工厂函数
  new(name, namespace, image, replicas=2, port=8080, resources={}):: {
    apiVersion: 'apps/v1',
    kind: 'Deployment',
    metadata: {
      name: name,
      namespace: namespace,
      labels: {
        'app.kubernetes.io/name': name,
        'app.kubernetes.io/managed-by': 'tanka',
      },
    },
    spec: {
      replicas: replicas,
      selector: {
        matchLabels: { app: name },
      },
      template: {
        metadata: {
          labels: { app: name },
        },
        spec: {
          securityContext: {
            runAsNonRoot: true,
            runAsUser: 1000,
            fsGroup: 1000,
          },
          containers: [{
            name: name,
            image: image,
            ports: [{ containerPort: port }],
            resources: resources + {
              requests: { cpu: '100m', memory: '128Mi' },
              limits: { cpu: '1', memory: '1Gi' },
            },
            securityContext: {
              allowPrivilegeEscalation: false,
              readOnlyRootFilesystem: true,
              capabilities: { drop: ['ALL'] },
            },
            livenessProbe: {
              httpGet: { path: '/healthz', port: port },
              initialDelaySeconds: 10,
              periodSeconds: 15,
            },
            readinessProbe: {
              httpGet: { path: '/ready', port: port },
              initialDelaySeconds: 5,
              periodSeconds: 10,
            },
          }],
        },
      },
    },
  },

  // 添加 HPA
  withHPA(deployment, minReplicas=2, maxReplicas=10, targetCPU=70):: {
    apiVersion: 'autoscaling/v2',
    kind: 'HorizontalPodAutoscaler',
    metadata: {
      name: deployment.metadata.name + '-hpa',
      namespace: deployment.metadata.namespace,
    },
    spec: {
      scaleTargetRef: {
        apiVersion: 'apps/v1',
        kind: 'Deployment',
        name: deployment.metadata.name,
      },
      minReplicas: minReplicas,
      maxReplicas: maxReplicas,
      metrics: [{
        type: 'Resource',
        resource: {
          name: 'cpu',
          target: { type: 'Utilization', averageUtilization: targetCPU },
        },
      }],
    },
  },
}
```

### 服务定义

```jsonnet
// services/api-server/main.jsonnet
// 🟢 低风险：服务配置
local deploy = import '../../components/deployment.libsonnet';
local svc = import '../../components/service.libsonnet';

local env = std.extVar('__tkEnv');
local config = import '../../config/' + env + '.libsonnet';

local name = 'api-server';
local namespace = config.namespace;

{
  deployment: deploy.new(
    name=name,
    namespace=namespace,
    image='registry.example.com/api-server:' + config.apiServer.version,
    replicas=config.apiServer.replicas,
    port=8080,
    resources={
      requests: { cpu: config.apiServer.cpu, memory: config.apiServer.memory },
      limits: { cpu: config.apiServer.cpuLimit, memory: config.apiServer.memoryLimit },
    },
  ),

  service: svc.new(name=name, namespace=namespace, port=8080),

  hpa: deploy.withHPA(
    deployment=$.deployment,
    minReplicas=config.apiServer.minReplicas,
    maxReplicas=config.apiServer.maxReplicas,
  ),

  // 条件资源：仅生产环境创建 PDB
  [if env == 'production' then 'pdb']: {
    apiVersion: 'policy/v1',
    kind: 'PodDisruptionBudget',
    metadata: { name: name + '-pdb', namespace: namespace },
    spec: {
      minAvailable: 1,
      selector: { matchLabels: { app: name } },
    },
  },
}
```

### 环境配置

```jsonnet
// config/production.libsonnet
// 🟡 中风险：生产环境配置
{
  namespace: 'production',
  apiServer: {
    version: '2.1.0',
    replicas: 5,
    minReplicas: 3,
    maxReplicas: 20,
    cpu: '1',
    memory: '1Gi',
    cpuLimit: '2',
    memoryLimit: '2Gi',
  },
  webFrontend: {
    version: '3.0.1',
    replicas: 3,
    minReplicas: 2,
    maxReplicas: 10,
    cpu: '500m',
    memory: '512Mi',
    cpuLimit: '1',
    memoryLimit: '1Gi',
  },
  features: {
    enableCanary: true,
    enableNetworkPolicy: true,
  },
}
```

### 环境入口

```jsonnet
// environments/production/main.jsonnet
// 🟡 中风险：生产环境入口
local apiServer = import '../../services/api-server/main.jsonnet';
local webFrontend = import '../../services/web-frontend/main.jsonnet';

// 合并所有服务资源
apiServer + webFrontend
```

### 部署操作

```bash
# 🟡 中风险：Tanka 部署
# 预览变更（diff）
tk diff environments/production

# 应用配置
tk apply environments/production

# 仅导出 YAML（不部署）
tk export environments/production --format yaml

# 查看渲染结果
tk show environments/production

# 删除环境资源
# 🔴 高风险：删除所有环境资源
tk delete environments/production
```

## 运维操作

### 多环境管理

```bash
# 🟢 低风险：环境管理
# 列出所有环境
tk env list

# 查看环境详情
tk env get environments/production

# 对比两个环境
diff <(tk show environments/staging) <(tk show environments/production)

# 添加新环境
tk env add environments/canary --server=https://k8s-canary.example.com:6443

# 并行部署多环境
for env in staging production; do
  echo "=== Deploying $env ==="
  tk apply environments/$env --auto-approve
done
```

### 依赖管理

```bash
# 🟢 低风险：Jsonnet 依赖管理
# 查看依赖
jb list

# 更新依赖
jb update

# 添加新依赖
jb install github.com/grafana/jsonnet-libs/prometheus@master

# 验证依赖完整性
jb install  # 重新安装所有依赖
```

### 测试

```bash
# 🟢 低风险：Jsonnet 单元测试
# 使用 jsonnetunit 或 tanka 内置测试
# tests/deployment_test.jsonnet
local deploy = import '../components/deployment.libsonnet';

{
  test_replicas_default: deploy.new('test', 'default', 'img:v1').spec.replicas == 2,
  test_replicas_custom: deploy.new('test', 'default', 'img:v1', replicas=5).spec.replicas == 5,
  test_security_context: deploy.new('test', 'default', 'img:v1').spec.template.spec.securityContext.runAsNonRoot == true,
}

# 运行测试
jsonnet tests/deployment_test.jsonnet
# 所有断言应返回 true
```

## 故障排查

### 常见问题

```bash
# 🟢 低风险：Jsonnet/Tanka 问题诊断
# 问题 1：Jsonnet 编译错误
tk show environments/production 2>&1
# 错误信息通常包含行号和原因

# 问题 2：环境连接失败
tk env get environments/production
# 检查 spec.json 中的 server 地址和证书

# 问题 3：资源冲突
tk apply environments/production
# 错误：resource already exists
# 解决：检查是否有其他工具管理了相同资源

# 问题 4：依赖版本冲突
jb list
# 检查 vendor/ 目录中的版本
cat vendor/github.com/grafana/jsonnet-libs/k8s-libsonnet/1.30/main.libsonnet | head -5
```

## 最佳实践

### 项目组织

1. **组件与服务分离**：`components/` 放可复用模板，`services/` 放具体服务配置
2. **环境配置外置**：环境差异通过 `config/<env>.libsonnet` 管理，不散落在代码中
3. **函数参数化**：组件通过函数参数接收配置，避免硬编码
4. **断言验证**：使用 `assert` 在编译时捕获配置错误
5. **命名规范**：文件名使用 `kebab-case`，Jsonnet 变量使用 `camelCase`

### 生产建议

1. **CI 中验证**：PR 时运行 `tk diff` 展示变更，`tk show` 验证渲染结果
2. **渐进式部署**：先 staging 后 production，使用 `tk diff` 确认变更
3. **版本锁定**：`jsonnetfile.lock.json` 锁定依赖版本
4. **与 [[03-清单模式/09-平台模式/02-cue-language-configuration|CUE]] 对比选择**：需要强类型选 CUE，需要复杂逻辑选 Jsonnet
5. **与 [[24-综合/02-交付与GitOps/helm-gitops|Helm GitOps]] 配合**：Tanka 管理基础设施，Helm 管理应用
6. **参考 [[03-清单模式/09-平台模式/index|平台模式索引]] 了解全局**

## Related

- [[03-清单模式/09-平台模式/02-cue-language-configuration|CUE 语言配置]]
- [[03-清单模式/09-平台模式/01-crossplane-compositions-patterns|Crossplane 组合模式]]
- [[24-综合/02-交付与GitOps/helm-gitops|Helm GitOps 综合]]
- [[10-平台工程/01-构建/01-platform-engineering-overview|平台工程概述]]
- [[03-清单模式/09-平台模式/index|平台模式索引]]
- [[24-综合/02-交付与GitOps/argocd-gitops|ArgoCD GitOps 综合]]
