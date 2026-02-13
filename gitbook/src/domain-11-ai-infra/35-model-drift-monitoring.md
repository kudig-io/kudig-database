# 35 - 模型漂移监控与预警

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [Evidently AI](https://www.evidentlyai.com/) | [WhyLabs](https://whylabs.ai/) | [Arize AI](https://arize.com/)

## 一、模型漂移监控架构

### 1.1 漂移监控全景架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Model Drift Monitoring Architecture                            │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                           Data Collection Layer                               │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Real-time       │  │ Batch           │  │ Reference       │               │  │
│  │  │ Data Stream     │  │ Processing      │  │ Data Store      │               │  │
│  │  │ (Kafka/Pulsar)  │  │ (Spark/Flink)   │  │ (Feature Store) │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                         Drift Detection Engine                                │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Statistical     │  │ Concept Drift   │  │ Feature Drift   │               │  │
│  │  │ Tests           │  │ Detectors       │  │ Detectors       │               │  │
│  │  │ (KS, PSI, etc)  │  │ (ADWIN, DDM)    │  │ (Correlation)   │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                          Alert & Action Layer                                 │  │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐               │  │
│  │  │ Alert Manager   │  │ Auto-trigger    │  │ Human Review    │               │  │
│  │  │ (Prometheus)    │  │ (Retraining)    │  │ (Approval)      │               │  │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────┘               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 漂移类型分类

| 漂移类型 | 定义 | 检测方法 | 影响 | 应对策略 |
|----------|------|----------|------|----------|
| **协变量漂移** | 输入特征分布变化 | PSI, KL散度 | 模型性能下降 | 重新训练 |
| **先验概率漂移** | 目标变量分布变化 | 卡方检验 | 决策阈值失效 | 调整阈值 |
| **概念漂移** | 输入输出关系变化 | DDM, ADWIN | 模型完全失效 | 紧急重训练 |
| **特征漂移** | 单个特征分布变化 | 单变量统计检验 | 局部性能下降 | 特征工程 |

---

## 二、实时漂移检测系统

### 2.1 漂移检测核心组件

```python
# drift_detector.py
import numpy as np
import pandas as pd
from scipy import stats
from sklearn.metrics import roc_auc_score
import warnings
warnings.filterwarnings('ignore')

class DriftDetector:
    def __init__(self, reference_data, threshold=0.05):
        self.reference_data = reference_data
        self.threshold = threshold
        self.drift_statistics = {}
        
    def population_stability_index(self, current_data, bins=10):
        \"\"\"计算群体稳定性指数(PSI)\"\"\"
        def psi(expected, actual, buckets):
            # 创建分箱
            breakpoints = np.quantile(expected, np.linspace(0, 1, buckets + 1))
            expected_counts = np.histogram(expected, breakpoints)[0]
            actual_counts = np.histogram(actual, breakpoints)[0]
            
            # 避免除零
            expected_pct = np.where(expected_counts == 0, 0.0001, expected_counts / len(expected))
            actual_pct = np.where(actual_counts == 0, 0.0001, actual_counts / len(actual))
            
            # 计算PSI
            psi_value = np.sum((actual_pct - expected_pct) * np.log(actual_pct / expected_pct))
            return psi_value
        
        psi_results = {}
        for column in self.reference_data.columns:
            if self.reference_data[column].dtype in ['int64', 'float64']:
                psi_value = psi(self.reference_data[column].values, 
                              current_data[column].values, bins)
                psi_results[column] = {
                    'psi': psi_value,
                    'drift_detected': psi_value > self.threshold,
                    'severity': self._classify_severity(psi_value)
                }
        
        return psi_results
    
    def kolmogorov_smirnov_test(self, current_data):
        \"\"\"Kolmogorov-Smirnov检验\"\"\"
        ks_results = {}
        for column in self.reference_data.columns:
            if self.reference_data[column].dtype in ['int64', 'float64']:
                statistic, p_value = stats.ks_2samp(
                    self.reference_data[column].values,
                    current_data[column].values
                )
                ks_results[column] = {
                    'ks_statistic': statistic,
                    'p_value': p_value,
                    'drift_detected': p_value < self.threshold,
                    'severity': self._classify_severity(statistic)
                }
        
        return ks_results
    
    def chi_square_test(self, current_data, bins=10):
        \"\"\"卡方检验（适用于分类变量）\"\"\"
        chi2_results = {}
        for column in self.reference_data.columns:
            if self.reference_data[column].dtype == 'object' or \
               self.reference_data[column].nunique() < bins:
                
                # 创建列联表
                ref_counts = self.reference_data[column].value_counts()
                curr_counts = current_data[column].value_counts()
                
                # 对齐类别
                all_categories = set(ref_counts.index) | set(curr_counts.index)
                ref_aligned = ref_counts.reindex(all_categories, fill_value=0)
                curr_aligned = curr_counts.reindex(all_categories, fill_value=0)
                
                # 卡方检验
                chi2, p_value = stats.chisquare(curr_aligned.values, ref_aligned.values)
                chi2_results[column] = {
                    'chi2_statistic': chi2,
                    'p_value': p_value,
                    'drift_detected': p_value < self.threshold,
                    'severity': self._classify_severity(chi2 / len(all_categories))
                }
        
        return chi2_results
    
    def _classify_severity(self, value):
        \"\"\"分类漂移严重程度\"\"\"
        if value < self.threshold * 0.5:
            return 'low'
        elif value < self.threshold:
            return 'medium'
        elif value < self.threshold * 2:
            return 'high'
        else:
            return 'critical'

class ConceptDriftDetector:
    def __init__(self, window_size=1000, threshold=0.05):
        self.window_size = window_size
        self.threshold = threshold
        self.performance_history = []
        self.warning_detected = False
        
    def detect_ddm(self, predictions, actuals):
        \"\"\"使用DDM算法检测概念漂移\"\"\"
        errors = (predictions != actuals).astype(int)
        
        # 计算累积错误率和标准差
        cum_error_rate = np.cumsum(errors) / np.arange(1, len(errors) + 1)
        cum_std = np.sqrt(cum_error_rate * (1 - cum_error_rate) / np.arange(1, len(errors) + 1))
        
        # DDM检测逻辑
        min_error_rate = np.min(cum_error_rate)
        min_std = np.min(cum_std[np.argmin(cum_error_rate):])
        
        current_error_rate = cum_error_rate[-1]
        current_std = cum_std[-1]
        
        # 检测条件
        if current_error_rate > min_error_rate + self.threshold + min_std:
            return {
                'drift_detected': True,
                'severity': 'critical',
                'detection_method': 'DDM',
                'error_rate_increase': current_error_rate - min_error_rate
            }
        elif current_error_rate > min_error_rate + 0.5 * self.threshold + min_std:
            return {
                'drift_detected': False,
                'warning_detected': True,
                'severity': 'medium',
                'detection_method': 'DDM_warning',
                'error_rate_increase': current_error_rate - min_error_rate
            }
        else:
            return {
                'drift_detected': False,
                'warning_detected': False,
                'severity': 'low',
                'detection_method': 'DDM_normal'
            }
    
    def detect_adwin(self, predictions, actuals):
        \"\"\"使用ADWIN算法检测概念漂移\"\"\"
        from river import drift
        
        adwin = drift.ADWIN(delta=self.threshold)
        drift_points = []
        warning_points = []
        
        for i, (pred, actual) in enumerate(zip(predictions, actuals)):
            error = int(pred != actual)
            adwin.update(error)
            
            if adwin.drift_detected:
                drift_points.append(i)
            elif adwin.warning_detected:
                warning_points.append(i)
        
        return {
            'drift_points': drift_points,
            'warning_points': warning_points,
            'drift_detected': len(drift_points) > 0,
            'warning_detected': len(warning_points) > 0,
            'severity': 'critical' if len(drift_points) > 0 else 'medium' if len(warning_points) > 0 else 'low'
        }

# 使用示例
# 初始化检测器
reference_data = pd.read_csv('/data/reference_dataset.csv')
detector = DriftDetector(reference_data, threshold=0.1)

# 检测新数据
current_data = pd.read_csv('/data/current_batch.csv')
psi_results = detector.population_stability_index(current_data)
ks_results = detector.kolmogorov_smirnov_test(current_data)

print(\"PSI检测结果:\", psi_results)
print(\"KS检测结果:\", ks_results)
```

### 2.2 实时监控服务

```python
# realtime_monitoring_service.py
from flask import Flask, request, jsonify
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import redis
import json
from prometheus_client import Counter, Gauge, Histogram, generate_latest
import threading
import time

app = Flask(__name__)

# Prometheus指标
drift_events = Counter('model_drift_events_total', 'Total drift events detected', ['feature', 'severity'])
prediction_latency = Histogram('prediction_latency_seconds', 'Prediction response time')
model_accuracy = Gauge('model_current_accuracy', 'Current model accuracy')

# Redis连接
redis_client = redis.Redis(host='localhost', port=6379, db=0)

class RealtimeDriftMonitor:
    def __init__(self, model_name, reference_data_path):
        self.model_name = model_name
        self.reference_data = pd.read_csv(reference_data_path)
        self.feature_store = {}
        self.alert_thresholds = {
            'psi': 0.1,
            'ks_statistic': 0.05,
            'concept_drift': 0.05
        }
        self.monitoring_window = 1000  # 样本窗口大小
        
    def process_prediction(self, features, prediction, actual=None):
        \"\"\"处理单个预测请求\"\"\"
        # 记录特征数据
        timestamp = datetime.now().isoformat()
        record = {
            'features': features,
            'prediction': prediction,
            'actual': actual,
            'timestamp': timestamp
        }
        
        # 存储到特征存储
        self._store_features(record)
        
        # 实时漂移检测
        drift_analysis = self._analyze_drift()
        
        # 性能监控
        if actual is not None:
            accuracy = int(prediction == actual)
            model_accuracy.set(accuracy)
        
        return drift_analysis
    
    def _store_features(self, record):
        \"\"\"存储特征数据到Redis\"\"\"
        key = f\"{self.model_name}:predictions:{record['timestamp']}\"
        redis_client.setex(key, 3600, json.dumps(record))  # 1小时过期
        
        # 维护滑动窗口
        window_key = f\"{self.model_name}:window\"
        redis_client.lpush(window_key, json.dumps(record))
        redis_client.ltrim(window_key, 0, self.monitoring_window - 1)
    
    def _analyze_drift(self):
        \"\"\"分析漂移情况\"\"\"
        # 获取窗口数据
        window_key = f\"{self.model_name}:window\"
        window_data = redis_client.lrange(window_key, 0, -1)
        
        if len(window_data) < 100:  # 数据不足
            return {'status': 'insufficient_data'}
        
        # 转换为DataFrame
        records = [json.loads(record) for record in window_data]
        current_df = pd.DataFrame([r['features'] for r in records])
        
        # 统计漂移检测
        drift_detector = DriftDetector(self.reference_data)
        psi_results = drift_detector.population_stability_index(current_df)
        
        # 概念漂移检测（如果有真实标签）
        if all('actual' in r and r['actual'] is not None for r in records[-100:]):
            recent_records = records[-100:]
            predictions = [r['prediction'] for r in recent_records]
            actuals = [r['actual'] for r in recent_records]
            
            concept_detector = ConceptDriftDetector()
            concept_drift = concept_detector.detect_ddm(np.array(predictions), np.array(actuals))
        else:
            concept_drift = {'drift_detected': False, 'severity': 'low'}
        
        # 聚合结果
        drift_summary = {
            'timestamp': datetime.now().isoformat(),
            'statistical_drift': psi_results,
            'concept_drift': concept_drift,
            'overall_status': self._assess_overall_status(psi_results, concept_drift)
        }
        
        # 触发告警
        self._trigger_alerts(drift_summary)
        
        return drift_summary
    
    def _assess_overall_status(self, statistical_drift, concept_drift):
        \"\"\"评估整体漂移状态\"\"\"
        # 统计漂移严重程度
        stat_severities = [result['severity'] for result in statistical_drift.values()]
        max_stat_severity = max(stat_severities, default='low')
        
        # 概念漂移严重程度
        concept_severity = concept_drift.get('severity', 'low')
        
        # 综合评估
        severity_order = ['low', 'medium', 'high', 'critical']
        overall_severity = max(max_stat_severity, concept_severity, 
                             key=lambda x: severity_order.index(x))
        
        return {
            'severity': overall_severity,
            'requires_attention': overall_severity in ['high', 'critical'],
            'recommended_action': self._get_recommendation(overall_severity)
        }
    
    def _get_recommendation(self, severity):
        \"\"\"根据严重程度提供建议\"\"\"
        recommendations = {
            'low': 'Continue monitoring',
            'medium': 'Increase monitoring frequency',
            'high': 'Prepare for model retraining',
            'critical': 'Immediate model intervention required'
        }
        return recommendations.get(severity, 'Unknown')
    
    def _trigger_alerts(self, drift_summary):
        \"\"\"触发告警\"\"\"
        overall_status = drift_summary['overall_status']
        
        if overall_status['requires_attention']:
            # 记录Prometheus指标
            drift_events.labels(
                feature='overall',
                severity=overall_status['severity']
            ).inc()
            
            # 发送告警（简化实现）
            alert_message = {
                'model': self.model_name,
                'severity': overall_status['severity'],
                'timestamp': drift_summary['timestamp'],
                'recommendation': overall_status['recommended_action']
            }
            
            # 发送到告警系统
            self._send_alert(alert_message)
    
    def _send_alert(self, alert_message):
        \"\"\"发送告警到外部系统\"\"\"
        # 这里可以集成到PagerDuty, Slack, Email等
        print(f\"ALERT: {alert_message}\")
        
        # 存储告警历史
        alert_key = f\"alerts:{self.model_name}:{datetime.now().strftime('%Y%m%d')}\"
        redis_client.lpush(alert_key, json.dumps(alert_message))

# 初始化监控器
monitor = RealtimeDriftMonitor(
    model_name='churn_prediction_model',
    reference_data_path='/data/reference_features.csv'
)

@app.route('/predict', methods=['POST'])
def predict():
    \"\"\"预测接口（带漂移监控）\"\"\"
    start_time = time.time()
    
    try:
        data = request.get_json()
        features = data['features']
        
        # 模拟模型预测
        prediction = np.random.choice([0, 1])  # 实际应调用真实模型
        
        # 处理预测并监控漂移
        drift_analysis = monitor.process_prediction(features, prediction)
        
        # 记录延迟
        latency = time.time() - start_time
        prediction_latency.observe(latency)
        
        return jsonify({
            'prediction': int(prediction),
            'drift_analysis': drift_analysis,
            'latency': latency
        })
        
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/metrics')
def metrics():
    \"\"\"Prometheus指标端点\"\"\"
    return generate_latest()

@app.route('/health')
def health_check():
    \"\"\"健康检查\"\"\"
    return jsonify({'status': 'healthy', 'model': monitor.model_name})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

---

## 三、批量漂移分析系统

### 3.1 批量数据分析管道

```python
# batch_drift_analysis.py
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import matplotlib.pyplot as plt
import seaborn as sns
from evidently.model_profile import Profile
from evidently.model_profile.sections import (
    DataDriftProfileSection,
    CatTargetDriftProfileSection,
    RegressionPerformanceProfileSection,
    ClassificationPerformanceProfileSection
)
import mlflow
import boto3
from airflow import DAG
from airflow.operators.python_operator import PythonOperator

class BatchDriftAnalyzer:
    def __init__(self, model_name, s3_bucket):
        self.model_name = model_name
        self.s3_bucket = s3_bucket
        self.s3_client = boto3.client('s3')
        
    def fetch_batch_data(self, date_range):
        \"\"\"获取批处理数据\"\"\"
        data_frames = []
        
        for date in pd.date_range(date_range[0], date_range[1], freq='D'):
            date_str = date.strftime('%Y-%m-%d')
            try:
                # 从S3下载数据
                obj = self.s3_client.get_object(
                    Bucket=self.s3_bucket,
                    Key=f\"predictions/{self.model_name}/{date_str}.csv\"
                )
                df = pd.read_csv(obj['Body'])
                df['date'] = date_str
                data_frames.append(df)
            except Exception as e:
                print(f\"Failed to fetch data for {date_str}: {e}\")
        
        return pd.concat(data_frames, ignore_index=True) if data_frames else pd.DataFrame()
    
    def analyze_data_drift(self, reference_data, current_data, output_path=None):
        \"\"\"分析数据漂移\"\"\"
        # 创建Evidently配置文件
        profile = Profile(sections=[DataDriftProfileSection()])
        
        # 生成分析报告
        profile.calculate(reference_data, current_data, column_mapping=None)
        
        # 提取漂移结果
        drift_results = profile.get_content()['data_drift']
        
        # 详细分析
        detailed_analysis = self._extract_detailed_drift_info(drift_results)
        
        # 可视化
        if output_path:
            self._generate_drift_visualizations(detailed_analysis, output_path)
        
        return detailed_analysis
    
    def analyze_target_drift(self, reference_data, current_data, target_column, output_path=None):
        \"\"\"分析目标变量漂移\"\"\"
        profile = Profile(sections=[CatTargetDriftProfileSection()])
        
        # 准备数据
        ref_with_target = reference_data.copy()
        ref_with_target['target'] = reference_data[target_column]
        
        curr_with_target = current_data.copy()
        curr_with_target['target'] = current_data[target_column]
        
        profile.calculate(ref_with_target, curr_with_target, 
                         column_mapping={'target': 'cat'})
        
        target_drift = profile.get_content()['cat_target_drift']
        
        if output_path:
            self._generate_target_drift_visualizations(target_drift, output_path)
        
        return target_drift
    
    def analyze_model_performance_drift(self, reference_predictions, current_predictions, 
                                      target_column, problem_type='classification', output_path=None):
        \"\"\"分析模型性能漂移\"\"\"
        if problem_type == 'classification':
            profile = Profile(sections=[ClassificationPerformanceProfileSection()])
        else:
            profile = Profile(sections=[RegressionPerformanceProfileSection()])
        
        # 准备数据
        ref_data = reference_predictions.copy()
        ref_data['target'] = reference_predictions[target_column]
        
        curr_data = current_predictions.copy()
        curr_data['target'] = current_predictions[target_column]
        
        profile.calculate(ref_data, curr_data, column_mapping={'target': 'cat'})
        
        performance_drift = profile.get_content()[f'{problem_type}_performance']
        
        if output_path:
            self._generate_performance_visualizations(performance_drift, output_path)
        
        return performance_drift
    
    def _extract_detailed_drift_info(self, drift_results):
        \"\"\"提取详细的漂移信息\"\"\"
        detailed_info = {
            'overall_drift_score': drift_results['data_drift_score'],
            'drift_detected': drift_results['data_drift_detected'],
            'feature_drifts': {},
            'timestamp': datetime.now().isoformat()
        }
        
        # 分析各个特征
        for feature_name, feature_info in drift_results['columns'].items():
            detailed_info['feature_drifts'][feature_name] = {
                'drift_score': feature_info['drift_score'],
                'drift_detected': feature_info['drift_detected'],
                'stat_test': feature_info['stattest_name'],
                'p_value': feature_info.get('p_value', None)
            }
        
        return detailed_info
    
    def _generate_drift_visualizations(self, drift_analysis, output_path):
        \"\"\"生成漂移可视化图表\"\"\"
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        
        # 1. 特征漂移分数分布
        feature_scores = [info['drift_score'] for info in drift_analysis['feature_drifts'].values()]
        feature_names = list(drift_analysis['feature_drifts'].keys())
        
        axes[0, 0].bar(range(len(feature_scores)), feature_scores)
        axes[0, 0].set_xlabel('Features')
        axes[0, 0].set_ylabel('Drift Score')
        axes[0, 0].set_title('Feature Drift Scores')
        axes[0, 0].axhline(y=0.5, color='r', linestyle='--', label='Threshold')
        axes[0, 0].legend()
        
        # 2. 漂移检测结果
        drift_detected = [info['drift_detected'] for info in drift_analysis['feature_drifts'].values()]
        colors = ['red' if detected else 'green' for detected in drift_detected]
        axes[0, 1].scatter(range(len(drift_detected)), drift_detected, c=colors)
        axes[0, 1].set_xlabel('Features')
        axes[0, 1].set_ylabel('Drift Detected')
        axes[0, 1].set_title('Drift Detection Results')
        axes[0, 1].set_yticks([0, 1])
        axes[0, 1].set_yticklabels(['No', 'Yes'])
        
        # 3. 统计测试分布
        stat_tests = [info['stat_test'] for info in drift_analysis['feature_drifts'].values()]
        test_counts = pd.Series(stat_tests).value_counts()
        axes[1, 0].pie(test_counts.values, labels=test_counts.index, autopct='%1.1f%%')
        axes[1, 0].set_title('Statistical Tests Used')
        
        # 4. 时间趋势（如果有多天数据）
        axes[1, 1].text(0.5, 0.5, 'Time Trend Analysis\n(Requires temporal data)', 
                       ha='center', va='center', transform=axes[1, 1].transAxes)
        axes[1, 1].set_title('Temporal Drift Trend')
        
        plt.tight_layout()
        plt.savefig(f\"{output_path}/drift_analysis.png\", dpi=300, bbox_inches='tight')
        plt.close()

def run_batch_drift_analysis(**context):
    \"\"\"Airflow任务函数\"\"\"
    # 参数获取
    model_name = context['params']['model_name']
    s3_bucket = context['params']['s3_bucket']
    reference_date = context['params']['reference_date']
    analysis_date = context['params']['analysis_date']
    
    # 初始化分析器
    analyzer = BatchDriftAnalyzer(model_name, s3_bucket)
    
    # 获取参考数据和当前数据
    reference_end = datetime.strptime(reference_date, '%Y-%m-%d')
    reference_start = reference_end - timedelta(days=30)
    
    current_end = datetime.strptime(analysis_date, '%Y-%m-%d')
    current_start = current_end - timedelta(days=7)
    
    reference_data = analyzer.fetch_batch_data([reference_start, reference_end])
    current_data = analyzer.fetch_batch_data([current_start, current_end])
    
    if reference_data.empty or current_data.empty:
        raise ValueError(\"Insufficient data for drift analysis\")
    
    # 执行漂移分析
    drift_results = analyzer.analyze_data_drift(
        reference_data, 
        current_data,
        output_path=f\"/tmp/reports/{model_name}\"
    )
    
    # 记录到MLflow
    with mlflow.start_run():
        mlflow.log_param(\"model_name\", model_name)
        mlflow.log_param(\"reference_period\", f\"{reference_start} to {reference_end}\")
        mlflow.log_param(\"analysis_period\", f\"{current_start} to {current_end}\")
        mlflow.log_metric(\"overall_drift_score\", drift_results['overall_drift_score'])
        mlflow.log_metric(\"drift_detected\", int(drift_results['drift_detected']))
        mlflow.log_artifact(f\"/tmp/reports/{model_name}/drift_analysis.png\")
        
        # 记录特征级漂移
        for feature, info in drift_results['feature_drifts'].items():
            mlflow.log_metric(f\"drift_score_{feature}\", info['drift_score'])
    
    return drift_results

# Airflow DAG定义
default_args = {
    'owner': 'ml-team',
    'depends_on_past': False,
    'start_date': datetime(2026, 1, 1),
    'email_on_failure': True,
    'email_on_retry': False,
    'retries': 1,
    'retry_delay': timedelta(minutes=5)
}

dag = DAG(
    'model_drift_monitoring',
    default_args=default_args,
    description='Daily model drift monitoring',
    schedule_interval='@daily',
    catchup=False
)

drift_analysis_task = PythonOperator(
    task_id='batch_drift_analysis',
    python_callable=run_batch_drift_analysis,
    params={
        'model_name': 'customer_churn_model',
        's3_bucket': 'ml-production-data',
        'reference_date': '{{ ds }}',  # 当前执行日期
        'analysis_date': '{{ ds }}'
    },
    dag=dag
)

drift_analysis_task
```

---

## 四、自动重训练触发机制

### 4.1 重训练决策引擎

```python
# auto_retraining_trigger.py
import pandas as pd
import numpy as np
from datetime import datetime, timedelta
import mlflow
from kubernetes import client, config
import yaml
import json

class RetrainingTrigger:
    def __init__(self, model_name, trigger_config):
        self.model_name = model_name
        self.config = trigger_config
        self.drift_history = []
        self.performance_history = []
        
        # Kubernetes配置
        config.load_kube_config()
        self.batch_v1 = client.BatchV1Api()
        
    def evaluate_retraining_need(self, drift_analysis, performance_metrics):
        \"\"\"评估是否需要重训练\"\"\"
        # 记录历史数据
        self.drift_history.append({
            'timestamp': datetime.now().isoformat(),
            'drift_score': drift_analysis.get('overall_drift_score', 0),
            'drift_detected': drift_analysis.get('drift_detected', False)
        })
        
        self.performance_history.append({
            'timestamp': datetime.now().isoformat(),
            'accuracy': performance_metrics.get('accuracy', 0),
            'precision': performance_metrics.get('precision', 0),
            'recall': performance_metrics.get('recall', 0)
        })
        
        # 多维度评估
        drift_trigger = self._evaluate_drift_trigger(drift_analysis)
        performance_trigger = self._evaluate_performance_trigger(performance_metrics)
        business_trigger = self._evaluate_business_trigger()
        
        # 综合决策
        should_retrain = drift_trigger or performance_trigger or business_trigger
        
        decision = {
            'should_retrain': should_retrain,
            'triggers': {
                'drift': drift_trigger,
                'performance': performance_trigger,
                'business': business_trigger
            },
            'confidence': self._calculate_decision_confidence(
                drift_trigger, performance_trigger, business_trigger
            ),
            'recommended_action': self._get_recommended_action(should_retrain),
            'timestamp': datetime.now().isoformat()
        }
        
        # 记录决策到MLflow
        self._log_decision(decision)
        
        return decision
    
    def _evaluate_drift_trigger(self, drift_analysis):
        \"\"\"评估漂移触发条件\"\"\"
        if not drift_analysis:
            return False
            
        # 检查整体漂移分数
        overall_drift = drift_analysis.get('overall_drift_score', 0)
        if overall_drift > self.config['drift_threshold']:
            return True
            
        # 检查关键特征漂移
        critical_features = self.config.get('critical_features', [])
        feature_drifts = drift_analysis.get('feature_drifts', {})
        
        for feature in critical_features:
            if feature in feature_drifts:
                if feature_drifts[feature]['drift_score'] > self.config['feature_drift_threshold']:
                    return True
        
        # 检查持续漂移趋势
        if len(self.drift_history) >= 3:
            recent_drifts = [entry['drift_score'] for entry in self.drift_history[-3:]]
            if all(drift > self.config['drift_threshold'] * 0.8 for drift in recent_drifts):
                return True
                
        return False
    
    def _evaluate_performance_trigger(self, performance_metrics):
        \"\"\"评估性能触发条件\"\"\"
        if not performance_metrics:
            return False
            
        # 检查准确率下降
        current_accuracy = performance_metrics.get('accuracy', 1.0)
        if len(self.performance_history) >= 2:
            previous_accuracy = self.performance_history[-2]['accuracy']
            accuracy_drop = previous_accuracy - current_accuracy
            
            if accuracy_drop > self.config['accuracy_drop_threshold']:
                return True
        
        # 检查精确率/召回率平衡
        precision = performance_metrics.get('precision', 1.0)
        recall = performance_metrics.get('recall', 1.0)
        f1_score = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
        
        if f1_score < self.config['min_f1_threshold']:
            return True
            
        return False
    
    def _evaluate_business_trigger(self):
        \"\"\"评估业务触发条件\"\"\"
        # 检查业务指标异常
        # 例如：错误成本增加、客户投诉增多等
        business_metrics = self._fetch_business_metrics()
        
        if business_metrics.get('error_cost_increase', 0) > self.config['max_error_cost_increase']:
            return True
            
        if business_metrics.get('customer_complaints', 0) > self.config['max_complaints_threshold']:
            return True
            
        return False
    
    def _fetch_business_metrics(self):
        \"\"\"获取业务指标（模拟实现）\"\"\"
        # 实际实现中应从业务系统获取真实数据
        return {
            'error_cost_increase': np.random.uniform(0, 0.2),
            'customer_complaints': np.random.poisson(5)
        }
    
    def _calculate_decision_confidence(self, drift_trigger, performance_trigger, business_trigger):
        \"\"\"计算决策置信度\"\"\"
        trigger_count = sum([drift_trigger, performance_trigger, business_trigger])
        total_triggers = 3
        
        if trigger_count == 0:
            return 0.1
        elif trigger_count == 1:
            return 0.6
        elif trigger_count == 2:
            return 0.8
        else:
            return 0.95
    
    def _get_recommended_action(self, should_retrain):
        \"\"\"获取推荐行动\"\"\"
        if not should_retrain:
            return \"continue_monitoring\"
        
        # 根据触发类型推荐具体行动
        if len(self.drift_history) >= 3 and \
           all(entry['drift_detected'] for entry in self.drift_history[-3:]):
            return \"immediate_retraining\"
        elif len(self.performance_history) >= 2:
            recent_perf_drop = self.performance_history[-1]['accuracy'] < \
                              self.performance_history[-2]['accuracy']
            if recent_perf_drop:
                return \"urgent_retraining\"
        else:
            return \"scheduled_retraining\"
    
    def _log_decision(self, decision):
        \"\"\"记录决策到MLflow\"\"\"
        with mlflow.start_run():
            mlflow.log_param(\"model_name\", self.model_name)
            mlflow.log_param(\"decision_timestamp\", decision['timestamp'])
            mlflow.log_metric(\"should_retrain\", int(decision['should_retrain']))
            mlflow.log_metric(\"decision_confidence\", decision['confidence'])
            
            # 记录触发条件
            for trigger_type, triggered in decision['triggers'].items():
                mlflow.log_metric(f\"{trigger_type}_trigger\", int(triggered))
    
    def trigger_retraining_job(self, decision):
        \"\"\"触发重训练作业\"\"\"
        if not decision['should_retrain']:
            return None
            
        # 创建Kubernetes Job配置
        job_config = self._create_training_job_config(decision)
        
        # 提交Job
        try:
            job = self.batch_v1.create_namespaced_job(
                namespace=\"ml-training\",
                body=job_config
            )
            
            print(f\"Retraining job created: {job.metadata.name}\")
            return job.metadata.name
        except Exception as e:
            print(f\"Failed to create retraining job: {e}\")
            return None
    
    def _create_training_job_config(self, decision):
        \"\"\"创建训练作业配置\"\"\"
        job_template = {
            \"apiVersion\": \"batch/v1\",
            \"kind\": \"Job\",
            \"metadata\": {
                \"name\": f\"{self.model_name}-retraining-{datetime.now().strftime('%Y%m%d-%H%M%S')}\" ,
                \"labels\": {
                    \"app\": \"ml-training\",
                    \"model\": self.model_name,
                    \"priority\": decision['recommended_action']
                }
            },
            \"spec\": {
                \"backoffLimit\": 3,
                \"template\": {
                    \"spec\": {
                        \"containers\": [{
                            \"name\": \"trainer\",
                            \"image\": self.config['training_image'],
                            \"command\": [\"python\", \"/app/train.py\"],
                            \"args\": [
                                \"--model-name\", self.model_name,
                                \"--retrain-reason\", decision['recommended_action'],
                                \"--confidence\", str(decision['confidence'])
                            ],
                            \"env\": [
                                {\"name\": \"MLFLOW_TRACKING_URI\", \"value\": self.config['mlflow_uri']},
                                {\"name\": \"TRAINING_DATA_PATH\", \"value\": self.config['data_path']},
                                {\"name\": \"MODEL_REGISTRY_URI\", \"value\": self.config['model_registry_uri']}
                            ],
                            \"resources\": {
                                \"requests\": {
                                    \"cpu\": self.config['cpu_request'],
                                    \"memory\": self.config['memory_request']
                                },
                                \"limits\": {
                                    \"cpu\": self.config['cpu_limit'],
                                    \"memory\": self.config['memory_limit']
                                }
                            },
                            \"volumeMounts\": [{
                                \"name\": \"training-data\",
                                \"mountPath\": \"/data\"
                            }]
                        }],
                        \"volumes\": [{
                            \"name\": \"training-data\",
                            \"persistentVolumeClaim\": {
                                \"claimName\": \"training-data-pvc\"
                            }
                        }],
                        \"restartPolicy\": \"Never\"
                    }
                }
            }
        }
        
        return job_template

# 使用示例
trigger_config = {
    'drift_threshold': 0.5,
    'feature_drift_threshold': 0.3,
    'accuracy_drop_threshold': 0.05,
    'min_f1_threshold': 0.7,
    'max_error_cost_increase': 0.15,
    'max_complaints_threshold': 10,
    'critical_features': ['income', 'credit_score', 'age'],
    'training_image': 'company/ml-trainer:v2.0',
    'mlflow_uri': 'http://mlflow-server:5000',
    'data_path': '/data/latest_training_data.parquet',
    'model_registry_uri': 'models:/customer_churn_model/Production',
    'cpu_request': '2',
    'memory_request': '4Gi',
    'cpu_limit': '4',
    'memory_limit': '8Gi'
}

retrainer = RetrainingTrigger('customer_churn_model', trigger_config)

# 模拟漂移分析结果
drift_analysis = {
    'overall_drift_score': 0.65,
    'drift_detected': True,
    'feature_drifts': {
        'income': {'drift_score': 0.72, 'drift_detected': True},
        'credit_score': {'drift_score': 0.45, 'drift_detected': False},
        'age': {'drift_score': 0.58, 'drift_detected': True}
    }
}

# 模拟性能指标
performance_metrics = {
    'accuracy': 0.82,
    'precision': 0.78,
    'recall': 0.85
}

# 评估重训练需求
decision = retrainer.evaluate_retraining_need(drift_analysis, performance_metrics)
print(\"Retraining decision:\", json.dumps(decision, indent=2))

# 如果需要重训练，则触发作业
if decision['should_retrain']:
    job_name = retrainer.trigger_retraining_job(decision)
    print(f\"Retraining job triggered: {job_name}\")
```

---

## 五、漂移监控告警系统

### 5.1 多层级告警配置

```yaml
# drift-alerting-config.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: model-drift-alerts
  namespace: monitoring
spec:
  groups:
  - name: model.drift.rules
    rules:
    # 紧急告警 - 高严重性漂移
    - alert: CriticalModelDrift
      expr: |
        model_drift_score{severity=\"critical\"} > 0
      for: 2m
      labels:
        severity: critical
        team: ml-team
      annotations:
        summary: \"Critical model drift detected\"
        description: \"Model {{ $labels.model }} has critical drift (score: {{ $value }}). Immediate attention required.\"
        runbook_url: \"https://wiki.company.com/ml-ops/model-drift-runbook\"
    
    # 警告告警 - 中等严重性漂移
    - alert: SignificantModelDrift
      expr: |
        model_drift_score{severity=\"high\"} > 0
      for: 10m
      labels:
        severity: warning
        team: ml-team
      annotations:
        summary: \"Significant model drift detected\"
        description: \"Model {{ $labels.model }} showing significant drift. Investigation recommended.\"
    
    # 通知告警 - 轻微漂移
    - alert: ModelDriftDetected
      expr: |
        model_drift_score{severity=\"medium\"} > 0
      for: 30m
      labels:
        severity: info
        team: ml-team
      annotations:
        summary: \"Model drift detected\"
        description: \"Model {{ $labels.model }} showing minor drift. Monitor closely.\"

    # 性能相关告警
    - alert: ModelAccuracyDrop
      expr: |
        model_accuracy < 0.8
      for: 15m
      labels:
        severity: warning
        team: ml-team
      annotations:
        summary: \"Model accuracy below threshold\"
        description: \"Model {{ $labels.model }} accuracy {{ $value }} is below 80% threshold\"
    
    - alert: PredictionLatencyHigh
      expr: |
        histogram_quantile(0.95, sum(rate(prediction_latency_seconds_bucket[5m])) by (le, model)) > 0.5
      for: 5m
      labels:
        severity: warning
        team: ml-platform
      annotations:
        summary: \"High prediction latency\"
        description: \"Model {{ $labels.model }} 95th percentile latency {{ $value }}s exceeds 500ms\"

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-config
  namespace: monitoring
data:
  config.yml: |
    global:
      smtp_smarthost: 'smtp.company.com:587'
      smtp_from: 'alerts@company.com'
      smtp_auth_username: 'alerts'
      smtp_auth_password: 'password'
    
    route:
      group_by: ['alertname', 'team']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 3h
      receiver: 'ml-team'
      
      routes:
      - match:
          team: 'ml-team'
        receiver: 'ml-team-pager'
        routes:
        - match:
            severity: 'critical'
          receiver: 'ml-team-critical'
        - match:
            severity: 'warning'
          receiver: 'ml-team-warning'
    
    receivers:
    - name: 'ml-team'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#ml-alerts'
        send_resolved: true
        title: '{{ template \"slack.title\" . }}'
        text: '{{ template \"slack.text\" . }}'
    
    - name: 'ml-team-pager'
      webhook_configs:
      - url: 'http://pagerduty-proxy:8080/pagerduty'
        send_resolved: true
    
    - name: 'ml-team-critical'
      pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_SERVICE_KEY'
        description: '{{ .CommonAnnotations.summary }}'
    
    - name: 'ml-team-warning'
      email_configs:
      - to: 'ml-team@company.com'
        send_resolved: true
        html: '{{ template \"email.default.html\" . }}'
```

### 5.2 漂移仪表板配置

```json
{
  "dashboard": {
    "title": "Model Drift & Performance Monitoring",
    "panels": [
      {
        "title": "Overall Drift Status",
        "type": "stat",
        "targets": [
          {
            "expr": "max(model_drift_score) by (model)",
            "legendFormat": "{{model}}"
          }
        ],
        "thresholds": [
          { "value": 0.3, "color": "green" },
          { "value": 0.6, "color": "yellow" },
          { "value": 0.8, "color": "red" }
        ]
      },
      {
        "title": "Feature Drift Heatmap",
        "type": "heatmap",
        "targets": [
          {
            "expr": "model_feature_drift_score",
            "legendFormat": "{{model}} - {{feature}}"
          }
        ],
        "color": {
          "mode": "scheme",
          "scheme": "Reds"
        }
      },
      {
        "title": "Model Performance Trends",
        "type": "graph",
        "targets": [
          {
            "expr": "model_accuracy",
            "legendFormat": "{{model}} Accuracy"
          },
          {
            "expr": "model_precision",
            "legendFormat": "{{model}} Precision"
          },
          {
            "expr": "model_recall",
            "legendFormat": "{{model}} Recall"
          }
        ]
      },
      {
        "title": "Drift Detection Timeline",
        "type": "timeline",
        "targets": [
          {
            "expr": "model_drift_events_total",
            "legendFormat": "{{model}} - {{severity}}"
          }
        ]
      },
      {
        "title": "Retraining Jobs Status",
        "type": "table",
        "targets": [
          {
            "expr": "kube_job_status_succeeded{job_name=~\".*retraining.*\"}",
            "legendFormat": "Successful"
          },
          {
            "expr": "kube_job_status_failed{job_name=~\".*retraining.*\"}",
            "legendFormat": "Failed"
          }
        ]
      }
    ],
    "templating": {
      "list": [
        {
          "name": "model",
          "type": "query",
          "datasource": "prometheus",
          "refresh": 1,
          "query": "label_values(model_drift_score, model)"
        }
      ]
    }
  }
}
```

---

**维护者**: Model Operations Team | **最后更新**: 2026-02 | **版本**: v1.0