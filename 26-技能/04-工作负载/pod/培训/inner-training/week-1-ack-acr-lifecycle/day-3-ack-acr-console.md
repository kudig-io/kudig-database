---
title: 'Day 3: ACK/ACR 控制台 & 功能'
description: '## 概述'
summary: '虽然 kubectl 和 API 是运维自动化的主要工具，但控制台在日常查看、紧急操作和新人上手方面仍然不可替代。今天你将系统性地巡览 ACK 和 ACR 控制台的所有功能模块，理解每个界面背后对应的 K8s 资源和 API 操作，并建立控制台操作与 kubectl 命令的对照关系。'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- helm
- docker
- statefulset
- daemonset
- job
- cronjob
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 3: ACK/ACR 控制台 & 功能 是什么'
- '如何 Day 3: ACK/ACR 控制台 & 功能'
trigger_keywords:
- Day
- '3:'
- ACK
- ACR
- 控制台
- 功能
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 3: ACK/ACR 控制台 & 功能
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - ACK console function modules overview
  - ACR console image management
  - ACK console kubectl command mapping
  - ACK cluster console operations guide
  - [[23-实体/kubernetes.md|[[kubernetes|kubernetes]]]] console operations tutorial
trigger_keywords:
  - console
  - 控制台
  - kubectl
  - operations
  - cluster management
  - node management
  - workload
  - network
  - storage
  - configuration
reading_level: beginner
audience:
  - All learners
  - Beginners
  - Operations personnel
estimated_read_time: 45min
related_domains:
  - 云厂商
  - 集群基础
related_topics:
  - ack-overview
  - ack-practical-guide
  - kubectl-commands-reference
---

# Day 3: ACK/ACR 控制台 & 功能

> **学习时间**: 4-5 小时 | **主题**: 熟悉 ACK/ACR 控制台界面与核心功能操作

---

## 概述

虽然 kubectl 和 API 是运维自动化的主要工具，但控制台在日常查看、紧急操作和新人上手方面仍然不可替代。今天你将系统性地巡览 ACK 和 ACR 控制台的所有功能模块，理解每个界面背后对应的 K8s 资源和 API 操作，并建立控制台操作与 kubectl 命令的对照关系。

---

## 今日目标

- [ ] 熟悉 ACK 控制台主要功能模块
- [ ] 熟悉 ACR 控制台镜像管理功能
- [ ] 能通过控制台完成常见运维操作
- [ ] 了解控制台与 API/kubectl 的操作对应关系

---

## 核心概念

### 1. 控制台、kubectl、API 三种操作方式对比

| 维度 | 控制台 | kubectl | API/SDK |
|------|--------|---------|---------|
| 学习曲线 | 低 | 中 | 高 |
| 操作效率 | 查看快，批量慢 | 批量快 | 自动化 |
| 适用场景 | 日常查看、紧急操作 | 日常运维 | 系统集成 |
| 审计记录 | 有操作日志 | 有审计日志 | 有审计日志 |
| 权限控制 | RAM + RBAC | RBAC | RAM + RBAC |

### 2. ACK 控制台功能模块与 K8s 资源映射

| 控制台模块 | 对应 K8s 资源 | 核心功能 |
|-----------|-------------|---------|
| 集群列表 | Cluster | 查看、创建、删除、升级 |
| 集群概览 | 多种资源 | 节点/Pod/事件汇总 |
| 节点管理 | Node / NodePool | 排水、移除、标签、污点 |
| 工作负载 | Deployment/StatefulSet/DaemonSet/Job | 创建、更新、回滚、扩缩容 |
| 服务与路由 | [[service\|Service]] / Ingress | 创建、配置、暴露 |
| 配置管理 | ConfigMap / Secret | 创建、编辑、删除 |
| 存储 | PV / PVC / StorageClass | 创建、绑定、扩容 |
| 安全 | RBAC / NetworkPolicy / PSS | 权限配置、网络策略 |
| 运维管理 | 组件/监控/日志 | 组件管理、监控面板、日志查询 |

---

## 理论学习 (1.5h)

### 必读文档

