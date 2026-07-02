---
title: Pod Pending：节点 taint 与 toleration 不匹配
description: 专有云 ACK 集群新上线 AI 推理服务 Pod 长期处于 Pending，根因为节点 taint 未配置对应 toleration，含诊断、修复与验证。
summary: 专有云 ACK 集群新上线 AI 推理服务 Pod 长期处于 Pending，根因为节点 taint 未配置对应 toleration，含诊断、修复与验证。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- pod-pending
- taint
- toleration
- gpu
- scheduling
- p1
tier: peripheral
created: '2026-06-26T10:00:00+08:00'
updated: '2026-06-26T12:20:00+08:00'
incident_id: TC-2026-037
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: ai-inference
ticket_type: 调度故障
skill_ref:
- Pod Pending 排查
- 资源调度治理
fta_ref:
- 'FTA: Pod Pending'
last_updated: 2026-06-26 12:20:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod Pending：节点 taint 与 toleration 不匹配 如何处理
trigger_keywords:
- ack
- zyy
- pod-pending
- taint
- toleration
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
- target: '[[domain-17-system-foundation/topic-dictionary/scheduling/toleration.md]]'
  type: related_to
- target: '[[domain-17-system-foundation/topic-dictionary/scheduling/taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在 ACK 专有云集群 `ack-zyy-prod-04` 的 `ai-inference` 命名空间部署新的 GPU 推理服务后，所有 Pod 长期处于 `Pending` 状态。客户描述如下：

> “我们新上线的 llm-inference 服务 4 个 Pod 全部 Pending，kubectl describe pod 看到 `0/8 nodes are available: 8 node(s) had untolerated taint {dedicated: gpu-inference}`. 节点状态都是 Ready，GPU 资源也看得到。是不是我们 Deployment 写错了？这个服务要跑在 GPU 节点上，麻烦帮忙看一下。”

该服务为在线大模型推理，业务计划今日灰度上线，当前因 Pod 无法调度导致上线阻塞。

## 分类与优先级判定

- **工单类型**：调度故障 / Pod Pending。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境新服务上线受阻，但已有业务未受影响，未达到全集群不可用级别。
2. 报错明确指向节点 taint 与 Pod toleration 不匹配，属于调度层配置问题。
3. 需要在 30 分钟内给出修复方案，确保业务按时灰度。

## 诊断步骤

按“先 Pod 事件、后节点污点、再工作负载配置”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pending Pod 状态与事件
kubectl get pod -n ai-inference -o wide
kubectl describe pod -n ai-inference llm-inference-7d9c4f8b5-xk2z9 | grep -A 30 Events

# 2. 查看节点列表及 taint 信息
kubectl get nodes -o custom-columns='NAME:.metadata.name,TAINTS:.spec.taints[*].key=.spec.taints[*].value,READY:.status.conditions[?(@.type=="Ready")].status'

# 3. 单独查看 GPU 节点详情
kubectl describe node cn-zhangjiakou.172.16.4.21 | grep -A 10 Taints
kubectl describe node cn-zhangjiakou.172.16.4.22 | grep -A 10 Taints

# 4. 检查 Deployment 中是否配置了 toleration
kubectl get deployment llm-inference -n ai-inference -o yaml | grep -A 20 tolerations

# 5. 检查节点标签与 Pod 节点亲和性配置
kubectl get node --show-labels | grep gpu
kubectl get deployment llm-inference -n ai-inference -o jsonpath='{.spec.template.spec.affinity}' | python3 -m json.tool

# 6. 查看调度器日志（可选，用于排除 scheduler 异常）
kubectl logs -n kube-system -l component=kube-scheduler --tail=100 | grep -i "llm-inference|FailedScheduling" | tail -30

