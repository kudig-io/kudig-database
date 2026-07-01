---
title: Pod Pending：资源不足与 Taint 不匹配
description: 专有云 ACK 集群因节点资源不足叠加自定义 taint 导致业务 Pod 长时间 Pending 的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- pod-pending
- scheduler
- taints
- resources
- p1
incident_id: INC-2026-ACK-042
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: data-proc
ticket_type: 调度失败
skill_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md|Pod 异常故障树分析]]'
- '[[domain-07-platform-engineering/governance/03-capacity-planning-resource-assessment.md|容量规划与资源评估]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/scheduler-fta.md|FTA: Scheduler
  异常]]'
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T15:45:00+08:00'
last_updated: 2026-06-26T15:45:00+08:00
duplicate_of: INC-2026-ACK-047
status: duplicate
duplication_reason: 与 "INC-2026-ACK-047" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod Pending：资源不足与 Taint 不匹配 如何处理
trigger_keywords:
- Pod
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
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md]]"
  type: related_to
---

# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-04` 的数据处理平台发布新版本后，发现 `data-proc` 命名空间内大量 Pod 处于 `Pending` 状态已超过 20 分钟。客户描述如下：

> “我们刚发了一版数据处理任务，Pod 一直 Pending。describe pod 看到 `0/12 nodes are available`，有的说 insufficient memory，有的说 node had taint。我们没改节点配置， yesterday 还能正常调度。麻烦看一下是不是调度器出问题了。”

受影响命名空间为 `data-proc`，主要运行 Spark executor 与 Flink taskmanager 等大数据处理任务。

## 分类与优先级判定

- **工单类型**：调度失败 / 资源不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境大量数据处理 Pod 无法调度，批处理任务堆积。
2. 报错同时涉及资源不足与 taint 不匹配，需要区分根因。
3. 服务降级但未完全不可用，需在 30 分钟内给出完整诊断与修复方案。

## 诊断步骤

按“先 Pod 事件、后节点资源、再调度约束”的顺序排查：

```bash
# 1. 查看 Pending Pod 列表与状态
kubectl get pod -n data-proc -o wide | grep Pending

# 2. 查看典型 Pending Pod 的事件与调度提示
kubectl describe pod spark-executor-7d9c4f8b5-xk2z9 -n data-proc | tail -50

# 3. 统计所有 Pending Pod 的调度失败原因
kubectl get pod -n data-proc --field-selector=status.phase=Pending -o json | jq -r '.items[] | "\(.metadata.name): \(.status.conditions[]? | select(.reason=="Unschedulable") | .message)"' | head -30

# 4. 检查节点资源总量与可分配量
kubectl describe node -l nodepool=data-compute | grep -A 5 "Allocated resources"

# 5. 检查节点 taint 与标签
kubectl get node -l nodepool=data-compute -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints,LABELS:.metadata.labels | head -20

# 6. 检查 Pod 是否设置了 toleration 与 nodeSelector/affinity
kubectl get pod spark-executor-7d9c4f8b5-xk2z9 -n data-proc -o yaml | grep -A 20 -E "tolerations|nodeSelector|affinity"

# 7. 检查 scheduler 日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100 | grep -i "data-proc"
```

## 根因分析

`data-proc` 命名空间内新版本的 Pod 资源请求从 `memory: 4Gi` 提升到了 `memory: 8Gi`，但未同步调整集群节点池规格。当前 `nodepool=data-compute` 的节点为 `ecs.r7.xlarge`，单节点可分配内存约 30Gi，已运行 3 个旧版 executor（每个 4Gi），剩余可分配内存约 18Gi，理论上可容纳 2 个新版 Pod。

进一步检查发现，运维团队在前一晚为了隔离问题节点，为 `cn-zhangjiakou.172.16.4.12` 打上了 `node.kubernetes.io/out-of-service:NoSchedule` 的 taint，但忘了在 30 分钟后移除。该污点未在 Pod 的 tolerations 中声明，导致 scheduler 认为可用节点从 12 台减少到 11 台，且资源最紧张的节点被排除后，新版 8Gi Pod 无法满足 `podAntiAffinity` 所需的分布条件，最终大量 Pending。

根本原因：资源请求突增 + 遗留 out-of-service taint + podAntiAffinity 分布约束共同导致调度失败。在实际排障过程中，这三个因素单独出现都不会造成大面积 Pending，但叠加后可用节点数量与单节点容量同时受限，使得 scheduler 无法满足 Redisis 类有状态服务对分布与资源的综合要求。这也说明调度失败排查必须结合资源、污点、亲和性、优先级等多维度信息，而不能仅看单一事件。

## 修复命令

**第一步：移除遗留的 out-of-service taint，恢复 12 台可用节点**

```bash
kubectl taint node cn-zhangjiakou.172.16.4.12 node.kubernetes.io/out-of-service:NoSchedule-
```

**第二步：临时扩容节点池，增加可调度容量**

