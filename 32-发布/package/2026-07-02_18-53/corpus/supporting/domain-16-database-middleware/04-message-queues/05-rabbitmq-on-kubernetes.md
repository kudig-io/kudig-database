---
title: RabbitMQ on Kubernetes 生产指南
description: 在阿里云专有云 ACK 集群中使用 RabbitMQ Cluster Operator 部署高可用 RabbitMQ：集群组建、镜像队列、Quorum
  Queue、持久化、监控告警与故障恢复。
summary: 在阿里云专有云 ACK 集群中使用 RabbitMQ Cluster Operator 部署高可用 RabbitMQ：集群组建、镜像队列、Quorum
  Queue、持久化、监控告警与故障恢复。
category: database-middleware
tags:
- rabbitmq
- message-queue
- operator
- quorum-queue
- mirrored-queue
- ha
- kubernetes
- alibaba-cloud
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06-29
difficulty: advanced
reading_level: advanced
audience:
- 中间件工程师
- SRE
- 专有云运维
estimated_read_time: 17min
intent_queries:
- RabbitMQ Kubernetes Operator 部署
- RabbitMQ 镜像队列与 Quorum Queue
- RabbitMQ on K8s 高可用配置
trigger_keywords:
- rabbitmq
- rabbitmq-cluster-operator
- quorum queue
- mirrored queue
- federation
- shovel
prerequisites:
- domain-16-database-middleware/03-message-queues/03-message-queue-comparison.md
- domain-16-database-middleware/03-message-queues/04-rocketmq-on-kubernetes.md
- domain-05-security-compliance/01-identity-access/README.md
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# RabbitMQ on Kubernetes 生产指南

> 适用场景：在阿里云专有云 ACK 集群中自托管 RabbitMQ，满足企业级消息队列需求：AMQP 协议兼容、灵活路由、高可用队列与细粒度权限控制。

## 目录

