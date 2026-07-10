---
title: 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
description: 专有云 ACK 集群节点因容器日志与镜像层占满磁盘触发 DiskPressure，kubelet 大量驱逐业务 Pod，含诊断、修复与验证。
summary: 专有云 ACK 集群节点因容器日志与镜像层占满磁盘触发 DiskPressure，kubelet 大量驱逐业务 Pod，含诊断、修复与验证。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- node
- diskpressure
- eviction
- kubelet
- log-rotation
- p0
tier: core
created: '2026-06-26T15:00:00+08:00'
updated: '2026-06-26T17:45:00+08:00'
incident_id: TC-2026-040
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-07
affected_namespace: log-service
ticket_type: 节点资源故障
skill_ref:
- 节点 DiskPressure 排查
- 日志成本优化
fta_ref:
- 'FTA: 节点磁盘压力'
last_updated: 2026-06-26 17:45:00+08:00
difficulty: advanced
reading_level: advanced
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
- node
- diskpressure
- eviction
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
- target: '[[生产运维/ticket-cases/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[生产运维/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户通过 ACK 专有云集群监控发现多个节点状态变为 `DiskPressure`，其上运行的日志服务 Pod 被大量驱逐。客户描述如下：

> “我们 log-service 命名空间里的 Pod 突然大批量被 Evicted，kubectl get node 看到好几个节点 DiskPressure。df 看节点根分区已经 95% 了，/var/lib/docker/containers 和 /var/log 下面文件特别大。是不是日志没轮转？现在日志采集链路快断了，麻烦紧急处理。”

受影响节点为 `cn-zhangjiakou.172.16.7.31` 至 `cn-zhangjiakou.172.16.7.33`，业务上日志采集与实时查询受到影响。

## 分类与优先级判定

- **工单类型**：节点资源故障 / DiskPressure / Pod 驱逐。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境多节点同时触发 DiskPressure，导致业务 Pod 被驱逐，服务可用性下降。
2. 日志服务为可观测性基础设施，影响范围跨多个业务命名空间。
3. 需要在 15 分钟内止血，释放磁盘空间并恢复节点状态。

## 诊断步骤

按“先节点状态、后磁盘使用、再日志与镜像”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认节点状态与压力条件
kubectl get node -o wide
kubectl get node -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="DiskPressure")].status}{"\n"}{end}'

# 2. 查看被驱逐的 Pod
kubectl get pod --all-namespaces --field-selector status.phase=Failed | grep Evicted | head -30
kubectl get events --all-namespaces --field-selector reason=Evicted --sort-by='.lastTimestamp' | tail -30

# 3. 登录问题节点检查磁盘使用（通过 ack-cli 或堡垒机）
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- df -h
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- du -sh /var/lib/docker/containers/* | sort -rh | head -10
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- du -sh /var/log/* | sort -rh | head -10

# 4. 检查容器日志大小
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- sh -c '
for f in /var/lib/docker/containers/*/*.log; do
  echo "$(du -h "$f" | cut -f1) $f"
done | sort -rh | head -10'

# 5. 检查镜像与容器层占用
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- docker system df -v 2>/dev/null || crictl system df 2>/dev/null

# 6. 检查 kubelet 配置与日志轮转策略
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- cat /etc/kubernetes/kubelet-config.json | grep -A 10 eviction
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- ls -la /etc/logrotate.d/

