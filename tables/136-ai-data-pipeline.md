# AI数据处理Pipeline与特征工程

## 一、AI数据Pipeline全景架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         AI数据处理全流程                                  │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │  数据采集层   │───▶│  数据处理层   │───▶│  特征工程层   │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│   • Kafka/Pulsar      • Ray Data          • Feature Store              │
│   • Object Storage    • Spark on K8s      • Feast/Tecton               │
│   • Database CDC      • Flink on K8s      • Feature Transform          │
│   • Log Collection    • Pandas/Dask       • Feature Validation         │
│                                                                           │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │  数据质量层   │───▶│  数据版本层   │───▶│  训练数据层   │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│   • Great Expectations • DVC              • DataLoader优化              │
│   • Data Validation   • Pachyderm         • Data Sharding              │
│   • Schema Evolution  • LakeFS            • Distributed Cache          │
│   • Anomaly Detection • Delta Lake        • Prefetching                │
│                                                                           │
│  ┌──────────────────────────────────────────────────────────┐           │
│  │              Kubernetes调度与存储基础设施                 │           │
│  │  • Volcano调度  • JuiceFS分布式存储  • Alluxio缓存       │           │
│  └──────────────────────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 二、Ray Data分布式数据处理

### 2.1 Ray Data架构

Ray Data是Ray生态中专为ML数据处理设计的库，提供分布式ETL能力。

**核心特性：**
- **流式处理**：支持大规模数据集的流式读取和处理
- **分布式转换**：自动并行化数据转换操作
- **与训练集成**：无缝对接Ray Train和PyTorch/TensorFlow
- **多格式支持**：Parquet、CSV、JSON、Images、自定义格式

### 2.2 Ray Cluster部署

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: ray-code
  namespace: ai-platform
data:
  data_pipeline.py: |
    import ray
    from ray import data
    from ray.data.preprocessors import StandardScaler, OneHotEncoder
    import pandas as pd
    
    # 初始化Ray
    ray.init(address="auto")
    
    # 读取数据（支持S3/GCS/Azure Blob/JuiceFS）
    ds = ray.data.read_parquet("s3://data-lake/raw/dataset.parquet")
    
    # 数据清洗
    def clean_data(batch: pd.DataFrame) -> pd.DataFrame:
        # 去除缺失值
        batch = batch.dropna(subset=['user_id', 'timestamp'])
        # 时间戳转换
        batch['timestamp'] = pd.to_datetime(batch['timestamp'])
        # 异常值过滤
        batch = batch[batch['amount'] > 0]
        return batch
    
    ds = ds.map_batches(clean_data, batch_format="pandas")
    
    # 特征工程
    def feature_engineering(batch: pd.DataFrame) -> pd.DataFrame:
        # 时间特征
        batch['hour'] = batch['timestamp'].dt.hour
        batch['day_of_week'] = batch['timestamp'].dt.dayofweek
        # 统计特征
        batch['amount_log'] = np.log1p(batch['amount'])
        return batch
    
    ds = ds.map_batches(feature_engineering, batch_format="pandas")
    
    # 数据分割
    train_ds, val_ds = ds.train_test_split(test_size=0.2, shuffle=True)
    
    # 写入训练数据
    train_ds.write_parquet("s3://data-lake/processed/train/")
    val_ds.write_parquet("s3://data-lake/processed/val/")
    
    print(f"训练集大小: {train_ds.count()}")
    print(f"验证集大小: {val_ds.count()}")
---
apiVersion: ray.io/v1alpha1
kind: RayCluster
metadata:
  name: ray-data-cluster
  namespace: ai-platform
spec:
  rayVersion: '2.9.0'
  headGroupSpec:
    serviceType: ClusterIP
    replicas: 1
    rayStartParams:
      dashboard-host: '0.0.0.0'
      num-cpus: '0'  # Head不参与计算
    template:
      spec:
        containers:
        - name: ray-head
          image: rayproject/ray:2.9.0-py310
          resources:
            requests:
              cpu: "4"
              memory: "16Gi"
            limits:
              cpu: "8"
              memory: "32Gi"
          ports:
          - containerPort: 6379
            name: gcs-server
          - containerPort: 8265
            name: dashboard
          - containerPort: 10001
            name: client
          volumeMounts:
          - name: ray-code
            mountPath: /home/ray/code
          - name: s3-credentials
            mountPath: /home/ray/.aws
            readOnly: true
        volumes:
        - name: ray-code
          configMap:
            name: ray-code
        - name: s3-credentials
          secret:
            secretName: s3-credentials
  workerGroupSpecs:
  - groupName: data-workers
    replicas: 10
    minReplicas: 5
    maxReplicas: 20
    rayStartParams:
      num-cpus: "16"
      object-store-memory: "34359738368"  # 32GB
    template:
      spec:
        containers:
        - name: ray-worker
          image: rayproject/ray:2.9.0-py310
          resources:
            requests:
              cpu: "16"
              memory: "64Gi"
            limits:
              cpu: "16"
              memory: "64Gi"
          volumeMounts:
          - name: s3-credentials
            mountPath: /home/ray/.aws
            readOnly: true
          - name: juicefs-cache
            mountPath: /mnt/jfs
        volumes:
        - name: s3-credentials
          secret:
            secretName: s3-credentials
        - name: juicefs-cache
          persistentVolumeClaim:
            claimName: juicefs-cache-pvc
