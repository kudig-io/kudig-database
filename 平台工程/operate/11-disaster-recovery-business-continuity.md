---
title: 灾难恢复与业务连续性 (Disaster Recovery & Business Continuity)
description: '## 概述'
summary: '灾难恢复与业务连续性是平台运维的生命线，通过建立完善的备份恢复策略、多活架构和应急响应机制，确保在各种问题场景下业务的持续可用性。'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- istio
- postgresql
- daemonset
- gateway
- operator
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 灾难恢复与业务连续性 (Disaster Recovery & Business Continuity) 是什么
- 如何 灾难恢复与业务连续性 (Disaster Recovery & Business Continuity)
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- 灾难恢复与业务连续性
- Disaster
- Recovery
- Business
- Continuity
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- service-mesh-basics
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
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 灾难恢复与业务连续性 (Disaster Recovery & Business Continuity)

<!-- chunk: 概述 -->
## 概述

灾难恢复与业务连续性是平台运维的生命线，通过建立完善的备份恢复策略、多活架构和应急响应机制，确保在各种问题场景下业务的持续可用性。

<!-- chunk: 灾难恢复策略 -->
## 灾难恢复策略

### RTO/RPO目标定义
```
RTO (Recovery Time Objective): 15分钟
RPO (Recovery Point Objective): 5分钟
MTTR (Mean Time To Recovery): 30分钟
```

### 灾难类型分类
- **自然灾害**: 地震、洪水、火灾等
- **人为灾害**: 误操作、恶意攻击、代码缺陷
- **技术问题**: 硬件问题、网络中断、电力问题
- **供应商问题**: 云服务商问题、第三方服务中断

<!-- chunk: 备份策略体系 -->
## 备份策略体系

### 数据备份层次
```
应用数据备份 → 系统配置备份 → 基础设施备份 → 灾备环境备份
```

### Velero备份配置
```yaml
# Velero安装配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: velero
  namespace: velero
spec:
  replicas: 2
  selector:
    matchLabels:
      name: velero
  template:
    metadata:
      labels:
        name: velero
    spec:
      restartPolicy: Always
      serviceAccountName: velero
      containers:
      - name: velero
        image: velero/velero:v1.11.0
        command:
        - /velero
        args:
        - server
        - --backup-sync-period=1m
        - --restic-timeout=1h
        env:
        - name: AWS_SHARED_CREDENTIALS_FILE
          value: /credentials/cloud
        - name: VELERO_SCRATCH_DIR
          value: /scratch
        volumeMounts:
        - name: cloud-credentials
          mountPath: /credentials
        - name: plugins
          mountPath: /plugins
        - name: scratch
          mountPath: /scratch

---
# 备份存储位置配置
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: default
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: velero-backup-bucket
    prefix: backups
  config:
    region: us-west-2
    s3ForcePathStyle: "true"
    s3Url: https://s3.us-west-2.amazonaws.com
```

### 备份策略配置
```yaml
# 应用数据备份策略
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: app-backup-hourly
  namespace: velero
spec:
  schedule: "0 * * * *"  # 每小时执行
  template:
    ttl: "168h"  # 保留7天
    includedNamespaces:
    - production
    includedResources:
    - persistentvolumeclaims
    - persistentvolumes
    snapshotVolumes: true
    storageLocation: default

---
# 系统配置备份策略
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: config-backup-daily
  namespace: velero
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  template:
    ttl: "720h"  # 保留30天
    includedNamespaces:
    - kube-system
    - monitoring
    - logging
    includedResources:
    - deployments
    - services
    - configmaps
    - secrets
    snapshotVolumes: false
```

### 跨区域备份
```yaml
# 跨区域备份位置
apiVersion: velero.io/v1
kind: BackupStorageLocation
metadata:
  name: cross-region-backup
  namespace: velero
spec:
  provider: aws
  objectStorage:
    bucket: velero-dr-bucket
    prefix: dr-backups
  config:
    region: us-east-1  # 灾备区域
    s3ForcePathStyle: "true"
    s3Url: https://s3.us-east-1.amazonaws.com

---
# 灾备备份策略
apiVersion: velero.io/v1
kind: Schedule
metadata:
  name: dr-backup-weekly
  namespace: velero
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨3点
  template:
    ttl: "8760h"  # 保留1年
    includedNamespaces:
    - production
    - staging
    storageLocation: cross-region-backup
    snapshotVolumes: true
```

<!-- chunk: 恢复演练流程 -->
## 恢复演练流程

