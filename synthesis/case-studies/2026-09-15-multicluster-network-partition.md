---
title: "[2026-09-15] [P0] 多集群联邦网络分区导致服务漂移"
category: case-study
tags: [production, incident, multi-cluster, federation, networking, split-brain]
date: "2026-09-15"
severity: P0
mttr: "60min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
---

# [2026-09-15] 多集群联邦网络分区导致 DNS 服务漂移，订单重复扣款

## 工单信息
- **工单编号**: INC-2026-0915-020
- **发现时间**: 2026-09-15 12:10 UTC
- **恢复时间**: 2026-09-15 13:10 UTC
- **影响范围**: 联邦集群 `cluster-ap-east` 和 `cluster-ap-south`
- **业务影响**: 订单重复扣款 1,247 笔，支付对账差异 12 万元

## 问题现象
12:10，客服收到大量投诉："同一订单扣了两次款"。

排查发现：
- `cluster-ap-east` 和 `cluster-ap-south` 的 `payment-service` 均处理同一笔订单
- 两集群间的网络连接在 12:05 中断，但各自认为对方已下线
- 联邦 DNS 在两集群中均返回本地服务地址

## 诊断过程

**12:15** — 检查联邦集群状态：
```bash
kubectl --context=fed get clusters
# NAME            AGE   READY
# cluster-ap-east  45d   True
# cluster-ap-south 45d   True

kubectl --context=fed get kubefedclusters
# NAME             READY   AGE
# cluster-ap-east  True    45d
# cluster-ap-south True    45d
```

联邦控制器显示两集群均 Ready，但实际网络已分区。

**12:18** — 检查集群间网络连通性：
```bash
# 从 cluster-ap-east 测试到 cluster-ap-south 的 API Server
kubectl --context=cluster-ap-east run -it --rm debug --image=nicolaka/netshoot \
  --restart=Never -- ping 10.0.100.15
# ping: sendto: No route to host

# 检查跨集群 VPC Peering
aws ec2 describe-vpc-peering-connections --filters Name=status-code,Values=active
# （无 active peering，都被 deleted）
```

**12:20** — 查看变更历史：
```bash
# 09-14 22:00，网络团队执行了 "清理过期 VPC Peering" 的自动化任务
# 误将生产环境 cluster-ap-east <-> cluster-ap-south 的 Peering 标记为过期并删除
```

**12:22** — 检查联邦 DNS 行为：
```bash
# cluster-ap-east 的 CoreDNS
kubectl --context=cluster-ap-east get svc payment-service -n prod-payment
# NAME             TYPE        CLUSTER-IP     EXTERNAL-IP
# payment-service  ClusterIP   10.96.10.10    <none>

# cluster-ap-south 的 CoreDNS
kubectl --context=cluster-ap-south get svc payment-service -n prod-payment
# NAME             TYPE        CLUSTER-IP     EXTERNAL-IP
# payment-service  ClusterIP   10.97.10.10    <none>
```

两集群各自有独立的 `payment-service` EndpointSlice，联邦 DNS 因无法同步，两集群均认为本地服务是"主"服务。

**12:25** — 检查订单数据一致性：
```bash
# cluster-ap-east 数据库
SELECT order_id, COUNT(*) FROM payments WHERE created_at > '2026-09-15 12:00:00' GROUP BY order_id HAVING COUNT(*) > 1;
# +----------+----------+
# | order_id | COUNT(*) |
# +----------+----------+
# | ORD-001  | 2        |
# | ORD-002  | 2        |
# | ...      | ...      |
# +----------+----------+
# 1247 rows
```

## 根因
1. 网络团队误删 VPC Peering，导致 `cluster-ap-east` 和 `cluster-ap-south` 网络分区
2. KubeFed 控制器因网络分区无法同步状态，但各自认为对方仍在线（基于缓存的 Ready 状态）
3. 联邦 DNS（MultiClusterServiceDNSRecord）在网络分区时，两集群均返回本地服务地址
4. 负载均衡器（Global Load Balancer）未检测到集群间问题，继续向两集群分发请求
5. 同一笔订单被两集群的 `payment-service` 同时处理，导致重复扣款

## 修复动作

**12:30** — 恢复 VPC Peering：
```bash
aws ec2 create-vpc-peering-connection \
  --vpc-id vpc-ap-east \
  --peer-vpc-id vpc-ap-south \
  --peer-region ap-south-1

aws ec2 accept-vpc-peering-connection \
  --vpc-peering-connection-id pcx-abc123

# 更新路由表
aws ec2 create-route \
  --route-table-id rtb-ap-east \
  --destination-cidr-block 10.97.0.0/16 \
  --vpc-peering-connection-id pcx-abc123
```

**12:40** — 验证集群间连通性：
```bash
kubectl --context=cluster-ap-east run -it --rm debug --image=nicolaka/netshoot \
  --restart=Never -- ping 10.0.100.15
# 64 bytes from 10.0.100.15: icmp_seq=1 ttl=64 time=12.3 ms
```

**12:42** — 联邦状态同步恢复：
```bash
kubectl --context=fed get kubefedclusters
# NAME             READY   AGE
# cluster-ap-east  True    45d
# cluster-ap-south True    45d

# 检查 DNS 同步
kubectl --context=fed get multiclusterservicednsrecords -n prod-payment
# NAME              AGE
# payment-service   2m
```

**12:45** — 处理重复扣款：
```bash
# 标记重复交易为退款
curl -X POST http://payment-reconcile.prod/api/refund-duplicates \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -d '{"from":"2026-09-15T12:00:00Z","to":"2026-09-15T13:10:00Z"}'
# {"refunded": 1247, "amount": 120000}
```

**12:55** — 更新 Global Load Balancer 健康检查：
```bash
# 配置跨集群健康检查，当集群间网络不可达时，将流量切换至单一集群
# AWS Global Accelerator 配置更新
globalaccelerator update-endpoint-group \
  --endpoint-group-arn arn:aws:globalaccelerator::123456789:endpoint-group/xxx \
  --health-check-protocol TCP \
  --health-check-port 443 \
  --threshold-count 3
```

## 验证
- 13:00 — 两集群间网络连通性恢复
- 13:05 — 新订单无重复扣款
- 13:10 — 全部退款处理完成，业务恢复正常

## 复盘
- **直接原因**: VPC Peering 误删 → 网络分区 → 联邦 DNS 分裂 → 两集群同时处理同一订单 → 重复扣款
- **根本原因**: 
  1. 网络清理脚本未区分生产和测试环境
  2. 联邦架构缺少网络分区时的脑裂保护
- **改进措施**:
  1. **网络分区检测**: 在联邦控制器中部署分区检测机制（如 witness 节点），网络分区时自动暂停非主集群的写操作
  2. **VPC Peering 保护**: 生产环境的 Peering 添加 `DoNotDelete` tag，清理脚本跳过带该 tag 的资源
  3. **幂等性**: 所有支付接口强制要求 `idempotency-key`，重复请求返回同一结果
  4. Global Load Balancer 配置跨集群健康检查，分区时只路由至主集群
  5. 每月执行多集群问题演练，模拟网络分区场景
- **相关 Skill**: [[ts-cluster-operations]]
- **相关 FTA**: [[ts-networking]]
