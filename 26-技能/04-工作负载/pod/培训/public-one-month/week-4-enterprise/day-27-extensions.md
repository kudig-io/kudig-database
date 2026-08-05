---
title: 'Day 27: 扩展生态 + 高级主题'
description: '- Helm Charts 管理'
summary: '- Helm Charts 管理'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- prometheus
- helm
- argocd
- redis
- mysql
- postgresql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 27: 扩展生态 + 高级主题 是什么'
- '如何 Day 27: 扩展生态 + 高级主题'
trigger_keywords:
- Day
- '27:'
- 扩展生态
- 高级主题
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- gitops-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 27: 扩展生态 + 高级主题
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[kubernetes|Kubernetes]] CRD 开发
  - [[helm|Helm]] Charts 管理
  - Operator 模式
  - K8s 扩展生态
trigger_keywords:
  - CRD
  - Helm
  - Operator
  - Operator SDK
  - Kubebuilder
  - Kustomize
  - Chart
  - 扩展生态
reading_level: intermediate
audience:
  - sre-engineer
  - platform-engineer
  - developer
estimated_read_time: 240min
related_domains:
  - 专项技术
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/day-28-final-project
  - 生产运维/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
---

# Day 27: 扩展生态 + 高级主题

## 概述

今天学习 K8s 的扩展生态，包括三个核心技术：**CRD（Custom Resource Definition）**、**Helm** 和 **Operator 模式**。这三者构成了 K8s 扩展性的基础，让你能够定义自定义资源、打包和分发应用、以及自动化复杂运维操作。

K8s 的核心设计理念之一就是可扩展性。K8s 内置了 Pod、Service、Deployment 等标准资源类型，但在实际使用中，你可能需要定义自己的资源类型（如 Database、Certificate、BackupPlan）并实现相应的自动化逻辑。CRD 提供了定义自定义资源的能力，Helm 提供了应用打包和版本管理的能力，Operator 提供了自动化运维的能力。

### 学习目标

- 理解 CRD 的概念和作用，能够创建和使用自定义资源
- 掌握 Helm 的核心操作：安装、升级、回滚、卸载应用
- 能够创建和管理自定义 Helm Chart
- 理解 Operator 模式的工作原理和使用场景

---

## 核心概念详解

### CRD（Custom Resource Definition）

CRD 是 K8s 扩展 API 的机制。通过 CRD，你可以定义新的资源类型，这些资源类型可以像原生资源（Pod、Service 等）一样通过 kubectl 管理。

CRD 的核心概念：

- **CRD（Custom Resource Definition）**: 定义了新资源类型的"schema"——它叫什么名字、有哪些字段、字段类型是什么、哪些字段是必填的。CRD 本身是一个集群级别的资源
- **CR（Custom Resource）**: CRD 定义的具体实例。就像 Pod 是 "Pod" 这个资源类型的实例一样，CR 是你定义的资源类型的实例

CRD 的价值：

- **声明式 API**: 自定义资源与原生资源使用相同的声明式管理方式（kubectl apply/get/delete）
- **与 K8s 生态集成**: 自定义资源可以使用 K8s 的 RBAC、准入控制、审计日志等功能
- **Controller 配合**: CRD 定义了"期望状态"，Controller 负责将实际状态向期望状态收敛

CRD 使用 OpenAPI v3 Schema 来定义字段的类型和验证规则。例如，你可以定义一个 `Database` CRD，其中包含 `engine`（引擎类型）、`version`（版本号）、`storage`（存储大小）等字段。创建一个 `Database` CR 后，相应的 Operator Controller 会自动创建实际的数据库实例。

### Helm 包管理

Helm 是 K8s 的"包管理器"，类似于 Ubuntu 的 apt 或 macOS的 Homebrew。它将一组 K8s 资源定义（YAML 文件）打包为一个 **Chart**，支持版本管理、配置参数化和一键部署。

Helm 的核心概念：

- **Chart**: 一个 Helm 包，包含了一组 K8s 资源模板和默认配置。Chart 的目录结构包含 `Chart.yaml`（元信息）、`values.yaml`（默认配置）、`templates/`（资源模板）
- **Release**: Chart 的一次安装实例。同一个 Chart 可以在集群中安装多次（不同的 Release Name），每次安装会生成不同的资源
- **Repository**: Chart 的存储和分发服务。可以是公共仓库（如 Bitnami、Artifact Hub）或私有仓库（如 ChartMuseum、ACR）
- **Values**: Chart 的配置参数。安装时可以通过 `-f values.yaml` 或 `--set key=value` 覆盖默认值

Helm 的核心操作流程：

