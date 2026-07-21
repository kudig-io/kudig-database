---
title: 工作负载知识词典
description: 涵盖 Kubernetes 工作负载全领域的完整术语体系，包括 Pod、Deployment、StatefulSet、Job、DaemonSet、自动扩缩容等
summary: 工作负载领域词典，覆盖 Pod、Deployment、StatefulSet、Job/CronJob、DaemonSet、OpenKruise、Sidecar 等核心概念
category: dictionary
tags:
- dictionary
- workloads
- pod
- deployment
- statefulset
- job
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: beginner
audience:
- 开发工程师
- 平台工程师
- SRE
---

# 工作负载知识词典（Workloads）

> 本词典覆盖 Kubernetes 工作负载领域的核心术语、技术组件及工程实践，是开发工程师和平台工程师管理应用生命周期的权威参考。

## 领域概述

工作负载是 Kubernetes 上运行的应用程序，核心类型包括：

- **无状态服务**：Deployment + ReplicaSet，可水平扩展
- **有状态服务**：StatefulSet，稳定网络标识 + 持久存储
- **批处理任务**：Job/CronJob，一次性或定时任务
- **节点守护**：DaemonSet，每节点一个 Pod
- **高级工作负载**：OpenKruise 增强控制器

## 核心术语定义

### Pod 基础

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Pod | K8s 最小调度单元，包含一个或多个容器 | 共享网络/存储、生命周期 |
| Container | Pod 内的运行单元 | 镜像、资源、探针 |
| Init Container | 初始化容器，主容器前顺序执行 | 依赖检查、配置生成 |
| Sidecar Container | 边车容器，与主容器并行运行 | 日志收集、代理 (K8s 1.29+ 原生) |
| Ephemeral Container | 临时调试容器，运行时注入 | kubectl debug |
| Pod Lifecycle | Pod 从创建到终止的状态机 | Pending/Running/Succeeded/Failed |
| Container Lifecycle Hooks | 容器生命周期钩子 | postStart/preStop |
| RuntimeClass | 容器运行时类选择 | runc/kata/gVisor |
| User Namespaces | 用户命名空间隔离 | 容器 root ≠ 主机 root |

### 工作负载控制器

| 术语 | 定义 | 适用场景 |
|------|------|----------|
| Deployment | 无状态应用声明式管理 | Web 服务、API |
| ReplicaSet | 维持指定副本数 | Deployment 内部使用 |
| StatefulSet | 有状态应用，稳定标识 + 存储 | 数据库、消息队列 |
| DaemonSet | 每节点运行一个 Pod | 日志收集、监控 Agent |
| Job | 一次性批处理任务 | 数据迁移、计算 |
| CronJob | 定时 Job | 定时备份、报表 |
| ReplicationController | 旧版副本控制器 | 已被 ReplicaSet 替代 |

### 高级工作负载管理

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| OpenKruise | 阿里开源工作负载增强 | CloneSet/Advanced StatefulSet |
| Pod Group Policy | Pod 组策略（批量调度） | Volcano/Kueue |
| Workload API | Gateway API 的工作负载扩展 | Gateway API |
| Spot/Preemptible | 竞价/可抢占实例工作负载 | 容错批处理 |
| Serverless Workflow | 无服务器工作流编排 | OpenFunction |

### 自动扩缩容

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| HPA | 水平 Pod 自动扩缩容 | CPU/Memory/自定义指标 |
| VPA | 垂直 Pod 自动扩缩容 | 自动调整 requests/limits |
| Autoscaling Workloads | 工作负载自动扩缩综合 | HPA + VPA + KEDA |

## 技术组件索引

### Pod 基础类

- [[系统基础/知识字典/workloads/pod.md|Pod（最小调度单元）]]
- [[系统基础/知识字典/workloads/pods.md|Pods（Pod 管理）]]
- [[系统基础/知识字典/workloads/pod-lifecycle.md|Pod 生命周期]]
- [[系统基础/知识字典/workloads/container-environment.md|容器环境]]
- [[系统基础/知识字典/workloads/container-lifecycle-hooks.md|容器生命周期钩子]]
- [[系统基础/知识字典/workloads/init-containers.md|Init Containers]]
- [[系统基础/知识字典/workloads/sidecar-containers.md|Sidecar Containers]]
- [[系统基础/知识字典/workloads/ephemeral-containers.md|临时容器]]
- [[系统基础/知识字典/workloads/images.md|镜像管理]]
- [[系统基础/知识字典/workloads/runtime-class.md|RuntimeClass]]
- [[系统基础/知识字典/workloads/user-namespaces.md|User Namespaces]]
- [[系统基础/知识字典/workloads/pod-hostname.md|Pod 主机名]]
- [[系统基础/知识字典/workloads/pod-quality-of-service-classes.md|Pod QoS 等级]]
- [[系统基础/知识字典/workloads/advanced-pod-configuration.md|高级 Pod 配置]]
- [[系统基础/知识字典/workloads/container-runtime-interface-cri.md|CRI（容器运行时接口）]]