- [1. 架构与核心概念](#1-架构与核心概念)
- [2. 安装 RabbitMQ Cluster Operator](#2-安装-rabbitmq-cluster-operator)
- [3. 部署 RabbitMQ 集群](#3-部署-rabbitmq-集群)
- [4. 高可用：镜像队列与 Quorum Queue](#4-高可用-镜像队列与-quorum-queue)
- [5. 持久化与存储](#5-持久化与存储)
- [6. 用户、权限与 TLS](#6-用户-权限与-tls)
- [7. 监控与告警](#7-监控与告警)
- [8. 常见问题排查](#8-常见问题排查)
- [9. 生产检查清单](#9-生产检查清单)
- [10. 典型工单诊断决策树](#10-典型工单诊断决策树)
- [11. 相关文档](#11-相关文档)
## 1. 架构与核心概念

RabbitMQ 集群由多个 Erlang 节点组成，共享同一个 Erlang cookie 以实现状态同步。核心对象包括：

- **Exchange**：消息路由交换机，支持 direct、topic、fanout、headers 等类型；
- **Queue**：消息队列，可声明为 classic、quorum 或 stream；
- **Binding**：Exchange 与 Queue 之间的路由规则；
- **Virtual Host**：逻辑隔离单元，常用于多租户；
- **Policy**：运行时策略，用于配置镜像队列、TTL、死信等。

在 Kubernetes 中，RabbitMQ Cluster Operator 负责管理集群生命周期，包括节点编排、配置更新、插件启用与证书轮转。

## 2. 安装 RabbitMQ Cluster Operator

RabbitMQ 官方提供 Kubernetes Operator，推荐在生产环境使用，避免手写 StatefulSet 的复杂配置。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 RabbitMQ Cluster Operator 到 rabbitmq-system 命名空间
kubectl apply -f https://github.com/rabbitmq/cluster-operator/releases/download/v2.9.0/cluster-operator.yml
kubectl get deployment rabbitmq-cluster-operator -n rabbitmq-system
```
安装完成后，可在任意命名空间创建 `RabbitmqCluster` 自定义资源。

## 3. 部署 RabbitMQ 集群

### 3.1 最小生产配置

```yaml
# rabbitmq-cluster.yaml
apiVersion: rabbitmq.com/v1beta1
kind: RabbitmqCluster
metadata:
  name: prod-rabbit
  namespace: middleware
spec:
  replicas: 3
  resources:
    requests:
      cpu: 1
      memory: 2Gi
    limits:
      cpu: 2
      memory: 4Gi
  persistence:
    storageClassName: alicloud-disk-ssd
    storage: 50Gi
  rabbitmq:
    additionalConfig: |
      vm_memory_high_watermark.relative = 0.7
      disk_free_limit.relative = 1.5
      cluster_partition_handling = autoheal
```

### 3.2 创建并验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 RabbitMQ 集群并等待就绪
kubectl apply -f rabbitmq-cluster.yaml
kubectl wait --for=condition=AllReplicasReady rabbitmqcluster/prod-rabbit -n middleware --timeout=300s
kubectl get rabbitmqcluster prod-rabbit -n middleware
```
Operator 会自动创建 StatefulSet、Service、Secret（用户名/密码）、ConfigMap 等资源。获取管理控制台凭据：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取默认用户名与密码
kubectl get secret prod-rabbit-default-user -n middleware -o jsonpath='{.data.username}' | base64 -d
echo ""
kubectl get secret prod-rabbit-default-user -n middleware -o jsonpath='{.data.password}' | base64 -d
echo ""
```
## 4. 高可用：镜像队列与 Quorum Queue

### 4.1 镜像队列（Mirrored Queues）

镜像队列是 RabbitMQ 3.x 的高可用方案，通过 policy 将队列主副本同步到多个节点。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建镜像队列策略，所有队列在集群内 3 个节点同步
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl set_policy ha-all "^" '
  {"ha-mode":"all", "ha-sync-mode":"automatic"}' --priority 0 --apply-to queues
```
镜像队列在 RabbitMQ 3.13+ 中已被标记为 deprecated，新集群应优先使用 Quorum Queue。

### 4.2 Quorum Queue（推荐）

Quorum Queue 基于 Raft 共识算法，提供更强的一致性与自动 Leader 选举，适合生产环境。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 声明 Quorum Queue 需要客户端在 queue 参数中指定 x-queue-type
# 也可通过 policy 强制所有匹配队列使用 quorum
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl set_policy quorum-all "^" '
  {"queue-type":"quorum"}' --priority 0 --apply-to queues
```
Quorum Queue 不支持某些经典队列特性（如优先级队列、独占队列），迁移前需评估业务兼容性。

## 5. 持久化与存储

RabbitMQ 将消息、元数据、日志持久化到 PVC 挂载目录 `/var/lib/rabbitmq`。生产环境建议：

- 使用 SSD 类型 StorageClass；
- 为每个副本分配独立 PVC，避免多节点共享存储；
- 监控磁盘使用率，防止触发 `disk_free_limit` 导致生产者阻塞。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 RabbitMQ PVC 使用情况
kubectl get pvc -n middleware -l app.kubernetes.io/name=prod-rabbit
kubectl exec -it prod-rabbit-server-0 -n middleware -- df -h /var/lib/rabbitmq
```
当需要扩容磁盘时，先确认 StorageClass 支持 `allowVolumeExpansion: true`，然后修改 `RabbitmqCluster` 的 `persistence.storage` 字段，Operator 会滚动更新节点。

## 6. 用户、权限与 TLS

### 6.1 创建业务用户

避免使用默认用户，应为每个应用创建独立用户并限制 vhost 与权限。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建业务用户并设置 vhost 权限
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl add_user order-service Passw0rd
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl set_user_tags order-service monitoring
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl set_permissions -p /order order-service "^order-.*" "^order-.*" "^order-.*"
```
### 6.2 启用 TLS

RabbitMQ Operator 支持通过 `tls` 字段自动挂载 Secret，启用 AMQPS 与管理界面 HTTPS。

```yaml
spec:
  tls:
    secretName: prod-rabbit-tls
    disableNonSSLListeners: false
```

证书可通过 cert-manager 自动签发，并配置在 `prod-rabbit-tls` Secret 中。

## 7. 监控与告警

RabbitMQ 内置 Prometheus metrics，可通过 `rabbitmq_prometheus` 插件暴露。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 启用 Prometheus 插件（Operator 默认已启用）
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmq-plugins enable rabbitmq_prometheus
```
创建 ServiceMonitor（假设已部署 Prometheus Operator）：

```yaml
# rabbitmq-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: prod-rabbit
  namespace: middleware
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: prod-rabbit
  endpoints:
    - port: prometheus
      path: /metrics
      interval: 15s
```

### 7.1 关键告警指标

| 指标 | 含义 | 告警阈值建议 |
| --- | --- | --- |
| `rabbitmq_queues` | 队列数量 | 突增或持续增长 |
| `rabbitmq_queue_messages_ready` | 待消费消息数 | > 10000 |
| `rabbitmq_queue_messages_unacked` | 未确认消息数 | > 5000 且持续增长 |
| `rabbitmq_disk_free_alarm` | 磁盘告警 | = 1 |
| `rabbitmq_memory_alarm` | 内存告警 | = 1 |
| `rabbitmq_node_partitions` | 网络分区次数 | > 0 |

## 8. 常见问题排查

### 8.1 集群节点无法组成集群

常见原因是 Erlang cookie 不一致或节点间 DNS 解析失败。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查各节点 Erlang cookie 是否一致
kubectl exec -it prod-rabbit-server-0 -n middleware -- cat /var/lib/rabbitmq/.erlang.cookie
kubectl exec -it prod-rabbit-server-1 -n middleware -- cat /var/lib/rabbitmq/.erlang.cookie
```
> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查节点间 DNS 解析与 25672 端口连通性
kubectl exec -it prod-rabbit-server-0 -n middleware -- nslookup prod-rabbit-server-1.prod-rabbit-nodes.middleware.svc.cluster.local
```
### 8.2 内存或磁盘告警触发

当 RabbitMQ 触发 `memory_alarm` 或 `disk_free_alarm` 时，会阻塞生产者。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前告警状态与资源使用
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl status
kubectl top pod -l app.kubernetes.io/name=prod-rabbit -n middleware
```
处理措施：扩容 Pod 资源、扩容 PVC、增加队列 TTL/死信策略、增加消费者。

### 8.3 Quorum Queue 无 Leader

当多数副本不可用时，Quorum Queue 无法选举 Leader，队列不可用。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Quorum Queue 状态与成员分布
kubectl exec -it prod-rabbit-server-0 -n middleware -- rabbitmqctl quorum_status "order.quorum"
```
## 9. 生产检查清单

- [ ] 使用 RabbitMQ Cluster Operator 部署，避免手写 StatefulSet；
- [ ] 副本数 >= 3，跨可用区分布；
- [ ] 新集群优先使用 Quorum Queue，旧集群有迁移计划；
- [ ] 每个副本使用独立 SSD PVC；
- [ ] 默认用户已禁用或限制，业务用户按 vhost 授权；
- [ ] TLS 已启用，管理界面不暴露到公网；
- [ ] Prometheus 插件与 ServiceMonitor 已配置；
- [ ] 内存、磁盘、队列积压、网络分区已配置告警。

## 10. 典型工单诊断决策树

RabbitMQ 在 Kubernetes 中的常见工单包括连接数告警、队列堆积、节点分裂与镜像队列未同步。

### 连接数告警

1. `rabbitmqctl list_connections` 查看当前连接与来源 IP。
2. 检查应用是否未正确关闭连接或存在连接泄漏。
3. 调整 `ulimit` 与 `file_descriptors` 限制。
4. 必要时扩容 RabbitMQ 节点。

### 队列堆积

1. 查看 `rabbitmq_queue_messages_ready` 与 `rabbitmq_queue_messages_unacked`。
2. 检查消费者 Pod 是否存活、资源是否充足。
3. 确认队列类型为 quorum 或镜像队列，避免单点故障。
4. 评估是否需要增加消费者或调整 prefetch count。

### 节点分裂（Partition）

1. `rabbitmqctl cluster_status` 查看节点是否分裂。
2. 检查网络策略与安全组是否误拦截节点间通信。
3. 按文档进行网络分区恢复，必要时重启节点重新加入集群。

### 镜像队列未同步

1. 使用 `rabbitmqctl list_queues name slave_pids synchronised_slave_pids`。
2. 检查未同步节点是否因网络或负载问题落后。
3. 必要时手动同步或迁移队列。

## 11. 相关文档

- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-16-database-middleware/04-message-queues/03-message-queue-comparison|消息队列选型对比]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-16-database-middleware/04-message-queues/04-rocketmq-on-kubernetes|RocketMQ on Kubernetes]]
- 身份认证与授权系统

```

<!-- risk-assessed -->
