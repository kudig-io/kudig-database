---
title: 控制平面故障排查
description: '# 控制平面故障排查'
category: skills
tags:
- k8s
- troubleshooting
- structural
- control-plane
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 控制平面故障排查 是什么
- 如何 控制平面故障排查
trigger_keywords:
- 控制平面故障排查
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- tls-basics
created: "2026-05-23"
---

# 控制平面故障排查

### 01 Apiserver Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **确认影响面**：`kubectl version --short && kubectl get --raw /readyz`，若失败同时检查 LB 健康检查与节点安全组端口 6443。
2. **看健康端点**：`curl -k https://$HOST:6443/readyz?verbose`，若等到 `[-]etcd`/`[-]informer-sync` 失败，优先检查 etcd/网络。
3. **看资源与限流**：`kubectl top pod -A | grep kube-apiserver`、`grep -E "429|throttling" /var/log/kube-apiserver.log | tail`，观察 APF 触发与 QPS 峰值。
4. **看 etcd 延迟**：`kubectl exec -n kube-system etcd-<node> -- etcdctl endpoint status --write-out=table`，关注 `db size`、`raft term` 与 `leader` 变更频率。
5. **看请求模式**：`kubectl logs -n kube-system kube-apiserver-<node> | grep "LIST" | head`，确认是否有大表全量 LIST 或 watch 风暴。
6. **快速缓解**：
   - LB / iptables 阶段：切换备用 LB 或移除异常后端。
   - 资源阶段：临时调高 CPU/memory request/limit，必要时水平扩容副本（前提：etcd/LB 配置允许）。
   - 流量阶段：临时调低过载来源（CI 扫描、监控抓取）并开启 APF 保护核心租户。
7. **记录证据**：在处置前后保存 `/readyz?verbose` 输出、pprof（`/debug/pprof/profile`）、关键日志与指标快照，以便后续复盘。

---

#### 2. 排查方法与步骤



#### 2.1 排查原理

API Server 是 Kubernetes 集群的核心组件，所有组件都通过 API Server 进行通信。排查 API Server 问题需要从以下层面入手：

#### 2.1.1 进程层面
- **生命周期管理**：理解 systemd/kubelet 如何管理 kube-apiserver 静态 Pod，重启策略与健康探针如何协同
- **启动依赖**：需依赖 etcd 可用、证书存在、配置文件合法，任一缺失都会导致启动失败
- **核心流程**：初始化 → 注册 API 资源 → 启动 Informer 缓存 → 监听端口 → 提供服务

#### 2.1.2 网络层面
- **多层连接校验**：客户端 → LB → API Server → etcd，每一跳都可能产生延迟/证书错误/超时
- **端口绑定与监听**：默认 6443(secure)、8080(insecure,已废弃)、健康端口(默认 6443 复用或独立)
- **TLS 握手**：客户端证书、服务端证书、CA 证书链，任一失效都会导致 `x509` 错误
- **负载均衡器健康检查**：LB 健康探针路径(如 `/healthz`)返回非 200 时会将后端标记为不健康

#### 2.1.3 存储层面
- **etcd 连接池**：API Server 维护与 etcd 的长连接池，连接断开会触发重连与缓存失效
- **Watch 机制**：所有资源变更通过 etcd watch 推送，etcd 延迟直接影响 API 响应速度
- **数据一致性**：API Server 作为 etcd 的唯一客户端，负责数据校验、版本控制(ResourceVersion)与冲突检测

#### 2.1.4 资源层面
- **内存管理**：Informer 缓存(所有资源在内存)、连接池、请求上下文，大集群内存消耗可达数 GB
- **CPU 瓶颈**：序列化/反序列化、准入控制、RBAC 鉴权、复杂 watch 过滤，高 QPS 下 CPU 成为瓶颈
- **文件描述符**：每个 watch 连接消耗一个 fd，大量长连接会耗尽 fd 限制

#### 2.1.5 配置层面
- **启动参数**：超过 200 个可配置参数，常见的如 `--etcd-servers`、`--tls-cert-file`、`--enable-admission-plugins`
- **准入控制器链**：MutatingAdmission → ValidatingAdmission → ResourceQuota，任一环节超时/失败都会拒绝请求
- **APF(API Priority and Fairness)**：请求分类、优先级队列、并发限制，配置不当会导致关键请求被限流

