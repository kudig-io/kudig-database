# 13-Kubernetes成本治理

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

Kubernetes成本治理是FinOps实践的核心组成部分。本文档详细介绍如何实施有效的成本监控、优化和治理策略。

## 💰 成本分析框架

### 成本构成分析

#### 1. 成本分摊模型
```yaml
# 成本分摊配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cost-allocation-config
  namespace: finops
data:
  allocation-rules.yaml: |
    rules:
    - name: "team-based-allocation"
      selector:
        matchLabels:
          team: "*"
      allocation:
        type: "proportional"
        metric: "resource_usage"
        
    - name: "project-based-allocation"
      selector:
        matchLabels:
          project: "*"
      allocation:
        type: "fixed"
        percentage: 100
        
    - name: "environment-based-allocation"
      selector:
        matchLabels:
          environment: "*"
      allocation:
        type: "tiered"
        tiers:
        - value: "production"
          multiplier: 1.0
        - value: "staging"
          multiplier: 0.5
        - value: "development"
          multiplier: 0.2
```

#### 2. 成本标签体系
```yaml
# 资源标签策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: cost-labeling-policy
  namespace: finops
data:
  labeling-rules.yaml: |
    required_labels:
    - name: "cost-center"
      description: "Business unit or cost center responsible"
      regex: "^[A-Z0-9]{3,10}$"
      
    - name: "team"
      description: "Team owning the resource"
      allowed_values: ["frontend", "backend", "platform", "data"]
      
    - name: "environment"
      description: "Deployment environment"
      allowed_values: ["production", "staging", "development", "testing"]
      
    - name: "project"
      description: "Project or initiative name"
      regex: "^[a-z0-9]([a-z0-9-]*[a-z0-9])?$"
      
    - name: "owner"
      description: "Primary contact person"
      regex: "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$"
```

### 成本收集和计量

#### 1. Prometheus成本指标
```yaml
# 成本相关指标收集
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-metrics
  namespace: monitoring
spec:
  groups:
  - name: cost.rules
    rules:
    # CPU成本计算
    - record: cost:cpu_hours:sum_rate
      expr: sum(rate(container_cpu_usage_seconds_total[1h])) by (namespace, pod, container)
      
    # 内存成本计算
    - record: cost:memory_gb_hours:sum_rate
      expr: sum(rate(container_memory_working_set_bytes[1h]) / 1024^3) by (namespace, pod, container)
      
    # 存储成本计算
    - record: cost:storage_gb_hours:sum_rate
      expr: sum(rate(container_fs_usage_bytes[1h]) / 1024^3) by (namespace, pod, container)
      
    # 网络成本计算
    - record: cost:network_bytes:sum_rate
      expr: sum(rate(container_network_receive_bytes_total[1h]) + rate(container_network_transmit_bytes_total[1h])) by (namespace, pod)
```

