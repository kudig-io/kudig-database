---
title: 节点 DiskPressure：容器日志与镜像占满系统盘
description: 专有云 ACK 集群因节点系统盘被容器日志与镜像层占满，触发 DiskPressure 导致 Pod 被驱逐的工单闭环样本。
summary: 专有云 ACK 集群因节点系统盘被容器日志与镜像层占满，触发 DiskPressure 导致 Pod 被驱逐的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- diskpressure
- node
- eviction
- logs
- p1
tier: peripheral
created: '2026-06-26T15:00:00+08:00'
updated: '2026-06-26T17:30:00+08:00'
incident_id: INC-2026-ACK-018
priority: P1
severity: high
affected_cluster: ack-zyy-prod-06
affected_namespace: kube-system
ticket_type: 节点故障
skill_ref:
- 节点 DiskPressure 排查
- 日志轮转最佳实践
fta_ref:
- 'FTA: 节点 DiskPressure 导致 Pod 驱逐'
last_updated: 2026-06-26 17:30:00+08:00
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
- 节点 DiskPressure：容器日志与镜像占满系统盘 如何处理
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
- target: '[[13-生产运维/05-工单案例/ticket-case-014-node-disk-pressure.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户通过 ACK 控制台告警发现集群中多个节点状态变为 `DiskPressure`，kubelet 开始驱逐节点上的业务 Pod，导致部分微服务实例数下降、请求超时。客户描述如下：

> “ACK 集群 ack-zyy-prod-06 里有几台节点状态变成 DiskPressure，describe node 看到 DiskPressure True。上面的 Pod 被 kubelet 一个个干掉，业务开始出现 504。我们 ssh 到节点上看 /var/log/containers 下面日志文件特别大，/var/lib/docker 占用也很高。麻烦尽快处理一下，不然节点都要被清空了。”

受影响节点包括 `cn-beijing.172.18.3.21`、`cn-beijing.172.18.3.22`、`cn-beijing.172.18.3.23`，主要命名空间为 `microservice-platform` 与 `kube-system`。节点系统盘规格为 100 GiB ESSD，业务容器日志未配置轮转。

## 分类与优先级判定

- **工单类型**：节点故障 / 存储压力。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境多节点同时 DiskPressure，kubelet 自动驱逐业务 Pod，造成服务降级。
2. 问题指向系统盘空间不足，若不快速释放空间并修复根因，将持续影响更多节点。
3. 需在 30 分钟内完成止血（释放磁盘空间）并制定长期治理方案。

## 诊断步骤

按“先看节点状态、再看磁盘占用、再看日志与镜像”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看节点状态与 DiskPressure 标记
kubectl get node -o json | jq '.items[] | {name: .metadata.name, diskPressure: .status.conditions[] | select(.type=="DiskPressure") | .status}'

# 2. 查看节点事件，确认驱逐行为
kubectl get events --field-selector reason=NodeHasDiskPressure --sort-by='.lastTimestamp' -A

# 3. SSH 到问题节点查看磁盘占用
ssh root@cn-beijing.172.18.3.21 'df -h'
ssh root@cn-beijing.172.18.3.21 'du -sh /var/lib/docker/* /var/log/containers/* 2>/dev/null | sort -rh | head -30'

# 4. 查看容器运行时镜像占用
ssh root@cn-beijing.172.18.3.21 'docker system df -v'
ssh root@cn-beijing.172.18.3.21 'crictl system df'

# 5. 查看未使用的悬空镜像与容器
ssh root@cn-beijing.172.18.3.21 'docker images -f "dangling=true"'

# 6. 检查 kubelet 磁盘驱逐阈值配置
ssh root@cn-beijing.172.18.3.21 'cat /etc/kubernetes/kubelet-conf.json | grep -A 5 eviction'

# 7. 通过 ACK 控制台查看节点系统盘监控
aliyun cms DescribeMetricList \
  --Namespace acs_k8s \
  --MetricName node.disk.usage \
  --Dimensions '[{"clusterId":"ack-zyy-prod-06","node":"cn-beijing.172.18.3.21"}]' \
  --RegionId cn-beijing