---

### 02 Etcd Troubleshooting

#### 0. 15 分钟快速诊断与止血

1. **健康检查一键看**：`ETCDCTL_API=3 etcdctl --write-out=table endpoint status --cluster`，关注 `Leader`、`DbSize`、`Raft Term`、`Recv/Send` 延迟；若无输出，立即检查监听端口/证书。
2. **磁盘 IO/空间快照**：`iostat -x 1 5`、`df -h /var/lib/etcd`，`DbSize` 接近配额时先 `compaction`+`defrag`，必要时临时扩容磁盘。
3. **网络与延迟**：`ping`/`mtr`/`tcptraceroute` 节点间 2379/2380，确认无丢包和 RTT 异常；云环境检查安全组/SLB 健康检查。
4. **证书有效性**：`openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -enddate`，证书将到期先做轮转，避免同时过期。
5. **热点与大 key**：`etcdctl --endpoints=... check perf` + `etcdctl --endpoints=... alarm list`，若有 `NOSPACE` 报警，执行 `compaction + defrag`；若观测到异常大 key，用 `etcdctl get <prefix> --prefix --keys-only | head` 定位来源。
6. **快速缓解策略**：
   - 读写受阻：先将高频 LIST/Watch 的租户/系统（如监控/同步器）降速或暂时停用。
   - Leader 抖动：优先锁定低延迟、性能好的节点作为首选 Leader（优化节点亲和或隔离噪声业务）。
   - 资源瓶颈：临时调高 etcd Pod request/limit，或在独立裸机/更快磁盘上运行。
7. **证据留存**：保存 `endpoint status`、`alarm list`、Prometheus etcd 指标（如 `etcd_server_leader_changes_seen_total`、`etcd_debugging_mvcc_db_total_size_in_bytes`）及系统 dmesg，便于复盘。

---

#### 2. 排查方法与步骤



#### 2.1 排查原理

etcd 是 Kubernetes 集群的核心数据存储，采用 Raft 共识算法保证数据一致性。排查 etcd 问题需要从以下层面入手：

1. **进程层面**：etcd 进程是否正常运行
2. **集群层面**：集群成员状态、Leader 选举、数据同步
3. **存储层面**：磁盘空间、IO 性能、数据完整性
4. **网络层面**：成员间网络连通性、延迟
5. **配置层面**：启动参数、证书配置

---

### 03 Scheduler Troubleshooting

#### 0. 10 分钟快速诊断

1. **确认 Scheduler 存活与选举**：`curl -k https://127.0.0.1:10259/healthz`，`kubectl get lease -n kube-system kube-scheduler -o wide`，若无 Leader/频繁切换先查证书/网络。
2. **观察 Pending 原因**：`kubectl get pods -A --field-selector=status.phase=Pending -o wide | head` + `kubectl describe pod <name> | grep -A30 Events`，锁定主因（资源不足/污点/亲和/拓扑约束/PVC）。
3. **看调度性能**：监控指标 `scheduler_scheduling_attempts_total`、`scheduler_pod_scheduling_duration_seconds`、`workqueue_depth`，或 `kubectl get --raw "/metrics" | grep scheduler_scheduling_duration_seconds_bucket | head`。
4. **热点插件/扩展点**：`kubectl logs -n kube-system kube-scheduler-<node> | grep "took too long" | head`，定位 Filter/Score/Bind 插件耗时；检查 `--parallelism`、`--bind-timeout-seconds` 配置。
5. **拓扑/分布/PDB 冲突**：对 Pending Pod 查看 `topologySpreadConstraints`、`pdb`，必要时用 `kubectl describe pdb` 与节点标签交叉验证。
6. **快速缓解**：
   - 资源侧：临时给节点加资源或移除影响调度的污点/亲和约束。
   - 流量侧：暂缓大规模创建/批量 Job；对控制面低优先级 Flow 使用 APF 调整。
   - 配置侧：适度提高 `--parallelism`，对插件耗时高的场景关闭不必要扩展或调整权重。
7. **证据留存**：保存 Pending Pod 事件、Scheduler 日志、关键指标 (调度时延直方图、抢占次数) 以便复盘。

---

