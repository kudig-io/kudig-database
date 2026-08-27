---
skill_id: "SKILL-STORE-002"
skill_name: "备份恢复与灾难恢复诊断 / Backup & Disaster Recovery Failure Diagnosis & Remediation"
version: "1.0"
category: "storage"
severity_range: "P0-P2"
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
estimated_resolution_time: "20-120min"
risk_level: "critical"
agent_execution_mode: "L1-advisory"
trigger_keywords:
  - "backup failed"
  - "restore failed"
  - "velero backup"
  - "etcd snapshot"
  - "灾难恢复"
  - "备份失败"
  - "恢复失败"
  - "快照"
  - "RTO/RPO 超标"
  - "数据丢失"
trigger_events:
  - "BackupFailed"
  - "RestoreFailed"
  - "SnapshotFailed"
  - "PodVolumeBackupFailed"
trigger_metrics:
  - 'velero_backup_failure_total'
  - 'velero_backup_last_successful_timestamp < time() - 86400'
  - 'velero_restore_failed_total'
  - 'etcd_snapshot_duration_seconds_bucket'
difficulty: "advanced"
reading_level: "advanced"
audience:
  - SRE
  - 平台工程师
  - 运维工程师
estimated_read_time: "15min"
prerequisites:
  - kubectl-basics
  - velero-basics
  - storage-basics
related_skills:
  - "./ts-storage.md"
  - "../../02-控制面/etcd/backup-restore-etcd.md"
  - "../../03-节点/node/"
fta_refs:
  - "../../02-控制面/etcd/backup-restore-fta.md"
knowledge_refs:
  - "./ts-storage.md"
  - "../../02-控制面/etcd/backup-restore-etcd.md"
cross_refs:
  - type: "fta"
    path: "../../02-控制面/etcd/backup-restore-fta.md"
    label: "备份恢复故障树分析"
  - type: "doc"
    path: "../../02-控制面/etcd/backup-restore-etcd.md"
    label: "etcd 快照与控制面备份恢复"
  - type: "doc"
    path: "./ts-storage.md"
    label: "CSI/PVC 存储通用排查"
authors:
  - name: KUDIG Team
    role: contributor
---

# 备份恢复与灾难恢复诊断 / Backup & Disaster Recovery Failure Diagnosis & Remediation

备份是数据安全的最后一道防线。Kubernetes 中存在两类备份机制：**资源级备份**（Velero 管理 k8s 对象 + Restic/Kopia 卷快照）与 **控制面级备份**（etcd snapshot）。两者相互独立、不可互替——etcd 快照不包含 PV 数据内容，Velero 备份也无法直接恢复被摧毁的控制平面。

备份系统的故障分两类性质完全不同的风险：其一，**备份任务失败**导致 RPO 窗口悄悄拉长（静默失效，多数组织在真正需要时才发现）；其二，**恢复执行失败**发生在最紧张的灾后时段，时间压力最大且回退余地最小。本 Skill 同时覆盖两类场景，并对"演练式验证"给出强制要求。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| Velero Backup 状态 Failed/PartiallyFailed | `kubectl get backups.velero.io -A` | 0.95 |
| 周期 Schedule 长时间无新成功备份 | `velero backup get` 按完成时间排序 | 0.95 |
| Restore 卡在 InProgress 或 Failed | `velero restore get/describe` | 0.95 |
| 节点卷快照（CSI VolumeSnapshot）失败 | `kubectl get volumesnapshot -A` 内容 Error | 0.90 |
| 备份对象存储写入超时 | velero pod 日志 upload/repo 相关错误 | 0.85 |
| etcd snapshot 执行失败或耗时异常增长 | cron job 日志 / `etcdctl snapshot status` | 0.90 |

**排除条件**: 单纯 PVC Pending → SKILL-STORE-001；etcd 健康 but 配额满等控制面故障先走 SKILL-CP-001

## 快速分级（2 分钟内完成）

```
业务影响 × 时间窗口
├── 恢复正在执行且失败（灾中）────────────────→ P0 CRITICAL（启用应急升级通道）
├── 关键 Schedule 连续 >=2 个周期未产出成功备份 ──→ P0（24h 内必须修复）
├── 全部备份超过 7 天未成功但无人感知 ──────────→ P0（审计事故，含复盘要求）
├── 单个一次性备份失败 ─────────────────────────→ P1
├── PartiallyFailed 但核心 namespace 已覆盖 ───→ P1（排查 skipped resources）
├── 快照性能下降但仍在成功 ─────────────────────→ P2
└── 备份存储空间余量告警 ───────────────────────→ P2
```