1. **搜索**: `helm search repo <keyword>` 在已添加的仓库中搜索 Chart
2. **查看**: `helm show values <chart>` 查看 Chart 的可配置参数
3. **安装**: `helm install <release> <chart>` 安装 Chart，创建 Release
4. **升级**: `helm upgrade <release> <chart>` 更新 Release 的配置或版本
5. **回滚**: `helm rollback <release> <revision>` 回滚到之前的版本
6. **卸载**: `helm uninstall <release>` 删除 Release 及其所有资源
7. **查看历史**: `helm history <release>` 查看 Release 的版本历史

**Helm vs Kustomize**:

Helm 和 Kustomize 都是 K8s 应用管理的工具，但设计理念不同：

- **Helm**: 使用 Go Template 生成最终的 YAML。适合需要参数化的通用 Chart（如数据库中间件的部署模板）
- **Kustomize**: 使用 Overlay 机制在基础 YAML 上叠加变更。适合管理同一应用在不同环境（dev/staging/prod）的差异化配置
- **选择建议**: 如果你需要分发给他人使用，选 Helm（Chart 生态丰富）；如果是内部应用的配置管理，选 Kustomize（学习成本低）。ArgoCD 同时支持两者

### Operator 模式

Operator 是将人类运维知识编码为软件的模式。一个 Operator 由两部分组成：

- **CRD**: 定义了运维对象的数据模型（如 Backup、Restore、ScaleUp）
- **Controller**: 持续监听 CR 的变化，执行相应的运维操作

Operator 的核心工作流程（Reconcile Loop）：

1. **Watch**: Controller 通过 Informer 监听 CR 和相关资源的变化事件
2. **Filter**: 过滤出需要处理的事件（如新建、更新、删除）
3. **Reconcile**: 对比期望状态（CR 中定义的）和实际状态（集群中的），执行操作使两者一致
4. **Update Status**: 更新 CR 的 Status 字段，反映当前的实际状态

Operator 的典型应用场景：

- **数据库运维**: 自动创建数据库实例、执行备份和恢复、管理主从切换（如 MySQL Operator、PostgreSQL Operator）
- **中间件运维**: 自动部署和配置 Kafka、Redis、Elasticsearch 等中间件集群
- **证书管理**: 自动签发和轮转 TLS 证书（如 [[cert-manager|cert-manager]]）
- **GitOps**: 自动同步 Git 仓库中的配置到集群（如 ArgoCD）
- **监控配置**: 自动管理 Prometheus 的采集目标和告警规则（如 Prometheus Operator）

**开发 Operator 的方式**:

- **Kubebuilder**: Go 语言的 Operator SDK，由 K8s 官方维护。功能强大，适合复杂的 Operator
- **Operator SDK（Ansible/Helm）**: 使用 Ansible Playbook 或 Helm Chart 作为 Reconcile 逻辑。适合简单的 Operator，不需要编写 Go 代码
- ** Kopf（Python）**: Python 语言的 Operator 框架。适合 Python 团队

---

## 实战演练

### 任务 1: Helm 实践 (1h)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `helm upgrade/install`：部署/升级 release

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 添加常用 Helm repo
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 搜索 Chart
helm search repo nginx
helm search repo nginx --versions

# 查看 Chart 详细信息
helm show chart bitnami/nginx
helm show values bitnami/nginx

# 导出默认 values 以供修改
helm show values bitnami/nginx > nginx-default-values.yaml

# 使用自定义 values 安装
cat > nginx-values.yaml << 'EOF'
replicaCount: 3
service:
  type: ClusterIP
  port: 80
resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 200m
    memory: 256Mi
autoscaling:
  enabled: true
  minReplicas: 2
  maxReplicas: 5
  targetCPUUtilizationPercentage: 80
EOF

helm install my-nginx bitnami/nginx -f nginx-values.yaml --namespace demo --create-namespace

# 查看安装状态
helm list
helm status my-nginx
helm get values my-nginx

# 升级（修改配置）
helm upgrade my-nginx bitnami/nginx -f nginx-values.yaml --set replicaCount=5 --namespace demo

# 查看版本历史
helm history my-nginx

# 回滚到上一个版本
helm rollback my-nginx 1 --namespace demo

