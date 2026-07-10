---
title: 阿里云专有云 DaemonSet 未在所有节点运行（日志采集 Agent 缺失）
description: Logtail 日志采集 DaemonSet 在新扩容节点与部分存量节点上未运行，导致业务日志漏采，根因为节点污点、镜像拉取失败与资源不足叠加，含诊断、修复与验证。
summary: Logtail 日志采集 DaemonSet 在新扩容节点与部分存量节点上未运行，导致业务日志漏采，根因为节点污点、镜像拉取失败与资源不足叠加，含诊断、修复与验证。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- daemonset
- logtail
- observability
- taint
- image-pull
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-025
priority: P1
severity: high
affected_cluster: ack-prod-vpc02
affected_namespace: kube-system
ticket_type: 可观测性组件故障
skill_ref: DaemonSet 诊断
fta_ref: 'FTA: DaemonSet 未全节点运行'
last_updated: 2026-06-26
duplicate_of: INC-2026-ACK-050
status: duplicate
duplication_reason: 与 "INC-2026-ACK-050" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 DaemonSet 未在所有节点运行（日志采集 Agent 缺失） 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- daemonset
- logtail
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
- target: '[[concepts/daemonset.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-050-daemonset-not-running-all-nodes.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-030-daemonset-not-ready-all-nodes.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 025：DaemonSet 未在所有节点运行（日志采集 Agent 缺失）

## 1. 工单描述

**用户原始描述：**

> 我们阿里云专有云 ACK 集群用的是 SLS Logtail 采集容器日志，namespace 是 kube-system。最近安全团队做了一次节点加固，给部分节点打了污点。今天我们发现新扩容的几台节点上业务日志没有采集到 SLS，存量节点上也有部分日志缺失。kubectl get ds 看到 logtail-ds 的 Desired 和 Ready 数量对不上，有几个 Pod 一直 ImagePullBackOff。业务那边投诉说排查问题看不到日志。麻烦尽快帮忙看一下，现在可观测性受影响。

## 2. 分类与优先级判定

- **任务类型：** 可观测性组件故障 / DaemonSet 异常 / 日志采集缺失
- **优先级：** P1（生产环境 + 日志采集缺失 + 影响故障排查）
- **严重程度：** high
- **响应时限：** 15 分钟内给出修复方案
- **安全级别：** 中风险（涉及节点污点与系统级 DaemonSet 变更，需确认影响范围）

## 3. 诊断步骤

### 3.1 查看 DaemonSet 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 logtail DaemonSet 整体状态
kubectl get daemonset logtail-ds -n kube-system

# 查看详细状态与事件
kubectl describe daemonset logtail-ds -n kube-system

# 查看所有 Pod 状态分布
kubectl get pod -n kube-system -l app=logtail-ds -o wide
```
### 3.2 查看异常 Pod 详情

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 ImagePullBackOff Pod 的事件
kubectl describe pod logtail-ds-abcde -n kube-system

# 查看 Pod 日志
kubectl logs -n kube-system -l app=logtail-ds --tail=100
kubectl logs -n kube-system -l app=logtail-ds --previous --tail=100
```
### 3.3 检查节点污点与标签

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点污点
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints,READY:.status.conditions[?(@.type=='Ready')].status

# 查看未调度到 DaemonSet Pod 的节点
kubectl get node -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | while read node; do
  count=$(kubectl get pod -n kube-system -l app=logtail-ds --field-selector spec.nodeName=$node --no-headers | wc -l)
  if [ "$count" -eq 0 ]; then
    echo "No logtail-ds Pod on $node"
  fi
done
```
### 3.4 检查镜像拉取与仓库认证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看异常 Pod 使用的镜像
kubectl get pod logtail-ds-abcde -n kube-system -o jsonpath='{.spec.containers[*].image}'

# 在异常节点上查看镜像拉取事件
kubectl get events -n kube-system --field-selector reason=FailedPullImage --sort-by='.lastTimestamp' | tail -30

# 检查 imagePullSecret 是否存在
kubectl get secret -n kube-system | grep regcred
```
### 3.5 检查节点资源

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看未运行 Pod 节点的资源
kubectl top node

# 查看节点 allocatable 与已分配资源
kubectl describe node <problem-node>
```
### 3.6 检查阿里云 SLS 与 Logtail 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Logtail 配置 ConfigMap
kubectl get configmap -n kube-system | grep logtail
kubectl get configmap logtail-config -n kube-system -o yaml

# 通过阿里云 CLI 查看 SLS 项目状态
aliyun log GetProject --project-name k8s-logs-prod
```
### 3.7 诊断过程补充说明

DaemonSet 的调度逻辑与 Deployment 不同，它不是通过 ReplicaSet 控制副本数，而是为每个符合条件的节点创建一个 Pod。因此 DaemonSet 的 Desired 数量等于符合条件的节点数，而 Ready 数量表示实际成功运行的 Pod 数。当 Desired != Ready 时，需要分别排查 "为什么没调度" 与 "为什么调度后没运行成功" 两个方向。