#### 2. 排查方法与步骤



#### 2.1 排查原理

Scheduler 负责将 Pod 分配到合适的节点。排查需要从以下层面：

#### 2.1.1 服务层面
- **调度循环(Scheduling Cycle)**：Scheduler 持续监听 API Server 中 `spec.nodeName` 为空的 Pod，将其加入调度队列
- **绑定循环(Binding Cycle)**：异步执行最终的 Pod 与 Node 绑定操作，避免阻塞调度决策
- **健康检查**：`/healthz` 端点检查 Leader 状态、Informer 缓存同步状态
- **优先级队列**：Pod 按 `priorityClass` 排序，高优先级 Pod 优先调度，同优先级按创建时间 FIFO

#### 2.1.2 连接层面
- **Informer 机制**：Scheduler 通过 Informer 缓存节点/Pod/PV/PVC 等资源，减少 API 调用
- **List-Watch**：初始 LIST 全量加载，后续 WATCH 增量更新，网络中断会导致缓存失效重建
- **客户端限流**：Scheduler 对 API Server 的 QPS/Burst 限制，避免过载
- **证书认证**：`--kubeconfig` 或 `--authentication-kubeconfig` 配置客户端证书

#### 2.1.3 选举层面
- **Lease 机制**：多个 Scheduler 实例通过 Lease 资源竞争成为 Leader，非 Leader 待命
- **Leader 标识**：`kube-system/kube-scheduler` Lease 的 `spec.holderIdentity` 标识当前 Leader
- **租约续期**：Leader 每隔 `--leader-elect-renew-deadline`(默认 10s) 续期，失败则触发重新选举
- **脑裂保护**：Lease 有全局唯一性，保证只有一个 Leader 执行调度

#### 2.1.4 算法层面 - 调度框架(Scheduling Framework)
Scheduler v1.19+ 采用插件化调度框架，核心扩展点：

1. **队列排序(QueueSort)**：决定 Pod 从队列中被取出的顺序（默认按优先级）
2. **PreFilter**：预处理，如检查 PVC 是否存在，失败则跳过本轮调度
3. **Filter(Predicate)**：**过滤阶段**，并行检查每个节点是否满足 Pod 要求
   - `NodeResourcesFit`：检查 CPU/内存/临时存储是否充足
   - `NodeName`：检查 `spec.nodeName` 是否匹配
   - `PodToleratesNodeTaints`：检查 Pod 是否容忍节点污点
   - `NodeAffinity`：检查节点是否满足亲和性
   - `PodTopologySpread`：检查拓扑分布约束
   - `VolumeBinding`：检查 PVC 绑定与节点存储能力
4. **PostFilter**：所有节点都被过滤后触发，执行**抢占(Preemption)**尝试驱逐低优先级 Pod
5. **PreScore**：评分前的预处理，如计算节点间平衡度
6. **Score(Priority)**：**打分阶段**，为通过过滤的节点打分（0-100）
   - `NodeResourc
...(截断)

---

### 04 Controller Manager Troubleshooting

#### 0. 10 分钟快速诊断

1. **健康与选举**：`curl -k https://127.0.0.1:10257/healthz?verbose`、`kubectl get lease -n kube-system kube-controller-manager -o wide`，若无 Leader 或频繁切换先查证书/网络/LB。
2. **核心控制器快照**：快速查看 `kubectl get deploy,rs,ds,statefulset,job,cronjob -A | head`、`kubectl get endpoints -A | head`、`kubectl get nodes`，锁定异常资源。
3. **事件与队列深度**：`kubectl describe <resource>` 关注控制器事件；`kubectl get --raw "/metrics" | grep -E "workqueue_(depth|retries|adds)_total" | head` 识别堆积。
4. **API/限流**：日志 grep `throttling` 或 `rate limiter`；检查 `--kube-api-qps/--kube-api-burst`、`--concurrent-*-syncs` 是否过低。
5. **Token/证书/SA**：若 Pod 无法创建 SA Token，检查 `kube-controller-manager` 是否有 `--use-service-account-credentials`，并确认签发证书未过期。
6. **快速缓解**：
   - 功能缺失：临时调高受影响控制器的 `--concurrent-*-syncs`（如 endpoints、replicaset、deployment）。
   - 压力过载：降低外部大规模变更，或临时提升 CM 资源 request/limit。
   - 选举异常：确保只有一个活跃 CM，排查 LB/iptables 导致的租约漂移。
