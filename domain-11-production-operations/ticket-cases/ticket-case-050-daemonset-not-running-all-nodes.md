---
title: DaemonSet 未在所有节点运行：日志采集 Agent 缺失
description: 专有云 ACK 集群 Logtail DaemonSet 在部分节点未调度，导致业务日志采集中断，根因涉及节点 Taint、资源不足与镜像拉取异常的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- daemonset
- logtail
- observability
- taint
- p1
incident_id: INC-2026-ACK-050
priority: P1
severity: high
affected_cluster: ack-zyy-prod-08
affected_namespace: kube-system
ticket_type: 可观测性故障 / DaemonSet 调度异常
skill_ref:
- '[[domain-02-workloads-applications/00-core-workloads/04-daemonset-management.md|DaemonSet
  管理]]'
- Logtail 排障
- '[[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL.md|节点诊断
  Skill]]'
fta_ref:
- 'FTA: DaemonSet 未覆盖全部节点'
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T17:45:00+08:00'
last_updated: 2026-06-26T17:45:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- DaemonSet 未在所有节点运行：日志采集 Agent 缺失 如何处理
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
- target: "[[concepts/daemonset.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]"
  type: related_to
- target: "[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]"
  type: related_to
---

# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-08` 中发现 SLS Logtail 日志采集出现遗漏，部分节点的业务日志未上传到 SLS。客户描述如下：

> “我们最近在查一个线上问题，发现有些节点的业务日志在 SLS 里查不到，但应用本身是正常的。去 kubectl 看 logtail-ds 这个 DaemonSet，desired 是 50，current 只有 47，有 3 个节点上没有 logtail Pod。describe daemonset 也看不到特别明确的错误。麻烦看一下为什么 DaemonSet 没能在所有节点上跑起来。”

受影响命名空间为 `kube-system`，DaemonSet 名称为 `logtail-ds`。日志采集缺失影响线上问题定位与审计合规。

## 分类与优先级判定

- **工单类型**：可观测性故障 / DaemonSet 调度异常。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境日志采集 Agent 未全覆盖，导致部分节点日志缺失，影响故障排查与审计。
2. DaemonSet 调度失败通常由 Taint、资源、镜像等多因素导致，需要系统排查。
3. 未直接影响在线业务可用性，但属于可观测性基础设施降级，符合 P1 标准。

## 诊断步骤

按“先看 DaemonSet 状态，再看未调度节点特征，最后查 Pod 事件”的顺序排查：

```bash
# 1. 查看 DaemonSet 整体状态
kubectl get daemonset logtail-ds -n kube-system -o wide
kubectl describe daemonset logtail-ds -n kube-system | head -60

# 2. 对比期望数量与实际数量，定位缺失节点
kubectl get node -o name | wc -l
kubectl get pod -n kube-system -l k8s-app=logtail-ds -o wide | awk '{print $7}' | sort | uniq -c

# 3. 找出没有 logtail Pod 的节点
kubectl get node -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' > /tmp/all_nodes.txt
kubectl get pod -n kube-system -l k8s-app=logtail-ds -o jsonpath='{range .items[*]}{.spec.nodeName}{"\n"}{end}' | sort -u > /tmp/logtail_nodes.txt
comm -23 <(sort /tmp/all_nodes.txt) /tmp/logtail_nodes.txt

# 4. 检查缺失节点的 Taint、Label 与状态
for node in $(comm -23 <(sort /tmp/all_nodes.txt) /tmp/logtail_nodes.txt); do
  echo "=== $node ==="
  kubectl describe node $node | grep -E "Taints|Labels|Conditions" | head -10
done

# 5. 查看缺失节点上的 Pod 调度事件
kubectl get events -n kube-system --field-selector involvedObject.kind=Pod --sort-by='.lastTimestamp' | grep -i "logtail" | tail -30

# 6. 检查 DaemonSet 的 Toleration 配置
kubectl get daemonset logtail-ds -n kube-system -o yaml | grep -A 30 tolerations

# 7. 检查节点资源是否足够
kubectl describe node $(comm -23 <(sort /tmp/all_nodes.txt) /tmp/logtail_nodes.txt | head -1) | grep -A 10 "Allocated resources"

# 8. 检查镜像拉取情况
kubectl get events -n kube-system --field-selector reason=FailedToPullImage --sort-by='.lastTimestamp' | grep logtail
kubectl get events -n kube-system --field-selector reason=ImagePullBackOff --sort-by='.lastTimestamp' | grep logtail

# 9. 检查 Logtail 所需的 HostPath/ConfigMap 是否存在
kubectl get configmap logtail-config -n kube-system
kubectl get daemonset logtail-ds -n kube-system -o jsonpath='{.spec.template.spec.volumes}' | python3 -m json.tool

# 10. 通过 ACK 控制台查看节点池与组件状态
ack-cli node diagnose $(comm -23 <(sort /tmp/all_nodes.txt) /tmp/logtail_nodes.txt | head -1) --cluster ack-zyy-prod-08 --module daemonset
```

