---
title: 'Week 3 自测: 节点与工作负载管理'
description: 'title: Week 3 自测: 节点与工作负载管理'
summary: 'title: Week 3 自测: 节点与工作负载管理'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- flannel
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 3 自测: 节点与工作负载管理 是什么'
- '如何 Week 3 自测: 节点与工作负载管理'
trigger_keywords:
- Week
- '自测:'
- 节点与工作负载管理
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 3 自测: 节点与工作负载管理
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|Kubernetes]] week 3 self-test assessment
  - Node and workload knowledge test
  - Node management troubleshooting quiz
  - Pod scheduling self-check
  - Cluster autoscaler troubleshooting
trigger_keywords:
  - checkpoint
  - self-test
  - quiz
  - week 3
  - 节点
  - 工作负载
  - 自测
  - 评估
reading_level: intermediate
audience:
  - Week 3 learners
  - ACK beginners
estimated_read_time: 30min
related_domains:
  - domain-3-node
  - domain-9-workload
  - domain-10-troubleshooting-diagnostics
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
  - component-ops
---

# Week 3 自测: 节点与工作负载管理

> **满分**: 50 分 | **建议用时**: 60 分钟

---

## 概述

Week 3 的学习聚焦于 Kubernetes 集群中最核心的运维对象——节点（Node）和工作负载（Workload）。节点是运行 Pod 的物理机或虚拟机，理解节点的状态管理、调度约束和维护操作是保障集群稳定运行的基础。工作负载是运行业务应用的载体，掌握 Pod 的生命周期管理、资源配额和故障排查是日常运维的核心能力。

本自测旨在检验你对 Week 3 所学内容的掌握程度，包含概念理解、命令实操和场景分析三个维度。请独立完成所有题目，不要查阅参考资料。完成后对照参考要点进行自我评估，记录薄弱环节并制定补强计划。

**自测目标**：
- 检验节点管理、调度约束、工作负载管理的概念理解
- 验证 kubectl 命令实操能力
- 评估场景分析和故障排查的综合能力

---

## 一、概念理解 (5 题, 每题 2 分, 共 10 分)

### 1. ACK 节点的三种状态分别是什么？节点处于 `NotReady` 状态时，调度器会如何处理？

> 你的回答:

**参考答案**:

节点有三种主要状态：

| 状态 | 含义 | 调度器行为 |
|------|------|-----------|
| **Ready** | 节点健康，可以接收新 Pod | 正常调度 |
| **NotReady** | 节点异常，[[kubelet|kubelet]] 心跳丢失 | 停止调度新 Pod |
| **Unknown** | 节点状态未知（通常因网络问题） | 停止调度新 Pod |

当节点处于 NotReady 状态超过 `pod-eviction-timeout`（默认 5 分钟）后，控制器会开始驱逐该节点上的 Pod。对于 Deployment 管理的 Pod，会在其他节点重新创建。

---

### 2. 解释 Taint 和 Toleration 的协作关系。一个节点设置了 `gpu=true:NoSchedule`，什么样的 Pod 可以调度上去？

> 你的回答:

**参考答案**:

Taint（污点）和 Toleration（容忍）是 Kubernetes 中实现节点专用的机制：

- **Taint** 附加在节点上，表示"排斥不能容忍此污点的 Pod"
- **Toleration** 附加在 Pod 上，表示"可以容忍特定的污点"

三种 Taint Effect：

| Effect | 行为 | 说明 |
|--------|------|------|
| **NoSchedule** | 不调度 | 新 Pod 不会被调度到该节点 |
| **PreferNoSchedule** | 尽量不调度 | 调度器尽量避开，但不保证 |
| **NoExecute** | 不调度 + 驱逐 | 新 Pod 不调度，已有的 Pod 被驱逐 |

当节点设置了 `gpu=true:NoSchedule` 时：
- **可以调度**: 带有 `tolerations: [{"key":"gpu","operator":"Equal","value":"true","effect":"NoSchedule"}]` 的 Pod
- **可以调度**: 带有 `tolerations: [{"operator":"Exists"}]` 的 Pod（容忍所有污点）
- **不可调度**: 没有对应容忍配置的普通 Pod

---

### 3. 托管节点池和自管理节点池有什么区别？各适用于什么场景？

> 你的回答:

**参考答案**:

| 特性 | 托管节点池 | 自管理节点池 |
|------|-----------|-------------|
| **节点配置** | 阿里云统一管理 kubelet 等组件 | 用户自行管理所有组件 |
| **自动修复** | 支持（节点异常自动替换） | 不支持（需手动处理） |
| **自动升级** | 支持（跟随集群版本自动升级） | 不支持（需手动升级） |
| **自定义程度** | 有限（支持部分自定义脚本） | 完全自定义 |
| **运维负担** | 低 | 高 |
| **适用场景** | 大多数生产环境 | 需要深度定制的场景（特殊内核、GPU 驱动等） |

