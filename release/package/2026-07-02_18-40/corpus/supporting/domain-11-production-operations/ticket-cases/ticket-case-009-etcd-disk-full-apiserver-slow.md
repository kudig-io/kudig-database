---
title: 阿里云专有云 etcd 数据目录磁盘满导致 apiserver 响应慢
description: 控制面 apiserver 延迟飙升、kubectl 操作卡顿，根因是 etcd 数据盘使用率 100%。包含只读诊断、安全修复、升级标准与交接信息。
summary: 控制面 apiserver 延迟飙升、kubectl 操作卡顿，根因是 etcd 数据盘使用率 100%。包含只读诊断、安全修复、升级标准与交接信息。
category: production-operations
tags:
- aliyun
- private-cloud
- ack
- etcd
- disk-full
- apiserver
- control-plane
- compaction
- defrag
- p0
- ticket-case
tier: supporting
created: 2026-06-26
updated: 2026-06-26
incident_id: TC-2026-009
priority: P0
severity: critical
affected_cluster: ack-prod-vpc01
affected_namespace: kube-system
ticket_type: 控制面故障
skill_ref: 控制面异常诊断
fta_ref: 'FTA: apiserver 响应慢'
last_updated: 2026-06-26
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 阿里云专有云 etcd 数据目录磁盘满导致 apiserver 响应慢 如何处理
trigger_keywords:
- aliyun
- private-cloud
- ack
- etcd
- disk-full
prerequisites:
- kubectl-basics
- k8s-backup
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
- target: '[[entities/etcd.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-005-kubelet-cert-expired.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[concepts/etcd × 可观测性.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单 009：etcd 数据目录磁盘满导致 apiserver 响应慢（升级标准）

## 1. 工单描述

**用户原始描述：**

> 今晚 20:30 开始，ack-prod-vpc01 集群所有 kubectl 操作都特别慢，`kubectl get pod` 经常要 10 秒以上，有时候还会 `Error from server (Timeout)`。ACK 控制台也有点卡，Deployment 滚动更新推进很慢。我们在天基（ASO）里看到 Master 节点磁盘告警，etcd 数据盘使用率 100%。这个集群是专有云生产控制面，麻烦立刻处理，所有发布都停了。

## 2. 分类与优先级判定

- **任务类型：** 控制面故障 / etcd 存储异常 / apiserver 性能退化
- **优先级：** P0（生产控制面 + 全局影响 + 发布停滞）
- **严重程度：** critical
- **响应时限：** 立即响应，5 分钟内给出临时缓解方案
- **安全级别：** 高风险（控制面操作，任何写操作需双重确认与升级标准）

## 3. 诊断步骤

### 3.1 确认 apiserver 延迟与错误率

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 apiserver 响应时间（需在 Master 节点或有权限的跳板机）
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket | head -20

# 查看 apiserver Pod 状态
kubectl get pod -n kube-system -l component=kube-apiserver
kubectl logs -n kube-system -l component=kube-apiserver --tail=200 | grep -i "timeout|etcd|slow"
```
### 3.2 检查 etcd 集群健康与磁盘使用

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 进入 etcd Pod 或使用 etcdctl
kubectl exec -it etcd-ack-prod-vpc01-master-0 -n kube-system -- sh

# 查看成员健康
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --cluster

# 查看每个 endpoint 的存储使用
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --cluster -w table
```
### 3.3 检查 Master 节点磁盘

```bash
# 在 Master 节点上执行
df -h | grep etcd
ls -lh /var/lib/etcd

# 阿里云 ASO 控制台查看
# 路径：天基/ASO > 集群运维 > ack-prod-vpc01 > Master 节点 > 磁盘监控
```

### 3.4 检查 etcd 日志

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -n kube-system -l component=etcd --tail=300 | grep -i "space|full|slow|wal|snapshot"
```
### 3.5 检查事件与资源规模

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 评估 etcd 数据量是否异常
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | tail -50
kubectl get configmap,secret --all-namespaces | wc -l
kubectl get pod --all-namespaces | wc -l
```
### 3.6 诊断过程补充说明