### 工作负载控制器类

- [[系统基础/知识字典/workloads/deployment.md|Deployment]]
- [[系统基础/知识字典/workloads/deployments.md|Deployments（综合）]]
- [[系统基础/知识字典/workloads/replicaset.md|ReplicaSet]]
- [[系统基础/知识字典/workloads/replicationcontroller.md|ReplicationController]]
- [[系统基础/知识字典/workloads/statefulset.md|StatefulSet]]
- [[系统基础/知识字典/workloads/statefulsets.md|StatefulSets（综合）]]
- [[系统基础/知识字典/workloads/daemonset.md|DaemonSet]]
- [[系统基础/知识字典/workloads/job.md|Job]]
- [[系统基础/知识字典/workloads/jobs.md|Jobs（综合）]]
- [[系统基础/知识字典/workloads/cronjob.md|CronJob]]
- [[系统基础/知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Job 自动清理]]

### 高级工作负载类

- [[系统基础/知识字典/workloads/openkruise.md|OpenKruise（增强控制器）]]
- [[系统基础/知识字典/workloads/openfunction.md|OpenFunction（Serverless）]]
- [[系统基础/知识字典/workloads/slimfaas.md|SlimFaas（轻量 FaaS）]]
- [[系统基础/知识字典/workloads/serverless-workflow.md|Serverless Workflow]]
- [[系统基础/知识字典/workloads/pod-group-policies.md|Pod Group 策略]]
- [[系统基础/知识字典/workloads/spot-and-preemptible-workloads.md|竞价实例工作负载]]

### 自动扩缩容类

- [[系统基础/知识字典/workloads/horizontal-pod-autoscaling.md|HPA]]
- [[系统基础/知识字典/workloads/vertical-pod-autoscaling.md|VPA]]
- [[系统基础/知识字典/workloads/autoscaling-workloads.md|自动扩缩综合]]

### 工作负载管理类

- [[系统基础/知识字典/workloads/managing-workloads.md|工作负载管理]]
- [[系统基础/知识字典/workloads/workload-management.md|工作负载治理]]
- [[系统基础/知识字典/workloads/workload-api.md|Workload API]]
- [[系统基础/知识字典/workloads/workload-reference.md|工作负载参考]]
- [[系统基础/知识字典/workloads/disruptions.md|中断管理]]
- [[系统基础/知识字典/workloads/downward-api.md|Downward API]]

## 工作负载选型决策

```
你的应用是什么类型？
│
├─ 无状态服务 (Web/API)
│   └─ Deployment + HPA
│       ├─ 需要金丝雀/蓝绿 → OpenKruise CloneSet
│       └─ 标准滚动更新 → Deployment
│
├─ 有状态服务 (数据库/消息队列)
│   └─ StatefulSet + PVC
│       ├─ 需要稳定网络标识 → StatefulSet (headless Service)
│       └─ 需要并行扩缩 → OpenKruise Advanced StatefulSet
│
├─ 批处理任务
│   ├─ 一次性 → Job
│   ├─ 定时 → CronJob
│   └─ 分布式训练 → Volcano Job / Kueue
│
├─ 节点守护进程
│   └─ DaemonSet (日志/监控/网络插件)
│
└─ Serverless/函数
    └─ Knative / OpenFaaS / OpenFunction
```

## 生产最佳实践

### Deployment 配置

1. **滚动更新策略**：maxSurge=25%, maxUnavailable=25%（平衡速度与可用性）
2. **就绪探针**：必须配置，确保流量只到就绪 Pod
3. **PDB 保护**：关键服务配置 minAvailable
4. **资源请求**：基于压测设置 requests，避免过度配置

### StatefulSet 配置

