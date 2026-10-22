---
sources:
- "集群基础/设计原则/07-resource-version-control.md"
- "对话沉淀: 2026-10-20 Service finalizer 与 update/patch 覆盖事故讨论"
title: Finalizer 与删除生命周期 (DeletionLifecycle)
summary: 深入解析 Kubernetes finalizer 删除两阶段机制、update/patch/SSA 三种写入路径对 finalizers 字段的并发语义差异,以及无 RV 整体回写导致云资源孤儿的事故链、嫌疑人清单与工程防护。
category: concepts
tags:
- finalizer
- deletion
- optimistic-concurrency
- resource-version
- update
- patch
- server-side-apply
tier: core
created: 2026-10-20
updated: 2026-10-20
last_updated: 2026-10
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- Operator 开发者
- 平台工程师
estimated_read_time: 20min
intent_queries:
- finalizer 是什么,对象删除为什么卡在 Terminating
- patch 会不会覆盖别的 controller 加的 finalizer
- update 和 patch 的并发写语义差在哪
- Service 删除后云上 SLB 没被清理怎么排查
- 如何安全地摘 finalizer 解卡 Terminating
trigger_keywords:
- finalizer
- deletionTimestamp
- Terminating
- resourceVersion
- strategic merge patch
- load-balancer-cleanup
- 孤儿资源
k8s_versions:
- '1.18'
- '1.22'
- '1.28'
- '1.34'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与资源是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成资源泄漏或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
>
> 特别提醒：手工摘除 finalizer（清空 `metadata.finalizers`）会**跳过所有 controller 的清理逻辑直接放行真删**。执行前必须逐项确认每个 finalizer 对应的外部资源（云 LB/EIP/PV 等）已经清理干净，否则等于亲手制造孤儿资源。

# Finalizer 与删除生命周期 (DeletionLifecycle)

Finalizer 是 K8s 对象身上的"删除锁"：`metadata.finalizers` 是一串字符串，**只要列表非空，DELETE 不会真删对象**，只写 `deletionTimestamp` 让对象卡在 `Terminating`，直到负责清理的 controller 干完活、摘掉自己的 finalizer 才放行。它保护的不是对象本身，而是对象背后那些"K8s 一删就没人管的真实资源"——云 SLB、EIP、云盘、外部 DNS 记录。

本文与 [[01-集群基础/02-设计原则/07-resource-version-control.md|资源版本控制]]、[[22-概念/01-核心架构/03-server-side-apply-field-ownership.md|Server-Side Apply 与字段所有权]] 互为补充：前者讲乐观并发的 409/410 治理，后者讲字段级所有权，本文聚焦**删除链路上 finalizer 的两阶段生命周期**，以及 update / patch / SSA 三种写入路径对 `finalizers` 字段的并发语义差异——一道高频面试陷阱题就在这里：**"外部 controller 用 patch 会覆盖别人加的 finalizer 吗？"方向恰恰说反了。**

---

## 一、finalizer 解决什么问题

### 1.1 核心矛盾：K8s 对象 ≠ 全部真相

删除一个 K8s 对象只是删掉 etcd 里一行记录，但对象往往对应集群外部的真实资源：

| 对象 | 外部欠账 |
| --- | --- |
| `type: LoadBalancer` Service | 云 SLB/CLB/NLB、监听器、EIP（ACK 上由 CCM 管理） |
| PVC | 底层云盘的卸载与释放 |
| 带外部副作用的自定义资源 | 外部 DNS 记录、防火墙规则、第三方系统账号 |

如果对象一删就没，controller 连"要去善后什么"的依据都丢了。finalizer 把删除从"一锤子买卖"变成**两阶段交接**：

1. **标记阶段**：用户 DELETE → API server 发现 finalizers 非空 → 不真删，写 `deletionTimestamp`，对象进入 `Terminating`；
2. **放行阶段**：负责清理的 controller watch 到 `deletionTimestamp` → 清理外部资源 → 从 finalizers 里摘掉自己的条目 → 列表清空后，写路径检查发现"有 deletionTimestamp 且无 finalizer" → etcd 真删。

