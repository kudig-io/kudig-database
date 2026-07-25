---
sources:
- "集群基础/设计原则/20-server-side-apply-field-ownership.md"
title: Server-Side Apply 与字段所有权 (FieldOwnership)
summary: 深入解析 Kubernetes Server-Side Apply 的字段管理器、ManagedFields、冲突检测与 1.33+ FieldOwnership 演进。
category: concepts
tags:
- server-side-apply
- ssa
- managed-fields
- field-ownership
- declarative
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- Operator 开发者
estimated_read_time: 25min
intent_queries:
- Server-Side Apply 是什么
- SSA 字段所有权如何工作
- ManagedFields 冲突如何解决
- FieldOwnership 1.33 新特性
trigger_keywords:
- Server-Side Apply
- SSA
- ManagedFields
- FieldOwnership
- 字段所有权
- conflict
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 特别提醒：Server-Side Apply 的 `--force-conflicts` 与 `fieldManager=X force=true` 会**静默剥夺其他管理器（可能是某个关键控制器）的字段所有权**，在多控制器协同的环境中（如 HPA、Deployment Controller、Operator 共管同一对象）极易造成"漂移 → 回写 → 再漂移"的死循环。任何 force 接管操作必须先通过 `--dry-run=server` 预检，并明确该字段当前的所有者是谁。

# Server-Side Apply 与字段所有权 (FieldOwnership)

Server-Side Apply（SSA）是 Kubernetes 在 **1.22 正式发布（GA）** 的服务端 apply 机制，它把"谁拥有这个字段的修改权"这一问题从客户端搬到了 API Server。核心思想非常简单：**每个字段在 etcd 中都关联到写入它的 FieldManager（字段管理器）**，多个 manager 可以并存，各自拥有对象的不同子集字段，互不覆盖。

本文是 [[01-集群基础/02-设计原则/06-resource-version-control.md|资源版本控制]] 中 SSA / Managed Fields 章节的**深度补强专题**。后者聚焦于 ResourceVersion 的乐观并发与 410/409 治理；本文则下沉到**字段级**——ManagedFields 的内部表示、conflict 检测算法、force 接管语义、`FieldOwnership` 在 1.33+ 的演进方向，以及多 controller 协同的工程实践。

---

## 一、概述

### 1.1 SSA 要解决的根本问题

在传统（client-side）apply 时代，多个自动化系统（kubectl、Helm、Argo CD、自定义 Operator、HPA、Deployment Controller……）同时修改同一个对象时，会发生两类灾难：

| 痛点 | 表现 | 后果 |
|------|------|------|
| **互相覆盖** | A 把 `replicas=3`，HPA 把 `replicas=5`，全量 Update 互相冲刷 | 扩缩容失效、抖动 |
| **无法归因** | 某字段被改了，但集群里查不到"是谁改的" | 故障排查极其困难 |
| **删除语义不清** | YAML 里删掉一行，apply 后集群里的字段却不消失 | 配置漂移 (drift) |
| **last-applied 丢失** | 新机器没有 `kubectl.kubernetes.io/last-applied-configuration` 注解，apply 退化为 replace | 字段被误删 |

SSA 的答案是：**让 API Server 自己记录每个字段由谁拥有**，apply 请求变成"我想声明对这些字段的所有权"。冲突在服务端被检测、被拒绝（HTTP 409），从而把"覆盖"问题升级为可观测、可处理的"所有权协商"问题。

### 1.2 一句话定义

> Server-Side Apply 是一种以 `Content-Type: application/apply-patch+yaml` 提交的 PATCH 操作；API Server 会比较"声明者想拥有的字段集合"与"对象当前各字段的归属"，据此决定接受、合并或拒绝（409 Conflict）。

### 1.3 关键概念速查表

| 概念 | 英文 | 说明 |
|------|------|------|
| 字段管理器 | FieldManager | 用 `name + operation + apiVersion` 标识"是谁在改" |
| 字段所有权 | Field Ownership | 某字段被某个 FieldManager 独占拥有 |
| Managed Fields | ManagedFields | `metadata.managedFields[]`，记录每个 manager 拥有的字段集合 |
| 字段树 | FieldsV1 (SSA Tree) | `fieldsV1` 中字段路径到空对象 `{}` 的映射 |
| 冲突 | Conflict | Apply 时想写的字段已被他人拥有 → HTTP 409 |
| 强制接管 | Force Takeover | `--force-conflicts` 剥夺他人所有权 |
| 应用操作 | Apply (operation) | `operation: Apply`，对应 SSA；`operation: Update` 对应普通 PUT/UPDATE |

---

## 二、演进：从 Client-Side Apply 到 SSA

### 2.1 Client-Side Apply（kubectl 本地三方合并）

传统 `kubectl apply -f` 的工作流是**纯客户端**的：

```
1. 读取本地 YAML（desired）
2. GET 集群对象（current）
3. 读取注解 kubectl.kubernetes.io/last-applied-configuration（last）
4. 客户端做 3-way merge：以 last 为基线，算出 desired 相对 last 的差异
5. 把 diff 编码成 strategic merge patch，PATCH 给 API Server
```