1. **Headless Service**：必须配置，提供稳定 DNS
2. **PVC 模板**：使用 volumeClaimTemplates 自动创建
3. **更新顺序**：默认 RollingUpdate，从最大序号开始
4. **数据保护**：reclaimPolicy=Retain 防止误删

### Job/CronJob 配置

1. **重试策略**：backoffLimit 控制失败重试次数
2. **超时控制**：activeDeadlineSeconds 防止任务挂起
3. **并发策略**：CronJob concurrencyPolicy=Forbid 防止重叠
4. **自动清理**：ttlSecondsAfterFinished 自动删除完成的 Job

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Pod CrashLoopBackOff | 应用崩溃/探针失败 | `kubectl logs --previous` |
| Pod Pending | 资源不足/调度约束 | `kubectl describe pod` |
| Deployment 更新卡住 | 新 Pod 未就绪/配额不足 | 检查 ReplicaSet 状态 |
| StatefulSet 扩容失败 | PVC 创建失败/存储不足 | 检查 PVC 事件 |
| Job 一直运行 | 任务挂起/未正确退出 | 检查 activeDeadlineSeconds |
| CronJob 未触发 | 时区问题/调度器异常 | 检查 CronJob 事件、时区 |

## 学习路径

```
基础: Pod → Deployment → Service
进阶: StatefulSet → Job/CronJob → HPA
高级: OpenKruise → 自定义控制器 → Operator 模式
专家: 工作负载编排 → Serverless → AI 训练工作负载
```

## 参考链接

- https://kubernetes.io/docs/concepts/workloads/
- https://kubernetes.io/docs/concepts/workloads/controllers/
- https://openkruise.io/
- https://knative.dev/

## Related

- [[系统基础/知识字典/scheduling/hpa.md|HPA 自动扩缩]]
- [[系统基础/知识字典/configuration/probe.md|探针配置]]
- [[系统基础/知识字典/storage/persistent-volume-claim.md|PVC 存储申请]]
- [[系统基础/知识字典/networking/service.md|Service 服务发现]]

## 工作负载配置示例

### 生产级 Deployment 配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-api
  labels:
    app: web-api
    version: v1
spec:
  replicas: 3
  revisionHistoryLimit: 10  # 保留历史版本
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%
      maxUnavailable: 0  # 保证可用性
  selector:
    matchLabels:
      app: web-api
  template:
    metadata:
      labels:
        app: web-api
        version: v1
    spec:
      terminationGracePeriodSeconds: 60
      containers:
      - name: api
        image: myregistry/web-api:v1.2.3
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            memory: 1Gi
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
        lifecycle:
          preStop:
            exec:
              command: ["sh", "-c", "sleep 10"]
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-api-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: web-api
```

### StatefulSet 配置 (PostgreSQL)

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  serviceName: postgres-headless
  replicas: 3
  podManagementPolicy: Parallel  # 并行扩缩
  updateStrategy:
    type: RollingUpdate
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: postgres:16
        ports:
        - containerPort: 5432
        env:
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: ssd-fast
      resources:
        requests:
          storage: 100Gi
```

### OpenKruise CloneSet (金丝雀发布)

```yaml
apiVersion: apps.kruise.io/v1alpha1
kind: CloneSet
metadata:
  name: web-canary
spec:
  replicas: 10
  updateStrategy:
    type: InPlaceIfPossible  # 原地升级
    maxSurge: 1
    maxUnavailable: 0
    partition: 9  # 9 个保持旧版，1 个升级新版 (金丝雀)
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: web
        image: myapp:v2  # 新版本
```

## 生产案例研究

### 案例：滚动更新导致服务中断

**背景：** 某公司 Deployment 更新时出现 5xx 错误激增。

**根因：**
- 未配置 preStop Hook，Pod 终止时 Service 尚未摘除
- readinessProbe 配置不当，新 Pod 未完全就绪就接收流量
- maxUnavailable=1，更新时可用副本不足

**修复：**
1. 添加 preStop: sleep 10（等待 Service 摘除传播）
2. 优化 readinessProbe（检查依赖服务连通性）
3. maxUnavailable=0（更新时不减少可用副本）
4. 添加 PDB 保护

## 常用运维命令速查

