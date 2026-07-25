---
title: Pod 生命周期事件表
description: 深入解析 Pod 生命周期状态转换、Phase 与 Condition 机制、Init Container、Sidecar、容器重启策略与事件体系
summary: 深入解析 Pod 生命周期状态转换、Phase 与 Condition 机制、Init Container、Sidecar、容器重启策略与事件体系
category: 工作负载
tags:
- k8s
- pod
- lifecycle
- phase
- condition
- init-container
- sidecar
- container
- pdb
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Pod 生命周期事件表 是什么
- 如何 Pod 生命周期事件表
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- Pod
- 生命周期事件表
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/pod-fta.md
  label: '故障树: pod'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
related_docs:
- path: 10-workload-controllers-overview.md
  type: depth
  desc: 工作负载控制器详解
- path: ../工作负载/21-hpa-vpa-autoscaling.md
  type: depth
  desc: HPA/VPA 自动扩缩容
- path: ../故障诊断/FTA故障树/list/pod-fta.md
  type: fta
  desc: Pod 故障树
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 37 - Pod生命周期事件表

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].io/docs/concepts/workloads/pods/pod-lifecycle](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/)

<!-- chunk: Pod阶段(Phase) -->
## Pod阶段(Phase)

| 阶段 | 描述 | 触发条件 | 正常/异常 | 后续状态 |
|-----|------|---------|----------|---------|
| **Pending** | 已创建，等待调度或镜像拉取 | 创建Pod | 正常(短暂) | Running/Failed |
| **Running** | 至少一个容器运行中 | 容器启动成功 | 正常 | Succeeded/Failed |
| **Succeeded** | 所有容器成功终止 | Job完成 | 正常(Job) | 终态 |
| **Failed** | 所有容器终止，至少一个失败 | 容器退出非0 | 异常 | 终态 |
| **Unknown** | 无法获取Pod状态 | 节点通信失败 | 异常 | 取决于恢复 |

<!-- chunk: 容器状态 -->
## 容器状态

| 状态 | 描述 | Reason字段 | 常见原因 |
|-----|------|-----------|---------|
| **Waiting** | 等待启动 | ContainerCreating | 拉取镜像 |
| | | ImagePullBackOff | 镜像拉取失败 |
| | | CrashLoopBackOff | 容器反复崩溃 |
| | | CreateContainerError | 容器创建失败 |
| | | ErrImagePull | 镜像拉取错误 |
| **Running** | 运行中 | - | 正常运行 |
| **Terminated** | 已终止 | Completed | 正常完成 |
| | | Error | 异常退出 |
| | | OOMKilled | 内存超限 |
| | | ContainerStatusUnknown | 状态未知 |

<!-- chunk: Pod Condition -->
## Pod Condition

| Condition | True含义 | False含义 | 检查命令 |
|----------|---------|----------|---------|
| **PodScheduled** | 已调度到节点 | 调度中/失败 | `kubectl describe pod` |
| **Initialized** | Init容器完成 | Init未完成 | 检查init容器 |
| **ContainersReady** | 所有容器就绪 | 有容器未就绪 | 检查就绪探针 |
| **Ready** | Pod就绪，可接收流量 | Pod未就绪 | 检查所有条件 |
| **DisruptionTarget** | 将被驱逐 | 正常 | v1.25+ |

<!-- chunk: 常见事件及处理 -->
## 常见事件及处理

