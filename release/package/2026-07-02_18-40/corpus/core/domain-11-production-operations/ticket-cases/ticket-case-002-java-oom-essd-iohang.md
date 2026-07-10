---
title: Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
description: 专有云 ACK 集群 Java 应用因堆内存不足触发 OOM，叠加 ESSD 磁盘 IO hang 导致 Pod 反复 CrashLoopBackOff
  的工单闭环样本。
summary: 专有云 ACK 集群 Java 应用因堆内存不足触发 OOM，叠加 ESSD 磁盘 IO hang 导致 Pod 反复 CrashLoopBackOff
  的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- java
- oom
- essd
- crashloopbackoff
- storage
- p0
tier: core
created: '2026-06-26T09:15:00+08:00'
updated: '2026-06-26T12:45:00+08:00'
incident_id: INC-2026-ACK-002
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-02
affected_namespace: app-order
ticket_type: 应用故障
skill_ref:
- Pod CrashLoopBackOff FTA
- JVM 容器调优
fta_ref:
- 'FTA: Java OOM 与 ESSD IO hang'
last_updated: 2026-06-26 12:45:00+08:00
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang 如何处理
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
- target: '[[domain-11-production-operations/工单案例/ticket-case-040-node-diskpressure-eviction.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-042-pod-pending-resource-taint.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈 `app-order` 命名空间下的 `order-api` 服务出现大面积 502/503，ACK 控制台显示多个 Pod 状态为 `CrashLoopBackOff`。客户提供的原始描述如下：

> “订单服务从早上 9 点开始一直重启，kubectl logs 看到 `java.lang.OutOfMemoryError: Java heap space`，然后容器就被 killed 了。宿主机磁盘 ESSD 监控显示 IO 等待时间很高，有时甚至到十几秒。我们怀疑是内存不够还是磁盘卡住了，请帮忙确认根因。”

受影响集群 `ack-zyy-prod-02`，命名空间 `app-order`，高峰期每分钟数千订单调用，故障持续 30 分钟以上。

## 分类与优先级判定

- **工单类型**：应用运行异常 / 存储 IO 异常。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境核心业务 Pod 反复崩溃，直接造成服务降级与业务超时。
2. 同时存在 Java OOM 与 ESSD IO hang 两类异常，二者相互放大，需立即止血。
3. 需在 15 分钟内给出明确的临时恢复命令与根因说明。

## 诊断步骤

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 Pod 状态与重启次数
kubectl get pod -n app-order -l app=order-api -o wide
kubectl get pod -n app-order -l app=order-api -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[0].restartCount}{"\t"}{.status.containerStatuses[0].lastState.terminated.reason}{"\n"}{end}'

# 2. 查看上一次崩溃日志
kubectl logs -n app-order deployment/order-api --previous --tail=200

# 3. 查看资源使用与限制
kubectl describe pod -n app-order order-api-7d9c4f8b5-xk2z9 | grep -A 10 -i resources
kubectl top pod -n app-order -l app=order-api

# 4. 进入节点查看 ESSD IO 与 dmesg
kubectl debug node/cn-zhangjiakou.172.16.2.14 -it --image=registry.aliyuncs.com/acs/busybox -- nsenter -t 1 -m -u -i -n -- dmesg -T | grep -i "I/O error|blocked|hang"

# 5. 查询 ESSD 磁盘监控
aliyun ecs DescribeDiskMonitorData \
  --RegionId cn-zhangjiakou \
  --DiskId d-8vbdummy14 \
  --StartTime 2026-06-26T08:00:00Z \
  --EndTime 2026-06-26T10:00:00Z \
  --Period 60 \
  --output cols=TimeStamp,DiskReadIO,DiskWriteIO,DiskReadLatency,DiskWriteLatency rows=MonitorData[]

# 6. 检查节点磁盘压力
kubectl describe node cn-zhangjiakou.172.16.2.14 | grep -A 5 Conditions

