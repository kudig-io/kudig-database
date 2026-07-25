---
title: Pod Pending：资源不足与污点不匹配
description: 专有云 ACK 集群 AI 训练任务因 GPU 节点污点未容忍及资源请求过大导致 Pod 长时间 Pending 的工单闭环样本。
summary: 专有云 ACK 集群 AI 训练任务因 GPU 节点污点未容忍及资源请求过大导致 Pod 长时间 Pending 的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- pod-pending
- gpu
- taint
- resource
- p1
- scheduling
tier: peripheral
created: '2026-06-26T08:30:00+08:00'
updated: '2026-06-26T11:00:00+08:00'
incident_id: INC-2026-ACK-027
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: ai-training
ticket_type: 调度异常
skill_ref:
- Pod Pending 诊断
- Pod 调度策略
fta_ref:
- 'FTA: Pod Pending 根因分析'
last_updated: 2026-06-26 11:00:00+08:00
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
- Pod Pending：资源不足与污点不匹配 如何处理
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
- target: '[[13-生产运维/05-工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈其提交到专有云 ACK 集群 `ack-zyy-prod-04` 的分布式 AI 训练任务 `tf-resnet50-pretrain` 已经 Pending 超过 30 分钟，所有 worker Pod 均无法调度。客户描述如下：

> “我们在 ai-training 命名空间提交了一个 4 worker 的 TensorFlow 训练 Job，每个 worker 要 8 核 CPU、64G 内存和 1 张 GPU。但 Pod 一直 Pending，describe 只看到 0/4 nodes are available。GPU 节点池明明是开着的，kubectl get node 也能看到 GPU 节点。帮忙看看为什么调度不过去。”

该训练任务属于本周重点模型迭代项目，原定当日完成 checkpoint，延迟会影响下游评测排期。

## 分类与优先级判定

- **工单类型**：调度异常 / 资源不足。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境训练任务无法调度，导致模型训练阻塞，属于关键业务路径异常。
2. 节点资源存在但 Pod 无法绑定，属于调度层问题，需快速定位并给出可执行方案。
3. 影响面为一个训练 Job，未造成在线服务不可用，因此定为 P1 而非 P0。

## 诊断步骤

按“先 Pod 事件、再节点资源与污点、最后调度约束”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 状态与事件
kubectl get pod -n ai-training -l job-name=tf-resnet50-pretrain -o wide
kubectl describe pod -n ai-training tf-resnet50-pretrain-worker-0 | tail -80

# 2. 查看所有节点资源使用与污点
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,CPU_ALLOCATABLE:.status.allocatable.cpu,MEM_ALLOCATABLE:.status.allocatable.memory,GPU_ALLOCATABLE:.status.allocatable.nvidia\.com/gpu,TAINTS:.spec.taints[*].key'

# 3. 单独查看 GPU 节点详情
kubectl describe node cn-beijing.192.168.10.21 | grep -A 15 Taints
kubectl describe node cn-beijing.192.168.10.21 | grep -A 30 Allocated

# 4. 查看节点标签，确认可用区与实例类型
kubectl get node cn-beijing.192.168.10.21 --show-labels | tr ',' '\n' | grep -E 'zone|instance|gpu|accelerator'

# 5. 检查训练 Job 的资源请求与容忍度
kubectl get job tf-resnet50-pretrain -n ai-training -o yaml | grep -A 40 template

# 6. 检查命名空间 ResourceQuota 是否已耗尽
kubectl get resourcequota -n ai-training
kubectl describe resourcequota -n ai-training