```bash
aliyun cs POST /clusters/ack-zyy-prod-04/nodes \
  --body '{"count":3,"instance_type":"ecs.r7.2xlarge","image_id":"aliyun_3_x64_20G_alibase_20240618.vhd","nodepool_id":"np-data-compute"}'
```

**第三步：回滚 Pod 资源请求到合理基线（若业务允许）**

```bash
kubectl set resources deployment/spark-executor -n data-proc --requests=memory=4Gi,cpu=2 --limits=memory=8Gi,cpu=4
kubectl rollout status deployment/spark-executor -n data-proc --timeout=300s
```

**第四步：验证节点污点已清除且新节点加入**

```bash
kubectl get node -l nodepool=data-compute -o custom-columns=NAME:.metadata.name,READY:.status.conditions[-1].status,TAINTS:.spec.taints
```

## 验证命令

```bash
# 1. Pending Pod 全部调度成功
kubectl get pod -n data-proc --field-selector=status.phase=Pending -o json | jq '.items | length'

# 2. 调度事件显示成功绑定节点
kubectl get events -n data-proc --field-selector reason=Scheduled --sort-by='.lastTimestamp' | tail -20

# 3. 节点资源使用恢复正常
kubectl top node -l nodepool=data-compute

# 4. 业务任务开始 Running
kubectl get pod -n data-proc -o wide | grep -E "Running|Pending"

# 5. ACK 控制台查看节点池 np-data-compute 节点数量
ack-cli nodepool status --cluster ack-zyy-prod-04 --nodepool np-data-compute
```

## 回复客户话术

> 您好，经排查，本次 `data-proc` 命名空间大量 Pod Pending 的根因是 **资源请求突增叠加遗留节点污点**。新版本 Pod memory request 从 4Gi 提升到 8Gi，同时节点 `cn-zhangjiakou.172.16.4.12` 上遗留了前日排查时打上的 `out-of-service:NoSchedule` 污点，导致 scheduler 可用节点减少且分布约束无法满足。我们已完成以下处置：
>
> 1. 移除遗留污点，恢复 12 台可用节点；
> 2. 临时扩容 3 台 `ecs.r7.2xlarge` 节点到 `np-data-compute` 节点池；
> 3. 将 Spark executor 的 request 回滚到 4Gi（limit 保持 8Gi），平衡调度密度与资源上限。
>
> 当前 Pending Pod 已全部调度并进入 Running。建议后续：
> - 版本发布前在测试环境验证资源请求变化对调度容量的影响；
> - 建立 taint 操作台账，临时污点必须设置自动过期或人工复核；
> - 配置 [[domain-07-platform-engineering/governance/03-capacity-planning-resource-assessment.md|容量规划]] 流程，评估节点池是否需要升级实例规格。
>
> 如有新异常请随时联系。

## 复盘与沉淀

本次故障体现了调度失败往往是多种约束叠加的结果。单纯看 `0/12 nodes are available` 容易误判为全局资源不足，实际上需要拆解：资源是否足够、taint 是否匹配、affinity/anti-affinity 是否满足、优先级与抢占策略是否生效。在专有云 ACK 中，节点池管理与 ECS 实例规格强相关，资源请求的提升会直接影响单节点可容纳 Pod 数量。

运维临时污点是一个高频人为失误点。建议在变更脚本中为所有 taint 操作增加 TTL 与告警，例如使用 `kubectl taint` 后 30 分钟自动检查并移除。同时可以在节点池维度维护“可用调度容量看板”，实时展示各节点池剩余可分配资源与最大 Pod 密度。对于大数据类任务，建议设置独立的节点池与优先级，并启用 scheduler 的抢占与重调度能力，避免低优任务长期 Pending 阻塞高优任务。

此外，版本发布时应将资源请求变化纳入变更评审。Spark、Flink 等大数据组件在扩容时往往一次性创建数十个 Pod，若 request 翻倍而节点池未扩容，调度失败会在秒级内大面积扩散。建议发布前使用 ACK 的调度模拟器或 `kube-scheduler` 的 `--write-config-to` 输出进行预演。

后续 SOP 更新要点：
1. 发布前使用 `kubectl describe node` 计算新 request 下的单节点承载量；
2. 临时污点必须登记到变更工单，并设置自动清理任务；
3. 配置告警：`kube_pod_status_unschedulable{namespace="data-proc"} > 0` 持续 5 分钟触发 P2 告警；
4. 大数据任务使用独立节点池与优先级，避免与在线业务竞争资源；
5. 将本案例写入 Pod Pending 回复模板。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若节点池扩容后仍频繁 Pending，需升级至 **容量管理团队** 评估升级节点池实例规格。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-042`
  - 根因：Pod request 提升 + 遗留节点污点 + podAntiAffinity 约束
  - 影响命名空间：`data-proc`
  - 临时修复：移除污点 + 扩容节点池 + 回滚 request
  - 长期方案：发布前容量评估 + taint 自动清理 + 调度失败监控
  - 待跟进：确认临时扩容节点是否保留或缩容，更新发布 checklist

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Ingress 控制器 Pod 异常导致 404/502
- [[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]
