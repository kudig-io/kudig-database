---
title: "数据目录与血缘（DataHub/Marquez/OpenLineage）"
description: "覆盖 DataHub、Apache Atlas、Marquez 数据目录与血缘系统在 K8s 上的部署与集成"
summary: "数据目录价值（发现/治理/合规），DataHub Helm 部署（依赖 Elasticsearch/MySQL/Kafka），Apache Atlas 对比，Marquez 数据血缘，OpenLineage 标准，与 K8s 数据管线集成，故障排查"
category: 数据库中间件
tags:
- database
- data-catalog
- datahub
- data-lineage
- openlineage
- governance
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 20min
intent_queries:
- "DataHub 如何在 K8s 上部署"
- "数据血缘如何追踪"
- "数据目录选型对比"
trigger_keywords:
- 数据目录
- DataHub
- 数据血缘
- OpenLineage
- Marquez
- 数据治理
prerequisites:
- kubectl-basics
- database-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 数据目录与血缘（DataHub/Marquez/OpenLineage）

## 概述

数据目录（Data Catalog）是数据治理的核心基础设施，为组织提供数据资产的发现、理解、追踪和治理能力。在 Kubernetes 云原生数据平台中，数据管线（ETL/ELT）日益复杂，数据血缘（Data Lineage）追踪变得不可或缺——它回答"这个指标从哪来"、"修改这张表会影响谁"等关键问题。

本文覆盖 DataHub、Apache Atlas 和 Marquez 三款主流数据目录/血缘系统在 K8s 上的部署与集成实践，帮助数据平台团队构建完整的数据治理体系。数据目录通常与 [[07-数据库中间件/06-数据流/index.md|06-数据流]] 中的管线编排配合使用。

## 架构与核心概念

### 数据目录价值

| 能力 | 说明 | 业务价值 |
|------|------|---------|
| **数据发现** | 搜索和浏览数据资产（表、列、Dashboard） | 减少"找数据"时间 80% |
| **数据血缘** | 追踪数据从源到目标的流转路径 | 影响分析、根因定位 |
| **数据治理** | 分类、标签、Owner、质量规则 | 合规审计、责任明确 |
| **元数据管理** | 技术元数据 + 业务元数据统一管理 | 消除数据孤岛 |
| **变更通知** | Schema 变更自动通知下游消费者 | 减少数据事故 |

### DataHub 架构

DataHub 由 LinkedIn 开源，采用微服务架构：

- **GMS（Generalized Metadata Service）**：核心元数据 API 服务
- **Frontend**：React Web UI
- **MAE/MCE Consumer**：元数据事件消费者（Kafka）
- **依赖组件**：
  - Elasticsearch / OpenSearch（搜索索引）
  - MySQL / PostgreSQL（主存储）
  - Kafka（事件总线）
  - Neo4j（可选，图查询血缘）

### OpenLineage 标准

OpenLineage 是 Linux Foundation 下的开放数据血缘标准：

- **Run Event**：描述一次数据处理运行（开始/完成/失败）
- **Dataset**：输入/输出数据集（含 Schema）
- **Job**：产生血缘的处理任务
- **集成**：Airflow、Spark、Flink、dbt 等原生支持

### 工具对比

| 特性 | DataHub | Apache Atlas | Marquez |
|------|---------|-------------|---------|
| 定位 | 全功能数据目录 | Hadoop 生态治理 | 轻量血缘服务 |
| 血缘粒度 | 表级 + 列级 | 表级 + 列级 | 表级 + 列级 |
| 搜索能力 | 强（ES 全文） | 中（Solr） | 无 |
| UI | 现代化 React | 较旧 | 简洁 |
| 部署复杂度 | 高（5+ 组件） | 高（HBase/Solr/Kafka） | 低（API + DB） |
| 扩展性 | 插件化 Ingestion | Hook 机制 | REST API |
| 社区活跃度 | 极高 | 中 | 中 |
| 适用场景 | 企业级数据平台 | Hadoop/大数据生态 | 轻量血缘追踪 |

## 生产部署

### DataHub 部署（Helm）

```yaml
# 🟡 中风险：DataHub Helm values（生产配置）
# datahub-values.yaml
elasticsearch:
  replicas: 3
  minimumMasterNodes: 2
  resources:
    requests:
      cpu: "2"
      memory: 4Gi
    limits:
      cpu: "4"
      memory: 8Gi
  persistence:
    enabled: true
    size: 50Gi
    storageClass: gp3-encrypted

mysql:
  enabled: true
  auth:
    database: datahub
  primary:
    persistence:
      enabled: true
      size: 20Gi
      storageClass: gp3-encrypted
    resources:
      requests:
        cpu: "1"
        memory: 2Gi

kafka:
  enabled: true
  replicas: 3
  persistence:
    enabled: true
    size: 20Gi
    storageClass: gp3-encrypted
  resources:
    requests:
      cpu: "1"
      memory: 2Gi

datahub-gms:
  replicas: 2
  resources:
    requests:
      cpu: "1"
      memory: 2Gi
    limits:
      cpu: "2"
      memory: 4Gi
  podAnnotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "4318"

datahub-frontend:
  replicas: 2
  resources:
    requests:
      cpu: "500m"
      memory: 1Gi
    limits:
      cpu: "1"
      memory: 2Gi

datahub-mae-consumer:
  replicas: 2
  resources:
    requests:
      cpu: "1"
      memory: 2Gi

datahub-mce-consumer:
  replicas: 2
  resources:
    requests:
      cpu: "1"
      memory: 2Gi

datahubActions:
  enabled: true
  replicas: 1
```

