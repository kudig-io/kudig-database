# Kubernetes 工作负载(Workload)ACK补充技术资料

> **适用版本**: Kubernetes v1.25 - v1.32 | **环境**: 阿里云专有云 & 公共云 | **重点产品**: ACK  
> **文档类型**: 补充技术资料 | **目标读者**: 运维工程师、架构师  

---

## 目录

1. [控制器选型决策指南](#1-控制器选型决策指南)
2. [安全加固实践](#2-安全加固实践)
3. [监控告警体系建设](#3-监控告警体系建设)
4. [成本优化策略](#4-成本优化策略)
5. [故障处理手册](#5-故障处理手册)
6. [最佳实践总结](#6-最佳实践总结)

---

## 1. 控制器选型决策指南

### 1.1 控制器选择决策树

```
应用特性分析
    ↓
有状态需求?
├── 是 → StatefulSet
└── 否 → 
    需要节点级部署?
    ├── 是 → DaemonSet
    └── 否 →
        批处理任务?
        ├── 是 → Job/CronJob
        └── 否 → Deployment (无状态应用)
```

### 1.2 不同场景的控制器选型

#### 场景一：Web应用服务

```yaml
# 推荐使用Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-application
  namespace: production
spec:
  replicas: 6
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
      - name: web
        image: web-app:latest
        ports:
        - containerPort: 8080
```

**选型理由**:
- 无状态应用，适合Deployment
- 需要滚动更新和自动恢复
- 支持水平扩缩容

#### 场景二：数据库集群

```yaml
# 推荐使用StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mongodb-cluster
  namespace: database
spec:
  serviceName: mongodb-headless
  replicas: 3
  podManagementPolicy: Parallel
  updateStrategy:
    type: RollingUpdate
  selector:
    matchLabels:
      app: mongodb
  template:
    metadata:
      labels:
        app: mongodb
    spec:
      containers:
      - name: mongodb
        image: mongo:5.0
        volumeMounts:
        - name: data
          mountPath: /data/db
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 200Gi
```

**选型理由**:
- 有状态应用，需要持久化存储
- 需要稳定的网络标识
- 有序部署和更新要求

#### 场景三：日志收集代理

```yaml
# 推荐使用DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd-logging
  namespace: monitoring
spec:
  selector:
    matchLabels:
      name: fluentd-logging
  template:
    metadata:
      labels:
        name: fluentd-logging
    spec:
      tolerations:
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      containers:
      - name: fluentd
        image: fluent/fluentd:v1.14
        volumeMounts:
        - name: varlog
          mountPath: /var/log
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
```

**选型理由**:
- 每个节点都需要运行
- 节点级系统服务
- 自动跟随节点变化

#### 场景四：定时数据处理

```yaml
# 推荐使用CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: daily-data-processing
  namespace: batch
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: processor
            image: data-processor:latest
            command: ["process-data", "--date=yesterday"]
          restartPolicy: OnFailure
```

**选型理由**:
- 定时执行需求
- 一次性任务处理
- 需要调度控制

### 1.3 控制器性能对比分析

| 控制器 | 创建速度 | 更新速度 | 资源开销 | 适用场景 |
|--------|----------|----------|----------|----------|
| **Deployment** | 快 | 快 | 低 | 无状态应用 |
| **StatefulSet** | 中 | 中 | 中 | 有状态应用 |
| **DaemonSet** | 快 | 快 | 低 | 节点级服务 |
| **Job** | 快 | - | 低 | 批处理任务 |
| **CronJob** | 快 | - | 低 | 定时任务 |

---

## 2. 安全加固实践

### 2.1 网络安全策略

```yaml
# 完整的网络安全隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: secure-workload-isolation
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: secure-app
  policyTypes:
  - Ingress
  - Egress
  
  # 入站流量控制
  ingress:
  # 只允许来自特定应用的流量
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  # 允许监控和健康检查
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    ports:
    - protocol: TCP
      port: 9090
  
  # 出站流量控制
  egress:
  # 只允许访问必要的后端服务
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - protocol: TCP
      port: 5432
  # 允许DNS查询
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
```

### 2.2 容器安全配置

```yaml
# 完整的安全上下文配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: security-hardened-app
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      # Pod安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        runAsGroup: 3000
        fsGroup: 2000
        supplementalGroups: [1001]
        seccompProfile:
          type: RuntimeDefault
        sysctls:
        - name: net.ipv4.tcp_syncookies
          value: "1"
      
      containers:
      - name: app
        image: secure-app:v1.0
        # 容器安全上下文
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
        # 只读文件系统挂载
        volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: app-config
          mountPath: /app/config
          readOnly: true
      
      volumes:
      - name: tmp
        emptyDir: {}
      - name: app-config
        configMap:
          name: app-config
```

### 2.3 镜像安全扫描

```yaml
# 镜像安全扫描集成
apiVersion: apps/v1
kind: Deployment
metadata:
  name: scanned-app
  namespace: production
  annotations:
    admission.trivy-operator.aquasecurity.github.io/image-ref: "registry.example.com/app:v1.0"
spec:
  replicas: 3
  template:
    metadata:
      annotations:
        trivy-operator.policy.security/aqua: "pass"
        trivy-operator.policy.security/critical: "0"
        trivy-operator.policy.security/high: "0"
    spec:
      imagePullSecrets:
      - name: registry-secret
      containers:
      - name: app
        image: registry.example.com/app:v1.0@sha256:abcdef123456...
        imagePullPolicy: Always
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
```

---

## 3. 监控告警体系建设

### 3.1 核心监控指标定义

```yaml
# Prometheus监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: workload-alerts
  namespace: monitoring
spec:
  groups:
  - name: workload.rules
    rules:
    # 副本数告警
    - alert: DeploymentReplicasMismatch
      expr: |
        kube_deployment_status_replicas_available != kube_deployment_status_replicas
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Deployment副本数不匹配 ({{ $labels.deployment }})"
        description: "期望副本: {{ $value }}, 可用副本: {{ $value }}"

    # Pod重启告警
    - alert: PodFrequentRestarts
      expr: |
        increase(kube_pod_container_status_restarts_total[1h]) > 5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Pod频繁重启 ({{ $labels.pod }})"
        description: "过去1小时重启次数: {{ $value }}"

    # 资源使用率告警
    - alert: ContainerCPUUsageHigh
      expr: |
        rate(container_cpu_usage_seconds_total[5m]) / 
        kube_pod_container_resource_limits{resource="cpu"} * 100 > 80
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "容器CPU使用率过高"
        description: "{{ $labels.pod }}/{{ $labels.container }} CPU使用率 {{ $value }}%"

    # 更新状态告警
    - alert: DeploymentRolloutStuck
      expr: |
        kube_deployment_status_observed_generation < kube_deployment_metadata_generation
      for: 15m
      labels:
        severity: critical
      annotations:
        summary: "Deployment更新卡住"
        description: "{{ $labels.deployment }} 更新超过15分钟未完成"
```

### 3.2 Grafana仪表板配置

```json
{
  "dashboard": {
    "title": "Kubernetes Workloads Monitoring",
    "panels": [
      {
        "title": "Deployment状态概览",
        "type": "stat",
        "targets": [
          {
            "expr": "count(kube_deployment_created)",
            "legendFormat": "总Deployments"
          },
          {
            "expr": "count(kube_deployment_status_replicas_unavailable > 0)",
            "legendFormat": "异常Deployments"
          }
        ]
      },
      {
        "title": "Pod状态分布",
        "type": "piechart",
        "targets": [
          {
            "expr": "count by (phase)(kube_pod_status_phase)",
            "legendFormat": "{{phase}}"
          }
        ]
      },
      {
        "title": "资源使用趋势",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(container_cpu_usage_seconds_total[5m])) by (namespace)",
            "legendFormat": "{{namespace}} CPU"
          },
          {
            "expr": "sum(container_memory_working_set_bytes) by (namespace) / 1024 / 1024 / 1024",
            "legendFormat": "{{namespace}} Memory (GB)"
          }
        ]
      },
      {
        "title": "Pod重启分析",
        "type": "table",
        "targets": [
          {
            "expr": "topk(10, increase(kube_pod_container_status_restarts_total[1h]))",
            "legendFormat": "{{namespace}}/{{pod}}"
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

### 4.1 资源使用分析工具

```python
#!/usr/bin/env python3
# 工作负载资源使用分析器

import yaml
from kubernetes import client, config
import numpy as np

class ResourceAnalyzer:
    def __init__(self):
        config.load_kube_config()
        self.apps_v1 = client.AppsV1Api()
        self.core_v1 = client.CoreV1Api()
    
    def analyze_deployments(self, namespace="default"):
        """分析Deployment资源使用情况"""
        deployments = self.apps_v1.list_namespaced_deployment(namespace)
        
        analysis_results = []
        
        for deployment in deployments.items:
            # 获取Pod模板资源请求
            template = deployment.spec.template
            containers = template.spec.containers
            
            total_requests = {'cpu': 0, 'memory': 0}
            total_limits = {'cpu': 0, 'memory': 0}
            
            for container in containers:
                if container.resources:
                    requests = container.resources.requests or {}
                    limits = container.resources.limits or {}
                    
                    # 累加资源请求
                    total_requests['cpu'] += self.parse_cpu(requests.get('cpu', '0'))
                    total_requests['memory'] += self.parse_memory(requests.get('memory', '0'))
                    
                    # 累加资源限制
                    total_limits['cpu'] += self.parse_cpu(limits.get('cpu', '0'))
                    total_limits['memory'] += self.parse_memory(limits.get('memory', '0'))
            
            # 计算资源利用率
            utilization = self.calculate_utilization(
                deployment.metadata.name, 
                namespace,
                total_requests
            )
            
            analysis_results.append({
                'deployment': deployment.metadata.name,
                'replicas': deployment.spec.replicas,
                'requests': total_requests,
                'limits': total_limits,
                'utilization': utilization,
                'recommendation': self.generate_recommendation(utilization, total_requests)
            })
        
        return analysis_results
    
    def parse_cpu(self, cpu_str):
        """解析CPU资源字符串"""
        if not cpu_str:
            return 0
        if cpu_str.endswith('m'):
            return int(cpu_str[:-1]) / 1000
        return float(cpu_str)
    
    def parse_memory(self, mem_str):
        """解析内存资源字符串"""
        if not mem_str:
            return 0
        if mem_str.endswith('Gi'):
            return int(mem_str[:-2]) * 1024
        elif mem_str.endswith('Mi'):
            return int(mem_str[:-2])
        elif mem_str.endswith('Ki'):
            return int(mem_str[:-2]) / 1024
        return int(mem_str) / 1024 / 1024  # bytes to Mi
    
    def calculate_utilization(self, deployment_name, namespace, requests):
        """计算实际资源利用率"""
        # 这里应该从metrics server获取实际使用数据
        # 简化示例返回模拟数据
        return {
            'cpu_avg': np.random.uniform(0.3, 0.8),
            'cpu_max': np.random.uniform(0.6, 1.0),
            'memory_avg': np.random.uniform(0.4, 0.7),
            'memory_max': np.random.uniform(0.7, 0.9)
        }
    
    def generate_recommendation(self, utilization, requests):
        """生成优化建议"""
        recommendations = []
        
        if utilization['cpu_avg'] < 0.3:
            recommendations.append("CPU请求过高，建议降低20-30%")
        elif utilization['cpu_max'] > 0.9:
            recommendations.append("CPU峰值使用率过高，建议增加20%")
        
        if utilization['memory_avg'] < 0.4:
            recommendations.append("内存请求过高，建议降低25-35%")
        elif utilization['memory_max'] > 0.95:
            recommendations.append("内存峰值使用率过高，建议增加30%")
        
        return recommendations

# 使用示例
analyzer = ResourceAnalyzer()
results = analyzer.analyze_deployments("production")

for result in results:
    print(f"\nDeployment: {result['deployment']}")
    print(f"副本数: {result['replicas']}")
    print(f"CPU请求: {result['requests']['cpu']:.2f}核")
    print(f"内存请求: {result['requests']['memory']:.0f}Mi")
    print(f"CPU平均利用率: {result['utilization']['cpu_avg']:.1%}")
    print(f"内存平均利用率: {result['utilization']['memory_avg']:.1%}")
    print("优化建议:")
    for rec in result['recommendation']:
        print(f"  - {rec}")
```

### 4.2 自动化成本优化策略

```yaml
# 成本优化Operator配置
apiVersion: cost.optimization.io/v1
kind: WorkloadOptimizer
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
  - name: "right-size-resources"
    enabled: true
    rules:
    - metric: "cpu_average_utilization"
      threshold: 30
      action: "reduce-request"
      adjustment: "20%"
      gracePeriod: "7d"
    
    - metric: "memory_average_utilization"
      threshold: 40
      action: "reduce-request"
      adjustment: "25%"
      gracePeriod: "7d"
    
    - metric: "cpu_peak_utilization"
      threshold: 90
      action: "increase-request"
      adjustment: "20%"
      gracePeriod: "1d"
  
  - name: "scale-down-idle"
    enabled: true
    rules:
    - metric: "request_per_second"
      threshold: 1
      duration: "30m"
      action: "scale-to-minimum"
      minimumReplicas: 1
  
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
    monthlyBudget: 100000  # 10万元/月
    alertThreshold: 80     # 80%预算时告警
```

### 4.3 Spot实例优化策略

```yaml
# Spot实例混合部署策略
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spot-optimized-app
  namespace: batch
spec:
  replicas: 20
  selector:
    matchLabels:
      app: spot-app
  template:
    metadata:
      labels:
        app: spot-app
        spot-instance: "true"
    spec:
      # Spot实例节点选择
      nodeSelector:
        node-pool: spot-instances
        alibabacloud.com/spot-instance: "true"
      
      # Spot实例容忍
      tolerations:
      - key: alibabacloud.com/spot-instance
        value: "true"
        effect: NoSchedule
      
      # 优雅终止配置
      terminationGracePeriodSeconds: 120
      
      containers:
      - name: batch-worker
        image: batch-worker:latest
        # 容错设计
        env:
        - name: ENABLE_CHECKPOINTING
          value: "true"
        - name: CHECKPOINT_INTERVAL
          value: "300"  # 5分钟检查点
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
```

---

## 5. 故障处理手册

### 5.1 系统化故障诊断流程

```
工作负载故障诊断决策树:

应用异常?
├── Pod状态异常?
│   ├── Pending → 检查资源配额和节点资源
│   │   ├── 资源不足 → 增加节点或调整请求
│   │   └── 调度约束 → 检查亲和性和污点
│   ├── CrashLoopBackOff → 检查应用日志和健康检查
│   │   ├── 启动失败 → 检查启动探针和依赖服务
│   │   └── 运行时错误 → 检查应用日志和配置
│   ├── ImagePullBackOff → 检查镜像仓库访问
│   │   ├── 认证失败 → 检查imagePullSecrets
│   │   └── 网络问题 → 检查网络连通性
│   └── Unknown → 检查节点状态
├── 副本数不符?
│   ├── 少于期望值 → 检查控制器状态和节点资源
│   │   ├── 控制器故障 → 重启控制器
│   │   └── 节点资源不足 → 增加节点
│   └── 多于期望值 → 检查是否有手动创建的Pod
├── 更新失败?
│   ├── Rollout卡住 → 检查新Pod状态和健康检查
│   │   ├── 健康检查失败 → 调整探针配置
│   │   └── 资源不足 → 检查资源请求和限制
│   └── 回滚失败 → 检查历史版本和权限
└── 性能问题?
    ├── 资源不足 → 检查资源请求和限制
    ├── 调度问题 → 检查节点亲和性和污点
    └── 网络问题 → 检查网络策略和服务
```

### 5.2 常见故障及解决方案

#### 故障1: Pod一直保持Pending状态

```bash
# 诊断步骤
echo "=== Pod Pending故障诊断 ==="

# 1. 检查Pod详细信息
kubectl describe pod <pod-name> -n <namespace>

# 2. 检查节点资源
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, allocatable: .status.allocatable}'

# 3. 检查资源配额
kubectl describe resourcequota -n <namespace>

# 4. 检查节点污点和容忍
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, taints: .spec.taints}'

# 常见解决方案:

# 方案1: 增加节点资源
# aliyun cs ScaleClusterNodePool --cluster-id <cluster-id> --nodepool-id <pool-id> --scaling-group-size <new-size>

# 方案2: 调整资源请求
kubectl patch deployment <deployment-name> -p '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"requests":{"cpu":"100m","memory":"128Mi"}}}]}}}}'

# 方案3: 添加容忍污点
kubectl patch deployment <deployment-name> -p '{"spec":{"template":{"spec":{"tolerations":[{"key":"dedicated","value":"batch","effect":"NoSchedule"}]}}}}'
```

#### 故障2: Pod频繁重启(CrashLoopBackOff)

```bash
# 诊断步骤
echo "=== Pod频繁重启诊断 ==="

# 1. 查看Pod日志
kubectl logs <pod-name> -n <namespace> --previous

# 2. 检查健康检查配置
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 10 "livenessProbe\|readinessProbe"

# 3. 检查资源使用情况
kubectl top pod <pod-name> -n <namespace>

# 4. 检查事件日志
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> --sort-by=.lastTimestamp

# 常见解决方案:

# 方案1: 调整健康检查参数
kubectl patch deployment <deployment-name> -n <namespace> -p '{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "app",
          "livenessProbe": {
            "initialDelaySeconds": 120,
            "periodSeconds": 30,
            "failureThreshold": 5
          }
        }]
      }
    }
  }
}'

