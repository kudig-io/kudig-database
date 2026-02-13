# AI模型部署与生命周期管理

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [KServe](https://kserve.github.io/website/) | [Seldon Core](https://docs.seldon.io/projects/seldon-core/) | [BentoML](https://docs.bentoml.org/)

## 一、模型部署架构概览

### 1.1 生产级模型部署架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        AI Model Deployment Architecture                             │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                            模型管理平台 (Model Management)                      │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │  Model      │  │  Model      │  │  Model      │  │  Model      │          │  │
│  │  │  Registry   │  │  Versioning │  │  Metadata   │  │  Governance │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 存储管理   │  │ • 版本控制   │  │ • 元数据    │  │ • 审计追踪   │          │  │
│  │  │ • 权限控制   │  │ • 血缘关系   │  │ • 标签分类   │  │ • 合规检查   │          │  │
│  │  │ • 搜索发现   │  │ • A/B测试    │  │ • 性能指标   │  │ • 安全扫描   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                          部署编排层 (Deployment Orchestration)                 │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   KServe    │  │ Seldon Core │  │  BentoML    │  │  Custom     │          │  │
│  │  │             │  │             │  │             │  │  Operator   │          │  │
│  │  │ • Inference │  │ • Graph     │  │ • Serving   │  │ • Domain    │          │  │
│  │  │ • Canary    │  │ • Ensemble  │  │ • Batch     │  │ • Specific  │          │  │
│  │  │ • Blue/Green│  │ • Explain   │  │ • Streaming │  │ • Logic     │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                          服务管理层 (Service Management)                       │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   Service   │  │   Gateway   │  │   Traffic   │  │   Security  │          │  │
│  │  │   Mesh      │  │   (Istio)   │  │   Control   │  │   (mTLS)    │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 服务发现   │  │ • 路由管理   │  │ • 负载均衡   │  │ • 身份认证   │          │  │
│  │  │ • 流量治理   │  │ • 熔断降级   │  │ • 故障转移   │  │ • 授权鉴权   │          │  │
│  │  │ • 链路追踪   │  │ • 限流控制   │  │ • 健康检查   │  │ • 加密传输   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                          基础设施层 (Infrastructure)                          │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   K8s       │  │    GPU      │  │   Storage   │  │   Network   │          │  │
│  │  │   Cluster   │  │   Nodes     │  │   System    │  │   Fabric    │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 调度管理   │  │ • 资源池    │  │ • PVC/PV    │  │ • CNI插件    │          │  │
│  │  │ • 自动扩缩   │  │ • 拓扑优化   │  │ • 快照备份   │  │ • RDMA网络   │          │  │
│  │  │ • 故障恢复   │  │ • 功耗管理   │  │ • 缓存加速   │  │ • 负载均衡   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 部署模式对比

| 部署模式 | 适用场景 | 优势 | 劣势 | 复杂度 |
|----------|----------|------|------|--------|
| **Serverless** | 突发流量、开发测试 | 自动扩缩、成本优化 | 冷启动延迟 | ⭐⭐ |
| **Deployment** | 稳定在线服务 | 简单可靠、易于调试 | 资源浪费 | ⭐ |
| **StatefulSet** | 有状态模型服务 | 数据持久化、有序部署 | 复杂度高 | ⭐⭐⭐ |
| **DaemonSet** | 节点本地服务 | 本地缓存、低延迟 | 资源利用率低 | ⭐⭐ |
| **Job/CronJob** | 批处理推理 | 一次性任务、定时执行 | 不适合在线服务 | ⭐⭐ |

---

## 二、KServe生产部署

### 2.1 完整部署配置

```yaml
# kserve-production.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: ai-models
  labels:
    istio-injection: enabled
    serving.kserve.io/inferenceservice: enabled
---
# KServe核心组件
apiVersion: operator.kserve.io/v1alpha1
kind: KServe
metadata:
  name: kserve-instance
  namespace: ai-models
spec:
  # 启用组件
  inferenceService:
    enabled: true
    resources:
      limits:
        cpu: "2"
        memory: 4Gi
      requests:
        cpu: "1"
        memory: 2Gi
        
  # 模型代理配置
  agent:
    enabled: true
    resources:
      limits:
        cpu: "500m"
        memory: 1Gi
      requests:
        cpu: "250m"
        memory: 512Mi
        
  # 存储初始化器
  storageInitializer:
    enabled: true
    image: kserve/storage-initializer:v0.12.0
    resources:
      limits:
        cpu: "500m"
        memory: 1Gi
      requests:
        cpu: "250m"
        memory: 512Mi
---
# InferenceService示例 - LLM推理
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama3-70b-inference
  namespace: ai-models
  annotations:
    # 启用Canary部署
    autoscaling.knative.dev/class: kpa.autoscaling.knative.dev
    autoscaling.knative.dev/metric: concurrency
    autoscaling.knative.dev/target: "10"
    autoscaling.knative.dev/minScale: "2"
    autoscaling.knative.dev/maxScale: "20"
spec:
  predictor:
    # 模型版本管理
    model:
      modelFormat:
        name: pytorch
        version: "2.1"
      runtime: kserve-lgbserver
      storageUri: s3://models-store/llama3/70b/v1.2
      protocolVersion: v2
      resources:
        limits:
          cpu: "8"
          memory: 64Gi
          nvidia.com/gpu: "4"
        requests:
          cpu: "4"
          memory: 32Gi
          nvidia.com/gpu: "4"
      env:
        - name: MODEL_NAME
          value: llama3-70b
        - name: TRANSFORMERS_CACHE
          value: /cache
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: huggingface-secret
              key: token
              
  # Transformer预处理
  transformer:
    containers:
      - image: custom/llm-transformer:v1.0
        name: transformer
        ports:
          - containerPort: 8080
            protocol: TCP
        env:
          - name: PREPROCESS_CONFIG
            value: "config/preprocess.yaml"
        resources:
          limits:
            cpu: "2"
            memory: 4Gi
          requests:
            cpu: "1"
            memory: 2Gi
            
  # explainer配置
  explainer:
    type: LIME
    containers:
      - image: kserve/alibi-explainer:v0.12.0
        name: explainer
        env:
          - name: EXPLAINER_MODEL_NAME
            value: llama3-70b
```

### 2.2 模型版本管理策略

```yaml
# model-versioning-strategy.yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: model-canary-deployment
  namespace: ai-models
spec:
  # 蓝绿部署配置
  predictor:
    canaryTrafficPercent: 10  # 10%流量到新版本
    model:
      # 当前稳定版本
      stable:
        name: llama3-70b-v1.0
        storageUri: s3://models/llama3-70b/v1.0
        resources:
          requests:
            nvidia.com/gpu: "4"
            
      # 新版本候选
      canary:
        name: llama3-70b-v1.1
        storageUri: s3://models/llama3-70b/v1.1
        resources:
          requests:
            nvidia.com/gpu: "4"
            
  # A/B测试配置
  router:
    traffic:
      - revisionName: llama3-70b-v1.0
        percent: 70
        headers:
          x-test-group: control
      - revisionName: llama3-70b-v1.1
        percent: 30
        headers:
          x-test-group: treatment
```

---

## 三、模型注册中心建设

### 3.1 MLflow模型注册表

```yaml
# mlflow-registry.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
  namespace: ai-models
spec:
  replicas: 2
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
        image: ghcr.io/mlflow/mlflow:2.9.2
        ports:
        - containerPort: 5000
        env:
        - name: MLFLOW_S3_ENDPOINT_URL
          value: "http://minio:9000"
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: minio-secret
              key: access-key
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: minio-secret
              key: secret-key
        - name: MLFLOW_BACKEND_STORE_URI
          value: "postgresql://mlflow:password@postgres-mlflow:5432/mlflow"
        volumeMounts:
        - name: models-volume
          mountPath: /mlflow-artifacts
        resources:
          requests:
            cpu: "1"
            memory: 2Gi
          limits:
            cpu: "2"
            memory: 4Gi
---
apiVersion: v1
kind: Service
metadata:
  name: mlflow-service
  namespace: ai-models
spec:
  selector:
    app: mlflow
  ports:
  - port: 5000
    targetPort: 5000
  type: ClusterIP
```

### 3.2 模型元数据管理

```python
# model_metadata_manager.py
import mlflow
from datetime import datetime
from typing import Dict, Any

class ModelMetadataManager:
    def __init__(self):
        self.client = mlflow.tracking.MlflowClient()
        
    def register_model_with_metadata(self, 
                                   model_uri: str,
                                   model_name: str,
                                   metadata: Dict[str, Any]):
        """注册模型并附加元数据"""
        
        # 注册模型
        model_version = mlflow.register_model(model_uri, model_name)
        
        # 添加自定义标签
        self.client.set_model_version_tag(
            name=model_name,
            version=model_version.version,
            key="framework",
            value=metadata.get("framework", "unknown")
        )
        
        self.client.set_model_version_tag(
            name=model_name,
            version=model_version.version,
            key="gpu_memory_required",
            value=str(metadata.get("gpu_memory_gb", 0))
        )
        
        self.client.set_model_version_tag(
            name=model_name,
            version=model_version.version,
            key="inference_latency_ms",
            value=str(metadata.get("latency_ms", 0))
        )
        
        # 记录性能指标
        with mlflow.start_run():
            mlflow.log_params({
                "model_name": model_name,
                "version": model_version.version,
                "registered_at": datetime.now().isoformat(),
                **metadata.get("performance_metrics", {})
            })
            
        return model_version

# 使用示例
manager = ModelMetadataManager()
model_info = manager.register_model_with_metadata(
    model_uri="runs:/abcd1234/model",
    model_name="llama3-70b",
    metadata={
        "framework": "pytorch",
        "gpu_memory_gb": 80,
        "latency_ms": 150,
        "performance_metrics": {
            "accuracy": 0.95,
            "throughput": 50,
            "max_tokens": 4096
        }
    }
)
```

---

## 四、部署流水线自动化

### 4.1 CI/CD流水线配置

```yaml
# .github/workflows/model-deployment.yaml
name: Model Deployment Pipeline
on:
  push:
    branches: [main]
    paths:
      - 'models/**'
  workflow_dispatch:

jobs:
  model-validation:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.9'
        
    - name: Install dependencies
      run: |
        pip install -r requirements.txt
        pip install pytest mlflow
    
    - name: Run model validation
      run: |
        pytest tests/model_validation/
        
    - name: Performance benchmark
      run: |
        python scripts/benchmark_model.py --model-path ./model
    
  deploy-staging:
    needs: model-validation
    runs-on: ubuntu-latest
    environment: staging
    steps:
    - uses: actions/checkout@v4
    
    - name: Deploy to staging
      run: |
        # 部署到预发环境
        kubectl apply -f k8s/staging/model-deployment.yaml
        kubectl rollout status deployment/model-staging
        
    - name: Run integration tests
      run: |
        pytest tests/integration/ --env staging
        
  canary-deployment:
    needs: deploy-staging
    runs-on: ubuntu-latest
    environment: production
    steps:
    - uses: actions/checkout@v4
    
    - name: Deploy canary release
      run: |
        # 更新canary流量百分比
        kubectl patch inferenceservice model-production \
          -p '{"spec":{"predictor":{"canaryTrafficPercent": 10}}}' \
          --type=merge
          
    - name: Monitor canary metrics
      run: |
        # 监控关键指标
        python scripts/monitor_canary.py --duration 30m
        
    - name: Promote to production
      if: success()
      run: |
        # 全量上线
        kubectl patch inferenceservice model-production \
          -p '{"spec":{"predictor":{"canaryTrafficPercent": 100}}}' \
          --type=merge
```

### 4.2 部署质量门禁

```python
# deployment_gate.py
import requests
import time
from typing import Dict, List

class DeploymentQualityGate:
    def __init__(self, service_endpoint: str, thresholds: Dict):
        self.endpoint = service_endpoint
        self.thresholds = thresholds
        self.metrics_client = self._setup_metrics_client()
        
    def _setup_metrics_client(self):
        """初始化监控客户端"""
        # 连接到Prometheus或其他监控系统
        pass
        
    def check_deployment_health(self) -> bool:
        """检查部署健康状况"""
        checks = [
            self._check_endpoint_availability(),
            self._check_response_time(),
            self._check_error_rate(),
            self._check_resource_utilization(),
            self._check_business_metrics()
        ]
        
        return all(checks)
        
    def _check_endpoint_availability(self) -> bool:
        """检查服务端点可用性"""
        try:
            response = requests.get(f"{self.endpoint}/health", timeout=5)
            return response.status_code == 200
        except Exception:
            return False
            
    def _check_response_time(self) -> bool:
        """检查响应时间"""
        # 从监控系统获取P99延迟
        p99_latency = self.metrics_client.query("histogram_quantile(0.99, ...)")
        return p99_latency <= self.thresholds.get("max_latency_ms", 500)
        
    def _check_error_rate(self) -> bool:
        """检查错误率"""
        error_rate = self.metrics_client.query("rate(http_requests_total{status=~'5..'}[5m])")
        return error_rate <= self.thresholds.get("max_error_rate", 0.01)
        
    def _check_resource_utilization(self) -> bool:
        """检查资源利用率"""
        # 检查CPU、内存、GPU利用率
        gpu_util = self.metrics_client.query("avg(DCGM_FI_DEV_GPU_UTIL)")
        return gpu_util >= self.thresholds.get("min_gpu_utilization", 30)
        
    def _check_business_metrics(self) -> bool:
        """检查业务指标"""
        # 检查准确率、召回率等业务指标
        accuracy = self.metrics_client.query("model_accuracy")
        return accuracy >= self.thresholds.get("min_accuracy", 0.9)

# 使用示例
gate = DeploymentQualityGate(
    service_endpoint="http://model-service.ai-models",
    thresholds={
        "max_latency_ms": 300,
        "max_error_rate": 0.005,
        "min_gpu_utilization": 40,
        "min_accuracy": 0.92
    }
)

if gate.check_deployment_health():
    print("✅ 部署质量检查通过")
else:
    print("❌ 部署质量检查失败")
    # 回滚操作
```

---

## 五、故障恢复与容灾

### 5.1 自动故障检测

```yaml
# fault-detection.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: model-health-checker
  namespace: ai-models
spec:
  replicas: 1
  selector:
    matchLabels:
      app: health-checker
  template:
    metadata:
      labels:
        app: health-checker
    spec:
      containers:
      - name: health-checker
        image: custom/model-health-checker:v1.0
        env:
        - name: TARGET_SERVICES
          value: "llama3-inference,llm-router,model-registry"
        - name: CHECK_INTERVAL
          value: "30s"
        - name: FAILURE_THRESHOLD
          value: "3"
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
---
# 健康检查脚本
apiVersion: batch/v1
kind: CronJob
metadata:
  name: periodic-model-validation
  namespace: ai-models
spec:
  schedule: "*/15 * * * *"  # 每15分钟执行一次
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: validator
            image: custom/model-validator:v1.0
            command:
            - python
            - /scripts/validate_models.py
            env:
            - name: VALIDATION_DATASET
              value: "s3://datasets/validation/latest"
            - name: ALERT_WEBHOOK
              value: "https://hooks.slack.com/services/..."
            resources:
              requests:
                cpu: "500m"
                memory: "1Gi"
          restartPolicy: OnFailure
```

### 5.2 自动回滚策略

```python
# auto_rollback.py
import logging
import time
from kubernetes import client, config
from prometheus_api_client import PrometheusConnect

logger = logging.getLogger(__name__)

class AutoRollbackManager:
    def __init__(self, service_name: str, namespace: str):
        config.load_kube_config()
        self.apps_v1 = client.AppsV1Api()
        self.prometheus = PrometheusConnect(url="http://prometheus:9090")
        self.service_name = service_name
        self.namespace = namespace
        
    def monitor_and_rollback(self, deployment_name: str, rollback_window_minutes: int = 10):
        """监控部署并在出现问题时自动回滚"""
        
        # 获取当前部署状态
        deployment = self.apps_v1.read_namespaced_deployment(deployment_name, self.namespace)
        current_replicas = deployment.spec.replicas
        
        # 监控窗口期内的指标
        start_time = time.time() - (rollback_window_minutes * 60)
        
        while time.time() - start_time < (rollback_window_minutes * 60):
            if self._should_rollback(deployment_name):
                logger.warning(f"检测到异常，触发自动回滚: {deployment_name}")
                self._perform_rollback(deployment_name)
                return True
                
            time.sleep(30)  # 每30秒检查一次
            
        logger.info(f"部署稳定，无需回滚: {deployment_name}")
        return False
        
    def _should_rollback(self, deployment_name: str) -> bool:
        """判断是否需要回滚"""
        
        # 检查错误率
        error_query = f'sum(rate(http_requests_total{{deployment="{deployment_name}",status=~"5.."}}[5m]))'
        error_rate = self._query_prometheus(error_query)
        if error_rate > 0.05:  # 错误率超过5%
            logger.warning(f"错误率过高: {error_rate}")
            return True
            
        # 检查延迟
        latency_query = f'histogram_quantile(0.99, rate(http_request_duration_seconds_bucket{{deployment="{deployment_name}"}}[5m]))'
        latency_99 = self._query_prometheus(latency_query)
        if latency_99 > 2.0:  # P99延迟超过2秒
            logger.warning(f"延迟过高: {latency_99}s")
            return True
            
        # 检查可用性
        availability_query = f'avg(up{{deployment="{deployment_name}"}})'
        availability = self._query_prometheus(availability_query)
        if availability < 0.95:  # 可用性低于95%
            logger.warning(f"可用性不足: {availability}")
            return True
            
        return False
        
    def _perform_rollback(self, deployment_name: str):
        """执行回滚操作"""
        try:
            # 回滚到上一个版本
            rollback_body = {
                "kind": "DeploymentRollback",
                "apiVersion": "apps/v1",
                "name": deployment_name
            }
            
            self.apps_v1.create_namespaced_deployment_rollback(
                name=deployment_name,
                namespace=self.namespace,
                body=rollback_body
            )
            
            logger.info(f"成功回滚部署: {deployment_name}")
            
            # 发送告警通知
            self._send_alert_notification(deployment_name, "automatic_rollback")
            
        except Exception as e:
            logger.error(f"回滚失败: {e}")
            raise
            
    def _query_prometheus(self, query: str) -> float:
        """查询Prometheus指标"""
        try:
            result = self.prometheus.custom_query(query=query)
            if result and len(result) > 0:
                return float(result[0]['value'][1])
            return 0.0
        except Exception as e:
            logger.error(f"Prometheus查询失败: {e}")
            return 0.0
            
    def _send_alert_notification(self, deployment_name: str, reason: str):
        """发送告警通知"""
        # 实现通知逻辑（Slack、邮件等）
        pass

# 使用示例
rollback_manager = AutoRollbackManager("llama3-inference", "ai-models")
rollback_manager.monitor_and_rollback("llama3-inference-deployment", rollback_window_minutes=15)
```

---

## 六、运维最佳实践

### 6.1 部署检查清单

✅ **预部署检查**
- [ ] 模型通过所有验证测试
- [ ] 性能基准测试完成
- [ ] 安全扫描和漏洞检查通过
- [ ] 成本评估和预算审批完成
- [ ] 回滚计划制定并测试

✅ **部署过程中**
- [ ] Canary流量逐步增加
- [ ] 关键指标实时监控
- [ ] 用户反馈及时收集
- [ ] 异常情况快速响应
- [ ] 部署日志完整记录

✅ **部署后验证**
- [ ] 功能测试通过
- [ ] 性能指标达标
- [ ] 用户体验良好
- [ ] 监控告警正常
- [ ] 文档更新完成

### 6.2 常见问题处理

**模型加载失败**
```bash
# 检查存储访问权限
kubectl get pvc -n ai-models
kubectl describe pv <pv-name>

# 验证模型文件完整性
kubectl exec -it <pod-name> -n ai-models -- ls -la /mnt/models/

# 查看加载日志
kubectl logs <pod-name> -n ai-models -c model-loader
```

**推理性能下降**
```bash
# 检查GPU资源使用
kubectl top nodes --selector=nvidia.com/gpu.present=true
dcgmi dmon -e 1001,1002,1003 -i 0

# 分析瓶颈
kubectl exec -it <pod-name> -n ai-models -- nvidia-smi
kubectl exec -it <pod-name> -n ai-models -- nvtop
```

**部署卡住不 progressing**
```bash
# 检查部署状态
kubectl describe deployment <deployment-name> -n ai-models
kubectl get events --field-selector involvedObject.name=<deployment-name>

# 查看Pod状态详情
kubectl describe pods -l app=<app-name> -n ai-models
```

---