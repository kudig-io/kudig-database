---
title: 'Day 5: K8S 集群删除'
description: '## 概述'
summary: '## 概述'
category: learning
tags:
- k8s
- training
- hands-on
- statefulset
- daemonset
- job
- cronjob
- ingress
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 5: K8S 集群删除 是什么'
- '如何 Day 5: K8S 集群删除'
trigger_keywords:
- Day
- '5:'
- K8S
- 集群删除
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---



---
title: Day 5: K8S 集群删除
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK cluster deletion resource cleanup
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] cluster removal retain resources
  - SLB ENI security group cleanup
  - aliyun cs DELETE cluster API
  - Cluster deletion failure troubleshooting
trigger_keywords:
  - delete cluster
  - 集群删除
  - resource cleanup
  - 资源清理
  - retain resources
  - 保留资源
  - SLB
  - ENI
  - deletion failure
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-12-cloud-providers
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-lifecycle-management
  - cluster-creation
  - cluster-upgrade
---

# Day 5: K8S 集群删除

> **学习时间**: 4-5 小时 | **主题**: 理解集群删除流程与注意事项

---

## 概述

集群删除是集群生命周期管理的最后一个环节，也是最容易被忽视的环节。不当的删除操作可能导致数据丢失、资源残留、费用持续产生等问题。今天你将学习集群删除的完整流程、删除前的检查清单、保留资源与完全删除的区别，以及删除失败时的排查方法。

---

## 今日目标

- [ ] 掌握集群删除的完整流程和先决条件
- [ ] 理解删除集群时的资源清理逻辑
- [ ] 了解"保留资源"与"完全删除"的区别
- [ ] 掌握删除失败的排查方法

---

## 核心概念

### 1. 集群删除涉及的资源

| 资源类型 | 自动清理 | 保留选项 | 手动清理 |
|----------|---------|---------|---------|
| ECS 实例 (Worker 节点) | 是 | 否 | - |
| SLB (LoadBalancer) | 是 | 可选保留 | 检查是否有外部引用 |
| ENI (弹性网卡) | 是 | 否 | 可能残留需手动释放 |
| 安全组规则 | 是 | 否 | - |
| NAT 网关 | 视配置 | 可选保留 | - |
| EIP (弹性公网 IP) | 视配置 | 可选保留 | - |
| 云盘 (PV 数据) | 视 ReclaimPolicy | 否 | 检查 Retain 策略的 PV |
| DNS 记录 | 视配置 | - | 可能需要手动删除 |
| ACR 镜像 | 不涉及 | - | 不自动清理 |

### 2. 集群删除流程

```
阶段 1: 业务资源清理 (手动)
  ├── 删除业务 Deployment/StatefulSet/DaemonSet
  ├── 删除 LoadBalancer 类型 Service
  ├── 删除 PVC (注意数据备份)
  └── 删除 Ingress

阶段 2: 集群删除 (API/控制台)
  ├── ACK 删除管控面组件
  ├── ACK 释放 Worker 节点 ECS
  ├── ACK 释放 SLB/ENI/安全组
  └── ACK 清理路由和 NAT

阶段 3: 残留资源清理 (手动)
  ├── 检查并释放残留 SLB
  ├── 检查并释放残留 ENI
  ├── 检查并删除残留云盘
  └── 清理 VPC/vSwitch (可选)
```

### 3. 删除方式对比

| 删除方式 | 操作 | 适用场景 | 耗时 |
|----------|------|---------|------|
| 完全删除 | 释放所有关联资源 | 测试集群、下线集群 | 10-15 分钟 |
| 保留资源删除 | 保留指定资源 (SLB/NAT/EIP) | 集群迁移、网络复用 | 10-15 分钟 |
| 仅删除 K8s | 保留 ECS 实例 | 集群重建、节点复用 | 5-10 分钟 |

---

## 理论学习 (2h)

### 必读文档

1. **ACK 集群管理**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: 集群删除相关的注意事项

2. **K8S 集群架构**
   - 文件: `../../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md`
   - 重点: 理解删除集群涉及的组件和资源

