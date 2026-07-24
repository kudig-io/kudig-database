---
title: Pods
summary: Pods：Pod 是 Kubernetes 中最小的可部署单元，包含一个或多个容器。
category: concepts
tags:
- core-concept
- k8s
- workloads
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# Pods

## 概述

Pod 是 Kubernetes 中最小的可部署和调度单元。一个 Pod 包含一个或多个紧密耦合的容器，它们共享网络命名空间（同一 IP 和端口空间）、IPC、UTS 以及存储卷，并通过 loopback 互相通信。Pod 是一个"逻辑主机"，其内部的容器总是被一起调度、一起启停。Kubernetes 不会直接运行容器，而是将容器封装在 Pod 中运行。

## 架构与工作原理

```
┌─────────────────── Pod（10.244.1.5）───────────────────┐
│  ┌──────────┐   ┌──────────┐   ┌──────────────┐         │
│  │ 容器 A   │   │ 容器 B   │   │ Pause 容器   │         │
│  │ app:web  │   │ sidecar  │   │ 持有 netns   │         │
│  └────┬─────┘   └────┬─────┘   └──────────────┘         │
│       │              │                                   │
│       └──── 共享 ────┴─── 共享 Volume (volumes) ─────┐   │
│                                                        │   │
└────────────────────────────────────────────────────────┼───┘
                                                         │
                   同一网络命名空间（eth0 / lo）
```

**Pod 生命周期阶段（phase）**：
- `Pending`：已提交，尚未调度或镜像尚未拉取。
- `Running`：所有容器已创建，至少一个仍在运行。
- `Succeeded`：所有容器成功退出（不会再重启），典型如 Job。
- `Failed`：所有容器退出，至少一个失败。
- `Unknown`：通常因与节点失联。

**容器状态（ContainerStatus）**：`Waiting` / `Running` / `Terminated`，配合 restartPolicy（Always / OnFailure / Never）决定是否重启。

**Pod 状态机驱动方式**：kubelet 负责逐个启动 initContainers（严格顺序），全部成功后并行启动普通容器；容器退出后由 restartPolicy 决定是否重新创建。

## 关键组件与特性

| 组成 | 说明 |
|------|------|
| pause 容器 | 基础设施容器，持有 Pod 的网络命名空间 |
| initContainers | 应用容器之前按序执行，做初始化/依赖等待 |
| containers | 业务容器，并行运行 |
| volumes | emptyDir、configMap、secret、persistentVolumeClaim 等 |
| probes | livenessProbe / readinessProbe / startupProbe |
| restartPolicy | Always（默认）/ OnFailure / Never |
| terminationGracePeriodSeconds | 优雅停止宽限期，默认 30s |

## 配置示例

一个包含主容器、sidecar、init 容器、健康探针的完整 Pod：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: webapp
  namespace: production
  labels:
    app: webapp
    tier: frontend
spec:
  restartPolicy: Always
  terminationGracePeriodSeconds: 60
  initContainers:
  - name: init-db-check
    image: busybox:1.36
    command: ['sh', '-c', 'until nc -z db 5432; do sleep 2; done']
  containers:
  - name: webapp
    image: registry.example.com/webapp:v1.2.0
    ports:
    - containerPort: 8080
    env:
    - name: APP_ENV
      value: production
    resources:
      requests:
        cpu: 250m
        memory: 256Mi
      limits:
        cpu: "1"
        memory: 512Mi
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 15
      periodSeconds: 20
      failureThreshold: 3
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 10
    volumeMounts:
    - name: config
      mountPath: /etc/webapp
      readOnly: true
  - name: log-shipper
    image: fluentbit:2.2
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
  volumes:
  - name: config
    configMap:
      name: webapp-config
```

## 常用操作与命令

```bash
# 查看 / 排查
kubectl get pods -n production -o wide
kubectl describe pod webapp
kubectl logs webapp -c webapp --tail=200 -f
kubectl logs webapp --all-containers=true --previous

# 交互式调试
kubectl exec -it webapp -c webapp -- /bin/sh
kubectl debug -it webapp --image=busybox --target=webapp

# 临时端口转发
kubectl port-forward pod/webapp 8080:8080

# 强制删除卡在 Terminating 的 Pod
kubectl delete pod webapp --grace-period=0 --force