### 恢复测试脚本

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# disaster-recovery-test.sh

set -e

NAMESPACE="dr-test"
BACKUP_NAME="test-backup-$(date +%Y%m%d-%H%M%S)"

echo "🚀 Starting Disaster Recovery Test"

# 1. 创建测试环境
echo "1. Creating test environment..."
kubectl create namespace $NAMESPACE

# 2. 部署测试应用
echo "2. Deploying test application..."
kubectl apply -f test-app.yaml -n $NAMESPACE

# 3. 等待应用就绪
echo "3. Waiting for application to be ready..."
kubectl wait --for=condition=ready pod -l app=test-app -n $NAMESPACE --timeout=300s

# 4. 执行备份
echo "4. Creating backup..."
velero backup create $BACKUP_NAME \
  --include-namespaces $NAMESPACE \
  --snapshot-volumes \
  --wait

# 5. 验证备份成功
echo "5. Verifying backup..."
if velero backup describe $BACKUP_NAME | grep -q "Completed"; then
    echo "✅ Backup completed successfully"
else
    echo "❌ Backup failed"
    exit 1
fi

# 6. 删除测试环境
echo "6. Deleting test environment..."
kubectl delete namespace $NAMESPACE --wait=false  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 7. 等待删除完成
sleep 30

# 8. 执行恢复
echo "7. Restoring from backup..."
velero restore create --from-backup $BACKUP_NAME \
  --namespace-mappings $NAMESPACE:$NAMESPACE-restored \
  --wait

# 9. 验证恢复
echo "8. Verifying restoration..."
kubectl wait --for=condition=ready pod -l app=test-app -n $NAMESPACE-restored --timeout=300s

# 10. 功能验证
echo "9. Performing functionality test..."
if curl -f http://test-app.$NAMESPACE-restored.svc.cluster.local/health; then
    echo "✅ Application restored and functioning properly"
else
    echo "❌ Application restoration verification failed"
    exit 1
fi

# 11. 清理测试资源
echo "10. Cleaning up test resources..."
kubectl delete namespace $NAMESPACE-restored  # ⚠️ 不可逆：永久删除命名空间及全部资源
velero backup delete $BACKUP_NAME --confirm

echo "🎉 Disaster Recovery Test Completed Successfully!"
```
### 恢复时间验证
```python
# 恢复时间监控脚本
import time
import subprocess
import json

class RecoveryTimeMonitor:
    def __init__(self):
        self.start_time = None
        self.end_time = None
        self.metrics = {}
    
    def start_monitoring(self):
        self.start_time = time.time()
        print(f"⏱️  Recovery monitoring started at {time.ctime(self.start_time)}")
    
    def check_recovery_completion(self, namespace, deployment):
        """检查恢复是否完成"""
        cmd = f"kubectl get deployment {deployment} -n {namespace} -o json"
        try:
            result = subprocess.run(cmd.split(), capture_output=True, text=True)
            deployment_info = json.loads(result.stdout)
            
            replicas = deployment_info['status'].get('replicas', 0)
            ready_replicas = deployment_info['status'].get('readyReplicas', 0)
            
            return replicas > 0 and ready_replicas == replicas
        except Exception as e:
            print(f"Error checking deployment status: {e}")
            return False
    
    def stop_monitoring(self):
        self.end_time = time.time()
        recovery_time = self.end_time - self.start_time
        self.metrics['recovery_time'] = recovery_time
        print(f"⏱️  Recovery completed in {recovery_time:.2f} seconds")
        return recovery_time
    
    def generate_report(self):
        return {
            'recovery_time_seconds': self.metrics.get('recovery_time'),
            'rto_compliance': self.metrics.get('recovery_time', 0) <= 900,  # 15分钟RTO
            'test_timestamp': time.ctime(self.start_time)
        }
```

<!-- chunk: 多活架构设计 -->
## 多活架构设计

### 主备集群架构
```
Primary Cluster (us-west) ←→ Standby Cluster (us-east)
        ↑                          ↑
    Load Balancer ← Health Check → Failover Mechanism
```

### 集群同步配置
```yaml
# 主集群配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sync-controller
  namespace: dr-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: sync-controller
  template:
    metadata:
      labels:
        app: sync-controller
    spec:
      containers:
      - name: sync-controller
        image: dr/sync-controller:v1.0
        env:
        - name: PRIMARY_CLUSTER
          value: "https://k8s-primary.example.com"
        - name: STANDBY_CLUSTER
          value: "https://k8s-standby.example.com"
        - name: SYNC_INTERVAL
          value: "30s"
        volumeMounts:
        - name: kubeconfig
          mountPath: /etc/kubernetes
          readOnly: true
      volumes:
      - name: kubeconfig
        secret:
          secretName: cluster-kubeconfigs