## 根因分析

通过对比有/无 Logtail Pod 的节点特征，确认存在以下三类原因：

**第一类：节点 Taint 未容忍（2 个节点）**

缺失节点上存在 Taint：

```
Taints:             observability=log-collection:NoSchedule
```

该 Taint 是运维团队为隔离部分测试节点手动添加的，但 `logtail-ds` 的 DaemonSet YAML 中未配置对应的 Toleration，导致 Pod 无法调度到这两个节点。

**第二类：节点 DiskPressure 导致 Pod 无法创建（1 个节点）**

节点状态显示：

```
Conditions:
  DiskPressure     True    ...    kubelet has disk pressure
```

该节点 `/var/log` 目录被业务日志占满，触发 `DiskPressure`。kubelet 拒绝创建新的 Pod（包括 Logtail DaemonSet Pod），导致该节点缺失日志采集 Agent。

**第三类：镜像拉取超时导致 Pod 反复 CrashLoop（已自愈，但历史原因）**

节点事件显示过去 24 小时内存在：

```
Warning  FailedToPullImage  ...  rpc error: code = DeadlineExceeded desc = failed to pull and unpack image: failed to resolve reference: deadline exceeded
```

部分节点因网络抖动拉取 Logtail 镜像超时，Pod 进入 `ImagePullBackOff`。虽然网络恢复后已自愈，但说明镜像拉取策略在专有云网络不稳定时存在风险。

## 修复命令

**第一步：为 Logtail DaemonSet 补充缺失的 Toleration**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch daemonset logtail-ds -n kube-system --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/template/spec/tolerations/-",
    "value": {
      "key": "observability",
      "operator": "Equal",
      "value": "log-collection",
      "effect": "NoSchedule"
    }
  }
]'
```

若希望 Logtail 在任何节点都运行，也可补充通用 toleration：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch daemonset logtail-ds -n kube-system --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/template/spec/tolerations/-",
    "value": {
      "operator": "Exists",
      "effect": "NoSchedule"
    }
  }
]'
```

**第二步：清理磁盘压力节点上的过期日志，恢复 kubelet 调度能力**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 登录到问题节点（假设节点名为 cn-zhangjiakou.172.16.8.12）
ssh root@cn-zhangjiakou.172.16.8.12

# 清理 7 天前的业务日志
find /var/log/containers /var/log/pods -type f -mtime +7 -delete

# 清理已退出的容器占用（可选，视情况而定）
crictl system prune -f

# 重启 kubelet 以尽快刷新 DiskPressure 状态
systemctl restart kubelet
```

在 Kubernetes 侧确认节点状态恢复：

```bash
kubectl describe node cn-zhangjiakou.172.16.8.12 | grep -A 5 Conditions
```

**第三步：为 Logtail 设置镜像拉取超时与重试策略**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch daemonset logtail-ds -n kube-system --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/template/spec/containers/0/imagePullPolicy",
    "value": "IfNotPresent"
  }
]'
```

同时在容器运行时层面增加镜像拉取超时：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 在 containerd 配置中增加 registry 超时
kubectl edit configmap -n kube-system coredns  # 仅示例，实际需修改 containerd 配置
```

更稳妥的做法是为 Logtail 镜像在节点上预加载：

```bash
for node in $(kubectl get node -o jsonpath='{.items[*].metadata.name}'); do
  kubectl debug node/$node -it --image=registry.aliyuncs.com/acs/busybox -- \
    sh -c "crictl pull registry-vpc.cn-zhangjiakou.aliyuncs.com/acs/logtail:latest"
done
```

**第四步：强制滚动更新 DaemonSet，确保所有节点覆盖**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart daemonset logtail-ds -n kube-system
kubectl rollout status daemonset logtail-ds -n kube-system --timeout=300s
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 确认 DaemonSet desired == current == ready
kubectl get daemonset logtail-ds -n kube-system -o wide

# 2. 确认所有节点都有 Logtail Pod 运行
kubectl get node -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | sort -u > /tmp/all_nodes.txt
kubectl get pod -n kube-system -l k8s-app=logtail-ds -o jsonpath='{range .items[*]}{.spec.nodeName}{"\n"}{end}' | sort -u > /tmp/logtail_nodes.txt
echo "缺失节点数: $(comm -23 <(sort /tmp/all_nodes.txt) /tmp/logtail_nodes.txt | wc -l)"

# 3. 检查缺失节点上的 Pod 事件无调度失败
kubectl get events -n kube-system --field-selector reason=FailedScheduling | grep logtail

# 4. 进入 Logtail Pod 验证日志采集配置
kubectl exec -n kube-system -it $(kubectl get pod -n kube-system -l k8s-app=logtail-ds -o jsonpath='{.items[0].metadata.name}') -- /usr/local/ilogtail/ilogtail --status

# 5. 在 SLS 控制台查询最近 10 分钟日志条数是否覆盖所有节点
# 使用 aliyun cli 查询 SLS Project 下各机器组的日志采集状态
aliyun log GetMachineGroup --ProjectName k8s-prod-logs --GroupName logtail-ds-group --RegionId cn-zhangjiakou

# 6. 检查 DiskPressure 节点已恢复 Ready
kubectl get node cn-zhangjiakou.172.16.8.12 -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'
```

