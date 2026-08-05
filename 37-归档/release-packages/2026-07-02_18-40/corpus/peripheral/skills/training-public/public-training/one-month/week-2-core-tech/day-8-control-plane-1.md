---
title: 'Day 8: 控制平面 - etcd + API Server'
description: '- "认证授权准入控制怎么配"'
summary: '- "认证授权准入控制怎么配"'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- scheduler
- coredns
- rbac
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 8: 控制平面 - etcd + API Server 是什么'
- '如何 Day 8: 控制平面 - etcd + API Server'
trigger_keywords:
- Day
- '8:'
- 控制平面
- etcd
- API
- Server
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 8: 控制平面 - [[etcd|etcd]] + API Server

```yaml
---
id: LEARN-ONE-MONTH-W2-DAY8
title: Day 8 - 控制平面 - etcd + API Server
topic: [[entities/kubernetes.md|kubernetes]]
type: hands-on-guide
tags: [etcd, apiserver, control-plane, raft, authentication, authorization, admission, hands-on, week-2]
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - "etcd 集群怎么工作"
  - "Raft 协议是什么"
  - "API Server 请求处理链"
  - "认证授权准入控制怎么配"
  - "etcd 备份恢复怎么做"
trigger_keywords:
  - etcd
  - API Server
  - Control Plane
  - Raft
  - Leader
  - Follower
  - Authentication
  - Authorization
  - Admission Control
  - LimitRanger
  - ResourceQuota
  - etcdctl
  - snapshot
  - 备份恢复
reading_level: advanced
audience:
  - sre
  - ops-engineer
estimated_read_time: 50min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - control-plane
  - etcd
  - apiserver
  - authentication
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-2-core-tech/day-9-control-plane-2.md
  - domain-01-cluster-fundamentals/11-etcd-deep-dive.md
---
```

> **学习时间**: 4-5 小时 | **主题**: K8s 数据存储与 API 网关

---

## 概述

etcd 和 API Server 是 Kubernetes 控制平面中最核心的两个组件。etcd 是整个集群的"数据库"，存储了所有集群状态数据，其可靠性直接决定了集群的生死。API Server 是集群的"网关"，所有对集群的操作都通过 API Server 进行，它负责认证、授权、准入控制和审计等关键安全功能。

深入理解 etcd 和 API Server 的工作原理，对于排查控制平面问题、优化集群性能和保障集群安全至关重要。本课程将从架构原理到实际操作，全面深入这两个核心组件。

**学习目标**：
- 深入理解 etcd 的架构和 Raft 协议
- 掌握 API Server 的请求处理链
- 能够直接操作 etcd 查看 K8s 数据

**前置条件**：
- 已完成 Week 1 的 K8s 架构基础学习
- 了解分布式系统基本概念
- 有 kubectl 操作经验

---

## 核心概念

### etcd 架构与 Raft 协议

etcd 是一个高可用的分布式键值存储系统，使用 Raft 共识算法保证数据一致性。在 Kubernetes 中，etcd 是唯一的数据存储后端，所有集群状态（Pod、[[Service|Service]]、ConfigMap、Secret 等）都存储在 etcd 中。

#### Raft 协议核心概念

| 概念 | 说明 | 在 etcd 中的作用 |
|------|------|-----------------|
| **Leader** | 集群中的主节点 | 处理所有写请求，同步数据到 Follower |
| **Follower** | 集群中的从节点 | 接收 Leader 的日志复制，响应读请求 |
| **Candidate** | 选举中的候选者 | Leader 不可用时发起选举 |
| **Term** | 选举周期 | 每次选举递增，用于标识 Leader 的合法性 |
| **Log Entry** | 日志条目 | 记录所有的写操作 |
| **Commit** | 提交 | 多数节点确认后日志被视为已提交 |

#### Raft 工作流程

```
写请求流程:
Client → Leader → 1. 写入本地日志
                  → 2. 发送给所有 Follower
                  → 3. 等待多数节点确认 (N/2 + 1)
                  → 4. 提交日志
                  → 5. 响应 Client
                  → 6. 通知 Follower 已提交

选举流程:
1. Follower 超时未收到 Leader 心跳
2. 转为 Candidate，自增 Term
3. 向其他节点发起投票请求
4. 获得多数票后成为新 Leader
```

#### etcd 生产部署最佳实践

| 配置项 | 推荐值 | 原因 |
|--------|--------|------|
| 节点数量 | 奇数 (3 或 5) | 多数派共识要求奇数 |
| 磁盘类型 | NVMe SSD | etcd 对磁盘延迟敏感 |
| 磁盘优先级 | 高（使用 ionice） | 避免被其他 IO 影响 |
| 网络延迟 | < 10ms | Leader 和 Follower 之间 |
| 自动压缩 | 定期（每小时） | 防止数据无限增长 |
| 备份频率 | 每天至少一次 | 灾难恢复 |

