---
title: 'Day 6: K8S 集群升级'
description: '## 概述'
summary: '集群升级是生产环境中最关键也最危险的运维操作之一。K8s 社区每年发布 3 个小版本，每个版本的维护周期约 14 个月。为了获得安全补丁和新特性，生产集群需要定期升级。今天你将学习 ACK 集群升级的两个阶段、升级前的兼容性检查方法、替换升级的完整操作流程，以及升级后验证和回滚策略。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- daemonset
- rbac
- operator
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
- 'Day 6: K8S 集群升级 是什么'
- '如何 Day 6: K8S 集群升级'
trigger_keywords:
- Day
- '6:'
- K8S
- 集群升级
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 6: K8S 集群升级
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK cluster upgrade strategy in-place replacement
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] version upgrade path compatibility
  - kubent API deprecation check upgrade
  - Cluster upgrade verification rollback
  - ACK managed cluster upgrade process
trigger_keywords:
  - cluster upgrade
  - version upgrade
  - upgrade path
  - kubent
  - replacement upgrade
  - in-place upgrade
  - API deprecation
  - control plane
  - node upgrade
reading_level: intermediate
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-07-platform-engineering
  - domain-12-cloud-providers
  - domain-10-troubleshooting-diagnostics
related_topics:
  - cluster-lifecycle-management
  - upgrade-paths-strategy
  - upgrade-migration-strategy
  - cluster-certificate
---

# Day 6: K8S 集群升级

> **学习时间**: 4-5 小时 | **主题**: 掌握集群版本升级策略与操作步骤

---

## 概述

集群升级是生产环境中最关键也最危险的运维操作之一。K8s 社区每年发布 3 个小版本，每个版本的维护周期约 14 个月。为了获得安全补丁和新特性，生产集群需要定期升级。今天你将学习 ACK 集群升级的两个阶段、升级前的兼容性检查方法、替换升级的完整操作流程，以及升级后验证和回滚策略。

---

## 今日目标

- [ ] 理解 ACK 集群升级的两个阶段 (管控面 + 节点)
- [ ] 掌握升级前的兼容性检查方法
- [ ] 能通过控制台和 API 完成集群升级
- [ ] 了解升级回滚策略和风险控制

---

## 核心概念

### 1. K8s 版本策略

| 版本类型 | 格式 | 示例 | 升级规则 |
|----------|------|------|---------|
| 大版本 | v1.x | v1.28 | 不跨大版本升级 |
| 小版本 | v1.x.y | v1.28.9 | 逐小版本升级 |
| 补丁版本 | v1.x.y-z | v1.28.9-aliyun.1 | 可跳跃升级 |

ACK 版本升级路径:

```
1.26.x → 1.27.x → 1.28.x → 1.29.x → 1.30.x → 1.31.x → 1.32.x → 1.33.x
  ↑                                                                 ↑
  不支持跨版本: 不能从 1.26 直接升级到 1.28                           当前最新
  必须逐版本升级: 1.26 → 1.27 → 1.28
```

### 2. 升级阶段

| 阶段 | 操作方 | 影响范围 | 耗时 |
|------|--------|---------|------|
| 管控面升级 | 阿里云 (托管版) | API Server 短暂不可用 | 5-10 分钟 |
| 节点升级 | 用户操作 | 节点逐个重启 | 取决于节点数 |

### 3. 节点升级方式对比

| 方式 | 流程 | 优点 | 缺点 | 推荐度 |
|------|------|------|------|--------|
| 原地升级 | 在原节点上升级 [[kubelet|kubelet]] | 操作简单 | 风险高，节点不可回退 | 仅测试环境 |
| 替换升级 | 新建节点 → 迁移 Pod → 删除旧节点 | 风险可控 | 需要额外资源 | 生产推荐 |

---

## 理论学习 (2h)

### 必读文档