---
apiVersion: batch/v1
kind: Job
metadata:
  name: ray-data-processing-job
  namespace: ai-platform
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: ray-submit
        image: rayproject/ray:2.9.0-py310
        command:
        - python
        - /home/ray/code/data_pipeline.py
        env:
        - name: RAY_ADDRESS
          value: "ray://ray-data-cluster-head-svc.ai-platform.svc.cluster.local:10001"
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
        volumeMounts:
        - name: ray-code
          mountPath: /home/ray/code
      volumes:
      - name: ray-code
        configMap:
          name: ray-code
```

### 2.3 高性能数据加载Pipeline

```python
# 推荐的数据处理Pipeline模式
import ray
from ray import data
from typing import Dict, Any

# 1. 数据读取优化
ds = ray.data.read_parquet(
    "s3://data-lake/dataset/",
    parallelism=200,  # 并行度：根据数据大小调整
    ray_remote_args={"num_cpus": 1}
)

# 2. 数据预处理Pipeline
def preprocessing_pipeline(batch: Dict[str, Any]) -> Dict[str, Any]:
    """组合多个预处理步骤减少数据传输"""
    # 文本清洗
    batch['text'] = batch['text'].str.lower().str.strip()
    # Tokenization
    batch['tokens'] = tokenize(batch['text'])
    # 填充缺失值
    batch.fillna({'category': 'unknown'}, inplace=True)
    return batch

ds = ds.map_batches(
    preprocessing_pipeline,
    batch_size=1024,  # 批次大小影响内存和吞吐
    batch_format="pandas",
    num_cpus=1
)

# 3. 特征提取（CPU密集型）
def extract_features(batch: Dict[str, Any]) -> Dict[str, Any]:
    from sklearn.feature_extraction.text import TfidfVectorizer
    vectorizer = TfidfVectorizer(max_features=1000)
    batch['features'] = vectorizer.fit_transform(batch['text']).toarray()
    return batch

ds = ds.map_batches(
    extract_features,
    batch_size=512,
    num_cpus=4,  # CPU密集操作分配更多核心
    compute="tasks"  # 使用Ray Tasks而非Actors
)

# 4. 数据增强（可选）
def augment_data(batch: Dict[str, Any]) -> Dict[str, Any]:
    # 随机替换、插入、删除
    augmented = []
    for text in batch['text']:
        if random.random() < 0.3:  # 30%概率增强
            augmented.append(augment_text(text))
        else:
            augmented.append(text)
    batch['text'] = augmented
    return batch

ds = ds.map_batches(augment_data, batch_size=256)

# 5. 数据Shuffle（训练必需）
ds = ds.random_shuffle(seed=42)

# 6. 数据持久化与迭代
ds = ds.materialize()  # 缓存到对象存储

# 7. 训练集成
train_ds, val_ds = ds.train_test_split(test_size=0.2)

# 直接传递给Ray Train
from ray.train.torch import TorchTrainer
trainer = TorchTrainer(
    train_loop_per_worker=train_func,
    datasets={"train": train_ds, "val": val_ds},
    scaling_config=ScalingConfig(num_workers=8, use_gpu=True)
)
```

---

## 三、Spark on Kubernetes大规模ETL

### 3.1 Spark Operator部署

```bash
# 安装Spark Operator
helm repo add spark-operator https://googlecloudplatform.github.io/spark-on-k8s-operator
helm install spark-operator spark-operator/spark-operator \
  --namespace spark-operator \
  --create-namespace \
  --set webhook.enable=true \
  --set sparkJobNamespace=ai-platform