# 8. 检查日志采集组件是否正常工作
kubectl get pod -n kube-system -l k8s-app=logtail
kubectl logs -n kube-system -l k8s-app=logtail --tail=100 | grep -iE "error|drop|block"
```
## 根因分析

经过排查，发现 `cn-beijing.172.18.3.21` 节点系统盘使用率已达 98%，触发 kubelet 默认磁盘驱逐阈值（`imagefs.available<15%` 或 `nodefs.available<10%`）。进一步分析磁盘占用来源：

1. **容器日志未轮转**：`/var/log/containers/` 目录下多个业务容器日志文件超过 20 GiB，部分微服务未配置 `logrotate` 或容器 `logging driver` 无大小限制。
2. **镜像层堆积**：节点上存在大量历史镜像版本与悬空镜像（dangling images），累计占用约 35 GiB。
3. **临时文件未清理**：部分 Job 容器退出后残留日志与 EmptyDir 数据，未配置 TTL 自动清理。
4. **系统盘规格不足**：节点系统盘仅 100 GiB，对于日志量大、镜像更新频繁的生产环境偏小。

根本原因为：节点磁盘空间缺乏有效治理，日志、镜像、临时数据持续增长，最终突破 kubelet 驱逐阈值，导致 Pod 被驱逐。

## 修复命令

**第一步：隔离问题节点，避免新 Pod 继续调度到磁盘压力节点**

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
for node in cn-beijing.172.18.3.21 cn-beijing.172.18.3.22 cn-beijing.172.18.3.23; do
  kubectl cordon $node
done
```
**第二步：SSH 到节点清理容器日志（仅清理已停止容器的日志文件）**

```bash
# 对每个问题节点执行
for node in cn-beijing.172.18.3.21 cn-beijing.172.18.3.22 cn-beijing.172.18.3.23; do
  ssh root@$node '
    find /var/log/containers -type f -name "*.log" -size +1G -mtime +3 -exec truncate -s 0 {} \;
    find /var/log/pods -type f -name "*.log" -size +500M -mtime +3 -exec truncate -s 0 {} \;
  '
done
```

**第三步：清理悬空镜像与未使用镜像**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
for node in cn-beijing.172.18.3.21 cn-beijing.172.18.3.22 cn-beijing.172.18.3.23; do
  ssh root@$node '
    docker image prune -f
    docker system prune -af --volumes  # ⚠️ 强制清理，可能杀运行中容器
  '
done
```
**第四步：调整 kubelet 日志轮转配置并重启 kubelet（在 ACK 节点池配置中修改）**

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
# 通过 ACK 控制台修改节点池配置：
# 容器运行时 → Docker → 日志驱动设置为 json-file，并设置 max-size=100m, max-file=5
# 或在节点上临时修改 /etc/docker/daemon.json
ssh root@cn-beijing.172.18.3.21 'cat > /etc/docker/daemon.json <<EOF
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "100m",
    "max-file": "5"
  }
}
EOF
systemctl restart docker'
```
**第五步：驱逐可迁移 Pod 到健康节点，释放磁盘压力节点上的业务负载**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
for node in cn-beijing.172.18.3.21 cn-beijing.172.18.3.22 cn-beijing.172.18.3.23; do
  kubectl drain $node \
    --ignore-daemonsets \
    --delete-emptydir-data \
    --force \
    --timeout=300s
done
```
**第六步：扩容系统盘或滚动替换节点（长期方案）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
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
# 调整节点池系统盘大小为 200 GiB，并滚动替换节点
aliyun cs POST /clusters/ack-zyy-prod-06/nodepools/np-zyy-compute \
  --body '{"scaling_group":{"system_disk_size":200}}'

# 对存量节点执行替换
kubectl delete node cn-beijing.172.18.3.21 cn-beijing.172.18.3.22 cn-beijing.172.18.3.23
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 节点 DiskPressure 状态恢复 False
kubectl get node cn-beijing.172.18.3.21 -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'

# 2. 系统盘使用率下降
ssh root@cn-beijing.172.18.3.21 'df -h /'

# 3. 无新的 Pod 驱逐事件
kubectl get events --field-selector reason=NodeHasDiskPressure --sort-by='.lastTimestamp' -A | tail -10

# 4. 业务 Pod 重新调度并 Running
kubectl get pod -n microservice-platform -o wide | grep -v Running

# 5. 容器日志轮转生效
ssh root@cn-beijing.172.18.3.21 'ls -lh /var/log/containers/ | head -10'

# 6. 镜像清理后占用下降
ssh root@cn-beijing.172.18.3.21 'docker system df'
```
## 回复客户话术

