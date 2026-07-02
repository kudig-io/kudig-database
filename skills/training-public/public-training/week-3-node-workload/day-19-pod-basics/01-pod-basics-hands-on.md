---
title: 'Day 19: Pod 容器组基础实操'
description: '- Pod 探针配置'
summary: 'kubectl get pod <pod-name> -o jsonpath='{.status.conditions[*].type}''
category: learning
tags:
- k8s
- training
- hands-on
- docker
- hpa
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 19: Pod 容器组基础实操 是什么'
- '如何 Day 19: Pod 容器组基础实操'
trigger_keywords:
- Day
- '19:'
- Pod
- 容器组基础实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 19: Pod 容器组基础实操
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] Pod 生命周期
  - Pod 探针配置
  - CrashLoopBackOff 排查
  - Pod QoS 等级
trigger_keywords:
  - Pod
  - 生命周期
  - 探针
  - livenessProbe
  - readinessProbe
  - CrashLoopBackOff
  - ImagePullBackOff
  - QoS
  - Init 容器
reading_level: intermediate
audience:
  - sre-engineer
  - ops-engineer
  - developer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - domain-02-workloads-applications
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-20-pod-advanced/01-pod-advanced-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
---

# Day 19: Pod 容器组基础实操

> **日期**: Week 3 Day 5 | **主题**: Pod 生命周期与基本操作 | **版本**: K8s 1.28-1.33

---

## 1. Pod 生命周期

### 1.1 Pod 状态与 Conditions

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 完整状态
kubectl get pod <pod-name> -o yaml | grep -A20 "status:"

# 查看 Pod Conditions
kubectl get pod <pod-name> -o jsonpath='{.status.conditions[*].type}'

# 常见 Conditions
# - Ready: Pod 可以接受流量
# - PodScheduled: Pod 已被调度
# - Initialized: Init 容器已完成
# - ContainersReady: 所有容器已就绪
```
### 1.2 Pod 阶段（Phase）

| Phase | 含义 | 正常 |
|-------|------|------|
| `Pending` | Pod 已被 K8s 系统接收，容器镜像未拉取或调度未完成 | 短暂 |
| `Running` | Pod 已绑定到节点，所有容器已创建且至少一个在运行 | 正常 |
| `Succeeded` | 所有容器已成功终止，不会重启 | 一次性 Job |
| `Failed` | 所有容器已终止，至少一个容器以非零退出 | 需排查 |
| `Unknown` | 无法获取 Pod 状态（通常节点通信问题） | 排查节点 |

### 1.3 容器状态

```
Created → Started → Ready → Terminating → Terminated
              ↓
          Waiting (镜像拉取中)
              ↓
          Running → ExitCode 0 (成功)
                  → ExitCode ≠ 0 (失败，可能重启)
```

---

## 2. Pod 生命周期管理

### 2.1 创建 Pod（最小配置）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nginx-pod
  namespace: default
spec:
  containers:
    - name: nginx
      image: nginx:1.25
      ports:
        - containerPort: 80
      resources:
        requests:
          cpu: "100m"
          memory: "128Mi"
        limits:
          cpu: "500m"
          memory: "512Mi"
```

### 2.2 Init 容器

```yaml
# Init 容器：主容器启动前执行初始化
apiVersion: v1
kind: Pod
metadata:
  name: web-app
spec:
  initContainers:
    - name: init-db
      image: busybox:1.36
      command:
        - sh
        - -c
        - |
          echo "等待数据库就绪..."
          until nslookup db-service; do
            sleep 5
          done
          echo "数据库已就绪"
    - name: migrate
      image: app-migrate:latest
      command: ["./migrate.sh"]
  containers:
    - name: web
      image: web:v1
      ports:
        - containerPort: 8080
```

### 2.3 探针（Probes）配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: web-server
spec:
  containers:
    - name: web
      image: web:v1
      ports:
        - containerPort: 8080

      # 存活探针（重启容器）
      livenessProbe:
        httpGet:
          path: /healthz
          port: 8080
        initialDelaySeconds: 15
        periodSeconds: 10
        failureThreshold: 3

      # 就绪探针（接受流量）
      readinessProbe:
        httpGet:
          path: /ready
          port: 8080
        initialDelaySeconds: 5
        periodSeconds: 5
        failureThreshold: 3

      # 启动探针（启动期间禁用探针）
      startupProbe:
        httpGet:
          path: /started
          port: 8080
        initialDelaySeconds: 0
        periodSeconds: 5
        failureThreshold: 30  # 最多等待 2.5 分钟
