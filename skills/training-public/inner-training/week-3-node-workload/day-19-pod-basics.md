---
title: 'Day 19: Pod 容器组基础'
description: '## 概述'
summary: '## 概述'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- scheduler
- istio
- envoy
- mysql
- job
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 19: Pod 容器组基础 是什么'
- '如何 Day 19: Pod 容器组基础'
trigger_keywords:
- Day
- '19:'
- Pod
- 容器组基础
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- service-mesh-basics
- mysql-basics
- gpu-scheduling-basics
---



---
title: Day 19: Pod 容器组基础
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|Kubernetes]] Pod lifecycle Pending Running Succeeded Failed
  - Pod container debugging logs exec
  - Kubernetes Sidecar multi-container pattern
  - Init Container initialization
  - Pod restartPolicy configuration
trigger_keywords:
  - Pod
  - lifecycle
  - Pending
  - Running
  - Sidecar
  - Init Container
  - restartPolicy
  - logs
  - exec
  - container
reading_level: intermediate
audience:
  - ACK operators
  - Developers
  - SRE engineers
estimated_read_time: 45min
related_domains:
  - domain-9-workload
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - pod-overview
  - pod-lifecycle
  - pod-troubleshooting
---

# Day 19: Pod 容器组基础

> **学习时间**: 4-5 小时 | **主题**: Pod 生命周期与基本操作

---

## 概述

本文深入讲解 Kubernetes 中最核心的概念——Pod。Pod 是 K8s 调度的最小单元，理解 Pod 的生命周期、多容器模式、健康检查和基本操作是所有 K8s 运维工作的基础。通过本文的学习，你将掌握 Pod 的创建、查看、调试和删除操作，以及 Sidecar 多容器模式和 Init Container 的使用场景。

### 学习目标

- 理解 Pod 在 K8S 集群中的核心地位和设计哲学
- 掌握 Pod 生命周期各阶段（Pending → Running → Succeeded/Failed）
- 能够创建、查看、删除 Pod 并排查基本问题
- 理解多容器 Pod（Sidecar）模式和 Init Container 的使用场景
- 掌握 Pod 日志查看、容器调试和端口转发等运维操作

---

## 核心概念详解

### Pod 的设计哲学

Pod 是 Kubernetes 中最小的可部署和调度单元。为什么 K8s 不直接调度容器？因为容器只提供单一进程的隔离，而实际应用常常需要多个紧密协作的进程共同工作。Pod 将一个或多个容器包装在一起，共享网络命名空间（同一个 IP、端口空间）、IPC 命名空间（可以通过 System V IPC 通信）和存储卷。同一个 Pod 中的容器就像运行在同一台机器上的多个进程一样，可以直接通过 localhost 互相访问。

Pod 的设计遵循以下原则：

- **原子性**: Pod 中的所有容器作为一个整体被调度、创建和销毁。如果一个容器失败，整个 Pod 可能被重启（取决于 restartPolicy）
- **共享网络**: Pod 中的所有容器共享同一个网络命名空间，使用同一个 IP 地址，通过 localhost 互相访问不同端口
- **共享存储**: Pod 中的容器可以通过共享的 Volume 来交换数据
- **短暂性**: Pod 本身是临时的，随时可能被驱逐、重建或迁移。有状态应该存储在 PersistentVolume 中

### Pod 生命周期详解

Pod 的生命周期包含以下几个阶段（Phase）：

- **Pending**: Pod 已被 API Server 接受，但尚未被调度到节点，或者正在拉取镜像。此时 Pod 还没有任何容器在运行
- **Running**: Pod 已被调度到节点，且至少一个容器正在运行或正在启动
- **Succeeded**: Pod 中的所有容器都已成功退出（exit code 0），且不会重启。适用于一次性任务（Job）
- **Failed**: Pod 中的所有容器都已退出，且至少一个容器以非零退出码退出
- **Unknown**: 无法获取 Pod 状态，通常是节点失联（kubelet 未在规定时间内上报状态）

每个容器还有自己的状态（Container State）：

- **Waiting**: 容器正在等待某个条件满足（如拉取镜像、获取 Secret）
- **Running**: 容器正在运行，包含了启动时间
- **Terminated**: 容器已退出，包含了退出码、退出原因和退出时间

### restartPolicy 重启策略

- **Always**（默认）: 容器退出后总是重启。适用于长期运行的服务（Web 服务、API 服务等）
- **OnFailure**: 容器以非零退出码退出时才重启。适用于批处理任务
- **Never**: 容器退出后不重启。适用于一次性任务

