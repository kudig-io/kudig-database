---
title: "多活架构设计"
description: "K8s 多活架构：同城双活、异地多活、数据同步策略、流量调度、冲突解决与多集群多活实践"
summary: "系统化的多活架构设计指南，覆盖同城双活与异地多活的架构模式、数据同步与一致性策略、基于 DNS/GSLB/Service Mesh 的流量调度、写冲突解决方案以及 Kubernetes 多集群多活的实现路径"
category: 可靠性
tags:
- multi-active
- dual-active
- geo-distributed
- data-sync
- traffic-scheduling
- conflict-resolution
- multi-cluster
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 多集群多活架构如何设计"
- "异地多活的数据同步和冲突解决策略"
- "同城双活和异地多活如何选择"
trigger_keywords:
- 多活
- 双活
- 异地多活
- 数据同步
- 流量调度
- 多集群
prerequisites:
- kubectl-basics
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

# 多活架构设计

## 概述

多活架构（Multi-Active Architecture）是高可用设计的最高形态——多个数据中心/区域同时承载生产流量，任一节点故障时流量自动切换到其他存活节点，用户无感知。与主备架构（Active-Standby）中备用节点长期空闲不同，多活架构中所有节点都在处理真实流量，资源利用率更高，故障切换更快。

本文覆盖从同城双活到异地多活的完整架构设计，包括数据同步策略、流量调度机制、写冲突解决方案以及 Kubernetes 多集群多活的实现路径。多活架构是 [[12-可靠性/06-SRE实践/09-multi-active-architecture.md|灾难恢复]] 能力的终极保障，也是 [[12-可靠性/06-SRE实践/01-availability-calculation-model.md|高可用性]] 的核心支撑。

## 核心概念

### 多活架构演进路径

```
┌─────────────────────────────────────────────────────────────────┐
│                  多活架构演进                                      │
│                                                                   │
│  Level 1: 主备 (Active-Standby)                                  │
│  ┌──────────┐    复制    ┌──────────┐                            │
│  │  主 (RW)  │──────────▶│  备 (RO)  │  RPO>0, RTO=分钟级        │
│  └──────────┘            └──────────┘                            │
│                                                                   │
│  Level 2: 同城双活 (Dual-Active, Same City)                      │
│  ┌──────────┐  同步复制  ┌──────────┐                            │
│  │ DC-A (RW) │◀────────▶│ DC-B (RW) │  RPO=0, RTO=秒级          │
│  └──────────┘  <2ms RTT └──────────┘                            │
│                                                                   │
│  Level 3: 异地多活 (Geo-Distributed Multi-Active)                │
│  ┌──────────┐            ┌──────────┐            ┌──────────┐   │
│  │Region-A  │◀──异步──▶│Region-B  │◀──异步──▶│Region-C  │   │
│  │(RW)      │  复制     │(RW)      │  复制     │(RW)      │   │
│  └──────────┘  >30ms    └──────────┘           └──────────┘   │
│  RPO>0 (秒级), RTO=秒级, 需要冲突解决                            │
│                                                                   │
│  Level 4: 单元化多活 (Unitized Multi-Active)                     │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  按用户/租户分片，每个单元自闭环                            │   │
│  │  Unit-A (用户 0-33%) | Unit-B (34-66%) | Unit-C (67-100%)│   │
│  │  单元内完整服务栈，跨单元仅异步同步                         │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### 多活架构模式对比

| 维度 | 同城双活 | 异地多活 | 单元化多活 |
|------|---------|---------|-----------|
| 网络延迟 | <2ms | 30-100ms | 30-100ms（单元内 <2ms） |
| 数据一致性 | 强一致（同步复制） | 最终一致（异步复制） | 单元内强一致，跨单元最终一致 |
| RPO | 0 | 秒级 | 秒级（跨单元） |
| RTO | 秒级 | 秒级 | 秒级 |
| 写冲突 | 无（同步锁） | 需要解决 | 单元内无，跨单元需解决 |
| 实现复杂度 | 中 | 高 | 极高 |
| 适用场景 | 金融核心、强一致要求 | 互联网应用、可容忍短暂不一致 | 超大规模、全球化部署 |
| 成本 | 2x | 3x+ | 3x+（但资源利用率高） |

### 数据同步策略

| 同步模式 | 一致性 | 延迟影响 | 适用数据库 | 适用场景 |
|---------|--------|---------|-----------|---------|
| 同步复制 | 强一致 | 写入延迟 += RTT | MySQL Group Replication, PostgreSQL 同步流复制 | 同城双活 |
| 半同步复制 | 准强一致 | 写入延迟 += 1 RTT | MySQL 半同步, TiDB Raft | 同城/近距异地 |
| 异步复制 | 最终一致 | 无额外延迟 | MySQL 异步, MongoDB Replica Set | 异地多活 |
| CRDT | 最终一致（无冲突） | 无额外延迟 | Redis CRDT, CockroachDB | 计数器、集合类数据 |
| 应用层双写 | 取决于实现 | 写入延迟 += 远程调用 | 任意 | 特定业务场景 |

## 生产部署/实现

### 同城双活：K8s 多集群 + 同步数据层

```yaml
# 🟡 中风险：多集群配置影响流量分发
# 集群 A 的 Service 配置（暴露为多集群服务）
apiVersion: networking.istio.io/v1beta1
kind: ServiceEntry
metadata:
  name: payment-service-remote
  namespace: production
