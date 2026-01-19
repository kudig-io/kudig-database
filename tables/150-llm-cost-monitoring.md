# LLM 成本监控与 FinOps

## 概述

LLM 工作负载的成本结构与传统应用显著不同,GPU 计算成本占主导地位。本文档详细介绍 LLM 成本监控体系、优化策略和 FinOps 实践。

## 成本架构

### LLM 成本构成模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              LLM 成本构成模型                                        │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                           总成本 (Total Cost)                                │   │
│   │                                                                              │   │
│   │   ┌─────────────────────────────────────────────────────────────────────┐   │   │
│   │   │                      GPU 计算成本 (65-80%)                           │   │   │
│   │   │                                                                      │   │   │
│   │   │   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐               │   │   │
│   │   │   │   训练成本   │   │   推理成本   │   │  微调成本   │               │   │   │
│   │   │   │             │   │             │   │             │               │   │   │
│   │   │   │ • 大批量    │   │ • 持续运行  │   │ • 中等批量  │               │   │   │
│   │   │   │ • 高显存    │   │ • 低延迟    │   │ • 周期性    │               │   │   │
│   │   │   │ • 可中断    │   │ • 弹性伸缩  │   │ • 可中断    │               │   │   │
│   │   │   └─────────────┘   └─────────────┘   └─────────────┘               │   │   │
│   │   │                                                                      │   │   │
│   │   └──────────────────────────────────────────────────────────────────────┘   │   │
│   │                                                                              │   │
│   │   ┌────────────────────────┐   ┌────────────────────────┐                   │   │
│   │   │     存储成本 (15-20%)   │   │     网络成本 (5-10%)    │                   │   │
│   │   │                        │   │                        │                   │   │
│   │   │  • 模型存储 (大)       │   │  • 跨区域传输          │                   │   │
│   │   │  • 检查点存储          │   │  • 推理请求流量        │                   │   │
│   │   │  • 数据集存储          │   │  • 模型分发            │                   │   │
│   │   │  • 缓存存储            │   │  • API 网关流量        │                   │   │
│   │   └────────────────────────┘   └────────────────────────┘                   │   │
│   │                                                                              │   │
│   │   ┌────────────────────────┐   ┌────────────────────────┐                   │   │
│   │   │     管理成本 (3-5%)     │   │     其他成本 (2-5%)     │                   │   │
│   │   │                        │   │                        │                   │   │
│   │   │  • 监控/可观测性       │   │  • 日志存储            │                   │   │
│   │   │  • 编排调度            │   │  • 安全审计            │                   │   │
│   │   │  • MLOps 工具          │   │  • 备份/DR             │                   │   │
│   │   └────────────────────────┘   └────────────────────────┘                   │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 成本监控架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              LLM 成本监控架构                                        │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                           数据采集层 (Collection)                            │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │   Kubecost   │   │   DCGM       │   │   云厂商     │   │   自定义     │ │   │
│   │   │   Exporter   │   │   Exporter   │   │   Billing    │   │   Metrics    │ │   │
│   │   │              │   │              │   │   API        │   │              │ │   │
│   │   │ • Pod 成本   │   │ • GPU 利用率 │   │ • 按需价格   │   │ • Token 数   │ │   │
│   │   │ • 节点成本   │   │ • 显存使用   │   │ • Spot 价格  │   │ • 请求数     │ │   │
│   │   │ • 存储成本   │   │ • 功耗       │   │ • RI/SP 信息 │   │ • 模型调用   │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                           存储层 (Storage)                                   │   │
│   │                                                                              │   │
│   │   ┌──────────────────────────────────────────────────────────────────────┐  │   │
│   │   │                         Prometheus                                    │  │   │
│   │   │   • kubecost_*          • DCGM_FI_*        • llm_*                   │  │   │
│   │   │   • node_*              • container_*      • custom_*                │  │   │
│   │   └──────────────────────────────────────────────────────────────────────┘  │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          分析层 (Analysis)                                   │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │   成本分配   │   │   趋势预测   │   │   异常检测   │   │   优化建议   │ │   │
│   │   │              │   │              │   │              │   │              │ │   │
│   │   │ • 按团队     │   │ • 日/周/月   │   │ • 成本飙升   │   │ • 资源调整   │ │   │
│   │   │ • 按项目     │   │ • 预算预测   │   │ • GPU 闲置   │   │ • 实例选择   │ │   │
│   │   │ • 按模型     │   │ • 容量规划   │   │ • 资源浪费   │   │ • 调度优化   │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          展示层 (Visualization)                              │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │   Grafana    │   │   Kubecost   │   │   自定义     │   │   告警通知   │ │   │
│   │   │   Dashboard  │   │   UI         │   │   报表       │   │   Slack/邮件 │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 成本构成详解

### 各类成本占比

