---
title: Cluster Autoscaler 扩容失败：节点池未触发自动扩容
description: 专有云 ACK 集群因 Cluster Autoscaler 配置与实例库存问题导致无法自动扩容，业务 Pod 长期 Pending 的工单闭环样本。
summary: 专有云 ACK 集群因 Cluster Autoscaler 配置与实例库存问题导致无法自动扩容，业务 Pod 长期 Pending 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- cluster-autoscaler
- autoscaler
- scaling
- p1
- capacity
tier: supporting
created: '2026-06-26T16:00:00+08:00'
updated: '2026-06-26T18:30:00+08:00'
incident_id: INC-2026-ACK-020
priority: P1
severity: high
affected_cluster: ack-zyy-prod-08
affected_namespace: recommendation-engine
ticket_type: 自动扩缩容异常
skill_ref:
- '[[32-发布/package/2026-07-02_18-29/corpus/core/domain-07-platform-engineering/01-karpenter-node-autoscaling-guide|节点自动扩缩容指南]]'
- Cluster Autoscaler 排查
fta_ref:
- 'FTA: Cluster Autoscaler 不扩容'
last_updated: 2026-06-26 18:30:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Cluster Autoscaler 扩容失败：节点池未触发自动扩容 如何处理
trigger_keywords:
- Cluster
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
relationships:
- target: '[[domain-17-system-foundation/知识字典/scheduling/cluster-autoscaler.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-045-cluster-autoscaler-scaleup-fail.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-019-kubeproxy-service-unreachable.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在推荐引擎业务高峰期通过 HPA 扩容后，发现大量 Pod 处于 Pending 状态，但 Cluster Autoscaler 迟迟没有触发节点扩容。客户描述如下：

> “ACK 集群 ack-zyy-prod-08 的 recommendation-engine 命名空间，HPA 已经把副本数从 30 扩到 100 了，但 Pod 一直 Pending。我们看节点池节点数没变化，Cluster Autoscaler 也没加节点。describe pod 提示 Insufficient cpu。手动扩容按钮在控制台也点了，但新节点创建不出来。麻烦看一下是不是 autoscaler 挂了。”

受影响命名空间为 `recommendation-engine`，核心应用为 `recommendation-api` 与 `feature-server`。节点池 `np-zyy-rec` 当前节点数为 6，最大节点数配置为 20，实例类型为 `ecs.g7.xlarge`。

## 分类与优先级判定

- **工单类型**：自动扩缩容异常 / 调度容量不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境业务已触发 HPA 扩容，但集群层无法提供足够节点，服务能力受限。
2. 问题指向 Cluster Autoscaler 或底层 IaaS 扩容链路，需快速区分是配置问题还是库存问题。
3. 推荐引擎属于核心在线服务，需在 30 分钟内恢复自动扩容能力或完成手动扩容。

## 诊断步骤

按“先看 Pod 与节点状态、再看 CA 日志、再看节点池配置、最后看 IaaS 事件”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 与节点状态
kubectl get pod -n recommendation-engine | grep Pending
kubectl get node -o wide

# 2. 查看 Cluster Autoscaler Pod 状态
kubectl get pod -n kube-system -l app=cluster-autoscaler -o wide

# 3. 采集 Cluster Autoscaler 日志，关注 scale-up 失败原因
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=500 | grep -iE "scale.up|failed|insufficient|no.increase|node.group|expand"

# 4. 查看 CA 当前配置与节点池状态
kubectl get configmap cluster-autoscaler-config -n kube-system -o yaml
aliyun cs GET /clusters/ack-zyy-prod-08/nodepools/np-zyy-rec

# 5. 检查节点池 autoscaling 配置
aliyun cs GET /clusters/ack-zyy-prod-08/nodepools/np-zyy-rec | jq '.auto_scaling'

# 6. 查看 ECS 伸缩组活动与库存信息
aliyun ess DescribeScalingActivities \
  --RegionId cn-hangzhou \
  --ScalingGroupId asg-dummyrecgroup \
  --PageSize 20

# 7. 检查 ACK 控制台节点池事件与告警
ack-cli event list --cluster ack-zyy-prod-08 --resource nodepool/np-zyy-rec

# 8. 检查 Pod 是否有阻止调度的约束（如 nodeSelector、taint）
kubectl get pod -n recommendation-engine -l app=recommendation-api -o json | \
  jq '.items[].spec | {nodeSelector, affinity, tolerations}'
