---
title: 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
description: 专有云 ACK 工作节点因 /var/log/pods 与容器日志膨胀触发 DiskPressure，kubelet 驱逐业务 Pod，造成服务降级的工单闭环样本。
summary: 专有云 ACK 工作节点因 /var/log/pods 与容器日志膨胀触发 DiskPressure，kubelet 驱逐业务 Pod，造成服务降级的工单闭环样本。
category: production-operations
tags:
- ack
- zyy
- diskpressure
- eviction
- kubelet
- node-pressure
- p1
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: TC-2026-035
priority: P1
severity: high
affected_cluster: ack-zyy-prod-01
affected_namespace: kube-system, multi-app
ticket_type: 节点压力/磁盘资源告警
skill_ref:
- '[[domain-01-cluster-fundamentals/03-control-plane/33-kubelet-eviction-thresholds.md|kubelet
  驱逐阈值]]'
- '[[domain-02-workloads-applications/00-core-workloads/18-node-management-operations.md|节点管理运维]]'
fta_ref:
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/nodepool-fta.md|FTA: 节点池异常]]'
- '[[domain-10-troubleshooting-diagnostics/topic-fta/list/pod-fta.md|FTA: Pod 异常]]'
last_updated: 2026-06-26 16:30:00+08:00
duplicate_of: TC-2026-040
status: duplicate
duplication_reason: 与 "TC-2026-040" 主题重复，内容角度相似，降低 RAG 权重
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐 如何处理
trigger_keywords:
- ack
- zyy
- diskpressure
- eviction
- kubelet
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
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md]]'
  type: related_to
---



# 工单描述

客户监控告警发现 `ack-zyy-prod-01` 集群中节点 `cn-zhangjiakou.172.16.5.40` 状态变为 `DiskPressure`，其上运行的电商推荐服务 Pod 被大量驱逐，业务 QPS 下降约 30%。客户描述如下：

> “我们收到节点 DiskPressure 告警，上去一看节点 Ready 但带 DiskPressure 污点了。推荐服务的 Pod 一个个被 Evicted，describe pod 看到 `The node was low on resource: ephemeral-storage`。kubectl get node 看磁盘使用率 92%。我们业务没有写大量本地文件，不知道为什么突然就满了。麻烦紧急处理，别让推荐服务再受影响。”

该集群为专有云 `ack-zyy-prod-01`，节点使用容器运行时 containerd，操作系统 Alibaba Cloud Linux 3。

## 分类与优先级判定

- **工单类型**：节点压力 / 磁盘资源告警 / Pod 驱逐。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境节点 DiskPressure 导致业务 Pod 被驱逐，服务降级。
2. 磁盘使用率 92% 已接近 kubelet 默认驱逐阈值，随时可能继续驱逐。
3. 需在 15 分钟内给出清理磁盘并防止复发的方案。

## 诊断步骤

按“先看节点状态、再上节点看磁盘占用、再看日志与 kubelet 事件”的顺序排查：

```bash
# 1. 确认节点状态与条件
kubectl get node cn-zhangjiakou.172.16.5.40 -o wide
kubectl describe node cn-zhangjiakou.172.16.5.40 | grep -A 20 Conditions

# 2. 查看被驱逐 Pod 列表
kubectl get pod --all-namespaces --field-selector spec.nodeName=cn-zhangjiakou.172.16.5.40 | grep Evicted
kubectl get events --field-selector reason=Evicted --sort-by='.lastTimestamp' | tail -30

# 3. 登录节点检查磁盘占用（需已有堡垒机/密钥）
ssh root@172.16.5.40 '
  df -h / /var/lib/kubelet /var/lib/containerd /var/log
  du -sh /var/log/pods /var/lib/containerd/io.containerd.grpc.v1.cri/containers/* /var/log/journal 2>/dev/null | sort -hr | head -20
'

# 4. 查看容器镜像与缓存占用
crictl ps -a | wc -l
crictl system df
docker system df 2>/dev/null || true

# 5. 查看 kubelet 日志中的驱逐决策
journalctl -u kubelet -n 500 --no-pager | grep -i 'eviction|DiskPressure|threshold' | tail -30

# 6. 查看 Pod 日志是否异常暴增
kubectl logs -n rec-service deploy/recommend-v2 --tail=200 | wc -c
```

