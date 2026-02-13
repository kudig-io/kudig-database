# 32 - MLOps端到端流水线

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [Kubeflow Pipelines](https://www.kubeflow.org/docs/components/pipelines/) | [MLflow](https://mlflow.org/) | [Argo Workflows](https://argoproj.github.io/argo-workflows/)

## 一、MLOps流水线架构

### 1.1 端到端流水线概览

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           MLOps End-to-End Pipeline                                 │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │  Data       │───▶│ Feature     │───▶│ Model       │───▶│ Model       │          │
│  │  Ingestion  │    │ Engineering │    │ Training    │    │ Evaluation  │          │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘          │
│        │                    │                    │                    │              │
│        ▼                    ▼                    ▼                    ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │ Data        │    │ Feature     │    │ Experiment  │    │ Performance │          │
│  │ Validation  │    │ Store       │    │ Tracking    │    │ Testing     │          │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘          │
│        │                    │                    │                    │              │
│        ▼                    ▼                    ▼                    ▼              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐          │
│  │ Data        │    │ Feature     │    │ Hyperparam  │    │ Model       │          │
│  │ Quality     │    │ Validation  │    │ Tuning      │    │ Registry    │          │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘          │
│                                                                                      │
│                              │              │              │                        │
│                              ▼              ▼              ▼                        │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                              Model Deployment                                 │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    │  │
│  │  │ Model       │───▶│ A/B         │───▶│ Canary      │───▶│ Production  │    │  │
│  │  │ Packaging   │    │ Testing     │    │ Rollout     │    │ Monitoring  │    │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘    │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 流水线组件详解

| 组件 | 功能 | 技术栈 | 运维关注点 |
|------|------|--------|------------|
| **数据摄取** | 数据采集、清洗、验证 | Airflow/Kafka | 数据一致性、延迟监控 |
| **特征工程** | 特征提取、转换、存储 | Feast/TF Transform | 特征漂移、版本管理 |
| **模型训练** | 分布式训练、超参调优 | Kubeflow/Katib | GPU利用率、训练时间 |
| **模型评估** | 性能评估、公平性检查 | MLflow/Evidently | 评估准确性、偏差检测 |
| **模型部署** | 打包、部署、流量切换 | KServe/Seldon | 部署成功率、延迟指标 |
| **在线服务** | 推理服务、自动扩缩容 | Istio/Knative | QPS、错误率、SLA |

---

## 二、Kubeflow Pipelines实现

### 2.1 流水线DSL定义

```python
# pipeline_definition.py
import kfp
from kfp import dsl
from kfp.components import create_component_from_func
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score
import mlflow
import evidently

@create_component_from_func
def data_ingestion(
    data_source: str,
    output_data_path: str
) -> str:
    \"\"\"数据摄取组件\"\"\"
    import pandas as pd
    import boto3
    
    # 从S3读取数据
    s3 = boto3.client('s3')
    bucket, key = data_source.replace('s3://', '').split('/', 1)
    s3.download_file(bucket, key, '/tmp/raw_data.csv')
    
    df = pd.read_csv('/tmp/raw_data.csv')
    
    # 数据验证
    assert df.shape[0] > 1000, \"数据量不足\"
    assert 'label' in df.columns, \"缺少标签列\"
    
    # 数据清洗
    df = df.dropna()
    df.to_csv(output_data_path, index=False)
    
    return f\"成功处理 {len(df)} 条记录\"

@create_component_from_func
def feature_engineering(
    input_data_path: str,
    output_features_path: str,
    output_labels_path: str
) -> dict:
    \"\"\"特征工程组件\"\"\"
    import pandas as pd
    from sklearn.preprocessing import StandardScaler, LabelEncoder
    import json
    
    df = pd.read_csv(input_data_path)
    
    # 特征选择
    feature_columns = [col for col in df.columns if col != 'label']
    X = df[feature_columns]
    y = df['label']
    
    # 特征标准化
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    
    # 标签编码
    le = LabelEncoder()
    y_encoded = le.fit_transform(y)
    
    # 保存特征和标签
    pd.DataFrame(X_scaled).to_csv(output_features_path, index=False, header=False)
    pd.Series(y_encoded).to_csv(output_labels_path, index=False, header=False)
    
    # 返回特征统计信息
    stats = {
        'feature_count': len(feature_columns),
        'sample_count': len(df),
        'label_classes': len(le.classes_),
        'scaling_params': {
            'mean': scaler.mean_.tolist(),
            'std': scaler.scale_.tolist()
        }
    }
    
    return json.dumps(stats)

@create_component_from_func
def model_training(
    features_path: str,
    labels_path: str,
    model_output_path: str,
    experiment_name: str
) -> dict:
    \"\"\"模型训练组件\"\"\"
    import pandas as pd
    import numpy as np
    from sklearn.ensemble import RandomForestClassifier
    from sklearn.model_selection import cross_val_score
    import mlflow
    import json
    import joblib
    
    # 加载数据
    X = pd.read_csv(features_path, header=None).values
    y = pd.read_csv(labels_path, header=None).values.ravel()
    
    # 开始MLflow实验
    mlflow.set_experiment(experiment_name)
    
    with mlflow.start_run() as run:
        # 模型训练
        model = RandomForestClassifier(
            n_estimators=100,
            max_depth=10,
            random_state=42
        )
        
        # 交叉验证
        cv_scores = cross_val_score(model, X, y, cv=5)
        
        # 最终训练
        model.fit(X, y)
        
        # 评估
        train_accuracy = model.score(X, y)
        
        # 记录MLflow参数
        mlflow.log_param(\"n_estimators\", 100)
        mlflow.log_param(\"max_depth\", 10)
        mlflow.log_metric(\"train_accuracy\", train_accuracy)
        mlflow.log_metric(\"cv_mean_accuracy\", cv_scores.mean())
        mlflow.log_metric(\"cv_std_accuracy\", cv_scores.std())
        
        # 保存模型
        joblib.dump(model, model_output_path)
        mlflow.log_artifact(model_output_path)
        
        # 返回结果
        result = {
            'run_id': run.info.run_id,
            'train_accuracy': float(train_accuracy),
            'cv_accuracy': float(cv_scores.mean()),
            'cv_std': float(cv_scores.std()),
            'model_path': model_output_path
        }
        
        return json.dumps(result)

@create_component_from_func
def model_evaluation(
    model_path: str,
    test_features_path: str,
    test_labels_path: str,
    evaluation_output_path: str
) -> dict:
    \"\"\"模型评估组件\"\"\"
    import pandas as pd
    import numpy as np
    import joblib
    from sklearn.metrics import (accuracy_score, precision_score, 
                               recall_score, f1_score, confusion_matrix)
    import json
    import evidently
    from evidently.model_profile import Profile
    from evidently.model_profile.sections import (
        DataDriftProfileSection,
        ClassificationPerformanceProfileSection
    )
    
    # 加载模型和测试数据
    model = joblib.load(model_path)
    X_test = pd.read_csv(test_features_path, header=None).values
    y_test = pd.read_csv(test_labels_path, header=None).values.ravel()
    
    # 预测
    y_pred = model.predict(X_test)
    y_proba = model.predict_proba(X_test)
    
    # 基础指标计算
    metrics = {
        'accuracy': float(accuracy_score(y_test, y_pred)),
        'precision': float(precision_score(y_test, y_pred, average='weighted')),
        'recall': float(recall_score(y_test, y_pred, average='weighted')),
        'f1_score': float(f1_score(y_test, y_pred, average='weighted'))
    }
    
    # 混淆矩阵
    cm = confusion_matrix(y_test, y_pred).tolist()
    metrics['confusion_matrix'] = cm
    
    # Evidently模型分析
    profile = Profile(sections=[
        ClassificationPerformanceProfileSection(),
        DataDriftProfileSection()
    ])
    
    # 创建DataFrame用于Evidently
    reference_data = pd.DataFrame({
        'prediction': y_pred,
        'target': y_test,
        **{f'prob_{i}': y_proba[:, i] for i in range(y_proba.shape[1])}
    })
    
    current_data = reference_data.copy()  # 在实际场景中应该是新数据
    
    profile.calculate(reference_data, current_data, column_mapping=None)
    
    # 添加Evidently指标
    metrics['classification_performance'] = profile.get_content()['classification_performance']
    
    # 保存评估结果
    with open(evaluation_output_path, 'w') as f:
        json.dump(metrics, f, indent=2)
    
    return json.dumps(metrics)

@dsl.pipeline(
    name='ml-training-pipeline',
    description='端到端机器学习训练流水线'
)
def ml_training_pipeline(
    data_source: str = 's3://company-data/training/dataset.csv',
    experiment_name: str = 'customer-churn-prediction'
):
    # 步骤1: 数据摄取
    data_op = data_ingestion(
        data_source=data_source,
        output_data_path='/tmp/cleaned_data.csv'
    )
    
    # 步骤2: 特征工程
    feature_op = feature_engineering(
        input_data_path=data_op.output,
        output_features_path='/tmp/features.csv',
        output_labels_path='/tmp/labels.csv'
    )
    
    # 步骤3: 模型训练
    train_op = model_training(
        features_path=feature_op.outputs['output'],
        labels_path='/tmp/labels.csv',
        model_output_path='/tmp/model.pkl',
        experiment_name=experiment_name
    )
    
    # 步骤4: 模型评估
    eval_op = model_evaluation(
        model_path=train_op.outputs['output'],
        test_features_path='/tmp/features.csv',  # 实际应使用独立测试集
        test_labels_path='/tmp/labels.csv',
        evaluation_output_path='/tmp/evaluation.json'
    )
    
    # 设置依赖关系
    feature_op.after(data_op)
    train_op.after(feature_op)
    eval_op.after(train_op)

# 编译流水线
if __name__ == '__main__':
    kfp.compiler.Compiler().compile(ml_training_pipeline, 'ml_training_pipeline.yaml')
```

---

## 三、CI/CD流水线集成

### 3.1 GitHub Actions配置

```yaml
# .github/workflows/ml-pipeline.yaml
name: ML Pipeline CI/CD

on:
  push:
    branches: [main, develop]
    paths:
      - 'ml-pipeline/**'
      - 'models/**'
      - '.github/workflows/ml-pipeline.yaml'
  pull_request:
    branches: [main]
    paths:
      - 'ml-pipeline/**'
      - 'models/**'

env:
  PYTHON_VERSION: '3.9'
  MLFLOW_TRACKING_URI: 'http://mlflow-server:5000'

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: ${{ env.PYTHON_VERSION }}
    
    - name: Install dependencies
      run: |
        pip install -r requirements.txt
        pip install pytest pytest-cov
    
    - name: Run unit tests
      run: |
        pytest tests/unit/ -v --cov=ml_pipeline
    
    - name: Run integration tests
      run: |
        pytest tests/integration/ -v
    
    - name: Upload coverage to Codecov
      uses: codecov/codecov-action@v3

  build:
    needs: test
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up Docker Buildx
      uses: docker/setup-buildx-action@v2
    
    - name: Login to Container Registry
      uses: docker/login-action@v2
      with:
        registry: ${{ secrets.CONTAINER_REGISTRY }}
        username: ${{ secrets.REGISTRY_USERNAME }}
        password: ${{ secrets.REGISTRY_PASSWORD }}
    
    - name: Build and push pipeline components
      uses: docker/build-push-action@v4
      with:
        context: ./ml-pipeline
        file: ./ml-pipeline/Dockerfile
        push: true
        tags: |
          company/ml-pipeline:${{ github.sha }}
          company/ml-pipeline:latest
        platforms: linux/amd64,linux/arm64

  deploy-staging:
    needs: build
    runs-on: ubuntu-latest
    environment: staging
    steps:
    - uses: actions/checkout@v3
    
    - name: Deploy to staging
      run: |
        # 更新Kubernetes部署
        kubectl set image deployment/ml-pipeline \
          data-ingestion=company/ml-pipeline:${{ github.sha }} \
          feature-engineering=company/ml-pipeline:${{ github.sha }} \
          model-training=company/ml-pipeline:${{ github.sha }} \
          model-evaluation=company/ml-pipeline:${{ github.sha }} \
          -n ml-staging
        
        # 运行测试流水线
        kubectl create -f test-pipeline-staging.yaml
    
    - name: Wait for pipeline completion
      run: |
        timeout 3600 bash -c \"
        while [[ \\$(kubectl get workflow -n ml-staging --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-1:].status.phase}') != 'Succeeded' ]]; do
          echo 'Waiting for pipeline completion...'
          sleep 30
        done
        \"

  deploy-production:
    needs: deploy-staging
    runs-on: ubuntu-latest
    environment: production
    if: github.ref == 'refs/heads/main'
    steps:
    - uses: actions/checkout@v3
    
    - name: Manual approval
      uses: trstringer/manual-approval@v1
      with:
        secret: ${{ secrets.APPROVAL_TOKEN }}
        approvers: ml-team-lead,data-science-manager
        minimum_approvals: 1
    
    - name: Promote to production
      run: |
        # 蓝绿部署
        kubectl patch deployment ml-pipeline-blue -p \
          '{\"spec\":{\"template\":{\"spec\":{\"containers\":[{\"name\":\"data-ingestion\",\"image\":\"company/ml-pipeline:${{ github.sha }}\"}]}}}}'
        
        # 流量切换
        kubectl patch service ml-pipeline -p \
          '{\"spec\":{\"selector\":{\"version\":\"blue\"}}}'
```

---

## 四、生产环境最佳实践

### 4.1 流水线可靠性保障

```yaml
# pipeline-reliability.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: pipeline-health-check
spec:
  template:
    spec:
      containers:
      - name: health-check
        image: company/pipeline-tools:health-check-v1.0
        command:
        - /bin/sh
        - -c
        - |
          # 检查各组件健康状态
          COMPONENTS=(\"data-ingestion\" \"feature-engineering\" \"model-training\" \"model-evaluation\")
          
          for component in ${COMPONENTS[@]}; do
            echo \"Checking $component...\"
            
            # 检查Pod状态
            if ! kubectl get pods -l app=$component -n ml-pipeline | grep Running; then
              echo \"ERROR: $component is not running\"
              exit 1
            fi
            
            # 检查最近执行状态
            recent_workflows=$(kubectl get workflows -l component=$component --sort-by=.metadata.creationTimestamp -o jsonpath='{.items[-5:].status.phase}')
            success_count=$(echo $recent_workflows | grep -o Succeeded | wc -l)
            
            if [ $success_count -lt 4 ]; then
              echo \"WARNING: Low success rate for $component\"
            fi
          done
          
          echo \"All pipeline components are healthy\"
        env:
        - name: KUBECONFIG
          value: /etc/kubernetes/admin.conf
        volumeMounts:
        - name: kubeconfig
          mountPath: /etc/kubernetes
      restartPolicy: Never
      volumes:
      - name: kubeconfig
        configMap:
          name: kubeconfig-admin
```

### 4.2 成本优化策略

```python
# cost_optimizer.py
import boto3
import kubernetes
from datetime import datetime, timedelta

class MLPipelineCostOptimizer:
    def __init__(self):
        self.ec2_client = boto3.client('ec2')
        self.k8s_client = kubernetes.client.ApiClient()
        
    def optimize_spot_instances(self):
        \"\"\"优化Spot实例使用\"\"\"
        # 获取Spot实例价格历史
        pricing = self.ec2_client.describe_spot_price_history(
            InstanceTypes=['p3.2xlarge', 'p3.8xlarge'],
            ProductDescriptions=['Linux/UNIX'],
            StartTime=datetime.utcnow() - timedelta(hours=24)
        )
        
        # 选择最具性价比的实例类型
        best_instance = min(pricing['SpotPriceHistory'], 
                          key=lambda x: float(x['SpotPrice']))
        
        return {
            'instance_type': best_instance['InstanceType'],
            'spot_price': best_instance['SpotPrice'],
            'availability_zone': best_instance['AvailabilityZone']
        }
    
    def scale_pipeline_resources(self, pipeline_demand):
        \"\"\"根据流水线需求动态调整资源\"\"\"
        base_resources = {
            'data_ingestion': {'cpu': '1', 'memory': '2Gi'},
            'feature_engineering': {'cpu': '2', 'memory': '4Gi'},
            'model_training': {'cpu': '8', 'memory': '32Gi', 'gpu': '1'},
            'model_evaluation': {'cpu': '2', 'memory': '8Gi'}
        }
        
        # 根据需求调整资源请求
        optimized_resources = {}
        for component, resources in base_resources.items():
            demand_factor = pipeline_demand.get(component, 1.0)
            optimized_resources[component] = {
                'requests': {
                    'cpu': str(int(resources['cpu'].rstrip('m')) * demand_factor) + '000m',
                    'memory': str(int(resources['memory'].rstrip('Gi')) * demand_factor) + 'Gi'
                },
                'limits': {
                    'cpu': str(int(resources['cpu'].rstrip('m')) * demand_factor * 1.5) + '000m',
                    'memory': str(int(resources['memory'].rstrip('Gi')) * demand_factor * 1.5) + 'Gi'
                }
            }
            
            if 'gpu' in resources:
                optimized_resources[component]['requests']['nvidia.com/gpu'] = resources['gpu']
                optimized_resources[component]['limits']['nvidia.com/gpu'] = resources['gpu']
        
        return optimized_resources

# 使用示例
optimizer = MLPipelineCostOptimizer()
spot_config = optimizer.optimize_spot_instances()
resource_config = optimizer.scale_pipeline_resources({
    'model_training': 2.0,  # 高需求
    'data_ingestion': 0.5   # 低需求
})
```

---

## 五、流水线治理与安全

### 5.1 安全加固配置

```yaml
# pipeline-security.yaml
apiVersion: security.kubeflow.org/v1
kind: PipelineSecurityPolicy
metadata:
  name: ml-pipeline-security
spec:
  # 数据安全
  dataProtection:
    encryption:
      atRest: true
      inTransit: true
      algorithm: \"AES-256\"
    
    accessControl:
      enabled: true
      policies:
        - resource: \"s3://training-data/*\"
          principals: [\"ml-training-pipeline\"]
          actions: [\"s3:GetObject\", \"s3:PutObject\"]
          condition:
            StringEquals:
              \"s3:prefix\": [\"training/\", \"validation/\"]
    
    # 敏感数据脱敏
    dataMasking:
      enabled: true
      rules:
        - pattern: \"\\\\b\\\\d{11}\\\\b\"  # 身份证号
          replacement: \"***********\"
        - pattern: \"\\\\b1[3-9]\\\\d{9}\\\\b\"  # 手机号
          replacement: \"138****8888\"

  # 模型安全
  modelSecurity:
    scanning:
      enabled: true
      engines:
        - name: \"model-scanner\"
          type: \"static-analysis\"
          rules:
            - \"no_hardcoded_credentials\"
            - \"no_sensitive_data_in_model\"
            - \"model_integrity_check\"
    
    signing:
      enabled: true
      keyManagement:
        provider: \"vault\"
        keyName: \"model-signing-key\"
      
      verification:
        enabled: true
        required: true

  # 网络安全
  networkSecurity:
    isolation:
      enabled: true
      namespaces:
        - \"ml-pipeline\"
        - \"ml-training\"
        - \"ml-serving\"
    
    egressControl:
      enabled: true
      rules:
        - to:
            cidr: \"10.0.0.0/8\"
          ports:
            - protocol: TCP
              port: 443
        - to:
            dnsName: \"mlflow-tracking.company.com\"
          ports:
            - protocol: TCP
              port: 5000

  # 运行时安全
  runtimeSecurity:
    podSecurityStandards:
      enforce: \"restricted\"
      audit: \"baseline\"
      warn: \"baseline\"
    
    seccompProfiles:
      enabled: true
      profile: \"runtime/default\"
    
    appArmorProfiles:
      enabled: true
      profiles:
        - \"docker-default\"
```

---

**维护者**: MLOps Team | **最后更新**: 2026-02 | **版本**: v1.0