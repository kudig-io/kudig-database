# AI实验管理与MLOps平台

## 一、实验管理平台架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        MLOps实验管理全景                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │  实验追踪     │───▶│  超参优化     │───▶│  模型评估     │              │
│  │  MLflow      │    │  Optuna      │    │  Metrics     │              │
│  │  W&B         │    │  Ray Tune    │    │  Validation  │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│       ▲                    ▲                    │                       │
│       │                    │                    ▼                       │
│  ┌────┴──────┐       ┌────┴──────┐       ┌────────────┐               │
│  │  代码版本  │       │  数据版本  │       │  模型注册   │               │
│  │  Git      │       │  DVC      │       │  Registry  │               │
│  └───────────┘       └───────────┘       └────────────┘               │
│                                                │                        │
│                                                ▼                        │
│  ┌──────────────────────────────────────────────────┐                  │
│  │              CI/CD Pipeline                       │                  │
│  │  训练 → 评估 → 注册 → 部署 → 监控                  │                  │
│  └──────────────────────────────────────────────────┘                  │
│                                                                           │
│  ┌──────────────────────────────────────────────────┐                  │
│  │         Kubernetes基础设施                         │                  │
│  │  • GPU调度  • 分布式训练  • 模型Serving           │                  │
│  └──────────────────────────────────────────────────┘                  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 二、MLflow实验追踪

### 2.1 MLflow完整部署

```yaml
# PostgreSQL后端存储
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mlflow-postgres
  namespace: ai-platform
spec:
  serviceName: mlflow-postgres
  replicas: 1
  selector:
    matchLabels:
      app: mlflow-postgres
  template:
    metadata:
      labels:
        app: mlflow-postgres
    spec:
      containers:
      - name: postgres
        image: postgres:15-alpine
        ports:
        - containerPort: 5432
          name: postgres
        env:
        - name: POSTGRES_DB
          value: mlflow
        - name: POSTGRES_USER
          value: mlflow
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mlflow-postgres-secret
              key: password
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
---
# MLflow Tracking Server
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
  namespace: ai-platform
spec:
  replicas: 3
  selector:
    matchLabels:
      app: mlflow-server
  template:
    metadata:
      labels:
        app: mlflow-server
    spec:
      containers:
      - name: mlflow
        image: ghcr.io/mlflow/mlflow:v2.9.2
        command:
        - mlflow
        - server
        - --host=0.0.0.0
        - --port=5000
        - --backend-store-uri=postgresql://mlflow:$(POSTGRES_PASSWORD)@mlflow-postgres:5432/mlflow
        - --default-artifact-root=s3://mlflow-artifacts/
        - --serve-artifacts
        - --gunicorn-opts=--workers 4 --timeout 120
        
        ports:
        - containerPort: 5000
          name: http
        
        env:
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mlflow-postgres-secret
              key: password
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: access-key-id
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: secret-access-key
        - name: MLFLOW_S3_ENDPOINT_URL
          value: "https://s3.amazonaws.com"
        
        livenessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 30
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 10
          periodSeconds: 10
        
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
---
apiVersion: v1
kind: Service
metadata:
  name: mlflow-server
  namespace: ai-platform
spec:
  type: ClusterIP
  ports:
  - port: 5000
    targetPort: 5000
    protocol: TCP
    name: http
  selector:
    app: mlflow-server
---
# Ingress暴露外部访问
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mlflow-ingress
  namespace: ai-platform
  annotations:
    nginx.ingress.kubernetes.io/proxy-body-size: "500m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "600"
spec:
  ingressClassName: nginx
  rules:
  - host: mlflow.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: mlflow-server
            port:
              number: 5000
  tls:
  - hosts:
    - mlflow.example.com
    secretName: mlflow-tls-secret
```

### 2.2 训练脚本集成MLflow

