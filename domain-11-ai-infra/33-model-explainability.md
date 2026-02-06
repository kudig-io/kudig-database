# 33 - 模型可解释性与透明度

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [SHAP](https://shap.readthedocs.io/) | [LIME](https://github.com/marcotcr/lime) | [InterpretML](https://interpret.ml/)

## 一、模型可解释性框架

### 1.1 可解释性全景架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                       Model Explainability Framework                              │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Global Interpretability                             │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Feature         │  │ Model           │  │ Partial         │               │  │
│  │  │ Importance      │  │ Summary         │  │ Dependence      │               │  │
│  │  │ (SHAP/LIME)     │  │ Plots           │  │ Plots           │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Local Interpretability                              │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Individual      │  │ Counterfactual  │  │ Adversarial     │               │  │
│  │  │ Prediction      │  │ Explanations    │  │ Examples        │               │  │
│  │  │ Explanations    │  │ (What-if)       │  │ (Robustness)    │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Operationalization                                  │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ API Endpoint    │  │ Dashboard       │  │ Alert System    │               │  │
│  │  │ (/explain)      │  │ Visualization   │  │ (Bias/Fairness) │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 可解释性方法分类

| 方法类别 | 技术名称 | 适用场景 | 优势 | 局限性 |
|----------|----------|----------|------|--------|
| **全局解释** | SHAP值 | 树模型、神经网络 | 全面、理论完备 | 计算复杂度高 |
| | 特征重要性 | 线性模型、树模型 | 简单直观 | 忽略特征交互 |
| | 部分依赖图 | 任意模型 | 可视化交互效应 | 维度诅咒 |
| **局部解释** | LIME | 黑盒模型 | 实例级别解释 | 不稳定性 |
| | 决策边界 | 分类模型 | 直观理解决策 | 高维空间难可视化 |
| | 梯度方法 | 深度学习 | 计算高效 | 对输入敏感 |
| **对比解释** | 反事实 | 任意模型 | 业务导向 | 生成困难 |
| | 对抗样本 | 安全检测 | 鲁棒性评估 | 可能误导 |

---

## 二、SHAP可解释性实现

### 2.1 SHAP基础配置

```python
# shap_explainer.py
import shap
import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt
import seaborn as sns
import mlflow
import joblib

class ModelExplainer:
    def __init__(self, model_path, training_data_path):
        self.model = joblib.load(model_path)
        self.training_data = pd.read_csv(training_data_path)
        self.feature_names = [col for col in self.training_data.columns if col != 'target']
        self.X_train = self.training_data[self.feature_names]
        self.y_train = self.training_data['target']
        
    def calculate_shap_values(self, sample_size=1000):
        \"\"\"计算SHAP值\"\"\"
        # 采样以提高计算效率
        X_sample = shap.utils.sample(self.X_train, sample_size)
        
        # 创建解释器
        if hasattr(self.model, 'predict_proba'):
            explainer = shap.TreeExplainer(self.model, X_sample)
            shap_values = explainer.shap_values(X_sample)
        else:
            explainer = shap.LinearExplainer(self.model, X_sample)
            shap_values = explainer.shap_values(X_sample)
        
        return explainer, shap_values, X_sample
    
    def global_explanations(self):
        \"\"\"生成全局解释\"\"\"
        explainer, shap_values, X_sample = self.calculate_shap_values()
        
        # 特征重要性排序
        feature_importance = np.abs(shap_values).mean(0)
        importance_df = pd.DataFrame({
            'feature': self.feature_names,
            'importance': feature_importance
        }).sort_values('importance', ascending=False)
        
        # 保存到MLflow
        with mlflow.start_run():
            mlflow.log_figure(
                self._plot_feature_importance(importance_df),
                'feature_importance.png'
            )
            
            # SHAP摘要图
            plt.figure(figsize=(10, 8))
            shap.summary_plot(shap_values, X_sample, show=False)
            plt.tight_layout()
            mlflow.log_figure(plt.gcf(), 'shap_summary.png')
            plt.close()
        
        return importance_df
    
    def local_explanations(self, instance_idx=0):
        \"\"\"生成局部解释\"\"\"
        explainer, shap_values, X_sample = self.calculate_shap_values()
        
        # 单个预测解释
        instance = X_sample.iloc[instance_idx:instance_idx+1]
        prediction = self.model.predict(instance)[0]
        probability = self.model.predict_proba(instance)[0][1] if hasattr(self.model, 'predict_proba') else None
        
        # SHAP力图
        plt.figure(figsize=(12, 6))
        shap.waterfall_plot(
            shap.Explanation(
                values=shap_values[instance_idx],
                base_values=explainer.expected_value,
                data=instance.iloc[0],
                feature_names=self.feature_names
            ),
            show=False
        )
        plt.tight_layout()
        
        # 保存解释结果
        explanation_result = {
            'instance_index': instance_idx,
            'prediction': float(prediction),
            'probability': float(probability) if probability else None,
            'shap_values': shap_values[instance_idx].tolist(),
            'feature_values': instance.iloc[0].to_dict()
        }
        
        return explanation_result, plt.gcf()
    
    def _plot_feature_importance(self, importance_df):
        \"\"\"绘制特征重要性图\"\"\"
        plt.figure(figsize=(10, 8))
        sns.barplot(
            data=importance_df.head(15),
            x='importance',
            y='feature',
            palette='viridis'
        )
        plt.title('Top 15 Feature Importance (SHAP)')
        plt.xlabel('Mean |SHAP Value|')
        plt.ylabel('Features')
        plt.tight_layout()
        return plt.gcf()

# 使用示例
explainer = ModelExplainer(
    model_path='/models/churn_model.pkl',
    training_data_path='/data/training_data.csv'
)

# 全局解释
global_importance = explainer.global_explanations()
print(global_importance.head(10))

# 局部解释
local_explanation, plot = explainer.local_explanations(instance_idx=42)
plt.show(plot)
```

### 2.2 可解释性API服务

```python
# explainability_api.py
from flask import Flask, request, jsonify
import shap
import numpy as np
import pandas as pd
import joblib
from prometheus_client import Counter, Histogram, generate_latest
import logging

app = Flask(__name__)

# 指标定义
explanation_requests = Counter('explanation_requests_total', 'Total explanation requests')
explanation_errors = Counter('explanation_errors_total', 'Explanation errors')
explanation_duration = Histogram('explanation_duration_seconds', 'Time spent processing explanations')

# 初始化模型和解释器
model = joblib.load('/models/production_model.pkl')
explainer = shap.TreeExplainer(model)

# 特征名称（需与训练时一致）
FEATURE_NAMES = [
    'age', 'income', 'credit_score', 'account_balance',
    'transaction_frequency', 'product_usage', 'support_tickets',
    'days_since_last_login', 'num_products', 'is_active_member'
]

@app.route('/health')
def health_check():
    return jsonify({'status': 'healthy'})

@app.route('/explain', methods=['POST'])
@explanation_duration.time()
def explain_prediction():
    \"\"\"提供模型预测解释\"\"\"
    explanation_requests.inc()
    
    try:
        # 解析请求
        data = request.get_json()
        instance = np.array(data['features']).reshape(1, -1)
        instance_df = pd.DataFrame(instance, columns=FEATURE_NAMES)
        
        # 计算SHAP值
        shap_values = explainer.shap_values(instance_df)
        
        # 生成解释
        base_value = explainer.expected_value
        prediction = model.predict(instance)[0]
        probability = model.predict_proba(instance)[0][1] if hasattr(model, 'predict_proba') else None
        
        # 构造响应
        explanation = {
            'prediction': int(prediction),
            'probability': float(probability) if probability else None,
            'base_value': float(base_value),
            'shap_values': shap_values.tolist()[0] if isinstance(shap_values, np.ndarray) else shap_values[1].tolist()[0],
            'feature_values': dict(zip(FEATURE_NAMES, instance[0])),
            'feature_contributions': sorted([
                {
                    'feature': FEATURE_NAMES[i],
                    'value': float(instance[0][i]),
                    'contribution': float(shap_values[0][i]) if isinstance(shap_values, np.ndarray) else float(shap_values[1][0][i]),
                    'abs_contribution': abs(float(shap_values[0][i]) if isinstance(shap_values, np.ndarray) else float(shap_values[1][0][i]))
                }
                for i in range(len(FEATURE_NAMES))
            ], key=lambda x: x['abs_contribution'], reverse=True)
        }
        
        return jsonify(explanation)
        
    except Exception as e:
        explanation_errors.inc()
        logging.error(f\"Explanation error: {str(e)}\")
        return jsonify({'error': str(e)}), 500

@app.route('/metrics')
def metrics():
    return generate_latest()

@app.route('/feature-importance')
def feature_importance():
    \"\"\"获取全局特征重要性\"\"\"
    try:
        # 使用训练数据计算全局重要性
        training_data = pd.read_csv('/data/training_sample.csv')
        X_sample = shap.utils.sample(training_data[FEATURE_NAMES], 1000)
        shap_values = explainer.shap_values(X_sample)
        
        # 计算平均绝对SHAP值
        importance = np.abs(shap_values).mean(0)
        importance_dict = dict(zip(FEATURE_NAMES, importance.tolist()))
        
        return jsonify({
            'feature_importance': importance_dict,
            'top_features': sorted(
                [{'feature': k, 'importance': v} for k, v in importance_dict.items()],
                key=lambda x: x['importance'],
                reverse=True
            )[:10]
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

### 2.3 Kubernetes部署配置

```yaml
# explainability-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: model-explainability-api
spec:
  replicas: 3
  selector:
    matchLabels:
      app: explainability-api
  template:
    metadata:
      labels:
        app: explainability-api
    spec:
      containers:
      - name: explainability-api
        image: company/explainability-api:v1.0
        ports:
        - containerPort: 5000
        env:
        - name: MODEL_PATH
          value: \"/models/production_model.pkl\"
        - name: TRAINING_DATA_PATH
          value: \"/data/training_sample.csv\"
        - name: FLASK_ENV
          value: \"production\"
        resources:
          requests:
            cpu: \"500m\"
            memory: \"1Gi\"
          limits:
            cpu: \"1\"
            memory: \"2Gi\"
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
          initialDelaySeconds: 5
          periodSeconds: 5
        volumeMounts:
        - name: model-storage
          mountPath: /models
        - name: data-storage
          mountPath: /data
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-pvc
      - name: data-storage
        persistentVolumeClaim:
          claimName: data-pvc

---
apiVersion: v1
kind: Service
metadata:
  name: explainability-api-service
spec:
  selector:
    app: explainability-api
  ports:
  - port: 80
    targetPort: 5000
  type: ClusterIP

---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: explainability-api-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: model-explainability-api
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
```

---

## 三、公平性与偏见检测

### 3.1 公平性评估框架

```python
# fairness_analyzer.py
import pandas as pd
import numpy as np
from sklearn.metrics import confusion_matrix, roc_auc_score
import matplotlib.pyplot as plt
import seaborn as sns
from aif360.datasets import BinaryLabelDataset
from aif360.metrics import ClassificationMetric
from aif360.algorithms.preprocessing import Reweighing
import mlflow

class FairnessAnalyzer:
    def __init__(self, predictions_df, protected_attributes):
        self.df = predictions_df
        self.protected_attrs = protected_attributes
        
    def calculate_disparate_impact(self, group1_condition, group2_condition):
        \"\"\"计算差异影响比\"\"\"
        group1_positive = self.df[group1_condition]['prediction'].mean()
        group2_positive = self.df[group2_condition]['prediction'].mean()
        
        if group2_positive == 0:
            return float('inf')
        
        disparate_impact = group1_positive / group2_positive
        return disparate_impact
    
    def demographic_parity_difference(self, group1_condition, group2_condition):
        \"\"\"计算人口统计奇偶性差异\"\"\"
        group1_positive_rate = self.df[group1_condition]['prediction'].mean()
        group2_positive_rate = self.df[group2_condition]['prediction'].mean()
        
        return abs(group1_positive_rate - group2_positive_rate)
    
    def equal_opportunity_difference(self, group1_condition, group2_condition):
        \"\"\"计算机会均等差异\"\"\"
        # 只考虑正类样本
        positive_samples = self.df[self.df['true_label'] == 1]
        
        group1_tpr = positive_samples[group1_condition]['prediction'].mean()
        group2_tpr = positive_samples[group2_condition]['prediction'].mean()
        
        return abs(group1_tpr - group2_tpr)
    
    def analyze_bias(self):
        \"\"\"全面偏见分析\"\"\"
        bias_metrics = {}
        
        for attr in self.protected_attrs:
            if attr not in self.df.columns:
                continue
                
            # 对于二元属性
            if self.df[attr].nunique() == 2:
                values = self.df[attr].unique()
                group1_cond = self.df[attr] == values[0]
                group2_cond = self.df[attr] == values[1]
                
                bias_metrics[f'{attr}_disparate_impact'] = self.calculate_disparate_impact(
                    group1_cond, group2_cond
                )
                bias_metrics[f'{attr}_demographic_parity'] = self.demographic_parity_difference(
                    group1_cond, group2_cond
                )
                bias_metrics[f'{attr}_equal_opportunity'] = self.equal_opportunity_difference(
                    group1_cond, group2_cond
                )
        
        return bias_metrics
    
    def plot_bias_analysis(self):
        \"\"\"可视化偏见分析结果\"\"\"
        bias_results = self.analyze_bias()
        
        # 创建可视化
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        
        # 差异影响比
        di_metrics = {k: v for k, v in bias_results.items() if 'disparate_impact' in k}
        axes[0].bar(di_metrics.keys(), di_metrics.values())
        axes[0].axhline(y=0.8, color='r', linestyle='--', label='Fairness threshold (0.8)')
        axes[0].axhline(y=1.2, color='r', linestyle='--', label='Fairness threshold (1.2)')
        axes[0].set_title('Disparate Impact Ratio')
        axes[0].set_ylabel('Ratio')
        axes[0].tick_params(axis='x', rotation=45)
        axes[0].legend()
        
        # 人口统计奇偶性
        dp_metrics = {k: v for k, v in bias_results.items() if 'demographic_parity' in k}
        axes[1].bar(dp_metrics.keys(), dp_metrics.values())
        axes[1].axhline(y=0.1, color='r', linestyle='--', label='Fairness threshold (0.1)')
        axes[1].set_title('Demographic Parity Difference')
        axes[1].set_ylabel('Difference')
        axes[1].tick_params(axis='x', rotation=45)
        axes[1].legend()
        
        # 机会均等
        eo_metrics = {k: v for k, v in bias_results.items() if 'equal_opportunity' in k}
        axes[2].bar(eo_metrics.keys(), eo_metrics.values())
        axes[2].axhline(y=0.1, color='r', linestyle='--', label='Fairness threshold (0.1)')
        axes[2].set_title('Equal Opportunity Difference')
        axes[2].set_ylabel('Difference')
        axes[2].tick_params(axis='x', rotation=45)
        axes[2].legend()
        
        plt.tight_layout()
        
        # 记录到MLflow
        with mlflow.start_run():
            mlflow.log_figure(fig, 'bias_analysis.png')
        
        return fig, bias_results

# 使用示例
predictions_df = pd.read_csv('/data/model_predictions.csv')
analyzer = FairnessAnalyzer(
    predictions_df=predictions_df,
    protected_attributes=['gender', 'race', 'age_group']
)

fig, bias_metrics = analyzer.plot_bias_analysis()
print(\"偏见分析结果:\")
for metric, value in bias_metrics.items():
    print(f\"{metric}: {value:.4f}\")
```

### 3.2 偏见缓解策略

```python
# bias_mitigation.py
from aif360.algorithms.preprocessing import Reweighing, DisparateImpactRemover
from aif360.algorithms.inprocessing import PrejudiceRemover
from aif360.algorithms.postprocessing import CalibratedEqOddsPostprocessing, RejectOptionClassification
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import RandomForestClassifier
import numpy as np

class BiasMitigationPipeline:
    def __init__(self, dataset, target_column, protected_attribute):
        self.dataset = dataset
        self.target_col = target_column
        self.protected_attr = protected_attribute
        
        # 转换为AIF360格式
        self.aif_dataset = BinaryLabelDataset(
            df=dataset,
            label_names=[target_column],
            protected_attribute_names=[protected_attribute]
        )
        
    def reweighing_preprocessing(self):
        \"\"\"重加权预处理\"\"\"
        rw = Reweighing(
            unprivileged_groups=[{self.protected_attr: 0}],
            privileged_groups=[{self.protected_attr: 1}]
        )
        
        transf_dataset = rw.fit_transform(self.aif_dataset)
        return transf_dataset
    
    def disparate_impact_remover(self, repair_level=1.0):
        \"\"\"差异影响消除\"\"\"
        dir = DisparateImpactRemover(
            repair_level=repair_level,
            sensitive_attribute=self.protected_attr
        )
        
        transf_dataset = dir.fit_transform(self.aif_dataset)
        return transf_dataset
    
    def prejudice_remover_inprocessing(self, eta=25.0):
        \"\"\"过程内偏见消除\"\"\"
        # 分割数据
        train_data, test_data = self.aif_dataset.split([0.7], shuffle=True)
        
        # 训练偏见消除模型
        pr = PrejudiceRemover(
            sensitive_attr=self.protected_attr,
            eta=eta
        )
        
        pr.fit(train_data)
        predictions = pr.predict(test_data)
        
        return predictions
    
    def calibrated_equalized_odds_postprocessing(self, model_predictions):
        \"\"\"校准的平等几率后处理\"\"\"
        # 创建预测数据集
        pred_dataset = self.aif_dataset.copy()
        pred_dataset.labels = model_predictions.reshape(-1, 1)
        
        # 分割数据
        dataset_orig_train, dataset_orig_test = self.aif_dataset.split([0.7], shuffle=True)
        dataset_pred_train, dataset_pred_test = pred_dataset.split([0.7], shuffle=True)
        
        # 训练后处理器
        cpp = CalibratedEqOddsPostprocessing(
            privileged_groups=[{self.protected_attr: 1}],
            unprivileged_groups=[{self.protected_attr: 0}],
            cost_constraint='fnr'  # 可选: 'fpr', 'weighted'
        )
        
        cpp = cpp.fit(dataset_orig_train, dataset_pred_train)
        transf_predictions = cpp.predict(dataset_pred_test)
        
        return transf_predictions
    
    def evaluate_fairness(self, predictions, original_labels):
        \"\"\"评估公平性指标\"\"\"
        # 创建度量对象
        metric = ClassificationMetric(
            self.aif_dataset,
            predictions,
            unprivileged_groups=[{self.protected_attr: 0}],
            privileged_groups=[{self.protected_attr: 1}]
        )
        
        fairness_metrics = {
            'disparate_impact': metric.disparate_impact(),
            'statistical_parity_difference': metric.statistical_parity_difference(),
            'equal_opportunity_difference': metric.equal_opportunity_difference(),
            'average_odds_difference': metric.average_odds_difference(),
            'theil_index': metric.theil_index()
        }
        
        return fairness_metrics

# 使用示例
mitigation = BiasMitigationPipeline(
    dataset=pd.read_csv('/data/training_data.csv'),
    target_column='loan_approved',
    protected_attribute='race'
)

# 应用不同的缓解策略
reweighed_data = mitigation.reweighing_preprocessing()
di_removed_data = mitigation.disparate_impact_remover(repair_level=0.8)
pr_predictions = mitigation.prejudice_remover_inprocessing(eta=50.0)

# 评估公平性
fairness_scores = mitigation.evaluate_fairness(pr_predictions, di_removed_data.labels)
print(\"公平性评估结果:\", fairness_scores)
```

---

## 四、可解释性监控与告警

### 4.1 可解释性监控系统

```yaml
# explainability-monitoring.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: explainability-alerts
  namespace: monitoring
spec:
  groups:
  - name: explainability.rules
    rules:
    # 特征重要性变化告警
    - alert: FeatureImportanceDrift
      expr: |
        abs(
          rate(feature_importance_shap[1h]) - 
          rate(feature_importance_baseline[1h])
        ) > 0.1
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: \"Feature importance drift detected\"
        description: \"SHAP feature importance has drifted significantly in the last hour\"
    
    # 预测分布变化告警
    - alert: PredictionDistributionShift
      expr: |
        histogram_quantile(0.95, sum(rate(prediction_confidence_bucket[1h])) by (le))
        - 
        histogram_quantile(0.95, sum(rate(baseline_prediction_confidence_bucket[1h])) by (le))
        > 0.2
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: \"Prediction distribution shift detected\"
        description: \"Model prediction confidence distribution has shifted significantly\"
    
    # 公平性违规告警
    - alert: FairnessViolation
      expr: |
        disparate_impact_ratio < 0.8 or disparate_impact_ratio > 1.2
      for: 30m
      labels:
        severity: critical
      annotations:
        summary: \"Fairness constraint violation\"
        description: \"Model disparate impact ratio {{ $value }} violates fairness constraints\"
    
    # 解释服务可用性告警
    - alert: ExplainabilityServiceDown
      expr: |
        up{job=\"explainability-api\"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: \"Explainability service is down\"
        description: \"Model explanation API service is not responding\"

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: explainability-dashboard
  namespace: monitoring
data:
  dashboard.json: |
    {
      \"dashboard\": {
        \"title\": \"Model Explainability Monitoring\",
        \"panels\": [
          {
            \"title\": \"Top Feature Importance\",
            \"type\": \"barchart\",
            \"targets\": [
              {
                \"expr\": \"topk(10, feature_importance_shap)\",
                \"legendFormat\": \"{{feature}}\"
              }
            ]
          },
          {
            \"title\": \"SHAP Value Distribution\",
            \"type\": \"heatmap\",
            \"targets\": [
              {
                \"expr\": \"shap_values_histogram\",
                \"legendFormat\": \"SHAP Values\"
              }
            ]
          },
          {
            \"title\": \"Disparate Impact Ratio\",
            \"type\": \"graph\",
            \"targets\": [
              {
                \"expr\": \"disparate_impact_ratio\",
                \"legendFormat\": \"Current\"
              },
              {
                \"expr\": \"1\",
                \"legendFormat\": \"Ideal (1.0)\"
              },
              {
                \"expr\": \"0.8\",
                \"legendFormat\": \"Lower Threshold\"
              },
              {
                \"expr\": \"1.2\",
                \"legendFormat\": \"Upper Threshold\"
              }
            ]
          },
          {
            \"title\": \"Explanation Response Time\",
            \"type\": \"graph\",
            \"targets\": [
              {
                \"expr\": \"histogram_quantile(0.95, sum(rate(explanation_duration_seconds_bucket[5m])) by (le))\",
                \"legendFormat\": \"95th Percentile\"
              },
              {
                \"expr\": \"histogram_quantile(0.50, sum(rate(explanation_duration_seconds_bucket[5m])) by (le))\",
                \"legendFormat\": \"Median\"
              }
            ]
          }
        ]
      }
    }
