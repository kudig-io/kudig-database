# 142 - LLM训练数据Pipeline与管理 (LLM Data Pipeline & Management)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01 | **参考**: [Ray Data](https://docs.ray.io/en/latest/data/data.html)

---

## 一、LLM数据Pipeline架构 (Pipeline Architecture)

### 1.1 端到端数据流水线

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    LLM 训练数据 Pipeline 架构                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     数据采集层 (Data Collection)                       │ │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │ │
│  │  │ Web爬虫 │  │ API抓取 │  │ 数据库  │  │ 文档导入│  │ 用户数据│    │ │
│  │  │ Scrapy  │  │ Requests│  │ Export  │  │ Unstructured │ Feedback│    │ │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘    │ │
│  └───────┴────────────┴────────────┴────────────┴────────────┴──────────┘ │
│                                    │                                        │
│                                    ▼                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     数据清洗层 (Data Cleaning)                         │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │ │
│  │  │ 去重        │  │ 质量过滤    │  │ PII脱敏     │  │ 格式标准化  │  │ │
│  │  │ MinHash/LSH │  │ FastText    │  │ Presidio    │  │ JSON/Parquet│  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
│                                    ▼                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     数据处理层 (Data Processing)                       │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │ │
│  │  │ Tokenization│  │ 数据配比    │  │ 数据增强    │  │ 序列打包    │  │ │
│  │  │ HF/SentenceP│  │ Mix Ratio   │  │ Augmentation│  │ Packing     │  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
│                                    ▼                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     数据存储层 (Data Storage)                          │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │ │
│  │  │ 对象存储    │  │ 分布式缓存  │  │ 向量数据库  │  │ 元数据管理  │  │ │
│  │  │ S3/OSS/GCS  │  │ Alluxio     │  │ Milvus      │  │ MLflow      │  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
│                                    ▼                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     数据加载层 (Data Loading)                          │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │ │
│  │  │ DataLoader  │  │ 预取缓冲    │  │ 分布式采样  │  │ 流式加载    │  │ │
│  │  │ Ray Data    │  │ Prefetch    │  │ DistSampler │  │ Streaming   │  │ │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 数据Pipeline组件对比

| 组件 | 类型 | 处理能力 | K8s集成 | GPU支持 | 适用场景 |
|-----|------|---------|--------|--------|---------|
| **Ray Data** | 分布式处理 | TB级 | Ray Operator | 是 | ML数据预处理 |
| **Spark** | 批处理 | PB级 | Spark Operator | 有限 | 大规模ETL |
| **Dask** | 并行计算 | TB级 | Dask Operator | 是 | Python原生 |
| **Flink** | 流处理 | 无限 | Flink Operator | 否 | 实时数据流 |
| **Prefect** | 编排 | 依赖后端 | K8s Agent | 否 | 工作流编排 |
| **Airflow** | 编排 | 依赖后端 | K8s Executor | 否 | DAG调度 |

---

## 二、数据格式与存储 (Data Formats & Storage)

### 2.1 LLM训练数据格式

| 格式 | 压缩率 | 读取速度 | 列裁剪 | 流式读取 | 适用场景 |
|-----|-------|---------|--------|---------|---------|
| **Parquet** | 高(70%) | 快 | 支持 | 支持 | 结构化数据 |
| **Arrow** | 无 | 极快 | 支持 | 支持 | 内存交换 |
| **WebDataset** | 高 | 极快 | 不支持 | 原生 | 图像/视频 |
| **JSONL** | 低 | 慢 | 不支持 | 原生 | LLM文本 |
| **MDS** | 高 | 极快 | 支持 | 原生 | MosaicML优化 |
| **TFRecord** | 中 | 快 | 不支持 | 支持 | TensorFlow |

### 2.2 数据格式转换