```python
import mlflow
import mlflow.pytorch
import torch
import torch.nn as nn
from torch.utils.data import DataLoader
from transformers import AutoModelForSequenceClassification, AutoTokenizer, AdamW
from datasets import load_dataset
import os

# 设置MLflow追踪URI
mlflow.set_tracking_uri("http://mlflow-server.ai-platform.svc.cluster.local:5000")
mlflow.set_experiment("bert-sentiment-classification")

# 超参数
params = {
    "model_name": "bert-base-uncased",
    "learning_rate": 2e-5,
    "batch_size": 32,
    "num_epochs": 3,
    "max_length": 128,
    "warmup_steps": 500,
    "weight_decay": 0.01
}

# 开始MLflow Run
with mlflow.start_run(run_name="bert-base-lr2e5-bs32") as run:
    
    # 1. 记录超参数
    mlflow.log_params(params)
    
    # 2. 记录代码版本
    mlflow.log_param("git_commit", os.popen("git rev-parse HEAD").read().strip())
    
    # 3. 记录环境信息
    mlflow.log_param("cuda_version", torch.version.cuda)
    mlflow.log_param("gpu_name", torch.cuda.get_device_name(0))
    mlflow.log_param("num_gpus", torch.cuda.device_count())
    
    # 加载模型和数据
    model = AutoModelForSequenceClassification.from_pretrained(
        params["model_name"],
        num_labels=2
    )
    tokenizer = AutoTokenizer.from_pretrained(params["model_name"])
    
    # 数据集
    dataset = load_dataset("imdb")
    train_dataset = dataset["train"].shuffle(seed=42).select(range(10000))
    val_dataset = dataset["test"].select(range(1000))
    
    def tokenize_function(examples):
        return tokenizer(
            examples["text"],
            padding="max_length",
            truncation=True,
            max_length=params["max_length"]
        )
    
    train_dataset = train_dataset.map(tokenize_function, batched=True)
    val_dataset = val_dataset.map(tokenize_function, batched=True)
    
    train_loader = DataLoader(train_dataset, batch_size=params["batch_size"], shuffle=True)
    val_loader = DataLoader(val_dataset, batch_size=params["batch_size"])
    
    # 优化器
    optimizer = AdamW(
        model.parameters(),
        lr=params["learning_rate"],
        weight_decay=params["weight_decay"]
    )
    
    # 训练循环
    model.cuda()
    model.train()
    
    global_step = 0
    for epoch in range(params["num_epochs"]):
        total_loss = 0
        for batch_idx, batch in enumerate(train_loader):
            inputs = {k: v.cuda() for k, v in batch.items() if k in ["input_ids", "attention_mask"]}
            labels = batch["label"].cuda()
            
            outputs = model(**inputs, labels=labels)
            loss = outputs.loss
            
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            
            total_loss += loss.item()
            global_step += 1
            
            # 4. 记录训练指标（每10步）
            if global_step % 10 == 0:
                mlflow.log_metric("train_loss", loss.item(), step=global_step)
                mlflow.log_metric("learning_rate", optimizer.param_groups[0]["lr"], step=global_step)
        
        avg_train_loss = total_loss / len(train_loader)
        
        # 5. 验证评估
        model.eval()
        val_loss = 0
        correct = 0
        total = 0
        
        with torch.no_grad():
            for batch in val_loader:
                inputs = {k: v.cuda() for k, v in batch.items() if k in ["input_ids", "attention_mask"]}
                labels = batch["label"].cuda()
                
                outputs = model(**inputs, labels=labels)
                val_loss += outputs.loss.item()
                
                predictions = torch.argmax(outputs.logits, dim=-1)
                correct += (predictions == labels).sum().item()
                total += labels.size(0)
        
        avg_val_loss = val_loss / len(val_loader)
        accuracy = correct / total
        
        # 6. 记录验证指标
        mlflow.log_metric("val_loss", avg_val_loss, step=epoch)
        mlflow.log_metric("val_accuracy", accuracy, step=epoch)
        
        print(f"Epoch {epoch+1}/{params['num_epochs']} - "
              f"Train Loss: {avg_train_loss:.4f}, Val Loss: {avg_val_loss:.4f}, "
              f"Val Accuracy: {accuracy:.4f}")
        
        model.train()
    
    # 7. 保存模型到MLflow
    mlflow.pytorch.log_model(
        model,
        "model",
        registered_model_name="bert-sentiment-classifier"
    )
    
    # 8. 记录额外artifacts
    # 保存confusion matrix图像
    import matplotlib.pyplot as plt
    from sklearn.metrics import confusion_matrix, ConfusionMatrixDisplay
    
    # 生成预测
    all_preds = []
    all_labels = []
    with torch.no_grad():
        for batch in val_loader:
            inputs = {k: v.cuda() for k, v in batch.items() if k in ["input_ids", "attention_mask"]}
            labels = batch["label"]
            outputs = model(**inputs)
            predictions = torch.argmax(outputs.logits, dim=-1).cpu()
            all_preds.extend(predictions.tolist())
            all_labels.extend(labels.tolist())
    
    cm = confusion_matrix(all_labels, all_preds)
    disp = ConfusionMatrixDisplay(confusion_matrix=cm, display_labels=["Negative", "Positive"])
    disp.plot()
    plt.savefig("confusion_matrix.png")
    mlflow.log_artifact("confusion_matrix.png")
    
    # 9. 记录模型签名
    from mlflow.models.signature import infer_signature
    sample_input = {
        "input_ids": torch.randint(0, 30522, (1, params["max_length"])),
        "attention_mask": torch.ones(1, params["max_length"])
    }
    sample_output = model(**sample_input).logits.detach()
    signature = infer_signature(sample_input, sample_output)
    mlflow.pytorch.log_model(model, "model_with_signature", signature=signature)
    
    # 10. 添加标签
    mlflow.set_tag("model_type", "transformer")
    mlflow.set_tag("task", "sentiment-classification")
    mlflow.set_tag("framework", "pytorch")
    mlflow.set_tag("status", "production-ready")
    
    print(f"Run ID: {run.info.run_id}")
    print(f"MLflow UI: http://mlflow.example.com/#/experiments/{run.info.experiment_id}/runs/{run.info.run_id}")
```