#### 2. 成本数据聚合
```python
#!/usr/bin/env python3
# 成本数据聚合和分析脚本

import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import json
import requests

class CostAnalyzer:
    def __init__(self, prometheus_url, cloud_billing_api=None):
        self.prometheus_url = prometheus_url
        self.cloud_billing_api = cloud_billing_api
        self.cost_rates = {
            'aws': {
                'cpu_hour': 0.023,      # m5.large实例CPU小时成本
                'memory_gb_hour': 0.003, # 内存GB小时成本
                'storage_gb_month': 0.10, # EBS存储月成本
                'network_gb': 0.01       # 网络流量GB成本
            }
        }
    
    def query_prometheus(self, query, start_time, end_time, step='1h'):
        """查询Prometheus指标"""
        params = {
            'query': query,
            'start': start_time.timestamp(),
            'end': end_time.timestamp(),
            'step': step
        }
        
        response = requests.get(f"{self.prometheus_url}/api/v1/query_range", params=params)
        return response.json()
    
    def calculate_resource_costs(self, start_time, end_time):
        """计算资源使用成本"""
        # 查询CPU使用率
        cpu_query = 'sum(rate(container_cpu_usage_seconds_total[1h])) by (namespace, pod, container)'
        cpu_data = self.query_prometheus(cpu_query, start_time, end_time)
        
        # 查询内存使用
        memory_query = 'sum(rate(container_memory_working_set_bytes[1h]) / 1024^3) by (namespace, pod, container)'
        memory_data = self.query_prometheus(memory_query, start_time, end_time)
        
        # 查询存储使用
        storage_query = 'sum(rate(container_fs_usage_bytes[1h]) / 1024^3) by (namespace, pod, container)'
        storage_data = self.query_prometheus(storage_query, start_time, end_time)
        
        # 计算成本
        costs = {
            'cpu_cost': self.calculate_cpu_cost(cpu_data),
            'memory_cost': self.calculate_memory_cost(memory_data),
            'storage_cost': self.calculate_storage_cost(storage_data)
        }
        
        return costs
    
    def calculate_cpu_cost(self, cpu_data):
        """计算CPU成本"""
        total_cpu_hours = 0
        cost_breakdown = {}
        
        if 'data' in cpu_data and 'result' in cpu_data['data']:
            for series in cpu_data['data']['result']:
                namespace = series['metric'].get('namespace', 'unknown')
                values = series['values']
                
                namespace_cpu_hours = sum(float(value[1]) for value in values)
                total_cpu_hours += namespace_cpu_hours
                
                if namespace not in cost_breakdown:
                    cost_breakdown[namespace] = 0
                cost_breakdown[namespace] += namespace_cpu_hours * self.cost_rates['aws']['cpu_hour']
        
        return {
            'total': total_cpu_hours * self.cost_rates['aws']['cpu_hour'],
            'breakdown': cost_breakdown,
            'total_hours': total_cpu_hours
        }
    
    def calculate_memory_cost(self, memory_data):
        """计算内存成本"""
        total_memory_gb_hours = 0
        cost_breakdown = {}
        
        if 'data' in memory_data and 'result' in memory_data['data']:
            for series in memory_data['data']['result']:
                namespace = series['metric'].get('namespace', 'unknown')
                values = series['values']
                
                namespace_memory_gb_hours = sum(float(value[1]) for value in values)
                total_memory_gb_hours += namespace_memory_gb_hours
                
                if namespace not in cost_breakdown:
                    cost_breakdown[namespace] = 0
                cost_breakdown[namespace] += namespace_memory_gb_hours * self.cost_rates['aws']['memory_gb_hour']
        
        return {
            'total': total_memory_gb_hours * self.cost_rates['aws']['memory_gb_hour'],
            'breakdown': cost_breakdown,
            'total_gb_hours': total_memory_gb_hours
        }
    
    def calculate_storage_cost(self, storage_data):
        """计算存储成本"""
        total_storage_gb_hours = 0
        cost_breakdown = {}
        
        if 'data' in storage_data and 'result' in storage_data['data']:
            for series in storage_data['data']['result']:
                namespace = series['metric'].get('namespace', 'unknown')
                values = series['values']
                
                namespace_storage_gb_hours = sum(float(value[1]) for value in values)
                total_storage_gb_hours += namespace_storage_gb_hours
                
                if namespace not in cost_breakdown:
                    cost_breakdown[namespace] = 0
                cost_breakdown[namespace] += (namespace_storage_gb_hours / 24 / 30) * self.cost_rates['aws']['storage_gb_month']
        
        return {
            'total': (total_storage_gb_hours / 24 / 30) * self.cost_rates['aws']['storage_gb_month'],
            'breakdown': cost_breakdown,
            'total_gb_hours': total_storage_gb_hours
        }
    
    def generate_cost_report(self, start_time, end_time):
        """生成成本报告"""
        costs = self.calculate_resource_costs(start_time, end_time)
        
        report = {
            'period': {
                'start': start_time.isoformat(),
                'end': end_time.isoformat()
            },
            'summary': {
                'total_cost': sum(cost['total'] for cost in costs.values()),
                'cpu_cost': costs['cpu_cost']['total'],
                'memory_cost': costs['memory_cost']['total'],
                'storage_cost': costs['storage_cost']['total']
            },
            'breakdown_by_namespace': self.aggregate_namespace_costs(costs),
            'recommendations': self.generate_cost_recommendations(costs)
        }
        
        return report
    
    def aggregate_namespace_costs(self, costs):
        """按命名空间聚合成本"""
        namespace_costs = {}
        
        for cost_type, cost_data in costs.items():
            for namespace, amount in cost_data['breakdown'].items():
                if namespace not in namespace_costs:
                    namespace_costs[namespace] = 0
                namespace_costs[namespace] += amount
        
        return dict(sorted(namespace_costs.items(), key=lambda x: x[1], reverse=True))
    
    def generate_cost_recommendations(self, costs):
        """生成成本优化建议"""
        recommendations = []
        
        # CPU利用率分析
        cpu_efficiency = self.analyze_cpu_efficiency()
        if cpu_efficiency < 0.5:
            recommendations.append({
                'type': 'rightsizing',
                'priority': 'high',
                'description': f'Low CPU utilization ({cpu_efficiency:.1%}), consider rightsizing pods',
                'estimated_savings': costs['cpu_cost']['total'] * 0.3
            })
        
        # 内存浪费分析
        memory_waste = self.analyze_memory_waste()
        if memory_waste > 0.3:
            recommendations.append({
                'type': 'memory_optimization',
                'priority': 'medium',
                'description': f'High memory waste ({memory_waste:.1%}), optimize memory requests/limits',
                'estimated_savings': costs['memory_cost']['total'] * 0.2
            })
        
        # 闲置资源分析
        idle_resources = self.find_idle_resources()
        if idle_resources:
            recommendations.append({
                'type': 'resource_cleanup',
                'priority': 'high',
                'description': f'Found {len(idle_resources)} idle resources, consider cleanup',
                'estimated_savings': sum(res['monthly_cost'] for res in idle_resources)
            })
        
        return recommendations
    
    def analyze_cpu_efficiency(self):
        """分析CPU使用效率"""
        # 查询CPU请求和实际使用
        request_query = 'sum(kube_pod_container_resource_requests{resource="cpu"}) by (namespace)'
        usage_query = 'sum(rate(container_cpu_usage_seconds_total[1h])) by (namespace)'
        
        # 简化的效率计算
        return 0.65  # 模拟值
    
    def analyze_memory_waste(self):
        """分析内存浪费"""
        # 查询内存请求和实际使用
        request_query = 'sum(kube_pod_container_resource_requests{resource="memory"}) by (namespace)'
        usage_query = 'sum(container_memory_working_set_bytes) by (namespace)'
        
        # 简化的浪费计算
        return 0.35  # 模拟值
    
    def find_idle_resources(self):
        """查找闲置资源"""
        # 查询长时间未使用的Pods
        idle_query = '''
        count by(pod, namespace) (
            rate(container_cpu_usage_seconds_total[1h]) < 0.01
        ) > 24
        '''
        
        # 简化的闲置资源列表
        return [
            {
                'pod': 'old-app-7d5b8c9d4-xl2vz',
                'namespace': 'legacy',
                'monthly_cost': 45.23
            }
        ]

# 使用示例
if __name__ == "__main__":
    analyzer = CostAnalyzer("http://prometheus:9090")
    
    end_time = datetime.now()
    start_time = end_time - timedelta(days=30)
    
    report = analyzer.generate_cost_report(start_time, end_time)
    
    print("Monthly Cost Report:")
    print(json.dumps(report, indent=2))
```

