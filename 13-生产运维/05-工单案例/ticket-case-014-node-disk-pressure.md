---
title: 节点 DiskPressure 导致 Pod 被驱逐
description: 专有云 ACK 集群节点因磁盘空间不足触发 DiskPressure，kubelet 驱逐业务 Pod 的工单闭环样本。
summary: 专有云 ACK 集群节点因磁盘空间不足触发 DiskPressure，kubelet 驱逐业务 Pod 的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- disk-pressure
- node-eviction
- kubelet
- p0
- node-failure
tier: supporting
created: '2026-06-26T08:00:00+08:00'
updated: '2026-06-26T10:45:00+08:00'
incident_id: INC-2026-ACK-014
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-05
affected_namespace: logistics-platform
ticket_type: 节点故障
skill_ref:
- 节点 NotReady FTA
- 容器镜像清理指南
fta_ref:
- 'FTA: 节点 DiskPressure'
last_updated: 2026-06-26 10:45:00+08:00
duplicate_of: TC-2026-040
status: duplicate
duplication_reason: 与 "TC-2026-040" 主题重复，内容角度相似，降低 RAG 权重
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 节点 DiskPressure 导致 Pod 被驱逐 如何处理
trigger_keywords:
- 节点
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

客户发现 `ack-zyy-prod-05` 集群中 `logistics-platform` 命名空间的多台业务 Pod 突然被驱逐，节点状态显示 `DiskPressure`。客户描述如下：

> “早上 8 点 logistics-platform 的几个 Pod 突然变成 Evicted 状态，kubectl describe node 看到 DiskPressure 是 True。我们上去 df -h 看了下，/var/lib/docker 这个分区使用率 98%。这台节点跑了很多日志量大的 Pod，是不是日志把磁盘打满了？请尽快处理，现在物流轨迹查询有延迟。”

受影响节点为 `cn-beijing.172.18.4.21`，上面运行了 `logistics-platform` 的 `track-service`、`route-engine` 以及 Filebeat DaemonSet Pod。

## 分类与优先级判定

- **工单类型**：节点故障 / 磁盘压力。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境节点触发 DiskPressure，kubelet 主动驱逐业务 Pod，导致服务不可用。
2. 磁盘空间问题会快速蔓延，若不及时清理可能影响整个节点池。
3. 物流轨迹查询属于核心业务链路，需在 15 分钟内完成止血。

## 诊断步骤

按“先看节点状态、再看磁盘使用、最后定位大文件”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认节点 DiskPressure 状态
kubectl get node cn-beijing.172.18.4.21 -o wide
kubectl describe node cn-beijing.172.18.4.21 | grep -A 15 Conditions

# 2. 查看被驱逐 Pod 与事件
kubectl get pod -n logistics-platform | grep Evicted
kubectl get events -n logistics-platform --field-selector reason=Evicted --sort-by='.lastTimestamp' | tail -30

# 3. 登录节点查看磁盘分区与 inode 使用
ssh root@cn-beijing.172.18.4.21 "df -h && df -i"

# 4. 定位大目录
ssh root@cn-beijing.172.18.4.21 "du -sh /var/lib/docker/* /var/log/* /var/lib/kubelet/* 2>/dev/null | sort -hr | head -30"

# 5. 检查容器日志大小
ssh root@cn-beijing.172.18.4.21 "find /var/lib/docker/containers -name '*.log' -exec ls -lh {} \; | sort -k5 -hr | head -20"

# 6. 检查镜像与容器层占用
ssh root@cn-beijing.172.18.4.21 "docker system df -v"  # 或 crictl system df
ssh root@cn-beijing.172.18.4.21 "crictl ps -a | wc -l"

# 7. 检查日志轮转配置
kubectl get ds filebeat -n logistics-platform -o yaml | grep -A 10 resources
```
## 根因分析

节点 `cn-beijing.172.18.4.21` 的根分区 `/` 总容量 200Gi，其中 `/var/lib/docker` 占用 185Gi，使用率达 98%。进一步分析发现：

1. **容器日志未轮转**：多个业务 Pod 的容器标准输出日志未配置 `logrotate`，单容器日志文件超过 20Gi；
2. **旧镜像未清理**：节点上积累了超过 300 个历史镜像层，其中不乏重复的基础镜像版本；
3. **已退出容器残留**：大量 `Completed` 和 `Error` 状态的容器未清理，占用写入层空间。

kubelet 在磁盘可用空间低于阈值时设置 `DiskPressure=True`，并根据驱逐策略优先驱逐 BestEffort 与 Burstable 类型中资源使用较高的 Pod。`track-service` 与 `route-engine` 均为 Burstable QoS，因此被优先驱逐，导致物流查询服务中断。

## 修复命令

**第一步：隔离节点，防止新 Pod 调度**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

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
kubectl cordon cn-beijing.172.18.4.21
```
**第二步：清理容器日志（释放最快、风险最低）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在节点上执行
ssh root@cn-beijing.172.18.4.21 "shred -u /var/lib/docker/containers/*/*.log"  # 慎用，确认非审计日志
# 更安全的做法：清空日志文件而不删除 inode
ssh root@cn-beijing.172.18.4.21 "for f in /var/lib/docker/containers/*/*.log; do > \$f; done"
```
**第三步：清理已退出容器与未使用镜像**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# containerd 运行时
ssh root@cn-beijing.172.18.4.21 "crictl rm -af"
ssh root@cn-beijing.172.18.4.21 "crictl rmi --prune"

# Docker 运行时（若使用）
ssh root@cn-beijing.172.18.4.21 "docker container prune -f"
ssh root@cn-beijing.172.18.4.21 "docker image prune -af"
```
**第四步：调整 kubelet 日志与镜像清理策略**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 在节点上编辑 /var/lib/kubelet/config.yaml
ssh root@cn-beijing.172.18.4.21 "cat >> /var/lib/kubelet/config.yaml <<EOF
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
containerLogMaxSize: 100Mi
containerLogMaxFiles: 5
EOF"
ssh root@cn-beijing.172.18.4.21 "systemctl restart kubelet"
```
**第五步：驱逐可迁移 Pod 并替换节点**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl drain cn-beijing.172.18.4.21 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --timeout=300s

# 由集群自动扩容替换或手动删除节点
kubectl delete node cn-beijing.172.18.4.21
```
**第六步：恢复节点调度（如保留该节点）**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl uncordon cn-beijing.172.18.4.21
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 节点 DiskPressure 状态恢复
kubectl describe node cn-beijing.172.18.4.21 | grep -A 5 DiskPressure