| 成本类型 | 占比范围 | 主要因素 | 优化方向 |
|---------|---------|---------|---------|
| **GPU 计算** | 65-80% | 实例类型、利用率、运行时长 | Spot 实例、批处理、量化 |
| **存储** | 15-20% | 模型大小、检查点、数据集 | 分层存储、压缩、清理策略 |
| **网络** | 5-10% | 跨区域传输、API 流量 | CDN、区域优化、压缩 |
| **管理** | 3-5% | 监控、编排、MLOps | 工具整合、自动化 |
| **其他** | 2-5% | 日志、审计、备份 | 保留策略、采样 |

### GPU 实例价格对比

| GPU 类型 | 显存 | 按需价格/小时 | Spot 价格/小时 | 节省比例 | 适用场景 |
|---------|------|-------------|--------------|---------|---------|
| **NVIDIA A100 80GB** | 80GB | $32.77 | ~$10.00 | 70% | 大模型训练、多卡训练 |
| **NVIDIA A100 40GB** | 40GB | $22.00 | ~$6.50 | 70% | 中型模型训练 |
| **NVIDIA A10G** | 24GB | $1.006 | ~$0.30 | 70% | 推理、微调 |
| **NVIDIA L4** | 24GB | $0.81 | ~$0.25 | 69% | 推理优化 |
| **NVIDIA T4** | 16GB | $0.526 | ~$0.16 | 70% | 轻量推理、开发 |
| **NVIDIA V100** | 16/32GB | $3.06 | ~$0.92 | 70% | 通用训练 |
| **NVIDIA H100** | 80GB | $50.00+ | ~$15.00 | 70% | 超大模型、高性能 |

### 存储成本细分

| 存储类型 | 单价参考 | 典型用量 | 月成本估算 | 优化策略 |
|---------|---------|---------|-----------|---------|
| **模型存储** | $0.023/GB | 500GB-5TB | $12-$115 | 压缩、去重 |
| **检查点存储** | $0.023/GB | 1TB-10TB | $23-$230 | 定期清理、保留策略 |
| **数据集存储** | $0.023/GB | 1TB-50TB | $23-$1150 | 分层存储、归档 |
| **缓存存储** | $0.10/GB | 100GB-1TB | $10-$100 | TTL 策略、LRU |
| **日志存储** | $0.50/GB | 50GB-500GB | $25-$250 | 采样、压缩 |

## Kubecost 部署与配置

### 完整部署配置

```yaml
# kubecost-values.yaml
# Kubecost Helm values 配置

global:
  # 启用 GPU 成本监控
  prometheus:
    enabled: true
    nodeExporter:
      enabled: true

# Kubecost 核心配置
kubecostModel:
  # 自定义 GPU 定价
  gpuCost:
    enabled: true
    # 按 GPU 类型设置价格
    gpuTypeCosts:
      nvidia-tesla-a100: "32.77"
      nvidia-tesla-a10g: "1.006"
      nvidia-tesla-t4: "0.526"
      nvidia-tesla-v100: "3.06"
      nvidia-tesla-h100: "50.00"
      
  # 自定义定价
  customPricing:
    enabled: true
    configPath: "/var/configs/pricing.json"
    
  # 成本分配配置
  allocation:
    # 按标签分配成本
    labelConfig:
      enabled: true
      labels:
        - team
        - project
        - model
        - environment
        - cost-center
        
# Prometheus 集成
prometheus:
  server:
    retention: "30d"
    resources:
      requests:
        cpu: "500m"
        memory: "2Gi"
      limits:
        cpu: "2"
        memory: "8Gi"
        
# DCGM Exporter 集成 (GPU 监控)
dcgmExporter:
  enabled: true
  
# 网络成本
networkCosts:
  enabled: true
  # 跨区域流量成本
  zoneCost: "0.01"
  regionCost: "0.02"
  internetCost: "0.12"

# 存储配置
persistentVolume:
  enabled: true
  size: "32Gi"
  storageClass: "gp3"

# 告警配置
alerts:
  enabled: true
  # 预算告警
  budget:
    enabled: true
    
# RBAC
rbac:
  create: true
  
# ServiceAccount
serviceAccount:
  create: true
  annotations:
    # AWS IRSA
    # eks.amazonaws.com/role-arn: arn:aws:iam::ACCOUNT:role/kubecost-role
```

### 部署命令

```bash
#!/bin/bash
# deploy-kubecost.sh
# Kubecost 部署脚本

set -e

NAMESPACE="kubecost"
RELEASE_NAME="kubecost"

echo "=== 部署 Kubecost ==="

# 添加 Helm repo
helm repo add kubecost https://kubecost.github.io/cost-analyzer/
helm repo update

# 创建命名空间
kubectl create namespace $NAMESPACE --dry-run=client -o yaml | kubectl apply -f -

# 部署 Kubecost
helm upgrade --install $RELEASE_NAME kubecost/cost-analyzer \
  --namespace $NAMESPACE \
  --values kubecost-values.yaml \
  --set kubecostToken="${KUBECOST_TOKEN}" \
  --wait

# 等待 Pod 就绪
kubectl wait --for=condition=Ready pod \
  -l app=cost-analyzer \
  -n $NAMESPACE \
  --timeout=300s

echo "=== 部署完成 ==="
echo "访问: kubectl port-forward -n $NAMESPACE svc/kubecost-cost-analyzer 9090:9090"
```

