# 运行时类（RuntimeClass）

## 概述

RuntimeClass 是 Kubernetes 中用于选择容器运行时配置的特性（自 v1.20 起进入 Stable）。它允许用户为不同的 Pod 指定不同的容器运行时配置，从而在性能与安全性之间取得平衡。

## 核心概念/原理

### 设计动机

- **安全隔离**：对于需要高信息安全保障的工作负载，可以将其调度到使用硬件虚拟化的容器运行时（如 Kata Containers、gVisor），以换取更强的隔离性
- **运行时差异化配置**：即使使用同一种容器运行时，也可以通过不同的 RuntimeClass 应用不同的设置

### 配置步骤

使用 RuntimeClass 需要完成两个步骤：

1. **在节点上配置 CRI 实现**：具体的配置取决于所使用的容器运行时（如 containerd、CRI-O）
2. **创建 RuntimeClass 资源**：为每种运行时配置创建对应的 RuntimeClass 对象

### RuntimeClass 资源定义

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: myclass          # 引用该 RuntimeClass 时使用的名称
handler: myconfiguration # 对应 CRI 实现中的 handler 名称
```

- `name` 必须是有效的 DNS 子域名
- `handler` 必须是有效的 DNS 标签名
- RuntimeClass 是集群级别的非命名空间资源
- 建议将 RuntimeClass 的写操作限制给集群管理员

### 运行时配置示例

**containerd**：在 `/etc/containerd/config.toml` 中配置
```toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.${HANDLER_NAME}]
```

**CRI-O**：在 `/etc/crio/crio.conf` 中配置
```ini
[crio.runtime.runtimes.${HANDLER_NAME}]
  runtime_path = "${PATH_TO_BINARY}"
```

## 关键机制或特性

### Pod 中指定 RuntimeClass

在 Pod 的 `spec` 中通过 `runtimeClassName` 字段指定要使用的 RuntimeClass：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mypod
spec:
  runtimeClassName: myclass
  # ...
```

- 若指定的 RuntimeClass 不存在，或 CRI 无法运行对应的 handler，Pod 将进入 `Failed` 终止状态
- 若未指定 `runtimeClassName`，则使用默认的 RuntimeHandler，等同于禁用 RuntimeClass 功能时的行为

### 调度约束（Scheduling）

通过 RuntimeClass 的 `scheduling` 字段，可以设置约束，确保使用该 RuntimeClass 的 Pod 被调度到支持它的节点上：

- **`nodeSelector`**：与 Pod 的 `nodeSelector` 取交集，确保 Pod 落在具有特定标签的节点上
- **`tolerations`**：与 Pod 的 `tolerations` 取并集，允许 Pod 容忍特定节点的污点

> **注意**：默认情况下，RuntimeClass 假设集群节点配置是同质的；若节点异构，应通过 `scheduling` 字段进行约束。

### Pod 开销（Pod Overhead）

自 v1.24 起进入 Stable。RuntimeClass 支持通过 `overhead` 字段声明运行 Pod 所需的额外资源开销（如虚拟化层消耗的资源），使调度器和其他组件在决策时能够将其纳入考量：

```yaml
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
```

## 使用场景

- **高安全隔离工作负载**：将敏感应用调度到基于 VM 的容器运行时（如 Kata Containers）
- **GPU 或特定硬件加速运行时**：为需要 NVIDIA GPU 的 Pod 指定 `nvidia` runtime handler
- **多运行时混合集群**：在同一集群中同时运行普通 runc 容器和沙箱容器，通过 RuntimeClass 进行区分

## 最佳实践/注意事项

- **限制 RuntimeClass 的写权限**：由于 RuntimeClass 影响节点级运行时行为，建议仅允许集群管理员创建和修改
- **确保节点标签与 RuntimeClass 的 nodeSelector 匹配**：在异构集群中，避免 Pod 因找不到匹配节点而长时间 Pending
- **准确声明 Pod Overhead**：低估开销可能导致节点资源耗尽，高估则造成资源浪费
- **验证 CRI handler 可用性**：在创建 RuntimeClass 前，确认对应 handler 已在目标节点的容器运行时中正确配置

## 参考链接

- [Kubernetes 官方文档：RuntimeClass](https://kubernetes.io/docs/concepts/containers/runtime-class/)
- [RuntimeClass API 参考](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/runtime-class-v1/)
- [Pod Overhead 概念文档](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-overhead/)
- [Assigning Pods to Nodes](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node/)