spec:
  hosts:
  - payment-service.production.svc.clusterset.local
  location: MESH_INTERNAL
  ports:
  - number: 8080
    name: http
    protocol: HTTP
  resolution: DNS
  endpoints:
  # 本地集群端点
  - address: payment-service.production.svc.cluster.local
    network: cluster-a
    locality: zone-a
    weight: 50
  # 远程集群端点（通过 East-West Gateway）
  - address: payment-service.eastwest-gateway.cluster-b.svc.clusterset.local
    network: cluster-b
    locality: zone-b
    weight: 50
---
# 多集群流量策略：优先本地，故障时切换
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: payment-service-multi-cluster
  namespace: production
spec:
  host: payment-service.production.svc.clusterset.local
  trafficPolicy:
    connectionPool:
      http:
        http2MaxRequests: 1000
    outlierDetection:
      consecutive5xxErrors: 3
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 100
    loadBalancer:
      localityLbSetting:
        enabled: true
        failover:
        - from: zone-a
          to: zone-b
        - from: zone-b
          to: zone-a
        distribute:
        - from: zone-a
          to:
            zone-a: 80
            zone-b: 20
        - from: zone-b
          to:
            zone-b: 80
            zone-a: 20
```

### 异地多活：流量调度层

```yaml
# 🟡 中风险：DNS/GSLB 配置影响全局流量分发
# 使用 ExternalDNS + GeoDNS 实现基于地理位置的流量调度
apiVersion: externaldns.k8s.io/v1alpha1
kind: DNSEndpoint
metadata:
  name: api-geo-routing
  namespace: production
spec:
  endpoints:
  - dnsName: api.company.com
    recordType: A
    targets:
    - 10.1.0.100  # Region-A LB
    recordTTL: 60
    providerSpecific:
    - name: external-dns.alpha.kubernetes.io/geo-code
      value: "CN-EAST"
  - dnsName: api.company.com
    recordType: A
    targets:
    - 10.2.0.100  # Region-B LB
    recordTTL: 60
    providerSpecific:
    - name: external-dns.alpha.kubernetes.io/geo-code
      value: "CN-SOUTH"
---
# 健康检查驱动的自动故障切换
apiVersion: v1
kind: ConfigMap
metadata:
  name: gslb-health-check-config
  namespace: traffic-management