## 成本标签体系

### 推荐标签规范

```yaml
# cost-labels-convention.yaml
# LLM 工作负载成本标签规范

---
# 训练任务 Pod
apiVersion: v1
kind: Pod
metadata:
  name: llama2-70b-training
  namespace: ml-training
  labels:
    # 组织标签
    team: ml-research
    department: ai-platform
    cost-center: "CC-12345"
    
    # 项目标签
    project: llama2-finetuning
    model: llama2-70b
    task-type: training
    
    # 环境标签
    environment: production
    
    # 资源标签
    gpu-type: a100-80g
    gpu-count: "8"
    
  annotations:
    # Kubecost 成本分配注解
    cost.kubernetes.io/team: "ml-research"
    cost.kubernetes.io/project: "llama2-finetuning"
    cost.kubernetes.io/environment: "production"
    
    # 任务元信息
    ml.kubernetes.io/experiment-id: "exp-20240115-001"
    ml.kubernetes.io/run-id: "run-abc123"
    
spec:
  containers:
  - name: trainer
    image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
    resources:
      limits:
        nvidia.com/gpu: 8
        memory: "640Gi"
        cpu: "64"
      requests:
        nvidia.com/gpu: 8
        memory: "512Gi"
        cpu: "32"
    env:
    - name: TRAINING_JOB_ID
      value: "job-20240115-001"

---
# 推理服务 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llama2-inference
  namespace: ml-inference
  labels:
    team: ml-platform
    project: llm-inference
    model: llama2-7b
    task-type: inference
    environment: production
    cost-center: "CC-67890"
spec:
  replicas: 3
  selector:
    matchLabels:
      app: llama2-inference
  template:
    metadata:
      labels:
        app: llama2-inference
        team: ml-platform
        project: llm-inference
        model: llama2-7b
        task-type: inference
        gpu-type: a10g
      annotations:
        cost.kubernetes.io/team: "ml-platform"
        cost.kubernetes.io/project: "llm-inference"
    spec:
      containers:
      - name: inference
        image: vllm/vllm-openai:latest
        resources:
          limits:
            nvidia.com/gpu: 1
          requests:
            nvidia.com/gpu: 1
```

### 成本标签验证策略

```yaml
# cost-label-policy.yaml
# 使用 Kyverno 强制成本标签

apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-cost-labels
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: require-team-label
      match:
        any:
          - resources:
              kinds:
                - Pod
              namespaces:
                - ml-*
                - ai-*
      validate:
        message: "ML 工作负载必须包含 team 标签"
        pattern:
          metadata:
            labels:
              team: "?*"
              
    - name: require-project-label
      match:
        any:
          - resources:
              kinds:
                - Pod
              namespaces:
                - ml-*
                - ai-*
      validate:
        message: "ML 工作负载必须包含 project 标签"
        pattern:
          metadata:
            labels:
              project: "?*"
              
    - name: require-cost-center
      match:
        any:
          - resources:
              kinds:
                - Pod
              selector:
                matchLabels:
                  task-type: training
      validate:
        message: "训练任务必须包含 cost-center 标签"
        pattern:
          metadata:
            labels:
              cost-center: "?*"
```

## 成本监控 API

### Python 成本监控客户端

