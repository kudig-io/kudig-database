# 25 - AI/ML 工作负载运维表 (AI/ML Workloads Ops)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Kubeflow Training Operator](https://www.kubeflow.org/docs/components/training/)

## AI 工作负载生命周期 (Lifecycle)

| 阶段 (Phase) | 核心任务 (Core Tasks) | 资源需求 (Requirements) | 关键组件 (Components) |
|-------------|-----------------------|------------------------|---------------------------|
| **数据处理** | ETL, 格式转换, 特征工程 | 高 I/O, 中 CPU | Spark-on-K8s, Argo, Ray |
| **分布式训练** | 多机多卡训练, 梯度同步 | 大量 GPU, RDMA, 高 IOPS | PyTorchJob, TFJob (Kubeflow) |
| **模型微调** | LoRA/QLoRA 适配 | 单卡或中等 GPU 集群 | Kueue, Volcano, Job |
| **模型推理** | 在线服务, 高并发处理 | 稳定 GPU, 显存优化 | vLLM, KServe, Triton |

## 分布式训练配置 (Distributed Training)

### PyTorchJob 生产级示例
```yaml
apiVersion: "kubeflow.org/v1"
kind: "PyTorchJob"
metadata:
  name: "llama3-70b-train"
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
          - name: pytorch
            image: "nvidia/pytorch:24.01"
            resources:
              limits:
                nvidia.com/gpu: 8
    Worker:
      replicas: 4
      template:
        spec:
          containers:
          - name: pytorch
            resources:
              limits:
                nvidia.com/gpu: 8
                rdma/hca: 1  # 开启 RDMA 加速
```

## 存储与网络加速 (Acceleration)

- **网络**: 使用 **Multus CNI** 配置辅助网络接口，启用 **RDMA/InfiniBand** 减少梯度同步延迟。
- **存储**: 使用 **Fluid** 进行数据集预热；Checkpoint 建议存放在 **JuiceFS** 等高性能并行文件系统。


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)