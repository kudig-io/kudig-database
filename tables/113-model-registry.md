# AI模型注册中心与版本管理

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [MLflow Model Registry](https://mlflow.org/docs/latest/model-registry.html) | [ModelMesh](https://github.com/kserve/modelmesh-serving)

## 模型生命周期管理架构

```
┌─────────────────────────────────────────────────────────────────┐
│                    模型开发与训练阶段                             │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  实验跟踪 (MLflow/W&B)                                   │  │
│  │  - 参数记录                                              │  │
│  │  - 指标记录                                              │  │
│  │  │  - 模型文件记录                                       │  │
│  └──────────────┬───────────────────────────────────────────┘  │
└─────────────────┼──────────────────────────────────────────────┘
                  │ 注册模型
                  v
┌─────────────────────────────────────────────────────────────────┐
│                    模型注册中心 (Model Registry)                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  模型元数据管理                                          │   │
│  │  ├── 版本控制 (v1, v2, v3...)                           │   │
│  │  ├── 阶段管理 (Staging → Production → Archived)        │   │
│  │  ├── 血缘追踪 (训练数据、代码版本、超参数)              │   │
│  │  ├── 性能指标 (Accuracy, Latency, Throughput)          │   │
│  │  └── 标签标注 (business_unit, use_case, owner)         │   │
│  └─────────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  存储后端                                                │   │
│  │  ├── 模型文件: S3/OSS/MinIO                            │   │
│  │  ├── 元数据: PostgreSQL/MySQL                           │   │
│  │  └── 缓存: Redis                                        │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────┬───────────────────────────────────────────────┘
                  │ 加载模型
                  v
┌─────────────────────────────────────────────────────────────────┐
│                    模型服务部署阶段                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  KServe      │  │  Triton      │  │  TorchServe  │         │
│  │  InferenceS  │  │  Inference   │  │              │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
```

---

## 一、MLflow Model Registry

### 1. MLflow部署架构

#### 生产级部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
  namespace: mlops
spec:
  replicas: 3
  selector:
    matchLabels:
      app: mlflow
  template:
    metadata:
      labels:
        app: mlflow
    spec:
      containers:
        - name: mlflow
          image: ghcr.io/mlflow/mlflow:v2.10.0
          args:
            - server
            - --host=0.0.0.0
            - --port=5000
            - --backend-store-uri=postgresql://mlflow:${DB_PASSWORD}@postgres:5432/mlflow
            - --default-artifact-root=s3://mlflow-artifacts/
            - --serve-artifacts
            - --artifacts-destination=s3://mlflow-artifacts/
          env:
            - name: AWS_ACCESS_KEY_ID
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: access-key
            - name: AWS_SECRET_ACCESS_KEY
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: secret-key
            - name: AWS_DEFAULT_REGION
              value: "us-east-1"
            - name: DB_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: postgres-credentials
                  key: password
          ports:
            - containerPort: 5000
          resources:
            requests:
              cpu: 1000m
              memory: 2Gi
            limits:
              cpu: 2000m
              memory: 4Gi
          livenessProbe:
            httpGet:
              path: /health
              port: 5000
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /health
              port: 5000
            initialDelaySeconds: 10
            periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: mlflow-service
  namespace: mlops
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-internal: "true"
spec:
  selector:
    app: mlflow
  ports:
    - port: 5000
      targetPort: 5000
  type: LoadBalancer
```

---

### 2. 模型注册与版本管理

#### Python SDK使用

```python
import mlflow
from mlflow.tracking import MlflowClient

# 连接MLflow服务器
mlflow.set_tracking_uri("http://mlflow-service.mlops:5000")
client = MlflowClient()

# ============ 训练阶段 ============
experiment_name = "llama2-finetuning"
mlflow.set_experiment(experiment_name)

with mlflow.start_run(run_name="llama2-7b-lora-v1") as run:
    # 记录超参数
    params = {
        "model_name": "meta-llama/Llama-2-7b-hf",
        "lora_r": 8,
        "lora_alpha": 16,
        "learning_rate": 2e-5,
        "batch_size": 32,
        "epochs": 3,
        "dataset": "alpaca-52k"
    }
    mlflow.log_params(params)
    
    # 训练模型
    model = train_model(params)
    
    # 记录指标
    metrics = {
        "train_loss": 0.35,
        "eval_loss": 0.42,
        "accuracy": 0.87,
        "bleu_score": 0.65
    }
    mlflow.log_metrics(metrics)
    
    # 记录模型
    mlflow.transformers.log_model(
        transformers_model=model,
        artifact_path="model",
        task="text-generation",
        # 模型签名(输入输出schema)
        signature=mlflow.models.infer_signature(
            model_input={"prompt": "Hello"},
            model_output={"generated_text": "Hello, how can I help you?"}
        ),
        # 额外元数据
        metadata={
            "training_dataset": "alpaca-52k",
            "training_time_hours": 12.5,
            "gpu_type": "A100-80GB",
            "framework_version": "transformers==4.36.0"
        }
    )
    
    # 记录训练曲线图片
    mlflow.log_artifact("training_curve.png")
    
    run_id = run.info.run_id

# ============ 注册模型 ============
model_name = "llama2-7b-chat"
model_uri = f"runs:/{run_id}/model"

# 注册新版本
model_version = mlflow.register_model(
    model_uri=model_uri,
    name=model_name,
    tags={
        "team": "nlp-team",
        "use_case": "customer-service-chatbot",
        "training_date": "2026-01-15"
    }
)

print(f"Model {model_name} version {model_version.version} registered")

# ============ 模型版本管理 ============

# 添加描述
client.update_model_version(
    name=model_name,
    version=model_version.version,
    description="""
    Llama2-7B fine-tuned on customer service conversations.
    
    **Performance Metrics:**
    - Accuracy: 87%
    - BLEU Score: 0.65
    - Average Latency: 120ms (A100)
    
    **Training Details:**
    - Dataset: 52K customer service dialogues
    - LoRA rank: 8
    - Training time: 12.5 hours on 8xA100
    """
)

# 添加标签
client.set_model_version_tag(
    name=model_name,
    version=model_version.version,
    key="validation_status",
    value="passed"
)

# ============ 阶段转换 ============

# 1. Staging环境测试
client.transition_model_version_stage(
    name=model_name,
    version=model_version.version,
    stage="Staging"
)

# 2. 生产环境部署(经过验证后)
client.transition_model_version_stage(
    name=model_name,
    version=model_version.version,
    stage="Production",
    archive_existing_versions=True  # 归档旧的生产版本
)

# 3. 归档旧版本
client.transition_model_version_stage(
    name=model_name,
    version="1",
    stage="Archived"
)

# ============ 查询模型 ============

# 获取最新生产版本
latest_prod_version = client.get_latest_versions(
    name=model_name,
    stages=["Production"]
)[0]

print(f"Latest Production Version: {latest_prod_version.version}")
print(f"Run ID: {latest_prod_version.run_id}")

# 加载模型用于推理
model = mlflow.transformers.load_model(
    model_uri=f"models:/{model_name}/Production"
)

# ============ 模型比较 ============

# 获取所有版本
versions = client.search_model_versions(f"name='{model_name}'")

for v in versions:
    run = client.get_run(v.run_id)
    print(f"Version {v.version}:")
    print(f"  Stage: {v.current_stage}")
    print(f"  Accuracy: {run.data.metrics.get('accuracy')}")
    print(f"  BLEU: {run.data.metrics.get('bleu_score')}")
```

---

### 3. 模型审批流程

#### Webhook配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: mlflow-webhooks
  namespace: mlops
data:
  webhooks.json: |
    [
      {
        "events": ["MODEL_VERSION_TRANSITIONED_STAGE"],
        "description": "Notify when model transitions to Production",
        "http_url_spec": {
          "url": "https://slack-webhook.company.com/notify",
          "method": "POST",
          "headers": {
            "Content-Type": "application/json"
          }
        },
        "job_spec": null
      },
      {
        "events": ["REGISTERED_MODEL_CREATED"],
        "description": "Trigger validation pipeline",
        "job_spec": {
          "job_id": "validation-pipeline-job",
          "workspace_url": "https://jenkins.company.com",
          "access_token": "${JENKINS_TOKEN}"
        }
      }
    ]
```

---

## 二、ModelMesh Serving

### 1. 架构优势

- **多模型复用**: 单GPU运行多个模型
- **智能加载**: 基于访问频率自动加载/卸载
- **高密度部署**: 提升GPU利用率 3-5x

#### ModelMesh部署

```bash
# 安装ModelMesh Serving
kubectl apply -f https://github.com/kserve/modelmesh-serving/releases/download/v0.11.2/modelmesh-serving-install.yaml

# 配置存储
kubectl create secret generic model-storage-config \
  --from-literal=type=s3 \
  --from-literal=access_key_id=${AWS_ACCESS_KEY} \
  --from-literal=secret_access_key=${AWS_SECRET_KEY} \
  --from-literal=endpoint_url=https://s3.amazonaws.com \
  --from-literal=default_bucket=models \
  -n modelmesh-serving
```

#### ServingRuntime配置

```yaml
apiVersion: serving.kserve.io/v1alpha1
kind: ServingRuntime
metadata:
  name: triton-runtime
  namespace: modelmesh-serving
spec:
  supportedModelFormats:
    - name: pytorch
      version: "1"
      autoSelect: true
    - name: onnx
      version: "1"
      autoSelect: true
    - name: tensorflow
      version: "2"
      autoSelect: true
  
  # 多模型配置
  multiModel: true
  
  grpcEndpoint: "port:8085"
  grpcDataEndpoint: "port:8001"
  
  containers:
    - name: triton
      image: nvcr.io/nvidia/tritonserver:23.12-py3
      args:
        - tritonserver
        - --model-control-mode=explicit
        - --strict-model-config=false
        - --strict-readiness=false
        - --allow-http=true
        - --allow-sagemaker=false
      resources:
        requests:
          cpu: 2
          memory: 8Gi
        limits:
          nvidia.com/gpu: 1
          cpu: 4
          memory: 16Gi
```

#### InferenceService配置

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: bert-qa-model
  namespace: modelmesh-serving
  annotations:
    serving.kserve.io/deploymentMode: ModelMesh
spec:
  predictor:
    model:
      modelFormat:
        name: pytorch
      storageUri: s3://models/bert-qa/v2
      runtime: triton-runtime
      resources:
        requests:
          memory: 2Gi
        limits:
          memory: 4Gi
```

---

## 三、Hugging Face Model Hub集成

### 1. 私有Model Hub部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: huggingface-hub
  namespace: mlops
spec:
  replicas: 2
  selector:
    matchLabels:
      app: hf-hub
  template:
    metadata:
      labels:
        app: hf-hub
    spec:
      containers:
        - name: hub
          image: huggingface/moon-landing:latest
          env:
            - name: POSTGRES_URL
              value: "postgresql://hub:password@postgres:5432/hub"
            - name: S3_BUCKET
              value: "hf-models"
            - name: S3_ENDPOINT
              value: "https://s3.amazonaws.com"
            - name: AWS_ACCESS_KEY_ID
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: access-key
            - name: AWS_SECRET_ACCESS_KEY
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: secret-key
          ports:
            - containerPort: 5000
          resources:
            requests:
              cpu: 1000m
              memory: 2Gi
```

### 2. 模型上传与下载

```python
from huggingface_hub import HfApi, login, snapshot_download

# 登录私有Hub
login(token="hf_xxx", add_to_git_credential=True)
api = HfApi(endpoint="https://hf-hub.company.com")

# ============ 上传模型 ============
api.create_repo(
    repo_id="company/llama2-7b-customer-service",
    repo_type="model",
    private=True
)

api.upload_folder(
    folder_path="./llama2-finetuned",
    repo_id="company/llama2-7b-customer-service",
    repo_type="model",
    commit_message="Add fine-tuned model v2.0"
)

# 添加模型卡片
api.upload_file(
    path_or_fileobj="README.md",
    path_in_repo="README.md",
    repo_id="company/llama2-7b-customer-service",
    repo_type="model"
)

# ============ 下载模型 ============
model_path = snapshot_download(
    repo_id="company/llama2-7b-customer-service",
    repo_type="model",
    cache_dir="/mnt/models",
    revision="v2.0"  # 指定版本
)

# ============ 版本管理 ============
# 创建tag
api.create_tag(
    repo_id="company/llama2-7b-customer-service",
    repo_type="model",
    tag="production",
    revision="main"
)

# 列出所有版本
refs = api.list_repo_refs(
    repo_id="company/llama2-7b-customer-service",
    repo_type="model"
)
print("Tags:", [tag.name for tag in refs.tags])
print("Branches:", [branch.name for branch in refs.branches])
```

---

## 四、DVC (Data Version Control)

### 1. DVC配置

```bash
# 初始化DVC
dvc init

# 配置远程存储
dvc remote add -d myremote s3://dvc-storage/models
dvc remote modify myremote region us-east-1

# 配置访问密钥
dvc remote modify myremote access_key_id ${AWS_ACCESS_KEY}
dvc remote modify myremote secret_access_key ${AWS_SECRET_KEY}
```

### 2. 模型版本管理

```bash
# 添加模型到DVC
dvc add models/llama2-7b-v1.pt

# 提交到Git(仅元数据)
git add models/llama2-7b-v1.pt.dvc .gitignore
git commit -m "Add llama2-7b model v1"
git tag -a "v1.0" -m "Production release v1.0"
git push origin v1.0

# 推送模型文件到远程存储
dvc push

# ============ 拉取模型 ============
# 切换到特定版本
git checkout v1.0

# 拉取对应模型文件
dvc pull
```

### 3. Pipeline配置

```yaml
# dvc.yaml
stages:
  preprocess:
    cmd: python preprocess.py
    deps:
      - data/raw
    outs:
      - data/processed
  
  train:
    cmd: python train.py
    deps:
      - data/processed
      - src/model.py
    params:
      - train.learning_rate
      - train.batch_size
    metrics:
      - metrics.json:
          cache: false
    outs:
      - models/model.pt
  
  evaluate:
    cmd: python evaluate.py
    deps:
      - models/model.pt
      - data/test
    metrics:
      - eval_metrics.json:
          cache: false
```

---

## 五、Model Governance

### 1. 模型元数据Schema

```yaml
# model_metadata.yaml
model_id: llama2-7b-customer-service-v2
model_name: "Llama2 7B Customer Service Chatbot"
version: 2.0.0
created_at: 2026-01-15T10:30:00Z
updated_at: 2026-01-15T10:30:00Z

# 所有权
owner:
  team: nlp-team
  contact: nlp-team@company.com
  department: AI Research

# 训练信息
training:
  dataset:
    name: customer-service-52k
    version: 1.2
    size: 52000
    source: internal
  framework:
    name: pytorch
    version: 2.1.0
  hyperparameters:
    learning_rate: 2e-5
    batch_size: 32
    epochs: 3
    lora_r: 8
    lora_alpha: 16
  compute:
    gpu_type: A100-80GB
    num_gpus: 8
    training_time_hours: 12.5
    cost_usd: 450

# 性能指标
performance:
  accuracy: 0.87
  bleu_score: 0.65
  rouge_l: 0.72
  latency_p50_ms: 95
  latency_p99_ms: 180
  throughput_qps: 120

# 合规性
compliance:
  data_privacy: GDPR-compliant
  model_bias_check: passed
  security_scan: passed
  approval_status: approved
  approved_by: john.doe@company.com
  approved_at: 2026-01-16T14:00:00Z

# 部署信息
deployment:
  environments:
    - name: staging
      deployed_at: 2026-01-16T15:00:00Z
      endpoint: https://staging-api.company.com/v1/chat
    - name: production
      deployed_at: 2026-01-18T10:00:00Z
      endpoint: https://api.company.com/v1/chat
  serving_framework: kserve
  replicas: 3
  resource_requirements:
    gpu: 1
    memory_gi: 16

# 监控
monitoring:
  dashboard_url: https://grafana.company.com/d/model-metrics
  alert_channels:
    - slack: #ml-alerts
    - email: oncall@company.com

# 血缘追踪
lineage:
  parent_model: llama2-7b-base
  training_code_commit: abc123def
  training_pipeline_run: pipeline-run-456
  artifacts:
    - model_weights.pt
    - config.json
    - tokenizer.json
```

---

## 六、A/B测试与金丝雀发布

### 1. KServe流量分割

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: chat-model
  namespace: production
spec:
  predictor:
    # 当前生产版本(90%流量)
    model:
      modelFormat:
        name: pytorch
      storageUri: s3://models/llama2-7b-v1
    minReplicas: 3
    maxReplicas: 10
  
  # 金丝雀版本(10%流量)
  canary:
    predictor:
      model:
        modelFormat:
          name: pytorch
        storageUri: s3://models/llama2-7b-v2
      minReplicas: 1
      maxReplicas: 3
  
  canaryTrafficPercent: 10
```

### 2. Argo Rollouts集成

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: model-serving
  namespace: production
spec:
  replicas: 5
  strategy:
    canary:
      steps:
        - setWeight: 10  # 10%流量到新版本
        - pause: {duration: 1h}  # 观察1小时
        - analysis:
            templates:
              - templateName: model-metrics-analysis
        - setWeight: 30
        - pause: {duration: 1h}
        - analysis:
            templates:
              - templateName: model-metrics-analysis
        - setWeight: 50
        - pause: {duration: 30m}
        - setWeight: 100
  
  selector:
    matchLabels:
      app: model-serving
  
  template:
    metadata:
      labels:
        app: model-serving
    spec:
      containers:
        - name: model-server
          image: model-server:v2.0
          env:
            - name: MODEL_PATH
              value: "s3://models/llama2-7b-v2"
---
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: model-metrics-analysis
  namespace: production
spec:
  metrics:
    - name: accuracy
      interval: 5m
      successCondition: result >= 0.85
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(model_predictions_correct[5m])) /
            sum(rate(model_predictions_total[5m]))
    
    - name: latency-p99
      interval: 5m
      successCondition: result <= 200
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            histogram_quantile(0.99,
              rate(model_inference_duration_milliseconds_bucket[5m])
            )
    
    - name: error-rate
      interval: 5m
      successCondition: result < 0.01
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(model_inference_errors[5m])) /
            sum(rate(model_inference_total[5m]))
```

---

## 七、模型血缘追踪

### 1. 血缘图示例

```
Training Data (v1.2)
       │
       ├─ customer_service_52k.jsonl
       ├─ data_preprocessing.py (commit: abc123)
       │
       v
Preprocessed Data
       │
       v
Training Pipeline (run_id: xyz789)
       │
       ├─ train.py (commit: def456)
       ├─ Hyperparameters: {lr: 2e-5, batch_size: 32}
       ├─ Base Model: meta-llama/Llama-2-7b-hf
       │
       v
Trained Model (v2.0)
       │
       ├─ model_weights.pt
       ├─ config.json
       ├─ Metrics: {accuracy: 0.87, bleu: 0.65}
       │
       v
Model Registry (MLflow)
       │
       ├─ Staging (v2.0)
       ├─ Validation Tests Passed
       │
       v
Production Deployment
       │
       ├─ KServe InferenceService
       ├─ Endpoint: https://api.company.com/v1/chat
       └─ Monitoring: Grafana Dashboard
```

---

## 八、生产最佳实践

### 模型注册检查清单

- ✅ 记录完整训练超参数
- ✅ 保存模型签名(输入输出schema)
- ✅ 记录性能指标(准确率、延迟、吞吐)
- ✅ 添加模型描述和文档
- ✅ 标注数据集版本和来源
- ✅ 记录训练成本(GPU小时)
- ✅ 添加所有者和联系方式
- ✅ 配置自动化测试流程
- ✅ 启用模型血缘追踪
- ✅ 设置告警和监控

### 版本管理规范

- ✅ 使用语义化版本号(major.minor.patch)
- ✅ 阶段化发布(Dev → Staging → Production)
- ✅ 保留生产版本至少3个历史版本
- ✅ 归档6个月未使用的模型
- ✅ 每次版本升级需审批流程
- ✅ 自动化回滚机制
- ✅ 版本变更通知(Slack/Email)
- ✅ 定期清理过期版本

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
