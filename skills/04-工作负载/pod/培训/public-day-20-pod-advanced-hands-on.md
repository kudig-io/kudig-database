---
title: 'Day 20: Pod 容器组进阶实操'
description: '- Kubernetes Pod 调度深度配置'
summary: '- Kubernetes Pod 调度深度配置'
category: learning
tags:
- k8s
- training
- hands-on
- scheduler
- pdb
- job
- operator
- gpu
- cuda
- nvidia
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 20: Pod 容器组进阶实操 是什么'
- '如何 Day 20: Pod 容器组进阶实操'
trigger_keywords:
- Day
- '20:'
- Pod
- 容器组进阶实操
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
title: Day 20: Pod 容器组进阶实操
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[kubernetes|Kubernetes]] Pod 调度深度配置
  - Pod 亲和性反亲和性
  - 拓扑分布约束配置
  - 探针与资源配置
trigger_keywords:
  - Pod
  - 调度
  - 亲和性
  - topology
  - 拓扑
  - 探针
  - livenessProbe
  - readinessProbe
  - PDB
  - PriorityClass
reading_level: advanced
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - 故障诊断
  - 工作负载
related_topics:
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-19-pod-basics/01-pod-basics-hands-on
  - 生产运维/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
---

# Day 20: Pod 容器组进阶实操

> **日期**: Week 3 Day 6 | **主题**: Pod 调度、探针与资源配置 | **版本**: K8s 1.28-1.33

---

## 1. Pod 调度深度配置

### 1.1 nodeSelector（节点选择器）

```yaml
# 调度到特定标签的节点
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  nodeSelector:
    gpu: "true"
    zone: us-east-1a
  containers:
    - name: ml
      image: pytorch:latest
      resources:
        limits:
          nvidia.com/gpu: 1
```

### 1.2 节点亲和性与反亲和性

```yaml
# 软偏好：优先调度到 GPU 节点，但其他节点也可以
apiVersion: v1
kind: Pod
metadata:
  name: ml-workload
spec:
  affinity:
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 80
          preference:
            matchExpressions:
              - key: "gpu"
                operator: In
                values: ["true"]
        - weight: 20
          preference:
            matchExpressions:
              - key: "zone"
                operator: In
                values: ["us-east-1a", "us-east-1b"]
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 100
          preference:
            matchLabels:
              app: ml-workload
  containers:
    - name: ml
      image: pytorch:latest
```

### 1.3 拓扑分布约束

```yaml
# 跨可用区均匀分布 Pod
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  replicas: 6
  selector:
    matchLabels:
      app: api-server
  template:
    spec:
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
        - maxSkew: 1
          topologyKey: kubernetes.io/hostname
          whenUnsatisfiable: ScheduleAnyway
          labelSelector:
            matchLabels:
              app: api-server
      containers:
        - name: api
          image: api:v1
```

---

## 2. 探针深度配置

### 2.1 HTTP 探针

```yaml
livenessProbe:
  httpGet:
    path: /healthz
    port: 8080
    httpHeaders:
      - name: X-Custom-Header
        value: "healthy"
  initialDelaySeconds: 15
  periodSeconds: 10
  timeoutSeconds: 5
  successThreshold: 1
  failureThreshold: 3
```

### 2.2 TCP 探针

```yaml
readinessProbe:
  tcpSocket:
    port: 5432
  initialDelaySeconds: 5
  periodSeconds: 10
  failureThreshold: 3
```

### 2.3 Exec 探针

```yaml
startupProbe:
  exec:
    command:
      - cat
      - /tmp/healthy
  initialDelaySeconds: 0
  periodSeconds: 5
  failureThreshold: 30
```

### 2.4 探针故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试探针端点
kubectl exec -it <pod-name> -- curl -s http://localhost:8080/healthz

# 测试 TCP 端口
kubectl exec -it <pod-name> -- nc -zv localhost 5432

# 查看探针失败原因
kubectl describe pod <pod-name> | grep -A15 "Liveness"
```
---

## 3. Pod 资源优化

### 3.1 资源 requests 与 limits

```yaml
# 生产环境推荐配置
resources:
  requests:
    cpu: "500m"      # 调度依据
    memory: "512Mi"
  limits:
    cpu: "1000m"     # 限制上限
    memory: "1Gi"     # 超过 OOM Kill
```

### 3.2 GPU 资源配置

```yaml
# GPU Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: ml-training
spec:
  containers:
    - name: ml
      image: pytorch:latest
      resources:
        limits:
          nvidia.com/gpu: "2"  # 请求 2 GPU
        requests:
          nvidia.com/gpu: "2"
      env:
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1"
```

### 3.3 临时存储管理

```yaml
# 临时存储配置
apiVersion: v1
kind: Pod
metadata:
  name: app-with-tmp
