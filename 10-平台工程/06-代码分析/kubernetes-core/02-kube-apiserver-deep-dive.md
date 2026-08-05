---
title: kube-apiserver 源码深度剖析
description: 基于 kubernetes-1.36.2 源码的 API Server 启动链路、Handler 链、认证授权准入、存储层与聚合层完整剖析
summary: 从 cmd/kube-apiserver 入口出发，剖析三层服务器链（Aggregator/KubeAPIServer/APIExtensions）、DefaultBuildHandlerChain 过滤器顺序、genericregistry.Store 存储抽象与 etcd3 落盘链路，全部函数附实测行号。
category: source-analysis
tags:
- k8s
- source-code
- apiserver
- admission
- authentication
- authorization
- etcd
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- kube-apiserver 请求处理流程源码
- DefaultBuildHandlerChain 过滤器顺序
- API Server 三层聚合架构 CreateServerChain
- 一个资源对象如何写入 etcd
trigger_keywords:
- kube-apiserver
- handler chain
- CreateServerChain
- admission
- genericregistry
- RESTStorage
related_domains:
- 集群基础
- 安全
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# kube-apiserver 源码深度剖析

> **源码基线**：`33-源码/控制平面/kubernetes-1.36.2/`
> 概念层配套阅读：[[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|控制平面：APIServer Deep Dive]]

## 概述

kube-apiserver 是集群唯一的「事实入口」：所有组件（kubectl、kubelet、controller、scheduler）都只与它通信，它是唯一直接读写 etcd 的组件。源码层面，它由三个 API Server 通过 delegation（责任链）组合而成，每个请求穿过一条固定顺序的 Filter 链后，进入资源对应的 REST Storage，最终经 etcd3 客户端落盘。

本文按「启动 → 请求 → 存储」三段剖析，回答：

1. 三层服务器链如何组装，一个未知路径的请求如何逐层委托？
2. Filter 链中认证、审计、限流、授权、准入的确切顺序是什么？
3. 一次 `kubectl apply` 从 HTTP 请求到 etcd key 的完整代码路径？

---

## 一、启动链路与三层服务器

### 1.1 入口

```go
// cmd/kube-apiserver/app/server.go:148
func Run(ctx context.Context, opts options.CompletedOptions) error {
    config, err := NewConfig(opts)          // Options → Config
    completed, err := config.Complete()     // 填充派生默认值
    server, err := CreateServerChain(completed)  // ★ 组装服务器链
    prepared, err := server.PrepareRun()    // 安装 healthz/openapi
    return prepared.Run(ctx)                // 启动 HTTPS 服务
}
```

### 1.2 CreateServerChain — 三层委托

```go
// cmd/kube-apiserver/app/server.go:176
func CreateServerChain(config CompletedConfig) (*aggregatorapiserver.APIAggregator, error) {
    // 1. 兜底: 404 NotFound handler
    notFoundHandler := notfoundhandler.New(...)
    // 2. APIExtensionsServer: 服务 CRD (apiextensions.k8s.io)
    apiExtensionsServer, err := config.ApiExtensions.New(delegate=notFoundHandler)
    // 3. KubeAPIServer: 服务内置资源 (core/apps/batch/...)
    kubeAPIServer, err := config.KubeAPIs.New(delegate=apiExtensionsServer)
    // 4. AggregatorServer: 服务 APIService 聚合 (metrics.k8s.io 等)
    aggregatorServer, err := createAggregatorServer(delegate=kubeAPIServer)
    return aggregatorServer, nil
}
```

请求路由的委托顺序（与构造顺序相反）：

```
请求 → AggregatorServer ──匹配 APIService?──→ 代理到扩展 API Server (如 metrics-server)
              │ 否
              ▼
        KubeAPIServer ──内置资源?──→ pkg/registry 存储层
              │ 否
              ▼
        APIExtensionsServer ──CRD?──→ CR 通用存储 (apiextensions-apiserver)
              │ 否
              ▼
        notFoundHandler → 404
```

**生产含义**：`kubectl get --raw /apis/metrics.k8s.io/v1beta1` 超时，问题大概率不在 apiserver 本身，而在 Aggregator 代理的后端（metrics-server）——三层结构决定了排障时要先分清请求落在哪一层。参见 [[01-集群基础/03-控制平面/29-api-extension-deep-dive.md|API 扩展机制深度剖析]]。

---

## 二、请求处理：DefaultBuildHandlerChain

