# AutoML与超参数调优

## 一、AutoML架构全景

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        AutoML完整流程                                      │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                  │
│  │ 数据预处理   │───▶│ 特征工程     │───▶│ 模型选择     │                  │
│  │ AutoFE      │    │ AutoFeature │    │ NAS         │                  │
│  └─────────────┘    └─────────────┘    └─────────────┘                  │
│                                                │                          │
│                                                ▼                          │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                  │
│  │ 超参数优化   │◀───│ 模型训练     │◀───│ 架构搜索     │                  │
│  │ HPO         │    │ Distributed │    │ DARTS/ENAS  │                  │
│  └─────────────┘    └─────────────┘    └─────────────┘                  │
│       │                    │                                              │
│       │                    ▼                                              │
│       │            ┌─────────────┐                                        │
│       └───────────▶│ 模型评估     │                                        │
│                    │ Validation  │                                        │
│                    └─────────────┘                                        │
│                            │                                              │
│                            ▼                                              │
│                    ┌─────────────┐                                        │
│                    │ 模型部署     │                                        │
│                    │ Production  │                                        │
│                    └─────────────┘                                        │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## 二、超参数优化算法

### 2.1 算法对比

| 算法 | 类型 | 优点 | 缺点 | 适用场景 |
|-----|------|------|------|---------|
| **Grid Search** | 网格搜索 | 简单、可重现 | 指数级复杂度 | 小搜索空间、离散参数 |
| **Random Search** | 随机搜索 | 高效、易并行 | 无利用历史信息 | 中等搜索空间、初步探索 |
| **Bayesian Optimization** | 贝叶斯优化 | 样本高效、智能 | 计算开销大 | 昂贵训练、连续参数 |
| **Hyperband/ASHA** | 早停策略 | 极快、资源高效 | 依赖性能曲线 | 大规模并行、快速筛选 |
| **Population Based Training** | 进化算法 | 动态调整、在线优化 | 需要多副本 | 长时间训练、RL |
| **BOHB** | 混合算法 | 结合BO+HB优点 | 实现复杂 | 生产环境推荐 |

### 2.2 贝叶斯优化原理

```python
"""
贝叶斯优化核心思想：
1. 构建目标函数的概率模型（高斯过程）
2. 使用采集函数（Acquisition Function）决定下一个采样点
3. 评估目标函数，更新概率模型
4. 重复2-3直至收敛

数学表示：
- 目标：max f(x), x ∈ X
- 高斯过程：f(x) ~ GP(μ(x), k(x,x'))
- 采集函数：α(x) = EI(x) | UCB(x) | PI(x)
  - EI (Expected Improvement): 期望改进
  - UCB (Upper Confidence Bound): 上置信界
  - PI (Probability of Improvement): 改进概率
"""

from sklearn.gaussian_process import GaussianProcessRegressor
from sklearn.gaussian_process.kernels import Matern
from scipy.stats import norm
import numpy as np

class BayesianOptimizer:
    def __init__(self, bounds, n_iter=50):
        self.bounds = bounds
        self.n_iter = n_iter
        self.X_observed = []
        self.y_observed = []
        
        # 高斯过程
        kernel = Matern(nu=2.5)
        self.gp = GaussianProcessRegressor(
            kernel=kernel,
            alpha=1e-6,
            normalize_y=True,
            n_restarts_optimizer=10
        )
    
    def acquisition_function(self, X, xi=0.01):
        """Expected Improvement采集函数"""
        mu, sigma = self.gp.predict(X, return_std=True)
        
        if len(self.y_observed) == 0:
            return mu
        
        mu_best = np.max(self.y_observed)
        
        with np.errstate(divide='warn'):
            improvement = mu - mu_best - xi
            Z = improvement / sigma
            ei = improvement * norm.cdf(Z) + sigma * norm.pdf(Z)
            ei[sigma == 0.0] = 0.0
        
        return ei
    
    def suggest(self):
        """推荐下一个采样点"""
        if len(self.X_observed) == 0:
            # 随机初始化
            return np.random.uniform(self.bounds[:, 0], self.bounds[:, 1])
        
        # 拟合高斯过程
        self.gp.fit(self.X_observed, self.y_observed)
        
        # 最大化采集函数
        X_candidates = np.random.uniform(
            self.bounds[:, 0],
            self.bounds[:, 1],
            size=(1000, len(self.bounds))
        )
        ei = self.acquisition_function(X_candidates)
        
        return X_candidates[np.argmax(ei)]
    
    def observe(self, X, y):
        """记录观测结果"""
        self.X_observed.append(X)
        self.y_observed.append(y)

# 使用示例
def objective_function(params):
    """目标函数：训练模型并返回验证精度"""
    lr, batch_size, dropout = params
    # 训练代码...
    accuracy = train_and_evaluate(lr, batch_size, dropout)
    return accuracy

# 定义搜索空间
bounds = np.array([
    [1e-5, 1e-2],  # learning_rate
    [16, 128],      # batch_size
    [0.1, 0.5]      # dropout
])

optimizer = BayesianOptimizer(bounds, n_iter=30)

for i in range(30):
    # 推荐参数
    params = optimizer.suggest()
    
    # 评估
    score = objective_function(params)
    
    # 记录结果
    optimizer.observe(params, score)
    
    print(f"Iteration {i+1}: params={params}, score={score}")

# 最佳参数
best_idx = np.argmax(optimizer.y_observed)
best_params = optimizer.X_observed[best_idx]
best_score = optimizer.y_observed[best_idx]
print(f"Best params: {best_params}, Best score: {best_score}")
```