```
## 根因分析

经过排查，Cluster Autoscaler 日志中出现以下关键信息：

```
I0626 16:15:23.123456       1 scale_up.go:456] No expansion options for recommendation-engine/recommendation-api-7d9c4f8b5-xk2z9: no node group can fit the pod
I0626 16:15:24.234567       1 alicloud_manager.go:234] Failed to create instances for node group np-zyy-rec: InvalidInstanceType.NotSupportStock, The instance type ecs.g7.xlarge is out of stock in the current availability zone
```

根本原因为：
1. **实例库存不足**：节点池 `np-zyy-rec` 使用的 `ecs.g7.xlarge` 在当前可用区库存售罄，Cluster Autoscaler 向 ESS 发起扩容请求后失败。
2. **节点池未配置多可用区/多实例规格**：节点池仅绑定单一可用区与单一实例类型，没有 fallback 能力，一旦该规格库存不足即无法扩容。
3. **HPA 与 CA 联动存在延迟**：HPA 扩容 Pod 后，CA 需要数分钟评估、请求 IaaS、创建并加入节点，整个过程在库存不足时完全失败。
4. **业务 Pod 的 nodeSelector 限制**：`recommendation-api` 配置了 `node.kubernetes.io/instance-type=ecs.g7.xlarge`，进一步限制了可扩容的节点类型。

## 修复命令

**第一步：临时创建备用节点池，使用库存充足的实例类型**

```bash
aliyun cs POST /clusters/ack-zyy-prod-08/nodepools \
  --body '{
    "nodepool_info": {"name": "np-zyy-rec-burst"},
    "scaling_group": {
      "instance_types": ["ecs.g7.2xlarge", "ecs.g7ne.xlarge"],
      "min_instance": 0,
      "max_instance": 15,
      "multi_az_policy": "COST_OPTIMIZED",
      "vswitch_ids": ["vsw-dummy1a", "vsw-dummy1b", "vsw-dummy1c"],
      "image_id": "aliyun_3_x64_20G_alibase_20240618.vhd",
      "system_disk_category": "cloud_essd",
      "system_disk_size": 120
    },
    "kubernetes_config": {
      "labels": {"workload": "recommendation-engine"},
      "taints": []
    }
  }'
```

**第二步：为业务 Pod 增加节点亲和性，允许调度到备用节点池**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment recommendation-api -n recommendation-engine --type='json' -p='[
  {"op": "remove", "path": "/spec/template/spec/nodeSelector"},
  {"op": "add", "path": "/spec/template/spec/affinity", "value": {
    "nodeAffinity": {
      "preferredDuringSchedulingIgnoredDuringExecution": [
        {"weight": 100, "preference": {"matchExpressions": [{"key": "workload", "operator": "In", "values": ["recommendation-engine"]}]}}
      ]
    }
  }}
]'
```
**第三步：手动触发 Cluster Autoscaler 重新评估（或等待默认扫描周期）**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
# 通过删除 CA Pod 强制重新启动并触发扫描
kubectl delete pod -n kube-system -l app=cluster-autoscaler --force --grace-period=0  # ⚠️ 跳过优雅终止，可能丢数据
kubectl wait --for=condition=Ready pod -n kube-system -l app=cluster-autoscaler --timeout=120s
```
**第四步：若 CA 仍无法扩容，临时手动扩容节点池**

```bash
aliyun cs POST /clusters/ack-zyy-prod-08/nodes \
  --body '{
    "count": 4,
    "instance_type": "ecs.g7.2xlarge",
    "nodepool_id": "np-zyy-rec-burst",
    "image_id": "aliyun_3_x64_20G_alibase_20240618.vhd"
  }'
```

**第五步：优化 CA 配置，增加扩容优先级与多规格支持**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch configmap cluster-autoscaler-config -n kube-system --type='json' -p='[
  {"op": "add", "path": "/data/expander", "value": "priority"},
  {"op": "add", "path": "/data/skip-nodes-with-local-storage", "value": "false"}
]'
kubectl rollout restart deployment cluster-autoscaler -n kube-system
```
## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 备用节点池创建成功
aliyun cs GET /clusters/ack-zyy-prod-08/nodepools | jq '.nodepools[] | select(.nodepool_info.name=="np-zyy-rec-burst") | .nodepool_info.name'

# 2. 新节点已加入并 Ready
kubectl get node -l workload=recommendation-engine -o wide

# 3. Pending Pod 开始 Running
kubectl get pod -n recommendation-engine | grep Pending | wc -l
kubectl get pod -n recommendation-engine -l app=recommendation-api

# 4. CA 日志显示 scale-up 成功
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=200 | grep "Scale-up finished"