---
# 数据同步策略
apiVersion: dr.system/v1
kind: DataSyncPolicy
metadata:
  name: production-sync
spec:
  source:
    cluster: primary
    namespaces:
    - production
    - staging
  target:
    cluster: standby
    namespaces:
    - production-dr
    - staging-dr
  syncMode: continuous
  conflictResolution: last-write-wins
  resources:
    include:
    - deployments
    - services
    - configmaps
    - secrets
    exclude:
    - events
    - pods
```

### 自动故障切换
```yaml
# 健康检查和故障切换
apiVersion: apps/v1
kind: Deployment
metadata:
  name: failover-controller
  namespace: dr-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: failover-controller
  template:
    metadata:
      labels:
        app: failover-controller
    spec:
      containers:
      - name: failover-controller
        image: dr/failover-controller:v1.0
        env:
        - name: HEALTH_CHECK_INTERVAL
          value: "10s"
        - name: FAILURE_THRESHOLD
          value: "3"
        - name: FAILOVER_TIMEOUT
          value: "300"  # 5分钟超时
        - name: NOTIFICATION_WEBHOOK
          value: "https://alerts.example.com/webhook"
```

<!-- chunk: 业务连续性保障 -->
## 业务连续性保障

### 应用多活部署
```yaml
# 多区域部署配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: multi-region-app
spec:
  replicas: 6
  selector:
    matchLabels:
      app: multi-region-app
  template:
    metadata:
      labels:
        app: multi-region-app
        version: v1.0
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - multi-region-app
              topologyKey: topology.kubernetes.io/zone
      containers:
      - name: app
        image: myapp:v1.0
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10

---
# 多区域Service配置
apiVersion: v1
kind: Service
metadata:
  name: multi-region-service
spec:
  selector:
    app: multi-region-app
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer
  loadBalancerSourceRanges:
  - 0.0.0.0/0

---
# 流量分割配置
apiVersion: networking.istio.io/v1alpha3
kind: VirtualService
metadata:
  name: multi-region-routing
spec:
  hosts:
  - multi-region.example.com
  gateways:
  - multi-region-gateway
  http:
  - route:
    - destination:
        host: multi-region-app.primary.svc.cluster.local
      weight: 80
    - destination:
        host: multi-region-app.standby.svc.cluster.local
      weight: 20
```

### 数据库高可用
```yaml
# PostgreSQL主备配置
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: postgres-ha
spec:
  instances: 3
  primaryUpdateStrategy: unsupervised
  storage:
    size: 50Gi
  bootstrap:
    initdb:
      database: app
      owner: app
  backup:
    barmanObjectStore:
      destinationPath: s3://postgres-backup/
      endpointURL: https://s3.us-west-2.amazonaws.com
      s3Credentials:
        accessKeyId:
          name: postgres-s3-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: postgres-s3-creds
          key: SECRET_ACCESS_KEY
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - topologyKey: kubernetes.io/hostname
        labelSelector:
          matchLabels:
            postgresql: postgres-ha
```

<!-- chunk: 应急响应流程 -->
## 应急响应流程

### 问题等级定义
```yaml
# 问题等级分类
incident_levels:
  P0:  # 紧急问题
    description: "核心业务中断，影响面>50%"
    response_time: "15分钟"
    escalation: "CTO通知"
    
  P1:  # 高优先级问题
    description: "重要业务受影响，影响面10-50%"
    response_time: "1小时"
    escalation: "技术总监通知"
    
  P2:  # 中等问题
    description: "一般业务受影响，影响面<10%"
    response_time: "4小时"
    escalation: "团队负责人通知"
    
  P3:  # 低优先级问题
    description: "轻微问题，无业务影响"
    response_time: "24小时"
    escalation: "常规处理"
```

### 应急响应手册
```markdown
# 应急响应手册

<!-- chunk: 联系人列表 -->
## 联系人列表
- **值班经理**: ops-oncall@example.com
- **基础设施团队**: infra-team@example.com
- **应用团队**: app-team@example.com
- **安全团队**: security-team@example.com

<!-- chunk: 标准操作程序(SOP) -->
## 标准操作程序(SOP)

### 1. 问题发现与确认
- 监控告警接收
- 初步故障定位
- 影响范围评估
- 问题等级确定