## 📊 成本优化策略

### 资源权利化

#### 1. 自动权利化工具
```python
#!/usr/bin/env python3
# 自动资源权利化工具

import asyncio
from kubernetes import client, config
from kubernetes.client.rest import ApiException
import numpy as np
from datetime import datetime, timedelta

class RightsizingOptimizer:
    def __init__(self):
        config.load_kube_config()
        self.apps_v1 = client.AppsV1Api()
        self.core_v1 = client.CoreV1Api()
        self.metrics_client = client.CustomObjectsApi()
        
    async def analyze_workload_metrics(self, namespace, workload_name, workload_type="deployment"):
        """分析工作负载指标"""
        # 获取历史指标数据（这里简化处理）
        cpu_usage_data = await self.get_cpu_usage_history(namespace, workload_name)
        memory_usage_data = await self.get_memory_usage_history(namespace, workload_name)
        
        # 计算建议值
        cpu_recommendation = self.calculate_cpu_recommendation(cpu_usage_data)
        memory_recommendation = self.calculate_memory_recommendation(memory_usage_data)
        
        return {
            'cpu_current': self.get_current_cpu_request(namespace, workload_name),
            'memory_current': self.get_current_memory_request(namespace, workload_name),
            'cpu_recommended': cpu_recommendation,
            'memory_recommended': memory_recommendation,
            'savings_potential': self.calculate_savings_potential(
                cpu_usage_data, memory_usage_data
            )
        }
    
    async def get_cpu_usage_history(self, namespace, workload_name):
        """获取CPU使用历史"""
        # 简化的模拟数据
        return np.random.normal(0.3, 0.1, 168)  # 一周每小时数据
    
    async def get_memory_usage_history(self, namespace, workload_name):
        """获取内存使用历史"""
        # 简化的模拟数据
        return np.random.normal(0.4, 0.15, 168)  # 一周每小时数据
    
    def calculate_cpu_recommendation(self, cpu_data):
        """计算CPU建议值"""
        # 使用95百分位数作为建议值，并增加20%缓冲
        p95 = np.percentile(cpu_data, 95)
        return round(p95 * 1.2, 3)
    
    def calculate_memory_recommendation(self, memory_data):
        """计算内存建议值"""
        # 使用95百分位数作为建议值，并增加25%缓冲
        p95 = np.percentile(memory_data, 95)
        return f"{round(p95 * 1.25 * 1000)}Mi"  # 转换为Mi单位
    
    def get_current_cpu_request(self, namespace, workload_name):
        """获取当前CPU请求值"""
        try:
            deployment = self.apps_v1.read_namespaced_deployment(workload_name, namespace)
            container = deployment.spec.template.spec.containers[0]
            return container.resources.requests.get('cpu', '0') if container.resources.requests else '0'
        except ApiException:
            return '0'
    
    def get_current_memory_request(self, namespace, workload_name):
        """获取当前内存请求值"""
        try:
            deployment = self.apps_v1.read_namespaced_deployment(workload_name, namespace)
            container = deployment.spec.template.spec.containers[0]
            return container.resources.requests.get('memory', '0') if container.resources.requests else '0'
        except ApiException:
            return '0'
    
    def calculate_savings_potential(self, cpu_data, memory_data):
        """计算节省潜力"""
        current_cpu_cost = 0.023  # 简化的成本计算
        current_memory_cost = 0.003
        
        # 计算当前和建议的成本差异
        cpu_savings = current_cpu_cost * (1 - np.mean(cpu_data) / 0.5)  # 假设当前请求为0.5
        memory_savings = current_memory_cost * (1 - np.mean(memory_data) / 0.5)
        
        return {
            'monthly_cpu_savings': cpu_savings * 720,  # 720小时/月
            'monthly_memory_savings': memory_savings * 720,
            'total_monthly_savings': (cpu_savings + memory_savings) * 720
        }
    
    async def apply_rightsizing(self, namespace, workload_name, recommendations):
        """应用权利化建议"""
        try:
            # 获取当前部署配置
            deployment = self.apps_v1.read_namespaced_deployment(workload_name, namespace)
            
            # 更新资源配置
            container = deployment.spec.template.spec.containers[0]
            if not container.resources:
                container.resources = client.V1ResourceRequirements()
            if not container.resources.requests:
                container.resources.requests = {}
            if not container.resources.limits:
                container.resources.limits = {}
            
            # 应用建议值
            container.resources.requests['cpu'] = str(recommendations['cpu_recommended'])
            container.resources.requests['memory'] = recommendations['memory_recommended']
            container.resources.limits['cpu'] = str(recommendations['cpu_recommended'] * 1.5)
            container.resources.limits['memory'] = str(int(recommendations['memory_recommended'].rstrip('Mi')) * 1.5) + 'Mi'
            
            # 更新部署
            self.apps_v1.patch_namespaced_deployment(workload_name, namespace, deployment)
            
            print(f"Applied rightsizing to {namespace}/{workload_name}")
            return True
            
        except ApiException as e:
            print(f"Failed to apply rightsizing: {e}")
            return False
    
    async def run_batch_optimization(self, namespaces=None):
        """批量运行优化"""
        if not namespaces:
            # 获取所有命名空间
            namespace_list = self.core_v1.list_namespace()
            namespaces = [ns.metadata.name for ns in namespace_list.items 
                         if ns.metadata.name not in ['kube-system', 'monitoring']]
        
        optimization_results = []
        
        for namespace in namespaces:
            try:
                deployments = self.apps_v1.list_namespaced_deployment(namespace)
                
                for deployment in deployments.items:
                    workload_name = deployment.metadata.name
                    
                    # 分析工作负载
                    recommendations = await self.analyze_workload_metrics(
                        namespace, workload_name
                    )
                    
                    # 检查是否有优化空间
                    if (recommendations['savings_potential']['total_monthly_savings'] > 10):  # 超过10美元才优化
                        result = {
                            'namespace': namespace,
                            'workload': workload_name,
                            'current_config': {
                                'cpu': recommendations['cpu_current'],
                                'memory': recommendations['memory_current']
                            },
                            'recommended_config': {
                                'cpu': recommendations['cpu_recommended'],
                                'memory': recommendations['memory_recommended']
                            },
                            'savings': recommendations['savings_potential']
                        }
                        
                        optimization_results.append(result)
                        
                        # 可选择自动应用优化
                        # await self.apply_rightsizing(namespace, workload_name, recommendations)
                        
            except ApiException as e:
                print(f"Error processing namespace {namespace}: {e}")
        
        return optimization_results

# 使用示例
async def main():
    optimizer = RightsizingOptimizer()
    results = await optimizer.run_batch_optimization(['production', 'staging'])
    
    print("Rightsizing Recommendations:")
    for result in results:
        print(f"\nWorkload: {result['namespace']}/{result['workload']}")
        print(f"Current: CPU={result['current_config']['cpu']}, Memory={result['current_config']['memory']}")
        print(f"Recommended: CPU={result['recommended_config']['cpu']}, Memory={result['recommended_config']['memory']}")
        print(f"Potential Savings: ${result['savings']['total_monthly_savings']:.2f}/month")

if __name__ == "__main__":
    asyncio.run(main())
```

