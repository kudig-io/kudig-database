---
title: KUDO
description: '## 概述'
summary: 'KUDO 是一个构建 Kubernetes Operator 的声明式工具包，允许开发者仅使用 YAML 定义复杂的有状态应用生命周期管理逻辑，无需编写 Go 代码。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kudo
- kubelet
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDO 是什么
- 如何 KUDO
trigger_keywords:
- KUDO
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KUDO

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KUDO（Kubernetes Universal Declarative Operator）是一个构建 Kubernetes Operator 的声明式工具包，由 D2iQ（原 Mesosphere）开发，2019 年加入 CNCF 沙箱。它允许开发者仅使用 YAML 定义复杂的有状态应用生命周期管理逻辑，无需编写 Go 代码。KUDO 将 Operator 的常见模式（安装、升级、备份、恢复、扩缩容等）抽象为声明式 Plan，每个 Plan 由有序的 Phase 和 Step 组成，支持参数化配置和模板渲染。这使得运维团队可以为 Kafka、Cassandra、Zookeeper 等复杂中间件快速构建生产级 Operator，而不需要掌握 Go 语言和 controller-runtime 框架。

## 核心能力

- **声明式 Operator**: 纯 YAML 定义 Operator 逻辑，无需编写 Go 代码
- **Plan/Phase/Step**: 结构化的生命周期操作模型，支持串行/并行执行
- **参数化模板**: 使用 Go template 参数化部署配置，支持参数覆盖和验证
- **内置任务**: 提供 Deploy、Pipe、Kubectl 等内置任务类型
- **状态管理**: 自动跟踪 Plan 执行进度和状态，支持恢复
- **Operator 仓库**: KUDO Manager 提供丰富的预制 Operator 目录

## 架构

KUDO 采用通用控制器 + 声明式 Operator 定义模式：

- **KUDO Manager**: 集群中运行的控制器，监听 Operator 和 Instance CRD
- **Operator CRD**: 定义 Operator 元数据（版本、参数 schema、Plan 定义）
- **Instance CRD**: 部署的实例，引用 Operator 并提供参数值
- **Plan**: 生命周期操作（deploy/upgrade/backup/restore），由 Phase 和 Step 组成
- **Phase**: Plan 中的有序阶段，Phase 之间串行执行
- **Step**: Phase 中的执行单元，Step 之间可并行或串行
- **Task**: 实际操作（创建资源、执行脚本等），由 Step 引用

执行模型：`Instance → Operator (Plan/Phase/Step) → Task → K8s 资源`

## K8s 集成

KUDO Manager 以 Deployment 运行在 Kubernetes 集群中，通过监听 Operator 和 Instance CRD 执行生命周期操作。每个 Operator 定义了 deploy、upgrade、backup、restore 等 Plan。当 Instance CRD 被创建时，KUDO Manager 触发 deploy Plan，按 Phase → Step 顺序执行任务（创建 Deployment、ConfigMap 等）。Step 中的资源模板渲染后通过 Kubernetes API 创建。KUDO 与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 CRD 机制完全兼容，Operator 定义以 YAML 存储在 Operator 仓库中。

## 生产场景

1. **中间件 Operator 开发**: 为 Kafka、Cassandra、Zookeeper 等快速构建生产级 Operator
2. **标准化运维流程**: 将备份、恢复、扩容等运维操作标准化为可重复执行的 Plan
3. **多版本管理**: 通过 Operator 版本管理实现平滑升级和回滚
4. **GitOps 集成**: 将 Operator 和 Instance YAML 存储在 Git 中，通过 GitOps 工具管理

## 安装与配置

```bash
# 安装 KUDO CLI
brew install kudo-cli
# 或
curl -s https://kudo.dev/install.sh | bash

# 安装 KUDO Manager
kubectl kudo init

# 安装预制 Operator（以 Kafka 为例）
kubectl kudo install kafka --instance=my-kafka \
  -p BROKER_COUNT=3 \
  -p BROKER_CPUS=2000m \
  -p BROKER_MEM=4096Mi

# 查看安装进度
kubectl kudo plan status --instance=my-kafka

# 执行备份 Plan
kubectl kudo plan trigger backup --instance=my-kafka
```

```yaml
# Operator 定义示例（operator.yaml）
apiVersion: kudo.dev/v1beta1
kind: Operator
metadata:
  name: redis-cluster
spec:
  maintainer:
  - name: Platform Team
  url: https://internal.example.com/redis-operator
  kubernetesVersion: 1.28.0
---
# 参数定义（params.yaml）
apiVersion: kudo.dev/v1beta1
kind: Parameter
metadata:
  name: redis-cluster
spec:
  parameters:
  - name: NODE_COUNT
    default: "6"
    description: "Redis 集群节点数"
  - name: MEMORY
    default: "512Mi"
    description: "每个节点内存"
  - name: PERSISTENCE_ENABLED
    default: "true"
    enum: ["true", "false"]
---
# Plan 定义（plans/deploy.yaml）
apiVersion: kudo.dev/v1beta1
kind: Plan
metadata:
  name: deploy
spec:
  strategy: serial
  phases:
  - name: init-config
    strategy: serial
    steps:
    - name: create-configmap
      tasks: [configmap]
  - name: deploy-nodes
    strategy: parallel
    steps:
    - name: deploy-redis
      tasks: [statefulset]
  - name: cluster-init
    strategy: serial
    steps:
    - name: init-cluster
      tasks: [cluster-init-job]
```

