# Local ephemeral storage（本地临时存储）

## 概述

节点的本地临时存储由本地可写设备（如磁盘）或 RAM 支持。“临时”意味着 Kubernetes 不提供长期的持久性保证。Pod 使用本地临时存储作为临时工作区、缓存和日志存放位置。kubelet 也使用此类存储来保存容器镜像、运行中容器的可写层以及节点级容器日志。

## 核心概念/原理

- **本地临时数据**：包括 `emptyDir` 卷（非 tmpfs）、容器可写层、容器镜像和节点级容器日志。
- **资源管理**：Kubernetes 允许跟踪、预留和限制 Pod 可以消费的本地临时存储量。
- **两种节点配置**：
  1. **单文件系统**：所有临时数据（emptyDir、日志、镜像、可写层）都在同一个文件系统上。
  2. **双文件系统**：一个文件系统用于 emptyDir 和日志，另一个独立的文件系统用于容器镜像层和可写层。

## 关键机制或特性

### 资源请求与限制

Pod 中的每个容器可以设置：
- `spec.containers[].resources.requests.ephemeral-storage`
- `spec.containers[].resources.limits.ephemeral-storage`

Pod 级别的请求和限制是各容器值的总和。

### 驱逐机制

- 当 Pod 使用的临时存储超过限制时，kubelet 会设置驱逐信号并触发 Pod 驱逐。
- **容器级隔离**：如果某个容器的可写层和日志使用超过其限制，Pod 会被标记驱逐。
- **Pod 级隔离**：如果所有容器的临时存储使用加上 Pod 的 `emptyDir` 卷总使用量超过 Pod 总限制，也会触发驱逐。

### emptyDir 卷与临时存储

- 默认 `emptyDir` 卷的数据计入 Pod 的本地临时存储使用量。
- `emptyDir` 可以设置 `sizeLimit`，超出后也可能触发 Pod 驱逐。
- **注意**：tmpfs 类型的 `emptyDir`（`medium: Memory`）计入容器内存使用，而非本地临时存储。

### 存储使用测量方式

1. **目录扫描（Directory Scan）**
   - kubelet 定期扫描 `emptyDir` 卷、容器日志目录和可写层，测量已用空间。
   - 不跟踪已删除但仍打开的文件描述符所占用的空间。

2. **项目配额（Project Quota）**
   - Kubernetes v1.31 [beta]（默认禁用）
   - 利用操作系统级项目配额功能（XFS/ext4 支持）更快速、准确地跟踪存储使用。
   - 能够正确统计已删除但仍有打开文件描述符的文件所占空间。
   - 需要启用 `LocalStorageCapacityIsolationFSQuotaMonitoring` 特性门，并且 Pod 必须运行在用户命名空间中。

## 使用场景

- **临时缓存**：应用将频繁访问的数据缓存到 `emptyDir` 中，提升访问速度。
- **日志缓冲**：将容器日志先写入本地临时存储，再由日志收集器聚合。
- **构建与批处理**：CI/CD 构建任务使用临时存储作为编译和产物输出目录。
- **沙盒数据**：需要隔离的临时文件操作空间，不需要跨 Pod 或节点共享。

## 最佳实践/注意事项

- 如果 Pod 规格中未设置 `ephemeral-storage` 限制，资源配额（ResourceQuota）不会对临时存储生效。
- 单文件系统配置时，建议将该文件系统专门用于 Kubernetes 数据，避免与其他系统服务竞争空间。
- 使用项目配额需要文件系统支持并正确挂载（XFS 原生支持；ext4 需通过 `tune2fs -O project -Q prjquota` 启用）。
- 注意区分 `emptyDir` 的默认磁盘介质和 `medium: Memory`（tmpfs）在资源统计上的差异。
- 如果 kubelet 未按支持的方式配置本地临时存储，即使 Pod 超出限制也不会被驱逐。

## 生产 YAML 示例

### Pod 临时存储 requests/limits

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: build-agent
  namespace: ci
spec:
  containers:
    - name: builder
      image: registry.example.com/builder:v3.0
      resources:
        requests:
          cpu: "2"
          memory: 4Gi
          ephemeral-storage: 10Gi          # 请求 10Gi 临时存储
        limits:
          cpu: "4"
          memory: 8Gi
          ephemeral-storage: 20Gi          # 最大 20Gi
      volumeMounts:
        - name: workspace
          mountPath: /workspace
  volumes:
    - name: workspace
      emptyDir:
        sizeLimit: 15Gi                    # emptyDir 独立限制
  restartPolicy: Never
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod 被驱逐，reason: Evicted | 临时存储使用超限 | `kubectl describe pod` 查看 eviction 原因；`kubectl exec` 检查磁盘使用 |
| emptyDir 数据丢失 | Pod 重建（非容器重启） | 正常行为；持久数据用 PVC |
| kubelet 未执行存储限制 | kubelet 未配置本地临时存储管理 | 检查 kubelet 配置和文件系统 |

## 生产检查清单

- [ ] CI/CD 构建 Pod 设置 `ephemeral-storage` limits
- [ ] emptyDir 设置 `sizeLimit`
- [ ] tmpfs emptyDir (`medium: Memory`) 计入内存使用
- [ ] 命名空间配置 ResourceQuota 包含 ephemeral-storage
- [ ] 配置日志轮转避免日志撑满临时存储

## 命令快速参考

```bash
# 查看 Pod 临时存储使用
kubectl exec <pod-name> -- df -h /

# 查看节点临时存储
kubectl describe node <node-name> | grep ephemeral-storage
```

## 交叉引用

- [临时卷](./ephemeral-volumes.md) — CSI 临时卷和通用临时卷
- [卷](./volumes.md) — emptyDir 卷类型

## 参考链接

- https://kubernetes.io/docs/concepts/storage/ephemeral-storage/
