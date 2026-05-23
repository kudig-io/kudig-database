---
title: "[2026-05-28] [P2] DaemonSet 节点亲和性导致部分节点未部署"
category: case-study
tags: [production, incident, workloads, daemonset, node-affinity, observability]
date: "2026-05-28"
severity: P2
mttr: "30min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
---

# [2026-05-28] Fluent Bit DaemonSet 未在新节点部署导致日志丢失

## 工单信息
- **工单编号**: INC-2026-0528-012
- **发现时间**: 2026-05-28 02:00 UTC
- **恢复时间**: 2026-05-28 02:30 UTC
- **影响范围**: 3 个新扩容节点，约 45 个 Pod 的日志未收集
- **业务影响**: 02:00-02:30 期间日志丢失，影响问题排查和安全审计

## 问题现象
02:00，值班工程师在处理一起业务告警时，发现 Kibana 中搜索不到部分 Pod 的日志。进一步排查发现，新扩容的 3 个节点上的所有 Pod 均无日志输出到 ES。

```bash
# 新节点
kubectl get nodes
# NAME                         STATUS   ROLES    AGE
# ip-10-0-8-10.ec2.internal    Ready    <none>   2h
# ip-10-0-8-11.ec2.internal    Ready    <none>   2h
# ip-10-0-8-12.ec2.internal    Ready    <none>   2h

# 检查 Fluent Bit Pod
kubectl get pods -n logging -l app=fluent-bit -o wide
# NAME               READY   STATUS    NODE
# fluent-bit-abc12   1/1     Running   ip-10-0-8-10.ec2.internal
# fluent-bit-def34   0/1     Pending   <none>
# ...
```

## 诊断过程

**02:05** — 查看 Pending 的 Fluent Bit Pod：
```bash
kubectl describe pod fluent-bit-def34 -n logging
# Events:
#   Warning  FailedScheduling  5m  ...  
#     0/20 nodes are available: 
#     3 node(s) didn't match Pod's node affinity/selector.
```

**02:07** — 检查 DaemonSet 的 nodeAffinity：
```bash
kubectl get daemonset fluent-bit -n logging -o yaml | grep -A20 nodeAffinity
# nodeAffinity:
#   requiredDuringSchedulingIgnoredDuringExecution:
#     nodeSelectorTerms:
#     - matchExpressions:
#       - key: node-type
#         operator: In
#         values:
#         - worker
#         - monitor
```

**02:09** — 检查新节点的 label：
```bash
kubectl get node ip-10-0-8-10.ec2.internal --show-labels
# NAME                        STATUS   LABELS
# ip-10-0-8-10.ec2.internal   Ready    beta.kubernetes.io/arch=amd64,...
#                                       kubernetes.io/os=linux,...
# （缺少 node-type=worker label）
```

**02:11** — 检查节点扩容流程：
```bash
# Cluster Autoscaler 扩容的节点使用新的 Launch Template (v3.2)
# v3.2 移除了节点初始化脚本中对 node-type label 的设置
# 旧节点使用 v3.1，有 node-type=worker
```

## 根因
1. 05-27 22:00，运维团队更新了 EC2 Launch Template 到 v3.2
2. v3.2 的初始化脚本（user-data）删除了 `kubectl label node $(hostname) node-type=worker` 步骤
3. 05-28 00:00，业务流量激增，Cluster Autoscaler 触发扩容，使用 v3.2 创建新节点
4. 新节点缺少 `node-type=worker` label
5. Fluent Bit DaemonSet 的 `nodeAffinity` 要求 `node-type` 为 `worker` 或 `monitor`
6. 新节点不满足亲和性要求，Fluent Bit Pod 无法调度，日志收集缺失

## 修复动作

**02:15** — 临时为新节点添加 label：
```bash
for node in ip-10-0-8-10.ec2.internal ip-10-0-8-11.ec2.internal ip-10-0-8-12.ec2.internal; do
  kubectl label node $node node-type=worker --overwrite
done
```

**02:18** — 验证 DaemonSet Pod 启动：
```bash
kubectl get pods -n logging -l app=fluent-bit -o wide
# NAME               READY   STATUS    NODE
# fluent-bit-abc12   1/1     Running   ip-10-0-8-10.ec2.internal
# fluent-bit-def34   1/1     Running   ip-10-0-8-11.ec2.internal
# fluent-bit-ghi56   1/1     Running   ip-10-0-8-12.ec2.internal
```

**02:20** — 验证日志收集：
```bash
kubectl exec -n logging fluent-bit-def34 -- ls /var/log/containers | head
# order-api-xxx_prod-order_order-api-xxx.log
# payment-api-xxx_prod-payment_payment-api-xxx.log

# Kibana 查询确认日志已恢复
```

**02:25** — 修复 Launch Template v3.2：
```bash
# 在 user-data 中添加节点 label 初始化
cat >> /etc/kubernetes/node-labels.yaml <<EOF
node-type: worker
EOF

# 或在 kubelet 启动参数中添加
# --node-labels=node-type=worker
```

## 验证
- 02:28 — 所有节点的 Fluent Bit Pod 均 Running
- 02:30 — Kibana 中可搜索到新节点上所有 Pod 的日志

## 复盘
- **直接原因**: Launch Template v3.2 删除了节点 label 初始化 → 新节点缺少 node-type → DaemonSet nodeAffinity 不匹配 → Fluent Bit 未部署 → 日志丢失
- **根本原因**: 节点初始化脚本的变更未经过节点 label 依赖检查
- **改进措施**:
  1. 节点初始化脚本变更后，运行 DaemonSet 覆盖度检查：`kubectl get ds -A -o json | jq '.items[].status.desiredNumberScheduled == .items[].status.currentNumberScheduled'`
  2. DaemonSet 使用更通用的亲和性规则，不依赖可能变化的自定义 label
  3. 为节点 label 设置统一配置中心，禁止在 user-data 中硬编码 label
  4. 添加告警：`fluent_bit_desired_pods != fluent_bit_current_pods`
- **相关 Skill**: [[ts-node-components]]
- **相关 FTA**: [[daemonset-fta]]