### 2. 应急响应启动
- 通知相关人员
- 启动应急会议
- 分配处理任务
- 建立沟通渠道

### 3. 故障处理执行
- 按照预案执行
- 实时进度更新
- 决策记录保存
- 相关方同步

### 4. 恢复验证
- 功能测试验证
- 性能指标检查
- 用户体验确认
- 业务回归测试

### 5. 事后总结
- 问题根本原因分析
- 处理过程复盘
- 改进措施制定
- 知识库更新
```

### 自动化应急响应
```python
# 自动化应急响应系统
class EmergencyResponseSystem:
    def __init__(self):
        self.handlers = {
            'node_failure': self.handle_node_failure,
            'network_outage': self.handle_network_outage,
            'data_corruption': self.handle_data_corruption,
            'security_breach': self.handle_security_breach
        }
    
    def trigger_response(self, incident_type, details):
        """触发应急响应"""
        handler = self.handlers.get(incident_type)
        if handler:
            return handler(details)
        else:
            return self.handle_unknown_incident(incident_type, details)
    
    def handle_node_failure(self, details):
        """处理节点问题"""
        affected_nodes = details.get('nodes', [])
        
        # 1. 隔离问题节点
        for node in affected_nodes:
            self.isolate_node(node)
        
        # 2. 迁移工作负载
        self.migrate_workloads(affected_nodes)
        
        # 3. 启动替换节点
        self.provision_replacement_nodes(len(affected_nodes))
        
        # 4. 验证服务恢复
        return self.verify_service_recovery()
    
    def handle_network_outage(self, details):
        """处理网络中断"""
        # 1. 检查网络连通性
        # 2. 切换备用网络路径
        # 3. 重新配置网络策略
        # 4. 验证网络恢复
        pass
    
    def isolate_node(self, node_name):
        """隔离问题节点"""
        cmd = f"kubectl cordon {node_name}"
        subprocess.run(cmd.split())
        cmd = f"kubectl drain {node_name} --ignore-daemonsets --delete-emptydir-data"
        subprocess.run(cmd.split())
    
    def migrate_workloads(self, nodes):
        """迁移工作负载"""
        for node in nodes:
            cmd = f"kubectl get pods --field-selector spec.nodeName={node} -o json"
            result = subprocess.run(cmd.split(), capture_output=True, text=True)
            pods = json.loads(result.stdout)
            
            for pod in pods.get('items', []):
                # 重新调度Pod
                pass
```

<!-- chunk: 持续改进机制 -->
## 持续改进机制

### 定期演练计划
```yaml
# 灾难恢复演练计划
disaster_recovery_exercises:
  quarterly_exercises:
    - name: "完整数据中心故障恢复"
      frequency: "每季度"
      participants: ["运维团队", "开发团队", "业务团队"]
      duration: "4小时"
      objectives:
        - 验证RTO/RPO指标
        - 测试跨区域恢复
        - 评估团队协作效率
      
    - name: "单应用故障恢复"
      frequency: "每月"
      participants: ["应用团队", "运维团队"]
      duration: "2小时"
      objectives:
        - 验证应用级恢复
        - 测试备份完整性
        - 优化恢复流程

  annual_exercises:
    - name: "大规模灾难恢复演练"
      frequency: "每年"
      participants: ["全员参与"]
      duration: "1天"
      objectives:
        - 全面验证DR能力
        - 测试业务连续性
        - 完善应急预案
```

### 改进措施跟踪
```yaml
# 改进措施跟踪系统
improvement_tracking:
  metrics_collection:
    - recovery_time_metrics
    - backup_success_rate
    - team_response_time
    - user_impact_assessment
  
  feedback_loop:
    - post_incident_reviews
    - exercise_debrief_sessions
    - stakeholder_feedback
    - industry_best_practices
  
  action_items:
    - automation_improvements
    - process_optimizations
    - tool_enhancements
    - training_program_updates
```

通过建立完善的灾难恢复和业务连续性体系，可以最大程度地减少问题对业务的影响，确保在各种极端情况下都能维持业务的正常运转。

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 KUDIG Database — Global MOC
- [[平台工程/README.md|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- index.md|Domain-9 平台运维 — 开源项目索引]]
- 平台运维概述
- 集群生命周期管理
- [[平台工程/governance/03-capacity-planning-resource-assessment.md|03 capacity planning resource assessment]]
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 09-cost-optimization-finops
- 10-security-compliance
- 12-backup-recovery-strategy
- 13-multi-cluster-management


<!-- risk-assessed -->