```python
# JSONL转Parquet (优化存储和读取)
import pyarrow as pa
import pyarrow.parquet as pq
import json

def jsonl_to_parquet(input_path, output_path, batch_size=10000):
    """JSONL转Parquet，支持流式处理大文件"""
    schema = None
    writer = None
    batch = []
    
    with open(input_path, 'r') as f:
        for line in f:
            batch.append(json.loads(line))
            
            if len(batch) >= batch_size:
                table = pa.Table.from_pylist(batch)
                
                if writer is None:
                    schema = table.schema
                    writer = pq.ParquetWriter(
                        output_path, 
                        schema,
                        compression='snappy'
                    )
                    
                writer.write_table(table)
                batch = []
                
    # 处理剩余数据
    if batch:
        table = pa.Table.from_pylist(batch)
        writer.write_table(table)
        
    if writer:
        writer.close()
```

```yaml
# WebDataset格式配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: webdataset-config
data:
  create_shards.py: |
    import webdataset as wds
    import json
    
    def create_shards(data_path, output_pattern, max_size=1e9):
        """创建WebDataset分片"""
        with wds.ShardWriter(
            output_pattern,
            maxsize=max_size,
            maxcount=10000
        ) as sink:
            for idx, sample in enumerate(load_samples(data_path)):
                sink.write({
                    "__key__": f"sample_{idx:08d}",
                    "json": json.dumps(sample).encode(),
                })
```

### 2.3 分布式存储配置

```yaml
# JuiceFS for LLM Data
apiVersion: v1
kind: Secret
metadata:
  name: juicefs-secret
  namespace: ml-data
type: Opaque
stringData:
  name: "llm-data"
  metaurl: "redis://:password@redis-master:6379/1"
  storage: "s3"
  bucket: "s3://llm-training-data"
  access-key: "${AWS_ACCESS_KEY_ID}"
  secret-key: "${AWS_SECRET_ACCESS_KEY}"
  
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-llm-data
provisioner: csi.juicefs.com
reclaimPolicy: Retain
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: ml-data
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: ml-data
  
---
# Alluxio数据缓存
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: llm-training-data
  namespace: ml-training
spec:
  mounts:
  - mountPoint: "s3://llm-training-data/processed"
    name: training-data
    path: "/"
    options:
      aws.accessKeyId: "${AWS_ACCESS_KEY_ID}"
      aws.secretKey: "${AWS_SECRET_ACCESS_KEY}"
      aws.region: "us-east-1"
      
  # 数据预热配置
  dataRestoreLocation:
    path: "s3://llm-training-data/cache"
    
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: llm-training-data
  namespace: ml-training
spec:
  replicas: 10
  
  # 分层存储
  tieredstore:
    levels:
    - mediumtype: MEM
      path: /dev/shm
      quota: 64Gi
      high: "0.95"
      low: "0.7"
    - mediumtype: SSD
      path: /mnt/nvme
      quota: 500Gi
      high: "0.95"
      low: "0.7"
      
  master:
    replicas: 3
    jvmOptions:
    - "-Xmx16g"
    - "-XX:+UseG1GC"
    resources:
      requests:
        cpu: "4"
        memory: "16Gi"
        
  worker:
    jvmOptions:
    - "-Xmx32g"
    - "-XX:MaxDirectMemorySize=64g"
    resources:
      requests:
        cpu: "8"
        memory: "64Gi"
      limits:
        cpu: "16"
        memory: "128Gi"
        
  fuse:
    jvmOptions:
    - "-Xmx8g"
    - "-Xms8g"
    args:
    - fuse
    - --fuse-opts=kernel_cache,entry_timeout=36000,attr_timeout=36000,max_readahead=134217728
    resources:
      requests:
        cpu: "2"
        memory: "8Gi"
```

---

## 三、数据清洗与质量 (Data Cleaning & Quality)

### 3.1 数据质量Pipeline