# 方案2: 增加资源限制
kubectl patch deployment <deployment-name> -n <namespace> -p '{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "app",
          "resources": {
            "requests": {"memory": "512Mi"},
            "limits": {"memory": "1Gi"}
          }
        }]
      }
    }
  }
}'
```

#### 故障3: Deployment更新卡住

```bash
# 诊断步骤
echo "=== Deployment更新卡住诊断 ==="

# 1. 检查更新状态
kubectl rollout status deployment/<deployment-name> -n <namespace>

# 2. 查看Deployment详细信息
kubectl describe deployment <deployment-name> -n <namespace>

# 3. 检查新创建的Pod状态
kubectl get pods -n <namespace> -l app=<app-label> --sort-by=.metadata.creationTimestamp

# 4. 检查ReplicaSet状态
kubectl get rs -n <namespace> -l app=<app-label>

# 常见解决方案:

# 方案1: 暂停并恢复更新
kubectl rollout pause deployment/<deployment-name> -n <namespace>
# 修复问题后
kubectl rollout resume deployment/<deployment-name> -n <namespace>

# 方案2: 回滚到上一版本
kubectl rollout undo deployment/<deployment-name> -n <namespace>

# 方案3: 强制重启
kubectl patch deployment <deployment-name> -n <namespace> -p "{\"spec\":{\"template\":{\"metadata\":{\"annotations\":{\"date\":\"$(date +%s)\"}}}}}"
```

### 5.3 紧急恢复操作指南

```bash
#!/bin/bash
# 工作负载紧急恢复脚本

