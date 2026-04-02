# API-initiated Eviction

## 概述

API 发起驱逐（API-initiated Eviction）是通过 Eviction API 创建 `Eviction` 对象来触发 Pod 优雅终止的过程。可以直接调用 Eviction API，也可以通过 `kubectl drain` 等工具间接调用。

## 核心概念/原理

使用 API 为 Pod 创建 Eviction 对象类似于对 Pod 执行受策略控制的 `DELETE` 操作。API 发起的驱逐尊重配置的 `PodDisruptionBudgets` 和 `terminationGracePeriodSeconds`。

### 调用方式

可以通过 Kubernetes 语言客户端访问 API 并创建 `Eviction` 对象，POST 操作示例如下：

```json
{
  "apiVersion": "policy/v1",
  "kind": "Eviction",
  "metadata": {
    "name": "quux",
    "namespace": "default"
  }
}
```

或者使用 curl：
```bash
curl -v -H 'Content-type: application/json' https://your-cluster-api-endpoint.example/api/v1/namespaces/default/pods/quux/eviction -d @eviction.json
```

## 关键机制或特性

### API 服务器响应

API 服务器执行准入检查后可能返回以下响应：

- **200 OK**：驱逐被允许，创建 Eviction 子资源，Pod 被删除（类似于发送 DELETE 请求到 Pod URL）。
- **429 Too Many Requests**：由于配置的 PodDisruptionBudget 限制，当前不允许驱逐。可以稍后重试。也可能因为 API 速率限制而返回此响应。
- **500 Internal Server Error**：由于配置错误（如多个 PodDisruptionBudget 引用同一个 Pod），驱逐不被允许。

### 驱逐流程

如果 API 服务器允许驱逐：

1. API 服务器更新 Pod 资源，添加 deletion timestamp，Pod 被视为已终止，并标记配置的宽限期。
2. Pod 所在节点的 kubelet 注意到 Pod 被标记为终止，开始优雅关闭本地 Pod。
3. 在 kubelet 关闭 Pod 期间，控制平面将 Pod 从 EndpointSlice 对象中移除，控制器不再将该 Pod 视为有效对象。
4. Pod 的宽限期到期后，kubelet 强制终止本地 Pod。
5. kubelet 通知 API 服务器移除 Pod 资源。
6. API 服务器删除 Pod 资源。

## 使用场景

- 节点维护前通过 `kubectl drain` 安全地驱逐节点上的所有 Pod。
- 自动化运维工具需要以受控方式移除 Pod，同时尊重 PodDisruptionBudget。
- 应用发布或缩容时，通过 API 驱逐实现优雅下线。

## 最佳实践/注意事项

- 如果应用进入故障状态（如 ReplicaSet 创建的新 Pod 无法进入 Ready 状态），Eviction API 可能持续返回 429 或 500，直到人工干预。
- 遇到卡住的驱逐时，可以尝试：
  - 中止或暂停导致问题的自动化操作，调查卡住的应用后再恢复。
  - 等待一段时间后，直接从集群控制平面删除 Pod（不使用 Eviction API）。
- API 发起的驱逐会尊重 PodDisruptionBudget，而节点压力驱逐不会。

## 参考链接

- [Kubernetes 官方文档 - API-initiated Eviction](https://kubernetes.io/docs/concepts/scheduling-eviction/api-eviction/)