# 7. 采集 JVM GC 日志（若已挂载）
kubectl cp -n app-order order-api-7d9c4f8b5-xk2z9:/app/logs/gc.log /tmp/gc.log
```
## 根因分析

`order-api` 容器配置为 JVM `-Xmx2g`，但容器 memory limit 仅设置为 `2Gi`。JVM 堆外内存（元空间、线程栈、JNI、JIT 缓存）加上业务对象导致实际使用超过 limit，触发容器被 OOMKilled（Exit Code 137）。OOM 时 JVM 启动 Full GC，产生大量磁盘 IO。

叠加该节点 ESSD 盘在同一时段出现 IO hang（`dmesg` 中可见 `task blocked for more than 120 seconds`），GC 停顿与 IO 等待形成恶性循环：应用请求处理变慢 → 对象堆积 → 频繁 Full GC → 更多 IO → 容器健康检查失败 → kubelet 重启容器 → `CrashLoopBackOff`。

根本原因是：
1. JVM 堆配置与容器 limit 不匹配；
2. ESSD 盘存在偶发性高延迟，未及时隔离或迁移负载。

## 修复命令

**第一步：临时扩大 memory limit 并启用堆转储**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment order-api -n app-order --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "4Gi"},
  {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "2Gi"},
  {"op": "add", "path": "/spec/template/spec/containers/0/env/-", "value": {"name": "JAVA_OPTS", "value": "-Xmx2g -Xms2g -XX:+HeapDumpOnOutOfMemoryError -XX:HeapDumpPath=/tmp/heapdump.hprof"}}
]'
```
**第二步：挂载 emptyDir 用于堆转储持久化**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment order-api -n app-order --type='merge' -p='{
  "spec": {
    "template": {
      "spec": {
        "volumes": [{"name": "heapdump", "emptyDir": {"sizeLimit": "2Gi"}}],
        "containers": [{
          "name": "order-api",
          "volumeMounts": [{"name": "heapdump", "mountPath": "/tmp/heapdump"}]
        }]
      }
    }
  }
}'
```
**第三步：将问题节点上的 Pod 驱赶到健康节点**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
kubectl cordon cn-zhangjiakou.172.16.2.14
kubectl drain cn-zhangjiakou.172.16.2.14 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --pod-selector='app=order-api' \
  --timeout=300s
```
**第四步：对 ESSD 盘进行快照备份并提交工单给阿里云存储团队**

```bash
aliyun ecs CreateSnapshot \
  --DiskId d-8vbdummy14 \
  --SnapshotName "order-api-essd-investigation-$(date +%Y%m%d%H%M%S)" \
  --Description "IO hang investigation"
```

**第五步：重启 Deployment 观察恢复情况**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/order-api -n app-order
kubectl rollout status deployment/order-api -n app-order --timeout=300s
```
## 验证命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. Pod 全部 Running 且重启次数不再增加
kubectl get pod -n app-order -l app=order-api -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

# 2. 资源使用正常
kubectl top pod -n app-order -l app=order-api

# 3. ESSD 延迟恢复
aliyun ecs DescribeDiskMonitorData --RegionId cn-zhangjiakou --DiskId d-8vbdummy14 --StartTime 2026-06-26T11:00:00Z --EndTime 2026-06-26T13:00:00Z --Period 60

# 4. 业务健康检查
kubectl get endpoints order-api -n app-order
curl -s http://order-api.app-order.svc.cluster.local/health | head

# 5. 查看是否再次 OOM
kubectl logs -n app-order -l app=order-api --tail=100 | grep -i "OutOfMemoryError|OOMKilled"
```
## 回复客户话术

> 您好，已定位 `app-order/order-api` 反复重启的根因：
>
> 1. **JVM OOM**：容器 memory limit 与 JVM 堆配置边界过近，堆外内存触发 OOMKilled；
> 2. **ESSD IO hang**：节点磁盘在同一时段出现高延迟，加剧了 GC 停顿。
>
> 已执行的修复：
>
> - 将 `order-api` memory limit 提升至 4Gi，并启用 OOM 堆转储；
> - 把问题节点上的订单服务 Pod 迁移到健康节点；
> - 对 ESSD 盘创建快照并推动阿里云存储团队排查 IO hang。
>
> 当前 Pod 已稳定 Running，业务健康检查通过。后续建议：
>
> - 根据压测结果重新校准 JVM 堆与容器 limit 的冗余比例（建议 1:1.25~1.5）；
> - 配置 Pod OOM 告警 与 ESSD 延迟告警；
> - 对关键业务配置 PDB 与多可用区反亲和，避免单节点 IO 问题影响全局。

