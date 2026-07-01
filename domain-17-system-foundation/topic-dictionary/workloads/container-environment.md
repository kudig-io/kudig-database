---
title: 容器环境（Container Environment）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器环境（Container Environment） 是什么
- 如何 容器环境（Container Environment）
trigger_keywords:
- 容器环境
- Container
- Environment
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---



# 容器环境（Container Environment）

## 概述

[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 容器环境为容器提供了若干重要资源，包括文件系统、容器自身信息，以及集群中其他对象的信息。了解这些资源有助于开发人员在容器内正确获取运行时的上下文信息。

## 核心概念/原理

### 文件系统

容器可用的文件系统由两部分组成：
- **容器镜像层**：包含应用程序和预置的静态文件
- **卷（Volumes）**：在 Pod 级别挂载到容器中的持久化或临时存储

### 容器自身信息

- **主机名（Hostname）**：容器的主机名即其所在 Pod 的名称。可通过 `hostname` 命令或 libc 的 `gethostname` 函数调用获取
- **Pod 名称和命名空间**：通过 [[domain-17-system-foundation/topic-dictionary/workloads/downward-api.md|Downward API]] 以环境变量的形式注入到容器中
- **用户定义的环境变量**：在 Pod 定义中通过 `env` 或 `envFrom` 指定的环境变量，以及容器镜像构建时静态设置的环境变量，均对容器可见

### 集群信息

当容器创建时，Kubernetes 会将同一命名空间内所有正在运行的 [[Service|Service]] 信息以环境变量的形式注入到该容器中。对于名为 `foo` 的 Service，会设置如下环境变量：

```
FOO_SERVICE_HOST=<服务所在的主机地址>
FOO_SERVICE_PORT=<服务暴露的端口>
```

Service 拥有独立的 IP 地址，如果集群启用了 DNS 插件，容器也可以通过 DNS 名称访问这些服务。

> **注意**：这种通过环境变量注入的服务发现方式仅限于容器创建时已经存在的同命名空间 Service 以及 Kubernetes 控制平面服务。

## 关键机制或特性

- **Downward API**：允许将 Pod 和节点的元数据（如 Pod 名称、命名空间、标签、IP 等）以环境变量或卷文件的形式暴露给容器
- **Service 环境变量注入**：在容器启动时自动完成，若后续新增 Service，已运行的容器不会自动获得新的环境变量
- **DNS 服务发现**：比环境变量更灵活的服务发现方式，推荐在大多数场景下使用 DNS 而非依赖环境变量

## 使用场景

- 容器内应用需要知道自身 Pod 名称或所在命名空间以进行日志标记或配置分区
- 在容器启动脚本中需要动态获取同一命名空间内其他服务的地址和端口
- 通过环境变量向容器传递配置参数、密钥引用或运行时上下文

## 最佳实践/注意事项

- **优先使用 DNS 进行服务发现**：Service 环境变量仅在容器创建时注入，后续新增的 Service 对已运行容器不可见；DNS 则没有此限制
- **合理利用 Downward API**：避免在镜像中硬编码 Pod 信息，使用 Downward API 动态注入
- **注意环境变量顺序和覆盖规则**：Pod 中定义的环境变量可以覆盖镜像中静态设置的环境变量
- **跨命名空间访问需使用 FQDN**：若通过 DNS 访问其他命名空间的服务，应使用完整域名（如 `my-service.other-namespace.svc.cluster.local`）

## 生产 YAML 示例

### 综合容器环境配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-env
  namespace: production
  labels:
    app: order-service
spec:
  containers:
  - name: app
    image: registry.example.com/apps/order-service:v4.0
    # === 用户定义的环境变量 ===
    env:
    # 直接指定值
    - name: APP_ENV
      value: "production"
    - name: LOG_LEVEL
      value: "info"
    # 从 Downward API 获取 Pod 元数据
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
    # 从 Downward API 获取资源限制
    - name: MEMORY_LIMIT
      valueFrom:
        resourceFieldRef:
          containerName: app
          resource: limits.memory
    # 从 ConfigMap 获取配置
    - name: DB_HOST
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database.host
    # 从 Secret 获取敏感信息
    - name: DB_PASSWORD
      valueFrom:
        secretKeyRef:
          name: app-secrets
          key: database.password
    # === 批量注入 ConfigMap 所有键值 ===
    envFrom:
    - configMapRef:
        name: app-feature-flags
      prefix: "FF_"              # 添加前缀避免冲突：FF_ENABLE_NEW_UI=true
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1"
        memory: "1Gi"
    volumeMounts:
    # Downward API 通过 Volume 暴露标签和注解
    - name: podinfo
      mountPath: /etc/podinfo
      readOnly: true
  volumes:
  - name: podinfo
    downwardAPI:
      items:
      - path: "labels"
        fieldRef:
          fieldPath: metadata.labels
      - path: "annotations"
        fieldRef:
          fieldPath: metadata.annotations
      - path: "cpu_limit"
        resourceFieldRef:
          containerName: app
          resource: limits.cpu
          divisor: "1m"          # 以毫核为单位
```

### Java 应用自适应内存配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: java-app
spec:
  containers:
  - name: java
    image: registry.example.com/apps/java-service:v3.0
    env:
    - name: MEMORY_LIMIT
      valueFrom:
        resourceFieldRef:
          resource: limits.memory
    # 使用 Downward API 的内存限制动态设置 JVM 堆大小
    - name: JAVA_OPTS
      value: "-XX:+UseContainerSupport -XX:MaxRAMPercentage=75.0"
    resources:
      requests:
        memory: "2Gi"
      limits:
        memory: "2Gi"
```

## 环境信息来源对照表

| 信息类型 | 获取方式 | 动态更新 | 示例 |
|----------|----------|----------|------|
| Pod 名称/命名空间 | Downward API (env) | 否（创建时固定） | `metadata.name` |
| Pod IP | Downward API (env) | 否 | `status.podIP` |
| Pod 标签/注解 | Downward API (volume) | 是（文件自动刷新） | `metadata.labels` |
| 资源 requests/limits | Downward API (env/volume) | 否 | `limits.memory` |
| 节点名称 | Downward API (env) | 否 | `spec.nodeName` |
| 同命名空间 Service | 自动注入 env | 否（仅创建时） | `FOO_SERVICE_HOST` |
| 跨命名空间 Service | DNS 查询 | 是 | `svc.ns.svc.cluster.local` |
| ConfigMap 值 | `envFrom`/`env.valueFrom` | 否（Pod 重建后生效） |  |
| Secret 值 | `env.valueFrom` | 否 | |
| ConfigMap (Volume) | Volume 挂载 | 是（kubelet 定期同步） | |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 环境变量为空 | ConfigMap/Secret 不存在或 key 拼写错误 | `kubectl get cm/secret <name> -o yaml`；检查 `optional: false` |
| Service 环境变量缺失 | Service 在 Pod 之后创建 | 确保 Service 先于引用它的 Pod 创建；优先使用 DNS 服务发现 |
| Downward API 值不更新 | env 方式注入的值在 Pod 创建时固定 | 使用 Volume 方式挂载需要动态更新的值（标签/注解） |
| 环境变量被意外覆盖 | Pod env 覆盖了镜像中的 ENV | 检查优先级：Pod env > envFrom > 镜像 ENV |
| envFrom 注入了不需要的变量 | ConfigMap 包含过多键值 | 使用 `prefix` 参数隔离命名空间 |

## 生产检查清单

- [ ] 敏感信息（密码、Token）使用 Secret 而非明文 env
- [ ] 服务发现优先使用 DNS 而非 Service 环境变量
- [ ] Downward API 需要动态更新的数据使用 Volume 方式
- [ ] ConfigMap 批量注入使用 `prefix` 避免变量名冲突
- [ ] Java 应用使用 `-XX:+UseContainerSupport` 感知容器内存限制
- [ ] 关键环境变量设置 `optional: false` 确保 Pod 在缺失时无法启动

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看容器内所有环境变量
kubectl exec <pod> -c <container> -- env | sort

# 查看特定环境变量
kubectl exec <pod> -- printenv POD_NAME POD_NAMESPACE

# 检查 Downward API Volume 内容
kubectl exec <pod> -- cat /etc/podinfo/labels

# 查看 Service 自动注入的环境变量
kubectl exec <pod> -- env | grep _SERVICE_

# 在集群内测试 DNS 解析
kubectl exec <pod> -- nslookup my-service.my-namespace.svc.cluster.local
```

## 交叉引用

- [Downward API](downward-api.md) — 详细的 Downward API 字段和用法
- [Pod Hostname](pod-hostname.md) — 容器主机名的配置和 DNS
- [容器生命周期钩子](container-lifecycle-hooks.md) — 容器启动/停止时的事件处理
- [容器镜像](images.md) — 镜像层文件系统和环境变量继承

## 参考链接

- [Kubernetes 官方文档：容器环境](https://kubernetes.io/docs/concepts/containers/container-environment/)
- [Kubernetes Downward API 文档](https://kubernetes.io/docs/concepts/workloads/pods/downward-api/)
- [Kubernetes Service 与 DNS](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[domain-17-system-foundation/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[domain-17-system-foundation/topic-dictionary/workloads/autoscaling-workloads.md|Autoscaling Workloads]]