1. **K8S 版本升级策略**
   - 文件: `../../../domain-01-cluster-fundamentals/07-upgrade-paths-strategy.md`
   - 重点: 版本兼容性、升级路径规划

2. **升级与迁移策略**
   - 文件: `../../../domain-01-cluster-fundamentals/18-upgrade-migration-strategy.md`
   - 重点: 升级风险评估、回滚方案

3. **ACK 集群管理**
   - 文件: `../../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: ACK 特有的升级流程和注意事项

---

## 实战演练 (2.5h)

### 任务 1: 升级前检查 (45min)

#### 1.1 版本检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前集群版本
kubectl version --short
# Client Version: v1.28.9
# Server Version: v1.28.9-aliyun.1

# 查看各节点版本
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'
# NAME           VERSION
# node-01        v1.28.9-aliyun.1
# node-02        v1.28.9-aliyun.1
# node-03        v1.28.9-aliyun.1

# 查看 API 版本信息
kubectl api-versions | sort
```
#### 1.2 查看可升级目标版本

```bash
aliyun cs GET /upgrade/cluster/<cluster_id> | jq '.'
# {
#   "current_version": "1.28.9-aliyun.1",
#   "next_versions": [
#     "1.28.13-aliyun.1",
#     "1.29.8-aliyun.1"
#   ],
#   "can_upgrade": true
# }
```

#### 1.3 API 废弃检查 (关键步骤)

```bash
# 安装 kubent (Kube No Trouble)
# macOS: brew install kubent
# Linux: curl -L https://github.com/doitintl/kube-no-trouble/releases/latest/download/kubent_linux_amd64.tar.gz | tar xz

kubent
# 示例输出:
# ---
# >> Deprecated APIs removed in 1.29 <<
# KIND         NAME             NAMESPACE
# FlowSchema   eks-privileged   kube-system
# ---
# >> Deprecated APIs removed in 1.30 <<
# (none found)
```

#### 1.4 组件兼容性检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查核心组件状态
kubectl get pods -n kube-system -o wide
# 所有 Pod 应为 Running 状态

# 检查组件升级状态
aliyun cs GET /clusters/<cluster_id>/components/upgradestatus | jq '.[].name'

# 检查 webhook 配置
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations
# 记录所有 webhook，升级后验证其正常工作
```
#### 1.5 备份关键资源

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
mkdir -p /tmp/cluster-backup

kubectl get all -A -o yaml > /tmp/cluster-backup/all-resources.yaml
kubectl get configmaps -A -o yaml > /tmp/cluster-backup/configmaps.yaml
kubectl get secrets -A -o yaml > /tmp/cluster-backup/secrets.yaml
kubectl get pvc -A -o yaml > /tmp/cluster-backup/pvc.yaml
kubectl get networkpolicies -A -o yaml > /tmp/cluster-backup/networkpolicies.yaml
kubectl get roles,rolebindings,clusterroles,clusterrolebindings -A -o yaml > /tmp/cluster-backup/rbac.yaml

echo "备份完成: /tmp/cluster-backup/"
ls -la /tmp/cluster-backup/
```
---

### 任务 2: 管控面升级 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 触发管控面升级 (托管版)
aliyun cs POST /api/v2/clusters/<cluster_id>/upgrade \
  --body '{
    "next_version": "1.29.8-aliyun.1"
  }'

# 返回:
# {"task_id":"t-xxxxxxxxx"}

# 持续查看升级进度
watch -n 10 "aliyun cs GET /clusters/<cluster_id>/logs | jq -r '.[-5:] | .[] | \"\(.created) \(.log)\"'"

# 示例日志:
# 2026-05-18T10:00:00 开始升级管控面
# 2026-05-18T10:02:00 升级 API Server
# 2026-05-18T10:04:00 升级 Controller Manager
# 2026-05-18T10:06:00 升级 Scheduler
# 2026-05-18T10:08:00 升级 etcd
# 2026-05-18T10:10:00 管控面升级完成