### API Server 请求处理链

API Server 是 Kubernetes 的核心网关，所有对集群的访问都经过 API Server。请求处理链包含以下阶段：

```
请求进入 → 1. 认证 (Authentication)
         → 2. 授权 (Authorization)
         → 3. 准入控制 (Admission Control)
         → 4. 验证 (Validation)
         → 5. 持久化到 etcd
         → 6. 响应客户端
```

#### 请求处理各阶段详解

| 阶段 | 功能 | 支持的插件/模式 |
|------|------|---------------|
| **认证** | 验证"你是谁" | X509 证书、Bearer Token、OIDC、Webhook |
| **授权** | 验证"你能做什么" | RBAC、ABAC、Node、Webhook |
| **准入控制** | 修改/验证请求 | LimitRanger、ResourceQuota、PodSecurity、MutatingWebhook |
| **验证** | 检查资源定义合法性 | Schema 验证 |
| **持久化** | 写入 etcd | 通过 etcd v3 API |

---

## 实战演练

### 任务 1: etcd 操作实践 (1h)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 获取 etcd Pod 名称
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
echo "etcd Pod: $ETCD_POD"

# 预期输出:
# etcd Pod: etcd-master-xxx

# Step 2: 进入 etcd 容器
kubectl exec -it -n kube-system $ETCD_POD -- sh

# Step 3: 设置 etcdctl 环境变量
export ETCDCTL_API=3
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379

# Step 4: 查看集群成员
etcdctl member list

# 预期输出:
# 6e3bd23ae5f: started, master-1, https://10.0.0.1:2380, https://10.0.0.1:2379, false
# a8268ec63db: started, master-2, https://10.0.0.2:2380, https://10.0.0.2:2379, false
# c92bc8f33d1: started, master-3, https://10.0.0.3:2380, https://10.0.0.3:2379, false

# Step 5: 查看集群健康状态
etcdctl endpoint health

# 预期输出:
# https://10.0.0.1:2379 is healthy: successfully committed proposal: took = 2.3ms
# https://10.0.0.2:2379 is healthy: successfully committed proposal: took = 3.1ms
# https://10.0.0.3:2379 is healthy: successfully committed proposal: took = 2.8ms

# Step 6: 查看集群详细状态
etcdctl endpoint status --write-table

# 预期输出:
# +-------------------+------------------+---------+---------+-----------+------------+-----------+
# |     ENDPOINT      |        ID        | VERSION | DB SIZE | IS LEADER | IS LEARNER | RAFT TERM |
# +-------------------+------------------+---------+---------+-----------+------------+-----------+
# | https://10.0.0.1  | 6e3bd23ae5f...   |  3.5.11 |  120 MB |     true  |    false   |    12345  |
# | https://10.0.0.2  | a8268ec63db...   |  3.5.11 |  118 MB |    false  |    false   |    12345  |
# | https://10.0.0.3  | c92bc8f33d1...   |  3.5.11 |  119 MB |    false  |    false   |    12345  |
# +-------------------+------------------+---------+---------+-----------+------------+-----------+

# Step 7: 查看 K8s 数据 (所有 key)
etcdctl get / --prefix --keys-only | head -50

# 预期输出:
# /registry/apiregistration.k8s.io/apiservices/v1.
# /registry/clusterrolebindings/admin
# /registry/clusterroles/admin
# /registry/configmaps/default/kube-root-ca.crt
# /registry/deployments/default/nginx-web
# /registry/namespaces/default
# /registry/namespaces/kube-system
# /registry/pods/kube-system/coredns-xxx
# /registry/services/default/kubernetes

# Step 8: 查看特定资源类型的所有 key
etcdctl get /registry/deployments/default --prefix --keys-only

# Step 9: 查看某个 Pod 的完整数据（二进制格式）
etcdctl get /registry/pods/kube-system/<pod-name> --write-out=json

# Step 10: etcd 备份
etcdctl snapshot save /var/lib/etcd/snapshot-$(date +%Y%m%d).db

# 预期输出:
# {"level":"info","ts":"2026-05-18T10:30:00.123Z","msg":"saved","path":"/var/lib/etcd/snapshot-20260518.db","size":"120 MB"}

# Step 11: 验证备份
etcdctl snapshot status /var/lib/etcd/snapshot-20260518.db --write-table