7. **证据留存**：保留 healthz 输出、Leader 租约 YAML、关键控制器日志、workqueue 指标快照用于复盘。

---

#### 2. 排查方法与步骤



#### 2.1 排查原理

Controller Manager 运行多个控制器，负责维护集群期望状态。排查需要从以下层面：

#### 2.1.1 服务层面
- **多控制器架构**：Controller Manager 实际是多个控制器的集合体，包括 Deployment、ReplicaSet、Node、Endpoint、ServiceAccount 等 20+ 个控制器
- **独立协程运行**：每个控制器在独立 goroutine 中运行，互不阻塞（但共享 API 客户端和 Informer）
- **控制循环模型**：每个控制器持续运行 `watch → compare → reconcile` 循环
  1. **Watch**：通过 Informer 监听资源变化
  2. **Compare**：比较实际状态与期望状态
  3. **Reconcile**：执行调和操作（创建/更新/删除子资源）
- **启动依赖**：API Server 可达、证书有效、Leader 选举成功、Informer 缓存同步完成

#### 2.1.2 连接层面
- **Shared Informer 机制**：所有控制器共享 Informer 工厂，避免重复 watch 相同资源
- **Informer 缓存**：本地内存缓存所有监听资源的全量数据，减少 API 调用
- **List-Watch 协议**：
  1. 启动时 LIST 全量加载
  2. 运行时 WATCH 增量更新
  3. 定期 Resync（默认 30s）触发全量对账，保证最终一致性
- **客户端限流**：
  - `--kube-api-qps`（默认 20）：每秒最大请求数
  - `--kube-api-burst`（默认 30）：突发请求数
  - 超出时排队等待，避免过载 API Server

#### 2.1.3 选举层面
- **Lease 租约机制**：多个 Controller Manager 实例通过 Lease 资源竞争 Leader
- **租约参数**：
  - `--leader-elect-lease-duration`（默认 15s）：租约有效期
  - `--leader-elect-renew-deadline`（默认 10s）：续期 deadline
  - `--leader-elect-retry-period`（默认 2s）：重试间隔
- **单 Leader 保证**：只有 Leader 执行控制逻辑，非 Leader 待命（避免并发冲突）
- **自动故障转移**：Leader 失联后，其他实例自动接管（通常 < 30s）

#### 2.1.4 控制器层面 - 核心控制器详解

##### 1. Deployment Controller
- **职责**：管理 Deployment → ReplicaSet 生命周期，实现滚动更新
- **工作流程**：
  1. 监听 Deployment 变化
  2. 创建新 ReplicaSet（或更新现有）
  3. 按 `maxSurge`/`maxUnavailable` 策略逐步缩放新旧 RS
  4. 更新 Deployment 状态（replicas/updatedReplicas/availableReplicas）
- **并发参数**：`--concurrent-deployment-syncs`（默认 5）

##### 2. ReplicaSet Controller
- **职责
...(截断)

---

### 05 Webhook Admission Troubleshooting

#### 0. 10 分钟快速诊断

1. **确认影响面**：`kubectl get mutatingwebhookconfigurations,validatingwebhookconfigurations`，识别是否为系统/业务 Webhook；查看受影响资源类型与命名空间。
2. **快速复现与事件**：`kubectl apply -f <manifest> --validate=false` 观察输出；`kubectl describe <resource>` 查看准入拒绝/超时事件。
3. **连通性与证书**：`kubectl get endpoints -n <ns> <svc>`、`kubectl logs -n kube-system kube-apiserver-<node> | grep webhook | head`、`openssl x509 -in <tls.crt> -noout -enddate` 验证到期。
4. **配置核对**：检查 `failurePolicy` (Fail/Ignore)、`timeoutSeconds`、`namespaceSelector`/`objectSelector`、`matchPolicy`、`sideEffects`，以及是否排除 `kube-system` 防止自阻塞。
5. **性能与超时**：`kubectl top pod -n <ns> -l app=<webhook>`，必要时提高 Webhook 副本、HPA，或提升 `timeoutSeconds`（默认 10s，建议 ≤ 30s）。
6. **快速缓解**：
   - 非关键拦截：临时将 `failurePolicy` 改为 `Ignore`，或收窄 `rules`/`namespaceSelector` 以放行核心流量。
   - 证书问题：立即轮转 TLS Secret 并更新 CA Bundle；使用 cert-manager 时触发 `renewBefore`。
   - 循环依赖：为 Webhook 自身资源添加排除标签/命名空间，必要时使用 `reinvocationPolicy=IfNeeded`。