# 5. HPA 当前副本数与目标一致
kubectl get hpa -n recommendation-engine recommendation-api -o jsonpath='{.status.currentReplicas}/{.status.desiredReplicas}'

# 6. 业务接口健康检查通过
kubectl run rec-test -n recommendation-engine --rm -i --restart=Never --image=registry.aliyuncs.com/acs/busybox -- \
  wget -qO- http://recommendation-api:8080/healthz
```
## 回复客户话术

> 您好，经排查，本次自动扩容失败的根因是 **节点池 `np-zyy-rec` 绑定的实例类型 `ecs.g7.xlarge` 在当前可用区库存售罄**，Cluster Autoscaler 向 ESS 发起扩容请求后失败，导致 HPA 新增的 Pod 无法调度。我们已完成以下处置：
>
> 1. 创建备用节点池 `np-zyy-rec-burst`，使用 `ecs.g7.2xlarge` 与 `ecs.g7ne.xlarge` 两种库存充足的规格，并绑定多可用区；
> 2. 移除 `recommendation-api` 的 instance-type nodeSelector，增加节点亲和性优先调度到推荐引擎节点池；
> 3. 手动触发 Cluster Autoscaler 重新评估，并补充手动扩容 4 台节点；
> 4. 优化 Cluster Autoscaler 配置，启用 priority expander 以优先使用成本与库存更优的节点池。
>
> 当前 Pending Pod 已逐步 Running，推荐服务处理能力已恢复。建议后续：
> - 为生产节点池配置多实例规格与多可用区，避免单一规格库存不足导致无法扩容；
> - 配置 Cluster Autoscaler 扩容失败告警；
> - 在大促前预扩容节点，避免依赖实时自动扩容。
>
> 如有新异常，请随时联系。

## 复盘与沉淀

本次故障充分说明，HPA、Cluster Autoscaler 与底层 IaaS 库存三者必须作为一个整体来管理。HPA 决定了 Pod 数量，CA 决定了节点数量，而 IaaS 库存决定了 CA 是否能真正创建出节点。任何一环失败都会导致“扩了 Pod 没扩节点”的局面。

关键经验教训：
1. **节点池不要绑定单一实例类型**：生产环境应至少配置 2-3 种同代、同架构的实例类型，由 CA 根据库存与成本自动选择；
2. **多可用区是刚需**：单可用区不仅存在库存风险，也存在可用区级别故障风险；
3. **避免使用 instance-type nodeSelector**：这会将业务与特定硬件绑定，丧失 CA 的灵活调度能力；应使用标签、污点/容忍或节点亲和性；
4. **大促前预扩容**：自动扩容存在分钟级延迟，且依赖库存，关键业务应在高峰前完成预扩容。

后续 SOP 更新要点：
1. 节点池配置评审必须包含：多实例规格、多可用区、max_size 设置、标签污点设计；
2. 配置告警：`cluster_autoscaler_failed_scale_ups_total` 增长或 `cluster_autoscaler_unschedulable_pods_count` > 10 持续 5 分钟触发 P1；
3. 在 ACK 控制台启用节点池库存预警，提前收到实例类型库存不足通知；
4. 将本案例写入 Cluster Autoscaler 扩容失败回复模板；
5. 建立大促前容量演练机制，模拟 HPA 最大副本 + CA 扩容全链路。

最后，建议在 FinOps 维度分析自动扩缩容成本：记录因库存不足而临时使用更高规格实例导致的额外成本、CA 扩容延迟造成的业务损失，以及预扩容与按需扩容的性价比差异。这些数据可用于优化节点池选型与采购策略。

## 是否需要升级及交接信息

- **是否升级**：已止血并恢复自动扩容能力，暂不需要升级；若多可用区/多规格扩容仍持续失败，需升级至 **IaaS 库存管理团队** 与 **ACK 产品支持**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-020`
  - 根因：节点池绑定实例类型 `ecs.g7.xlarge` 库存售罄，Cluster Autoscaler 无法扩容
  - 影响集群：`ack-zyy-prod-08`
  - 影响命名空间：`recommendation-engine`
  - 临时修复：创建多规格/多可用区备用节点池、移除 instance-type nodeSelector、手动扩容
  - 长期方案：节点池多规格/多可用区改造、CA priority expander、大促预扩容 SOP
  - 待跟进：确认备用节点池稳定运行、将多规格策略推广到其他生产节点池

## Related

- Cluster Autoscaler
- 集群自动扩缩容（Cluster Autoscaler）扩容失败
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- Service 访问异常：kube-proxy 未同步 Endpoint 导致 ClusterIP 不通


<!-- risk-assessed -->
