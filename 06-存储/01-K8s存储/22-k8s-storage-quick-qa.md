---
title: K8s 存储快问快答（61 题）
description: 面试向 K8s 存储快问快答速查：StorageClass、PVC、PV、动态/静态供给、绑定机制、CSI、volumeBindingMode、回收策略、访问模式、工作负载与排查命令
summary: 61 组问题+答案成对给出的 K8s 存储速查面经，覆盖 StorageClass/PVC/PV 联动、CSI 组件、绑定模式、生命周期、访问模式、StatefulSet 存储与故障排查命令
category: 存储
tags:
- k8s
- storage
- storageclass
- pvc
- pv
- csi
- interview
tier: core
created: '2026-08-31'
last_updated: 2026-08
difficulty: beginner
reading_level: beginner
audience:
- 面试候选人
- SRE
- 运维工程师
- Kubernetes 初学者
estimated_read_time: 15min
intent_queries:
- K8s 存储面试题快问快答
- StorageClass PVC PV 面试速查
- Kubernetes 存储常见问题
trigger_keywords:
- 快问快答
- 存储面试
- StorageClass
- PVC
- PV
prerequisites:
- kubectl-basics
- storage-basics
related_docs:
- path: 21-storageclass-pvc-pv-interview-notes.md
  type: related
  desc: StorageClass/PVC/PV 联动机制面经详解
- path: 02-pv-architecture-fundamentals.md
  type: depth
  desc: PV/PVC 核心概念与绑定机制
- path: 05-storageclass-dynamic-provisioning.md
  type: depth
  desc: StorageClass 动态供给流程
---

# K8s 存储快问快答（61 题）

> 姊妹篇：[[06-存储/01-K8s存储/21-storageclass-pvc-pv-interview-notes.md|StorageClass、PVC、PV 联动机制面经]]（联动机制详解版）。本文为速查版，问题与答案成对给出，适合面试前快速过一遍。

## 一、基础概念

### 1. StorageClass 创建后会自动创建 PV 吗？

**不会。**

StorageClass 只是存储模板，不代表真实存储资源。只有 PVC 使用该 StorageClass，并触发动态供给时，才会自动创建 PV。

### 2. PVC 是什么？

**PVC 是用户对存储的申请单。**

它声明：

- 需要多大容量；
- 访问模式是什么；
- 使用哪个 StorageClass；
- 文件系统卷还是块设备卷。

Pod 通常不直接使用 PV，而是引用 PVC。

### 3. PV 是什么？

**PV 是 Kubernetes 对真实存储资源的抽象。**

它背后可能对应：

- 云盘；
- NFS 目录；
- Ceph RBD；
- iSCSI；
- 本地磁盘；
- CSI 创建的卷。

PV 是集群级资源，不属于 namespace。

### 4. StorageClass 是什么？

**StorageClass 是动态供给的存储模板。**

它定义：

- 用哪个 provisioner；
- 创建什么类型的存储；
- 使用哪些参数；
- 回收策略；
- 是否允许扩容；
- 绑定时机。

### 5. PVC 和 PV 是什么关系？

**PVC 申请存储，PV 提供存储，两者一对一绑定。**

一个 PVC 只能绑定一个 PV，一个 PV 也只能被一个 PVC 绑定。

### 6. Pod、PVC、PV 的使用链路是什么？

**Pod → PVC → PV → 后端真实存储。**

Pod 不关心具体存储实现，只引用 PVC。

## 二、供给机制

### 7. 动态供给的流程是什么？

```text
创建 StorageClass
  → 创建 PVC
  → PVC 引用 StorageClass
  → provisioner/CSI Driver 创建后端卷
  → 自动创建 PV
  → PVC 与 PV 绑定
  → Pod 挂载使用
```

### 8. 静态供给的流程是什么？

```text
管理员提前创建真实存储
  → 管理员创建 PV
  → 用户创建 PVC
  → PVC 匹配已有 PV
  → PVC 与 PV 绑定
  → Pod 挂载使用
```