# 7. 检查 ResourceQuota 与 LimitRange 是否拦截
kubectl get resourcequota -n ai-inference
kubectl describe resourcequota -n ai-inference
```
## 根因分析

GPU 节点池在创建时被平台团队打上了 `dedicated=gpu-inference:NoSchedule` 的 taint，用于隔离 GPU 推理负载与普通计算负载。客户提交的 `llm-inference` Deployment 中只配置了 `nodeSelector: {node-type: gpu-inference}`，但未配置对应的 `tolerations`，因此 scheduler 无法将 Pod 调度到 GPU 节点。

报错信息明确：

```
0/8 nodes are available: 8 node(s) had untolerated taint {dedicated: gpu-inference}
```

根因置信度：**高**。

### 风险与影响评估

- **业务影响：** `llm-inference` 服务无法上线，导致今日大模型推理灰度计划推迟，影响新业务功能发布与推理能力验证。
- **扩散风险：** 同一集群中其他 GPU 推理服务若使用相同 Deployment 模板，可能普遍存在 toleration 缺失问题，需要批量检查与修复。
- **数据风险：** 不涉及数据丢失，但推理服务延迟上线可能导致相关业务指标未达预期。
- **恢复关键：** 在 Pod 配置中补充与节点 taint 匹配的 toleration，而非移除节点 taint，以保持 GPU 节点隔离策略并避免普通负载抢占 GPU 资源。

## 修复命令

**第一步：为 Deployment 增加 toleration（推荐）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment llm-inference -n ai-inference --type=json -p='[
  {
    "op": "add",
    "path": "/spec/template/spec/tolerations",
    "value": [
      {
        "key": "dedicated",
        "operator": "Equal",
        "value": "gpu-inference",
        "effect": "NoSchedule"
      }
    ]
  }
]'
```
**第二步：等待滚动更新完成**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status deployment llm-inference -n ai-inference --timeout=300s
```
**第三步：如果业务急需，可临时移除节点 taint（不推荐长期保留）**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

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
# 仅在紧急测试时使用，生产环境应优先使用 toleration
# kubectl taint nodes -l node-type=gpu-inference dedicated=gpu-inference:NoSchedule-
```
**第四步：将 toleration 固化到 GitOps 仓库**

```bash
# 导出当前 Deployment 并提交到 Git
cat <<'EOF' > /tmp/llm-inference-deployment-patch.yaml
spec:
  template:
    spec:
      tolerations:
      - key: "dedicated"
        operator: "Equal"
        value: "gpu-inference"
        effect: "NoSchedule"
      nodeSelector:
        node-type: gpu-inference
EOF
# 由用户合并到业务 GitOps 仓库
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 Pod 已调度并 Running
kubectl get pod -n ai-inference -o wide

# 2. 确认 Pod 运行在 GPU 节点上
kubectl get pod -n ai-inference -l app=llm-inference -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}'

# 3. 检查 toleration 已生效
kubectl get pod -n ai-inference -l app=llm-inference -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.tolerations}{"\n"}{end}'

# 4. 确认 GPU 资源已分配
kubectl exec -n ai-inference deploy/llm-inference -- nvidia-smi -L 2>/dev/null || echo "请确认镜像包含 nvidia-smi"

# 5. 测试推理服务端口连通性
kubectl run scheduler-test --rm -it --restart=Never -n default --image=registry.aliyuncs.com/acs/busybox -- \
  wget -qO- --timeout=5 http://llm-inference.ai-inference.svc.cluster.local:8000/health

# 6. 检查调度事件无 FailedScheduling
kubectl get events -n ai-inference --field-selector reason=FailedScheduling --sort-by='.lastTimestamp' | tail -10
```
## 回复客户话术