### 1.2 典型链路：LB Service 与 `service.kubernetes.io/load-balancer-cleanup`

以 `type: LoadBalancer` 的 Service 为例（ACK 上由 cloud-controller-manager 管理 CLB/NLB）：

1. Service 创建后，service controller 打上 finalizer `service.kubernetes.io/load-balancer-cleanup`，含义是"这个对象在云上有欠账，删之前得先问我"；任何自研 operator 也可以给 Service 加自己的 finalizer（清理 DNS、防火墙、外部系统）。
2. 用户 `kubectl delete svc` → API server 存储层（`rest.BeforeDelete` + etcd3 store 的 `GuaranteedUpdate`）发现 finalizers 非空 → 写入 `deletionTimestamp` 与 `deletionGracePeriodSeconds`，返回删除"成功"，对象实际还在。
3. service controller watch 到 deletionTimestamp → 调云 API 释放 SLB/监听器/EIP → 成功后移除自己的 finalizer。
4. finalizers 清空 → 后续任意一次写路径检查（update 路径同样会检查）触发 etcd 硬删。删除完成，云上不留孤儿。

**关键细节**：finalizer 的判定发生在**存储层**而非某个 controller——所以第 4 步不需要谁专门去删，任何一次让 finalizers 变空的写操作都会顺带触发真删。这也正是本文事故链的机关所在。

---

## 二、update / patch / SSA 三种写入路径的并发语义

三种路径的本质区别是**基准对象不同**：

- **update（PUT）** 的基准是**你手里可能过期的副本**——你发什么，对象就是什么；
- **patch** 的基准是**API server 内存里的最新对象**——服务端在最新对象上打补丁，你没写的字段原样保留；
- **SSA（apply patch）** 的基准是**字段所有权表**（managedFields）——你只动自己拥有的字段，动别人的会冲突。

### 2.1 update：安全性完全取决于 resourceVersion

| 写法 | 行为 | 后果 |
| --- | --- | --- |
| PUT 带 RV（读到的 resourceVersion） | etcd 比对 mod-revision，不一致 → 409 Conflict | 乐观锁，安全，需重试 |
| PUT 不带 RV（无条件替换） | 不做任何前置检查，整体覆盖 | 期间别人加的 finalizer/label/annotation/ownerReference **全部蒸发** |

controller-runtime 的 `client.Update()` 会随对象携带 RV，冲突就返回 error 由 reconcile 重试——这是"update 用对了"的样子。"用错了"的经典姿势：凭空构造对象只填几个字段就 Update、`kubectl replace -f`（文件里没 RV）、从外部搬对象回写（RV 天然对不上）。

### 2.2 patch：四种类型对 `finalizers` 的行为完全不同

这是最容易被含糊带过、也最能拉开差距的一段。`metadata.finalizers` 在 apimachinery 里声明为 `[]string` 且带 `patchStrategy:"merge"`（无 patchMergeKey，按集合元素做并集），四种 patch 类型对它的实际行为：

| patch 类型 | 对 finalizers 的语义 | 加自己的 | 冲掉别人的 |
| --- | --- | --- | --- |
| **strategic merge patch**（kubectl 默认） | **并集**（patchStrategy:merge，原始 ∪ patch） | ✅ 安全追加 | ❌ 不会（并集删不掉元素） |
| **merge patch**（RFC 7386，`--type=merge`） | **整体替换**；`null` 删除字段 | ✅（但携带的列表会覆盖现状） | ⚠️ 会——把 GET 到的旧列表打出去，窗口期别人加的条目蒸发 |
| **json patch**（RFC 6902，`--type=json`） | 按 op 精确操作 | ✅ add | ⚠️ 按索引 `/finalizers/0` 盲删有位置竞态，需配 `test` op |
| **SSA**（apply patch） | 所有权语义 | ✅（首次认领即拥有） | ⚠️ 认领过之后，**漏带别人条目的 apply 会静默移除它**（你拥有该字段，不报冲突） |