```python
# llm_cost_monitor.py
# LLM 成本监控客户端

import requests
import pandas as pd
from datetime import datetime, timedelta
from typing import Dict, List, Optional
import json

class KubecostClient:
    """Kubecost API 客户端"""
    
    def __init__(self, base_url: str = "http://kubecost-cost-analyzer.kubecost:9090"):
        self.base_url = base_url
        self.session = requests.Session()
        
    def get_allocation(
        self,
        window: str = "7d",
        aggregate: str = "namespace",
        filter_labels: Optional[Dict] = None
    ) -> Dict:
        """获取成本分配数据"""
        url = f"{self.base_url}/model/allocation"
        params = {
            "window": window,
            "aggregate": aggregate,
            "accumulate": "true"
        }
        
        if filter_labels:
            filter_str = ",".join([f'{k}:"{v}"' for k, v in filter_labels.items()])
            params["filterLabels"] = filter_str
            
        response = self.session.get(url, params=params)
        response.raise_for_status()
        return response.json()
    
    def get_cost_by_label(
        self,
        label: str,
        window: str = "7d"
    ) -> Dict:
        """按标签获取成本"""
        url = f"{self.base_url}/model/allocation"
        params = {
            "window": window,
            "aggregate": f"label:{label}",
            "accumulate": "true"
        }
        response = self.session.get(url, params=params)
        response.raise_for_status()
        return response.json()
    
    def get_gpu_cost(
        self,
        namespace: Optional[str] = None,
        window: str = "7d"
    ) -> Dict:
        """获取 GPU 成本"""
        url = f"{self.base_url}/model/allocation"
        params = {
            "window": window,
            "aggregate": "namespace,label:gpu-type",
            "accumulate": "true"
        }
        
        if namespace:
            params["filterNamespaces"] = namespace
            
        response = self.session.get(url, params=params)
        response.raise_for_status()
        return response.json()


class LLMCostMonitor:
    """LLM 成本监控器"""
    
    def __init__(self, kubecost_url: str = "http://kubecost:9090"):
        self.kubecost = KubecostClient(kubecost_url)
        
    def get_training_cost(
        self,
        project: str,
        window: str = "30d"
    ) -> Dict:
        """获取训练任务成本"""
        data = self.kubecost.get_allocation(
            window=window,
            aggregate="label:project",
            filter_labels={"task-type": "training", "project": project}
        )
        
        return self._parse_allocation(data, project)
    
    def get_inference_cost(
        self,
        model: str,
        window: str = "30d"
    ) -> Dict:
        """获取推理服务成本"""
        data = self.kubecost.get_allocation(
            window=window,
            aggregate="label:model",
            filter_labels={"task-type": "inference", "model": model}
        )
        
        return self._parse_allocation(data, model)
    
    def get_team_cost_summary(
        self,
        window: str = "30d"
    ) -> pd.DataFrame:
        """获取团队成本汇总"""
        data = self.kubecost.get_cost_by_label("team", window)
        
        results = []
        for team, allocation in data.get("data", [{}])[0].items():
            if team == "__unmounted__":
                continue
            results.append({
                "Team": team,
                "CPU Cost": allocation.get("cpuCost", 0),
                "Memory Cost": allocation.get("ramCost", 0),
                "GPU Cost": allocation.get("gpuCost", 0),
                "Storage Cost": allocation.get("pvCost", 0),
                "Network Cost": allocation.get("networkCost", 0),
                "Total Cost": allocation.get("totalCost", 0)
            })
            
        df = pd.DataFrame(results)
        df = df.sort_values("Total Cost", ascending=False)
        return df
    
    def get_gpu_utilization_cost(
        self,
        namespace: str,
        window: str = "7d"
    ) -> Dict:
        """获取 GPU 利用率与成本"""
        # 获取成本数据
        cost_data = self.kubecost.get_gpu_cost(namespace, window)
        
        # 计算 GPU 效率
        gpu_cost = sum([
            alloc.get("gpuCost", 0) 
            for alloc in cost_data.get("data", [{}])[0].values()
            if isinstance(alloc, dict)
        ])
        
        return {
            "namespace": namespace,
            "window": window,
            "gpu_cost": gpu_cost,
            "estimated_waste": gpu_cost * 0.3  # 假设 30% 闲置
        }
    
    def _parse_allocation(self, data: Dict, key: str) -> Dict:
        """解析分配数据"""
        allocations = data.get("data", [{}])[0]
        allocation = allocations.get(key, {})
        
        return {
            "cpu_cost": allocation.get("cpuCost", 0),
            "memory_cost": allocation.get("ramCost", 0),
            "gpu_cost": allocation.get("gpuCost", 0),
            "storage_cost": allocation.get("pvCost", 0),
            "network_cost": allocation.get("networkCost", 0),
            "total_cost": allocation.get("totalCost", 0),
            "gpu_hours": allocation.get("gpuHours", 0),
            "cpu_hours": allocation.get("cpuCoreHours", 0)
        }


class BudgetManager:
    """预算管理器"""
    
    def __init__(
        self,
        monthly_budget: float,
        alert_thresholds: List[float] = [0.5, 0.75, 0.9, 1.0]
    ):
        self.monthly_budget = monthly_budget
        self.daily_budget = monthly_budget / 30
        self.alert_thresholds = alert_thresholds
        
    def check_budget_status(
        self,
        current_spend: float,
        days_elapsed: int
    ) -> Dict:
        """检查预算状态"""
        # 计算预计月支出
        daily_avg = current_spend / max(days_elapsed, 1)
        projected_monthly = daily_avg * 30
        
        # 计算预算使用率
        budget_used = current_spend / self.monthly_budget
        projected_usage = projected_monthly / self.monthly_budget
        
        # 确定状态
        if projected_usage > 1.1:
            status = "CRITICAL"
            message = f"预计超支 {(projected_usage - 1) * 100:.1f}%"
        elif projected_usage > 1.0:
            status = "WARNING"
            message = "预计超出预算"
        elif projected_usage > 0.9:
            status = "CAUTION"
            message = "接近预算上限"
        else:
            status = "OK"
            message = "预算使用正常"
            
        return {
            "status": status,
            "message": message,
            "current_spend": current_spend,
            "monthly_budget": self.monthly_budget,
            "budget_used_percent": budget_used * 100,
            "projected_monthly": projected_monthly,
            "projected_usage_percent": projected_usage * 100,
            "remaining_budget": self.monthly_budget - current_spend,
            "daily_budget_remaining": (self.monthly_budget - current_spend) / max(30 - days_elapsed, 1)
        }
    
    def get_alerts(
        self,
        current_spend: float
    ) -> List[Dict]:
        """获取预算告警"""
        alerts = []
        budget_used = current_spend / self.monthly_budget
        
        for threshold in self.alert_thresholds:
            if budget_used >= threshold:
                alerts.append({
                    "threshold": threshold * 100,
                    "current": budget_used * 100,
                    "severity": "critical" if threshold >= 1.0 else "warning" if threshold >= 0.9 else "info"
                })
                
        return alerts


# 使用示例
if __name__ == "__main__":
    # 初始化监控器
    monitor = LLMCostMonitor("http://kubecost:9090")
    
    # 获取团队成本汇总
    team_costs = monitor.get_team_cost_summary(window="30d")
    print("=== 团队成本汇总 ===")
    print(team_costs.to_markdown())
    
    # 获取训练成本
    training_cost = monitor.get_training_cost("llama2-finetuning", "30d")
    print(f"\n=== 训练成本 ===")
    print(f"GPU 成本: ${training_cost['gpu_cost']:.2f}")
    print(f"总成本: ${training_cost['total_cost']:.2f}")
    
    # 预算检查
    budget = BudgetManager(monthly_budget=50000)
    status = budget.check_budget_status(current_spend=25000, days_elapsed=15)
    print(f"\n=== 预算状态 ===")
    print(f"状态: {status['status']}")
    print(f"已使用: {status['budget_used_percent']:.1f}%")
    print(f"预计月支出: ${status['projected_monthly']:.2f}")
```

