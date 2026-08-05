---
title: 'P3: 节点与工作负载管理实践'
description: 'title: P3: 节点与工作负载管理实践'
summary: 'title: P3: 节点与工作负载管理实践'
category: learning
tags:
- k8s
- training
- hands-on
- flannel
- coredns
- hpa
- vpa
- daemonset
- job
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'P3: 节点与工作负载管理实践 是什么'
- '如何 P3: 节点与工作负载管理实践'
trigger_keywords:
- 'P3:'
- 节点与工作负载管理实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: P3: 节点与工作负载管理实践
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - ACK multi-nodepool architecture design
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] node maintenance drain uncordon
  - Pod scheduling affinity anti-affinity
  - Kubernetes health probes configuration
  - Cluster autoscaler scaling policy
trigger_keywords:
  - nodepool
  - node maintenance
  - drain
  - cordon
  - uncordon
  - scheduling
  - affinity
  - probes
  - Cluster Autoscaler
  - spot instance
reading_level: advanced
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-3-node
  - domain-9-workload
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
---

# P3: 节点与工作负载管理实践

> **对应周次**: Week 3 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 概述

本实践项目要求你设计一个多节点池架构，完成节点运维操作（扩缩容、维护、升级），部署多种工作负载并配置调度策略与健康检查。通过这个项目，你将综合运用 Week 3 所学的所有知识——节点管理、节点池运维、Pod 调度、探针配置和组件健康检查。

### 项目目标

设计和实施多节点池架构，完成节点运维操作（扩缩容、维护、升级），部署多种工作负载并配置调度策略与健康检查。

### 前置条件

- 完成 Week 3 全部教案 (Day 15-21)
- 有运行中的 ACK 集群（至少 3 个节点）
- 了解节点池和 Pod 调度概念

---

## 核心概念回顾

### 多节点池架构设计原则

生产环境的节点池设计通常遵循"分层隔离"原则，将不同类型的 workload 隔离到不同的节点池中：

- **系统节点池**: 运行 [[CoreDNS|CoreDNS]]、[[Ingress|Ingress]] Controller、监控 Agent 等系统组件。使用 Taint 阻止业务 Pod 调度。通常 2-3 个节点，使用中等规格实例
- **业务节点池**: 运行应用工作负载。可以根据业务类型（在线/离线）进一步细分。支持自动扩缩容
- **专用节点池**: 运行 GPU 任务、高内存任务等特殊工作负载。使用标签和污点精确调度

### 调度策略选择指南

| 策略 | 适用场景 | 复杂度 |
|------|---------|--------|
| nodeSelector | 简单的标签匹配 | 低 |
| nodeAffinity (required) | 必须调度到特定节点 | 中 |
| nodeAffinity (preferred) | 优先调度到特定节点 | 中 |
| podAntiAffinity | 分散 Pod 到不同节点 | 中 |
| Taint + Toleration | 专用节点池调度 | 高 |

---

## 实施步骤

### Step 1: 多节点池架构设计与创建 (40min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1.1 设计节点池架构
# ┌─────────────────────────────────────────┐
# │           ACK 集群                       │
# ├──────────────┬──────────────┬───────────┤
# │ system-pool  │ app-pool     │ spot-pool │
# │ 4C16G × 2   │ 8C32G × 2-5 │ 8C32G × 0-3│
# │ 系统组件     │ 业务应用      │ 弹性任务   │
# └──────────────┴──────────────┴───────────┘

# 1.2 查看现有节点池
aliyun cs GET /clusters/<cluster_id>/nodepools
# 预期输出: 列出所有节点池及其配置

# 1.3 创建 spot (抢占式) 节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "spot-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id>"],
    "instance_types": ["ecs.g6.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 1,
    "spot_strategy": "SpotWithPriceLimit",
    "spot_price_limit": [{"instance_type": "ecs.g6.2xlarge", "price_limit": "0.5"}]
  },
  "kubernetes_config": {
    "labels": [{"key": "node-type", "value": "spot"}],
    "taints": [{"key": "spot-instance", "value": "true", "effect": "PreferNoSchedule"}]
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 0,
    "max_instances": 3
  }
}'