7. **证据留存**：保存拒绝事件、API Server Webhook 调用日志、Webhook Pod 日志、配置 diff 和 CA/证书到期时间。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
Webhook/准入控制问题
        │
        ├─── 资源被拒绝？
        │         │
        │         ├─ 查看拒绝原因 ──→ 检查 Webhook 日志/策略
        │         ├─ 确认是否应该被拒绝 ──→ 调整资源或策略
        │         └─ Webhook 配置错误 ──→ 修正 matchLabels/rules
        │
        ├─── 连接/超时问题？
        │         │
        │         ├─ Webhook 服务是否运行 ──→ 检查 Deployment/Pod
        │         ├─ Service 是否可达 ──→ 检查 Endpoints
        │         ├─ TLS 证书是否有效 ──→ 检查证书配置
        │         └─ 超时设置是否合理 ──→ 调整 timeoutSeconds
        │
        ├─── Webhook 未生效？
        │         │
        │         ├─ 检查 failurePolicy ──→ Ignore vs Fail
        │         ├─ 检查 matchPolicy ──→ Exact vs Equivalent
        │         ├─ 检查 namespaceSelector ──→ 确认匹配
        │         └─ 检查 objectSelector ──→ 确认匹配
        │
        └─── 系统影响？
                  │
                  ├─ 检查系统命名空间排除 ──→ 添加 kube-system 排除
                  └─ 检查循环依赖 ──→ 使用 reinvocationPolicy
```

---

### 06 Apf Troubleshooting

#### 0. 10 分钟快速诊断

1. **确认 APF 状态**：`kubectl get --raw "/readyz?verbose" | grep flowcontrol` 确认 ready；`kubectl get flowschema,prioritylevelconfiguration` 确认资源存在。
2. **定位被限流的请求**：查客户端/组件日志的 `429`，配合 API Server 日志 `apiserver_flowcontrol` 相关字段；抓取 `apiserver_flowcontrol_rejected_requests_total`、`apiserver_flowcontrol_request_queue_length_after_enqueue_bucket` 最高的 FlowSchema/PL。
3. **匹配校验**：`kubectl describe flowschema <name>`，检查 `matchingPrecedence`、`distinguisherMethod`、`rules` 是否覆盖预期请求；`kubectl auth can-i --as=<user> --list` 辅助确认主体匹配。
4. **并发与队列容量**：检查命中的 PriorityLevelConfiguration：`concurrencyShares`、`queues`、`queueLengthLimit`、`handSize`，确认是否过低导致排队/拒绝。
5. **系统流量保护**：确保 `exempt`、`system`、`leader-election` 这类 PL 未被错误修改；关键控制面请求应落在高优先级队列。
6. **快速缓解**：
   - 对被 429 的业务请求：临时提高对应 PL 的 `concurrencyShares` 或 `queueLengthLimit`，或提升业务客户端 `qps/burst` 合理限速，避免爆发性 LIST/Watch。
   - 对系统/控制面受影响：恢复官方默认 APF 配置，或将控制面流量重定向到 `exempt/system` 级别；同时压制异常大流量来源。
   - 若配置混乱：导出当前 FS/PL，回滚到备份或默认模板后再逐步调优。
7. **证据留存**：保存 `/readyz?verbose` 输出、被限流请求示例、FS/PL 配置、关键 APF 指标快照用于复盘。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
APF 问题
    │
    ▼
┌───────────────────────┐
│  问题类型是什么？      │
└───────────────────────┘
    │
    ├── 请求被限流 (429) ────────────────────────────────────┐
    │                                                         │
    │   ┌─────────────────────────────────────────┐          │
    │   │ 确定请求被分配到哪个优先级              │          │
    │   │ 检查 APF 指标                           │          │
    │   └─────────────────────────────────────────┘          │
    │                  │                                      │
    │                  ▼                                      │
    │   ┌─────────────────────────────────────────┐          │
    │   │ 该优先级的并发配额是否足够?             │          │
    │   └─────────────────────────────────────────┘          │
    │          │                │                             │
    │         否               是                             │
    │          │                │                             │
    │          ▼                ▼                             │
    │   ┌────────────┐   ┌────────────────┐                  │
    │   │ 调整优先级 │   │ 检查请求是否   │                  │
    │   │ 或并发配置 │   │ 分配到错误级别 │                  │
    │   └────────────┘   └────────────────┘                  │
    │                                                         │
    ├── FlowSchema 配置问题 ─────────────────────────────────┤
    │                                                         │
    │   ┌─────────────────────────────────────────┐          │
    │ 
...(截断)

---

### 07 Control Plane Security Troubleshooting

#### 🎯 排查方法与步骤



#### 排查原理说明

控制平面安全加固涉及多个层面的安全配置，包括：
1. **传输层安全**：TLS 加密、证书管理
2. **身份认证**：RBAC、ServiceAccount、OIDC
3. **授权控制**：细粒度权限管理
4. **审计日志**：操作记录与合规性
5. **网络安全**：网络策略、防火墙规则

#### 排查逻辑决策树

```
安全问题发现
    ├── TLS 配置检查
    │   ├── 证书有效性
    │   ├── 加密算法强度
    │   └── 证书轮换机制
    ├── 认证授权检查
    │   ├── RBAC 配置合理性
    │   ├── ServiceAccount 权限
    │   └── 外部认证集成
    ├── 审计日志检查
    │   ├── 日志策略配置
    │   ├── 日志存储安全性
    │   └── 日志完整性保护
    └── 网络安全检查
        ├── 控制平面网络隔离
        ├── 组件间通信加密
        └── 外部访问控制
