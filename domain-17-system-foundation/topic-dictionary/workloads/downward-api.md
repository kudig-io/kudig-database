---
title: Downward API
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Downward API 是什么
- 如何 Downward API
trigger_keywords:
- Downward
- API
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
created: "2026-05-23"
---

# Downward API

## 概述
Downward API 允许容器在不使用 [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 客户端或访问 API Server 的情况下，消费关于自身或集群的信息。它降低了应用与 Kubernetes 的耦合度。

## 核心概念/原理
Downward API 提供两种方式将 Pod/容器级别的信息暴露给运行中的容器：
1. **环境变量**：通过 `fieldRef` 或 `resourceFieldRef` 注入。
2. **文件**：通过 `downwardAPI` 卷类型挂载为文件。

## 关键机制或特性
### 通过 `fieldRef` 可用的字段
- `metadata.name`（Pod 名称）
- `metadata.namespace`（命名空间）
- `metadata.uid`（Pod 唯一 ID）
- `metadata.annotations['<KEY>']`（单个注解值）
- `metadata.labels['<KEY>']`（单个标签值）

**仅支持环境变量**：
- `spec.serviceAccountName`
- `spec.nodeName`
- `status.hostIP` / `status.hostIPs`
- `status.podIP` / `status.podIPs`

**仅支持 downwardAPI 卷**：
- `metadata.labels`（全部标签，每行一个）
- `metadata.annotations`（全部注解，每行一个）

### 通过 `resourceFieldRef` 可用的字段
- `limits.cpu` / `requests.cpu`
- `limits.memory` / `requests.memory`
- `limits.hugepages-*` / `requests.hugepages-*`
- `limits.ephemeral-storage` / `requests.ephemeral-storage`

**注意**：若容器支持原地调整 CPU/内存资源，downwardAPI 卷会动态更新，但环境变量仅在容器重启后才会更新。

**默认值行为**：若未设置 CPU/内存 limit，downward API 会默认暴露节点可分配的最大值。

## 使用场景
- 将 Pod 名称注入到应用已知的环境变量中作为唯一标识。
- 在容器内获取节点 IP 或 Pod IP 进行日志记录或本地决策。
- 将标签/注解以文件形式挂载到容器内，供配置文件或脚本读取。
- 让应用根据自身的资源 limit/request 动态调整线程池或缓冲区大小。

## 最佳实践/注意事项
- 对于可能随时间变化的信息（如资源限制），优先使用 downwardAPI 卷而非环境变量，以获得更新能力。
- 不要依赖 downward API 替代专业的 Kubernetes 客户端库进行复杂的控制面交互。
- 确保 Pod 模板中正确引用容器名称（使用 `resourceFieldRef` 时）。

## 实战 YAML 示例

### 通过环境变量注入 Pod 信息

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: downward-env-demo
  namespace: prod
  labels:
    app: myapp
    version: v1.2.0
  annotations:
    owner: "platform-team"
spec:
  containers:
  - name: app
    image: myregistry.com/myapp:v1.2.0
    env:
    # Pod 元数据
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
    - name: NODE_NAME
      valueFrom:
        fieldRef:
          fieldPath: spec.nodeName
    - name: NODE_IP
      valueFrom:
        fieldRef:
          fieldPath: status.hostIP
    - name: SERVICE_ACCOUNT
      valueFrom:
        fieldRef:
          fieldPath: spec.serviceAccountName
    # 容器资源信息
    - name: CPU_REQUEST
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: requests.cpu
    - name: MEMORY_LIMIT
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: limits.memory
          divisor: "1Mi"                     # 以 MiB 为单位输出
    resources:
      requests:
        cpu: "250m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "512Mi"
```

### 通过 downwardAPI 卷挂载（支持动态更新）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: downward-vol-demo
  namespace: prod
  labels:
    app: myapp
    version: v1.2.0
  annotations:
    config.hash: "abc123"
spec:
  containers:
  - name: app
    image: myregistry.com/myapp:v1.2.0
    volumeMounts:
    - name: podinfo
      mountPath: /etc/podinfo
      readOnly: true
    resources:
      requests:
        cpu: "250m"
        memory: "256Mi"
      limits:
        cpu: "1000m"
        memory: "512Mi"
  volumes:
  - name: podinfo
    downwardAPI:
      items:
      - path: "labels"                       # /etc/podinfo/labels
        fieldRef:
          fieldPath: metadata.labels
      - path: "annotations"                  # /etc/podinfo/annotations
        fieldRef:
          fieldPath: metadata.annotations
      - path: "cpu_limit"                    # /etc/podinfo/cpu_limit
        resourceFieldRef:
          containerName: app
          resource: limits.cpu
          divisor: "1m"                      # 以 millicores 输出
      - path: "mem_limit"                    # /etc/podinfo/mem_limit
        resourceFieldRef:
          containerName: app
          resource: limits.memory
          divisor: "1Mi"
```

### Java 应用动态调优示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: java-app
spec:
  containers:
  - name: java
    image: myregistry.com/java-app:v1.0
    env:
    - name: MEM_LIMIT
      valueFrom:
        resourceFieldRef:
          resource: limits.memory
          divisor: "1Mi"
    - name: CPU_LIMIT
      valueFrom:
        resourceFieldRef:
          resource: limits.cpu
    command:
    - java
    - -XX:MaxRAMPercentage=75.0              # 使用容器内存 limit 的 75%
    - -XX:ActiveProcessorCount=$(CPU_LIMIT)  # 按实际 CPU limit 设置线程数
    - -jar
    - /app/app.jar
    resources:
      requests:
        cpu: "2000m"
        memory: "2Gi"
      limits:
        cpu: "4000m"
        memory: "4Gi"
```

## 故障排查

### 环境变量为空或值不正确
- **症状**: 容器内读取的环境变量为空字符串。
- **常见原因**: `fieldPath` 引用了不支持的字段；`containerName` 未正确指定。
- **诊断命令**:
  ```bash
  # 查看 Pod 中环境变量实际值
  kubectl exec <pod-name> -n prod -- env | grep -E "POD_|NODE_|CPU_|MEM_"
  
  # 查看 Pod spec 确认 Downward API 配置
  kubectl get pod <pod-name> -n prod -o yaml | grep -A 5 "fieldRef\|resourceFieldRef"
  ```

### downwardAPI 卷文件未更新
- **症状**: 修改 Pod 标签/注解后，卷中的文件未同步更新。
- **常见原因**: kubelet 的同步周期未到（默认约 1 分钟）；环境变量方式天然不支持动态更新。
- **解决方案**: 等待 kubelet 同步周期；如需实时更新，使用卷方式而非环境变量。

### 未设置 limit 时 Downward API 返回节点最大值
- **症状**: 资源信息返回异常大的值（如几十个 CPU core）。
- **原因**: 未设置 `limits` 时，Downward API 会返回节点可分配的最大值。
- **解决方案**: 始终为容器设置 `resources.limits`。

## 生产检查清单

- [ ] Pod 名称、命名空间等元数据通过 Downward API 注入，用于日志和追踪
- [ ] 资源限制值通过 Downward API 注入，供应用动态调优
- [ ] 需要动态更新的信息使用卷方式（而非环境变量）
- [ ] 所有容器均设置了 `resources.limits`，避免 Downward API 返回节点级别的值

## 命令快速参考

```bash
# 查看容器中的 Downward API 环境变量
kubectl exec <pod-name> -n prod -- env | sort

# 查看 downwardAPI 卷中的文件内容
kubectl exec <pod-name> -n prod -- cat /etc/podinfo/labels
kubectl exec <pod-name> -n prod -- cat /etc/podinfo/annotations

# 查看 Pod 的资源配置（验证 Downward API 源值）
kubectl get pod <pod-name> -n prod -o jsonpath='{.spec.containers[0].resources}'
```

## 交叉引用

- [Pods 基础](./pods.md)
- [Pod 生命周期](./pod-lifecycle.md)
- [容器环境](./container-environment.md)
- [高级 Pod 配置](./advanced-pod-configuration.md)
- [工作负载概览与架构](../../domain-02-workloads-applications/01-workload-overview-architecture.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/downward-api/