> 您好，经排查，本次节点 DiskPressure 的根因是 **容器日志未轮转、历史镜像层堆积，导致节点系统盘使用率突破 kubelet 驱逐阈值**，kubelet 自动驱逐了部分业务 Pod。我们已完成以下处置：
>
> 1. 隔离 3 台问题节点，避免新 Pod 继续调度；
> 2. 清理超过 3 天的大体积容器日志与悬空镜像，释放系统盘空间；
> 3. 调整 Docker 日志驱动配置，启用 `max-size=100m, max-file=5` 轮转策略；
> 4. 将可迁移业务 Pod 驱逐到健康节点，待系统盘扩容后重新加入调度；
> 5. 将节点池系统盘从 100 GiB 扩容至 200 GiB 并滚动替换存量节点。
>
> 当前 DiskPressure 已解除，业务 Pod 全部 Running。建议后续：
> - 使用 日志轮转最佳实践 统一所有业务容器日志策略；
> - 配置 节点磁盘使用率告警，阈值建议 80% 触发 P2；
> - 定期清理镜像与日志，并将系统盘规格纳入节点池标准化配置。
>
> 如有新异常，请随时联系。

## 复盘与沉淀

本次故障是典型的“磁盘空间慢性病急性发作”。容器日志与镜像层在日常运行中持续增长，缺乏轮转与清理策略，最终在生产高峰触发驱逐。需要建立磁盘空间的常态化治理机制，而非仅依赖故障后清理。

关键经验教训：
1. **日志必须轮转**：无论使用 Docker json-file、containerd 还是第三方日志采集，都必须设置单文件大小上限与保留份数；
2. **镜像需要回收**：CI/CD 频繁发布会导致节点上积累大量历史镜像，应定期清理悬空镜像与未使用镜像；
3. **系统盘规格要预留**：生产节点系统盘建议不低于 200 GiB，日志密集型业务建议 300 GiB 以上；
4. **监控要提前**：不能等 DiskPressure 触发驱逐才发现问题，应在磁盘使用率达到 80% 时触发告警。

后续 SOP 更新要点：
1. 节点池创建时强制启用 Docker 日志轮转配置；
2. 每周执行一次镜像清理脚本，回收悬空镜像；
3. 配置告警：`node_filesystem_avail_bytes / node_filesystem_size_bytes < 0.2` 持续 5 分钟触发 P1；
4. 将本案例写入 节点 DiskPressure 回复模板；
5. 在 FinOps 看板中增加节点磁盘成本与清理收益指标。

另外，建议在专有云 ACK 环境中结合 Logtail 或 Fluentd 将容器日志实时采集到 SLS，避免日志长时间驻留节点。对于 EmptyDir、HostPath 等临时存储，应在工作负载中设置合理的资源限制与清理策略，避免临时数据无限增长。

## 是否需要升级及交接信息

- **是否升级**：已止血并恢复，暂不需要升级；若扩容系统盘后仍频繁 DiskPressure，需升级至 **存储团队** 与 **ACK 产品支持**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-018`
  - 根因：容器日志未轮转 + 镜像层堆积导致系统盘满
  - 影响集群：`ack-zyy-prod-06`
  - 影响节点：`cn-beijing.172.18.3.21`、`cn-beijing.172.18.3.22`、`cn-beijing.172.18.3.23`
  - 临时修复：清理日志与镜像、启用日志轮转、驱逐业务 Pod
  - 长期方案：系统盘扩容至 200 GiB、节点池标准化日志策略、磁盘使用率监控
  - 待跟进：确认节点滚动替换完成、更新节点池创建 SOP

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- [[13-生产运维/05-工单案例/ticket-case-014-node-disk-pressure.md|节点 DiskPressure 导致 Pod 被驱逐]]
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- [[13-生产运维/05-工单案例/ticket-case-014-node-disk-pressure.md|节点 DiskPressure 导致 Pod 被驱逐]]
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang


<!-- risk-assessed -->