## 回复客户话术

> 您好，经排查，本次 Logtail DaemonSet 未覆盖全部节点的根因是 **三类节点差异**：
>
> 1. **2 个节点存在 `observability=log-collection:NoSchedule` Taint**，但 `logtail-ds` 未配置对应 Toleration；
> 2. **1 个节点触发 `DiskPressure`**，kubelet 拒绝创建新 Pod，导致该节点缺失 Logtail；
> 3. **历史网络抖动导致镜像拉取超时**，部分节点曾出现 `ImagePullBackOff`，虽已自愈但需加固。
>
> 我们已完成以下处置：
> - 为 `logtail-ds` 补充了 Toleration，使其能在带 Taint 的节点上运行；
> - 清理了磁盘压力节点上的过期日志，重启 kubelet 后 DiskPressure 已恢复；
> - 强制滚动更新了 DaemonSet，当前 `desired/current/ready` 均为 50/50/50；
> - 将 Logtail 镜像拉取策略调整为 `IfNotPresent`，并计划预加载镜像到节点。
>
> 当前所有节点日志已恢复上传至 SLS。建议后续：
> - 对带 Taint 的节点建立标准化 Toleration 清单，参考 [[domain-02-workloads-applications/00-core-workloads/04-daemonset-management.md|DaemonSet 管理]]；
> - 配置节点磁盘使用率告警：`node_filesystem_avail_bytes / node_filesystem_size_bytes < 0.15` 触发 P2 告警；
> - 在 SLS 侧配置机器组心跳告警，及时发现日志采集 Agent 缺失。
>
> 如有疑问，请随时联系。

## 复盘与沉淀

本次 DaemonSet 未全覆盖故障是可观测性 Agent 部署中的常见问题。核心教训：

1. **DaemonSet 必须配置足够的 Toleration**：在专有云 ACK 中，节点常因业务隔离、GPU、专有云组件等原因被打上各种 Taint。基础设施类 DaemonSet（日志、监控、安全）应使用通用 Toleration 或维护一份显式的 Taint/Toleration 映射表。
2. **节点压力状态会阻塞 DaemonSet Pod 创建**：`DiskPressure`、`MemoryPressure`、`PIDPressure` 都会让 kubelet 拒绝调度新 Pod。对于 Logtail 这类需要写本地日志的 Agent，磁盘压力既是结果也是原因，需要及时处理。
3. **镜像拉取策略影响 DaemonSet 自愈能力**：`Always` 策略在镜像仓库网络抖动时容易失败；`IfNotPresent` 配合节点预加载可显著提升稳定性，但需确保镜像 tag 不可变。

建议将本案例加入 DaemonSet 未全覆盖 FTA，并在日常巡检中增加：
- DaemonSet `desired != ready` 告警；
- 节点 DiskPressure/MemoryPressure 告警；
- SLS 机器组心跳异常告警。

后续 SOP 更新要点：
1. 所有基础设施 DaemonSet 必须配置 `operator: Exists` 或等效通用 Toleration；
2. 节点打 Taint 前必须评估对基础设施 DaemonSet 的影响；
3. 每月执行一次 DaemonSet 覆盖率检查脚本，输出缺失节点清单。

## 是否需要升级及交接信息

- **是否升级**：已定位并修复，暂不需要升级；若后续发现 Logtail 镜像或 SLS 服务端存在持续性问题，需升级至 **可观测性团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-050`
  - 根因：节点 Taint 未容忍 + DiskPressure + 镜像拉取超时
  - 影响集群：`ack-zyy-prod-08`
  - 影响命名空间：`kube-system`
  - 临时修复：补充 Toleration、清理磁盘、滚动更新 DaemonSet、调整镜像拉取策略
  - 长期方案：标准化 DaemonSet Toleration、配置节点压力告警、镜像预加载
  - 待跟进：确认 SLS 机器组心跳稳定 24 小时，更新 DaemonSet 部署 SOP

## Related

- DaemonSet
- Pod Pending：资源不足与 Taint 不匹配
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