data:
  config.yaml: |
    health_checks:
    - name: region-a-check
      endpoint: https://api-east.company.com/healthz
      interval: 10s
      timeout: 5s
      unhealthy_threshold: 3
      healthy_threshold: 2
      expected_status: 200
    - name: region-b-check
      endpoint: https://api-south.company.com/healthz
      interval: 10s
      timeout: 5s
      unhealthy_threshold: 3
      healthy_threshold: 2
      expected_status: 200

    failover_policy:
      # 当 Region-A 不健康时，流量全部切到 Region-B
      - trigger: region-a-check == unhealthy
        action:
          type: weight_adjust
          target: api.company.com
          weights:
            region-a: 0
            region-b: 100
        notification:
          channel: "#traffic-ops"
          message: "Region-A unhealthy, traffic shifted to Region-B"

      # Region-A 恢复后，渐进式回切流量
      - trigger: region-a-check == healthy AND previous_state == failover
        action:
          type: gradual_restore
          target: api.company.com
          steps:
          - weight: {region-a: 10, region-b: 90}
            duration: 5m
          - weight: {region-a: 30, region-b: 70}
            duration: 10m
          - weight: {region-a: 50, region-b: 50}
            duration: 10m
          - weight: {region-a: 50, region-b: 50}  # 恢复正常比例
```

### 数据同步与冲突解决

```yaml
# 🔴 高风险：数据同步配置错误可能导致数据丢失或不一致
# MySQL Group Replication 多主模式配置（同城双活）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-gr
  namespace: database
spec:
  serviceName: mysql-gr
  replicas: 3
  selector:
    matchLabels:
      app: mysql-gr
  template:
    metadata:
      labels:
        app: mysql-gr
    spec:
      containers:
      - name: mysql
        image: mysql/mysql-server:8.0.36
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-root-secret
              key: password
        - name: GROUP_REPLICATION_MODE
          value: "MULTI_PRIMARY"  # 多主模式
        - name: GROUP_REPLICATION_GROUP_NAME
          value: "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"
        - name: GROUP_REPLICATION_LOCAL_ADDRESS
          valueFrom:
            fieldRef:
              fieldPath: status.podIP
        - name: GROUP_REPLICATION_GROUP_SEEDS
          value: "mysql-gr-0.mysql-gr.database.svc:33061,mysql-gr-1.mysql-gr.database.svc:33061,mysql-gr-2.mysql-gr.database.svc:33061"
        # 冲突检测配置
        - name: GROUP_REPLICATION_CONFLICT_THRESHOLD
          value: "1000000"
        - name: GROUP_REPLICATION_MEMBER_WEIGHT
          value: "50"
        ports:
        - containerPort: 3306
          name: mysql
        - containerPort: 33061
          name: gr-comm
        resources:
          requests:
            cpu: "2"
            memory: 4Gi
          limits:
            cpu: "4"
            memory: 8Gi
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        - name: config
          mountPath: /etc/mysql/conf.d
      volumes:
      - name: config
        configMap:
          name: mysql-gr-config
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
      storageClassName: ssd-encrypted
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-gr-config
  namespace: database
data:
  group-replication.cnf: |
    [mysqld]
    # Group Replication 多主模式
    plugin_load_add='group_replication.so'
    group_replication_group_name="aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"
    group_replication_start_on_boot=ON
    group_replication_single_primary_mode=OFF
    group_replication_enforce_update_everywhere_checks=ON

    # 冲突检测与解决
    group_replication_member_weight=50
    group_replication_autorejoin_tries=3

    # Binlog 配置（用于跨地域异步复制）
    server_id=${SERVER_ID}
    log_bin=mysql-bin
    binlog_format=ROW
    binlog_row_image=FULL
    sync_binlog=1

    # 性能优化
    innodb_flush_log_at_trx_commit=1
    innodb_buffer_pool_size=4G
    max_connections=500
```

### 单元化架构：流量路由规则

```yaml
# 🟡 中风险：单元化路由配置影响请求分发
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: unitized-routing
  namespace: production
spec:
  hosts:
  - api.company.com
  http:
  # 根据用户 ID 哈希路由到对应单元
  - name: unit-routing
    match:
    - headers:
        x-user-id:
          regex: ".*"
    route:
    # Unit-A: 用户 ID 哈希 % 3 == 0
    - destination:
        host: api-gateway.unit-a.svc.cluster.local
        port:
          number: 8080
      weight: 33
      headers:
        request:
          set:
            x-unit: unit-a
    # Unit-B: 用户 ID 哈希 % 3 == 1
    - destination:
        host: api-gateway.unit-b.svc.cluster.local
        port:
          number: 8080
      weight: 33
      headers:
        request:
          set:
            x-unit: unit-b
    # Unit-C: 用户 ID 哈希 % 3 == 2
    - destination:
        host: api-gateway.unit-c.svc.cluster.local
        port:
          number: 8080
      weight: 34
      headers:
        request:
          set:
            x-unit: unit-c