重启机制使用指数退避策略：第一次立即重启，第二次延迟 10 秒，第三次延迟 20 秒...最大延迟 5 分钟。如果容器运行超过 10 分钟没有崩溃，退避计时器重置。

### 多容器模式

**Sidecar 模式** 是最常用的多容器模式。主容器运行应用逻辑，Sidecar 容器提供辅助功能（如日志采集、代理、监控）。Sidecar 与主容器共享网络和存储，可以通过 localhost 通信和通过 Volume 交换数据。

常见的 Sidecar 场景：
- 日志采集：主容器写日志到共享 Volume，Sidecar 读取并上传到日志系统
- 代理/网络：[[envoy|Envoy]]/Istio Sidecar 拦截所有进出流量，实现服务网格功能
- 配置更新：Sidecar 监听配置变化并通知主容器重新加载

**Init Container** 在主容器启动前按顺序执行，每个 Init Container 必须成功完成后下一个才能启动。Init Container 的应用场景包括：等待依赖服务就绪、初始化数据库、下载配置文件、注册服务发现等。与普通容器不同，Init Container 不支持 livenessProbe/readinessProbe。

### 容器探针（Probes）

K8s 支持三种探针来检查容器的健康状态：

- **livenessProbe（存活探针）**: 检测容器是否仍在正常运行。如果检测失败，K8s 会重启容器。适用于检测死锁等需要重启才能恢复的场景
- **readinessProbe（就绪探针）**: 检测容器是否准备好接收流量。如果检测失败，Pod 会从 Service 的 Endpoints 中移除。适用于应用启动慢或依赖外部服务的场景
- **startupProbe（启动探针）**: 检测容器是否已完成初始化。在 startupProbe 成功之前，其他探针不会执行。适用于启动时间很长的应用

探针类型包括：httpGet（HTTP 请求）、tcpSocket（TCP 连接）、exec（执行命令）、grpc（gRPC 健康检查）。

---

## 实战演练

### 任务 1: Pod 创建与基本操作 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 创建带资源限制和探针的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: nginx-demo
  labels:
    app: nginx
    env: training
spec:
  containers:
  - name: nginx
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    ports:
    - containerPort: 80
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
    livenessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 10
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 5
      periodSeconds: 5
    env:
    - name: NODE_NAME
      valueFrom:
        fieldRef:
          fieldPath: spec.nodeName
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
EOF

# 预期输出: pod/nginx-demo created

# 查看 Pod 状态
kubectl get pod nginx-demo -o wide
# 预期输出:
# NAME          READY   STATUS    RESTARTS   AGE   IP            NODE     NOMINATED NODE
# nginx-demo    1/1     Running   0          30s   172.20.1.15   node-1   <none>

# 查看 Pod 详细信息（重点关注 Events）
kubectl describe pod nginx-demo
# 预期输出:
# Events:
#   Type    Reason     Age   From               Message
#   Normal  Scheduled  60s   default-scheduler  Successfully assigned default/nginx-demo to node-1
#   Normal  Pulling    59s   kubelet            Pulling image "registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24"
#   Normal  Pulled     55s   kubelet            Successfully pulled image in 4s
#   Normal  Created    55s   kubelet            Created container nginx
#   Normal  Started    54s   kubelet            Started container nginx

# 查看 Pod YAML（完整定义）
kubectl get pod nginx-demo -o yaml
```

### 任务 2: Pod 日志与调试 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 查看 Pod 日志
kubectl logs nginx-demo
# 预期输出: nginx 访问日志

# 实时跟踪日志（类似 tail -f）
kubectl logs nginx-demo -f
# 按 Ctrl+C 退出

# 查看最近 N 行日志
kubectl logs nginx-demo --tail=20

# 查看前一小时的日志
kubectl logs nginx-demo --since=1h

# 进入 Pod 容器执行命令
kubectl exec -it nginx-demo -- /bin/bash
# 在容器内检查:
# curl localhost:80          # 测试 nginx 响应
# cat /etc/nginx/nginx.conf  # 查看 nginx 配置
# echo $POD_NAME             # 查看环境变量
# echo $POD_IP               # 查看 Pod IP
# echo $NODE_NAME            # 查看所在节点
# exit

# 端口转发到本地（不通过 Service 直接访问 Pod）
kubectl port-forward pod/nginx-demo 8080:80
# 预期输出: Forwarding from 127.0.0.1:8080 -> 80
# 新终端验证:
# curl http://localhost:8080

# 查看 Pod 的资源使用（需要 metrics-server）
kubectl top pod nginx-demo
# 预期输出:
# NAME          CPU(cores)   MEMORY(bytes)
# nginx-demo    1m           5Mi

# 查看 Pod 的标签和注解
kubectl get pod nginx-demo --show-labels
kubectl get pod nginx-demo -o jsonpath='{.metadata.annotations}'

# 添加标签
kubectl label pod nginx-demo version=v1
kubectl get pod nginx-demo --show-labels
```