```

### 3.2 PySpark数据处理Job

```yaml
apiVersion: sparkoperator.k8s.io/v1beta2
kind: SparkApplication
metadata:
  name: data-etl-job
  namespace: ai-platform
spec:
  type: Python
  pythonVersion: "3"
  mode: cluster
  image: "apache/spark-py:v3.5.0"
  imagePullPolicy: Always
  mainApplicationFile: s3a://code-bucket/etl_pipeline.py
  sparkVersion: "3.5.0"
  
  # Spark配置优化
  sparkConf:
    # 内存管理
    "spark.executor.memory": "16g"
    "spark.executor.memoryOverhead": "4g"
    "spark.driver.memory": "8g"
    "spark.driver.memoryOverhead": "2g"
    # 动态分配
    "spark.dynamicAllocation.enabled": "true"
    "spark.dynamicAllocation.minExecutors": "5"
    "spark.dynamicAllocation.maxExecutors": "50"
    "spark.dynamicAllocation.initialExecutors": "10"
    # Shuffle优化
    "spark.sql.shuffle.partitions": "200"
    "spark.shuffle.service.enabled": "true"
    # S3访问
    "spark.hadoop.fs.s3a.impl": "org.apache.hadoop.fs.s3a.S3AFileSystem"
    "spark.hadoop.fs.s3a.aws.credentials.provider": "com.amazonaws.auth.DefaultAWSCredentialsProviderChain"
    "spark.hadoop.fs.s3a.endpoint": "s3.amazonaws.com"
    # 性能优化
    "spark.sql.adaptive.enabled": "true"
    "spark.sql.adaptive.coalescePartitions.enabled": "true"
    "spark.sql.files.maxPartitionBytes": "134217728"  # 128MB
  
  hadoopConf:
    "fs.s3a.access.key": "AKIAIOSFODNN7EXAMPLE"
    "fs.s3a.secret.key": "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
  
  # Driver配置
  driver:
    cores: 4
    coreLimit: "4"
    memory: "8g"
    labels:
      version: "3.5.0"
    serviceAccount: spark-driver
    env:
    - name: AWS_REGION
      value: "us-west-2"
  
  # Executor配置
  executor:
    cores: 4
    instances: 10
    memory: "16g"
    labels:
      version: "3.5.0"
    env:
    - name: AWS_REGION
      value: "us-west-2"
  
  # 依赖管理
  deps:
    packages:
    - "org.apache.hadoop:hadoop-aws:3.3.4"
    - "com.amazonaws:aws-java-sdk-bundle:1.12.262"
  
  restartPolicy:
    type: OnFailure
    onFailureRetries: 3
    onFailureRetryInterval: 10
    onSubmissionFailureRetries: 5
    onSubmissionFailureRetryInterval: 20
---
# etl_pipeline.py 内容示例
"""
from pyspark.sql import SparkSession
from pyspark.sql.functions import col, when, regexp_replace, to_timestamp
from pyspark.sql.types import StructType, StructField, StringType, DoubleType

# 初始化Spark
spark = SparkSession.builder.appName("DataETL").getOrCreate()

# 读取原始数据
df = spark.read.parquet("s3a://data-lake/raw/user_events/")

# 数据清洗
df_clean = df \
    .filter(col("user_id").isNotNull()) \
    .filter(col("event_time").isNotNull()) \
    .withColumn("event_time", to_timestamp(col("event_time"))) \
    .withColumn("amount", when(col("amount") < 0, 0).otherwise(col("amount"))) \
    .dropDuplicates(["user_id", "event_time"])

# 特征工程
from pyspark.ml.feature import VectorAssembler, StandardScaler

# 组装特征向量
assembler = VectorAssembler(
    inputCols=["amount", "duration", "click_count"],
    outputCol="features_raw"
)
df_features = assembler.transform(df_clean)

# 标准化
scaler = StandardScaler(
    inputCol="features_raw",
    outputCol="features",
    withStd=True,
    withMean=True
)
scaler_model = scaler.fit(df_features)
df_scaled = scaler_model.transform(df_features)

# 写入处理后的数据（按日期分区）
df_scaled.write \
    .mode("overwrite") \
    .partitionBy("event_date") \
    .parquet("s3a://data-lake/processed/user_features/")

spark.stop()
"""
```

### 3.3 Spark性能监控

```yaml
apiVersion: v1
kind: Service
metadata:
  name: spark-history-server
  namespace: ai-platform
spec:
  type: ClusterIP
  ports:
  - port: 18080
    targetPort: 18080
    name: http
  selector:
    app: spark-history-server
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: spark-history-server
  namespace: ai-platform