etcd 磁盘满属于 Kubernetes 控制面最高危场景之一。任何对 etcd 的写操作都可能进一步消耗 WAL 空间，因此在磁盘接近 100% 时，应优先执行只读诊断，避免执行会产生新事件或新资源的命令。compact 操作本身会产生新的 revision 记录，但在磁盘已满前通常可以回收大量空间；defrag 会锁定单个 member，必须逐台执行并确认集群健康。

在阿里云专有云环境中，etcd 通常以静态 Pod 形式部署在 Master 节点上，数据盘可能与其他系统盘共用，也可能独立挂载。诊断时需要先通过 `df -h` 确认具体挂载点，再通过 `etcdctl endpoint status` 的 `DB SIZE` 字段判断 etcd 实际数据大小，从而区分是 "etcd 数据膨胀" 还是 "非 etcd 文件占满磁盘"。

## 4. 根因分析

综合 apiserver 超时、etcd 磁盘 100% 与 endpoint status 输出，判定根因为 **"etcd 数据目录所在磁盘爆满，导致 WAL 写入阻塞、raft 同步延迟，进而拖慢 apiserver"**，置信度 **高**。

1. **磁盘空间耗尽：** etcd 数据盘 `/var/lib/etcd` 使用率 100%，无法继续追加 WAL 日志与 snapshot，etcd 进入只读或慢写状态。
2. **revision 膨胀：** 近 7 天某控制器频繁更新某 ConfigMap/Lease，导致 etcd revision 快速增长，历史版本未及时 compact/defrag，实际物理空间远超当前有效数据。
3. **apiserver 连锁反应：** 所有读请求都需经过 etcd，etcd 响应慢导致 apiserver list/watch 延迟飙升，进一步触发客户端重试，形成恶性循环。

### 4.1 风险与影响评估

- **业务影响：** 控制面全局变慢，所有 kubectl、ACK 控制台、CI/CD 发布、Operator 同步均受影响，严重时可导致 Pod 调度与状态同步延迟。
- **扩散风险：** 若不及时处理，etcd 可能进入只读模式，apiserver 写操作被拒绝，集群进入不可变更状态。
- **数据风险：** compact 与 defrag 为不可逆操作，会清除历史 revision，但不会影响当前有效数据；操作前建议对 etcd 数据目录做快照备份。

## 5. 修复命令

### 5.0 操作前快照备份（必须在维护窗口执行）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在磁盘仍有少量余量时执行 etcd snapshot
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /var/backups/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db

# 验证快照完整性
etcdctl snapshot status /var/backups/etcd-snapshot-*.db
```
### 5.1 临时缓解：清理可删除的审计/临时日志（非 etcd 数据）

```bash
# 在 Master 节点上，先清理非 etcd 但占用同一块盘的日志
journalctl --vacuum-time=1d
find /var/log/pods -type f -mtime +3 -delete
find /var/log/containers -type f -mtime +3 -delete
```

### 5.2 若磁盘为独立 etcd 盘，且剩余空间来自日志，重试 apiserver

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deploy kube-apiserver -n kube-system
```
### 5.3 执行 etcd compact（核心修复，需升级审批）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取当前 revision
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

