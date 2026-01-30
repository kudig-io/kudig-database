# 64 - LLM模型版本管理

> **适用版本**: v1.25 - v1.32 | **参考**: [MLflow](https://mlflow.org/)

## 一、版本管理系统

| 系统 | 特性 | 集成 | 适用场景 |
|-----|------|------|---------|
| **MLflow** | 实验追踪、模型注册 | Python SDK | 通用ML |
| **DVC** | Git-like版本控制 | CLI/Python | 数据+模型 |
| **Weights & Biases** | 可视化、协作 | Python SDK | 研究团队 |
| **HuggingFace Hub** | 模型托管 | transformers | HF模型 |

## 二、MLflow部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mlflow-postgres
spec:
  serviceName: mlflow-postgres
  replicas: 1
  template:
    spec:
      containers:
      - name: postgres
        image: postgres:15
        env:
        - name: POSTGRES_DB
          value: mlflow
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: mlflow
        image: ghcr.io/mlflow/mlflow:v2.9.2
        command:
        - mlflow
        - server
        - --host=0.0.0.0
        - --backend-store-uri=postgresql://mlflow@postgres/mlflow
        - --default-artifact-root=s3://mlflow-artifacts/
        ports:
        - containerPort: 5000
```

## 三、模型注册

```python
import mlflow
from transformers import AutoModelForCausalLM

# 设置MLflow URI
mlflow.set_tracking_uri("http://mlflow-server:5000")
mlflow.set_experiment("llama2-finetuning")

with mlflow.start_run(run_name="llama2-7b-alpaca"):
    # 记录参数
    mlflow.log_params({
        "model": "llama-2-7b",
        "learning_rate": 3e-4,
        "lora_r": 8
    })
    
    # 训练
    model = train_model()
    
    # 记录指标
    mlflow.log_metrics({
        "val_loss": 0.45,
        "val_accuracy": 0.92
    })
    
    # 注册模型
    mlflow.pytorch.log_model(
        model,
        "model",
        registered_model_name="llama2-7b-alpaca"
    )
```

## 四、版本管理

```python
from mlflow.tracking import MlflowClient

client = MlflowClient("http://mlflow-server:5000")

# 获取模型版本
versions = client.search_model_versions("name='llama2-7b-alpaca'")

# 版本晋级
client.transition_model_version_stage(
    name="llama2-7b-alpaca",
    version=3,
    stage="Production"
)

# 版本对比
version_1 = client.get_model_version("llama2-7b-alpaca", 1)
version_3 = client.get_model_version("llama2-7b-alpaca", 3)

print(f"v1 accuracy: {version_1.run_data.metrics['accuracy']}")
print(f"v3 accuracy: {version_3.run_data.metrics['accuracy']}")
```

## 五、模型元数据

```yaml
model_card:
  name: "llama2-7b-alpaca-v3"
  version: "3.0.0"
  date: "2024-01-15"
  
  description: "Llama2-7B微调模型，使用Alpaca指令数据集"
  
  training:
    dataset: "tatsu-lab/alpaca"
    samples: 52000
    epochs: 3
    learning_rate: 3e-4
    
  performance:
    accuracy: 0.92
    val_loss: 0.45
    
  resources:
    gpu: "1×A10G"
    training_time: "6 hours"
    cost: "$36"
    
  limitations:
    - "仅支持英文"
    - "上下文长度2048"
```

## 六、A/B测试

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama2-ab-test
spec:
  predictor:
    canaryTrafficPercent: 20  # 20%流量到v3
    containers:
    - name: model-v2
      image: model-registry/llama2-7b-alpaca:v2
    canary:
      containers:
      - name: model-v3
        image: model-registry/llama2-7b-alpaca:v3
---
# 监控A/B测试
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ab-test-metrics
spec:
  groups:
  - name: ab_test
    rules:
    - record: model_accuracy_by_version
      expr: |
        sum(rate(model_correct_predictions[5m])) by (version)
        / sum(rate(model_total_predictions[5m])) by (version)
```

## 七、回滚策略

```python
class ModelRollback:
    def __init__(self, mlflow_client):
        self.client = mlflow_client
    
    def rollback_to_version(self, model_name, target_version):
        """回滚到指定版本"""
        # 获取当前生产版本
        current = self.client.get_latest_versions(
            model_name, 
            stages=["Production"]
        )[0]
        
        # 降级当前版本
        self.client.transition_model_version_stage(
            name=model_name,
            version=current.version,
            stage="Archived"
        )
        
        # 晋升目标版本
        self.client.transition_model_version_stage(
            name=model_name,
            version=target_version,
            stage="Production"
        )
        
        print(f"Rolled back from v{current.version} to v{target_version}")
    
    def auto_rollback_if_degraded(self, model_name, threshold=0.05):
        """指标下降时自动回滚"""
        current = self.client.get_latest_versions(model_name, ["Production"])[0]
        previous = self.client.get_model_version(model_name, current.version - 1)
        
        current_acc = current.run_data.metrics.get("accuracy", 0)
        previous_acc = previous.run_data.metrics.get("accuracy", 0)
        
        if current_acc < previous_acc - threshold:
            self.rollback_to_version(model_name, previous.version)
            return True
        
        return False

# 使用
rollback = ModelRollback(client)
if rollback.auto_rollback_if_degraded("llama2-7b-alpaca", threshold=0.05):
    print("Auto rollback triggered!")
```

## 八、版本生命周期

```
Staging → Production → Archived
   ↑          ↓
   └──────回滚────────┘

生命周期规则:
- Staging: 新版本测试，保留30天
- Production: 当前服务版本，保留90天
- Archived: 历史版本，保留365天后删除
```

## 九、最佳实践

1. **版本命名**: 使用语义化版本 (major.minor.patch)
2. **实验命名**: 包含日期和关键参数
3. **指标记录**: 记录训练/验证/测试指标
4. **Artifact保存**: 模型、配置、checkpoint全部保存
5. **元数据**: 记录Git commit、数据集版本、训练环境

## 十、成本优化

**存储策略:**
- 活跃版本: S3 Standard
- 归档版本: S3 Glacier (节省80%)
- 删除策略: 1年后删除Archived版本

**示例成本 (100个模型版本, 每个7GB):**
- Standard: 700GB × $0.023 = $16/月
- Glacier: 700GB × $0.004 = $2.8/月
- 节省: 82%

---
**相关**: [113-AI模型注册中心](../113-ai-model-registry.md) | **版本**: MLflow 2.9+