```
        ┌─────────────┐
desired │ pod.yaml    │  本地新 YAML
        └──────┬──────┘
               │  diff
        ┌──────▼──────┐
last    │ annotation  │  上次 apply 的快照（存在注解里）
        └──────┬──────┘
               │  3-way merge (kubectl 本地完成)
        ┌──────▼──────┐
current │ GET object  │  集群当前状态
        └──────┬──────┘
               │
        ┌──────▼──────┐
patch   │ strategic   │ → PATCH /api/v1/namespaces/.../pods/x
        │ merge patch │
        └─────────────┘
```

**致命缺陷**：

1. **依赖 `last-applied-configuration` 注解**。如果对象是 `kubectl create` 创建的、或被 Controller 直接 Update 的，没有这条注解，kubectl 退化为"只增不删"的合并，删除字段不生效。
2. **客户端无法感知服务端的字段所有权**。HPA 改了 `replicas`，kubectl 下次 apply 会**无感知地覆盖**它。
3. **归因信息丢失**。集群里只有"最后一次完整对象"，查不到"哪个子系统改了哪个字段"。
4. **`last-applied` 注解膨胀**。注解里存整个对象快照，大对象（CRD）会让对象体积显著膨胀。

### 2.2 Server-Side Apply（服务端字段所有权追踪）

SSA 把合并逻辑搬到 API Server 内部：

| 维度 | Client-Side Apply | Server-Side Apply |
|------|-------------------|-------------------|
| 合并位置 | kubectl 本地 | API Server（apiserver 内 structured-merge-diff 库） |
| 基线来源 | `last-applied-configuration` 注解 | `metadata.managedFields`（服务端权威） |
| 字段所有权 | 无概念 | 每字段归属某个 FieldManager |
| 冲突表现 | 静默覆盖 | HTTP 409，明确拒绝 |
| 删除语义 | 依赖注解，易失效 | desired 中不出现的字段 → 自动 prune |
| Content-Type | `application/strategic-merge-patch+json` | `application/apply-patch+yaml` |
| 资源版本 | 1.5+（kubectl 本地） | 1.16 beta / **1.22 GA** |

**演进时间线**：

- **1.14**：alpha，仅部分资源支持。
- **1.16**：beta，默认开启 `--validate=false` 不再需要，kubectl 可通过 `KUBECTL_APPLY_BACKEND=server-side` 试用。
- **1.18**：`metadata.managedFields` 出现在所有对象的元数据中（即使没用 SSA，Update 操作也会写入 `operation: Update` 的记录）。
- **1.22**：**GA（General Availability）**，`kubectl apply` 在新版 kubectl 中默认走 SSA。
- **1.32 / 1.33**：CRD 的 SSA 支持成熟；OpenAPI v3 schema 让 CRD 的合并策略更精确；kubectl 增加配置别名/默认 flags 的能力（含 SSA 默认开关）。社区持续讨论"字段所有权的显式化、可移交性"作为后续演进方向。

---

## 三、字段所有权模型

### 3.1 FieldManager 的四要素

每个对对象的修改都会被归到一个 FieldManager。FieldManager 由四个维度唯一刻画：

| 要素 | 字段 | 示例 | 含义 |
|------|------|------|------|
| 名称 | `manager` | `"my-operator"` | 管理器逻辑名（用户/控制器自定义） |
| 操作类型 | `operation` | `"Apply"` / `"Update"` | 是 SSA Apply 还是普通 Update |
| API 版本 | `apiVersion` | `"v1"` / `"apps/v1"` | 写入时使用的 API 版本 |
| 子资源 | `subresource` | `"scale"` / `"status"` / 空 | 写的是哪个子资源（主资源 / scale / status） |
| 时间 | `time` | `"2026-07-23T10:00:00Z"` | 该 manager 最近一次接管/释放字段的时间 |

> 注意：`manager` 名并不是身份认证，而是**逻辑标识**。它的真实值由 kube-apiserver 根据 HTTP `User-Agent` 或客户端显式传入的 `fieldManager` 参数推导：
> - kubectl client-side apply → `kubectl-client-side-apply`
> - kubectl server-side apply（未指定）→ `kubectl-edit` / `kubectl-client-side-apply` 取决于场景
> - controller-runtime `client.FieldOwner("x")` → `"x"`
> - 未指定 fieldManager 的 Update → 由 User-Agent 截断到 128 字符

### 3.2 字段归属规则

**每个叶子字段（leaf field）在任意时刻最多被一个 FieldManager 拥有。**

这是 SSA 不变式（invariant）的核心：

- `spec.replicas` 当前被 `hpa` 拥有 → 别人 Apply `replicas=3` 会 409。
- `spec.template.spec.containers[name=app].image` 被 `kubectl` 拥有 → Operator Apply 同字段也会 409。

但**结构化的中间节点**（如 `spec.template` 本身）可以被多个 manager "经过"——它们各自拥有其下的不同叶子。所有权是**叶子级**的，不是路径级。这一点在阅读 `fieldsV1` 时至关重要。

### 3.3 Set-Based 集合模型

整个对象的"所有权地图"可形式化为：

```
ManagedFields = [ Manager₁ → Set₁,  Manager₂ → Set₂, ... ]
约束: ∀ i≠j:  Setᵢ ∩ Setⱼ = ∅   (叶子字段两两不相交)
    ⋃ Setᵢ ⊆ 全部叶子字段        (未被任何 manager 触碰的字段不在任何 Set 中)
```

