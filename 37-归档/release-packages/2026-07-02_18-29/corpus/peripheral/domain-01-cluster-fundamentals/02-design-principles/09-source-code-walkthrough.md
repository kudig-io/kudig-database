---
title: 09 - Kubernetes 源码结构与阅读指南 (Source Code)
description: '# 09 - Kubernetes 源码结构与阅读指南 (Source Code)'
summary: '理解控制逻辑的第一步是掌握 `staging/src/k8s.io/client-go`。'
category: design-principles
tags:
- k8s
- design
- principles
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Kubernetes 源码结构与阅读指南 (Source Code) 是什么
- 如何 Kubernetes 源码结构与阅读指南 (Source Code)
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- Kubernetes
- 源码结构与阅读指南
- Source
- Code
- design
- principles
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 09 - [[Kubernetes|Kubernetes]] 源码结构与阅读指南 (Source Code)

> **交叉引用**：Domain-1 中有更详细的 Kubernetes 源码架构分析，请参考 [Domain-1: 源码结构](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-01-cluster-fundamentals/01-architecture-overview/01-source-code-structure.md) 和 [Domain-1: 源码架构](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-01-cluster-fundamentals/01-architecture-overview/05-kubernetes-source-code-architecture.md)。

<!-- chunk: 阅读建议：先看抽象，再看实现 -->
## 阅读建议：先看抽象，再看实现

阅读 K8s 源码最忌讳“深陷细节”。建议遵循以下路径：

### 核心抽象库 (client-go)
理解控制逻辑的第一步是掌握 `staging/src/k8s.io/client-go`。
* **Reflector**: 负责 List-Watch。
* **DeltaFIFO**: 负责事件排队与合并。
* **Indexer**: 负责本地对象缓存与检索。

如果你能理清这三者的交互，你就理解了 80% 的 K8s 控制器实现逻辑。

<!-- chunk: 源码仓库结构 -->
## 源码仓库结构

| 目录 | 说明 |
|-----|------|
| /cmd | 各组件入口main函数 |
| /pkg | 核心库代码 |
| /staging | 独立发布的client库 |
| /api | API定义(OpenAPI规范) |
| /build | 构建脚本和配置 |
| /hack | 开发辅助脚本 |
| /test | 测试代码 |
| /vendor | 依赖库 |

<!-- chunk: /cmd目录详解 -->
## /cmd目录详解

| 子目录 | 组件 | 说明 |
|-------|-----|------|
| /cmd/kube-apiserver | API Server | API服务入口 |
| /cmd/kube-controller-manager | Controller Manager | 控制器管理器 |
| /cmd/kube-scheduler | Scheduler | 调度器 |
| /cmd/kubelet | [[kubelet|Kubelet]] | 节点代理 |
| /cmd/kube-proxy | Kube-proxy | 网络代理 |
| /cmd/kubectl | Kubectl | CLI工具 |
| /cmd/kubeadm | Kubeadm | 集群引导工具 |

<!-- chunk: /pkg核心包 -->
## /pkg核心包

| 包 | 说明 |
|---|------|
| /pkg/api | 内部API类型 |
| /pkg/apis | API注册 |
| /pkg/controller | 控制器实现 |
| /pkg/scheduler | 调度器实现 |
| /pkg/kubelet | Kubelet实现 |
| /pkg/proxy | Kube-proxy实现 |
| /pkg/registry | API存储层 |
| /pkg/volume | 存储卷插件 |

<!-- chunk: /staging独立库 -->
## /staging独立库

| 库 | 导入路径 | 说明 |
|---|---------|------|
| client-go | k8s.io/client-go | K8s客户端库 |
| api | k8s.io/api | API类型定义 |
| apimachinery | k8s.io/apimachinery | API基础机制 |
| apiserver | k8s.io/apiserver | API Server库 |
| controller-runtime | sigs.k8s.io/controller-runtime | 控制器框架 |

<!-- chunk: API Server核心流程 -->
## API Server核心流程

| 阶段 | 代码位置 | 说明 |
|-----|---------|------|
| 入口 | cmd/kube-apiserver/apiserver.go | main函数 |
| 初始化 | pkg/controlplane/instance.go | 创建APIServer实例 |
| 认证 | staging/src/k8s.io/apiserver/pkg/authentication | 认证处理 |
| 授权 | staging/src/k8s.io/apiserver/pkg/authorization | RBAC授权 |
| 准入 | staging/src/k8s.io/apiserver/pkg/admission | 准入控制 |
| 存储 | pkg/registry | etcd存储层 |

### API请求处理链

```
请求 → Authentication → Authorization → Admission(Mutating) 
    → Validation → Admission(Validating) → etcd持久化 → 响应
```

<!-- chunk: Controller Manager核心 -->
## Controller Manager核心

| 文件 | 说明 |
|-----|------|
| cmd/kube-controller-manager/app/controllermanager.go | 入口和启动 |
| pkg/controller/deployment/deployment_controller.go | Deployment控制器 |
| pkg/controller/replicaset/replica_set.go | ReplicaSet控制器 |
| pkg/controller/job/job_controller.go | Job控制器 |
| pkg/controller/garbagecollector/garbagecollector.go | GC控制器 |