# 预期输出: 节点池创建任务 ID

# 1.4 等待节点池就绪
aliyun cs GET /clusters/<cluster_id>/nodepools/<spot-pool-id>
# 检查 state 是否为 active

# 1.5 查看所有节点池
aliyun cs GET /clusters/<cluster_id>/nodepools | jq '.[].nodepool_info.name'
# 预期输出:
# "system-pool"
# "app-pool"
# "spot-pool"

# 1.6 查看节点标签分布
kubectl get nodes --show-labels | grep node-type
# 预期输出:
# node-1   Ready   ...   node-type=spot,...
```
### Step 2: 节点运维操作 (40min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

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
# 2.1 获取 spot 节点名称
NODE_NAME=$(kubectl get nodes -l node-type=spot -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
echo "Spot node: ${NODE_NAME}"

# 如果没有 spot 节点，使用任意节点
if [ -z "$NODE_NAME" ]; then
  NODE_NAME=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
fi

# 2.2 节点维护模式 (cordon + drain)
# 标记不可调度（阻止新 Pod 被调度到此节点）
kubectl cordon ${NODE_NAME}
# 预期输出: node/<node-name> cordoned

kubectl get nodes
# 预期输出: 该节点显示 SchedulingDisabled
# NAME      STATUS                     ROLES    AGE   VERSION
# node-1    Ready,SchedulingDisabled   <none>   30d   v1.28.3

# 驱逐 Pod（优雅迁移到其他节点）
kubectl drain ${NODE_NAME} --ignore-daemonsets --delete-emptydir-data --timeout=120s
# 预期输出:
# evicting pod default/web-app-abc12
# evicting pod default/web-app-def34
# pod/web-app-abc12 evicted
# pod/web-app-def34 evicted
# node/node-1 drained

# 模拟维护操作（查看节点信息）
kubectl describe node ${NODE_NAME} | grep -A 5 "Conditions:"
# 预期输出: 节点条件详情

# 模拟维护完成，恢复调度
kubectl uncordon ${NODE_NAME}
# 预期输出: node/<node-name> uncordoned

kubectl get nodes
# 预期输出: 节点恢复正常 Ready 状态

# 2.3 节点标签管理
kubectl label nodes ${NODE_NAME} environment=staging
# 预期输出: node/<node-name> labeled

kubectl label nodes ${NODE_NAME} team=backend
# 预期输出: node/<node-name> labeled

# 查看节点标签
kubectl get node ${NODE_NAME} --show-labels | tr ',' '\n' | grep -E "environment|team"
# 预期输出:
# environment=staging
# team=backend

# 2.4 节点污点管理
kubectl taint nodes ${NODE_NAME} maintenance=true:NoExecute
# 预期输出: node/<node-name> tainted

# 观察没有 Toleration 的 Pod 被驱逐
kubectl get pods -o wide | grep ${NODE_NAME}

# 移除污点
kubectl taint nodes ${NODE_NAME} maintenance=true:NoExecute-
# 预期输出: node/<node-name> untainted
```
### Step 3: 工作负载部署与调度 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 3.1 部署 Deployment（调度到 app-pool）
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: week3-practice
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      nodeSelector:
        node-role: app
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values: [web-app]
            topologyKey: kubernetes.io/hostname
      containers:
      - name: web
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        startupProbe:
          httpGet:
            path: /
            port: 80
          failureThreshold: 30
          periodSeconds: 2
        livenessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 10
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
          failureThreshold: 3
EOF

# 预期输出: deployment.apps/web-app created

