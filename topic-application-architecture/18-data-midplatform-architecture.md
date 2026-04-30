# 数据中台 Kubernetes 生产架构设计

> **适用场景**: 企业数据中台 / 数据湖 / 实时数仓 / 数据资产平台 / 数据治理 / BI 分析  
> **云厂商**: 阿里云 ACK + 大数据产品体系  
> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **目标读者**: 数据架构师、数据平台负责人、阿里云解决方案架构师

---

## 📋 目录

- [一、整体架构全景](#一整体架构全景)
- [二、数据采集与接入架构](#二数据采集与接入架构)
- [三、数据存储与计算架构](#三数据存储与计算架构)
- [四、数据治理与质量管理架构](#四数据治理与质量管理架构)
- [五、数据服务化 (Data API) 架构](#五数据服务化-data-api-架构)
- [六、实时数仓与流批一体架构](#六实时数仓与流批一体架构)
- [七、数据安全与隐私计算架构](#七数据安全与隐私计算架构)
- [八、ACK 阿里云部署架构](#八ack-阿里云部署架构)

---

## 一、整体架构全景

```mermaid
flowchart TB
    subgraph Sources["数据源"]
        DB_SRC["业务数据库<br">MySQL/Oracle"]
        LOG_SRC["日志<br">App/服务器"]
        IOT_SRC["IoT 数据<br">传感器/设备"]
        EXTERNAL["外部数据<br">第三方/API"]
    end

    subgraph Ingestion["数据接入"]
        BATCH["批量采集<br">DataX/OGG"]
        REALTIME["实时采集<br">Flink/Logstash"]
        MESSAGE["消息接入<br">Kafka/RocketMQ"]
    end

    subgraph Storage["数据存储"]
        LAKE["数据湖<br">OSS/HDFS"]
        WAREHOUSE["数据仓库<br">MaxCompute/Hologres"]
        OLAP["OLAP<br">Hologres/ClickHouse"]
        CACHE_DATA["缓存<br">Redis"]
    end

    subgraph Compute["数据计算"]
        OFFLINE_COMP["离线计算<br">Spark/MaxCompute"]
        STREAM_COMP["实时计算<br">Flink"]
        ML_COMP["机器学习<br">PAI"]
    end

    subgraph Service["数据服务"]
        API_DATA["Data API<br">OneService"]
        BI["BI 报表<br">QuickBI"]
        LABEL_SYS["标签体系<br">用户/商品"]
        REC_DATA["推荐/搜索<br">算法服务"]
    end

    Sources --> Ingestion --> Storage --> Compute --> Service

    style Storage fill:#e3f2fd
    style Compute fill:#fff8e1
    style Service fill:#e8f5e9
```

### 阿里云产品映射

| 架构层 | 阿里云方案 | 说明 |
|:---|:---|:---|
| 容器平台 | **ACK Pro** | 计算任务弹性调度 |
| 数据集成 | **DataWorks 数据集成** | 离线与实时同步 |
| 数据仓库 | **MaxCompute** + **Hologres** | 离线+实时一体化 |
| 数据湖 | **OSS** + **DLF** | 数据湖构建与管理 |
| 实时计算 | **实时计算 Flink 版** | 流处理 |
| 机器学习 | **PAI** | 模型训练与推理 |
| 数据治理 | **DataWorks 数据治理** | 元数据/质量/资产 |
| BI | **QuickBI** | 可视化分析 |
| 调度 | **DataWorks 调度** | 工作流编排 |

---

## 二、数据采集与接入架构

```mermaid
flowchart TB
    subgraph Batch["批量采集"]
        DATA_X["DataX<br">异构同步"]
        OGG["OGG / Canal<br">CDC 增量"]
        SQOOP["Sqoop<br">Hadoop 导入"]
    end

    subgraph RealTime["实时采集"]
        FLINK_CDC["Flink CDC<br">数据库日志"]
        LOG_AGENT["Logstash / Fluentd<br">日志采集"]
        BEATS["Beats<br">轻量采集"]
    end

    subgraph MessageQueue["消息缓冲"]
        KAFKA_DATA["Kafka<br">高吞吐日志"]
        ROCKETMQ_DATA["RocketMQ<br">交易数据"]
    end

    Batch & RealTime --> MessageQueue

    style Batch fill:#e3f2fd
    style RealTime fill:#e8f5e9
    style MessageQueue fill:#fff8e1
```

---

## 三、数据存储与计算架构

```mermaid
flowchart TB
    subgraph Lakehouse["湖仓一体"]
        RAW["原始数据层 ODS<br">数据湖"]
        CLEANED["明细数据层 DWD<br">清洗后"]
        SUMMARY["汇总数据层 DWS<br">主题宽表"]
        APPLICATION["应用数据层 ADS<br">面向应用"]
    end

    subgraph ComputeEngine["计算引擎"]
        SPARK["Spark<br">离线批处理"]
        FLINK_ENGINE["Flink<br">实时流处理"]
        PRESTO["Presto/Trino<br">交互式查询"]
    end

    subgraph Serving["数据服务"]
        HOLOGRES["Hologres<br">实时数仓"]
        MYSQL_ADS["MySQL<br">小结果集"]
        REDIS_DATA["Redis<br">热点缓存"]
    end

    Lakehouse --> ComputeEngine --> Serving

    style Lakehouse fill:#e3f2fd
    style ComputeEngine fill:#fff8e1
    style Serving fill:#e8f5e9
```

---

## 四、数据治理与质量管理架构

```mermaid
flowchart TB
    subgraph Governance["数据治理"]
        META["元数据管理<br">数据地图/血缘"]
        QUALITY["数据质量<br">规则/监控/告警"]
        STANDARD["数据标准<br">字典/规范"]
        SECURITY["数据安全<br">分级/脱敏/权限"]
    end

    subgraph Lifecycle["数据生命周期"]
        CREATE["数据产生"]
        STORE["存储管理<br">冷热分层"]
        USE["使用消费<br">API/报表"]
        ARCHIVE["归档销毁<br">合规留存"]
    end

    Governance --> Lifecycle

    style Governance fill:#e3f2fd
    style Lifecycle fill:#e8f5e9
```

---

## 五、数据服务化 (Data API) 架构

```mermaid
flowchart TB
    subgraph DataAssets["数据资产"]
        TABLE["数据表<br">物理表/视图"]
        API_DEF["API 定义<br">SQL/配置"]
        DATASET["数据集<br">多维分析"]
    end

    subgraph GatewayData["API 网关"]
        AUTH_DATA["认证鉴权<br">AppKey/签名"]
        RATE_DATA["限流<br">QPS/并发"]
        ROUTE_DATA["路由<br">版本/灰度"]
    end

    subgraph ConsumersData["消费者"]
        APP_DATA["业务应用<br">实时查询"]
        DASHBOARD_DATA["数据大屏<br">可视化"]
        ALGO_DATA["算法模型<br">特征获取"]
    end

    DataAssets --> GatewayData --> ConsumersData

    style GatewayData fill:#e3f2fd
    style ConsumersData fill:#e8f5e9
```

---

## 六、实时数仓与流批一体架构

```mermaid
flowchart TB
    subgraph SourcesStream["数据源"]
        DB_CDC["MySQL CDC"]
        LOG_STREAM["日志流"]
        EVENT_STREAM["业务事件"]
    end

    subgraph StreamProcess["流处理"]
        FLINK_ETL["Flink ETL<br">清洗/转换"]
        FLINK_AGG["Flink 聚合<br">窗口计算"]
        FLINK_JOIN["Flink Join<br">流关联"]
    end

    subgraph UnifiedStorage["统一存储"]
        HOLOGRES_RT["Hologres<br">实时写入"]
        MAXC_MP["MaxCompute<br">批量写入"]
    end

    subgraph Query["统一查询"]
        ADHOC["即席查询<br">SQL"]
        BI_RT["实时 BI<br">Dashboard"]
        API_RT["Data API<br">应用查询"]
    end

    SourcesStream --> StreamProcess --> UnifiedStorage --> Query

    style StreamProcess fill:#e3f2fd
    style UnifiedStorage fill:#fff8e1
    style Query fill:#e8f5e9
```

---

## 七、数据安全与隐私计算架构

```mermaid
flowchart TB
    subgraph SecurityLayers["安全层级"]
        ACCESS["访问控制<br">RBAC/ABAC"]
        MASKING["数据脱敏<br">静态/动态"]
        ENCRYPT_DATA["加密<br">传输/存储"]
        AUDIT_DATA["审计<br">操作日志"]
    end

    subgraph PrivacyCompute["隐私计算"]
        MPC["安全多方计算<br">MPC"]
        FL["联邦学习<br">联合建模"]
        TEE["可信执行环境<br">Intel SGX"]
        DP["差分隐私<br">噪声注入"]
    end

    subgraph ComplianceData["合规"]
        GRADE["数据分级<br">公开/内部/敏感"]
        PII["PII 识别<br">敏感字段"]
        RETENTION["留存策略<br">过期删除"]
    end

    SecurityLayers --> PrivacyCompute --> ComplianceData

    style SecurityLayers fill:#e3f2fd
    style PrivacyCompute fill:#fff8e1
    style ComplianceData fill:#e8f5e9
```

---

## 八、ACK 阿里云部署架构

### Flink on ACK 部署

```yaml
apiVersion: flink.apache.org/v1beta1
kind: FlinkDeployment
metadata:
  name: realtime-etl-job
  namespace: data-platform
spec:
  image: registry.cn-hangzhou.aliyuncs.com/flink/flink:1.18-scala_2.12
  flinkVersion: v1.18
  jobManager:
    resource:
      memory: "4Gi"
      cpu: 2
  taskManager:
    resource:
      memory: "8Gi"
      cpu: 4
    replicas: 5
  podTemplate:
    spec:
      containers:
        - name: flink-main-container
          volumeMounts:
            - name: checkpoint-storage
              mountPath: /opt/flink/checkpoints
      volumes:
        - name: checkpoint-storage
          persistentVolumeClaim:
            claimName: flink-checkpoint-pvc
  job:
    jarURI: local:///opt/flink/usrlib/realtime-etl.jar
    parallelism: 20
    upgradeMode: savepoint
    state: running
    args:
      - --kafka-brokers
      - kafka-0.kafka:9092
      - --hologres-endpoint
      - holo-cn-hangzhou.aliyuncs.com
      - --checkpoint-interval
      - "60000"
---
# Spark on K8s 离线任务
apiVersion: sparkoperator.k8s.io/v1beta2
kind: SparkApplication
metadata:
  name: daily-aggregation-job
  namespace: data-platform
spec:
  type: Scala
  mode: cluster
  image: registry.cn-hangzhou.aliyuncs.com/spark/spark:v3.4.0
  mainClass: com.example.DailyAggregation
  mainApplicationFile: local:///opt/spark/jobs/daily-aggregation.jar
  sparkVersion: "3.4.0"
  driver:
    cores: 2
    memory: "4G"
    serviceAccount: spark-driver
  executor:
    cores: 4
    instances: 10
    memory: "8G"
  arguments:
    - --date
    - "2026-04-24"
    - --output-path
    - "oss://data-warehouse/dws/daily/"
```

---

## 参考链接

- [阿里云 DataWorks](https://www.aliyun.com/product/bigdata/ide)
- [阿里云 MaxCompute](https://www.aliyun.com/product/odps)
- [阿里云 Hologres](https://www.aliyun.com/product/hologres)
- [阿里云 PAI](https://www.aliyun.com/product/bigdata/learn)