## 成本告警规则

### Prometheus 告警规则

```yaml
# llm-cost-alerting-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-cost-alerts
  namespace: monitoring
  labels:
    prometheus: k8s
    role: alert-rules
spec:
  groups:
    # =================================================================
    # 成本阈值告警
    # =================================================================
    - name: llm.cost.thresholds
      interval: 5m
      rules:
        - alert: LLMDailyCostHigh
          expr: |
            sum(increase(kubecost_cluster_cost_total[24h])) > 1000
          for: 30m
          labels:
            severity: warning
            team: finops
          annotations:
            summary: "LLM 日成本超过 $1000"
            description: |
              过去 24 小时 LLM 工作负载成本: ${{ $value | printf "%.2f" }}
              阈值: $1000
            runbook_url: "https://wiki/runbooks/llm-cost-high"
            
        - alert: LLMDailyCostCritical
          expr: |
            sum(increase(kubecost_cluster_cost_total[24h])) > 5000
          for: 15m
          labels:
            severity: critical
            team: finops
          annotations:
            summary: "LLM 日成本严重超标 (>$5000)"
            description: |
              过去 24 小时 LLM 工作负载成本: ${{ $value | printf "%.2f" }}
              需要立即检查和优化
              
        - alert: LLMCostSpike
          expr: |
            (
              sum(increase(kubecost_namespace_cost_total{namespace=~"ml-.*|ai-.*"}[1h]))
              /
              avg_over_time(sum(increase(kubecost_namespace_cost_total{namespace=~"ml-.*|ai-.*"}[1h]))[24h:1h])
            ) > 2
          for: 15m
          labels:
            severity: warning
          annotations:
            summary: "LLM 成本突然激增 200%"
            description: |
              当前小时成本相比 24 小时平均值增长 {{ $value | printf "%.1f" }}x
              请检查是否有异常任务运行
              
    # =================================================================
    # GPU 利用率成本告警
    # =================================================================
    - name: llm.gpu.efficiency
      interval: 1m
      rules:
        - alert: GPUIdleHighCost
          expr: |
            (
              avg(DCGM_FI_DEV_GPU_UTIL{namespace=~"ml-.*|ai-.*"}) < 30
            ) and (
              sum(kubecost_pod_gpu_cost{namespace=~"ml-.*|ai-.*"}) > 0
            )
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "GPU 利用率低但成本高"
            description: |
              GPU 利用率: {{ $value | printf "%.1f" }}%
              GPU 处于低利用率状态超过 1 小时,造成成本浪费
              建议:
              - 检查任务是否卡住
              - 考虑使用更小的 GPU 实例
              - 启用自动缩容
              
        - alert: GPUMemoryUnderutilized
          expr: |
            (
              avg(DCGM_FI_DEV_FB_USED{namespace=~"ml-.*"}) 
              / 
              avg(DCGM_FI_DEV_FB_TOTAL{namespace=~"ml-.*"})
            ) < 0.5
          for: 2h
          labels:
            severity: info
          annotations:
            summary: "GPU 显存利用率低"
            description: |
              GPU 显存利用率: {{ $value | printf "%.1f" }}%
              建议考虑使用更小显存的 GPU 实例以节省成本
              
    # =================================================================
    # 预算告警
    # =================================================================
    - name: llm.budget
      interval: 15m
      rules:
        - alert: MonthlyBudget75Percent
          expr: |
            (
              sum(kubecost_cluster_cost_total)
              /
              kubecost_budget_monthly_total
            ) > 0.75
          labels:
            severity: warning
          annotations:
            summary: "月度预算已使用 75%"
            description: "当前月度支出已达预算的 {{ $value | printf \"%.1f\" }}%"
            
        - alert: MonthlyBudget90Percent
          expr: |
            (
              sum(kubecost_cluster_cost_total)
              /
              kubecost_budget_monthly_total
            ) > 0.90
          labels:
            severity: critical
          annotations:
            summary: "月度预算已使用 90%"
            description: "当前月度支出已达预算的 {{ $value | printf \"%.1f\" }}%,需要立即控制支出"
            
        - alert: ProjectedOverBudget
          expr: |
            (
              predict_linear(kubecost_cluster_cost_total[7d], 30*24*3600)
              >
              kubecost_budget_monthly_total * 1.1
            )
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "预计月度支出将超预算 10%"
            description: |
              基于过去 7 天趋势,预计月度支出将达 ${{ $value | printf "%.2f" }}
              
    # =================================================================
    # 资源浪费告警
    # =================================================================
    - name: llm.waste
      interval: 30m
      rules:
        - alert: UnusedGPUNode
          expr: |
            (
              count(kube_node_status_condition{condition="Ready",status="true"} 
                * on(node) kube_node_labels{label_node_kubernetes_io_instance_type=~".*gpu.*"})
              -
              count(kube_pod_info{pod=~".*gpu.*"} * on(node) kube_node_labels{label_node_kubernetes_io_instance_type=~".*gpu.*"})
            ) > 0
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "存在闲置 GPU 节点"
            description: "有 {{ $value }} 个 GPU 节点没有运行任何 GPU Pod,建议缩容"
            
        - alert: OverprovisionedInference
          expr: |
            (
              sum(kube_deployment_spec_replicas{deployment=~".*inference.*"})
              /
              sum(kube_deployment_status_replicas_available{deployment=~".*inference.*"})
            ) > 1.5
          for: 1h
          labels:
            severity: info
          annotations:
            summary: "推理服务可能过度配置"
            description: "推理服务副本数可能过多,请检查 HPA 配置"
```