# 查看部署状态和调度位置
kubectl get pods -n week3-practice -o wide
# 预期输出:
# NAME                       READY   STATUS    RESTARTS   AGE   IP            NODE
# web-app-6d4f7b8c9d-abc12   1/1     Running   0          1m    172.20.0.10   node-app-1
# web-app-6d4f7b8c9d-def34   1/1     Running   0          1m    172.20.1.11   node-app-2
# web-app-6d4f7b8c9d-ghi56   1/1     Running   0          1m    172.20.2.12   node-app-3

# 3.2 部署可调度到 spot 节点的批处理任务
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-job
  namespace: week3-practice
spec:
  completions: 5
  parallelism: 3
  backoffLimit: 6
  template:
    spec:
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: PreferNoSchedule
      nodeSelector:
        node-type: spot
      containers:
      - name: worker
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "Processing task $(date) on $(hostname)..." && sleep 30 && echo "Done at $(date)"']
        resources:
          requests:
            cpu: 100m
            memory: 64Mi
          limits:
            cpu: 200m
            memory: 128Mi
      restartPolicy: Never
EOF

# 预期输出: job.batch/batch-job created

# 3.3 查看调度结果
kubectl get pods -n week3-practice -o wide
# 预期输出: 所有 Pod 都调度到了正确的节点池

kubectl get pods -n week3-practice -l job-name=batch-job -o wide
# 预期输出: Job Pod 调度到 spot 节点

# 查看 Job 完成状态
kubectl get job batch-job -n week3-practice
# 预期输出:
# NAME        COMPLETIONS   DURATION   AGE
# batch-job   5/5           2m         3m
```
### Step 4: 组件健康检查 (20min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 4.1 全面组件检查脚本
echo "=========================================="
echo "  集群组件健康检查"
echo "=========================================="

echo ""
echo "=== kube-system 组件 ==="
kubectl get pods -n kube-system --sort-by='.status.phase'
# 预期输出: 所有 Pod 为 Running 状态

echo ""
echo "=== CoreDNS ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns
# 预期输出:
# NAME                       READY   STATUS    RESTARTS   AGE
# coredns-7f6cb4b4f7-abc12   1/1     Running   0          30d

echo ""
echo "=== kube-proxy ==="
kubectl get ds -n kube-system kube-proxy
# 预期输出:
# NAME         DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE
# kube-proxy   3         3         3       3            3

echo ""
echo "=== CNI 插件 ==="
kubectl get ds -n kube-system | grep -E "terway|flannel"
# 预期输出: CNI DaemonSet 正常

echo ""
echo "=== API Server 健康 ==="
kubectl get --raw /healthz
# 预期输出: ok

echo ""
echo "=== 节点状态 ==="
kubectl get nodes -o wide
# 预期输出: 所有节点 Ready

# 4.2 DNS 测试
kubectl run dns-check --rm -it --restart=Never \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  -n week3-practice -- \
  sh -c 'echo "=== DNS Test ==="; nslookup kubernetes.default; echo "DNS OK"'
# 预期输出:
# Server:    172.21.0.10
# Name:      kubernetes.default
# Address 1: 172.21.0.1 kubernetes.default.svc.cluster.local
# DNS OK

# 4.3 节点资源使用检查
kubectl top nodes
# 预期输出:
# NAME      CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-1    500m         12%    4096Mi          25%
# node-2    350m         8%     3584Mi          21%
# node-3    200m         5%     2048Mi          12%
```
---

## 配置示例

### 完整的多节点池工作负载清单

```yaml
# web-app-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: week3-practice
  labels:
    app: web-app
    tier: frontend
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: web-app
        tier: frontend
    spec:
      nodeSelector:
        node-role: app
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: [web-app]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: web
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        startupProbe:
          httpGet:
            path: /
            port: 80
          failureThreshold: 30
          periodSeconds: 2
        livenessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 5"]
      terminationGracePeriodSeconds: 30
---
apiVersion: v1
kind: Service
metadata:
  name: web-app
  namespace: week3-practice
spec:
  selector:
    app: web-app
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
---
# batch-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-job
  namespace: week3-practice
spec:
  completions: 5
  parallelism: 3
  backoffLimit: 6
  activeDeadlineSeconds: 600
  template:
    spec:
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: PreferNoSchedule
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: NoExecute
      nodeSelector:
        node-type: spot
      restartPolicy: Never
      containers:
      - name: worker
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "Task start: $(date)" && sleep 30 && echo "Task done: $(date)"']
        resources:
          requests:
            cpu: 100m
            memory: 64Mi
          limits:
            cpu: 200m
            memory: 128Mi

```

