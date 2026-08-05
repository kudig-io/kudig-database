---
title: 备份恢复故障排查指南 [topic-structural-trouble-shooting]
description: '# 备份恢复故障排查指南'
summary: '1. **Velero 状态检查**：`kubectl get [[Pods|pods]] -n velero`，确认 velero [[DaemonSet|DaemonSet]] 和 Deployment 均为 Running。'
category: structural-troubleshooting
tags:
- backup
- restore
- velero
- etcd-snapshot
- disaster-recovery
- troubleshooting
- etcd
- mysql
- statefulset
- daemonset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 备份恢复故障排查指南 是什么
- 如何 备份恢复故障排查指南
- 备份恢复故障排查指南 故障排查
- 备份恢复故障排查指南 排障步骤
trigger_keywords:
- 备份恢复故障排查指南
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
- mysql-basics
- backup-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 备份恢复故障排查指南

> **适用版本**: [[Kubernetes|Kubernetes]] v1.28 - v1.32 | Velero v1.12+ | **最后更新**: 2026-05 | **难度**: 中高级

---

## 0. 快速诊断

1. **Velero 状态检查**：`kubectl get [[Pods|pods]] -n velero`，确认 velero [[DaemonSet|DaemonSet]] 和 Deployment 均为 Running。
2. **备份任务状态**：`kubectl get backup -n velero`，查看 Recent Backup 的 Phase（New/InProgress/Completed/Failed）。
3. **[[etcd|etcd]] 快照状态**：`kubectl get pods -n kube-system -l app=etcd-operator`，确认 etcd backup operator 正常运行。
4. **日志快速排查**：
   - Velero：`kubectl logs -n velero deployment/velero --tail=50 | grep -i error`
   - etcd-snapshot：`kubectl logs -n kube-system -l app=etcd-operator --tail=30`
5. **RTO/RPO 检查**：核对最近一次成功备份的时间戳，计算是否在 SLA 范围内。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Velero Backup 失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Backup Phase 为 Failed | `Backup "xxx" is failed: plugin error` | Velero Pod | `kubectl get backup -n velero` |
| Volume 快照失败 | `failed to snapshot volumes: CSI error` | CSI Driver | Velero Pod 日志 |
| 资源选择器遗漏 | 备份中缺少某些 Namespace 的资源 | 配置错误 | `velero backup describe` |
| Hook 执行失败 | `hook failed: exit code 127` | Application Hook | `kubectl describe backup` |
| 存储后端不可达 | `connection timeout` | 对象存储 | Velero Pod 日志 |

#### 1.1.2 etcd 快照异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 快照创建失败 | `failed to create snapshot: disk space insufficient` | etcd | etcd Pod 日志 |
| 快照不完整 | `snapshot file corrupted` | etcd | 快照文件校验 |
| 快照超时 | `snapshot timeout: 30m exceeded` | etcd-operator | CronJob 日志 |
| 监控未检测到备份缺失 | 无告警 | 监控配置 | AlertManager |

#### 1.1.3 恢复流程异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 恢复后数据丢失 | - | 备份数据问题 | 应用层验证 |
| 恢复顺序错误 | `resource not found` | Velero | `velero restore describe` |
| Namespace 不存在 | `namespaces "xxx" not found` | Kubernetes API | restore 日志 |
| CRD 未先恢复 | `no kind "xxx" is registered` | Kubernetes API | restore 日志 |

---

## 2. 排查方法与步骤

### 2.1 FTA 路径映射

```
TE-BACKUP-001 (备份恢复异常)
    │
    ├── OR → A. etcd 快照异常
    │       └── A1: 快照创建失败 → 磁盘空间不足 / etcd 过载
    │
    ├── OR → B. 应用级备份异常 (Velero)
    │       └── B1: Velero Backup 失败 → Plugin 错误
    │       └── B3: Volume 快照失败 → CSI Snapshot 错误
    │
    ├── OR → C. 存储后端异常
    │       └── C1: 存储不可达 → 网络/Endpoint 异常
    │       └── C2: 凭据失效 → AccessKey/Secret 过期
    │
    └── OR → E. 恢复流程异常
            └── E1: 恢复顺序错误 → 依赖资源未先创建
```