### 2.3 MLflow Kubernetes训练Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: bert-training-mlflow
  namespace: ai-platform
spec:
  template:
    metadata:
      labels:
        app: bert-training
    spec:
      restartPolicy: OnFailure
      containers:
      - name: trainer
        image: myregistry/pytorch-trainer:latest
        command:
        - python
        - /workspace/train.py
        
        env:
        - name: MLFLOW_TRACKING_URI
          value: "http://mlflow-server.ai-platform.svc.cluster.local:5000"
        - name: MLFLOW_EXPERIMENT_NAME
          value: "bert-sentiment-classification"
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: access-key-id
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: secret-access-key
        
        resources:
          requests:
            cpu: "8"
            memory: "32Gi"
            nvidia.com/gpu: 1
          limits:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: 1
        
        volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: dshm
          mountPath: /dev/shm
      
      volumes:
      - name: workspace
        configMap:
          name: training-code
      - name: dshm
        emptyDir:
          medium: Memory
          sizeLimit: 16Gi
```

---

## 三、Weights & Biases集成

### 3.1 W&B特性优势

相比MLflow，W&B提供更丰富的可视化和协作功能：

- **实时可视化**：训练曲线实时更新
- **超参数对比**：Parallel Coordinates图
- **模型对比**：Run Comparison Table
- **报告生成**：Markdown格式实验报告
- **团队协作**：共享实验、评论、讨论

### 3.2 W&B训练集成

```python
import wandb
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer, Trainer, TrainingArguments
from datasets import load_dataset

# 初始化W&B
wandb.init(
    project="llama2-finetuning",
    name="llama2-7b-alpaca-lora",
    config={
        "model": "meta-llama/Llama-2-7b-hf",
        "dataset": "alpaca",
        "learning_rate": 3e-4,
        "batch_size": 4,
        "gradient_accumulation_steps": 4,
        "num_epochs": 3,
        "lora_r": 8,
        "lora_alpha": 16,
        "lora_dropout": 0.05
    },
    tags=["lora", "instruction-tuning", "llama2"]
)

config = wandb.config

# 加载模型和数据
model = AutoModelForCausalLM.from_pretrained(
    config.model,
    load_in_8bit=True,
    device_map="auto"
)
tokenizer = AutoTokenizer.from_pretrained(config.model)

# LoRA配置
from peft import LoraConfig, get_peft_model, TaskType

lora_config = LoraConfig(
    task_type=TaskType.CAUSAL_LM,
    r=config.lora_r,
    lora_alpha=config.lora_alpha,
    lora_dropout=config.lora_dropout,
    target_modules=["q_proj", "v_proj", "k_proj", "o_proj"]
)
model = get_peft_model(model, lora_config)

# 数据集
dataset = load_dataset("tatsu-lab/alpaca")

# 训练参数
training_args = TrainingArguments(
    output_dir="./checkpoints",
    per_device_train_batch_size=config.batch_size,
    gradient_accumulation_steps=config.gradient_accumulation_steps,
    learning_rate=config.learning_rate,
    num_train_epochs=config.num_epochs,
    logging_steps=10,
    save_steps=500,
    eval_steps=500,
    evaluation_strategy="steps",
    report_to="wandb",  # 自动同步到W&B
    run_name=wandb.run.name
)

# Trainer会自动记录指标到W&B
trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=dataset["train"],
    eval_dataset=dataset["test"]
)

# 训练
trainer.train()

# 手动记录额外信息
wandb.log({
    "trainable_params": sum(p.numel() for p in model.parameters() if p.requires_grad),
    "total_params": sum(p.numel() for p in model.parameters())
})