```yaml
# 数据质量检查Job
apiVersion: batch/v1
kind: Job
metadata:
  name: data-quality-check
  namespace: ml-data
spec:
  template:
    spec:
      containers:
      - name: quality-checker
        image: ml-platform/data-quality:latest
        command: ["python", "quality_check.py"]
        args:
        - "--input=s3://raw-data/corpus"
        - "--output=s3://processed-data/corpus"
        - "--config=/config/quality_config.yaml"
        
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
          limits:
            cpu: "32"
            memory: "128Gi"
            
        volumeMounts:
        - name: config
          mountPath: /config
          
      volumes:
      - name: config
        configMap:
          name: quality-config
          
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: quality-config
data:
  quality_config.yaml: |
    # 数据质量检查配置
    
    # 文本长度过滤
    length_filter:
      min_chars: 100
      max_chars: 100000
      min_words: 20
      max_words: 20000
      
    # 语言检测
    language_filter:
      enabled: true
      languages: ["en", "zh"]
      min_confidence: 0.9
      
    # 质量分数
    quality_score:
      enabled: true
      model: "fasttext"
      min_score: 0.7
      
    # 重复检测
    deduplication:
      enabled: true
      method: "minhash"
      threshold: 0.8
      num_perm: 128
      
    # PII检测
    pii_detection:
      enabled: true
      entities: ["PERSON", "EMAIL", "PHONE", "SSN", "CREDIT_CARD"]
      action: "mask"  # mask/remove/flag
      
    # 有害内容过滤
    content_filter:
      enabled: true
      categories: ["hate", "violence", "sexual", "self_harm"]
      threshold: 0.5
```

### 3.2 数据去重实现

```python
# MinHash去重 (Kubernetes Job)
from datasketch import MinHash, MinHashLSH
import ray
from ray import data as ray_data

@ray.remote
class DeduplicationWorker:
    def __init__(self, num_perm=128, threshold=0.8):
        self.num_perm = num_perm
        self.threshold = threshold
        self.lsh = MinHashLSH(threshold=threshold, num_perm=num_perm)
        
    def compute_minhash(self, text):
        """计算文本的MinHash签名"""
        m = MinHash(num_perm=self.num_perm)
        for word in text.split():
            m.update(word.encode('utf-8'))
        return m
        
    def is_duplicate(self, doc_id, text):
        """检查是否为重复文档"""
        minhash = self.compute_minhash(text)
        result = self.lsh.query(minhash)
        
        if not result:
            self.lsh.insert(doc_id, minhash)
            return False
        return True

def deduplicate_dataset(input_path, output_path):
    """分布式去重"""
    # 初始化Ray
    ray.init()
    
    # 创建去重Worker
    workers = [DeduplicationWorker.remote() for _ in range(100)]
    
    # 读取数据
    ds = ray_data.read_parquet(input_path)
    
    # 分布式去重
    def check_duplicate(batch, worker_idx):
        worker = workers[worker_idx % len(workers)]
        results = []
        for row in batch:
            is_dup = ray.get(worker.is_duplicate.remote(
                row['id'], 
                row['text']
            ))
            if not is_dup:
                results.append(row)
        return results
        
    # 执行去重
    deduped_ds = ds.map_batches(
        check_duplicate,
        batch_size=1000,
        num_cpus=1
    )
    
    # 写入结果
    deduped_ds.write_parquet(output_path)
```

### 3.3 PII脱敏配置

