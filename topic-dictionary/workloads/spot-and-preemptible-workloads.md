# Spot 与可抢占工作负载

## 概述

在云原生环境中，**Spot 实例（AWS）、Preemptible VM（GCP）和 Low-priority VM（Azure）** 是云厂商以大幅折扣出售的闲置计算容量。2026 年的最佳实践表明，通过将**容错型工作负载**（如 AI 训练、批处理、CI/CD）部署到 Spot 实例上，企业可将计算成本降低 **50%–90%**。Kubernetes 结合 Kueue、Cluster Autoscaler 和 checkpoint 机制，已能安全、自动化地管理可抢占工作负载的生命周期。

## 核心概念/原理

### 1. 可抢占实例的工作机制

云厂商在需要容量时会提前通知（通常提前 **30 秒–2 分钟**）并回收 Spot 实例：
- **AWS Spot Instances**：提供最多 2 分钟的终止通知（ITN, Instance Termination Notice）
- **GCP Preemptible VMs**：提供 30 秒的抢占通知
- **Azure Low-priority VMs**：可被随时回收

Kubernetes 中的 Pod 会收到 `SIGTERM` 信号，随后进入优雅终止期（默认 30 秒），超时后收到 `SIGKILL`。

### 2. Kubernetes 对中断的响应

Kubernetes 提供了多个机制配合 Spot 实例：
- **Pod Disruption Budget（PDB）**：控制同时中断的 Pod 数量，保护有状态服务
- **Node Termination Handler**：部署为 DaemonSet，监听 IMDS（Instance Metadata Service）获取中断通知，提前将节点标记为 `NoSchedule` 并驱逐 Pod
- **Cluster Autoscaler / Karpenter**：在 Spot 实例被回收后，自动在其他可用区或实例类型上补充新节点

### 3. Checkpoint 与容错设计

将工作负载迁移到 Spot 的前提是应用必须具备**中断恢复能力**：
- **AI 训练**：定期保存模型权重和优化器状态（如每 N 个 epoch 或每 X 分钟）
- **批处理作业**：将中间结果写入对象存储（S3），支持断点续传
- **CI/CD Pipeline**：将构建产物和缓存持久化到外部存储
- **数据处理**：使用支持 exactly-once 语义的消息队列（如 Kafka）保证数据处理不丢失

### 4. Spot 与按需实例的混合架构

2026 年的最佳实践推荐**混合节点池（Mixed Instance Pools）**：
- **按需/Reserved 实例**：运行核心控制平面、有状态数据库、关键在线服务
- **Spot 实例**：运行训练任务、大数据批处理、开发测试环境、无状态 Worker
- **可回退策略**：当 Spot 容量不足时，自动回退到按需实例（AWS Spot Fleet / Karpenter 支持）

## 关键机制或特性

### Node Termination Handler 工作流程

```
1. 云厂商发出 Spot 中断通知
        ↓
2. Node Termination Handler 检测到 IMDS 信号
        ↓
3. 立即对节点执行 `cordon`（禁止新 Pod 调度）
        ↓
4. 发起 Pod 驱逐（Eviction），给应用 30 秒优雅关闭时间
        ↓
5. 应用执行 checkpoint 并保存状态
        ↓
6. Pod 被重新调度到其他节点（Spot 或 On-Demand）
        ↓
7. 应用从 checkpoint 恢复，继续执行
```

### Kueue 与 Spot 队列

结合 **Kueue** 可实现更精细的 Spot 工作负载管理：
- 为 Spot 实例定义独立的 `ResourceFlavor`
- 只有标记为 `spot-tolerant` 的作业才能进入 Spot 队列
- 当 Spot 容量不足时，Kueue 自动将作业保持在队列中等待，或路由到按需队列

### Spot 实例多样化策略

通过 **Karpenter** 或 **Cluster Autoscaler（Mixed Instance Policy）** 配置多种实例类型：
- 不同实例族（如 m6i、m5、m4）
- 不同可用区
- 这显著提高了获取 Spot 容量的概率

## 使用场景

1. **大模型分布式训练**：在 100+ Spot GPU 节点上训练 LLM，每 15 分钟 checkpoint 一次，中断后自动恢复
2. ** nightly 数据仓库 ETL**：使用 Spot 实例运行数小时的 Spark/Flink 批处理任务
3. **CI/CD 构建农场**：将无状态的编译、测试任务调度到 Spot 实例，大幅降低 DevOps 基础设施成本
4. **渲染农场**：影视特效的分布式渲染任务天然适合 Spot 实例的短时、可中断特性
5. **开发/测试环境**：开发集群全天候运行成本高，使用 Spot 实例配合定时启停策略可节省 70%+

## 最佳实践/注意事项

- **Checkpoint 频率是关键**：Spot 实例可能每小时被中断多次，checkpoint 间隔应控制在 5–15 分钟
- **最小化启动时间**：使用预置镜像和容器缓存，确保 Pod 在迁移后能快速恢复运行
- **PDB 不适用于纯 Spot 批处理**：批处理任务通常不需要 PDB，但混合部署时需注意不要将有状态服务调度到 Spot 节点
- **节点亲和性/反亲和性**：使用 `nodeAffinity` 或污点（Taints）明确区分 Spot 节点和按需节点
- **监控中断率和恢复时间**：核心指标包括 Spot 中断频率、checkpoint 成功率、任务总完成时间（含恢复）
- **存储必须持久化**：Spot 实例的本地磁盘会随实例终止而丢失，所有重要数据必须写入 PVC 或对象存储
- **避免跨可用区通信**：Spot 节点分散在不同 AZ 时，注意控制平面和数据传输的跨 AZ 带宽成本
- **成本建模**：不仅比较实例单价，还需考虑因中断导致的重复计算成本和额外存储开销

## 参考链接

- [AWS Spot Instances Best Practices](https://docs.aws精华剃须刀.com/AWSEC2/latest/UserGuide/spot-best-practices.html)
- [GCP Preemptible VMs](https://cloud.google.com/compute/docs/instances/preemptible)
- [Azure Spot Virtual Machines](https://docs.microsoft.com/en-us/azure/virtual-machines/spot-vms)
- [AWS Node Termination Handler](https://github.com/aws/aws-node-termination-handler)
- [Karpenter Documentation](https://karpenter.sh/docs/)
- [CIO - Kubernetes GPU Utilization Best Practices](https://www.cio.com/article/4152554/how-kubernetes-is-finally-solving-the-gpu-utilization-crisis-to-save-your-ai-budget.html)