# 保存模型到W&B Artifacts
artifact = wandb.Artifact("llama2-7b-alpaca-lora", type="model")
artifact.add_dir("./checkpoints/checkpoint-final")
wandb.log_artifact(artifact)

wandb.finish()
```

### 3.3 W&B Sweeps超参数搜索

```yaml
# sweep_config.yaml
program: train.py
method: bayes  # 贝叶斯优化
metric:
  name: eval/loss
  goal: minimize
parameters:
  learning_rate:
    distribution: log_uniform_values
    min: 1e-5
    max: 1e-3
  lora_r:
    values: [4, 8, 16, 32]
  lora_alpha:
    values: [8, 16, 32, 64]
  batch_size:
    values: [2, 4, 8]
  gradient_accumulation_steps:
    values: [2, 4, 8, 16]
early_terminate:
  type: hyperband
  min_iter: 3
  eta: 2
```

```python
# 启动Sweep
import wandb

# 创建Sweep
sweep_id = wandb.sweep(
    sweep_config,
    project="llama2-finetuning"
)

# 运行Sweep Agent（可在多个节点运行）
wandb.agent(sweep_id, function=train, count=20)
```

**Kubernetes并行Sweep：**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: wandb-sweep-agent
  namespace: ai-platform
spec:
  parallelism: 10  # 10个并行agent
  completions: 20   # 总共20次实验
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: sweep-agent
        image: myregistry/wandb-trainer:latest
        command:
        - wandb
        - agent
        - user/project/sweep-id
        env:
        - name: WANDB_API_KEY
          valueFrom:
            secretKeyRef:
              name: wandb-secret
              key: api-key
        resources:
          requests:
            nvidia.com/gpu: 1
          limits:
            nvidia.com/gpu: 1
```

---

## 四、Kubeflow Pipelines

### 4.1 完整ML Pipeline定义

```python
from kfp import dsl, compiler
from kfp.dsl import Input, Output, Dataset, Model, Metrics

@dsl.component(
    base_image="python:3.9",
    packages_to_install=["pandas", "scikit-learn", "boto3"]
)
def data_preprocessing(
    raw_data_path: str,
    processed_data: Output[Dataset],
    train_test_split_ratio: float = 0.8
):
    """数据预处理组件"""
    import pandas as pd
    from sklearn.model_selection import train_test_split
    import pickle
    
    # 读取数据
    df = pd.read_csv(raw_data_path)
    
    # 清洗
    df = df.dropna()
    df = df[df['amount'] > 0]
    
    # 分割
    train_df, test_df = train_test_split(df, test_size=1-train_test_split_ratio, random_state=42)
    
    # 保存
    output_data = {
        "train": train_df.to_dict(),
        "test": test_df.to_dict()
    }
    with open(processed_data.path, 'wb') as f:
        pickle.dump(output_data, f)

@dsl.component(
    base_image="pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime",
    packages_to_install=["transformers", "mlflow"]
)
def model_training(
    processed_data: Input[Dataset],
    model_output: Output[Model],
    metrics_output: Output[Metrics],
    learning_rate: float = 2e-5,
    num_epochs: int = 3
):
    """模型训练组件"""
    import torch
    from transformers import AutoModelForSequenceClassification, Trainer, TrainingArguments
    import pickle
    import mlflow
    import json
    
    # 加载数据
    with open(processed_data.path, 'rb') as f:
        data = pickle.load(f)
    
    # 训练模型
    model = AutoModelForSequenceClassification.from_pretrained("bert-base-uncased", num_labels=2)
    
    training_args = TrainingArguments(
        output_dir="/tmp/model",
        learning_rate=learning_rate,
        num_train_epochs=num_epochs,
        per_device_train_batch_size=32
    )
    
    trainer = Trainer(model=model, args=training_args, train_dataset=data["train"])
    trainer.train()
    
    # 评估
    eval_results = trainer.evaluate(data["test"])
    
    # 保存模型
    model.save_pretrained(model_output.path)
    
    # 保存指标
    metrics_output.log_metric("accuracy", eval_results["eval_accuracy"])
    metrics_output.log_metric("loss", eval_results["eval_loss"])

@dsl.component(
    base_image="python:3.9",
    packages_to_install=["mlflow", "boto3"]
)
def model_registration(
    model: Input[Model],
    metrics: Input[Metrics],
    accuracy_threshold: float = 0.85
) -> str:
    """模型注册组件"""
    import mlflow
    
    # 读取指标
    accuracy = metrics.metadata["accuracy"]
    
    if accuracy >= accuracy_threshold:
        # 注册到MLflow
        mlflow.set_tracking_uri("http://mlflow-server.ai-platform.svc.cluster.local:5000")
        
        with mlflow.start_run():
            mlflow.log_metric("accuracy", accuracy)
            model_uri = mlflow.pytorch.log_model(model.path, "model")
            
            # 注册模型
            result = mlflow.register_model(
                model_uri,
                "bert-classifier",
                tags={"stage": "production"}
            )
            return result.version
    else:
        raise ValueError(f"模型精度 {accuracy} 低于阈值 {accuracy_threshold}")

@dsl.pipeline(
    name="ML Training Pipeline",
    description="完整的ML训练Pipeline"
)
def ml_training_pipeline(
    data_path: str = "s3://data/raw/dataset.csv",
    learning_rate: float = 2e-5,
    num_epochs: int = 3,
    accuracy_threshold: float = 0.85
):
    # 数据预处理
    preprocess_task = data_preprocessing(raw_data_path=data_path)
    
    # 模型训练
    train_task = model_training(
        processed_data=preprocess_task.outputs["processed_data"],
        learning_rate=learning_rate,
        num_epochs=num_epochs
    )
    
    # 模型注册
    register_task = model_registration(
        model=train_task.outputs["model_output"],
        metrics=train_task.outputs["metrics_output"],
        accuracy_threshold=accuracy_threshold
    )

# 编译Pipeline
compiler.Compiler().compile(
    pipeline_func=ml_training_pipeline,
    package_path="ml_pipeline.yaml"
)
```