| 事件类型 | Event Reason | 描述 | 排查命令 | 解决方案 |
|---------|-------------|------|---------|---------|
| **调度** | FailedScheduling | 无法调度 | `kubectl describe pod` | 检查资源/污点/亲和性 |
| | Scheduled | 调度成功 | - | 正常 |
| **镜像** | Pulling | 拉取镜像中 | - | 等待 |
| | Pulled | 镜像拉取完成 | - | 正常 |
| | Failed | 拉取失败 | 检查镜像名和凭证 | 修复镜像配置 |
| **容器** | Created | 容器创建 | - | 正常 |
| | Started | 容器启动 | - | 正常 |
| | Killing | 容器终止中 | - | 正常 |
| | BackOff | 重启退避 | `kubectl logs --previous` | 修复应用问题 |
| **探针** | Unhealthy | 探针失败 | `kubectl describe pod` | 调整探针参数 |
| **资源** | FailedMount | 挂载失败 | 检查PV/Secret | 修复存储配置 |
| | NodeNotReady | 节点NotReady | `kubectl get nodes` | 修复节点 |
| **驱逐** | Evicted | Pod被驱逐 | `kubectl describe pod` | 调整资源/节点 |

<!-- chunk: 事件查看命令 -->
## 事件查看命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看Pod事件
kubectl describe pod <pod-name>

# 使用events命令(v1.26+)
kubectl events --for pod/<pod-name>

# 查看所有事件
kubectl get events --sort-by='.lastTimestamp'

# 过滤Warning事件
kubectl get events --field-selector=type=Warning

# 查看特定命名空间事件
kubectl get events -n <namespace> --sort-by='.lastTimestamp'

# 持续监控事件
kubectl get events -w
```
<!-- chunk: Pod重启原因分析 -->
## Pod重启原因分析

| 重启原因 | 诊断方法 | 常见原因 | 解决方案 |
|---------|---------|---------|---------|
| **OOMKilled** | `kubectl describe pod` | 内存超限 | 增加memory limits |
| **应用崩溃** | `kubectl logs --previous` | 应用bug | 修复应用 |
| **存活探针失败** | `kubectl describe pod` | 探针配置错误 | 调整探针参数 |
| **节点重启** | 节点事件 | 节点维护/问题 | 配置PDB |
| **抢占** | 事件 | 高优先级Pod抢占 | 调整优先级 |

<!-- chunk: 探针配置最佳实践 -->
## 探针配置最佳实践

| 探针类型 | 用途 | 默认值 | 推荐配置 |
|---------|------|-------|---------|
| **livenessProbe** | 容器是否存活 | 无 | 应用真正死锁时才失败 |
| **readinessProbe** | 是否就绪接收流量 | 无 | 应用可处理请求时成功 |
| **startupProbe** | 启动完成检查 | 无 | 慢启动应用使用 |

```yaml
# 探针配置示例
spec:
  containers:
  - name: app
    image: app:latest
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
      timeoutSeconds: 5
      failureThreshold: 3
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 3
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 0
      periodSeconds: 10
      timeoutSeconds: 5
      failureThreshold: 30  # 最多等待5分钟启动
```

<!-- chunk: Pod终止流程 -->
## Pod终止流程

| 步骤 | 操作 | 超时 | 配置 |
|-----|------|------|------|
| 1 | Pod标记为Terminating | - | - |
| 2 | 从Service Endpoints移除 | - | - |
| 3 | 执行preStop钩子 | terminationGracePeriodSeconds | lifecycle.preStop |
| 4 | 发送SIGTERM | - | - |
| 5 | 等待优雅终止 | terminationGracePeriodSeconds | spec.terminationGracePeriodSeconds |
| 6 | 发送SIGKILL | - | - |

```yaml
# 优雅终止配置
spec:
  terminationGracePeriodSeconds: 60  # 默认30s
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["/bin/sh", "-c", "sleep 10 && kill -SIGTERM 1"]

```

<!-- chunk: Pod驱逐场景 -->
## Pod驱逐场景

| 驱逐类型 | 触发条件 | 版本变更 | 预防措施 |
|---------|---------|---------|---------|
| **资源压力驱逐** | 内存/磁盘/PID压力 | v1.26增强 | 设置资源请求 |
| **节点维护** | kubectl drain | 稳定 | 配置PDB |
| **抢占驱逐** | 高优先级Pod抢占 | 稳定 | 使用适当优先级 |
| **污点驱逐** | 节点污点变化 | 稳定 | 配置容忍 |
| **API驱逐** | Eviction API调用 | 稳定 | 配置PDB |

<!-- chunk: PDB配置 -->
## PDB配置

```yaml
# 保证最少可用数
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb
spec:
  minAvailable: 2  # 或使用百分比: "50%"
  selector:
    matchLabels:
      app: myapp