---

## 实战演练 (2.5h)

### 任务 1: 删除前检查清单 (45min)

```bash
echo "========== 集群删除前检查 =========="
echo "集群: $(kubectl config current-context)"
echo "时间: $(date)"
echo ""

echo "--- 1. 检查业务工作负载 ---"
kubectl get deployments -A --no-headers | grep -v kube-system
kubectl get statefulsets -A --no-headers | grep -v kube-system
kubectl get daemonsets -A --no-headers | grep -v kube-system
echo ""

echo "--- 2. 检查 LoadBalancer 类型 Service ---"
kubectl get svc -A | grep LoadBalancer
echo ""
echo "详细列表:"
kubectl get svc -A -o jsonpath='{range .items[?(@.spec.type=="LoadBalancer")]}{.metadata.namespace}/{.metadata.name}: {.status.loadBalancer.ingress[0].ip}{"\n"}{end}'
echo ""

echo "--- 3. 检查 PVC 和 PV ---"
kubectl get pvc -A
echo ""
kubectl get pv
echo ""

echo "--- 4. 检查 Ingress ---"
kubectl get ingress -A
echo ""

echo "--- 5. 检查 CronJob ---"
kubectl get cronjobs -A --no-headers | grep -v kube-system
echo ""

echo "--- 6. 检查 Namespace 列表 ---"
kubectl get namespaces --no-headers | grep -v -E 'kube-system|kube-public|default|kube-node-lease'
echo ""

echo "--- 7. 统计资源数量 ---"
echo "Pods: $(kubectl get pods -A --no-headers | grep -v kube-system | wc -l)"
echo "Deployments: $(kubectl get deployments -A --no-headers | grep -v kube-system | wc -l)"
echo "Services: $(kubectl get svc -A --no-headers | grep -v kube-system | wc -l)"
echo "PVCs: $(kubectl get pvc -A --no-headers | wc -l)"
echo ""
echo "========== 检查完毕 =========="
```

---

### 任务 2: 业务资源清理 (45min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

```bash
# 列出所有业务 Namespace
kubectl get namespaces --no-headers | awk '{print $1}' | grep -v -E 'kube-system|kube-public|default|kube-node-lease'

# 逐个清理业务 Namespace (注意: 这是不可逆操作!)
# kubectl delete namespace <business-ns-1>  # ⚠️ 不可逆：永久删除命名空间及全部资源
# kubectl delete namespace <business-ns-2>  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 清理 default 命名空间中的资源
kubectl delete all --all -n default  # ⚠️ 批量删除，波及面大
kubectl delete pvc --all -n default  # ⚠️ 批量删除，波及面大
kubectl delete ingress --all -n default  # ⚠️ 批量删除，波及面大
kubectl delete configmap --all -n default  # ⚠️ 批量删除，波及面大
kubectl delete secret --all -n default  # ⚠️ 批量删除，波及面大

# 等待所有 Pod 终止
kubectl get pods -A | grep -v 'kube-system|Running|Completed'

# 确认 LoadBalancer 类型 Service 已清理
kubectl get svc -A | grep LoadBalancer
# 应该没有输出

# 确认 PVC 已清理
kubectl get pvc -A
# 应该没有输出

# 最终确认: 所有业务资源已清理
kubectl get all -A --no-headers | grep -v kube-system
# 应该没有输出
```

---

### 任务 3: 通过控制台删除集群 (30min)

ACK 控制台删除路径和选项:

```
登录阿里云控制台 → 容器服务 ACK → 集群列表
  → 选择目标集群 → 更多 → 删除集群

删除选项:
┌─────────────────────────────────────────────┐
│ 删除集群                                      │
│                                               │
│ 请输入集群名称确认: [training-cluster-01]       │
│                                               │
│ 删除方式:                                     │
│ ○ 删除集群并释放所有资源                       │
│   - 删除所有节点 ECS                          │
│   - 释放 SLB 实例                             │
│   - 释放 ENI 网卡                             │
│   - 清理安全组规则                             │
│                                               │
│ ○ 删除集群但保留部分资源                       │
│   ☑ 保留 SLB 实例                             │
│   ☑ 保留 NAT 网关                             │
│   ☑ 保留 EIP                                  │
│   ☐ 保留 ECS 实例 (仅移除 K8S 组件)           │
│                                               │
│ [确认删除]                                    │
└─────────────────────────────────────────────┘
```