---

### 4. Pod 的 livenessProbe 和 readinessProbe 失败后的行为有什么不同？

> 你的回答:

**参考答案**:

| 探针类型 | 失败行为 | 用途 | 示例场景 |
|----------|---------|------|---------|
| **livenessProbe** | 重启容器 | 检测应用是否存活 | 应用死锁、进程崩溃 |
| **readinessProbe** | 从 Service Endpoints 移除 | 检测应用是否就绪 | 应用启动加载、依赖等待 |

关键区别：
- livenessProbe 失败 → **容器重启**（Pod 重启计数增加）
- readinessProbe 失败 → **流量停止转发**（但容器继续运行）
- 两者独立工作，可以同时配置不同的检测条件

---

### 5. ACK 托管版集群中，哪些组件由阿里云维护，哪些需要用户自行运维？

> 你的回答:

**参考答案**:

| 维护方 | 组件 | 说明 |
|--------|------|------|
| **阿里云维护** | etcd | 分布式键值存储 |
| **阿里云维护** | kube-apiserver | API 网关 |
| **阿里云维护** | kube-scheduler | 调度器 |
| **阿里云维护** | kube-controller-manager | 控制器管理器 |
| **阿里云维护** | cloud-controller-manager | 云控制器 |
| **用户维护** | kubelet | 节点代理（Worker 节点上） |
| **用户维护** | kube-proxy | 网络代理 |
| **用户维护** | 容器运行时 | containerd / Docker |
| **用户维护** | CNI 插件 | Terway / Flannel |
| **用户维护** | CSI 插件 | 存储插件 |

---

## 二、命令实操 (5 题, 每题 2 分, 共 10 分)

### 1. 写出将节点 `node-1` 标记为不可调度并驱逐 Pod 的命令序列:

**参考答案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
# Step 1: 标记节点不可调度
kubectl cordon node-1

# 预期输出:
# node/node-1 cordoned

# Step 2: 验证节点状态
kubectl get nodes

# 预期输出:
# NAME      STATUS                     ROLES    AGE   VERSION
# node-1    Ready,SchedulingDisabled   worker   30d   v1.30.1
# node-2    Ready                      worker   30d   v1.30.1

# Step 3: 驱逐节点上的 Pod
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --timeout=300s

# 预期输出:
# node/node-1 already cordoned
# WARNING: ignoring DaemonSet-managed Pods...
# evicting pod default/my-app-7d9f8b6c4-abc12
# evicting pod default/my-app-7d9f8b6c4-def34
# evicting pod monitoring/prometheus-0
# pod/prometheus-0 evicted
# pod/my-app-7d9f8b6c4-abc12 evicted
# pod/my-app-7d9f8b6c4-def34 evicted
# node/node-1 drained

# Step 4: 确认 Pod 已迁移
kubectl get pods -A -o wide | grep node-1
# 应该只剩 DaemonSet 管理的 Pod

# Step 5: 维护完成后恢复
kubectl uncordon node-1

# 预期输出:
# node/node-1 uncordoned
```
---

### 2. 写出查看节点池列表和指定节点池详情的 aliyun CLI 命令:

**参考答案**:

```bash
# 查看集群的节点池列表
aliyun cs GET /clusters/<cluster_id>/nodepools

# 预期输出:
# {
#   "nodepools": [
#     {
#       "nodepool_id": "np-xxx",
#       "name": "default-nodepool",
#       "type": "ess",
#       "status": "active",
#       "node_count": 3,
#       "auto_scaling": {
#         "enabled": true,
#         "min_instances": 2,
#         "max_instances": 10
#       }
#     }
#   ]
# }

# 查看指定节点池详情
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>

# 预期输出包含:
# - 节点规格 (instance_type)
# - 节点数量期望值 (desired_size)
# - 自动伸缩配置
# - 节点标签和污点
# - 运行时配置
```

---

### 3. 写出创建带有 nodeSelector 调度约束的 Pod YAML (调度到 `env=production` 的节点):

**参考答案**:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: production-app
  namespace: default
  labels:
    app: production-app
    environment: production
spec:
  nodeSelector:
    env: production
  containers:
  - name: app
    image: nginx:1.25-alpine
    ports:
    - containerPort: 80
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      limits:
        cpu: "1"
        memory: 512Mi
    livenessProbe:
      httpGet:
        path: /healthz
        port: 80
      initialDelaySeconds: 10
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /ready
        port: 80
      initialDelaySeconds: 5
      periodSeconds: 5
  restartPolicy: Always
```

---

### 4. 写出检查 CoreDNS 运行状态并测试 DNS 解析的命令:

**参考答案**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 查看 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 预期输出:
# NAME                       READY   STATUS    RESTARTS   AGE   IP           NODE
# coredns-66f5b8f7f5-abc12   1/1     Running   0          30d   10.0.0.100   node-1
# coredns-66f5b8f7f5-def34   1/1     Running   0          30d   10.0.0.101   node-2

# Step 2: 查看 CoreDNS Service
kubectl get svc -n kube-system kube-dns

# 预期输出:
# NAME       TYPE        CLUSTER-IP   EXTERNAL-IP   PORT(S)                  AGE
# kube-dns   ClusterIP   10.96.0.10   <none>        53/UDP,53/TCP,9153/TCP   30d

# Step 3: 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# Step 4: 测试 DNS 解析
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local

# 预期输出:
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      kubernetes.default.svc.cluster.local
# Address 1: 10.96.0.1 kubernetes.default.svc.cluster.local

# Step 5: 测试外部 DNS 解析
kubectl run dns-test2 --image=busybox:1.36 --rm -it --restart=Never -- nslookup www.alibaba.com

# Step 6: 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50
```
---

### 5. 写出为 Pod 配置 resources.requests 和 resources.limits 的 YAML 片段:

**参考答案**:

```yaml
spec:
  containers:
  - name: app
    image: my-app:v1.0
    resources:
      requests:
        cpu: 250m
        memory: 256Mi
        ephemeral-storage: 1Gi
      limits:
        cpu: "1"
        memory: 512Mi
        ephemeral-storage: 2Gi
```

**资源单位说明**：

| 资源 | requests 含义 | limits 含义 | 单位 |
|------|--------------|-------------|------|
| CPU | 调度保证（最少可用的 CPU） | 最大可用 CPU | `m` (millicores) 或核数 |
| Memory | 调度保证（最少可用的内存） | 最大可用内存 | `Mi`, `Gi` |
| ephemeral-storage | 调度保证（最少可用临时存储） | 最大可用临时存储 | `Mi`, `Gi` |

---

## 三、场景分析 (4 题, 每题 5 分, 共 20 分)

### 场景 1: 节点池扩容失败

**现象**: 配置了 Cluster Autoscaler，Pod 一直处于 Pending 状态，但节点池没有自动扩容。

**参考答案 - 完整排查流程**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 检查 Pod Pending 原因
kubectl describe pod <pending-pod> | grep -A 10 Events

# 预期输出:
# Events:
#   Type     Reason            Age   Message
#   Warning  FailedScheduling  2m    0/3 nodes are available: 3 Insufficient cpu.

# Step 2: 检查 Cluster Autoscaler 状态
kubectl get pods -n kube-system -l app=cluster-autoscaler
kubectl logs -n kube-scale -l app=cluster-autoscaler --tail=50

# 常见日志错误:
# "NodePool max capacity reached" -> 节点池已达上限
# "Instance type out of stock" -> ECS 库存不足
# "Quota exceeded" -> 账号配额不足
# "Failed to create instance" -> 创建失败

# Step 3: 检查节点池自动伸缩配置
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '.auto_scaling'

# 确认:
# - enabled: true (是否启用)
# - min_instances 和 max_instances (伸缩范围)
# - 当前实例数是否已达上限

# Step 4: 检查 ECS 配额
aliyun ecs DescribeAccountAttributes --RegionId cn-hangzhou

# Step 5: 检查 vSwitch 可用 IP
aliyun vpc DescribeVSwitchAttributes --VSwitchId <vswitch-id>
```
**常见原因和解决方案**：

| 原因 | 症状 | 解决方案 |
|------|------|---------|
| 节点池未启用自动伸缩 | CA 日志无相关记录 | 启用节点池自动伸缩 |
| 已达 max_instances 上限 | CA 日志: "max capacity reached" | 增加 max_instances |
| ECS 库存不足 | CA 日志: "out of stock" | 更换实例规格或可用区 |
| 账号配额不足 | CA 日志: "quota exceeded" | 申请提高配额 |
| vSwitch IP 耗尽 | Pod 分配不到 IP | 扩展 Pod CIDR 或添加 vSwitch |

---

### 场景 2: Pod 反复 CrashLoopBackOff

**参考答案 - 完整排查流程**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看 Pod 状态和重启次数
kubectl get pods -A | grep CrashLoopBackOff

# 预期输出:
# NAME                      READY   STATUS             RESTARTS   AGE
# my-app-7d9f8b6c4-xyz12   0/1     CrashLoopBackOff   15         45m

# Step 2: 查看 Pod 详情和 Events
kubectl describe pod my-app-7d9f8b6c4-xyz12 -n default

# 关注:
# - Last State: {Reason: Error, Exit Code: 137 (OOMKilled) 或 1 (应用错误)}
# - Events 中的 Back-off 信息