# 预期输出:
# +----------+----------+------------+------------+
# | REVISION |   HASH   | COMPACTION | RAFT TERM  |
# +----------+----------+------------+------------+
# | 12345678 | abcdef12 |   12340000 |       567  |
# +----------+----------+------------+------------+
```
### 任务 2: API Server 请求追踪 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 使用 verbose 模式查看完整请求
kubectl get pods -v=8

# 预期输出:
# I0518 10:30:00.123456  123456 round_trippers.go:553] GET https://10.0.0.1:6443/api/v1/namespaces/default/pods?limit=500
# I0518 10:30:00.123789  123456 round_trippers.go:570] HTTP Response: Status 200 OK
# I0518 10:30:00.124000  123456 request.go:1188] Response Body: {"kind":"PodList","apiVersion":"v1","items":[...]}

# Step 2: 查看更详细的请求信息
kubectl get pods -v=9  # 包含 HTTP 请求/响应头

# Step 3: 查看 API Server 日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=100

# 预期输出 (部分):
# I0518 10:30:00.123456 1 trace.go:205] trace[123456]: "Create /api/v1/namespaces/default/pods" (started: ...)
# I0518 10:30:00.234567 1 trace.go:225] trace[123456]: "Create" response: (200)

# Step 4: 使用 curl 直接调用 API
# 方法A: 通过 kubectl proxy
kubectl proxy --port=8001 &
sleep 2

curl http://localhost:8001/api/v1/namespaces
curl http://localhost:8001/api/v1/namespaces/default/pods
curl http://localhost:8001/apis/apps/v1/deployments

# 预期输出 (namespaces):
# {
#   "kind": "NamespaceList",
#   "apiVersion": "v1",
#   "items": [
#     {"metadata": {"name": "default"}, ...},
#     {"metadata": {"name": "kube-system"}, ...}
#   ]
# }

# 方法B: 直接调用 API Server
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
TOKEN=$(kubectl create token default --duration=3600s)

curl -k -H "Authorization: Bearer $TOKEN" \
  $APISERVER/api/v1/namespaces/default/pods

# Step 5: 查看 API 资源列表
kubectl api-resources | head -30

# 预期输出:
# NAME                              SHORTNAMES   APIVERSION                             NAMESPACED   KIND
# pods                              po           v1                                     true         Pod
# services                          svc          v1                                     true         Service
# deployments                       deploy       apps/v1                                true         Deployment
# configmaps                        cm           v1                                     true         ConfigMap
# secrets                                        v1                                     true         Secret

kill %1
```
### 任务 3: 准入控制实验 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 查看启用的准入控制器
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep admission

# 预期输出:
# - --enable-admission-plugins=NodeRestriction,PodSecurity,LimitRanger,ServiceAccount,ResourceQuota,...

# Step 2: 创建 LimitRange（资源限制范围）
cat > limitrange.yaml << 'EOF'
apiVersion: v1
kind: LimitRange
metadata:
  name: default-limits
  namespace: default
spec:
  limits:
  - type: Container
    default:
      cpu: 200m
      memory: 256Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "2"
      memory: 2Gi
    min:
      cpu: 50m
      memory: 64Mi
  - type: Pod
    max:
      cpu: "4"
      memory: 4Gi
EOF

kubectl apply -f limitrange.yaml

# 预期输出:
# limitrange/default-limits created

# Step 3: 创建一个没有资源限制的 Pod，观察自动添加
kubectl run test-limit --image=nginx:1.25-alpine

# Step 4: 查看自动添加的资源限制
kubectl get pod test-limit -o yaml | grep -A 10 resources

# 预期输出:
# resources:
#   limits:
#     cpu: 200m
#     memory: 256Mi
#   requests:
#     cpu: 100m
#     memory: 128Mi
# LimitRange 自动为 Pod 添加了默认资源限制

# Step 5: 创建 ResourceQuota（命名空间资源配额）
cat > resourcequota.yaml << 'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: compute-quota
  namespace: default
spec:
  hard:
    requests.cpu: "4"
    requests.memory: 8Gi
    limits.cpu: "8"
    limits.memory: 16Gi
    pods: "20"
    services: "10"
    persistentvolumeclaims: "5"
EOF

kubectl apply -f resourcequota.yaml

# Step 6: 查看配额使用情况
kubectl describe resourcequota compute-quota -n default

# 预期输出:
# Name:            compute-quota
# Namespace:       default
# Resource         Used   Hard
# --------         ----   ----
# limits.cpu       600m   8
# limits.memory    768Mi  16Gi
# pods             3      20
# requests.cpu     300m   4
# requests.memory  384Mi  8Gi
# services         2      10