### 控制器注册表

```go
// 位置: cmd/kube-controller-manager/app/controllermanager.go
func NewControllerInitializers() map[string]InitFunc {
    controllers := map[string]InitFunc{}
    controllers["deployment"] = startDeploymentController
    controllers["replicaset"] = startReplicaSetController
    controllers["statefulset"] = startStatefulSetController
    controllers["daemonset"] = startDaemonSetController
    controllers["job"] = startJobController
    // ... 更多控制器
    return controllers
}
```

<!-- chunk: Scheduler核心 -->
## Scheduler核心

| 文件 | 说明 |
|-----|------|
| cmd/kube-scheduler/app/server.go | 入口 |
| pkg/scheduler/scheduler.go | 调度器主逻辑 |
| pkg/scheduler/framework/interface.go | 调度框架接口 |
| pkg/scheduler/framework/plugins | 调度插件实现 |

### 调度流程

| 阶段 | 说明 | 插件类型 |
|-----|------|---------|
| PreFilter | 预处理检查 | PreFilterPlugin |
| Filter | 节点过滤 | FilterPlugin |
| PostFilter | 过滤后处理 | PostFilterPlugin |
| PreScore | 预评分 | PreScorePlugin |
| Score | 节点评分 | ScorePlugin |
| Reserve | 资源预留 | ReservePlugin |
| Permit | 批准检查 | PermitPlugin |
| PreBind | 预绑定 | PreBindPlugin |
| Bind | 实际绑定 | BindPlugin |
| PostBind | 绑定后处理 | PostBindPlugin |

<!-- chunk: Kubelet核心 -->
## Kubelet核心

| 文件 | 说明 |
|-----|------|
| cmd/kubelet/kubelet.go | 入口 |
| pkg/kubelet/kubelet.go | Kubelet主逻辑 |
| pkg/kubelet/pod/pod_manager.go | Pod管理 |
| pkg/kubelet/container/runtime.go | 容器运行时接口 |
| pkg/kubelet/cri/remote/remote_runtime.go | CRI客户端 |

### Kubelet主循环

```go
// pkg/kubelet/kubelet.go
func (kl *Kubelet) syncLoop(updates <-chan kubetypes.PodUpdate) {
    for {
        select {
        case u := <-updates:
            switch u.Op {
            case kubetypes.ADD:
                kl.HandlePodAdditions(u.Pods)
            case kubetypes.UPDATE:
                kl.HandlePodUpdates(u.Pods)
            case kubetypes.DELETE:
                kl.HandlePodRemoves(u.Pods)
            case kubetypes.RECONCILE:
                kl.HandlePodReconcile(u.Pods)
            }
        }
    }
}
```

<!-- chunk: client-go核心组件 -->
## client-go核心组件

| 组件 | 路径 | 说明 |
|-----|-----|------|
| Clientset | kubernetes/clientset.go | 类型化客户端集 |
| DynamicClient | dynamic/interface.go | 动态客户端 |
| Informer | tools/cache/shared_informer.go | 缓存+事件 |
| Lister | tools/cache/listers.go | 缓存读取 |
| WorkQueue | util/workqueue | 工作队列 |

<!-- chunk: 代码阅读技巧 -->
## 代码阅读技巧

| 技巧 | 说明 |
|-----|------|
| 从cmd入口开始 | 理解启动流程 |
| 关注接口定义 | interface定义核心抽象 |
| 使用IDE跳转 | GoLand/VSCode |
| 看注释和文档 | 代码注释详尽 |
| 运行单元测试 | 理解预期行为 |
| 使用日志调试 | 添加klog输出 |

<!-- chunk: 核心接口 -->
## 核心接口

| 接口 | 位置 | 说明 |
|-----|-----|------|
| runtime.Object | apimachinery/pkg/runtime | 所有API对象基接口 |
| client.Client | controller-runtime/pkg/client | 统一客户端接口 |
| Reconciler | controller-runtime/pkg/reconcile | 调谐器接口 |
| Manager | controller-runtime/pkg/manager | 控制器管理器 |

<!-- chunk: 开发调试 -->
## 开发调试

| 工具 | 用途 |
|-----|------|
| dlv | Go调试器 |
| kind | 本地K8s集群 |
| make | 构建系统 |
| hack/local-up-cluster.sh | 本地启动集群 |

### 本地构建

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 构建所有组件
make

# 构建特定组件
make WHAT=cmd/kubectl

# 运行测试
make test

# 本地启动集群
hack/local-up-cluster.sh
```
---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- Domain-2 设计原则 — 开源项目索引
- Kubernetes 设计原则与哲学
- 声明式 API 与面向终态设计
- 控制器模式与调谐循环
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 06 - 资源版本与并发控制 (Concurrency Control)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 08 - 高可用架构模式 (HA Patterns)
- 10 - CAP 定理与分布式系统基础 (CAP Theorem)

## See Also

- 07-distributed-consensus-etcd
- 08-high-availability-patterns
- 10-cap-theorem-distributed-systems
- 11-extensibility-design-patterns

## Related

- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