# Step 3: 查看上一次崩溃的日志（关键步骤）
kubectl logs my-app-7d9f8b6c4-xyz12 -n default --previous

# 常见日志特征:
# Exit Code 1: "Error: Cannot connect to database" -> 依赖不可用
# Exit Code 137: OOMKilled -> 内存限制过低
# Exit Code 132: SIGILL -> 二进制不兼容
# Exit Code 139: SIGSEGV -> 段错误

# Step 4: 根据具体原因修复
# 情况A: 配置缺失 -> 补充 ConfigMap/Secret
# 情况B: OOMKilled -> 增大 memory limits
# 情况C: 探针配置不当 -> 调整 initialDelaySeconds
# 情况D: 启动命令错误 -> 修复 command/args

# Step 5: 验证修复
kubectl get pods -w
```
---

### 场景 3: 节点资源不足

**参考答案 - 完整排查流程**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看节点资源分配情况
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 预期输出:
# Allocated resources:
#   CPU Requests  CPU Limits  Memory Requests  Memory Limits
#   3800m (95%)   5000m (125%) 12Gi (75%)      16Gi (100%)
# 显示 CPU requests 已用 95%，且 CPU limits 已超额分配

# Step 2: 查看实际资源使用
kubectl top node <node-name>

# 预期输出:
# NAME         CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-1       1800m        45%    10000Mi         62%

# Step 3: 查看哪些 Pod 占用最多资源
kubectl top pods -A --sort-by=memory | head -10
kubectl top pods -A --sort-by=cpu | head -10

# Step 4: 解决方案
# 方案A: 优化 Pod 资源配置（减小 requests）
# 方案B: 扩容节点池（增加新节点）
# 方案C: 调整 PriorityClass（优先级调度）
# 方案D: 清理不必要的工作负载
```
---

### 场景 4: kube-system 组件异常

**参考答案 - 完整排查流程**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 现象: Pod 之间无法通过 Service 名称访问，但 IP 直连正常
# -> 典型的 DNS 解析问题

# Step 1: 确认 DNS 是否有问题
kubectl run test-dns --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default

# 如果超时或失败 -> DNS 问题确认

# Step 2: 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# Step 3: 检查 CoreDNS Service
kubectl get svc kube-dns -n kube-system
kubectl get endpoints kube-dns -n kube-system

# Step 4: 检查 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# Step 5: 如果 CoreDNS 正常，检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50

# Step 6: 如果 DNS 和 kube-proxy 正常，检查 CNI
kubectl get pods -n kube-system -l app=terway
# 或
kubectl get pods -n kube-system -l app=flannel

# 分层排查路径: DNS -> kube-proxy -> CNI -> 节点网络
```
---

## 四、评分统计

| 部分 | 满分 | 得分 |
|------|------|------|
| 概念理解 | 10 | |
| 命令实操 | 10 | |
| 场景分析 | 20 | |
| **自评加分** | 10 | |
| **合计** | **50** | |

**自评加分标准** (最高 10 分):
- 本周每日教案按时完成 +2
- 独立排查了节点/Pod 问题 +3
- 实践了多节点池架构设计 +3
- 整理了组件运维手册 +2

**评估标准**：
- **45-50 分**: 优秀，完全掌握本周内容，可以独立处理节点和工作负载相关运维任务
- **35-44 分**: 良好，核心概念理解，部分细节需加强实践
- **25-34 分**: 及格，建议重点复习薄弱环节
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

| 薄弱点 | 对应 Day | 补强计划 |
|--------|---------|---------|
| | | |
| | | |
| | | |

---

## 要点总结

- **节点状态**有 Ready/NotReady/Unknown 三种，NotReady 超时后 Pod 会被驱逐
- **cordon → drain → 维护 → uncordon** 是标准的节点维护流程
- **Taint/Toleration** 实现节点专用化，配合 nodeSelector 和 nodeAffinity 实现精细调度
- **livenessProbe** 失败重启容器，**readinessProbe** 失败停止流量
- **CrashLoopBackOff** 排查三板斧：`describe` → `logs --previous` → `get events`
- **资源不足**排查：`describe node` 看 Allocated resources，`top pods` 找大消费者

---

## 下周计划调整

基于本周自测结果，调整 Week 4 学习重点:

- [ ] 需要加强: ___
- [ ] 可以快速过: ___
- [ ] 特别关注: ___

---

## 延伸阅读

- [节点管理最佳实践](https://kubernetes.io/docs/concepts/architecture/nodes/)
- [Pod 生命周期](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/)
- [调度器文档](https://kubernetes.io/docs/concepts/scheduling-eviction/)
- [文件: `../../../domain-02-workloads-applications/02-deployment-production-patterns.md`](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-02-workloads-applications/00-core-workloads/01-deployment-production-patterns.md)

## Related

- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