通用 Filter 链在 `staging/src/k8s.io/apiserver/pkg/server/config.go:1036` 组装。Go HTTP 中间件是洋葱模型——**代码从下往上包裹，请求从上往下穿过**。按请求实际经过的顺序：

| 顺序 | Filter | 实测位置 (config.go) | 职责 | 失败表现 |
|------|--------|---------------------|------|---------|
| 1 | WithPanicRecovery / WithRequestInfo | 链尾包裹 | 恢复 panic、解析请求为 RequestInfo | — |
| 2 | `WithAuthentication` | :1077 | X509/Token/OIDC 认证，注入 user.Info | 401 Unauthorized |
| 3 | `WithAudit`(Init) | :1064, :1116 | 生成 auditID、记录 RequestReceived | — |
| 4 | `WithImpersonation` | :1059 | 处理 Impersonate-User 头 | 403 |
| 5 | `WithPriorityAndFairness` | :1048 | APF 流控（FlowSchema 分类、排队、丢弃） | 429 Too Many Requests |
| 6 | `WithAuthorization` | :1040 | RBAC/Node/Webhook 授权 | 403 Forbidden |
| 7 | (资源 handler 内) Admission | 见 2.2 | Mutating → Validating 准入 | 400/403/webhook 错误 |

**排障映射**：

- 401 → 认证层：检查证书/SA token（`WithAuthentication`）
- 403 → 先分清是授权（RBAC，403 且消息含 `forbidden: User ...`）还是准入拒绝（消息含 `admission webhook ... denied`）
- 429 → APF：`kubectl get flowschemas`，观察 `apiserver_flowcontrol_rejected_requests_total`，详见 [[01-集群基础/03-控制平面/18-api-priority-fairness.md|API 优先级与公平性]]

### 2.2 准入链的位置

准入不在 Filter 链中，而是在资源 handler 内部（解码出对象之后才能做对象级校验）：

```
解码(decode) → 版本转换(convert to internal) → MutatingAdmission
  → (对 CREATE/UPDATE) Validation → ValidatingAdmission → Storage
```

内置准入插件注册于 `pkg/kubeapiserver/options/plugins.go`；插件实现在 `plugin/pkg/admission/`（如 `noderestriction/`、`podsecurity/`）。Webhook 准入的调用逻辑在 `staging/src/k8s.io/apiserver/pkg/admission/plugin/webhook/`。

---

## 三、存储层：从 REST Storage 到 etcd

### 3.1 genericregistry.Store — 所有资源的通用 CRUD

每种资源的 REST 实现几乎都是对 `genericregistry.Store` 的薄封装 + 一个 Strategy：

```go
// staging/src/k8s.io/apiserver/pkg/registry/generic/registry/store.go
// Create:454  Update:625  Get:855（实测行号）
func (e *Store) Create(ctx context.Context, obj runtime.Object, ...) (runtime.Object, error) {
    // 1. rest.BeforeCreate: 调用 Strategy.PrepareForCreate / Validate
    // 2. e.Storage.Create(...): 写入 etcd3
    // 3. AfterCreate / Decorator 钩子
}
```

Strategy 模式是理解「字段为何被改写」的钥匙。以 Pod 为例（`pkg/registry/core/pod/strategy.go`）：

- `PrepareForCreate`：清空 Status、初始化 Generation
- `PrepareForUpdate`：**丢弃用户对 Status 的修改**（Status 只能走 `/status` 子资源）
- `Validate`：内部版本的完整校验

生产上「PATCH 了 status 却没生效」的根因就在 PrepareForUpdate 的字段裁剪。

### 3.2 etcd3 存储实现

```go
// staging/src/k8s.io/apiserver/pkg/storage/etcd3/store.go（实测行号）
func (s *store) Create(...)           // :274  写入，key 形如 /registry/pods/<ns>/<name>
func (s *store) GuaranteedUpdate(...) // :463  ★ CAS 乐观并发的核心
func (s *store) GetList(...)          // :736  List + 分页(continue token)
```

`GuaranteedUpdate` 是乐观并发控制的落点：

```go
// 简化逻辑（etcd3/store.go:463）
for {
    origState := getCurrentState()               // 读当前值 + ModRevision
    ret := tryUpdate(origState.obj)              // 应用调用方的更新函数
    txn := client.KV.Txn(ctx).If(
        clientv3.Compare(clientv3.ModRevision(key), "=", origState.rev),
    ).Then(clientv3.OpPut(key, data)).Else(clientv3.OpGet(key))
    if !txnResp.Succeeded { continue }           // 冲突→携最新值重试
    return decode(...)
}
```