---
# 限制最大不可用数
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: app-pdb-max
spec:
  maxUnavailable: 1  # 或 "25%"
  selector:
    matchLabels:
      app: myapp
```

<!-- chunk: Pod状态监控 -->
## Pod状态监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 监控Pod状态变化
kubectl get pods -w

# 检查重启次数高的Pod
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{range .status.containerStatuses[*]}{.restartCount}{"\t"}{end}{"\n"}{end}' | awk '$3>5'

# 检查OOMKilled的Pod
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{range .status.containerStatuses[*]}{.lastState.terminated.reason}{"\t"}{end}{"\n"}{end}' | grep OOMKilled
```

<!-- chunk: Pod生命周期异常诊断实战 -->
## Pod生命周期异常诊断实战

### 诊断流程（五步法）

```
Step 1: 确认 Pod Phase
  kubectl get pod <pod> -o jsonpath='{.status.phase}'
  └── Pending → Step 2
  └── Running 但 Not Ready → Step 3
  └── Failed → Step 4
  └── Unknown → Step 5

Step 2: Pending 诊断
  kubectl describe pod <pod> | grep -A 20 "Events"
  └── FailedScheduling → 检查资源/污点/亲和性
  └── FailedMount → 检查 PVC/Secret/ConfigMap
  └── 长时间 ContainerCreating → 检查镜像拉取/CNI

Step 3: Running 但 Not Ready
  kubectl get pod <pod> -o jsonpath='{.status.conditions[?(@.type=="Ready")]}'
  └── readinessProbe 失败 → 检查探针配置与应用健康端点
  └── 容器重启中 → kubectl logs --previous
  └── Init 容器未完成 → kubectl logs -c <init-container>

Step 4: Failed 诊断
  kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[0].state.terminated}'
  └── Exit Code 1 → 应用错误
  └── Exit Code 137 → OOMKilled/SIGKILL
  └── Exit Code 143 → SIGTERM（正常终止）
  └── Reason: DeadlineExceeded → Job 超时

Step 5: Unknown 诊断
  kubectl get node <node> -o jsonpath='{.status.conditions}'
  └── 节点 NotReady → 检查 kubelet/网络
  └── 节点已删除 → Pod 残留，手动清理
```

### 异常状态快速修复表

| 异常状态 | 快速诊断 | 修复操作 | 风险等级 |
|---------|---------|---------|----------|
| Pending > 5min | `kubectl describe pod` 查看事件 | 调整资源/污点/PVC | 🟢 |
| ImagePullBackOff | `crictl pull <image>` 测试 | 修复镜像名/凭证/网络 | 🟢 |
| CrashLoopBackOff | `kubectl logs --previous` | 修复应用/探针配置 | 🟡 |
| OOMKilled | `kubectl describe pod` 查看 lastState | 增大 memory limit | 🟡 |
| Evicted | `kubectl describe pod` 查看原因 | 调整资源请求/节点容量 | 🟡 |
| Terminating 卡住 | `kubectl get pod -o yaml` 查看 finalizers | 移除 finalizer/强制删除 | 🔴 |
| Unknown | `kubectl get nodes` 检查节点 | 修复节点/清理残留 Pod | 🟡 |

### 强制删除卡住的 Pod

```bash
# 🔴 高风险：强制删除可能导致数据不一致，仅在 Pod 确认无法恢复时使用
# 先尝试正常删除
kubectl delete pod <pod> -n <ns> --grace-period=30

# 若超过 grace period 仍为 Terminating，强制删除
kubectl delete pod <pod> -n <ns> --grace-period=0 --force

# 若因 finalizer 卡住，移除 finalizer
kubectl patch pod <pod> -n <ns> -p '{"metadata":{"finalizers":null}}'
```