```

### 4.2 自动化可解释性报告

```python
# automated_explainability_report.py
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from datetime import datetime, timedelta
import jinja2
import pdfkit
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders

class ExplainabilityReporter:
    def __init__(self, model_name, data_source):
        self.model_name = model_name
        self.data_source = data_source
        self.report_date = datetime.now()
        
    def generate_weekly_report(self):
        \"\"\"生成周度可解释性报告\"\"\"
        # 收集数据
        weekly_data = self._collect_weekly_data()
        
        # 生成分析结果
        analysis_results = self._perform_analysis(weekly_data)
        
        # 创建报告
        report_html = self._create_report_template(analysis_results)
        
        # 生成PDF
        pdf_path = f'/reports/{self.model_name}_explainability_report_{self.report_date.strftime(\"%Y%m%d\")}.pdf'
        pdfkit.from_string(report_html, pdf_path)
        
        # 发送报告
        self._send_report(pdf_path)
        
        return pdf_path
    
    def _collect_weekly_data(self):
        \"\"\"收集一周的数据\"\"\"
        # 从监控系统获取数据
        # 这里简化为模拟数据
        data = {
            'feature_importance': self._get_feature_importance_data(),
            'prediction_distribution': self._get_prediction_distribution(),
            'fairness_metrics': self._get_fairness_metrics(),
            'explanation_requests': self._get_explanation_stats()
        }
        return data
    
    def _perform_analysis(self, data):
        \"\"\"执行分析\"\"\"
        analysis = {}
        
        # 特征重要性分析
        importance_df = pd.DataFrame(data['feature_importance'])
        analysis['top_features'] = importance_df.head(10).to_dict('records')
        analysis['importance_changes'] = self._calculate_importance_changes(importance_df)
        
        # 公平性分析
        fairness_df = pd.DataFrame(data['fairness_metrics'])
        analysis['fairness_summary'] = fairness_df.describe().to_dict()
        analysis['fairness_alerts'] = self._detect_fairness_issues(fairness_df)
        
        # 性能分析
        analysis['explanation_performance'] = data['explanation_requests']
        
        return analysis
    
    def _create_report_template(self, analysis_results):
        \"\"\"创建报告模板\"\"\"
        template_str = \"\"\"
        <!DOCTYPE html>
        <html>
        <head>
            <title>{{ model_name }} Explainability Report</title>
            <style>
                body { font-family: Arial, sans-serif; margin: 20px; }
                .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
                .section { margin: 20px 0; }
                .chart { margin: 10px 0; }
                table { border-collapse: collapse; width: 100%; }
                th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
                th { background-color: #f2f2f2; }
            </style>
        </head>
        <body>
            <div class=\"header\">
                <h1>{{ model_name }} Explainability Report</h1>
                <p>Generated on: {{ report_date }}</p>
            </div>
            
            <div class=\"section\">
                <h2>Executive Summary</h2>
                <ul>
                    <li>Total explanation requests: {{ analysis.explanation_performance.total_requests }}</li>
                    <li>Average response time: {{ analysis.explanation_performance.avg_response_time }}ms</li>
                    <li>Fairness violations detected: {{ analysis.fairness_alerts|length }}</li>
                </ul>
            </div>
            
            <div class=\"section\">
                <h2>Top Feature Importance</h2>
                <table>
                    <tr><th>Feature</th><th>Importance</th><th>Change</th></tr>
                    {% for feature in analysis.top_features %}
                    <tr>
                        <td>{{ feature.feature }}</td>
                        <td>{{ \"%.4f\"|format(feature.importance) }}</td>
                        <td>{{ \"%.4f\"|format(feature.change) }}</td>
                    </tr>
                    {% endfor %}
                </table>
            </div>
            
            <div class=\"section\">
                <h2>Fairness Analysis</h2>
                {% if analysis.fairness_alerts %}
                <div style=\"color: red;\">
                    <h3>⚠️ Fairness Issues Detected</h3>
                    <ul>
                    {% for alert in analysis.fairness_alerts %}
                        <li>{{ alert }}</li>
                    {% endfor %}
                    </ul>
                </div>
                {% else %}
                <p style=\"color: green;\">✅ No fairness violations detected</p>
                {% endif %}
            </div>
        </body>
        </html>
        \"\"\"
        
        template = jinja2.Template(template_str)
        return template.render(
            model_name=self.model_name,
            report_date=self.report_date.strftime('%Y-%m-%d'),
            analysis=analysis_results
        )
    
    def _send_report(self, pdf_path):
        \"\"\"发送报告\"\"\"
        # 邮件配置
        msg = MIMEMultipart()
        msg['From'] = 'ml-governance@company.com'
        msg['To'] = 'stakeholders@company.com'
        msg['Subject'] = f'{self.model_name} Explainability Report - {self.report_date.strftime(\"%Y-%m-%d\")}'
        
        # 邮件正文
        body = f\"\"\"Dear Stakeholders,

Please find attached the weekly explainability report for {self.model_name}.

Key highlights:
- Feature importance analysis completed
- Fairness metrics reviewed
- Performance benchmarks assessed

Best regards,
ML Governance Team
        \"\"\"
        msg.attach(MIMEText(body, 'plain'))
        
        # 附加PDF报告
        with open(pdf_path, \"rb\") as attachment:
            part = MIMEBase('application', 'octet-stream')
            part.set_payload(attachment.read())
        
        encoders.encode_base64(part)
        part.add_header(
            'Content-Disposition',
            f'attachment; filename= {pdf_path.split(\"/\")[-1]}'
        )
        msg.attach(part)
        
        # 发送邮件（实际使用时需要配置SMTP）
        # server = smtplib.SMTP('smtp.company.com', 587)
        # server.starttls()
        # server.login('ml-governance@company.com', 'password')
        # server.send_message(msg)
        # server.quit()

# 使用示例
reporter = ExplainabilityReporter(
    model_name='customer_churn_model',
    data_source='production_logs'
)

report_path = reporter.generate_weekly_report()
print(f\"Report generated: {report_path}\")
```

---

**维护者**: AI Ethics Team | **最后更新**: 2026-02 | **版本**: v1.0