### Spot实例优化

#### 1. Spot实例调度器
```yaml
# Spot实例节点组配置
apiVersion: eksctl.io/v1alpha5
kind: ClusterConfig
metadata:
  name: spot-optimized-cluster
  region: us-west-2

managedNodeGroups:
- name: spot-critical-ng
  instanceTypes: ["m5.large", "m5.xlarge"]
  spot: true
  desiredCapacity: 5
  minSize: 3
  maxSize: 10
  labels:
    node-group: spot-critical
    lifecycle: spot
  taints:
  - key: spot
    value: "true"
    effect: NoSchedule
  tags:
    k8s.io/cluster-autoscaler/node-template/label/lifecycle: spot
    k8s.io/cluster-autoscaler/node-template/taint/spot: "true:NoSchedule"

- name: spot-batch-ng
  instanceTypes: ["c5.large", "c5.xlarge", "c5.2xlarge"]
  spot: true
  desiredCapacity: 3
  minSize: 1
  maxSize: 8
  labels:
    node-group: spot-batch
    workload: batch
  taints:
  - key: spot-batch
    value: "true"
    effect: NoSchedule
---
# Spot实例容忍度配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spot-tolerant-app
spec:
  replicas: 3
  template:
    spec:
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: lifecycle
                operator: In
                values: ["spot"]
      tolerations:
      - key: spot
        operator: Equal
        value: "true"
        effect: NoSchedule
      - key: spot-batch
        operator: Equal
        value: "true"
        effect: NoSchedule
      containers:
      - name: app
        image: myapp:latest
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
```