> 您好，工单 TC-2026-037 已处理完成。
>
> **现象确认：** `ai-inference/llm-inference` 的 4 个 Pod 全部 Pending，`kubectl describe pod` 提示 `untolerated taint {dedicated: gpu-inference}`。
>
> **根因：** 集群 GPU 节点被打上了 `dedicated=gpu-inference:NoSchedule` 的 taint，用于隔离 GPU 负载；但 `llm-inference` Deployment 中只配置了 `nodeSelector`，未配置对应的 `tolerations`，导致 scheduler 无法将 Pod 调度到 GPU 节点。
>
> **已执行修复：**
> 1. 为 `llm-inference` Deployment 增加 toleration，匹配 GPU 节点 taint；
> 2. 触发滚动更新，Pod 已成功调度到 GPU 节点并 Running；
> 3. 提供 Deployment patch，建议固化到 GitOps 仓库。
>
> **当前状态：** 4 个推理 Pod 全部 Running，服务健康检查通过，推理端口可正常访问。
>
> **后续建议：**
> - 在 GPU 类 Deployment 模板中统一添加 `dedicated=gpu-inference` 的 toleration，避免重复踩坑；
> - 建议平台团队将节点 taint 与 toleration 规范写入 资源调度治理 文档，并在 CI 中校验 GPU 工作负载是否包含必要 toleration；
> - 上线前在预发环境验证 Pod 调度路径，确认 `nodeSelector` 与 `tolerations` 同时生效；
> - 对关键服务设置 PodDisruptionBudget，避免滚动更新期间可用副本不足。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障是 Kubernetes 调度中 taint/toleration 机制的典型误用场景。很多业务团队在申请到带 taint 的 GPU 节点池后，只关注 `nodeSelector` 或 `nodeAffinity`，却忽略了 `NoSchedule` taint 需要显式 toleration 才能调度。这种错误在普通节点池上不会出现，因此业务在预发环境（若无 GPU 节点池或节点无 taint）往往无法提前发现。

在排障过程中，`kubectl describe pod` 输出的 `0/8 nodes are available: 8 node(s) had untolerated taint` 已经直接给出了根因方向，但部分同学会误以为这是节点资源不足或 GPU 驱动问题，从而浪费时间去检查 nvidia-device-plugin 或节点资源。因此，遇到 Pod Pending 时，应优先阅读 scheduler 给出的不可用原因，而不是盲目检查节点状态。

建议平台团队在 GPU 节点池交付文档中明确写出：
1. 节点 taint 的 key/value/effect；
2. 业务 Deployment 必须包含的 toleration 示例；
3. 推荐同时配置 `nodeSelector` 与 `tolerations` 的 YAML 模板。

同时，建议在 CI 或准入控制器（如 Kyverno/OPA Gatekeeper）中增加策略校验：对于带有 `node-type: gpu-inference` 的 Pod，必须同时包含 `dedicated=gpu-inference:NoSchedule` 的 toleration，否则拦截发布。这样可以从源头避免同类问题。

此外，对于 AI 推理类服务，建议在预发环境搭建与生产一致的 GPU 节点池（至少包含 taint 配置），确保上线前调度路径已被验证。可参考 资源调度治理 建立 GPU 工作负载发布 checklist。

最后，建议在值班手册中补充 Pod Pending 的快速排查决策树：第一步看 `kubectl describe pod` 中的 Events，第二步判断是资源不足、污点不匹配、亲和性冲突、PVC 未绑定还是权限问题。对于 GPU 场景，优先检查 `nvidia.com/gpu` 资源请求、`nodeSelector`、`tolerations` 以及 device plugin 状态，可以显著缩短定位时间。

## 是否需要升级及交接信息

- **是否升级**：否（已闭环）。若发现多个 GPU 服务均存在同类 toleration 缺失，需升级至 **平台工程团队** 统一修复模板并补充治理规范。
- **交接信息**：
  - 故障单号：`TC-2026-037`
  - 根因：GPU 节点 taint 与 Pod toleration 不匹配
  - 影响集群：`ack-zyy-prod-04`
  - 影响命名空间：`ai-inference`
  - 临时修复：为 Deployment 增加 toleration
  - 长期方案：在 GitOps 模板与 CI 校验中固化 toleration 规范
  - 待跟进：确认业务灰度上线结果，检查其他 GPU 服务是否存在同类问题

## Related

- 容忍
- 污点
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502
- 污点
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
