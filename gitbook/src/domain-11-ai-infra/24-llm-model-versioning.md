# 24 - LLM模型版本管理与治理

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 专家级 | **参考**: [MLflow](https://mlflow.org/) | [DVC](https://dvc.org/) | [HuggingFace Hub](https://huggingface.co/) | CNCF Model Registry

## 一、企业级模型版本治理体系

### 1.1 模型生命周期管理架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                     Enterprise Model Lifecycle Management                           │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Model Development Phase                             │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Experiment  │  │ Training    │  │ Validation  │  │ Model       │          │  │
│  │  │ Tracking    │  │ Pipeline    │  │ Testing     │  │ Registration │          │  │
│  │  │ (MLflow)    │  │ (Kubeflow)  │  │ (Unit Test) │  │ (Registry)   │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Model Staging Phase                                 │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ A/B Testing │  │ Performance │  │ Security    │  │ Compliance  │          │  │
│  │  │ (Canary)    │  │ Benchmark   │  │ Scanning    │  │ Check       │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Model Production Phase                              │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Deployment  │  │ Monitoring  │  │ Auto        │  │ Rollback    │          │  │
│  │  │ (Blue/Green)│  │ (Prometheus)│  │ Promotion   │  │ (Automated) │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 企业级版本管理策略

| 策略维度 | 实施要点 | 技术实现 | 运维考虑 |
|----------|----------|----------|----------|
| **语义化版本** | MAJOR.MINOR.PATCH | Git标签 + MLflow版本 | 自动化版本生成 |
| **分支策略** | Git Flow + 模型分支 | feature/model-*分支 | 分支合并审查 |
| **环境隔离** | dev/staging/prod三环境 | 命名空间隔离 | 资源配额管理 |
| **审批流程** | 多级审批机制 | GitOps + Approvals | 审批时效控制 |
| **回滚机制** | 一键回滚 + 渐进式 | Blue/Green + Canary | 回滚时间窗 |
| **审计追踪** | 完整变更日志 | Git提交 + MLflow日志 | 合规性要求 |

## 二、MLflow企业级部署

### 2.1 高可用MLflow架构

```yaml
# mlflow-production-deployment.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mlflow-postgres-primary
  namespace: mlflow
spec:
  serviceName: mlflow-postgres
  replicas: 3
  selector:
    matchLabels:
      app: mlflow-postgres
      role: primary
  template:
    metadata:
      labels:
        app: mlflow-postgres
        role: primary
    spec:
      containers:
      - name: postgres
        image: postgres:15-alpine
        env:
        - name: POSTGRES_DB
          value: mlflow
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: mlflow-db-secret
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mlflow-db-secret
              key: password
        - name: PGDATA
          value: /var/lib/postgresql/data/pgdata
        ports:
        - containerPort: 5432
        volumeMounts:
        - name: postgres-storage
          mountPath: /var/lib/postgresql/data
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
        livenessProbe:
          exec:
            command:
            - pg_isready
            - -U
            - postgres
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command:
            - pg_isready
            - -U
            - postgres
          initialDelaySeconds: 5
          periodSeconds: 5
  volumeClaimTemplates:
  - metadata:
      name: postgres-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
      storageClassName: fast-ssd

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
  namespace: mlflow
spec:
  replicas: 5
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
        image: ghcr.io/mlflow/mlflow:v2.10.0
        command:
        - mlflow
        - server
        - --host=0.0.0.0
        - --port=5000
        - --backend-store-uri=postgresql://$(DB_USER):$(DB_PASS)@mlflow-postgres-headless:5432/mlflow
        - --default-artifact-root=s3://mlflow-artifacts-production/
        - --workers=4
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: mlflow-db-secret
              key: username
        - name: DB_PASS
          valueFrom:
            secretKeyRef:
              name: mlflow-db-secret
              key: password
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: mlflow-s3-secret
              key: access-key
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: mlflow-s3-secret
              key: secret-key
        - name: MLFLOW_S3_ENDPOINT_URL
          value: "https://s3.amazonaws.com"
        ports:
        - containerPort: 5000
        resources:
          requests:
            cpu: "500m"
            memory: "1Gi"
          limits:
            cpu: "1"
            memory: "2Gi"
        livenessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 60
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 30
          periodSeconds: 10
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values:
                  - mlflow-server
              topologyKey: kubernetes.io/hostname

---
apiVersion: v1
kind: Service
metadata:
  name: mlflow-service
  namespace: mlflow
spec:
  selector:
    app: mlflow-server
  ports:
  - port: 80
    targetPort: 5000
  type: LoadBalancer
  loadBalancerSourceRanges:
  - 10.0.0.0/8
  - 172.16.0.0/12
  - 192.168.0.0/16

---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: mlflow-ingress
  namespace: mlflow
  annotations:
    nginx.ingress.kubernetes.io/auth-type: basic
    nginx.ingress.kubernetes.io/auth-secret: mlflow-basic-auth
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  tls:
  - hosts:
    - mlflow.company.com
    secretName: mlflow-tls
  rules:
  - host: mlflow.company.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: mlflow-service
            port:
              number: 80
```

### 2.2 模型注册与元数据管理

```python
# enterprise_model_registry.py
import mlflow
from mlflow.entities import ViewType
from mlflow.tracking import MlflowClient
from datetime import datetime, timedelta
import json
import yaml
from typing import Dict, List, Optional
import pandas as pd
from dataclasses import dataclass

@dataclass
class ModelMetadata:
    """模型元数据结构"""
    model_name: str
    version: str
    description: str
    author: str
    team: str
    created_at: datetime
    training_dataset: str
    dataset_version: str
    hyperparameters: Dict
    performance_metrics: Dict
    hardware_requirements: Dict
    deployment_targets: List[str]
    compliance_tags: List[str]
    dependencies: List[str]
    model_card: str

class EnterpriseModelRegistry:
    def __init__(self, tracking_uri: str, registry_uri: str = None):
        self.client = MlflowClient(tracking_uri=tracking_uri)
        mlflow.set_tracking_uri(tracking_uri)
        if registry_uri:
            mlflow.set_registry_uri(registry_uri)
        
    def register_model_with_governance(self, 
                                     model_uri: str, 
                                     model_name: str,
                                     metadata: ModelMetadata) -> str:
        """注册模型并应用治理策略"""
        
        # 开始注册流程
        with mlflow.start_run() as run:
            # 记录模型元数据
            self._log_model_metadata(metadata)
            
            # 记录性能指标
            for metric_name, value in metadata.performance_metrics.items():
                mlflow.log_metric(metric_name, value)
            
            # 记录超参数
            mlflow.log_params(metadata.hyperparameters)
            
            # 记录依赖项
            mlflow.log_dict(
                {"dependencies": metadata.dependencies},
                "requirements.json"
            )
            
            # 注册模型
            model_info = mlflow.register_model(
                model_uri=model_uri,
                name=model_name,
                tags={
                    "team": metadata.team,
                    "author": metadata.author,
                    "compliance": ",".join(metadata.compliance_tags),
                    "hardware": json.dumps(metadata.hardware_requirements)
                }
            )
            
            # 创建模型卡片
            self._create_model_card(model_info, metadata)
            
            # 触发治理检查
            governance_passed = self._run_governance_checks(model_info, metadata)
            
            if governance_passed:
                # 自动晋级到Staging
                self.client.transition_model_version_stage(
                    name=model_name,
                    version=model_info.version,
                    stage="Staging"
                )
                print(f"Model {model_name} v{model_info.version} registered and moved to Staging")
            else:
                print(f"Model {model_name} v{model_info.version} failed governance checks")
                
            return f"{model_name}/{model_info.version}"
    
    def _log_model_metadata(self, metadata: ModelMetadata):
        """记录模型元数据"""
        # 记录核心元数据
        mlflow.set_tag("model.description", metadata.description)
        mlflow.set_tag("model.author", metadata.author)
        mlflow.set_tag("model.team", metadata.team)
        mlflow.set_tag("model.dataset", metadata.training_dataset)
        mlflow.set_tag("model.dataset_version", metadata.dataset_version)
        
        # 记录硬件需求
        mlflow.set_tag("hardware.cpu", metadata.hardware_requirements.get("cpu", "unknown"))
        mlflow.set_tag("hardware.memory", metadata.hardware_requirements.get("memory", "unknown"))
        mlflow.set_tag("hardware.gpu", metadata.hardware_requirements.get("gpu", "none"))
        
        # 记录合规标签
        mlflow.set_tag("compliance.tags", ",".join(metadata.compliance_tags))
        
    def _create_model_card(self, model_info, metadata: ModelMetadata):
        """创建模型卡片"""
        model_card = {
            "model_identity": {
                "name": metadata.model_name,
                "version": metadata.version,
                "created_at": metadata.created_at.isoformat(),
                "author": metadata.author,
                "team": metadata.team
            },
            "model_details": {
                "description": metadata.description,
                "version": metadata.version,
                "license": "Apache 2.0",
                "contact": f"{metadata.author}@company.com"
            },
            "datasets": {
                "training_data": {
                    "name": metadata.training_dataset,
                    "version": metadata.dataset_version,
                    "description": "Training dataset used for model development"
                }
            },
            "testing": {
                "performance_metrics": metadata.performance_metrics,
                "validation_approach": "Cross-validation with holdout set"
            },
            "model_parameters": {
                "hyperparameters": metadata.hyperparameters,
                "model_architecture": "Transformer-based"
            },
            "considerations": {
                "limitations": [
                    "Performance may vary with out-of-distribution data",
                    "Requires specific hardware configuration"
                ],
                "tradeoffs": [
                    "Higher accuracy with increased computational requirements"
                ],
                "ethical_considerations": [
                    "Potential bias in training data",
                    "Privacy considerations for input data"
                ]
            },
            "deployment": {
                "hardware_requirements": metadata.hardware_requirements,
                "deployment_targets": metadata.deployment_targets,
                "dependencies": metadata.dependencies
            }
        }
        
        # 保存模型卡片
        mlflow.log_dict(model_card, "model_card.json")
        
    def _run_governance_checks(self, model_info, metadata: ModelMetadata) -> bool:
        """运行治理检查"""
        checks_passed = []
        
        # 1. 性能基线检查
        baseline_checks = self._check_performance_baseline(metadata)
        checks_passed.append(("performance_baseline", baseline_checks))
        
        # 2. 合规性检查
        compliance_checks = self._check_compliance(metadata)
        checks_passed.append(("compliance", compliance_checks))
        
        # 3. 安全扫描
        security_checks = self._check_security(model_info)
        checks_passed.append(("security", security_checks))
        
        # 4. 依赖项检查
        dependency_checks = self._check_dependencies(metadata)
        checks_passed.append(("dependencies", dependency_checks))
        
        # 汇总结果
        all_passed = all(check["passed"] for _, check in checks_passed)
        
        # 记录检查结果
        mlflow.set_tag("governance.checks", json.dumps({
            "timestamp": datetime.now().isoformat(),
            "results": checks_passed,
            "overall_status": "passed" if all_passed else "failed"
        }))
        
        return all_passed
    
    def _check_performance_baseline(self, metadata: ModelMetadata) -> Dict:
        """检查性能基线"""
        required_metrics = ["accuracy", "precision", "recall", "f1_score"]
        actual_metrics = list(metadata.performance_metrics.keys())
        
        missing_metrics = set(required_metrics) - set(actual_metrics)
        baseline_met = all(
            metadata.performance_metrics.get(metric, 0) >= 0.8 
            for metric in required_metrics 
            if metric in metadata.performance_metrics
        )
        
        return {
            "passed": len(missing_metrics) == 0 and baseline_met,
            "details": {
                "missing_metrics": list(missing_metrics),
                "baseline_met": baseline_met,
                "required_metrics": required_metrics
            }
        }
    
    def _check_compliance(self, metadata: ModelMetadata) -> Dict:
        """合规性检查"""
        required_compliance = ["GDPR", "SOC2"]
        has_required = all(tag in metadata.compliance_tags for tag in required_compliance)
        
        return {
            "passed": has_required,
            "details": {
                "required_compliance": required_compliance,
                "provided_compliance": metadata.compliance_tags,
                "missing_compliance": [tag for tag in required_compliance if tag not in metadata.compliance_tags]
            }
        }
    
    def _check_security(self, model_info) -> Dict:
        """安全检查"""
        # 模拟安全扫描
        vulnerabilities_found = []  # 实际应该调用安全扫描工具
        
        return {
            "passed": len(vulnerabilities_found) == 0,
            "details": {
                "vulnerabilities_found": vulnerabilities_found,
                "scan_timestamp": datetime.now().isoformat()
            }
        }
    
    def _check_dependencies(self, metadata: ModelMetadata) -> Dict:
        """依赖项检查"""
        # 检查是否有已知的安全漏洞依赖
        insecure_deps = []  # 实际应该检查CVE数据库
        
        return {
            "passed": len(insecure_deps) == 0,
            "details": {
                "total_dependencies": len(metadata.dependencies),
                "insecure_dependencies": insecure_deps
            }
        }
    
    def promote_to_production(self, model_name: str, version: str) -> bool:
        """将模型晋级到生产环境"""
        try:
            # 获取当前生产版本
            current_prod = self.client.get_latest_versions(model_name, stages=["Production"])
            
            # 降级当前生产版本到归档
            if current_prod:
                self.client.transition_model_version_stage(
                    name=model_name,
                    version=current_prod[0].version,
                    stage="Archived"
                )
            
            # 晋级新版本到生产
            self.client.transition_model_version_stage(
                name=model_name,
                version=version,
                stage="Production"
            )
            
            print(f"Model {model_name} v{version} promoted to Production")
            return True
            
        except Exception as e:
            print(f"Failed to promote model: {e}")
            return False
    
    def get_model_lineage(self, model_name: str, version: str) -> Dict:
        """获取模型血缘关系"""
        model_version = self.client.get_model_version(model_name, version)
        run_id = model_version.run_id
        
        # 获取训练运行信息
        run = self.client.get_run(run_id)
        
        lineage = {
            "model": {
                "name": model_name,
                "version": version,
                "created_at": model_version.creation_timestamp
            },
            "training_run": {
                "run_id": run_id,
                "start_time": run.info.start_time,
                "end_time": run.info.end_time,
                "user_id": run.info.user_id
            },
            "dataset": {
                "name": run.data.tags.get("model.dataset", "unknown"),
                "version": run.data.tags.get("model.dataset_version", "unknown")
            },
            "parameters": dict(run.data.params),
            "metrics": dict(run.data.metrics),
            "artifacts": [artifact.path for artifact in self.client.list_artifacts(run_id)]
        }
        
        return lineage

# 使用示例
if __name__ == "__main__":
    # 初始化注册表
    registry = EnterpriseModelRegistry(
        tracking_uri="http://mlflow.company.com",
        registry_uri="models://mlflow.company.com"
    )
    
    # 创建模型元数据
    metadata = ModelMetadata(
        model_name="customer-churn-predictor",
        version="2.1.0",
        description="Customer churn prediction model with improved accuracy",
        author="data-science-team",
        team="ml-platform",
        created_at=datetime.now(),
        training_dataset="customer-interactions-v2",
        dataset_version="2024-Q1",
        hyperparameters={
            "learning_rate": 0.001,
            "batch_size": 128,
            "epochs": 50,
            "hidden_layers": 3
        },
        performance_metrics={
            "accuracy": 0.92,
            "precision": 0.89,
            "recall": 0.91,
            "f1_score": 0.90
        },
        hardware_requirements={
            "cpu": "2 cores",
            "memory": "4GB",
            "gpu": "1x T4"
        },
        deployment_targets=["kubernetes", "sagemaker"],
        compliance_tags=["GDPR", "SOC2", "ISO27001"],
        dependencies=["scikit-learn==1.3.0", "pandas==2.0.3", "numpy==1.24.3"],
        model_card=""
    )
    
    # 注册模型
    model_uri = "runs:/abc123/model"
    registered_model = registry.register_model_with_governance(
        model_uri=model_uri,
        model_name="customer-churn-predictor",
        metadata=metadata
    )
    
    print(f"Registered model: {registered_model}")
```

### 2.3 自动化A/B测试框架

```python
# automated_ab_testing.py
import mlflow
from mlflow.tracking import MlflowClient
import numpy as np
import pandas as pd
from datetime import datetime, timedelta
import time
from typing import Dict, List, Tuple
import json

class AutomatedABTesting:
    def __init__(self, tracking_uri: str):
        self.client = MlflowClient(tracking_uri=tracking_uri)
        mlflow.set_tracking_uri(tracking_uri)
        
    def setup_canary_deployment(self, 
                              model_name: str,
                              baseline_version: str,
                              candidate_version: str,
                              traffic_split: float = 0.1) -> str:
        """设置金丝雀部署"""
        
        # 创建A/B测试实验
        experiment_name = f"ab-test-{model_name}-{datetime.now().strftime('%Y%m%d')}"
        experiment_id = mlflow.create_experiment(experiment_name)
        
        with mlflow.start_run(experiment_id=experiment_id) as run:
            # 记录测试配置
            mlflow.log_params({
                "model_name": model_name,
                "baseline_version": baseline_version,
                "candidate_version": candidate_version,
                "traffic_split": traffic_split,
                "duration_hours": 24,
                "success_criteria": "candidate_accuracy > baseline_accuracy + 0.02"
            })
            
            # 部署金丝雀配置
            canary_config = {
                "baseline": {
                    "model_name": model_name,
                    "version": baseline_version,
                    "weight": 1 - traffic_split
                },
                "candidate": {
                    "model_name": model_name,
                    "version": candidate_version,
                    "weight": traffic_split
                },
                "monitoring": {
                    "metrics": ["accuracy", "latency", "error_rate"],
                    "check_interval_minutes": 5,
                    "minimum_samples": 1000
                }
            }
            
            mlflow.log_dict(canary_config, "canary_config.json")
            
            # 启动监控
            self._start_monitoring(run.info.run_id, canary_config)
            
            return run.info.run_id
    
    def _start_monitoring(self, run_id: str, config: Dict):
        """启动A/B测试监控"""
        baseline_metrics = []
        candidate_metrics = []
        
        monitoring_duration = 24 * 60 * 60  # 24小时
        check_interval = 5 * 60  # 5分钟
        start_time = time.time()
        
        while time.time() - start_time < monitoring_duration:
            # 模拟收集指标
            baseline_acc = self._collect_metrics(config["baseline"])
            candidate_acc = self._collect_metrics(config["candidate"])
            
            baseline_metrics.append(baseline_acc)
            candidate_metrics.append(candidate_acc)
            
            # 记录指标到MLflow
            with mlflow.start_run(run_id=run_id):
                mlflow.log_metrics({
                    "baseline_accuracy": baseline_acc,
                    "candidate_accuracy": candidate_acc,
                    "traffic_split": config["candidate"]["weight"]
                }, step=int((time.time() - start_time) / check_interval))
            
            # 检查早期停止条件
            if len(baseline_metrics) >= 10:  # 至少10个样本点
                if self._should_stop_early(baseline_metrics, candidate_metrics):
                    print("Early stopping criteria met")
                    break
            
            time.sleep(check_interval)
        
        # 评估最终结果
        self._evaluate_ab_test(run_id, baseline_metrics, candidate_metrics)
    
    def _collect_metrics(self, model_config: Dict) -> float:
        """收集模型指标（模拟）"""
        # 实际实现中应该从监控系统获取真实指标
        base_accuracy = 0.85 if "baseline" in model_config["version"] else 0.87
        noise = np.random.normal(0, 0.02)
        return max(0, min(1, base_accuracy + noise))
    
    def _should_stop_early(self, baseline_metrics: List[float], 
                          candidate_metrics: List[float]) -> bool:
        """检查是否应该提前停止"""
        if len(baseline_metrics) < 10:
            return False
            
        # 计算最近10个点的平均值
        recent_baseline = np.mean(baseline_metrics[-10:])
        recent_candidate = np.mean(candidate_metrics[-10:])
        
        # 如果候选模型显著优于基线模型，提前结束
        if recent_candidate > recent_baseline + 0.03:
            return True
            
        # 如果候选模型显著劣于基线模型，提前结束
        if recent_candidate < recent_baseline - 0.05:
            return True
            
        return False
    
    def _evaluate_ab_test(self, run_id: str, 
                         baseline_metrics: List[float],
                         candidate_metrics: List[float]):
        """评估A/B测试结果"""
        baseline_mean = np.mean(baseline_metrics)
        candidate_mean = np.mean(candidate_metrics)
        baseline_std = np.std(baseline_metrics)
        candidate_std = np.std(candidate_metrics)
        
        # 统计显著性检验（简化版）
        pooled_std = np.sqrt((baseline_std**2 + candidate_std**2) / 2)
        effect_size = (candidate_mean - baseline_mean) / pooled_std
        sample_size = len(baseline_metrics)
        
        # 简化的显著性判断
        is_significant = abs(effect_size) > 2.0 and sample_size > 30
        
        # 记录结果
        with mlflow.start_run(run_id=run_id):
            mlflow.log_metrics({
                "baseline_final_accuracy": baseline_mean,
                "candidate_final_accuracy": candidate_mean,
                "effect_size": effect_size,
                "statistical_significance": float(is_significant),
                "samples_collected": sample_size
            })
            
            # 决策逻辑
            if is_significant and candidate_mean > baseline_mean:
                decision = "promote_candidate"
                recommendation = "Promote candidate model to production"
            elif is_significant and candidate_mean < baseline_mean:
                decision = "keep_baseline"
                recommendation = "Keep baseline model, candidate underperforms"
            else:
                decision = "inconclusive"
                recommendation = "Results inconclusive, collect more data"
            
            mlflow.set_tag("ab_test_decision", decision)
            mlflow.set_tag("recommendation", recommendation)
            
            print(f"A/B Test Results:")
            print(f"Baseline accuracy: {baseline_mean:.4f} ± {baseline_std:.4f}")
            print(f"Candidate accuracy: {candidate_mean:.4f} ± {candidate_std:.4f}")
            print(f"Effect size: {effect_size:.4f}")
            print(f"Decision: {recommendation}")

# 使用示例
if __name__ == "__main__":
    ab_tester = AutomatedABTesting("http://mlflow.company.com")
    
    # 设置A/B测试
    run_id = ab_tester.setup_canary_deployment(
        model_name="customer-churn-predictor",
        baseline_version="1.2.0",
        candidate_version="2.0.0",
        traffic_split=0.1
    )
    
    print(f"A/B test started with run ID: {run_id}")
```

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
**相关**: [113-AI模型注册中心](../09-ai-model-registry.md) | **版本**: MLflow 2.9+