# 验证管控面升级完成
kubectl version --short
# Server Version: v1.29.8-aliyun.1  ← 已更新

# 检查管控组件状态
kubectl get pods -n kube-system | grep -v Running
# 应该没有非 Running 的 Pod
```
---

### 任务 3: 节点升级 - 替换升级方式 (45min)

#### 3.1 替换升级完整流程

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
CLUSTER_ID="<cluster_id>"
NODEPOOL_ID="<nodepool_id>"

echo "=== Step 1: 查看当前节点 ==="
kubectl get nodes -o wide

echo "=== Step 2: 扩容新节点 ==="
aliyun cs POST /clusters/$CLUSTER_ID/nodepools/$NODEPOOL_ID \
  --body '{"scaling_group":{"desired_size":4}}'

echo "=== Step 3: 等待新节点 Ready ==="
kubectl get nodes -w
# 等待新节点状态变为 Ready

echo "=== Step 4: 确认所有节点 Ready ==="
kubectl get nodes
# NAME           STATUS   VERSION
# old-node-01    Ready    v1.28.9-aliyun.1  ← 旧版本
# old-node-02    Ready    v1.28.9-aliyun.1  ← 旧版本
# old-node-03    Ready    v1.28.9-aliyun.1  ← 旧版本
# new-node-04    Ready    v1.29.8-aliyun.1  ← 新版本
```
#### 3.2 逐个迁移旧节点

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
OLD_NODE="old-node-01"

echo "=== Step 5: Cordon 旧节点 $OLD_NODE ==="
kubectl cordon $OLD_NODE
# node/old-node-01 cordoned

echo "=== Step 6: Drain 旧节点 $OLD_NODE ==="
kubectl drain $OLD_NODE \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --grace-period=60 \
  --timeout=300s

# 示例输出:
# node/old-node-01 already cordoned
# evicting pod default/nginx-xxx-xxx
# evicting pod default/api-xxx-xxx
# pod/nginx-xxx-xxx evicted
# pod/api-xxx-xxx evicted
# node/old-node-01 drained

echo "=== Step 7: 确认 Pod 已迁移 ==="
kubectl get pods -A -o wide | grep $OLD_NODE
# 应该没有 Pod 在旧节点上 (除 DaemonSet)

echo "=== Step 8: 移除旧节点 ==="
# 获取节点实例 ID
NODE_ID=$(aliyun cs GET /clusters/$CLUSTER_ID/nodes | jq -r ".nodes[] | select(.node_name==\"$OLD_NODE\") | .instance_id")

aliyun cs POST /clusters/$CLUSTER_ID/nodes \
  --body "{\"nodes\":[\"$NODE_ID\"],\"release_node\":true}"

echo "=== Step 9: 验证节点已移除 ==="
kubectl get nodes -o wide
```
#### 3.3 重复迁移其余旧节点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 对每个旧节点重复 Step 5-9
# 建议每次只迁移一个节点，确认业务正常后继续

# 最终验证: 所有节点版本一致
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'
# NAME           VERSION
# new-node-04    v1.29.8-aliyun.1
# new-node-05    v1.29.8-aliyun.1
# new-node-06    v1.29.8-aliyun.1
```
---

### 任务 4: 升级后验证 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "========== 升级后验证 =========="

echo "--- 1. 验证集群版本 ---"
kubectl version --short

echo "--- 2. 验证所有节点版本一致 ---"
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'

echo "--- 3. 验证核心组件状态 ---"
kubectl get pods -n kube-system | grep -v Running
kubectl get cs 2>/dev/null || echo "cs 命令在 1.29+ 可能已弃用"

echo "--- 4. 验证业务 Pod 状态 ---"
kubectl get pods -A | grep -v 'Running|Completed'

echo "--- 5. 验证 Service 可用性 ---"
kubectl get svc -A | grep LoadBalancer
# 测试 SLB 可达性
# curl -s http://<slb-ip>/healthz