```

---

## 3. Pod 基本操作

### 3.1 查看 Pod 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 列表
kubectl get pods

# 查看 Pod 详情
kubectl describe pod <pod-name>

# 查看 Pod 日志（主容器）
kubectl logs <pod-name>

# 查看上一个 terminated 容器的日志
kubectl logs <pod-name> --previous

# 查看特定容器的日志（多容器 Pod）
kubectl logs <pod-name> -c <container-name>

# 实时跟踪日志
kubectl logs -f <pod-name> --tail=100
```
### 3.2 进入容器调试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入容器（如有 shell）
kubectl exec -it <pod-name> -- /bin/sh

# 进入特定容器（多容器 Pod）
kubectl exec -it <pod-name> -c <container-name> -- /bin/bash

# 执行单个命令
kubectl exec <pod-name> -- ls /app

# 复制文件
kubectl cp <pod-name>:/app/log.txt ./log.txt
kubectl cp ./config.yaml <pod-name>:/app/config.yaml
```
### 3.3 Pod 扩缩容

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 通过 Deployment 管理（推荐）
kubectl scale deployment <deploy-name> --replicas=3

# 通过 HPA 自动扩缩容
kubectl autoscale deployment <deploy-name> --cpu-percent=80 --min=2 --max=10

# 查看扩缩容状态
kubectl get hpa
kubectl describe hpa <hpa-name>
```
---

## 4. Pod 故障排查

### 4.1 CrashLoopBackOff

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看容器状态
kubectl get pod <pod-name> -o wide

# 2. 查看重启次数
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].restartCount}'

# 3. 查看上一个容器的日志
kubectl logs <pod-name> --previous

# 4. 查看详细事件
kubectl describe pod <pod-name> | grep -A10 "Events:"

# 常见原因：
# - 应用启动命令错误
# - 配置文件缺失
# - 依赖服务不可达
# - OOM（内存限制过低）
```
### 4.2 ImagePullBackOff

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看拉取错误
kubectl describe pod <pod-name> | grep -A5 "ImagePull"

# 2. 确认镜像存在
docker pull <image-name>

# 3. 检查镜像标签
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 4. 检查 ImagePullSecrets（私有仓库）
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets}'

# 常见原因：
# - 镜像名称拼写错误
# - 镜像不存在或标签错误
# - 没有 ImagePullSecrets（私有仓库）
# - 网络问题无法拉取
```
### 4.3 Pending（调度失败）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看调度原因
kubectl describe pod <pod-name> | grep -A20 "Events:"

# 2. 常见原因
# - Insufficient cpu/memory: 增加资源或清理节点
# - node(s) had taint: 添加污点容忍
# - node(s) didn't match node selector: 检查 nodeSelector

# 3. 检查节点资源
kubectl describe nodes | grep -A5 "Allocated resources"
kubectl top nodes
```
---

## 5. Pod 资源管理

### 5.1 QoS 等级

```
Guaranteed（最高优先级）
  → 所有容器设置了 CPU 和内存的 limits 且与 requests 相等

Burstable（中等优先级）
  → 容器设置了 CPU 或内存 requests 但不满足 Guaranteed

BestEffort（最低优先级）
  → 未设置任何 requests 或 limits
```

### 5.2 资源配额配置

```yaml
# 设置默认资源限制（LimitRange）
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: production
spec:
  limits:
    - type: Container
      default:
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:
        cpu: "100m"
        memory: "128Mi"
      max:
        cpu: "4"
        memory: "8Gi"
      min:
        cpu: "50m"
        memory: "64Mi"
```

### 5.3 资源监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 资源使用
kubectl top pods -n production

# 按内存排序
kubectl top pods -n production --sort-by=memory

# 按 CPU 排序
kubectl top pods -n production --sort-by=cpu

# 查看所有命名空间
kubectl top pods -A
```
---

## 6. Pod 安全配置

### 6.1 SecurityContext

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
  containers:
    - name: app
      image: app:v1
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop:
            - ALL
      volumeMounts:
        - name: tmp
          mountPath: /tmp
  volumes:
    - name: tmp
      emptyDir: {}
```

---

## 7. 实战练习

**练习 1**: 创建包含 Init 容器的 Pod，Init 容器等待 Service 就绪后再启动主容器

**练习 2**: 配置 livenessProbe、readinessProbe、startupProbe 三个探针

**练习 3**: 模拟 CrashLoopBackOff 问题，排查并修复应用配置

**练习 4**: 配置 SecurityContext，禁止容器以 root 运行

---



<!-- risk-assessed -->