Apply 操作的形式语义就是：

```
对 desired 中的每个叶子字段 f:
    if f 被 others 拥有 且 force=false:
        → Conflict (409)
    else:
        Set_new_manager := Set_new_manager ∪ {f}     // 接管
        (若 f 原属他人且 force=true，从原 Set 移除)
```

### 3.4 Apply vs Update：用户态与控制器态

`operation` 字段区分了两种"取得所有权"的方式，对应两类客户端：

| operation | 触发方式 | 接管语义 |
|-----------|----------|----------|
| `Apply` | `Content-Type: application/apply-patch+yaml` | **精确声明**：desired 出现的字段才拥有；desired 未出现的字段→自动释放（prune）。这是"用户态"声明式管理。 |
| `Update` | `PUT` / `PATCH`（strategic/json/merge） | **粗粒度夺取**：写入路径下的所有已存在字段都会被该 manager 接管。这是"控制器态"，因为 controller 通常 GET → 改内存 → Update 整对象。 |

这个区分决定了多 controller 协同的两种典型模式（见第八节）。**一个常见误区**：以为"只要用了 SSA 就不会冲突"。事实上，如果一个 controller 用 `Update` 写了整个 `spec`，它就夺走了 `spec` 下所有字段的 ownership，随后用户的 Apply 反而会冲突。

---

## 四、ManagedFields 内部结构

### 4.1 完整 YAML 示例

以一个 Deployment 为例，它的 `metadata.managedFields` 通常长这样：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: default
  resourceVersion: "1234567"
  generation: 3
  managedFields:
  - manager: kubectl-client-side-apply        # 历史遗留：以前用 client-side apply
    operation: Update
    apiVersion: v1
    time: "2024-01-15T10:00:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations:
          .: {}
          f:kubectl.kubernetes.io/last-applied-configuration: {}
      f:spec:
        f:replicas: {}

  - manager: my-operator                      # 自定义 Operator（SSA）
    operation: Apply
    apiVersion: apps/v1
    time: "2026-07-23T08:30:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:spec:
        f:template:
          f:spec:
            f:containers:
              k:{"name":"app"}:
                .: {}
                f:image: {}
                f:resources:
                  f:limits:
                    f:cpu: {}

  - manager: horizontal-pod-autoscaler        # HPA 改 replicas
    operation: Update
    apiVersion: autoscaling/v1
    subresource: scale
    time: "2026-07-23T09:15:00Z"
    fieldsType: FieldsV1
    fieldsV1:
      f:metadata:
        f:annotations: {}
      f:spec:
        f:replicas: {}
```

### 4.2 fieldsV1：SSA 字段树

`fieldsV1` 是一颗用 YAML 表示的字段路径树，称为 **SSA Tree** 或 **FieldsV1**。规则：

| 节点写法 | 含义 |
|----------|------|
| `f:replicas: {}` | 标量叶子字段 `replicas` 被拥有 |
| `f:containers: { k:{"name":"app"}: {...} }` | 列表项，用 `k:` 前缀的 **set key**（通常是 `name` 或 merge key 的 JSON）定位 |
| `.: {}` | 当前路径节点本身（结构体/对象层级）被拥有 |
| `f:annotations:\n  .: {}\n  f:foo: {}` | map 类型：`.` 表示 map 本身被拥有，`f:foo` 表示具体 key `foo` 被拥有 |

**关键点**：
1. **只有叶子节点才是真正的"所有权"**。中间的 `f:spec: f:template:` 只是为了路径表达，不构成独立所有权。
2. **列表项用 set key 定位**（`k:{"name":"app"}`），这是 SSA 能精确合并 list 而不冲刷其他元素的根本原因。CRD 必须在 OpenAPI schema 中声明 `x-kubernetes-list-map-keys` 才能享受这种合并。
3. **空对象 `{}` 是占位符**，不代表值。SSA Tree 编码的是"路径集合"，不是"值"。

### 4.3 时间字段 time 的含义

`time` 是该 manager **最近一次改变其拥有集合的时间**——不是"上次写值"的时间。如果该 manager 的字段集合没变（只是值变了），`time` 不会更新。这点在做漂移检测时容易误判。

### 4.4 多 manager 的并集视图

把上例三个 manager 的 Set 合起来，相当于对象上所有"被追踪字段"的全集：

```
kubectl-client-side-apply : { metadata.annotations.last-applied-configuration, spec.replicas(历史) }
my-operator               : { spec.template.spec.containers[name=app].image,
                             spec.template.spec.containers[name=app].resources.limits.cpu }
horizontal-pod-autoscaler : { metadata.annotations, spec.replicas }
```

注意 `spec.replicas` 最终归属 HPA——因为它最后一次通过 `scale` 子资源写入了这个字段，从 `kubectl-client-side-apply` 手中"夺走"了所有权。这就是 Update 接管语义的体现。

---

## 五、Conflict 检测算法

### 5.1 什么情况下报 409

当一次 Apply 操作试图"声明所有权"的字段已经被**其他 manager** 拥有时，API Server 返回：

```
HTTP/1.1 409 Conflict
{
  "kind": "Status",
  "code": 409,
  "message": "Apply failed with 1 conflict: conflict... please apply ... with fieldManager X ... to force the changes",
  "reason": "Conflict",
  "details": {
    "causes": [
      {"reason": "FieldManagerConflict",
       "message": "field \"spec.replicas\" is owned by \"horizontal-pod-autoscaler\""}
    ]
  }
}
```

### 5.2 算法形式化

```
INPUT:
  obj     : 当前集群对象（含其 managedFields）
  desired : 本次 Apply 提交的声明对象（按 SSA merge 算法展开为字段集合）
  fm_self : 本次 Apply 的 FieldManager