EMERGENCY_TYPE=${1:?"Usage: $0 <emergency-type> [namespace]"}
NAMESPACE=${2:-default}

case $EMERGENCY_TYPE in
  "deployment-stuck")
    echo "=== Deployment更新卡住紧急恢复 ==="
    
    DEPLOYMENT_NAME=${3:?"需要指定Deployment名称"}
    
    # 1. 暂停更新
    kubectl rollout pause deployment/$DEPLOYMENT_NAME -n $NAMESPACE
    
    # 2. 检查问题Pod
    kubectl get pods -n $NAMESPACE -l app=$DEPLOYMENT_NAME --sort-by=.metadata.creationTimestamp | tail -5
    
    # 3. 强制删除卡住的Pod
    kubectl delete pods -n $NAMESPACE -l app=$DEPLOYMENT_NAME --field-selector=status.phase!=Running
    
    # 4. 恢复更新
    kubectl rollout resume deployment/$DEPLOYMENT_NAME -n $NAMESPACE
    
    # 5. 监控更新状态
    kubectl rollout status deployment/$DEPLOYMENT_NAME -n $NAMESPACE --timeout=300s
    ;;
    
  "pod-crashloop")
    echo "=== Pod频繁重启紧急处理 ==="
    
    POD_NAME=${3:?"需要指定Pod名称"}
    
    # 1. 立即查看错误日志
    kubectl logs $POD_NAME -n $NAMESPACE --previous | tail -20
    
    # 2. 临时禁用健康检查
    kubectl patch deployment -n $NAMESPACE -p '{
      "spec": {
        "template": {
          "spec": {
            "containers": [{
              "name": "app",
              "livenessProbe": null,
              "readinessProbe": null
            }]
          }
        }
      }
    }'
    
    # 3. 监控Pod状态
    kubectl get pod $POD_NAME -n $NAMESPACE -w
    ;;
    
  "resource-exhaustion")
    echo "=== 资源耗尽紧急扩容 ==="
    
    # 1. 检查节点资源使用
    kubectl top nodes
    
    # 2. 扩容节点池(ACK)
    # aliyun cs ScaleClusterNodePool --cluster-id <cluster-id> --nodepool-id <pool-id> --scaling-group-size <new-size>
    
    # 3. 或者减少不必要的工作负载
    kubectl scale deployment -n $NAMESPACE --replicas=1 -l emergency=scale-down
    ;;
