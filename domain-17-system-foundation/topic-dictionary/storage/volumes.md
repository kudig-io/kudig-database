---
title: Volumes（卷）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- opa
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volumes（卷） 是什么
- 如何 Volumes（卷）
trigger_keywords:
- Volumes
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- policy-basics
created: "2026-05-23"
---

# Volumes（卷）

## 概述

[[Kubernetes|Kubernetes]] Volumes 为 Pod 中的容器提供了一种通过文件系统访问和共享数据的机制。容器内的磁盘文件默认是临时的，容器崩溃或停止后数据会丢失。Volume 解决了数据持久化和容器间共享存储的问题。

## 核心概念/原理

- **Volume 本质**：一个目录，可能包含数据，可被 Pod 中的容器访问。具体目录如何产生、由什么介质支持、包含什么内容，取决于使用的 volume 类型。
- **生命周期**：
  - **Ephemeral volume**（临时卷）：生命周期与 Pod 绑定，Pod 被删除时卷也被销毁（如 `emptyDir`）。
  - **Persistent volume**（持久卷）：生命周期独立于 Pod，Pod 删除后卷仍然存在。
- **使用方式**：在 Pod 的 `.spec.volumes` 中定义卷，在 `.spec.containers[*].volumeMounts` 中声明挂载路径。

## 关键机制或特性

### 常见卷类型

| 类型 | 说明 |
|------|------|
| `emptyDir` | Pod 分配到节点时创建的空目录，可用于容器间共享临时数据；支持设置 `sizeLimit`，也可设置为 `medium: Memory`（tmpfs）。 |
| `hostPath` | 挂载宿主机上的文件或目录到 Pod 中，存在安全风险，建议尽量使用 `local` PV 替代。 |
| `configMap` / `secret` | 将 ConfigMap 或 Secret 中的数据作为文件挂载到 Pod 中，默认只读。 |
| `downwardAPI` | 将 Pod 的元数据（如 labels、annotations）以文件形式暴露给容器。 |
| `persistentVolumeClaim` | 通过 PVC 挂载持久卷，实现数据的持久化存储。 |
| `nfs` / `iscsi` / `fc` | 挂载现有的网络存储或块存储设备。 |
| `csi` | 通过容器存储接口（CSI）挂载第三方存储系统提供的卷，是当前推荐的扩展方式。 |
| `projected` | 将多个现有卷源映射到同一个目录中。 |
| `image`（Beta） | 将 OCI 镜像或制品作为只读卷挂载到容器中。 |

### 子路径（subPath / subPathExpr）

- `subPath`：指定卷内的子路径进行挂载，使同一个卷可在同一 Pod 中被多个容器以不同子目录挂载。
- `subPathExpr`：支持使用 [[domain-17-system-foundation/topic-dictionary/workloads/downward-api.md|downward API]] 环境变量动态构建子路径名。

### 挂载传播（Mount Propagation）

- `None`（默认）：容器内外的挂载互不可见。
- `HostToContainer`：宿主机后续挂载对该容器可见。
- `Bidirectional`：容器内挂载也会传播回宿主机及其他使用相同卷的 Pod（仅允许特权容器使用）。

### 递归只读挂载（Recursive Read-Only Mounts）

- Kubernetes v1.33 [stable]：设置 `recursiveReadOnly: Enabled` 可使挂载点及其所有子挂载都变为递归只读。

## 使用场景

- **数据持久化**：数据库、文件服务等需要保存数据，防止容器重启后数据丢失。
- **配置注入**：通过 `configMap` 或 `secret` 卷将配置和敏感信息挂载到容器中。
- **容器间共享数据**：同一 Pod 内的多个容器通过 `emptyDir` 共享临时文件或缓存。
- **访问外部存储**：通过 `nfs`、`iscsi` 或 CSI 驱动连接企业级存储系统。

## 最佳实践/注意事项