---

## 三、Optuna深度实践

### 3.1 Optuna高级特性

```python
import optuna
from optuna.pruners import MedianPruner
from optuna.samplers import TPESampler
import mlflow
import torch
from transformers import Trainer, TrainingArguments

# 1. 定义目标函数
def objective(trial):
    """Optuna目标函数"""
    
    # 超参数采样
    params = {
        "learning_rate": trial.suggest_float("learning_rate", 1e-5, 1e-3, log=True),
        "per_device_train_batch_size": trial.suggest_categorical("batch_size", [16, 32, 64]),
        "num_train_epochs": trial.suggest_int("num_epochs", 2, 5),
        "warmup_ratio": trial.suggest_float("warmup_ratio", 0.0, 0.2),
        "weight_decay": trial.suggest_float("weight_decay", 0.0, 0.1),
        
        # 模型架构参数
        "num_hidden_layers": trial.suggest_int("num_hidden_layers", 6, 12),
        "hidden_dropout_prob": trial.suggest_float("dropout", 0.1, 0.3),
        "attention_probs_dropout_prob": trial.suggest_float("attention_dropout", 0.1, 0.3)
    }
    
    # MLflow追踪
    with mlflow.start_run(nested=True):
        mlflow.log_params(params)
        
        # 构建模型
        model_config = AutoConfig.from_pretrained("bert-base-uncased")
        model_config.num_hidden_layers = params["num_hidden_layers"]
        model_config.hidden_dropout_prob = params["hidden_dropout_prob"]
        model_config.attention_probs_dropout_prob = params["attention_probs_dropout_prob"]
        
        model = AutoModelForSequenceClassification.from_config(model_config)
        
        # 训练参数
        training_args = TrainingArguments(
            output_dir="/tmp/optuna_trial",
            learning_rate=params["learning_rate"],
            per_device_train_batch_size=params["per_device_train_batch_size"],
            num_train_epochs=params["num_train_epochs"],
            warmup_ratio=params["warmup_ratio"],
            weight_decay=params["weight_decay"],
            evaluation_strategy="epoch",
            save_strategy="no",
            load_best_model_at_end=False
        )
        
        # Trainer回调：中间剪枝
        class OptunaCallback:
            def __init__(self, trial):
                self.trial = trial
            
            def on_evaluate(self, args, state, control, metrics, **kwargs):
                # 报告中间结果
                accuracy = metrics.get("eval_accuracy")
                self.trial.report(accuracy, state.epoch)
                
                # 检查是否应该剪枝
                if self.trial.should_prune():
                    raise optuna.TrialPruned()
        
        trainer = Trainer(
            model=model,
            args=training_args,
            train_dataset=train_dataset,
            eval_dataset=eval_dataset,
            callbacks=[OptunaCallback(trial)]
        )
        
        # 训练
        trainer.train()
        
        # 最终评估
        eval_results = trainer.evaluate()
        accuracy = eval_results["eval_accuracy"]
        
        mlflow.log_metric("accuracy", accuracy)
        
        return accuracy

# 2. 创建Study（支持分布式）
study = optuna.create_study(
    study_name="bert-classification-hpo",
    direction="maximize",
    storage="postgresql://optuna:password@postgres.ai-platform.svc.cluster.local:5432/optuna",
    load_if_exists=True,
    
    # TPE采样器
    sampler=TPESampler(
        n_startup_trials=10,  # 前10次随机采样
        n_ei_candidates=24,
        seed=42
    ),
    
    # Median剪枝器
    pruner=MedianPruner(
        n_startup_trials=5,
        n_warmup_steps=2,
        interval_steps=1
    )
)

# 3. 运行优化
study.optimize(
    objective,
    n_trials=100,
    timeout=7200,  # 2小时超时
    n_jobs=1,  # 单进程（Kubernetes并行）
    show_progress_bar=True
)

# 4. 结果分析
print(f"Best trial: {study.best_trial.number}")
print(f"Best value: {study.best_value}")
print(f"Best params: {study.best_params}")

# 统计信息
print(f"Finished trials: {len(study.trials)}")
print(f"Pruned trials: {len(study.get_trials(states=[optuna.trial.TrialState.PRUNED]))}")
print(f"Complete trials: {len(study.get_trials(states=[optuna.trial.TrialState.COMPLETE]))}")

# 5. 可视化
import optuna.visualization as vis

# 优化历史
fig = vis.plot_optimization_history(study)
fig.write_html("optuna_history.html")

# 参数重要性
fig = vis.plot_param_importances(study)
fig.write_html("optuna_importances.html")

# 平行坐标图
fig = vis.plot_parallel_coordinate(study)
fig.write_html("optuna_parallel.html")

# 超参数关系
fig = vis.plot_contour(study, params=["learning_rate", "batch_size"])
fig.write_html("optuna_contour.html")
```