esac
```

---

## 6. 最佳实践总结

### 6.1 生产环境配置清单

```yaml
# 生产环境工作负载配置基线
apiVersion: v1
kind: ConfigMap
metadata:
  name: workload-production-baseline
  namespace: production
data:
  baseline-config.yaml: |
    # 工作负载配置检查清单
    
    ## Deployment配置
    [ ] 使用明确的命名规范
    [ ] 配置适当的副本数
    [ ] 设置合理的更新策略(maxSurge/maxUnavailable)
    [ ] 配置资源请求和限制
    [ ] 添加完整的健康检查探针
    [ ] 配置Pod中断预算(PDB)
    [ ] 设置适当的标签和注解
    
    ## StatefulSet配置
    [ ] 使用Headless Service
    [ ] 配置持久化存储(volumeClaimTemplates)
    [ ] 设置适当的更新策略
    [ ] 配置Pod管理策略
    [ ] 添加存储保留策略
    
    ## DaemonSet配置
    [ ] 配置适当的节点选择器
    [ ] 添加必要的污点容忍
    [ ] 设置更新策略
    [ ] 配置资源限制
    
    ## Job/CronJob配置
    [ ] 设置适当的并发策略
    [ ] 配置重启策略
    [ ] 设置超时时间
    [ ] 配置历史记录保留
    
    ## 安全配置
    [ ] 配置网络策略
    [ ] 设置安全上下文
    [ ] 使用非root用户运行
    [ ] 配置RBAC权限
    
    ## 监控告警
    [ ] 部署监控指标
    [ ] 配置告警规则
    [ ] 设置通知渠道
    [ ] 建立仪表板
    
    ## 备份恢复
    [ ] 配置定期备份
    [ ] 建立恢复流程
    [ ] 定期演练恢复