# 查看资源使用（需 metrics-server）
kubectl top pod webapp --containers
```

## 最佳实践

1. **务必设置 resources**：未设 requests 的 Pod 会被视为可压缩资源为 0，调度器无法正确评估。
2. **配置两类探针**：readinessProbe 控制是否接流量，livenessProbe 控制是否重启；启动慢的应用用 startupProbe 解耦。
3. **优雅停止**：捕获 SIGTERM，完成在途请求后再退出；长任务调大 `terminationGracePeriodSeconds`。
4. **不要"裸 Pod"**：生产环境用 Deployment/StatefulSet 管理，裸 Pod 不受控制器保护，节点故障不会重建。
5. **镜像 digest 优先**：使用 `image@sha256:...` 而非 `:latest`，避免不可变部署被悄悄替换。
6. **配置通过 ConfigMap/Secret 注入**：不要将配置打进镜像。

## 常见陷阱

- **CrashLoopBackOff**：容器反复崩溃，看 `kubectl logs --previous` 找根因（启动命令错误、依赖未就绪、OOM）。
- **OOMKilled**：内存 limit 太低，`kubectl describe` 的 Last State 会显示 Reason: OOMKilled。
- **ImagePullBackOff**：检查 imagePullSecrets、镜像名称、仓库网络。
- **多容器争抢 CPU**：未设 requests 时，sidecar 可能抢占主容器 CPU；用 cpuset / requests 隔离。
- **共享卷写冲突**：多个容器同时写同一个 emptyDir 可能数据损坏，约定单一写入者。

## 源码实现分析

### kubelet Pod 生命周期管理

```go
// k8s.io/kubernetes/pkg/kubelet/kuberuntime/kuberuntime_manager.go
// kubelet 通过 CRI 管理 Pod 生命周期
func (m *kubeGenericRuntimeManager) SyncPod(ctx context.Context, pod *v1.Pod, podStatus *kubecontainer.PodStatus) {
    // 1. 创建 Pod 沙箱（pause 容器 + 网络命名空间）
    sandboxID := m.createPodSandbox(ctx, pod)
    
    // 2. 执行 Init Containers（顺序）
    for _, initContainer := range pod.Spec.InitContainers {
        m.startContainer(ctx, initContainer, sandboxID)
        m.waitForExit(initContainer)  // 等待完成
    }
    
    // 3. 启动主容器
    for _, container := range pod.Spec.Containers {
        m.startContainer(ctx, container, sandboxID)
    }
    
    // 4. 启动探针监控
    m.startProbers(pod)  // liveness + readiness + startup
}

// Pod 终止流程
func (m *kubeGenericRuntimeManager) killPod(ctx context.Context, pod *v1.Pod) {
    // 1. 发送 SIGTERM
    m.killContainer(container, gracePeriod)
    // 2. 等待 terminationGracePeriodSeconds
    // 3. 超时后发送 SIGKILL
    // 4. 清理沙箱和网络
}
```

### Pod 生命周期状态机

```
┌───────────────────────────────────────────────────────────┐
│          Pod 生命周期状态机                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  Pending → Running → Succeeded/Failed                    │
│    │          │                                           │
│    │          ├─ 容器启动中 (Waiting)                  │
│    │          ├─ 容器运行中 (Running)                  │
│    │          └─ 容器终止 (Terminated)                 │
│    │                                                      │
│    ├─ 调度中: 等待节点分配                           │
│    ├─ 拉取镜像: ImagePullBackOff 可能卡住            │
│    └─ Init 执行: Init:0/2, Init:CrashLoopBackOff    │
│                                                           │
│  探针机制:                                               │
│  • startupProbe: 启动期间检测，失败则重启           │
│  • livenessProbe: 运行期检测，失败则重启            │
│  • readinessProbe: 控制是否接收流量                  │
│                                                           │
│  优雅终止:                                               │
│  1. Pod 标记为 Terminating                              │
│  2. 从 Endpoints 移除（停止接收新流量）            │
│  3. 执行 preStop hook                                   │
│  4. 发送 SIGTERM                                        │
│  5. 等待 terminationGracePeriodSeconds (默认 30s)  │
│  6. 发送 SIGKILL                                        │
└───────────────────────────────────────────────────────────┘
```

### 生产 Pod 配置示例（🟡 部署到集群）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: production-app
spec:
  terminationGracePeriodSeconds: 60  # 优雅停止时间
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
  containers:
  - name: app
    image: my-app@sha256:abc123  # 使用 digest
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi
    startupProbe:
      httpGet:
        path: /health
        port: 8080
      failureThreshold: 30
      periodSeconds: 2
    livenessProbe:
      httpGet:
        path: /health
        port: 8080
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      periodSeconds: 5
    lifecycle:
      preStop:
        exec:
          command: ["sh", "-c", "sleep 5"]  # 等待流量排干
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
```

## 面试要点

1. **Pod 的生命周期状态有哪些？**
   - Pending：调度中/拉取镜像中
   - Running：至少一个容器在运行
   - Succeeded/Failed：所有容器终止
   - Unknown：状态无法获取

2. **三种探针的作用和区别？**
   - startupProbe：启动期间检测，失败则重启
   - livenessProbe：运行期检测，失败则重启
   - readinessProbe：控制是否接收流量，失败则从 Endpoints 移除

3. **Pod 优雅终止的流程？**
   - 标记 Terminating → 移除 Endpoints → preStop hook
   - SIGTERM → 等待 grace period → SIGKILL
   - 关键：preStop sleep 等待流量排干

4. **为什么生产环境不用裸 Pod？**
   - 裸 Pod 不受控制器保护，节点故障不会重建
   - 应用 Deployment/StatefulSet/DaemonSet 管理
   - 控制器提供自愈、滚动更新、扩缩容

## 相关概念

- [[概念/kubernetes.md|Kubernetes]] — 核心平台
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[概念/init-containers.md|Init Containers]]
- [[概念/sidecar-containers.md|Sidecar Containers]]
- [[概念/deployments.md|Deployment]] — Pod 的上层管理器
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
