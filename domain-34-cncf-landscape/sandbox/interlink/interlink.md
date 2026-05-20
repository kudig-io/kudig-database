---
title: InterLink
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- helm
- docker
- job
- gpu
- nvidia
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- InterLink 是什么
- 如何 InterLink
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- InterLink
- cncf
- landscape
---

# InterLink

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://intertwin-eu.github.io/interLink/ |
| **GitHub** | https://github.com/interTwin-eu/interLink |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

InterLink 是一个 Virtual Kubelet 提供者实现，允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。它通过标准的 Kubernetes API 将传统 HPC 集群（Slurm、HTCondor）和云计算平台作为 Kubernetes 的扩展计算资源，使科研人员和工程师能够使用熟悉的 Kubernetes 工作流提交和管理 HPC 作业。

### 核心特性

- **Virtual Kubelet Provider**: 将外部计算资源呈现为 Kubernetes 虚拟节点
- **HPC 集群集成**: 原生支持 Slurm 和 HTCondor 作业调度系统
- **云计算扩展**: 可对接远程云计算资源池
- **透明调度**: 用户通过标准 Pod spec 提交作业，无需感知底层 HPC 系统
- **Sidecar 模式**: 支持 Init Container 和 Sidecar，保持 Kubernetes 工作负载语义
- **数据管理**: 自动处理数据在 Kubernetes 集群和 HPC 系统间的传输

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│              Kubernetes Cluster                       │
│                                                       │
│  ┌───────────────┐     ┌───────────────────────────┐ │
│  │  Real Node     │     │  Virtual Node (InterLink) │ │
│  │  ┌──────────┐ │     │  ┌─────────────────────┐  │ │
│  │  │  Pod A   │ │     │  │  Virtual Kubelet    │  │ │
│  │  │  Pod B   │ │     │  │  (InterLink Plugin) │  │ │
│  │  └──────────┘ │     │  └──────────┬──────────┘  │ │
│  └───────────────┘     └─────────────┼─────────────┘ │
└──────────────────────────────────────┼───────────────┘
                                       │ REST API
                              ┌────────▼────────┐
                              │  InterLink API   │
                              │  Server          │
                              └────────┬─────────┘
                                       │
                    ┌──────────────────┼──────────────────┐
                    │                  │                   │
             ┌──────▼──────┐   ┌──────▼──────┐   ┌───────▼─────┐
             │  Slurm       │   │  HTCondor   │   │  Cloud      │
             │  Plugin      │   │  Plugin     │   │  Plugin     │
             └──────┬───────┘   └──────┬──────┘   └──────┬──────┘
                    │                  │                   │
             ┌──────▼───────┐  ┌──────▼──────┐   ┌───────▼─────┐
             │  Slurm HPC   │  │  HTCondor   │   │  Cloud      │
             │  Cluster     │  │  Pool       │   │  Provider   │
             └──────────────┘  └─────────────┘   └─────────────┘
```

---

## 快速开始

### 安装 InterLink

```bash
# 克隆仓库
git clone https://github.com/interTwin-eu/interLink.git
cd interLink

# 构建
make build

# 或使用 Helm 安装
helm repo add interlink https://intertwin-eu.github.io/interLink/
helm install interlink interlink/interlink \
  --namespace interlink-system \
  --create-namespace
```

### 配置 Virtual Kubelet

```yaml
# interlink-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: interlink-config
  namespace: interlink-system
data:
  InterLinkURL: "http://interlink-api:3000"
  VKTokenFile: "/opt/interlink/token"
  # 虚拟节点配置
  NodeName: "hpc-slurm-node"
  NodeCPU: "1000"
  NodeMemory: "4Ti"
  NodePods: "10000"
  # Slurm 配置
  SlurmPartition: "gpu"
  SlurmAccount: "research"
```

### 部署 Virtual Node

```yaml
# virtual-node.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: interlink-vk
  namespace: interlink-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: interlink-vk
  template:
    metadata:
      labels:
        app: interlink-vk
    spec:
      containers:
        - name: virtual-kubelet
          image: ghcr.io/intertwin-eu/interlink/virtual-kubelet:latest
          args:
            - --nodename=hpc-slurm-node
            - --provider=interlink
            - --startup-timeout=30s
          env:
            - name: KUBECONFIG
              value: /etc/kubernetes/kubeconfig
          volumeMounts:
            - name: config
              mountPath: /etc/interlink
      volumes:
        - name: config
          configMap:
            name: interlink-config
