# 14 - Namespace、资源管理与垃圾回收事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 Namespace 管理、垃圾回收、ResourceQuota、LimitRange 和 PDB 相关的所有事件。**

---

## 目录

- [一、事件总览](#一事件总览)
- [二、Namespace 生命周期与终止流程](#二namespace-生命周期与终止流程)
- [三、垃圾回收机制与 OwnerReferences](#三垃圾回收机制与-ownerreferences)
- [四、Namespace Controller 事件](#四namespace-controller-事件)
- [五、Garbage Collector 事件](#五garbage-collector-事件)
- [六、ResourceQuota 事件](#六resourcequota-事件)
- [七、LimitRange 事件](#七limitrange-事件)
- [八、PodDisruptionBudget 事件](#八poddisruptionbudget-事件)
- [九、典型排查场景](#九典型排查场景)
- [十、生产环境最佳实践](#十生产环境最佳实践)

---

## 一、事件总览

### 1.1 本文档覆盖的事件列表

| 事件原因 (Reason) | 类型 | 来源组件 | 生产频率 | 适用版本 | 简要说明 |
|:---|:---|:---|:---|:---|:---|
| **Namespace Controller** |
| `NamespaceDeletionContentFailure` | Warning | namespace-controller | 低频 | v1.0+ | Namespace 内容删除失败 |
| `NamespaceContentRemaining` | Normal | namespace-controller | 中频 | v1.0+ | Namespace 仍有资源未删除 |
| `NamespaceFinalizersRemaining` | Normal | namespace-controller | 中频 | v1.0+ | Namespace 仍有 finalizers 未处理 |
| **Garbage Collector** |
| `DeletingDependents` | Normal | garbage-collector | 中频 | v1.5+ | 正在删除依赖对象 |
| `GracefulDeletion` | Normal | garbage-collector | 中频 | v1.5+ | 开始优雅删除 |
| `OrphanFinal` | Normal | garbage-collector | 低频 | v1.7+ | 孤立依赖对象（删除 owner 但保留依赖） |
| **ResourceQuota** |
| `FailedQuota` | Warning | quota | 中频 ⚠️ | v1.1+ | 超过资源配额限制 |
| `FailedQuotaCheck` | Warning | quota | 罕见 | v1.1+ | 配额检查失败 |
| **LimitRange** |
| `LimitRangeDefaults` | Normal | limitranger | 高频 | v1.0+ | 应用默认资源限制 |
| `InvalidLimitRange` | Warning | limitranger | 罕见 | v1.0+ | LimitRange 配置无效 |
| **PodDisruptionBudget** |
| `NoPods` | Warning | disruption-controller | 低频 | v1.5+ | PDB 未找到匹配的 Pod |
| `CalculateExpectedPodCountFailed` | Warning | disruption-controller | 罕见 | v1.5+ | 计算期望 Pod 数失败 |
| `Stale` | Warning | disruption-controller | 低频 | v1.5+ | PDB 状态过期 |
| `DisruptionAllowed` | Normal | disruption-controller | 中频 | v1.21+ | 允许驱逐 Pod |
| `InsufficientBudget` | Warning | disruption-controller | 中频 | v1.5+ | 预算不足，无法驱逐 |

### 1.2 快速索引

| 问题场景 | 关注事件 | 跳转章节 |
|:---|:---|:---|
| Namespace 无法删除（卡在 Terminating） | `NamespaceFinalizersRemaining` | [四.3](#43-namespacefinalizersremaining---finalizers-未处理完成) |
| Namespace 删除慢 | `NamespaceContentRemaining` | [四.2](#42-namespacecontentremaining---namespace-仍有资源未删除) |
| Pod 创建失败，提示超过配额 | `FailedQuota` | [六.1](#61-failedquota---超过资源配额) |
| 无法驱逐 Pod（drain 失败） | `InsufficientBudget` | [八.5](#85-insufficientbudget---预算不足无法驱逐) |
| 级联删除未生效 | `DeletingDependents` | [五.1](#51-deletingdependents---正在删除依赖对象) |
| Pod 被自动设置资源限制 | `LimitRangeDefaults` | [七.1](#71-limitrangedefaults---应用默认资源限制) |

---

## 二、Namespace 生命周期与终止流程

### 2.1 Namespace 状态机

```
Namespace 生命周期
═══════════════════════════════════════════════════════════════

┌──────────────┐
│    Active    │ ◀─── Namespace 正常运行
└──────┬───────┘
       │
       │ [用户执行 kubectl delete namespace]
       │
       ▼
┌──────────────┐
│ Terminating  │ ◀─── Namespace 正在删除（status.phase: Terminating）
│              │      metadata.deletionTimestamp 被设置
└──────┬───────┘
       │
       │ [namespace-controller 执行清理流程]
       │
       ├─▶ 1. 列举所有命名空间资源
       │   └─▶ NamespaceContentRemaining (如有剩余资源)
       │
       ├─▶ 2. 删除所有可删除资源
       │   ├─▶ Pods
       │   ├─▶ Services
       │   ├─▶ Deployments, ReplicaSets
       │   ├─▶ ConfigMaps, Secrets
       │   └─▶ PVCs, etc.
       │
       ├─▶ 3. 等待所有资源删除完成
       │   └─▶ NamespaceDeletionContentFailure (如删除失败)
       │
       ├─▶ 4. 处理 finalizers
       │   └─▶ NamespaceFinalizersRemaining (如有 finalizers)
       │
       ├─▶ 5. 所有 finalizers 处理完成
       │   └─▶ 移除 metadata.finalizers
       │
       ▼
  [Namespace 被彻底删除，从 etcd 移除]
```

### 2.2 Namespace 删除的四个阶段

**阶段 1: 标记删除** (用户操作)
```bash
$ kubectl delete namespace my-namespace

# Namespace 状态变化:
# - metadata.deletionTimestamp: 设置为当前时间
# - status.phase: "Terminating"
# - Namespace 不再接受新资源创建
```

**阶段 2: 内容删除** (namespace-controller)
```
namespace-controller 工作流程:
1. 发现 Namespace 的 deletionTimestamp 已设置
2. 列举该 Namespace 下所有 API 资源
3. 对每个资源发起删除请求 (DELETE)
4. 等待资源数量降为 0
5. 产生事件: NamespaceContentRemaining (如有剩余)
```

**阶段 3: Finalizers 处理** (各个 controller)
```
常见 Finalizers:
- kubernetes                           # 默认 finalizer，确保资源清理完成
- kubernetes.io/pv-protection          # PV 保护（防止删除使用中的 PV）
- example.com/custom-finalizer         # 自定义 finalizer（CR 清理逻辑）

处理流程:
1. namespace-controller 检查 metadata.finalizers 列表
2. 等待外部 controller 移除其 finalizer
3. 所有 finalizer 移除后，Namespace 才能被删除
4. 产生事件: NamespaceFinalizersRemaining (如等待过久)
```

**阶段 4: 最终删除** (kube-apiserver)
```
1. metadata.finalizers 为空
2. 所有资源已删除
3. kube-apiserver 从 etcd 删除 Namespace 对象
```

### 2.3 Namespace Terminating 卡住的常见原因

| 卡住原因 | 现象 | 排查方法 | 典型场景 |
|:---|:---|:---|:---|
| **资源删除失败** | `NamespaceDeletionContentFailure` | 查看事件 message | API 资源的 finalizer 未处理 |
| **Finalizer 未移除** | `NamespaceFinalizersRemaining` | 查看 `metadata.finalizers` | CR 的 controller 已删除或异常 |
| **PV 保护** | Namespace 卡住 | 检查 PVC/PV 状态 | PVC 仍绑定 PV，无法删除 |
| **Webhook 拦截** | 删除请求被拒绝 | 查看 kube-apiserver 日志 | ValidatingWebhook 阻止删除 |
| **API Server 故障** | 删除请求超时 | 检查 API Server 健康 | etcd 故障或网络问题 |

---

## 三、垃圾回收机制与 OwnerReferences

### 3.1 OwnerReferences 详解

Kubernetes 使用 `ownerReferences` 字段建立资源间的从属关系，实现**级联删除**（Cascade Deletion）。

**OwnerReferences 结构**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-pod
  ownerReferences:
    - apiVersion: apps/v1
      kind: ReplicaSet
      name: my-replicaset-abc123
      uid: 12345678-1234-1234-1234-123456789abc
      controller: true           # 标识为控制器引用
      blockOwnerDeletion: true   # 阻止 owner 删除直到该对象被删除
```

**OwnerReferences 字段含义**:
- `apiVersion`, `kind`, `name`, `uid`: 唯一标识 owner 对象
- `controller: true`: 表示这是**控制器引用**（一个对象只能有一个 controller owner）
- `blockOwnerDeletion: true`: 阻止 owner 被删除，直到该 dependent 被删除（用于关键资源保护）

### 3.2 垃圾回收的三种删除策略

**1. Foreground 删除（前台删除）**

```bash
$ kubectl delete deployment my-app --cascade=foreground

流程:
1. Deployment 被标记删除，设置 deletionTimestamp
2. Deployment 添加 finalizer: "foregroundDeletion"
3. Garbage Collector 删除所有 dependents (ReplicaSet, Pod)
4. 等待所有 dependents 删除完成
5. 移除 "foregroundDeletion" finalizer
6. 删除 Deployment
```

**事件流**:
```
[Deployment] → DeletingDependents (开始删除 ReplicaSet)
[ReplicaSet] → DeletingDependents (开始删除 Pods)
[Pods]       → Killing, Deleted
[ReplicaSet] → Deleted
[Deployment] → Deleted
```

**2. Background 删除（后台删除，默认）**

```bash
$ kubectl delete deployment my-app
# 等同于:
$ kubectl delete deployment my-app --cascade=background

流程:
1. Deployment 立即被删除
2. Garbage Collector 在后台异步删除 dependents
3. ReplicaSets 和 Pods 逐步被删除
```

**事件流**:
```
[Deployment] → Deleted (立即删除)
[Background] → DeletingDependents (异步删除 ReplicaSet)
[Background] → DeletingDependents (异步删除 Pods)
```

**3. Orphan 删除（孤立删除）**

```bash
$ kubectl delete deployment my-app --cascade=orphan

流程:
1. 从 dependents 的 ownerReferences 中移除 owner 引用
2. 删除 Deployment
3. ReplicaSets 和 Pods 变为孤立对象（orphaned），不被删除
```

**事件流**:
```
[Deployment] → OrphanFinal (孤立依赖对象)
[Deployment] → Deleted
[ReplicaSet, Pods] → 保留，变为孤立对象
```

**使用场景对比**:

| 策略 | 删除速度 | Owner 删除时机 | 适用场景 |
|:---|:---|:---|:---|
| **Background** | 快 | 立即删除 | **默认选择**，适合大部分场景 |
| **Foreground** | 慢 | 等待所有 dependents 删除后 | 需要保证 dependents 先删除的场景（如数据一致性） |
| **Orphan** | 最快 | 立即删除 | 只删除控制器，保留 Pod/PVC 等资源 |

### 3.3 垃圾回收器工作原理

```
Garbage Collector 架构
═══════════════════════════════════════════════════════════

┌─────────────────────────────────────────────────────────┐
│          Garbage Collector Controller                   │
│  (kube-controller-manager 的一部分)                     │
└─────────────────┬───────────────────────────────────────┘
                  │
                  ├─▶ GraphBuilder (图构建器)
                  │   ├─ 监听所有 API 资源的变化
                  │   ├─ 根据 ownerReferences 构建依赖图
                  │   └─ 维护 owner → dependents 关系
                  │
                  ├─▶ Garbage Collector Worker
                  │   ├─ 检测 owner 是否被删除 (deletionTimestamp 已设置)
                  │   ├─ 根据删除策略执行级联删除
                  │   │   ├─ Foreground: 先删 dependents，后删 owner
                  │   │   ├─ Background: 先删 owner，异步删 dependents
                  │   │   └─ Orphan: 移除引用，保留 dependents
                  │   └─ 产生事件: DeletingDependents, OrphanFinal
                  │
                  └─▶ 孤立对象处理器 (Orphan Handler)
                      └─ 处理 ownerReferences 指向不存在对象的资源
```

**关键逻辑**:
1. **监听资源变化**: GraphBuilder 通过 informer 监听所有资源的 CREATE/UPDATE/DELETE 事件
2. **构建依赖图**: 解析 `ownerReferences` 字段，维护 owner → dependents 的多对多关系图
3. **检测删除**: 当发现资源的 `deletionTimestamp` 不为空，触发垃圾回收逻辑
4. **执行删除**: 根据 `finalizers` 中的删除策略执行相应操作

---

## 四、Namespace Controller 事件

### 4.1 `NamespaceDeletionContentFailure` - Namespace 内容删除失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | namespace-controller |
| **关联资源** | Namespace |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

namespace-controller 在尝试删除 Namespace 中的资源时遇到错误，导致某些资源无法被删除。这会阻止 Namespace 的删除流程。

**触发条件**:
- API 资源类型已删除（CRD 被删除），但实例仍存在
- 资源的 finalizer 阻止删除
- Validating Webhook 拒绝删除请求
- API Server 返回错误（如网络超时、权限不足）

#### 典型事件消息

```bash
$ kubectl describe namespace my-namespace

Events:
  Type     Reason                           Age   From                  Message
  ----     ------                           ----  ----                  -------
  Warning  NamespaceDeletionContentFailure  10s   namespace-controller  Failed to delete content: unable to delete resource "customresources.example.com": the server could not find the requested resource
  Warning  NamespaceDeletionContentFailure  10s   namespace-controller  Failed to delete content: resource "pods" is forbidden: User "system:serviceaccount:kube-system:namespace-controller" cannot delete resource "pods" in namespace "my-namespace"
```

**message 格式**:
```
Failed to delete content: <具体错误原因>
```

#### 影响面说明

- **用户影响**: 高 - Namespace 无法删除，卡在 Terminating 状态
- **服务影响**: 中 - 如果是测试环境，会占用集群资源
- **集群影响**: 低 - 不影响其他 Namespace，但会占用 etcd 空间
- **关联事件链**: `NamespaceDeletionContentFailure` → `NamespaceContentRemaining` (持续等待)

#### 排查建议

```bash
# 1. 查看 Namespace 状态
kubectl get namespace my-namespace -o yaml

# 检查关键字段:
# - status.phase: Terminating
# - metadata.deletionTimestamp: 已设置
# - metadata.finalizers: 查看是否有 "kubernetes" finalizer

# 2. 查看 Namespace 中剩余的资源
kubectl api-resources --verbs=list --namespaced -o name | \
  xargs -n 1 kubectl get --show-kind --ignore-not-found -n my-namespace

# 3. 查看错误事件的完整信息
kubectl get events -n my-namespace --field-selector type=Warning,reason=NamespaceDeletionContentFailure

# 4. 查看 namespace-controller 日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep "my-namespace"

# 5. 检查是否有 CRD 被删除但实例仍存在
kubectl get crd
kubectl api-resources --verbs=list --namespaced -o name | grep -v "^/"
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **could not find the requested resource** | CRD 已删除但 CR 仍存在 | `kubectl get crd` | 手动删除残留的 CR 或移除 finalizer |
| **is forbidden: User cannot delete** | RBAC 权限不足 | 检查 namespace-controller 的权限 | 修复 ClusterRole 权限配置 |
| **connection refused** | API Server 不可达 | `kubectl get --raw /healthz` | 检查 API Server 和网络连接 |
| **context deadline exceeded** | 删除请求超时 | 检查 etcd 性能 | 优化 etcd 或增加超时时间 |
| **webhook denied the request** | ValidatingWebhook 拦截 | 查看 webhook 配置 | 调整 webhook 规则或暂时禁用 |

**典型案例解决方案**:

**案例 1: CRD 已删除，CR 实例残留**

```bash
# 现象: could not find the requested resource "mycustomresources.example.com"

# 排查:
kubectl get crd mycustomresources.example.com
# 输出: Error from server (NotFound): customresourcedefinitions.apiextensions.k8s.io "mycustomresources.example.com" not found

# 尝试列举资源（会失败）
kubectl get mycustomresources -n my-namespace
# 输出: error: the server doesn't have a resource type "mycustomresources"

# 解决方案 1: 重新创建 CRD，删除 CR，再删除 CRD
kubectl apply -f my-crd.yaml
kubectl delete mycustomresources --all -n my-namespace
kubectl delete crd mycustomresources.example.com

# 解决方案 2: 直接移除 Namespace 的 finalizers (强制删除，谨慎操作)
kubectl get namespace my-namespace -o json | \
  jq '.spec.finalizers = []' | \
  kubectl replace --raw /api/v1/namespaces/my-namespace/finalize -f -
```

**案例 2: 资源被 Webhook 拦截**

```bash
# 现象: webhook "validator.example.com" denied the request

# 排查:
kubectl get validatingwebhookconfiguration
kubectl describe validatingwebhookconfiguration my-webhook

# 查看 webhook 的 namespaceSelector
kubectl get validatingwebhookconfiguration my-webhook -o jsonpath='{.webhooks[*].namespaceSelector}'

# 解决方案 1: 暂时禁用 webhook（在删除 Namespace 期间）
kubectl delete validatingwebhookconfiguration my-webhook

# 或修改 webhook，排除 Terminating 状态的 Namespace
kubectl patch validatingwebhookconfiguration my-webhook --type='json' -p='[
  {
    "op": "add",
    "path": "/webhooks/0/namespaceSelector/matchExpressions",
    "value": [{
      "key": "kubernetes.io/metadata.name",
      "operator": "NotIn",
      "values": ["my-namespace"]
    }]
  }
]'

# 解决方案 2: 强制删除 Namespace（移除 finalizers）
kubectl patch namespace my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge
```

**案例 3: 资源有 finalizer 无法删除**

```bash
# 现象: Pod 无法删除，卡在 Terminating

# 排查:
kubectl get pods -n my-namespace -o json | jq -r '.items[] | select(.metadata.deletionTimestamp != null) | "\(.metadata.name) \(.metadata.finalizers)"'
# 输出: my-pod ["example.com/custom-finalizer"]

# 解决: 移除 Pod 的 finalizer
kubectl patch pod my-pod -n my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge

# 批量移除所有 Pod 的 finalizers
kubectl get pods -n my-namespace -o name | \
  xargs -I {} kubectl patch {} -n my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge
```

---

### 4.2 `NamespaceContentRemaining` - Namespace 仍有资源未删除

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | namespace-controller |
| **关联资源** | Namespace |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

namespace-controller 正在等待 Namespace 中的资源被删除。这是正常的删除过程事件，用于跟踪删除进度。

**事件产生时机**:
- namespace-controller 每隔 5 秒检查一次 Namespace 中的资源
- 如果仍有资源存在，产生此事件并记录剩余资源类型和数量

#### 典型事件消息

```bash
$ kubectl describe namespace my-namespace

Events:
  Type    Reason                      Age   From                  Message
  ----    ------                      ----  ----                  -------
  Normal  NamespaceContentRemaining   10s   namespace-controller  Some resources are remaining: pods has 3 resource instances, services has 1 resource instances
  Normal  NamespaceContentRemaining   5s    namespace-controller  Some resources are remaining: pods has 1 resource instances
```

**message 格式**:
```
Some resources are remaining: <resource-type> has <count> resource instances[, ...]
```

#### 影响面说明

- **用户影响**: 低 - 正常删除过程，需要耐心等待
- **服务影响**: 无
- **集群影响**: 无
- **关联事件链**: `NamespaceContentRemaining` (循环) → (所有资源删除) → Namespace 删除完成

#### 排查建议

```bash
# 1. 查看 Namespace 中剩余的资源类型
kubectl api-resources --verbs=list --namespaced -o name | \
  xargs -n 1 kubectl get --show-kind --ignore-not-found -n my-namespace

# 2. 查看 Terminating 状态的 Pod
kubectl get pods -n my-namespace --field-selector status.phase=Terminating

# 3. 查看 Finalizing 状态的资源
kubectl get all -n my-namespace -o json | \
  jq -r '.items[] | select(.metadata.deletionTimestamp != null) | "\(.kind)/\(.metadata.name)"'

# 4. 查看资源的 finalizers
kubectl get pods -n my-namespace -o json | \
  jq -r '.items[] | select(.metadata.finalizers != null) | "\(.metadata.name): \(.metadata.finalizers)"'
```

#### 解决建议

**正常情况**: 耐心等待资源删除完成（通常 30-60 秒）

**如果等待时间过长** (超过 5 分钟):

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Pod 卡在 Terminating | `terminationGracePeriodSeconds` 过长 | 强制删除: `kubectl delete pod --grace-period=0 --force` |
| PVC 无法删除 | PV 仍处于 Bound 状态 | 删除 PV: `kubectl delete pv <pv-name>` |
| Service 无法删除 | LoadBalancer 云资源未释放 | 检查云平台负载均衡器状态 |
| 自定义资源卡住 | CR 的 finalizer 未处理 | 移除 finalizer 或重启相关 controller |

**加速 Namespace 删除的方法**:
```bash
# 1. 并行强制删除所有 Pod (谨慎操作！)
kubectl delete pods --all -n my-namespace --grace-period=0 --force

# 2. 删除所有有 finalizer 的资源的 finalizer
for resource in $(kubectl api-resources --verbs=delete --namespaced -o name); do
  kubectl get $resource -n my-namespace -o name | \
    xargs -I {} kubectl patch {} -n my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge 2>/dev/null
done

# 3. 直接移除 Namespace 的 finalizers (最终手段)
kubectl patch namespace my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge
```

---

### 4.3 `NamespaceFinalizersRemaining` - Finalizers 未处理完成

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | namespace-controller |
| **关联资源** | Namespace |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |

#### 事件含义

Namespace 的所有资源已删除，但 `metadata.finalizers` 列表中仍有 finalizer 未被移除，导致 Namespace 无法彻底删除。

**常见 Finalizers**:

| Finalizer | 添加者 | 作用 | 移除时机 |
|:---|:---|:---|:---|
| `kubernetes` | namespace-controller | 确保 Namespace 内资源清理完成 | 所有资源删除后自动移除 |
| `controller.cattle.io/namespace-auth` | Rancher | Rancher 权限清理 | Rancher controller 清理完成后 |
| `kubernetes.io/pv-protection` | pv-protection-controller | 保护使用中的 PV | PVC 删除后自动移除 |
| 自定义 finalizer | 自定义 controller | 清理外部资源（如云资源） | 自定义 controller 处理完成后 |

#### 典型事件消息

```bash
$ kubectl describe namespace my-namespace

Events:
  Type    Reason                        Age   From                  Message
  ----    ------                        ----  ----                  -------
  Normal  NamespaceFinalizersRemaining  10s   namespace-controller  Some finalizers are remaining: ["example.com/my-finalizer"]
```

**message 格式**:
```
Some finalizers are remaining: ["<finalizer-1>", "<finalizer-2>", ...]
```

**Namespace YAML 状态**:
```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: my-namespace
  deletionTimestamp: "2026-02-10T12:00:00Z"  # 已标记删除
  finalizers:
    - example.com/my-finalizer                # 未移除的 finalizer
status:
  phase: Terminating
```

#### 影响面说明

- **用户影响**: 高 - Namespace 卡在 Terminating 状态，无法删除
- **服务影响**: 中 - 该 Namespace 无法重建（名称冲突）
- **集群影响**: 低 - 占用 etcd 空间，影响集群整洁度
- **关联事件链**: `NamespaceFinalizersRemaining` (循环) → (finalizer 未移除) → Namespace 永久卡住

#### 排查建议

```bash
# 1. 查看 Namespace 的 finalizers 列表
kubectl get namespace my-namespace -o jsonpath='{.metadata.finalizers}'
# 输出: ["example.com/my-finalizer"]

# 2. 查看 Namespace 的完整状态
kubectl get namespace my-namespace -o yaml

# 3. 确认资源是否已全部删除
kubectl api-resources --verbs=list --namespaced -o name | \
  xargs -n 1 kubectl get --show-kind --ignore-not-found -n my-namespace
# 应输出为空

# 4. 查找负责该 finalizer 的 controller
kubectl get deployments,daemonsets,statefulsets -A | grep -i "<finalizer-name>"

# 5. 检查相关 controller 的日志
kubectl logs -n <controller-namespace> <controller-pod> | grep "my-namespace"
```

#### 解决建议

**方案 1: 等待负责的 controller 处理 finalizer** (推荐)

```bash
# 如果 controller 正常运行，只是处理较慢，耐心等待

# 检查 controller 是否正常运行
kubectl get pods -A | grep "<controller-name>"

# 查看 controller 日志
kubectl logs -n <controller-namespace> <controller-pod> -f
```

**方案 2: 重启负责的 controller**

```bash
# 如果 controller 卡住或有 bug，尝试重启
kubectl rollout restart deployment/<controller-name> -n <controller-namespace>

# 等待 controller 重启后重新处理 finalizer
```

**方案 3: 手动移除 finalizer** (谨慎操作！)

```bash
# ⚠️ 警告: 移除 finalizer 可能导致资源泄漏（如云资源未清理）

# 方法 1: 使用 kubectl patch
kubectl patch namespace my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge

# 方法 2: 使用 kubectl edit
kubectl edit namespace my-namespace
# 删除 metadata.finalizers 字段，保存退出

# 方法 3: 使用 API 调用
kubectl get namespace my-namespace -o json | \
  jq '.metadata.finalizers = []' | \
  kubectl replace --raw /api/v1/namespaces/my-namespace/finalize -f -
```

**方案 4: 删除特定 finalizer（保留其他）**

```bash
# 仅移除 "example.com/my-finalizer"，保留 "kubernetes"
kubectl patch namespace my-namespace --type json -p='[
  {
    "op": "remove",
    "path": "/metadata/finalizers/0"
  }
]'

# 或使用 jq 过滤
kubectl get namespace my-namespace -o json | \
  jq '.metadata.finalizers = [.metadata.finalizers[] | select(. != "example.com/my-finalizer")]' | \
  kubectl replace --raw /api/v1/namespaces/my-namespace/finalize -f -
```

**典型案例解决方案**:

**案例 1: Rancher Namespace 卡住**

```bash
# 现象: Finalizer 为 "controller.cattle.io/namespace-auth"

# 原因: Rancher 已卸载，但 finalizer 残留

# 解决:
kubectl patch namespace my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge
```

**案例 2: 自定义 CRD 的 finalizer 残留**

```bash
# 现象: Finalizer 为 "finalizer.example.com"

# 原因: 自定义 operator 已删除，但 CR 的 finalizer 未处理

# 排查:
kubectl get crd | grep example.com
kubectl get <crd-name> -n my-namespace

# 解决:
# 1. 如果 CRD 仍存在，删除所有 CR
kubectl delete <crd-name> --all -n my-namespace

# 2. 如果 CRD 已删除，直接移除 Namespace finalizer
kubectl patch namespace my-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge
```

---

## 五、Garbage Collector 事件

### 5.1 `DeletingDependents` - 正在删除依赖对象

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | garbage-collector-controller |
| **关联资源** | 任何有 dependents 的资源 |
| **适用版本** | v1.5+ |
| **生产频率** | 中频 |

#### 事件含义

Garbage Collector 检测到资源被标记删除（`deletionTimestamp` 已设置），正在根据级联删除策略删除其依赖对象（dependents）。

**触发条件**:
- 删除策略为 `Foreground` 或 `Background`
- 资源有 `ownerReferences` 引用的 dependents

#### 典型事件消息

```bash
$ kubectl describe deployment my-app

Events:
  Type    Reason              Age   From                  Message
  ----    ------              ----  ----                  -------
  Normal  DeletingDependents  10s   garbage-collector     Deleting dependents in foreground
  Normal  DeletingDependents  10s   garbage-collector     Deleting dependents in background
```

**message 格式**:
```
Deleting dependents in <foreground|background>
```

#### 影响面说明

- **用户影响**: 低 - 正常删除流程
- **服务影响**: 取决于删除的资源（如删除 Deployment 会导致 Pod 终止）
- **集群影响**: 无
- **关联事件链**: 
  - Foreground: `DeletingDependents` → (等待 dependents 删除) → Owner 删除
  - Background: Owner 删除 → `DeletingDependents` (异步)

#### 排查建议

```bash
# 1. 查看资源的 ownerReferences
kubectl get <resource-type> <resource-name> -o jsonpath='{.metadata.ownerReferences}'

# 2. 查看资源的删除策略 (finalizers)
kubectl get <resource-type> <resource-name> -o jsonpath='{.metadata.finalizers}'

# 3. 列举 dependents
# 例如: Deployment → ReplicaSet → Pod
kubectl get replicasets -l app=my-app -o json | jq -r '.items[] | "\(.metadata.name) owner: \(.metadata.ownerReferences[0].name)"'
kubectl get pods -l app=my-app -o json | jq -r '.items[] | "\(.metadata.name) owner: \(.metadata.ownerReferences[0].name)"'
```

#### 解决建议

**正常情况**: 无需干预，等待删除完成

**如果删除时间过长**:
- 检查 dependents 是否卡在 Terminating 状态
- 强制删除 dependents: `kubectl delete <resource> --grace-period=0 --force`

---

### 5.2 `GracefulDeletion` - 开始优雅删除

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | garbage-collector-controller |
| **关联资源** | 任何被删除的资源 |
| **适用版本** | v1.5+ |
| **生产频率** | 中频 |

#### 事件含义

Garbage Collector 开始执行优雅删除流程，处理资源的 finalizers。

#### 典型事件消息

```bash
Events:
  Type    Reason            Age   From                  Message
  ----    ------            ----  ----                  -------
  Normal  GracefulDeletion  10s   garbage-collector     Graceful deletion started
```

#### 影响面说明

- **用户影响**: 低 - 正常删除过程
- **服务影响**: 无
- **集群影响**: 无

---

### 5.3 `OrphanFinal` - 孤立依赖对象

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | garbage-collector-controller |
| **关联资源** | 被孤立删除的资源 |
| **适用版本** | v1.7+ |
| **生产频率** | 低频 |

#### 事件含义

资源使用 `--cascade=orphan` 策略删除，其 dependents 被孤立（orphaned），不会被级联删除。

**使用场景**:
- 删除 Deployment 但保留 Pod (用于调试或手动接管)
- 删除 StatefulSet 但保留 PVC (避免数据丢失)
- 删除自定义 CR 但保留关联资源

#### 典型事件消息

```bash
$ kubectl describe deployment my-app

Events:
  Type    Reason       Age   From                  Message
  ----    ------       ----  ----                  -------
  Normal  OrphanFinal  10s   garbage-collector     Orphaning dependents
```

**message 格式**:
```
Orphaning dependents
```

#### 影响面说明

- **用户影响**: 中 - 需要手动管理孤立的资源
- **服务影响**: 低 - Pod 继续运行，但失去控制器管理
- **集群影响**: 低 - 孤立资源会占用集群资源，直到手动删除

#### 排查建议

```bash
# 1. 查找孤立的资源 (没有 ownerReferences)
kubectl get pods -o json | \
  jq -r '.items[] | select(.metadata.ownerReferences == null) | .metadata.name'

# 2. 查找曾经有 owner 但现在 owner 不存在的资源
kubectl get pods -o json | \
  jq -r '.items[] | select(.metadata.ownerReferences != null) | "\(.metadata.name) owner: \(.metadata.ownerReferences[0].name)"'

# 然后检查 owner 是否存在:
kubectl get replicaset <owner-name>
```

#### 解决建议

**如果需要重新管理孤立资源**:

```bash
# 方案 1: 重新创建控制器（Deployment 会自动接管匹配 label 的 Pod）
kubectl apply -f deployment.yaml

# 方案 2: 手动添加 ownerReferences
kubectl patch pod my-pod --type merge -p '
{
  "metadata": {
    "ownerReferences": [{
      "apiVersion": "apps/v1",
      "kind": "ReplicaSet",
      "name": "my-replicaset-abc123",
      "uid": "12345678-1234-1234-1234-123456789abc"
    }]
  }
}'

# 方案 3: 删除孤立资源
kubectl delete pod <orphaned-pod>
```

---

## 六、ResourceQuota 事件

### 6.1 `FailedQuota` - 超过资源配额

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | quota |
| **关联资源** | Pod, PVC, Service 等 |
| **适用版本** | v1.1+ |
| **生产频率** | 中频 ⚠️ |

#### 事件含义

创建或更新资源时，超过了 Namespace 的 ResourceQuota 限制，导致操作被拒绝。这是生产环境中常见的资源限制事件。

**ResourceQuota 可限制的资源类型**:

| 资源类型 | Quota 字段 | 限制内容 | 示例 |
|:---|:---|:---|:---|
| **计算资源** | `requests.cpu`, `limits.cpu` | CPU 请求和限制总量 | `requests.cpu: "10"` |
| | `requests.memory`, `limits.memory` | 内存请求和限制总量 | `limits.memory: "20Gi"` |
| **存储资源** | `requests.storage` | PVC 请求的存储总量 | `requests.storage: "100Gi"` |
| | `persistentvolumeclaims` | PVC 对象数量 | `persistentvolumeclaims: "10"` |
| | `<storageclass>.storageclass.storage.k8s.io/requests.storage` | 特定 StorageClass 的存储量 | `ssd.storageclass.storage.k8s.io/requests.storage: "50Gi"` |
| **对象数量** | `pods`, `services`, `configmaps`, `secrets` | 对象数量限制 | `pods: "50"` |
| | `replicationcontrollers`, `deployments.apps` | 控制器数量 | `deployments.apps: "20"` |
| **服务资源** | `services.loadbalancers` | LoadBalancer 类型 Service 数量 | `services.loadbalancers: "5"` |
| | `services.nodeports` | NodePort 类型 Service 数量 | `services.nodeports: "10"` |

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason       Age   From   Message
  ----     ------       ----  ----   -------
  Warning  FailedQuota  10s   quota  Error creating: pods "my-app-7d5bc-xyz12" is forbidden: exceeded quota: resource-quota, requested: requests.cpu=500m,requests.memory=1Gi, used: requests.cpu=9.5,requests.memory=19Gi, limited: requests.cpu=10,requests.memory=20Gi
  Warning  FailedQuota  10s   quota  exceeded quota: pod-quota, requested: pods=1, used: pods=50, limited: pods=50
```

**message 格式**:
```
Error creating: <resource> "<name>" is forbidden: exceeded quota: <quota-name>, requested: <resources>, used: <current>, limited: <limit>
```

**配额检查时机**:
- Pod 创建时 (检查 CPU/内存配额)
- PVC 创建时 (检查存储配额)
- Service 创建时 (检查 Service 数量配额)
- 任何配额资源的 CREATE/UPDATE 操作

#### 影响面说明

- **用户影响**: **高** - Pod 无法创建，Deployment 无法扩容
- **服务影响**: **严重** - 如果 Deployment 的所有副本都失败，服务完全不可用
- **集群影响**: 低 - 仅影响该 Namespace，不影响其他 Namespace
- **关联事件链**: 
  - Deployment: `FailedCreate` (ReplicaSet 事件) → `FailedQuota` (Pod 事件)
  - HPA: `FailedGetScale` (HPA 无法扩容)

#### 排查建议

```bash
# 1. 查看 Namespace 的 ResourceQuota
kubectl get resourcequota -n <namespace>

# 2. 查看 ResourceQuota 的详细使用情况
kubectl describe resourcequota <quota-name> -n <namespace>

# 输出示例:
# Name:            resource-quota
# Namespace:       my-namespace
# Resource         Used   Hard
# --------         ----   ----
# requests.cpu     9.5    10       # 已使用 9.5 核，限制 10 核
# requests.memory  19Gi   20Gi
# pods             50     50       # 已达上限

# 3. 查看 Namespace 中所有 Pod 的资源使用
kubectl get pods -n <namespace> -o json | \
  jq -r '.items[] | "\(.metadata.name): CPU=\(.spec.containers[].resources.requests.cpu // "0") Memory=\(.spec.containers[].resources.requests.memory // "0")"'

# 4. 统计 Namespace 中资源的总使用量
kubectl get pods -n <namespace> -o json | \
  jq '[.items[].spec.containers[].resources.requests.cpu // "0"] | map(if . == "0" then 0 else (. | rtrimstr("m") | tonumber / 1000) end) | add'

# 5. 查看因配额失败的事件
kubectl get events -n <namespace> --field-selector reason=FailedQuota

# 6. 查看 ReplicaSet 的创建失败事件
kubectl describe replicaset <rs-name> -n <namespace> | grep -A 5 "FailedCreate"
```

#### 解决建议

| 问题场景 | 根本原因 | 短期解决方案 | 长期解决方案 |
|:---|:---|:---|:---|
| **CPU/内存配额不足** | Namespace 资源使用接近上限 | 删除闲置 Pod 或扩大配额 | 为 Pod 设置合理的 requests，避免过度申请 |
| **Pod 数量达到上限** | Namespace 中 Pod 过多 | 删除已完成的 Job Pod | 清理 Evicted/Completed Pod，或增加配额 |
| **存储配额不足** | PVC 申请总量超过配额 | 删除未使用的 PVC | 使用对象存储代替 PVC |
| **LoadBalancer 数量上限** | 云平台限制 LB 数量 | 合并 Service 或使用 Ingress | 改用 Ingress Controller |
| **配额未合理分配** | 不同团队/项目配额不均衡 | 临时调整配额 | 重新规划 Namespace 和配额策略 |

**典型案例解决方案**:

**案例 1: Pod 数量配额不足**

```bash
# 现象: exceeded quota: pod-quota, requested: pods=1, used: pods=50, limited: pods=50

# 排查: 查看 Namespace 中的 Pod 分布
kubectl get pods -n my-namespace --no-headers | wc -l
# 输出: 50

kubectl get pods -n my-namespace --field-selector status.phase=Succeeded
# 发现有 20 个 Completed 状态的 Job Pod

kubectl get pods -n my-namespace --field-selector status.phase=Failed
# 发现有 5 个 Evicted 状态的 Pod

# 解决方案 1: 清理已完成的 Pod
kubectl delete pods -n my-namespace --field-selector status.phase=Succeeded
kubectl delete pods -n my-namespace --field-selector status.phase=Failed

# 解决方案 2: 为 Job 配置自动清理
apiVersion: batch/v1
kind: Job
metadata:
  name: my-job
spec:
  ttlSecondsAfterFinished: 3600  # Job 完成后 1 小时自动删除
  template:
    spec:
      restartPolicy: Never

# 解决方案 3: 增加配额
kubectl edit resourcequota pod-quota -n my-namespace
# 修改 spec.hard.pods: "100"
```

**案例 2: CPU 配额不足**

```bash
# 现象: exceeded quota: resource-quota, requested: requests.cpu=500m, used: requests.cpu=9.5, limited: requests.cpu=10

# 排查: 查看哪些 Pod 占用了 CPU 配额
kubectl get pods -n my-namespace -o json | \
  jq -r '.items[] | "\(.metadata.name): \([.spec.containers[].resources.requests.cpu // "0"] | join("+"))"' | \
  sort -t: -k2 -rn | head -10

# 输出:
# large-app-abc123: 2000m
# large-app-def456: 2000m
# medium-app-xyz: 1000m
# ...

# 解决方案 1: 优化 Pod 的 CPU requests（如果过度申请）
kubectl edit deployment large-app -n my-namespace
# 修改:
# resources:
#   requests:
#     cpu: "500m"  # 从 2000m 降低到 500m
#   limits:
#     cpu: "1000m"

# 解决方案 2: 增加 ResourceQuota
kubectl edit resourcequota resource-quota -n my-namespace
# 修改 spec.hard.requests.cpu: "20"

# 解决方案 3: 删除闲置的 Deployment
kubectl get deployments -n my-namespace
kubectl delete deployment <unused-deployment>
```

**案例 3: 存储配额不足**

```bash
# 现象: exceeded quota: storage-quota, requested: requests.storage=10Gi, used: requests.storage=95Gi, limited: requests.storage=100Gi

# 排查: 查看所有 PVC 的使用情况
kubectl get pvc -n my-namespace -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,CAPACITY:.spec.resources.requests.storage

# 输出:
# NAME          STATUS   CAPACITY
# data-db-0     Bound    50Gi
# data-db-1     Bound    50Gi
# cache-pvc     Bound    10Gi     ← 可能不再使用

# 解决方案 1: 删除未使用的 PVC
# 先确认 PVC 是否被 Pod 使用
kubectl get pods -n my-namespace -o json | \
  jq -r '.items[] | .spec.volumes[]? | select(.persistentVolumeClaim != null) | .persistentVolumeClaim.claimName' | sort -u

# 如果 cache-pvc 未被列出，说明未使用，可删除
kubectl delete pvc cache-pvc -n my-namespace

# 解决方案 2: 增加存储配额
kubectl edit resourcequota storage-quota -n my-namespace
# 修改 spec.hard.requests.storage: "200Gi"

# 解决方案 3: 使用对象存储（如 S3）代替 PVC
```

**案例 4: 配额检查失败导致 Pod 创建慢**

```bash
# 现象: Pod 创建延迟数秒，事件显示配额检查

# 原因: ResourceQuota 数量过多，导致 API Server 检查时间长

# 排查:
kubectl get resourcequota -n my-namespace
# 输出: 发现有 10 个 ResourceQuota 对象

# 解决方案: 合并 ResourceQuota（一个 ResourceQuota 可包含多种资源）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: combined-quota
  namespace: my-namespace
spec:
  hard:
    requests.cpu: "10"
    requests.memory: "20Gi"
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
    requests.storage: "100Gi"

# 删除旧的多个 ResourceQuota
kubectl delete resourcequota <old-quota-1> <old-quota-2> -n my-namespace
```

---

### 6.2 `FailedQuotaCheck` - 配额检查失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | quota |
| **关联资源** | Pod, PVC 等 |
| **适用版本** | v1.1+ |
| **生产频率** | 罕见 |

#### 事件含义

API Server 在执行资源配额检查时遇到错误，导致无法判断是否超过配额。这通常表明集群内部问题。

**触发条件**:
- ResourceQuota 对象配置错误
- API Server 无法连接到 quota-controller
- etcd 故障导致配额状态无法读取

#### 典型事件消息

```bash
Events:
  Type     Reason            Age   From   Message
  ----     ------            ----  ----   -------
  Warning  FailedQuotaCheck  10s   quota  Failed to check quota: unable to get resource quota "my-quota": connection refused
```

#### 影响面说明

- **用户影响**: 高 - 资源创建失败
- **服务影响**: 严重 - 无法创建新 Pod
- **集群影响**: 高 - 通常表明 API Server 或 etcd 有问题

#### 排查建议

```bash
# 1. 检查 ResourceQuota 对象是否存在
kubectl get resourcequota -n <namespace>

# 2. 检查 ResourceQuota 配置是否正确
kubectl describe resourcequota <quota-name> -n <namespace>

# 3. 检查 API Server 状态
kubectl get --raw /healthz

# 4. 检查 etcd 状态
kubectl get --raw /metrics | grep etcd

# 5. 查看 kube-controller-manager 日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep quota
```

#### 解决建议

| 错误原因 | 解决方案 |
|:---|:---|
| ResourceQuota 配置错误 | 修正 ResourceQuota 的 `spec.hard` 字段 |
| API Server 不可达 | 检查 API Server 和网络连接 |
| etcd 故障 | 检查 etcd 集群健康状态 |

---

## 七、LimitRange 事件

### 7.1 `LimitRangeDefaults` - 应用默认资源限制

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | limitranger |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

LimitRanger admission controller 自动为未设置资源 requests/limits 的容器应用默认值。这是 Kubernetes 资源管理的重要机制。

**LimitRange 的作用**:
1. **设置默认值**: 为未指定 requests/limits 的容器自动设置默认值
2. **限制范围**: 限制单个容器/Pod 的资源请求上下限
3. **限制比例**: 限制 requests 和 limits 的比例

**LimitRange 配置示例**:
```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: resource-limits
  namespace: my-namespace
spec:
  limits:
    # 容器级别限制
    - type: Container
      default:                    # 默认 limits (如果容器未指定)
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:             # 默认 requests (如果容器未指定)
        cpu: "250m"
        memory: "256Mi"
      max:                        # 单个容器的最大值
        cpu: "2000m"
        memory: "2Gi"
      min:                        # 单个容器的最小值
        cpu: "50m"
        memory: "64Mi"
      maxLimitRequestRatio:       # limits/requests 的最大比例
        cpu: "4"                  # limits.cpu 不能超过 requests.cpu 的 4 倍
        memory: "2"
    
    # Pod 级别限制 (所有容器的总和)
    - type: Pod
      max:
        cpu: "4000m"
        memory: "4Gi"
    
    # PVC 存储限制
    - type: PersistentVolumeClaim
      max:
        storage: "50Gi"
      min:
        storage: "1Gi"
```

#### 典型事件消息

```bash
$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type    Reason              Age   From         Message
  ----    ------              ----  ----         -------
  Normal  LimitRangeDefaults  10s   limitranger  Container "app" set with default cpu request "250m", cpu limit "500m", memory request "256Mi", memory limit "512Mi"
```

**message 格式**:
```
Container "<container-name>" set with default <resource> request "<value>", <resource> limit "<value>"[, ...]
```

#### 影响面说明

- **用户影响**: 低 - 自动配置，简化 Pod 定义
- **服务影响**: 无
- **集群影响**: 正面 - 确保资源合理分配，避免资源不受限的 Pod
- **关联事件链**: `LimitRangeDefaults` → Pod 创建成功

#### 排查建议

```bash
# 1. 查看 Namespace 的 LimitRange
kubectl get limitrange -n <namespace>

# 2. 查看 LimitRange 的详细配置
kubectl describe limitrange <limitrange-name> -n <namespace>

# 3. 查看 Pod 实际应用的资源限制
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].resources}'

# 4. 比较 Pod 定义和实际值
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 10 "resources:"
```

#### 解决建议

**正常情况**: 无需干预，这是推荐的最佳实践

**如果默认值不合适**:

| 问题场景 | 解决方案 |
|:---|:---|
| 默认值过小，Pod OOMKilled | 调整 LimitRange 的 `default` 和 `defaultRequest` |
| 默认值过大，资源浪费 | 减少 LimitRange 的默认值 |
| 希望容器使用自定义值 | 在 Pod spec 中显式设置 `resources` |

**修改 LimitRange**:
```bash
# 编辑 LimitRange
kubectl edit limitrange resource-limits -n my-namespace

# 或使用 patch
kubectl patch limitrange resource-limits -n my-namespace --type merge -p '
{
  "spec": {
    "limits": [{
      "type": "Container",
      "default": {
        "cpu": "1000m",
        "memory": "1Gi"
      },
      "defaultRequest": {
        "cpu": "500m",
        "memory": "512Mi"
      }
    }]
  }
}'
```

**在 Pod 中显式设置资源**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-app
spec:
  containers:
    - name: app
      image: myapp:v1
      resources:
        requests:
          cpu: "100m"        # 显式设置，不使用 LimitRange 默认值
          memory: "128Mi"
        limits:
          cpu: "200m"
          memory: "256Mi"
```

---

### 7.2 `InvalidLimitRange` - LimitRange 配置无效

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | limitranger |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 罕见 |

#### 事件含义

Pod 的资源配置违反了 LimitRange 的限制，导致 Pod 创建被拒绝。

**常见违反情况**:
- 容器的 requests 或 limits 超过了 LimitRange 的 `max`
- 容器的 requests 或 limits 低于 LimitRange 的 `min`
- `limits.cpu / requests.cpu` 超过了 `maxLimitRequestRatio.cpu`
- Pod 的资源总和超过了 Pod 级别的 `max`

#### 典型事件消息

```bash
Events:
  Type     Reason             Age   From         Message
  ----     ------             ----  ----         -------
  Warning  InvalidLimitRange  10s   limitranger  Pod "my-app" is forbidden: maximum cpu usage per Container is 2000m, but limit is 4000m
  Warning  InvalidLimitRange  10s   limitranger  Pod "my-app" is forbidden: minimum memory usage per Container is 64Mi, but request is 32Mi
  Warning  InvalidLimitRange  10s   limitranger  Pod "my-app" is forbidden: cpu max limit to request ratio per Container is 4, but provided ratio is 10.000000
```

**message 格式**:
```
Pod "<pod-name>" is forbidden: <violation-reason>
```

#### 影响面说明

- **用户影响**: 高 - Pod 无法创建
- **服务影响**: 严重 - Deployment 无法部署
- **集群影响**: 低 - 仅影响该 Pod
- **关联事件链**: `InvalidLimitRange` → `FailedCreate` (ReplicaSet)

#### 排查建议

```bash
# 1. 查看 Pod 的资源配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].resources}'

# 2. 查看 LimitRange 的限制
kubectl describe limitrange -n <namespace>

# 3. 计算 limits/requests 比例
# 如果 Pod 定义为:
#   requests.cpu: "100m"
#   limits.cpu: "1000m"
# 比例 = 1000m / 100m = 10

# 检查 LimitRange 的 maxLimitRequestRatio.cpu 是否小于 10
```

#### 解决建议

| 违反类型 | 解决方案 |
|:---|:---|
| **超过 max** | 减少 Pod 的 requests/limits，或增加 LimitRange 的 max |
| **低于 min** | 增加 Pod 的 requests/limits，或减少 LimitRange 的 min |
| **超过比例限制** | 减少 limits 或增加 requests，或调整 LimitRange 的 maxLimitRequestRatio |
| **Pod 总资源超限** | 减少容器数量或每个容器的资源，或调整 Pod 级别的 max |

**修正 Pod 配置**:
```yaml
# 原配置 (违反 LimitRange)
resources:
  requests:
    cpu: "100m"      # 比例 = 4000/100 = 40 (超过 maxLimitRequestRatio: 4)
  limits:
    cpu: "4000m"     # 超过 LimitRange max: 2000m

# 修正后
resources:
  requests:
    cpu: "500m"      # 比例 = 2000/500 = 4 ✅
  limits:
    cpu: "2000m"     # 符合 LimitRange max ✅
```

---

## 八、PodDisruptionBudget 事件

### 8.1 PodDisruptionBudget (PDB) 原理

**PDB 的作用**:
保护应用在**自愿性驱逐**（Voluntary Disruptions）时的可用性，确保至少保留一定数量或比例的 Pod。

**PDB 保护的场景** (Voluntary):
- ✅ `kubectl drain` 节点维护
- ✅ `kubectl delete pod` 手动删除
- ✅ Cluster Autoscaler 缩容节点
- ✅ `kubectl rollout` 滚动更新

**PDB 不保护的场景** (Involuntary):
- ❌ 节点故障 (NodeNotReady)
- ❌ 节点资源不足导致的驱逐 (Evicted)
- ❌ OOMKilled
- ❌ 网络分区

**PDB 配置示例**:
```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
  namespace: my-namespace
spec:
  # 方式 1: 指定最少可用 Pod 数
  minAvailable: 2
  
  # 方式 2: 指定最少可用 Pod 百分比
  # minAvailable: "50%"
  
  # 方式 3: 指定最多不可用 Pod 数 (v1.7+)
  # maxUnavailable: 1
  
  # 方式 4: 指定最多不可用 Pod 百分比
  # maxUnavailable: "30%"
  
  selector:
    matchLabels:
      app: my-app
```

**PDB 状态字段**:
```yaml
status:
  currentHealthy: 3         # 当前健康 Pod 数
  desiredHealthy: 2         # 期望最少健康 Pod 数
  disruptionsAllowed: 1     # 允许驱逐的 Pod 数 (currentHealthy - desiredHealthy)
  expectedPods: 3           # 期望 Pod 总数
```

---

### 8.2 `NoPods` - PDB 未找到匹配的 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | disruption-controller |
| **关联资源** | PodDisruptionBudget |
| **适用版本** | v1.5+ |
| **生产频率** | 低频 |

#### 事件含义

PDB 的 `selector` 未匹配到任何 Pod，导致 PDB 无法生效。

**触发条件**:
- PDB 的 `selector.matchLabels` 配置错误
- 目标 Deployment/StatefulSet 尚未创建 Pod
- 目标 Pod 的 label 与 PDB 不匹配

#### 典型事件消息

```bash
$ kubectl describe pdb my-app-pdb

Events:
  Type     Reason  Age   From                    Message
  ----     ------  ----  ----                    -------
  Warning  NoPods  10s   disruption-controller   No matching pods found
```

#### 影响面说明

- **用户影响**: 中 - PDB 无法提供保护，驱逐操作可能影响所有 Pod
- **服务影响**: 中 - 没有 PDB 保护，可能导致服务完全不可用
- **集群影响**: 无
- **关联事件链**: `NoPods` (持续) → PDB 不生效

#### 排查建议

```bash
# 1. 查看 PDB 的 selector
kubectl get pdb my-app-pdb -o jsonpath='{.spec.selector.matchLabels}'

# 2. 查找匹配的 Pod
kubectl get pods -l app=my-app -n my-namespace

# 3. 对比 Pod 的 labels 和 PDB 的 selector
kubectl get pods -n my-namespace --show-labels

# 4. 查看 PDB 的状态
kubectl get pdb my-app-pdb -o yaml
# 检查 status.expectedPods 是否为 0
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **Selector 错误** | PDB 的 matchLabels 配置错误 | 修正 PDB 的 selector 使其匹配 Pod 的 labels |
| **Pod 未创建** | Deployment 尚未部署 | 先部署 Deployment，再创建 PDB |
| **Namespace 不匹配** | PDB 和 Pod 不在同一个 Namespace | 将 PDB 移动到正确的 Namespace |

**修正 PDB selector**:
```bash
# 查看 Pod 的 labels
kubectl get pod -l app=my-app -n my-namespace --show-labels
# 输出: app=my-app,version=v1

# 修正 PDB
kubectl edit pdb my-app-pdb -n my-namespace
# 修改 spec.selector.matchLabels:
#   app: my-app  # 确保与 Pod labels 一致
```

---

### 8.3 `CalculateExpectedPodCountFailed` - 计算期望 Pod 数失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | disruption-controller |
| **关联资源** | PodDisruptionBudget |
| **适用版本** | v1.5+ |
| **生产频率** | 罕见 |

#### 事件含义

disruption-controller 无法计算 PDB 保护的 Pod 期望数量，通常是因为 PDB 配置错误或控制器故障。

#### 典型事件消息

```bash
Events:
  Type     Reason                           Age   From                    Message
  ----     ------                           ----  ----                    -------
  Warning  CalculateExpectedPodCountFailed  10s   disruption-controller   Failed to calculate the number of expected pods: unable to find controller for pod
```

#### 影响面说明

- **用户影响**: 高 - PDB 无法正常工作
- **服务影响**: 中 - 没有 PDB 保护
- **集群影响**: 低 - 可能表明 controller-manager 有问题

#### 排查建议

```bash
# 1. 检查 PDB 配置
kubectl get pdb my-app-pdb -o yaml

# 2. 检查 Pod 是否有 ownerReferences
kubectl get pods -l app=my-app -o jsonpath='{.items[*].metadata.ownerReferences}'

# 3. 检查 disruption-controller 日志
kubectl logs -n kube-system -l component=kube-controller-manager | grep disruption
```

#### 解决建议

| 问题原因 | 解决方案 |
|:---|:---|
| PDB 配置错误 | 修正 PDB 的 minAvailable/maxUnavailable 字段 |
| Pod 没有控制器 | 确保 Pod 由 Deployment/StatefulSet 管理 |
| controller-manager 故障 | 重启 kube-controller-manager |

---

### 8.4 `Stale` - PDB 状态过期

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | disruption-controller |
| **关联资源** | PodDisruptionBudget |
| **适用版本** | v1.5+ |
| **生产频率** | 低频 |

#### 事件含义

PDB 的状态长时间未更新，可能是 disruption-controller 异常。

#### 排查建议

```bash
# 检查 kube-controller-manager 健康状态
kubectl get pods -n kube-system -l component=kube-controller-manager
kubectl logs -n kube-system -l component=kube-controller-manager | tail -100
```

---

### 8.5 `InsufficientBudget` - 预算不足,无法驱逐

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | disruption-controller |
| **关联资源** | Pod |
| **适用版本** | v1.5+ |
| **生产频率** | 中频 |

#### 事件含义

尝试驱逐 Pod 时，因 PDB 的限制而被拒绝。这是 PDB 保护机制的正常工作表现。

**触发场景**:
- 执行 `kubectl drain` 时，尝试驱逐的 Pod 超过了 PDB 允许的数量
- 手动删除 Pod 时，剩余 Pod 数低于 PDB 的 `minAvailable`

#### 典型事件消息

```bash
# kubectl drain 时的输出
$ kubectl drain node-1 --ignore-daemonsets

evicting pod my-namespace/my-app-7d5bc-xyz12
error when evicting pod "my-app-7d5bc-xyz12" (will retry after 5s): Cannot evict pod as it would violate the pod's disruption budget.

$ kubectl describe pod my-app-7d5bc-xyz12

Events:
  Type     Reason              Age   From                    Message
  ----     ------              ----  ----                    -------
  Warning  InsufficientBudget  10s   disruption-controller   Cannot evict pod as it would violate the pod's disruption budget
```

**message 格式**:
```
Cannot evict pod as it would violate the pod's disruption budget
```

#### 影响面说明

- **用户影响**: 中 - `kubectl drain` 操作被阻塞，需要等待
- **服务影响**: 无 - 这是保护机制，确保服务可用性
- **集群影响**: 低 - 节点维护被延迟
- **关联事件链**: `InsufficientBudget` → (等待其他 Pod Ready) → 驱逐成功

#### 排查建议

```bash
# 1. 查看 PDB 的当前状态
kubectl get pdb my-app-pdb -n my-namespace -o yaml

# 检查关键字段:
# status.currentHealthy: 2      # 当前健康 Pod 数
# status.desiredHealthy: 2      # 期望最少健康 Pod 数
# status.disruptionsAllowed: 0  # 允许驱逐的 Pod 数 (0 表示不能驱逐)

# 2. 查看匹配 PDB 的所有 Pod
kubectl get pods -l app=my-app -n my-namespace -o wide

# 3. 查看 Pod 的 Ready 状态
kubectl get pods -l app=my-app -n my-namespace -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 4. 查看 Deployment 的副本数
kubectl get deployment my-app -n my-namespace -o jsonpath='{.spec.replicas} desired, {.status.replicas} current, {.status.readyReplicas} ready'
```

#### 解决建议

**正常情况**: 等待其他 Pod Ready 后，PDB 会自动允许驱逐

**如果长时间阻塞**:

| 问题场景 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **所有 Pod 在同一节点** | 反亲和性配置不当 | 查看 Pod 的 `spec.nodeName` | 调整 podAntiAffinity 策略 |
| **副本数等于 minAvailable** | PDB 配置过于严格 | 比较 replicas 和 minAvailable | 增加副本数或调整 PDB |
| **新 Pod 无法启动** | 资源不足或调度失败 | 查看 Pending Pod 的事件 | 解决调度问题 |
| **滚动更新卡住** | maxSurge=0 且 PDB 限制 | 查看 Deployment 策略 | 增加 maxSurge 或调整 PDB |
| **节点维护紧急** | 必须立即 drain | - | 临时删除 PDB 或使用 `--disable-eviction` |

**典型案例解决方案**:

**案例 1: kubectl drain 被 PDB 阻塞**

```bash
# 现象: Cannot evict pod as it would violate the pod's disruption budget

# 排查:
kubectl get pdb -n my-namespace
# NAME          MIN AVAILABLE   MAX UNAVAILABLE   ALLOWED DISRUPTIONS   AGE
# my-app-pdb    2               N/A               0                     10d

kubectl get pods -l app=my-app -n my-namespace -o wide
# NAME              READY   STATUS    NODE
# my-app-abc123     1/1     Running   node-1    ← 在被 drain 的节点
# my-app-def456     0/1     Pending   <none>    ← 新 Pod 无法调度

# 根本原因: 只有 1 个 Pod Running，低于 minAvailable=2

# 解决方案 1: 等待新 Pod Ready (推荐)
kubectl describe pod my-app-def456 -n my-namespace
# 查看 Pending 原因，解决调度问题 (如添加节点、清理资源)

# 解决方案 2: 临时调整 PDB (谨慎)
kubectl patch pdb my-app-pdb -n my-namespace -p '{"spec":{"minAvailable":1}}'

# 解决方案 3: 紧急情况下强制 drain (危险！)
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data --disable-eviction --force
# ⚠️ 这会绕过 PDB，可能导致服务完全不可用

# 解决方案 4: 先扩容后 drain
kubectl scale deployment my-app -n my-namespace --replicas=3
# 等待新 Pod Ready
kubectl wait --for=condition=Ready pod -l app=my-app -n my-namespace --timeout=300s
# 再执行 drain
kubectl drain node-1 --ignore-daemonsets
```

**案例 2: PDB 阻止滚动更新**

```bash
# 现象: Deployment 滚动更新卡住

# 排查:
kubectl rollout status deployment my-app -n my-namespace
# 输出: Waiting for deployment "my-app" rollout to finish: 2 out of 3 new replicas have been updated...

kubectl get pods -l app=my-app -n my-namespace
# NAME                      READY   STATUS        RESTARTS   AGE
# my-app-old-abc123         1/1     Running       0          10m
# my-app-old-def456         1/1     Terminating   0          10m  ← 无法被驱逐
# my-app-new-xyz789         1/1     Running       0          2m
# my-app-new-uvw012         1/1     Running       0          2m

kubectl describe pod my-app-old-def456
# Events:
#   Warning  InsufficientBudget  Cannot evict pod as it would violate the pod's disruption budget

# 原因: Deployment 的 maxSurge=0，且 PDB 限制了同时不可用的 Pod 数

# 解决方案 1: 增加 maxSurge (推荐)
kubectl patch deployment my-app -n my-namespace -p '
{
  "spec": {
    "strategy": {
      "rollingUpdate": {
        "maxSurge": 1,        # 允许超出 replicas 数量
        "maxUnavailable": 0   # 不允许不可用 Pod
      }
    }
  }
}'

# 解决方案 2: 调整 PDB 的 maxUnavailable
kubectl patch pdb my-app-pdb -n my-namespace -p '{"spec":{"maxUnavailable":1}}'

# 解决方案 3: 临时删除 PDB (谨慎)
kubectl delete pdb my-app-pdb -n my-namespace
# 更新完成后重新创建 PDB
```

**案例 3: 所有 Pod 在同一节点，无法 drain**

```bash
# 现象: 3 副本 Pod 全在同一节点

# 排查:
kubectl get pods -l app=my-app -n my-namespace -o wide
# NAME              NODE
# my-app-abc123     node-1
# my-app-def456     node-1
# my-app-xyz789     node-1

# 原因: 未配置 podAntiAffinity

# 解决方案: 配置反亲和性
kubectl edit deployment my-app -n my-namespace

# 添加:
spec:
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:  # 软反亲和
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: my-app
                topologyKey: kubernetes.io/hostname

# 或使用硬反亲和 (要求副本数 ≤ 节点数):
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app: my-app
              topologyKey: kubernetes.io/hostname

# 触发滚动更新，Pod 会分散到不同节点
kubectl rollout restart deployment my-app -n my-namespace
```

---

### 8.6 `DisruptionAllowed` - 允许驱逐 Pod

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | disruption-controller |
| **关联资源** | Pod |
| **适用版本** | v1.21+ |
| **生产频率** | 中频 |

#### 事件含义

PDB 检查通过，允许驱逐该 Pod。这是正常的驱逐流程事件。

#### 典型事件消息

```bash
Events:
  Type    Reason             Age   From                    Message
  ----    ------             ----  ----                    -------
  Normal  DisruptionAllowed  10s   disruption-controller   Disruption is allowed for this pod
```

#### 影响面说明

- **用户影响**: 无 - 正常流程
- **服务影响**: 低 - Pod 会被驱逐，但 PDB 确保了足够的可用副本
- **集群影响**: 无

---

## 九、典型排查场景

### 9.1 场景: Namespace 卡在 Terminating 状态

**问题描述**:
```bash
$ kubectl get namespace test-namespace
NAME             STATUS        AGE
test-namespace   Terminating   10m

$ kubectl delete namespace test-namespace
# 命令无响应或提示 namespace 已在删除中
```

**排查流程**:

```bash
# 1. 查看 Namespace 的 finalizers
kubectl get namespace test-namespace -o jsonpath='{.metadata.finalizers}'
# 输出: ["kubernetes","example.com/my-finalizer"]

# 2. 查看 Namespace 的事件
kubectl describe namespace test-namespace
# 查找:
#   - NamespaceContentRemaining: 检查哪些资源未删除
#   - NamespaceFinalizersRemaining: 检查哪些 finalizer 未处理
#   - NamespaceDeletionContentFailure: 检查删除失败原因

# 3. 列举 Namespace 中剩余的资源
kubectl api-resources --verbs=list --namespaced -o name | \
  xargs -n 1 kubectl get --show-kind --ignore-not-found -n test-namespace

# 4. 如果有资源残留，尝试强制删除
kubectl delete pods --all -n test-namespace --grace-period=0 --force
kubectl delete pvc --all -n test-namespace

# 5. 如果是 finalizer 问题，移除 finalizer (谨慎操作)
kubectl patch namespace test-namespace -p '{"metadata":{"finalizers":[]}}' --type=merge

# 6. 验证 Namespace 是否删除
kubectl get namespace test-namespace
# 应输出: Error from server (NotFound)
```

**常见原因对照表**:

| 卡住原因 | 事件提示 | 解决方法 |
|:---|:---|:---|
| PVC 未删除 | NamespaceContentRemaining: persistentvolumeclaims | `kubectl delete pvc --all -n <ns>` |
| CRD 已删除但 CR 残留 | NamespaceDeletionContentFailure: could not find the requested resource | 移除 finalizers 或重建 CRD 后删除 CR |
| 自定义 finalizer 未处理 | NamespaceFinalizersRemaining: ["custom-finalizer"] | 重启相关 controller 或手动移除 finalizer |
| Webhook 拦截删除 | NamespaceDeletionContentFailure: webhook denied | 禁用 webhook 或修改规则 |

---

### 9.2 场景: Pod 创建失败,提示超过配额

**问题描述**:
```bash
$ kubectl get pods -n my-namespace
NAME                     READY   STATUS    RESTARTS   AGE
my-app-7d5bc-xyz12       0/1     Pending   0          5m

$ kubectl describe pod my-app-7d5bc-xyz12 -n my-namespace
Events:
  Warning  FailedQuota  Error creating: exceeded quota: resource-quota, requested: requests.cpu=500m, used: requests.cpu=9.5, limited: requests.cpu=10
```

**排查流程**:

```bash
# 1. 查看 ResourceQuota 状态
kubectl describe resourcequota -n my-namespace
# Name:            resource-quota
# Resource         Used   Hard
# --------         ----   ----
# requests.cpu     9.5    10      ← 已接近上限
# requests.memory  19Gi   20Gi

# 2. 统计各个 Deployment 的资源使用
kubectl get deployments -n my-namespace -o json | \
  jq -r '.items[] | "\(.metadata.name): \(.spec.replicas) replicas × \(.spec.template.spec.containers[0].resources.requests.cpu // "0")"'

# 输出:
# app-a: 5 replicas × 1000m  = 5000m (5 核)
# app-b: 3 replicas × 500m   = 1500m (1.5 核)
# app-c: 3 replicas × 1000m  = 3000m (3 核)
# Total: 9.5 核 (已达配额)

# 3. 决策:
# 选项 A: 删除闲置的 Deployment
kubectl delete deployment app-c -n my-namespace

# 选项 B: 减少副本数
kubectl scale deployment app-a -n my-namespace --replicas=3

# 选项 C: 优化 Pod 的资源 requests (如果过度申请)
kubectl edit deployment app-b -n my-namespace
# 修改: requests.cpu: "250m"  (从 500m 降低)

# 选项 D: 增加 ResourceQuota
kubectl edit resourcequota resource-quota -n my-namespace
# 修改: requests.cpu: "20"

# 4. 验证 Pod 是否创建成功
kubectl get pods -n my-namespace -w
```

---

### 9.3 场景: kubectl drain 被 PDB 阻塞

**问题描述**:
```bash
$ kubectl drain node-1 --ignore-daemonsets
evicting pod my-namespace/my-app-7d5bc-xyz12
error when evicting pod "my-app-7d5bc-xyz12": Cannot evict pod as it would violate the pod's disruption budget.
```

**排查流程**:

```bash
# 1. 查看 PDB 状态
kubectl get pdb -A
kubectl describe pdb my-app-pdb -n my-namespace

# Status:
#   Current Healthy:       2
#   Desired Healthy:       2
#   Disruptions Allowed:   0    ← 不允许驱逐
#   Expected Pods:         3
#   Total:                 3

# 2. 查看所有副本的状态
kubectl get pods -l app=my-app -n my-namespace -o wide

# NAME              READY   STATUS    NODE
# my-app-abc123     1/1     Running   node-1    ← 在被 drain 的节点
# my-app-def456     1/1     Running   node-2
# my-app-xyz789     0/1     Pending   <none>    ← 新 Pod 未 Ready

# 3. 查看 Pending Pod 的原因
kubectl describe pod my-app-xyz789 -n my-namespace
# Events:
#   Warning  FailedScheduling  0/3 nodes are available: 1 node(s) had untolerated taint, 2 Insufficient cpu.

# 4. 解决调度问题 (根据具体原因)
# 如果是资源不足:
kubectl top node
# 如果需要，添加新节点或清理资源

# 5. 等待新 Pod Ready
kubectl wait --for=condition=Ready pod -l app=my-app -n my-namespace --timeout=300s

# 6. 验证 PDB 允许驱逐
kubectl get pdb my-app-pdb -n my-namespace -o jsonpath='{.status.disruptionsAllowed}'
# 输出: 1 (表示可以驱逐 1 个 Pod)

# 7. 重新执行 drain
kubectl drain node-1 --ignore-daemonsets

# 8. 如果紧急情况，强制 drain (跳过 PDB 检查)
kubectl drain node-1 --ignore-daemonsets --disable-eviction --force
# ⚠️ 警告: 这会绕过 PDB 保护，可能导致服务不可用
```

---

## 十、生产环境最佳实践

### 10.1 Namespace 管理

**Namespace 生命周期管理**:
```yaml
# 为 Namespace 添加标签和注解，便于管理
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    environment: production
    team: platform
  annotations:
    owner: "platform-team@example.com"
    cost-center: "CC-1234"
```

**防止误删 Namespace**:
```bash
# 使用 finalizer 保护重要 Namespace
kubectl patch namespace production -p '
{
  "metadata": {
    "finalizers": ["example.com/protect"]
  }
}'

# 删除保护 (需先移除 finalizer)
kubectl patch namespace production -p '{"metadata":{"finalizers":[]}}' --type=merge
kubectl delete namespace production
```

**自动化清理策略**:
```yaml
# 使用 CronJob 定期清理 Evicted Pod
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cleanup-evicted-pods
  namespace: kube-system
spec:
  schedule: "0 */6 * * *"  # 每 6 小时执行一次
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: cleanup-sa
          containers:
            - name: kubectl
              image: bitnami/kubectl:latest
              command:
                - /bin/sh
                - -c
                - |
                  kubectl get pods -A --field-selector status.phase=Failed -o json | \
                    jq -r '.items[] | select(.status.reason=="Evicted") | "\(.metadata.namespace) \(.metadata.name)"' | \
                    xargs -n2 sh -c 'kubectl delete pod -n $0 $1'
          restartPolicy: OnFailure
```

---

### 10.2 ResourceQuota 最佳实践

**为每个 Namespace 设置配额**:
```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: production
spec:
  hard:
    # 计算资源配额
    requests.cpu: "50"              # 总 CPU 请求量
    requests.memory: "100Gi"        # 总内存请求量
    limits.cpu: "100"               # 总 CPU 限制量
    limits.memory: "200Gi"          # 总内存限制量
    
    # 对象数量配额
    pods: "100"                     # 最多 100 个 Pod
    services: "20"                  # 最多 20 个 Service
    services.loadbalancers: "5"     # 最多 5 个 LoadBalancer
    services.nodeports: "10"        # 最多 10 个 NodePort
    
    # 存储配额
    persistentvolumeclaims: "20"    # 最多 20 个 PVC
    requests.storage: "500Gi"       # 总存储请求量
    
    # 其他对象配额
    configmaps: "50"
    secrets: "50"
    replicationcontrollers: "20"
    deployments.apps: "20"
    statefulsets.apps: "10"
    jobs.batch: "50"
    cronjobs.batch: "10"

---
# 针对不同 QoS 的配额 (可选)
apiVersion: v1
kind: ResourceQuota
metadata:
  name: besteffort-quota
  namespace: production
spec:
  hard:
    pods: "10"                      # 最多 10 个 BestEffort Pod
  scopes:
    - BestEffort

---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota-low-priority
  namespace: production
spec:
  hard:
    requests.cpu: "10"
    requests.memory: "20Gi"
  scopeSelector:
    matchExpressions:
      - operator: In
        scopeName: PriorityClass
        values:
          - low-priority
```

**配额监控和告警**:
```bash
# 查询所有 Namespace 的配额使用率
kubectl get resourcequota -A -o json | \
  jq -r '.items[] | "\(.metadata.namespace) | \(.metadata.name) | CPU: \(.status.used["requests.cpu"]) / \(.status.hard["requests.cpu"]) | Memory: \(.status.used["requests.memory"]) / \(.status.hard["requests.memory"])"'

# 设置 Prometheus 告警规则
groups:
  - name: resourcequota_alerts
    rules:
      - alert: NamespaceQuotaHighUsage
        expr: |
          (kube_resourcequota{type="used"} / kube_resourcequota{type="hard"}) > 0.9
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Namespace {{ $labels.namespace }} quota {{ $labels.resource }} usage is high"
          description: "Namespace {{ $labels.namespace }} is using {{ $value | humanizePercentage }} of its quota for {{ $labels.resource }}"
```

---

### 10.3 LimitRange 最佳实践

**为每个 Namespace 设置 LimitRange**:
```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: resource-limits
  namespace: production
spec:
  limits:
    # 容器级别限制
    - type: Container
      default:                      # 默认 limits (如果未指定)
        cpu: "500m"
        memory: "512Mi"
        ephemeral-storage: "1Gi"
      defaultRequest:               # 默认 requests (如果未指定)
        cpu: "100m"
        memory: "128Mi"
        ephemeral-storage: "100Mi"
      max:                          # 单个容器的最大值
        cpu: "4000m"
        memory: "8Gi"
        ephemeral-storage: "10Gi"
      min:                          # 单个容器的最小值
        cpu: "50m"
        memory: "64Mi"
        ephemeral-storage: "10Mi"
      maxLimitRequestRatio:         # limits/requests 的最大比例
        cpu: "10"                   # limits.cpu 不超过 requests.cpu 的 10 倍
        memory: "2"                 # limits.memory 不超过 requests.memory 的 2 倍
    
    # Pod 级别限制 (所有容器的总和)
    - type: Pod
      max:
        cpu: "8000m"
        memory: "16Gi"
        ephemeral-storage: "20Gi"
    
    # PVC 存储限制
    - type: PersistentVolumeClaim
      max:
        storage: "100Gi"
      min:
        storage: "1Gi"
```

**不同环境的 LimitRange 策略**:
```yaml
# 开发环境 (宽松)
spec:
  limits:
    - type: Container
      default:
        cpu: "1000m"
        memory: "1Gi"
      defaultRequest:
        cpu: "100m"
        memory: "128Mi"

---
# 生产环境 (严格)
spec:
  limits:
    - type: Container
      default:
        cpu: "500m"
        memory: "512Mi"
      defaultRequest:
        cpu: "200m"
        memory: "256Mi"
      maxLimitRequestRatio:
        cpu: "4"      # 更严格的比例限制
        memory: "2"
```

---

### 10.4 PodDisruptionBudget 最佳实践

**为关键应用设置 PDB**:
```yaml
# 方式 1: 最少可用 Pod 数 (适合固定副本数)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
  namespace: production
spec:
  minAvailable: 2           # 始终保持至少 2 个 Pod 可用
  selector:
    matchLabels:
      app: my-app

---
# 方式 2: 最少可用百分比 (适合动态副本数，如 HPA)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
  namespace: production
spec:
  minAvailable: "50%"       # 始终保持至少 50% 的 Pod 可用
  selector:
    matchLabels:
      app: my-app

---
# 方式 3: 最多不可用 Pod 数 (推荐，更灵活)
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
  namespace: production
spec:
  maxUnavailable: 1         # 最多允许 1 个 Pod 不可用
  selector:
    matchLabels:
      app: my-app

---
# 方式 4: 最多不可用百分比
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
  namespace: production
spec:
  maxUnavailable: "30%"     # 最多允许 30% 的 Pod 不可用
  selector:
    matchLabels:
      app: my-app
```

**PDB 配置建议**:

| 副本数 | 推荐 PDB 配置 | 说明 |
|:---|:---|:---|
| 1 | 不设置 PDB | 单副本无法保证高可用 |
| 2 | `minAvailable: 1` 或 `maxUnavailable: 1` | 始终保持 1 个 Pod 可用 |
| 3-5 | `minAvailable: 2` 或 `maxUnavailable: 1` | 允许 1 个 Pod 驱逐，保留 2 个 |
| 6-10 | `maxUnavailable: 2` 或 `minAvailable: "60%"` | 允许同时驱逐 2 个 Pod |
| 10+ | `maxUnavailable: "30%"` 或 `minAvailable: "70%"` | 按百分比动态调整 |

**PDB 与 Deployment 滚动更新的协调**:
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 5
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1           # 允许超出 replicas 数量 1 个
      maxUnavailable: 1     # 允许不可用 Pod 数量 1 个
  template:
    spec:
      containers:
        - name: app
          image: myapp:v1

---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
spec:
  maxUnavailable: 1         # 与 Deployment 的 maxUnavailable 一致
  selector:
    matchLabels:
      app: my-app
```

**PDB 监控**:
```bash
# 查看所有 PDB 的状态
kubectl get pdb -A -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
MIN-AVAILABLE:.spec.minAvailable,\
MAX-UNAVAILABLE:.spec.maxUnavailable,\
ALLOWED-DISRUPTIONS:.status.disruptionsAllowed,\
CURRENT:.status.currentHealthy,\
DESIRED:.status.desiredHealthy

# Prometheus 告警规则
groups:
  - name: pdb_alerts
    rules:
      - alert: PDBViolation
        expr: kube_poddisruptionbudget_status_current_healthy < kube_poddisruptionbudget_status_desired_healthy
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "PDB {{ $labels.namespace }}/{{ $labels.poddisruptionbudget }} is violated"
          description: "Current healthy pods ({{ $value }}) is below desired ({{ $labels.desired_healthy }})"
      
      - alert: PDBNoDisruptionsAllowed
        expr: kube_poddisruptionbudget_status_disruptions_allowed == 0
        for: 30m
        labels:
          severity: info
        annotations:
          summary: "PDB {{ $labels.namespace }}/{{ $labels.poddisruptionbudget }} allows no disruptions"
          description: "This may block kubectl drain operations"
```

---

### 10.5 垃圾回收策略

**为资源设置合适的级联删除策略**:
```yaml
# 默认 (Background 删除)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
# 不设置 deletionPolicy，使用默认 Background 删除

---
# Foreground 删除 (确保 dependents 先删除)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  finalizers:
    - foregroundDeletion  # 显式指定前台删除

---
# Orphan 删除 (保留 dependents)
# 使用 kubectl 命令:
kubectl delete deployment my-app --cascade=orphan
```

**为自定义资源设置 ownerReferences**:
```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: my-config
  ownerReferences:
    - apiVersion: apps/v1
      kind: Deployment
      name: my-app
      uid: 12345678-1234-1234-1234-123456789abc
      controller: true
      blockOwnerDeletion: true  # 阻止 Deployment 被删除，直到该 ConfigMap 被删除
data:
  config.yaml: |
    ...
```

---

### 10.6 事件监控和告警

**使用 Prometheus 监控事件**:
```yaml
# kube-state-metrics 会导出事件指标
kube_event{type="Warning", reason="FailedQuota"}
kube_event{type="Warning", reason="NamespaceDeletionContentFailure"}
kube_event{type="Warning", reason="InsufficientBudget"}

# 告警规则示例
groups:
  - name: namespace_alerts
    rules:
      - alert: NamespaceStuckTerminating
        expr: |
          kube_namespace_status_phase{phase="Terminating"} == 1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Namespace {{ $labels.namespace }} stuck in Terminating"
          description: "Namespace has been Terminating for more than 10 minutes"
      
      - alert: ResourceQuotaExceeded
        expr: |
          increase(kube_event{type="Warning", reason="FailedQuota"}[5m]) > 10
        labels:
          severity: warning
        annotations:
          summary: "Frequent FailedQuota events in namespace {{ $labels.namespace }}"
          description: "More than 10 FailedQuota events in the last 5 minutes"
      
      - alert: PDBBlockingDrain
        expr: |
          increase(kube_event{type="Warning", reason="InsufficientBudget"}[10m]) > 5
        labels:
          severity: info
        annotations:
          summary: "PDB is blocking pod eviction"
          description: "PDB {{ $labels.poddisruptionbudget }} has blocked eviction {{ $value }} times in the last 10 minutes"
```

---

## 相关文档

- [02 - Pod 与容器生命周期事件](./02-pod-container-lifecycle-events.md) - Pod 终止和驱逐事件
- [05 - 调度与抢占事件](./05-scheduling-preemption-events.md) - Pod 调度和资源分配
- [06 - 节点生命周期与状态事件](./06-node-lifecycle-condition-events.md) - 节点压力和驱逐
- [07 - Deployment 与 ReplicaSet 事件](./07-deployment-replicaset-events.md) - FailedCreate 事件

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 14/15