### 9. 动态供给和静态供给最大区别是什么？

**PV 由谁创建、什么时候创建。**

- 静态供给：管理员提前创建 PV；
- 动态供给：PVC 创建后由 provisioner 自动创建 PV。

### 10. PVC 创建后一定会创建新 PV 吗？

**不一定。**

如果已有合适 PV，PVC 会直接绑定已有 PV。只有没有合适 PV，并且 PVC 使用支持动态供给的 StorageClass，才会新建 PV。

## 三、绑定与匹配

### 11. PVC 和 PV 绑定看哪些条件？

主要看：

- 容量；
- accessModes；
- storageClassName；
- volumeMode；
- selector；
- PV 是否处于 Available 状态。

### 12. PVC 请求 10Gi，可以绑定 20Gi 的 PV 吗？

**可以。**

PV 容量只要大于等于 PVC 请求即可。但绑定后整个 PV 都归这个 PVC 使用，多出来的容量不会再分给其他 PVC。

### 13. PVC 和 PV 绑定后还能自动换绑吗？

**不会。**

绑定是稳定且排他的。绑定成功后，不会因为出现更合适的 PV 就自动换绑。

### 14. StorageClass 是 namespace 级资源吗？

**不是。**

StorageClass 是集群级资源。PVC 是 namespace 级资源。

### 15. PV 是 namespace 级资源吗？

**不是。**

PV 是集群级资源。它可以被某个 namespace 下的 PVC 绑定，但 PV 本身不属于 namespace。

### 16. PVC 是 namespace 级资源吗？

**是。**

PVC 属于某个 namespace，Pod 只能引用同 namespace 下的 PVC。

### 17. `storageClassName` 为空是什么意思？

要分情况：

- `storageClassName: ""`：明确表示不使用 StorageClass，只匹配没有 StorageClass 的静态 PV；
- 不写 `storageClassName`：可能会使用默认 StorageClass。

### 18. 默认 StorageClass 是什么？

默认 StorageClass 是带有这个注解的 StorageClass：

```yaml
storageclass.kubernetes.io/is-default-class: "true"
```

如果 PVC 没写 `storageClassName`，可能会自动使用默认 StorageClass。

### 19. 一个集群可以有多个默认 StorageClass 吗？

技术上可能出现，但不推荐。

如果有多个默认 StorageClass，PVC 默认选择行为可能导致混乱。生产环境应只保留一个默认 StorageClass。

## 四、CSI 与组件

### 20. `provisioner` 字段有什么用？

`provisioner` 指定由哪个存储插件负责创建卷。

例如：

```yaml
provisioner: diskplugin.csi.alibabacloud.com
```

它告诉 Kubernetes：这个 StorageClass 的动态卷由该 CSI 驱动创建。

### 21. CSI 是什么？

**CSI 是 Container Storage Interface。**

它是容器编排系统和存储系统之间的标准接口。Kubernetes 通过 CSI Driver 对接不同存储后端。

### 22. external-provisioner 是什么？

**external-provisioner 是 CSI 体系里的控制器组件。**

它监听 PVC，当发现需要动态供给时，调用 CSI Driver 的 `CreateVolume` 创建真实卷，再创建 PV 对象。

## 五、volumeBindingMode

### 23. PVC 一直 Pending，常见原因有哪些？

常见原因：

- 没有可匹配 PV；
- StorageClass 不存在；
- provisioner 不可用；
- CSI Driver 创建卷失败；
- accessModes 不支持；
- 容量不匹配；
- 拓扑限制不满足；
- 使用 `WaitForFirstConsumer`，但还没有 Pod 消费 PVC。

### 24. `volumeBindingMode: Immediate` 是什么？

**PVC 创建后立即创建或绑定 PV。**

```text
PVC 创建 → 立即供给/绑定 PV → Pod 后续调度
```

适合没有明显拓扑约束的存储。

### 25. `volumeBindingMode: WaitForFirstConsumer` 是什么？

