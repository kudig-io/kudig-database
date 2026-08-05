---
title: 集群自动扩缩容（Cluster Autoscaler）扩容失败
description: 专有云 ACK 集群因 Cluster Autoscaler 权限不足导致 HPA 触发后节点池无法自动扩容的工单闭环样本。
summary: 专有云 ACK 集群因 Cluster Autoscaler 权限不足导致 HPA 触发后节点池无法自动扩容的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- cluster-autoscaler
- autoscaler
- scaling
- hpa
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:00:00+08:00'
incident_id: INC-2026-ACK-045
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: kube-system
ticket_type: 自动扩缩容故障
skill_ref:
- '[[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/01-karpenter-node-autoscaling-guide|节点自动扩缩容指南]]'
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/cluster-autoscaler-fta.md|Cluster
  Autoscaler 异常故障树分析]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/FTA故障树/list/cluster-autoscaler-fta.md|FTA:
  Cluster Autoscaler 异常]]'
last_updated: 2026-06-26 16:00:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 集群自动扩缩容（Cluster Autoscaler）扩容失败 如何处理
trigger_keywords:
- 集群自动扩缩容（Cluster
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
- target: '[[domain-11-production-operations/工单案例/ticket-case-020-cluster-autoscaler-scale-failure.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[concepts/autoscaling-strategies.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-04` 的电商大促期间，发现 HPA 已经触发并创建了额外 Pod，但大量 Pod 长时间处于 `Pending` 状态，节点数量没有自动增加。客户描述如下：

> “我们配置了 HPA 和 Cluster Autoscaler，CPU 打高后 Pod 扩出来了，但是一直 Pending，describe 说是 insufficient cpu。节点池没有自动加机器，autoscaler Pod 是 Running 的。ACK 控制台看节点池也没有扩容活动。麻烦看一下是不是 autoscaler 出问题了。”

受影响命名空间主要为 `mall-prod` 与 `kube-system`，大促期间流量突增，自动扩容链路失效。

## 分类与优先级判定

- **工单类型**：自动扩缩容故障 / Cluster Autoscaler 故障。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 大促期间 HPA 已触发但节点池未扩容，业务 Pod 堆积 Pending，服务能力受限。
2. Cluster Autoscaler 是弹性核心组件，失效会放大资源不足影响。
3. 未造成完全不可用，但处于持续降级状态，需尽快修复。

## 诊断步骤

按“先 Pending 状态、后 CA 日志、再权限与配置”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 与 HPA 状态
kubectl get pod -n mall-prod -o wide | grep Pending
kubectl get hpa -n mall-prod
kubectl describe hpa mall-api -n mall-prod | tail -30

# 2. 查看 Cluster Autoscaler Pod 状态
kubectl get pod -n kube-system -l app=cluster-autoscaler -o wide
kubectl describe pod -n kube-system -l app=cluster-autoscaler

# 3. 查看 Cluster Autoscaler 日志中的扩容失败原因
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=300 | grep -iE "scale-up|error|fail|permission|InsufficientInstanceCapacity"

# 4. 查看节点池配置与扩缩容开关
aliyun cs GET /clusters/ack-zyy-prod-04/nodepools/np-mall-compute

# 5. 检查 Cluster Autoscaler 的 RBAC 权限
kubectl auth can-i create node --as system:serviceaccount:kube-system:cluster-autoscaler
kubectl get clusterrolebinding cluster-autoscaler -o yaml

# 6. 检查 autoscaler 启动参数
kubectl get deployment cluster-autoscaler -n kube-system -o jsonpath='{.spec.template.spec.containers[0].command}' | tr ' ' '\n' | grep -E "node-group-auto-discovery|cloud-provider|expander"