1. **ACK 实操指南**
   - 文件: `../../../云厂商/04-alicloud-ack/service-ack-practical-guide.md`
   - 重点: 控制台核心操作流程

2. **K8S 架构与组件**
   - 文件: `../../../集群基础/02-core-components-deep-dive.md`
   - 重点: 理解控制台背后对应的 K8S 资源对象

---

## 实战演练 (3h)

### 任务 1: ACK 控制台功能巡览 (1h)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
登录阿里云控制台 → 容器服务 ACK

==============================
模块 1: 集群列表页面
==============================
功能:
- 查看所有集群的状态、版本、节点数、地域
- 搜索和过滤集群
- 快捷操作: 连接集群、升级、删除

关注字段:
┌──────────┬──────────┬──────────┬──────────┬──────────┐
│ 集群名称  │ 状态     │ 版本     │ 节点数    │ 地域     │
├──────────┼──────────┼──────────┼──────────┼──────────┤
│ prod-01  │ Running  │ 1.28.9   │ 5        │ 杭州     │
│ staging  │ Running  │ 1.28.9   │ 3        │ 上海     │
└──────────┴──────────┴──────────┴──────────┴──────────┘

点击集群名称进入详情 →

==============================
模块 2: 集群详情 → 概览
==============================
信息:
- 集群基本信息 (ID、版本、状态、创建时间、类型)
- 资源概览 (节点数、Pod 数、CPU/内存使用率)
- API Server 连接地址
- 事件列表 (最近事件)
- 组件状态

==============================
模块 3: 节点管理
==============================
功能:
- 节点列表: 状态、IP、规格、标签、污点
- 节点池: 池配置、伸缩状态、实例规格
- 节点操作:
  - 排水 (Cordon + Drain)
  - 移除节点
  - 标签管理
  - 污点管理

kubectl 对照:
  控制台"查看节点列表" → kubectl get nodes -o wide
  控制台"节点排水"     → kubectl cordon <node> && kubectl drain <node>
  控制台"添加标签"     → kubectl label node <node> key=value

==============================
模块 4: 工作负载
==============================
功能:
- 无状态 (Deployment): 创建、更新、回滚、扩缩容
- 有状态 (StatefulSet): 创建、管理
- 守护进程 (DaemonSet): 创建、管理
- 任务 (Job/CronJob): 创建、管理
- Pod 列表: 查看所有 Pod 状态

操作:
- 使用表单创建工作负载 (适合简单场景)
- 使用 YAML 编辑器创建工作负载 (适合复杂场景)
- 查看 Pod 日志
- 进入 Pod 终端

kubectl 对照:
  控制台"创建 Deployment" → kubectl apply -f deployment.yaml
  控制台"扩缩容"         → kubectl scale deployment <name> --replicas=3
  控制台"查看日志"       → kubectl logs <pod> -n <ns>
  控制台"Pod 终端"       → kubectl exec -it <pod> -n <ns> -- /bin/sh

==============================
模块 5: 服务与路由
==============================
功能:
- Service 列表: ClusterIP / NodePort / LoadBalancer
- Ingress 列表: 域名路由规则
- 创建 Service / Ingress

kubectl 对照:
  控制台"查看 Service"  → kubectl get svc -A
  控制台"查看 Ingress" → kubectl get ingress -A

==============================
模块 6: 配置管理
==============================
功能:
- ConfigMap: 配置文件管理
- Secret: 敏感信息管理

kubectl 对照:
  控制台"查看 ConfigMap" → kubectl get cm -A
  控制台"查看 Secret"   → kubectl get secrets -A
```
---

### 任务 2: ACR 控制台功能巡览 (45min)

```
# 🟢 低风险：只读/信息收集，通常无副作用
登录阿里云控制台 → 容器镜像服务 ACR

==============================
ACR 个人版
==============================
功能:
- 命名空间管理: 创建和管理仓库命名空间
- 仓库列表: 查看所有镜像仓库
- 镜像版本: 查看每个仓库的镜像标签
- 基础操作: 推送/拉取镜像

示例操作:
# 登录 ACR
docker login --username=xxx registry.cn-hangzhou.aliyuncs.com

