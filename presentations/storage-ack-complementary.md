# Kubernetes 存储(PV/PVC/StorageClass)ACK补充技术资料

> **适用版本**: Kubernetes v1.25 - v1.32 | **环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK  
> **文档类型**: 补充技术资料 | **目标读者**: 运维工程师、架构师  

---

## 目录

1. [存储选型决策指南](#1-存储选型决策指南)
2. [安全加固实践](#2-安全加固实践)
3. [监控告警体系建设](#3-监控告警体系建设)
4. [成本优化策略](#4-成本优化策略)
5. [故障处理手册](#5-故障处理手册)
6. [最佳实践总结](#6-最佳实践总结)

---

## 1. 存储选型决策指南

### 1.1 存储类型选择决策树

```
业务需求分析
    ↓
数据访问模式?
├── 单Pod独占 → RWO存储类型
├── 多Pod共享 → RWX存储类型
└── 只读共享 → ROX存储类型
    ↓
性能要求?
├── 极高IOPS → ESSD PL2/PL3
├── 高吞吐量 → ESSD PL1
├── 标准性能 → SSD云盘
└── 成本敏感 → 高效云盘
    ↓
数据特征?
├── 结构化数据 → 云盘(EBS)
├── 非结构化文件 → NAS
├── 大对象文件 → OSS
└── 临时数据 → 临时卷
    ↓
部署环境?
├── ACK公共云 → 原生云存储
├── 专有云 → 兼容性存储
└── 混合云 → 统一存储层
```

### 1.2 不同场景的存储选型

#### 场景一：关系型数据库(MySQL/PostgreSQL)

```yaml
# 推荐配置：ESSD PL1/PL2
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: database-optimized
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1  # 5万IOPS，适合大多数数据库
  fsType: xfs  # 适合大文件和数据库
mountOptions:
  - noatime
  - nodiratime
  - logbufs=8
```

**选型理由**:
- ESSD提供稳定的低延迟和高IOPS
- XFS文件系统对数据库友好
- PL1性价比最高，PL2适合高并发场景

#### 场景二：NoSQL数据库(MongoDB/Redis)

```yaml
# Redis缓存 - 高性能要求
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: redis-high-performance
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL2  # 10万IOPS
  fsType: ext4
mountOptions:
  - noatime
  - nodiratime
  - discard  # 启用TRIM，适合频繁写入
```

**选型理由**:
- Redis需要极低延迟和高并发IOPS
- PL2提供足够的性能余量
- ext4在小文件场景下表现更好

#### 场景三：日志收集和分析

```yaml
# 日志存储 - 大容量低成本
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: logging-storage
provisioner: nasplugin.csi.alibabacloud.com
parameters:
  volumeAs: subpath
  server: "xxx.cn-hangzhou.nas.aliyuncs.com:/logs"
  path: "/"
mountOptions:
  - nolock
  - rsize=1048576
  - wsize=1048576
```

**选型理由**:
- 日志主要是追加写入，对IOPS要求不高
- NAS支持多Pod共享写入
- 成本远低于云盘

#### 场景四：机器学习训练数据

```yaml
# AI训练 - 大文件高吞吐
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ai-training-storage
provisioner: ossplugin.csi.alibabacloud.com
parameters:
  bucket: "ml-datasets"
  url: "oss-cn-hangzhou.aliyuncs.com"
  # OSS适合存储大规模训练数据集
```

**选型理由**:
- 训练数据集通常很大(数百GB-TB级)
- OSS按量付费，成本可控
- 支持并发读取多个训练Pod

### 1.3 存储容量规划计算器

```python
#!/usr/bin/env python3
# 存储容量规划工具

import math

class StoragePlanner:
    def __init__(self, data_growth_rate=0.2, safety_margin=0.3):
        """
        data_growth_rate: 月增长率 (20%)
        safety_margin: 安全余量 (30%)
        """
        self.data_growth_rate = data_growth_rate
        self.safety_margin = safety_margin
    
    def calculate_required_capacity(self, current_size_gb, months_ahead=12):
        """计算未来所需容量"""
        future_size = current_size_gb * ((1 + self.data_growth_rate) ** months_ahead)
        required_capacity = future_size * (1 + self.safety_margin)
        return {
            'current_size': current_size_gb,
            'future_size': round(future_size, 2),
            'required_capacity': round(required_capacity, 2),
            'recommended_class': self._recommend_storage_class(required_capacity)
        }
    
    def _recommend_storage_class(self, size_gb):
        """根据容量推荐存储类型"""
        if size_gb <= 100:
            return "高效云盘"
        elif size_gb <= 1000:
            return "SSD云盘"
        else:
            return "ESSD云盘"
    
    def cost_analysis(self, size_gb, storage_type="essd"):
        """成本分析"""
        pricing = {
            "efficiency": 0.35,  # 元/GB/月
            "ssd": 0.45,
            "essd_pl0": 1.05,
            "essd_pl1": 1.50,
            "essd_pl2": 3.00
        }
        
        monthly_cost = size_gb * pricing.get(storage_type, 1.50)
        annual_cost = monthly_cost * 12
        
        return {
            'monthly_cost': round(monthly_cost, 2),
            'annual_cost': round(annual_cost, 2)
        }

# 使用示例
planner = StoragePlanner()
result = planner.calculate_required_capacity(500, 6)  # 当前500GB，预测6个月后
print(f"存储规划结果:")
print(f"当前容量: {result['current_size']} GB")
print(f"预测容量: {result['future_size']} GB")
print(f"建议容量: {result['required_capacity']} GB")
print(f"推荐类型: {result['recommended_class']}")
```

---

## 2. 安全加固实践

### 2.1 存储访问安全策略

```yaml
# NetworkPolicy限制存储访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: storage-security-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: critical-app
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 只允许必要的内部访问
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9100  # 监控端口
  egress:
  # 限制存储后端访问
  - to:
    - ipBlock:
        cidr: 100.100.0.0/16  # 阿里云内网段
    ports:
    - protocol: TCP
      port: 443  # 存储API端口
```

### 2.2 存储加密配置

```yaml
# 完整的加密存储方案
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: encrypted-enterprise-storage
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  # 基础加密
  type: cloud_essd
  performanceLevel: PL1
  encrypted: "true"
  kmsKeyId: "key-enterprise-master-key"
  
  # 高级安全参数
  enableTls: "true"
  encryptionAlgorithm: "AES256"
  keyRotationPeriod: "90d"  # 90天密钥轮换
  
  # 合规性标记
  complianceTag: "pci-dss"
  dataClassification: "confidential"

mountOptions:
  - noexec     # 禁止执行
  - nosuid     # 禁止setuid
  - nodev      # 禁止设备文件
  - noatime
  - nodiratime
```

### 2.3 数据脱敏和访问控制

```yaml
# 数据脱敏配置
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: storage-data-protection
  namespace: production
spec:
  selector:
    matchLabels:
      app: data-processor
  rules:
  - from:
    - source:
        principals: ["cluster.local/ns/production/sa/data-processor"]
    to:
    - operation:
        methods: ["GET", "LIST"]
        paths: ["/api/v1/data/*"]
    when:
    - key: request.headers[x-data-sensitivity]
      values: ["public"]
  - from:
    - source:
        principals: ["cluster.local/ns/security/sa/data-analyst"]
    to:
    - operation:
        methods: ["GET", "POST", "PUT", "DELETE"]
        paths: ["/api/v1/data/*"]
    when:
    - key: request.headers[x-data-sensitivity]
      values: ["confidential"]
```

### 2.4 安全审计配置

```yaml
# 完整的审计策略
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 存储操作详细审计
- level: RequestResponse
  resources:
  - group: ""
    resources: ["persistentvolumes", "persistentvolumeclaims"]
  - group: "storage.k8s.io"
    resources: ["storageclasses", "volumeattachments"]
  verbs: ["create", "update", "delete", "patch"]
  userGroups: ["system:authenticated"]
  omitStages:
  - "RequestReceived"

# 快照操作审计
- level: Metadata
  resources:
  - group: "snapshot.storage.k8s.io"
    resources: ["volumesnapshots", "volumesnapshotcontents"]
  verbs: ["create", "delete"]
  userGroups: ["system:authenticated"]

# 异常行为检测
- level: RequestResponse
  resources:
  - group: ""
    resources: ["persistentvolumeclaims"]
  verbs: ["create"]
  userGroups: ["system:serviceaccounts"]
  omitStages:
  - "RequestReceived"
```

---

## 3. 监控告警体系建设

### 3.1 核心监控指标定义

```yaml
# Prometheus监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-alerts
  namespace: monitoring
spec:
  groups:
  - name: storage.rules
    rules:
    # 容量告警
    - alert: PVCStorageUsageHigh
      expr: |
        kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes * 100 > 85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC存储使用率过高 ({{ $labels.persistentvolumeclaim }})"
        description: "{{ $labels.persistentvolumeclaim }} 在 {{ $labels.namespace }} 命名空间中使用率已达 {{ $value }}%"

    - alert: PVCStorageUsageCritical
      expr: |
        kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes * 100 > 95
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "PVC存储使用率达到危险水平"
        description: "紧急: {{ $labels.persistentvolumeclaim }} 使用率 {{ $value }}%，请立即处理"

    # Inode告警
    - alert: PVCInodeUsageHigh
      expr: |
        kubelet_volume_stats_inodes_used / kubelet_volume_stats_inodes * 100 > 90
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC inode使用率过高"
        description: "{{ $labels.persistentvolumeclaim }} inode使用率 {{ $value }}%"

    # 性能告警
    - alert: StorageIOPSAnomaly
      expr: |
        rate(node_disk_reads_completed_total[5m]) > 10000 or
        rate(node_disk_writes_completed_total[5m]) > 5000
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "存储IOPS异常"
        description: "检测到异常的I/O操作模式"
```

### 3.2 Grafana仪表板配置

```json
{
  "dashboard": {
    "title": "Kubernetes Storage Monitoring",
    "panels": [
      {
        "title": "存储使用率概览",
        "type": "gauge",
        "targets": [
          {
            "expr": "avg(kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes * 100)",
            "legendFormat": "平均使用率"
          }
        ]
      },
      {
        "title": "PVC容量趋势",
        "type": "graph",
        "targets": [
          {
            "expr": "kubelet_volume_stats_used_bytes / 1024 / 1024 / 1024",
            "legendFormat": "{{persistentvolumeclaim}} (GB)"
          }
        ]
      },
      {
        "title": "存储IOPS监控",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(node_disk_reads_completed_total[5m])",
            "legendFormat": "读IOPS - {{device}}"
          },
          {
            "expr": "rate(node_disk_writes_completed_total[5m])",
            "legendFormat": "写IOPS - {{device}}"
          }
        ]
      },
      {
        "title": "存储延迟分析",
        "type": "heatmap",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(node_disk_read_time_seconds_total[5m]))",
            "legendFormat": "读延迟(p95)"
          }
        ]
      }
    ]
  }
}
```

### 3.3 告警通知配置

```yaml
# Alertmanager配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-config
  namespace: monitoring
data:
  alertmanager.yml: |
    global:
      smtp_smarthost: 'smtp.company.com:587'
      smtp_from: 'alerts@company.com'
      smtp_auth_username: 'alert-user'
      smtp_auth_password: 'password'
    
    route:
      group_by: ['alertname', 'namespace']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 3h
      receiver: 'default-receiver'
      
      routes:
      - match:
          severity: 'critical'
        receiver: 'ops-team'
        group_wait: 10s
        repeat_interval: 30m
      
      - match:
          severity: 'warning'
        receiver: 'dev-team'
        group_wait: 1m
        repeat_interval: 2h
    
    receivers:
    - name: 'default-receiver'
      email_configs:
      - to: 'team@company.com'
        send_resolved: true
    
    - name: 'ops-team'
      webhook_configs:
      - url: 'http://ops-notification-service:8080/webhook'
        send_resolved: true
      pagerduty_configs:
      - service_key: 'pagerduty-service-key'
    
    - name: 'dev-team'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/XXX/YYY/ZZZ'
        channel: '#dev-alerts'
        send_resolved: true
```

---

## 4. 成本优化策略

### 4.1 存储成本分析工具

```python
#!/usr/bin/env python3
# 存储成本优化分析器

class StorageCostOptimizer:
    def __init__(self):
        self.pricing = {
            'efficiency': {'price': 0.35, 'iops': 2500, 'throughput': 180},
            'ssd': {'price': 0.45, 'iops': 20000, 'throughput': 250},
            'essd_pl0': {'price': 1.05, 'iops': 10000, 'throughput': 180},
            'essd_pl1': {'price': 1.50, 'iops': 50000, 'throughput': 350},
            'essd_pl2': {'price': 3.00, 'iops': 100000, 'throughput': 750}
        }
    
    def analyze_workload(self, workload_profile):
        """
        workload_profile = {
            'avg_iops': 5000,
            'peak_iops': 25000,
            'throughput_mb': 100,
            'capacity_gb': 500,
            'access_pattern': 'random_write'  # or 'sequential_read'
        }
        """
        recommendations = []
        
        # 1. 性能匹配分析
        suitable_storages = []
        for storage_type, specs in self.pricing.items():
            if (specs['iops'] >= workload_profile['peak_iops'] * 1.2 and 
                specs['throughput'] >= workload_profile['throughput_mb'] * 1.2):
                suitable_storages.append({
                    'type': storage_type,
                    'cost': specs['price'] * workload_profile['capacity_gb'],
                    'performance_margin': specs['iops'] / workload_profile['peak_iops']
                })
        
        # 2. 成本效益排序
        suitable_storages.sort(key=lambda x: x['cost'])
        
        # 3. 生成建议
        if suitable_storages:
            best_option = suitable_storages[0]
            recommendations.append({
                'type': 'storage_selection',
                'priority': 'high',
                'message': f"推荐使用 {best_option['type']}，月成本约 {best_option['cost']:.2f}元",
                'savings': self._calculate_savings(workload_profile, best_option['type'])
            })
        
        # 4. 容量优化建议
        if workload_profile['capacity_gb'] > 1000:
            recommendations.append({
                'type': 'capacity_optimization',
                'priority': 'medium',
                'message': "建议实施分层存储策略，将冷数据迁移至成本更低的存储"
            })
        
        return recommendations
    
    def _calculate_savings(self, workload, selected_type):
        """计算相比默认选项的节省金额"""
        default_cost = self.pricing['essd_pl1']['price'] * workload['capacity_gb']
        selected_cost = self.pricing[selected_type]['price'] * workload['capacity_gb']
        return {
            'monthly_savings': round(default_cost - selected_cost, 2),
            'annual_savings': round((default_cost - selected_cost) * 12, 2),
            'savings_percentage': round((default_cost - selected_cost) / default_cost * 100, 1)
        }

# 使用示例
optimizer = StorageCostOptimizer()
workload = {
    'avg_iops': 8000,
    'peak_iops': 30000,
    'throughput_mb': 150,
    'capacity_gb': 800,
    'access_pattern': 'random_write'
}

recommendations = optimizer.analyze_workload(workload)
for rec in recommendations:
    print(f"[{rec['priority'].upper()}] {rec['message']}")
    if 'savings' in rec:
        savings = rec['savings']
        print(f"  预计月节省: {savings['monthly_savings']}元 ({savings['savings_percentage']}%)")
```

### 4.2 自动化成本优化策略

```yaml
# 成本优化Operator配置
apiVersion: cost.optimization.io/v1
kind: StorageOptimizer
metadata:
  name: production-cost-optimizer
  namespace: cost-optimization
spec:
  # 监控的目标命名空间
  targetNamespaces:
  - production
  - staging
  
  # 优化策略
  strategies:
  - name: "right-size-storage"
    enabled: true
    rules:
    - metric: "average_usage_percent"
      threshold: 30
      action: "downsize"
      gracePeriod: "7d"
    
    - metric: "peak_usage_percent"
      threshold: 90
      action: "upscale"
      gracePeriod: "1d"
  
  - name: "tier-down-inactive"
    enabled: true
    rules:
    - metric: "last_access_time"
      threshold: "30d"
      action: "move-to-archive"
      targetStorageClass: "archive-storage"
  
  # 通知配置
  notifications:
    email:
      recipients:
      - cost-team@company.com
      - team-lead@company.com
    webhook:
      url: "http://cost-dashboard:8080/api/v1/notifications"
  
  # 预算限制
  budgetLimits:
    monthlyBudget: 50000  # 5万元/月
    alertThreshold: 80    # 80%预算时告警
```

### 4.3 分层存储策略

```yaml
# 分层存储配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: hot-storage
  annotations:
    tier: "hot"
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1

---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: warm-storage
  annotations:
    tier: "warm"
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL0

---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: cold-storage
  annotations:
    tier: "cold"
provisioner: nasplugin.csi.alibabacloud.com
parameters:
  volumeAs: subpath
  server: "archive.nas.aliyuncs.com:/cold"

---
# 数据生命周期管理策略
apiVersion: batch/v1
kind: CronJob
metadata:
  name: data-tiering-manager
  namespace: data-management
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: tiering-manager
            image: data-tiering-controller:latest
            command:
            - /tiering-controller
            - --config=/config/tiering-policy.yaml
            volumeMounts:
            - name: config
              mountPath: /config
          volumes:
          - name: config
            configMap:
              name: tiering-policy
          restartPolicy: OnFailure
```

---

## 5. 故障处理手册

### 5.1 系统化故障诊断流程

```
存储故障诊断决策树:

存储异常?
├── 无法创建PVC?
│   ├── StorageClass不存在? → 创建或修正SC
│   ├── CSI驱动异常? → 检查CSI Pod状态
│   ├── 配额不足? → 检查ResourceQuota
│   └── 参数错误? → 检查PVC配置
├── PVC Pending?
│   ├── 等待调度? → 检查节点资源
│   ├── 跨AZ问题? → 使用WaitForFirstConsumer
│   └── 后端故障? → 联系云服务商
├── 挂载失败?
│   ├── 权限问题? → 检查SecurityContext
│   ├── 路径冲突? → 检查mountPath
│   └── 网络问题? → 检查网络连通性
└── 性能问题?
    ├── IOPS瓶颈? → 升级存储类型
    ├── 延迟高? → 检查AZ分布
    └── 吞吐量低? → 优化文件系统参数
```

### 5.2 常见故障及解决方案

#### 故障1: PVC一直保持Pending状态

```bash
# 诊断步骤
echo "=== PVC Pending故障诊断 ==="

# 1. 检查PVC基本信息
kubectl describe pvc <pvc-name> -n <namespace>

# 2. 检查StorageClass
kubectl get sc
kubectl describe sc <storageclass-name>

# 3. 检查CSI驱动状态
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system <csi-provisioner-pod> --tail=100

# 4. 检查事件日志
kubectl get events -n <namespace> --field-selector involvedObject.name=<pvc-name>

# 常见解决方案:

# 方案1: 修正StorageClass名称
kubectl patch pvc <pvc-name> -p '{"spec":{"storageClassName":"correct-sc-name"}}'

# 方案2: 检查并增加配额
kubectl describe resourcequota -n <namespace>

# 方案3: 重启CSI控制器
kubectl delete pod -n kube-system -l app=csi-provisioner
```

#### 故障2: Pod无法挂载存储卷

```bash
# 诊断步骤
echo "=== 存储挂载失败诊断 ==="

# 1. 检查Pod状态和事件
kubectl describe pod <pod-name> -n <namespace>

# 2. 检查节点上的挂载点
kubectl exec <pod-name> -n <namespace> -- df -h

# 3. 检查安全上下文
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 10 securityContext

# 4. 检查文件系统权限
kubectl exec <pod-name> -n <namespace> -- ls -la /mount/path

# 常见解决方案:

# 方案1: 修正fsGroup配置
apiVersion: v1
kind: Pod
spec:
  securityContext:
    fsGroup: 1000  # 设置正确的组ID

# 方案2: 检查挂载路径冲突
# 确保不同的volumeMount使用不同的mountPath

# 方案3: 重新创建Pod
kubectl delete pod <pod-name> -n <namespace>
# 让Deployment/StatefulSet自动重建
```

#### 故障3: 存储性能不足

```bash
# 性能诊断脚本
#!/bin/bash

PVC_NAME=${1:?"Usage: $0 <pvc-name>"}
NAMESPACE=${2:-default}

echo "=== 存储性能诊断 ==="
echo "PVC: $PVC_NAME"
echo "Namespace: $NAMESPACE"
echo

# 1. 检查当前性能指标
echo "[1] 当前IOPS统计"
kubectl exec -n $NAMESPACE <pod-using-pvc> -- iostat -x 1 5

# 2. 检查存储类型
echo "[2] 存储类型检查"
kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.storageClassName}' | \
xargs kubectl get sc -o yaml

# 3. 基准测试
echo "[3] 执行性能基准测试"
kubectl exec -n $NAMESPACE <pod-using-pvc> -- dd if=/dev/zero of=/test/testfile bs=1M count=1000 oflag=direct

# 解决方案:

# 方案1: 升级存储性能等级
# 从PL0升级到PL1或PL2
kubectl patch sc <storageclass-name> -p '{"parameters":{"performanceLevel":"PL2"}}'

# 方案2: 优化文件系统参数
# 添加挂载选项优化
mountOptions:
  - noatime
  - nobarrier
  - largeio

# 方案3: 调整应用架构
# 考虑读写分离或缓存层
```

### 5.3 紧急恢复操作指南

```bash
#!/bin/bash
# 存储紧急恢复脚本

EMERGENCY_TYPE=${1:?"Usage: $0 <emergency-type> [namespace]"}
NAMESPACE=${2:-default}

case $EMERGENCY_TYPE in
  "pvc-lost")
    echo "=== PVC丢失紧急恢复 ==="
    
    # 1. 查找关联的PV
    PV_NAME=$(kubectl get pv -o json | jq -r '.items[] | select(.spec.claimRef.name=="'$PVC_NAME'") | .metadata.name')
    
    # 2. 检查PV状态
    kubectl describe pv $PV_NAME
    
    # 3. 如果PV存在但PVC丢失，重新创建PVC
    if [ ! -z "$PV_NAME" ]; then
      cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: recovered-pvc
  namespace: $NAMESPACE
spec:
  accessModes:
  - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
  volumeName: $PV_NAME
EOF
    fi
    ;;
    
  "storage-full")
    echo "=== 存储空间满紧急处理 ==="
    
    # 1. 立即扩容
    kubectl patch pvc critical-storage -n $NAMESPACE -p '{"spec":{"resources":{"requests":{"storage":"200Gi"}}}}'
    
    # 2. 清理临时文件
    kubectl exec -n $NAMESPACE <pod-name> -- find /data -name "*.tmp" -mtime +7 -delete
    
    # 3. 重启Pod强制重新挂载
    kubectl delete pod -n $NAMESPACE -l app=critical-app
    ;;
    
  "csi-failure")
    echo "=== CSI驱动故障恢复 ==="
    
    # 1. 检查CSI Pod状态
    kubectl get pods -n kube-system | grep csi
    
    # 2. 重启故障Pod
    kubectl delete pods -n kube-system -l app=csi-plugin
    
    # 3. 验证恢复
    sleep 30
    kubectl get csidrivers
    ;;
esac
```

---

## 6. 最佳实践总结

### 6.1 生产环境配置清单

```yaml
# 生产环境存储配置基线
apiVersion: v1
kind: ConfigMap
metadata:
  name: storage-production-baseline
  namespace: production
data:
  baseline-config.yaml: |
    # 存储配置检查清单
    
    ## StorageClass配置
    [ ] 使用WaitForFirstConsumer绑定模式
    [ ] 启用allowVolumeExpansion
    [ ] 配置适当的reclaimPolicy (生产环境建议Retain)
    [ ] 设置合理的mountOptions
    [ ] 启用存储加密
    
    ## PVC配置
    [ ] 使用明确的命名规范
    [ ] 添加适当的标签用于管理
    [ ] 设置资源请求和限制
    [ ] 配置备份标签
    
    ## 应用配置
    [ ] StatefulSet使用volumeClaimTemplates
    [ ] 配置Pod反亲和避免单点故障
    [ ] 设置合理的资源请求
    [ ] 配置健康检查探针
    
    ## 监控告警
    [ ] 部署存储监控
    [ ] 配置容量告警(80%/90%)
    [ ] 配置性能告警
    [ ] 设置通知渠道
    
    ## 备份策略
    [ ] 配置定期快照
    [ ] 实施应用级备份
    [ ] 定期演练恢复流程
    [ ] 维护备份生命周期
```

### 6.2 定期维护任务

```yaml
# 存储维护CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: storage-maintenance
  namespace: maintenance
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨3点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: maintenance
            image: storage-maintenance:latest
            command:
            - /maintenance-script.sh
            env:
            - name: MAINTENANCE_TASKS
              value: "cleanup,analyze,optimize,report"
            volumeMounts:
            - name: scripts
              mountPath: /scripts
          volumes:
          - name: scripts
            configMap:
              name: maintenance-scripts
          restartPolicy: OnFailure
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: maintenance-scripts
  namespace: maintenance
data:
  maintenance-script.sh: |
    #!/bin/bash
    echo "=== 存储系统维护开始 ==="
    date
    
    # 1. 清理未使用的快照
    echo "[1] 清理过期快照"
    kubectl get volumesnapshot -A | grep 30d | awk '{print $2" -n "$1}' | xargs -I {} kubectl delete volumesnapshot {}
    
    # 2. 分析存储使用模式
    echo "[2] 分析存储使用"
    kubectl get pvc -A -o json | jq -r '.items[] | "\(.metadata.namespace):\(.metadata.name):\(.status.capacity.storage)"'
    
    # 3. 优化存储配置
    echo "[3] 检查存储优化机会"
    # 检查是否有过度配置的PVC
    
    # 4. 生成维护报告
    echo "[4] 生成维护报告"
    echo "维护完成时间: $(date)" > /reports/maintenance-$(date +%Y%m%d).txt
    
    echo "=== 存储系统维护完成 ==="
```

### 6.3 性能基准测试模板

```bash
#!/bin/bash
# 存储性能基准测试框架

TEST_SUITE=${1:-comprehensive}
STORAGE_CLASS=${2:-alicloud-disk-essd}
CAPACITY=${3:-100Gi}

echo "=== 存储性能基准测试 ==="
echo "测试套件: $TEST_SUITE"
echo "存储类型: $STORAGE_CLASS"
echo "测试容量: $CAPACITY"
echo

# 创建测试环境
setup_test_environment() {
  cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: benchmark-pvc
  namespace: benchmark
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: $STORAGE_CLASS
  resources:
    requests:
      storage: $CAPACITY
---
apiVersion: v1
kind: Pod
metadata:
  name: benchmark-pod
  namespace: benchmark
spec:
  containers:
  - name: fio
    image: ljishen/fio:latest
    command: ["sleep", "3600"]
    volumeMounts:
    - name: test-volume
      mountPath: /data
  volumes:
  - name: test-volume
    persistentVolumeClaim:
      claimName: benchmark-pvc
EOF
}

# 执行基准测试
run_benchmarks() {
  case $TEST_SUITE in
    "quick")
      # 快速测试
      kubectl exec -n benchmark benchmark-pod -- fio \
        --name=quick-test --directory=/data --size=1G \
        --rw=randrw --bs=4k --iodepth=16 --numjobs=4 --runtime=60
      ;;
    "comprehensive")
      # 综合测试
      echo "执行顺序读测试..."
      kubectl exec -n benchmark benchmark-pod -- fio \
        --name=seq-read --directory=/data --size=2G \
        --rw=read --bs=1M --iodepth=1 --runtime=120
      
      echo "执行顺序写测试..."
      kubectl exec -n benchmark benchmark-pod -- fio \
        --name=seq-write --directory=/data --size=2G \
        --rw=write --bs=1M --iodepth=1 --runtime=120
      
      echo "执行随机读写测试..."
      kubectl exec -n benchmark benchmark-pod -- fio \
        --name=rand-rw --directory=/data --size=2G \
        --rw=randrw --bs=4k --iodepth=16 --numjobs=4 --runtime=120
      ;;
  esac
}

# 清理测试环境
cleanup() {
  kubectl delete namespace benchmark --ignore-not-found=true
}

# 主执行流程
trap cleanup EXIT
setup_test_environment
kubectl wait --for=condition=Ready pod/benchmark-pod -n benchmark --timeout=300s
run_benchmarks
```

---