spec:
  replicas: 1
  selector:
    matchLabels:
      app: spark-history-server
  template:
    metadata:
      labels:
        app: spark-history-server
    spec:
      containers:
      - name: spark-history
        image: apache/spark:v3.5.0
        command:
        - "/opt/spark/sbin/start-history-server.sh"
        env:
        - name: SPARK_HISTORY_OPTS
          value: "-Dspark.history.fs.logDirectory=s3a://spark-logs/history -Dspark.history.ui.port=18080"
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
        ports:
        - containerPort: 18080
          name: http
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
```

---

## 四、Feature Store特征工程平台

### 4.1 Feast特征存储架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Feast架构                               │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  离线特征                在线特征                推送引擎     │
│  ┌─────────┐          ┌─────────┐            ┌─────────┐   │
│  │ Parquet │─────────▶│  Redis  │◀───────────│ Stream  │   │
│  │ BigQuery│  物化     │DynamoDB │   实时更新  │ Kafka   │   │
│  │  S3     │          │ Bigtable│            │ Flink   │   │
│  └─────────┘          └─────────┘            └─────────┘   │
│      ▲                     ▲                                │
│      │                     │                                │
│      │                     │                                │
│  ┌───┴──────┐         ┌───┴──────┐                         │
│  │  训练    │         │  推理    │                          │
│  │ 批量获取  │         │ 低延迟   │                          │
│  └──────────┘         └──────────┘                         │
└─────────────────────────────────────────────────────────────┘
```

### 4.2 Feast on Kubernetes部署

```yaml
# Feast Registry（元数据存储）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: feast-registry
  namespace: ai-platform
spec:
  replicas: 2
  selector:
    matchLabels:
      app: feast-registry
  template:
    metadata:
      labels:
        app: feast-registry
    spec:
      containers:
      - name: feast-server
        image: feastdev/feature-server:0.35.0
        ports:
        - containerPort: 6566
          name: grpc
        env:
        - name: FEAST_REGISTRY_PATH
          value: "s3://feast-registry/registry.db"
        - name: FEAST_ONLINE_STORE_TYPE
          value: "redis"
        - name: FEAST_ONLINE_STORE_CONNECTION_STRING
          value: "redis-master.ai-platform.svc.cluster.local:6379"
        - name: FEAST_OFFLINE_STORE_TYPE
          value: "file"
        - name: FEAST_OFFLINE_STORE_PATH
          value: "s3://data-lake/feast-offline/"
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
---
# Redis作为在线特征存储
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-feast
  namespace: ai-platform
spec:
  serviceName: redis-feast
  replicas: 3
  selector:
    matchLabels:
      app: redis-feast
  template:
    metadata:
      labels:
        app: redis-feast
    spec:
      containers:
      - name: redis
        image: redis:7.2-alpine
        command:
        - redis-server
        - --appendonly yes
        - --maxmemory 8gb
        - --maxmemory-policy allkeys-lru
        ports:
        - containerPort: 6379
          name: redis
        volumeMounts:
        - name: data
          mountPath: /data
        resources:
          requests:
            cpu: "2"
            memory: "10Gi"
          limits:
            cpu: "4"
            memory: "12Gi"
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

### 4.3 特征定义与注册

```python
# feature_repo/features.py
from feast import Entity, Feature, FeatureView, FileSource, ValueType
from feast.types import Float32, Int64, String
from datetime import timedelta

# 定义实体
user = Entity(
    name="user_id",
    value_type=ValueType.INT64,
    description="用户唯一标识"
)

# 离线数据源
user_stats_source = FileSource(
    path="s3://data-lake/feast-offline/user_stats.parquet",
    timestamp_field="event_timestamp",
    created_timestamp_column="created_timestamp"
)

# 特征视图
user_stats_fv = FeatureView(
    name="user_statistics",
    entities=[user],
    ttl=timedelta(days=7),  # 在线特征TTL
    schema=[
        Feature(name="total_purchases", dtype=Int64),
        Feature(name="avg_purchase_amount", dtype=Float32),
        Feature(name="last_purchase_days", dtype=Int64),
        Feature(name="user_segment", dtype=String)
    ],
    online=True,
    source=user_stats_source,
    tags={"team": "data-science", "version": "v2"}
)

# 实时特征（Kafka流）
from feast.data_source import KafkaSource

user_activity_source = KafkaSource(
    name="user_activity_stream",
    kafka_bootstrap_servers="kafka.ai-platform.svc.cluster.local:9092",
    topic="user-activity",
    timestamp_field="event_timestamp",
    created_timestamp_column="created_timestamp",
    message_format="json"
)