rev=$(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')
compact_rev=$((rev - 10000))

# 执行压缩
etcdctl compact $compact_rev
```
### 5.4 执行 etcd defrag 回收物理空间（需升级审批 + 维护窗口）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 逐台 defrag，每次只对一台执行，避免集群不可用
etcdctl --endpoints=https://10.0.0.11:2379 defrag
etcdctl --endpoints=https://10.0.0.12:2379 defrag
etcdctl --endpoints=https://10.0.0.13:2379 defrag

# 每执行一台后检查 endpoint status
etcdctl endpoint status --cluster -w table
```
### 5.5 配置自动 compact/defrag（长期预防）

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
# 编辑 etcd 静态 Pod manifest，增加或调整以下参数
# --auto-compaction-mode=revision
# --auto-compaction-retention=10000
# --quota-backend-bytes=8589934592

kubectl edit pod etcd-ack-prod-vpc01-master-0 -n kube-system
```
### 5.6 扩容 etcd 数据盘（如磁盘确实不足）

```bash
# 阿里云 ASO 控制台：专有云 > 弹性计算 > 云盘 > 扩容
# 或使用 OpenAPI（需阿里云运维权限）
aliyun ecs ResizeDisk --DiskId d-xxxxxxxx --NewSize 200
# 扩容后需在节点内扩展文件系统
resize2fs /dev/vdb1
```

## 6. 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 磁盘使用率下降
kubectl exec -it etcd-ack-prod-vpc01-master-0 -n kube-system -- df -h /var/lib/etcd

# 2. etcd 集群健康
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --cluster

# 3. etcd endpoint 状态正常
etcdctl endpoint status --cluster -w table

# 4. apiserver 响应恢复
kubectl get --raw /readyz
kubectl get pod -n default --timeout=10s

# 5. ACK 控制台无控制面超时告警
# ASO 路径：天基 > 告警中心 > ack-prod-vpc01 > 控制面
```
## 7. 回复客户话术

> 您好，工单 TC-2026-009 已按 P0 流程处理完成。
>
> **现象确认：** ack-prod-vpc01 集群自 20:30 起 kubectl 与 ACK 控制台响应极慢，部分操作超时，发布全面停滞。
>
> **根因：** etcd 数据目录所在磁盘使用率已达 100%，WAL 写入受阻导致 etcd 响应延迟，进而拖慢整个 kube-apiserver。
>
> **已执行修复：**
> 1. 清理 Master 节点非 etcd 日志释放少量空间作为临时缓冲；
> 2. 在获得升级授权后，执行 etcd compact 清理历史 revision；
> 3. 逐台对 etcd member 执行 defrag 回收物理空间；
> 4. 调整 etcd 自动 compact 策略并评估磁盘扩容。
>
> **当前状态：** etcd 磁盘使用率已降至 45%，集群健康，apiserver 响应恢复正常，发布已恢复。
>
> **后续建议：**
> - 立即为 etcd 数据盘扩容至 200G，并设置 70%/80%/90% 三级磁盘告警；
> - 启用 etcd 自动 compaction，建议 `--auto-compaction-retention=10000`；
> - 排查过去 7 天内导致 revision 快速增长的 Controller/CronJob，避免重复更新；
> - 将 etcd 磁盘满纳入 P0 应急预案，并每季度演练一次；
- 建议对频繁更新 ConfigMap/Lease 的 Controller 进行代码 review，降低写放大；
- 建议将 etcd DB SIZE 与磁盘使用率纳入天基/ASO 一级告警，阈值设置为 70%/80%/90%。
>
> 本次故障已按升级标准完成交接，后续如有复发将自动升级至平台架构师。

## 8. 是否需要升级及交接信息

- **是否升级：** 是（P0 故障已按升级标准执行）
- **升级对象：** 值班经理 → 平台架构师 → 客户技术负责人
- **是否需要变更审批：** 是（etcd compact/defrag 为控制面高危操作，已走 P0 变更绿色通道）
- **交接信息：**
  - 已创建升级工单 INC-2026-009-CP，记录完整时间线、命令与审批人；
  - 已通知阿里云 TAM 与专有云二线值班跟进磁盘扩容与根因复盘；
  - 建议 24 小时内召开 postmortem，输出 etcd 容量治理 SOP；
  - 本案例已纳入控制面 P0 应急知识库，供后续同类故障参考；
- 已通知值班经理在 24 小时内跟进磁盘扩容落地情况，未按期完成将升级至客户技术负责人；
- 建议在下次变更窗口对 etcd 进行全量健康巡检，包括证书有效期、DB SIZE、defrag 状态与集群一致性。

---

*更新时间：2026-06-26 | 责任域：domain-11-production-operations/ticket-cases*

## Related

- etcd (entities)
- 证书过期导致 kubelet 无法连接 apiserver
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- etcd × 可观测性


<!-- risk-assessed -->