ALGORITHM:
  conflicts := []
  for each leaf field f in desired:
      owner := lookup_owner(obj.managedFields, f)
      if owner exists AND owner != fm_self:
          conflicts.append( (f, owner) )

  if conflicts is non-empty AND force == false:
      return HTTP 409  with causes = conflicts
  else if conflicts is non-empty AND force == true:
      for each (f, owner) in conflicts:
          remove f from owner.Set
          add   f to fm_self.Set
      proceed to merge
  else:
      proceed to merge   // 正常合并 + 接管新字段
```

关键性质：
- **只对 desired 中出现的字段做冲突检查**。desired 里没写的字段（哪怕属于他人）不会被检查，也**不会被释放**（除非配 prune 选项且字段类型支持）。
- **冲突是"单向声明冲突"**：你声明要 X 字段，而别人已经拥有 X。这与乐观锁的 RV 冲突（409 on PUT）是**两套独立机制**——一个 SSA 请求可能在 RV 层面成功，却在字段层面冲突；反之亦然。

### 5.3 Apply 与 Update 的冲突差异

| 维度 | Apply（operation=Apply） | Update（operation=Update） |
|------|--------------------------|----------------------------|
| 冲突检测 | 严格：每个声明字段都查归属 | 不触发字段冲突，但**会夺取目标字段所有权** |
| 其他 manager 的字段 | 拒绝（409） | 静默接管（剥夺原 owner） |
| 典型客户端 | kubectl apply、controller-runtime Apply patch | controller 的 PUT/Update、HPA scale |
| 设计意图 | 协商式声明 | 紧急/权威修改 |

**这导致一个反直觉现象**：一个用 `Update` 的 controller 永远不会因 SSA 冲突而失败，但它会**无声地吃掉用户 Apply 的字段所有权**。用户下次 Apply 就开始报 409。生产中这类"谁先 Update 谁赢"的拉锯战非常常见。

### 5.4 一个完整的冲突诞生过程（worked trace）

下面这个时间线展示了"看起来无害"的日常操作如何演化成 409 死结。这是生产中最高频的故障剧本：

```
T0  集群空。managedFields = []

T1  CI 首次部署 (server-side apply):
    kubectl apply --server-side --field-manager=ci-deployer -f deploy.yaml
    desired = { spec.replicas: 3, spec.template...[image: v1] }
    → 无任何字段被他人拥有, 全部接受
    managedFields:
      ci-deployer (Apply): { spec.replicas, spec.template.spec.containers[name=app].image }

T2  运维为业务高峰手工扩容 (注意: 用了 kubectl scale, 走 /scale 子资源 Update):
    kubectl scale deploy/nginx --replicas=10
    → operation=Update, subresource=scale, manager="kubectl-scale"
    → Update 静默夺取 spec.replicas 的 ownership
    managedFields:
      ci-deployer (Apply): { spec.template.spec.containers[name=app].image }   ← replicas 被夺走
      kubectl-scale (Update, scale): { spec.replicas }

T3  业务接入 HPA:
    hpa controller 通过 /scale 子资源 Update 写 replicas
    → Update 再夺权, manager="horizontal-pod-autoscaler"
    managedFields:
      ci-deployer (Apply):            { spec.template...image }
      horizontal-pod-autoscaler (Update, scale): { spec.replicas }

T4  CI 推送镜像新版 (同样的 deploy.yaml, 仍含 replicas: 3):
    kubectl apply --server-side --field-manager=ci-deployer -f deploy.yaml
    desired 含 spec.replicas: 3
    → 冲突! spec.replicas 属于 HPA
    → HTTP 409, 镜像也没更新 (整个 Apply 被拒, 不是部分成功)

T5  现场尝试 "绕过":
    kubectl apply --server-side --force-conflicts -f deploy.yaml
    → ci-deployer 夺回 replicas, 值=3
    → 但 HPA 下一轮 (T6) 又 Update 抢回 replicas=8
    → 死循环开始
```

**从这个 trace 得出的工程结论**：

1. **YAML 里出现的字段 = 你要持续声明的字段**。把 `replicas` 留在 Git 仓库里，就是和 HPA 签了长期对抗协议。修复办法：YAML 删掉 `replicas`（让 CI 的 desired 不再声明它），或加 `# +kustomize` 注释让工具链过滤。
2. **409 是原子的**。一次 Apply 要么全成功要么全失败，不会"镜像更新了但 replicas 冲突回滚"。因此镜像也发不上去——这正是 CI 卡死的常见原因。
3. **Update 是"夺权"而非"声明"**。任何走 Update 的控制器都会无声侵蚀 Apply 客户端的 ownership，且不留任何冲突记录。

### 5.5 子资源的影响