## 成本优化策略

### 训练成本优化矩阵

| 优化策略 | 实现方式 | 预期节省 | 复杂度 | 适用场景 |
|---------|---------|---------|-------|---------|
| **Spot/抢占实例** | Karpenter 配置优先 Spot | 60-70% | 低 | 可中断训练 |
| **混合精度训练** | FP16/BF16 + 动态损失缩放 | 30-40% | 低 | 大多数模型 |
| **梯度累积** | 小批量 + 累积更新 | 20-30% | 低 | 显存受限场景 |
| **梯度检查点** | 时间换显存 | 20-30% | 中 | 大模型训练 |
| **Early Stopping** | 验证集监控 | 15-25% | 低 | 过拟合检测 |
| **模型并行** | Tensor/Pipeline 并行 | - | 高 | 超大模型 |
| **数据并行优化** | FSDP/DeepSpeed | 20-30% | 中 | 多卡训练 |

### 推理成本优化矩阵

| 优化策略 | 实现方式 | 预期节省 | 复杂度 | 适用场景 |
|---------|---------|---------|-------|---------|
| **动态批处理** | vLLM continuous batching | 60-80% | 中 | 高并发推理 |
| **模型量化** | INT8/INT4 量化 | 50-75% | 中 | 推理部署 |
| **KV Cache 优化** | PagedAttention | 30-50% | 低 | 长序列生成 |
| **推测解码** | Speculative Decoding | 20-40% | 高 | 延迟敏感场景 |
| **自动扩缩容** | HPA/KEDA | 30-50% | 低 | 负载波动大 |
| **请求缓存** | Semantic Cache | 20-40% | 中 | 重复请求多 |
| **模型蒸馏** | 小模型替代 | 60-80% | 高 | 特定任务 |

### 成本优化配置示例