spec:
  containers:
    - name: app
      image: app:v1
      volumeMounts:
        - name: tmp
          mountPath: /tmp
      resources:
        limits:
          ephemeral-storage: "1Gi"
  volumes:
    - name: tmp
      emptyDir:
        sizeLimit: "1Gi"
```

---

## 4. Pod 调度优先级

### 4.1 PriorityClass

```yaml
# 创建优先级类
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 100000
globalDefault: false
description: "高优先级工作负载"
```

### 4.2 使用优先级

```yaml
# Pod 使用优先级
apiVersion: v1
kind: Pod
metadata:
  name: critical-job
spec:
  priorityClassName: high-priority
  containers:
    - name: app
      image: app:v1
```

### 4.3 抢占与驱逐

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 低优先级 Pod 被高优先级 Pod 抢占
kubectl get pods --sort-by='.spec.priority' | tail -20

# 查看抢占事件
kubectl describe pod <pod-name> | grep -A10 "Events:" | grep -i "preempt"
```
---

## 5. Pod 中断与容忍

### 5.1 污点容忍

```yaml
# 容忍所有污点（谨慎使用）
tolerations:
  - operator: Exists  # 匹配所有污点
    effect: NoSchedule

# 容忍特定污点
tolerations:
  - key: "node.kubernetes.io/not-ready"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 300
```

### 5.2 Pod 中断预算（PDB）

```yaml
# 保证最少可用副本
apiVersion: policy/v1beta1
kind: PodDisruptionBudget
metadata:
  name: api-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: api

# 或使用百分比
apiVersion: policy/v1beta1
kind: PodDisruptionBudget
metadata:
  name: api-pdb-percent
spec:
  maxUnavailable: "25%"
  selector:
    matchLabels:
      app: api
```

---

## 6. Pod 网络配置

### 6.1 Host 网络

```yaml
# 使用宿主机网络（谨慎）
apiVersion: v1
kind: Pod
metadata:
  name: host-network-pod
spec:
  hostNetwork: true
  dnsPolicy: ClusterFirstWithHostNet
  containers:
    - name: app
      image: app:v1
```

### 6.2 DNS 配置

```yaml
# 自定义 DNS 配置
apiVersion: v1
kind: Pod
metadata:
  name: custom-dns-pod
spec:
  dnsPolicy: ClusterFirst  # 默认：集群 DNS
  # dnsPolicy: Default      # 使用节点 DNS
  # dnsPolicy: ClusterFirstWithHostNet  # 配合 hostNetwork 使用
  # dnsPolicy: None         # 自定义 dnsConfig
  dnsConfig:
    nameservers:
      - 8.8.8.8
    searches:
      - svc.cluster.local
      - mydomain.com
    options:
      - name: ndots
        value: "2"
```

---

## 7. Pod 生命周期钩子

### 7.1 PostStart 钩子

```yaml
# 容器启动后执行
apiVersion: v1
kind: Pod
metadata:
  name: with-post-start
spec:
  containers:
    - name: app
      image: app:v1
      lifecycle:
        postStart:
          exec:
            command:
              - sh
              - -c
              - "echo '容器已启动' > /tmp/startup.log"
```

### 7.2 PreStop 钩子

```yaml
# 容器终止前执行（优雅关闭）
apiVersion: v1
kind: Pod
metadata:
  name: with-pre-stop
spec:
  containers:
    - name: app
      image: app:v1
      lifecycle:
        preStop:
          exec:
            command:
              - sh
              - -c
              - "nginx -s quit; sleep 5"
      terminationGracePeriodSeconds: 30
```

---

## 8. Pod 问题高级排查

### 8.1 调度异常

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看调度决策详情
kubectl describe pod <pod-name> | grep -A30 "Events:"

# 测试调度（dry-run）
kubectl create -f pod.yaml --dry-run=client

# 查看调度器日志
kubectl logs -n kube-system kube-scheduler-xxx --tail=50 | grep <pod-name>
```
### 8.2 网络异常

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试 Pod 间连通性
kubectl exec -it <pod-a> -- ping -c 3 <service-b>

# 查看 Pod DNS 解析
kubectl exec -it <pod-a> -- nslookup service-name.namespace.svc.cluster.local

# 查看 Pod 网络接口
kubectl exec -it <pod-a> -- ip addr
```
---

## 9. 实战练习

**练习 1**: 配置 Deployment 使用 topologySpreadConstraints 跨 3 个可用区均匀分布

**练习 2**: 配置 livenessProbe 检测应用健康，readinessProbe 检测依赖就绪

**练习 3**: 创建 PriorityClass 并验证高优先级 Pod 抢占低优先级 Pod

**练习 4**: 配置 PDB 确保升级期间核心服务始终有 2 个以上副本可用

---



<!-- risk-assessed -->