```bash
# 🟡 中风险：安装 DataHub
helm repo add datahub https://helm.datahubproject.io/
helm repo update
kubectl create namespace data-catalog
helm install datahub datahub/datahub \
  -n data-catalog \
  -f datahub-values.yaml \
  --timeout 15m \
  --wait

# 🟢 低风险：验证部署状态
kubectl get pods -n data-catalog
kubectl port-forward -n data-catalog svc/datahub-frontend 9002:9002
```

### Marquez 部署（轻量血缘）

```yaml
# 🟡 中风险：Marquez 部署（API + PostgreSQL）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: marquez-api
  namespace: data-catalog
spec:
  replicas: 2
  selector:
    matchLabels:
      app: marquez-api
  template:
    metadata:
      labels:
        app: marquez-api
    spec:
      containers:
      - name: marquez
        image: marquezproject/marquez:0.50.0
        ports:
        - containerPort: 5000
          name: http
        - containerPort: 5001
          name: admin
        env:
        - name: POSTGRES_HOST
          value: "marquez-db.data-catalog.svc"
        - name: POSTGRES_PORT
          value: "5432"
        - name: POSTGRES_DB
          value: "marquez"
        - name: POSTGRES_USER
          valueFrom:
            secretKeyRef:
              name: marquez-db-creds
              key: username
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: marquez-db-creds
              key: password
        resources:
          requests:
            cpu: "500m"
            memory: 1Gi
          limits:
            cpu: "1"
            memory: 2Gi
        livenessProbe:
          httpGet:
            path: /api/v1/namespaces
            port: 5000
          initialDelaySeconds: 15
          periodSeconds: 20
        readinessProbe:
          httpGet:
            path: /api/v1/namespaces
            port: 5000
          initialDelaySeconds: 5
          periodSeconds: 10
---
apiVersion: v1
kind: Service
metadata:
  name: marquez-api
  namespace: data-catalog
spec:
  selector:
    app: marquez-api
  ports:
  - port: 5000
    targetPort: 5000
  type: ClusterIP
```

### 与 K8s 数据管线集成

```yaml
# 🟡 中风险：Airflow DAG 集成 OpenLineage（自动上报血缘到 DataHub）
apiVersion: v1
kind: ConfigMap
metadata:
  name: airflow-openlineage-config
  namespace: data-pipeline
data:
  openlineage.env: |
    # OpenLineage 配置
    OPENLINEAGE_URL=http://datahub-gms.data-catalog.svc:8080/openapi/openlineage/
    OPENLINEAGE_NAMESPACE=kubernetes-prod
    # Airflow OpenLineage 插件
    AIRFLOW__LINEAGE__BACKEND=openlineage.lineage_backend.OpenLineageBackend
    # DataHub 特定配置
    DATAHUB_GMS_URL=http://datahub-gms.data-catalog.svc:8080
    DATAHUB_TOKEN=${DATAHUB_TOKEN}
```

```python
# 🟡 中风险：Spark 作业集成 OpenLineage（Python 示例）
# 在 Spark 作业中自动上报血缘
from openlineage.spark import OpenLineageSparkListener

spark = SparkSession.builder \
    .appName("etl-user-events") \
    .config("spark.extraListeners", "io.openlineage.spark.agent.OpenLineageSparkListener") \
    .config("spark.openlineage.url", "http://marquez-api.data-catalog.svc:5000/api/v1") \
    .config("spark.openlineage.namespace", "kubernetes-prod") \
    .config("spark.openlineage.jobName", "etl-user-events") \
    .getOrCreate()

# 血缘自动追踪：输入表 → 转换 → 输出表
df = spark.read.parquet("s3://data-lake/raw/events/")
result = df.filter(df.event_type == "purchase").groupBy("user_id").count()
result.write.mode("overwrite").parquet("s3://data-lake/analytics/purchase_counts/")
```

## 运维操作

### DataHub 元数据摄入