# 卸载
helm uninstall my-nginx --namespace demo  # ⚠️ 删除 release 及关联资源
```
### 任务 2: 创建自定义 Helm Chart (1h)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `helm uninstall`：删除 release 及其释放的所有资源
> - `helm upgrade/install`：部署/升级 release

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 创建 Chart 骨架
helm create my-app
tree my-app

# 修改 Chart.yaml
cat > my-app/Chart.yaml << 'EOF'
apiVersion: v2
name: my-app
description: A custom Helm chart for deploying a web application
type: application
version: 0.1.0
appVersion: "1.0.0"
maintainers:
  - name: devops-team
EOF

# 修改 values.yaml
cat > my-app/values.yaml << 'EOF'
replicaCount: 2
image:
  repository: nginx
  pullPolicy: IfNotPresent
  tag: alpine
service:
  type: ClusterIP
  port: 80
ingress:
  enabled: false
resources:
  requests:
    cpu: 50m
    memory: 64Mi
  limits:
    cpu: 100m
    memory: 128Mi
livenessProbe:
  httpGet:
    path: /
    port: http
readinessProbe:
  httpGet:
    path: /
    port: http
EOF

# 验证 Chart 语法
helm lint my-app

# 渲染模板（查看生成的 YAML）
helm template my-app my-app/

# 打包 Chart
helm package my-app
ls -la my-app-*.tgz

# 本地安装测试
helm install test-release my-app/ --namespace demo --create-namespace
kubectl get all -n demo

# 调试: 查看渲染后的 YAML
helm get manifest test-release --namespace demo

# 清理
helm uninstall test-release --namespace demo  # ⚠️ 删除 release 及关联资源
```
### 任务 3: CRD 和 Operator 实践 (1h)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看集群中已有的 CRD
kubectl get crd
kubectl get crd -o wide

# 查看 Prometheus Operator 创建的 CRD
kubectl get crd | grep monitoring.coreos.com

# 查看自定义资源实例
kubectl get prometheus -A
kubectl get servicemonitor -A
kubectl get prometheusrule -A

# 创建一个简单的 CRD
cat > crd-webapp.yaml << 'EOF'
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: webapps.example.com
spec:
  group: example.com
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              image:
                type: string
              replicas:
                type: integer
                minimum: 1
                maximum: 10
  scope: Namespaced
  names:
    plural: webapps
    singular: webapp
    kind: WebApp
    shortNames:
    - wa
EOF

kubectl apply -f crd-webapp.yaml

# 查看新创建的 CRD
kubectl get crd webapps.example.com
kubectl explain webapp

# 创建 CR 实例
cat > webapp-sample.yaml << 'EOF'
apiVersion: example.com/v1
kind: WebApp
metadata:
  name: my-webapp
  namespace: default
spec:
  image: nginx:alpine
  replicas: 3
EOF

kubectl apply -f webapp-sample.yaml

# 查看 CR
kubectl get webapp
kubectl get webapp my-webapp -o yaml

# 清理
kubectl delete webapp my-webapp
kubectl delete crd webapps.example.com
```
---

## 常见问题

### Q1: Helm Chart 和 Kustomize 如何选择？

Helm 适合需要参数化的通用 Chart（如数据库、中间件的部署模板），有丰富的社区生态。Kustomize 适合管理同一应用在不同环境的差异化配置，无需模板语法。两者可以组合使用：Helm 生成基础 YAML + Kustomize 叠加环境特定配置。

### Q2: Operator 模式的开发成本高吗？

取决于复杂度。使用 Operator SDK（Ansible/Helm 模式）可以在几小时内创建一个简单的 Operator。使用 Kubebuilder（Go 模式）开发复杂的 Operator 可能需要几周。建议先从 Helm/Ansible 模式入门，有需要时再考虑 Go 模式。

### Q3: CRD 的数据存储在哪里？

CRD 的数据存储在 etcd 中，与原生资源使用相同的存储机制。因此，CRD 的数据也受 etcd 的性能和容量限制。建议避免在 CRD 中存储大量数据（单个 CR 不超过 1MB）。

### Q4: 如何找到可用的 Helm Chart？

Artifact Hub（artifacthub.io）是 CNCF 官方的 Helm Chart 搜索引擎。Bitnami 提供了大量高质量的 Chart。对于阿里云用户，ACR 也支持 Helm Chart 仓库。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| CRD | 定义自定义资源类型的 Schema，与原生资源使用相同的管理方式 |
| Helm | K8s 包管理器，Chart + Values + Release 实现应用版本化部署 |
| Operator | CRD + Controller，将运维知识编码为自动化软件 |
| Reconcile Loop | Watch → Filter → Reconcile → Update Status |

---

## 延伸阅读

- [CRD 开发指南](../../../../../../16-%E4%B8%93%E9%A1%B9%E6%8A%80%E6%9C%AF/03-%E6%89%A9%E5%B1%95%E6%9C%BA%E5%88%B6/01-crd-development-guide.md)
- [Helm Charts 管理](../../../../../../16-%E4%B8%93%E9%A1%B9%E6%8A%80%E6%9C%AF/03-%E6%89%A9%E5%B1%95%E6%9C%BA%E5%88%B6/06-helm-charts-management.md)
- [CRD/Operator 开发](../../../../../../10-平台工程/01-构建/10-crd-operator-development.md)
- [控制器模式](../../../../../../01-集群基础/02-设计原则/04-controller-pattern.md)


<!-- risk-assessed -->