### 2.2 Velero Backup 失败排查

#### Step 1: 检查 Velero 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Velero Pod 状态
kubectl get pods -n velero

# 检查备份任务详情
kubectl describe backup -n velero {backup-name}

# 查看 Velero Pod 日志
kubectl logs -n velero deployment/velero --tail=100 | grep -i error
```
#### Step 2: 检查存储后端

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 BackupStorageLocation 状态
kubectl get backupstoragelocation -n velero

# 描述 BackupStorageLocation 获取详细错误
kubectl describe backupstoragelocation -n velero default

# 检查凭据 Secret
kubectl get secret -n velero velero-backup-creds
```
#### Step 3: 检查 Volume 快照

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 VolumeSnapshotClass
kubectl get volumesnapshotclass

# 检查 VolumeSnapshot
kubectl get volumesnapshot -n {namespace}

# 检查 CSI Driver 状态
kubectl get csidriver
kubectl get pods -n kube-system | grep csi
```
### 2.3 etcd 快照异常排查

#### Step 1: 检查 etcd-operator 状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd-operator Pod
kubectl get pods -n kube-system -l app=etcd-operator

# 查看 etcd-operator 日志
kubectl logs -n kube-system -l app=etcd-operator --tail=50
```
#### Step 2: 检查磁盘空间

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 etcd 节点磁盘空间
kubectl exec -n kube-system etcd-{node-name} -- df -h /var/lib/etcd

# 检查 etcd 数据库大小
kubectl exec -n kube-system etcd-{node-name} -- etcdctl endpoint status
```
#### Step 3: 检查快照 CronJob

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd-snapshot CronJob
kubectl get cronjob -n kube-system etcd-snapshot

# 查看最近执行的 Job
kubectl get jobs -n kube-system | grep etcd-snapshot

# 查看 Job 日志
kubectl logs -n kube-system job/etcd-snapshot-{timestamp}
```
### 2.4 恢复流程排查

#### Step 1: 检查备份数据可用性

```bash
# 列出所有备份
velero backup get

# 描述备份详情
velero backup describe {backup-name} --details

# 检查备份文件是否完整
velero backup-location get
```

#### Step 2: 执行恢复前验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查目标 Namespace 是否存在
kubectl get namespace {target-namespace}

# 检查恢复顺序依赖
velero restore describe {restore-name} --show-contents | grep -i error
```
#### Step 3: 执行恢复

```bash
# 创建恢复任务
velero restore create \
  --from-backup {backup-name} \
  --namespace-mappings old-ns:new-ns \
  --restore-volumes

# 查看恢复状态
velero restore get
velero restore describe {restore-name}
```

---

## 3. RTO/RPO 实战演练

### 3.1 RTO/RTO 定义

| 指标 | 定义 | 目标值（示例） |
|------|------|---------------|
| RPO | 数据恢复点目标 - 最多丢失多少数据 | 4 小时 |
| RTO | 恢复时间目标 - 恢复需要多久 | 2 小时 |

### 3.2 演练步骤

#### Step 1: 模拟灾难场景

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 备份当前状态
velero backup create pre-drill-backup --include-namespaces production

# 验证备份完成
kubectl get backup -n velero pre-drill-backup -o jsonpath='{.status.phase}'
# 期望输出: Completed
```
#### Step 2: 模拟数据丢失

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

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
# 删除目标 Namespace（模拟灾难）
kubectl delete namespace production  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 验证 Namespace 已删除
kubectl get namespace production
# 期望输出: NotFound
```
#### Step 3: 执行恢复并计时

```bash
# 开始计时
START_TIME=$(date +%s)

# 执行恢复
velero restore create drill-restore \
  --from-backup pre-drill-backup \
  --namespace-mappings production:production