**等第一个 Pod 使用 PVC 时，才创建或绑定 PV。**

```text
PVC 创建 → 等待 Pod → 调度器选择节点 → 根据节点拓扑创建/绑定 PV
```

适合多可用区云盘、本地盘、有拓扑约束的存储。

### 26. 为什么 `WaitForFirstConsumer` 很重要？

因为它能避免卷和 Pod 被放到不同拓扑区域。

例如云盘只能挂到同可用区节点，如果先创建卷，可能创建到 A 可用区，但 Pod 被调度到 B 可用区，导致挂载失败。

## 六、生命周期与回收

### 27. PV 有哪些生命周期状态？

常见状态：

- `Available`：可用，尚未绑定；
- `Bound`：已绑定 PVC；
- `Released`：PVC 删除了，但资源还没回收；
- `Failed`：回收失败。

### 28. PVC 有哪些常见状态？

常见状态：

- `Pending`：等待绑定；
- `Bound`：已绑定 PV；
- `Lost`：绑定的 PV 丢失或不可用。

### 29. `reclaimPolicy` 是什么？

**回收策略。**

决定 PVC 删除后，PV 和后端数据如何处理。

常见值：

- `Delete`；
- `Retain`。

### 30. `Delete` 回收策略是什么意思？

PVC 删除后，PV 通常会被删除，后端真实卷也会被删除。

动态供给的云盘类 StorageClass 常用这个策略。

### 31. `Retain` 回收策略是什么意思？

PVC 删除后，PV 和后端数据保留。

PV 通常进入 `Released` 状态，需要管理员手工清理或重新利用。

### 32. 生产环境什么时候用 `Retain`？

数据非常重要、不希望 PVC 删除就误删后端数据时，用 `Retain`。

比如数据库、关键业务持久化数据、审计数据等。

### 33. `allowVolumeExpansion` 是什么？

表示是否允许 PVC 扩容。

```yaml
allowVolumeExpansion: true
```

开启后，可以修改 PVC 请求容量进行扩容。

### 34. PVC 可以缩容吗？

**通常不支持缩容。**

Kubernetes 原生支持的是扩容，不支持安全缩容。缩容一般需要数据迁移后重建卷。

## 七、访问模式

### 35. `accessModes` 有哪些？

常见三种：

- `ReadWriteOnce`，简称 RWO；
- `ReadOnlyMany`，简称 ROX；
- `ReadWriteMany`，简称 RWX。

新版本还有 `ReadWriteOncePod`，简称 RWOP。

### 36. RWO 是什么？

**ReadWriteOnce。**

卷可以被一个节点以读写方式挂载。注意是一个节点，不一定是一个 Pod。

### 37. ROX 是什么？

**ReadOnlyMany。**

卷可以被多个节点以只读方式挂载。

### 38. RWX 是什么？

**ReadWriteMany。**

卷可以被多个节点以读写方式挂载。

常见支持 RWX 的后端有 NFS、CephFS、部分文件存储服务。

### 39. RWOP 是什么？

**ReadWriteOncePod。**

卷只能被一个 Pod 以读写方式挂载，比 RWO 更严格。

### 40. 云盘一般支持 RWX 吗？

通常不支持。

多数云盘是块存储，常见访问模式是 RWO。RWX 一般需要文件存储，如 NFS、NAS、CephFS。

## 八、工作负载与存储

### 41. StatefulSet 使用存储有什么特点？

StatefulSet 通常通过 `volumeClaimTemplates` 为每个副本自动创建独立 PVC。

```text
data-mysql-0
data-mysql-1
data-mysql-2
```

每个 Pod 有自己的持久化卷。

### 42. StatefulSet 删除后 PVC 会自动删除吗？

通常不会。

StatefulSet 删除后，PVC 默认保留，以避免数据误删。需要管理员或用户手工清理 PVC。

### 43. Deployment 适合挂载独占 PVC 吗？

不太适合多副本挂同一个 RWO PVC。