<!-- chunk: 容器重启自动化治理 -->
## 容器重启自动化治理

### 重启原因分类与响应

| 重启原因 | 自动响应 | 人工介入条件 | 预防措施 |
|---------|---------|-------------|----------|
| OOMKilled | 自动重启 + 告警 | 连续 3 次 OOM | 调整 memory limit、修复内存泄漏 |
| 应用崩溃 (Exit 1) | 自动重启 + 日志采集 | 连续 5 次崩溃 | 修复代码、添加错误处理 |
| 探针失败 | 自动重启 | 重启后仍失败 | 调整探针参数、检查依赖 |
| 节点故障 | 重新调度 | 多节点同时故障 | 配置 PDB、跨 AZ 部署 |
| 抢占 | 重新调度 | 频繁被抢占 | 提高优先级、增加资源 |

### 重启次数监控告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-lifecycle-alerts
  namespace: monitoring
spec:
  groups:
    - name: pod.lifecycle.rules
      rules:
        - alert: PodCrashLooping
          expr: |
            rate(kube_pod_container_status_restarts_total{job="kube-state-metrics"}[15m]) * 60 * 15 > 3
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 频繁重启"
            description: "15分钟内重启超过 3 次，当前重启总数: {{ $value }}"

        - alert: PodOOMKilled
          expr: |
            kube_pod_container_status_last_terminated_reason{reason="OOMKilled"} == 1
          for: 0m
          labels:
            severity: critical
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 被 OOMKilled"
            description: "容器 {{ $labels.container }} 因内存超限被终止"

        - alert: PodPendingTooLong
          expr: |
            kube_pod_status_phase{phase="Pending"} == 1
            and on(pod, namespace)
            (time() - kube_pod_created) > 300
          for: 1m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Pending 超过 5 分钟"

        - alert: PodTerminatingStuck
          expr: |
            kube_pod_status_phase{phase="Running"} == 1
            and on(pod, namespace)
            kube_pod_deletion_timestamp > 0
            and on(pod, namespace)
            (time() - kube_pod_deletion_timestamp) > 120
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} Terminating 卡住超过 2 分钟"

        - alert: ContainerWaitingHigh
          expr: |
            kube_pod_container_status_waiting_reason{reason!="ContainerCreating"} == 1
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "容器 {{ $labels.container }} 处于 {{ $labels.reason }} 状态超过 10 分钟"
```

### 自动修复 CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: pod-health-remediation
  namespace: kube-system
spec:
  schedule: "*/5 * * * *"
  concurrencyPolicy: Forbid
  successfulJobsHistoryLimit: 3
  failedJobsHistoryLimit: 3
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: pod-remediator
          restartPolicy: OnFailure
          containers:
            - name: remediator
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  echo "=== Pod 健康巡检 $(date) ==="
                  
                  # 1. 清理 Evicted Pod
                  EVICTED=$(kubectl get pods -A --field-selector=status.phase=Failed \
                    -o jsonpath='{range .items[?(@.status.reason=="Evicted")]}{.metadata.namespace} {.metadata.name}{"\n"}{end}')
                  if [ -n "$EVICTED" ]; then
                    echo "清理 Evicted Pod:"
                    echo "$EVICTED" | while read ns name; do
                      kubectl delete pod "$name" -n "$ns" --grace-period=0
                      echo "  已删除: $ns/$name"
                    done
                  fi
                  
                  # 2. 清理 Succeeded 的 Job Pod（超过 1 小时）
                  kubectl get pods -A --field-selector=status.phase=Succeeded \
                    -o json | jq -r '.items[] | select((now - (.metadata.creationTimestamp | fromdateiso8601)) > 3600) | "\(.metadata.namespace) \(.metadata.name)"' | \
                  while read ns name; do
                    kubectl delete pod "$name" -n "$ns"
                    echo "  清理已完成 Pod: $ns/$name"
                  done
                  
                  # 3. 报告高重启次数 Pod
                  echo "=== 高重启 Pod 报告 ==="
                  kubectl get pods -A -o json | jq -r '
                    .items[] |
                    select(.status.containerStatuses != null) |
                    select(.status.containerStatuses[].restartCount > 10) |
                    "\(.metadata.namespace)/\(.metadata.name) restarts=\(.status.containerStatuses[].restartCount)"'
                  
                  echo "=== 巡检完成 ==="
```