user_activity_fv = FeatureView(
    name="user_realtime_activity",
    entities=[user],
    ttl=timedelta(hours=1),
    schema=[
        Feature(name="last_action", dtype=String),
        Feature(name="session_duration", dtype=Int64),
        Feature(name="page_views_1h", dtype=Int64)
    ],
    online=True,
    source=user_activity_source
)
```

```bash
# 应用特征定义到Registry
cd feature_repo
feast apply

# 物化离线特征到在线存储
feast materialize-incremental $(date -u +"%Y-%m-%dT%H:%M:%S")
```

### 4.4 特征获取（训练与推理）

```python
from feast import FeatureStore
import pandas as pd
from datetime import datetime

# 初始化Feature Store
store = FeatureStore(repo_path=".")

# ===== 训练场景：批量获取历史特征 =====
entity_df = pd.DataFrame({
    "user_id": [1001, 1002, 1003, 1004],
    "event_timestamp": [
        datetime(2024, 1, 1, 12, 0, 0),
        datetime(2024, 1, 1, 13, 0, 0),
        datetime(2024, 1, 1, 14, 0, 0),
        datetime(2024, 1, 1, 15, 0, 0)
    ]
})

# 获取历史特征（point-in-time正确性）
training_df = store.get_historical_features(
    entity_df=entity_df,
    features=[
        "user_statistics:total_purchases",
        "user_statistics:avg_purchase_amount",
        "user_statistics:user_segment",
        "user_realtime_activity:page_views_1h"
    ]
).to_df()

print(training_df.head())

# ===== 推理场景：在线低延迟获取 =====
from feast import FeatureStore
store = FeatureStore(repo_path=".")

# 单用户特征获取（<10ms延迟）
features = store.get_online_features(
    features=[
        "user_statistics:total_purchases",
        "user_statistics:avg_purchase_amount",
        "user_realtime_activity:last_action"
    ],
    entity_rows=[{"user_id": 1001}]
).to_dict()

print(features)
# {'user_id': [1001], 'total_purchases': [45], 'avg_purchase_amount': [89.5], ...}

# 批量用户特征获取
entity_rows = [
    {"user_id": 1001},
    {"user_id": 1002},
    {"user_id": 1003}
]
batch_features = store.get_online_features(
    features=["user_statistics:total_purchases"],
    entity_rows=entity_rows
).to_dict()
```

### 4.5 实时特征推送Pipeline

```python
# real_time_feature_push.py
from feast import FeatureStore
from kafka import KafkaConsumer
import json
from datetime import datetime

store = FeatureStore(repo_path=".")

# 消费Kafka流并推送特征
consumer = KafkaConsumer(
    'user-activity',
    bootstrap_servers='kafka.ai-platform.svc.cluster.local:9092',
    value_deserializer=lambda m: json.loads(m.decode('utf-8')),
    group_id='feast-feature-pusher'
)

for message in consumer:
    event = message.value
    
    # 计算实时特征
    feature_data = {
        "user_id": event["user_id"],
        "last_action": event["action_type"],
        "session_duration": event["session_duration"],
        "page_views_1h": event["page_count"],
        "event_timestamp": datetime.fromisoformat(event["timestamp"])
    }
    
    # 推送到在线存储
    store.push(
        push_source_name="user_activity_push",
        df=pd.DataFrame([feature_data])
    )
```

部署为Kubernetes Job：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: feast-feature-pusher
  namespace: ai-platform
spec:
  replicas: 3
  selector:
    matchLabels:
      app: feast-pusher
  template:
    metadata:
      labels:
        app: feast-pusher
    spec:
      containers:
      - name: pusher
        image: myregistry/feast-pusher:latest
        env:
        - name: KAFKA_BOOTSTRAP_SERVERS
          value: "kafka.ai-platform.svc.cluster.local:9092"
        - name: FEAST_REPO_PATH
          value: "/feast-repo"
        volumeMounts:
        - name: feast-config
          mountPath: /feast-repo
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
          limits:
            cpu: "2"
            memory: "4Gi"
      volumes:
      - name: feast-config
        configMap:
          name: feast-repo-config
```

---

## 五、数据质量与验证

### 5.1 Great Expectations集成