冲突检测**分主资源 / 子资源**：

- `spec.replicas` 由主资源 Apply 检查。
- `scale` 子资源的 `spec.replicas` 实际指向同一字段，但 `fieldManager` 带 `subresource: scale`。
- HPA 通过 `/scale` 子资源写 → manager 是 `horizontal-pod-autoscaler` + `subresource: scale`。
- 用户在主资源里 Apply `spec.replicas` → 会被这个带 `scale` subresource 的 manager 拦下，报 409。

这就是"HPA 与 kubectl apply 抢 replicas"的根因，也是为什么生产中 `replicas` 字段建议**完全交给 HPA**，YAML 中留空或加 `+kubebuilder:default` 注释让其由控制器决定。

---

## 六、Force 接管（强制夺取所有权）

### 6.1 何时需要 force

| 场景 | 是否建议 force |
|------|----------------|
| 旧的 controller 已下线，残留字段所有权需要迁移 | ✅ |
| 应急修复：某个被 bug controller 锁住的字段必须立即改 | ✅（短期） |
| 日常部署中遇到 409，想"省事"绕过 | ❌ 强烈不建议 |
| 与 HPA/Deployment Controller 共管字段时强抢 | ❌ 会引发循环 |

### 6.2 kubectl force

```bash
# 🟡 中风险：强制接管字段所有权，会覆盖其他管理器（可能是 HPA/Controller）的声明
kubectl apply --server-side \
  --field-manager=my-controller \
  --force-conflicts \
  -f deploy.yaml
```

`--force-conflicts` 等价于 `PatchOptions.Force = pointer.Bool(true)`。它会让 API Server：

1. 把冲突字段从原 owner 的 Set 中**移除**；
2. 加入本次 `fieldManager` 的 Set；
3. **用本次 desired 的值覆盖**。

### 6.3 编程式 force（controller-runtime）

```go
import (
    "sigs.k8s.io/controller-runtime/pkg/client"
)

patchOpts := []client.PatchOption{
    client.FieldOwner("my-operator"),
    client.ForceOwnership,   // 等价于 Force=true
}
err := r.Patch(ctx, desired, client.Apply, patchOpts...)
```

### 6.4 force 之后会发生什么

假设原 owner 是 HPA，被你 force 抢了 `replicas`：

1. **立即**：字段值变成你的 desired 值，ownership 转移到 `my-controller`。
2. **下一轮 HPA reconcile**：HPA 通过 `/scale` Update 写 `replicas` → **Update 会静默再夺回所有权**。
3. **结果**：你下次 Apply 又 409，循环开始。

这就是为什么 force 必须配合"停掉竞争者"或"协商好 ownership 边界"才有意义。**force 不是日常工具，是迁移/应急工具。**

### 6.5 Prune：放弃所有权与字段删除

Apply 还有一对常被混淆的语义：**放弃所有权** vs **删除字段值**。它们对应 `PatchOptions` 的两个开关：

| 选项 | 默认 | 行为 |
|------|------|------|
| `force` | false | 夺取他人所有权 |
| （隐式 prune） | 开启 | desired 中不再出现的、且属于**本 manager** 的字段 → 自动从对象删除 |
| `--prune` (kubectl) | false | 跨对象 prune：删除本次 apply set 之外、由同 manager 拥有的其它对象 |

关键规则：

1. **你只能 prune 自己拥有的字段**。desired 里删掉某字段，只会删除**你自己 manager 拥有**的那个字段；属于他人的字段即使不在 desired 里，也不会被删。这是 SSA 防止互相"暗杀"的安全网。
2. **Update 永远不 prune**。Update 把对象整体写一遍，所有权会扩散但旧字段不会被 SSA 主动清理。
3. **跨对象 prune（kubectl `--prune`）依赖 label/owner 约束**，不要与 SSA 字段 prune 混为一谈。

```bash
# 🟡 中风险：apply set 之外、由本 manager 拥有的对象会被删除
kubectl apply --server-side \
  --field-manager=ci-deployer \
  --prune \
  --prune-allowlist apps/v1/Deployment \
  -f manifests/
```

> **常见坑**：用户期望"YAML 删了字段就生效"，结果发现集群里字段还在。99% 的原因是该字段属于另一个 manager（如某个控制器写入的默认值），Apply 客户端无权 prune 它。诊断办法：查 `managedFields` 确认该字段归属。

---

## 七、FieldOwnership（1.33+ 的演进方向）