"没调度" 最常见的原因是节点污点（Taint）与 DaemonSet 容忍（Toleration）不匹配。Kubernetes 默认会给 master 节点打上 `node-role.kubernetes.io/master:NoSchedule` 污点，系统级 DaemonSet 通常会默认容忍该污点。但如果安全团队或运维人员额外添加了自定义污点，而 DaemonSet 未同步更新，就会出现部分节点遗漏。

"调度后没运行成功" 最常见的原因包括镜像拉取失败、资源不足、启动命令错误、配置挂载失败等。对于镜像拉取失败，需要区分是私有仓库认证问题还是镜像本身不存在；对于资源不足，需要查看节点上是否已分配资源过多，导致 DaemonSet Pod 无法被调度或被驱逐。kubelet 驱逐时会参考 Pod 的 QoS 等级与 PriorityClass，系统级 DaemonSet 建议设置较高优先级。

在阿里云 ACK 专有云环境中，Logtail DaemonSet 通常由 ACK 集群创建并管理，其镜像存放在阿里云私有仓库。新扩容节点如果未正确配置容器运行时认证或 imagePullSecret，会普遍出现 ImagePullBackOff。建议在节点初始化脚本或节点池配置中统一注入仓库认证，而不是依赖每个 DaemonSet 单独配置。

## 4. 根因分析

综合 DaemonSet 状态、Pod 事件、节点污点与镜像拉取情况，判定根因为 **"安全加固后新增的节点污点未被 logtail-ds 容忍，导致部分节点未调度；新扩容节点因 imagePullSecret 缺失无法拉取私有仓库镜像；部分存量节点因 CPU/内存资源不足导致 DaemonSet Pod 被驱逐"**，置信度 **高**。

1. **污点未容忍：** 安全团队加固后给部分节点打了 `security-hardened=true:NoSchedule` 污点，logtail-ds 未配置对应 toleration。
2. **镜像拉取失败：** 新扩容节点未同步 imagePullSecret，拉取私有镜像 `registry-vpc.cn-shanghai.aliyuncs.com/acs/logtail` 失败，Pod 处于 ImagePullBackOff。
3. **资源不足：** 部分旧节点资源使用率长期高位，Logtail Pod 被 kubelet 驱逐。

### 4.1 风险与影响评估

- **业务影响：** 日志采集缺失导致故障排查困难，安全审计与业务分析缺少数据支撑，问题定位时间显著延长。
- **扩散风险：** 其他系统级 DaemonSet（如 Prometheus Node Exporter、Falco、 security agent）可能也存在相同问题，可观测性与安全防护面同时收缩。
- **数据风险：** 已缺失的日志无法补采，需依赖应用自身日志落盘保留策略，若应用日志仅保留 7 天，超过窗口后彻底无法追溯。
- **合规风险：** 安全审计与等保合规要求日志完整留存，采集缺失可能导致审计不达标。
- **运维风险：** 节点污点与镜像认证变更未同步到系统 DaemonSet，反映变更管理流程存在缺口。

## 5. 修复命令

### 5.1 临时缓解：为异常节点添加容忍或删除污点

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
# 方案 A：给 logtail-ds 添加污点容忍（推荐）
cat <<'EOF' | kubectl patch daemonset logtail-ds -n kube-system --type=merge --patch-file=/dev/stdin
spec:
  template:
    spec:
      tolerations:
        - key: "security-hardened"
          operator: "Equal"
          value: "true"
          effect: "NoSchedule"
        - key: "dedicated"
          operator: "Exists"
          effect: "NoSchedule"
        - operator: "Exists"
          effect: "NoExecute"
EOF

# 方案 B：若污点误加，可删除污点（不推荐用于生产加固节点）
# kubectl taint node <node-name> security-hardened=true:NoSchedule-
```
### 5.2 修复镜像拉取认证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 imagePullSecret（若缺失）
kubectl create secret docker-registry aliyun-regcred \
  --docker-server=registry-vpc.cn-shanghai.aliyuncs.com \
  --docker-username=<your-username> \
  --docker-password=<your-password> \
  -n kube-system

# 将 imagePullSecret 绑定到 DaemonSet ServiceAccount
kubectl patch serviceaccount default -n kube-system -p '{"imagePullSecrets": [{"name": "aliyun-regcred"}]}'

# 或直接在 DaemonSet 中引用 imagePullSecret
cat <<'EOF' | kubectl patch daemonset logtail-ds -n kube-system --type=merge --patch-file=/dev/stdin
spec:
  template:
    spec:
      imagePullSecrets:
        - name: aliyun-regcred
EOF
```
### 5.3 提升 DaemonSet 资源优先级

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为 logtail-ds 设置较高 PriorityClass，减少被驱逐概率
cat <<'EOF' | kubectl apply -f -
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: system-logging-critical
value: 2000001000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "Critical system logging DaemonSet"
EOF