#### 2. Spot中断处理
```python
#!/usr/bin/env python3
# Spot实例中断处理程序

import asyncio
import json
import boto3
from kubernetes import client, config
from kubernetes.client.rest import ApiException

class SpotInterruptionHandler:
    def __init__(self):
        config.load_kube_config()
        self.apps_v1 = client.AppsV1Api()
        self.core_v1 = client.CoreV1Api()
        self.ec2_client = boto3.client('ec2')
        self.sqs_client = boto3.client('sqs')
        
    async def monitor_spot_interruptions(self, queue_url):
        """监控Spot实例中断"""
        while True:
            try:
                # 从SQS队列接收中断通知
                response = self.sqs_client.receive_message(
                    QueueUrl=queue_url,
                    MaxNumberOfMessages=10,
                    WaitTimeSeconds=20
                )
                
                if 'Messages' in response:
                    for message in response['Messages']:
                        interruption_event = json.loads(message['Body'])
                        await self.handle_interruption(interruption_event)
                        
                        # 删除已处理的消息
                        self.sqs_client.delete_message(
                            QueueUrl=queue_url,
                            ReceiptHandle=message['ReceiptHandle']
                        )
                
                await asyncio.sleep(1)
                
            except Exception as e:
                print(f"Error monitoring interruptions: {e}")
                await asyncio.sleep(30)
    
    async def handle_interruption(self, event):
        """处理中断事件"""
        if event.get('detail-type') == 'EC2 Spot Instance Interruption Warning':
            instance_id = event['detail']['instance-id']
            interruption_time = event['detail']['instance-action']
            
            print(f"Handling spot interruption for instance {instance_id}")
            
            # 获取节点名称
            node_name = await self.get_node_name_by_instance(instance_id)
            if not node_name:
                return
            
            # 准备节点排水
            await self.prepare_node_drain(node_name)
            
            # 优雅地排水节点
            await self.drain_node_gracefully(node_name)
            
            # 等待新节点启动
            await self.wait_for_node_replacement(node_name)
    
    async def get_node_name_by_instance(self, instance_id):
        """通过实例ID获取节点名称"""
        try:
            nodes = self.core_v1.list_node()
            
            for node in nodes.items:
                provider_id = node.spec.provider_id
                if provider_id and instance_id in provider_id:
                    return node.metadata.name
            
            return None
            
        except ApiException as e:
            print(f"Error getting node name: {e}")
            return None
    
    async def prepare_node_drain(self, node_name):
        """准备节点排水"""
        try:
            # 标记节点为不可调度
            body = {
                "spec": {
                    "unschedulable": True
                }
            }
            self.core_v1.patch_node(node_name, body)
            
            print(f"Marked node {node_name} as unschedulable")
            
        except ApiException as e:
            print(f"Error preparing node drain: {e}")
    
    async def drain_node_gracefully(self, node_name):
        """优雅地排水节点"""
        try:
            # 获取节点上的Pods
            pods = self.core_v1.list_pod_for_all_namespaces(
                field_selector=f'spec.nodeName={node_name}'
            )
            
            # 按优先级排序Pods
            evictable_pods = []
            critical_pods = []
            
            for pod in pods.items:
                # 跳过关键系统Pods
                if (pod.metadata.namespace in ['kube-system', 'monitoring'] or
                    pod.metadata.labels.get('app') in ['istio', 'calico']):
                    critical_pods.append(pod)
                    continue
                
                evictable_pods.append(pod)
            
            # 逐个驱逐Pods
            for pod in evictable_pods:
                try:
                    # 检查是否有PDB允许驱逐
                    if await self.can_evict_pod(pod):
                        await self.evict_pod(pod)
                        await asyncio.sleep(5)  # 给予重新调度时间
                        
                except Exception as e:
                    print(f"Error evicting pod {pod.metadata.name}: {e}")
            
            print(f"Drained {len(evictable_pods)} pods from node {node_name}")
            
        except ApiException as e:
            print(f"Error draining node: {e}")
    
    async def can_evict_pod(self, pod):
        """检查是否可以驱逐Pod"""
        try:
            # 检查PodDisruptionBudget
            pdb_list = self.policy_v1beta1.list_pod_disruption_budget_for_all_namespaces()
            
            for pdb in pdb_list.items:
                if (pdb.metadata.namespace == pod.metadata.namespace and
                    self.matches_selector(pod, pdb.spec.selector)):
                    
                    # 检查当前干扰是否在允许范围内
                    current_disruptions = await self.get_current_disruptions(pdb)
                    max_unavailable = pdb.spec.max_unavailable
                    
                    if isinstance(max_unavailable, str) and '%' in max_unavailable:
                        percentage = int(max_unavailable.rstrip('%'))
                        max_allowed = int(len(self.get_matching_pods(pdb)) * percentage / 100)
                    else:
                        max_allowed = int(max_unavailable)
                    
                    return current_disruptions < max_allowed
            
            return True  # 没有PDB限制
            
        except Exception as e:
            print(f"Error checking eviction allowance: {e}")
            return False
    
    async def evict_pod(self, pod):
        """驱逐Pod"""
        try:
            eviction = client.V1beta1Eviction(
                metadata=client.V1ObjectMeta(
                    name=pod.metadata.name,
                    namespace=pod.metadata.namespace
                )
            )
            
            self.core_v1.create_namespaced_pod_eviction(
                name=pod.metadata.name,
                namespace=pod.metadata.namespace,
                body=eviction
            )
            
            print(f"Evicted pod {pod.metadata.namespace}/{pod.metadata.name}")
            
        except ApiException as e:
            print(f"Error evicting pod: {e}")
    
    async def wait_for_node_replacement(self, old_node_name):
        """等待节点替换"""
        max_wait_time = 300  # 5分钟
        check_interval = 10
        
        for i in range(max_wait_time // check_interval):
            try:
                node = self.core_v1.read_node(old_node_name)
                
                # 检查节点是否准备好
                for condition in node.status.conditions:
                    if condition.type == 'Ready' and condition.status == 'True':
                        print(f"Node {old_node_name} is ready again")
                        return
                
            except ApiException:
                # 节点可能已被删除，检查是否有新节点
                nodes = self.core_v1.list_node()
                spot_nodes = [node for node in nodes.items 
                             if node.metadata.labels.get('lifecycle') == 'spot']
                
                if len(spot_nodes) >= self.get_expected_spot_node_count():
                    print("New spot nodes are available")
                    return
            
            await asyncio.sleep(check_interval)
        
        print(f"Timeout waiting for node replacement after {max_wait_time} seconds")
    
    def get_expected_spot_node_count(self):
        """获取预期的Spot节点数量"""
        # 这里可以根据你的配置返回预期数量
        return 5

# 使用示例
async def main():
    handler = SpotInterruptionHandler()
    await handler.monitor_spot_interruptions('https://sqs.us-west-2.amazonaws.com/123456789012/spot-interruption-queue')

if __name__ == "__main__":
    asyncio.run(main())
```