# 等待恢复完成
velero restore wait drill-restore

# 结束计时
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo "恢复耗时: ${DURATION} 秒"
```

#### Step 4: 验证数据完整性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Pod 恢复状态
kubectl get pods -n production

# 检查 PVC 数据
kubectl exec -n production deployment/{app} -- ls -la /data

# 检查数据库数据
kubectl exec -n production statefulset/mysql -- mysql -e "SELECT COUNT(*) FROM app_table;"
```
### 3.3 演练报告模板

```
# DR 演练报告

## 基本信息
- 演练日期: YYYY-MM-DD
- 备份时间点: HH:MM
- 演练团队: @names

## RPO/RTO 测量
- RPO 实际值: X 小时（上次备份到演练开始）
- RTO 实际值: Y 分钟（恢复完成耗时）

## 发现问题
1. [问题描述]
   - 影响:
   - 建议修复:

## 结论
- [x] RPO 达标 / [ ] RPO 不达标
- [x] RTO 达标 / [ ] RTO 不达标
```

---

## 4. 自动修复动作

### 4.1 Velero Backup 失败自动修复

```yaml
auto_heal_actions:
  - action_id: "HA-BACKUP-001"
    description: "重新执行失败的备份任务"
    risk_level: "low"
    auto_executable: true
    command: |
      kubectl delete backup -n velero {failed-backup}
      velero backup create {new-backup-name} \
        --include-namespaces {namespace} \
        --storage-location default

  - action_id: "HA-BACKUP-002"
    description: "修复存储后端凭据"
    risk_level: "medium"
    auto_executable: false
    requires_approval: true
    command: |
      kubectl create secret generic velero-backup-creds \
        --from-file=cloud={provider}=/path/to/new/credentials
```

### 4.2 etcd 快照失败自动修复

```yaml
  - action_id: "HA-BACKUP-003"
    description: "清理旧快照并重新触发快照"
    risk_level: "medium"
    auto_executable: false
    requires_approval: true
    command: |
      kubectl exec -n kube-system etcd-{node} -- \
        rm -rf /var/etcd/backups/*
      kubectl create job -n kube-system etcd-manual-snapshot \
        --from=cronjob/etcd-snapshot
```

---

## 5. 相关文档

| 文档类型 | 路径 | 说明 |
|----------|------|------|
| FTA | `domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md` | 备份恢复异常故障树 |
| Domain | `domain-01-cluster-fundamentals/11-etcd-deep-dive.md` | 11-etcd-deep-dive |

---

## 6. 多集群备份策略

### 6.1 中心化备份架构

```yaml
centralized_backup:
  # 备份中心配置
  backup_hub:
    cluster: "backup-hub-cluster"
    location: "备份中心集群"
    storage:
      provider: "oss"  # 阿里云 OSS
      bucket: "k8s-backup-hub"
      region: "cn-hangzhou"
    retention:
      daily: 7
      weekly: 4
      monthly: 12
      yearly: 3

  # 联邦备份策略
  federation:
    source_clusters:
      - name: "prod-cluster-1"
        priority: 1
        backup_schedule: "0 2 * * *"  # 每天 02:00
      - name: "prod-cluster-2"
        priority: 1
        backup_schedule: "0 3 * * *"
      - name: "staging-cluster"
        priority: 2
        backup_schedule: "0 4 * * *"

    sync_interval: 1h
    bandwidth_limit_mbps: 100
```

### 6.2 备份带宽节流

```yaml
bandwidth_throttling:
  # 避免备份影响业务
  enabled: true
  peak_hours:
    - "09:00-18:00"  # 工作时间限制
    - "20:00-23:00"  # 晚高峰限制
  throttled_rate: "50Mbps"
  off_peak_rate: "unlimited"
  per_cluster_limit: "200Mbps"
```

---

## 7. 合规与审计

### 7.1 备份合规要求