```python
# data_validation.py
import great_expectations as gx
from great_expectations.data_context import FileDataContext
from great_expectations.checkpoint import Checkpoint

# 初始化GX Context
context = FileDataContext.create(project_root_dir="./gx")

# 连接数据源（S3 Parquet）
datasource = context.sources.add_spark_s3(
    name="s3_datasource",
    bucket="data-lake",
    boto3_options={
        "endpoint_url": "https://s3.amazonaws.com",
        "region_name": "us-west-2"
    }
)

# 添加数据资产
data_asset = datasource.add_parquet_asset(
    name="user_events",
    s3_prefix="processed/user_events/"
)

# 创建批次请求
batch_request = data_asset.build_batch_request()

# 定义Expectation Suite
validator = context.get_validator(
    batch_request=batch_request,
    expectation_suite_name="user_events_suite"
)

# 添加数据质量期望
validator.expect_table_row_count_to_be_between(min_value=1000, max_value=10000000)
validator.expect_column_values_to_not_be_null(column="user_id")
validator.expect_column_values_to_not_be_null(column="event_timestamp")
validator.expect_column_values_to_be_between(
    column="amount",
    min_value=0,
    max_value=10000
)
validator.expect_column_values_to_be_in_set(
    column="event_type",
    value_set=["click", "purchase", "view", "signup"]
)

# 保存Expectation Suite
validator.save_expectation_suite(discard_failed_expectations=False)

# 创建Checkpoint运行验证
checkpoint = Checkpoint(
    name="user_events_checkpoint",
    run_name_template="%Y%m%d-%H%M%S",
    data_context=context,
    batch_request=batch_request,
    expectation_suite_name="user_events_suite",
    action_list=[
        {
            "name": "store_validation_result",
            "action": {"class_name": "StoreValidationResultAction"}
        },
        {
            "name": "update_data_docs",
            "action": {"class_name": "UpdateDataDocsAction"}
        },
        {
            "name": "send_slack_notification",
            "action": {
                "class_name": "SlackNotificationAction",
                "slack_webhook": "https://hooks.slack.com/services/YOUR/WEBHOOK/URL",
                "notify_on": "failure"
            }
        }
    ]
)

# 执行验证
result = checkpoint.run()

if not result["success"]:
    raise ValueError("数据质量验证失败！")
```

### 5.2 自动化数据验证CronJob

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: data-quality-check
  namespace: ai-platform
spec:
  schedule: "0 */6 * * *"  # 每6小时执行一次
  jobTemplate:
    spec:
      template:
        spec:
          restartPolicy: OnFailure
          containers:
          - name: gx-validator
            image: myregistry/great-expectations:latest
            command:
            - python
            - /app/data_validation.py
            env:
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
            - name: SLACK_WEBHOOK_URL
              valueFrom:
                secretKeyRef:
                  name: slack-webhook
                  key: url
            resources:
              requests:
                cpu: "2"
                memory: "4Gi"
              limits:
                cpu: "4"
                memory: "8Gi"
```

---

## 六、数据版本控制（DVC）

### 6.1 DVC配置

```yaml
# .dvc/config
[core]
    remote = s3storage
    autostage = true

['remote "s3storage"']
    url = s3://data-lake/dvc-storage
    region = us-west-2
```

### 6.2 数据Pipeline定义

```yaml
# dvc.yaml
stages:
  data_collection:
    cmd: python scripts/collect_data.py
    deps:
    - scripts/collect_data.py
    outs:
    - data/raw/user_events.csv
    
  data_preprocessing:
    cmd: python scripts/preprocess.py
    deps:
    - scripts/preprocess.py
    - data/raw/user_events.csv
    outs:
    - data/processed/train.parquet
    - data/processed/val.parquet
    metrics:
    - metrics/preprocessing.json:
        cache: false
    
  feature_engineering:
    cmd: python scripts/feature_engineering.py
    deps:
    - scripts/feature_engineering.py
    - data/processed/train.parquet
    outs:
    - data/features/train_features.parquet
    params:
    - feature_config.yaml:
        - n_components
        - scaler_type
    
  model_training:
    cmd: python scripts/train.py
    deps:
    - scripts/train.py
    - data/features/train_features.parquet
    outs:
    - models/model.pkl
    metrics:
    - metrics/train_metrics.json:
        cache: false
    plots:
    - plots/confusion_matrix.png:
        cache: false

# metrics/preprocessing.json
{
  "rows_before": 1000000,
  "rows_after": 950000,
  "null_ratio": 0.02,
  "duplicate_ratio": 0.03
}
```

```bash
# 运行整个Pipeline
dvc repro

# 查看Pipeline可视化
dvc dag

# 查看指标对比
dvc metrics show
dvc metrics diff

