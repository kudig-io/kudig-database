---
title: DaemonSet 未在所有节点运行：Logtail 多架构与污点容忍缺失
description: 专有云 ACK 集群 Logtail DaemonSet 因镜像不支持 ARM 架构且未容忍主节点污点，导致部分节点日志采集中断的工单闭环样本。
summary: 专有云 ACK 集群 Logtail DaemonSet 因镜像不支持 ARM 架构且未容忍主节点污点，导致部分节点日志采集中断的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- daemonset
- logtail
- arm
- taint
- p1
- observability
tier: peripheral
created: '2026-06-26T06:30:00+08:00'
updated: '2026-06-26T09:45:00+08:00'
incident_id: INC-2026-ACK-030
priority: P1
severity: high
affected_cluster: ack-zyy-prod-07
affected_namespace: monitoring
ticket_type: 可观测性组件异常
skill_ref:
- DaemonSet 未就绪诊断
- Logtail Kubernetes 采集
fta_ref:
- 'FTA: DaemonSet 未覆盖全部节点'
last_updated: 2026-06-26 09:45:00+08:00
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
- DaemonSet 未在所有节点运行：Logtail 多架构与污点容忍缺失 如何处理
trigger_keywords:
- DaemonSet
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
- target: '[[生产运维/工单案例/ticket-case-050-daemonset-not-running-all-nodes.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈其专有云 ACK 集群 `ack-zyy-prod-07` 中部分节点的应用日志无法被 SLS Logtail 采集到，阿里云 SLS 控制台显示日志产生量明显低于预期。客户描述如下：

> “我们新加了一个 ARM 节点池后，发现那个节点池上的 Pod 日志在 SLS 里看不到。kubectl get ds 看 logtail-ds 的 Desired 和 Ready 不一致，有些节点上 Pod 是 CrashLoopBackOff，有些是 Pending。麻烦排查一下。”

该集群日志采集链路为业务排障与审计合规的关键基础设施，部分节点采集中断会影响故障定位与安全审计。

## 分类与优先级判定

- **工单类型**：可观测性组件异常 / DaemonSet 未覆盖。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境日志采集链路部分失效，影响故障排查与合规审计。
2. 问题集中在 DaemonSet 调度与镜像兼容性，修复后可快速恢复全节点覆盖。
3. 涉及新扩容节点池，属于变更引入的问题，需闭环并沉淀到变更检查清单。

## 诊断步骤

按“先 DaemonSet 状态、再节点差异与架构、最后 Pod 事件与日志”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 DaemonSet 整体状态
kubectl get daemonset logtail-ds -n monitoring -o wide

# 2. 查看所有 logtail Pod 状态与所在节点
kubectl get pod -n monitoring -l k8s-app=logtail -o wide

# 3. 查看异常 Pod 的事件与退出原因
kubectl describe pod -n monitoring logtail-ds-xxxxx | tail -80
kubectl get events -n monitoring --field-selector reason=BackOff --sort-by='.lastTimestamp'

# 4. 查看异常 Pod 日志
kubectl logs -n monitoring logtail-ds-xxxxx --tail=200
kubectl logs -n monitoring logtail-ds-xxxxx --previous --tail=200

# 5. 查看节点架构、污点与标签
kubectl get nodes -L kubernetes.io/arch,kubernetes.io/os,nodepool
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,ARCH:.status.nodeInfo.architecture,TAINTS:.spec.taints[*].key'

# 6. 查看 DaemonSet 的 nodeSelector 与 tolerations
kubectl get daemonset logtail-ds -n monitoring -o yaml | grep -A 30 "nodeSelector:|tolerations:"

# 7. 检查镜像 manifest 支持的平台
docker manifest inspect registry-vpc.cn-hangzhou.aliyuncs.com/log-service/logtail:latest 2>/dev/null | jq '.manifests[].platform'