# 2. 磁盘空间释放
ssh root@cn-beijing.172.18.4.21 "df -h /var/lib/docker"

# 3. 业务 Pod 重新调度并 Running
kubectl get pod -n logistics-platform | grep Evicted
kubectl get pod -n logistics-platform -l app=track-service -o wide
kubectl rollout status deployment/track-service -n logistics-platform --timeout=300s

# 4. 镜像与容器清理效果
ssh root@cn-beijing.172.18.4.21 "crictl system df"
```
## 回复客户话术

> 您好，经排查，本次 Pod 被驱逐的根因是 **节点磁盘空间不足触发 DiskPressure**。节点 `/var/lib/docker` 使用率达 98%，主要原因是容器标准输出日志未轮转、历史镜像层与已退出容器未清理。kubelet 根据驱逐策略优先驱逐了 `logistics-platform` 下的 Burstable Pod，导致物流查询服务受影响。我们已完成以下处置：
>
> 1. 隔离问题节点，避免新 Pod 继续调度；
> 2. 清空容器日志并清理已退出容器、未使用镜像，快速释放磁盘空间；
> 3. 调整 kubelet 镜像清理阈值与容器日志轮转策略；
> 4. 驱逐并重新调度业务 Pod，当前服务已恢复。
>
> 建议后续：
> - 为所有业务 Pod 配置合理的日志输出与轮转，避免标准输出日志无限增长；
> - 使用 sidecar 或日志采集 Agent 将日志导出到外部存储（如 SLS）；
> - 配置节点磁盘使用率告警，阈值建议 > 85% 触发 P2，> 90% 触发 P1；
> - 参考 成本影响评估 统计本次扩容与清理成本。
>
> 当前物流轨迹查询服务已恢复正常，请继续观察。

## 复盘与沉淀

DiskPressure 是 Kubernetes 节点常见故障类型之一，但很多企业只关注 CPU/内存监控，忽视了磁盘容量与 inode 使用量。一旦 `/var/lib/docker` 或 `/var/log` 打满，kubelet 会立即驱逐 Pod，且驱逐对象不可控，可能对核心业务造成误伤。

本次故障暴露了两个治理盲点：
1. **日志管理缺失**：业务 Pod 将大量日志输出到 stdout，未配置轮转，也未接入外部日志服务；
2. **镜像治理缺失**：节点长期不清理旧镜像，导致镜像层无限累积。

建议建立以下机制：
1. 在 DaemonSet 中统一配置日志轮转，或限制容器日志最大大小；
2. 接入 SLS/ELK 等日志平台，将日志实时采集到集群外；
3. 配置 kubelet `imageGCHighThresholdPercent` 与 `imageGCLowThresholdPercent`，自动清理未使用镜像；
4. 将磁盘使用率、inode 使用率纳入节点健康巡检；
5. 将本案例写入 节点 DiskPressure 回复模板。

对于日志密集型业务，建议单独划分大容量数据盘挂载到 `/var/lib/docker` 和 `/var/log`，避免与系统盘争用空间。同时，可在节点池层面设置磁盘容量预警，提前触发扩容或清理。在专有云 ACK 中，可以通过自定义节点镜像或初始化脚本将数据目录挂载到独立云盘，并在节点池创建时指定数据盘大小。对于已经上线的节点，若根分区容量不足，可以通过替换节点方式升级到更大系统盘规格，确保长期稳定运行。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若多节点同时出现 DiskPressure 且清理后反复复发，需升级至 **存储基础设施团队** 与 **SRE 容量管理团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-014`
  - 根因：`节点磁盘空间耗尽触发 DiskPressure，kubelet 驱逐业务 Pod`
  - 影响集群：`ack-zyy-prod-05`
  - 影响命名空间：`logistics-platform`
  - 影响节点：`cn-beijing.172.18.4.21`
  - 临时修复：清理日志/容器/镜像 + 调整 kubelet 策略
  - 长期方案：建立日志轮转、镜像 GC、磁盘容量监控与告警
  - 待跟进：确认节点是否保留或替换，更新节点巡检 SOP

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