```yaml
# cost-optimized-training.yaml
# 成本优化的训练任务配置

apiVersion: batch/v1
kind: Job
metadata:
  name: llama2-finetune-optimized
  namespace: ml-training
  labels:
    team: ml-research
    project: llama2-finetuning
    cost-optimization: enabled
spec:
  backoffLimit: 3
  template:
    metadata:
      labels:
        team: ml-research
        project: llama2-finetuning
        spot-tolerant: "true"
    spec:
      # 优先使用 Spot 实例
      nodeSelector:
        node.kubernetes.io/instance-type: p4d.24xlarge
      tolerations:
        - key: "spot"
          operator: "Equal"
          value: "true"
          effect: "NoSchedule"
        - key: "nvidia.com/gpu"
          operator: "Exists"
          effect: "NoSchedule"
          
      # 检查点存储 (支持中断恢复)
      volumes:
        - name: checkpoint
          persistentVolumeClaim:
            claimName: training-checkpoint-pvc
        - name: dataset
          persistentVolumeClaim:
            claimName: training-dataset-pvc
            
      containers:
        - name: trainer
          image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
          command:
            - python
            - -m
            - torch.distributed.run
            - --nproc_per_node=8
            - train.py
            - --mixed_precision=bf16        # 混合精度
            - --gradient_accumulation=4     # 梯度累积
            - --gradient_checkpointing=true # 梯度检查点
            - --checkpoint_dir=/checkpoints
            - --resume_from_checkpoint=auto # 自动恢复
          env:
            - name: PYTORCH_CUDA_ALLOC_CONF
              value: "max_split_size_mb:512"
          resources:
            limits:
              nvidia.com/gpu: 8
              memory: "640Gi"
              cpu: "96"
            requests:
              nvidia.com/gpu: 8
              memory: "512Gi"
              cpu: "64"
          volumeMounts:
            - name: checkpoint
              mountPath: /checkpoints
            - name: dataset
              mountPath: /data
              
      restartPolicy: OnFailure
      terminationGracePeriodSeconds: 300

---
# cost-optimized-inference.yaml
# 成本优化的推理服务配置

apiVersion: apps/v1
kind: Deployment
metadata:
  name: llama2-inference-optimized
  namespace: ml-inference
spec:
  replicas: 2
  selector:
    matchLabels:
      app: llama2-inference
  template:
    metadata:
      labels:
        app: llama2-inference
        team: ml-platform
        cost-optimization: enabled
    spec:
      nodeSelector:
        node.kubernetes.io/instance-type: g5.2xlarge  # 使用更小的 GPU
      containers:
        - name: vllm
          image: vllm/vllm-openai:latest
          args:
            - --model=/models/llama2-7b-chat
            - --tensor-parallel-size=1
            - --dtype=float16
            - --quantization=awq              # 启用量化
            - --max-model-len=4096
            - --gpu-memory-utilization=0.9
            - --enable-prefix-caching         # 启用前缀缓存
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: "32Gi"
            requests:
              nvidia.com/gpu: 1
              memory: "24Gi"
          ports:
            - containerPort: 8000

---
# HPA 配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: llama2-inference-hpa
  namespace: ml-inference
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: llama2-inference-optimized
  minReplicas: 1
  maxReplicas: 10
  metrics:
    - type: Pods
      pods:
        metric:
          name: vllm_requests_running
        target:
          type: AverageValue
          averageValue: "50"
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
        - type: Percent
          value: 100
          periodSeconds: 15
```

## 成本报告生成

### 自动化报告脚本