# 7. 通过 ASO 或 ACK 控制台查看节点磁盘告警
ack-cli node diagnose cn-zhangjiakou.172.16.7.31 --cluster ack-zyy-prod-07 --module disk
```
## 根因分析

节点 `/var/lib/docker/containers` 目录下容器日志文件未被有效轮转，单个日志文件膨胀至数十 GiB，占满系统盘。触发 kubelet 的 DiskPressure 条件后，kubelet 按照驱逐优先级开始驱逐 Pod。

具体根因链：

1. **日志未轮转：** 业务容器标准输出日志未配置 logrotate 或 kubelet 容器日志轮转参数未启用，日志持续追加。
2. **业务日志暴增：** `log-service` 中部分 Pod 因上游流量突增打印大量错误日志，单日日志量超过预期。
3. **磁盘容量规划不足：** 系统盘仅 100GiB，且 `/var/lib/docker` 与 `/var/log` 共用根分区，未单独挂载数据盘。
4. **kubelet 驱逐：** 磁盘可用空间低于 `eviction-hard` 阈值（默认 imagefs.available<15% 或 nodefs.available<10%），触发 DiskPressure 并驱逐 Pod。

根因置信度：**高**。

### 风险与影响评估

- **业务影响：** `log-service` 为集群可观测性基础设施，Pod 被驱逐后日志采集能力下降，影响故障排查与审计合规；同时被驱逐的业务 Pod 需要重新调度，短期内服务可用性下降。
- **扩散风险：** DiskPressure 会同时影响节点上所有 Pod，若多个节点因相同原因（如统一镜像或统一应用配置）同时触发，可能演变为集群级故障。
- **数据风险：** 被驱逐 Pod 的本地 EmptyDir 数据会丢失；若应用未正确处理 SIGTERM，可能导致请求中断。
- **恢复关键：** 优先释放磁盘空间使节点退出 DiskPressure，随后再修复日志轮转与磁盘规划，避免在修复过程中继续驱逐。

## 修复命令

**第一步：紧急清理大日志文件，释放磁盘空间（仅删除已 rotate 的旧日志）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在问题节点上执行，先清理已停止容器的日志与过期日志
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- sh -c '
find /var/lib/docker/containers -name "*.log" -size +1G -mtime +1 -exec truncate -s 0 {} \;
find /var/log -type f -name "*.log-*" -mtime +7 -delete
journalctl --vacuum-time=3d
'
```
**第二步：手动触发容器日志轮转**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 登录节点执行 logrotate
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- logrotate -f /etc/logrotate.d/docker-container-log
```
**第三步：配置 kubelet 容器日志轮转参数**

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
# 修改 kubelet 配置，启用容器日志轮转
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- sh -c '
cat <<EOF >> /etc/kubernetes/kubelet-config.json
{
  "containerLogMaxSize": "100Mi",
  "containerLogMaxFiles": 5
}
EOF
systemctl restart kubelet
'
```
**第四步：清理未使用镜像（低峰期执行）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- docker image prune -a -f 2>/dev/null || crictl rmi --prune 2>/dev/null
```
**第五步：对高日志量 Pod 增加临时 sidecar 日志限制或调整应用日志级别**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 临时调整应用日志级别为 WARN
kubectl set env deployment/log-processor -n log-service LOG_LEVEL=WARN
kubectl rollout status deployment/log-processor -n log-service --timeout=180s
```
## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 节点 DiskPressure 条件恢复
kubectl get node cn-zhangjiakou.172.16.7.31 -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'

# 2. 磁盘使用率下降
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- df -h /

# 3. 已无新 Evicted 事件产生
kubectl get events --field-selector reason=Evicted --all-namespaces --sort-by='.lastTimestamp' | tail -10

# 4. 新创建的测试 Pod 可正常调度并 Running
kubectl run disk-test --image=registry.aliyuncs.com/acs/busybox --restart=Never -n default -- sleep 600
kubectl get pod disk-test -n default

# 5. log-service Pod 恢复 Running
kubectl get pod -n log-service -o wide | grep -v Running

