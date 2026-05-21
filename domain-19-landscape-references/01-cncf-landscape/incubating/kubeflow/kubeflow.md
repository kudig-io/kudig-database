---
title: Kubeflow
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- job
- operator
- gpu
- nvidia
- kubeflow
- kserve
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubeflow 是什么
- 如何 Kubeflow
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Kubeflow
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- gpu-scheduling-basics
---

title: Kubeflow
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- job
- operator
- gpu
- nvidia
- kubeflow
- kserve
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kubeflow 是什么
- 如何 Kubeflow
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kubeflow
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Kubeflow

> **成熟度**: Incubating | **加入时间**: 2023-07 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.kubeflow.org |
| **GitHub** | https://github.com/kubeflow/kubeflow |
| **文档** | https://www.kubeflow.org/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Python, Go |
| **CNCF 分类** | AI/ML |

---

## 项目概述

### 简介
Kubeflow 是在 Kubernetes 上运行机器学习工作流的开源平台。它提供从实验到生产的完整 ML 生命周期管理，包括数据准备、模型训练、调参、部署和监控。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | Google 开源 |
| 2023-07 | 加入 CNCF Incubating |

### 核心定位
Kubeflow 是 Kubernetes 上 MLOps 的标准平台，让数据科学家可以专注于 ML 而非基础设施。

---

## 核心组件

```
┌─────────────────────────────────────────────────────────────────┐
│                     Kubeflow 组件                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    Kubeflow Pipelines                        ││
│  │  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐    ││
│  │  │  Data   │──►│ Train   │──►│ Evaluate│──►│ Deploy  │    ││
│  │  │ Prep    │   │ Model   │   │ Model   │   │ Model   │    ││
│  │  └─────────┘   └─────────┘   └─────────┘   └─────────┘    ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐       │
│  │   Notebooks   │  │   Training    │  │    KServe     │       │
│  │   (Jupyter)   │  │   Operator    │  │  (Inference)  │       │
│  │               │  │ TF/PyTorch/   │  │               │       │
│  │ 交互式开发    │  │ MPI/XGBoost   │  │ 模型部署服务  │       │
│  └───────────────┘  └───────────────┘  └───────────────┘       │
│                                                                  │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐       │
│  │    Katib      │  │   Spark       │  │   Feature     │       │
│  │   (AutoML)    │  │   Operator    │  │   Store       │       │
│  │               │  │               │  │               │       │
│  │ 超参数调优    │  │ Spark 集成    │  │ 特征管理      │       │
│  └───────────────┘  └───────────────┘  └───────────────┘       │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Kubeflow Pipelines 示例

```python
from kfp import dsl
from kfp.dsl import Input, Output, Dataset, Model

@dsl.component
def preprocess_data(raw_data: Input[Dataset], processed_data: Output[Dataset]):
    import pandas as pd
    df = pd.read_csv(raw_data.path)
    # 数据预处理
    df.to_csv(processed_data.path, index=False)

@dsl.component
def train_model(data: Input[Dataset], model: Output[Model]):
    import pickle
    from sklearn.ensemble import RandomForestClassifier
    # 训练模型
    clf = RandomForestClassifier()
    # ...
    with open(model.path, 'wb') as f:
        pickle.dump(clf, f)

@dsl.pipeline(name='ML Pipeline')
def ml_pipeline():
    preprocess = preprocess_data(raw_data=dsl.importer(...))
    train = train_model(data=preprocess.outputs['processed_data'])

# 编译并提交
from kfp import compiler
compiler.Compiler().compile(ml_pipeline, 'pipeline.yaml')
```

---

## 分布式训练

```yaml
# TFJob - TensorFlow 分布式训练
apiVersion: kubeflow.org/v1
kind: TFJob
metadata:
  name: mnist-distributed
spec:
  tfReplicaSpecs:
    PS:
      replicas: 2
      template:
        spec:
          containers:
            - name: tensorflow
              image: tensorflow/tensorflow:2.12.0
              command: ["python", "/app/train.py"]
    Worker:
      replicas: 4
      template:
        spec:
          containers:
            - name: tensorflow
              image: tensorflow/tensorflow:2.12.0
              resources:
                limits:
                  nvidia.com/gpu: 1
```

---

## 安装

```bash
# 使用 kustomize 安装
git clone https://github.com/kubeflow/manifests.git
cd manifests
while ! kustomize build example | kubectl apply -f -; do sleep 10; done
```

---

## 参考资源

- [官方文档](https://www.kubeflow.org/docs)
- [GitHub Repo](https://github.com/kubeflow/kubeflow)
- [CNCF 项目页面](https://www.cncf.io/projects/kubeflow/)
- [KServe](https://kserve.github.io/)
- [Katib](https://www.kubeflow.org/docs/components/katib/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/kserve.md|kserve]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