# Step 7: 清理
kubectl delete pod test-limit
kubectl delete limitrange default-limits
kubectl delete resourcequota compute-quota
```
---

## 配置参考

### etcd 性能参数

| 参数 | 说明 | 推荐值 | 默认值 |
|------|------|--------|--------|
| `--quota-backend-bytes` | 数据库大小限制 | 8Gi | 2Gi |
| `--auto-compaction-mode` | 压缩模式 | periodic | none |
| `--auto-compaction-retention` | 压缩保留 | 1h | 1h |
| `--snapshot-count` | 快照间隔 | 10000 | 100000 |
| `--heartbeat-interval` | 心跳间隔 | 100ms | 100ms |
| `--election-timeout` | 选举超时 | 1000ms | 1000ms |
| `--max-request-bytes` | 最大请求大小 | 10Mi | 1.5Mi |

### API Server 关键参数

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `--max-requests-inflight` | 最大并发请求数 | 800 |
| `--max-mutating-requests-inflight` | 最大并发写请求 | 400 |
| `--request-timeout` | 请求超时 | 60s |
| `--audit-log-maxage` | 审计日志保留天数 | 30 |
| `--audit-log-maxsize` | 审计日志文件大小 | 200Mi |
| `--enable-admission-plugins` | 启用的准入控制器 | PodSecurity,LimitRanger,... |

---

## 常见问题

### Q1: etcd 的 Raft 协议如何保证数据一致性？

**A**: Raft 协议通过以下机制保证一致性：
1. **Leader 选举**: 任何时刻只有一个 Leader 处理写请求
2. **日志复制**: Leader 将写操作复制到所有 Follower
3. **多数派确认**: 写操作必须被 N/2+1 个节点确认后才算成功
4. **安全性保证**: 已提交的日志永远不会被覆盖

### Q2: 为什么只有 API Server 能直接访问 etcd？

**A**: 
1. **安全**: API Server 统一处理认证、授权和审计
2. **一致性**: 通过 API Server 的乐观并发控制（ResourceVersion）避免冲突
3. **验证**: API Server 验证所有数据的合法性
4. **缓存**: API Server 内置缓存，减少 etcd 负载
5. 直接访问 etcd 绕过了所有安全控制，极其危险

### Q3: etcd 备份应该怎么做？

**A**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 定期备份命令
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- \
  etcdctl snapshot save /var/lib/etcd/backup-$(date +%Y%m%d-%H%M).db

# 建议:
# - 每天至少一次
# - 集群升级前必须备份
# - 测试备份的恢复能力
# - ACK 托管版阿里云自动备份
```
### Q4: API Server 过载怎么排查？

**A**:
1. 查看 API Server 日志中的延迟指标
2. 检查 `--max-requests-inflight` 是否需要调大
3. 使用 `kubectl get --raw "/metrics"` 查看 apiserver_request_count
4. 找出产生大量请求的客户端（通常是有问题的 Controller 或 Informer）
5. 检查是否有大范围的 list 操作（如 `kubectl get pods -A`）

### Q5: LimitRange 和 ResourceQuota 有什么区别？

**A**:
- **LimitRange**: 限制单个容器/Pod 的资源范围，设置默认值。是"微观"级别的限制
- **ResourceQuota**: 限制整个 Namespace 的资源总量。是"宏观"级别的限制
- 两者配合使用：LimitRange 设置默认值和上下限，ResourceQuota 设置总量限制

---

## 要点总结

- **etcd** 是 K8s 的唯一数据存储，使用 Raft 协议保证一致性，必须奇数节点部署
- **Raft 协议** 通过 Leader 选举 + 日志复制 + 多数派确认保证数据不丢失
- **API Server** 是集群网关，请求经过 认证 → 授权 → 准入控制 → etcd 持久化
- **etcd 备份** 是灾难恢复的关键，每天至少一次
- **LimitRange** 设置默认资源限制，**ResourceQuota** 设置命名空间总量限制
- etcd 对 **磁盘 IO** 极其敏感，必须使用 SSD 并隔离 IO

---

## 延伸阅读

- [etcd 官方文档](https://etcd.io/docs/)
- [Raft 论文](https://raft.github.io/raft.pdf)
- [Kubernetes API Server 文档](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-apiserver/)
- [文件: `../../domain-01-cluster-fundamentals/11-etcd-deep-dive.md`](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-01-cluster-fundamentals/03-control-plane/04-etcd-deep-dive.md)
- [文件: `../../domain-01-cluster-fundamentals/12-apiserver-deep-dive.md`](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-01-cluster-fundamentals/03-control-plane/05-apiserver-deep-dive.md)
- [文件: `../../domain-01-cluster-fundamentals/07-distributed-consensus-etcd.md`](08-distributed-consensus-etcd.md)

---

## 明日预告

Day 9 将学习 Scheduler 和 Controller Manager，理解 K8s 如何实现自动化管理。


<!-- risk-assessed -->