### 3.2 Kubernetes分布式Optuna

```yaml
# PostgreSQL存储
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: optuna-postgres
  namespace: ai-platform
spec:
  serviceName: optuna-postgres
  replicas: 1
  selector:
    matchLabels:
      app: optuna-postgres
  template:
    metadata:
      labels:
        app: optuna-postgres
    spec:
      containers:
      - name: postgres
        image: postgres:15-alpine
        ports:
        - containerPort: 5432
        env:
        - name: POSTGRES_DB
          value: optuna
        - name: POSTGRES_USER
          value: optuna
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: optuna-postgres-secret
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
      resources:
        requests:
          storage: 50Gi
---
# Optuna Worker Job
apiVersion: batch/v1
kind: Job
metadata:
  name: optuna-hpo-workers
  namespace: ai-platform
spec:
  parallelism: 20  # 20个并行worker
  completions: 100  # 总共100次trial
  template:
    metadata:
      labels:
        app: optuna-worker
    spec:
      restartPolicy: OnFailure
      containers:
      - name: worker
        image: myregistry/optuna-trainer:latest
        command:
        - python
        - optimize.py
        - --study-name=bert-classification-hpo
        - --n-trials=1
        
        env:
        - name: OPTUNA_STORAGE
          value: "postgresql://optuna:$(POSTGRES_PASSWORD)@optuna-postgres:5432/optuna"
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: optuna-postgres-secret
              key: password
        - name: MLFLOW_TRACKING_URI
          value: "http://mlflow-server:5000"
        
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
        - name: code
          mountPath: /workspace
        - name: dshm
          mountPath: /dev/shm
      
      volumes:
      - name: code
        configMap:
          name: optuna-training-code
      - name: dshm
        emptyDir:
          medium: Memory
          sizeLimit: 16Gi
```

---

## 四、Ray Tune大规模并行

### 4.1 Ray Tune架构