```yaml
compliance_requirements:
  # 行业标准
  standards:
    - name: "SOC2 Type II"
      backup_window: 24h
      test_restore_frequency: quarterly

    - name: "ISO 27001"
      backup_encryption: required
      access_control: "need-to-know basis"
      audit_trail: required

    - name: "GDPR"
      personal_data_backup: "pseudonymized or excluded"
      right_to_erasure: "supported"

  # 数据分类
  data_classification:
    critical:
      - "etcd cluster data"
      - "Secret/ConfigMap with secrets"
      - "TLS certificates"
      - description: "必须加密、必须测试恢复"
    sensitive:
      - "User authentication data"
      - "Payment information"
      - description: "加密、可选测试恢复"
    standard:
      - "Application configs"
      - "Non-secret resources"
      - description: "标准备份策略"

  # 加密要求
  encryption:
    at_rest: "AES-256"
    in_transit: "TLS 1.3"
    key_rotation: 90d
    kms_key_id: "alias/backup-master-key"
```

### 7.2 备份审计日志

```yaml
backup_audit:
  # 记录所有备份操作
  operations_logged:
    - "backup_started"
    - "backup_completed"
    - "backup_failed"
    - "restore_started"
    - "restore_completed"
    - "restore_failed"
    - "backup_deleted"
    - "credentials_accessed"

  fields:
    timestamp: ISO8601
    operation: string
    backup_id: string
    cluster: string
    namespace: string
    size_bytes: integer
    duration_seconds: integer
    operator: "system|human"
    result: "success|failure"
    error_message: string

  # 合规报告生成
  compliance_reports:
    monthly:
      - "Backup Success Rate"
      - "Storage Usage Trends"
      - "Restore Test Results"
      - "Policy Violations"
    quarterly:
      - "Full Restore Test Certification"
      - "Disaster Recovery Drill Results"
      - "Compliance Attestation"
```

---

## 8. 备份验证自动化

### 8.1 自动恢复测试

```yaml
restore_test_automation:
  # 周期性恢复测试
  schedule:
    critical_data: "weekly"
    sensitive_data: "bi-weekly"
    standard_data: "monthly"

  # 测试环境
  test_environment:
    name: "backup-test-cluster"
    isolated: true
    network_policy: "deny-all"
    namespace_prefix: "restore-test-"

  # 测试步骤
  test_steps:
    - name: "创建测试 Namespace"
      command: "kubectl create ns {test-ns}"

    - name: "执行恢复"
      command: |
        velero restore create {restore-name} \
          --from-backup {backup-name} \
          --namespace-mappings {source}:{test-ns} \
          --namespace-mappings {source-pvc}:{test-ns-pvc}

    - name: "验证 Pod 就绪"
      command: "kubectl wait --for=condition=Ready pod -l app={app} -n {test-ns} --timeout=300s"

    - name: "验证数据完整性"
      command: |
        # 数据库数据校验
        kubectl exec -n {test-ns} deployment/{db} -- \
          mysql -e "SELECT COUNT(*) FROM {table};" | grep -q "^[0-9]"

    - name: "执行应用冒烟测试"
      command: |
        kubectl exec -n {test-ns} deploy/{app} -- \
          curl -s localhost:{port}/health | grep -q "ok"

    - name: "清理测试环境"
      command: "kubectl delete ns {test-ns}"

  # 测试结果记录
  test_results:
    stored_in: "backup-test-results/"
    retention: 90d
    fields:
      test_id: string
      backup_id: string
      test_timestamp: ISO8601
      test_duration_seconds: integer
      result: "pass|fail"
      issues_found: [string]
      data_integrity_verified: boolean
```

### 8.2 备份健康度监控