```yaml
# Presidio PII脱敏服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: presidio-analyzer
  namespace: ml-data
spec:
  replicas: 5
  selector:
    matchLabels:
      app: presidio-analyzer
  template:
    spec:
      containers:
      - name: analyzer
        image: mcr.microsoft.com/presidio-analyzer:latest
        ports:
        - containerPort: 3000
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
        env:
        - name: ANALYZER_CONF_FILE
          value: "/config/analyzer_config.yaml"
        volumeMounts:
        - name: config
          mountPath: /config
          
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: presidio-anonymizer
  namespace: ml-data
spec:
  replicas: 5
  selector:
    matchLabels:
      app: presidio-anonymizer
  template:
    spec:
      containers:
      - name: anonymizer
        image: mcr.microsoft.com/presidio-anonymizer:latest
        ports:
        - containerPort: 3001
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
            
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: presidio-config
data:
  analyzer_config.yaml: |
    nlp_engine_name: spacy
    models:
    - lang_code: en
      model_name: en_core_web_lg
    - lang_code: zh
      model_name: zh_core_web_lg
      
    supported_entities:
    - PERSON
    - EMAIL_ADDRESS
    - PHONE_NUMBER
    - CREDIT_CARD
    - US_SSN
    - IP_ADDRESS
    - LOCATION
    - DATE_TIME
    - NRP  # 国籍/宗教/政治
    
    recognizers:
    - name: CustomPhoneRecognizer
      supported_entity: PHONE_NUMBER
      patterns:
      - name: phone_pattern
        regex: "\\b\\d{3}[-.]?\\d{4}[-.]?\\d{4}\\b"
        score: 0.9
```

---

## 四、Ray Data分布式处理 (Ray Data Processing)

### 4.1 Ray Cluster部署

```yaml
# Ray Cluster for Data Processing
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: ray-data-cluster
  namespace: ml-data
spec:
  rayVersion: '2.9.0'
  enableInTreeAutoscaling: true
  
  # Autoscaler配置
  autoscalerOptions:
    upscalingMode: Default
    idleTimeoutSeconds: 60
    
  headGroupSpec:
    rayStartParams:
      dashboard-host: '0.0.0.0'
      block: 'true'
      num-cpus: '0'  # Head不运行任务
      
    template:
      spec:
        containers:
        - name: ray-head
          image: rayproject/ray:2.9.0-py310
          ports:
          - containerPort: 6379
            name: gcs
          - containerPort: 8265
            name: dashboard
          - containerPort: 10001
            name: client
          resources:
            limits:
              cpu: "8"
              memory: "32Gi"
            requests:
              cpu: "4"
              memory: "16Gi"
          volumeMounts:
          - name: ray-logs
            mountPath: /tmp/ray
        volumes:
        - name: ray-logs
          emptyDir: {}
          
  workerGroupSpecs:
  - groupName: data-workers
    replicas: 10
    minReplicas: 5
    maxReplicas: 50
    rayStartParams:
      block: 'true'
      
    template:
      spec:
        containers:
        - name: ray-worker
          image: rayproject/ray:2.9.0-py310
          resources:
            limits:
              cpu: "16"
              memory: "64Gi"
            requests:
              cpu: "8"
              memory: "32Gi"
          env:
          - name: RAY_worker_register_timeout_seconds
            value: "120"
          volumeMounts:
          - name: data-cache
            mountPath: /mnt/cache
        volumes:
        - name: data-cache
          emptyDir:
            sizeLimit: "100Gi"
```

### 4.2 Ray Data处理Pipeline