### 4.2 Kubeflow部署

```bash
# 安装Kubeflow Pipelines
export PIPELINE_VERSION=2.0.5
kubectl apply -k "github.com/kubeflow/pipelines/manifests/kustomize/cluster-scoped-resources?ref=$PIPELINE_VERSION"
kubectl wait --for condition=established --timeout=60s crd/applications.app.k8s.io
kubectl apply -k "github.com/kubeflow/pipelines/manifests/kustomize/env/platform-agnostic?ref=$PIPELINE_VERSION"

# 端口转发访问UI
kubectl port-forward -n kubeflow svc/ml-pipeline-ui 8080:80
# 访问 http://localhost:8080
```

### 4.3 提交Pipeline运行

```python
import kfp

# 连接到Kubeflow Pipelines
client = kfp.Client(host="http://ml-pipeline-ui.kubeflow.svc.cluster.local")

# 上传Pipeline
pipeline_id = client.upload_pipeline(
    pipeline_package_path="ml_pipeline.yaml",
    pipeline_name="ML Training Pipeline v1.0"
)

# 创建实验
experiment = client.create_experiment(name="bert-classification-experiments")

# 提交运行
run = client.run_pipeline(
    experiment_id=experiment.id,
    job_name="bert-training-run-001",
    pipeline_id=pipeline_id,
    params={
        "data_path": "s3://data/raw/reviews.csv",
        "learning_rate": 3e-5,
        "num_epochs": 5,
        "accuracy_threshold": 0.90
    }
)

print(f"Run ID: {run.id}")
print(f"Run URL: http://localhost:8080/#/runs/details/{run.id}")
```

---

## 五、实验对比与分析

### 5.1 MLflow UI实验对比

```python
from mlflow.tracking import MlflowClient

client = MlflowClient("http://mlflow-server.ai-platform.svc.cluster.local:5000")

# 获取实验所有runs
experiment_id = "1"
runs = client.search_runs(
    experiment_ids=[experiment_id],
    filter_string="metrics.val_accuracy > 0.85",
    order_by=["metrics.val_accuracy DESC"],
    max_results=10
)

# 对比最佳runs
import pandas as pd

comparison_data = []
for run in runs:
    comparison_data.append({
        "run_id": run.info.run_id,
        "learning_rate": run.data.params.get("learning_rate"),
        "batch_size": run.data.params.get("batch_size"),
        "val_accuracy": run.data.metrics.get("val_accuracy"),
        "val_loss": run.data.metrics.get("val_loss"),
        "duration": run.info.end_time - run.info.start_time
    })

df = pd.DataFrame(comparison_data)
print(df.to_string())

# 输出示例：
#        run_id  learning_rate batch_size  val_accuracy  val_loss  duration(ms)
# 0  abc123def45           2e-5         32        0.9234    0.2145      1234567
# 1  ghi789jkl01           3e-5         32        0.9187    0.2298      1198765
# 2  mno456pqr78           2e-5         64        0.9156    0.2401      987654
```