两个直接推论：

1. **默认 strategic patch 恰恰是误伤风险最低的写法**——对 finalizers 只能加不能删。"patch 会覆盖别人的 finalizer"这个常见说法，对默认 patch 不成立；对 `--type=merge` 的旧列表、对 SSA 认领者才成立。
2. 官方文档的解卡命令用的正是替换语义：`kubectl patch svc x -p '{"metadata":{"finalizers":null}}' --type=merge`（RFC 7386 的 null 清空整个列表）——解卡命令能生效，本身就证明了替换语义的存在。

### 2.3 与字段所有权的关系

SSA 的 managedFields 记录每个字段归哪个 fieldManager 所有。详见 [[22-概念/01-核心架构/03-server-side-apply-field-ownership.md|Server-Side Apply 与字段所有权]]。对 finalizer 场景的结论只有一条：**controller 不要用 SSA 认领 `metadata.finalizers`**；一旦认领（比如 apply 过含 finalizers 的清单），之后每次 apply 都必须回带当前完整列表，漏了别人的就静默丢。

---

## 三、事故链：finalizer 被冲掉之后发生什么

### 3.1 删太快 → 云资源孤儿（隐蔽且持续扣费）

时间窗：deletionTimestamp 已写、CCM 还没来得及释放云 LB。

```
外部 controller 无 RV 整体 PUT（副本里没有 CCM 加的 finalizer）
  → 服务端对象 finalizers 变空
  → 存储层发现 deletionTimestamp 已设 + finalizers 为空 → 立即硬删
  → CCM 的清理 reconcile 永远不执行
  → 云上 SLB/监听器/EIP 成为孤儿：持续计费 + 残留对外暴露面
  → Service 对象已消失，事后排查连"曾经有哪些服务"的线索都少一半
```

### 3.2 删不掉 → 卡 Terminating（另一方向，与覆盖无关）

controller 加了 finalizer 后进程挂了/逻辑 bug 不移除 → 对象永远 `Terminating`。这与并发覆盖无关，是 finalizer 无人认领。解卡见第五节，但必须先确认外部资源已清理。

### 3.3 嫌疑人清单（按真实出现频率）

| 嫌疑人 | 典型姿势 | 为什么会踩 |
| --- | --- | --- |
| **自研 operator / 业务 controller** | 凭空构造对象只填几个字段就 `Update()`；对账逻辑把"不认识的 finalizer"当漂移清掉 | 不懂 finalizer 语义，副本里根本没有别人的条目 |
| **CI/CD 与发布系统** | `kubectl replace -f`（无 RV）、`apply --force`、自研同步循环直接 PUT | 把"发布"当成了"整体回写" |
| **多集群搬运工具** | 把 A 集群对象原样搬到 B 集群 | 手里副本天然是另一个集群的版本，RV 对不上 |
| **平台层例行回写** | 统一改 label/annotation 做管理标记时用 update 不用 patch | 图省事，整体 PUT |

**官方 controller 为什么不踩**：kube-controller-manager、CCM 全部遵守标准姿势——informer cache 里的对象自带 RV → `Update()` 带 RV → 冲突收 409 → `retry.RetryOnConflict` 重试。它们是 finalizer 的**添加者与守护者**，不是破坏者。真实的踩坑组合是"CCM（守规矩）+ 自研 operator（不守规矩）共存"。

---

## 四、正确姿势（工程红线）

### 4.1 代码侧