### 任务 3: 多容器 Pod（Sidecar 模式）(40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 创建带 Sidecar 日志采集器的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: sidecar-demo
  labels:
    app: sidecar-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/nginx
    livenessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 5
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /
        port: 80
      initialDelaySeconds: 3
  - name: log-collector
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'echo "Log collector started" && tail -f /var/log/nginx/access.log']
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/nginx
  volumes:
  - name: shared-logs
    emptyDir:
      medium: ""
      sizeLimit: 100Mi
EOF

# 预期输出: pod/sidecar-demo created

# 查看多容器 Pod 状态
kubectl get pod sidecar-demo
# 预期输出:
# NAME            READY   STATUS    RESTARTS   AGE
# sidecar-demo    2/2     Running   0          1m
# READY 2/2 表示两个容器都已就绪

# 查看详细信息
kubectl describe pod sidecar-demo
# 重点关注:
# Containers:
#   app:          Image: .../nginx:1.24, State: Running
#   log-collector: Image: .../busybox:1.36, State: Running

# 分别查看不同容器的日志
kubectl logs sidecar-demo -c app
# 预期输出: nginx 访问日志

kubectl logs sidecar-demo -c log-collector
# 预期输出:
# Log collector started
# (tail 输出的 access.log 内容)

# 生成一些访问日志
kubectl exec sidecar-demo -c log-collector -- wget -qO- http://localhost:80

# 再次查看 Sidecar 日志
kubectl logs sidecar-demo -c log-collector --tail=5
# 预期输出: 新的 access.log 条目
```

### 任务 4: Init Container 与 Pod 生命周期观察 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 创建带多个 Init Container 的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: init-demo
spec:
  initContainers:
  - name: init-wait
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'echo "Init: Waiting 5 seconds..." && sleep 5 && echo "Init: Wait complete"']
  - name: init-setup
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'echo "Init: Running setup..." && echo "Setup data" > /data/init.txt && echo "Init: Setup complete"']
    volumeMounts:
    - name: data-volume
      mountPath: /data
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/nginx:1.24
    volumeMounts:
    - name: data-volume
      mountPath: /data
    command: ['sh', '-c', 'echo "App started" && cat /data/init.txt && sleep 3600']
  volumes:
  - name: data-volume
    emptyDir: {}
EOF

# 预期输出: pod/init-demo created

# 观察 Pod 从 Init 到 Running 的过程
kubectl get pod init-demo -w
# 预期输出（动态更新）:
# NAME        READY   STATUS            RESTARTS   AGE
# init-demo   0/1     Init:0/2          0          2s
# init-demo   0/1     Init:1/2          0          6s
# init-demo   0/1     PodInitializing   0          8s
# init-demo   1/1     Running           0          10s

# 查看 Init Container 日志
kubectl logs init-demo -c init-wait
# 预期输出:
# Init: Waiting 5 seconds...
# Init: Wait complete

kubectl logs init-demo -c init-setup
# 预期输出:
# Init: Running setup...
# Init: Setup complete

# 查看主容器日志（验证 Init 数据传递）
kubectl logs init-demo -c app
# 预期输出:
# App started
# Setup data

# 查看 Events 了解调度与拉镜像过程
kubectl describe pod init-demo | grep -A 30 "Events:"
# 预期输出:
# Events:
#   Type    Reason     Age   From               Message
#   Normal  Scheduled  1m    default-scheduler  Successfully assigned...
#   Normal  Pulled     1m    kubelet            Container image ".../busybox:1.36" already present
#   Normal  Created    1m    kubelet            Created container init-wait
#   Normal  Started    1m    kubelet            Started container init-wait
#   Normal  Pulled     1m    kubelet            Container image ".../busybox:1.36" already present
#   Normal  Created    1m    kubelet            Created container init-setup
#   Normal  Started    1m    kubelet            Started container init-setup
#   ...

# 清理
kubectl delete pod nginx-demo sidecar-demo init-demo
# 预期输出:
# pod "nginx-demo" deleted
# pod "sidecar-demo" deleted
# pod "init-demo" deleted
```

