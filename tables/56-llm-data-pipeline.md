# 56 - LLM训练数据Pipeline与管理

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Ray Data](https://docs.ray.io/en/latest/data/data.html)

## 一、数据Pipeline组件

| 组件 | 用途 | K8s集成 | 适用场景 |
|-----|------|--------|---------|
| **Ray Data** | 分布式数据处理 | Ray Operator | ML数据预处理 |
| **Spark** | 批处理 | Spark Operator | 结构化数据 |
| **Flink** | 流处理 | Flink Operator | 实时数据流 |
| **Alluxio** | 分布式缓存 | DaemonSet | 训练加速 |

## 二、数据格式

| 格式 | 压缩率 | 速度 | 场景 |
|-----|-------|------|------|
| **Parquet** | 高 | 快 | 表格数据 |
| **WebDataset** | 高 | 极快 | 图像/视频 |
| **JSONL** | 低 | 慢 | LLM文本 |

## 三、Ray Data部署

```yaml
apiVersion: ray.io/v1alpha1
kind: RayCluster
metadata:
  name: ray-data-cluster
spec:
  rayVersion: '2.9.0'
  workerGroupSpecs:
  - replicas: 10
    minReplicas: 5
    maxReplicas: 20
```

## 四、存储优化

**S3生命周期:**
- 7天 → Standard-IA (节省45%)
- 30天 → Glacier (节省82%)

## 五、最佳实践

1. **分区**: 按日期，每分区100-500MB
2. **压缩**: Parquet + Snappy
3. **并行度**: Workers = 2-3×CPU核心
4. **缓存**: Alluxio缓存热数据

---
**相关**: [115-AI数据Pipeline](../115-ai-data-pipeline.md) | **版本**: Ray 2.9.0+, K8s v1.27+