# 推送镜像
docker tag nginx:alpine registry.cn-hangzhou.aliyuncs.com/my-ns/nginx:alpine
docker push registry.cn-hangzhou.aliyuncs.com/my-ns/nginx:alpine

# 拉取镜像
docker pull registry.cn-hangzhou.aliyuncs.com/my-ns/nginx:alpine

==============================
ACR 企业版 (如有)
==============================
高级功能:
- 实例管理: 多实例隔离
- 镜像安全扫描:
  - 自动检测 CVE 漏洞
  - 漏洞等级分类 (Critical/High/Medium/Low)
  - 阻断策略: 阻止高危镜像部署
- 镜像签名:
  - 公钥加密验证镜像完整性
  - 防止被篡改的镜像运行
- 分发规则:
  - 跨地域自动同步
  - 带宽和并发控制
- 访问控制:
  - 命名空间级别权限
  - RAM 集成
- Helm Chart 仓库:
  - Chart 版本管理
  - Chart 推送/拉取

==============================
构建功能
==============================
功能:
- 自动构建规则: 代码提交触发镜像构建
- 构建日志: 查看构建过程和结果
- 代码源绑定: GitHub / GitLab / Codeup
- 多架构构建: amd64 / arm64

示例配置:
源代码仓库: https://github.com/xxx/app
分支: main
Dockerfile 路径: ./Dockerfile
镜像标签: latest + git-sha
触发规则: 推送到 main 分支自动构建
```
---

### 任务 3: 控制台与 kubectl 操作对照 (45min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

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
echo "========== 控制台 ↔ kubectl 对照表 =========="

# --- 节点管理 ---
# 控制台: 节点管理 → 节点列表
kubectl get nodes -o wide

# 控制台: 节点管理 → 节点详情
kubectl describe node <node-name>

# 控制台: 节点排水
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 控制台: 节点标签管理
kubectl label node <node-name> key=value
kubectl label node <node-name> key-

# --- 工作负载 ---
# 控制台: 工作负载 → Pod 列表
kubectl get pods -A

# 控制台: 查看 Pod 日志
kubectl logs <pod-name> -n <namespace>

# 控制台: Pod 终端
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh

# 控制台: YAML 编辑器
kubectl edit <resource> <name> -n <namespace>

# --- 事件 ---
# 控制台: 概览 → 事件列表
kubectl get events --sort-by='.lastTimestamp' -A
kubectl get events --field-selector type=Warning -A

# --- 组件 ---
# 控制台: 运维管理 → 组件管理
kubectl get pods -n kube-system
kubectl get pods -n arms-prom

# --- 配置 ---
# 控制台: 配置管理 → ConfigMap
kubectl get configmaps -A

# 控制台: 配置管理 → Secret
kubectl get secrets -A

# --- 存储 ---
# 控制台: 存储 → 存储声明
kubectl get pvc -A

# 控制台: 存储 → 存储类
kubectl get storageclass

echo "========== 对照完毕 =========="
```
---

### 任务 4: 通过控制台完成一套完整操作 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
完成以下操作流程并记录每个步骤对应的 kubectl 命令:

步骤 1: 在控制台创建一个 Namespace (test-console)
  kubectl create namespace test-console

步骤 2: 使用 YAML 编辑器创建一个 Deployment (nginx, 2 replicas)
  kubectl apply -f - <<EOF
  apiVersion: apps/v1
  kind: Deployment
  metadata:
    name: nginx
    namespace: test-console
  spec:
    replicas: 2
    selector:
      matchLabels:
        app: nginx
    template:
      metadata:
        labels:
          app: nginx
      spec:
        containers:
        - name: nginx
          image: nginx:alpine
          ports:
          - containerPort: 80
  EOF

步骤 3: 为 Deployment 创建一个 ClusterIP Service
  kubectl expose deployment nginx --port=80 --target-port=80 -n test-console

步骤 4: 查看 Pod 状态和日志
  kubectl get pods -n test-console
  kubectl logs <pod-name> -n test-console

步骤 5: 进入 Pod 终端执行 curl localhost
  kubectl exec -it <pod-name> -n test-console -- curl -s localhost