```yaml
# Instance CRD（部署实例）
apiVersion: kudo.dev/v1beta1
kind: Instance
metadata:
  name: my-redis
  namespace: production
  labels:
    operator: redis-cluster
spec:
  operatorVersion:
    name: redis-cluster-1.0.0
    namespace: kudo-system
  parameters:
    NODE_COUNT: "6"
    MEMORY: "1024Mi"
    PERSISTENCE_ENABLED: "true"
```

## 运维操作

```bash
# 🟢 低风险：查看 Operator 和 Instance 状态
kubectl get operators -A
kubectl get instances -A
kubectl kudo plan status --instance=my-redis -n production

# 🟡 中风险：触发 Plan 执行
kubectl kudo plan trigger upgrade --instance=my-redis -n production
kubectl kudo plan trigger backup --instance=my-redis -n production

# 🟡 中风险：更新参数（触发重新部署）
kubectl kudo update --instance=my-redis -n production -p NODE_COUNT=8

# 🟢 低风险：查看 Plan 执行历史
kubectl kudo plan history --instance=my-redis -n production

# 🔴 高风险：删除 Instance（删除所有托管资源）
kubectl kudo uninstall --instance=my-redis -n production

# 🟢 低风险：查看 Operator 仓库
kubectl kudo list --repo https://kudo-repo.example.com
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Plan 卡在 PENDING | 前置 Plan 未完成 | `kubectl kudo plan status --instance=<name>` | 等待前置 Plan 完成或手动触发 |
| Step 执行失败 | 资源模板渲染错误 | `kubectl describe instance <name>` | 检查参数值和模板语法 |
| Instance 未创建 | Operator 版本不存在 | `kubectl get operatorversions -A` | 确认 operatorVersion 引用正确 |
| 升级失败 | 参数不兼容 | `kubectl kudo plan history --instance=<name>` | 回滚到上一版本 |
| Pod 未就绪 | 资源不足/配置错误 | `kubectl describe pod -l instance=<name>` | 调整参数或节点资源 |

```
排查流程：
├── Plan 执行异常？
│   ├── kubectl kudo plan status → 查看当前进度
│   ├── kubectl describe instance → 查看 Events
│   └── 检查 Step 对应的 Pod/Job 状态
├── 参数更新无效？
│   ├── kubectl get instance -o yaml → 确认参数已更新
│   ├── 检查是否触发了正确的 Plan
│   └── kubectl kudo plan trigger deploy → 手动触发
└── 升级/回滚失败？
    ├── kubectl kudo plan history → 查看历史
    ├── 检查 OperatorVersion 兼容性
    └── kubectl kudo update --operator-version=<old> → 回滚
```

## 生产案例

### 案例 1：Kafka 集群标准化运维

- **场景**：运维团队管理 20+ Kafka 集群，升级/扩容流程不统一，常出事故
- **排查**：每次升级需要手动执行 10+ 步骤，不同工程师操作顺序不一致
- **方案**：使用 KUDO Kafka Operator，将升级流程编排为 Plan（滚动重启 Broker → 更新配置 → 验证），一键执行
- **效果**：升级时间从 4h 缩短至 30min，零事故升级

### 案例 2：数据库 Operator 快速开发

- **场景**：需要为内部 PostgreSQL 集群构建 Operator，团队无 Go 开发经验
- **排查**：使用 Kubebuilder 开发 Operator 需要 2 个月，团队不熟悉 Go
- **方案**：使用 KUDO 纯 YAML 定义 Operator，包含 deploy/backup/restore/upgrade Plan，2 周完成
- **效果**：开发时间从 2 月缩短至 2 周，运维团队可自主维护

## 对比

| 特性 | KUDO | Kubebuilder | Operator SDK | OLM |
|------|------|-------------|--------------|-----|
| 编程方式 | YAML 声明式 | Go 代码 | Go/Ansible/Helm | 元数据管理 |
| 学习曲线 | 低 | 高 | 中 | 低 |
| 灵活性 | ⚠️ 有限 | ✅ 最高 | ✅ 高 | ⚠️ 元数据层 |
| 适合场景 | 快速构建 | 复杂逻辑 | 多语言 | Operator 生命周期 |

## 架构定位

在 CNCF 生态中，KUDO 属于 **Orchestration** 类别，为云原生应用提供声明式 Operator 开发能力。

## 参考链接

- [[crossplane]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[openfeature]] — OpenFeature
- tools]] — Podman Desktop
- [[k3s]] — k3s 轻量级 Kubernetes
- [[virtual-kubelet]] — Virtual Kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kudo
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