> **诚实说明（重要）**
>
> 截至本文撰写时（Kubernetes 1.33 / 2026 年中），**并没有一个命名为 `FieldOwnership` 的独立 GA FeatureGate 在 1.33 发布**。`managedFields` 仍然是字段所有权模型的**唯一权威载体**。社区在 KEP-555（Server-Side Apply）及其后续讨论中，持续探索如何让所有权语义更**显式、可观测、可移交**，但具体的 FeatureGate 名称、阶段（alpha/beta）在不同小版本之间仍在演进。本节描述的是**演进方向与设计意图**，而非一个已固化的产品功能。请在生产前以你目标版本的官方 release notes 与 [KEP-555](https://github.com/kubernetes/enhancements/blob/master/keps/sig-api-machinery/555-server-side-apply/README.md) 为准。

### 7.1 为什么需要"更强的所有权语义"

当前 `managedFields` 模型有几个长期痛点，这些是 FieldOwnership 概念演进的驱动力：

| 痛点 | 现状 |
|------|------|
| **所有权不可显式声明** | 字段归属是"写出来的副作用"，没有 API 让你显式说"我要声明这块所有权但不改值" |
| **移交无原子语义** | 把一组字段从 manager A 移交到 manager B，需要 A 先 Apply（放弃）再 B Apply，中间窗口存在冲突 |
| **managedFields 体积膨胀** | KEP-555 指出它可占对象体积的 **60%**，大 CRD 集群的 etcd/网络压力可观 |
| **CRD schema 与合并策略耦合不透明** | list map keys、atomic vs granular 合并策略藏在 OpenAPI v3 注解里，调试困难 |
| **所有权生命周期不可查** | 没有"该字段历史上属于过谁"的记录，迁移审计困难 |

### 7.2 FieldOwnership 作为概念模型

"FieldOwnership" 在社区语境中更应被理解为**对 managedFields 语义的进一步抽象**，目标是：

- **所有权声明与值写入解耦**：未来可能允许"仅声明 ownership，不改值"的操作，便于平滑迁移。
- **分阶段移交（Gradual Ownership Transfer）**：允许新旧 manager 在若干次 reconcile 中逐步交接字段，而不是一刀切 force。
- **更紧凑的表示**：探索比 FieldsV1 tree 更节省的 ownership 编码（这是缓解 60% 体积问题的关键方向）。
- **更可观测**：把 ownership 变更纳入审计事件、metrics，使迁移过程可追踪。

### 7.3 FieldOwnership 与现有 managedFields 的关系

| 维度 | managedFields（当前） | FieldOwnership（演进方向） |
|------|----------------------|---------------------------|
| 载体 | `metadata.managedFields[]`（内联） | 仍基于该结构，探索更紧凑/外置表示 |
| 语义 | 写副作用（隐式） | 显式声明（解耦值与所有权） |
| 移交 | force（剥夺式） | 协商/分阶段（保留式） |
| 适用 | 全版本（1.18+） | 未来版本，需以 release notes 为准 |
| 你今天该用哪个 | **就用 managedFields** | 关注 KEP 进展，**不要**在生产中等待假设中的 FeatureGate |

### 7.4 1.32 / 1.33 的实际相关进展

尽管没有名为 `FieldOwnership` 的独立 gate，1.32–1.33 在 SSA 周边确实有可落地的改进，这些是字段所有权模型的真实演进：

- **CRD OpenAPI v3 schema 的合并策略成熟**：`x-kubernetes-list-type=map` + `x-kubernetes-list-map-keys` 让自定义资源也能享受精确的 SSA 字段合并与 ownership 追踪。这是 Operator 作者最该关注的"让 CRD 字段所有权可追踪"的杠杆。
- **kubectl 配置别名 / 默认 flags**（1.33 alpha）：可在 `kubectl config` 中把 `--server-side` 设为默认，推动 SSA 普及。
- **managedFields 序列化优化**：社区持续在减小 managedFields 的序列化体积（与 KEP-555 一脉相承）。
- **conflict 错误信息增强**：409 响应里更明确地提示"apply with fieldManager=X force=true"，降低上手门槛。

> **结论性建议**：在 1.33 生产中，把"FieldOwnership"理解为"**用 managedFields + 显式 fieldManager 命名 + CRD schema 合并策略**三者组合出来的所有权治理能力"，而非等待某个 FeatureGate。这是今天就能落地的最强形式。

---

## 八、多 Controller 协同模式

这是 SSA 真正发挥价值的场景。典型的"多写者共管"组合：

```
                    ┌──────────────────┐
                    │  Deployment 对象  │
                    └────────┬─────────┘
                             │ 各自拥有不同字段
        ┌────────────────────┼────────────────────┐
        │                    │                    │
   spec.replicas      spec.template           spec.strategy
   (HPA via scale)    (kubectl / CI)          (Operator)
```

### 8.1 模式一：用户 Apply + 控制器 Update（经典模式）

- **用户/CI**：用 `kubectl apply --server-side --field-manager=ci-deployer`，声明 `spec.template`（镜像、env、resources）。
- **HPA**：通过 `/scale` 子资源 Update，拥有 `spec.replicas`。
- **Deployment Controller**：内部循环，拥有 `metadata.annotations[deployment.kubernetes.io/revision]` 等系统字段。

**字段边界清晰，互不冲突**，这是推荐的稳态。

### 8.2 模式二：多控制器都用 Apply（推荐）

每个控制器只声明自己关心的字段子集，用不同的 `fieldManager` 名。这样 controller 之间也是协商式而非覆盖式：

```go
// 控制器 A：只管镜像
func (r *ImageReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    desired := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{Name: req.Name, Namespace: req.Namespace},
        Spec: appsv1.DeploymentSpec{
            Template: corev1.PodTemplateSpec{
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{
                        {Name: "app", Image: r.DesiredImage},
                    },
                },
            },
        },
    }
    return ctrl.Result{}, r.Patch(ctx, desired, client.Apply,
        client.FieldOwner("image-controller"),
        client.ForceOwnership,
    )
}

// 控制器 B：只管副本数与策略
func (r *ScalingReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    desired := &appsv1.Deployment{
        ObjectMeta: metav1.ObjectMeta{Name: req.Name, Namespace: req.Namespace},
        Spec: appsv1.DeploymentSpec{
            Replicas: pointer.Int32(r.DesiredReplicas),
            Strategy: appsv1.DeploymentStrategy{Type: appsv1.RollingUpdateDeploymentStrategyType},
        },
    }
    return ctrl.Result{}, r.Patch(ctx, desired, client.Apply,
        client.FieldOwner("scaling-controller"),
    )
}
```

**注意 `client.Apply` 的"零值陷阱"**：SSA 的 desired 是"完整声明"，未被拥有的标量零值会被写进去。因此 Apply 的对象应**只填要声明的字段**，其余留空（Go 零值）。上面的 `ImageReconciler` 故意不填 `Replicas`——如果填了 0，会声明"我要把 replicas 设为 0"，把 HPA 的副本数清零。

### 8.3 模式三：Controller 用 Update + 用户用 Apply（常见但易翻车）

很多存量 Operator 用 `Get → 改内存 → Update` 写 status / spec。这会让 controller 隐式夺走所有写入字段的 ownership。用户随后在 YAML 里改这些字段会开始 409。

**缓解策略**：
- status 子资源用 Update 是合理的（status 本就是控制器领域）。
- spec 子资源尽量改用 Apply + `FieldOwner`。
- 若必须 Update，明确"我只该改 spec 的某子树"，并在文档中声明 ownership 边界。

### 8.4 controller-runtime 的 SSA 实践要点

```go
// 完整的 SSA patch 调用
err := r.Patch(ctx, desired, client.Apply,
    client.FieldOwner("my-operator"),     // 必须唯一且稳定
    client.ForceOwnership,                // 仅在确需接管时开启
)
```

要点清单：

| 要点 | 说明 |
|------|------|
| `FieldOwner` 名必须唯一稳定 | 不要用随机/pod 名，否则每次重启都是新 manager，残留一堆 ownership |
| 仅声明自己关心的字段 | 避免零值覆盖（SSA 不会区分"0 是声明"还是"0 是没设"） |
| status 用 `r.Status().Patch(..., client.Apply, client.FieldOwner(...))` | 主资源与 status subresource 的 ownership 隔离 |
| 不要在每个 reconcile 都 force | force 应是配置态决策，不是循环里默认 |
| 对 CRD 启用 OpenAPI v3 + list map keys | 否则 SSA 退化为 atomic 合并，list 会被整体替换 |

---

## 九、生产实践

### 9.1 诊断命令

```bash
# 🟢 只读：查看对象所有字段管理器
kubectl get deployment nginx -o jsonpath='{.metadata.managedFields}' | jq '.'

# 🟢 只读：查看特定字段的管理者及其 fieldsV1
kubectl get deployment nginx -o json | jq '.metadata.managedFields[] | {manager, operation, subresource, fieldsV1}'

# 🟢 只读：列出所有 manager 及其管理的字段数量（粗略）
kubectl get deployment nginx -o json | \
  jq '.metadata.managedFields[] | {manager, fields: (.fieldsV1 | tostring | length)}'

# 🟢 只读：找出谁拥有 spec.replicas（人工读 fieldsV1）
kubectl get deployment nginx -o json | \
  jq '.metadata.managedFields[] | select(.fieldsV1.f.spec.f.replicas) | .manager'

# 🟢 只读：检查 SSA 冲突（server-side dry-run，不改集群）
kubectl apply --server-side --field-manager=ci-deployer --dry-run=server -f deploy.yaml
```

### 9.2 漂移检测（CI 集成）

CI 流水线里用 SSA dry-run 做"声明 vs 集群"漂移检测：

```bash
# 🟢 只读：dry-run=server 模拟 Apply，输出会包含 409 冲突或 patch 差异
kubectl apply --server-side \
  --field-manager=ci-deployer \
  --dry-run=server \
  -f deploy.yaml

# 🟢 只读：用 diff 看声明与集群差异（kubectl 1.18+）
kubectl diff --server-side --field-manager=ci-deployer -f deploy.yaml
```

- **退出码 0**：无差异（无漂移）。
- **退出码 1**：有差异（漂移或冲突），CI 可据此告警或阻断。
- **退出码 >1**：真实错误。

注意 `kubectl diff` 在冲突场景下也会非零退出，需在 CI 脚本里区分"差异"与"冲突"。

### 9.3 接管操作（谨慎）

```bash
# 🟡 中风险：强制接管字段所有权（剥夺其他 manager）
kubectl apply --server-side \
  --field-manager=my-controller \
  --force-conflicts \
  -f deploy.yaml

# 🔴 高风险：先 force 再立刻删字段（prune），可能误删被他人拥有的字段
kubectl apply --server-side \
  --field-manager=my-controller \
  --force-conflicts \
  --prune \
  -f deploy.yaml
```

> **force 黄金法则**：force 前先 `--dry-run=server` 看 409 里列出的 `causes`，确认你**真的**要从那个 manager 夺权，而不是误伤。

### 9.4 Controller 必备实践

| 实践 | 理由 |
|------|------|
| 唯一稳定的 `fieldManager` name（如 `myapp-controller`） | 重启不残留，审计可追踪 |
| spec 用 Apply，status 用 Status().Apply | 主资源 / 子资源 ownership 隔离 |
| desired 只填要声明的字段 | 避免零值覆盖 |
| CRD 启用 OpenAPI v3 + `x-kubernetes-list-type=map` | list 才能精确 ownership |
| 暴露 ownership metrics（如 `field_manager_conflicts_total`） | 多 controller 协同可观测 |
| 永远不默认 `ForceOwnership` | force 是迁移工具，不是常态 |

### 9.5 CRD 合并策略示例

```yaml
# 一个支持 SSA 精确合并的 CRD 片段
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: apps.example.com
spec:
  group: example.com
  versions:
  - name: v1
    served: true
    storage: true
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              replicas:
                type: integer
              regions:                       # ← atomic: 整体替换
                type: array
                x-kubernetes-list-type: atomic
              backends:                      # ← map by name: 元素级 ownership
                type: array
                x-kubernetes-list-type: map
                x-kubernetes-list-map-keys: [name]
                items:
                  type: object
                  properties:
                    name: {type: string}
                    weight: {type: integer}
```

- `atomic` 列表：任何元素变动 = 整列表换主，**无法**多 manager 共享。
- `map` 列表（按 key）：不同 manager 可分别拥有不同 backend 元素，**这是 CRD 实现"字段级协同"的关键**。

### 9.6 监控指标（Prometheus）

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ssa-ownership-alerts
  namespace: monitoring
spec:
  groups:
    - name: server-side-apply
      rules:
        - alert: HighSSAConflictRate
          expr: |
            sum(rate(apiserver_request_total{code="409", verb="PATCH"}[5m])) by (resource) > 5
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "SSA 冲突率过高 ({{ $value | printf \"%.1f\" }}/s)，检查 fieldManager 竞争"

        - alert: ManagedFieldsBloat
          expr: |
            avg(kube_object_managedFields_bytes / kube_object_size_bytes) > 0.5
          for: 30m
          labels:
            severity: info
          annotations:
            summary: "managedFields 占对象体积 >50%，考虑迁移到更紧凑的所有权模型"
```

### 9.7 常见问题速查

| 现象 | 根因 | 解决 |
|------|------|------|
| Apply 报 409，提示 `owned by "horizontal-pod-autoscaler"` | HPA 拥有 `replicas` | YAML 删掉 `replicas`，或停 HPA |
| Controller 重启后残留一堆 ownership | `fieldManager` 用了 pod/随机名 | 改用固定 manager name，必要时 force 清理 |
| Apply 后字段没被删除 | desired 没加 `--prune` 或该字段不在 apply set | 用 `--prune` + 显式 ownership 边界 |
| CRD 的 list 元素被整体替换 | list 是 `atomic` 或没设 `list-type` | 改为 `map` + `list-map-keys` |
| `kubectl diff` 退出码 1 但 Apply 成功 | dry-run 算出的差异是 ownership 差异，不是值差异 | 区分 ownership diff vs value diff |
| force 接管后循环冲突 | 竞争 controller 用 Update 静默夺回 | 停竞争者，或改用 Apply 协商模式 |

---

## 十、设计原则总结

| 原则 | 落地 |
|------|------|
| **字段边界即契约** | 每个 controller 在文档中声明它拥有哪些字段，与 HPA/用户/CI 协商 |
| **声明优于覆盖** | 优先 Apply（声明），慎用 Update（覆盖） |
| **零值即声明** | SSA 不区分 0 和未设；Apply 对象只填要声明的字段 |
| **force 是迁移工具，不是日常工具** | 每次使用需有明确迁移目标与回滚计划 |
| **ownership 可观测** | manager name、metrics、审计事件必须能反查 |
| **CRD schema 决定合并粒度** | 用对 `list-type` 才有真正的字段级协同 |
| **子资源隔离所有权** | status / scale / 主资源分别声明，避免跨边界 |

---

## 十一、相关文档

- [[01-集群基础/02-设计原则/06-resource-version-control.md|资源版本控制]] — 乐观锁、409/410 治理、SSA 与 Managed Fields 章节的本专题源。
- [[01-集群基础/02-设计原则/02-declarative-api-pattern.md|声明式 API]] — 声明式 API 与面向终态设计，SSA 是其服务端实现。
- [[01-集群基础/02-设计原则/12-operator-development-guide.md|Operator 开发指南]] — controller-runtime 的 SSA、Cache、Reconcile 实践。
- [[01-集群基础/02-设计原则/03-controller-pattern.md|控制器模式]] — 控制器与调谐循环，多 controller 共管对象时的协作基础。
- [[01-集群基础/02-设计原则/13-admission-control-webhooks.md|准入控制 Webhooks]] — mutating/validating webhook 与 SSA 的交互顺序。
- [Kubernetes 官方：Server-Side Apply](https://kubernetes.io/docs/reference/using-api/server-side-apply/)
- [KEP-555: Server-Side Apply](https://github.com/kubernetes/enhancements/blob/master/keps/sig-api-machinery/555-server-side-apply/README.md)

---

**作者**: KUDIG Team — 文档版本 v1.0 (2026-07-23)，对应 Kubernetes 1.28–1.33。

<!-- risk-assessed -->