- 尽量避免使用 `hostPath` 卷，以防止安全风险和节点差异导致的问题；如需本地存储，优先使用 `local` PersistentVolume。
- `emptyDir` 的默认存储介质取决于节点的 [[kubelet|kubelet]] 根目录所在磁盘，可通过 `medium: Memory` 使用内存加速访问。
- 使用 `subPath` 挂载的容器不会自动接收到 ConfigMap/Secret 的更新。
- 尽量使用 CSI 驱动替代已弃用的 in-tree 存储插件。

## 生产 YAML 示例

### 多卷类型组合（emptyDir + ConfigMap + PVC）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  namespace: production
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
      containers:
        - name: app
          image: registry.example.com/web:v3.0
          volumeMounts:
            - name: cache
              mountPath: /tmp/cache          # emptyDir 缓存
            - name: config
              mountPath: /etc/app/config
              readOnly: true                 # ConfigMap 配置
            - name: data
              mountPath: /var/data           # PVC 持久数据
          resources:
            requests:
              cpu: "250m"
              memory: 256Mi
        - name: log-collector
          image: registry.example.com/fluentbit:v2.0
          volumeMounts:
            - name: cache
              mountPath: /var/log/app        # 与 app 容器共享 emptyDir
              readOnly: true
          resources:
            requests:
              cpu: "50m"
              memory: 64Mi
      volumes:
        - name: cache
          emptyDir:
            sizeLimit: 1Gi                   # 限制 emptyDir 大小
        - name: config
          configMap:
            name: web-app-config
            items:
              - key: application.yaml
                path: application.yaml
        - name: data
          persistentVolumeClaim:
            claimName: web-app-data
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 卡在 ContainerCreating | PVC 未绑定或 CSI 驱动异常 | `kubectl describe pod`；`kubectl get pvc` 检查绑定状态 |
| 容器内挂载点为空 | ConfigMap/Secret 不存在或 key 拼写错误 | `kubectl get configmap/secret` 确认存在；检查 items 配置 |
| emptyDir 数据丢失 | Pod 重新调度到不同节点 | emptyDir 随 Pod 销毁是正常行为；持久数据应使用 PVC |
| subPath 挂载后内容不更新 | subPath 挂载不接收自动更新 | 改为挂载整个目录或重启 Pod |
| 递归只读挂载失败 | 容器运行时或内核版本不支持 | 确认使用 Linux 5.12+ 内核和兼容运行时 |

## 生产检查清单

- [ ] 避免使用 hostPath 卷，使用 CSI 或 local PV 替代
- [ ] emptyDir 设置 `sizeLimit` 防止磁盘撑满
- [ ] Memory-backed emptyDir (`medium: Memory`) 计入内存 limits
- [ ] 使用 CSI 驱动替代已弃用的 in-tree 存储插件
- [ ] ConfigMap/Secret 卷避免使用 subPath（无法自动更新）
- [ ] 敏感卷文件设置合适的 defaultMode（如 0400）

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 的卷配置
kubectl get pod <pod-name> -o jsonpath='{.spec.volumes}' | jq .

# 查看容器挂载点
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[0].volumeMounts}' | jq .

# 进入容器检查挂载内容
kubectl exec <pod-name> -- ls -la /var/data

# 查看 PVC 绑定状态
kubectl get pvc -n production
```

## 交叉引用

- [持久卷](./persistent-volumes.md) — PV/PVC 持久化存储
- [临时卷](./ephemeral-volumes.md) — CSI 临时卷和通用临时卷
- [投射卷](./projected-volumes.md) — 多源投射到同一目录
- [存储类](./storage-classes.md) — 动态供给后端配置

## 参考链接

- https://kubernetes.io/docs/concepts/storage/volumes/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume.md|Persistent Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/persistent-volume-claim.md|Persistent Volume Claim]]
- [[domain-17-system-foundation/topic-dictionary/storage/storage-class.md|Storage Class]]
- [[domain-17-system-foundation/topic-dictionary/storage/volume.md|Volume]]
- [[domain-17-system-foundation/topic-dictionary/storage/emptydir.md|Emptydir]]