**立即升级条件**：
- 正在执行真实灾害恢复时出现新失败：立即双通道并行（继续排查 + 启动备用恢复路径如异地副本/手工导出）
- 发现备份从未成功过（虚假安全感）：信息同步至管理层与合规负责人

## 执行流程

```
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.6
│ Phase 1      │    内容: Velero/etcd 备份状态总览 + 最近错误定位（只读）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.7
│ Phase 2      │    内容: 组件深度分析（velero/pod-volume/restic 节点代理/对象存储）（只读）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测
       ▼
┌──────────────┐    Step: D3.1-D3.5
│ Phase 3      │    内容: 手动试跑小规模备份/恢复到隔离集群验证（低风险）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~009
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6（必须包含至少一次真实恢复演练验证）
│ 验证确认      │
└──────────────┘
```

## 症状识别

### 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | Backup phase=Failed 且 logs 提及 blob/s3/minio 上传错误 | `velero backup describe` + `--details` | 0.95 | 无 |
| S2 | PodVolumeBackup (restic/kopia) Failed 多集中于特定节点 | 按 node 分组统计 BSL 与 PVB 状态 | 0.90 | 若全节点均匀失败更可能是凭证问题 |
| S3 | CSI VolumeSnapshot Content 未生成且 events 引用 snapshot-controller | VSC / volumesnapshotcontent 检查 | 0.90 | 无 |
| S4 | Restore 卡住 InProgress > 1h | restore progress 详细输出停留在某资源 | 0.95 | 大型集群正常窗口需基线参考 |
| S5 | Restore 完成但应用 NotReady | 目标 ns pods/events 逐项检查 | 0.80 | 参考 SKILL-WORK-* 系列交叉诊断 |
| S6 | Schedule 到点没有触发任何新 backup | controller 日志 schedule reconciler 错误 | 0.85 | 手动 paused=true 属人为暂停 |
| S7 | 备份成功但大小骤降 | 比较历史 backup size 序列 | 0.75 | 可能误删 include/exclude 或空目录 |

## 快速命令集

```bash
# ── Phase 1 快速检查（只读）─────────────────────────────
# D1.1 备份全局健康
velero backup get | tail -10          # 关注最近完成时间与 STATUS
kubectl get schedules.velero.io -A    # 每个 schedule 的 last-backup 是否新鲜
velero schedule get                   # 含 CronExpr 与 LastBackup

# D1.2 定位最近一次失败的详情（三层结构: backup -> volume errors)
LAST_FAILED=$(velero backup get -o json | jq -r '.items | map(select(.status.phase=="Failed")) | sort_by(.status.completionTimestamp) | reverse[0].metadata.name')
velero backup describe "${LAST_FAILED}" --details
velero backup logs "${LAST_FAILED}" | grep -iE "error|failed" | head -50

# D1.3 恢复操作状态检查
velero restore get
velero restore describe ${RESTORE_NAME} --details
velero restore logs ${RESTORE_NAME}

# D1.4 备份存储位置(BSL)可达性
kubectl get bsl                        # BackupStorageLocation credential/endpoint 状态
kubectl describe bsl default           # phase 必须为 Available

# D1.5 快照位置(VSL)与 snapshot-controller
kubectl get vsl                        # VolumeSnapshotLocation
kubectl get volumesnapshot,volumesnapshotcontent -A --no-headers | head

# D1.6 Velero 自身组件
kubectl get pods -n velero
kubectl top pod -n velero              # 大型备份可能 OOM

# ── Phase 2 深度检查（只读）─────────────────────────────
# D2.1 Velero 主控制器日志
kubectl logs deploy/velero -n velero --tail=300 | grep -viE "^I[0-9]{4}" | tail -40

# D2.2 节点级 restic/kopia 代理日志（BVP/PVB 失败高发区）
kubectl get ds -n velero               # node-agent daemonset 名称可能是 node-agent / restic
FAILED_NODE=$(kubectl get pvb -A -o jsonpath='{range .items[*]}{.spec.node}{"\n"}{end}' | sort | uniq -c | sort -rn | head -1 | awk '{print $2}')
POD_ON_NODE=$(kubectl get pods -n velero -o wide | grep "${FAILED_NODE}" | awk '{print $1}' | head -1)
kubectl logs ds/node-agent -n velero --tail=200 | grep -v "^time=" | grep -iE "error" | tail -30

# D2.3 对象存储桶连通性与权限探测（从 cluster 内发起）
BSL_ENDPOINT=$(kubectl get bsl default -n velero -o jsonpath='{.spec.objectStorage.endpoint}')
kubectl run probe-blob --image=curlimages/curl --restart=Never --rm -it -- curl -sI --max-time 10 "${BSL_ENDPOINT}" || echo "blob endpoint unreachable"

# D2.4 CSI snapshot 功能链路
kubectl get deploy snapshot-controller -n kube-system   # v1.28+ 通常为 stable
kubectl get crd volumesnapshots.snapshot.storage.k8s.io
kubectl describe volumesnapshot <snap> -n <ns> | tail -30

# D2.5 etcd snapshot（若自建集群）
# 在 control-plane 节点上：
# ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
#   --cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=... --key=... \
#   endpoint health && etcdctl snapshot save /tmp/test-snap.db --command-timeout=60s

# D2.6 排查 skipped resources（备份完整性缺口）
velero backup describe "${LAST_OK_BACKUP}" --details | grep -A200 "Errors:" || true

# D2.7 Namespace Quota 是否阻止了 restore 期间的对象重建
kubectl get resourcequota,limitrange -n <target-ns>

# ── Phase 3 主动探测（低风险，优先非生产集群）────────────
# D3.1 手动小范围备份试运行
velero backup create smoke-test-${NOW} --include namespaces=default --wait=false

# D3.2 (强烈推荐) 在隔离的 staging 集群做完整还原演练
# (确保已有独立 kubeconfig + 小规格实例池)

# D3.3 验证单一 PVC 数据可恢复（用 file-level 还原方式，无需整库回滚）
# restic 解包示例（在节点上）:
# restic snapshots --repo ... ; restic restore <snapshot-id> --target /tmp/recovered --path /host_pods/...

# D3.4 验证 schedule 触发器（手动注入 annotation 不适用，改用 immediate backup 替代观察 pipeline 各阶段耗时）
# D3.5 使用 --dry-run 输出 restore plan 而不实际改动目标集群
velero restore create --from-backup "${LATEST}" --dry-run -o yaml | head -80
```