# 7. 检查 ACK 控制台节点池事件
ack-cli nodepool events --cluster ack-zyy-prod-04 --nodepool np-mall-compute
```
## 根因分析

Cluster Autoscaler Pod 日志显示：

```
Failed to get nodepool np-mall-compute: Aliyun API Error: InvalidAccessKeyId.NotFound
Failed to scale up: could not increase node group np-mall-compute size
```

根本原因是 Cluster Autoscaler 使用的 RAM Role 对应的 AccessKey 在两天前被轮换，但 ACK 集群的 `cluster-autoscaler` Secret 中保存的 `access-key-id` 与 `access-key-secret` 未同步更新。Cluster Autoscaler 进程虽然可以读取集群内资源状态并判定需要扩容，但调用阿里云 OpenAPI 创建 ECS 实例时认证失败，因此节点池没有任何扩容活动。此类故障具有隐蔽性，因为 CA Pod 本身不会 Crash，日志中的错误也容易被海量的 scale-up 调度信息淹没，需要针对性地过滤 `Aliyun API Error` 关键字。

## 修复命令

**第一步：获取当前有效的 RAM AccessKey**

```bash
aliyun ram ListAccessKeys --UserName k8s-cluster-autoscaler --output cols=AccessKeyId,Status rows=AccessKeys.AccessKey[]
```

**第二步：更新 Cluster Autoscaler 使用的 Secret**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 先删除旧 Secret
kubectl delete secret cluster-autoscaler-cloud-config -n kube-system

# 创建新 Secret
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: cluster-autoscaler-cloud-config
  namespace: kube-system
type: Opaque
stringData:
  access-key-id: "LTAI5t8Z3y8Y7Z7Z7Z7Z7Z7Z"
  access-key-secret: "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
  region-id: "cn-zhangjiakou"
EOF
```
**第三步：重启 Cluster Autoscaler 使其读取新凭证**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/cluster-autoscaler -n kube-system
kubectl rollout status deployment/cluster-autoscaler -n kube-system --timeout=120s
```
**第四步：临时手动扩容节点池缓解 Pending 状态**

```bash
aliyun cs POST /clusters/ack-zyy-prod-04/nodes \
  --body '{"count":4,"instance_type":"ecs.c7.2xlarge","image_id":"aliyun_3_x64_20G_alibase_20240618.vhd","nodepool_id":"np-mall-compute"}'
```

手动扩容仅用于快速缓解业务压力，待 CA 凭证恢复后应让 CA 接管后续扩缩容决策，避免人工扩容节点与自动缩容策略冲突导致节点资源浪费。

**第五步：验证 Cluster Autoscaler 能正常调用 OpenAPI**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100 | grep -iE "scale-up|success|node group"
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Cluster Autoscaler Pod Running 且日志无认证错误
kubectl get pod -n kube-system -l app=cluster-autoscaler -o wide
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100 | grep -i "InvalidAccessKeyId" | wc -l

# 2. 新节点已加入并 Ready
kubectl get node -l nodepool=np-mall-compute -o wide

# 3. Pending Pod 开始调度
kubectl get pod -n mall-prod --field-selector=status.phase=Pending -o json | jq '.items | length'

# 4. HPA 目标 Deployment 副本全部可用
kubectl get deployment mall-api -n mall-prod -o jsonpath='{.status.availableReplicas}/{.status.replicas}'

# 5. ACK 控制台查看节点池扩容活动
ack-cli nodepool status --cluster ack-zyy-prod-04 --nodepool np-mall-compute
```
验证时还需要模拟缩容场景，确认 Cluster Autoscaler 在负载下降后能够正常缩容节点，避免大促结束后产生大量闲置节点造成成本浪费。可通过临时降低 HPA 目标利用率或手动删除测试 Deployment 触发缩容，观察 CA 日志中的 `scale-down` 事件与节点移除动作。

## 回复客户话术