```

## 运维操作

### 多集群状态检查

```bash
# 🟢 低风险：只读检查
# 检查多集群连接状态（Istio Multi-Cluster）
istioctl remote-clusters

# 检查各集群服务健康状态
for cluster in cluster-a cluster-b; do
  echo "=== $cluster ==="
  kubectl --context=$cluster get pods -n production -l app=payment-service --no-headers | \
    awk '{print $1, $2, $3}'
done

# 检查跨集群服务发现
istioctl proxy-config endpoints payment-service-xxx.production --cluster cluster-a | \
  grep "clusterset"

# 检查数据同步延迟
mysql -h mysql-gr-0.mysql-gr.database.svc -u monitor -p$MONITOR_PWD -e "
  SELECT member_host, member_state, member_role,
    COUNT_TRANSACTIONS_IN_QUEUE AS pending_txns
  FROM performance_schema.replication_group_members;
"
```

### 流量切换操作

```bash
# 🔴 高风险：流量切换影响所有用户
# 紧急切换：将 Region-A 流量全部切到 Region-B
# 通过修改 DNS 权重或 Istio 路由规则
kubectl patch virtualservice api-routing -n traffic-management \
  --type='json' \
  -p='[
    {"op":"replace","path":"/spec/http/0/route/0/weight","value":0},
    {"op":"replace","path":"/spec/http/0/route/1/weight","value":100}
  ]'

# 验证流量切换
watch -n 5 'curl -s http://prometheus.monitoring.svc:9090/api/v1/query \
  --data-urlencode="query=sum(rate(istio_requests_total[1m])) by (destination_cluster)" | \
  jq ".data.result"'

# 渐进式回切（Region-A 恢复后）
# Step 1: 10% 流量回切
kubectl patch virtualservice api-routing -n traffic-management \
  --type='json' \
  -p='[
    {"op":"replace","path":"/spec/http/0/route/0/weight","value":10},
    {"op":"replace","path":"/spec/http/0/route/1/weight","value":90}
  ]'
# 观察 5 分钟，确认无异常后继续增加比例...
```

### 数据一致性校验

```bash
# 🟢 低风险：只读校验
# 跨集群数据一致性校验（定期执行）
# 对比两个集群的关键表行数
for cluster in cluster-a cluster-b; do
  echo "=== $cluster ==="
  kubectl --context=$cluster exec -n database statefulset/mysql-gr-0 -- \
    mysql -u monitor -p$MONITOR_PWD -N -e "
      SELECT 'orders' AS tbl, COUNT(*) AS cnt FROM orders
      UNION ALL
      SELECT 'users', COUNT(*) FROM users
      UNION ALL
      SELECT 'payments', COUNT(*) FROM payments;
    "
done

# 检查 Group Replication 延迟
mysql -h mysql-gr-0.mysql-gr.database.svc -u monitor -p$MONITOR_PWD -e "
  SELECT channel_name, last_error_number, last_error_message,
    received_transaction_set, applied_transaction_set
  FROM performance_schema.replication_connection_status;
"
```

## 故障排查

### 脑裂（Split-Brain）检测与处理

```bash
# 🔴 高风险：脑裂处理可能导致数据丢失
# 检测 Group Replication 是否发生脑裂
mysql -h mysql-gr-0.mysql-gr.database.svc -u root -p$ROOT_PWD -e "
  SELECT member_id, member_host, member_state, member_role
  FROM performance_schema.replication_group_members;
"
# 如果看到多个 PRIMARY 或成员状态不一致，说明发生了脑裂

# 查看 Group Replication 错误日志
kubectl logs -n database statefulset/mysql-gr-0 --tail=100 | grep -i "group_replication\|conflict\|expel"