<!-- chunk: 生命周期钩子高级用法 -->
## 生命周期钩子高级用法

### postStart 与 preStop 对比

| 特性 | postStart | preStop |
|-----|-----------|----------|
| 触发时机 | 容器创建后 | 容器终止前 |
| 与 ENTRYPOINT 关系 | 并行执行，不保证顺序 | 在 SIGTERM 之前执行 |
| 失败影响 | 容器被杀死 | 继续终止流程 |
| 典型用途 | 注册服务、预热缓存 | 注销服务、排空连接 |
| 超时 | 无独立超时 | 计入 terminationGracePeriodSeconds |

### 生产级 preStop 配置

```yaml
# 场景 1: HTTP 服务优雅下线
spec:
  terminationGracePeriodSeconds: 60
  containers:
    - name: api-server
      lifecycle:
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 1. 通知负载均衡器停止发送新流量
                curl -s -X POST http://localhost:8080/admin/drain || true
                # 2. 等待现有请求完成（最多 30 秒）
                sleep 15
                # 3. 等待 Service Endpoints 更新传播
                sleep 5
---
# 场景 2: 消息队列消费者优雅下线
spec:
  terminationGracePeriodSeconds: 120
  containers:
    - name: consumer
      lifecycle:
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 1. 停止消费新消息
                curl -s -X POST http://localhost:9090/consumer/pause || true
                # 2. 等待当前消息处理完成（最多 90 秒）
                timeout 90 sh -c 'while curl -s http://localhost:9090/consumer/active | grep -q "processing"; do sleep 2; done'
                echo "Consumer drained"
---
# 场景 3: 数据库连接池优雅关闭
spec:
  terminationGracePeriodSeconds: 45
  containers:
    - name: app
      lifecycle:
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 1. 标记为不健康，触发 Readiness 失败
                touch /tmp/shutting-down
                # 2. 等待 Endpoints 移除（kubelet 同步周期）
                sleep 10
                # 3. 等待活跃事务完成
                sleep 20
```

### postStart 初始化示例

```yaml
spec:
  containers:
    - name: app
      lifecycle:
        postStart:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 预热连接池
                curl -s http://localhost:8080/warmup || true
                # 注册到服务发现
                curl -s -X POST http://service-registry:8500/v1/agent/service/register \
                  -d '{"Name":"app","Port":8080}' || true
```

> **注意**: postStart 与容器 ENTRYPOINT 并行执行，不保证先后顺序。若初始化必须在应用启动前完成，应使用 Init Container。

<!-- chunk: Pod状态转换监控 -->
## Pod状态转换监控

### 状态转换时序图

```
创建 Pod
   │
   ▼
Pending ─────────────────────────────────────┐
   │                                          │
   ├─ Scheduled (PodScheduled=True)           │
   ├─ Init Containers 运行                    │
   ├─ Initialized=True                        │
   ├─ 拉取镜像 (Pulling → Pulled)              │
   ├─ 创建容器 (Created)                       │
   ├─ 启动容器 (Started)                       │
   │                                          │
   ▼                                          │
Running ─────────────────────────────────────┤
   │                                          │
   ├─ Readiness 探针通过                       │
   ├─ Ready=True                              │
   ├─ 加入 Service Endpoints                  │
   ├─ 接收流量                                │
   │                                          │
   ├─── 正常终止 ────────────────────────────┤
   │    ├─ preStop 钩子                       │
   │    ├─ SIGTERM                            │
   │    ├─ 等待 grace period                  │
   │    ├─ SIGKILL (若超时)                    │
   │    └─ Succeeded/Failed                   │
   │                                          │
   └─── 异常终止 ────────────────────────────┘
        ├─ OOMKilled (Exit 137)
        ├─ 应用崩溃 (Exit 1)
        ├─ 探针失败 → 重启
        └─ 节点故障 → 重新调度
```