## 根因分类

### 根因清单

| RC ID | 根因 | 概率 | 典型证据 | 首选修复 | 风险 |
|-------|------|------|---------|---------|------|
| RC-001 | 对象存储凭证过期/权限不足 | 高 | BSL unavailable; 403 SignatureDoesNotMatch | REM-004 更新 BSL secret | LOW |
| RC-002 | 对象存储不可达（网络策略/VPC Endpoint/证书） | 高 | curl probe 超时或 x509 错误 | REM-003 修通网络路径 | MEDIUM |
| RC-003 | 存储/负载导致快照慢而超时（大盘备份） | 高 | BackupSize 长期 >500GB；upload QPS 低 | REM-007 分片+并行+增大超时 | MEDIUM |
| RC-004 | node-agent (restic/kopia) 故障集中在部分节点 | 中 | PVB by node 分布倾斜；ds pod Crashing | REM-005 重启或重装 node-agent | MEDIUM |
| RC-005 | Velero Server OOM / 协调延迟 | 中 | OOMKilled；controller restart loop | REM-005 同上 |
| RC-006 | restore 因配额/LimitRange/CRD 缺失被拒 | 中 | target ns events forbidden/no matches kind | REM-006 先建环境再恢复 | MEDIUM |
| RC-007 | CSI snapshot 链路缺失（controller/CSIDriver 版本） | 中 | no snapshotter; VSC empty | REM-002 安装 snapshot CRDs+controller | HIGH |
| RC-008 | Schedule 内 Cron 表达式/timezone/expected-pause 异常 | 低 | manual pause / schedule reconcile error | REM-008 修 schedule 定义 | LOW |
| RC-009 | etcd snapshot 失败（空间/权限/集成脚本 bug） | 低 | script exit code 非 0；disk usage 满 | REM-009 清理+修正脚本 | HIGH |

### FTA 映射

| RC | FTA 底事件 step_ids | 文件 |
|----|--------------------|-----|
| RC-001 | evt_blob_credential_invalid, evt_blob_authz_denied | topic-fta/list/backup-restore-fta.md |
| RC-002 | evt_blob_unreachable, evt_tls_selfsigned | 同上 |
| RC-003 | evt_backup_timeout, evt_slow_upload | 同上 |
| RC-004 | evt_node_agent_crash, evt_pvb_fail | 同上 |
| RC-005 | evt_velero_server_oom, evt_reconcile_lag | 同上 |
| RC-006 | evt_restore_quota_deny, evt_restore_crd_missing | 同上 |
| RC-007 | evt_csi_controller_missing, evt_snapshot_api_error | 同上 |
| RC-008 | evt_schedule_misconfig | 同上 |
| RC-009 | evt_etcd_snap_disk_full, evt_etcd_snap_script_bug | 同上 |