---

## 常见问题

### Q1: 抢占式实例被回收后 Job 会怎样？

如果 Job 的 Pod 运行在抢占式实例上，当实例被回收时 Pod 会丢失。如果 Job 的 restartPolicy 为 Never，该 Pod 会被标记为 Failed，Job Controller 会创建新的 Pod 在其他节点上重试（直到 backoffLimit）。建议为运行在 spot 节点上的 Job 设置合理的 backoffLimit 和 activeDeadlineSeconds。

### Q2: podAntiAffinity 导致 Pod 无法调度怎么办？

如果使用 requiredDuringSchedulingIgnoredDuringExecution（硬性约束），当节点数少于 Pod 数量时，多余的 Pod 无法调度。解决方法：使用 preferredDuringSchedulingIgnoredDuringExecution（软性约束），或者增加节点数量。

### Q3: 三种探针如何搭配使用？

推荐配置：startupProbe（长启动时间应用才需要，覆盖应用初始化阶段）+ livenessProbe（只检查最基本存活，如 TCP 端口）+ readinessProbe（检查完整服务就绪，如 HTTP 200）。startupProbe 成功后才会触发 livenessProbe 和 readinessProbe。

### Q4: 如何验证节点标签和污点是否正确？

使用 `kubectl get nodes --show-labels` 查看所有节点标签，使用 `kubectl describe node <name> | grep -A 5 Taints` 查看污点。在部署应用前，先验证 nodeSelector 和 tolerations 是否匹配。

---

## 验收清单

- [ ] 成功创建多节点池架构（system + app + spot）
- [ ] 完成节点 cordon/drain/uncordon 操作
- [ ] Deployment 正确调度到指定节点池
- [ ] Job 使用 tolerations 调度到 spot 节点
- [ ] 三种探针均配置正确且工作正常
- [ ] 所有 kube-system 组件运行正常
- [ ] DNS 解析测试通过
- [ ] 节点资源使用正常

---

## 要点总结

| 操作 | 命令 | 说明 |
|------|------|------|
| 创建节点池 | `aliyun cs POST /clusters/<id>/nodepools` | 指定实例规格、标签、污点 |
| 节点维护 | `kubectl cordon/drain/uncordon` | 维护前驱逐 Pod |
| 标签管理 | `kubectl label nodes <name> key=value` | 用于 nodeSelector 调度 |
| 污点管理 | `kubectl taint nodes <name> key=value:effect` | 用于专用节点池 |
| 组件检查 | `kubectl get pods -n kube-system` | 确保系统组件正常 |
| DNS 测试 | `kubectl run dns-test --rm -it -- nslookup` | 验证 CoreDNS |

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete deployment web-app -n week3-practice
kubectl delete job batch-job -n week3-practice
kubectl delete namespace week3-practice  # ⚠️ 不可逆：永久删除命名空间及全部资源
# 删除 spot 节点池（可选）
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<spot-pool-id>
```
---

## 延伸阅读

- [ACK 节点池管理](../../domain-12-cloud-providers/04-alicloud-ack/230-ack-node-pool.md)
- [Pod 调度策略](../../domain-09-workload/05-pod-scheduling-strategies.md)
- [HPA/VPA 自动伸缩](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-02-workloads-applications/00-core-workloads/11-hpa-vpa-autoscaling.md)
- [节点 NotReady 诊断](../../domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md)

```

<!-- risk-assessed -->
