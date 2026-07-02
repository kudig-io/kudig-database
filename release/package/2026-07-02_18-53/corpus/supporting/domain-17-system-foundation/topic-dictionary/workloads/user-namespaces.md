---
title: User Namespaces
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- containerd
- cri-o
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- User Namespaces 是什么
- 如何 User Namespaces
trigger_keywords:
- User
- Namespaces
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# User Namespaces

## 概述
用户命名空间（User Namespaces）是 Linux 的一项特性，用于将容器内的用户与主机（节点）上的用户隔离。容器内以 root 运行的进程，在主机上可映射为非 root 用户，从而显著降低容器逃逸后对主机或其他 Pod 的危害。

## 核心概念/原理
- **启用方式**：Pod 通过设置 `pod.spec.hostUsers: false` 来启用用户命名空间（默认 `true`，即与主机共享用户命名空间）。
- **UID/GID 映射**：[[kubelet|kubelet]] 会为每个 Pod 分配唯一的主机 UID/GID 映射范围，确保同一节点上不同 Pod 的映射不重叠。
- **Capabilities 隔离**：授予 Pod 的 capabilities 仅在 Pod 的用户命名空间内有效，在宿主机上基本无效。例如：
  - `CAP_SYS_MODULE` 无法加载内核模块。
  - `CAP_SYS_ADMIN` 仅限 Pod 内部使用。
- **卷挂载行为**：`runAsUser`、`runAsGroup`、`fsGroup` 等字段始终指容器内部用户；Pod 挂载卷时看到的文件所有权与未启用用户命名空间时一致。
- **默认 UID/GID 范围**：启用后，有效范围为 0–65535；超出此范围的文件将显示为 overflow ID（通常为 65534）。

## 关键机制或特性
- **节点要求**：
  - Linux 6.3+（tmpfs 支持 idmap 挂载）。
  - 文件系统（如 ext4、xfs、btrfs、overlayfs、tmpfs）支持 idmap 挂载。
  - OCI 运行时：crun ≥1.9（推荐 ≥1.13）或 runc ≥1.2。
  - CRI 运行时：[[containerd|containerd]] ≥2.0 或 CRI-O ≥1.25。
- **kubelet 子 ID 配置**：
  - 系统需存在 `kubelet` 用户。
  - 需安装 `getsubids`（shadow-utils）。
  - `/etc/subuid` 和 `/etc/subgid` 中需为 `kubelet` 用户配置 subordinate ID 范围。
  - 起始 ID 必须是 65536 的倍数且 ≥65536；数量至少为 `65536 × maxPods`。
- **每 Pod ID 数量**：自 v1.33 起，可通过 `KubeletConfiguration` 的 `userNamespaces.idsPerPod` 配置（默认 65536，必须是 65536 的倍数）。
- **Pod 安全准入放宽（Alpha）**：启用用户命名空间的 Linux Pod，Pod Security Standards 对 `runAsNonRoot`、`runAsUser`、`procMount` 等字段的约束会适当放宽。
- **限制**：启用用户命名空间时，不允许同时使用 `hostNetwork`、`hostIPC`、`hostPID`，也不允许使用 `volumeDevices`（原始块设备）。

## 使用场景
- 运行需要 root 权限但希望降低宿主机风险的应用。
- 对安全要求较高的多租户环境。
- 使用 distroless 或最小化镜像时，进一步增强隔离。

## 最佳实践/注意事项
- 确保节点内核、文件系统、运行时均支持 idmap 挂载后再启用此特性。
- 配置好 `/etc/subuid` 和 `/etc/subgid`，避免 ID 范围重叠。
- 需要访问主机命名空间或块设备的 Pod 无法启用用户命名空间。
- 可通过 kubelet 指标 `started_user_namespaced_pods_total` 和 `started_user_namespaced_pods_errors_total` 监控使用情况。

## 实战 YAML 示例

### 启用用户命名空间的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: userns-demo
  namespace: prod