```

### 6.2 定期维护任务

```yaml
# 工作负载维护CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: workload-maintenance
  namespace: maintenance
spec:
  schedule: "0 3 * * 0"  # 每周日凌晨3点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: maintenance
            image: workload-maintenance:latest
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
    echo "=== 工作负载系统维护开始 ==="
    date
    
    # 1. 清理已完成的Job
    echo "[1] 清理过期Job"
    kubectl get jobs -A -o json | jq -r '
      .items[] | 
      select(.status.succeeded > 0 and (.status.completionTime | fromdateiso8601 < (now - 86400*7))) |
      "\(.metadata.namespace) \(.metadata.name)"
    ' | while read ns job; do
      kubectl delete job $job -n $ns
    done
    
    # 2. 分析资源使用
    echo "[2] 分析资源使用模式"
    kubectl get deployments -A -o json | jq -r '
      .items[] | 
      "\(.metadata.namespace) \(.metadata.name) \(.spec.replicas) \(.status.availableReplicas)"
    '
    
    # 3. 检查异常Pod
    echo "[3] 检查异常Pod"
    kubectl get pods -A --field-selector=status.phase!=Running | head -10
    
    # 4. 生成维护报告
    echo "[4] 生成维护报告"
    echo "维护完成时间: $(date)" > /reports/maintenance-$(date +%Y%m%d).txt
    
    echo "=== 工作负载系统维护完成 ==="
