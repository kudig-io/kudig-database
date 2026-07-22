---
title: InterLink (entities)
description: '## 概述'
summary: 'InterLink 是一个 Virtual Kubelet 提供者实现，允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。'
category: entities
tags:
- k8s
- cncf
- edge
- interlink
- kubelet
- prometheus
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- InterLink 是什么
- 如何 InterLink
trigger_keywords:
- InterLink
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# InterLink

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Go

## 概述

InterLink 是一个 Virtual Kubelet 提供者实现，由 CS Italy（意大利国家核物理研究院）等科研机构推动，2023 年加入 CNCF 沙箱。它允许将 Kubernetes Pod 调度到远程 HPC（高性能计算）和云计算基础设施上执行。InterLink 通过标准的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 将传统 HPC 集群（Slurm、HTCondor）和云计算平台作为 Kubernetes 的扩展计算资源，使科研人员和工程师能够使用熟悉的 Kubernetes 工作流提交和管理 HPC 作业。InterLink 会在远程 HPC 节点上将容器转换为 Singularity/Apptainer 格式运行，解决了 HPC 环境不支持 Docker/root 权限的限制。

## 核心能力

- **Virtual Kubelet 集成**: 将远程 HPC/云资源注册为 Kubernetes 虚拟节点
- **多调度器支持**: Slurm、HTCondor、OpenStack、AWS Batch 等后端
- **容器转换**: 自动将 OCI 容器镜像转换为 Singularity/Apptainer 格式（HPC 兼容）
- **Pod 生命周期管理**: 将 Kubernetes Pod 状态映射到 HPC 作业状态
- **数据管理**: 支持将 PVC 挂载到 HPC 共享文件系统
- **资源映射**: 虚拟节点容量反映 HPC 集群实际可用资源

## 架构

InterLink 架构分为云端控制面和边缘执行面两层：

- **InterLink Controller**: 部署在 Kubernetes 控制面集群，实现 Virtual Kubelet Provider 接口
- **Virtual Node**: 在 Kubernetes 中注册的虚拟节点，代表远程 HPC 集群
- **InterLinker (Tunnel)**: 建立云到 HPC 集群的安全隧道连接
- **Sidecar (HPC 侧)**: 部署在 HPC 登录节点，接收 Pod 定义并转换为 Slurm 脚本
- **Singularity Runtime**: 在 HPC 计算节点上以 Singularity/Apptainer 方式运行容器

执行流程：`Kubernetes Pod → InterLink (Virtual Kubelet) → Slurm 脚本 → HPC 计算节点 → Singularity 容器`

## K8s 集成

InterLink 通过 Virtual Kubelet 框架与 Kubernetes 深度集成。它在集群中注册一个或多个虚拟节点，节点的 capacity 反映远程 HPC 集群的可用资源。当 Pod 被调度到虚拟节点时，InterLink Controller 将 Pod spec 转换为 HPC 作业脚本（如 Slurm SBATCH），通过 SSH/API 提交到 HPC 调度器。Pod 状态（Pending → Running → Succeeded/Failed）映射到 HPC 作业状态。支持通过 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 nodeSelector、tolerations 和 affinity 控制哪些 Pod 被调度到 HPC。

## 生产场景

1. **HPC 作业管理**: 科研人员通过 kubectl 提交 Slurm 作业，无需学习 Slurm 语法
2. **混合计算池**: 将本地 Kubernetes 集群与远程超算中心组成统一计算池
3. **大规模 AI 训练**: 将 GPU 密集型训练任务调度到 HPC 集群的 GPU 节点
4. **弹性计算扩展**: Kubernetes 集群资源不足时，自动将作业溢出到 HPC/云

## 安装与配置

```bash
# 安装 Virtual Kubelet 和 InterLink
helm repo add interlink https://intertwin-eu.github.io/interLink/
helm install interlink interlink/interlink -n interlink-system --create-namespace \
  --set virtualKubelet.image.tag=latest

# 配置 HPC 后端连接
kubectl create secret generic hpc-credentials \
  --from-file=ssh-key=$HOME/.ssh/id_rsa \
  --from-literal=hpc-user=sci-user \
  --from-literal=hpc-host=hpc-login.cluster.edu \
  -n interlink-system

# 等待虚拟节点就绪
kubectl get nodes | grep virtual
kubectl describe node hpc-cluster
```

```yaml
# InterLink 配置示例（Slurm 后端）
apiVersion: interlink.io/v1alpha1
kind: InterLinkConfig
metadata:
  name: hpc-slurm
  namespace: interlink-system
spec:
  hpc:
    type: slurm
    loginNode: hpc-login.cluster.edu
    user: sci-user
    sshKeySecret: hpc-credentials
    workDir: /scratch/{username}/interlink
  containerRuntime:
    type: singularity
    version: "3.11"
  resources:
    cpu: "1024"
    memory: "4096Gi"
    nvidia.com/gpu: "64"
---
# 提交到 HPC 的 Pod 示例
apiVersion: v1
kind: Pod
metadata:
  name: ml-training-job
  labels:
    hpc-workload: "true"
spec:
  nodeSelector:
    kubernetes.io/hostname: hpc-cluster
  tolerations:
  - key: hpc
    operator: Exists
    effect: NoSchedule
  containers:
  - name: training
    image: pytorch/pytorch:2.1-cuda12.1
    command: ["python", "train.py", "--epochs=100"]
    resources:
      limits:
        cpu: "32"
        memory: "128Gi"
        nvidia.com/gpu: "4"
  restartPolicy: Never
```