# 7. 查看 kube-scheduler 审计日志（如权限允许）
kubectl logs -n kube-system -l component=kube-scheduler --tail=200 | grep tf-resnet50-pretrain
```
## 根因分析

经排查，Pending 原因由两个因素叠加导致：

1. **GPU 节点存在污点但 Pod 未设置 toleration**。客户节点池 `np-zyy-gpu` 中的节点均带有污点：

   ```yaml
   taints:
     - key: nvidia.com/gpu
       value: "true"
       effect: NoSchedule
   ```

   而训练 Job 的 PodTemplate 中没有对应的 `tolerations`，因此调度器在预过滤阶段直接将这些节点排除。

2. **CPU/Memory 请求过大导致可调度节点进一步减少**。每个 worker 请求 `cpu: 8`、`memory: 64Gi`，但当前 GPU 节点（`ecs.gn7i-c8g1.2xlarge`）的 allocatable CPU 仅约 7.8 核、内存约 58Gi，扣除系统预留后根本无法满足单 Pod 8 核 64Gi 的需求。即使污点问题修复，资源请求本身也无法被满足。

`kubectl describe pod` 输出中同时出现：

```
0/4 nodes are available: 4 node(s) had taint {nvidia.com/gpu: true}, that the pod didn't tolerate.
0/4 nodes are available: 4 Insufficient cpu, 4 Insufficient memory.
```

根本原因是 **训练任务资源声明与节点实际 allocatable 不匹配，且未正确容忍 GPU 节点污点**。

## 修复命令

**第一步：确认节点真实 allocatable，避免资源请求超过单节点上限**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node cn-beijing.192.168.10.21 -o jsonpath='{.status.allocatable}'
```
输出显示该节点 allocatable CPU 约 7800m、内存约 58Gi，因此将每个 worker 请求调整为 `cpu: 6`、`memory: 48Gi`，Limit 保持 `cpu: 8`、`memory: 64Gi`。

**第二步：为训练 Job 添加 GPU 节点污点容忍**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch job tf-resnet50-pretrain -n ai-training --type='merge' -p '{
  "spec": {
    "template": {
      "spec": {
        "tolerations": [
          {
            "key": "nvidia.com/gpu",
            "operator": "Equal",
            "value": "true",
            "effect": "NoSchedule"
          }
        ]
      }
    }
  }
}'
```
> 注意：Job 的 PodTemplate 字段不可直接 patch，实际操作建议编辑 YAML 后重新 apply，或删除 Job 后重新创建。

**第三步：修改资源请求并重新提交训练 Job**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 导出当前 Job YAML
kubectl get job tf-resnet50-pretrain -n ai-training -o yaml > /tmp/tf-resnet50-pretrain.yaml

# 修改 resources.requests 为 cpu: 6、memory: 48Gi，并添加上述 tolerations
# 然后删除旧 Job（不级联删除 Pod 会失败，Job 会重建）
kubectl delete job tf-resnet50-pretrain -n ai-training
kubectl apply -f /tmp/tf-resnet50-pretrain.yaml
```
**第四步：如业务确实需要 8 核 64Gi，临时扩容 GPU 节点池**

```bash
aliyun cs POST /clusters/ack-zyy-prod-04/nodes \
  --body '{
    "count": 2,
    "instance_type": "ecs.gn7i-c16g1.4xlarge",
    "nodepool_id": "np-zyy-gpu",
    "image_id": "aliyun_3_x64_20G_alibase_20240618.vhd"
  }'
```

**第五步：如训练任务对可用区有要求，增加节点亲和性**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch job tf-resnet50-pretrain -n ai-training --type='merge' -p '{
  "spec": {
    "template": {
      "spec": {
        "affinity": {
          "nodeAffinity": {
            "requiredDuringSchedulingIgnoredDuringExecution": {
              "nodeSelectorTerms": [
                {
                  "matchExpressions": [
                    {"key": "node.kubernetes.io/instance-type", "operator": "In", "values": ["ecs.gn7i-c16g1.4xlarge"]}
                  ]
                }
              ]
            }
          }
        }
      }
    }
  }
}'
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Pending Pod 全部 Running
kubectl get pod -n ai-training -l job-name=tf-resnet50-pretrain -o wide

# 2. 查看 Pod 调度到的节点与资源分配
kubectl describe pod -n ai-training tf-resnet50-pretrain-worker-0 | grep -A 5 "Node:|Limits|Requests"

# 3. 检查节点 GPU 已分配情况
kubectl describe node cn-beijing.192.168.10.21 | grep -A 5 "Allocated resources"

# 4. 训练任务开始输出日志
kubectl logs -n ai-training -l job-name=tf-resnet50-pretrain --tail=100