注意事项:
- 删除操作不可逆
- 必须输入集群名称才能执行
- 删除过程约 5-15 分钟
- 删除后可查看日志确认

---

### 任务 4: 通过 API 删除集群 (30min)

```bash
# 方式 1: 完全删除 (释放所有资源)
aliyun cs DELETE /clusters/<cluster_id>
# 返回: {"cluster_id":"c-xxx","task_id":"t-xxx"}

# 方式 2: 删除集群但保留 SLB
aliyun cs DELETE /clusters/<cluster_id> \
  --body '{"retain_resources":["SLB"]}'

# 方式 3: 删除集群但保留 SLB 和 NAT
aliyun cs DELETE /clusters/<cluster_id> \
  --body '{"retain_resources":["SLB","NAT"]}'
# 注意: 不同 ACK 版本支持的 retain_resources 参数可能不同

# 查看删除进度
aliyun cs GET /clusters/<cluster_id>/logs | jq -r '.[] | "\(.created) \(.log)"' | tail -20
# 示例输出:
# 2026-05-18T10:00:00 开始删除集群
# 2026-05-18T10:01:00 正在删除节点池
# 2026-05-18T10:05:00 正在释放 ECS 实例
# 2026-05-18T10:08:00 正在释放 SLB
# 2026-05-18T10:10:00 正在清理安全组
# 2026-05-18T10:12:00 集群删除完成

# 验证删除完成
aliyun cs GET /clusters/<cluster_id>
# 应返回 404 或 state=deleted
```

---

### 任务 5: 删除失败排查 (30min)

```bash
echo "========== 删除失败排查 =========="

echo "--- 1. 检查残留 SLB ---"
aliyun slb DescribeLoadBalancers \
  --RegionId cn-hangzhou \
  --AddressType internet \
  | jq '.LoadBalancers.LoadBalancer[] | select(.VpcId=="<vpc_id>") | {LoadBalancerId, LoadBalancerName, Address}'

echo ""
echo "--- 2. 检查残留 ENI ---"
aliyun ecs DescribeNetworkInterfaces \
  --RegionId cn-hangzhou \
  | jq '.NetworkInterfaceSets.NetworkInterfaceSet[] | select(.VpcId=="<vpc_id>") | {NetworkInterfaceId, Status, Description}'

echo ""
echo "--- 3. 检查安全组引用 ---"
aliyun ecs DescribeSecurityGroupReferences \
  --RegionId cn-hangzhou \
  --SecurityGroupId.1 <sg_id> \
  | jq '.SecurityGroupReferenceSet.SecurityGroupReference[]'

echo ""
echo "--- 4. 检查残留云盘 ---"
aliyun ecs DescribeDisks \
  --RegionId cn-hangzhou \
  | jq '.Disks.Disk[] | select(.Status=="available") | {DiskId, DiskName, Size, Category}'

echo ""
echo "========== 排查完毕 =========="
```

常见删除失败原因与解决方案:

| 失败原因 | 症状 | 解决方法 |
|----------|------|---------|
| SLB 被其他服务引用 | SLB 释放失败 | 在 SLB 控制台手动释放 |
| ENI 无法释放 | 网卡残留 | 先解除安全组关联再释放 |
| 安全组被引用 | 安全组删除失败 | 先移除引用的安全组规则 |
| ECS 实例无法释放 | 节点残留 | 检查实例是否有保护策略 |
| 集群状态异常 | 删除超时 | 联系阿里云 oncall 强制清理 |

手动清理残留资源:

```bash
# 释放残留 SLB
aliyun slb DeleteLoadBalancer --LoadBalancerId <slb-id>

# 释放残留 ENI
aliyun ecs DeleteNetworkInterface --NetworkInterfaceId <eni-id>

# 释放残留云盘
aliyun ecs DeleteDisk --DiskId <disk-id>

# 重新尝试删除集群
aliyun cs DELETE /clusters/<cluster_id>

# 如仍失败，提交工单联系阿里云
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **删除 ACK 集群前，必须做哪些检查？为什么？**
   - 提示: 业务负载、SLB、存储、外部依赖

2. **"保留资源删除"和"完全删除"有什么区别？什么场景用哪种？**
   - 提示: 保留 SLB/NAT 适合迁移场景

3. **集群删除失败最常见的原因是什么？如何处理？**
   - 提示: 资源被引用无法释放

---

## 今日检验

- [ ] 能列出删除集群前的完整检查清单
- [ ] 能通过控制台和 API 两种方式删除集群
- [ ] 理解保留资源和完全删除的区别
- [ ] 能排查集群删除失败的常见原因

---

## 配置参考

### 删除前检查脚本

```bash
cat > pre-delete-check.sh << 'SCRIPT'
#!/bin/bash
CLUSTER_ID=${1:?"Usage: $0 <cluster_id>"}

echo "集群 $CLUSTER_ID 删除前检查"
echo "=============================="

# 检查 kubectl 资源
echo "[1/5] 检查业务工作负载..."
WORKLOAD_COUNT=$(kubectl get deployments,statefulsets,daemonsets -A --no-headers 2>/dev/null | grep -v kube-system | wc -l)
echo "  业务工作负载数: $WORKLOAD_COUNT"

echo "[2/5] 检查 LoadBalancer Service..."
LB_COUNT=$(kubectl get svc -A --no-headers 2>/dev/null | grep LoadBalancer | wc -l)
echo "  LoadBalancer 数: $LB_COUNT"

echo "[3/5] 检查 PVC..."
PVC_COUNT=$(kubectl get pvc -A --no-headers 2>/dev/null | wc -l)
echo "  PVC 数: $PVC_COUNT"

echo "[4/5] 检查 Ingress..."
ING_COUNT=$(kubectl get ingress -A --no-headers 2>/dev/null | wc -l)
echo "  Ingress 数: $ING_COUNT"

echo "[5/5] 检查集群信息..."
aliyun cs GET /clusters/$CLUSTER_ID | jq '{name, state, size, current_version}'

echo ""
echo "=============================="
if [ "$WORKLOAD_COUNT" -gt 0 ] || [ "$LB_COUNT" -gt 0 ] || [ "$PVC_COUNT" -gt 0 ]; then
  echo "[警告] 仍有业务资源未清理，请先清理后再删除"
else
  echo "[通过] 可以安全删除集群"
fi
SCRIPT

chmod +x pre-delete-check.sh
```

---

## 常见问题

### Q1: 删除集群后费用还在产生？

检查是否有残留的 SLB、EIP、NAT 网关等按量付费资源。在费用中心查看账单明细，按资源类型筛选。

### Q2: 如何只删除集群但保留 ECS 节点？

在删除选项中选择"保留 ECS 实例 (仅移除 K8S 组件)"。ECS 实例上的 K8s 组件会被卸载，但实例本身保留。

### Q3: 删除集群后 PV 数据还在吗？

取决于 PV 的 ReclaimPolicy:
- Delete: 数据随 PV 自动删除
- Retain: 数据保留在云盘中，需手动清理

---

## 要点总结

| 场景 | 操作 | 注意事项 |
|------|------|---------|
| 测试集群清理 | 完全删除 | 确认无业务数据 |
| 集群迁移 | 保留 SLB/NAT | 新集群复用网络资源 |
| 删除失败 | 检查残留资源 | SLB/ENI/安全组被引用 |
| 数据保护 | 先备份 PV 数据 | 删除后无法恢复 |

---

## 明日预告

Day 6 将学习集群升级策略，掌握版本升级的操作步骤和风险控制。

---

## 延伸阅读

- [ACK 集群管理](../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md)
- [K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [集群生命周期管理](../../domain-07-platform-engineering/02-cluster-lifecycle-management.md)