```python
from ray import tune
from ray.tune import CLIReporter
from ray.tune.schedulers import ASHAScheduler, PopulationBasedTraining
from ray.tune.integration.mlflow import MLflowLoggerCallback
import torch
from functools import partial

def train_model(config, checkpoint_dir=None, data_dir=None):
    """训练函数"""
    import torch
    import torch.nn as nn
    from torch.utils.data import DataLoader
    from transformers import AutoModelForSequenceClassification, Trainer, TrainingArguments
    
    # 构建模型
    model = AutoModelForSequenceClassification.from_pretrained(
        "bert-base-uncased",
        num_labels=2
    )
    
    # 训练参数
    training_args = TrainingArguments(
        output_dir="/tmp/ray_tune",
        learning_rate=config["learning_rate"],
        per_device_train_batch_size=config["batch_size"],
        num_train_epochs=config["num_epochs"],
        evaluation_strategy="epoch",
        save_strategy="no"
    )
    
    # 数据加载
    train_dataset = load_dataset(data_dir, "train")
    eval_dataset = load_dataset(data_dir, "eval")
    
    # Trainer
    trainer = Trainer(
        model=model,
        args=training_args,
        train_dataset=train_dataset,
        eval_dataset=eval_dataset
    )
    
    # 训练循环（支持checkpointing）
    for epoch in range(config["num_epochs"]):
        trainer.train()
        eval_results = trainer.evaluate()
        
        # 报告指标给Ray Tune
        tune.report(
            accuracy=eval_results["eval_accuracy"],
            loss=eval_results["eval_loss"]
        )

# 搜索空间
search_space = {
    "learning_rate": tune.loguniform(1e-5, 1e-3),
    "batch_size": tune.choice([16, 32, 64]),
    "num_epochs": tune.choice([3, 5, 7]),
    "weight_decay": tune.uniform(0.0, 0.1)
}

# ASHA调度器（异步连续减半）
scheduler = ASHAScheduler(
    metric="accuracy",
    mode="max",
    max_t=10,  # 最大epoch数
    grace_period=1,  # 至少运行1个epoch
    reduction_factor=2  # 每轮淘汰50%
)

# CLIReporter美化输出
reporter = CLIReporter(
    metric_columns=["accuracy", "loss", "training_iteration"],
    max_progress_rows=20
)

# 运行超参数搜索
analysis = tune.run(
    partial(train_model, data_dir="/data"),
    resources_per_trial={"cpu": 8, "gpu": 1},
    config=search_space,
    num_samples=50,  # 50次trial
    scheduler=scheduler,
    progress_reporter=reporter,
    local_dir="/tmp/ray_results",
    
    # MLflow集成
    callbacks=[
        MLflowLoggerCallback(
            tracking_uri="http://mlflow-server:5000",
            experiment_name="ray-tune-hpo",
            save_artifact=True
        )
    ],
    
    # 容错
    max_failures=3,
    raise_on_failed_trial=False
)

# 最佳配置
best_trial = analysis.best_trial
print(f"Best trial config: {best_trial.config}")
print(f"Best trial final validation accuracy: {best_trial.last_result['accuracy']}")

# 获取最佳模型checkpoint
best_checkpoint = analysis.best_checkpoint
```

### 4.2 Population Based Training (PBT)

```python
from ray.tune.schedulers import PopulationBasedTraining

# PBT调度器
pbt_scheduler = PopulationBasedTraining(
    time_attr="training_iteration",
    metric="accuracy",
    mode="max",
    
    # 扰动超参数
    perturbation_interval=2,  # 每2个epoch扰动一次
    hyperparam_mutations={
        "learning_rate": lambda: tune.loguniform(1e-5, 1e-3).sample(),
        "weight_decay": lambda: tune.uniform(0.0, 0.1).sample()
    },
    
    # 探索策略
    resample_probability=0.25,  # 25%概率重新采样
    
    # 种群大小
    quantile_fraction=0.25,  # 淘汰底部25%
    
    # 资源配置
    log_config=True
)

analysis = tune.run(
    train_model,
    name="pbt_experiment",
    scheduler=pbt_scheduler,
    num_samples=16,  # 种群大小16
    config={
        "learning_rate": tune.loguniform(1e-5, 1e-3),
        "weight_decay": tune.uniform(0.0, 0.1),
        "batch_size": 32,  # 固定
        "num_epochs": 20
    },
    resources_per_trial={"cpu": 4, "gpu": 1},
    stop={"training_iteration": 20}
)
```

---

## 五、Neural Architecture Search (NAS)

### 5.1 DARTS可微架构搜索