### 数据来源一致性说明
本文档融合 domain-12/31 + domain-9/12 两份来源并泛化至通用 Velero v1.13+ 行为。版本差异较大（如 restic -> kopia 迁移），生产中以现场组件版本文档为准。

## 修复操作

### REM-001: （预留编号）数据恢复后的应用回归脚本 🟢
**说明**: 属于流程性脚本建议而非危险操作；在 DR 后用于批量探活。
此处保留编号以便于整体编排索引连续性。

### REM-002: 补齐 CSI Snapshot 能力栈 🔴
**适用根因**: RC-007
**前置检查**: 确认 CSIDriver 是否原生支持 snapshot class（例如 ACK 盘云盘需开启 snapshot 服务，EKS EBS 需要 snapshot controller 独立安装）
**步骤**:
1. 按 K8s 官方 external-snapshotter 项目顺序部署 CRDs -> snapshot-controller -> RBAC
2. 创建 VolumeSnapshotClass 并设置 deletionPolicy=Retain 以免误删底层快照
3. 用一个小型 PVC 端到端试创建快照再删除
**审批要求**: 涉及新组件上线，需平台变更窗口
**验证**: volumesnapshotcontent ReadyToUse=True

### REM-003: 打通对象存储网络通路 🟢
**适用根因**: RC-002
**步骤**:
1. 明确阻断层级：NetworkPolicy / SecurityGroup / Route Table / DNS / Proxy
2. 自签名 CA 场景将 CA 添加至 velero deployment trust store (`extraVolumes`挂载并配置 env)
3. 公有云场景优先使用 VPC Endpoint 走内网以避免公网限流和费用
**验证**: BSL phase 回复 Available

### REM-004: 轮转对象存储凭证 🔴
**适用根因**: RC-001
**步骤**:
1. 在 IAM/AccessKey 平台创建新 key（最小权限限制在 bucket 内部子目录）
2. 编辑对应 secret `cloud-credentials`
3. `kubectl rollout restart deploy/velero -n velero` 使新凭据生效
4. 触发一次小规模备份验证
**审批要求**: 凭证敏感，遵循双盲或工单化交接，禁止聊天工具明文传递 Secret AccessKey
**验证**: 新增 backup 成功 Completed

### REM-005: 修复 node-agent / Velero Server 🟢
**适用根因**: RC-004, RC-005
**步骤**:
1. 单节点问题：delete 问题 pod 让 ds 自动拉起；必要时 drain 后重启kubelet（联动 SKILL-NODE-001）
2. server OOM：按备份吞吐调整 velero memory limit（经验值：每 TB 并发约 512Mi~1Gi 起步），同时降低 `podVolumeOperationTimeout`、启用 concurrency 控制
3. 版本兼容性导致的 crashloop：升级到近期 stable patch，严禁跨大版本直跳
**验证**: 连续三个周期无 restart；同类型备份通过

### REM-006: 恢复前环境准备清单 🔴
**适用根因**: RC-006
**核心思想**: Restore 不是裸奔，必须按依赖顺序准备地基
**步骤（顺序强制）**:
1. 先安装所需 CRD 层（参考 App-of-Apps 第一层）
2. 建 namespace & ResourceQuota & LimitRange（若原集群有约束）
3. 建 StorageClass 与动态供应能力
4. 确保 Webhook 安全策略允许该来源的资源模板进入
5. 再执行 velero restore（加 `--namespace-mappings` / label filter 精细化控制范围）
6. 分阶段恢复：stateless 先行 -> stateful 最后；多 phase 之间留健康检查间隔
**审批要求**: 生产恢复必须经 Inc Commander 授权；涉及数据变更前必须有时间点备份兜底
**验证**: 应用层健康 + 业务指标恢复区间内

### REM-007: 大规模备份切片调优 🟢
**适用根因**: RC-003
**手段矩阵**:

| 维度 | 参数/手段 | 经验值 |
|------|----------|--------|
| 并发 | clientConcurrency.podVolume | 根据CPU core 调整 (4c 建议 4) |
| 超时 | podVolumeOperationTimeout / csiSnapshotTimeout | 默认 4h 大盘上调至 8h+ |
| 切片 | 拆分多个 include-namespaces schedules 错峰 | 大 NS 单独一份 |
| 过滤 | 排除 ephemeral/log/cache 目录 via FS freeze annotations | 降低 PVB IO 约 30% |
| 压缩 | 开启 podVolume 一体化压缩 (kopia 默认 zstd-fast) | CPU 换带宽权衡 |

**验证**: 完成 window 保持在业务 SLA 内

