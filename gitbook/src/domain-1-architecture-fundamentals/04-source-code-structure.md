# 04 - Kubernetes 源码结构深度解析

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **源码版本**: 以 v1.32.0 为基准 | **参考**: [kubernetes/kubernetes](https://github.com/kubernetes/kubernetes)

## 文档概览

本文档提供 Kubernetes 源码结构的全景图，涵盖顶层目录、核心组件、关键模块的代码路径、接口定义、版本演变和生产环境排障价值。适用于源码阅读、组件开发、故障调试、二次开发场景。

**阅读对象**: 平台工程师、Operator 开发者、CRI/CNI/CSI 插件开发者、源码贡献者

**核心价值**:
- 快速定位组件入口和核心逻辑
- 理解 staging 机制与独立客户端库
- 掌握调度器/Kubelet/控制器代码结构
- 追踪版本演进中的重大代码变更

---

## 顶级目录结构（生产环境价值解读）

| 路径/模块 | 用途 | 关键文件 | 版本演变 | 生产环境价值 |
|----------|------|---------|---------|-------------|
| **cmd/** | 所有组件的 main 入口 | main.go, app/options/ | 持续稳定 | **启动参数解析**、组件初始化流程、信号处理逻辑 |
| **pkg/** | 核心业务逻辑库 | 控制器/调度器/kubelet | v1.22 API 重构<br>v1.24 CRI 变更 | **源码调试入口**、控制器实现、调度算法、网络代理 |
| **staging/** | 独立发布的 Go 模块 | k8s.io/client-go 等 | 每版本同步 | **客户端开发**、CRD 开发、API 扩展 |
| **vendor/** | Go modules 依赖 | go.mod, go.sum | 每版本锁定 | 依赖冲突排查、CVE 漏洞追踪 |
| **api/** | OpenAPI 规范 | openapi-spec/, swagger-spec/ | 每版本生成 | 客户端代码生成、API 文档生成 |
| **build/** | 构建系统 | Makefile, build-image/ | v1.26+ 容器化构建 | 自定义构建、交叉编译、镜像构建 |
| **cluster/** | 部署脚本(已弃用) | - | v1.24 完全移除 | 历史参考，推荐用 kubeadm |
| **docs/** | 设计文档 | KEP(增强提案) | 每版本新增 | 理解 Feature Gate、API 变更决策 |
| **hack/** | 开发工具脚本 | verify-*.sh, update-*.sh | 持续更新 | 代码生成、lint 检查、本地测试环境 |
| **test/** | 测试套件 | e2e/, integration/, fuzz/ | 持续增强 | E2E 测试调试、集成测试编写 |
| **third_party/** | 第三方代码 | protobuf/, forked/ | 较少变化 | Protobuf 定义、API 序列化 |
| **plugin/** | 插件示例 | admission/, scheduler/ | v1.25+ 稳定 | 准入 Webhook、调度器插件示例 |
| **translations/** | 国际化翻译 | kubectl 多语言 | 持续更新 | kubectl 输出本地化 |
| **logo/** | 品牌资源 | SVG/PNG | 稳定 | 文档/演示/营销 |

---

## cmd/ 目录详解：组件入口与启动流程

### 核心控制平面组件

| 组件路径 | 用途 | 关键文件/包 | 版本变更 | 生产排障价值 |
|---------|------|-------------|---------|-------------|
| **cmd/kube-apiserver/** | API Server 主程序 | `app/server.go`<br>`app/options/options.go` | v1.29: CEL 准入<br>v1.30: WatchList GA | **API 启动失败排查**、审计日志配置、OIDC 认证调试 |
| **cmd/kube-controller-manager/** | 控制器管理器 | `app/controllermanager.go`<br>`app/options/` | v1.27: 控制器插件化<br>v1.30: 增强选举 | **控制器异常排查**、Leader 选举问题、控制器启用/禁用 |
| **cmd/kube-scheduler/** | 调度器主程序 | `app/server.go`<br>`app/config/` | v1.25: 调度框架 v1<br>v1.28: DRA Alpha | **调度失败调试**、调度器配置加载、插件注册 |
| **cmd/kubelet/** | 节点代理 | `app/server.go`<br>`app/options/` | v1.24: Dockershim 移除<br>v1.27: 就地重启 | **节点 NotReady**、CRI 初始化失败、cgroup 配置 |
| **cmd/kube-proxy/** | 网络代理 | `app/server.go`<br>`app/conntrack.go` | v1.26: nftables Beta<br>v1.31: nftables GA | **Service 不通排查**、代理模式切换、conntrack 调优 |

### 客户端与工具

| 组件路径 | 用途 | 关键文件/包 | 版本变更 | 开发价值 |
|---------|------|-------------|---------|----------|
| **cmd/kubectl/** | 命令行工具 | `cmd/kubectl.go`<br>`pkg/cmd/` | v1.26: apply --prune<br>v1.28: events 改进 | **kubectl 插件开发**、子命令扩展、输出格式定制 |
| **cmd/kubeadm/** | 集群初始化工具 | `app/cmd/`, `app/phases/` | v1.29: 外部 etcd 增强<br>v1.31: 证书轮换 | **集群部署调试**、证书配置、网络插件集成 |
| **cmd/kube-aggregator/** | API 聚合层 | `pkg/apiserver/` | v1.25 稳定 | **聚合 API 调试**、Metrics Server、自定义 API |
| **cmd/cloud-controller-manager/** | 云控制器 | `app/cloudcontrollermanager.go` | v1.25: 外部化完成 | **云负载均衡器**、Node IP 管理、云路由 |

---

## pkg/ 核心库详解：业务逻辑与接口定义

### 核心业务逻辑包

| 包路径 | 用途 | 关键类型/接口 | 版本变更 | 生产价值 |
|-------|------|--------------|---------|---------|
| **pkg/apis/** | 内部 API 类型 | `core/types.go`<br>`apps/types.go` | 随 API 演进 | **理解资源字段语义**、内外部类型转换、默认值设置 |
| **pkg/controller/** | 核心控制器 | `deployment/`, `replicaset/`<br>`nodelifecycle/` | v1.27: Job 索引<br>v1.28: StatefulSet 扩展 | **控制器行为分析**、Reconcile 逻辑、速率限制 |
| **pkg/kubelet/** | Kubelet 核心 | `kubelet.go`, `pod_workers.go`<br>`cri/`, `eviction/` | v1.24: CRI v1<br>v1.27: 就地重启 | **Pod 启动失败**、PLEG 超时、驱逐策略、CRI 调用 |
| **pkg/scheduler/** | 调度器核心 | `framework/`, `internal/queue/`<br>`internal/cache/` | v1.25: 框架 v1<br>v1.28: DRA | **调度失败诊断**、插件开发、调度队列、亲和性计算 |
| **pkg/proxy/** | kube-proxy 实现 | `iptables/`, `ipvs/`<br>`nftables/` | v1.26: nftables Beta<br>v1.31: nftables GA | **Service 不通**、代理规则生成、负载均衡算法 |
| **pkg/registry/** | API Server 存储层 | `core/pod/storage/`<br>`apps/deployment/storage/` | 持续稳定 | **理解资源 CRUD**、存储策略、etcd 编码 |
| **pkg/volume/** | 存储卷插件 | `csi/`, `util/`<br>`plugins/` | CSI 主流<br>In-tree 冻结 | **卷挂载失败**、CSI 调用、设备管理 |
| **pkg/cloudprovider/** | 云厂商接口 | `providers/`, `cloud.go` | v1.25: 外部化完成 | 云集成开发（已弃用，用 CCM） |
| **pkg/auth/** | 认证授权 | `authenticator/`, `authorizer/`<br>`nodeidentifier/` | v1.30: CEL 授权 | **RBAC 问题**、Token 认证、Node 授权 |
| **pkg/util/** | 通用工具 | `wait/`, `sets/`, `workqueue/` | 稳定 | 控制器开发复用、重试逻辑、集合操作 |
| **pkg/features/** | Feature Gate | `kube_features.go` | 每版本新增 | **Alpha/Beta 功能开关**、兼容性判断 |
| **pkg/quota/** | 资源配额 | `v1/evaluator/` | 持续增强 | ResourceQuota 计算、配额超限问题 |
| **pkg/kubelet/cm/** | cgroup 管理 | `cgroup_manager_linux.go`<br>`cpumanager/`, `memorymanager/` | v1.24: cgroup v2<br>v1.27: 拓扑管理 | **资源隔离**、CPU 绑定、内存拓扑 |

---

## staging/ 独立发布机制：客户端库与接口定义

### staging 目录说明

staging 目录包含可独立发布为 Go 模块的代码，通过 `k8s.io/*` 命名空间发布。代码通过 `staging/publishing/` 脚本同步到独立仓库。

**开发原则**:
- staging 代码不能依赖 `k8s.io/kubernetes` 外的 pkg 代码
- 修改 staging 代码后运行 `make update-staging-client-go`
- 版本号与 Kubernetes 主版本同步（如 v0.32.0）

### 核心 staging 包

| 包路径 | 用途 | 独立仓库 | 版本策略 | 典型使用场景 |
|-------|------|---------|---------|-------------|
| **k8s.io/api** | 外部 API 类型定义 | [api](https://github.com/kubernetes/api) | v0.xx.y 同步 | **Go 客户端**、资源 YAML 生成、类型断言 |
| **k8s.io/apimachinery** | API 机制库 | [apimachinery](https://github.com/kubernetes/apimachinery) | v0.xx.y 同步 | **CRD 开发**、Scheme 注册、序列化/反序列化 |
| **k8s.io/client-go** | 官方 Go 客户端 | [client-go](https://github.com/kubernetes/client-go) | v0.xx.y 同步 | **Operator 开发**、Informer、WorkQueue、RESTClient |
| **k8s.io/kubectl** | kubectl 核心库 | [kubectl](https://github.com/kubernetes/kubectl) | v0.xx.y 同步 | **kubectl 插件**、apply 逻辑、输出格式化 |
| **k8s.io/apiserver** | API Server 框架 | [apiserver](https://github.com/kubernetes/apiserver) | v0.xx.y 同步 | **自定义 API Server**、聚合 API、准入控制 |
| **k8s.io/kube-scheduler** | 调度器框架 | [kube-scheduler](https://github.com/kubernetes/kube-scheduler) | v0.xx.y 同步 | **调度器插件**、调度配置 API |
| **k8s.io/kubelet** | Kubelet API | [kubelet](https://github.com/kubernetes/kubelet) | v0.xx.y 同步 | CRI 开发、设备插件、拓扑管理 |
| **k8s.io/cri-api** | CRI 接口定义 | [cri-api](https://github.com/kubernetes/cri-api) | v0.xx.y 同步 | **容器运行时**（containerd/CRI-O）、镜像服务 |
| **k8s.io/csi-translation-lib** | CSI 迁移库 | [csi-translation-lib](https://github.com/kubernetes/csi-translation-lib) | v0.xx.y 同步 | In-tree 卷迁移到 CSI |
| **k8s.io/metrics** | Metrics API 类型 | [metrics](https://github.com/kubernetes/metrics) | v0.xx.y 同步 | HPA、自定义指标适配器 |
| **k8s.io/component-base** | 组件基础库 | [component-base](https://github.com/kubernetes/component-base) | v0.xx.y 同步 | **组件开发**、配置加载、日志、指标 |
| **k8s.io/controller-manager** | 控制器框架 | [controller-manager](https://github.com/kubernetes/controller-manager) | v0.xx.y 同步 | 自定义控制器管理器 |
| **k8s.io/kms** | KMS 加密插件 | [kms](https://github.com/kubernetes/kms) | v0.xx.y 同步 | Secret 加密、云 KMS 集成 |
| **k8s.io/dynamic-resource-allocation** | DRA 框架 | [dynamic-resource-allocation](https://github.com/kubernetes/dynamic-resource-allocation) | v1.28+ | GPU/NPU 资源调度 |

---

## 调度器代码结构（Scheduler Framework）

### 调度框架核心

| 路径 | 用途 | 关键接口/类型 | 版本变更 | 开发价值 |
|-----|------|--------------|---------|---------|
| **pkg/scheduler/framework/** | 调度框架接口 | `Framework`, `Plugin`<br>`Handle`, `CycleState` | v1.25: 框架 v1 GA | **调度器插件开发**、扩展点注册 |
| **pkg/scheduler/framework/interface.go** | 插件接口定义 | `PreFilterPlugin`, `FilterPlugin`<br>`ScorePlugin`, `BindPlugin` | 持续稳定 | 理解调度生命周期、插件扩展点 |
| **pkg/scheduler/framework/plugins/** | 内置调度插件 | `NodeAffinity`, `TaintToleration`<br>`NodeResourcesFit` | v1.28: DRA 插件<br>v1.30: 拓扑感知 | **插件实现参考**、评分算法、过滤逻辑 |
| **pkg/scheduler/apis/config/** | 调度配置 API | `KubeSchedulerConfiguration`<br>`Profile` | v1.25: v1 稳定 | 多调度器配置、插件权重、扩展资源 |
| **pkg/scheduler/internal/queue/** | 调度队列 | `PriorityQueue`, `SchedulingQueue` | v1.28: QueueSort 增强 | 理解 Pod 调度顺序、优先级抢占 |
| **pkg/scheduler/internal/cache/** | 调度缓存 | `Cache`, `NodeInfo`, `ImageStateSummary` | 持续优化 | 理解调度快照、Node 资源视图 |
| **pkg/scheduler/schedule_one.go** | 调度主循环 | `scheduleOne()` 函数 | 持续重构 | 理解调度流程、错误处理、抢占逻辑 |

---

## Kubelet 代码结构（Node Agent）

### Kubelet 核心模块

| 路径 | 用途 | 关键类型/接口 | 版本变更 | 生产排障价值 |
|-----|------|--------------|---------|-------------|
| **pkg/kubelet/kubelet.go** | Kubelet 主结构体 | `Kubelet` struct<br>`syncLoop()` | 持续重构 | **Node NotReady**、Kubelet 启动失败、同步循环卡死 |
| **pkg/kubelet/pod_workers.go** | Pod 工作协程 | `PodWorkers`<br>`podWorker()` | v1.27: 就地重启 | **Pod 启动/停止卡住**、Pod 状态同步 |
| **pkg/kubelet/container/** | 容器生命周期 | `Runtime`, `Manager` | v1.24: CRI v1 | 容器创建失败、镜像拉取、生命周期钩子 |
| **pkg/kubelet/cri/** | CRI 客户端 | `RemoteRuntimeService`<br>`RemoteImageService` | v1.24: 移除 Dockershim | **CRI 调用失败**、gRPC 超时、运行时连接 |
| **pkg/kubelet/pleg/** | PLEG 事件生成器 | `GenericPLEG`, `EventType` | v1.27: PLEG 优化 | **PLEG 超时**、容器状态不同步、性能优化 |
| **pkg/kubelet/eviction/** | 驱逐管理器 | `Manager`, `ThresholdNotifier` | v1.28: 优雅驱逐 | **节点资源压力**、OOM 驱逐、磁盘压力 |
| **pkg/kubelet/cm/** | cgroup 管理 | `ContainerManager`<br>`CgroupManager` | v1.24: cgroup v2<br>v1.27: 拓扑管理 | **资源隔离**、CPU 管理策略、内存 QoS |
| **pkg/kubelet/volumemanager/** | 卷管理器 | `VolumeManager`<br>`DesiredStateOfWorldPopulator` | CSI 主流 | **卷挂载失败**、卷设备管理、清理逻辑 |
| **pkg/kubelet/prober/** | 健康探测 | `Manager`, `Prober` | v1.28: Startup Probe 增强 | Liveness/Readiness 失败、探测超时 |
| **pkg/kubelet/status/** | 状态管理器 | `Manager` | 持续优化 | Pod 状态更新失败、API Server 同步 |
| **pkg/kubelet/kuberuntime/** | 运行时管理器 | `kubeGenericRuntimeManager` | v1.28: Sidecar 容器 | 容器启动顺序、镜像垃圾回收 |
| **pkg/kubelet/nodestatus/** | 节点状态 | `Setter` | 持续优化 | Node Condition、心跳上报 |

---

## API Server 代码结构（Control Plane Core）

### API Server 核心模块

| 路径 | 用途 | 关键类型/接口 | 版本变更 | 生产价值 |
|-----|------|--------------|---------|---------|
| **pkg/registry/** | 资源存储实现 | `RESTStorage`, `Store` | 持续稳定 | **理解资源 CRUD**、etcd 存储键、默认值设置 |
| **pkg/registry/core/pod/storage/** | Pod 存储 | `PodStorage`, `StatusREST` | 持续优化 | Pod 创建/更新逻辑、子资源（/exec /log） |
| **pkg/kubeapiserver/** | API Server 配置 | `Config`, `CompletedConfig` | 持续更新 | 启动配置、插件注册、API 组注册 |
| **k8s.io/apiserver/pkg/server/** | 通用 API Server 框架 | `GenericAPIServer`<br>`SecureServingInfo` | 持续重构 | **自定义 API Server**、聚合 API |
| **k8s.io/apiserver/pkg/admission/** | 准入控制框架 | `Interface`, `Handler`<br>`ValidationInterface` | v1.30: CEL 准入 GA | **Webhook 超时**、准入链调试、CEL 表达式 |
| **k8s.io/apiserver/pkg/authentication/** | 认证框架 | `Authenticator`, `TokenAuthenticator` | v1.29: 结构化认证 | **Token 认证失败**、OIDC 配置、证书认证 |
| **k8s.io/apiserver/pkg/authorization/** | 授权框架 | `Authorizer`, `AttributesRecord` | v1.30: CEL 授权 Beta | **RBAC 拒绝**、Node 授权、Webhook 授权 |
| **k8s.io/apiserver/pkg/storage/** | etcd 存储接口 | `Interface`, `Cacher` | v1.27: WatchList<br>v1.30: WatchList GA | **etcd 性能**、Watch 机制、一致性读 |
| **k8s.io/apiserver/pkg/endpoints/** | API 端点处理 | `APIInstaller`, `RequestScope` | 持续优化 | 理解 API 请求路由、参数解析 |
| **k8s.io/apiserver/pkg/audit/** | 审计日志 | `Backend`, `EventRecorder` | v1.29: 审计增强 | 审计日志配置、事件过滤、性能优化 |
| **k8s.io/apiserver/pkg/util/flowcontrol/** | APF 限流 | `PriorityLevelConfiguration` | v1.29: APF GA | **限流 429 错误**、优先级配置、并发限制 |

---

## 控制器代码结构（Controller Manager）

### 核心工作负载控制器

| 路径 | 用途 | 控制器类型 | 关键逻辑/函数 | 生产排障价值 |
|-----|------|-----------|-------------|-------------|
| **pkg/controller/deployment/** | Deployment 控制器 | `DeploymentController` | `syncDeployment()`<br>滚动更新逻辑 | **Deployment 更新卡住**、ReplicaSet 创建、回滚逻辑 |
| **pkg/controller/replicaset/** | ReplicaSet 控制器 | `ReplicaSetController` | `syncReplicaSet()`<br>副本数调节 | **Pod 数量不符**、扩缩容失败、Selector 匹配 |
| **pkg/controller/job/** | Job 控制器 | `JobController` | `syncJob()`<br>完成/失败追踪 | **Job 未完成**、Pod 清理策略、索引任务 |
| **pkg/controller/cronjob/** | CronJob 控制器 | `CronJobController` | 定时调度逻辑 | **定时任务不触发**、时区问题、并发策略 |
| **pkg/controller/statefulset/** | StatefulSet 控制器 | `StatefulSetController` | 有序部署/更新 | **StatefulSet 更新卡住**、PVC 绑定、Pod 序号 |
| **pkg/controller/daemonset/** | DaemonSet 控制器 | `DaemonSetController` | 节点匹配逻辑 | **DaemonSet Pod 缺失**、节点选择器、更新策略 |

### 节点与服务控制器

| 路径 | 用途 | 控制器类型 | 关键逻辑/函数 | 生产排障价值 |
|-----|------|-----------|-------------|-------------|
| **pkg/controller/nodelifecycle/** | 节点生命周期 | `Controller` | Taint 管理<br>驱逐逻辑 | **Node NotReady 驱逐**、污点自动添加、驱逐速率 |
| **pkg/controller/endpoint/** | Endpoints 控制器 | `EndpointController` | Service 端点同步 | **Endpoints 为空**、Service 发现失败 |
| **pkg/controller/endpointslice/** | EndpointSlice 控制器 | `Controller` | 端点分片<br>(v1.21+ 默认) | **大规模 Service**、端点更新性能 |
| **pkg/controller/service/** | Service 控制器 | `Controller` | NodePort 分配<br>ClusterIP 分配 | Service IP 冲突、端口分配失败 |
| **pkg/controller/namespace/** | Namespace 控制器 | `NamespaceController` | 级联删除 Finalizer | **Namespace Terminating 卡住** |
| **pkg/controller/serviceaccount/** | ServiceAccount 控制器 | `ServiceAccountController` | Token 创建/轮换 | **SA Token 缺失**、Token 过期 |

---

## 版本演进重大代码变更（v1.16 - v1.32）

### API 与类型系统变更

| 版本 | 变更模块 | 变更内容 | 影响范围 | 迁移建议 | 参考 PR |
|-----|---------|---------|---------|---------|--------|
| **v1.16** | pkg/apis/apiextensions | CRD 结构化 Schema 强制 | 所有 CRD 必须定义 OpenAPI v3 Schema | 添加 `spec.validation` | [#78458](https://github.com/kubernetes/kubernetes/pull/78458) |
| **v1.22** | staging/src/k8s.io/api | 移除多个 beta API | Ingress/PodSecurityPolicy/CRD 等 | 迁移到 v1 API | [#102106](https://github.com/kubernetes/kubernetes/pull/102106) |
| **v1.25** | pkg/apis/flowcontrol | APF 配置 v1beta3 → v1 | API 优先级和公平性配置 | 更新 FlowSchema/PriorityLevelConfiguration | [#109060](https://github.com/kubernetes/kubernetes/pull/109060) |
| **v1.27** | pkg/apis/admissionregistration | ValidatingAdmissionPolicy v1alpha1 | CEL 原生准入控制 | 替代部分 Webhook | [#113314](https://github.com/kubernetes/kubernetes/pull/113314) |
| **v1.30** | pkg/apis/admissionregistration | ValidatingAdmissionPolicy GA | CEL 准入生产可用 | 完全替代 Webhook | [#117836](https://github.com/kubernetes/kubernetes/pull/117836) |

### 容器运行时变更

| 版本 | 变更模块 | 变更内容 | 影响范围 | 迁移路径 | 参考 PR |
|-----|---------|---------|---------|---------|--------|
| **v1.20** | pkg/kubelet/dockershim | Dockershim 弃用警告 | Docker 作为容器运行时 | 迁移到 containerd/CRI-O | [#94624](https://github.com/kubernetes/kubernetes/pull/94624) |
| **v1.23** | pkg/kubelet/kubelet_pods.go | Pod 层级 cgroup | Pod 资源隔离增强 | 测试 cgroup 层级 | [#102190](https://github.com/kubernetes/kubernetes/pull/102190) |
| **v1.24** | pkg/kubelet/cri | 移除 Dockershim | 必须使用 CRI 运行时 | containerd 1.6+ / CRI-O | [#97252](https://github.com/kubernetes/kubernetes/pull/97252) |
| **v1.27** | pkg/kubelet/kuberuntime | 原地 Pod 垂直伸缩 | Alpha: 容器资源动态调整 | 启用 InPlacePodVerticalScaling | [#102884](https://github.com/kubernetes/kubernetes/pull/102884) |
| **v1.28** | pkg/kubelet/kuberuntime | Sidecar 容器原生支持 | Beta: Init 容器增强 | 使用 `restartPolicy: Always` | [#116428](https://github.com/kubernetes/kubernetes/pull/116428) |

### 调度器变更

| 版本 | 变更模块 | 变更内容 | 影响范围 | 插件开发建议 | 参考 PR |
|-----|---------|---------|---------|-------------|--------|
| **v1.25** | pkg/scheduler/framework | 调度框架 v1 GA | 插件 API 稳定 | 基于稳定 API 开发 | [#110175](https://github.com/kubernetes/kubernetes/pull/110175) |
| **v1.26** | pkg/scheduler/framework/plugins | 节点资源拓扑插件 | Beta: NUMA 感知调度 | 配置 TopologyManager | [#111597](https://github.com/kubernetes/kubernetes/pull/111597) |
| **v1.28** | pkg/scheduler/framework/plugins/dynamicresources | 动态资源分配 (DRA) | Alpha: GPU/FPGA 调度 | 实验性启用 DRA | [#118468](https://github.com/kubernetes/kubernetes/pull/118468) |
| **v1.30** | pkg/scheduler/framework | 调度器队列排序增强 | 性能优化 | 无需变更 | [#122018](https://github.com/kubernetes/kubernetes/pull/122018) |
| **v1.32** | pkg/scheduler/framework/plugins/dynamicresources | DRA v1alpha3 | GPU 共享/切分支持 | 测试 GPU 设备插件 | [#123789](https://github.com/kubernetes/kubernetes/pull/123789) |

### 网络变更

| 版本 | 变更模块 | 变更内容 | 影响范围 | 迁移策略 | 参考 PR |
|-----|---------|---------|---------|---------|--------|
| **v1.26** | pkg/proxy/nftables | nftables 代理模式 Beta | kube-proxy 新后端 | 测试 `--proxy-mode=nftables` | [#112541](https://github.com/kubernetes/kubernetes/pull/112541) |
| **v1.29** | pkg/proxy/nftables | nftables 性能优化 | 规则生成优化 | 生产环境验证 | [#119456](https://github.com/kubernetes/kubernetes/pull/119456) |
| **v1.31** | pkg/proxy/nftables | nftables 代理模式 GA | 推荐用于生产 | 逐步替代 iptables | [#122789](https://github.com/kubernetes/kubernetes/pull/122789) |
| **v1.27** | staging/src/k8s.io/apiserver | WatchList Beta | 提升 Watch 性能 | 无需变更 | [#115402](https://github.com/kubernetes/kubernetes/pull/115402) |
| **v1.30** | staging/src/k8s.io/apiserver | WatchList GA | 大幅降低 API Server 负载 | 默认启用 | [#120901](https://github.com/kubernetes/kubernetes/pull/120901) |

### 存储变更

| 版本 | 变更模块 | 变更内容 | 影响范围 | 行动建议 | 参考 PR |
|-----|---------|---------|---------|---------|--------|
| **v1.25** | pkg/volume | In-tree 卷插件冻结 | 所有云厂商卷插件 | 迁移到 CSI 驱动 | [#109869](https://github.com/kubernetes/kubernetes/pull/109869) |
| **v1.26** | pkg/volume | In-tree → CSI 迁移 GA | AWS EBS/GCE PD/Azure Disk | 启用自动迁移 | [#111301](https://github.com/kubernetes/kubernetes/pull/111301) |
| **v1.28** | staging/src/k8s.io/csi-translation-lib | vSphere 迁移 GA | vSphere 卷插件 | 部署 vSphere CSI | [#117698](https://github.com/kubernetes/kubernetes/pull/117698) |
| **v1.29** | pkg/volume/csi | CSI 卷扩展增强 | 在线扩容稳定性 | 测试卷扩展 | [#119123](https://github.com/kubernetes/kubernetes/pull/119123) |

---

## 生产环境代码导航指南

### 故障排查代码路径速查

| 故障类型 | 关键代码路径 | 关键函数/方法 | 日志关键字 |
|---------|------------|--------------|----------|
| **Pod Pending** | `pkg/scheduler/schedule_one.go` | `scheduleOne()`, `findNodesThatFitPod()` | `FailedScheduling`, `Unschedulable` |
| **Node NotReady** | `pkg/kubelet/kubelet_node_status.go`<br>`pkg/controller/nodelifecycle/` | `updateNodeStatus()`, `doNoScheduleTaintingPass()` | `NodeNotReady`, `NodeStatusUnknown` |
| **PLEG 超时** | `pkg/kubelet/pleg/generic.go` | `relist()`, `updateCache()` | `PLEG is not healthy` |
| **Service 不通** | `pkg/proxy/iptables/proxier.go`<br>`pkg/proxy/ipvs/proxier.go` | `syncProxyRules()` | `Failed to sync proxy rules` |
| **Endpoints 为空** | `pkg/controller/endpoint/endpoints_controller.go` | `syncService()` | `FailedToUpdateEndpoint` |
| **CRI 调用失败** | `pkg/kubelet/cri/remote/remote_runtime.go` | `RunPodSandbox()`, `CreateContainer()` | `Failed to create pod sandbox` |
| **卷挂载失败** | `pkg/kubelet/volumemanager/reconciler/reconstruct.go` | `mountAttachVolumes()` | `Unable to attach or mount volumes` |
| **准入拒绝** | `staging/src/k8s.io/apiserver/pkg/admission/` | `Admit()`, `Validate()` | `admission webhook denied` |
| **认证失败** | `staging/src/k8s.io/apiserver/pkg/authentication/` | `AuthenticateRequest()` | `Unauthorized`, `Unable to authenticate` |
| **API 限流** | `staging/src/k8s.io/apiserver/pkg/util/flowcontrol/` | `Wait()` | `rate limited`, `request timeout` |

### 性能优化代码路径

| 优化场景 | 关键代码路径 | 可调参数/逻辑 | 影响 |
|---------|------------|--------------|------|
| **Informer 性能** | `staging/src/k8s.io/client-go/tools/cache/` | `ResyncPeriod`, `WatchErrorHandler` | 减少 API Server 负载 |
| **调度吞吐** | `pkg/scheduler/internal/queue/scheduling_queue.go` | `podMaxBackoffSeconds`, `podInitialBackoffSeconds` | 提升调度速率 |
| **Kubelet 吞吐** | `pkg/kubelet/config/config.go` | `syncFrequency`, `MaxPods` | 提升节点 Pod 密度 |
| **etcd 压缩** | `staging/src/k8s.io/apiserver/pkg/storage/etcd3/` | `CompactionInterval` | 降低 etcd 存储 |
| **API Server 缓存** | `staging/src/k8s.io/apiserver/pkg/storage/cacher/` | `watchCacheSize` | 加速 List/Watch |

---

## 开发者常用命令与最佳实践

### 构建与测试命令

```bash
# ========== 构建相关 ==========
# 构建所有组件（生产二进制）
make all WHAT=cmd/kube-apiserver GOFLAGS=-v

# 快速构建（无优化，用于开发）
make quick-release

# 构建特定组件
make WHAT=cmd/kubectl
make WHAT=cmd/kube-scheduler
make WHAT=cmd/kubelet

# 交叉编译（Linux AMD64）
make cross KUBE_BUILD_PLATFORMS=linux/amd64

# 构建容器镜像
make quick-release-images

# ========== 测试相关 ==========
# 运行单元测试（特定包）
make test WHAT=./pkg/controller/deployment
make test WHAT=./pkg/scheduler/framework

# 运行所有单元测试
make test

# 运行集成测试
make test-integration

# 运行 E2E 测试（需要集群）
make test-e2e
make test-e2e-node  # 节点 E2E 测试

# 运行 E2E 测试（特定用例）
go run hack/e2e.go -- --test --test_args='--ginkgo.focus=Deployment'

# ========== 代码生成 ==========
# 生成所有代码（API 变更后必须执行）
make generated_files

# 仅生成 deepcopy/defaulter/conversion
make gen

# 生成 OpenAPI 规范
make gen_openapi

# 更新 vendor 依赖
go mod tidy
go mod vendor
make update-vendor

# 更新 staging 依赖
make update-staging-client-go

# ========== 代码验证 ==========
# 验证所有代码（CI 检查）
make verify

# 验证代码格式
make verify-gofmt

# 验证生成的代码是否最新
make verify-codegen

# 验证 staging 依赖
make verify-staging-client-go

# ========== 本地调试 ==========
# 本地启动集群（单机模式）
hack/local-up-cluster.sh

# 本地启动 API Server（独立调试）
_output/bin/kube-apiserver \
  --etcd-servers=http://127.0.0.1:2379 \
  --service-cluster-ip-range=10.0.0.0/24 \
  --insecure-port=8080 \
  --v=5

# 本地启动调度器（连接到已有集群）
_output/bin/kube-scheduler \
  --kubeconfig=/path/to/kubeconfig \
  --v=5

# ========== 性能分析 ==========
# 启用 pprof（API Server）
_output/bin/kube-apiserver --profiling=true
# 访问 http://localhost:6443/debug/pprof/

# 生成 CPU profile
curl http://localhost:6443/debug/pprof/profile?seconds=30 > cpu.prof
go tool pprof -http=:8080 cpu.prof

# 生成 Heap profile
curl http://localhost:6443/debug/pprof/heap > heap.prof
go tool pprof -http=:8080 heap.prof
```

### 开发环境搭建

```bash
# 克隆代码仓库
git clone https://github.com/kubernetes/kubernetes.git
cd kubernetes

# 检查依赖
make verify-dependencies

# 快速验证开发环境
make quick-verify

# 配置 IDE（VSCode）
# 安装 Go 插件后，在 .vscode/settings.json 中添加：
{
  "go.toolsEnvVars": {
    "GOOS": "linux",
    "GOARCH": "amd64"
  },
  "go.buildTags": "selinux,providerless",
  "gopls": {
    "build.directoryFilters": [
      "-vendor",
      "-staging/src/k8s.io/code-generator"
    ]
  }
}
```

### 常见开发任务

```bash
# ========== 添加新的 API 字段 ==========
# 1. 修改 pkg/apis/core/types.go（内部类型）
# 2. 修改 staging/src/k8s.io/api/core/v1/types.go（外部类型）
# 3. 生成代码
make generated_files
# 4. 更新测试
make test WHAT=./pkg/apis/core

# ========== 添加新的调度器插件 ==========
# 1. 在 pkg/scheduler/framework/plugins/ 下创建新包
# 2. 实现 Plugin 接口
# 3. 在 pkg/scheduler/framework/plugins/registry.go 中注册
# 4. 编写测试
make test WHAT=./pkg/scheduler/framework/plugins/yourplugin

# ========== 添加新的控制器 ==========
# 1. 在 pkg/controller/ 下创建新包
# 2. 实现 Controller 接口
# 3. 在 cmd/kube-controller-manager/app/controllermanager.go 中注册
# 4. 运行集成测试
make test-integration WHAT=./test/integration/controllermanager

# ========== 调试技巧 ==========
# 启用详细日志
export KUBE_LOG_LEVEL=5

# 启用 API 审计日志
_output/bin/kube-apiserver \
  --audit-log-path=/tmp/audit.log \
  --audit-log-maxage=30

# 启用调度器调试日志
_output/bin/kube-scheduler --v=5 --alsologtostderr

# 使用 Delve 调试
dlv exec _output/bin/kube-scheduler -- --kubeconfig=/path/to/kubeconfig
```

---

## 生产环境源码定制指南

### 常见定制场景

| 场景 | 涉及代码路径 | 定制方式 | 风险等级 |
|-----|------------|---------|----------|
| **自定义调度器** | `pkg/scheduler/framework/plugins/` | 实现 Plugin 接口 | 低（推荐） |
| **自定义准入控制** | 外部 Webhook 或 CEL | ValidatingAdmissionPolicy | 低（推荐） |
| **修改驱逐策略** | `pkg/kubelet/eviction/` | 修改阈值逻辑 | 中 |
| **修改 API 默认值** | `pkg/apis/*/v1/defaults.go` | 修改 SetDefaults 函数 | 中 |
| **修改控制器行为** | `pkg/controller/*/controller.go` | 修改 Reconcile 逻辑 | 高 |
| **修改网络代理** | `pkg/proxy/iptables/proxier.go` | 修改规则生成 | 高 |

### 定制化构建流程

```bash
# 1. Fork 官方仓库
git clone https://github.com/yourorg/kubernetes.git
cd kubernetes

# 2. 创建功能分支
git checkout -b custom-scheduler-plugin

# 3. 修改代码（示例：添加调度器插件）
vim pkg/scheduler/framework/plugins/custom/custom.go

# 4. 注册插件
vim pkg/scheduler/framework/plugins/registry.go

# 5. 生成代码
make generated_files

# 6. 编译
make WHAT=cmd/kube-scheduler

# 7. 运行测试
make test WHAT=./pkg/scheduler/framework/plugins/custom

# 8. 构建镜像（带自定义标签）
make quick-release-images
docker tag registry.k8s.io/kube-scheduler:latest yourregistry.com/kube-scheduler:v1.32.0-custom

# 9. 推送到私有仓库
docker push yourregistry.com/kube-scheduler:v1.32.0-custom
```

---

## YAML 示例：调度器插件配置

### 自定义调度器配置

```yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
profiles:
- schedulerName: custom-scheduler
  plugins:
    # 扩展点：PreFilter（预过滤）
    preFilter:
      enabled:
      - name: NodeResourcesFit
      - name: NodePorts
      - name: CustomPreFilterPlugin  # 自定义插件
    # 扩展点：Filter（过滤）
    filter:
      enabled:
      - name: NodeUnschedulable
      - name: TaintToleration
      - name: CustomFilterPlugin  # 自定义插件
    # 扩展点：Score（评分）
    score:
      enabled:
      - name: NodeResourcesBalancedAllocation
        weight: 1
      - name: ImageLocality
        weight: 1
      - name: CustomScorePlugin  # 自定义插件
        weight: 5
  pluginConfig:
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: MostAllocated
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
  - name: CustomScorePlugin
    args:
      customParameter: "value"
      enableFeatureX: true
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: scheduler-config
  namespace: kube-system
data:
  scheduler-config.yaml: |
    apiVersion: kubescheduler.config.k8s.io/v1
    kind: KubeSchedulerConfiguration
    profiles:
    - schedulerName: production-scheduler
      plugins:
        score:
          enabled:
          - name: NodeResourcesBalancedAllocation
            weight: 3
          - name: NodeAffinity
            weight: 2
```

### 部署自定义调度器

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: custom-scheduler
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: custom-scheduler-as-kube-scheduler
subjects:
- kind: ServiceAccount
  name: custom-scheduler
  namespace: kube-system
roleRef:
  kind: ClusterRole
  name: system:kube-scheduler
  apiGroup: rbac.authorization.k8s.io
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: custom-scheduler
  namespace: kube-system
  labels:
    component: scheduler
spec:
  replicas: 1
  selector:
    matchLabels:
      component: scheduler
  template:
    metadata:
      labels:
        component: scheduler
    spec:
      serviceAccountName: custom-scheduler
      priorityClassName: system-cluster-critical
      containers:
      - name: kube-scheduler
        image: yourregistry.com/kube-scheduler:v1.32.0-custom
        command:
        - /usr/local/bin/kube-scheduler
        - --config=/etc/kubernetes/scheduler-config.yaml
        - --v=5
        volumeMounts:
        - name: config-volume
          mountPath: /etc/kubernetes
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
      volumes:
      - name: config-volume
        configMap:
          name: scheduler-config
```

---

## 附录：关键接口定义速查

### 调度器插件接口

```go
// pkg/scheduler/framework/interface.go

// 预过滤插件：在过滤前进行预处理
type PreFilterPlugin interface {
    Plugin
    PreFilter(ctx context.Context, state *CycleState, p *v1.Pod) (*PreFilterResult, *Status)
    PreFilterExtensions() PreFilterExtensions
}

// 过滤插件：过滤不符合条件的节点
type FilterPlugin interface {
    Plugin
    Filter(ctx context.Context, state *CycleState, pod *v1.Pod, nodeInfo *NodeInfo) *Status
}

// 评分插件：为通过过滤的节点打分
type ScorePlugin interface {
    Plugin
    Score(ctx context.Context, state *CycleState, p *v1.Pod, nodeName string) (int64, *Status)
    ScoreExtensions() ScoreExtensions
}

// 绑定插件：将 Pod 绑定到节点
type BindPlugin interface {
    Plugin
    Bind(ctx context.Context, state *CycleState, p *v1.Pod, nodeName string) *Status
}
```

### 控制器接口

```go
// pkg/controller/controller_utils.go

// 控制器接口
type Interface interface {
    Run(ctx context.Context, workers int)
}

// Reconciler 模式
type Reconciler interface {
    Reconcile(ctx context.Context, req Request) (Result, error)
}
```

### CRI 接口

```go
// staging/src/k8s.io/cri-api/pkg/apis/runtime/v1/api.pb.go

// 运行时服务接口
type RuntimeServiceClient interface {
    RunPodSandbox(ctx context.Context, in *RunPodSandboxRequest) (*RunPodSandboxResponse, error)
    StopPodSandbox(ctx context.Context, in *StopPodSandboxRequest) (*StopPodSandboxResponse, error)
    CreateContainer(ctx context.Context, in *CreateContainerRequest) (*CreateContainerResponse, error)
    StartContainer(ctx context.Context, in *StartContainerRequest) (*StartContainerResponse, error)
    StopContainer(ctx context.Context, in *StopContainerRequest) (*StopContainerResponse, error)
    RemoveContainer(ctx context.Context, in *RemoveContainerRequest) (*RemoveContainerResponse, error)
    ListContainers(ctx context.Context, in *ListContainersRequest) (*ListContainersResponse, error)
    ContainerStatus(ctx context.Context, in *ContainerStatusRequest) (*ContainerStatusResponse, error)
    ExecSync(ctx context.Context, in *ExecSyncRequest) (*ExecSyncResponse, error)
}
```

---

## 学习路径建议

### 初级（了解架构）
1. 阅读 `cmd/` 下各组件的 main.go，理解启动流程
2. 阅读 `pkg/apis/core/types.go`，理解核心资源定义
3. 跟踪一个简单控制器（如 ReplicaSet）的 Reconcile 逻辑
4. 使用 `kubectl -v=8` 观察 client-go 的 API 调用

### 中级（理解机制）
1. 深入理解 Informer/WorkQueue 机制（`staging/src/k8s.io/client-go/tools/cache/`）
2. 分析调度器的完整调度流程（`pkg/scheduler/schedule_one.go`）
3. 研究 Kubelet 的 Pod 生命周期管理（`pkg/kubelet/pod_workers.go`）
4. 理解 API Server 的认证/授权/准入链路

### 高级（定制开发）
1. 开发自定义调度器插件
2. 开发 CRD + Operator（基于 client-go 和 controller-runtime）
3. 贡献代码到上游社区
4. 性能调优和大规模集群优化

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01

