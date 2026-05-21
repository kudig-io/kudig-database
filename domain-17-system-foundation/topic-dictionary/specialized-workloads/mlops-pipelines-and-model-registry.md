---
title: MLOps 流水线与模型仓库
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- job
- cronjob
- operator
- gpu
- kubeflow
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MLOps 流水线与模型仓库 是什么
- 如何 MLOps 流水线与模型仓库
trigger_keywords:
- MLOps
- 流水线与模型仓库
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
---

# MLOps 流水线与模型仓库

## 概述

**MLOps（Machine Learning Operations）** 是将 DevOps 工程实践应用于机器学习生命周期的方法论。2026 年的行业最佳实践要求 AI 基础设施具备完整的**数据准备、模型训练、实验追踪、模型注册、自动部署与监控反馈**能力。在 Kubernetes 上，MLOps 通常通过 **Kubeflow、MLflow、Airflow** 等工具链以及原生的 Jobs/CronJobs/Pipelines 来实现。

## 核心概念/原理

### 1. MLOps 生命周期

完整的 MLOps 生命周期包含以下阶段：
1. **数据工程**：ETL/ELT、数据清洗、特征工程
2. **实验管理**：超参数调优、模型对比、指标记录
3. **模型训练**：分布式训练、自动调参（AutoML）
4. **模型评估**：离线评估、A/B 测试、影子验证
5. **模型注册**：版本化存储、血缘追踪、审批流程
6. **模型部署**：金丝雀发布、蓝绿部署、自动回滚
7. **生产监控**：漂移检测（Drift Detection）、性能衰退告警
8. **持续训练（CT）**：根据新数据自动触发再训练

### 2. Kubeflow：Kubernetes 原生 MLOps 平台

**Kubeflow** 是专为 Kubernetes 设计的开源 MLOps 平台，核心组件包括：
- **Kubeflow Pipelines**：基于 Argo 的工作流编排引擎，定义可复用的 ML Pipeline
- **Kubeflow Notebooks**：为数据科学家提供 Jupyter/VSCode 开发环境
- **Katib**：自动超参数调优（Hyperparameter Tuning）
- **KServe**：集成在 Kubeflow 中的模型推理服务（现已独立为 CNCF 项目）
- **Training Operator**：支持 TensorFlow/PyTorch/MPIJob/XGBoost 的分布式训练

### 3. MLflow：模型生命周期管理

**MLflow** 专注于模型追踪、打包和注册：
- **Tracking**：记录实验参数、指标和 artifact
- **Projects**：标准化 ML 代码打包格式
- **Models**：跨平台模型打包格式，支持多种 serving flavor
- **Registry**：模型版本仓库，支持 staging/production 阶段流转

### 4. 特征商店（Feature Store）

特征商店确保训练阶段和在线推理阶段使用**一致的特征定义和数据**：
- **在线特征服务**：低延迟提供实时特征
- **离线特征存储**：批量提供训练数据集
- 主流方案：Feast、Tecton、Vertex AI Feature Store

## 关键机制或特性

### CI/CD/CT 流水线

```yaml
# 简化版 MLOps Pipeline 示例（Kubeflow Pipelines DSL）
@dsl.pipeline(
    name='Training Pipeline',
    description='End-to-end model training and deployment'
)
def training_pipeline(data_path: str):
    preprocess = preprocess_op(data_path)
    train = train_op(preprocess.outputs['processed_data'])
    evaluate = evaluate_op(train.outputs['model'])
    
    with dsl.Condition(evaluate.outputs['accuracy'] > 0.85):
        deploy = deploy_op(train.outputs['model'])
```

### 模型部署策略

| 策略 | 说明 | 风险 |
|------|------|------|
| **蓝绿部署** | 同时运行新旧版本，瞬时切换 | 资源翻倍 |
| **金丝雀发布** | 逐步将 1%→10%→100% 流量切换到新版本 | 需要精细监控 |
| **影子验证** | 新版本接收生产流量但不返回结果 | 安全但资源消耗大 |
| **A/B 测试** | 按用户比例分配流量对比模型效果 | 需要统计显著性 |

### 漂移检测与反馈闭环

- **数据漂移（Data Drift）**：输入数据分布与训练数据发生偏移
- **概念漂移（Concept Drift）**：特征与目标变量之间的关系发生变化
- **模型衰退监控**：通过 Prometheus/Grafana 监控模型预测准确率、延迟、错误率
- **反馈闭环**：收集用户评分或业务指标，定期触发再训练

## 使用场景

1. **自动化模型再训练**：使用 Kubernetes CronJob  nightly 执行数据更新、重训练、评估和自动部署
2. **多实验并行调参**：利用 Katib 启动数十个超参数搜索实验，自动筛选最优模型
3. **跨环境模型晋升**：模型从 Dev → Staging → Production 环境通过 MLflow Registry 自动流转
4. **实时推荐系统**：特征商店提供在线特征，KServe 提供低延迟推理，Pipeline 负责定期更新模型

## 最佳实践/注意事项

- **版本化一切**：数据、代码、模型、特征定义都必须版本化，确保实验可复现
- **训练与推理环境一致**：使用相同的容器镜像和依赖版本，避免"在我机器上能跑"的问题
- **Pipeline 即代码**：将 MLOps Pipeline 定义为 YAML 或 Python DSL，纳入 Git 版本控制
- **模型测试自动化**：除了单元测试，还需进行模型准确性测试、偏见检测和性能基准测试
- **敏感数据脱敏**：在数据摄取阶段即进行 PII 检测和匿名化，避免泄露到训练日志
- **资源配额管理**：为实验环境设置严格的 GPU/CPU 配额，防止科研团队无限制占用资源
- **Artifact 存储优化**：大模型 artifact 应存储在对象存储（S3/GCS）而非 Git 仓库中，Pipeline 中通过引用访问

## 参考链接

- [Kubeflow Official Documentation](https://www.kubeflow.org/docs/)
- [MLflow Documentation](https://mlflow.org/docs/latest/index.html)
- [Feast Feature Store](https://docs.feast.dev/)
- [Google SRE - MLOps Best Practices](https://cloud.google.com/architecture/mlops-continuous-delivery-and-automation-pipelines-in-machine-learning)
- [TLVTech - Building Production-Ready AI Infrastructure](https://www.tlvtech.io/post/building-ai-infrastructure)

## Related

- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
