---
title: Vineyard
description: '## 概述'
summary: 'Vineyard 是一个内存中的不可变数据管理器，为大数据和 AI/ML 工作流提供零拷贝数据共享。它通过共享内存机制在同一节点上的不同计算引擎（如 Spark、PyTorch、Dask、GraphScope）之间实现高效数据传递，避免了传统方式中序列化/反序列化和磁盘 IO 的开销，可将数据流水线的端到端性能提升数倍。'
category: entities
tags:
- k8s
- cncf
- data
- vineyard
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Vineyard 是什么
- 如何 Vineyard
trigger_keywords:
- Vineyard
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Vineyard

> **CNCF 状态**: Sandbox | **类别**: Data | **主要语言**: C++, Python

## 概述

Vineyard（葡萄藤）是一个 CNCF 沙箱项目，由阿里巴巴和北京大学联合开发，是一个面向数据密集型计算的共享内存对象存储系统。它专门解决分布式计算环境中大数据对象在不同进程和任务间的高效共享问题。Vineyard 通过零拷贝的共享内存机制，避免了 Python 对象在进程间传输时的序列化/反序列化开销，特别适合大规模机器学习和图计算场景。

## Key Features（核心能力）

- **共享内存对象存储**：基于 IPC 共享内存实现进程间零拷贝数据共享
- **Python 原生集成**：支持 NumPy、pandas、PyTorch Tensor 等常用数据结构
- **分布式架构**：跨节点的对象管理和迁移
- **K8s 集成**：通过 Vineyard Operator 管理 K8s 上的分布式 Vineyard 实例
- **Plasma 兼容**：与 Apache Arrow Plasma 格式兼容
- **多种后端支持**：可将对象溢出到磁盘或对象存储

## 架构与工作原理

Vineyard 架构包含 Vineyardd（守护进程，管理每个节点上的共享内存段）、IPC 层（本地进程通过 Unix Domain Socket 访问共享内存）、RPC 层（跨节点通信和对象迁移）。对象以 Blob（二进制数据块）和 Meta（元数据描述）两级结构组织。Vineyard Operator 在 K8s 上以 DaemonSet 方式部署 Vineyardd 到每个计算节点，为 Pod 提供共享内存卷。

## K8s 集成

Vineyard 通过 Vineyard Operator 与 Kubernetes 集成。Operator 以 DaemonSet 方式在每个计算节点部署 vineyardd 守护进程。计算 Pod 通过 Device Plugin 挂载 Vineyard 共享内存段。Vineyard CRD 定义集群配置和对象恢复策略。与 Dask、Ray、Kubeflow 等分布式计算框架集成时，Vineyard 作为中间数据存储层加速计算任务间的数据交换。

## 生产用例

- **分布式 ML 训练**：训练任务间共享大型数据集和模型参数
- **图计算**：大规模图数据的跨进程高效共享
- **数据管道**：ETL 流水线中数据处理任务间的数据传递
- **科学计算**：大规模数值模拟数据的实时分析

## 安装与配置

```bash
# 🟢 Python SDK 安装
pip install vineyard

# 🟢 K8s Operator 部署
helm repo add vineyard https://vineyard.oss-ap-southeast-1.aliyuncs.com/charts/
helm repo update
helm install vineyardd vineyard/vineyardd \
  -n vineyard-system --create-namespace \
  --set vineyardd.socket=/var/run/vineyard.sock \
  --set vineyardd.size=10Gi

# 🟢 验证安装
kubectl get pods -n vineyard-system
kubectl get crd | grep vineyard

# 🟢 本地快速体验
python3 -c "
import vineyard
import numpy as np

# 启动本地 vineyardd
client = vineyard.connect('/var/run/vineyard.sock')

# 存储对象
data = np.random.rand(1000, 1000)
object_id = client.put(data)
print(f'Stored object: {object_id}')

# 零拷贝读取
result = client.get(object_id)
print(f'Retrieved shape: {result.shape}')
"
```

### Vineyard Operator CRD 示例