```python
# generate_cost_report.py
# 成本报告生成脚本

import pandas as pd
from datetime import datetime, timedelta
from typing import Dict, List
import matplotlib.pyplot as plt
import io
import base64

class CostReportGenerator:
    """成本报告生成器"""
    
    def __init__(self, kubecost_client, output_dir: str = "/reports"):
        self.kubecost = kubecost_client
        self.output_dir = output_dir
        
    def generate_monthly_report(self, year: int, month: int) -> str:
        """生成月度成本报告"""
        
        # 获取数据
        team_costs = self._get_team_costs(f"{year}-{month:02d}")
        project_costs = self._get_project_costs(f"{year}-{month:02d}")
        gpu_costs = self._get_gpu_costs(f"{year}-{month:02d}")
        trends = self._get_cost_trends(f"{year}-{month:02d}")
        
        # 生成报告
        report = f"""
# LLM 成本月度报告

## 报告信息
- **报告期间**: {year}年{month}月
- **生成时间**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

## 执行摘要

| 指标 | 本月值 | 环比变化 | 状态 |
|-----|-------|---------|-----|
| 总成本 | ${team_costs['total']:.2f} | {trends['total_change']:.1f}% | {'⚠️' if trends['total_change'] > 20 else '✅'} |
| GPU 成本 | ${gpu_costs['total']:.2f} | {trends['gpu_change']:.1f}% | {'⚠️' if trends['gpu_change'] > 25 else '✅'} |
| 平均 GPU 利用率 | {gpu_costs['avg_utilization']:.1f}% | - | {'⚠️' if gpu_costs['avg_utilization'] < 50 else '✅'} |

## 团队成本分布

{self._format_team_table(team_costs['by_team'])}

## 项目成本 Top 10

{self._format_project_table(project_costs['top_10'])}

## GPU 使用分析

### 按 GPU 类型
{self._format_gpu_table(gpu_costs['by_type'])}

### GPU 利用率分布
- 高利用率 (>70%): {gpu_costs['high_util_percent']:.1f}%
- 中等利用率 (30-70%): {gpu_costs['medium_util_percent']:.1f}%
- 低利用率 (<30%): {gpu_costs['low_util_percent']:.1f}%

## 优化建议

{self._generate_recommendations(team_costs, gpu_costs)}

## 下月预测

基于当前趋势,预计下月成本: **${trends['next_month_forecast']:.2f}**

---
*报告由 LLM 成本监控系统自动生成*
"""
        
        return report
    
    def _format_team_table(self, data: List[Dict]) -> str:
        """格式化团队成本表格"""
        header = "| 团队 | 总成本 | GPU成本 | 占比 |\n|-----|-------|--------|-----|"
        rows = []
        total = sum(d['total'] for d in data)
        for d in data:
            pct = d['total'] / total * 100 if total > 0 else 0
            rows.append(f"| {d['team']} | ${d['total']:.2f} | ${d['gpu']:.2f} | {pct:.1f}% |")
        return header + "\n" + "\n".join(rows)
    
    def _format_project_table(self, data: List[Dict]) -> str:
        """格式化项目成本表格"""
        header = "| 项目 | 成本 | GPU小时 | 效率 |\n|-----|------|--------|-----|"
        rows = []
        for d in data:
            rows.append(f"| {d['project']} | ${d['cost']:.2f} | {d['gpu_hours']:.1f} | {d['efficiency']:.1f}% |")
        return header + "\n" + "\n".join(rows)
    
    def _format_gpu_table(self, data: List[Dict]) -> str:
        """格式化 GPU 成本表格"""
        header = "| GPU类型 | 成本 | 使用时长 | 平均利用率 |\n|--------|------|---------|----------|"
        rows = []
        for d in data:
            rows.append(f"| {d['type']} | ${d['cost']:.2f} | {d['hours']:.1f}h | {d['util']:.1f}% |")
        return header + "\n" + "\n".join(rows)
    
    def _generate_recommendations(self, team_costs: Dict, gpu_costs: Dict) -> str:
        """生成优化建议"""
        recommendations = []
        
        if gpu_costs['avg_utilization'] < 50:
            recommendations.append(
                "1. **提高 GPU 利用率**: 当前平均利用率仅 {:.1f}%,建议:\n"
                "   - 启用动态批处理\n"
                "   - 使用更小的 GPU 实例\n"
                "   - 配置自动缩容".format(gpu_costs['avg_utilization'])
            )
            
        if gpu_costs['low_util_percent'] > 30:
            recommendations.append(
                "2. **减少低效 GPU 使用**: {:.1f}% 的 GPU 时间处于低利用率状态,建议:\n"
                "   - 审查长时间运行的任务\n"
                "   - 设置 GPU 空闲超时".format(gpu_costs['low_util_percent'])
            )
            
        return "\n\n".join(recommendations) if recommendations else "当前成本使用效率良好,无特别优化建议。"
    
    def _get_team_costs(self, period: str) -> Dict:
        # 模拟数据获取
        return {
            'total': 45000,
            'by_team': [
                {'team': 'ml-research', 'total': 25000, 'gpu': 20000},
                {'team': 'ml-platform', 'total': 15000, 'gpu': 10000},
                {'team': 'data-science', 'total': 5000, 'gpu': 3000},
            ]
        }
    
    def _get_project_costs(self, period: str) -> Dict:
        return {
            'top_10': [
                {'project': 'llama2-finetuning', 'cost': 15000, 'gpu_hours': 500, 'efficiency': 75},
                {'project': 'gpt-inference', 'cost': 10000, 'gpu_hours': 1000, 'efficiency': 85},
            ]
        }
    
    def _get_gpu_costs(self, period: str) -> Dict:
        return {
            'total': 35000,
            'avg_utilization': 62,
            'high_util_percent': 45,
            'medium_util_percent': 35,
            'low_util_percent': 20,
            'by_type': [
                {'type': 'A100-80G', 'cost': 25000, 'hours': 800, 'util': 70},
                {'type': 'A10G', 'cost': 8000, 'hours': 2000, 'util': 55},
                {'type': 'T4', 'cost': 2000, 'hours': 1500, 'util': 45},
            ]
        }
    
    def _get_cost_trends(self, period: str) -> Dict:
        return {
            'total_change': 15.5,
            'gpu_change': 18.2,
            'next_month_forecast': 52000
        }
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **Kubecost 2.0** | 新增 GPU 成本追踪 | 更精确的 LLM 成本分析 |
| **OpenCost 1.0** | CNCF 毕业项目 | 开源替代方案 |
| **v1.28** | 原生 GPU 监控增强 | 更好的 Device Plugin 支持 |

## 最佳实践总结

### 成本监控检查清单

- [ ] 部署 Kubecost 或 OpenCost
- [ ] 配置 DCGM Exporter 监控 GPU
- [ ] 建立成本标签规范并强制执行
- [ ] 配置成本告警规则
- [ ] 设置团队/项目预算
- [ ] 定期生成成本报告
- [ ] 持续优化高成本工作负载

### 关键监控指标

- `kubecost_cluster_cost_total` - 集群总成本
- `kubecost_pod_gpu_cost` - Pod GPU 成本
- `DCGM_FI_DEV_GPU_UTIL` - GPU 利用率
- `DCGM_FI_DEV_FB_USED` - GPU 显存使用

---

**参考资料**:
- [Kubecost 文档](https://docs.kubecost.com/)
- [OpenCost 项目](https://www.opencost.io/)
- [FinOps 基金会](https://www.finops.org/)