```python
import torch
import torch.nn as nn

class DARTSCell(nn.Module):
    """DARTS搜索单元"""
    def __init__(self, C_in, C_out, stride=1):
        super().__init__()
        self.stride = stride
        
        # 候选操作
        self.ops = nn.ModuleList([
            nn.Identity(),
            nn.MaxPool2d(3, stride=stride, padding=1),
            nn.AvgPool2d(3, stride=stride, padding=1),
            SepConv(C_in, C_out, 3, stride),
            SepConv(C_in, C_out, 5, stride),
            DilConv(C_in, C_out, 3, stride, dilation=2),
            DilConv(C_in, C_out, 5, stride, dilation=2),
            nn.Sequential()  # zero operation
        ])
        
        # 架构参数（可学习）
        self.alpha = nn.Parameter(torch.randn(len(self.ops)))
    
    def forward(self, x):
        # 加权求和所有操作
        weights = torch.softmax(self.alpha, dim=0)
        return sum(w * op(x) for w, op in zip(weights, self.ops))

class DARTSNetwork(nn.Module):
    def __init__(self, C=16, num_cells=8, num_classes=10):
        super().__init__()
        self.stem = nn.Conv2d(3, C, 3, padding=1)
        
        # 堆叠DARTS单元
        self.cells = nn.ModuleList([
            DARTSCell(C, C) for _ in range(num_cells)
        ])
        
        self.classifier = nn.Linear(C, num_classes)
    
    def forward(self, x):
        x = self.stem(x)
        for cell in self.cells:
            x = cell(x)
        x = x.mean([2, 3])  # Global average pooling
        return self.classifier(x)
    
    def arch_parameters(self):
        """返回架构参数"""
        return [cell.alpha for cell in self.cells]
    
    def model_parameters(self):
        """返回模型权重参数"""
        params = []
        for name, param in self.named_parameters():
            if 'alpha' not in name:
                params.append(param)
        return params

# DARTS训练过程
def train_darts(model, train_loader, val_loader, epochs=50):
    # 两个优化器
    w_optimizer = torch.optim.SGD(
        model.model_parameters(),
        lr=0.025,
        momentum=0.9,
        weight_decay=3e-4
    )
    
    alpha_optimizer = torch.optim.Adam(
        model.arch_parameters(),
        lr=3e-4,
        betas=(0.5, 0.999),
        weight_decay=1e-3
    )
    
    for epoch in range(epochs):
        # 1. 更新架构参数α（在验证集上）
        for batch in val_loader:
            images, labels = batch
            
            alpha_optimizer.zero_grad()
            logits = model(images)
            loss = nn.CrossEntropyLoss()(logits, labels)
            loss.backward()
            alpha_optimizer.step()
        
        # 2. 更新模型权重w（在训练集上）
        for batch in train_loader:
            images, labels = batch
            
            w_optimizer.zero_grad()
            logits = model(images)
            loss = nn.CrossEntropyLoss()(logits, labels)
            loss.backward()
            w_optimizer.step()
        
        print(f"Epoch {epoch+1}: Architecture weights updated")
    
    # 导出最终架构
    for i, cell in enumerate(model.cells):
        best_op_idx = torch.argmax(cell.alpha).item()
        print(f"Cell {i}: Best operation index = {best_op_idx}")
```

### 5.2 Once-for-All (OFA) Network

```python
"""
Once-for-All思想：
训练一个超网络，支持多种架构配置（深度、宽度、kernel size）
部署时根据硬件约束选择子网络，无需重新训练

优势：
- 一次训练，多次部署
- 支持边缘设备（移动端、IoT）
- 延迟-精度权衡
"""

from ofa.model_zoo import ofa_net
from ofa.nas.accuracy_predictor import AccuracyPredictor
from ofa.nas.efficiency_predictor import LatencyPredictor

# 加载预训练OFA网络
ofa_network = ofa_net('ofa_mbv3_d234_e346_k357_w1.2', pretrained=True)

# 搜索最优子网络
accuracy_predictor = AccuracyPredictor(ofa_network)
latency_predictor = LatencyPredictor(device='note10')  # Samsung Note10

# 约束条件
latency_constraint = 20  # 20ms延迟

# 进化搜索
best_config = evolutionary_search(
    ofa_network,
    accuracy_predictor,
    latency_predictor,
    latency_constraint=latency_constraint,
    population_size=100,
    num_generations=30
)

print(f"Best config: {best_config}")
print(f"Estimated accuracy: {accuracy_predictor(best_config)}")
print(f"Estimated latency: {latency_predictor(best_config)}ms")

# 导出子网络
subnet = ofa_network.get_active_subnet(best_config)
torch.save(subnet.state_dict(), "optimized_subnet.pth")
```