```yaml
apiVersion: k8s.v6d.io/v1alpha1
kind: Vineyardd
metadata:
  name: vineyardd-cluster
  namespace: vineyard-system
spec:
  vineyard:
    image: vineyardcloudnative/vineyardd:latest
    socket: /var/run/vineyard.sock
    size: 20Gi
    streamThreshold: 80
  etcd:
    image: bitnami/etcd:3.5
    replicas: 3
  metric:
    enable: true
    port: 9600
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ml-training
  annotations:
    # 启用 Vineyard Sidecar 注入
    vineyard.io/enable: "true"
spec:
  replicas: 4
  selector:
    matchLabels:
      app: ml-training
  template:
    metadata:
      labels:
        app: ml-training
      annotations:
        vineyard.io/enable: "true"
        vineyard.io/vineyardd: vineyardd-cluster
    spec:
      containers:
        - name: trainer
          image: myorg/ml-trainer:v1
          env:
            - name: VINEYARD_SOCKET
              value: /var/run/vineyard.sock
          volumeMounts:
            - name: vineyard-sock
              mountPath: /var/run/vineyard.sock
      volumes:
        - name: vineyard-sock
          hostPath:
            path: /var/run/vineyard.sock
```

## 运维操作

```bash
# 🟢 查看 Vineyard 集群状态
kubectl get vineyardd -A
kubectl get pods -n vineyard-system

# 🟢 查看存储对象列表
kubectl exec -n vineyard-system deploy/vineyardd -- vineyard-ctl ls

# 🟢 查看内存使用情况
kubectl exec -n vineyard-system deploy/vineyardd -- vineyard-ctl stat

# 🟡 清理过期对象
kubectl exec -n vineyard-system deploy/vineyardd -- vineyard-ctl del --all-expired

# 🟡 调整共享内存大小
kubectl edit vineyardd vineyardd-cluster -n vineyard-system
# 修改 spec.vineyard.size

# 🔴 重启 Vineyard 集群（会丢失内存中所有对象）
kubectl rollout restart daemonset/vineyardd -n vineyard-system
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| 共享内存不足 | 对象累积未清理 | `vineyard-ctl stat` | 清理过期对象或增大 size |
| Pod 无法连接 Vineyard | Socket 文件不存在 | `ls /var/run/vineyard.sock` | 检查 DaemonSet 状态 |
| 跨节点对象访问失败 | etcd 连接问题 | `kubectl get pods -n vineyard-system` | 检查 etcd 集群健康 |
| 数据丢失 | Vineyardd 重启 | 查看 Pod 重启次数 | 配置对象持久化后端 |

```bash
# 排查流程
# 1. 检查 Vineyardd DaemonSet 状态
kubectl get daemonset -n vineyard-system
kubectl get pods -n vineyard-system -o wide

# 2. 检查共享内存使用
df -h /dev/shm
kubectl exec -n vineyard-system ds/vineyardd -- vineyard-ctl stat

# 3. 检查 etcd 连接
kubectl logs -n vineyard-system -l app=etcd --tail=50

# 4. 检查 Pod 挂载
kubectl describe pod <ml-pod> | grep -A5 vineyard
```

## 生产案例

### 案例1：分布式 ML 训练数据共享
- **场景**：大规模图像分类训练，多个 Worker Pod 需要共享预处理后的数据集（500GB）
- **方案**：Vineyard DaemonSet 部署在每个 GPU 节点；数据预处理 Pod 将结果存入 Vineyard；训练 Pod 零拷贝读取，避免重复 I/O
- **效果**：数据加载时间从 20min 降到 30s，训练吐吐量提升 3x

### 案例2：图计算流水线加速
- **场景**：社交网络分析，Spark ETL + GraphScope 图计算的多阶段流水线
- **方案**：Spark 输出直接写入 Vineyard；GraphScope 从 Vineyard 零拷贝读取图数据；避免中间结果序列化到 HDFS
- **效果**：端到端流水线时间从 2小时 缩短到 25分钟

## 对比替代方案

| 维度 | Vineyard | Redis | Apache Plasma | 共享文件系统 |
|------|----------|-------|--------------|------------|
| 零拷贝 | 是 | 否 | 是 | 否 |
| 分布式 | 支持 | 支持 | 无 | 支持 |
| K8s 集成 | Operator | 无 | 无 | CSI |
| 大数据对象 | 专为设计 | 小对象 | 中 | 中 |
| 序列化开销 | 无 | 高 | 无 | 高 |

## 检查清单

- [ ] Vineyard Operator 已部署且 Pod Running
- [ ] Vineyardd DaemonSet 已在所有计算节点运行
- [ ] 共享内存大小已根据数据集调整
- [ ] 计算 Pod 已正确挂载 Vineyard Socket
- [ ] etcd 集群健康（分布式模式）
- [ ] 对象过期清理策略已配置
- [ ] 内存使用监控已配置

## Related

- [[hami]] — HAMI
- [[open-policy-containers]] — [[23-实体/06-安全/open-policy-containers.md|Open Policy Containers (OPCR)]]
- [[werf]] — werf
- [[dalec]] — Dalec
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vineyard
- storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