步骤 6: 查看 Namespace 事件
  kubectl get events -n test-console --sort-by='.lastTimestamp'

步骤 7: 清理: 删除 Namespace
  kubectl delete namespace test-console  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK 控制台的主要功能模块有哪些？各自管理什么资源？**
   - 提示: 集群管理、节点管理、工作负载、网络、存储、配置

2. **什么时候用控制台操作，什么时候用 kubectl/API？**
   - 提示: 控制台适合日常查看，kubectl 适合批量操作，API 适合自动化

3. **ACR 企业版的镜像安全扫描功能有什么价值？**
   - 提示: 发现已知 CVE、基础镜像风险、合规要求

---

## 今日检验

- [ ] 能在控制台找到集群详情、节点列表、组件状态
- [ ] 能通过控制台 YAML 编辑器创建工作负载
- [ ] 能在控制台查看 Pod 日志和进入终端
- [ ] 能说出控制台操作对应的 kubectl 命令

---

## 配置参考

### 控制台快捷入口

| 功能 | 控制台路径 |
|------|-----------|
| 集群列表 | 阿里云控制台 → 容器服务 ACK → 集群 |
| 集群概览 | 集群列表 → 点击集群名称 |
| 节点管理 | 集群详情 → 节点管理 → 节点/节点池 |
| 工作负载 | 集群详情 → 工作负载 → 无状态/有状态/守护进程 |
| 服务路由 | 集群详情 → 服务与路由 → Service/Ingress |
| 配置管理 | 集群详情 → 配置管理 → ConfigMap/Secret |
| 存储 | 集群详情 → 存储 → 存储声明/存储类 |
| 组件管理 | 集群详情 → 运维管理 → 组件管理 |
| 监控 | 集群详情 → 运维管理 → Prometheus 监控 |
| ACR | 阿里云控制台 → 容器镜像服务 ACR |

---

## 常见问题

### Q1: 控制台无法查看 Pod 日志怎么办？

可能原因: 1) Pod 已退出且日志轮转; 2) 日志插件未安装; 3) RBAC 权限不足。解决: 使用 `kubectl logs` 命令行查看。

### Q2: YAML 编辑器保存后报错？

检查 YAML 格式是否正确，字段名是否拼写正确。建议先在本地用 `kubectl apply --dry-run=client` 验证后再粘贴。

### Q3: 控制台操作记录在哪里？

ACK 控制台的操作记录在阿里云 ActionTrail (操作审计) 中，可以按时间、操作类型查询。

---

## 要点总结

| 功能模块 | 对应 K8S 资源 | 常用操作 | kubectl 对照 |
|----------|--------------|---------|-------------|
| 集群管理 | Cluster | 查看状态、版本、日志 | aliyun cs GET /clusters |
| 节点管理 | Node / NodePool | 排水、移除、扩缩容 | kubectl cordon/drain |
| 工作负载 | Deployment / StatefulSet | 创建、更新、回滚 | kubectl apply/scale |
| 服务与路由 | Service / Ingress | 创建、配置、暴露 | kubectl expose/create ingress |
| 配置管理 | ConfigMap / Secret | 创建、编辑 | kubectl create cm/secret |
| 存储 | PV / PVC / StorageClass | 创建、绑定 | kubectl get pv/pvc/sc |

---

## 明日预告

Day 4 将学习 K8S 集群创建的完整流程，包括参数配置、网络规划和节点池设置。

---

## 延伸阅读

- [ACK 实操指南](../../云厂商/04-alicloud-ack/service-ack-practical-guide.md)
- [K8s 核心组件](../../../../../../01-%E9%9B%86%E7%BE%A4%E5%9F%BA%E7%A1%80/01-%E6%9E%B6%E6%9E%84%E6%80%BB%E8%A7%88/02-core-components-deep-dive.md)
- [kubectl 命令参考](../../../../../../01-%E9%9B%86%E7%BE%A4%E5%9F%BA%E7%A1%80/05-kubectl/05-kubectl-commands-reference.md)


<!-- risk-assessed -->