这就是 HTTP 409 Conflict / `the object has been modified` 的源码出处；resourceVersion 即 etcd ModRevision 的透出。并发模型详见 [[01-集群基础/02-设计原则/07-resource-version-control.md|资源版本与并发控制]]。

### 3.3 Watch Cache — apiserver 内置的读缓存

etcd 之上还有一层 Cacher（默认对多数资源开启），List/Watch 优先由它服务，避免海量 Watch 直压 etcd：

```go
// staging/src/k8s.io/apiserver/pkg/storage/cacher/cacher.go:509
func (c *Cacher) Watch(ctx context.Context, key string, opts storage.ListOptions) (watch.Interface, error)

// watch_cache.go:283 — 事件进入环形缓冲并更新内部 store
func (w *watchCache) processEvent(event watch.Event, resourceVersion uint64, ...) error
```

关键行为：

- Cacher 自身用一个 Reflector 对 etcd 做 List-Watch，客户端的 Watch 从环形缓冲（默认 100 事件，动态扩容）回放
- 客户端 RV 太旧、缓冲已滚动 → 返回 `410 Gone`，触发 client-go Reflector 重新 List（详见 [[10-平台工程/06-代码分析/kubernetes-core/06-declarative-api-informer-mechanism.md|Informer 机制剖析]]）
- 大集群 `kubectl get pods -A` 慢，观察 `apiserver_watch_cache_*` 与 etcd `range` 延迟即可区分瓶颈层

---

## 四、一次写请求的完整代码路径

以 `kubectl apply -f pod.yaml`（创建）为例：

```
HTTP POST /api/v1/namespaces/default/pods
  → DefaultBuildHandlerChain（认证→审计→APF→授权）      config.go:1036
  → restfulCreateResource → createHandler
      staging/.../endpoints/handlers/create.go
  → 解码 + 转内部版本 + MutatingAdmission（注入 SA token 卷等）
  → genericregistry.Store.Create                        store.go:454
      → Strategy.PrepareForCreate / Validate            pkg/registry/core/pod/strategy.go
  → etcd3 store.Create                                  etcd3/store.go:274
      → PUT /registry/pods/default/<name>（protobuf 编码）
  → etcd Raft 提交（见 05 篇）
  ← 201 Created（对象含分配的 uid/resourceVersion）
之后：watchCache 收到事件 → 广播给 scheduler/kubelet 等 Watcher
```

---

## 五、生产排障速查

| 症状 | 源码定位 | 检查手段 |
|------|---------|---------|
| 401 | `WithAuthentication` (config.go:1077) | 证书过期 `openssl x509 -dates`；SA token audience |
| 403 forbidden | RBAC 授权器 `plugin/pkg/auth/authorizer/rbac/` | `kubectl auth can-i --as=<user>` |
| 429 | APF (config.go:1048) | `apiserver_flowcontrol_rejected_requests_total` |
| 409 Conflict 频繁 | `GuaranteedUpdate` (etcd3/store.go:463) | 控制器热点对象、减少全量 Update 改用 Patch |
| 410 Gone / relist 风暴 | Cacher 环形缓冲 (cacher.go:509) | watch 缓冲、客户端 resync 周期 |
| Status 修改丢失 | Pod Strategy `PrepareForUpdate` | 改用 `/status` 子资源 |
| 请求 504 但 apiserver 正常 | Aggregator 代理层 (server.go:176) | `kubectl get apiservice`，看 False 条目 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-core/01-source-tree-architecture.md|01 - 源码整体架构与目录结构]]
- [[10-平台工程/06-代码分析/kubernetes-core/05-etcd-storage-deep-dive.md|05 - etcd 与存储链路源码剖析]]（Raft 提交后半程）
- [[10-平台工程/06-代码分析/kubernetes-core/07-component-interaction-dataflow.md|07 - 组件交互关系与数据流向]]
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|控制平面：APIServer Deep Dive]]（运维视角）
- [[01-集群基础/03-控制平面/17-apiserver-tuning.md|APIServer 调优]]
- [[01-集群基础/03-控制平面/28-authz-authn-deep-dive.md|认证授权深度剖析]]
- [[08-安全/01-身份与访问/index.md|安全域：身份与访问]]
