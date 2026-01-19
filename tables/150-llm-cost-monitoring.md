# 63 - LLM成本监控与FinOps

> **适用版本**: v1.25 - v1.32 | **参考**: [Kubecost](https://www.kubecost.com/)

## 一、成本构成

| 类型 | 占比 | 主要因素 | 优化方向 |
|-----|------|---------|---------|
| **GPU计算** | 65% | 实例类型、利用率 | Spot实例、批处理 |
| **存储** | 20% | 模型、数据集大小 | 分层存储、压缩 |
| **网络** | 10% | 跨区域传输 | CDN、区域优化 |
| **其他** | 5% | 日志、监控 | 保留策略 |

## 二、GPU成本对比

| GPU | 价格/小时 | Spot价格 | 节省 | 场景 |
|-----|----------|---------|------|------|
| A100 80GB | $32.77 | ~$10 | 70% | 训练 |
| A10G 24GB | $1.006 | ~$0.30 | 70% | 推理 |
| T4 16GB | $0.526 | ~$0.16 | 70% | 轻量推理 |

## 三、Kubecost部署

```bash
helm install kubecost kubecost/cost-analyzer \
  --namespace kubecost \
  --create-namespace \
  --set kubecostToken="your-token"

kubectl port-forward -n kubecost svc/kubecost-cost-analyzer 9090:9090
```

## 四、成本标签

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: llm-training-pod
  labels:
    team: ml-research
    project: llama2-finetuning
    cost-center: "R&D"
  annotations:
    cost.kubernetes.io/team: "ml-research"
    cost.kubernetes.io/project: "llama2"
spec:
  containers:
  - name: trainer
    resources:
      limits:
        nvidia.com/gpu: 4
```

## 五、成本监控API

```python
import requests

class KubecostAPI:
    def __init__(self, url="http://kubecost:9090"):
        self.base_url = url
    
    def get_allocation(self, window="7d"):
        url = f"{self.base_url}/model/allocation"
        params = {"window": window, "aggregate": "namespace"}
        response = requests.get(url, params=params)
        return response.json()
    
    def get_cost_by_label(self, label="team"):
        url = f"{self.base_url}/model/allocation"
        params = {"aggregate": f"label:{label}"}
        response = requests.get(url, params=params)
        return response.json()

# 使用
kubecost = KubecostAPI()
costs = kubecost.get_allocation(window="30d")
print(f"Monthly cost: ${costs['total_cost']:.2f}")
```

## 六、成本告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-alerts
spec:
  groups:
  - name: cost_management
    rules:
    - alert: HighDailyCost
      expr: sum(increase(kubecost_cluster_cost_total[24h])) > 1000
      annotations:
        summary: "日成本超过$1000"
    
    - alert: GPUIdleWaste
      expr: avg(DCGM_FI_DEV_GPU_UTIL) < 30 and kubecost_pod_gpu_cost > 0
      for: 1h
      annotations:
        summary: "GPU利用率<30%持续1小时"
    
    - alert: CostSpike
      expr: |
        (sum(increase(kubecost_namespace_cost_total[1h]))
        / avg_over_time(sum(increase(kubecost_namespace_cost_total[1h]))[24h:1h]))
        > 2
      annotations:
        summary: "成本激增200%"
```

## 七、优化策略

### 训练成本优化
| 策略 | 效果 | 实现 |
|-----|------|------|
| Spot实例 | 节省70% | Karpenter配置 |
| 混合精度 | 节省40% | FP16训练 |
| Early Stopping | 节省25% | 验证集监控 |
| 梯度累积 | 节省30% | 使用小GPU |

### 推理成本优化
| 策略 | 效果 | 实现 |
|-----|------|------|
| 批处理 | 节省80% | vLLM配置 |
| 量化 | 节省75% | INT8/INT4 |
| 缓存 | 节省30% | Redis |
| 自动扩缩容 | 节省50% | HPA |

## 八、预算管理

```python
class BudgetManager:
    def __init__(self, monthly_budget):
        self.monthly_budget = monthly_budget
        self.daily_budget = monthly_budget / 30
    
    def check_status(self, current_spend, days_elapsed):
        projected = (current_spend / days_elapsed) * 30
        
        if projected > self.monthly_budget * 1.1:
            return "CRITICAL", "预计超支10%"
        elif projected > self.monthly_budget:
            return "WARNING", "预计超支"
        else:
            return "OK", "预算健康"

# 使用
budget = BudgetManager(monthly_budget=50000)
status, msg = budget.check_status(current_spend=25000, days_elapsed=15)
print(f"Status: {status}, {msg}")
```

## 九、成本报告

```python
def generate_monthly_report(kubecost_api):
    # 按团队聚合
    team_costs = kubecost_api.get_cost_by_label("team")
    
    report = []
    for team, cost in team_costs.items():
        report.append({
            "Team": team,
            "Cost": f"${cost:.2f}",
            "Percentage": f"{cost/total_cost*100:.1f}%"
        })
    
    return pd.DataFrame(report)

report = generate_monthly_report(kubecost)
print(report.to_markdown())
```

## 十、ROI计算

**优化前后对比:**
| 项目 | 优化前 | 优化后 | 节省 |
|-----|--------|--------|------|
| Spot实例 | $180K | $54K | 70% |
| GPU利用率 | $120K | $72K | 40% |
| 存储分层 | $48K | $19K | 60% |
| 推理优化 | $78K | $27K | 65% |
| **总计** | **$426K** | **$172K** | **60%** |

---
**相关**: [120-AI成本FinOps](../120-ai-cost-analysis-finops.md) | **版本**: Kubecost 1.106+