### REM-008: 修正 Schedule 定义 🟢
**适用根因**: RC-008
**步骤**: edit schedule；检查 `useOwnerReferencesInBackup`, timezone, paused flag；为重要业务开启 successHistoryLimit 控制对象堆积。
**验证**: 下一个 tick 正常产出新 backup

### REM-009: etcd 快照故障处置 ⚫ CRITICAL
**适用根因**: RC-009
**步骤**:
1. 登陆对应 control-plane 节点（联动 SKILL-CP-001 步骤）
2. 清理旧快照或将快照输出重定向至更大分区（建议独立盘/OSS bucket）
3. 检查脚本逻辑是否正确关闭文件句柄、是否使用了带 TTL 的临时目录
4. 强制手动完成一次 snapshot save 验证文件 checksum 可用：`ETCDCTL_API=3 etcdctl snapshot status <file>`
5. 上传远端（如已集成）确认成功
**风险提示**: 与 SKILL-CP-002 的升级快照流程互斥，避免同时进行；绝对不能在生产 etcd 上随意扩容磁盘 fstab 格式化 —— 仅清理或增加挂载点。
**审批要求**: Control plane 变更双人在场执行；先做 Cluster State Capture (journalctl/sysstat/fstrim info)
**验证**: cron下一次执行成功且远端有副本校验。

## 验证确认

| 编号 | 项目 | 方法 | 通过标准 |
|-----|------|------|---------|
| V1 | BSL / VSL 可用性 | `kubectl get bsl,vsl` | 均 Available |
| V2 | 新备份成功产出 | 下一个周期的 backup STATUS = Completed | ✅ |
| V3 | 备份完整性抽查 | `velero backup describe --details` 无 skipped resources（或在容许白名单内） | ✅ |
| V4 | **恢复演练通过**（关键项） | 至少月度一次在隔离环境完成全量还原，比对核心表行数/hash | 业务可启动 |
| V5 | 文件级恢复可用性 | 抽取单个文件走 PVB 路径下载校验 checksum | md5 一致 |
| V6 | RPO/RTO 度量报告 | 将本次 incident 从发现到闭环的时间记录进 DR Dashboard | RPO < 上报阈值 |

## 升级协议

升级条件：

- 恢复演练连续两次失败（触发架构评审级别讨论：如考虑切换至云厂商原生 backup service 或 CBT 类方案）
- 发现长期虚假成功（S7 场景误报）
- 需要修改删除策略 deletionPolicy / 生命周期规则
- 需要 rebuild 整套 Velero 及其 repositories（含 redaction of history）

## 附录 A: 最小 Velero 恢复 SOP 速查卡

```text
[步骤] 停写 -> 备时间点 ->
  1. kubectl cordon / scale down ingress if needed
  2. choose snapshot or backup point closest to failure with verification hash
  3. prepare env (CRD/quota/storageclass/webhook rules)
  4. velero restore create --from-backup X [filters...]
  5. watch until phase=Completed
  6. app health loop & spot check data rows
  7. uncordon & traffic switch back
  8. log metrics into DR tracker
```

## 附录 B: 云厂商特异性

| 环境 | 主推方案 | 注意事项 |
|------|---------|---------|
| ACK | Velero + OSS；或 云盘快照小组件 | NAS 型 storageclass 部分不支持 volume snapshot，走 PVB |
| EKS | AWS Backup for EKS (推荐) 或 Velero | PodVolumeBackup 建议 kopia 模式；IRSA 凭证绑定 |
| GKE | Backup for GKE(托管)；PV 具备原生 PD snapshot | 区域级快照享低成本存储类 BackupsSchedule API |
| 自建 | Velero + MinIO/OSS；kubeadm 附带 etcd-cronjob | 双通道必备：Velero 资源 + 外部脚本做 DBMS 层一致性 dump |

## 附录 C: Agent 自动化接口契约

```yaml
agent_contract:
  preconditions:
    tools_required: [kubectl, velero]
    rbac_minimum:
      group: velero.io
      verbs: [get, list]            # 只读巡检所需
  safe_actions:                     # 默认允许建议或执行（L1 建议为主）
    - collect_status_overview
    - run_diagnose_smoke_backup     # 只动非生产 namespace
  approval_required_actions:
    - REM-002 install-snapshot-stack
    - REM-004 rotate-blob-credential
    - REM-006 production-restore-run
    - REM-009 control-plane-etcd-actions
  hard_block_actions:               # Agent 绝不允许触碰
    - delete existing backups/repositories
    - change deletionPolicy without dual approval
    - operate on etcd datastore directly except via validated runbook scripts
  escalation_path:
    primary: platform-sre-oncall
    secondary: data-protection-lead
```