echo "--- 6. 验证存储 ---"
kubectl get pvc -A
kubectl get pv

echo "--- 7. 验证 API 资源 ---"
kubectl api-resources | head -20

echo "--- 8. 验证 webhook ---"
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations

echo "--- 9. 检查事件 ---"
kubectl get events -A --sort-by='.lastTimestamp' | tail -20

echo "========== 验证完毕 =========="
```
---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK 集群升级分哪两个阶段？各自的操作方式是什么？**
   - 提示: 管控面 (托管版自动) + 节点 (原地/替换)

2. **为什么推荐替换升级而不是原地升级？**
   - 提示: 风险可控、可回滚、不影响业务

3. **升级前使用 kubent 工具检查什么？为什么重要？**
   - 提示: 检查已废弃的 API 版本，避免升级后资源无法管理

4. **升级过程中如何确保业务零中断？**
   - 提示: 多副本 + readinessProbe + maxUnavailable=0

---

## 今日检验

- [ ] 能说出 ACK 集群升级的两个阶段
- [ ] 能进行升级前的兼容性检查
- [ ] 能通过替换方式完成节点升级
- [ ] 能完成升级后的全面验证

---

## 配置参考

### 升级检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
cat > pre-upgrade-check.sh << 'SCRIPT'
#!/bin/bash
echo "========== 升级前检查 =========="

echo "[1] 当前版本:"
kubectl version --short 2>/dev/null || kubectl version

echo ""
echo "[2] 节点版本一致性:"
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'

echo ""
echo "[3] 核心组件状态:"
kubectl get pods -n kube-system | grep -v Running | grep -v Completed

echo ""
echo "[4] Webhook 配置:"
kubectl get validatingwebhookconfigurations --no-headers
kubectl get mutatingwebhookconfigurations --no-headers

echo ""
echo "[5] 业务 Pod 状态:"
kubectl get pods -A | grep -v Running | grep -v Completed | grep -v kube-system

echo ""
echo "[6] 资源使用:"
kubectl top nodes 2>/dev/null || echo "metrics-server 未安装"

echo ""
echo "========== 检查完毕 =========="
SCRIPT

chmod +x pre-upgrade-check.sh
```
---

## 常见问题

### Q1: 管控面升级失败怎么办？

管控面升级由阿里云管理，极少失败。如果失败，集群会回滚到升级前状态。查看日志: `aliyun cs GET /clusters/<cluster_id>/logs`

### Q2: 节点升级后 Pod 无法启动？

检查: 1) API 版本是否兼容; 2) 镜像是否可拉取; 3) 节点标签和污点是否正确; 4) 存储卷是否正常挂载。

### Q3: 升级过程中如何回滚？

- 管控面: 托管版不支持回滚，需联系阿里云支持
- 节点: 替换升级方式下，旧节点尚未删除前可 cordon 新节点、uncordon 旧节点恢复

---

## 要点总结

| 升级方式 | 优点 | 缺点 | 适用场景 |
|----------|------|------|---------|
| 管控面升级 | 托管版自动，无需操作 | 不可回滚 | 所有托管版集群 |
| 原地升级 | 操作简单 | 风险高，影响业务 | 测试环境 |
| 替换升级 | 风险可控，可回滚 | 需要额外资源 | 生产环境推荐 |

---

## 明日预告

Day 7 将学习集群证书管理，理解证书类型、过期处理和轮换机制。

---

## 延伸阅读

- [K8s 版本升级策略](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-01-cluster-fundamentals/05-upgrade-paths/02-upgrade-paths-strategy.md)
- [升级与迁移策略](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-01-cluster-fundamentals/05-upgrade-paths/03-upgrade-migration-strategy.md)
- [ACK 集群管理](../../domain-12-cloud-providers/04-alicloud-ack/alicloud-ack-overview.md)


<!-- risk-assessed -->