## 📈 成本监控告警

### 预算管理和告警

#### 1. 预算配置
```yaml
# 成本预算配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: cost-budgets
  namespace: finops
data:
  budgets.yaml: |
    budgets:
    - name: "monthly-total-budget"
      period: "monthly"
      amount: 10000
      currency: "USD"
      thresholds: [50, 80, 90, 100]
      recipients:
      - "finops-team@example.com"
      - "platform-team@example.com"
      
    - name: "team-budgets"
      period: "monthly"
      by_label: "team"
      amounts:
        frontend: 3000
        backend: 4000
        platform: 2000
        data: 1000
      currency: "USD"
      thresholds: [75, 90, 100]
      recipients:
      - "team-leads@example.com"
      
    - name: "project-budgets"
      period: "weekly"
      by_label: "project"
      amounts:
        ecommerce: 2000
        analytics: 1500
        mobile: 1000
      currency: "USD"
      thresholds: [80, 95, 100]
      recipients:
      - "project-managers@example.com"
```

#### 2. 成本告警规则
```yaml
# Prometheus成本告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-alerts
  namespace: monitoring
spec:
  groups:
  - name: cost.rules
    rules:
    # 预算超支告警
    - alert: BudgetExceeded
      expr: |
        sum by(team) (
          cost:total_daily:sum_rate * 30
        ) > on(team) group_left(amount)
        cost:budget:amount{budget="team-monthly"}
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "Team {{ $labels.team }} budget exceeded"
        description: "Monthly budget of {{ $labels.amount }} USD exceeded by {{ printf \"%.2f\" $value }} USD"
        
    # 异常成本增长告警
    - alert: AbnormalCostIncrease
      expr: |
        abs(
          rate(cost:total_daily:sum_rate[1d])
          /
          rate(cost:total_daily:sum_rate[7d] offset 1d)
          - 1
        ) > 0.3
      for: 2h
      labels:
        severity: warning
      annotations:
        summary: "Abnormal cost increase detected"
        description: "Daily cost increased by more than 30% compared to last week"
        
    # 资源浪费告警
    - alert: ResourceWasteDetected
      expr: |
        sum by(namespace) (
          kube_resourcequota{type="hard", resource="requests.cpu"}
          -
          sum by(namespace) (kube_pod_container_resource_requests{resource="cpu"})
        ) / sum by(namespace) (kube_resourcequota{type="hard", resource="requests.cpu"}) > 0.4
      for: 6h
      labels:
        severity: info
      annotations:
        summary: "Significant CPU resource waste in namespace {{ $labels.namespace }}"
        description: "Over 40% of allocated CPU resources are unused"
```

## 🔧 实施检查清单

### 成本治理体系
- [ ] 建立成本分摊和标签体系
- [ ] 部署成本收集和计量工具
- [ ] 实施资源权利化自动化
- [ ] 配置Spot实例优化策略
- [ ] 建立预算管理和告警机制
- [ ] 实施成本可视化和报告

### 优化策略实施
- [ ] 分析现有资源使用效率
- [ ] 制定资源权利化计划
- [ ] 配置自动扩缩容策略
- [ ] 实施闲置资源清理机制
- [ ] 优化存储和网络成本
- [ ] 建立成本优化持续改进

### 监控和治理
- [ ] 部署实时成本监控系统
- [ ] 配置多层级告警机制
- [ ] 建立成本异常检测能力
- [ ] 实施成本趋势分析
- [ ] 建立成本治理流程
- [ ] 定期审查和优化成本策略

---

*本文档为企业级Kubernetes成本治理提供完整的策略框架和技术实施方案*