Deployment 副本可能调度到不同节点，RWO 卷可能无法同时挂载，容易出现挂载冲突。

### 44. 数据库一般用 Deployment 还是 StatefulSet？

通常用 StatefulSet。

因为数据库需要稳定网络标识、稳定存储、按序启动/停止等能力。

## 九、卷类型与对比

### 45. `volumeMode: Filesystem` 是什么？

表示把卷格式化成文件系统后挂载到容器目录。

这是最常见模式。

### 46. `volumeMode: Block` 是什么？

表示把卷作为裸块设备暴露给容器。

适合数据库、存储系统等需要直接管理块设备的场景。

### 47. PV 的 `claimRef` 是什么？

`claimRef` 记录 PV 绑定的 PVC 信息。

它表示这个 PV 已经被某个 PVC 占用。

### 48. 可以手工指定 PVC 绑定某个 PV 吗？

可以。

常见方式：

- PVC 使用 `volumeName` 指定 PV；
- 或者通过 label selector 匹配特定 PV。

### 49. PVC 删除后，Pod 会怎样？

如果 Pod 正在使用该 PVC，Kubernetes 的保护机制会阻止 PVC 立即删除。

PVC 会等到不再被 Pod 使用后再真正删除。

### 50. PV 删除后，后端数据一定删除吗？

不一定。

取决于：

- `reclaimPolicy`；
- 存储插件实现；
- PV 是否是动态供给；
- 后端存储系统行为。

### 51. emptyDir 和 PVC 有什么区别？

`emptyDir` 是临时存储，Pod 删除后数据消失。

PVC 是持久化存储，Pod 重建后仍可重新挂载使用。

### 52. hostPath 和 PV/PVC 有什么区别？

`hostPath` 直接挂宿主机路径，节点耦合强，生产风险高。

PV/PVC 是标准持久化抽象，可以对接不同存储后端，更适合生产。

### 53. ConfigMap/Secret 算 PV 吗？

不算。

ConfigMap/Secret 可以作为 volume 挂载，但它们不是 PV/PVC 持久化存储模型。

### 54. 本地存储 Local PV 有什么特点？

Local PV 使用节点本地磁盘，性能好，但强依赖节点。

Pod 必须调度到拥有该本地盘的节点上，通常配合 `WaitForFirstConsumer` 使用。

### 55. NFS 适合什么场景？

NFS 适合多 Pod 共享读写，也就是 RWX 场景。

但 NFS 的性能和一致性要看后端实现，不适合所有高性能数据库场景。

## 十、故障排查

### 56. 为什么 PVC 明明 Bound，Pod 还是挂载失败？

可能原因：

- 节点无法访问后端存储；
- CSI node plugin 异常；
- 权限问题；
- 文件系统损坏；
- 云盘和节点不在同可用区；
- 多节点同时挂载 RWO 卷；
- mountOptions 配置错误。

### 57. 查看 PVC 状态用什么命令？

```bash
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>
```

重点看 `Status`、`StorageClass`、`Volume`、`Events`。

### 58. 查看 PV 状态用什么命令？

```bash
kubectl get pv
kubectl describe pv <pv-name>
```

重点看 `Status`、`Claim`、`Reclaim Policy`、`StorageClass`、`Events`。

### 59. 查看 StorageClass 用什么命令？

```bash
kubectl get storageclass
kubectl describe storageclass <name>
```

也可以简写：

```bash
kubectl get sc
```

### 60. 存储问题排查顺序是什么？

```text
Pod Events
  → PVC 状态
  → PV 状态
  → StorageClass
  → CSI Controller
  → CSI Node
  → 后端存储系统
```

不要只看 Pod，很多存储问题根因在 PVC、PV 或 CSI 层。

## 十一、终极速记

### 61. 一句话记住整个体系

**StorageClass 定义怎么造卷。**

**PVC 负责申请卷。**

**PV 代表真实卷。**

**动态供给：PVC 触发创建 PV。**

**静态供给：管理员先创建 PV，PVC 再匹配。**