> 您好，经排查，本次 HPA 触发后节点池未自动扩容的根因是 **Cluster Autoscaler 的阿里云 AccessKey 过期**。该组件可以正常读取 Pod 状态并判定需要扩容，但调用 OpenAPI 创建 ECS 时因凭证失效而失败，导致没有节点加入集群。我们已完成以下处置：
>
> 1. 查询并获取当前有效的 RAM AccessKey；
> 2. 更新 `kube-system` 下 `cluster-autoscaler-cloud-config` Secret；
> 3. 重启 Cluster Autoscaler 并临时手动扩容 4 台节点，缓解 Pending 状态。
>
> 当前新节点已 Ready，Pending Pod 已全部调度，HPA 目标副本全部可用。建议后续：
> - 建立 AccessKey 轮换自动化，确保证书/密钥过期前同步更新 K8s Secret；
> - 为 Cluster Autoscaler 配置 RAM Role 的 STSToken 方案，避免长期 AccessKey；
> - 配置 Cluster Autoscaler 失败告警。
>
> 如有新异常请随时联系。

## 复盘与沉淀

本次故障揭示了自动扩缩容链路中“控制面认证”这一隐性依赖。即使 Cluster Autoscaler 本身运行正常，其与云厂商 OpenAPI 的凭证失效也会导致整个弹性能力瘫痪。在专有云 ACK 中，Cluster Autoscaler 通常通过 RAM AccessKey 或 STS Token 访问 ECS/OpenAPI，凭证管理必须纳入运维变更台账。

建议优先使用 RRSA（RAM Roles for Service Accounts）方案，将 RAM Role 直接绑定到 Cluster Autoscaler 使用的 ServiceAccount，避免在 Secret 中硬编码长期 AccessKey。RRSA 不仅安全性更高，而且可以自动刷新临时凭证，减少人工维护成本。如果使用 AccessKey，必须建立自动轮换机制，并在轮换后通过 External Secrets Operator 或 ACK 托管组件自动同步到 K8s Secret。

同时，应建立扩缩容链路的全链路监控。除了 Cluster Autoscaler 自身的指标外，还需要监控 HPA 触发的副本变化、Pending Pod 数量、节点加入耗时、节点 Ready 耗时等指标。大促前应进行全链路演练，模拟高负载下 HPA 触发、CA 扩容、Pod 调度、服务注册的完整流程，确保每个环节都能在 SLA 内完成。演练结果应形成书面报告，明确每个环节的 RTO 与 RPO 指标，并作为大促保障评审的必审材料纳入标准化变更管理流程。

后续 SOP 更新要点：
1. Cluster Autoscaler 凭证纳入密钥管理生命周期，设置到期前 7 天告警；
2. 评估并迁移到 RRSA 认证方案，避免长期 AccessKey；
3. 监控 `cluster_autoscaler_errors_total` 与 `cluster_autoscaler_failed_scale_ups_total`；
4. 大促前进行扩缩容演练，验证 CA 与 HPA 联动链路；
5. 在密钥轮换后强制触发一次 CA 扩缩容测试，确保证书链路与 OpenAPI 调用均正常；
6. 将本案例写入 Cluster Autoscaler 失败回复模板。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若后续希望迁移到 RRSA 或节点 RAM Role，需升级至 **平台安全团队** 评估认证方案改造。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-045`
  - 根因：Cluster Autoscaler 阿里云 AccessKey 过期，OpenAPI 调用认证失败
  - 影响节点池：`np-mall-compute`
  - 影响命名空间：`mall-prod`、`kube-system`
  - 临时修复：更新 Secret + 重启 CA + 手动扩容 4 台节点
  - 长期方案：AccessKey 自动轮换 / RRSA 认证改造 + 弹性链路演练
  - 待跟进：确认 RRSA 改造排期，更新密钥管理 SOP

## Related

- Cluster Autoscaler
- [[domain-11-production-operations/工单案例/ticket-case-020-cluster-autoscaler-scale-failure.md|Cluster Autoscaler 扩容失败：节点池未触发自动扩容]]
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- [[concepts/autoscaling-strategies.md|Autoscaling Strategies]]


<!-- risk-assessed -->
