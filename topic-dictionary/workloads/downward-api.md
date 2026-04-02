# Downward API

## 概述
Downward API 允许容器在不使用 Kubernetes 客户端或访问 API Server 的情况下，消费关于自身或集群的信息。它降低了应用与 Kubernetes 的耦合度。

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

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/downward-api/