## 根因分析

综合节点状态、磁盘占用与 kubelet 日志，判定根因为 **`/var/log/pods` 下容器标准输出日志与 journal 日志未设置大小上限，长期累积占满系统盘，触发 kubelet DiskPressure 驱逐**，置信度 **高**。

1. **日志膨胀**：推荐服务 Pod 近期接入全量 debug 日志，单容器标准输出每小时产生约 2GiB 日志，containerd 默认将 stdout/stderr 以 JSON 文件形式保存在 `/var/log/pods/<pod-uid>/<container-name>/`，3 天后累计超过 120GiB。
2. **journal 未限制**：systemd journal 未配置 `SystemMaxUse`，与容器日志叠加，系统盘使用率突破 92%。
3. **镜像缓存未清理**：节点上存在大量历史镜像层与已停止容器，额外占用约 20GiB 镜像存储空间。
4. **kubelet 默认阈值**：`evictionHard.imagefs.available<15%` 与 `nodefs.available<10%` 被触发，kubelet 按 QoS 顺序驱逐 Pod。

## 修复命令

**第一步：隔离节点并安全驱逐业务 Pod**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
kubectl cordon cn-zhangjiakou.172.16.5.40
kubectl drain cn-zhangjiakou.172.16.5.40 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --timeout=300s
```

**第二步：登录节点清理容器日志与镜像缓存**

```bash
ssh root@172.16.5.40 '
  # 压缩并清空超过 100MB 的容器日志
  find /var/log/pods -type f -size +100M -exec sh -c "> {}" \;
  
  # 清空已退出容器的日志
  find /var/lib/containerd/io.containerd.grpc.v1.cri/containers -type f -name "*.log" -size +50M -exec sh -c "> {}" \;
  
  # 限制 journal 大小
  journalctl --vacuum-size=500M
  
  # 清理未使用的镜像与容器（仅删除已停止且无标签的镜像）
  crictl rm -af 2>/dev/null || true
  crictl rmi --prune 2>/dev/null || true
'
```

**第三步：扩容系统盘**

```bash
aliyun ecs ResizeDisk \
  --DiskId d-8vbdummy05 \
  --NewSize 200 \
  --Type offline
```

> 扩容后需在节点内执行 `growpart` 与 `resize2fs`/`xfs_growfs`（由 ACK 节点自动修复功能或运维脚本完成）。

**第四步：调整 kubelet 驱逐阈值与日志轮转**

通过 ACK 控制台节点池配置或修改 kubelet 配置：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# 在节点上编辑 /etc/kubernetes/kubelet-config.json 后重启 kubelet（推荐通过 ACK 节点池运维脚本下发）
cat <<'EOF' >> /etc/kubernetes/kubelet-config.json
{
  "evictionHard": {
    "imagefs.available": "10%",
    "nodefs.available": "10%",
    "nodefs.inodesFree": "5%"
  },
  "evictionSoft": {
    "imagefs.available": "15%",
    "nodefs.available": "15%"
  },
  "evictionSoftGracePeriodSeconds": {
    "imagefs.available": "1m",
    "nodefs.available": "1m"
  }
}
EOF
systemctl restart kubelet
```

**第五步：恢复节点调度**

```bash
kubectl uncordon cn-zhangjiakou.172.16.5.40
```

## 验证命令

```bash
# 1. 节点 DiskPressure 条件消失
kubectl get node cn-zhangjiakou.172.16.5.40 -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'

# 2. 磁盘使用率下降
ssh root@172.16.5.40 'df -h / /var/lib/kubelet /var/lib/containerd /var/log'

# 3. 新 Pod 可正常调度到该节点
kubectl run disk-test --rm -it --restart=Never -n default --image=registry-vpc.cn-zhangjiakou.aliyuncs.com/acs/busybox:latest --overrides='{"spec":{"nodeName":"cn-zhangjiakou.172.16.5.40"}}' -- df -h

# 4. 无新驱逐事件
kubectl get events --field-selector reason=Evicted --sort-by='.lastTimestamp' | tail -10

# 5. 推荐服务 Pod 恢复 Running
kubectl get pod -n rec-service -o wide | grep -v Running
```