```

### 提交 HPC 作业

```yaml
# hpc-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: ml-training-job
spec:
  template:
    metadata:
      annotations:
        # Slurm 特有注解
        slurm.interlink/partition: "gpu"
        slurm.interlink/gres: "gpu:4"
        slurm.interlink/time: "24:00:00"
    spec:
      nodeSelector:
        kubernetes.io/hostname: hpc-slurm-node  # 调度到虚拟节点
      containers:
        - name: training
          image: pytorch/pytorch:latest
          command: ["python", "train.py"]
          resources:
            requests:
              cpu: 32
              memory: 128Gi
              nvidia.com/gpu: 4
      restartPolicy: Never
```

---

## 高级功能

### Slurm Plugin 配置

```yaml
# slurm-plugin-config.yaml
SlurmConfig:
  # Slurm 集群连接
  SshHost: "login.hpc.example.com"
  SshPort: 22
  SshUser: "interlink"
  SshKeyFile: "/etc/interlink/ssh-key"

  # 作业默认参数
  DefaultPartition: "compute"
  DefaultAccount: "cloud-burst"
  DefaultQOS: "normal"

  # 数据传输
  DataRootPath: "/scratch/interlink/data"
  SingularityCachePath: "/scratch/interlink/singularity"

  # 容器运行时 (Singularity/Apptainer)
  ContainerRuntime: "singularity"
  SingularityPrefix: "docker://"
```

### HTCondor Plugin 配置

```yaml
# htcondor-plugin-config.yaml
HTCondorConfig:
  CondorSubmitHost: "submit.htcondor.example.com"
  CondorPool: "cm.htcondor.example.com"
  DefaultUniverse: "docker"
  DataTransferMechanism: "http"
  SubmitDirectory: "/var/interlink/condor-submit"
```

### 多集群 HPC 联邦

```yaml
# 部署多个虚拟节点，对接不同 HPC 集群
apiVersion: v1
kind: ConfigMap
metadata:
  name: interlink-gpu-cluster
data:
  NodeName: "hpc-gpu-cluster"
  SlurmPartition: "gpu"
  NodeCPU: "2000"
  NodeMemory: "8Ti"
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: interlink-cpu-cluster
data:
  NodeName: "hpc-cpu-cluster"
  SlurmPartition: "cpu"
  NodeCPU: "10000"
  NodeMemory: "40Ti"
```

---

## 与其他方案对比

| 特性 | InterLink | Virtual Kubelet | Admiralty | Liqo |
|:---|:---|:---|:---|:---|
| HPC 集成 | Slurm/HTCondor 原生 | 需自定义 | 不支持 | 不支持 |
| 容器运行时 | Singularity/Docker | 取决于 Provider | Docker | Docker |
| 数据传输 | 自动处理 | 需自定义 | K8s 原生 | K8s 原生 |
| 科研场景 | 专为科研设计 | 通用框架 | 多集群调度 | 多集群联邦 |
| GPU 支持 | Slurm GRES 原生 | 取决于 Provider | K8s 原生 | K8s 原生 |

---

## 最佳实践

1. **资源映射**: 合理配置虚拟节点容量，反映 HPC 集群的实际可用资源
2. **数据预置**: 对于大型数据集，预先将数据放置到 HPC 共享文件系统，避免运行时传输
3. **容器镜像**: 使用 Singularity/Apptainer 兼容的容器镜像，确保 HPC 环境兼容性
4. **超时设置**: 根据 HPC 队列等待时间调整 Pod 超时阈值
5. **监控**: 配置对虚拟节点状态的监控，及时发现 HPC 集群连接异常

---

## 参考资源

- [InterLink 官方文档](https://intertwin-eu.github.io/interLink/)
- [InterLink GitHub](https://github.com/interTwin-eu/interLink)
- [Virtual Kubelet 项目](https://github.com/virtual-kubelet/virtual-kubelet)
- [interTwin 项目](https://www.intertwin.eu/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