### 5.2 自动生成实验报告

```python
import mlflow
from mlflow.tracking import MlflowClient
import matplotlib.pyplot as plt

client = MlflowClient()

experiment_id = "1"
runs = client.search_runs(experiment_ids=[experiment_id], max_results=20)

# 生成学习曲线对比图
plt.figure(figsize=(12, 6))

for run in runs[:5]:  # 前5个最佳runs
    run_id = run.info.run_id
    metrics = client.get_metric_history(run_id, "val_accuracy")
    steps = [m.step for m in metrics]
    values = [m.value for m in metrics]
    
    label = f"LR={run.data.params['learning_rate']}, BS={run.data.params['batch_size']}"
    plt.plot(steps, values, label=label, marker='o')

plt.xlabel("Epoch")
plt.ylabel("Validation Accuracy")
plt.title("Validation Accuracy Comparison")
plt.legend()
plt.grid(True)
plt.savefig("accuracy_comparison.png", dpi=300)

# 记录到MLflow
with mlflow.start_run():
    mlflow.log_artifact("accuracy_comparison.png")
```

---

## 六、分布式超参数优化

### 6.1 Ray Tune集成

```python
from ray import tune
from ray.tune.schedulers import ASHAScheduler
from ray.tune.integration.mlflow import MLflowLoggerCallback
import torch
from transformers import AutoModelForSequenceClassification, Trainer, TrainingArguments

def train_model(config):
    """训练函数"""
    model = AutoModelForSequenceClassification.from_pretrained(
        "bert-base-uncased",
        num_labels=2
    )
    
    training_args = TrainingArguments(
        output_dir="/tmp/model",
        learning_rate=config["learning_rate"],
        per_device_train_batch_size=config["batch_size"],
        num_train_epochs=3,
        evaluation_strategy="epoch"
    )
    
    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=eval_dataset
    )
    
    # 训练并返回结果
    result = trainer.train()
    eval_result = trainer.evaluate()
    
    # 报告指标给Ray Tune
    tune.report(
        accuracy=eval_result["eval_accuracy"],
        loss=eval_result["eval_loss"]
    )

# Ray Tune搜索空间
search_space = {
    "learning_rate": tune.loguniform(1e-5, 1e-3),
    "batch_size": tune.choice([16, 32, 64]),
    "weight_decay": tune.uniform(0.0, 0.1)
}

# ASHA调度器（早停）
scheduler = ASHAScheduler(
    metric="accuracy",
    mode="max",
    max_t=10,
    grace_period=1,
    reduction_factor=2
)

# 运行超参数搜索
analysis = tune.run(
    train_model,
    config=search_space,
    num_samples=20,  # 20次试验
    scheduler=scheduler,
    resources_per_trial={"cpu": 8, "gpu": 1},
    callbacks=[
        MLflowLoggerCallback(
            tracking_uri="http://mlflow-server.ai-platform.svc.cluster.local:5000",
            experiment_name="ray-tune-hpo",
            save_artifact=True
        )
    ]
)

# 最佳配置
best_config = analysis.best_config
print(f"Best config: {best_config}")
print(f"Best accuracy: {analysis.best_result['accuracy']}")
```

### 6.2 Optuna优化器

```python
import optuna
import mlflow

def objective(trial):
    """Optuna目标函数"""
    # 定义超参数搜索空间
    learning_rate = trial.suggest_float("learning_rate", 1e-5, 1e-3, log=True)
    batch_size = trial.suggest_categorical("batch_size", [16, 32, 64])
    num_layers = trial.suggest_int("num_layers", 6, 12)
    dropout = trial.suggest_float("dropout", 0.1, 0.5)
    
    # 训练模型
    model = build_model(num_layers=num_layers, dropout=dropout)
    accuracy = train_and_evaluate(model, learning_rate, batch_size)
    
    # 记录到MLflow
    with mlflow.start_run(nested=True):
        mlflow.log_params({
            "learning_rate": learning_rate,
            "batch_size": batch_size,
            "num_layers": num_layers,
            "dropout": dropout
        })
        mlflow.log_metric("accuracy", accuracy)
    
    return accuracy

# 创建Optuna Study
study = optuna.create_study(
    study_name="bert-optimization",
    direction="maximize",
    storage="postgresql://optuna:password@postgres:5432/optuna",  # 持久化
    load_if_exists=True,
    sampler=optuna.samplers.TPESampler(),  # Tree-structured Parzen Estimator
    pruner=optuna.pruners.MedianPruner(n_startup_trials=5, n_warmup_steps=3)
)

# 运行优化（分布式）
with mlflow.start_run():
    study.optimize(objective, n_trials=50, timeout=3600)
    
    # 记录最佳结果
    mlflow.log_params(study.best_params)
    mlflow.log_metric("best_accuracy", study.best_value)

print(f"Best trial: {study.best_trial.number}")
print(f"Best params: {study.best_params}")
print(f"Best accuracy: {study.best_value}")

# 可视化
import optuna.visualization as vis

# 优化历史
fig = vis.plot_optimization_history(study)
fig.write_html("optimization_history.html")

# 参数重要性
fig = vis.plot_param_importances(study)
fig.write_html("param_importances.html")
```