```python
# LLM数据处理Pipeline
import ray
from ray import data as ray_data
from transformers import AutoTokenizer
import pyarrow as pa

# 初始化Ray
ray.init(address="ray://ray-data-cluster-head-svc:10001")

# 加载tokenizer
tokenizer = AutoTokenizer.from_pretrained("meta-llama/Llama-2-7b-hf")

def tokenize_batch(batch):
    """批量tokenize"""
    texts = batch["text"]
    encodings = tokenizer(
        texts,
        truncation=True,
        max_length=4096,
        padding=False,
        return_attention_mask=False
    )
    return {
        "input_ids": encodings["input_ids"],
        "length": [len(ids) for ids in encodings["input_ids"]]
    }

def filter_by_length(batch):
    """按长度过滤"""
    mask = [100 <= length <= 4096 for length in batch["length"]]
    return {
        k: [v for v, m in zip(batch[k], mask) if m]
        for k in batch.keys()
    }

def pack_sequences(batch, max_length=4096):
    """序列打包，提高GPU利用率"""
    packed_input_ids = []
    current_pack = []
    current_length = 0
    
    for input_ids in batch["input_ids"]:
        if current_length + len(input_ids) + 1 <= max_length:
            if current_pack:
                current_pack.append(tokenizer.eos_token_id)
            current_pack.extend(input_ids)
            current_length = len(current_pack)
        else:
            if current_pack:
                # 填充到max_length
                current_pack.extend([tokenizer.pad_token_id] * (max_length - len(current_pack)))
                packed_input_ids.append(current_pack)
            current_pack = input_ids
            current_length = len(input_ids)
            
    if current_pack:
        current_pack.extend([tokenizer.pad_token_id] * (max_length - len(current_pack)))
        packed_input_ids.append(current_pack)
        
    return {"packed_input_ids": packed_input_ids}

# 构建Pipeline
ds = ray_data.read_parquet("s3://llm-data/cleaned/")

processed_ds = (
    ds
    .map_batches(tokenize_batch, batch_size=1000, num_cpus=1)
    .map_batches(filter_by_length, batch_size=1000)
    .map_batches(pack_sequences, batch_size=10000, num_cpus=2)
)

# 写入处理后的数据
processed_ds.write_parquet(
    "s3://llm-data/tokenized/",
    num_rows_per_file=100000
)

# 查看统计信息
print(f"Total samples: {processed_ds.count()}")
print(f"Schema: {processed_ds.schema()}")
```

### 4.3 RayJob提交

```yaml
# RayJob for Data Processing
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: llm-data-processing
  namespace: ml-data
spec:
  entrypoint: python /app/process_data.py --input s3://raw --output s3://processed
  
  shutdownAfterJobFinishes: true
  ttlSecondsAfterFinished: 300
  
  runtimeEnvYAML: |
    pip:
      - transformers==4.36.0
      - datasets==2.16.0
      - pyarrow==14.0.0
      - boto3==1.34.0
    env_vars:
      AWS_ACCESS_KEY_ID: "${AWS_ACCESS_KEY_ID}"
      AWS_SECRET_ACCESS_KEY: "${AWS_SECRET_ACCESS_KEY}"
      HF_TOKEN: "${HF_TOKEN}"
      
  rayClusterSpec:
    rayVersion: '2.9.0'
    headGroupSpec:
      rayStartParams:
        dashboard-host: '0.0.0.0'
      template:
        spec:
          containers:
          - name: ray-head
            image: rayproject/ray:2.9.0-py310
            resources:
              limits:
                cpu: "8"
                memory: "32Gi"
            volumeMounts:
            - name: app
              mountPath: /app
          volumes:
          - name: app
            configMap:
              name: data-processing-scripts
              
    workerGroupSpecs:
    - groupName: workers
      replicas: 20
      minReplicas: 10
      maxReplicas: 100
      rayStartParams: {}
      template:
        spec:
          containers:
          - name: ray-worker
            image: rayproject/ray:2.9.0-py310
            resources:
              limits:
                cpu: "16"
                memory: "64Gi"
```

---

## 五、数据配比与采样 (Data Mixing & Sampling)

### 5.1 数据配比策略

| 数据类型 | 推荐占比 | 说明 | 来源示例 |
|---------|---------|------|---------|
| **通用文本** | 40-50% | 网页、书籍、维基百科 | CommonCrawl, Wikipedia |
| **代码** | 15-20% | 各编程语言代码 | GitHub, StackOverflow |
| **科学论文** | 5-10% | 学术论文、技术文档 | ArXiv, PubMed |
| **对话数据** | 10-15% | 多轮对话、QA | ShareGPT, OASST |
| **指令数据** | 10-15% | 指令-响应对 | Alpaca, Dolly |
| **数学推理** | 5-10% | 数学问题、证明 | GSM8K, MATH |

### 5.2 动态数据混合

