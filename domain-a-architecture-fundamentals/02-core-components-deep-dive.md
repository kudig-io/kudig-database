# Kubernetes 核心组件深度剖析 (Core Components Deep Dive)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubernetes.io/docs/concepts/overview/components](https://kubernetes.io/docs/concepts/overview/components/)

---

## 目录

1. [控制平面组件概览](#1-控制平面组件概览)
2. [kube-apiserver 深度剖析](#2-kube-apiserver-深度剖析)
3. [etcd 生产实践](#3-etcd-生产实践)
4. [kube-scheduler 调度优化](#4-kube-scheduler-调度优化)
5. [kube-controller-manager 控制器详解](#5-kube-controller-manager-控制器详解)
6. [cloud-controller-manager 云集成](#6-cloud-controller-manager-云集成)
7. [节点组件深度解析](#7-节点组件深度解析)
8. [附加组件](#8-附加组件)
9. [组件故障场景与恢复](#9-组件故障场景与恢复)
10. [性能基准与容量规划](#10-性能基准与容量规划)
11. [生产级配置模板](#11-生产级配置模板)
12. [诊断与排查工具](#12-诊断与排查工具)
13. [多角色视角专栏](#13-多角色视角专栏)

---

## 1. 控制平面组件概览

### 1.1 组件总览表

| 组件 | 角色 | 默认端口 | 关键配置标志 | 版本特定变更 | 监控端点 | 故障模式 | 运维最佳实践 |
|-----|------|---------|-------------|-------------|---------|---------|-------------|
| **kube-apiserver** | API网关，认证授权，准入控制 | 6443 | `--etcd-servers`, `--service-cluster-ip-range`, `--authorization-mode` | v1.29: 审计日志增强; v1.30: CEL准入策略GA | `/metrics`, `/healthz`, `/livez`, `/readyz` | 过载导致503; 证书过期 | 启用审计日志; 配置`--max-requests-inflight=400`; 监控请求延迟P99 |
| **etcd** | 分布式一致性存储，集群状态持久化 | 2379/2380 | `--data-dir`, `--quota-backend-bytes`, `--snapshot-count` | v3.5.x: 推荐版本; v3.5.9+: 修复数据损坏问题 | `/metrics`, `/health` | 磁盘满导致只读; 网络分区 | SSD存储; `--quota-backend-bytes=8589934592`(8GB); 每小时自动快照 |
| **kube-scheduler** | Pod到节点的调度决策 | 10259 | `--config`, `--leader-elect` | v1.25: 调度框架Beta; v1.27: 调度门GA | `/metrics`, `/healthz` | 调度延迟过高; 资源计算错误 | 自定义调度配置文件; 监控`scheduler_pending_pods`指标 |
| **kube-controller-manager** | 运行核心控制器循环 | 10257 | `--controllers`, `--concurrent-deployment-syncs`, `--leader-elect` | v1.27: 控制器拆分可选 | `/metrics`, `/healthz` | 控制器goroutine泄漏 | 调整并发参数; 监控控制器队列深度 |
| **cloud-controller-manager** | 云厂商集成（LB、节点、路由）| 10258 | `--cloud-provider`, `--controllers` | v1.25: 外部云控制器稳定 | `/metrics`, `/healthz` | 云API限流; 凭证过期 | 配置云API重试; ACK自动管理 |

### 1.2 组件间通信矩阵

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    Kubernetes Control Plane Communication                        │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   ┌──────────────┐                                                              │
│   │    etcd      │ ◄──── 唯一的状态存储 (gRPC :2379)                            │
│   │   集群       │       所有状态变更最终写入etcd                                │
│   └──────┬───────┘                                                              │
│          │ gRPC + TLS                                                           │
│          │ 客户端证书认证                                                        │
│          ▼                                                                      │
│   ┌──────────────┐     HTTPS :6443                                              │
│   │kube-apiserver│ ◄────────────────────────────────────────┐                   │
│   │   (HA x3)    │                                          │                   │
│   └──────┬───────┘                                          │                   │
│          │                                                  │                   │
│    ┌─────┼─────────────────────┬────────────────────┐       │                   │
│    │     │                     │                    │       │                   │
│    ▼     ▼                     ▼                    ▼       │                   │
│ ┌────────────┐  ┌──────────────┐  ┌─────────────┐  ┌───────┴──────┐            │
│ │ scheduler  │  │  controller  │  │   cloud     │  │   kubelet    │            │
│ │  :10259    │  │   manager    │  │  controller │  │   :10250     │            │
│ │            │  │   :10257     │  │   :10258    │  │              │            │
│ └────────────┘  └──────────────┘  └─────────────┘  └──────────────┘            │
│                                                            │                    │
│   组件 → apiserver: 使用ServiceAccount Token或客户端证书     │                    │
│   apiserver → kubelet: 使用kubelet serving证书              │                    │
│                                                            ▼                    │
│                                                     ┌──────────────┐            │
│                                                     │  kube-proxy  │            │
│                                                     │   :10249     │            │
│                                                     └──────────────┘            │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 组件启动顺序与依赖

| 启动顺序 | 组件 | 依赖条件 | 启动超时 | 健康检查 | 故障影响 |
|---------|------|---------|---------|---------|---------|
| 1 | **etcd** | 网络, 存储 | 60s | `etcdctl endpoint health` | 整个集群不可用 |
| 2 | **kube-apiserver** | etcd健康 | 30s | `curl -k https://localhost:6443/healthz` | 所有API调用失败 |
| 3 | **kube-controller-manager** | apiserver可用 | 30s | `/healthz` 端点 | 控制器停止协调 |
| 4 | **kube-scheduler** | apiserver可用 | 30s | `/healthz` 端点 | 新Pod无法调度 |
| 5 | **kubelet** | apiserver可用 | 30s | `curl http://localhost:10248/healthz` | 节点NotReady |
| 6 | **kube-proxy** | apiserver, kubelet | 30s | `/healthz` 端点 | Service网络故障 |
| 7 | **CoreDNS** | kube-proxy, CNI | 60s | DNS查询测试 | 服务发现失败 |
| 8 | **CNI** | kubelet | 60s | Pod网络测试 | Pod网络不通 |

### 1.4 组件版本兼容性矩阵

| 组件 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|-----|-------|-------|-------|-------|-------|-------|-------|-------|
| **etcd** | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x | 3.5.x |
| **containerd** | 1.6+ | 1.6+ | 1.7+ | 1.7+ | 1.7+ | 1.7+/2.0 | 1.7+/2.0 | 2.0+ |
| **CoreDNS** | 1.9+ | 1.9+ | 1.10+ | 1.10+ | 1.11+ | 1.11+ | 1.11+ | 1.11+ |
| **Metrics Server** | 0.6+ | 0.6+ | 0.6+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ | 0.7+ |
| **Calico** | 3.24+ | 3.25+ | 3.26+ | 3.27+ | 3.27+ | 3.28+ | 3.28+ | 3.28+ |
| **Cilium** | 1.12+ | 1.13+ | 1.14+ | 1.14+ | 1.15+ | 1.15+ | 1.16+ | 1.16+ |

---

## 2. kube-apiserver 深度剖析

### 2.1 架构概述

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           kube-apiserver Architecture                            │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                         Request Processing Pipeline                      │    │
│  │                                                                          │    │
│  │   Client Request                                                         │    │
│  │        │                                                                 │    │
│  │        ▼                                                                 │    │
│  │   ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐     │    │
│  │   │ 1.认证 AuthN │───▶│ 2.授权 AuthZ│───▶│ 3.准入控制 Admission   │     │    │
│  │   │             │    │             │    │   Mutating → Validating │     │    │
│  │   │ - X509证书  │    │ - RBAC      │    │   - Pod Security       │     │    │
│  │   │ - Token     │    │ - Node      │    │   - ResourceQuota      │     │    │
│  │   │ - OIDC      │    │ - Webhook   │    │   - LimitRange         │     │    │
│  │   │ - Webhook   │    │ - ABAC(弃用)│    │   - Webhook            │     │    │
│  │   └─────────────┘    └─────────────┘    └───────────┬─────────────┘     │    │
│  │                                                     │                    │    │
│  │                                                     ▼                    │    │
│  │   ┌─────────────────────────────────────────────────────────────────┐   │    │
│  │   │                    4. REST Handler                               │   │    │
│  │   │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │   │    │
│  │   │  │  GET    │ │  LIST   │ │  WATCH  │ │ CREATE  │ │ DELETE  │   │   │    │
│  │   │  └─────────┘ └─────────┘ └─────────┘ └─────────┘ └─────────┘   │   │    │
│  │   └──────────────────────────────┬──────────────────────────────────┘   │    │
│  │                                  │                                      │    │
│  │                                  ▼                                      │    │
│  │   ┌─────────────────────────────────────────────────────────────────┐   │    │
│  │   │                    5. Storage Layer                              │   │    │
│  │   │   ┌──────────────┐    ┌──────────────┐    ┌──────────────────┐  │   │    │
│  │   │   │   Cacher     │    │  etcd Client │    │   Watch Cache    │  │   │    │
│  │   │   │ (内存缓存)    │    │  (gRPC)      │    │   (事件缓存)     │  │   │    │
│  │   │   └──────────────┘    └──────────────┘    └──────────────────┘  │   │    │
│  │   └──────────────────────────────┬──────────────────────────────────┘   │    │
│  │                                  │                                      │    │
│  └──────────────────────────────────┼──────────────────────────────────────┘    │
│                                     │                                           │
│                                     ▼                                           │
│                              ┌──────────────┐                                   │
│                              │     etcd     │                                   │
│                              └──────────────┘                                   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 认证机制详解

| 认证方式 | 配置参数 | 适用场景 | 安全级别 | 运维复杂度 |
|---------|---------|---------|---------|----------|
| **X509客户端证书** | `--client-ca-file` | 组件间通信、管理员 | 高 | 高(证书轮转) |
| **静态Token** | `--token-auth-file` | 测试环境 | 低 | 低 |
| **Bootstrap Token** | `--enable-bootstrap-token-auth` | 节点加入集群 | 中 | 低 |
| **ServiceAccount Token** | 自动 | Pod内访问API | 中 | 低 |
| **OIDC** | `--oidc-issuer-url` | 企业SSO集成 | 高 | 中 |
| **Webhook Token** | `--authentication-token-webhook-config-file` | 自定义认证 | 高 | 高 |

```yaml
# ServiceAccount Token 投射配置示例 (v1.20+推荐)
apiVersion: v1
kind: Pod
metadata:
  name: app-with-token
spec:
  serviceAccountName: my-sa
  containers:
  - name: app
    image: myapp:latest
    volumeMounts:
    - name: token
      mountPath: /var/run/secrets/tokens
  volumes:
  - name: token
    projected:
      sources:
      - serviceAccountToken:
          path: token
          expirationSeconds: 3600   # 1小时过期
          audience: api             # Token受众
```

### 2.3 授权机制详解

| 授权模式 | 配置 | 说明 | 推荐场景 |
|---------|------|------|---------|
| **RBAC** | `--authorization-mode=RBAC` | 基于角色的访问控制 | 生产环境标准 |
| **Node** | `--authorization-mode=Node` | kubelet专用授权 | 必须启用 |
| **Webhook** | `--authorization-webhook-config-file` | 外部授权服务 | 复杂授权需求 |
| **ABAC** | `--authorization-policy-file` | 基于属性(已弃用) | 不推荐 |

```yaml
# RBAC ClusterRole 示例: 只读运维角色
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ops-readonly
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets", "nodes"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "daemonsets", "statefulsets"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get"]
---
# 聚合ClusterRole (自动聚合到admin角色)
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: custom-crd-admin
  labels:
    rbac.authorization.k8s.io/aggregate-to-admin: "true"
rules:
- apiGroups: ["example.com"]
  resources: ["*"]
  verbs: ["*"]
```

### 2.4 准入控制详解

| 准入控制器 | 类型 | 作用 | 默认启用 | 版本 |
|-----------|------|------|---------|------|
| **NamespaceLifecycle** | Validating | 防止在终止中的NS创建对象 | 是 | 全版本 |
| **LimitRanger** | Mutating | 应用默认资源限制 | 是 | 全版本 |
| **ResourceQuota** | Validating | 强制资源配额 | 是 | 全版本 |
| **PodSecurity** | Validating | Pod安全标准(替代PSP) | 是 | v1.25+ |
| **MutatingAdmissionWebhook** | Mutating | 动态修改请求 | 是 | v1.16+ |
| **ValidatingAdmissionWebhook** | Validating | 动态验证请求 | 是 | v1.16+ |
| **ValidatingAdmissionPolicy** | Validating | CEL表达式验证 | 否 | v1.30 GA |

```yaml
# ValidatingAdmissionPolicy 示例 (v1.30+)
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-resource-limits
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: "object.spec.containers.all(c, has(c.resources) && has(c.resources.limits))"
    message: "所有容器必须设置资源limits"
    reason: Invalid
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: require-resource-limits-binding
spec:
  policyName: require-resource-limits
  validationActions:
  - Deny
  matchResources:
    namespaceSelector:
      matchLabels:
        enforce-limits: "true"
```

### 2.5 API优先级和公平性 (APF)

```yaml
# FlowSchema: 定义请求分类规则
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: system-critical
spec:
  priorityLevelConfiguration:
    name: system
  matchingPrecedence: 100  # 越小优先级越高
  distinguisherMethod:
    type: ByUser
  rules:
  - subjects:
    - kind: Group
      group:
        name: system:masters
    resourceRules:
    - verbs: ["*"]
      apiGroups: ["*"]
      resources: ["*"]
---
# PriorityLevelConfiguration: 定义优先级行为
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: workload-high
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 100
    lendablePercent: 50
    limitResponse:
      type: Queue
      queuing:
        queues: 64
        handSize: 6
        queueLengthLimit: 50
```

### 2.6 审计日志配置

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
# 不记录的请求
omitStages:
  - "RequestReceived"
rules:
  # 不记录健康检查
  - level: None
    users: ["system:kube-proxy"]
    verbs: ["watch"]
    resources:
      - group: ""
        resources: ["endpoints", "services", "services/status"]
  
  # 不记录系统只读请求
  - level: None
    userGroups: ["system:nodes"]
    verbs: ["get"]
    resources:
      - group: ""
        resources: ["nodes", "nodes/status"]
  
  # Secret读取只记录元数据
  - level: Metadata
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
  
  # 记录所有认证相关操作
  - level: RequestResponse
    resources:
      - group: "authentication.k8s.io"
      - group: "authorization.k8s.io"
  
  # 默认记录元数据
  - level: Metadata
    omitStages:
      - "RequestReceived"
```

### 2.7 关键配置参数

| 参数 | 默认值 | 推荐生产值 | 说明 |
|-----|-------|----------|------|
| `--max-requests-inflight` | 400 | 800-1600 | 最大并发非变更请求 |
| `--max-mutating-requests-inflight` | 200 | 400-800 | 最大并发变更请求 |
| `--request-timeout` | 1m | 1m | 请求超时时间 |
| `--watch-cache-sizes` | 默认 | 自定义 | Watch缓存大小 |
| `--etcd-compaction-interval` | 5m | 5m | etcd压缩间隔 |
| `--audit-log-maxage` | 0 | 30 | 审计日志保留天数 |
| `--audit-log-maxbackup` | 0 | 10 | 审计日志备份数 |
| `--audit-log-maxsize` | 0 | 100 | 单个审计日志大小(MB) |
| `--enable-priority-and-fairness` | true | true | 启用APF限流 |

### 2.8 关键监控指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `apiserver_request_duration_seconds` | 请求延迟 | P99 < 1s | P99 > 5s |
| `apiserver_current_inflight_requests` | 当前并发请求 | < max*0.8 | > max*0.9 |
| `apiserver_request_total` | 请求总数(按code) | 5xx < 0.1% | 5xx > 1% |
| `etcd_request_duration_seconds` | etcd请求延迟 | P99 < 100ms | P99 > 500ms |
| `apiserver_admission_controller_admission_duration_seconds` | 准入控制延迟 | P99 < 100ms | P99 > 500ms |
| `apiserver_watch_events_sizes` | Watch事件大小 | - | 持续增长 |

---

## 3. etcd 生产实践

### 3.1 Raft共识机制

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            etcd Raft Consensus                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   正常运行状态 (Leader处理所有写请求):                                           │
│                                                                                  │
│        ┌──────────────┐                                                         │
│        │   Client     │                                                         │
│        └──────┬───────┘                                                         │
│               │ Write Request                                                   │
│               ▼                                                                 │
│        ┌──────────────┐    Append Entries    ┌──────────────┐                  │
│        │   Leader     │ ──────────────────▶  │  Follower 1  │                  │
│        │   (Node 1)   │                      └──────────────┘                  │
│        │              │    Append Entries    ┌──────────────┐                  │
│        │   Term: 5    │ ──────────────────▶  │  Follower 2  │                  │
│        └──────┬───────┘                      └──────────────┘                  │
│               │                                                                 │
│               │ 多数节点确认后提交                                               │
│               ▼                                                                 │
│        ┌──────────────┐                                                         │
│        │   Commit     │                                                         │
│        │   Index: 100 │                                                         │
│        └──────────────┘                                                         │
│                                                                                  │
│   Leader选举 (选举超时触发):                                                     │
│                                                                                  │
│   Follower ──(election timeout)──▶ Candidate ──(majority votes)──▶ Leader      │
│       │                                 │                              │        │
│       │ discovers higher term           │ discovers higher term        │        │
│       └─────────────────────────────────┴──────────────────────────────┘        │
│                                                                                  │
│   关键参数:                                                                      │
│   - heartbeat-interval: 100ms (Leader心跳间隔)                                  │
│   - election-timeout: 1000ms (选举超时, 建议10x heartbeat)                      │
│   - 网络RTT应 < heartbeat-interval/2                                            │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 MVCC存储引擎

| 组件 | 作用 | 关键特性 |
|------|------|---------|
| **TreeIndex** | 内存B树索引 | Key → Revision映射，支持范围查询 |
| **BoltDB** | 持久化后端 | B+树存储，ACID事务，页面管理 |
| **WAL** | 预写日志 | 顺序写入，崩溃恢复，数据完整性 |
| **Snapshot** | 状态快照 | 定期压缩，加速恢复，减少WAL |
| **Compaction** | 版本压缩 | 清理历史版本，释放空间 |

```bash
# etcd 数据目录结构
/var/lib/etcd/
├── member/
│   ├── snap/           # 快照文件
│   │   ├── 0000000000000001-0000000000100000.snap
│   │   └── db          # BoltDB数据文件
│   └── wal/            # WAL日志文件
│       ├── 0000000000000000-0000000000000000.wal
│       └── 0000000000000001-0000000000010000.wal
```

### 3.3 集群部署配置

```yaml
# etcd StaticPod配置示例 (kubeadm生成)
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
  labels:
    component: etcd
    tier: control-plane
spec:
  hostNetwork: true
  priorityClassName: system-node-critical
  containers:
  - name: etcd
    image: registry.k8s.io/etcd:3.5.12-0
    command:
    - etcd
    # === 集群配置 ===
    - --name=master-1
    - --data-dir=/var/lib/etcd
    - --listen-peer-urls=https://192.168.1.101:2380
    - --listen-client-urls=https://192.168.1.101:2379,https://127.0.0.1:2379
    - --advertise-client-urls=https://192.168.1.101:2379
    - --initial-advertise-peer-urls=https://192.168.1.101:2380
    - --initial-cluster=master-1=https://192.168.1.101:2380,master-2=https://192.168.1.102:2380,master-3=https://192.168.1.103:2380
    - --initial-cluster-state=new
    - --initial-cluster-token=etcd-cluster-prod
    # === 性能调优 ===
    - --quota-backend-bytes=8589934592        # 8GB存储配额
    - --snapshot-count=10000                   # 每10000次事务触发快照
    - --auto-compaction-mode=periodic
    - --auto-compaction-retention=1h           # 每小时自动压缩
    - --max-request-bytes=10485760            # 10MB最大请求
    # === 安全配置 ===
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --peer-client-cert-auth=true
    - --client-cert-auth=true
    # === 监控 ===
    - --listen-metrics-urls=http://0.0.0.0:2381
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
      limits:
        cpu: 2000m
        memory: 4Gi
    volumeMounts:
    - mountPath: /var/lib/etcd
      name: etcd-data
    - mountPath: /etc/kubernetes/pki/etcd
      name: etcd-certs
    livenessProbe:
      httpGet:
        path: /health?serializable=true
        port: 2381
        scheme: HTTP
      initialDelaySeconds: 10
      periodSeconds: 10
      timeoutSeconds: 15
      failureThreshold: 8
  volumes:
  - hostPath:
      path: /var/lib/etcd
      type: DirectoryOrCreate
    name: etcd-data
  - hostPath:
      path: /etc/kubernetes/pki/etcd
      type: DirectoryOrCreate
    name: etcd-certs
```

### 3.4 备份与恢复

```bash
# === 备份 ===
# 创建快照
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M%S).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证快照
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-20260120.db --write-out=table

# === 恢复 (灾难恢复) ===
# 1. 停止所有kube-apiserver
# 2. 停止所有etcd
# 3. 在每个etcd节点执行恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260120.db \
  --name=master-1 \
  --initial-cluster=master-1=https://192.168.1.101:2380,master-2=https://192.168.1.102:2380,master-3=https://192.168.1.103:2380 \
  --initial-advertise-peer-urls=https://192.168.1.101:2380 \
  --data-dir=/var/lib/etcd-restore

# 4. 替换数据目录
mv /var/lib/etcd /var/lib/etcd-old
mv /var/lib/etcd-restore /var/lib/etcd
# 5. 启动etcd和apiserver

# === 自动备份脚本 ===
#!/bin/bash
# etcd-backup.sh - 放入cron每小时执行
BACKUP_DIR=/backup/etcd
RETENTION_DAYS=7
TIMESTAMP=$(date +%Y%m%d-%H%M%S)

# 创建备份
ETCDCTL_API=3 etcdctl snapshot save ${BACKUP_DIR}/etcd-${TIMESTAMP}.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 清理旧备份
find ${BACKUP_DIR} -name "etcd-*.db" -mtime +${RETENTION_DAYS} -delete

# 上传到对象存储 (可选)
# aws s3 cp ${BACKUP_DIR}/etcd-${TIMESTAMP}.db s3://k8s-backup/etcd/
```

### 3.5 运维操作手册

| 操作 | 命令 | 风险等级 | 说明 |
|------|------|---------|------|
| **健康检查** | `etcdctl endpoint health --cluster` | 低 | 检查所有节点健康 |
| **成员列表** | `etcdctl member list -w table` | 低 | 查看集群成员 |
| **数据库状态** | `etcdctl endpoint status -w table` | 低 | 查看数据库大小、Leader |
| **告警查看** | `etcdctl alarm list` | 低 | 查看告警(如空间不足) |
| **手动压缩** | `etcdctl compact <revision>` | 中 | 压缩到指定版本 |
| **碎片整理** | `etcdctl defrag --cluster` | 中 | 回收磁盘空间(会短暂阻塞) |
| **解除告警** | `etcdctl alarm disarm` | 中 | 解除空间告警 |
| **添加成员** | `etcdctl member add <name> --peer-urls=<url>` | 高 | 扩容集群 |
| **移除成员** | `etcdctl member remove <id>` | 高 | 缩容或替换故障节点 |

### 3.6 关键监控指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `etcd_server_has_leader` | 是否有Leader | 1 | 0 |
| `etcd_server_leader_changes_seen_total` | Leader切换次数 | 增长缓慢 | 快速增长 |
| `etcd_mvcc_db_total_size_in_bytes` | 数据库大小 | < quota*0.8 | > quota*0.9 |
| `etcd_disk_wal_fsync_duration_seconds` | WAL同步延迟 | P99 < 10ms | P99 > 25ms |
| `etcd_disk_backend_commit_duration_seconds` | 后端提交延迟 | P99 < 25ms | P99 > 100ms |
| `etcd_network_peer_round_trip_time_seconds` | 节点间RTT | P99 < 50ms | P99 > 100ms |

---

## 4. kube-scheduler 调度优化

### 4.1 调度框架架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes Scheduler Framework                            │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   Pod待调度队列                                                                  │
│        │                                                                        │
│        ▼                                                                        │
│   ┌─────────────────────────────────────────────────────────────────────────┐  │
│   │                     Scheduling Cycle (同步)                              │  │
│   │                                                                          │  │
│   │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐                 │  │
│   │  │ PreFilter    │──▶│    Filter    │──▶│ PostFilter   │                 │  │
│   │  │ 预过滤        │   │    过滤      │   │ 后过滤(抢占) │                 │  │
│   │  └──────────────┘   └──────────────┘   └──────────────┘                 │  │
│   │         │                  │                  │                          │  │
│   │         ▼                  ▼                  ▼                          │  │
│   │  - 检查Pod需求      - NodeAffinity      - 抢占调度                       │  │
│   │  - 计算扩展资源     - Taints/Tolerations - 找到可抢占Pod                 │  │
│   │                     - NodePorts                                          │  │
│   │                     - PodTopologySpread                                  │  │
│   │                                                                          │  │
│   │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐                 │  │
│   │  │ PreScore     │──▶│    Score     │──▶│ NormalizeScore│                │  │
│   │  │ 预评分        │   │    评分      │   │   归一化评分  │                 │  │
│   │  └──────────────┘   └──────────────┘   └──────────────┘                 │  │
│   │         │                  │                  │                          │  │
│   │         ▼                  ▼                  ▼                          │  │
│   │  - 计算评分状态     - NodeAffinity: 0-100   - 归一化到0-100              │  │
│   │                     - ImageLocality: 0-100                               │  │
│   │                     - BalancedAllocation                                 │  │
│   │                                                                          │  │
│   │  ┌──────────────┐                                                        │  │
│   │  │   Reserve    │◀── 选择得分最高的节点                                   │  │
│   │  │   预留资源    │                                                        │  │
│   │  └──────────────┘                                                        │  │
│   │                                                                          │  │
│   └─────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
│   ┌─────────────────────────────────────────────────────────────────────────┐  │
│   │                      Binding Cycle (异步)                                │  │
│   │                                                                          │  │
│   │  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐                 │  │
│   │  │   Permit     │──▶│   PreBind    │──▶│     Bind     │                 │  │
│   │  │  许可/等待    │   │   绑定前     │   │    绑定      │                 │  │
│   │  └──────────────┘   └──────────────┘   └──────────────┘                 │  │
│   │         │                  │                  │                          │  │
│   │         ▼                  ▼                  ▼                          │  │
│   │  - Gang调度等待     - PV绑定          - 更新Pod.spec.nodeName            │  │
│   │  - 资源锁定         - 存储预留        - 通知kubelet                       │  │
│   │                                                                          │  │
│   │  ┌──────────────┐                                                        │  │
│   │  │   PostBind   │                                                        │  │
│   │  │   绑定后     │                                                        │  │
│   │  └──────────────┘                                                        │  │
│   │                                                                          │  │
│   └─────────────────────────────────────────────────────────────────────────┘  │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 内置调度插件

| 插件名称 | 阶段 | 作用 | 默认权重 |
|---------|------|------|---------|
| **NodeUnschedulable** | Filter | 过滤不可调度节点 | - |
| **NodeName** | Filter | 匹配指定节点名 | - |
| **TaintToleration** | Filter/Score | Taint容忍 | 3 |
| **NodeAffinity** | Filter/Score | 节点亲和性 | 2 |
| **PodTopologySpread** | Filter/Score | Pod拓扑分布 | 2 |
| **NodeResourcesFit** | Filter/Score | 资源匹配 | 1 |
| **NodeResourcesBalancedAllocation** | Score | 资源均衡分配 | 1 |
| **ImageLocality** | Score | 镜像本地性 | 1 |
| **InterPodAffinity** | Filter/Score | Pod间亲和性 | 2 |
| **VolumeBinding** | Filter/PreBind | 存储卷绑定 | - |
| **VolumeRestrictions** | Filter | 卷限制检查 | - |

### 4.3 调度配置文件

```yaml
# /etc/kubernetes/scheduler-config.yaml
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
leaderElection:
  leaderElect: true
  resourceNamespace: kube-system
  resourceName: kube-scheduler
clientConnection:
  kubeconfig: /etc/kubernetes/scheduler.conf
  qps: 100
  burst: 200

profiles:
- schedulerName: default-scheduler
  plugins:
    # 启用的评分插件及权重
    score:
      enabled:
      - name: NodeResourcesBalancedAllocation
        weight: 1
      - name: ImageLocality
        weight: 1
      - name: NodeAffinity
        weight: 2
      - name: TaintToleration
        weight: 3
      - name: PodTopologySpread
        weight: 2
  pluginConfig:
  # NodeResourcesFit配置
  - name: NodeResourcesFit
    args:
      scoringStrategy:
        type: LeastAllocated  # 或 MostAllocated, RequestedToCapacityRatio
        resources:
        - name: cpu
          weight: 1
        - name: memory
          weight: 1
        - name: nvidia.com/gpu
          weight: 5  # GPU资源更高权重
  # PodTopologySpread配置
  - name: PodTopologySpread
    args:
      defaultingType: List
      defaultConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway

# 第二个调度器配置(GPU专用)
- schedulerName: gpu-scheduler
  plugins:
    score:
      enabled:
      - name: NodeResourcesBalancedAllocation
        weight: 1
      - name: NodeResourcesFit
        weight: 10  # 资源匹配更高权重
```

### 4.4 高级调度策略

```yaml
# Pod拓扑分布约束 - 跨可用区均匀分布
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 6
  template:
    spec:
      topologySpreadConstraints:
      # 跨可用区分布
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-app
      # 跨节点分布
      - maxSkew: 2
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: web-app
      # 节点亲和性
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: node-type
                operator: In
                values:
                - production
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: gpu-type
                operator: In
                values:
                - nvidia-a100
        # Pod反亲和性 - 避免同一节点
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: web-app
            topologyKey: kubernetes.io/hostname
```

### 4.5 抢占调度

| PriorityClass | 值 | 抢占策略 | 使用场景 |
|--------------|-----|---------|---------|
| system-node-critical | 2000001000 | PreemptLowerPriority | 系统关键Pod |
| system-cluster-critical | 2000000000 | PreemptLowerPriority | 集群关键Pod |
| high-priority | 1000000 | PreemptLowerPriority | 业务关键服务 |
| default | 0 | PreemptLowerPriority | 默认 |
| batch-low | -100 | Never | 批处理任务 |

```yaml
# 自定义PriorityClass
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority-nonpreempting
value: 1000000
preemptionPolicy: Never  # 不抢占其他Pod
globalDefault: false
description: "高优先级但不抢占"
---
# 使用PriorityClass
apiVersion: v1
kind: Pod
metadata:
  name: critical-pod
spec:
  priorityClassName: high-priority-nonpreempting
  containers:
  - name: app
    image: myapp:latest
```

### 4.6 关键监控指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `scheduler_pending_pods` | 待调度Pod数 | < 10 | > 100 |
| `scheduler_scheduling_attempt_duration_seconds` | 调度尝试延迟 | P99 < 100ms | P99 > 1s |
| `scheduler_preemption_attempts_total` | 抢占尝试次数 | 增长缓慢 | 快速增长 |
| `scheduler_queue_incoming_pods_total` | 入队Pod总数 | - | - |
| `scheduler_schedule_attempts_total` | 调度尝试总数(按result) | scheduled占多数 | unschedulable持续增长 |

---

## 5. kube-controller-manager 控制器详解

### 5.1 控制器分类总览

| 类别 | 控制器 | 职责 | 关键参数 |
|------|-------|------|---------|
| **工作负载** | DeploymentController | 管理Deployment滚动更新 | `--concurrent-deployment-syncs` |
| | ReplicaSetController | 维护Pod副本数 | `--concurrent-replicaset-syncs` |
| | StatefulSetController | 管理有状态应用 | `--concurrent-statefulset-syncs` |
| | DaemonSetController | 确保节点运行守护Pod | `--concurrent-daemonset-syncs` |
| | JobController | 管理批处理任务 | `--concurrent-job-syncs` |
| | CronJobController | 定时任务调度 | - |
| **服务发现** | EndpointsController | 维护Endpoints对象 | `--concurrent-endpoint-syncs` |
| | EndpointSliceController | 维护EndpointSlice | `--concurrent-service-endpoint-syncs` |
| | ServiceController | 管理LoadBalancer类型Service | - |
| **节点管理** | NodeController | 节点健康检查、驱逐 | `--node-eviction-rate`, `--node-monitor-grace-period` |
| | NodeLifecycleController | 节点生命周期管理 | `--pod-eviction-timeout` |
| | TaintManager | 基于Taint驱逐Pod | - |
| **存储** | PersistentVolumeController | PV/PVC绑定 | - |
| | AttachDetachController | 卷附加/分离 | - |
| | VolumeExpandController | 卷扩展 | - |
| **安全** | ServiceAccountController | 创建默认SA | - |
| | TokenController | 管理SA Token | - |
| | CertificateSigningRequestController | 处理CSR | - |
| **资源管理** | ResourceQuotaController | 资源配额 | `--concurrent-resource-quota-syncs` |
| | NamespaceController | 命名空间清理 | `--concurrent-namespace-syncs` |
| | GarbageCollectorController | 级联删除 | `--concurrent-gc-syncs` |
| | HorizontalPodAutoscalerController | HPA伸缩 | `--horizontal-pod-autoscaler-sync-period` |

### 5.2 控制器工作原理

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                       Controller Pattern (Level-Triggered)                       │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   ┌───────────────────────────────────────────────────────────────────────────┐ │
│   │                           Informer Framework                               │ │
│   │                                                                            │ │
│   │  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────────────┐│ │
│   │  │   Reflector │───▶│   DeltaFIFO │───▶│         Indexer (Cache)         ││ │
│   │  │             │    │             │    │                                 ││ │
│   │  │ List & Watch│    │ Add/Update/ │    │  ┌─────────────────────────────┐││ │
│   │  │ API Server  │    │ Delete/Sync │    │  │  Thread-Safe Store          │││ │
│   │  └─────────────┘    └──────┬──────┘    │  │  (Namespace/Name Index)     │││ │
│   │                            │           │  └─────────────────────────────┘││ │
│   │                            ▼           └─────────────────────────────────┘│ │
│   │                    ┌──────────────┐                                        │ │
│   │                    │ Event Handler│                                        │ │
│   │                    │ OnAdd/Update/│                                        │ │
│   │                    │ Delete       │                                        │ │
│   │                    └──────┬───────┘                                        │ │
│   │                           │                                                │ │
│   └───────────────────────────┼────────────────────────────────────────────────┘ │
│                               │                                                  │
│                               ▼                                                  │
│   ┌───────────────────────────────────────────────────────────────────────────┐ │
│   │                            WorkQueue                                       │ │
│   │                                                                            │ │
│   │  ┌─────────────────────────────────────────────────────────────────────┐  │ │
│   │  │  Rate Limited Queue (指数退避重试)                                   │  │ │
│   │  │                                                                      │  │ │
│   │  │  namespace/name ──▶ [item1, item2, item3, ...]                      │  │ │
│   │  │                                                                      │  │ │
│   │  │  特性:                                                               │  │ │
│   │  │  - 去重: 同一对象多次事件只处理一次                                  │  │ │
│   │  │  - 限速: 失败重试有延迟(5ms→10ms→20ms...→1000s)                     │  │ │
│   │  │  - 公平: FIFO保证处理顺序                                           │  │ │
│   │  └─────────────────────────────────────────────────────────────────────┘  │ │
│   │                                                                            │ │
│   └──────────────────────────────────┬────────────────────────────────────────┘ │
│                                      │                                          │
│                                      ▼                                          │
│   ┌───────────────────────────────────────────────────────────────────────────┐ │
│   │                         Worker Goroutines                                  │ │
│   │                                                                            │ │
│   │  ┌─────────────────────────────────────────────────────────────────────┐  │ │
│   │  │  syncHandler(key string) error                                       │  │ │
│   │  │                                                                      │  │ │
│   │  │  1. 从Indexer获取对象当前状态 (Observed State)                       │  │ │
│   │  │  2. 计算期望状态 (Desired State = Spec)                              │  │ │
│   │  │  3. 比较差异, 执行调谐动作                                           │  │ │
│   │  │  4. 返回nil(成功)或error(重新入队)                                   │  │ │
│   │  │                                                                      │  │ │
│   │  │  示例: Deployment Controller                                         │  │ │
│   │  │  - 检查ReplicaSet数量是否匹配                                        │  │ │
│   │  │  - 执行滚动更新策略                                                  │  │ │
│   │  │  - 更新Deployment Status                                             │  │ │
│   │  └─────────────────────────────────────────────────────────────────────┘  │ │
│   │                                                                            │ │
│   └───────────────────────────────────────────────────────────────────────────┘ │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 5.3 节点控制器详解

| 参数 | 默认值 | 说明 |
|------|-------|------|
| `--node-monitor-period` | 5s | 检查NodeStatus周期 |
| `--node-monitor-grace-period` | 40s | 节点无响应标记Unknown前等待时间 |
| `--pod-eviction-timeout` | 5m | 节点NotReady后开始驱逐Pod等待时间 |
| `--node-eviction-rate` | 0.1 | 每秒驱逐节点数(正常情况) |
| `--secondary-node-eviction-rate` | 0.01 | 大规模故障时驱逐速率 |
| `--unhealthy-zone-threshold` | 0.55 | 不健康Zone阈值(>55%节点不健康) |
| `--large-cluster-size-threshold` | 50 | 大集群节点数阈值 |

```
节点状态机:
Ready ──(40s无心跳)──▶ Unknown ──(5m)──▶ 开始驱逐Pod

大规模故障保护:
- 单Zone >55%节点不健康 → 降低驱逐速率为0.01/s
- 所有Zone >55%不健康 → 停止驱逐
- 防止网络分区导致大规模误驱逐
```

### 5.4 关键配置参数

```yaml
# kube-controller-manager 生产配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-controller-manager
  namespace: kube-system
spec:
  containers:
  - name: kube-controller-manager
    image: registry.k8s.io/kube-controller-manager:v1.32.0
    command:
    - kube-controller-manager
    # === 通用配置 ===
    - --kubeconfig=/etc/kubernetes/controller-manager.conf
    - --authentication-kubeconfig=/etc/kubernetes/controller-manager.conf
    - --authorization-kubeconfig=/etc/kubernetes/controller-manager.conf
    - --leader-elect=true
    - --leader-elect-lease-duration=15s
    - --leader-elect-renew-deadline=10s
    - --leader-elect-retry-period=2s
    
    # === API限流 ===
    - --kube-api-qps=100
    - --kube-api-burst=200
    
    # === 并发控制 ===
    - --concurrent-deployment-syncs=10
    - --concurrent-replicaset-syncs=10
    - --concurrent-statefulset-syncs=10
    - --concurrent-endpoint-syncs=10
    - --concurrent-service-endpoint-syncs=10
    - --concurrent-namespace-syncs=10
    - --concurrent-gc-syncs=30
    
    # === 节点控制器 ===
    - --node-monitor-period=5s
    - --node-monitor-grace-period=40s
    - --pod-eviction-timeout=5m
    - --node-eviction-rate=0.1
    
    # === HPA ===
    - --horizontal-pod-autoscaler-sync-period=15s
    - --horizontal-pod-autoscaler-tolerance=0.1
    
    # === 证书 ===
    - --cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt
    - --cluster-signing-key-file=/etc/kubernetes/pki/ca.key
    - --cluster-signing-duration=8760h  # 1年
    
    # === 服务账号 ===
    - --service-account-private-key-file=/etc/kubernetes/pki/sa.key
    - --root-ca-file=/etc/kubernetes/pki/ca.crt
    - --use-service-account-credentials=true
    
    # === 资源配额 ===
    resources:
      requests:
        cpu: 200m
        memory: 512Mi
      limits:
        cpu: 2000m
        memory: 2Gi
```

### 5.5 关键监控指标

| 指标名称 | 含义 | 健康基准 | 告警阈值 |
|---------|------|---------|---------|
| `workqueue_depth` | 工作队列深度 | < 100 | > 1000 |
| `workqueue_adds_total` | 入队总数 | - | - |
| `workqueue_retries_total` | 重试总数 | 增长缓慢 | 快速增长 |
| `workqueue_work_duration_seconds` | 处理延迟 | P99 < 1s | P99 > 10s |
| `node_collector_evictions_total` | 节点驱逐总数 | 增长缓慢 | 突然增长 |

---

## 6. cloud-controller-manager 云集成

### 6.1 云控制器架构

| 控制器 | 作用 | 云厂商实现 |
|-------|------|-----------|
| **Node Controller** | 初始化节点标签、地址；检测节点删除 | 所有云厂商 |
| **Service Controller** | 为LoadBalancer类型Service创建云LB | SLB/CLB/ALB |
| **Route Controller** | 配置云VPC路由 | VPC路由表 |
| **Volume Controller** | 管理云存储卷(部分云) | 云盘/NAS |

### 6.2 阿里云ACK集成

| 组件 | ACK托管方式 | 配置入口 | 特殊优化 |
|-----|------------|---------|---------|
| **控制平面** | 全托管(Pro版) | 控制台 | 自动HA, 自动升级 |
| **etcd** | 托管 | 控制台 | 自动备份(每24小时) |
| **cloud-controller-manager** | 托管 | 控制台 | SLB自动配置 |
| **Ingress** | ALB/Nginx可选 | 控制台+YAML | ALB原生集成 |
| **CNI** | Terway/Flannel | 创建时选择 | ENI直通网络 |
| **存储** | CSI驱动托管 | StorageClass | 云盘/NAS/OSS |

```yaml
# ACK LoadBalancer Service示例
apiVersion: v1
kind: Service
metadata:
  name: nginx-lb
  annotations:
    # SLB配置
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: slb.s1.small
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: intranet
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-charge-type: paybytraffic
    # 健康检查
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: tcp
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-interval: "10"
spec:
  type: LoadBalancer
  selector:
    app: nginx
  ports:
  - port: 80
    targetPort: 80
```

---

## 7. 节点组件深度解析

### 7.1 kubelet 核心模块

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           kubelet Core Modules                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────── Pod Lifecycle Manager ───────────────────────────┐   │
│  │  syncPod() - 核心调谐函数                                                │   │
│  │  ├── 检查网络就绪 (CNI插件)                                              │   │
│  │  ├── 创建Pod Sandbox (pause容器)                                         │   │
│  │  ├── 启动Init Containers (顺序执行)                                      │   │
│  │  ├── 启动Main Containers (并行启动)                                      │   │
│  │  ├── 执行PostStart Hook                                                  │   │
│  │  └── 持续健康检查 (Liveness/Readiness/Startup)                           │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
│  ┌─────────────────────── PLEG (Pod Lifecycle Event Generator) ─────────────┐   │
│  │  职责: 监听容器运行时事件，生成Pod状态变化事件                            │   │
│  │                                                                           │   │
│  │  工作流程:                                                                │   │
│  │  1. 每秒调用CRI RuntimeService.ListPodSandbox()                          │   │
│  │  2. 对比上次状态 (Old State vs New State)                                │   │
│  │  3. 生成PodLifecycleEvent (ContainerStarted/Died/Removed)                │   │
│  │  4. 更新Pod状态缓存                                                       │   │
│  │  5. 触发syncPod()调谐                                                     │   │
│  │                                                                           │   │
│  │  PLEG故障: "PLEG is not healthy: pleg was last seen active..."          │   │
│  │  原因: CRI调用超时 (>3min) / 容器运行时hung                              │   │
│  │  影响: 节点标记NotReady, Pod无法调度到该节点                             │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
│  ┌─────────────────────── Probe Manager (健康检查) ─────────────────────────┐   │
│  │  Startup Probe: 成功前阻止其他探测                                        │   │
│  │  Liveness Probe: 失败→重启容器                                            │   │
│  │  Readiness Probe: 失败→从Service Endpoints移除                            │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
│  ┌─────────────────────── Eviction Manager (驱逐管理) ──────────────────────┐   │
│  │  监控资源压力: memory.available, nodefs.available, imagefs.available     │   │
│  │  Hard Eviction: 立即驱逐 (memory.available<100Mi)                        │   │
│  │  Soft Eviction: 宽限期后驱逐 (memory.available<200Mi, grace=1m30s)       │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 kubelet 生产配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# === 基础配置 ===
clusterDNS:
- 10.96.0.10
clusterDomain: cluster.local
staticPodPath: /etc/kubernetes/manifests

# === 容器运行时 ===
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd

# === 资源管理 ===
maxPods: 110

systemReserved:
  cpu: 1000m
  memory: 2Gi
  ephemeral-storage: 10Gi

kubeReserved:
  cpu: 100m
  memory: 1Gi
  ephemeral-storage: 5Gi

enforceNodeAllocatable:
- pods
- system-reserved
- kube-reserved

# === 驱逐配置 ===
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"

evictionMaxPodGracePeriod: 90

# === 镜像GC ===
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
imageMinimumGCAge: 2m

# === 日志 ===
containerLogMaxSize: 10Mi
containerLogMaxFiles: 5

# === 认证授权 ===
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
    cacheTTL: 2m
  x509:
    clientCAFile: /etc/kubernetes/pki/ca.crt

authorization:
  mode: Webhook

# === 特性门控 ===
featureGates:
  RotateKubeletServerCertificate: true
  GracefulNodeShutdown: true
```

### 7.3 kube-proxy 模式对比

| 特性 | iptables | IPVS | eBPF (Cilium) |
|------|----------|------|---------------|
| **查找复杂度** | O(n) | O(1) | O(1) |
| **规则更新** | 全量刷新 | 增量更新 | 增量更新 |
| **负载均衡** | 随机概率 | rr/lc/sh/dh/sed | 多种算法 |
| **连接保持** | 无 | 有 | 有 |
| **最大Service数** | ~5000 | ~100000 | ~100000+ |
| **内存占用** | 高(规则多) | 低 | 最低 |
| **调试难度** | 中 | 低 | 高 |
| **内核要求** | 2.4+ | 2.6+ | 4.19+ |

```yaml
# IPVS模式配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    kind: KubeProxyConfiguration
    mode: ipvs
    ipvs:
      scheduler: rr
      strictARP: true
      syncPeriod: 30s
    conntrack:
      maxPerCore: 32768
      min: 131072
      tcpEstablishedTimeout: 86400s
```

### 7.4 容器运行时对比

| 特性 | containerd | CRI-O | Docker (弃用) |
|------|-----------|-------|---------------|
| **架构** | 模块化 | 极简 | 单体 |
| **性能** | 高 | 高 | 中 |
| **内存占用** | ~50MB | ~30MB | ~100MB |
| **K8s原生** | 是 | 是 | 否 |
| **调试工具** | nerdctl, crictl | crictl, podman | docker |
| **适用场景** | 通用生产 | K8s专用 | 开发环境 |

---

## 8. 附加组件

| 组件 | 角色 | 部署方式 | 关键配置 | 监控指标 | 运维建议 |
|-----|------|---------|---------|---------|---------|
| **CoreDNS** | 集群DNS服务 | Deployment | `Corefile` ConfigMap | `coredns_dns_requests_total` | 副本数=max(2, nodes/100) |
| **Metrics Server** | 资源指标API | Deployment | `--kubelet-preferred-address-types` | `metrics_server_*` | HPA/VPA依赖 |
| **Ingress Controller** | 入口流量路由 | Deployment/DaemonSet | 因控制器而异 | 控制器特定 | 高可用部署 |
| **CNI Plugin** | 容器网络 | DaemonSet | `/etc/cni/net.d/` | 插件特定 | 选择适合场景的CNI |

---

## 9. 组件故障场景与恢复

### 9.1 控制平面故障矩阵

| 组件 | 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|------|---------|------|---------|---------|---------|
| **apiserver** | 503错误 | 过载/etcd慢 | 检查`apiserver_current_inflight_requests` | 增大并发限制/修复etcd | 启用APF限流 |
| **apiserver** | 证书过期 | 未自动轮转 | `kubeadm certs check-expiration` | `kubeadm certs renew all` | 监控证书有效期 |
| **etcd** | 数据库满 | 超过quota | `etcdctl alarm list` | 压缩+碎片整理+解除告警 | 自动压缩 |
| **etcd** | Leader切换频繁 | 网络/磁盘慢 | 检查`etcd_server_leader_changes_seen_total` | 检查网络/换SSD | 专用网络+NVMe |
| **scheduler** | Pod长时间Pending | 资源不足 | `kubectl describe pod` | 增加节点/放宽约束 | 监控资源使用率 |
| **controller** | Deployment不更新 | API限流/故障 | 检查`workqueue_depth` | 重启/提高QPS | 监控队列深度 |

### 9.2 节点组件故障矩阵

| 组件 | 故障现象 | 根因 | 诊断步骤 | 恢复方案 | 预防措施 |
|------|---------|------|---------|---------|---------|
| **kubelet** | PLEG不健康 | CRI超时 | `journalctl -u kubelet \| grep PLEG` | 重启containerd/kubelet | SSD存储 |
| **kubelet** | Node NotReady | 心跳失败 | `kubectl describe node` | 重启kubelet/检查网络 | 监控节点状态 |
| **kubelet** | 磁盘压力驱逐 | 空间不足 | 检查`nodefs.available` | 清理镜像/日志 | 配置镜像GC |
| **kube-proxy** | Service不通 | 规则未生成 | `ipvsadm -Ln` | 重启kube-proxy | 监控规则同步 |
| **kube-proxy** | conntrack满 | 高并发 | `sysctl net.netfilter.nf_conntrack_count` | 增大conntrack_max | 调整参数 |
| **containerd** | 镜像拉取失败 | 网络/认证 | `crictl pull <image>` | 配置镜像加速器 | 内网镜像仓库 |

---

## 10. 性能基准与容量规划

### 10.1 组件资源推荐配置

| 组件 | 小集群(<50节点) | 中集群(50-200节点) | 大集群(200-1000节点) | 超大集群(1000+节点) |
|-----|----------------|-------------------|---------------------|-------------------|
| **kube-apiserver** | 2核4G×2 | 4核8G×3 | 8核16G×3 | 16核32G×5 |
| **etcd** | 2核4G SSD×3 | 4核8G SSD×3 | 8核16G NVMe×5 | 16核32G NVMe×7 |
| **kube-scheduler** | 1核2G×2 | 2核4G×3 | 4核8G×3 | 8核16G×5 |
| **kube-controller-manager** | 2核4G×2 | 4核8G×3 | 8核16G×3 | 16核32G×5 |
| **CoreDNS** | 2副本,100m/128Mi | 3副本,200m/256Mi | 5副本,500m/512Mi | 10副本,1核/1Gi |

### 10.2 etcd存储容量规划

| 对象类型 | 单对象大小 | 10000对象 | 50000对象 | 100000对象 |
|---------|----------|----------|----------|-----------|
| **Pod** | ~5KB | 50MB | 250MB | 500MB |
| **Service** | ~2KB | 20MB | 100MB | 200MB |
| **ConfigMap** | ~10KB | 100MB | 500MB | 1GB |
| **Secret** | ~5KB | 50MB | 250MB | 500MB |
| **Event** | ~1KB | 10MB | 50MB | 100MB |

**推荐etcd配额**: 小集群4GB | 中集群8GB | 大集群16GB | 超大集群32GB

### 10.3 性能基准数据

| 场景 | 指标 | SSD基准 | NVMe基准 |
|------|------|---------|----------|
| **etcd写入** | fsync延迟 | 5-10ms | 1-3ms |
| **etcd读取** | 延迟 | <1ms | <0.5ms |
| **apiserver List** | P99延迟(5000 Pods) | 200ms | 150ms |
| **scheduler** | 调度吞吐量 | 50 Pods/s | 100 Pods/s |

---

## 11. 生产级配置模板

### 11.1 HA控制平面 kubeadm配置

```yaml
# kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.32.0
controlPlaneEndpoint: "loadbalancer.example.com:6443"

etcd:
  external:
    endpoints:
    - https://192.168.1.101:2379
    - https://192.168.1.102:2379
    - https://192.168.1.103:2379
    caFile: /etc/kubernetes/pki/etcd/ca.crt
    certFile: /etc/kubernetes/pki/apiserver-etcd-client.crt
    keyFile: /etc/kubernetes/pki/apiserver-etcd-client.key

networking:
  serviceSubnet: 10.96.0.0/12
  podSubnet: 10.244.0.0/16
  dnsDomain: cluster.local

apiServer:
  certSANs:
  - loadbalancer.example.com
  - 192.168.1.100
  extraArgs:
    max-requests-inflight: "800"
    max-mutating-requests-inflight: "400"
    audit-log-path: /var/log/kubernetes/audit.log
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    audit-policy-file: /etc/kubernetes/audit-policy.yaml
    event-ttl: "1h"

scheduler:
  extraArgs:
    kube-api-qps: "100"
    kube-api-burst: "200"

controllerManager:
  extraArgs:
    concurrent-deployment-syncs: "10"
    concurrent-replicaset-syncs: "10"
    kube-api-qps: "100"
    kube-api-burst: "200"
    node-eviction-rate: "0.1"
```

---

## 12. 诊断与排查工具

### 12.1 快速诊断脚本

```bash
#!/bin/bash
# k8s-healthcheck.sh

echo "=== 1. 集群基础信息 ==="
kubectl version --short 2>/dev/null || kubectl version
kubectl cluster-info

echo -e "\n=== 2. 节点状态 ==="
kubectl get nodes -o wide

echo -e "\n=== 3. 控制平面组件 ==="
kubectl get pods -n kube-system -o wide | grep -E "apiserver|etcd|scheduler|controller"

echo -e "\n=== 4. API健康检查 ==="
kubectl get --raw='/readyz?verbose' 2>/dev/null | grep -E "^\[|readyz"

echo -e "\n=== 5. etcd状态 ==="
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key 2>/dev/null || echo "需在master节点执行"

echo -e "\n=== 6. 待调度Pod ==="
kubectl get pods --all-namespaces --field-selector=status.phase=Pending 2>/dev/null | head -20

echo -e "\n=== 7. 证书有效期 ==="
kubeadm certs check-expiration 2>/dev/null || echo "需在master节点执行"

echo -e "\n=== 8. 资源使用 ==="
kubectl top nodes 2>/dev/null || echo "需安装metrics-server"
```

### 12.2 常用诊断命令

```bash
# 组件状态
kubectl get --raw='/readyz?verbose'
kubectl get componentstatuses  # 已弃用但可用

# 节点诊断
kubectl describe node <node-name>
kubectl describe node <node-name> | grep -A 5 "Allocated resources"

# Pod诊断
kubectl describe pod <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --tail=100 --previous

# etcd诊断
ETCDCTL_API=3 etcdctl member list -w table
ETCDCTL_API=3 etcdctl alarm list
ETCDCTL_API=3 etcdctl endpoint health --cluster

# 容器运行时
crictl ps -a
crictl logs <container-id>
crictl images

# 网络诊断
ipvsadm -Ln  # IPVS模式
iptables -t nat -L -n -v | grep KUBE  # iptables模式
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup kubernetes
```

---

## 13. 多角色视角专栏

### 13.1 架构师视角

| 关注点 | 核心问题 | 决策要点 | 推荐方案 |
|-------|---------|---------|---------|
| **高可用设计** | 控制平面如何HA? | etcd奇数节点、apiserver多副本+LB | 3 Master + 外部etcd集群 |
| **容量规划** | 多大规模需要什么配置? | 节点数、Pod数、对象数 | 参考10.1资源推荐表 |
| **扩展性** | 如何支持万级节点? | apiserver分片、etcd调优 | APF限流+IPVS+etcd NVMe |
| **多租户** | 如何隔离不同业务? | 命名空间、RBAC、网络策略 | Namespace+ResourceQuota+NetworkPolicy |
| **灾备设计** | RTO/RPO目标? | etcd备份频率、恢复时间 | 每小时备份,RTO<30min |

### 13.2 测试工程师视角

| 测试类型 | 测试目标 | 测试工具 | 关键指标 |
|---------|---------|---------|---------|
| **功能测试** | 组件功能正确性 | Sonobuoy, e2e测试 | 测试通过率100% |
| **性能测试** | API延迟、调度吞吐 | kube-burner, ClusterLoader2 | P99延迟<1s |
| **压力测试** | 极限负载下稳定性 | Litmus, ChaosMesh | 无数据丢失 |
| **故障注入** | 组件故障恢复能力 | ChaosBlade, PodChaos | 自动恢复<5min |
| **升级测试** | 版本升级兼容性 | kubeadm upgrade | 零停机升级 |

```bash
# 性能测试示例 - kube-burner
kube-burner init -c cluster-density.yaml --uuid $(uuidgen)

# 混沌测试示例 - ChaosMesh
kubectl apply -f - <<EOF
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: etcd-pod-kill
spec:
  action: pod-kill
  mode: one
  selector:
    namespaces:
    - kube-system
    labelSelectors:
      component: etcd
  scheduler:
    cron: "@every 10m"
EOF
```

### 13.3 产品经理视角

| 关注维度 | 关键指标 | 业务影响 | SLA承诺 |
|---------|---------|---------|---------|
| **可用性** | 控制平面可用率 | 集群管理能力 | 99.95% (月度) |
| **性能** | API响应延迟 | 部署速度 | P99 < 1s |
| **稳定性** | 故障恢复时间(MTTR) | 业务中断时长 | < 30min |
| **可观测性** | 监控覆盖率 | 问题定位速度 | 100%组件监控 |
| **合规性** | 审计日志完整性 | 安全合规要求 | 保留30天 |
| **成本** | 控制平面资源开销 | 基础设施成本 | 中集群~30核56G |

**SLA分级建议**:

| 级别 | 可用性 | 适用场景 | 架构要求 |
|------|--------|---------|---------|
| **Standard** | 99.5% | 开发测试 | 单Master |
| **Pro** | 99.9% | 生产业务 | 3 Master HA |
| **Enterprise** | 99.95% | 关键业务 | 3 Master + 外部etcd + 多AZ |

### 13.4 安全工程师视角

| 安全领域 | 检查项 | 加固措施 | 验证方法 |
|---------|--------|---------|---------|
| **认证** | 禁用匿名访问 | `--anonymous-auth=false` | 未认证请求返回401 |
| **授权** | RBAC最小权限 | 审计ClusterRoleBinding | `kubectl auth can-i` |
| **准入** | Pod安全标准 | 启用PodSecurity | 违规Pod创建失败 |
| **审计** | 审计日志启用 | 配置audit-policy | 检查/var/log/kubernetes/audit.log |
| **加密** | etcd加密 | `--encryption-provider-config` | Secret解码验证 |
| **网络** | API访问控制 | 网络策略限制6443 | 端口扫描验证 |

```yaml
# etcd加密配置示例
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
- resources:
  - secrets
  - configmaps
  providers:
  - aescbc:
      keys:
      - name: key1
        secret: <base64-encoded-32-byte-key>
  - identity: {}
```

---

## 相关文档引用

| 主题 | 文档编号 | 说明 |
|------|---------|------|
| etcd详解 | [35-etcd-deep-dive](./35-etcd-deep-dive.md) | Raft共识、MVCC、备份恢复 |
| API Server详解 | [36-kube-apiserver-deep-dive](./36-kube-apiserver-deep-dive.md) | 认证授权、APF限流 |
| Controller Manager详解 | [37-kube-controller-manager-deep-dive](./37-kube-controller-manager-deep-dive.md) | 40+控制器详解 |
| CCM详解 | [38-cloud-controller-manager-deep-dive](./38-cloud-controller-manager-deep-dive.md) | 云厂商集成 |
| Kubelet详解 | [39-kubelet-deep-dive](./39-kubelet-deep-dive.md) | Pod生命周期、PLEG |
| kube-proxy详解 | [40-kube-proxy-deep-dive](./40-kube-proxy-deep-dive.md) | iptables/IPVS/eBPF |
| Scheduler详解 | [164-kube-scheduler-deep-dive](./164-kube-scheduler-deep-dive.md) | 调度框架、插件 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01