### 关键状态转换指标

| 指标 | 含义 | 正常范围 | 异常信号 |
|-----|------|---------|----------|
| Pending → Running 时间 | 调度+拉取+启动总耗时 | < 30s | > 60s 需排查 |
| Running → Ready 时间 | 应用启动+探针通过 | < 10s | > 30s 需优化 |
| Terminating → 删除 时间 | 优雅终止耗时 | < grace period | 接近/超过 grace period |
| 重启间隔 | 两次重启之间的时间 | 稳定增长(退避) | 固定短间隔=持续崩溃 |

<!-- chunk: 优雅终止生产配置 -->
## 优雅终止生产配置

### 不同工作负载类型的终止策略

| 工作负载类型 | 推荐 grace period | preStop 策略 | 注意事项 |
|-------------|------------------|-------------|----------|
| 无状态 API | 30-60s | sleep 10-15s + drain | 等待 Endpoints 传播 |
| WebSocket 服务 | 120-300s | 通知客户端重连 | 长连接需特殊处理 |
| 消息消费者 | 60-120s | 暂停消费 + 等待处理完 | 避免消息丢失 |
| 批处理 Job | 300-600s | 保存检查点 | 支持断点续传 |
| 数据库 | 300-600s | 刷盘 + 关闭连接 | StatefulSet 顺序终止 |
| 缓存服务 | 60s | 持久化热数据 | 避免缓存雪崩 |

### 完整优雅终止配置模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: production-api
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    spec:
      terminationGracePeriodSeconds: 60
      containers:
        - name: api
          image: myregistry/api:v1.2.3
          ports:
            - containerPort: 8080
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - |
                    # 阶段 1: 停止接收新流量
                    echo "$(date) - Starting graceful shutdown" >> /var/log/shutdown.log
                    # 标记为不健康（触发 Readiness 失败）
                    touch /tmp/shutting-down
                    # 等待 kubelet 更新 Endpoints
                    sleep 10
                    # 阶段 2: 排空现有连接
                    # 发送 SIGUSR1 触发应用内部优雅关闭
                    kill -USR1 1 || true
                    # 等待活跃请求完成
                    sleep 20
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8080
            initialDelaySeconds: 30
            periodSeconds: 10
            failureThreshold: 3
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 5
            failureThreshold: 3
            # 关键: 检查 shutting-down 标记
            # 应用应在 /ready 中检查 /tmp/shutting-down 是否存在
          startupProbe:
            httpGet:
              path: /healthz
              port: 8080
            failureThreshold: 30
            periodSeconds: 10
```

### 终止流程验证脚本

```bash
#!/bin/bash
# 🟢 低风险：验证优雅终止配置是否正确
set -euo pipefail

POD=${1:?"Usage: $0 <pod-name> [namespace]"}
NS=${2:-default}

echo "=== 优雅终止配置验证: ${NS}/${POD} ==="

# 1. 检查 terminationGracePeriodSeconds
GRACE=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.terminationGracePeriodSeconds}')
echo "✓ terminationGracePeriodSeconds: ${GRACE:-30(default)}"

# 2. 检查 preStop 钩子
PRESTOP=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.containers[0].lifecycle.preStop.exec.command}')
if [ -n "$PRESTOP" ]; then
  echo "✓ preStop 钩子已配置: $PRESTOP"
else
  echo "⚠️ 未配置 preStop 钩子"
fi

