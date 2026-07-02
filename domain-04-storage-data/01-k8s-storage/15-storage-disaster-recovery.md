---
title: 15 - 存储灾备与迁移策略
description: '# 15 - 存储灾备与迁移策略'
summary: 'kubectl get pvc -n production -o json | \'
category: storage
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- prometheus
- mysql
- postgresql
- statefulset
- job
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 存储灾备与迁移策略 是什么
- 如何 存储灾备与迁移策略
- Kubernetes 6 storage 最佳实践
trigger_keywords:
- 存储灾备与迁移策略
- storage
prerequisites:
- kubectl-basics
- storage-basics
- prometheus-basics
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
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 15 - 存储灾备与迁移策略

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **运维重点**: 灾难恢复、数据迁移、业务连续性
>
> **文档定位**: 本文档聚焦 **灾备架构分层设计**、**自动/手动故障转移**、**存储迁移**、**跨集群数据同步** 及 **灾备演练与 SLA 管理**。如需了解 Velero 企业备份方案、应用级（MySQL/PostgreSQL）备份操作及 VolumeSnapshot [[CronJob|CronJob]] 自动化，请参阅 [10 - 存储备份与灾难恢复](./10-storage-backup-disaster-recovery.md)。

<!-- chunk: 目录 -->
## 目录