```python
# 动态数据混合配置
import ray
from ray import data as ray_data

class DynamicDataMixer:
    def __init__(self, datasets_config):
        """
        datasets_config: {
            "web_text": {"path": "s3://...", "weight": 0.4},
            "code": {"path": "s3://...", "weight": 0.2},
            "papers": {"path": "s3://...", "weight": 0.1},
            "conversations": {"path": "s3://...", "weight": 0.15},
            "instructions": {"path": "s3://...", "weight": 0.15}
        }
        """
        self.config = datasets_config
        self.datasets = {}
        
    def load_datasets(self):
        """加载所有数据集"""
        for name, cfg in self.config.items():
            self.datasets[name] = ray_data.read_parquet(cfg["path"])
            
    def create_mixed_dataset(self, total_samples):
        """按权重混合数据集"""
        mixed_parts = []
        
        for name, cfg in self.config.items():
            ds = self.datasets[name]
            num_samples = int(total_samples * cfg["weight"])
            
            # 采样
            if ds.count() >= num_samples:
                sampled = ds.random_shuffle().limit(num_samples)
            else:
                # 过采样
                repeats = (num_samples // ds.count()) + 1
                sampled = ds.repeat(repeats).limit(num_samples)
                
            mixed_parts.append(sampled)
            
        # 合并并打乱
        mixed_ds = ray_data.from_blocks(
            [ds.get_internal_block_refs() for ds in mixed_parts]
        ).random_shuffle()
        
        return mixed_ds

# 使用示例
config = {
    "web_text": {"path": "s3://data/web_text/", "weight": 0.4},
    "code": {"path": "s3://data/code/", "weight": 0.2},
    "papers": {"path": "s3://data/papers/", "weight": 0.1},
    "conversations": {"path": "s3://data/conversations/", "weight": 0.15},
    "instructions": {"path": "s3://data/instructions/", "weight": 0.15}
}

mixer = DynamicDataMixer(config)
mixer.load_datasets()
mixed_ds = mixer.create_mixed_dataset(total_samples=10_000_000)
mixed_ds.write_parquet("s3://data/mixed_training_data/")
```

---

## 六、数据版本管理 (Data Versioning)

### 6.1 DVC集成

```yaml
# DVC Pipeline配置
apiVersion: batch/v1
kind: Job
metadata:
  name: dvc-data-pipeline
  namespace: ml-data
spec:
  template:
    spec:
      containers:
      - name: dvc
        image: ml-platform/dvc:latest
        command: ["dvc", "repro"]
        env:
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: access-key
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: aws-credentials
              key: secret-key
        volumeMounts:
        - name: repo
          mountPath: /repo
        - name: dvc-config
          mountPath: /repo/dvc.yaml
          subPath: dvc.yaml
          
      volumes:
      - name: repo
        persistentVolumeClaim:
          claimName: data-repo-pvc
      - name: dvc-config
        configMap:
          name: dvc-pipeline-config
          
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: dvc-pipeline-config
data:
  dvc.yaml: |
    stages:
      download:
        cmd: python scripts/download_raw.py
        deps:
          - scripts/download_raw.py
        outs:
          - data/raw
          
      clean:
        cmd: python scripts/clean_data.py
        deps:
          - scripts/clean_data.py
          - data/raw
        params:
          - clean.min_length
          - clean.max_length
          - clean.language
        outs:
          - data/cleaned
          
      deduplicate:
        cmd: python scripts/deduplicate.py
        deps:
          - scripts/deduplicate.py
          - data/cleaned
        params:
          - dedup.threshold
          - dedup.method
        outs:
          - data/deduped
          
      tokenize:
        cmd: python scripts/tokenize.py
        deps:
          - scripts/tokenize.py
          - data/deduped
        params:
          - tokenize.model
          - tokenize.max_length
        outs:
          - data/tokenized
          
      mix:
        cmd: python scripts/mix_datasets.py
        deps:
          - scripts/mix_datasets.py
          - data/tokenized
        params:
          - mix.ratios
        outs:
          - data/final
          
    params:
      - params.yaml
```

