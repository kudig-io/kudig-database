---
title: Ephemeral Containers
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ephemeral Containers 是什么
- 如何 Ephemeral Containers
trigger_keywords:
- Ephemeral
- Containers
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- **不支持 Static [[pods|Pods]]**。

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

## 实战操作示例

### 场景 1：为 Distroless 容器注入调试工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 向运行中的 Pod 注入一个带完整工具集的调试容器
# --target 参数共享目标容器的进程命名空间
kubectl debug -it myapp-pod \
  --image=nicolaka/netshoot:latest \
  --target=app \
  -n prod

# 在调试容器中可以：
# - 查看主容器进程: ps aux
# - 检查网络连通性: curl localhost:8080/healthz
# - 抓包分析: tcpdump -i eth0 port 8080
# - 检查 DNS: nslookup [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]].default
```
### 场景 2：调试已崩溃的容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Pod 副本进行调试（不影响原 Pod）
kubectl debug myapp-pod \
  --copy-to=myapp-debug \
  --container=app \
  --image=busybox:1.36 \
  -n prod -- sh

# 或者覆盖原容器的命令，阻止其崩溃，以便检查环境
kubectl debug myapp-pod \
  --copy-to=myapp-debug \
  --container=app \
  --set-image=app=busybox:1.36 \
  -n prod -- sleep 3600
```
### 场景 3：通过 API 创建临时容器（YAML）

```yaml
# 使用 kubectl patch 或 kubectl replace --subresource=ephemeralcontainers
apiVersion: v1
kind: EphemeralContainers
metadata:
  name: myapp-pod
ephemeralContainers:
- name: debugger
  image: busybox:1.36
  command: ["sh"]
  stdin: true
  tty: true
  targetContainerName: app               # 共享目标容器的进程命名空间
  securityContext:
    capabilities:
      add: ["SYS_PTRACE"]               # 允许 strace 等追踪工具
```

## 故障排查

### kubectl debug 命令不生效
- **症状**: `kubectl debug` 命令提示 `error: ephemeral containers are disabled`。
- **原因**: 集群版本低于 v1.25（临时容器在 v1.25 GA），或 API Server 禁用了该特性。
- **诊断命令**:
  ```bash
  # 检查集群版本
  kubectl version --short
  # 检查 API Server 是否支持临时容器
  kubectl api-resources | grep ephemeralcontainers
  ```

### 调试容器无法看到目标容器进程
- **症状**: 在调试容器中执行 `ps aux` 只能看到自身进程。
- **原因**: 未启用进程命名空间共享，或未使用 `--target` 参数。
- **解决方案**:
  ```bash
  # 确保使用 --target 指定目标容器
  kubectl debug -it <pod-name> --image=busybox --target=<container-name> -n prod
  ```
  如果 Pod 未启用 `shareProcessNamespace: true`，某些场景下 `--target` 仍可工作，但建议在 Pod spec 中显式设置。

### RBAC 权限不足
- **症状**: `Error from server (Forbidden): pods "<pod>" is forbidden: User "xxx" cannot patch resource "pods/ephemeralcontainers"`。
- **解决方案**: 确保用户具有以下 RBAC 权限：
  ```yaml
  - apiGroups: [""]
    resources: ["pods/ephemeralcontainers"]
    verbs: ["get", "update", "patch"]
  ```

## 生产检查清单

- [ ] 集群版本 >= v1.25（临时容器 GA）
- [ ] 运维人员已被授予 `pods/ephemeralcontainers` 的 RBAC 权限
- [ ] 标准调试镜像已预拉取到节点（如 `nicolaka/netshoot`、`busybox`）
- [ ] 了解临时容器不可删除的特性，避免在高安全环境中留下痕迹
- [ ] 审计日志已启用，记录临时容器的创建操作

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 注入调试容器到运行中的 Pod
kubectl debug -it <pod-name> --image=busybox:1.36 --target=<container-name> -n <namespace>

# 创建 Pod 副本进行调试（不影响原 Pod）
kubectl debug <pod-name> --copy-to=<debug-pod-name> --container=<container> --image=busybox -n <namespace> -- sh

# 查看 Pod 中的临时容器
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.ephemeralContainers}'

# 查看临时容器日志
kubectl logs <pod-name> -c <ephemeral-container-name> -n <namespace>

# 节点级调试（创建特权 Pod 访问节点文件系统）
kubectl debug node/<node-name> -it --image=ubuntu:22.04
```
## 交叉引用

- [Pod 综合故障排查手册](../../故障诊断/08-pod-comprehensive-troubleshooting.md)
- [高级 Pod 运维模式](../../工作负载/12-advanced-pod-patterns.md)
- [容器运行时接口 (CRI)](./container-runtime-interface-cri.md)
- [Pod 生命周期](./pod-lifecycle.md)
- [Pod 故障树分析 (FTA)](../../../19-%E6%95%85%E9%9A%9C%E8%AF%8A%E6%96%AD/06-FTA%E6%95%85%E9%9A%9C%E6%A0%91/list/pod-fta.md)

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/ephemeral-containers/

## Related

- [[17-系统基础/06-知识字典/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[17-系统基础/06-知识字典/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[17-系统基础/06-知识字典/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