```bash
# 🟡 中风险：使用 DataHub CLI 摄入 PostgreSQL 元数据
kubectl exec -n data-catalog deploy/datahub-actions -- \
  datahub ingest -c - <<EOF
source:
  type: postgres
  config:
    host_port: postgres-primary.database.svc:5432
    database: myapp
    username: datahub_reader
    password: "\${PG_PASSWORD}"
    include_tables: true
    include_views: true
    profiling:
      enabled: true
sink:
  type: datahub-rest
  config:
    server: http://datahub-gms.data-catalog.svc:8080
EOF

# 🟢 低风险：查看摄入状态
kubectl exec -n data-catalog deploy/datahub-actions -- \
  datahub ingest list-runs
```

### 血缘查询

```bash
# 🟢 低风险：通过 DataHub GraphQL API 查询血缘
curl -X POST "http://datahub-gms.data-catalog.svc:8080/api/graphql" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "query { dataset(urn: \"urn:li:dataset:(urn:li:dataPlatform:postgres,myapp.public.user_events,PROD)\") { lineage { upstream { dataset { name } } downstream { dataset { name } } } } }"
  }'

# 🟢 低风险：Marquez 血缘查询
curl "http://marquez-api.data-catalog.svc:5000/api/v1/namespaces/kubernetes-prod/jobs/etl-user-events/runs?limit=10"
```

## 故障排查

### DataHub GMS 不可用

```bash
# 🟢 低风险：检查 GMS 健康状态
kubectl exec -n data-catalog deploy/datahub-gms -- \
  curl -s http://localhost:8080/health

# 🟢 低风险：检查依赖组件连通性
kubectl exec -n data-catalog deploy/datahub-gms -- \
  curl -s http://elasticsearch-master.data-catalog.svc:9200/_cluster/health

# 🟢 低风险：查看 GMS 日志
kubectl logs -n data-catalog deploy/datahub-gms --tail=100 | grep -i "error\|exception\|timeout"

# 🟢 低风险：检查 Kafka 消费者 lag
kubectl exec -n data-catalog kafka-0 -- \
  kafka-consumer-groups.sh --bootstrap-server localhost:9092 \
  --describe --group datahub-mae-consumer
```

### 血缘数据缺失

**现象**：某些作业的血缘关系未出现在 DataHub 中。

排查步骤：
1. 确认 OpenLineage 插件版本与 Airflow/Spark 版本兼容
2. 检查 `OPENLINEAGE_URL` 是否可达
3. 查看 Airflow/Spark 日志中是否有 OpenLineage 上报错误
4. 确认 DataHub MCE Consumer 正常运行（Kafka 消费无积压）

```bash
# 🟢 低风险：检查 Airflow 日志中的 OpenLineage 事件
kubectl logs -n data-pipeline airflow-worker-0 --tail=200 | grep -i "openlineage\|lineage"

# 🟢 低风险：检查 DataHub MCE Consumer 消费状态
kubectl logs -n data-catalog deploy/datahub-mce-consumer --tail=50 | grep -i "error\|commit\|lag"
```

### Elasticsearch 索引异常

```bash
# 🟢 低风险：检查 ES 索引状态
curl -s "http://elasticsearch-master.data-catalog.svc:9200/_cat/indices?v&h=index,health,status,docs.count,store.size"

# 🟡 中风险：重建 DataHub 搜索索引（数据量大时耗时较长）
kubectl exec -n data-catalog deploy/datahub-gms -- \
  curl -X POST "http://localhost:8080/operations?action=restoreIndices" \
  -H "Content-Type: application/json" \
  -d '{"aspect": "datasetProperties", "urnLike": "urn:li:dataset:%"}'
```

## 最佳实践

1. **元数据摄入自动化**：通过 CronJob 定期同步数据库 Schema 变更到 DataHub，避免手动维护
2. **血缘粒度**：至少实现表级血缘，核心指标实现列级血缘
3. **OpenLineage 标准化**：所有数据管线统一使用 OpenLineage 协议上报，避免厂商锁定
4. **权限控制**：DataHub 集成 LDAP/OIDC，按团队划分数据资产 Owner
5. **变更通知**：配置 DataHub Actions，Schema 变更自动通知 Slack/钉钉
6. **资源规划**：Elasticsearch 是 DataHub 的性能瓶颈，预留足够内存（heap = 50% 容器内存）
7. **与数据管线集成**：在 [[07-数据库中间件/06-数据流/index.md|06-数据流]] 的 Flink/Airflow 管线中嵌入 OpenLineage SDK
8. **监控告警**：监控 GMS 延迟、Kafka 消费 lag、ES 索引大小，接入 [[09-可观测性/index.md|09-可观测性]] 平台
9. **数据库元数据同步**：定期从 [[07-数据库中间件/01-数据库/index.md|01-数据库]] 中的 PostgreSQL/MySQL 同步 Schema 元数据

## Related

- [[07-数据库中间件/06-数据流/index.md|06-数据流]]
- [[07-数据库中间件/01-数据库/index.md|01-数据库]]
- [[09-可观测性/index.md|09-可观测性]]
- [[07-数据库中间件/05-Operator管理/index.md|05-Operator管理]]
- [[15-AI基础设施/index.md|15-AI基础设施]]