# 8. 查看 LimitRange 与 ResourceQuota
kubectl get limitrange -n monitoring
kubectl get resourcequota -n monitoring
```
## 根因分析

经排查，发现 `logtail-ds` DaemonSet 未能在所有节点就绪的原因有两个：

1. **镜像不支持 ARM64 架构**。客户新扩容的节点池 `np-zyy-arm` 使用 `ecs.c8y.xlarge`（ARM 架构）。当前 DaemonSet 使用的镜像 `registry-vpc.cn-hangzhou.aliyuncs.com/log-service/logtail:v1.5.0-amd64` 仅包含 AMD64 manifest，Pod 调度到 ARM 节点后启动时报错：

   ```
   exec /usr/local/ilogtail/ilogtail: exec format error
   CrashLoopBackOff
   ```

2. **未容忍主节点污点**。控制平面节点（Master）带有 `node-role.kubernetes.io/master:NoSchedule` 或 `node-role.kubernetes.io/control-plane:NoSchedule` 污点，而 `logtail-ds` 未配置对应 tolerations，因此 Master 节点上没有 logtail Pod。虽然 Master 上业务 Pod 较少，但部分系统组件日志仍需采集，且 Desired 与 Ready 不一致会持续触发监控告警。

`kubectl get daemonset logtail-ds -n monitoring` 输出：

```
NAME        DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR   AGE
logtail-ds  15        13        11      13           11          <none>          180d
```

根本原因是 **DaemonSet 镜像未适配多架构，且 tolerations 配置不完整**。

## 修复命令

**第一步：确认节点架构分布**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.architecture}{"\t"}{.spec.taints[*].key}{"\n"}{end}'
```
**第二步：将 DaemonSet 镜像切换为支持多架构的统一标签**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl set image daemonset/logtail-ds -n monitoring \
  logtail=registry-vpc.cn-hangzhou.aliyuncs.com/log-service/logtail:v1.5.0
```
> `v1.5.0` 为包含 AMD64 与 ARM64 manifest 的多架构镜像。若官方未提供，可分别按架构使用 nodeSelector 部署两个 DaemonSet。

**第三步：为主节点污点与专用节点污点添加 tolerations**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch daemonset logtail-ds -n monitoring --type='merge' -p '{
  "spec": {
    "template": {
      "spec": {
        "tolerations": [
          {
            "key": "node-role.kubernetes.io/master",
            "operator": "Exists",
            "effect": "NoSchedule"
          },
          {
            "key": "node-role.kubernetes.io/control-plane",
            "operator": "Exists",
            "effect": "NoSchedule"
          },
          {
            "key": "dedicated",
            "operator": "Equal",
            "value": "gpu",
            "effect": "NoSchedule"
          }
        ]
      }
    }
  }
}'
```
**第四步：如官方镜像不支持 ARM，可按架构分别部署 DaemonSet**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 为 AMD64 节点保留原 DaemonSet，并增加 nodeSelector
kubectl patch daemonset logtail-ds -n monitoring --type='merge' -p '{
  "spec": {
    "template": {
      "spec": {
        "nodeSelector": {
          "kubernetes.io/arch": "amd64"
        }
      }
    }
  }
}'

# 2. 新建 ARM64 专用 DaemonSet
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: logtail-ds-arm64
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: logtail
      arch: arm64
  template:
    metadata:
      labels:
        k8s-app: logtail
        arch: arm64
    spec:
      nodeSelector:
        kubernetes.io/arch: arm64
      tolerations:
        - key: node-role.kubernetes.io/master
          operator: Exists
          effect: NoSchedule
        - key: node-role.kubernetes.io/control-plane
          operator: Exists
          effect: NoSchedule
      containers:
        - name: logtail
          image: registry-vpc.cn-hangzhou.aliyuncs.com/log-service/logtail:v1.5.0-arm64
EOF
```
**第五步：滚动更新并观察 DaemonSet 状态**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout status daemonset logtail-ds -n monitoring --timeout=300s
kubectl rollout status daemonset logtail-ds-arm64 -n monitoring --timeout=300s
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. DaemonSet 的 DESIRED == CURRENT == READY
kubectl get daemonset logtail-ds -n monitoring
kubectl get daemonset logtail-ds-arm64 -n monitoring

# 2. 每个节点上都有 logtail Pod 且状态 Running
kubectl get pod -n monitoring -l k8s-app=logtail -o wide

# 3. ARM 节点上的 logtail Pod 不再报 exec format error
kubectl logs -n monitoring -l k8s-app=logtail,arch=arm64 --tail=50

# 4. Master 节点上也有 logtail Pod
kubectl get pod -n monitoring -l k8s-app=logtail -o jsonpath='{range .items[*]}{.spec.nodeName}{"\t"}{.status.phase}{"\n"}{end}'

# 5. SLS 控制台查看日志采集量恢复
# 登录阿里云控制台 -> 日志服务 -> Project -> Logstore -> 查询统计

# 6. 在 ARM 节点上的业务 Pod 产生测试日志并确认采集
kubectl exec -n demo deploy/arm-app-demo -- sh -c 'echo "logtail-test-arm $(date)" >> /var/log/app/test.log'
# 在 SLS 控制台查询：* and "logtail-test-arm"
```
## 回复客户话术