---

## 六、AutoML平台对比

| 平台 | 算法支持 | 分布式 | NAS支持 | 易用性 | 开源 |
|-----|---------|--------|---------|--------|------|
| **Optuna** | BO, TPE, CMA-ES | ✅ | ❌ | ★★★★★ | ✅ |
| **Ray Tune** | Grid, Random, BO, ASHA, PBT | ✅ | ✅ | ★★★★☆ | ✅ |
| **Hyperopt** | TPE, Random | ❌ | ❌ | ★★★☆☆ | ✅ |
| **Keras Tuner** | Random, Hyperband, BO | ❌ | ✅ | ★★★★☆ | ✅ |
| **Auto-sklearn** | SMAC, meta-learning | ❌ | ❌ | ★★★★★ | ✅ |
| **Google Vertex AI** | BO, Grid | ✅ | ❌ | ★★★★☆ | ❌ |
| **Azure AutoML** | 多种 | ✅ | ✅ | ★★★★☆ | ❌ |

**推荐选择：**
- 深度学习：Ray Tune（大规模并行） + Optuna（精细优化）
- 传统ML：Auto-sklearn（快速baseline）
- 生产环境：Ray Tune + Kubernetes（弹性扩缩容）
- 学术研究：Optuna（灵活、可扩展）

---

## 七、多目标优化

### 7.1 帕累托前沿搜索

```python
import optuna

def multi_objective(trial):
    """多目标优化：精度 vs 延迟"""
    # 超参数
    num_layers = trial.suggest_int("num_layers", 6, 24)
    hidden_size = trial.suggest_categorical("hidden_size", [256, 512, 768, 1024])
    
    # 训练模型
    model = build_model(num_layers, hidden_size)
    accuracy = train_and_evaluate(model)
    
    # 推理延迟（ms）
    latency = benchmark_latency(model)
    
    return accuracy, latency

# 创建多目标Study
study = optuna.create_study(
    directions=["maximize", "minimize"],  # 最大化精度，最小化延迟
    study_name="multi-objective-hpo"
)

study.optimize(multi_objective, n_trials=100)

# 获取帕累托前沿
pareto_trials = study.best_trials

print(f"Number of Pareto optimal trials: {len(pareto_trials)}")

for trial in pareto_trials:
    print(f"Trial {trial.number}:")
    print(f"  Accuracy: {trial.values[0]:.4f}")
    print(f"  Latency: {trial.values[1]:.2f}ms")
    print(f"  Params: {trial.params}")

# 可视化帕累托前沿
import optuna.visualization as vis
fig = vis.plot_pareto_front(study, target_names=["Accuracy", "Latency"])
fig.write_html("pareto_front.html")
```

---

## 八、AutoML Pipeline

### 8.1 完整AutoML工作流

```python
from sklearn.pipeline import Pipeline
from sklearn.compose import ColumnTransformer
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.ensemble import RandomForestClassifier
from auto_sklearn.classification import AutoSklearnClassifier
import pandas as pd

# 加载数据
df = pd.read_csv("data.csv")
X = df.drop("target", axis=1)
y = df["target"]

# 自动特征工程
numeric_features = X.select_dtypes(include=['int64', 'float64']).columns
categorical_features = X.select_dtypes(include=['object']).columns

preprocessor = ColumnTransformer(
    transformers=[
        ('num', StandardScaler(), numeric_features),
        ('cat', OneHotEncoder(handle_unknown='ignore'), categorical_features)
    ]
)

# Auto-sklearn自动模型选择+超参数优化
automl = AutoSklearnClassifier(
    time_left_for_this_task=3600,  # 1小时
    per_run_time_limit=300,  # 每次trial 5分钟
    n_jobs=8,
    ensemble_size=50,
    ensemble_nbest=200,
    initial_configurations_via_metalearning=25,
    metric=autosklearn.metrics.accuracy
)

# 完整Pipeline
pipeline = Pipeline([
    ('preprocessor', preprocessor),
    ('classifier', automl)
])

# 训练（自动搜索）
pipeline.fit(X_train, y_train)

# 评估
y_pred = pipeline.predict(X_test)
accuracy = accuracy_score(y_test, y_pred)

print(f"Test Accuracy: {accuracy:.4f}")

# 查看Auto-sklearn找到的最佳模型
print(automl.show_models())

# 输出示例：
# [(0.52, SimpleClassificationPipeline(...RandomForest...)),
#  (0.28, SimpleClassificationPipeline(...GradientBoosting...)),
#  (0.20, SimpleClassificationPipeline(...SVM...))]
```