# 推送数据到远程
dvc push
```

---

## 七、训练数据优化

### 7.1 PyTorch DataLoader优化

```python
import torch
from torch.utils.data import Dataset, DataLoader, DistributedSampler
from torch.utils.data.dataloader import default_collate
import numpy as np

class OptimizedDataset(Dataset):
    """高性能Dataset实现"""
    def __init__(self, data_path, transform=None):
        # 使用内存映射减少内存占用
        self.data = np.load(data_path, mmap_mode='r')
        self.transform = transform
    
    def __len__(self):
        return len(self.data)
    
    def __getitem__(self, idx):
        # 懒加载数据
        sample = self.data[idx]
        if self.transform:
            sample = self.transform(sample)
        return sample

# 自定义collate_fn加速批次组装
def fast_collate(batch):
    """优化的collate函数"""
    imgs = torch.stack([item[0] for item in batch])
    labels = torch.tensor([item[1] for item in batch])
    return imgs, labels

# DataLoader配置
train_dataset = OptimizedDataset("data/train.npy")

# 分布式训练Sampler
train_sampler = DistributedSampler(
    train_dataset,
    num_replicas=world_size,
    rank=rank,
    shuffle=True,
    seed=42
)

train_loader = DataLoader(
    train_dataset,
    batch_size=256,
    sampler=train_sampler,
    num_workers=8,  # CPU核心数
    pin_memory=True,  # 固定内存加速GPU传输
    prefetch_factor=2,  # 预取批次数
    persistent_workers=True,  # 保持worker进程
    collate_fn=fast_collate
)

# 训练循环
for epoch in range(num_epochs):
    train_sampler.set_epoch(epoch)  # 确保每个epoch不同的shuffle
    for batch_idx, (data, target) in enumerate(train_loader):
        data, target = data.cuda(non_blocking=True), target.cuda(non_blocking=True)
        # 训练步骤...
```

### 7.2 数据缓存层（Alluxio）

```yaml
# Alluxio分布式缓存加速数据访问
apiVersion: v1
kind: ConfigMap
metadata:
  name: alluxio-config
  namespace: ai-platform
data:
  alluxio-site.properties: |
    alluxio.master.hostname=alluxio-master
    alluxio.master.mount.table.root.ufs=s3://data-lake/
    alluxio.underfs.s3.region=us-west-2
    # 缓存配置
    alluxio.user.file.writetype.default=CACHE_THROUGH
    alluxio.user.file.readtype.default=CACHE
    alluxio.worker.memory.size=200GB
    alluxio.worker.tieredstore.levels=2
    alluxio.worker.tieredstore.level0.alias=MEM
    alluxio.worker.tieredstore.level0.dirs.path=/mnt/ramdisk
    alluxio.worker.tieredstore.level0.dirs.quota=200GB
    alluxio.worker.tieredstore.level1.alias=SSD
    alluxio.worker.tieredstore.level1.dirs.path=/mnt/ssd
    alluxio.worker.tieredstore.level1.dirs.quota=1TB
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: alluxio-worker
  namespace: ai-platform