## 回复客户话术

> 您好，工单 TC-2026-035 已处理完成。
>
> **现象确认：** 节点 `cn-zhangjiakou.172.16.5.40` 出现 `DiskPressure`，`rec-service` 命名空间下推荐服务 Pod 被 kubelet 驱逐，业务 QPS 下降约 30%。
>
> **根因：** 推荐服务近日接入全量 debug 日志，容器标准输出与 systemd journal 未限制大小，快速占满系统盘，磁盘使用率达 92%，触发 kubelet 默认驱逐阈值。
>
> **已执行修复：**
> 1. 隔离问题节点并安全驱逐业务 Pod；
> 2. 清理超过 100MB 的容器日志、journal 日志与未使用镜像；
> 3. 将系统盘扩容至 200Gi；
> 4. 调整 kubelet 驱逐阈值并重启 kubelet；
> 5. 恢复节点调度，推荐服务 Pod 重新调度并 Running。
>
> **当前状态：** 节点 `DiskPressure` 条件已消失，磁盘使用率降至 65% 以下，业务 Pod 全部 Running，无新驱逐事件。
>
> **后续建议：**
> - 参考 [[domain-01-cluster-fundamentals/03-control-plane/33-kubelet-eviction-thresholds.md|kubelet 驱逐阈值]] 为节点池配置合理的磁盘告警；
> - 为推荐服务容器配置 日志轮转 Sidecar 或限制 stdout 日志量，关闭不必要的 debug 日志；
> - 在节点初始化脚本中配置 `logrotate` 与 `journalctl --vacuum-size`，并定期清理镜像；
> - 通过 节点维护手册 将磁盘清理纳入月度巡检；
> - 考虑将日志采集模式改为 sidecar 输出到外部日志服务，减少本地磁盘占用。
>
> 如有异常请随时联系。

## 复盘与沉淀

节点 DiskPressure 往往不是因为业务写本地文件，而是容器 stdout 日志、journal 与镜像缓存失控。containerd/docker 默认不会自动轮转容器日志，依赖节点级 `logrotate` 或集群日志采集方案。推荐在生产节点初始化时即配置 `logrotate` 对 `/var/log/pods/**/*.log` 按大小与时间轮转，并限制 journal 大小。

同时，应建立节点磁盘水位监控：在磁盘使用率达到 70% 时触发 P3 预警，80% 触发 P2，接近 kubelet 驱逐阈值时触发 P1。对于日志量大的应用，优先使用 Sidecar 日志采集器将日志直接发送到外部存储，或调整应用日志级别，避免全量 debug 日志输出到 stdout。kubelet 驱逐阈值可根据磁盘容量与业务特点调整，但调整前需评估是否会导致问题被掩盖。

## 是否需要升级及交接信息

- **是否升级**：已闭环，无需升级。若多个节点同时出现 DiskPressure 且清理后快速复现，需升级至 **可观测性团队** 审查日志量级与采集策略。
- **是否需要变更审批**：是（节点隔离、磁盘扩容、kubelet 配置调整已登记变更台账）。
- **交接信息**：
  - 故障单号：`TC-2026-035`
  - 根因：`容器日志与 journal 膨胀导致系统盘满，触发 DiskPressure 驱逐`
  - 影响节点：`cn-zhangjiakou.172.16.5.40`
  - 修复动作：隔离节点 + 清理日志/镜像 + 扩容系统盘 + 调整 kubelet 阈值
  - 待跟进：验证磁盘扩容后文件系统已 online resize，观察 24 小时磁盘增长趋势

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502
- [[domain-11-production-operations/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]