# 3. 检查 Readiness 探针
READINESS=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.containers[0].readinessProbe.httpGet.path}')
echo "✓ Readiness 探针路径: ${READINESS:-未配置}"

# 4. 检查 PDB
PDB=$(kubectl get pdb -n "$NS" -o jsonpath='{range .items[?(@.spec.selector.matchLabels.app)]}{.metadata.name}{end}')
echo "✓ PDB: ${PDB:-未配置}"

# 5. 检查 Pod 优先级
PRIORITY=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.priorityClassName}')
echo "✓ PriorityClass: ${PRIORITY:-default}"

echo "=== 验证完成 ==="
```

<!-- chunk: Pod生命周期检查清单 -->
## Pod生命周期检查清单

### 生产环境必检项

| 序号 | 检查项 | 验证方法 | 通过标准 |
|-----|--------|---------|----------|
| 1 | 三种探针均已配置 | `kubectl get pod -o yaml` | liveness + readiness + startup |
| 2 | startupProbe 给足启动时间 | 检查 failureThreshold × periodSeconds | ≥ 应用最大启动时间 |
| 3 | terminationGracePeriodSeconds 合理 | 检查配置值 | ≥ preStop 时间 + 排空时间 |
| 4 | preStop 钩子已配置 | 检查 lifecycle.preStop | 有 sleep 或 drain 逻辑 |
| 5 | PDB 已配置 | `kubectl get pdb` | minAvailable 或 maxUnavailable |
| 6 | 重启策略正确 | 检查 restartPolicy | Always(长期服务)/OnFailure(Job) |
| 7 | 资源请求与限制已设置 | 检查 resources | requests ≤ 实际使用 ≤ limits |
| 8 | 优先级已设置 | 检查 priorityClassName | 关键服务有明确优先级 |
| 9 | 拓扑分布已配置 | 检查 topologySpreadConstraints | 跨 AZ/节点分布 |
| 10 | 监控告警已覆盖 | 检查 PrometheusRule | 重启/OOM/Pending 告警 |

### 常见配置错误与修复

| 错误配置 | 后果 | 正确做法 |
|---------|------|----------|
| liveness 检查依赖服务 | 依赖故障导致级联重启 | liveness 只检查自身存活 |
| readiness 与 liveness 相同 | 无法区分"未就绪"和"已死亡" | readiness 检查依赖，liveness 检查自身 |
| 未配置 startupProbe | 慢启动应用被 liveness 杀死 | 配置 startupProbe，liveness 在其后生效 |
| grace period 过短 | 请求被截断、数据丢失 | 根据业务特点设置足够时间 |
| preStop 中无 sleep | Endpoints 未更新即终止 | sleep 10-15s 等待 Endpoints 传播 |
| 未配置 PDB | 节点维护时全部 Pod 同时终止 | 配置 minAvailable 或 maxUnavailable |

---

**生命周期原则**: 正确配置探针，设置PDB，处理优雅终止

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 工作负载 KUDIG Database — Global MOC
- [[02-工作负载/README.md|Domain-4: Kubernetes工作负载管理]]
- index.md|Domain-4 工作负载 — 开源项目索引]]
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - [[02-工作负载/01-核心工作负载/03-statefulset-advanced-operations.md|03 statefulset advanced operations]]
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## Related

- [[deployment]]
- [[22-概念/11-交叉分析/Pod 生命周期 × 存储模型.md|Pod 生命周期 × 存储模型]]

- 工作负载控制器详解
- HPA/VPA 自动扩缩容
- 相关知识域: 集群基础
- 相关知识域: 可观测性
- [[17-系统基础/05-速查卡/k8s.md|速查卡: k8s]]
- [[21-生态参考/03-领域索引/pod-index.md|Pod 知识图谱索引]]

## See Also

- 09-edge-computing-deployment
- 10-workload-controllers-overview
- 12-advanced-pod-patterns
- 13-container-lifecycle-hooks

```

<!-- risk-assessed -->
