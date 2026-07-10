---
title: Storage & Data 生产就绪运维指南
description: 面向生产环境的 K8s 存储与数据运维检查清单、风险缓解、日常操作与排障速查。
summary: 面向生产环境的 K8s 存储与数据运维检查清单、风险缓解、日常操作与排障速查。
category: storage
tags:
- production
- best-practices
- storage
- operations
- csi
- pv
- pvc
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Storage & Data 生产就绪运维指南是什么
- 如何按生产环境要求运维 K8s 存储
trigger_keywords:
- 生产就绪
- 运维指南
- storage
- 存储运维
- PVC
- CSI
prerequisites:
- kubectl-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Storage & Data 生产就绪运维指南

> **适用版本**: Kubernetes v1.28 - v1.33 | **最后更新**: 2026-07 | **运维重点**: 生产准入检查、日常巡检、风险缓解、故障闭环

本指南聚焦将 `domain-04-storage-data` 的存储能力从“可用”推进到“可长期稳定运行”，重点补齐当前知识库在**存储多租户治理、临时存储管理、CSI 供应链安全、容量规划、存储混沌演练**五个方向的缺口。阅读前建议先熟悉 [[K8s存储/01-storage-architecture-overview.md|存储架构概览]] 与 [[K8s存储/07-storage-daily-operations.md|存储日常运维操作手册]]。

生产环境中的存储故障往往具有“级联放大”特征：一个 CSI Node 插件异常可能导致整节点有状态 Pod 无法重启；一个未设置 sizeLimit 的 emptyDir 可能在数小时内占满节点磁盘，触发大规模驱逐；一次未经验证的备份恢复演练可能在真正灾难时暴露 RTO 不可达。因此，本指南不仅提供检查清单，更强调“可验证、可回滚、可审计”的运维闭环。

<!-- chunk: 目录 -->
## 目录