```yaml
backup_health_monitoring:
  # 监控指标
  metrics:
    - name: "backup_last_success_timestamp"
      type: "gauge"
      description: "上次成功备份时间戳"
      alert_if_age: "24h"

    - name: "backup_success_rate"
      type: "gauge"
      description: "备份成功率 (7天滚动)"
      alert_if_below: 0.95

    - name: "backup_size_bytes"
      type: "gauge"
      description: "备份大小"
      trend_alert: "+20%"

    - name: "restore_test_last_pass_timestamp"
      type: "gauge"
      description: "上次恢复测试通过时间戳"
      alert_if_age: "30d"

  # 告警规则
  alert_rules:
    - name: "Backup Failed"
      severity: P1
      condition: "backup_last_success_timestamp > 25h"
      channels: ["pagerduty", "slack-backup-alerts"]

    - name: "Backup Success Rate Low"
      severity: P2
      condition: "backup_success_rate < 0.90"
      channels: ["slack-backup-alerts"]

    - name: "Restore Test Overdue"
      severity: P2
      condition: "restore_test_last_pass_timestamp > 35d"
      channels: ["slack-backup-alerts"]

    - name: "Backup Size Anomaly"
      severity: P3
      condition: "backup_size_bytes change > +30%"
      channels: ["slack-backup-alerts"]
```

---

## 9. 灾难恢复演练

### 9.1 DR 演练计划模板

```yaml
dr_drill_plan:
  # 演练类型
  drill_types:
    - name: "桌面演练 (Tabletop)"
      duration: 2h
      participants: "SRE + Management"
      scope: "Read-only 演练"

    - name: "部分恢复演练"
      duration: 4h
      participants: "SRE + Dev"
      scope: "非生产 Namespace 恢复"

    - name: "全量 DR 演练"
      duration: 8h
      participants: "Full Team"
      scope: "生产环境完整恢复"
      requires_approval: "VP Engineering"

  # 演练场景
  scenarios:
    - id: "DR-001"
      name: "单集群完全不可用"
      rto_target: 4h
      rpo_target: 1h
      steps:
        - "模拟集群所有节点宕机"
        - "验证 DNS/ ingress 切换"
        - "恢复备份到备用集群"
        - "验证应用功能"

    - id: "DR-002"
      name: "etcd 数据丢失"
      rto_target: 2h
      rpo_target: 15m
      steps:
        - "模拟 etcd 数据损坏"
        - "从最新快照恢复"
        - "验证集群状态"
        - "确认无数据丢失"

    - id: "DR-003"
      name: "误删除 Namespace"
      rto_target: 1h
      rpo_target: 30m
      steps:
        - "模拟误删关键 Namespace"
        - "从备份恢复 Namespace"
        - "验证数据完整性"
        - "确认服务恢复"
```

### 9.2 DR 演练检查清单

```yaml
dr_drill_checklist:
  pre_drill:
    - [ ] "DR 演练计划已审批"
    - [ ] "备用集群可用"
    - [ ] "备份数据完整性验证"
    - [ ] "演练团队已通知"
    - [ ] "回滚方案已准备"
    - [ ] "监控告警已临时调整"
    - [ ] "演练时间窗口已确认"

  during_drill:
    - [ ] "记录开始时间"
    - [ ] "按演练步骤执行"
    - [ ] "记录每个步骤耗时"
    - [ ] "记录发现的问题"
    - [ ] "定期汇报状态"

  post_drill:
    - [ ] "记录结束时间"
    - [ ] "计算 RTO 实际值"
    - [ ] "计算 RPO 实际值"
    - [ ] "验证所有服务功能"
    - [ ] "清理演练数据"
    - [ ] "恢复监控告警"
    - [ ] "生成 DR 演练报告"
    - [ ] "更新 DR 计划改进项"
```

---

## 10. 备份容量规划

### 10.1 存储容量计算

```yaml
storage_capacity_planning:
  # 增长因子
  growth_factors:
    data_growth_rate: 1.2  # 每月 20% 增长
    new_clusters: 2  # 未来 6 个月新增集群
    retention_extension: 1.5  # 合规要求延长保留期

  # 计算公式
  calculation:
    current_size_tb: 10
    monthly_growth: 1.2
    retention_months: 12
    total_size_tb: |
      current_size * (1 + monthly_growth)^retention_months *
      (1 + new_clusters_factor)
    safety_buffer: 1.2  # 20% 安全缓冲
    recommended_size_tb: total_size * safety_buffer

  # 告警阈值
  alert_thresholds:
    warning: 70%  # 容量使用 70% 告警
    critical: 85%  # 容量使用 85% 告警
    emergency: 95%  # 容量使用 95% 立即处理
```