```bash
# === Deployment ===
# 查看 Deployment 状态
kubectl rollout status deployment/web-api
# 查看更新历史
kubectl rollout history deployment/web-api
# 回滚到上一版本
kubectl rollout undo deployment/web-api
# 回滚到指定版本
kubectl rollout undo deployment/web-api --to-revision=3
# 暂停/恢复更新
kubectl rollout pause deployment/web-api
kubectl rollout resume deployment/web-api

# === StatefulSet ===
# 查看 StatefulSet 状态
kubectl get statefulset -o wide
# 强制删除 Pod (不等待)
kubectl delete pod postgres-0 --force --grace-period=0
# 扩容
kubectl scale statefulset postgres --replicas=5

# === Job/CronJob ===
# 查看 Job 状态
kubectl get jobs -A
# 查看 Job 日志
kubectl logs job/my-job
# 手动触发 CronJob
kubectl create job --from=cronjob/my-cronjob manual-run-001
# 删除完成的 Job
kubectl delete jobs --field-selector status.successful=1

# === 调试 ===
# 临时调试容器
kubectl debug -it my-pod --image=nicolaka/netshoot --target=api
# 查看 Pod 事件
kubectl get events --field-selector involvedObject.name=my-pod
# 查看 Pod 资源使用
kubectl top pod my-pod
```

## 常见问题 FAQ

**Q1: Deployment 和 StatefulSet 怎么选？**

A: 
- Deployment: 无状态服务，Pod 可互换，滚动更新快
- StatefulSet: 有状态服务，需要稳定网络标识 (pod-0, pod-1) + 持久存储
判断标准：Pod 重启后是否需要保留数据/身份？是 → StatefulSet，否 → Deployment

**Q2: Sidecar 容器和 Init 容器有什么区别？**

A: 
- Init Container: 主容器启动前顺序执行，完成后退出
- Sidecar Container: 与主容器并行运行，整个 Pod 生命周期存在
K8s 1.29+ 原生支持 Sidecar (restartPolicy: Always 的 init container)，保证主容器退出后 Sidecar 才退出。

**Q3: 如何实现零停机部署？**

A: 关键配置：
1. readinessProbe: 确保新 Pod 就绪后才接收流量
2. preStop Hook: sleep 等待 Service 摘除
3. maxUnavailable=0: 更新时不减少可用副本
4. PDB: 保护最小可用数
5. 优雅停机: 应用处理 SIGTERM，完成存量请求

**Q4: Job 失败后如何自动重试？**

A: Job spec 配置：
- backoffLimit: 最大重试次数（默认 6）
- backoffDelay: 重试间隔（指数退避）
- activeDeadlineSeconds: 总超时时间
- restartPolicy: OnFailure（容器失败重启）或 Never（Pod 失败重建）

**Q5: OpenKruise 相比原生有什么优势？**

A: 
- 原地升级 (InPlace Update): 不重建 Pod，只更新镜像，更快
- 指定 Pod 删除: 精确控制删除哪个 Pod
- 金丝雀发布: partition 控制升级比例
- 并行扩缩: StatefulSet 并行创建/删除
- 镜像预热: ImagePullJob 提前拉取镜像

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| PDB | Pod Disruption Budget | Pod 中断预算 |
| QoS | Quality of Service | 服务质量等级 |
| CRI | Container Runtime Interface | 容器运行时接口 |
| HPA | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩 |
| VPA | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩 |
| FaaS | Function as a Service | 函数即服务 |

## 版本兼容性矩阵

| 特性 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| Sidecar Containers | Alpha | Beta | Beta | GA |
| User Namespaces | Alpha | Beta | Beta | Beta |
| Pod Scheduling Readiness | Beta | GA | GA | GA |
| OpenKruise | v1.5+ | v1.6+ | v1.7+ | v1.8+ |
| KEDA | v2.12+ | v2.13+ | v2.14+ | v2.15+ |

## 工作负载健康检查清单

| 检查项 | 说明 | 状态 |
|--------|------|------|
| 配置 readinessProbe | 确保流量只到就绪 Pod | ☐ |
| 配置 livenessProbe | 自动重启无响应容器 | ☐ |
| 设置资源 requests | 保证调度和 QoS | ☐ |
| 配置 PDB | 保护最小可用数 | ☐ |
| preStop Hook | 优雅停机，避免流量丢失 | ☐ |
| 镜像标签固定 | 避免 :latest，使用具体版本 | ☐ |
| 滚动更新策略 | maxUnavailable=0 保证可用性 | ☐ |
| 日志结构化 | JSON 格式，含 trace_id | ☐ |

