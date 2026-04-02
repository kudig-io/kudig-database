# Managing Workloads

## 概述
本页介绍在 Kubernetes 中部署应用后，如何使用各种工具和实践来管理、更新和扩展工作负载，涵盖 kubectl 批量操作、应用更新、金丝雀发布、资源注解和扩缩容等内容。

## 核心概念/原理
- **资源配置组织**：将同一微服务的相关资源（如 Deployment + Service）放在同一个 YAML 文件中，用 `---` 分隔，便于统一管理。
- **kubectl 批量操作**：
  - `kubectl apply -f <dir> --recursive`：递归处理目录下的所有清单文件。
  - `kubectl delete -f <file>` 或 `kubectl delete <resource>/<name>`：删除资源。
  - 通过标签选择器 `-l` 进行批量过滤和操作。
  - 利用 `xargs` 或命令替换 `$()` 链式操作资源。
- **应用更新**：
  - 使用 Deployment、DaemonSet、StatefulSet 的滚动更新机制，逐步将流量切换到新版本的 Pod。
  - `kubectl rollout` 系列命令用于管理、暂停、恢复和查看更新进度。
  - `kubectl patch`、`kubectl edit`、`kubectl apply` 用于对资源进行原地更新。
  - 对于不可变字段的修改，可使用 `kubectl replace --force`（先删除再重建）。

## 关键机制或特性
- **金丝雀部署（Canary Deployment）**：通过为不同版本设置不同标签（如 `track: stable` 和 `track: canary`），让 Service 同时覆盖两组 Pod，逐步将流量导向新版本。
- **自动扩缩容**：
  - `kubectl scale`：手动调整副本数。
  - `kubectl autoscale`：创建 HorizontalPodAutoscaler，根据 CPU 利用率等指标自动扩缩容。
- **原地更新（In-place Updates）**：
  - `kubectl apply`：基于声明式配置进行差异更新，推荐与版本控制配合使用。
  - `kubectl edit`：交互式编辑资源。
  - `kubectl patch`：支持 JSON patch、JSON merge patch 和 strategic merge patch。

## 使用场景
- 日常应用的生命周期管理、版本发布和回滚。
- 需要零停机更新的生产环境。
- 根据负载动态调整应用容量的自动扩缩容场景。
- 通过金丝雀发布验证新版本稳定性。

## 最佳实践/注意事项
- 将同一应用的相关资源放在同一个目录或文件中，并使用版本控制管理清单。
- 使用 `kubectl apply` 而非直接 `replace`，以保留自动化字段（如 `resourceVersion`）。
- 滚动更新时，设置合理的 `maxSurge` 和 `maxUnavailable`，平衡可用性和更新速度。
- 使用 HPA 时，建议从 Deployment/StatefulSet 的 manifest 中移除 `spec.replicas`，避免 `kubectl apply` 与 HPA 冲突。
- 对于破坏性更新（需修改不可变字段），使用 `replace --force` 并确认业务影响。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/management/