cat <<'EOF' | kubectl patch daemonset logtail-ds -n kube-system --type=merge --patch-file=/dev/stdin
spec:
  template:
    spec:
      priorityClassName: system-logging-critical
EOF
```
### 5.4 滚动重启 DaemonSet

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 滚动重启 logtail-ds
kubectl rollout restart daemonset logtail-ds -n kube-system

# 观察重启进度
kubectl rollout status daemonset logtail-ds -n kube-system --timeout=300s
```
## 6. 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 DaemonSet 所有节点都有 Pod
kubectl get daemonset logtail-ds -n kube-system

# 2. 确认每个节点上都有 Running 的 logtail Pod
kubectl get pod -n kube-system -l app=logtail-ds -o wide

# 3. 统计无 Pod 的节点
for node in $(kubectl get node -o jsonpath='{.items[*].metadata.name}'); do
  count=$(kubectl get pod -n kube-system -l app=logtail-ds --field-selector spec.nodeName=$node --no-headers | wc -l)
  [ "$count" -eq 0 ] && echo "Missing: $node"
done

# 4. 查看异常 Pod 日志已无错误
kubectl logs -n kube-system -l app=logtail-ds --tail=50

# 5. 在业务 Pod 上生成测试日志并确认 SLS 能采集
kubectl run log-test --rm -it --restart=Never -n default --image=registry-vpc.cn-shanghai.aliyuncs.com/acs/busybox:latest -- \
  sh -c 'for i in $(seq 1 10); do echo "[TEST] logtail verification $i $(date)"; done'

# 6. 通过阿里云 CLI 查询 SLS 最近日志
aliyun log GetLogs \
  --project-name=k8s-logs-prod \
  --logstore=k8s-container-log \
  --from=$(date -u -v-10M +%s) \
  --to=$(date -u +%s) \
  --query='* | select count(*) as total'
```
## 7. 回复客户话术

> 您好，工单 TC-2026-025 已处理完成。
>
> **现象确认：** kube-system namespace 下 Logtail DaemonSet `logtail-ds` 的 Desired 与 Ready 数量不一致，新扩容节点与部分存量节点无日志采集 Pod，业务日志漏采到 SLS。
>
> **根因：**
> 1. 安全团队节点加固后新增了 `security-hardened=true:NoSchedule` 污点，logtail-ds 未配置 toleration；
> 2. 新扩容节点缺少私有镜像仓库的 imagePullSecret，Logtail 镜像拉取失败处于 ImagePullBackOff；
> 3. 部分存量节点资源紧张，Logtail Pod 被 kubelet 驱逐。
>
> **已执行修复：**
> 1. 为 logtail-ds 添加 security-hardened 等关键污点容忍；
> 2. 创建并绑定阿里云私有镜像仓库 imagePullSecret；
> 3. 创建高优先级 PriorityClass 并绑定到 logtail-ds，降低被驱逐概率；
> 4. 滚动重启 DaemonSet，确认所有节点 Running。
>
> **当前状态：** logtail-ds Desired = Ready，所有节点均有日志采集 Pod 运行，SLS 已能正常收到测试日志。
>
> **后续建议：**
> - 对所有系统级 DaemonSet 统一审计 tolerations、imagePullSecrets 与资源请求；
> - 节点加固或扩容时，同步更新 DaemonSet 容忍与认证配置；
> - 建立 DaemonSet 覆盖率监控告警，Desired != Ready 时立即通知；
> - 在 GitOps 中固化系统 DaemonSet 模板，避免手动遗漏；
> - 对关键可观测性组件设置独立节点池或更高资源保障。
>
> 如有异常请随时联系。

## 8. 是否需要升级及交接信息

- **是否升级：** 否（已闭环）
- **是否需要变更审批：** 是（系统级 DaemonSet 配置变更已记录变更台账）
- **交接信息：**
  - 已将修复后的 logtail-ds 配置提交至 GitOps 仓库；
  - 建议安全团队与运维团队建立节点污点变更同步机制；
  - 若其他系统 DaemonSet（如 Prometheus Node Exporter、Falco）存在同类问题，建议按本案例模板批量修复；
  - 本案例已沉淀至可观测性组件故障知识库，供后续 DaemonSet 排查参考。

---

*更新时间：2026-06-26 | 责任域：domain-11-production-operations/ticket-cases*

## Related

- DaemonSet
- DaemonSet 未在所有节点运行：日志采集 Agent 缺失
- DaemonSet 未在所有节点运行：Logtail 多架构与污点容忍缺失
- DaemonSet 未在所有节点运行：日志采集 Agent 缺失
- DaemonSet 未在所有节点运行：Logtail 多架构与污点容忍缺失


<!-- risk-assessed -->
