# Ephemeral Containers

## 概述
Ephemeral（临时）容器是一种在现有 Pod 中临时运行的特殊容器，主要用于用户发起的故障排查操作（如调试），不适用于构建应用程序。

## 核心概念/原理
- **不可追加性**：Pod 创建后无法直接添加常规容器；临时容器提供了一种无需删除和重新创建 Pod 即可进行排查的机制。
- **无保障执行**：临时容器不保证资源或执行，且永远不会被自动重启。
- **API 方式创建**：通过 `ephemeralcontainers` 子资源创建，无法使用 `kubectl edit` 直接修改 `pod.spec`。
- **字段限制**：
  - 不允许设置 `ports`、`livenessProbe`、`readinessProbe`。
  - 不允许设置 `resources`（Pod 资源分配不可变）。
  - 创建后不可修改或移除。
- **不支持 Static Pods**。

## 关键机制或特性
- **调试利器**：当 `kubectl exec` 不足以排查问题时（如容器已崩溃或镜像未包含调试工具），可注入临时容器。
- **Distroless 镜像友好**：对于没有 shell 或调试工具的最小化镜像，临时容器是主要的现场调试手段。
- **进程命名空间共享**：建议启用进程命名空间共享（process namespace sharing），以便在临时容器中查看其他容器的进程。

## 使用场景
- 排查运行中 Pod 的疑难问题。
- 调试已崩溃或无法启动的容器。
- 为 distroless 或最小化镜像提供临时调试环境。

## 最佳实践/注意事项
- 临时容器仅用于交互式排查，不要将其纳入应用架构设计。
- 由于临时容器没有资源保障，避免在其上运行资源密集型操作。
- 临时容器一旦添加就无法移除，只能随 Pod 一起删除。
- 需要适当的 RBAC 权限才能创建临时容器。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/ephemeral-containers/