# 6. kubelet 日志无新的驱逐记录
ack-cli node exec cn-zhangjiakou.172.16.7.31 -- journalctl -u kubelet --since "30 minutes ago" | grep -i "eviction|diskpressure" | tail -20
```
## 回复客户话术

> 您好，工单 TC-2026-040 已处理完成。
>
> **现象确认：** `ack-zyy-prod-07` 多节点触发 `DiskPressure`，`log-service` 命名空间大量 Pod 被 kubelet 驱逐。
>
> **根因：** 节点 `/var/lib/docker/containers` 下容器标准输出日志未有效轮转，日志文件膨胀占满系统盘，磁盘可用空间低于 kubelet `eviction-hard` 阈值，触发 DiskPressure 并驱逐 Pod。
>
> **已执行修复：**
> 1. 紧急 truncate 过期大日志文件，释放磁盘空间；
> 2. 配置 kubelet 容器日志轮转参数（`containerLogMaxSize: 100Mi`、`containerLogMaxFiles: 5`）；
> 3. 清理节点上未使用的历史镜像；
> 4. 临时将 `log-processor` 日志级别调整为 WARN，降低日志生成速率。
>
> **当前状态：** 节点 DiskPressure 已解除，新 Pod 可正常调度，`log-service` Pod 全部 Running，无新驱逐事件。
>
> **后续建议：**
> - 为日志密集型节点单独挂载大容量数据盘，并将 `/var/lib/docker` 与 `/var/log` 迁移至数据盘；
> - 在集群层面统一启用 kubelet 容器日志轮转，并纳入节点初始化模板；
> - 配置磁盘使用率告警，建议根分区 >75% 预警、>85% P1 告警；
> - 参考 日志成本优化 评估日志采样与分级存储策略；
> - 对 log-service 增加日志缓冲与背压机制，避免异常流量导致日志暴增。
>
> 如有异常请随时联系。

## 复盘与沉淀

本次故障是 Kubernetes 节点磁盘资源管理失效的典型场景。kubelet 的 eviction 机制虽然可以保护节点不因磁盘耗尽而完全不可用，但代价是主动驱逐业务 Pod，对生产环境仍然造成较大影响。因此，预防 DiskPressure 比事后清理更重要。

排查时应快速区分磁盘压力的来源：是容器日志、镜像层、EmptyDir、还是 journald。本例中通过 `du -sh /var/lib/docker/containers/*` 迅速定位到单个容器日志文件占用了数十 GiB，明确了日志未轮转的根因。若盲目清理镜像，可能无法快速释放足够空间，且会触发镜像重新拉取，增加恢复时间。

在修复时需要注意：`truncate -s 0` 是一种安全的日志释放方式，它会清空文件内容但不会删除 inode，因此不会影响容器继续写入。直接删除正在写入的日志文件可能导致磁盘空间未真正释放（因为进程仍持有文件句柄）。清理完成后，必须验证节点 DiskPressure 条件是否解除，而不仅仅是 `df -h` 显示空间下降，因为 kubelet 会按一定周期同步节点状态。

建议建立以下长效机制：
1. **节点磁盘规划：** 为可观测性、大数据、日志类节点单独挂载 500GiB 以上的数据盘，并将 `/var/lib/docker`、`/var/log`、`/var/lib/kubelet` 迁移到数据盘；
2. **统一日志轮转：** 在节点初始化模板中启用 kubelet `containerLogMaxSize` 与 `containerLogMaxFiles`，并配置系统级 logrotate；
3. **磁盘告警：** 配置 node-exporter 或 ACK 云监控告警，根分区使用率 75% 预警、85% P1、90% P0；
4. **日志治理：** 对高日志量应用进行日志分级、采样与限流，参考 日志成本优化 建立日志生命周期管理。

## 是否需要升级及交接信息

- **是否升级**：否（已闭环）。若扩容系统盘后仍频繁触发 DiskPressure，需升级至 **基础设施团队** 评估节点磁盘架构与日志平台方案。
- **交接信息**：
  - 故障单号：`TC-2026-040`
  - 根因：容器日志未轮转导致系统盘满，触发 kubelet DiskPressure 驱逐
  - 影响集群：`ack-zyy-prod-07`
  - 影响命名空间：`log-service` 等
  - 临时修复：清理日志、启用 kubelet 日志轮转、降低应用日志级别
  - 长期方案：为日志节点挂载独立数据盘、统一日志轮转策略、配置磁盘告警
  - 待跟进：确认所有受影响的 Pod 已重建完成，将日志轮转参数同步到节点池模板

## Related

- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502
- [[生产运维/ticket-cases/ticket-case-017-pod-pending-resource-exhaustion.md|Pod 大量 Pending：节点 CPU/内存资源不足]]


<!-- risk-assessed -->