**Kubernetes分布式Optuna：**
```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: optuna-worker
spec:
  parallelism: 10  # 10个并行worker
  completions: 50   # 总共50次trial
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: worker
        image: myregistry/optuna-trainer:latest
        command:
        - python
        - optimize.py
        env:
        - name: OPTUNA_STORAGE
          value: "postgresql://optuna:password@postgres.ai-platform:5432/optuna"
        - name: STUDY_NAME
          value: "bert-optimization"
        resources:
          limits:
            nvidia.com/gpu: 1
```

---

## 七、CI/CD for ML

### 7.1 GitLab CI ML Pipeline

```yaml
# .gitlab-ci.yml
stages:
  - data-validation
  - train
  - evaluate
  - register
  - deploy

variables:
  MLFLOW_TRACKING_URI: "http://mlflow-server.ai-platform.svc.cluster.local:5000"
  EXPERIMENT_NAME: "bert-classification-ci"

data-validation:
  stage: data-validation
  image: python:3.9
  script:
    - pip install great-expectations pandas
    - python scripts/validate_data.py
    - echo "Data validation passed"
  artifacts:
    reports:
      junit: validation-report.xml
  only:
    - main
    - merge_requests

train-model:
  stage: train
  image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
  script:
    - pip install -r requirements.txt
    - python train.py --experiment-name $EXPERIMENT_NAME
    - echo $MLFLOW_RUN_ID > run_id.txt
  artifacts:
    paths:
      - run_id.txt
      - model/
    expire_in: 7 days
  tags:
    - gpu
  only:
    - main

evaluate-model:
  stage: evaluate
  image: python:3.9
  script:
    - pip install mlflow scikit-learn
    - export RUN_ID=$(cat run_id.txt)
    - python scripts/evaluate_model.py --run-id $RUN_ID
    - export ACCURACY=$(python scripts/get_metric.py --run-id $RUN_ID --metric accuracy)
    - echo "Model accuracy: $ACCURACY"
    - |
      if (( $(echo "$ACCURACY < 0.85" | bc -l) )); then
        echo "Accuracy below threshold, failing pipeline"
        exit 1
      fi
  dependencies:
    - train-model
  only:
    - main

register-model:
  stage: register
  image: python:3.9
  script:
    - pip install mlflow
    - export RUN_ID=$(cat run_id.txt)
    - python scripts/register_model.py --run-id $RUN_ID --model-name bert-classifier
  dependencies:
    - train-model
    - evaluate-model
  only:
    - main

deploy-to-staging:
  stage: deploy
  image: bitnami/kubectl:latest
  script:
    - kubectl config use-context staging-cluster
    - export MODEL_VERSION=$(cat model_version.txt)
    - envsubst < k8s/inference-service.yaml | kubectl apply -f -
  environment:
    name: staging
    url: https://model-staging.example.com
  dependencies:
    - register-model
  only:
    - main

deploy-to-production:
  stage: deploy
  image: bitnami/kubectl:latest
  script:
    - kubectl config use-context prod-cluster
    - export MODEL_VERSION=$(cat model_version.txt)
    - envsubst < k8s/inference-service.yaml | kubectl apply -f -
  environment:
    name: production
    url: https://model.example.com
  when: manual  # 手动触发生产部署
  dependencies:
    - register-model
  only:
    - main
```

---

## 八、实验管理最佳实践

### 8.1 实验命名规范

```python
"""
推荐的实验命名格式：
{model}_{task}_{date}_{variant}

示例：
- bert_sentiment_20240115_baseline
- llama2-7b_instruction_20240115_lora-r8
- resnet50_imagenet_20240115_mixup
"""

import mlflow
from datetime import datetime

def create_run_name(model_name, task, variant=""):
    date_str = datetime.now().strftime("%Y%m%d")
    parts = [model_name, task, date_str]
    if variant:
        parts.append(variant)
    return "_".join(parts)

# 使用
run_name = create_run_name("bert-base", "sentiment", "lr2e5")
with mlflow.start_run(run_name=run_name):
    # 训练代码
    pass
```

