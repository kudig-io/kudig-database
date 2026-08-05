---
title: 18-跨区域容灾部署
description: 'title: 18-跨区域容灾部署'
summary: 'title: 18-跨区域容灾部署'
category: general
tags:
- k8s
- production
- best-practice
- istio
- opa
- redis
- mysql
- statefulset
- ingress
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- cross-region-disaster-recovery是什么？
- cross-region-disaster-recovery的使用方法
- cross-region-disaster-recovery的最佳实践
trigger_keywords:
- 跨区域容灾部署
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- service-mesh-basics
- redis-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 18-跨区域容灾部署
description: '# 18-跨区域容灾部署'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[istio|istio]]
- opa
- redis
- mysql
- [[statefulset|statefulset]]
- [[ingress|ingress]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 跨区域容灾部署 是什么
- 如何 跨区域容灾部署
- Kubernetes 18 production operations 最佳实践
trigger_keywords:
- 跨区域容灾部署
- production
- operations
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 18-跨区域容灾部署

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

跨区域容灾部署是保障业务连续性的高级策略。本文档详细介绍多活架构设计、数据同步机制和故障切换方案。

<!-- chunk: 🌍 多活架构设计 -->## 🌍 多活架构设计

## 地域分布策略

## 1. 三地域五中心架构
```yaml
# 多地域集群配置
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: primary-region-cluster
  labels:
    region: us-east-1
    role: primary
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: primary-control-plane
---
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: secondary-region-cluster
  labels:
    region: us-west-2
    role: secondary
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: secondary-control-plane
---
apiVersion: cluster.x-k8s.io/v1beta1
kind: Cluster
metadata:
  name: tertiary-region-cluster
  labels:
    region: eu-west-1
    role: tertiary
spec:
  controlPlaneRef:
    apiVersion: controlplane.cluster.x-k8s.io/v1beta1
    kind: KubeadmControlPlane
    name: tertiary-control-plane
```

## 2. 流量分发策略
```yaml
# 全球负载均衡配置
apiVersion: v1
kind: Service
metadata:
  name: global-load-balancer
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "external"
    service.beta.kubernetes.io/aws-load-balancer-nlb-target-type: "ip"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "HTTP"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-port: "8080"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/health"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
    protocol: TCP
  selector:
    app: global-app
---
# 地域路由配置
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: regional-routing
spec:
  hosts:
  - "*.example.com"
  gateways:
  - global-gateway
  http:
  - matchers:
    - - headers=""
    - region=""
    - exact="us-east-1"
    - route=""
    - - destination=""
    - host="app.us-east-1.svc.cluster.local"
    - port=""
    - number="8080"
    - weight="100"
  - matchers:
    - - headers=""
    - region=""
    - exact="us-west-2"
    - route=""
    - - destination=""
    - host="app.us-west-2.svc.cluster.local"
    - port=""
    - number="8080"
    - weight="100"
  - route:
    - destination:
        host: app.us-east-1.svc.cluster.local
        port:
          number: 8080
      weight: 60
    - destination:
        host: app.us-west-2.svc.cluster.local
        port:
          number: 8080
      weight: 30
    - destination:
        host: app.eu-west-1.svc.cluster.local
        port:
          number: 8080
      weight: 10
```

## 数据同步架构

## 1. 数据库多活配置
```yaml
# MySQL主主复制配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-primary
spec:
  serviceName: mysql-primary
  replicas: 3
  selector:
    matchLabels:
      app: mysql
      role: primary
  template:
    metadata:
      labels:
        app: mysql
        role: primary
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: root-password
        - name: MYSQL_REPLICATION_USER
          value: "replicator"
        - name: MYSQL_REPLICATION_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: replication-password
        ports:
        - containerPort: 3306
        volumeMounts:
        - name: mysql-data
          mountPath: /var/lib/mysql
        - name: mysql-config
          mountPath: /etc/mysql/conf.d
  volumeClaimTemplates:
  - metadata:
      name: mysql-data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-config
data:
  master.cnf: |
    [mysqld]
    log-bin=mysql-bin
    server-id=1
    binlog-format=ROW
    gtid-mode=ON
    enforce-gtid-consistency=ON
    log-slave-updates=ON
    binlog-ignore-db=mysql
    replicate-ignore-db=mysql
```

## 2. Redis集群多地域部署
```yaml
# Redis多地域集群配置
apiVersion: databases.spotahome.com/v1
kind: RedisFailover
metadata:
  name: redis-multi-region
spec:
  redis:
    replicas: 6
    resources:
      requests:
        cpu: 100m
        memory: 100Mi
      limits:
        cpu: 200m
        memory: 200Mi
    exporter:
      enabled: true
    affinity:
      podAntiAffinity:
        requiredDuringSchedulingIgnoredDuringExecution:
        - labelSelector:
            matchLabels:
              app.kubernetes.io/component: redis
          topologyKey: topology.kubernetes.io/zone
  sentinel:
    replicas: 3
    resources:
      requests:
        cpu: 100m
        memory: 100Mi
      limits:
        cpu: 200m
        memory: 200Mi
---
# Redis跨地域同步配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: redis-sync-config
data:
  redis-sync.conf: |
    # 主地域Redis配置
    masterauth yourpassword
    requirepass yourpassword
    
    # 从地域同步配置
    slaveof primary-redis-endpoint 6379
    slave-read-only yes
    
    # 哨兵配置
    sentinel monitor mymaster primary-redis-endpoint 6379 2
    sentinel auth-pass mymaster yourpassword
    sentinel down-after-milliseconds mymaster 5000
    sentinel failover-timeout mymaster 10000
    sentinel parallel-syncs mymaster 1
```

<!-- chunk: 🔄 故障检测与切换 -->## 🔄 故障检测与切换

## 智能故障检测

## 1. 多维度健康检查
```python
#!/usr/bin/env python3
# 智能故障检测系统

import asyncio
import aiohttp
import json
from datetime import datetime, timedelta
from kubernetes import client, config
import socket

class IntelligentFailureDetector:
    def __init__(self):
        config.load_kube_config()
        self.core_v1 = client.CoreV1Api()
        self.apps_v1 = client.AppsV1Api()
        
        self.health_check_config = {
            'regions': ['us-east-1', 'us-west-2', 'eu-west-1'],
            'check_intervals': {
                'critical': 30,    # 30秒检查一次关键服务
                'important': 60,   # 1分钟检查重要服务
                'routine': 300     # 5分钟常规检查
            },
            'failure_thresholds': {
                'network_latency': 1000,  # ms
                'http_error_rate': 0.05,  # 5%
                'database_response_time': 2000,  # ms
                'cpu_utilization': 0.85,  # 85%
                'memory_utilization': 0.90  # 90%
            }
        }
        
        self.region_status = {region: 'healthy' for region in self.health_check_config['regions']}
        self.failure_history = {}
    
    async def run_continuous_monitoring(self):
        """持续监控运行"""
        while True:
            try:
                # 并行执行各类检查
                await asyncio.gather(
                    self.check_network_connectivity(),
                    self.check_application_health(),
                    self.check_database_connectivity(),
                    self.check_infrastructure_health(),
                    self.analyze_performance_metrics()
                )
                
                # 评估整体健康状况
                await self.evaluate_overall_health()
                
                # 等待下次检查周期
                await asyncio.sleep(30)
                
            except Exception as e:
                print(f"Error in monitoring cycle: {e}")
                await asyncio.sleep(60)
    
    async def check_network_connectivity(self):
        """检查网络连通性"""
        network_checks = []
        
        for region in self.health_check_config['regions']:
            check_task = self.perform_region_network_check(region)
            network_checks.append(check_task)
        
        results = await asyncio.gather(*network_checks, return_exceptions=True)
        
        for i, result in enumerate(results):
            region = self.health_check_config['regions'][i]
            if isinstance(result, Exception):
                self.region_status[region] = 'network_failure'
                self.record_failure(region, 'network', str(result))
            elif result['latency'] > self.health_check_config['failure_thresholds']['network_latency']:
                self.region_status[region] = 'high_latency'
                self.record_failure(region, 'latency', f"Latency: {result['latency']}ms")
            else:
                # 如果之前有问题，现在恢复正常
                if self.region_status[region] in ['network_failure', 'high_latency']:
                    self.region_status[region] = 'healthy'
    
    async def perform_region_network_check(self, region):
        """执行地域网络检查"""
        endpoints = self.get_region_endpoints(region)
        latencies = []
        
        async with aiohttp.ClientSession() as session:
            for endpoint in endpoints:
                try:
                    start_time = datetime.now()
                    async with session.get(endpoint, timeout=5) as response:
                        end_time = datetime.now()
                        latency = (end_time - start_time).total_seconds() * 1000
                        latencies.append(latency)
                except Exception as e:
                    latencies.append(9999)  # 表示连接失败
        
        return {
            'region': region,
            'latency': sum(latencies) / len(latencies) if latencies else 9999,
            'success_rate': len([l for l in latencies if l < 5000]) / len(latencies) if latencies else 0
        }
    
    async def check_application_health(self):
        """检查应用健康状态"""
        try:
            # 检查各地域的应用部署
            for region in self.health_check_config['regions']:
                deployments = await self.get_region_deployments(region)
                
                for deployment in deployments:
                    health_status = await self.check_deployment_health(deployment, region)
                    if not health_status['healthy']:
                        self.record_failure(region, 'application', 
                                          f"Deployment {deployment} unhealthy: {health_status['reason']}")
                        
        except Exception as e:
            print(f"Error checking application health: {e}")
    
    async def check_database_connectivity(self):
        """检查数据库连通性"""
        try:
            db_endpoints = self.get_database_endpoints()
            
            for region, endpoint in db_endpoints.items():
                db_health = await self.test_database_connection(endpoint)
                if not db_health['reachable']:
                    self.record_failure(region, 'database', 
                                      f"Database unreachable: {db_health['error']}")
                elif db_health['response_time'] > self.health_check_config['failure_thresholds']['database_response_time']:
                    self.record_failure(region, 'database', 
                                      f"Database slow response: {db_health['response_time']}ms")
                    
        except Exception as e:
            print(f"Error checking database connectivity: {e}")
    
    async def check_infrastructure_health(self):
        """检查基础设施健康"""
        try:
            nodes = self.core_v1.list_node()
            
            for node in nodes.items:
                region = self.get_node_region(node)
                if region:
                    node_health = self.evaluate_node_health(node)
                    if not node_health['healthy']:
                        self.record_failure(region, 'infrastructure', 
                                          f"Node {node.metadata.name} unhealthy: {node_health['issues']}")
                        
        except Exception as e:
            print(f"Error checking infrastructure health: {e}")
    
    async def analyze_performance_metrics(self):
        """分析性能指标"""
        try:
            # 从监控系统获取性能数据
            metrics = await self.fetch_performance_metrics()
            
            for region in self.health_check_config['regions']:
                region_metrics = metrics.get(region, {})
                
                # 检查CPU使用率
                cpu_util = region_metrics.get('cpu_utilization', 0)
                if cpu_util > self.health_check_config['failure_thresholds']['cpu_utilization']:
                    self.record_failure(region, 'performance', 
                                      f"High CPU utilization: {cpu_util:.1%}")
                
                # 检查内存使用率
                memory_util = region_metrics.get('memory_utilization', 0)
                if memory_util > self.health_check_config['failure_thresholds']['memory_utilization']:
                    self.record_failure(region, 'performance', 
                                      f"High memory utilization: {memory_util:.1%}")
                    
        except Exception as e:
            print(f"Error analyzing performance metrics: {e}")
    
    async def evaluate_overall_health(self):
        """评估整体健康状况"""
        unhealthy_regions = [region for region, status in self.region_status.items() 
                           if status != 'healthy']
        
        if len(unhealthy_regions) >= 2:
            # 多个地域同时出现问题，触发紧急告警
            await self.trigger_emergency_response(unhealthy_regions)
        elif len(unhealthy_regions) == 1:
            # 单个地域问题，准备故障切换
            await self.prepare_failover(unhealthy_regions[0])
    
    def record_failure(self, region, failure_type, description):
        """记录问题信息"""
        if region not in self.failure_history:
            self.failure_history[region] = []
        
        failure_record = {
            'timestamp': datetime.now().isoformat(),
            'type': failure_type,
            'description': description,
            'severity': self.assess_failure_severity(failure_type, description)
        }
        
        self.failure_history[region].append(failure_record)
        
        # 保持历史记录在合理范围内
        if len(self.failure_history[region]) > 100:
            self.failure_history[region] = self.failure_history[region][-50:]
    
    def assess_failure_severity(self, failure_type, description):
        """评估问题严重程度"""
        critical_indicators = ['unreachable', 'failure', 'timeout', 'critical']
        warning_indicators = ['slow', 'high', 'latency', 'degraded']
        
        description_lower = description.lower()
        
        if any(indicator in description_lower for indicator in critical_indicators):
            return 'critical'
        elif any(indicator in description_lower for indicator in warning_indicators):
            return 'warning'
        else:
            return 'info'
    
    async def trigger_emergency_response(self, affected_regions):
        """触发紧急响应"""
        emergency_notification = {
            'type': 'emergency',
            'timestamp': datetime.now().isoformat(),
            'affected_regions': affected_regions,
            'status': 'multiple_region_failure',
            'recommended_action': 'activate_disaster_recovery_protocol'
        }
        
        print(f"EMERGENCY RESPONSE TRIGGERED: {json.dumps(emergency_notification, indent=2)}")
        
        # 这里应该集成具体的告警和响应系统
        await self.notify_stakeholders(emergency_notification)
        await self.activate_backup_systems()
    
    async def prepare_failover(self, failed_region):
        """准备故障切换"""
        failover_plan = {
            'failed_region': failed_region,
            'timestamp': datetime.now().isoformat(),
            'traffic_redirection_ready': await self.check_traffic_redirection_capability(),
            'backup_systems_healthy': await self.verify_backup_systems_health(),
            'estimated_switch_time': '5-10 minutes'
        }
        
        print(f"Failover preparation for {failed_region}: {json.dumps(failover_plan, indent=2)}")
        
        # 预热备份系统
        await self.preheat_backup_systems(failed_region)
    
    # 辅助方法（简化实现）
    def get_region_endpoints(self, region):
        return [f"https://{region}-endpoint.example.com/health"]
    
    async def get_region_deployments(self, region):
        return [f"app-{region}"]
    
    async def check_deployment_health(self, deployment, region):
        return {'healthy': True, 'reason': ''}
    
    def get_database_endpoints(self):
        return {region: f"{region}-db.example.com" for region in self.health_check_config['regions']}
    
    async def test_database_connection(self, endpoint):
        return {'reachable': True, 'response_time': 100, 'error': ''}
    
    def get_node_region(self, node):
        return node.metadata.labels.get('topology.kubernetes.io/region')
    
    def evaluate_node_health(self, node):
        return {'healthy': True, 'issues': []}
    
    async def fetch_performance_metrics(self):
        return {region: {} for region in self.health_check_config['regions']}
    
    async def notify_stakeholders(self, notification):
        print(f"Notifying stakeholders: {notification}")
    
    async def activate_backup_systems(self):
        print("Activating backup systems...")
    
    async def check_traffic_redirection_capability(self):
        return True
    
    async def verify_backup_systems_health(self):
        return True
    
    async def preheat_backup_systems(self, failed_region):
        print(f"Preheating backup systems for {failed_region}...")

# 使用示例
async def main():
    detector = IntelligentFailureDetector()
    await detector.run_continuous_monitoring()

if __name__ == "__main__":
    asyncio.run(main())
```

## 2. 自动故障切换
```yaml
# 自动故障切换配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: failover-controller
  namespace: dr-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: failover-controller
  template:
    metadata:
      labels:
        app: failover-controller
    spec:
      containers:
      - name: controller
        image: custom/failover-controller:latest
        env:
        - name: PRIMARY_REGION
          value: "us-east-1"
        - name: SECONDARY_REGION
          value: "us-west-2"
        - name: FAILOVER_THRESHOLD
          value: "3"  # 连续3次检查失败后切换
        - name: HEALTH_CHECK_INTERVAL
          value: "30"
        volumeMounts:
        - name: config
          mountPath: /config
      volumes:
      - name: config
        configMap:
          name: failover-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: failover-config
  namespace: dr-system
data:
  failover-strategy.yaml: |
    strategies:
    - name: "graceful-failover"
      trigger_conditions:
        consecutive_failures: 3
        failure_duration: "90s"
        affected_services: ["critical", "database"]
      actions:
        - "redirect-traffic"
        - "promote-secondary"
        - "notify-stakeholders"
        - "start-recovery-monitoring"
      
    - name: "emergency-failover"
      trigger_conditions:
        consecutive_failures: 1
        failure_type: "complete-outage"
        affected_regions: [">=2"]
      actions:
        - "immediate-traffic-switch"
        - "activate-hot-standby"
        - "emergency-notification"
        - "executive-briefing"
```

<!-- chunk: 📊 数据一致性保障 -->## 📊 数据一致性保障

## 分布式事务处理

## 1. 最终一致性协议
```python
#!/usr/bin/env python3
# 分布式数据一致性管理器

import asyncio
import json
import hashlib
from datetime import datetime, timedelta
from typing import Dict, List, Optional

class DataConsistencyManager:
    def __init__(self):
        self.regions = ['us-east-1', 'us-west-2', 'eu-west-1']
        self.consistency_config = {
            'sync_interval': 5,  # 秒
            'consistency_window': 60,  # 秒
            'conflict_resolution': 'timestamp_based',
            'data_validation_enabled': True
        }
        
        self.data_checksums = {region: {} for region in self.regions}
        self.sync_queue = asyncio.Queue()
        self.conflict_log = []
    
    async def run_consistency_monitoring(self):
        """运行一致性监控"""
        tasks = [
            self.monitor_data_changes(),
            self.synchronize_data_between_regions(),
            self.validate_data_consistency(),
            self.resolve_conflicts()
        ]
        
        await asyncio.gather(*tasks)
    
    async def monitor_data_changes(self):
        """监控数据变更"""
        while True:
            try:
                # 模拟检测各区域数据变更
                for region in self.regions:
                    changes = await self.detect_region_changes(region)
                    if changes:
                        await self.sync_queue.put({
                            'region': region,
                            'changes': changes,
                            'timestamp': datetime.now().isoformat()
                        })
                
                await asyncio.sleep(self.consistency_config['sync_interval'])
                
            except Exception as e:
                print(f"Error monitoring data changes: {e}")
                await asyncio.sleep(10)
    
    async def synchronize_data_between_regions(self):
        """在地域间同步数据"""
        while True:
            try:
                sync_item = await self.sync_queue.get()
                await self.propagate_changes(sync_item)
                self.sync_queue.task_done()
                
            except Exception as e:
                print(f"Error synchronizing data: {e}")
    
    async def validate_data_consistency(self):
        """验证数据一致性"""
        while True:
            try:
                # 定期验证所有区域数据一致性
                inconsistencies = await self.check_global_consistency()
                
                if inconsistencies:
                    print(f"Data inconsistencies detected: {inconsistencies}")
                    for inconsistency in inconsistencies:
                        await self.handle_inconsistency(inconsistency)
                
                await asyncio.sleep(self.consistency_config['consistency_window'])
                
            except Exception as e:
                print(f"Error validating consistency: {e}")
                await asyncio.sleep(30)
    
    async def resolve_conflicts(self):
        """解决数据冲突"""
        while True:
            try:
                # 检查并解决冲突
                conflicts = await self.detect_conflicts()
                
                for conflict in conflicts:
                    resolution = await self.resolve_conflict(conflict)
                    self.conflict_log.append({
                        'conflict': conflict,
                        'resolution': resolution,
                        'resolved_at': datetime.now().isoformat()
                    })
                
                await asyncio.sleep(10)
                
            except Exception as e:
                print(f"Error resolving conflicts: {e}")
                await asyncio.sleep(15)
    
    async def detect_region_changes(self, region: str) -> Optional[List[Dict]]:
        """检测地域数据变更"""
        # 模拟数据变更检测
        # 实际实现应该监控数据库变更日志或使用CDC
        import random
        
        if random.random() < 0.1:  # 10%概率有变更
            return [
                {
                    'table': 'users',
                    'operation': 'UPDATE',
                    'key': f'user_{random.randint(1, 1000)}',
                    'timestamp': datetime.now().isoformat()
                }
            ]
        return None
    
    async def propagate_changes(self, sync_item: Dict):
        """传播变更到其他地域"""
        source_region = sync_item['region']
        changes = sync_item['changes']
        
        # 向其他地域传播变更
        for target_region in self.regions:
            if target_region != source_region:
                try:
                    await self.apply_changes_to_region(target_region, changes, source_region)
                    print(f"Changes propagated from {source_region} to {target_region}")
                except Exception as e:
                    print(f"Failed to propagate changes to {target_region}: {e}")
                    await self.queue_retry(sync_item, target_region)
    
    async def apply_changes_to_region(self, region: str, changes: List[Dict], source_region: str):
        """将变更应用到指定地域"""
        # 这里应该实现具体的数据库变更应用逻辑
        # 包括冲突检测和解决
        for change in changes:
            # 应用变更
            await self.apply_single_change(region, change)
            
            # 更新校验和
            await self.update_data_checksum(region, change)
    
    async def apply_single_change(self, region: str, change: Dict):
        """应用单个变更"""
        # 实现具体的变更应用逻辑
        print(f"Applying {change['operation']} to {change['table']} in {region}")
    
    async def update_data_checksum(self, region: str, change: Dict):
        """更新数据校验和"""
        table = change['table']
        key = change['key']
        
        if table not in self.data_checksums[region]:
            self.data_checksums[region][table] = {}
        
        # 生成校验和（简化实现）
        checksum = hashlib.md5(f"{table}:{key}:{datetime.now()}".encode()).hexdigest()
        self.data_checksums[region][table][key] = {
            'checksum': checksum,
            'timestamp': change['timestamp']
        }
    
    async def check_global_consistency(self) -> List[Dict]:
        """检查全局数据一致性"""
        inconsistencies = []
        
        # 比较各区域相同数据的校验和
        all_tables = set()
        for region_checksums in self.data_checksums.values():
            all_tables.update(region_checksums.keys())
        
        for table in all_tables:
            # 收集所有区域的该表数据键
            all_keys = set()
            for region in self.regions:
                if table in self.data_checksums[region]:
                    all_keys.update(self.data_checksums[region][table].keys())
            
            # 检查每个键的一致性
            for key in all_keys:
                checksums = {}
                timestamps = {}
                
                for region in self.regions:
                    if (table in self.data_checksums[region] and 
                        key in self.data_checksums[region][table]):
                        checksums[region] = self.data_checksums[region][table][key]['checksum']
                        timestamps[region] = self.data_checksums[region][table][key]['timestamp']
                
                # 如果有多个不同的校验和，说明存在不一致
                if len(set(checksums.values())) > 1:
                    inconsistencies.append({
                        'table': table,
                        'key': key,
                        'inconsistent_regions': list(checksums.keys()),
                        'checksums': checksums,
                        'timestamps': timestamps,
                        'detected_at': datetime.now().isoformat()
                    })
        
        return inconsistencies
    
    async def handle_inconsistency(self, inconsistency: Dict):
        """处理数据不一致"""
        print(f"Handling inconsistency: {inconsistency}")
        
        # 基于时间戳解决冲突
        latest_region = max(inconsistency['timestamps'].items(), key=lambda x: x[1])[0]
        
        # 将最新数据同步到其他区域
        for region in inconsistency['inconsistent_regions']:
            if region != latest_region:
                await self.sync_data_from_region(region, latest_region, 
                                               inconsistency['table'], inconsistency['key'])
    
    async def sync_data_from_region(self, target_region: str, source_region: str, table: str, key: str):
        """从源地域同步数据到目标地域"""
        print(f"Syncing {table}.{key} from {source_region} to {target_region}")
        # 实现具体的数据同步逻辑
    
    async def detect_conflicts(self) -> List[Dict]:
        """检测数据冲突"""
        # 检测并发修改导致的冲突
        conflicts = []
        
        # 简化实现：检查同一时间段内的修改
        recent_changes = self.get_recent_changes(timedelta(seconds=30))
        
        for change_group in self.group_simultaneous_changes(recent_changes):
            if len(change_group) > 1:
                conflicts.append({
                    'type': 'simultaneous_modification',
                    'changes': change_group,
                    'detected_at': datetime.now().isoformat()
                })
        
        return conflicts
    
    async def resolve_conflict(self, conflict: Dict) -> Dict:
        """解决数据冲突"""
        if conflict['type'] == 'simultaneous_modification':
            # 基于时间戳解决冲突
            latest_change = max(conflict['changes'], key=lambda x: x['timestamp'])
            
            return {
                'strategy': 'timestamp_based',
                'winner': latest_change,
                'losers': [c for c in conflict['changes'] if c != latest_change],
                'applied_at': datetime.now().isoformat()
            }
        
        return {'strategy': 'manual_review_needed'}
    
    def get_recent_changes(self, time_window: timedelta) -> List[Dict]:
        """获取最近的变更"""
        # 简化实现
        return []
    
    def group_simultaneous_changes(self, changes: List[Dict]) -> List[List[Dict]]:
        """将同时发生的变更分组"""
        # 简化实现
        return []
    
    async def queue_retry(self, sync_item: Dict, target_region: str):
        """排队重试失败的同步"""
        retry_item = {
            **sync_item,
            'retry_target': target_region,
            'retry_count': sync_item.get('retry_count', 0) + 1,
            'retry_at': (datetime.now() + timedelta(minutes=1)).isoformat()
        }
        
        if retry_item['retry_count'] <= 3:  # 最多重试3次
            await asyncio.sleep(60)  # 等待1分钟后重试
            await self.sync_queue.put(retry_item)

# 使用示例
async def main():
    manager = DataConsistencyManager()
    await manager.run_consistency_monitoring()

if __name__ == "__main__":
    asyncio.run(main())
```

## 读写分离策略

## 1. 智能路由配置
```yaml
# 数据库读写分离配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: database-router
  namespace: database-system
spec:
  replicas: 3
  selector:
    matchLabels:
      app: database-router
  template:
    metadata:
      labels:
        app: database-router
    spec:
      containers:
      - name: router
        image: custom/database-router:latest
        ports:
        - containerPort: 3306
        env:
        - name: PRIMARY_REGION
          value: "us-east-1"
        - name: READ_REPLICAS
          value: "us-west-2,eu-west-1"
        - name: FAILOVER_STRATEGY
          value: "promote-closest-replica"
        - name: HEALTH_CHECK_INTERVAL
          value: "5"
        volumeMounts:
        - name: config
          mountPath: /config
      volumes:
      - name: config
        configMap:
          name: database-routing-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: database-routing-config
  namespace: database-system
data:
  routing-rules.yaml: |
    rules:
    - name: "primary-write-routing"
      condition: "operation == 'WRITE'"
      action: "route-to-primary"
      primary_region: "us-east-1"
      
    - name: "read-replica-routing"
      condition: "operation == 'READ' AND consistency == 'eventual'"
      action: "route-to-nearest-replica"
      replica_selection: "latency-based"
      
    - name: "strong-consistency-reads"
      condition: "operation == 'READ' AND consistency == 'strong'"
      action: "route-to-primary"
      
    - name: "failover-routing"
      condition: "primary_unhealthy == true"
      action: "promote-replica"
      promotion_order: ["us-west-2", "eu-west-1"]
```

<!-- chunk: 📈 性能优化 -->## 📈 性能优化

## 延迟优化策略

## 1. CDN和边缘计算
```yaml
# 全球CDN配置
apiVersion: networking.istio.io/v1beta1
kind: Gateway
metadata:
  name: global-gateway
spec:
  selector:
    istio: ingressgateway
  servers:
  - port:
      number: 80
      name: http
      protocol: HTTP
    hosts:
    - "*.example.com"
  - port:
      number: 443
      name: https
      protocol: HTTPS
    hosts:
    - "*.example.com"
    tls:
      mode: SIMPLE
      credentialName: global-tls-cert
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: cdn-routing
spec:
  hosts:
  - "*.example.com"
  gateways:
  - global-gateway
  http:
  - matchers:
    - - uri=""
    - prefix="/static/"
    - route=""
    - - destination=""
    - host="cdn-edge-cluster"
    - port=""
    - number="80"
    - headers=""
    - response=""
    - set=""
    - cache-control="public, max-age=31536000"
  - matchers:
    - - uri=""
    - prefix="/api/"
    - route=""
    - - destination=""
    - host="app-primary-region"
    - port=""
    - number="8080"
    - weight="70"
    - - destination=""
    - host="app-secondary-region"
    - port=""
    - number="8080"
    - weight="30"
```

## 2. 缓存策略优化
```yaml
# 多级缓存配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: multi-tier-cache
spec:
  replicas: 3
  selector:
    matchLabels:
      app: multi-tier-cache
  template:
    metadata:
      labels:
        app: multi-tier-cache
    spec:
      containers:
      - name: cache-manager
        image: custom/cache-manager:latest
        env:
        - name: L1_CACHE_TTL
          value: "300"  # 5分钟
        - name: L2_CACHE_TTL
          value: "3600" # 1小时
        - name: L3_CACHE_TTL
          value: "86400" # 24小时
        - name: CACHE_INVALIDATION_STRATEGY
          value: "write-through"
        ports:
        - containerPort: 6379
        volumeMounts:
        - name: cache-storage
          mountPath: /cache
      volumes:
      - name: cache-storage
        emptyDir: {}
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: cache-strategy-config
data:
  cache-strategy.yaml: |
    layers:
    - name: "l1-redis"
      type: "redis"
      ttl: 300
      location: "same-region"
      use_case: "hot-data"
      
    - name: "l2-memcached"
      type: "memcached"
      ttl: 3600
      location: "regional"
      use_case: "warm-data"
      
    - name: "l3-cdn"
      type: "cdn"
      ttl: 86400
      location: "global"
      use_case: "static-content"
    
    invalidation_rules:
    - pattern: "/api/users/*"
      layers: ["l1-redis", "l2-memcached"]
      trigger: "database-update"
      
    - pattern: "/static/images/*"
      layers: ["l3-cdn"]
      trigger: "content-change"
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## 架构设计阶段
- [ ] 设计多地域部署架构和网络拓扑
- [ ] 规划数据同步和一致性策略
- [ ] 制定故障检测和切换机制
- [ ] 设计全球负载均衡和路由策略
- [ ] 配置安全隔离和访问控制
- [ ] 建立监控告警和运维体系

## 部署实施阶段
- [ ] 部署多地域Kubernetes集群
- [ ] 配置数据库多活复制
- [ ] 实施应用多地域部署
- [ ] 部署智能路由和负载均衡
- [ ] 配置数据同步和一致性保障
- [ ] 实施安全和合规控制

## 运营维护阶段
- [ ] 建立常态化演练机制
- [ ] 实施性能监控和优化
- [ ] 维护故障处理流程
- [ ] 持续改进架构设计
- [ ] 定期评估和更新策略
- [ ] 培养跨地域协作能力

---

*本文档为企业级跨区域容灾部署提供完整的架构设计和实施指导*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- 生产运维 MOC
- [[13-生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[01-集群基础/02-设计原则/02-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单

## Related

- 22-production-checklist
- [[02-工作负载/02-Java-on-K8s/01-spring-boot-kubernetes-production.md|02-spring-boot-kubernetes-production]]
- [[21-生态参考/03-领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]

## See Also

- 16-enterprise-backup-strategy
- 17-disaster-recovery-drills
- 19-cluster-performance-tuning
- 20-network-performance-optimization


<!-- risk-assessed -->