## 复盘与沉淀

Java 应用在容器环境中出现 OOM 时，不能仅看 JVM 堆配置，还必须关注容器 memory limit 与 JVM 堆外内存的总和。本例中 `-Xmx2g` 加上元空间、线程栈、JIT 编译缓存、JNI 以及容器内其他进程（如 jcmd、arthas agent）后，实际内存占用很容易突破 2Gi。建议的基线做法是：容器 memory limit = JVM 堆上限 × 1.25 ~ 1.5，并显式设置 `-XX:MaxMetaspaceSize`、`-XX:MaxDirectMemorySize`，避免堆外内存无限制增长。

ESSD 磁盘 IO hang 是云上偶发但危害极大的故障类型。ESSD 虽然提供高 IOPS 与低延迟，但在后端存储集群异常、快照回滚、磁盘扩容、宿主机迁移等场景下，仍可能出现秒级甚至十秒级 IO 挂起。对于数据库、缓存、消息队列等 IO 敏感型应用，应在应用层设置合理的超时与重试，并在 Kubernetes 侧配置 `PodDisruptionBudget` 与反亲和，确保单节点 IO 异常不会导致全服务不可用。

本次故障还暴露出监控覆盖不足：客户仅有容器级 CPU/内存告警，缺少 JVM 堆使用率、Full GC 频率、ESSD 磁盘延迟的细粒度监控。建议后续引入 JVM GC 日志采集 与 节点磁盘 IO 延迟监控，并在 Grafana 中建立联合视图，便于在 OOM 与 IO hang 同时出现时快速定位关联性。

最后，堆转储文件应统一收集到对象存储或持久卷，避免 Pod 重启后丢失。可在 Deployment 中挂载 NFS 或 OSS PVC 到 `/tmp/heapdump`，并配置 `HeapDumpOnOutOfMemoryError` 与 `HeapDumpPath`，为后续内存泄漏分析保留现场。

后续整改清单：
1. 全量梳理 Java 应用容器 memory limit 与 JVM 参数匹配情况，建立 `-Xmx` / limit 比例基线；
2. 对 ESSD 盘挂载的业务增加 `node.kubernetes.io/unreachable` 与 `disk-pressure` 容忍策略评估；
3. 配置 ESSD 延迟 > 100ms 持续 2 分钟的 P1 告警；
4. 将本案例纳入 Java OOM & IO hang 回复模板。

此外，建议在应用发布流程中增加“启动前资源基线检查”：若检测到 memory limit 与 JVM 堆比例低于 1.25，或 ESSD 盘延迟持续高于阈值，则阻断发布。通过将本案例沉淀为 Java OOM & IO hang 回复模板 与 故障复盘模板，可持续提升团队对复合型故障的响应效率。

另外，建议将 ESSD 延迟指标与业务黄金指标（订单成功率、P99 延迟）放在同一张 Grafana 看板中，便于在存储抖动时快速判断业务影响面。通过持续沉淀此类“复合根因”案例，可逐步完善 多故障叠加 playbook。

最后，将本次处置过程的关键命令与结论整理到工单备注中，便于后续审计与知识复用。同时，建议在非生产环境复现该场景，验证 JVM 参数与 ESSD 延迟告警的联动效果，确保告警阈值不会漏报或误报。

## 是否需要升级及交接信息

- **是否升级**：ESSD IO hang 已提交阿里云存储团队工单（工单号 `TICKET-STOR-20260626-002`），需持续跟进。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-002`
  - 根因：`JVM OOM + ESSD IO hang 叠加`
  - 影响应用：`app-order/order-api`
  - 临时修复：扩容 memory limit、Pod 迁移、ESSD 快照
  - 长期方案：JVM 参数基线整改、ESSD 延迟监控与节点隔离策略
  - 待跟进：存储团队 IO hang 根因报告、heapdump 分析结果

## Related

- 节点磁盘压力 DiskPressure 导致 Pod 被驱逐
- Pod Pending：资源不足与 Taint 不匹配
- Ingress 控制器 Pod 异常导致 404/502


<!-- risk-assessed -->