> 您好，经排查，部分节点日志未被 Logtail 采集的根因是：
>
> 1. **新扩容的 ARM 节点池使用的 logtail 镜像仅支持 AMD64 架构**，Pod 启动时报 `exec format error` 并进入 CrashLoopBackOff；
> 2. **DaemonSet 未配置主节点污点 toleration**，导致 Master 节点未部署 logtail Pod。
>
> 我们已完成以下处置：
> - 将 logtail 镜像切换为支持多架构的统一版本；
> - 为 DaemonSet 添加 `node-role.kubernetes.io/master`、`node-role.kubernetes.io/control-plane` 等污点容忍；
> - 若官方镜像暂不支持 ARM，已额外部署 `logtail-ds-arm64` 专用 DaemonSet；
> - 验证所有节点 logtail Pod 均已 Running，SLS 日志采集量恢复正常。
>
> 建议后续：
> - 在新增异构节点池前，确认关键 DaemonSet（监控、日志、安全）镜像支持目标架构；
> - 参考 Logtail Kubernetes 采集 维护 DaemonSet 多架构部署规范；
> - 配置 DaemonSet 未就绪告警。
>
> 如有日志缺失或延迟问题，请随时联系。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若涉及 ACK 托管版 Master 节点日志采集权限限制，需升级至 **ACK 产品支持** 与 **SLS 团队** 确认。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-030`
  - 根因：`logtail 镜像不支持 ARM 架构 + DaemonSet 未容忍主节点污点`
  - 影响集群：`ack-zyy-prod-07`
  - 影响命名空间：`monitoring`
  - 影响组件：DaemonSet `logtail-ds`
  - 临时修复：切换多架构镜像 + 补充 tolerations + 部署 ARM64 专用 DaemonSet
  - 长期方案：建立异构节点池上线检查清单，统一 DaemonSet 多架构策略
  - 待跟进：确认 SLS 侧所有 Logstore 数据完整性，观察 24 小时采集延迟

## 复盘与沉淀

本次故障是 **异构计算（ARM/AMD 混合）节点池** 引入后，可观测性 Agent 未同步适配的典型案例。随着 ARM 实例在成本敏感场景越来越普及，监控、日志、安全、CNI 等基础设施 DaemonSet 必须提前完成多架构验证。

复盘要点：
1. **镜像多架构化**：核心 DaemonSet 应使用支持 `linux/amd64` 与 `linux/arm64` 的多架构镜像，并通过 CI 流水线自动构建和推送 manifest list。
2. **污点容忍标准化**：Master 节点、GPU 节点、专用节点通常带有污点，DaemonSet 需要根据业务需求决定是否覆盖。建议将常见 tolerations 作为 DaemonSet 模板默认配置。
3. **变更前检查清单**：新增节点池时，除了确认业务 Pod 可调度，还需检查所有 DaemonSet 的 nodeSelector、tolerations、镜像架构是否匹配。
4. **可观测性覆盖完整性**：不能仅看 `kubectl get ds` 的 READY 数，还需按节点维度核对每个节点是否都有对应 Pod，并按架构分组验证。
5. **镜像版本与 manifest 校验**：在更新 DaemonSet 镜像前，应使用 `docker manifest inspect` 或 `crane manifest` 验证镜像是否包含目标平台。对于只提供 AMD64 版本的第三方 Agent，应提前规划 ARM64 替代方案或单独维护 ARM64 DaemonSet。
6. **GitOps 审计**：应将 DaemonSet 镜像版本与架构支持情况纳入 GitOps 审计流水线，确保每次升级不会引入新的平台兼容风险。

后续 SOP 更新要点：
- 将 DaemonSet 多架构检查写入 节点池上线检查清单；
- 在 Prometheus 中配置告警：`kube_daemonset_status_number_ready / kube_daemonset_status_desired_number_scheduled < 1` 持续 5 分钟触发 P1；
- 将本案例写入 DaemonSet 未就绪回复模板，提升一线响应效率。

## Related

- DaemonSet
- DaemonSet 未在所有节点运行：日志采集 Agent 缺失
- Pod Pending：资源不足与 Taint 不匹配


<!-- risk-assessed -->