```

### 6.3 性能基准测试模板

```bash
#!/bin/bash
# 工作负载性能基准测试框架

TEST_SUITE=${1:-comprehensive}
DEPLOYMENT_NAME=${2:-benchmark-app}
NAMESPACE=${3:-benchmark}

echo "=== 工作负载性能基准测试 ==="
echo "测试套件: $TEST_SUITE"
echo "测试应用: $DEPLOYMENT_NAME"
echo "命名空间: $NAMESPACE"
echo

# 创建测试环境
setup_test_environment() {
  cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: $DEPLOYMENT_NAME
  namespace: $NAMESPACE
spec:
  replicas: 10
  selector:
    matchLabels:
      app: benchmark
  template:
    metadata:
      labels:
        app: benchmark
    spec:
      containers:
      - name: stress-ng
        image: progrium/stress-ng:latest
        command: ["stress-ng"]
        args: ["--cpu", "1", "--timeout", "300s", "--metrics-brief"]
        resources:
          requests:
            cpu: "500m"
            memory: "256Mi"
          limits:
            cpu: "1"
            memory: "512Mi"
EOF
}

# 执行基准测试
run_benchmarks() {
  case $TEST_SUITE in
    "quick")
      # 快速测试
      echo "执行快速基准测试..."
      kubectl rollout status deployment/$DEPLOYMENT_NAME -n $NAMESPACE --timeout=300s
      kubectl top pods -n $NAMESPACE -l app=benchmark
      ;;
    "comprehensive")
      # 综合测试
      echo "等待部署完成..."
      kubectl rollout status deployment/$DEPLOYMENT_NAME -n $NAMESPACE --timeout=600s
      
      echo "执行压力测试..."
      # CPU压力测试
      kubectl exec -n $NAMESPACE deployment/$DEPLOYMENT_NAME -- stress-ng --cpu 4 --timeout 60s
      
      # 内存压力测试
      kubectl exec -n $NAMESPACE deployment/$DEPLOYMENT_NAME -- stress-ng --vm 2 --vm-bytes 100M --timeout 60s
      
      # 网络延迟测试
      kubectl exec -n $NAMESPACE deployment/$DEPLOYMENT_NAME -- ping -c 10 kubernetes.default
      
      echo "收集性能指标..."
      kubectl top pods -n $NAMESPACE -l app=benchmark
      ;;
  esac
}

# 清理测试环境
cleanup() {
  echo "清理测试环境..."
  kubectl delete namespace $NAMESPACE --ignore-not-found=true
}

# 主执行流程
trap cleanup EXIT

# 创建测试命名空间
kubectl create namespace $NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

setup_test_environment
run_benchmarks
```

---