### 6.2 数据血缘追踪

```yaml
# MLflow数据追踪
apiVersion: v1
kind: ConfigMap
metadata:
  name: data-lineage-config
data:
  track_data.py: |
    import mlflow
    from datetime import datetime
    
    class DataLineageTracker:
        def __init__(self, experiment_name):
            mlflow.set_experiment(experiment_name)
            
        def log_dataset_version(
            self,
            dataset_name,
            version,
            source_path,
            output_path,
            stats,
            params
        ):
            with mlflow.start_run(run_name=f"{dataset_name}_{version}"):
                # 记录参数
                mlflow.log_params(params)
                
                # 记录数据统计
                mlflow.log_metrics({
                    "num_samples": stats["num_samples"],
                    "total_tokens": stats["total_tokens"],
                    "avg_length": stats["avg_length"],
                    "dedup_rate": stats["dedup_rate"]
                })
                
                # 记录血缘
                mlflow.log_param("source_path", source_path)
                mlflow.log_param("output_path", output_path)
                mlflow.log_param("created_at", datetime.now().isoformat())
                
                # 记录schema
                mlflow.log_dict(stats["schema"], "schema.json")
                
                # 标记版本
                mlflow.set_tag("dataset_version", version)
                mlflow.set_tag("dataset_name", dataset_name)
```

---

## 七、数据加载优化 (Data Loading Optimization)

### 7.1 高性能DataLoader

```python
# 优化的分布式DataLoader
import torch
from torch.utils.data import IterableDataset, DataLoader
import ray
from ray import data as ray_data

class StreamingLLMDataset(IterableDataset):
    """流式LLM数据集，支持分布式训练"""
    
    def __init__(
        self,
        data_path,
        tokenizer,
        max_length=4096,
        world_size=1,
        rank=0,
        buffer_size=10000,
        shuffle_buffer=5000
    ):
        self.data_path = data_path
        self.tokenizer = tokenizer
        self.max_length = max_length
        self.world_size = world_size
        self.rank = rank
        self.buffer_size = buffer_size
        self.shuffle_buffer = shuffle_buffer
        
    def __iter__(self):
        # 使用Ray Data进行流式读取
        ds = ray_data.read_parquet(
            self.data_path,
            parallelism=200
        )
        
        # 分片到当前worker
        ds = ds.split(self.world_size)[self.rank]
        
        # 流式迭代
        for batch in ds.iter_batches(batch_size=self.buffer_size):
            # 本地shuffle
            indices = torch.randperm(len(batch["input_ids"]))
            
            for idx in indices[:self.shuffle_buffer]:
                input_ids = batch["input_ids"][idx]
                
                # 填充/截断
                if len(input_ids) < self.max_length:
                    input_ids = input_ids + [self.tokenizer.pad_token_id] * (
                        self.max_length - len(input_ids)
                    )
                else:
                    input_ids = input_ids[:self.max_length]
                    
                yield {
                    "input_ids": torch.tensor(input_ids),
                    "labels": torch.tensor(input_ids)
                }

def create_distributed_dataloader(
    data_path,
    tokenizer,
    batch_size,
    world_size,
    rank,
    num_workers=4
):
    """创建分布式DataLoader"""
    dataset = StreamingLLMDataset(
        data_path=data_path,
        tokenizer=tokenizer,
        world_size=world_size,
        rank=rank
    )
    
    return DataLoader(
        dataset,
        batch_size=batch_size,
        num_workers=num_workers,
        pin_memory=True,
        prefetch_factor=2,
        persistent_workers=True
    )
```

### 7.2 数据预取配置