# 紧急处理：将少数派节点设为只读
mysql -h mysql-gr-minority-node -u root -p$ROOT_PWD -e "
  SET GLOBAL super_read_only = ON;
  STOP GROUP_REPLICATION;
"
```

### 跨地域同步延迟

```bash
# 🟢 低风险：只读诊断
# 检查异步复制延迟
mysql -h mysql-replica-region-b -u monitor -p$MONITOR_PWD -e "
  SHOW SLAVE STATUS\G
" | grep -E "Seconds_Behind_Master|Slave_IO_Running|Slave_SQL_Running|Retrieved_Gtid_Set|Executed_Gtid_Set"

# 检查网络延迟
kubectl exec -n production deployment/latency-probe -- \
  ping -c 10 region-b-endpoint.internal

# 检查同步队列积压
mysql -h mysql-primary-region-a -u monitor -p$MONITOR_PWD -e "
  SELECT COUNT(*) AS pending_events
  FROM performance_schema.events_transactions_current
  WHERE state = 'ACTIVE';
"
```

### 流量调度异常

```bash
# 🟢 低风险：只读诊断
# 检查 DNS 解析是否正确
dig api.company.com +short
nslookup api.company.com

# 检查各 Region 的实际流量分布
curl -s 'http://prometheus.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(istio_requests_total{destination_service_name="api-gateway"}[5m])) by (destination_cluster)' | \
  jq '.data.result'

# 检查健康检查是否误判
kubectl logs -n traffic-management deployment/gslb-controller --tail=50 | grep -i "health\|failover"
```

## 最佳实践

### 多活架构设计原则

1. **数据分片优先于数据同步**：能通过分片避免跨地域写入的，优先选择分片（单元化）。

2. **读写分离降低冲突**：写操作路由到数据归属单元，读操作可以就近读取（接受短暂不一致）。

3. **幂等性是多活的前提**：所有写操作必须幂等，确保重试和冲突解决不产生副作用。

4. **渐进式流量切换**：故障恢复后不要一次性回切所有流量，按 10% → 30% → 50% → 100% 渐进恢复。

5. **定期演练**：每季度执行一次多活切换演练，验证 RTO/RPO 达标。参考 [[12-可靠性/02-灾难恢复/index|02-灾难恢复]] 演练流程。

### 冲突解决策略

| 策略 | 适用场景 | 实现方式 | 缺点 |
|------|---------|---------|------|
| Last Write Wins (LWW) | 非关键数据、可容忍覆盖 | 时间戳比较 | 时钟偏差导致误判 |
| 应用层合并 | 协作编辑、购物车 | CRDT / OT 算法 | 实现复杂 |
| 人工仲裁 | 金融交易、库存扣减 | 告警 + 人工处理 | 延迟高 |
| 避免冲突（分片） | 用户数据、订单数据 | 按用户 ID 路由到固定单元 | 跨单元查询复杂 |

### 与现有体系集成

多活架构需要与以下体系深度集成：
- [[09-可观测性/02-指标/16-multi-cluster-monitoring-governance.md|多集群监控治理]]：统一监控所有活跃集群
- [[12-可靠性/06-SRE实践/03-incident-command-system.md|事件指挥系统]]：多活切换作为标准事件流程
- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops.md|Argo CD 多集群]]：多集群统一部署
- [[12-可靠性/06-SRE实践/01-availability-calculation-model.md|可用性计算]]：多活对可用性指标的影响

## Related

- [[12-可靠性/06-SRE实践/01-availability-calculation-model.md|可用性计算模型]]
- [[12-可靠性/02-灾难恢复/index|02-灾难恢复]]
- [[12-可靠性/01-备份恢复/index|01-备份恢复]]
- [[09-可观测性/02-指标/16-multi-cluster-monitoring-governance.md|多集群监控治理]]
- [[11-发布变更/01-GitOps/01-argo-cd-enterprise-gitops.md|Argo CD 企业级 GitOps]]
- [[12-可靠性/06-SRE实践/08-resilience-patterns-circuit-breaker.md|弹性模式]]
- [[12-可靠性/06-SRE实践/03-incident-command-system.md|事件指挥系统]]