---

## 九、生产环境部署

### 9.1 超参数优化Job模板

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: hpo-bert-classification
  namespace: ai-platform
  labels:
    app: hpo
    model: bert
spec:
  parallelism: 10
  completions: 50
  template:
    metadata:
      labels:
        app: hpo-worker
    spec:
      restartPolicy: OnFailure
      
      # Init容器：等待Optuna DB就绪
      initContainers:
      - name: wait-for-db
        image: busybox:latest
        command:
        - sh
        - -c
        - |
          until nc -z optuna-postgres 5432; do
            echo "Waiting for PostgreSQL..."
            sleep 2
          done
      
      containers:
      - name: hpo-worker
        image: myregistry/optuna-bert-trainer:v1.0
        command:
        - python
        - /workspace/optimize.py
        - --study-name=bert-classification-hpo
        - --n-trials=1
        
        env:
        - name: OPTUNA_STORAGE
          value: "postgresql://optuna:$(POSTGRES_PASSWORD)@optuna-postgres:5432/optuna"
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: optuna-postgres-secret
              key: password
        - name: MLFLOW_TRACKING_URI
          value: "http://mlflow-server:5000"
        - name: MLFLOW_EXPERIMENT_NAME
          value: "hpo-bert-classification"
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
        - name: cache
          mountPath: /root/.cache
        - name: dshm
          mountPath: /dev/shm
      
      volumes:
      - name: workspace
        configMap:
          name: hpo-training-code
      - name: cache
        emptyDir: {}
      - name: dshm
        emptyDir:
          medium: Memory
          sizeLimit: 16Gi
      
      # 节点亲和性：优先使用Spot实例
      affinity:
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: karpenter.sh/capacity-type
                operator: In
                values: ["spot"]
```

### 9.2 成本优化策略

**Spot实例节省：**
```yaml
# Karpenter Provisioner for HPO
apiVersion: karpenter.sh/v1alpha5
kind: Provisioner
metadata:
  name: hpo-spot-provisioner
spec:
  requirements:
  - key: karpenter.sh/capacity-type
    operator: In
    values: ["spot"]
  - key: node.kubernetes.io/instance-type
    operator: In
    values: ["g5.xlarge", "g5.2xlarge", "g4dn.xlarge"]
  - key: nvidia.com/gpu
    operator: Exists
  
  # Spot中断处理
  ttlSecondsAfterEmpty: 30
  ttlSecondsUntilExpired: 3600  # 1小时
  
  limits:
    resources:
      nvidia.com/gpu: 50

# 成本节省：
# - On-Demand g5.xlarge: $1.006/小时
# - Spot g5.xlarge: ~$0.30/小时 (70% off)
# - 50次trial × 30分钟 = 25 GPU小时
# - On-Demand成本: $25.15
# - Spot成本: $7.50
# - 节省: $17.65 (70%)
```

---

## 十、最佳实践

### 10.1 超参数搜索策略

**1. 粗搜索 + 精搜索：**
```python
# Stage 1: 粗搜索（大范围、少样本）
coarse_search_space = {
    "learning_rate": tune.loguniform(1e-6, 1e-2),  # 4个数量级
    "batch_size": tune.choice([8, 16, 32, 64, 128]),
    "num_layers": tune.randint(6, 24)
}

coarse_analysis = tune.run(
    train_model,
    config=coarse_search_space,
    num_samples=20,  # 少量样本
    resources_per_trial={"gpu": 1}
)

