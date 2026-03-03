# Day 3: ACK/ACR 控制台 & 功能

> **学习时间**: 4-5 小时 | **主题**: 熟悉 ACK/ACR 控制台界面与核心功能操作

---

## 今日目标

- [ ] 熟悉 ACK 控制台主要功能模块
- [ ] 熟悉 ACR 控制台镜像管理功能
- [ ] 能通过控制台完成常见运维操作
- [ ] 了解控制台与 API/kubectl 的操作对应关系

---

## 理论学习 (1.5h)

### 必读文档

1. **ACK 实操指南**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/service-ack-practical-guide.md`
   - 重点: 控制台核心操作流程

2. **K8S 架构与组件**
   - 文件: `../../../domain-1-architecture-fundamentals/02-core-components-deep-dive.md`
   - 重点: 理解控制台背后对应的 K8S 资源对象

### 阅读要点

- ACK 控制台功能模块: 集群列表、节点管理、工作负载、服务与路由、配置管理、存储
- ACR 控制台功能模块: 仓库管理、镜像版本、安全扫描、构建规则
- 控制台操作本质上是调用 ACK OpenAPI 或 kubectl

---

## 实践任务 (3h)

### 任务 1: ACK 控制台功能巡览 (1h)

```
# 登录阿里云控制台 -> 容器服务 ACK

# 1. 集群列表页面
#    - 查看集群状态、版本、节点数
#    - 点击集群名称进入详情
#    - 注意: 集群 ID、地域、VPC 信息

# 2. 集群详情 -> 概览
#    - 集群基本信息 (版本、状态、创建时间)
#    - 资源概览 (节点数、Pod 数、负载)
#    - 事件列表

# 3. 节点管理
#    - 节点列表: 状态、IP、规格、标签
#    - 节点池: 池配置、伸缩状态
#    - 节点操作: 排水、移除、标签管理

# 4. 工作负载
#    - Deployments / StatefulSets / DaemonSets / Jobs
#    - 创建工作负载 (YAML 编辑器 vs 表单)
#    - 查看 Pod 日志、终端

# 5. 服务与路由
#    - Service 列表
#    - Ingress 列表
#    - 创建 Service / Ingress

# 6. 配置管理
#    - ConfigMap
#    - Secret
```

### 任务 2: ACR 控制台功能巡览 (45min)

```
# 登录阿里云控制台 -> 容器镜像服务 ACR

# 1. 个人版实例
#    - 命名空间管理
#    - 仓库列表
#    - 镜像版本与标签

# 2. 企业版实例 (如有)
#    - 实例管理
#    - 镜像安全扫描结果
#    - 分发规则 (多地域同步)
#    - 访问控制策略

# 3. 构建功能
#    - 自动构建规则
#    - 构建日志查看
#    - 代码源绑定 (GitHub/GitLab/Codeup)
```

### 任务 3: 控制台与 kubectl 操作对照 (45min)

```bash
# 控制台"查看节点列表" 等价于:
kubectl get nodes -o wide

# 控制台"查看 Pod 列表" 等价于:
kubectl get pods -A

# 控制台"查看 Pod 日志" 等价于:
kubectl logs <pod-name> -n <namespace>

# 控制台"Pod 终端" 等价于:
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh

# 控制台"查看事件" 等价于:
kubectl get events --sort-by='.lastTimestamp' -A

# 控制台"YAML 编辑" 等价于:
kubectl edit <resource> <name> -n <namespace>

# 控制台"查看集群组件" 等价于:
kubectl get pods -n kube-system
kubectl get pods -n arms-prom
```

### 任务 4: 通过控制台完成一套完整操作 (30min)

```
# 完成以下操作流程:
# 1. 在控制台创建一个 Namespace (test-console)
# 2. 使用 YAML 编辑器创建一个 Deployment (nginx, 2 replicas)
# 3. 为 Deployment 创建一个 ClusterIP Service
# 4. 查看 Pod 状态和日志
# 5. 进入 Pod 终端执行 curl localhost
# 6. 查看 Namespace 事件
# 7. 清理: 删除 Namespace
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

## 核心概念总结

| 功能模块 | 对应 K8S 资源 | 常用操作 |
|----------|--------------|---------|
| 集群管理 | Cluster | 查看状态、版本、日志 |
| 节点管理 | Node / NodePool | 排水、移除、扩缩容 |
| 工作负载 | Deployment / StatefulSet | 创建、更新、回滚 |
| 服务与路由 | Service / Ingress | 创建、配置、暴露 |
| 配置管理 | ConfigMap / Secret | 创建、编辑 |
| 存储 | PV / PVC / StorageClass | 创建、绑定 |

---

## 明日预告

Day 4 将学习 K8S 集群创建的完整流程，包括参数配置、网络规划和节点池设置。