spec:
  serviceName: alluxio-worker
  replicas: 10
  selector:
    matchLabels:
      app: alluxio-worker
  template:
    metadata:
      labels:
        app: alluxio-worker
    spec:
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      containers:
      - name: alluxio-worker
        image: alluxio/alluxio:2.9.3
        command: ["/entrypoint.sh"]
        args: ["worker-only", "--no-format"]
        env:
        - name: ALLUXIO_WORKER_MEMORY_SIZE
          value: "200GB"
        - name: ALLUXIO_RAM_FOLDER
          value: "/mnt/ramdisk"
        volumeMounts:
        - name: alluxio-config
          mountPath: /opt/alluxio/conf
        - name: ramdisk
          mountPath: /mnt/ramdisk
        - name: ssd-cache
          mountPath: /mnt/ssd
        resources:
          requests:
            cpu: "8"
            memory: "220Gi"
          limits:
            cpu: "16"
            memory: "240Gi"
      volumes:
      - name: alluxio-config
        configMap:
          name: alluxio-config
      - name: ramdisk
        emptyDir:
          medium: Memory
          sizeLimit: 200Gi
  volumeClaimTemplates:
  - metadata:
      name: ssd-cache
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 1Ti
```

---

## 八、性能基准测试

| 数据处理引擎 | 数据量 | 处理时间 | 吞吐量 | 适用场景 |
|------------|--------|---------|--------|---------|
| Ray Data | 100GB | 8分钟 | 208MB/s | 中小规模、复杂转换、ML集成 |
| Spark | 1TB | 15分钟 | 1.1GB/s | 大规模批处理、SQL查询 |
| Dask | 50GB | 12分钟 | 69MB/s | Pandas兼容、原型开发 |
| Flink | 实时流 | - | 100万事件/秒 | 实时流处理、低延迟 |

**成本对比（10TB数据处理）：**
- Spark on K8s: $45（10节点 × 1小时 × $4.5/节点）
- Ray Data: $38（8节点 × 1.2小时 × $4/节点）
- 云托管EMR: $120（10节点 + 管理费用）
- **节省65%成本通过自建K8s集群**

---

## 九、最佳实践

### 9.1 数据Pipeline设计原则

**1. 分层架构：**
```
Bronze层（原始数据） → Silver层（清洗数据） → Gold层（特征数据）
```

**2. 幂等性保证：**
- 所有ETL任务支持重复执行
- 使用日期分区避免重复处理
- 写入时使用`overwrite`模式覆盖

**3. 增量处理：**
```python
# 只处理新增数据
last_processed_date = get_last_watermark()
new_data = df.filter(col("event_date") > last_processed_date)
process_and_write(new_data)
update_watermark(current_date)
```

**4. 数据血缘追踪：**
- 每个输出数据集记录来源和转换逻辑
- 使用DVC或Delta Lake记录版本

### 9.2 性能优化Checklist

- [ ] **分区策略**：按日期/用户ID分区，避免小文件
- [ ] **文件格式**：使用Parquet（列式存储）而非CSV
- [ ] **压缩算法**：Snappy（速度）或Zstd（压缩率）
- [ ] **并行度**：Spark partitions = 2-3倍CPU核心数
- [ ] **内存管理**：Executor内存 = 数据大小 / 并行度 × 1.5
- [ ] **缓存层**：Alluxio缓存热数据，减少S3访问
- [ ] **预取**：DataLoader使用`prefetch_factor=2`
- [ ] **异步I/O**：torch.utils.data使用`num_workers > 0`

### 9.3 数据质量监控

```yaml
# Prometheus告警规则
groups:
- name: data_quality
  interval: 5m
  rules:
  - alert: DataPipelineFailure
    expr: rate(spark_job_failures_total[5m]) > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Spark作业失败率 > 0"
      
  - alert: DataFreshnessIssue
    expr: (time() - data_last_update_timestamp) > 7200
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "数据超过2小时未更新"
      
  - alert: DataQualityDegraded
    expr: data_null_ratio > 0.1
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "数据缺失率 > 10%"
```

---

## 十、故障排查

### 10.1 常见问题

**问题1：Spark OOM错误**
```
解决方案：
1. 增加executor内存：spark.executor.memory=16g
2. 增加分区数：spark.sql.shuffle.partitions=400
3. 使用persist()缓存中间结果避免重算
4. 检查数据倾斜：repartition(col("key"))
```

**问题2：Ray Data慢**
```
解决方案：
1. 增加并行度：ds.repartition(200)
2. 调整batch_size：map_batches(func, batch_size=1024)
3. 使用.materialize()持久化中间结果
4. 检查是否使用了慢速UDF，改用向量化操作
```

**问题3：Feast在线特征延迟高**
```
解决方案：
1. Redis添加只读副本分担读负载
2. 启用Redis连接池：connection_pool_size=50
3. 批量获取减少往返：get_online_features(entity_rows=[...])
4. 检查网络延迟：Redis与应用Pod在同一可用区
```

### 10.2 监控关键指标

| 指标 | 阈值 | 说明 |
|-----|------|------|
| Spark作业成功率 | >99% | 低于阈值检查代码或资源 |
| 数据处理延迟 | <30分钟 | 超时影响模型训练 |
| Feast在线获取延迟 | <10ms (p99) | 影响推理性能 |
| 数据缺失率 | <5% | 影响模型质量 |
| S3读取吞吐 | >500MB/s | 瓶颈在存储 |

---

**版本信息：**
- Ray: 2.9.0+
- Spark: 3.5.0+
- Feast: 0.35.0+
- Great Expectations: 0.18.0+
- Kubernetes: v1.27+

**相关表格：**
- [111-AI基础设施架构](./111-ai-infrastructure.md)
- [112-分布式训练框架](./112-distributed-training-frameworks.md)
- [113-AI模型注册中心](./113-model-registry.md)
- [08-存储卷管理](./08-storage-volumes.md)