---

## 配置示例

### 完整的 Pod 定义模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: pod-template
  labels:
    app: myapp
    version: v1
  annotations:
    description: "This is a pod template example"
spec:
  restartPolicy: Always
  nodeName: specific-node
  nodeSelector:
    disktype: ssd
  tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "gpu"
    effect: "NoSchedule"
  serviceAccountName: my-sa
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
  initContainers:
  - name: init-db-check
    image: busybox:1.36
    command: ['sh', '-c', 'until nslookup mysql.default.svc.cluster.local; do echo waiting for mysql; sleep 2; done']
  containers:
  - name: app
    image: myapp:1.0
    ports:
    - containerPort: 8080
      protocol: TCP
    env:
    - name: ENV_VAR
      value: "production"
    - name: SECRET_VAR
      valueFrom:
        secretKeyRef:
          name: my-secret
          key: password
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 15
      periodSeconds: 10
      failureThreshold: 3
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      failureThreshold: 30
      periodSeconds: 10
    volumeMounts:
    - name: config
      mountPath: /etc/config
    - name: data
      mountPath: /data
  - name: sidecar
    image: log-collector:1.0
    volumeMounts:
    - name: data
      mountPath: /data
      readOnly: true
  volumes:
  - name: config
    configMap:
      name: my-config
  - name: data
    persistentVolumeClaim:
      claimName: my-pvc
```

---

## 常见问题

### Q1: Pod 一直处于 Pending 怎么办？

使用 `kubectl describe pod <name>` 查看 Events 部分。常见原因：节点资源不足（CPU/内存不够）、没有匹配 nodeSelector 的节点、存在不可容忍的 Taint、PVC 无法绑定 PV。

### Q2: Pod 一直处于 ContainerCreating 怎么办？

通常是镜像拉取问题。检查：镜像名称是否正确、镜像仓库是否可访问、是否配置了 imagePullSecrets、节点磁盘空间是否充足。使用 `kubectl describe pod <name>` 查看 Events 中的具体错误信息。

### Q3: Pod 频繁重启（CrashLoopBackOff）怎么排查？

先查看日志：`kubectl logs <pod-name> --previous`（查看上一次运行的日志）。常见原因：应用启动失败（配置错误、依赖缺失）、OOMKilled（内存不足，调大 limits）、探针配置不当（livenessProbe 检测路径错误导致误判）。

### Q4: Sidecar 容器的日志怎么看？

使用 `-c` 参数指定容器名：`kubectl logs <pod-name> -c <container-name>`。如果 Sidecar 容器启动失败，`kubectl describe pod <name>` 会显示每个容器的状态。

### Q5: Init Container 失败了会怎样？

Init Container 失败后，Pod 不会继续启动主容器。如果 restartPolicy 为 Always 或 OnFailure，K8s 会重启 Init Container。如果 restartPolicy 为 Never，Pod 状态变为 Failed。使用 `kubectl logs <pod-name> -c <init-container-name>` 查看 Init Container 的日志。

### Q6: 如何在不删除 Pod 的情况下更新容器镜像？

直接更新单个 Pod 的镜像不是推荐做法（应该使用 Deployment 管理）。但如果确实需要，可以：`kubectl set image pod/<pod-name> <container-name>=<new-image>`。更新后 Pod 会被重建。

---

## 要点总结

| 概念 | 说明 | 常用命令 |
|------|------|---------|
| Pod Phase | 描述 Pod 整体状态（Pending/Running/Succeeded/Failed） | `kubectl get pod` |
| Container State | 描述容器运行状态（Waiting/Running/Terminated） | `kubectl describe pod` |
| Init Container | 在主容器前按顺序执行的初始化容器 | `spec.initContainers` |
| Sidecar | 辅助容器，与主容器共享网络和存储 | `spec.containers` 中追加 |
| restartPolicy | Pod 容器重启策略 | Always / OnFailure / Never |
| livenessProbe | 存活探针，失败重启容器 | `httpGet` / `tcpSocket` / `exec` |
| readinessProbe | 就绪探针，失败从 Service 移除 | `httpGet` / `tcpSocket` / `exec` |

---

## 延伸阅读

- [Pod 基础概念](../../domain-09-workload/01-pod-overview.md)
- [Pod 生命周期详解](../../domain-09-workload/02-pod-lifecycle.md)
- [Pod 综合排障](../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md)
- [ACK 工作负载管理](../../domain-12-cloud-providers/04-alicloud-ack/250-ack-workload.md)

```