## 运维操作

```bash
# 🟢 查看虚拟节点状态
kubectl get nodes -l type=virtual-kubelet
kubectl describe node hpc-cluster

# 🟢 查看 HPC 作业状态
kubectl get pods --field-selector spec.nodeName=hpc-cluster
kubectl logs ml-training-job  # 获取 HPC 作业输出

# 🟡 取消 HPC 作业（删除 Pod）
kubectl delete pod ml-training-job --grace-period=0

# 🟢 查看 InterLink 控制器日志
kubectl logs -n interlink-system -l app=interlink-controller --tail=50

# 🟡 重启 InterLink 连接（隧道断开时）
kubectl rollout restart deployment/interlink-controller -n interlink-system

# 🔴 删除虚拟节点（断开 HPC 连接）
kubectl delete node hpc-cluster
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 虚拟节点 NotReady | SSH 连接失败或隧道断开 | `kubectl describe node hpc-cluster` | 检查 SSH 密钥和网络连通性 |
| Pod 长时间 Pending | HPC 队列排队或资源不足 | `kubectl describe pod <name>` | 检查 HPC 队列状态 (squeue) |
| 容器启动失败 | Singularity 转换失败 | `kubectl logs <pod>` | 检查镜像兼容性和 Singularity 版本 |
| 日志无法获取 | HPC 作业输出路径错误 | `kubectl exec interlink-controller -- ls /scratch` | 检查 workDir 配置 |
| GPU 分配失败 | HPC 节点 GPU 资源不足 | `kubectl describe node hpc-cluster` | 检查 HPC GPU 队列可用性 |

```
排查流程：
├── 虚拟节点异常
│   ├── kubectl describe node 查看 conditions
│   ├── 检查 SSH 连接: ssh -i key user@hpc-login
│   ├── 查看 InterLink controller 日志
│   └── 确认防火墙允许 SSH 端口
├── 作业执行失败
│   ├── kubectl logs 查看作业输出
│   ├── 检查 Singularity 镜像转换日志
│   ├── 确认 HPC 队列配置正确
│   └── 检查共享文件系统权限
└── 资源映射问题
    ├── 确认虚拟节点 capacity 反映 HPC 实际资源
    ├── 检查 Pod resources 是否超过 HPC 可用量
    └── 确认 GPU 驱动和 CUDA 版本兼容
```

## 生产案例

### 案例 1：科研机构统一计算平台

- **场景**：大学物理系研究人员需要同时使用本地 K8s 集群和国家级超算中心，学习两套调度系统成本高
- **排查**：研究人员需要学习 Slurm SBATCH 语法，作业提交错误率高，本地集群和 HPC 资源无法统一视图
- **方案**：部署 InterLink 将 Slurm 集群注册为 K8s 虚拟节点，研究人员用 kubectl 提交作业
- **效果**：作业提交错误率降低 80%，资源利用率提升 30%，新研究人员上手时间从 2 周降至 1 天

### 案例 2：AI 训练弹性溢出到 HPC

- **场景**：AI 团队本地 GPU 集群资源不足，训练任务排队等待，但合作 HPC 中心有大量空闲 GPU
- **排查**：本地 8 张 A100 满载，训练任务排队 3 天，而 HPC 中心 GPU 利用率仅 40%
- **方案**：InterLink 连接 HPC GPU 队列，通过 nodeSelector 将大模型训练任务调度到 HPC
- **效果**：训练任务等待时间从 3 天降至 2 小时，GPU 资源成本降低 50%

## 对比

| 特性 | InterLink | Virtual Kubelet (ACI) | Kueue | 适用场景 |
|------|-----------|-----------------------|-------|----------|
| HPC 支持 | ✅ Slurm/HTCondor | ❌ 仅 ACI | ⚠️ 有限 | 科研 HPC |
| 容器转换 | ✅ Singularity | ❌ | ❌ | HPC 无 root 环境 |
| 批处理调度 | ✅ | ❌ | ✅ | 大规模并行计算 |
| 云原生集成 | 中 | 高 | 高 | 混合计算池 |

## 架构定位

在 CNCF 生态中，InterLink 属于 **Edge** 类别，为云原生应用提供 HPC 计算资源集成能力。

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]
- [[实体/kubelet.md|kubelet]]

## Related

- [[实体/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[virtual-kubelet]] — Virtual Kubelet

- interlink
- [[实体/akri.md|Akri]]
- [[实体/openyurt.md|OpenYurt]]
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