```yaml
# Kubernetes Job with Optimized Data Loading
apiVersion: batch/v1
kind: Job
metadata:
  name: llm-training-optimized
  namespace: ml-training
spec:
  template:
    spec:
      containers:
      - name: trainer
        image: nvcr.io/nvidia/pytorch:24.01-py3
        resources:
          limits:
            nvidia.com/gpu: 8
            
        env:
        # 数据加载优化
        - name: DATALOADER_NUM_WORKERS
          value: "8"
        - name: DATALOADER_PIN_MEMORY
          value: "true"
        - name: DATALOADER_PREFETCH_FACTOR
          value: "4"
        
        # 内存映射优化
        - name: PYTORCH_CUDA_ALLOC_CONF
          value: "max_split_size_mb:512"
          
        # NCCL优化
        - name: NCCL_IB_DISABLE
          value: "0"
        - name: NCCL_NET_GDR_LEVEL
          value: "5"
          
        volumeMounts:
        - name: data-cache
          mountPath: /mnt/cache
        - name: shm
          mountPath: /dev/shm
          
      volumes:
      # 本地数据缓存
      - name: data-cache
        hostPath:
          path: /mnt/nvme/cache
          type: DirectoryOrCreate
      # 共享内存
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "128Gi"
```

---

## 八、监控与告警 (Monitoring & Alerting)

### 8.1 数据Pipeline监控指标

| 指标类别 | 指标名称 | 说明 | 告警阈值 |
|---------|---------|------|---------|
| **处理进度** | samples_processed | 已处理样本数 | 停滞>1h |
| **处理进度** | processing_rate | 每秒处理样本 | < 1000/s |
| **数据质量** | duplicate_rate | 重复率 | > 10% |
| **数据质量** | pii_detection_rate | PII检出率 | > 1% |
| **数据质量** | filter_drop_rate | 过滤丢弃率 | > 50% |
| **资源使用** | worker_cpu_util | Worker CPU使用率 | > 90% |
| **资源使用** | memory_usage | 内存使用 | > 85% |
| **存储状态** | storage_write_rate | 存储写入速率 | < 100MB/s |
| **错误率** | processing_errors | 处理错误数 | > 0 |

### 8.2 Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: data-pipeline-alerts
  namespace: monitoring
spec:
  groups:
  - name: data-pipeline
    rules:
    # 处理停滞
    - alert: DataProcessingStalled
      expr: |
        rate(data_samples_processed_total[10m]) == 0
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "数据处理停滞"
        description: "Pipeline {{ $labels.pipeline }} 30分钟内无进展"
        
    # 高错误率
    - alert: DataProcessingHighErrorRate
      expr: |
        rate(data_processing_errors_total[5m]) / rate(data_samples_processed_total[5m]) > 0.01
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "数据处理错误率高"
        description: "错误率: {{ $value | humanizePercentage }}"
        
    # 质量下降
    - alert: DataQualityDegraded
      expr: |
        data_quality_score < 0.8
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "数据质量下降"
        description: "质量分数: {{ $value }}"
        
    # 存储空间不足
    - alert: DataStorageNearFull
      expr: |
        data_storage_used_bytes / data_storage_total_bytes > 0.85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "数据存储空间即将满"
```

---

## 九、快速参考 (Quick Reference)

### 9.1 数据规模估算

| 模型规模 | 推荐训练数据 | Tokens数量 | 存储空间 | 处理时间 |
|---------|------------|-----------|---------|---------|
| 1B | 20-50GB | 20B+ | ~100GB | 1-2天 |
| 7B | 200-500GB | 200B+ | ~1TB | 3-5天 |
| 13B | 500GB-1TB | 500B+ | ~2TB | 1周 |
| 70B | 1-2TB | 1T+ | ~5TB | 2-3周 |

### 9.2 常用命令

```bash
# Ray Data状态
ray status

# 查看处理进度
kubectl logs -f job/data-processing -n ml-data

# 数据统计
python -c "import ray; ray.data.read_parquet('s3://...').count()"

# 存储使用
aws s3 ls --summarize --human-readable s3://llm-data/

# DVC状态
dvc status
dvc dag
```

---

**数据Pipeline原则**: 质量优先 → 去重彻底 → 格式统一 → 版本追踪 → 缓存加速

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