spec:
  hostUsers: false                           # 启用用户命名空间隔离
  containers:
  - name: app
    image: myregistry.com/myapp:v1.0
    command: ["sleep", "infinity"]
    securityContext:
      runAsUser: 0                           # 容器内 root，但主机上映射为非特权用户
      capabilities:
        add: ["NET_BIND_SERVICE"]            # 仅在 Pod 用户命名空间内有效
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
```

### 节点配置前置条件

```bash
# 1. 检查内核版本（需要 >= 6.3）
uname -r

# 2. 创建 kubelet 用户（如不存在）
useradd -r -s /sbin/nologin kubelet

# 3. 配置 subordinate ID 范围
# 格式: kubelet:起始ID:数量
# 起始ID 必须是 65536 的倍数且 >= 65536
# 数量 >= 65536 * maxPods (默认 maxPods=110)
echo "kubelet:65536:7208960" >> /etc/subuid
echo "kubelet:65536:7208960" >> /etc/subgid

# 4. 验证配置
getsubids kubelet
```

## 故障排查

### Pod 创建失败：用户命名空间不支持
- **症状**: Pod 事件显示 `failed to create user namespace` 或 `hostUsers: false is not supported`。
- **常见原因**: 节点内核版本过低；容器运行时不支持；`/etc/subuid` 和 `/etc/subgid` 未配置。
- **诊断命令**:
  ```bash
  # 检查节点内核版本
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}: {.status.nodeInfo.kernelVersion}{"\n"}{end}'
  
  # 检查容器运行时版本
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}: {.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'
  
  # 在节点上检查 subuid/subgid 配置
  # (需要 SSH 到节点)
  cat /etc/subuid | grep kubelet
  cat /etc/subgid | grep kubelet
  ```

### 与 hostNetwork 等特性冲突
- **症状**: Pod 创建失败，提示 `hostUsers: false` 与 `hostNetwork/hostPID/hostIPC` 不兼容。
- **解决方案**: 需要主机命名空间的 Pod 不能启用用户命名空间。这类 Pod 应通过其他方式加固安全（如 `readOnlyRootFilesystem`、最小 capabilities）。

### 文件权限问题
- **症状**: 容器内无法访问挂载的卷文件，提示 `Permission denied`。
- **常见原因**: 卷文件的 UID/GID 不在映射范围内。
- **解决方案**: 确保文件系统支持 idmap 挂载；检查 `fsGroup` 配置是否正确。

## 生产检查清单

- [ ] 节点内核版本 >= 6.3
- [ ] 容器运行时支持用户命名空间（containerd >= 2.0 或 CRI-O >= 1.25）
- [ ] `/etc/subuid` 和 `/etc/subgid` 已正确配置
- [ ] 不与 `hostNetwork`/`hostPID`/`hostIPC`/`volumeDevices` 同时使用
- [ ] 监控 kubelet 指标 `started_user_namespaced_pods_total`
- [ ] 了解 capabilities 在用户命名空间内的限制行为

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Pod 是否启用了用户命名空间
kubectl get pod <pod-name> -n prod -o jsonpath='{.spec.hostUsers}'

# 验证容器内用户映射（在容器内执行）
kubectl exec <pod-name> -n prod -- id
kubectl exec <pod-name> -n prod -- cat /proc/self/uid_map
kubectl exec <pod-name> -n prod -- cat /proc/self/gid_map

# 查看 kubelet 用户命名空间指标
kubectl get --raw /api/v1/nodes/<node-name>/proxy/metrics | grep user_namespace
```
## 交叉引用

- [高级 Pod 配置](./advanced-pod-configuration.md)
- [Pod 安全相关](../../domain-05-security-compliance/01-authentication-deep-dive.md)
- [容器运行时接口 (CRI)](./container-runtime-interface-cri.md)
- [运行时类 (RuntimeClass)](./runtime-class.md)
- [Pods 基础](./pods.md)

## 参考链接
- https://[[entities/kubernetes.md|kubernetes]].io/docs/concepts/workloads/pods/user-namespaces/

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/advanced-pod-configuration.md|Advanced Pod Configuration]]
- [[domain-17-system-foundation/topic-dictionary/workloads/automatic-cleanup-for-finished-jobs.md|Automatic Cleanup for Finished Jobs]]
- [[domain-17-system-foundation/topic-dictionary/workloads/autoscaling-workloads.md|Autoscaling Workloads]]


<!-- risk-assessed -->