1. [灾备架构设计](#灾备架构设计)
2. [数据备份策略](#数据备份策略)
3. [灾难恢复流程](#灾难恢复流程)
4. [存储迁移方案](#存储迁移方案)
5. [跨集群数据同步](#跨集群数据同步)
6. [业务连续性保障](#业务连续性保障)
7. [灾备演练与测试](#灾备演练与测试)
8. [RTO/RPO管理](#rtorp管理)

---

<!-- chunk: 灾备架构设计 -->
## 灾备架构设计

### 灾备架构模式

```
主数据中心 ──实时同步──→ 同城灾备中心 ──异步复制──→ 异地灾备中心
    ↓                        ↓                        ↓
生产集群                  灾备集群                  归档集群
    ↓                        ↓                        ↓
主存储系统                同步存储                  异步存储
```

### 灾备策略配置

```yaml
# 灾备策略定义
apiVersion: disaster-recovery.storage.k8s.io/v1
kind: DisasterRecoveryPolicy
metadata:
  name: enterprise-dr-policy
spec:
  recoveryObjectives:
    rto: "15m"  # 恢复时间目标
    rpo: "5m"   # 恢复点目标
    
  tiers:
    - name: "同城双活"
      location: "dc1-primary"
      replication: "synchronous"
      rto: "2m"
      rpo: "0s"
      priority: "highest"
      
    - name: "同城灾备"
      location: "dc1-secondary"
      replication: "semi-synchronous"
      rto: "15m"
      rpo: "5m"
      priority: "high"
      
    - name: "异地灾备"
      location: "dc2-remote"
      replication: "asynchronous"
      rto: "2h"
      rpo: "30m"
      priority: "medium"
```

---

<!-- chunk: 数据备份策略 -->
## 数据备份策略

### 分层备份策略

```yaml
# 分层备份配置
apiVersion: backup.storage.k8s.io/v1
kind: BackupPolicy
metadata:
  name: tiered-backup-policy
spec:
  tier1:  # 实时快照
    type: "snapshot"
    frequency: "5m"
    retention: "24h"
    
  tier2:  # 每日备份
    type: "full-backup"
    frequency: "24h"
    retention: "30d"
    consistency: "application-consistent"
    
  tier3:  # 每周归档
    type: "archive"
    frequency: "168h"
    retention: "365d"
    compression: "true"
    encryption: "true"
```

### 备份自动化脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# automated-backup-manager.sh

execute_backup() {
  TIER=$1
  case $TIER in
    "tier1")
      # 快照备份
      kubectl get pvc -n production -o json | \
        jq -r '.items[].metadata.name' | while read pvc; do
          TIMESTAMP=$(date +%Y%m%d-%H%M%S)
          kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: ${pvc}-snap-${TIMESTAMP}
  namespace: production
spec:
  volumeSnapshotClassName: fast-snapshot-class
  source:
    persistentVolumeClaimName: $pvc
EOF
        done
      ;;
  esac
}

# 根据时间调度执行不同层级备份
case "$(date +%M)" in
  "00"|"05"|"10"|"15"|"20"|"25"|"30"|"35"|"40"|"45"|"50"|"55")
    execute_backup "tier1"
    ;;
esac
```
---

<!-- chunk: 灾难恢复流程 -->
## 灾难恢复流程

### 自动故障转移配置

```yaml
# 自动故障转移配置
apiVersion: disaster-recovery.storage.k8s.io/v1
kind: AutoFailoverConfig
metadata:
  name: auto-failover-config
spec:
  healthChecks:
    storageConnectivity:
      timeout: "10s"
      interval: "30s"
      failureThreshold: 3
      
  failoverDecision:
    criteria:
      - condition: "primary-storage-unavailable"
        duration: "2m"
        action: "failover-to-secondary"
```

### 手动恢复流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# manual-recovery-workflow.sh

RECOVERY_SITE="secondary-dc"

manual_recovery() {
  echo "开始手动灾难恢复流程"
  
  # 1. 环境检查
  check_environment
  
  # 2. 激活存储系统
  activate_storage_system
  
  # 3. 数据恢复
  restore_data
  
  # 4. 启动应用服务
  start_applications
  
  echo "灾难恢复流程完成"
}

check_environment() {
  kubectl get nodes -l site=$RECOVERY_SITE --no-headers | grep -q "Ready"
  if [ $? -ne 0 ]; then
    echo "灾备站点节点状态异常"
    exit 1
  fi
}

activate_storage_system() {
  kubectl patch storageclass disaster-recovery -p '{"metadata":{"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'
}

restore_data() {
  LATEST_BACKUP=$(kubectl get volumesnapshot -n backup-system --sort-by=.metadata.creationTimestamp | tail -1 | awk '{print $1}')
  
  kubectl get pvc -n production -o json | \
    jq -r '.items[].metadata.name' | while read pvc; do
      kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ${pvc}-restored
  namespace: production
spec:
  dataSource:
    name: $LATEST_BACKUP
    kind: VolumeSnapshot
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
EOF
    done
}

start_applications() {
  APPLICATIONS=("database" "web-server")
  
  for app in "${APPLICATIONS[@]}"; do
    kubectl scale deployment $app --replicas=3 -n production
    kubectl wait --for=condition=available deployment/$app -n production --timeout=300s
  done
}

manual_recovery
```
---

<!-- chunk: 存储迁移方案 -->
## 存储迁移方案

### 跨集群迁移配置

```yaml
# 存储迁移配置
apiVersion: migration.storage.k8s.io/v1
kind: StorageMigrationPlan
metadata:
  name: cluster-migration-plan
spec:
  source:
    clusterEndpoint: "https://source-cluster.example.com"
    namespace: "production"
    
  destination:
    clusterEndpoint: "https://dest-cluster.example.com"
    namespace: "production"
    
  migrationStrategy:
    type: "live-migration"
    batchSize: 5
    downtimeWindow: "2h"
```

### 迁移执行脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# storage-migration-executor.sh

migrate_pvc() {
  PVC_NAME=$1
  
  # 1. 在目标集群创建PVC
  kubectl config use-context dest-cluster
  kubectl apply -f - <<EOF
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: $PVC_NAME-migrated
  namespace: production
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: migrated-storage
  resources:
    requests:
      storage: 100Gi
EOF
  
  # 2. 数据同步
  SOURCE_POD=$(kubectl get pods -n production -l app=data-source -o jsonpath='{.items[0].metadata.name}')
  DEST_POD=$(kubectl get pods -n production -l app=data-destination -o jsonpath='{.items[0].metadata.name}')
  
  kubectl exec -it $SOURCE_POD -n production -- \
    rsync -avz --delete /data/ dest-cluster:/data/
  
  echo "PVC $PVC_NAME 迁移完成"
}

# 批量迁移
kubectl config use-context source-cluster
kubectl get pvc -n production -o jsonpath='{.items[*].metadata.name}' | \
  tr ' ' '\n' | while read pvc; do
    migrate_pvc $pvc
  done
```
---

<!-- chunk: 跨集群数据同步 -->
## 跨集群数据同步

### 数据同步配置

```yaml
# 跨集群数据同步
apiVersion: datasync.storage.k8s.io/v1
kind: CrossClusterSync
metadata:
  name: cross-cluster-sync
spec:
  source:
    cluster: "cluster-1"
    namespace: "production"
    
  target:
    cluster: "cluster-2"
    namespace: "production"
    
  syncMode: "continuous"
  schedule: "*/10 * * * *"  # 每10分钟同步
  compression: "true"
  encryption: "true"
```

---

<!-- chunk: 业务连续性保障 -->
## 业务连续性保障

### 高可用架构

```yaml
# 高可用存储配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: ha-database
spec:
  replicas: 3
  serviceName: database-ha
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - database
            topologyKey: kubernetes.io/hostname
      containers:
      - name: database
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: ha-storage-class
      resources:
        requests:
          storage: 500Gi
```

---

<!-- chunk: 灾备演练与测试 -->
## 灾备演练与测试

### 演练计划配置

```yaml
# 灾备演练计划
apiVersion: disaster-recovery.storage.k8s.io/v1
kind: DRDrillPlan
metadata:
  name: quarterly-dr-drill
spec:
  schedule: "0 2 1 */3 *"  # 每季度第一天凌晨2点
  scope:
    namespaces: ["production", "database"]
    resources: ["pv", "pvc", "statefulsets"]
    
  drillSteps:
    - name: "simulate-outage"
      action: "network-disruption"
      duration: "10m"
      
    - name: "failover-activation"
      action: "trigger-failover"
      timeout: "15m"
      
    - name: "service-validation"
      action: "application-health-check"
      timeout: "30m"
      
    - name: "rollback-procedure"
      action: "failback-to-primary"
      timeout: "1h"
```

### 演练执行脚本

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
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
#!/bin/bash
# dr-drill-executor.sh

DRILL_PLAN="quarterly-dr-drill"

execute_dr_drill() {
  echo "开始灾备演练: $DRILL_PLAN"
  
  # 1. 模拟问题
  simulate_outage
  
  # 2. 验证自动故障转移
  verify_failover
  
  # 3. 业务功能验证
  validate_services
  
  # 4. 生成演练报告
  generate_drill_report
  
  echo "灾备演练完成"
}

simulate_outage() {
  echo "模拟存储问题..."
  kubectl cordon node-with-storage
  kubectl delete pod -n production -l app=storage-controller
}

verify_failover() {
  echo "验证故障转移..."
  sleep 300  # 等待故障转移完成
  
  FAILOVER_STATUS=$(kubectl get pods -n dr-system -l app=dr-controller -o jsonpath='{.items[0].status.phase}')
  if [ "$FAILOVER_STATUS" = "Running" ]; then
    echo "✅ 故障转移成功"
  else
    echo "❌ 故障转移失败"
  fi
}

validate_services() {
  echo "验证业务服务..."
  kubectl get svc -n production | while read svc; do
    # 验证服务可用性
    echo "检查服务: $svc"
  done
}

generate_drill_report() {
  cat > /tmp/dr-drill-report-$(date +%Y%m%d).md <<EOF
# 灾备演练报告

<!-- chunk: 演练基本信息 -->
## 演练基本信息
- 时间: $(date)
- 计划: $DRILL_PLAN
- 结果: 成功

<!-- chunk: 关键指标 -->
## 关键指标
- 故障检测时间: 30秒
- 故障转移时间: 8分钟
- 服务恢复时间: 12分钟
- 数据完整性: 100%

<!-- chunk: 改进建议 -->
## 改进建议
1. 优化故障检测算法
2. 缩短DNS切换时间
3. 增强监控告警及时性
EOF
}

execute_dr_drill
```
---

<!-- chunk: RTO/RPO管理 -->
## RTO/RPO管理

### SLA监控仪表板

```yaml
# RTO/RPO监控配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: dr-sla-monitoring
spec:
  groups:
  - name: dr.sla.rules
    rules:
    # RTO监控
    - alert: RTOExceeded
      expr: |
        disaster_recovery_failover_duration_seconds > 900  # 15分钟
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "灾难恢复时间超过SLA"
        
    # RPO监控
    - alert: RPOExceeded
      expr: |
        disaster_recovery_data_lag_seconds > 300  # 5分钟
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "数据恢复点超过SLA"
        
    # 备份完整性监控
    - alert: BackupIncomplete
      expr: |
        backup_success_rate < 0.95
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "备份成功率低于95%"
```

### SLA报告生成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# sla-report-generator.sh

generate_sla_report() {
  echo "📊 灾备SLA报告生成"
  
  # 收集RTO数据
  RTO_DATA=$(kubectl get events -n dr-system --field-selector reason=FailoverComplete -o json | \
    jq -r '.items[] | .firstTimestamp + "," + .message' | \
    tail -30)
    
  # 收集RPO数据
  RPO_DATA=$(kubectl get volumesnapshot -n production --sort-by=.metadata.creationTimestamp | \
    tail -100 | \
    awk '{print $1","$2","$3}')
    
  # 生成报告
  cat > /tmp/sla-report-$(date +%Y%m).md <<EOF
# 灾备SLA月度报告

<!-- chunk: 基本信息 -->
## 基本信息
- 报告月份: $(date +%Y-%m)
- 报告生成时间: $(date)

<!-- chunk: RTO指标 -->
## RTO指标
- 平均恢复时间: 8.5分钟
- 最大恢复时间: 12分钟
- SLA达成率: 99.2%

<!-- chunk: RPO指标 -->
## RPO指标
- 平均数据延迟: 2.3分钟
- 最大数据延迟: 4.8分钟
- SLA达成率: 99.8%

<!-- chunk: 备份指标 -->
## 备份指标
- 备份成功率: 99.5%
- 备份完整性: 100%
- 平均备份时间: 15分钟

<!-- chunk: 趋势分析 -->
## 趋势分析
- RTO呈下降趋势
- RPO保持稳定
- 备份效率持续提升

<!-- chunk: 改进建议 -->
## 改进建议
1. 优化故障转移流程
2. 增强监控告警系统
3. 定期进行灾备演练
EOF
  
  echo "SLA报告已生成: /tmp/sla-report-$(date +%Y%m).md"
}

generate_sla_report
```
---
**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-04-storage-data KUDIG Database — Global MOC
- [[domain-04-storage-data/README.md|[[Storage Domain 存储领域知识库|Storage Domain 存储领域知识库]]]]
- index.md|Domain-6 存储 — 开源项目索引]]
- 存储架构概览与核心组件
- PV/PVC 核心概念与企业级实践
- 03 - PVC使用模式与最佳实践
- StorageClass 动态供给与多租户管理
- 05 - CSI驱动集成与运维管理
- 06 - 存储基础概念详解
- 07 - 存储日常运维操作手册
- 08 - 存储性能调优与优化策略
- 09 - PV/PVC故障排查与解决方案

## See Also

- 13-storage-security-compliance
- 14-cloud-native-storage
- 16-csi-migration-in-tree-to-csi
- completion-summary

## Related

- [[domain-19-landscape-references/topic-index/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]


<!-- risk-assessed -->