# 5. 节点池扩容完成
aliyun cs GET /clusters/ack-zyy-prod-04/nodes \
  --output cols=InstanceId,InstanceType,NodeStatus rows=nodes.node[]
```
## 回复客户话术

> 您好，经排查，训练任务 `tf-resnet50-pretrain` 长时间 Pending 的根因有两个：
>
> 1. **GPU 节点存在 `nvidia.com/gpu=true:NoSchedule` 污点，但训练 Pod 未配置 toleration**，调度器直接排除了这些节点；
> 2. **每个 worker 请求的 8 核 CPU、64Gi 内存超过了当前 GPU 节点实际可分配资源**，即使污点修复也无法调度。
>
> 我们已完成以下处置：
> - 为训练 Job 添加 GPU 节点污点容忍；
> - 将每个 worker 的 requests 调整为 6 核 CPU / 48Gi 内存，limits 保持 8 核 / 64Gi；
> - 如需按原始规格运行，已扩容 2 台更高规格 GPU 节点到节点池 `np-zyy-gpu`。
>
> 当前训练 Pod 已全部 Running 并开始输出训练日志。建议后续：
> - 在提交 GPU 任务前，使用 `kubectl describe node` 确认节点 allocatable，避免请求超过单节点上限；
> - 参考 Pod 调度策略 为 GPU 任务统一配置 tolerations 与 nodeSelector；
> - 配置 GPU 节点资源告警，提前发现资源瓶颈。
>
> 如有训练进度异常，请随时联系。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若扩容后仍频繁出现调度失败，需升级至 **ACK 调度团队** 与 **AI 平台团队** 评估节点池规格。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-027`
  - 根因：`GPU 节点污点未容忍 + 资源请求超过节点 allocatable`
  - 影响集群：`ack-zyy-prod-04`
  - 影响命名空间：`ai-training`
  - 临时修复：添加 toleration + 调整 requests + 扩容 GPU 节点池
  - 长期方案：建立 GPU 任务资源基线，统一 PodTemplate 模板
  - 待跟进：确认扩容节点加入集群并正常分配 GPU，训练任务是否按预期产出 checkpoint

## 复盘与沉淀

本次工单是 ACK 集群 GPU 调度的典型问题。很多 AI 团队从 CPU 集群迁移到 GPU 集群时，容易忽略两个关键点：**节点污点** 与 **allocatable 资源**。GPU 节点通常带有 `nvidia.com/gpu` 污点是 ACK 的标准安全设计，防止普通业务抢占 GPU 资源；但这也意味着所有 GPU 任务必须显式声明 toleration。同时，云厂商的 `ecs.gn7i-c8g1.2xlarge` 等实例虽然规格表写 8 核 32Gi，但扣除 kube-reserved、system-reserved 以及 NVIDIA 驱动、容器运行时等开销后，allocatable 通常只有 7 核左右和 28Gi 左右内存。本例中客户将 requests 设为 8 核 64Gi，显然超过了单节点能力。

复盘要点：
1. **建立 GPU 任务提交模板**：在 `ai-training` 命名空间提供标准 PodTemplate，内置 `tolerations`、`nodeSelector`、`nvidia.com/gpu` 请求以及合理的 requests/limits 比例。
2. **资源基线常态化核对**：在 CI/CD 或 MLOps 平台中增加校验逻辑，提交前检查 `requests.cpu <= node_allocatable.cpu` 与 `requests.memory <= node_allocatable.memory`。
3. **调度失败快速定位**：`kubectl describe pod` 的 `Events` 已经给出明确原因，一线工程师应优先查看事件，而不是先查看业务日志。
4. **容量弹性**：对于周期性训练任务，建议启用 **Cluster Autoscaler** 或 **ACK 弹性节点池**，在任务提交时自动扩容 GPU 节点，任务完成后自动缩容，降低成本。

后续 SOP 更新要点：
- 将 GPU 节点污点与 toleration 示例写入 GPU 工作负载最佳实践；
- 在 Prometheus 中配置告警：节点 `nvidia.com/gpu` allocatable 不足或 Pending GPU Pod 数 > 0 持续 10 分钟；
- 将本案例写入 Pod Pending 回复模板，用于快速响应同类问题。

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