1. **永远不对共享对象做无条件 PUT**。要整体写就 GET → 改 → `Update()`（RV 随对象带上）→ 409 重试。
2. **finalizer 增删的标准写法**：reconcile loop 里 `controllerutil.AddFinalizer` / `controllerutil.RemoveFinalizer` + `Update()` + 冲突重试。靠乐观锁保证"读-改-写"原子性，这是官方 controller 的同款做法。
3. **改其他字段用最小 patch**，patch 体里永远不出现 `metadata.finalizers`。
4. **SSA 不认领 finalizers**；已经认领的，每次 apply 必须回带全量列表。
5. json patch 删列表元素必须配 `test` op 先断言，禁止按索引盲删。

### 4.2 解卡 Terminating（运维侧）

```bash
# 1. 先看对象卡在谁身上 🟢
kubectl get svc <name> -o jsonpath='{.metadata.finalizers}'
kubectl get svc <name> -o jsonpath='{.metadata.deletionTimestamp}'

# 2. 逐项确认外部资源已清理（云控制台/API 核对 SLB、EIP、监听器）🟢

# 3. 确认后摘锁（RFC 7386 null 清空列表）🔴
kubectl patch svc <name> -p '{"metadata":{"finalizers":null}}' --type=merge

# 或者更精确地只摘自己的条目 🟡
kubectl patch svc <name> --type=json \
  -p '[{"op":"test","path":"/metadata/finalizers/0","value":"example.com/my-cleanup"},\
       {"op":"remove","path":"/metadata/finalizers/0"}]'
```

🔴 不确认外部资源就摘锁 = 亲手制造孤儿资源；摘锁前先摘的是"责任"，不是"字段"。

### 4.3 事后排查：谁干的

```bash
# managedFields 能看到每个 fieldManager 拥有哪些字段（含 f:finalizers）🟢
kubectl get svc <name> -o yaml | grep -A 30 managedFields

# audit log 里找无 RV 的 PUT：requestObject 无 resourceVersion 且整对象替换
# （audit policy 需提前覆盖 write 请求，默认不开）
```

---

## 五、与相邻概念的关系

- **ownerReference 级联删除（GC）**：解决"**从属对象**跟着宿主一起删"（Deployment→RS→Pod），由 kube-controller-manager 的 garbage collector 驱动；finalizer 解决"**删除前先清外部欠账**"。两者可以共存：GC 负责集群内级联，finalizer 负责集群外交接。详见 [[22-概念/01-核心架构/02-garbage-collection-owner-reference.md|垃圾回收与 ownerReference 级联删除机制]]。
- **resourceVersion 乐观并发**：finalizer 安全性的底层保障，409/410 治理见 [[01-集群基础/02-设计原则/07-resource-version-control.md|资源版本控制]]。
- **API server 删除路径**：admission → `rest.BeforeDelete` → 存储层 finalizer 判定，全景见 [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|apiserver deep-dive]]；service controller 与云资源清理见 [[01-集群基础/03-控制平面/14-cloud-controller-manager-deep-dive.md|cloud-controller-manager deep-dive]]。

---

## 六、一页速记

| 问题 | 答案 |
| --- | --- |
| finalizer 是什么 | `metadata.finalizers` 非空时 DELETE 只写 deletionTimestamp,对象卡 Terminating,摘干净才真删 |
| 为什么需要 | 删除前强制清外部欠账(云 SLB/EIP/云盘),否则对象没了没人善后 |
| patch 会冲掉别人的 finalizer 吗 | 默认 strategic 不会(并集语义);`--type=merge` 携带旧列表会(整体替换);SSA 认领后漏带会(静默移除) |
| 谁会冲掉 | 无 RV 整体 PUT 是主凶:自研 operator、`kubectl replace`、多集群搬运、平台层回写 |
| 冲掉的后果 | 删除链路上对象被瞬间放行真删,云资源孤儿、持续计费 |
| 正确姿势 | finalizer 增删用带 RV 的 Update+重试;改字段用最小 patch;SSA 不认领 finalizers;摘锁前逐项确认外部资源已清 |