### 8.2 必须记录的信息

**1. 代码版本：**
```python
import subprocess

git_commit = subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()
git_branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('ascii').strip()

mlflow.log_param("git_commit", git_commit)
mlflow.log_param("git_branch", git_branch)
mlflow.set_tag("git_repo", "github.com/org/repo")
```

**2. 数据版本：**
```python
# 使用DVC
import dvc.api

data_version = dvc.api.get_url("data/train.csv", rev="main")
mlflow.log_param("data_version", data_version)
mlflow.log_param("data_commit", dvc.api.get_rev())
```

**3. 环境信息：**
```python
import torch
import transformers
import sys

mlflow.log_param("python_version", sys.version)
mlflow.log_param("pytorch_version", torch.__version__)
mlflow.log_param("transformers_version", transformers.__version__)
mlflow.log_param("cuda_version", torch.version.cuda)
mlflow.log_param("gpu_count", torch.cuda.device_count())
mlflow.log_param("gpu_name", torch.cuda.get_device_name(0))
```

**4. 数据统计：**
```python
mlflow.log_param("train_samples", len(train_dataset))
mlflow.log_param("val_samples", len(val_dataset))
mlflow.log_param("num_classes", num_classes)
mlflow.log_param("avg_sequence_length", avg_seq_len)
```

### 8.3 模型性能追踪

```python
# 训练过程追踪
class MLflowCallback:
    def __init__(self, log_every_n_steps=10):
        self.log_every_n_steps = log_every_n_steps
        self.step = 0
    
    def on_batch_end(self, loss, metrics):
        self.step += 1
        if self.step % self.log_every_n_steps == 0:
            mlflow.log_metric("train_loss", loss, step=self.step)
            for key, value in metrics.items():
                mlflow.log_metric(f"train_{key}", value, step=self.step)
    
    def on_epoch_end(self, epoch, val_metrics):
        for key, value in val_metrics.items():
            mlflow.log_metric(f"val_{key}", value, step=epoch)

# 使用
callback = MLflowCallback(log_every_n_steps=10)

for epoch in range(num_epochs):
    for batch in train_loader:
        loss = train_step(batch)
        callback.on_batch_end(loss, {"accuracy": acc})
    
    val_metrics = evaluate()
    callback.on_epoch_end(epoch, val_metrics)
```

---

## 九、成本与ROI分析

| 平台 | 部署成本 | 维护成本 | 功能完整性 | 推荐场景 |
|-----|---------|---------|-----------|---------|
| **MLflow** | 低（自建）| 低 | ★★★☆☆ | 小团队、基础追踪 |
| **W&B** | 中（SaaS）| 极低 | ★★★★★ | 研究团队、快速迭代 |
| **Kubeflow** | 高（复杂）| 高 | ★★★★☆ | 企业级、端到端 |
| **自建方案** | 极高 | 极高 | 定制 | 大厂、强定制需求 |

**成本节省策略：**
- MLflow自建：$200/月（K8s集群 + PostgreSQL + S3）
- W&B Team版：$50/用户/月（但节省开发时间）
- 混合方案：MLflow追踪 + W&B可视化（最优性价比）

---

## 十、监控告警

```yaml
# Prometheus告警规则
groups:
- name: mlflow_alerts
  interval: 30s
  rules:
  - alert: MLflowServerDown
    expr: up{job="mlflow-server"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "MLflow服务不可用"
  
  - alert: ExperimentRunFailureRateHigh
    expr: rate(mlflow_run_failures_total[10m]) > 0.1
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "实验失败率 > 10%"
  
  - alert: ModelRegistrationStuck
    expr: (time() - mlflow_last_model_registration_timestamp) > 86400
    for: 1h
    labels:
      severity: info
    annotations:
      summary: "超过24小时未注册新模型"
```

---

**相关表格：**
- [111-AI基础设施架构](./111-ai-infrastructure.md)
- [112-分布式训练框架](./112-distributed-training-frameworks.md)
- [113-AI模型注册中心](./113-model-registry.md)
- [115-AI数据处理Pipeline](./115-ai-data-pipeline.md)
- [116-LLM模型Serving架构](./116-llm-serving-architecture.md)

**版本信息：**
- MLflow: v2.9.0+
- Weights & Biases: latest
- Kubeflow Pipelines: v2.0+
- Ray Tune: v2.9.0+
- Optuna: v3.5.0+
- Kubernetes: v1.27+