```

---

### 08 Control Plane Performance Troubleshooting

#### 排查方法与步骤



#### 诊断原理说明

控制平面性能瓶颈通常来源于以下几个方面：

1. **API Server 层面**：
   - 请求处理能力不足
   - 对象序列化/反序列化开销
   - 鉴权/授权处理延迟
   - Watch 机制资源消耗

2. **etcd 层面**：
   - 磁盘 I/O 性能瓶颈
   - 网络延迟影响
   - 数据库大小增长
   - 压缩/碎片整理不及时

3. **组件层面**：
   - 控制器工作队列积压
   - 调度算法复杂度过高
   - 缓存同步延迟

#### 性能诊断决策树

```
性能问题发现
    ├── API Server 性能分析
    │   ├── 请求延迟分布
    │   ├── QPS 统计分析
    │   ├── 资源对象数量
    │   └── 连接数限制
    ├── etcd 性能分析
    │   ├── WAL fsync 延迟
    │   ├── 磁盘 I/O 性能
    │   ├── 数据库大小
    │   └── 网络延迟
    ├── 组件性能分析
    │   ├── 工作队列深度
    │   ├── 控制器同步延迟
    │   ├── 调度器评分时间
    │   └── 缓存命中率
    └── 系统资源分析
        ├── CPU 使用率
        ├── 内存使用情况
        ├── 磁盘空间占用
        └── 网络带宽使用
```

---

### 09 Control Plane Ha Troubleshooting

#### 常见高可用问题现象

| 问题现象 | 典型表现 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| 控制平面节点宕机 | `control plane node NotReady` | ⭐⭐⭐ 高 | P0 |
| etcd 集群脑裂 | `etcd cluster is unhealthy` | ⭐⭐⭐ 高 | P0 |
| API Server 负载不均衡 | `some apiservers not responding` | ⭐⭐ 中 | P1 |
| Leader 选举频繁切换 | `leader election churning` | ⭐⭐ 中 | P1 |
| 控制平面组件启动失败 | `contr

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[skills/FTA Diagnostic Execution Engine|FTA 诊断引擎]]
- [[skills/backup-restore-etcd|etcd 备份恢复]]

## Related

- [[entities/kube-apiserver|kube-apiserver]] — kube-apiserver
- [[etcd]] — etcd
- [[cert-manager]] — cert-manager
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