1. [生产环境检查清单](#生产环境检查清单)
2. [关键风险与缓解措施](#关键风险与缓解措施)
3. [日常运维操作](#日常运维操作)
4. [故障排查速查](#故障排查速查)
5. [与其他域的协作边界](#与其他域的协作边界)
6. [推荐阅读](#推荐阅读)

---

<!-- chunk: 生产环境检查清单 -->
## 生产环境检查清单

在将承载有状态工作负载的集群或命名空间标记为生产就绪前，建议逐项确认以下 12 项。

| 编号 | 检查项 | 通过标准 | 关键命令 / 配置 |
|---|---|---|---|
| 1 | CSI 驱动健康 | Controller 与 Node 插件全部 `Running`，版本在厂商支持矩阵内 | `kubectl get pods -A -l app.kubernetes.io/component=csi-driver` |
| 2 | StorageClass 生产化 | 非默认删除策略按需设置，`allowVolumeExpansion=true`，`volumeBindingMode=WaitForFirstConsumer` | `kubectl get sc -o custom-columns=NAME:.metadata.name,EXP:.allowVolumeExpansion,BIND:.volumeBindingMode` |
| 3 | 默认 StorageClass 受控 | 有且仅有一个默认 SC，且默认 SC 不用于生产敏感负载 | `kubectl get sc -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'` |
| 4 | 存储多租户配额 | 每个命名空间配置 `ResourceQuota`（`requests.storage`、`persistentvolumeclaims`）与 `LimitRange` | `kubectl get resourcequota,limitrange -n <ns>` |
| 5 | 临时存储限制 | 关键命名空间 Pod 设置 `emptyDir` sizeLimit，`ephemeral-storage` request/limit | `kubectl get pods -n <ns> -o jsonpath='{..emptyDir.sizeLimit}'` |
| 6 | 数据静态加密 | 生产 SC 启用后端加密（KMS/云盘加密），敏感应用不使用本地未加密卷 | `kubectl get sc <sc> -o jsonpath='{.parameters.encrypted}'` |
| 7 | 快照与恢复就绪 | VolumeSnapshotClass 已创建，`deletionPolicy` 符合 RPO 要求，恢复演练通过 | `kubectl get volumesnapshotclass` |
| 8 | 备份策略落地 | 使用 Velero 或云厂商方案对关键 PVC/命名空间执行定期备份，并验证还原 | 见 [[分布式存储/01-velero-backup-recovery.md|Velero 备份恢复]] |
| 9 | 容量与告警 | 监控 PVC 使用率、Pool/磁盘组容量、CSI 侧卡错误率，阈值 ≤ 80% 触发预警 | Prometheus + Alertmanager 规则 |
| 10 | CSI 供应链安全 | CSI 镜像来自受信仓库并经过签名校验，RBAC 最小权限，不挂载 hostPath | `kubectl get clusterrole,role -l app.kubernetes.io/component=csi-driver` |
| 11 | 有状态应用存储模式 | MySQL/PostgreSQL/Kafka/Redis 使用对应 StatefulSet 存储模式，禁止多 Pod 挂载 RWO | `kubectl get sts -n <ns> -o wide` |
| 12 | 运维文档与 Runbook | 扩容、迁移、故障、回滚步骤文档化，值班人员可 15 分钟内定位到根因 | 见 [故障排查速查](#故障排查速查) |

### 检查清单关键说明

- **WaitForFirstConsumer** 是生产 StorageClass 的推荐绑定模式，它延迟 PV 创建直到 Pod 被调度，可有效避免跨可用区挂载失败。若使用 `Immediate`，需额外验证拓扑标签与节点分布。
- **默认 StorageClass** 应仅用于开发测试或无需特别 SLA 的工作负载；生产敏感负载必须在 PVC 中显式声明 StorageClass，防止因默认类变更导致卷类型漂移。
- **临时存储限制** 不仅是 Pod 层面的 `ephemeral-storage` limit，还应在节点层面配置日志轮转、镜像清理阈值以及 kubelet `evictionHard` 参数，形成多层防护。
- **快照不等于备份**：快照通常与原始数据位于同一故障域，必须配合跨 Region/AZ 的复制或独立备份工具（如 Velero）才能满足真正的灾难恢复目标。

<!-- chunk: 关键风险与缓解措施 -->
## 关键风险与缓解措施

### 1. CSI 控制面或节点插件故障导致全集群无法挂载

**风险**: CSI Controller 单点、Node 插件 CrashLoopBackOff 或版本与 Kubernetes 不兼容，会造成新 Pod 无法启动、现有 Pod 重启后无法挂载。

**缓解措施**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 为 CSI Controller 设置 PDB 与多副本
kubectl get pdb -n kube-system -l app.kubernetes.io/component=csi-driver

# 2. 固定 CSI 镜像版本并校验签名，禁止直接使用 :latest
kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver \
  -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}'

# 3. 检查 CSI Node 插件 DaemonSet 滚动更新策略，确保至少保留 maxUnavailable=1
kubectl get ds -n kube-system -l app.kubernetes.io/component=csi-node -o yaml | grep -A 5 rollingUpdate
```
**补充要点**：CSI Controller 建议部署为 Deployment 并设置反亲和性，确保跨节点分布；Node 插件作为 DaemonSet 升级时应避免在业务高峰批量重启。升级前必须在非生产环境验证 CSI 驱动与当前 Kubernetes 版本的兼容性矩阵，并保留镜像 digest 以便快速回滚。

### 2. PVC Pending / 供应风暴拖垮存储后端

**风险**: 批量创建 StatefulSet 或 Job 时，大量 PVC 同时 Pending，触发 provisioner 对云 API 的限流或后端存储资源耗尽。

**缓解措施**:

```yaml
# 命名空间级 ResourceQuota 示例
apiVersion: v1
kind: ResourceQuota
metadata:
  name: storage-quota
  namespace: production
spec:
  hard:
    requests.storage: 2Ti
    persistentvolumeclaims: "50"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: storage-limit
  namespace: production
spec:
  limits:
  - type: PersistentVolumeClaim
    max:
      storage: 500Gi
    min:
      storage: 1Gi
```

**补充要点**：除命名空间级配额外，建议在集群入口通过 Admission Webhook（Kyverno/OPA）强制所有生产 PVC 声明 StorageClass，并限制单 PVC 最大容量。对于批量创建场景（如训练 Job、大数据任务），应在任务队列或控制器层面限制并发 provision 数量，避免对云 API 造成冲击。

### 3. 临时存储耗尽触发节点 DiskPressure

**风险**: 日志、缓存、emptyDir 无限制增长，导致节点 `DiskPressure`，kubelet 开始驱逐 Pod，影响同节点所有负载。

**缓解措施**:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-limits
spec:
  containers:
  - name: app
    resources:
      requests:
        ephemeral-storage: "1Gi"
      limits:
        ephemeral-storage: "5Gi"
    volumeMounts:
    - name: cache
      mountPath: /cache
  volumes:
  - name: cache
    emptyDir:
      sizeLimit: 2Gi
```

日常巡检命令:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes -o custom-columns=NAME:.metadata.name,DISK_PRESSURE:.status.conditions[?(@.type=="DiskPressure")].status
kubectl top node  # 结合 metrics-server 观察节点资源
```
**补充要点**：临时存储问题常出现在日志密集型应用、AI 训练缓存、CI/CD Runner 等场景。除限制 Pod 外，建议在节点层设置 kubelet `evictionHard` 阈值（如 `imagefs.available<15%`），并配合 `logrotate` 与容器运行时垃圾回收策略。对于必须使用大容量临时存储的工作负载，优先考虑使用 PVC 替代 emptyDir，以获得更好的可观测性与生命周期管理。

### 4. 未验证的备份导致灾难时无法恢复

**风险**: 仅创建快照但未做恢复演练，快照损坏、Region 级故障或跨 AZ 复制缺失时无法达到 RPO/RTO。

**缓解措施**:

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 每月执行一次恢复演练：从快照创建 PVC 并启动验证 Pod
kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: restore-test-pvc
  namespace: backup-test
spec:
  storageClassName: <production-sc>
  dataSource:
    name: <snapshot-name>
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes: [ReadWriteOnce]
  resources:
    requests:
      storage: 100Gi
EOF
```
**补充要点**：恢复演练应至少覆盖“同命名空间还原”、“跨命名空间克隆”、“跨可用区/Region 还原”三种场景。演练结果需记录 RTO（实际恢复时间）与 RPO（数据丢失窗口），并与业务连续性目标对比。对于数据库类应用，还需验证从快照恢复后的一致性（如 MySQL 的 `mysqlcheck`、PostgreSQL 的 `pg_checksums`）。

### 5. 多租户共享存储类导致成本与性能不可控

**风险**: 所有团队使用同一个默认 StorageClass，无法区分 IOPS 等级、加密策略、计费归属，也无法限制高成本卷滥用。

**缓解措施**:

```yaml
# 按业务线划分 StorageClass allowlist（结合 Kyverno/OPA/Gatekeeper 策略）
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-storageclass
spec:
  validationFailureAction: Enforce
  rules:
  - name: validate-sc
    match:
      resources:
        kinds:
        - PersistentVolumeClaim
    validate:
      message: "production 命名空间只能使用以下 StorageClass"
      pattern:
        spec:
          storageClassName: "fast-ssd-pl2|standard-ssd-pl1|encrypted-essd"
```

**补充要点**：建议为 StorageClass 添加业务线标签（如 `cost-center`、`team`、`environment`），便于 FinOps 团队进行费用分摊。对于高性能卷（如 ESSD PL2/PL3、EBS io2、Azure Premium SSD v2），应设置更严格的审批流程，避免非关键业务滥用高成本存储。

<!-- chunk: 日常运维操作 -->
## 日常运维操作

### 每日巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# storage-daily-check.sh
set -euo pipefail

kubectl get sc -o custom-columns=NAME:.metadata.name,PROVISIONER:.provisioner,EXP:.allowVolumeExpansion,BIND:.volumeBindingMode
kubectl get pvc -A --field-selector=status.phase=Pending
kubectl get volumeattachment -o jsonpath='{range .items[?(@.status.attached==false)]}{.metadata.name}{"\n"}{end}'
kubectl get pods -A -l app.kubernetes.io/component=csi-driver --field-selector=status.phase!=Running
kubectl get nodes -o custom-columns=NAME:.metadata.name,DISK_PRESSURE:.status.conditions[?(@.type=="DiskPressure")].status
```
### 容量与使用率检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 按 StorageClass 统计已分配容量
kubectl get pvc -A -o json | jq -r '
  .items | group_by(.spec.storageClassName) |
  .[] | "\(.[0].spec.storageClassName // "static"): \(map(.status.capacity.storage // "0Gi") | length) 个 PVC"
'

# 查看节点 ephemeral-storage 可分配量
kubectl get nodes -o custom-columns=NAME:.metadata.name,ALLOCATABLE:.status.allocatable.ephemeral-storage
```
### 快照生命周期管理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建保留策略为 7 天的快照（配合 CronJob 执行）
SNAPSHOT_NAME="backup-$(date +%Y%m%d-%H%M%S)"
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: $SNAPSHOT_NAME
  namespace: production
spec:
  volumeSnapshotClassName: default-snapshot-class
  source:
    persistentVolumeClaimName: app-data-pvc
EOF

# 清理 7 天前的快照
kubectl get volumesnapshot -n production -o json | \
  jq -r '.items[] | select(.metadata.creationTimestamp < "'$(date -d '7 days ago' --iso-8601)'") | .metadata.name' | \
  xargs -I {} kubectl delete volumesnapshot {} -n production
```
### PVC 在线扩容

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 `--dry-run` 或 diff 确认。

扩容前确认：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get sc <sc-name> -o jsonpath='{.allowVolumeExpansion}'   # 必须为 true
kubectl get pvc <pvc-name> -n <ns> -o jsonpath='{.status.phase}' # 必须为 Bound
```
执行扩容并验证：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch pvc <pvc-name> -n <ns> -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
kubectl get pvc <pvc-name> -n <ns> -w
# 状态序列：Resizing -> FileSystemResizePending -> Bound
kubectl exec deploy/<app> -n <ns> -- df -h | grep <mount-path>
```
更详细的步骤参考 [[02-pvc-expansion-guide.md|PVC 扩容指南]]。

### CSI 驱动升级前检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 备份当前 CSIDriver 与 StorageClass
kubectl get csidriver,storageclass -o yaml > csi-config-backup-$(date +%Y%m%d).yaml

# 2. 记录当前镜像版本
kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver \
  -o jsonpath='{range .items[*]}{.metadata.name}{": "}{.spec.containers[*].image}{"\n"}{end}'

# 3. 在测试集群验证相同 K8s 版本后再升级生产
```
### 容量规划与预测

生产存储容量规划不能仅看当前使用率，而应建立趋势预测模型。建议每月执行以下动作：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出近 30 天 PVC 容量增长趋势（需 metrics-server 或 Prometheus 数据）
kubectl get pvc -A -o json | jq -r '
  .items[] |
  "\(.metadata.namespace)/\(.metadata.name)\t\(.spec.resources.requests.storage)\t\(.status.capacity.storage // "N/A")"
'
```
规划原则：

- 块存储总可用容量应保留至少 **30% 安全水位**，对象存储保留 **20%**。
- 对年增长超过 **50%** 的业务线，提前一个季度申请扩容或新增 StorageClass。
- 建立按团队/命名空间的存储使用报表，识别僵尸 PVC 与过度申请。

### 存储混沌工程

定期开展受控故障注入，验证存储韧性。建议每季度执行一次：

- **CSI Node 插件重启**：在单节点删除 CSI Node Pod，观察同节点有状态 Pod 是否能保持 I/O 并在重启后正常挂载。
- **PVC 供应延迟注入**：通过限制 provisioner 并发或临时禁用 StorageClass，验证应用对 PVC Pending 的容错能力。
- **节点磁盘压力模拟**：在节点上填充临时文件触发 `DiskPressure`，验证 kubelet 驱逐顺序与业务影响面。

执行混沌实验前必须：获得变更审批、确认有完整备份、设定自动回滚条件、在监控大盘实时观察 golden signals。

<!-- chunk: 故障排查速查 -->
## 故障排查速查

| 现象 | 可能根因 | 确认命令 | 修复动作 |
|---|---|---|---|
| PVC 长期 `Pending` | SC 不存在、provisioner 异常、配额不足、拓扑约束不满足 | `kubectl describe pvc <name> -n <ns>` | 修正 SC、重启 CSI Controller、调整 ResourceQuota、确认 Pod 已调度 |
| Pod 事件 `MountVolume.SetUp failed` | VolumeAttachment 失败、节点无法访问后端、RWO 卷被多节点挂载 | `kubectl get volumeattachment \| grep <pv>` | 删除异常 Pod 触发重新挂载、检查节点网络/权限、确认 StatefulSet 未误调度到多节点 |
| PVC 扩容后 `FileSystemResizePending` | 文件系统扩容需重启 Pod | `kubectl get pvc <name> -n <ns>` | `kubectl rollout restart deployment/<name> -n <ns>` 或删除并重建 StatefulSet Pod |
| CSI Node 插件 `CrashLoopBackOff` | 驱动与内核/节点 OS 不兼容、hostPath 挂载失败 | `kubectl logs -n kube-system ds/csi-node -c csi-driver --tail=100` | 查看厂商 Release Notes，回滚到稳定版本，检查节点驱动依赖 |
| 节点 `DiskPressure=True` | emptyDir/日志/镜像占满节点盘 | `kubectl describe node <node>` | 清理镜像缓存、设置 log rotation、为 Pod 配置 ephemeral-storage limit |
| 快照 `ReadyToUse=false` | CSI snapshotter 异常、后端快照配额满 | `kubectl get volumesnapshot <name> -n <ns> -o yaml` | 检查 snapshot-controller 日志、清理旧快照、确认 VolumeSnapshotClass 参数 |

通用诊断脚本可直接使用 [[K8s存储/09-pv-pvc-troubleshooting.md|PV/PVC 故障排查与解决方案]] 中的 `pv-pvc-diagnostics.sh`。

### 排障核心原则

1. **先状态后日志**：先看 `kubectl get pvc,pv,VolumeAttachment` 确定资源状态，再深入 CSI 日志。
2. **先控制面后节点**：Provisioner/Attacher 问题通常体现为 PVC Pending 或 VolumeAttachment stuck；节点问题通常体现为 MountVolume 失败。
3. **先隔离后修复**：对无法快速恢复的有状态 Pod，优先通过快照保护数据，再执行重启/重建操作。
4. **记录时间线**：保留 `kubectl describe` 事件、`journalctl -u kubelet` 日志、云厂商存储事件，便于后续根因分析。

<!-- chunk: 与其他域的协作边界 -->
## 与其他域的协作边界

| 相关域 | 协作内容 | 典型交接点 |
|---|---|---|
| [[domain-05-security-compliance/README.md|安全与合规]] | 存储加密、KMS 密钥轮换、CSI RBAC、镜像签名、Pod Security Standards | 本域负责 SC/PVC 加密配置与 CSI 权限，安全域负责密钥生命周期与准入策略 |
| [[domain-06-observability/README.md|可观测性]] | PVC 使用率、CSI 侧卡指标、IOPS/延迟、节点磁盘压力告警 | 本域提供采集目标与阈值建议，可观测域提供 Dashboard、Alertmanager 路由与 SLO 模板 |
| [[domain-09-reliability-engineering/README.md|可靠性工程]] | 备份 RPO/RTO、跨 AZ/Region 复制、故障演练、容量规划 | 本域执行 PVC 快照与恢复演练，可靠性域制定业务连续性目标与混沌工程场景 |
| [[domain-10-troubleshooting-diagnostics/README.md|故障排查诊断]] | 存储类故障的升级路径、根因分析方法、War Room 信息模板 | 本域提供存储专用决策树，故障域提供通用排查框架与值班流程 |
| [[domain-12-cloud-providers/README.md|云服务商]] | 云盘 CSI 版本、可用区拓扑、IOPS/吞吐限制、云侧事件 | 本域定义 K8s 侧配置，云域跟踪厂商公告与容量上限 |
| [[domain-16-database-middleware/README.md|数据库中间件]] | MySQL/PostgreSQL/Kafka/Redis 的 StatefulSet 存储模式、备份一致性 | 本域负责通用 PV/PVC/SC 与 CSI 运维，数据库域负责应用级备份与一致性校验 |

<!-- chunk: 推荐阅读 -->
## 推荐阅读

### 本域相关

- [[K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]]
- [[K8s存储/07-storage-daily-operations.md|存储日常运维操作手册]]
- [[K8s存储/09-pv-pvc-troubleshooting.md|PV/PVC 故障排查与解决方案]]
- [[K8s存储/13-storage-security-compliance.md|存储安全与合规管理]]
- [[K8s存储/10-storage-backup-disaster-recovery.md|存储备份与灾难恢复]]
- [[02-pvc-expansion-guide.md|PVC 扩容指南]]
- [[分布式存储/01-velero-backup-recovery.md|Velero 备份恢复]]

### 跨域相关

- [[domain-05-security-compliance/README.md|domain-05-security-compliance]]
- [[domain-06-observability/README.md|domain-06-observability]]
- [[domain-09-reliability-engineering/README.md|domain-09-reliability-engineering]]

---

*本文件定位为生产准入与日常运维入口，具体 StorageClass 参数、CSI 驱动升级与云厂商操作请结合对应子文档执行。*


<!-- risk-assessed -->