### 10.2 成本优化

```yaml
cost_optimization:
  # 存储分层
  tiered_storage:
    hot:
      - name: "最近 7 天备份"
        storage_class: "Standard"
        replication: 3
    warm:
      - name: "8-30 天备份"
        storage_class: "IA"  # 低访问频次
        replication: 2
    cold:
      - name: "31-365 天备份"
        storage_class: "Archive"
        replication: 1

  # 压缩与去重
  optimization:
    compression: true
    compression_algorithm: "gzip"
    dedup: true
    dedup_chunk_size: "4KB"
    estimated_savings: "60-70%"

  # 定期清理
  cleanup:
    orphaned_backups: 7d  # 删除孤立备份
    failed_backups: 30d  # 保留失败备份供审计
    expired_backups: immediate  # 过期立即删除
```

---

## 11. 备份恢复 SLA

### 11.1 SLA 目标

```yaml
backup_sla:
  # 备份 SLA
  backup:
    schedule_compliance: 99.9%  # 备份按计划执行率
    success_rate: 99.5%  # 备份成功率
    max_backup_duration: 2h  # 最大备份时长
    max_recovery_point_age: 4h  # 最大恢复点延迟 (RPO)

  # 恢复 SLA
  restore:
    rto_target: 2h  # 恢复时间目标
    rto_guaranteed: 4h  # 保证恢复时间
    rpo_target: 1h  # 恢复点目标
    partial_restore_time: 30m  # 部分恢复时间
    full_restore_time: 4h  # 全量恢复时间

  # 测试 SLA
  test:
    restore_test_frequency: monthly
    drill_frequency: quarterly
    drill_success_target: 100%
```

### 11.2 SLA 监控报告

```yaml
sla_reporting:
  # 每日 SLA 报告
  daily:
    generated_at: "06:00 UTC"
    channels: ["#sre-metrics"]
    metrics:
      - "昨日备份成功率"
      - "RPO 实际值"
      - "异常备份列表"

  # 每周 SLA 报告
  weekly:
    generated_at: "Monday 06:00 UTC"
    channels: ["#sre-leadership"]
    metrics:
      - "本周 SLA 达成率"
      - "备份健康度评分"
      - "待处理行动项"

  # 每月 SLA 报告
  monthly:
    generated_at: "1st of month 06:00 UTC"
    channels: ["#management"]
    metrics:
      - "SLA 趋势分析"
      - "容量规划建议"
      - "成本分析"
      - "合规状态"
```

---

> **版本**: v1.1
> **维护团队**: SRE Team / Platform Team
> **更新日期**: 2026-05-19
> **下一步**: 集成到备份管理平台，支持自动备份健康度检测和 DR 演练自动化
| Skills | `domain-10-troubleshooting-diagnostics/topic-skills/backup-restore-skill.md` | 备份恢复技能卡片 |

---

> **版本**: v1.0
> **维护团队**: SRE Team / Platform Team
> **下一步**: 集成到 AI Agent 执行引擎，支持自动备份健康度检测

## Related

- 08-docker-troubleshooting-guide
- [[entities/kubernetes.md|kubernetes]]
- [[hot|hot]]
- [[domain-17-system-foundation/知识字典/workloads/cronjob.md|cronjob]]

## See Also

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/02-tekton-troubleshooting|02-tekton-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/03-flux-image-automation-troubleshooting|03-flux-image-automation-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/01-gitops-devops-troubleshooting|01-gitops-devops-troubleshooting]]
- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/14-gitops-devops/02-tekton-troubleshooting|02-tekton-troubleshooting]]

```

<!-- risk-assessed -->