best_coarse_lr = coarse_analysis.best_config["learning_rate"]

# Stage 2: 精搜索（小范围、多样本）
fine_search_space = {
    "learning_rate": tune.uniform(
        best_coarse_lr * 0.5,
        best_coarse_lr * 2.0
    ),  # 围绕最佳值
    "batch_size": tune.choice([32, 64]),  # 固定两个候选
    "num_layers": 12  # 固定
}

fine_analysis = tune.run(
    train_model,
    config=fine_search_space,
    num_samples=50,  # 更多样本
    resources_per_trial={"gpu": 1}
)
```

**2. 先优化学习率，后优化其他：**
```python
# 学习率对模型性能影响最大，优先优化
# Step 1: 只优化LR
lr_search = {
    "learning_rate": tune.loguniform(1e-5, 1e-3),
    "batch_size": 32,  # 固定
    "num_epochs": 3
}

# Step 2: 固定最佳LR，优化其他
best_lr = lr_analysis.best_config["learning_rate"]

other_search = {
    "learning_rate": best_lr,  # 固定
    "batch_size": tune.choice([16, 32, 64]),
    "weight_decay": tune.uniform(0.0, 0.1),
    "warmup_ratio": tune.uniform(0.0, 0.2)
}
```

### 10.2 避免过拟合搜索空间

```python
"""
常见错误：在测试集上评估超参数，导致信息泄露

正确做法：
1. 训练集：训练模型
2. 验证集：超参数优化
3. 测试集：最终评估（仅一次）
"""

# ❌ 错误示例
def objective_wrong(trial):
    model = train(config)
    accuracy = evaluate(model, test_dataset)  # 泄露测试集信息！
    return accuracy

# ✅ 正确示例
def objective_correct(trial):
    model = train(config, train_dataset)
    accuracy = evaluate(model, val_dataset)  # 使用验证集
    return accuracy

# 最终评估（仅一次）
best_model = load_model(best_trial)
final_accuracy = evaluate(best_model, test_dataset)
```

### 10.3 早停策略

```python
# Optuna MedianPruner
pruner = MedianPruner(
    n_startup_trials=10,  # 前10次trial不剪枝
    n_warmup_steps=2,  # 前2个epoch不剪枝
    interval_steps=1  # 每个epoch检查一次
)

# 在训练循环中报告中间结果
for epoch in range(num_epochs):
    train_loss = train_one_epoch()
    val_accuracy = validate()
    
    # 报告给Optuna
    trial.report(val_accuracy, epoch)
    
    # 检查是否应该剪枝
    if trial.should_prune():
        raise optuna.TrialPruned()

# 效果：节省50-70%计算资源
```

---

## 十一、监控与可视化

### 11.1 实时监控Dashboard

```python
import optuna
from optuna.visualization import plot_optimization_history, plot_param_importances
import streamlit as st

# Streamlit实时Dashboard
st.title("Hyperparameter Optimization Dashboard")

# 连接Optuna Study
study = optuna.load_study(
    study_name="bert-classification-hpo",
    storage="postgresql://optuna:password@postgres:5432/optuna"
)

# 实时刷新
while True:
    # 优化历史
    st.subheader("Optimization History")
    fig1 = plot_optimization_history(study)
    st.plotly_chart(fig1)
    
    # 参数重要性
    st.subheader("Hyperparameter Importances")
    fig2 = plot_param_importances(study)
    st.plotly_chart(fig2)
    
    # 统计信息
    st.subheader("Statistics")
    st.metric("Total Trials", len(study.trials))
    st.metric("Best Value", f"{study.best_value:.4f}")
    st.metric("Best Trial", study.best_trial.number)
    
    # 最佳参数
    st.subheader("Best Parameters")
    st.json(study.best_params)
    
    time.sleep(10)  # 每10秒刷新
```

---

**相关表格：**
- [111-AI基础设施架构](./111-ai-infrastructure.md)
- [112-分布式训练框架](./112-distributed-training-frameworks.md)
- [117-AI实验管理](./117-ai-experiment-management.md)

**版本信息：**
- Optuna: v3.5.0+
- Ray Tune: v2.9.0+
- Auto-sklearn: v0.15.0+
- Kubernetes: v1.27+
